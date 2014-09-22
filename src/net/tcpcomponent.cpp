/******************************************************
 *   FileName: TCPComponent.cpp
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#include "cnet.h"
#include "tcpcomponent.h"
#include "tbtimeutil.h"
#include "../../comlog.h"
#include "databuffer.h"
#include "stats.h"

#define TBNET_MAX_TIME (1ll<<62)

namespace triones
{

TCPComponent::TCPComponent(Transport *owner, Socket *socket, TransProtocol *streamer,
        IServerAdapter *serverAdapter)
		: IOComponent(owner, socket)
{
	_startConnectTime = 0;
	_isServer = false;

	/**connection ****/
	_socket = socket;
	_streamer = streamer;
	_serverAdapter = serverAdapter;
	_defaultPacketHandler = NULL;
	_iocomponent = NULL;
	_queueTimeout = 5000;
	_queueLimit = 50;
	_queueTotalSize = 0;

	/**tcpconnection ****/
	_gotHeader = false;
	_writeFinishClose = false;
	memset(&_packetHeader, 0, sizeof(_packetHeader));
}

//��������
TCPComponent::~TCPComponent()
{
	//��CONNECTION��TCPCONNECTION�����������ŵ�����
	//tcpconnection ������
	//connection����
	disconnect();
	_socket = NULL;
	_iocomponent = NULL;
}

//���ӶϿ��������з��Ͷ����е�packetȫ����ʱ
void TCPComponent::disconnect()
{
	_output_mutex.lock();
	_myQueue.moveTo(&_outputQueue);
	_output_mutex.unlock();
	checkTimeout(TBNET_MAX_TIME);
}

//���ӵ�ָ���Ļ���, isServer: �Ƿ��ʼ��һ����������Connection
bool TCPComponent::init(bool isServer)
{
	_socket->setSoBlocking(false);
	_socket->setSoLinger(false, 0);
	_socket->setReuseAddress(true);
	_socket->setIntOption(SO_KEEPALIVE, 1);
	_socket->setIntOption(SO_SNDBUF, 64000);
	_socket->setIntOption(SO_RCVBUF, 64000);
	_socket->setTcpNoDelay(true);

	if (!isServer)
	{
		if (!socket_connect() && _autoReconn == false)
		{
			return false;
		}
	}
	else
	{
		_state = TRIONES_CONNECTED;
	}

	setServer(isServer);
	_isServer = isServer;

	return true;
}

/*
 * ���ӵ�socket
 */
bool TCPComponent::socket_connect()
{
	if (_state == TRIONES_CONNECTED || _state == TRIONES_CONNECTING)
	{
		return true;
	}
	_socket->setSoBlocking(false);

	if (_socket->connect())
	{
		if (_socketEvent)
		{
			_socketEvent->addEvent(_socket, true, true);
		}
		_state = TRIONES_CONNECTED;

		_startConnectTime = time(NULL);
	}
	else
	{
		int error = Socket::getLastError();

		if (error == EINPROGRESS || error == EWOULDBLOCK)
		{
			_state = TRIONES_CONNECTING;

			if (_socketEvent)
			{
				_socketEvent->addEvent(_socket, true, true);
			}
		}
		else
		{
			_socket->close();
			_state = TRIONES_CLOSED;
			OUT_ERROR(NULL, 0, NULL, "���ӵ� %s ʧ��, %s(%d)", _socket->getAddr().c_str(),
			        strerror(error), error);
			return false;
		}
	}
	return true;
}

void TCPComponent::close()
{
	if (_socket)
	{
		if (_socketEvent)
		{
			_socketEvent->removeEvent(_socket);
		}
		if (_connection && isConnectState())
		{
			//������¼�ת����һ��socketת����ȥ
			setDisconnState();
		}
		_socket->close();

		if (_connection)
		{
			clearInputBuffer(); // clear input buffer after socket closed
		}

		_state = TRIONES_CLOSED;
	}
}

/*
 * �������ݿ�д��ʱ��Transport����
 *
 * @return �Ƿ�ɹ�, true - �ɹ�, false - ʧ�ܡ�
 */
