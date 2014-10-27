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

	UDPComponent *get(uint64_t sockid, Socket *socket, bool connected = true)
	{
		UDPComponent *ioc = NULL;

		_mutex.lock();
		std::map<uint64_t, UDPComponent *>::iterator iter = _mpsock.find(sockid);

		if(iter != _mpsock.end())
		{
			ioc = iter->second;
			_mutex.unlock();
			return ioc;
		}

		ioc = (UDPComponent*)_queue.pop() ;
		if ( ioc == NULL ) {
			ioc = new UDPComponent;
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

	void put(UDPComponent *ioc)
	{

	}

private:
	// ���ݶ���ͷ
	TQueue<UDPComponent> _queue ;
	// ���߶��в�������
	std::set<UDPComponent*> _index ;
	// ���߶��й���
	TQueue<UDPComponent> _online ;
	// ����ͬ��������
	triones::Mutex _mutex ;
	// ���Ӷ�����ҹ���
	std::map<uint64_t, UDPComponent*> _mpsock;
};

} /* namespace triones */
#endif /* UDPMANAGE_H_ */
