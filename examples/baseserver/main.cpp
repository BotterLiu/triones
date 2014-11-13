/******************************************************
*   FileName: main.cpp
*     Author: triones  2014-10-9
*Description:
*******************************************************/

#include "baseserver.h"
#include  "comm/comlog.h"

int main()
{
	BaseServer base_server;
	base_server.start("tcp:127.0.0.1:7406", 1);
	base_server.start("udp:127.0.0.1:7407", 1);

	while(1)
	{
		sleep(5);
	}

	base_server.destroy();
	LOGSTOP();

//	IOComponent *ioc = new TCPAcceptor(NULL, NULL, NULL, NULL);
//	delete ioc;


}

