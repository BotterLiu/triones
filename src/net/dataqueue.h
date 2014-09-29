/*
 * dataqueue.h
 *
 *  Created on: 2011-11-16
 *      Author: humingqing
 *  ���ݶ���ģ���࣬��Ҫ��������̶߳���ʹ�ã�ʵ��ͨ�õ���������PUSH��POP����������ʹ��ģ���б�����_nextָ��
 */

#ifndef __DATAQUEUE_H__
#define __DATAQUEUE_H__

#include "../thread/queuethread.h"

namespace triones
{

// ���ݶ��д������
template<typename T>
class CDataQueue: public IPackQueue
{
public:
	CDataQueue(int maxsize = -1)
			: _max(maxsize)
	{
		_head = _tail = NULL;
		_size = 0;
	}
	~CDataQueue()
	{
		Clear();
	}

	// �������
	bool push(void *data)
	{
		T *pack = (T *) data;
		// ������ڶ��е���󳤶Ⱦ�ֱ�ӱ�����
		if (_size > _max && _max > 0)
		{
			return false;
		}

		// �����������Ϊͷ��Ϊ��
		if (_head == NULL)
		{
			_head = _tail = pack;
		}
		else
		{  // ����м�ڵ�
			_tail->_next = pack;
			_tail = pack;
		}
		_tail->_next = NULL;

		++_size;

		return true;
	}

	// ��������
	void * pop(void)
	{
		// ���Ϊ����ֱ�ӷ�����
		if (_size == 0 || _head == NULL)
		{
			return NULL;
		}

		// �Ӷ�����ȡ����
		T *pack = _head;
		if (_head == _tail)
		{
			_head = _tail = NULL;
		}
		else
		{
			_head = _head->_next;
		}
		--_size;
		return (void*) pack;
	}

	// �ͷ�����
	void free(void *packet)
	{
		if (packet == NULL) return;
		delete (T*) packet;
	}
	// ȡ�ö��еĳ���
	int size(void)
	{
		return _size;
	}

protected:
	// ��������û�л��յ��ڴ�
	void Clear()
	{
		if (_size == 0 || _head == NULL)
		{
			return;
		}

		// �������δ���������
		T *pre, *p = _head;
		while (p != NULL)
		{
			pre = p;
			p = p->_next;
			delete pre;
		}
		_head = NULL;
		_tail = NULL;
		_size = 0;
	}

protected:
	// ���е���󳤶�
	int _max;
	// ��¼�������ݰ��ĸ���
	int _size;
	// ָ������ͷ��ָ��
	T * _head;
	// ָ������β��ָ��
	T * _tail;
};

}

#endif /* DATAQUEUE_H_ */
