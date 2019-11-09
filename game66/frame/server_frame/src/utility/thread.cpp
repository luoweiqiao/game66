

#include "utility/thread.h"
#include <unistd.h>

void Thread::sleep(int millis)
{
	usleep(millis*1000);
}

ThreadID Thread::getId()
{
	return pthread_self();
}
Thread::~Thread()
{
}
void Thread::start()
{
	pthread_create(&p_thread,0,runThread,this);
}

//等待线程完成
void Thread::join()
{
	pthread_join(p_thread,NULL);
}
//立即杀死线程，不建议使用，可能引起内存泄漏
void Thread::kill()
{
	pthread_cancel(p_thread);
}
void* runThread(void* p)
{
    Thread *thread=(Thread*)p;
    thread->run();
    return 0;
}


