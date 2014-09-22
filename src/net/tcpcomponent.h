/******************************************************
 *   FileName: TCPComponent.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef TCPCOMPONENT_H_
#define TCPCOMPONENT_H_

#include "../pack/tprotocol.h"
#include "../thread/mutex.h"

class IServerAdapter;

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

	~TCPComponent();

	bool init(bool isServer = false);

	void close();

	bool handleWriteEvent();

	bool handleReadEvent();

	TCPConnection *get_connection()
	{
		return _connection;
	}

	void checkTimeout(int64_t now);

	bool socket_connect();

	//connection的disconn
	void disconnect();

    /*
     * 设置是否为服务器端
     */
    void setServer(bool isServer) {
        _isServer = isServer;
    }

    void setIOComponent(IOComponent *ioc) {
        _iocomponent = ioc;
    }

    IOComponent *getIOComponent() {
        return _iocomponent;
    }
    /*
     * 设置默认的packetHandler
     */
    void setDefaultPacketHandler(IPacketHandler *ph) {
        _defaultPacketHandler = ph;
    }

    /*
     * 发送packet到发送队列
     *
     * @param packet: 数据包
     * @param packetHandler: packet句柄
     * @param args: 参数
     * @param timeout: 超时时间
     */
    bool postPacket(Packet *packet, IPacketHandler *packetHandler = NULL, void *args = NULL, bool noblocking = true);

    /*
     * 当数据收到时的处理函数
     */
    bool handlePacket(DataBuffer *input, PacketHeader *header);

    /*
     * 写出数据
     */
    bool writeData() = 0;

    /*
     * 读入数据
     */
    bool readData() = 0;

    /*
     * 设置写完是否关闭, 只TCP要用
     */
    virtual void setWriteFinishClose(bool v) {
      UNUSED(v);
    }

    /*
     * 设置对列的超时时间
     */
    void setQueueTimeout(int queueTimeout) {
        _queueTimeout = queueTimeout;
    }

    /*
     * 清空output的buffer
     */
    virtual void clearOutputBuffer() {
        ;
    }

    /*
     * 设置queue最大长度, 0 - 不限制
     */
    void setQueueLimit(int limit) {
        _queueLimit = limit;
    }

    /**
     * 连接状态
     */
    bool isConnectState();

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
     * 清空output的buffer
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
     * 发送setDisconnState
     */
    void setDisconnState();

private:
	// TCP连接
	TCPConnection *_connection;
	time_t _startConnectTime;

	/**   原先connection的部分  ****************/
    IPacketHandler *_defaultPacketHandler;  // connection的默认的packet handler
    bool _isServer;                         // 是服务器端
    IOComponent *_iocomponent;
    Socket *_socket;                        // Socket句柄
    IPacketStreamer *_streamer;             // Packet解析
    IServerAdapter *_serverAdapter;         // 服务器适配器

    PacketQueue _outputQueue;               // 发送队列
    PacketQueue _inputQueue;                // 发送队列
    PacketQueue _myQueue;                   // 在write中处理时暂时用
    triones::Mutex _output_mutex;           // 发送队列锁
//    tbsys::CThreadCond _outputCond;       // 发送队列的条件变量

    ChannelPool _channelPool;               // channel pool
    int _queueTimeout;                      // 队列超时时间
    int _queueTotalSize;                    // 队列总长度
    int _queueLimit;                        // 队列最长长度, 如果超过这个值post进来就会被wait

    /**  TCPCONNECTION 部分  ******************/
    DataBuffer _output;      // 输出的buffer
    DataBuffer _input;       // 读入的buffer
    PacketHeader _packetHeader; // 读入的packet header
    bool _gotHeader;            // packet header已经取过
    bool _writeFinishClose;     // 写完断开
};

} /* namespace triones */
#endif /* TCPCOMPONENT_H_ */
