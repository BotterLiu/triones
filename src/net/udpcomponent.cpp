/******************************************************
 *   FileName: UDPComponent.cpp
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#include "cnet.h"
#include "../pack/tprotocol.h"
#include "../comm/comlog.h"
#include "tbtimeutil.h"
#include "stats.h"

namespace triones
{

UDPComponent::UDPComponent(Transport *owner, Socket *socket, TransProtocol *streamer,
        IServerAdapter *serverAdapter, int type)
		: IOComponent(owner, socket, type)
{
	_streamer = streamer;
	_serverAdapter = serverAdapter;
}

UDPComponent::~UDPComponent()
{

}

bool UDPComponent::init(bool isServer)
{
	if (!isServer)
	{
		if (!_socket->connect())
		{
			return false;
		}
	}

	_isServer = isServer;
	return true;
}

void UDPComponent::close()
{

}

//UDPComponent�������д�¼���UDPComponent��д������ͬ���ӿ�
bool UDPComponent::handleWriteEvent()
{
	return true;
}

bool UDPComponent::handleReadEvent()
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

//�ṩ���Ѿ����ӵ�UDPSocketʹ�ã�
bool UDPComponent::readData()
{
	__INTO_FUN__

	//ֻ��TRIONES_UDPCONN���ܵ�������ӿڣ�
	//TRIONES_UDPACCETOR���õ���readFrom
	//TRIONES_UDPACTCONN��read����TRIONES_UDPACCETOR
	if(_type != TRIONES_UDPCONN)
		return false;

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

//�ṩ���Ѿ����ӵ�UDPSocketʹ�ã�
bool UDPComponent::writeData()
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

		if(_type == TRIONES_UDPCONN)
		{
			// write data
			ret = _socket->sendto(_output.getData(), _output.getDataLen(), _sock_addr);
		}
		else if(_type == TRIONES_UDPACTCONN)
		{
			ret = _socket->write(_output.getData(), _output.getDataLen());
		}

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

//	if (_writeFinishClose)
//	{
//		OUT_ERROR(NULL, 0, NULL, "�����Ͽ�.");
//		return false;
//	}

	return true;
}


} /* namespace triones */
