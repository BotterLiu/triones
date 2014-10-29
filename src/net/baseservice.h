/******************************************************
 *   FileName: baseservice.h
 *     Author: triones  2014-9-26
 *Description: ��1��BaseService���з�װ����װΪһ���Ѿ�����һ���첽���е�transport
 *Description: ��2���ְ�������������Ϊ��ͳһ��server accept��client��ͬһ���ְ�����
 *Description: 	  �����������ӵ�client�����������Լ��ķְ�����
 *Description:
 *******************************************************/

#ifndef BASESERVICE_H_
#define BASESERVICE_H_

#include "cnet.h"
#include "../pack/pack.h"
#include "../comm/queuethread.h"
#include "../comm/dataqueue.h"
#include "../pack/tprotocol.h"

using namespace triones;

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

	//queuethread �첽���У�ҵ���̵߳Ĵ�������, thread_num
	bool init(int thread_num = 1);

	//spec:���ӵ�ַ tcp:127.0.0.1:7008, streamer�ְ���
	IOComponent* connect(const char *spec, int streamer, bool autoReconn = false);

	//spec:���ӵ�ַ tcp:127.0.0.1:7008, streamer�ְ���
	IOComponent* listen(const char *spec, int streamer);

	//QueueThread�첽���е��첽�ص�����
	virtual void handle_queue(void *packet);

	//������ͬ��ҵ���Ĵ��������service��ʵ��
	virtual void handle_packet(IOComponent *ioc, Packet *packet);

	//�ϲ�ҵ�����ٵĻص��������Ǳ���ʵ�֣�
	virtual bool destroy_service();

	//ֹͣ�̣߳��ͷ������Դ��
	bool destroy();

public:

	BaseService();

	virtual ~BaseService();

	//IServerAdapter�Ļص�������������packet�������ֱ�Ӽ���ҵ������У�������������������ҵ���İ��룻
	virtual bool SynHandlePacket(IOComponent *connection, Packet *packet);

protected:
	//����ģ�ͣ� ���ó�protected, �����п��ܻ����
	Transport* _transport;

	//�����ܲ���ʹ��
	char _send_buffer[1024 * 64 - 1024];
private:
	//�Ƿ��Ѿ�������ʼ��
	bool _inited;

	//�첽���У� ��_queue_threadʹ��
	CDataQueue<Packet>	* _packqueue;

	//packet�̶߳���
	QueueThread *_queue_thread;
};

} /* namespace triones */
#endif /* BASESERVICE_H_ */
