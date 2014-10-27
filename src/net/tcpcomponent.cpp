/******************************************************
 *   FileName: TCPComponent.cpp
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#include "cnet.h"
#include "tcpcomponent.h"
#include "tbtimeutil.h"
#include "../comm/comlog.h"
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
//	_defaultPacketHandler = NULL;
//	_iocomponent = NULL;
	_queueTimeout = 5000;
	_queueLimit = 50;
	_queueTotalSize = 0;

	/**tcpconnection ****/
	_gotHeader = false;
	_writeFinishClose = false;
//	memset(&_packetHeader, 0, sizeof(_packetHeader));
}

//��������
TCPComponent::~TCPComponent()
{
	__INTO_FUN__
	//��CONNECTION��TCPCONNECTION�����������ŵ�����
	//tcpconnection ������
	//connection����
	disconnect();
	_socket = NULL;
//	_iocomponent = NULL;
}

//���ӶϿ��������з��Ͷ����е�packetȫ����ʱ
void TCPComponent::disconnect()
{
	__INTO_FUN__
	_output_mutex.lock();
	_myQueue.moveto(&_outputQueue);
	_output_mutex.unlock();
	checkTimeout(TBNET_MAX_TIME);
}

//���ӵ�ָ���Ļ���, isServer: �Ƿ��ʼ��һ����������Connection
bool TCPComponent::init(bool isServer)
{
	__INTO_FUN__
	_socket->setSoBlocking(false);
	_socket->setSoLinger(false, 0);
	_socket->setReuseAddress(true);
	_socket->setIntOption(SO_KEEPALIVE, 1);
	_socket->setIntOption(SO_SNDBUF, 64000);
	_socket->setIntOption(SO_RCVBUF, 64000);
	_socket->setTcpNoDelay(true);

	if (!isServer)
	{
		printf("%s %d \n", __FILE__, __LINE__);
		if (!socket_connect() && _autoReconn == false)
		{
			printf("%s %d \n", __FILE__, __LINE__);
			return false;
		}
	}
	else
	{
		printf("%s %d \n", __FILE__, __LINE__);
		_state = TRIONES_CONNECTED;
	}

	printf("%s %d \n", __FILE__, __LINE__);
	setServer(isServer);
	_isServer = isServer;

	return true;
}


/*
 * ���ӵ�socket
 */
bool TCPComponent::socket_connect()
{
	__INTO_FUN__

	//ע�⣬������������ǿ�����ģ������в�ͬ�û����̵߳�������ӿڣ���ȫ����state�ж�Ҳ���Ǻ����ܵģ�2014-10-11
	if (_state == TRIONES_CONNECTED || _state == TRIONES_CONNECTING)
	{
		return true;
	}
	_socket->setSoBlocking(false);

	_startConnectTime = time(NULL);
	if (_socket->connect())
	{
		if (_socketEvent)
		{
			_socketEvent->addEvent(_socket, true, true);
		}
		_state = TRIONES_CONNECTED;
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
			OUT_ERROR(NULL, 0, NULL, "connect %s fail, %s(%d)", _socket->getAddr().c_str(),
			        strerror(error), error);
			return false;
		}
	}
	return true;
}

