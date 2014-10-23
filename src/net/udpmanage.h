/******************************************************
 *   FileName: udpmanage.h
 *     Author: triones  2014-10-23
 *Description:
 *******************************************************/

#ifndef UDPMANAGE_H_
#define UDPMANAGE_H_

#include "tqueue.h"

namespace triones
{

class UDPManage
{
public:
	UDPManage(){}
	virtual ~UDPManage(){}

	IOComponent *get(int sockfd, const char *ip, unsigned short port, int ctype, bool connected = true)
	{
		IOComponent *ioc = NULL;
		char szid[128] = {0};
		snprintf(szid, sizeof(szid) - 1, "%d_%s_%d", sockfd, ip, port);

		_mutex.lock();
		std::map<std::string, IOComponent *>::iterator iter = _mpsock.find(szid);
		if(iter != _mpsock.end())
		{
			ioc = iter->second;
			_mutex.unlock();
			return ioc;
		}

		ioc = (IOComponent*)_queue.pop() ;
		if ( ioc == NULL ) {
			ioc = new IOComponent ;
		}

		ioc->_type = FD_UDP ;
		ioc->init( sockfd, ip, port , ctype ) ;
		ioc->_last = time(NULL) ;  // ���һ��ʹ��ʱ��
		ioc->_ptr  = NULL ; // ��������չ����
		ioc->_next = NULL ;
		ioc->_pre  = NULL ;
		ioc->_activity = ( connected ) ? SOCKET_CONNECTED: SOCKET_CONNECTING; // �Ƿ������ӳɹ�

		// ��Է�������FD��Դ�����Լ�����
		if ( queue ) {
			_online.push( p ) ;
			_index.insert( std::set<socket_t*>::value_type(p) ) ;
			_mpsock.insert( std::make_pair( szid, p ) ) ;
		}
		_mutex.unlock() ;
	}

	void put(IOComponent *ioc)
	{

	}

private:
	// ���ݶ���ͷ
	TQueue<IOComponent> _queue ;
	// ���߶��в�������
	std::set<IOComponent*> _index ;
	// ���߶��й���
	TQueue<IOComponent> _online ;
	// ����ͬ��������
	triones::Mutex _mutex ;
	// ���Ӷ�����ҹ���
	std::map<std::string, IOComponent*> _mpsock;
};

} /* namespace triones */
#endif /* UDPMANAGE_H_ */
