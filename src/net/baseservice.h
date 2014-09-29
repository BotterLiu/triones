/******************************************************
 *   FileName: baseservice.h
 *     Author: triones  2014-9-26
 *Description:
 *******************************************************/

#ifndef BASESERVICE_H_
#define BASESERVICE_H_

#include "cnet.h"
#include "../pack/pack.h"
#include "../thread/queuethread.h"
#include "dataqueue.h"

namespace triones
{

class BasePacket
{
public:
	Packet *_packet;
	IOComponent *_ioc;

	BasePacket *_next;
	BasePacket *_pre;
};

class BaseService: public IServerAdapter, public IQueueHandler
{
public:

#define MAXQUEUE_LENGTH        102400    // �����г���

	BaseService();

	virtual ~BaseService();

	//IServerAdapter�Ļص�������������packet�������ֱ�Ӽ���ҵ������У�������������������ҵ���İ��룻
	virtual bool handlePacket(IOComponent *connection, Packet *packet);

	virtual void handle_queue(void *packet);

	//������ͬ��ҵ���Ĵ��������service��ʵ��
	virtual void handle_queue_packet(IOComponent *ioc, Packet *packet);

private:

	int initialize_network(const char* app_name)
	{
		UNUSED(app_name);
		return 0;
	}

protected:
	//����ģ��
	Transport* _transport;
private:

	CDataQueue<BasePacket>	* _packqueue ;
	//packet�̶߳���
	QueueThread *_queue_thread;
};

} /* namespace triones */
#endif /* BASESERVICE_H_ */
