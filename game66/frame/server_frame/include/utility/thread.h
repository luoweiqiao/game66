
#ifndef THREAD_H_20110719
#define THREAD_H_20110719


#include <pthread.h>
#include "singleton.h"

//共享锁
class TLock
{
public :
    pthread_mutex_t 	m_Mutex; 

    TLock(){ pthread_mutex_init( &m_Mutex , NULL );} ;
    ~TLock(){ pthread_mutex_destroy( &m_Mutex) ; } ;
    void	Lock(){ pthread_mutex_lock(&m_Mutex); } ;
    void	Unlock(){ pthread_mutex_unlock(&m_Mutex); } ;
};

//自动加锁解锁器
class AutoLock_T
{
public:
    AutoLock_T(TLock& rLock)
    {
        m_pLock = &rLock;
        m_pLock->Lock();
    }
    ~AutoLock_T()
    {
        m_pLock->Unlock();
    }
protected:
private:
    AutoLock_T();
    TLock* m_pLock;
};

class TCond 
{
public:
	inline TCond()
	{
		pthread_cond_init(&cond_, NULL);
	}
	inline ~TCond()
	{
		pthread_cond_destroy(&cond_);
	}

	inline void Signal()
	{
		pthread_cond_signal(&cond_);
	}
	inline void Broadcast()
	{
		pthread_cond_broadcast(&cond_);
	}
	inline void Wait(TLock* lock)
	{
		pthread_cond_wait(&cond_, &lock->m_Mutex);
	}
	inline bool Wait(TLock* lock, int seconds,int msec)
	{
		timespec tv;
		tv.tv_nsec = msec*1000000;
		tv.tv_sec = seconds;
		if(pthread_cond_timedwait(&cond_, &lock->m_Mutex, &tv) == 0)
			return true;
		else
			return false;
	}

private:
	pthread_cond_t cond_;

};

class RWLock : private NonCopyableClass
{
public:
	RWLock() { ::pthread_rwlock_init(&rwlock, NULL); }
	~RWLock() { ::pthread_rwlock_destroy(&rwlock); }
	void rdlock() { ::pthread_rwlock_rdlock(&rwlock); } 
	void wrlock() { ::pthread_rwlock_wrlock(&rwlock); }
	void unlock() { ::pthread_rwlock_unlock(&rwlock); } 

private:
	pthread_rwlock_t rwlock;
};

template <bool lock = true>
class RWLocker
{
private:
	RWLock rwlock;

public:
	void rdlock() { rwlock.rdlock(); }
	void wrlock() { rwlock.wrlock(); }
	void unlock() { rwlock.unlock(); }
};

template <>
class RWLocker<false>
{
public:
	void rdlock() { }
	void wrlock() { }
	void unlock() { }
};


typedef pthread_t ThreadID;
void* runThread(void* p);

class Thread
{
public:
    virtual ~Thread();//为了让Thread的子类能正确析构
	virtual void run()=0;
	void        start();
	//等待线程完成
	void        join();
	//立即杀死线程，不建议使用，可能引起内存泄漏
	void        kill();
protected:
    void        sleep(int millis);
    ThreadID    getId();
private:
    ThreadID p_thread;
	
};



#endif //THREAD_H_20110719


