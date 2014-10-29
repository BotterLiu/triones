/******************************************************
 *   FileName: UDPAcceptor.h
 *     Author: triones  2014-9-18
 *Description: (1) UDPAcceptor ��������������������UDPComponent��������Transportֱ�ӹ���
 *Description: ���е�UDPComponent����һ��socket�����еĶ�����UDPAcceptor���
 *Description: д������UDPComponent��ɣ�ͬʱд��������ͬ���ӿڡ�
 *Description: ��2�� socket�رյĲ��ԣ������־socket��״̬�������������Դ��ҵ����packet��������ͨ��packet��֪ͨ
 *Description:     ������ֱ�ӻص���
 *Description: ��3�����ü����Ĳ��ԣ���������ۺ�A���ü�����1������ۺϹ�ϵ���ü�����1
 *******************************************************/

#ifndef UDPACCEPTOR_H_
#define UDPACCEPTOR_H_

#include "../comm/tqueue.h"

//UDP������󳤶ȼ�Ϊ64KB
#define TRIONES_UDP_MAX_PACK  (64 * 1024)
#define TRIONES_UDP_RECV_SIZE (64 * 1024)

namespace triones
{
class UDPAcceptor : public IOComponent
{
public:
    UDPAcceptor(Transport *owner, Socket *socket,
    		triones::TransProtocol *streamer, IServerAdapter *serverAdapter);

    virtual ~UDPAcceptor(){}


    bool init(bool isServer = false);

    //����д�¼�
    bool handleReadEvent();

    //������д�¼�
    bool handleWriteEvent()
    {
        return true;
    }

    //����Ϊ64KB��Ҳ��һ��IP���ĳ��ȡ���������UDP�����һ��Ӧ�ð��������
    //����һ��UDP���а������Ӧ�ð��������
    bool readData();

    //������д����, ��UDPAcceptor������õ�writeData
    bool writeData();

    //��鳬ʱ�����_online���ص���ʱ������
    void checkTimeout(int64_t now);

    //����sockid��ȡ��Ӧ��UDPComponent, ���û���ҵ��½�һ��
    UDPComponent *get(uint64_t sockid);

    //�����յ�ioc�Żص�������
    void put(UDPComponent* ioc);

private:

    struct  sockaddr_in      _sock_addr;

	//�����_isServerָ��accpect������socket��������listen socket
    bool _isServer;

    // Socket���
    Socket *_socket;

    // Packet����
    TransProtocol *_streamer;

    // ���Ͷ�����
    triones::Mutex _output_mutex;

    //UDP���ճ���
    char _read_buff[TRIONES_UDP_RECV_SIZE];

    //���ն��У����ʱʹ��
    PacketQueue _inputQueue;

    // packet header�Ѿ�ȡ��
    bool _gotHeader;

    //����Ĳ��֣����Acceptor UDP���е��û�����
	// ���ݶ���ͷ
	TQueue<IOComponent> _queue ;

//	// ���߶��в�������
//	std::set<UDPComponent*> _index ;

	// ���߶��й���
	TQueue<IOComponent> _online ;

	// ����ͬ��������
	triones::Mutex _mutex ;

	// ���Ӷ�����ҹ���
	std::map<uint64_t, UDPComponent*> _mpsock;
};

} /* namespace triones */

#endif /* UDPACCEPTOR_H_ */
