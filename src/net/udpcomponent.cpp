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
	if (_type == TRIONES_UDPCONN)
	{
		if (_socket)
		{
			_socket->close();
			delete _socket;
			_socket = NULL;
		}
	}
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

//UDPComponent不处理可写事件，UDPComponent的写操作是同步接口
bool UDPComponent::handleWriteEvent()
{
	return true;
}

//UDPComponent只有TRIONES_UDPCONN才处理写事件
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

//提供给已经连接的UDPSocket使用；
bool UDPComponent::readData()
{
	__INTO_FUN__

	//只有TRIONES_UDPCONN类型的才可以调用到这个地方
	if (_type != TRIONES_UDPCONN)
	{
		OUT_INFO(NULL, 0, NULL, "read type error, type %d", _type);
		return false;
	}

	int n = _socket->read(_read_buff, sizeof(_read_buff));
	if (n < 0) return false;

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

//提供给已经连接的UDPSocket使用；
bool UDPComponent::writeData()
{
	__INTO_FUN__
	return true;
}

//数据发送的同步接口
bool UDPComponent::postPacket(Packet *packet)
{
	//TRIONES_UDPCONN和TRIONES_UDPACTCONN可以调用到这个地方
	if (_type != TRIONES_UDPCONN && _type != TRIONES_UDPACTCONN)
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
		//UDP包的最大发送长度TRIONES_UDP_MAX_PACK
		send_len = min(len, TRIONES_UDP_MAX_PACK);

		//如果是已经调用connect的情况
		if (_type == TRIONES_UDPCONN)
		{
			sendbytes = _socket->write(packet->getData() + offset, send_len);
		}
		else if (_type == TRIONES_UDPACTCONN)
		{
			sendbytes = _socket->sendto(packet->getData() + offset, send_len, _sock_addr);
		}

		if (sendbytes < 0)
		{
			ret = false;
			break;
		}

		offset += sendbytes;
	}

	if (ret) delete packet;

	return ret;
}

//todo : 2014-10-29
void UDPComponent::checkTimeout(int64_t now)
{
	UNUSED(now);
	return;
}

} /* namespace triones */
