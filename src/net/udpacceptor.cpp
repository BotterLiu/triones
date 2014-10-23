/******************************************************
 *   FileName: UDPAcceptor.cpp
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#include "cnet.h"

namespace triones
{

/**
 * ���캯������Transport���á�
 *
 * @param  owner:    ��������
 * @param  host:   ����ip��ַ��hostname
 * @param port:   �����˿�
 * @param streamer:   ���ݰ���˫��������packet����������������
 * @param serverAdapter:  ���ڷ������ˣ���Connection��ʼ����Channel����ʱ�ص�ʱ��
 */
UDPAcceptor::UDPAcceptor(Transport *owner, Socket *socket, TransProtocol *streamer,
        IServerAdapter *serverAdapter)
		: IOComponent(owner, socket, TRIONES_UDPACCETOR)
{
	_streamer = streamer;
	_serverAdapter = serverAdapter;
}

/*
 * ��ʼ��, ��ʼ����
 */
bool UDPAcceptor::init(bool isServer)
{
	UNUSED(isServer);
	_socket->setSoBlocking(false);
	return ((ServerSocket*) _socket)->listen();
}

/**
 * �������ݿɶ�ʱ��Transport����
 *
 * @return �Ƿ�ɹ�
 */
bool UDPAcceptor::handleReadEvent()
{

}

bool UDPAcceptor::readData()
{
	struct sockaddr_in read_addr;
	int n = _socket->recvfrom(_read_buff, sizeof(_read_buff), read_addr);

}

bool UDPAcceptor::writeData()
{

}

void UDPAcceptor::checkTimeout(int64_t now)
{
	UNUSED(now);
	return;
}

} /* namespace triones */
