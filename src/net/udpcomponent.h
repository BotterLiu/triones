/******************************************************
 *   FileName: UDPComponent.h
 *     Author: triones  2014-9-18
 *Description: ����UDP��˵�����ڷ���˶�����UDPAcceptor���������еķ��������Ŀͻ��˶�ʹ��һ��socket��
 *Description: ���ڿͻ�����˵��ÿһ���ͻ���Ϊ�����һ��socket
 *******************************************************/

#ifndef UDPCOMPONENT_H_
#define UDPCOMPONENT_H_

namespace triones
{

class UDPComponent : public IOComponent
{
public:

	friend class UDPManage;

    UDPComponent(Transport *owner, Socket *socket, TransProtocol *streamer,
    		IServerAdapter *serverAdapter, int type = 0);

    ~UDPComponent();

    //��ʼ��
    bool init(bool isServer = false);

    //�ر�
    void close();

    //TransPort��д�Ļص�����
    bool handleWriteEvent();

    //TransPort�ɶ��Ļص�����
    bool handleReadEvent();

    bool readData();

    bool writeData();

private:
//    __gnu_cxx::hash_map<int, UDPConnection*> _connections;  // UDP���Ӽ���
    IServerAdapter *_serverAdapter;

    int     _udp_type;
    struct  sockaddr_in      _sock_addr;

	//�����_isServerָ��accpect������socket��������listen socket
    bool _isServer;                         // �Ƿ�������
    Socket *_socket;                        // Socket���
    TransProtocol *_streamer;               // Packet����

    PacketQueue _outputQueue;               // ���Ͷ���
    PacketQueue _inputQueue;                // ���ն���
    PacketQueue _myQueue;                   // ��write�д���ʱ��ʱ��
    triones::Mutex _output_mutex;           // ���Ͷ�����

    int _queueTimeout;                      // ���г�ʱʱ��
    int _queueTotalSize;                    // �����ܳ���
    int _queueLimit;                        // ���������, ����������ֵpost�����ͻᱻwait

    /**  TCPCONNECTION ����  ******************/
    DataBuffer _output;      // �����buffer
    DataBuffer _input;       // �����buffer
    bool _gotHeader;            // packet header�Ѿ�ȡ��
    bool _writeFinishClose;     // д��Ͽ�
};

} /* namespace triones */
#endif /* UDPCOMPONENT_H_ */
