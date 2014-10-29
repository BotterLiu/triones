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

//UDPComponentֻ��TRIONES_UDPCONN�Ŵ���д�¼�
bool UDPComponent::handleReadEvent()
{
	__INTO_FUN__
	_lastUseTime = triones::CTimeUtil::getTime();
	bool rc = false;
	if (_type == TRIONES_UDPCONN)
	{
		rc = readData();
	}
	return rc;
}

//�ṩ���Ѿ����ӵ�UDPSocketʹ�ã�
bool UDPComponent::readData()
{
	__INTO_FUN__

	//ֻ��TRIONES_UDPCONN���͵Ĳſ��Ե��õ�����ط�
	if(_type != TRIONES_UDPCONN)
	{
		OUT_INFO(NULL, 0, NULL, "read type error, type %d", _type);
		return false;
	}

	int n = _socket->read(_read_buff, sizeof(_read_buff));
	if(n < 0)
		return false;

	int decode = _streamer->decode(_read_buff, n, &_inputQueue);

	if (decode > 0)
	{
		Packet *pack = NULL;
		while ((pack = _inputQueue.pop()) != NULL)
		{
			_serverAdapter->SynHandlePacket(this, pack);
		}
	}

	return true;
}

//�ṩ���Ѿ����ӵ�UDPSocketʹ�ã�
bool UDPComponent::writeData()
{
	__INTO_FUN__
	return true;
}

//���ݷ��͵�ͬ���ӿ�
bool UDPComponent::postPacket(Packet *packet)
{
	//TRIONES_UDPCONN��TRIONES_UDPACTCONN���Ե��õ�����ط�
	if(_type != TRIONES_UDPCONN && _type != TRIONES_UDPACTCONN)
	{
		OUT_INFO(NULL, 0, NULL, "write type error, type %d", _type);
		return false;
	}

	bool ret = false;
	int len = packet->getDataLen();
	int offset = 0;
	int sendbytes = 0;
	int send_len = 0;

	while (offset < len)
	{
		//UDP��������ͳ���TRIONES_UDP_MAX_PACK
		send_len = min(len, TRIONES_UDP_MAX_PACK);

		//������Ѿ�����connect�����
		if (_type == TRIONES_UDPCONN)
		{
			sendbytes = _socket->write(packet->getData() + offset, send_len);
		}
		else if (_type == TRIONES_UDPACTCONN)
		{
			sendbytes = _socket->sendto(packet->getData() + offset, send_len, _sock_addr);
		}

		if(sendbytes < 0)
		{
			ret = false;
			break;
		}

		offset += sendbytes;
	}

	if(ret)
		delete packet;

	return ret;
}

//todo : 2014-10-29
void UDPComponent::checkTimeout(int64_t now)
{
	UNUSED(now);
	return;
}

} /* namespace triones */
