/*
 * (C) 2007-2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *
 */

#ifndef TBNET_TRANSPORT_H_
#define TBNET_TRANSPORT_H_

#include <sys/ptrace.h>
#include "../comm/thread.h"

namespace triones
{

class Transport: public triones::TBRunnable
{

public:

	Transport();

	~Transport();

	//起动运输层，创建两个线程，一个用于读，一个用写。
	bool start();

	//停止，停掉读写线程，及销毁。
	bool stop();

	//等待线程完全退出
	bool wait();

	//线程的运行函数，实现Runnable接口中的函数
	void run(triones::TBThread *thread, void *arg);

	/*
	 * 起一个监听端口。
	 * @param spec: 格式 [upd|tcp]:ip:port
	 * @param streamer: 数据包的双向流，用packet创建，解包，组包。
	 * @param serverAdapter: 用在服务器端，当Connection初始化及Channel创建时回调时用
	 * @return IO组件一个对象的指针
	 */
	IOComponent *listen(const char *spec, triones::TransProtocol *streamer,
	        IServerAdapter *serverAdapter);

	/*
	 * 创建一个Connection，连接到指定的地址，并加入到Socket的监听事件中。
	 *
	 * @param spec: 格式 [upd|tcp]:ip:port
	 * @param streamer: 数据包的双向流，用packet创建，解包，组包。
	 * @param autoReconn: 是否重连
	 */
	IOComponent *connect(const char *spec, triones::TransProtocol *streamer,
	        bool autoReconn = false);

	//主动断开
	bool disconnect(TCPComponent *conn);

	/*
	 * 加入到iocomponents中
	 *
	 * @param  ioc: IO组件
	 * @param  readOn: 初始化把读事件打开
	 * @param  writeOn: 初始化把写事件打开
	 */
	void addComponent(IOComponent *ioc, bool readOn, bool writeOn);

	// 从iocomponents中删除掉
	void removeComponent(IOComponent *ioc);

	// 是否为stop
	bool* getStop();

private:

	/*
	 * 把[upd|tcp]:ip:port分开放在args中
	 *
	 * @param src: 源格式
	 * @param args: 目标数组
	 * @param   cnt: 数组中最大个数
	 * @return  返回的数组中个数
	 */
	int parseAddr(char *src, char **args, int cnt);

	// socket event 的检测
	void eventLoop(SocketEvent *socketEvent);

	//超时检查
	void timeoutLoop();

	//释放变量
	void destroy();

private:

	SocketEvent _socketEvent;              // 读写socket事件
	triones::TBThread _readWriteThread;    // 读写处理线程
	triones::TBThread _timeoutThread;      // 超时检查线程
	bool _stop;                            // 是否被停止

	IOComponent *_delListHead, *_delListTail;   // 等待删除的IOComponent集合
	IOComponent *_iocListHead, *_iocListTail;   // IOComponent集合
	bool _iocListChanged;                       // IOComponent集合被改过
	int _iocListCount;

	triones::Mutex _iocsMutex;

	HashSock    _hash_socks;
};

}

#endif /*TRANSPORT_H_*/