bool TCPComponent::handleWriteEvent()
{
	_lastUseTime = triones::CTimeUtil::getTime();
	bool rc = true;
	if (_state == TRIONES_CONNECTED)
	{
		rc = writeData();
	}
	else if (_state == TRIONES_CONNECTING)
	{
		int error = _socket->getSoError();
		if (error == 0)
		{
			enableWrite(true);
			clearOutputBuffer();
			_state = TRIONES_CONNECTED;
		}
		else
		{
			OUT_ERROR(NULL, 0, NULL, "���ӵ� %s ʧ��: %s(%d)", _socket->getAddr().c_str(),
			        strerror(error), error);
			if (_socketEvent)
			{
				_socketEvent->removeEvent(_socket);
			}
			_socket->close();
			_state = TRIONES_CLOSED;
		}
	}
	return rc;
}

/**
 * �������ݿɶ�ʱ��Transport����
 *
 * @return �Ƿ�ɹ�, true - �ɹ�, false - ʧ�ܡ�
 */
bool TCPComponent::handleReadEvent()
{
	_lastUseTime = triones::CTimeUtil::getTime();
	bool rc = false;
	if (_state == TRIONES_CONNECTED)
	{
		rc = readData();
	}
	return rc;
}

/*
 * ��ʱ���
 *
 * @param    now ��ǰʱ��(��λus)
 */
void TCPComponent::checkTimeout(int64_t now)
{
	// ����Ƿ����ӳ�ʱ
	if (_state == TRIONES_CONNECTING)
	{
		if (_startConnectTime > 0 && _startConnectTime < (now - static_cast<int64_t>(2000000)))
		{ // ���ӳ�ʱ 2 ��
			_state = TRIONES_CLOSED;
			OUT_ERROR(NULL, 0, NULL, "���ӵ� %s ��ʱ.", _socket->getAddr().c_str());
			_socket->shutdown();
		}
	}
	else if (_state == TRIONES_CONNECTED && _isServer == true && _autoReconn == false)
	{ // ���ӵ�ʱ��, ֻ���ڷ�������
		int64_t idle = now - _lastUseTime;
		if (idle > static_cast<int64_t>(900000000))
		{ // ����15min�Ͽ�
			_state = TRIONES_CLOSED;
			OUT_INFO(NULL, 0, NULL, "%s ������: %d (s) ���Ͽ�.", _socket->getAddr().c_str(),
			        (idle / static_cast<int64_t>(1000000)));
			_socket->shutdown();
		}
	}
	// ��ʱ���
	checkTimeout(now);
}

/**** ԭ��connectiong�Ĳ��� *********************/
void TCPComponent::disconnect()
{
	_output_mutex.lock();
	_myQueue.moveTo(&_outputQueue);
	_output_mutex.unlock();
	checkTimeout(TBNET_MAX_TIME);
}

/*
 * ����packet�����Ͷ���
 */
bool TCPComponent::postPacket(Packet *packet, IPacketHandler *packetHandler, void *args,
        bool noblocking)
{
	if (!isConnectState())
	{
		if (_iocomponent == NULL || _iocomponent->isAutoReconn() == false)
		{
			return false;
		}
		else if (_outputQueue.size() > 10)
		{
			return false;
		}
		else
		{
			TCPComponent *ioc = dynamic_cast<TCPComponent*>(_iocomponent);
			bool ret = false;
			if (ioc != NULL)
			{
				_output_mutex.lock();
				ret = ioc->init(false);
				_output_mutex.unlock();
			}
			if (!ret) return false;
		}
	}

//	// �����client, ������queue���ȵ�����
//	_output_mutex.lock();
//	_queueTotalSize = _outputQueue.size() + _channelPool.getUseListCount() + _myQueue.size();
//	if (!_isServer && _queueLimit > 0 && noblocking && _queueTotalSize >= _queueLimit)
//	{
//		_output_mutex.unlock();
//		return false;
//	}
//	_output_mutex.unlock();
//	Channel *channel = NULL;
//	packet->setExpireTime(_queueTimeout);           // ���ó�ʱ
//	if (_streamer->existPacketHeader())
//	{           // ���ڰ�ͷ
//		uint32_t chid = packet->getChannelId();     // ��packet��ȡ
//		if (_isServer)
//		{
//			assert(chid != 0);                      // ����Ϊ��
//		}
//		else
//		{
//			channel = _channelPool.allocChannel();
//
//			// channelû�ҵ���
//			if (channel == NULL)
//			{
//				OUT_ASSERT(NULL, 0, NULL, "����channel����, id: %u", chid);
//				return false;
//			}
//
//			channel->setHandler(packetHandler);
//			channel->setArgs(args);
//			packet->setChannel(channel);            // ���û�ȥ
//		}
//	}
//	_output_mutex.lock();
//	// д�뵽outputqueue��
//	_outputQueue.push(packet);
//	if (_iocomponent != NULL && _outputQueue.size() == 1U)
//	{
//		_iocomponent->enableWrite(true);
//	}
//	_output_mutex.unlock();
//	if (!_isServer && _queueLimit > 0)
//	{
//		_output_mutex.lock();
//		_queueTotalSize = _outputQueue.size() + _channelPool.getUseListCount() + _myQueue.size();
//		if (_queueTotalSize > _queueLimit && noblocking == false)
//		{
//			bool *stop = NULL;
//			if (_iocomponent && _iocomponent->getOwner())
//			{
//				stop = _iocomponent->getOwner()->getStop();
//			}
//			while (_queueTotalSize > _queueLimit && stop && *stop == false)
//			{
//				if (_outputCond.wait(1000) == false)
//				{
//					if (!isConnectState())
//					{
//						break;
//					}
//					_queueTotalSize = _outputQueue.size() + _channelPool.getUseListCount()
//					        + _myQueue.size();
//				}
//			}
//		}
//		_output_mutex.unlock();
//	}

	if (_isServer && _iocomponent)
	{
		_iocomponent->subRef();
	}

	return true;
}

