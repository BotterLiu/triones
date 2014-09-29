/******************************************************
 *   FileName: baseservice.cpp
 *     Author: triones  2014-9-26
 *Description:
 *******************************************************/

#include "baseservice.h"

namespace triones
{

BaseService::BaseService()
{
	_packqueue   = new CDataQueue<BasePacket>(MAXQUEUE_LENGTH);
}

BaseService::~BaseService()
{
	// TODO Auto-generated destructor stub
}


//IServerAdapter�Ļص���������������packet�������ֱ�Ӽ���ҵ������У�������������������ҵ���İ��룻
bool BaseService::handlePacket(IOComponent *connection, Packet *packet)
{
	BasePacket *base_pack = new BasePacket;
	base_pack->_ioc = connection;
	base_pack->_packet = packet;

	/* *************************
	 * ��1��BasePacket���������������ͷ�connectiong��packet��connection�����������ͷţ�packet����handle_queue�����ͷŵģ�
	 * ��2��pushʧ��ʱ��packet�������︺���ͷŵģ�
	 * ��3�����pushʧ��ʱ��queue_thread�ǲ������ͷ�base_pack�ģ�������������ͷš�
	 * *************************/
	if(! _queue_thread.push((void*)base_pack))
	{
		delete base_pack->_packet;
		delete base_pack;
	}
}

void BaseService::handle_queue(void *packet)
{
	BasePacket *base_pack = (BasePacket*)packet;

	handle_queue_packet(base_pack->_ioc, base_pack->_packet);

	//�����ｲpacket�ͷŵ���base_pack���ͷ���queuethread�����
	delete base_pack->_packet;
}

//������ͬ��ҵ���Ĵ����������service��ʵ��
void BaseService::handle_queue_packet(IOComponent *ioc, Packet *packet)
{
	printf("handle pack %s, %s", ioc->getSocket()->getAddr(), packet->_pdata);
	return ;
}


} /* namespace triones */