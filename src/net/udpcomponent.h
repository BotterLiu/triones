/******************************************************
 *   FileName: UDPComponent.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef UDPCOMPONENT_H_
#define UDPCOMPONENT_H_

namespace triones
{

class UDPComponent : public IOComponent
{
public:
    /**
     * ���캯������Transport���á�
     *
     * @param owner:      Transport
     * @param socket:     Socket
     * @param streamer:   ���ݰ���˫��������packet����������������
     * @param serverAdapter:  ���ڷ������ˣ���Connection��ʼ����Channel����ʱ�ص�ʱ��
     */
    UDPComponent(Transport *owner, Socket *socket, TransProtocol *streamer, IServerAdapter *serverAdapter);

    /*
     * ��������
     */
    ~UDPComponent();

    /*
        * ��ʼ��
        *
        * @return �Ƿ�ɹ�
        */
    bool init(bool isServer = false);

    /*
     * �ر�
     */
    void close();

    /*
        * �������ݿ�д��ʱ��Transport����
        *
        * @return �Ƿ�ɹ�, true - �ɹ�, false - ʧ�ܡ�
        */
    bool handleWriteEvent();

    /*
     * �������ݿɶ�ʱ��Transport����
     *
     * @return �Ƿ�ɹ�, true - �ɹ�, false - ʧ�ܡ�
     */
    bool handleReadEvent();

private:
//    __gnu_cxx::hash_map<int, UDPConnection*> _connections;  // UDP���Ӽ���
    TransProtocol *_streamer;                             // streamer
    IServerAdapter *_serverAdapter;
};

} /* namespace triones */
#endif /* UDPCOMPONENT_H_ */