/*
 * handlePacket ����
 */
bool TCPComponent::handlePacket(DataBuffer *input, PacketHeader *header)
{
	//�ͻ��˵ķ���û��ȷ�Ϸ�ʽ������client��server���ǲ���handlerpacket�ķ�ʽ
	if (_iocomponent)
		_iocomponent->addRef();

	Packet *packet;
	return _serverAdapter->handlePacket(this, packet);

//	Packet *packet;
//	IPacketHandler::HPRetCode rc;
//	void *args = NULL;
//	Channel *channel = NULL;
//	IPacketHandler *packetHandler = NULL;
//
//	if (_streamer->existPacketHeader() && !_isServer)
//	{ // ���ڰ�ͷ
//		uint32_t chid = header->_chid;    // ��header��ȡ
//		chid = (chid & 0xFFFFFFF);
//		channel = _channelPool.offerChannel(chid);
//
//		// channelû�ҵ�
//		if (channel == NULL)
//		{
//			input->drainData(header->_dataLen);
//			OUT_ASSERT(NULL, 0, NULL, "û�ҵ�channel, id: %u, %s", chid,
//			        tbsys::CNetUtil::addrToString(getServerId()).c_str());
//			return false;
//		}
//
//		packetHandler = channel->getHandler();
//		args = channel->getArgs();
//	}
//
//	// ����
//	packet = _streamer->decode(input, header);
//	if (packet == NULL)
//	{
//		packet = &ControlPacket::BadPacket;
//	}
//	else
//	{
//		packet->setPacketHeader(header);
//		// ����������, ֱ�ӷ���queue, ����
//		if (_isServer && _serverAdapter->_batchPushPacket)
//		{
//			if (_iocomponent) _iocomponent->addRef();
//			_inputQueue.push(packet);
//			if (_inputQueue.size() >= 15)
//			{ // ����15��packet�͵���һ��
//				_serverAdapter->handleBatchPacket(this, _inputQueue);
//				_inputQueue.clear();
//			}
//			return true;
//		}
//	}
//
//	// ����handler
//	if (_isServer)
//	{
//		if (_iocomponent) _iocomponent->addRef();
//		rc = _serverAdapter->handlePacket(this, packet);
//	}
//	else
//	{
//		if (packetHandler == NULL)
//		{    // ��Ĭ�ϵ�
//			packetHandler = _defaultPacketHandler;
//		}
//		assert(packetHandler != NULL);
//
//		rc = packetHandler->handlePacket(packet, args);
//		channel->setArgs(NULL);
//		// ���ջ����ͷŵ�
//		if (channel)
//		{
//			_channelPool.appendChannel(channel);
//		}
//	}
//
//	return true;
}