void TCPComponent::close()
{
	__INTO_FUN__
	if (_socket)
	{
		if (_socketEvent)
		{
			_socketEvent->removeEvent(_socket);
		}
		if (isConnectState())
		{
			//������¼�ת����һ��socketת����ȥ
			setDisconnState();
		}
		_socket->close();

		clearInputBuffer(); // clear input buffer after socket closed

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
	__INTO_FUN__

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
	__INTO_FUN__
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
 * @param    now ��ǰʱ��(��λus)
 */
void TCPComponent::checkTimeout(int64_t now)
{
	//	__INTO_FUN__
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
	//��Ҫ������socket
	else if(_state == TRIONES_CLOSED && _isServer == false && _autoReconn == true)
	{
		//ÿ������������һ��
		if (_startConnectTime > 0 && _startConnectTime < (now - static_cast<int64_t>(5000000)))
		{
			//�����Ƿ����ӳɹ�������������ʱ�䣬ʱ��������5000000
			_startConnectTime = time(NULL);
			socket_connect();
		}
	}

	// ԭ��connect�ĳ�ʱ���
    // checkTimeout(now);
}

/**** ԭ��connectiong�Ĳ��� *********************/

/*
 * handlePacket ����
 */
bool TCPComponent::handlePacket(Packet *packet)
{
	__INTO_FUN__
	//�ͻ��˵ķ���û��ȷ�Ϸ�ʽ������client��server���ǲ���handlerpacket�ķ�ʽ
	this->addRef();

	return _serverAdapter->SynHandlePacket(this, packet);
}

/*** ˵�� 2014-09-21
 * ��1������ط�Ӧ���޶�д�����������ڵ��̵߳������£�д������������󣬵���������socket������һֱ���ڵȴ�״̬ ��
 * ��������Ĭ�������ֻд10�εġ�
 * ��2������д�¼��г��ֵ��쳣����EPIPE�ȣ�û�������⴦�������Ļ�ֻ��ͨ�����¼����ж�socket�ĶϿ������
 * ��3����packet���л�Ϊ�����ݵķ�ʽ�����Ա��Ŀ���У����е�ͨ���ڲ����ǲ���ͬһ��Э�飬
 * ��ͬһ��Э������˷�װ�����ݴ��䶼��ͨ��packet�ķ�ʽ�����packet�Ǵ���chanelID�ģ��������Ͷ˺ͽ��ն˵�ҵ����ܶ�Ӧ������
 * �������ǵ�ҵ���У�ʵ�ֵ��Ǹ�ͨ�ÿͻ��˵�ͨ�ţ����ֿͻ��˿��ܲ�cnetд�ģ������ߵ�Э�飨transprotocol��
 *  *********/
bool TCPComponent::writeData()
{
	__INTO_FUN__
	// �� _outputQueue copy�� _myQueue��, ��_myQueue�����ⷢ��
	_output_mutex.lock();
	_outputQueue.moveto(&_myQueue);

	//���socket�е������Ѿ�ȫ����������ˣ��ÿ�д�¼�Ϊfalse��Ȼ���˳���
	if (_myQueue.size() == 0 && _output.getDataLen() == 0)
	{
		this->enableWrite(false);
		_output_mutex.unlock();
		return true;
	}
	_output_mutex.unlock();

	Packet *packet;
	int ret;
	int writeCnt = 0;
	int myQueueSize = _myQueue.size();

	//todo:��������Ҫ��д��packet������Ǵ�_output�̳й�����
	//���write����ERRORAGAIN���������һ�����������ͣ�

	do
	{
		while (_output.getDataLen() < READ_WRITE_SIZE)
		{
			// ���п��˾��˳�
			if (myQueueSize == 0) break;
			packet = _myQueue.pop();

			//Ϊʲô��packet���뵽_output�з��ͣ����������ʧ�ܵ���������Խ�δ���͵����ݷ��뵽out_put�У�
			//����������չ�����packet���Ǽ̳�DataBuffer,���Խ�packet���г���������
			//ȱ����������һ���ڴ濽����
			_output.writeBytes(packet->getData(), packet->getDataLen());
			myQueueSize--;
			delete packet;
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
	if (queueSize == 0 || _writeFinishClose)
	{
		this->enableWrite(false);
	}
	_output_mutex.unlock();

	if (_writeFinishClose)
	{
		OUT_ERROR(NULL, 0, NULL, "�����Ͽ�.");
		return false;
	}

	return true;
}

/****************************
 *(1)socket�ܹ���ȡ�����İ��Ƕ���
 ***************************/
bool TCPComponent::readData()
{
	__INTO_FUN__
	//ÿ��������Ϊ8K�Ĵ�С����һ�ζ�ȡ���������8K
	_input.ensureFree(READ_WRITE_SIZE);

	//ret��ʾ�Ѿ���ȡ��������
	int ret = _socket->read(_input.getFree(), _input.getFreeLen());

	int read_cnt = 0;
	bool broken = false;

	//�����������ȡ10�Σ���������л�������������socketʹ��
	while (ret > 0 && ++read_cnt <= 10)
	{
		_input.pourData(ret);
		int decode = _streamer->decode(_input.getData(), _input.getDataLen(), &_inputQueue);

		//һ��Ҫ����drainData����Ϊdecodeʱ��û�н�_input�Ķ�λ��ǰ�ơ�
		if(decode > 0)
		{
			_input.drainData(decode);
		}

		//��������˶Ͽ��¼�������_inputû�ж�����˵�������������Ѿ�û�������ˣ�
		if (broken || _input.getFreeLen() > 0) break;

		//����ж��������İ���û�н�����ȫ˵����������δ�������İ����
		//ԭ�ȵ��ж�����Ϊdecode > 0, �޸�Ϊdecode >= 0, decode == 0ʱ������һ�����
		//���ݰ���û����ȫ�������
		if (decode >= 0 && decode < _input.getDataLen())
		{
			_input.ensureFree(READ_WRITE_SIZE);
		}

		//todo: ���������һ�������Ҫ��encode���ֳ�������֪�ϲ㣬��ô_input���������Լ��ķ�Χ����Ӧ����ķ��͡�
		ret = _socket->read(_input.getFree(), _input.getFreeLen());
	}

	//�Զ���������ҵ��ص�����ע������ط���������packet���ͷţ��������ⲿ���ͷŵ�
	if (_inputQueue._size > 0)
	{
		if (_serverAdapter->_batchPushPacket)
		{
			_serverAdapter->handleBatchPacket(this, _inputQueue);
		}
		else
		{
			Packet *pack = NULL;
			while ((pack = _inputQueue.pop()) != NULL)
			{
				_serverAdapter->SynHandlePacket(this, pack);
			}
		}
	}

	//�����������ع鵽��ʼλ��
	_input.shrink();

	/*************
	 * broken�¼���������������¼��������õģ����Ե���ȡ�������󣬶����Ͽ��¼�ʱ����Ӱ���Ѿ����������ݵĴ�������Ҫһ��ǰ��������
	 * ��1���Ͽ��¼����ܲ���ֱ�ӻص��ķ�ʽ�����Ǹ�����packet����һ�����д����Ŷӣ�����ط�Ҫ��packet���д���
	 *************/
	if (!broken)
	{
		if (ret == 0)
		{
			OUT_INFO(NULL, 0, NULL, "%s recv 0, disconnect", _socket->getAddr().c_str());
			broken = true;
		}
		else if (ret < 0)
		{
			int error = Socket::getLastError();
			broken = (error != EAGAIN);
		}
	}

	return !broken;
}

//postPacket��Ϊ�ͻ��ˣ������������ݵĽӿڣ�client���Բ��õȵ�conn success�ص��ɹ����͵�������ӿڡ�
//todo: ��Ҫע��_outputQueue��ʵ�����Ѿ������ˣ�����ּ�����������Ҫ�������������кϲ�2014-10-11
bool TCPComponent::postPacket(Packet *packet)
{
	if (!isConnectState())
	{
		//�����������״̬�����Ҳ����Զ������ģ���ôֱ�ӷ���
		if (isAutoReconn() == false)
		{
			return false;
		}
		//����״̬����󻺴�İ������ݲ��ܳ���10
		else if (_outputQueue.size() > 10)
		{
			return false;
		}
		else
		{
			//init�ڲ����������ӵĹ��̣�
			bool ret = init(false);
			if (!ret) return false;
		}
	}

	// �����client, ������queue���ȵ�����
	_output_mutex.lock();
	_queueTotalSize = _outputQueue.size() + _myQueue.size();
	if (!_isServer && _queueLimit > 0 && _queueTotalSize >= _queueLimit)
	{
		_output_mutex.unlock();
		return false;
	}
	_output_mutex.unlock();

	_output_mutex.lock();
	// д�뵽outputqueue��
	_outputQueue.push(packet);
	if (_outputQueue.size() == 1U)
	{
		enableWrite(true);
	}
	_output_mutex.unlock();

	if (_isServer)
	{
		subRef();
	}

	return true;
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
