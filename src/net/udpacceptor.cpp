/******************************************************
 *   FileName: UDPAcceptor.cpp
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#include "cnet.h"

namespace triones
{

UDPAcceptor::UDPAcceptor(Transport *owner, Socket *socket, TransProtocol *streamer,
        IServerAdapter *serverAdapter)
		: IOComponent(owner, socket, TRIONES_UDPACCETOR)
{
	_streamer = streamer;
	_serverAdapter = serverAdapter;
}

bool UDPAcceptor::init(bool isServer)
{
	UNUSED(isServer);
	_socket->setSoBlocking(false);
	return ((ServerSocket*) _socket)->listen();
}


bool UDPAcceptor::handleReadEvent()
{
	return readData();
}

bool UDPAcceptor::readData()
{
	struct sockaddr_in read_addr;
	int n = _socket->recvfrom(_read_buff, sizeof(_read_buff), read_addr);
	if(n < 0)
		return false;

	uint64_t sockid = triones::sockutil::sock_addr2id(&read_addr);
	UDPComponent *ioc = get(sockid);
	ioc->_lastUseTime = triones::CTimeUtil::getTime();

	int decode = _streamer->decode(_read_buff, n, &_inputQueue);

	if (decode > 0)
	{
		Packet *pack = NULL;
		while ((pack = _inputQueue.pop()) != NULL)
		{
			_serverAdapter->SynHandlePacket(ioc, pack);
		}
	}

	return true;
}

bool UDPAcceptor::writeData()
{
	return true;
}

void UDPAcceptor::checkTimeout(int64_t now)
{

	UNUSED(now);
	return;
}

//����sockid��ȡ��Ӧ��UDPComponent, ���û���ҵ��½�һ��
UDPComponent *UDPAcceptor::get(uint64_t sockid)
{
	UDPComponent *ioc = NULL;

	_mutex.lock();
	std::map<uint64_t, UDPComponent*>::iterator iter = _mpsock.find(sockid);
	if(iter != NULL)
	{
		ioc = iter->second;
		_mutex.unlock();
	}

	ioc = new UDPComponent(NULL, _socket, _streamer, _serverAdapter, TRIONES_UDPACTCONN);
	_online.push(ioc);
	_mpsock.insert(make_pair(sockid, ioc));

	_mutex.unlock();

	return ioc;
}

//�����յ�ioc�Żص�������
void UDPAcceptor::put(UDPComponent* ioc)
{
	return;
}

} /* namespace triones */