///*
// * �ͻ��˵����ӳ�ʱ���
// */
//bool TCPComponent::checkTimeout(int64_t now)
//{
//	// �õ���ʱ��channel��list
//	Channel *list = _channelPool.getTimeoutList(now);
//	Channel *channel = NULL;
//	IPacketHandler *packetHandler = NULL;
//
//	if (list != NULL)
//	{
//		if (!_isServer)
//		{ // client endpoint, ��ÿ��channel��һ����ʱpacket, �������˰�channel����
//			channel = list;
//			while (channel != NULL)
//			{
//				packetHandler = channel->getHandler();
//				if (packetHandler == NULL)
//				{    // ��Ĭ�ϵ�
//					packetHandler = _defaultPacketHandler;
//				}
//				// �ص�
//				if (packetHandler != NULL)
//				{
//					packetHandler->handlePacket(&ControlPacket::TimeoutPacket, channel->getArgs());
//					channel->setArgs(NULL);
//				}
//				channel = channel->getNext();
//			}
//		}
//		// �ӵ�freelist��
//		_channelPool.appendFreeList(list);
//	}
//
//	// ��PacketQueue��ʱ���
//	_output_mutex.lock();
//	Packet *packetList = _outputQueue.getTimeoutList(now);
//	_output_mutex.unlock();
//	while (packetList)
//	{
//		Packet *packet = packetList;
//		packetList = packetList->getNext();
//		channel = packet->getChannel();
//		packet->free();
//		if (channel)
//		{
//			packetHandler = channel->getHandler();
//			if (packetHandler == NULL)
//			{    // ��Ĭ�ϵ�
//				packetHandler = _defaultPacketHandler;
//			}
//			// �ص�
//			if (packetHandler != NULL)
//			{
//				packetHandler->handlePacket(&ControlPacket::TimeoutPacket, channel->getArgs());
//				channel->setArgs(NULL);
//			}
//			_channelPool.freeChannel(channel);
//		}
//	}
//
//	// �����client, ������queue���ȵ�����
//	if (!_isServer && _queueLimit > 0 && _queueTotalSize > _queueLimit)
//	{
//		_output_mutex.lock();
//		_queueTotalSize = _outputQueue.size() + _channelPool.getUseListCount() + _myQueue.size();
//		if (_queueTotalSize <= _queueLimit)
//		{
//			_outputCond.broadcast();
//		}
//		_output_mutex.unlock();
//	}
//
//	return true;
//}

/**
 * ����״̬
 */
bool TCPComponent::isConnectState()
{
	if (_iocomponent != NULL)
	{
		return _iocomponent->isConnectState();
	}
	return false;
}

/********* ԭ��TCPCONNECTION�ĵط�  **************************/
/** ˵�� 2014-09-21 ����  **
 * ��1������ط�Ӧ���޶�д�����������ڵ��̵߳������£�д������������󣬵���������socket������һֱ���ڵȴ�״̬ ��
 * ��������Ĭ�������ֻд10�εġ�
 * ��2������д�¼��г��ֵ��쳣����EPIPE�ȣ�û�������⴦�������Ļ�ֻ��ͨ�����¼����ж�socket�ĶϿ������
 * ��3����packet���л�Ϊ�����ݵķ�ʽ�����Ա��Ŀ���У����е�ͨ���ڲ����ǲ���ͬһ��Э�飬
 * ��ͬһ��Э������˷�װ�����ݴ��䶼��ͨ��packet�ķ�ʽ�����packet�Ǵ���chanelID�ģ��������Ͷ˺ͽ��ն˵�ҵ����ܶ�Ӧ������
 * �������ǵ�ҵ���У�ʵ�ֵ��Ǹ�ͨ�ÿͻ��˵�ͨ�ţ����ֿͻ��˿��ܲ�cnetд�ģ������ߵ�Э�飨transprotocol��
 *  *********/

