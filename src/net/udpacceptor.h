/******************************************************
 *   FileName: UDPAcceptor.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef UDPACCEPTOR_H_
#define UDPACCEPTOR_H_

//UDP������󳤶ȼ�Ϊ64KB
#define TRIONES_UDP_RECV_SIZE (64 * 1024)

namespace triones
{
class UDPAcceptor : public IOComponent
{
public:
    /**
    * ���캯������Transport���á�
    *
    * @param  owner:    ��������
    * @param  socket:   Socket����
    * @param streamer:   ���ݰ���˫��������packet����������������
    * @param serverAdapter:  ���ڷ������ˣ���Connection��ʼ����Channel����ʱ�ص�ʱ��
    */
    UDPAcceptor(Transport *owner, Socket *socket,
    		triones::TransProtocol *streamer, IServerAdapter *serverAdapter);

    virtual ~UDPAcceptor(){}
    /*
     * ��ʼ��
     *
     * @return �Ƿ�ɹ�
     */
    bool init(bool isServer = false);

    /**
    * �������ݿɶ�ʱ��Transport����
    *
    * @return �Ƿ�ɹ�, true - �ɹ�, false - ʧ�ܡ�
    */
    bool handleReadEvent();

    /**
     * ��accept��û��д�¼�
     */
    bool handleWriteEvent()
    {
        return true;
    }

    bool readData();

    bool writeData();

    void checkTimeout(int64_t now);

private:

    int     _udp_type;
    struct  sockaddr_in      _sock_addr;

	//�����_isServerָ��accpect������socket��������listen socket
    bool _isServer;                         // �Ƿ�������
    Socket *_socket;                        // Socket���
    TransProtocol *_streamer;               // Packet����

    triones::Mutex _output_mutex;           // ���Ͷ�����

    char _read_buff[TRIONES_UDP_RECV_SIZE]; //UDP���ճ���

    bool _gotHeader;                        // packet header�Ѿ�ȡ��
    bool _writeFinishClose;                 // д��Ͽ�
};

} /* namespace triones */

#endif /* UDPACCEPTOR_H_ */
