/******************************************************
*   FileName: cond.h
*     Author: triones  2014-10-16
*Description: ��ֲ�Ա��Ĵ���
*******************************************************/

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

#ifndef COND_H_
#define COND_H_

#include <assert.h>
#include <sys/time.h>

namespace triones {

/*
 * author cjxrobot
 *
 * Linux�߳���
 */

/**
* @brief linux�߳����������򵥷�װ
*/
class CThreadMutex {

public:
    /*
     * ���캯��
     */
    CThreadMutex() {
        //assert(pthread_mutex_init(&_mutex, NULL) == 0);
        const int iRet = pthread_mutex_init(&_mutex, NULL);
        (void) iRet;
        assert( iRet == 0 );
    }

    /*
     * ���캯��
     */
    ~CThreadMutex() {
        pthread_mutex_destroy(&_mutex);
    }

    /**
     * ����
     */

    void lock () {
        pthread_mutex_lock(&_mutex);
    }

    /**
     * trylock����
     */

    int trylock () {
        return pthread_mutex_trylock(&_mutex);
    }

    /**
     * ����
     */
    void unlock() {
        pthread_mutex_unlock(&_mutex);
    }

protected:

    pthread_mutex_t _mutex;
};

/**
 * @brief �̵߳�Guard
 */
class CThreadGuard
{
public:
    CThreadGuard(CThreadMutex *mutex)
    {
      _mutex = NULL;
        if (mutex) {
            _mutex = mutex;
            _mutex->lock();
        }
    }
    ~CThreadGuard()
    {
        if (_mutex) {
            _mutex->unlock();
        }
    }
private:
    CThreadMutex *_mutex;
};

/**
 * @brief  Linux�߳���������
 */
class CThreadCond : public CThreadMutex {

public:

    /**
     * ���캯��
     */
    CThreadCond() {
        pthread_cond_init(&_cond, NULL);
    }

    /**
     * ���캯��
     */
    ~CThreadCond() {
        pthread_cond_destroy(&_cond);
    }

    /**
     * �ȴ��ź�
     *
     * @param  milliseconds  �ȴ���ʱ��(��λ:ms), 0 = ���õȴ���
     */
    bool wait(int milliseconds = 0) {
        bool ret = true;

        if (milliseconds == 0) { // ���õȴ�
            pthread_cond_wait(&_cond, &_mutex);
        } else {

            struct timeval curtime;

            struct timespec abstime;
            gettimeofday(&curtime, NULL);

            int64_t us = (static_cast<int64_t>(curtime.tv_sec) *
                          static_cast<int64_t>(1000000) +
                          static_cast<int64_t>(curtime.tv_usec) +
                          static_cast<int64_t>(milliseconds) *
                          static_cast<int64_t>(1000));

            abstime.tv_sec = static_cast<int>(us / static_cast<int64_t>(1000000));
            abstime.tv_nsec = static_cast<int>(us % static_cast<int64_t>(1000000)) * 1000;
            ret = (pthread_cond_timedwait(&_cond, &_mutex, &abstime) == 0);
        }

        return ret;
    }

    /**
     * ����һ��
     */
    void signal() {
        pthread_cond_signal(&_cond);
    }

    /**
     * ��������
     */
    void broadcast() {
        pthread_cond_broadcast(&_cond);
    }

private:
    pthread_cond_t _cond;
};

}



#endif /* COND_H_ */
