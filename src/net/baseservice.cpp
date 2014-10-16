/******************************************************
 *   FileName: baseservice.cpp
 *     Author: triones  2014-9-26
 *Description:
 *******************************************************/

#include "baseservice.h"

namespace triones
{

BaseService::BaseService()
		: _inited(false)
{
	_packqueue = new triones::CDataQueue<triones::Packet>(MAXQUEUE_LENGTH);
	_queue_thread = new QueueThread(_packqueue, this);
	_transport = new Transport();

	//��ƹ�Ҳ���ʱ�����ܲ�������
	memset(_send_buffer, 0x31, sizeof(_send_buffer) - 2);
	_send_buffer[sizeof(_send_buffer) - 2] = 0x0d;
	_send_buffer[sizeof(_send_buffer) - 1] = 0x0a;
}

BaseService::~BaseService()
{
	// TODO Auto-generated destructor stub
}

bool BaseService::init(int thread_num /* = 1 */)
{
	//���������룬���Ǻ����ʼ����thread_num���������ˡ��û����������thread_num
	//��ֱ�ӵ��ú����connect �� listen
	if (!_inited) return true;

	//ʵ�������������߳̾��Ѿ������ˣ�������start
	if (!_queue_thread->init(thread_num)) return false;

	if (!_transport->start())
	{
		_queue_thread->stop();
		return false;
	}

	return true;
}

IOComponent *BaseService::connect(const char *spec, int streamer, bool autoReconn)
{
	if(!init()) return false;

	triones::TransProtocol *tp = __trans_protocol.get(streamer);
	if (tp == NULL) return NULL;

	IOComponent *tc = _transport->connect(spec, tp, autoReconn);
	if (tc != NULL)
	{
		tc->setServerAdapter(this);
	}
	return tc;
}

IOComponent* BaseService::listen(const char *spec, int streamer)
{
	if(!init()) return false;

	triones::TransProtocol *tp = __trans_protocol.get(streamer);
	if (tp == NULL) return NULL;

	return _transport->listen(spec, tp, this);
}

//IServerAdapter�Ļص�������������packet�������ֱ�Ӽ���ҵ������У�������������������ҵ���İ��룻
bool BaseService::SynHandlePacket(IOComponent *connection, Packet *packet)
{
	__INTO_FUN__

	packet->_ioc = (void*) connection;

	if (!_queue_thread->push((void*) packet))
	{
		delete packet;
	}

//  ������ֱ�ӻص�ʱ��ƹ�Ҳ��Ե����ܲ��Դ���
//	UNUSED(packet);
//	static int count = 3;
//
//	if (count++ > 0)
//	{
//		if(count > 10000)
//		{
//			count = 1;
//			printf("#########################SEND %lu count %d\n", sizeof(_send_buffer), count);
//		}
//
//		Packet *pack1 = new Packet;
//		pack1->writeBytes(_send_buffer, sizeof(_send_buffer));
//		if (!connection->postPacket(pack1))
//		{
//			delete pack1;
//			pack1 = NULL;
//		}
//	}

	return true;
}

//QueueThread�첽���е��첽�ص�����
void BaseService::handle_queue(void *packet)
{
	__INTO_FUN__
	handle_packet((IOComponent*) (((Packet*) packet)->_ioc), (Packet*) packet);
}

//������ͬ��ҵ���Ĵ��������service��ʵ��
void BaseService::handle_packet(IOComponent *ioc, Packet *packet)
{
	__INTO_FUN__
	printf("BaseService handle pack %s, %s", ioc->getSocket()->getAddr().c_str(), packet->_pdata);
	return;
}

bool BaseService::destroy_service()
{
	return true;
}

bool BaseService::destroy()
{
	if(_transport != NULL)
	{
		//ֻ��transport����stop��־λ���̻߳����ܱ�֤�Ѿ�ȫ��������
		_transport->stop();
	}

	//����_transport stop��wait���м�
	destroy_service();

	if(_queue_thread != NULL)
	{
		_queue_thread->stop();
	}

	if(_transport != NULL)
	{
		_transport->wait();
	}

	delete _transport;
	_transport = NULL;

	return true;
}

} /* namespace triones */
