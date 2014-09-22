/******************************************************
 *   FileName: ServerSocket.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef SERVERSOCKET_H_
#define SERVERSOCKET_H_

#include "cnet.h"

namespace triones
{

class ServerSocket : public Socket
{
public:
	ServerSocket();
	virtual ~ServerSocket();

	Socket *accept();

	/*
	 * �򿪼���
	 *
	 * @return �Ƿ�ɹ�
	 */
	bool listen();

private:
	int _backLog; // backlog
};

} /* namespace triones */
#endif /* SERVERSOCKET_H_ */
