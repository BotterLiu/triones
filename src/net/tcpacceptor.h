/******************************************************
 *   FileName: TCPAcceptor.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef TCPACCEPTOR_H_
#define TCPACCEPTOR_H_

#include "iocomponent.h"
#include "iserveradapter.h"

namespace triones
{

class TCPAcceptor : public IOComponent
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
    TCPAcceptor(Transport *owner, Socket *socket,
                IPacketStreamer *streamer, IServerAdapter *serverAdapter);

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
    bool handleWriteEvent() {
        return true;
    }

    /*
     * ��ʱ���
     *
     * @param    now ��ǰʱ��(��λus)
     */
    void checkTimeout(int64_t now);

private:
    IPacketStreamer *_streamer;      // ���ݰ�������
    IServerAdapter  *_serverAdapter; // ������������
};

} /* namespace triones */
#endif /* TCPACCEPTOR_H_ */
