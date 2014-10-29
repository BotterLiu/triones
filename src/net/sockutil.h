/******************************************************
 *   FileName: sockutil.h
 *     Author: triones  2014-10-27
 *Description:
 *******************************************************/

#ifndef SOCKUTIL_H_
#define SOCKUTIL_H_

#include <string>

namespace triones
{

class sockutil
{
private:
	struct triones_sockaddr
	{
		unsigned short family;
		unsigned short port;
		unsigned int   host;
	};

	union seriaddr
	{
		uint64_t sockid;
		triones_sockaddr sockaddr;
	};

public:
	//�������ַת��Ϊһ��64λ���޷������͵ı�ʶID������keyֵ�ĵĲ���,�����Ϊstring���ܺܵ�
	static uint64_t sock_addr2id(struct sockaddr_in *sockaddr);

	//��ID��תΪ�����ַ����
	static void sock_id2addr(uint64_t sockid, struct sockaddr_in *sockaddr);

	//�������ַתΪ�ַ���
	static std::string sock_addr2str(struct sockaddr_in *sockaddr);

	//������IDתΪ�ַ��� example:10692856960556924930 -> udp:192.168.100.148:4000
	static std::string sock_id2str(uint64_t id);
};


} /* namespace triones */
#endif /* SOCKUTIL_H_ */