bool TCPComponent::writeData()
{
	// �� _outputQueue copy�� _myQueue��
	_output_mutex.lock();
	_outputQueue.moveTo(&_myQueue);

	//���socket�е������Ѿ�ȫ����������ˣ��ÿ�д�¼�Ϊfalse��Ȼ���˳���
	if (_myQueue.size() == 0 && _output.getDataLen() == 0)
	{
		_iocomponent->enableWrite(false);
		_output_mutex.unlock();
		return true;
	}
	_output_mutex.unlock();

	Packet *packet;
	int ret;
	int writeCnt = 0;
	int myQueueSize = _myQueue.size();

	do
	{
		// д����
		while (_output.getDataLen() < READ_WRITE_SIZE)
		{
			// ���п��˾��˳�
			if (myQueueSize == 0)
				break;

			packet = _myQueue.pop();
			myQueueSize--;
			_streamer->encode(packet, &_output);
//			_channelPool.setExpireTime(packet->getChannel(), packet->getExpireTime());
			packet->free();
			TBNET_COUNT_PACKET_WRITE(1);
		}

		if (_output.getDataLen() == 0)
		{
			break;
		}

		// write data
		ret = _socket->write(_output.getData(), _output.getDataLen());
		if (ret > 0)
		{
			_output.drainData(ret);
		}
		writeCnt++;
		/*******
		* _output.getDataLen() == 0 ˵�����͵����ݶ�������
		* ֹͣ���͵�������
		* (1)���͵Ľ��ret <= 0, ����ʧ�ܣ�����д�������Ѿ����ˡ�
		* ��2�� _output.getDataLen() > 0 ˵��һ��û�з����꣬����û�з���������ݡ���ֱ���˳���ֹͣ�����ˡ�
		* 	 ��ô�������ȥ�����
		* (3)����myqueue��output��δ����������ݶ�������ȥ��
		**********/
	} while (ret > 0 && _output.getDataLen() == 0 && myQueueSize > 0 && writeCnt < 10);


	// ����
	_output.shrink();

	_output_mutex.lock();
	int queueSize = _outputQueue.size() + _myQueue.size() + (_output.getDataLen() > 0 ? 1 : 0);
	if ((queueSize == 0 || _writeFinishClose)
			&& _iocomponent != NULL)
	{
		_iocomponent->enableWrite(false);
	}
	_output_mutex.unlock();

	if (_writeFinishClose)
	{
		OUT_ERROR(NULL, 0, NULL, "�����Ͽ�.");
		return false;
	}

//	// �����client, ������queue���ȵ�����
//	if (!_isServer && _queueLimit > 0 && _queueTotalSize > _queueLimit)
//	{
//		_output_mutex.lock();
//		_queueTotalSize = queueSize + _channelPool.getUseListCount();
//		if (_queueTotalSize <= _queueLimit)
//		{
//			_outputCond.broadcast();
//		}
//		_output_mutex.unlock();
//	}

	return true;


	printf("GGIIITTT\n");

}

/****************************
 *(1)read
 ***************************/
bool TCPComponent::readData()
{
	_input.ensureFree(READ_WRITE_SIZE);
	int ret = _socket->read(_input.getFree(), _input.getFreeLen());
	int readCnt = 0;
	int freeLen = 0;
	bool broken = false;

	while (ret > 0)
	{
		_input.pourData(ret);
		freeLen = _input.getFreeLen();

		while (1)
		{
			if (!_gotHeader)
			{
				_gotHeader = _streamer->getPacketInfo(&_input, &_packetHeader, &broken);
				if (broken) break;
			}
			// ������㹻������, decode, ���ҵ���handlepacket
			if (_gotHeader && _input.getDataLen() >= _packetHeader._dataLen)
			{
				handlePacket(&_input, &_packetHeader);
				_gotHeader = false;
				_packetHeader._dataLen = 0;
				TBNET_COUNT_PACKET_READ(1);
			}
			else
			{
				break;
			}
		}

		if (broken || freeLen > 0 || readCnt >= 10)
		{
			break;
		}

		if (_packetHeader._dataLen - _input.getDataLen() > READ_WRITE_SIZE)
		{
			_input.ensureFree(_packetHeader._dataLen - _input.getDataLen());
		}
		else
		{
			_input.ensureFree(READ_WRITE_SIZE);
		}

		ret = _socket->read(_input.getFree(), _input.getFreeLen());
		readCnt++;
	}

	_socket->setTcpQuickAck(true);

	// �Ƿ�Ϊ�����ص�
	if (_isServer && _serverAdapter->_batchPushPacket && _inputQueue.size() > 0)
	{
		_serverAdapter->handleBatchPacket(this, _inputQueue);
		_inputQueue.clear();
	}

	_input.shrink();
	if (!broken)
	{
		if (ret == 0)
		{
			broken = true;
		}
		else if (ret < 0)
		{
			int error = Socket::getLastError();
			broken = (error != EAGAIN);
		}
	}
	else
	{
		_gotHeader = false;
	}

	return !broken;
}

/**
 * ����setDisconnState, �ͻ��˵�on_dis_connection
 */
void TCPComponent::setDisconnState()
{
	disconnect();
//	if (_defaultPacketHandler && _isServer == false)
//	{
//		_defaultPacketHandler->handlePacket(&ControlPacket::DisconnPacket, _socket);
//	}
}

} /* namespace triones */
