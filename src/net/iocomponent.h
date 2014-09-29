/******************************************************
 *   FileName: IOComponent.h
 *     Author: triones  2014-9-18
 *Description:
 *******************************************************/

#ifndef IOCOMPONENT_H_
#define IOCOMPONENT_H_

namespace triones
{

class IOComponent
{
public:
	enum
	{
		TRIONES_CONNECTING = 1, TRIONES_CONNECTED, TRIONES_CLOSED, TRIONES_UNCONNECTED
	};

	friend class Transport;

public:
	/*
	 * 构造函数
	 */
	IOComponent(Transport *owner, Socket *socket);
	/*
	 * 析构函数
	 */
	virtual ~IOComponent();

	/*
	 * 初始化
	 *
	 * @return 是否成功
	 */
	virtual bool init(bool isServer = false) = 0;

	/*
	 * 关闭
	 */
	virtual void close()
	{
	}

	/*
	 * 当有数据可写到时被Transport调用
	 *
	 * @return 是否成功, true - 成功, false - 失败。
	 */
	virtual bool handleWriteEvent() = 0;

	/*
	 * 当有数据可读时被Transport调用
	 *
	 * @return 是否成功, true - 成功, false - 失败。
	 */
	virtual bool handleReadEvent() = 0;

	/*
	 * 超时检查
	 *
	 * @param    now 当前时间(单位us)
	 */
	virtual void checkTimeout(int64_t now) = 0;

	/*
	 * 得到socket句柄
	 *
	 * @return Socket
	 */
	Socket *getSocket()
	{
		return _socket;
	}

	/*
	 * 设置SocketEvent
	 */
	void setSocketEvent(SocketEvent *socketEvent)
	{
		_socketEvent = socketEvent;
	}

	/*
	 * 设置能读写
	 *
	 * @param writeOn 写是否打开
	 */
	void enableWrite(bool writeOn)
	{
		if (_socketEvent)
		{
			_socketEvent->setEvent(_socket, true, writeOn);
		}
	}

	/*
	 * 增加引用计数
	 */
	int addRef()
	{
		return atomic_add_return(1, &_refcount);
	}

	/*
	 * 减少引用计数
	 */
	void subRef()
	{
		atomic_dec(&_refcount);
	}

	/*
	 * 取出引用计数
	 */
	int getRef()
	{
		return atomic_read(&_refcount);
	}

	/*
	 * 是否连接状态, 包括正在连接
	 */
	bool isConnectState()
	{
		return (_state == TRIONES_CONNECTED || _state == TRIONES_CONNECTING);
	}

	/*
	 * 得到连接状态
	 */
	int getState()
	{
		return _state;
	}

	/*
	 * 设置是否重连
	 */
	void setAutoReconn(bool on)
	{
		_autoReconn = on;
	}

	/*
	 * 得到重连标志
	 */
	bool isAutoReconn()
	{
		return (_autoReconn && !_isServer);
	}

	/**
	 * 是否在ioclist中
	 */
	bool isUsed()
	{
		return _inUsed;
	}

	/**
	 * 设置是否被用
	 */
	void setUsed(bool b)
	{
		_inUsed = b;
	}

	/**
	 * 最近使用
	 */
	int64_t getLastUseTime()
	{
		return _lastUseTime;
	}

	/**
	 * owner
	 */
	triones::Transport *getOwner();

protected:
	triones::Transport *_owner;
	Socket *_socket;    // 一个Socket的文件句柄
	SocketEvent *_socketEvent;
	int _state;         // 连接状态
	atomic_t _refcount; // 引用计数
	bool _autoReconn;   // 是否重连
	bool _isServer;     // 是否为服务器端
	bool _inUsed;       // 是否在用
	int64_t _lastUseTime;   // 最近使用的系统时间

private:
	IOComponent *_prev; // 用于链表
	IOComponent *_next; // 用于链表
};
} /* namespace triones */
#endif /* IOCOMPONENT_H_ */
