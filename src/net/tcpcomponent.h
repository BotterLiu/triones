/******************************************************
 *   FileName: TCPComponent.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef TCPCOMPONENT_H_
#define TCPCOMPONENT_H_

#include "../pack/tprotocol.h"
#include "../comm/mutex.h"
#include "../comm/databuffer.h"

#define READ_WRITE_SIZE 8192
#ifndef UNUSED
#define UNUSED(v) ((void)(v))
#endif

namespace triones
{

class TCPComponent : public IOComponent
{
public:
	TCPComponent(Transport *owner, Socket *socket, TransProtocol *streamer,
	        IServerAdapter *serverAdapter);

	virtual ~TCPComponent();

	bool init(bool isServer = false);

	void close();

	bool handleWriteEvent();

	bool handleReadEvent();

	void checkTimeout(int64_t now);

	bool socket_connect();

	//connection��disconn
	void disconnect();

    /*
     * �����Ƿ�Ϊ��������
     */
    void setServer(bool isServer) {
        _isServer = isServer;
    }


    bool postPacket(Packet *packet);
    /*
     * �������յ�ʱ�Ĵ�����
     */
    bool handlePacket(Packet *packet);

    /*
     * д������
     */
    virtual bool writeData();

    /*
     * ��������
     */
    virtual bool readData();


    /*
     * ���ö��еĳ�ʱʱ��
     */
    void setQueueTimeout(int queueTimeout) {
        _queueTimeout = queueTimeout;
    }

    /*
     * ����queue��󳤶�, 0 - ������
     */
    void setQueueLimit(int limit) {
        _queueLimit = limit;
    }

    //Ϊ������baseService�Ľӿڶ����ӵ�  2014-10-14
    void setServerAdapter(IServerAdapter *sa)
    {
    	_serverAdapter = sa;
    }


    /**
     * serverId
     */
    uint64_t getServerId() {
        if (_socket) {
            return _socket->getId();
        }
        return 0;
    }

    uint64_t getPeerId() {
        if (_socket) {
            return _socket->getPeerId();
        }
        return 0;
    }

    /**
     * localPort
     */
    int getLocalPort() {
        if (_socket) {
            return _socket->getLocalPort();
        }
        return -1;
    }


    void setWriteFinishClose(bool v) {
        _writeFinishClose = v;
    }

    /*
     * ���output��buffer
     */
    void clearOutputBuffer() {
        _output.clear();
    }

    /*
     * clear input buffer
     */
    void clearInputBuffer() {
        _input.clear();
    }

    /**
     * ����setDisconnState
     */
    void setDisconnState();

private:
	// TCP����
	time_t _startConnectTime;

	/**   ԭ��connection�Ĳ���  ****************/
//    IPacketHandler *_defaultPacketHandler;  // connection��Ĭ�ϵ�packet handler
	//�����_isServerָ��accpect������socket��������listen socket
    bool _isServer;                         // �Ƿ�������
//    IOComponent *_iocomponent;
    Socket *_socket;                        // Socket���
    TransProtocol *_streamer;             // Packet����
    IServerAdapter *_serverAdapter;         // ������������

    PacketQueue _outputQueue;               // ���Ͷ���
    PacketQueue _inputQueue;                // ���ն���
    PacketQueue _myQueue;                   // ��write�д���ʱ��ʱ��
    triones::Mutex _output_mutex;           // ���Ͷ�����
//    tbsys::CThreadCond _outputCond;       // ���Ͷ��е���������

//    ChannelPool _channelPool;               // channel pool
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
#endif /* TCPCOMPONENT_H_ */
