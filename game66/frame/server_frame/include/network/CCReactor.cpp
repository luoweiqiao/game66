#include <assert.h>
#include "mempool.h"
#include "memcheck.h"
#include "CCReactor.h"

CMemPool* _memPool = NULL;

CCReactor::CCReactor()
{

}

CCReactor::~CCReactor()
{

}

int CCReactor::Init(int maxpoller)
{
	this->_maxpoller = maxpoller;

	NEW (CPollerUnit(_maxpoller), _pollerunit);
	assert (_pollerunit);
	
	if(_pollerunit->InitializePollerUnit() < 0)
    {
        printf("poller unit init failed");
		abort();
    }

    NEW(CTimerMng(), _timer_mng);
    assert(_timer_mng);

	NEW( CMemPool(), _memPool);
	assert (_memPool);

	return 0;
}

void CCReactor::Update()
{
    _pollerunit->WaitPollerEvents(0);
    _pollerunit->ProcessPollerEvents();
    _timer_mng->check_expired();
}

 int CCReactor::RegistServer(ICC_TCP_Server* server) 
 {
	return this->AttachPoller (server);
 }

 int CCReactor::RegistClient(NetworkObject* client)
 {
	return this->AttachPoller (client);
 }

int CCReactor::AttachPoller (CPollerObject* poller)
{
    if (poller->Init() != 0)
	{
		printf("poller Attach failed.");
		return -1;
	}
	return poller->AttachPoller(this->_pollerunit);
}

bool CCReactor::StartTimer(CTimerNotify* timerable, uint32_t interval)
{
    return _timer_mng->start_timer(timerable, interval);
}

void CCReactor::StopTimer(CTimerNotify* timerable)
{
    _timer_mng->stop_timer(timerable);
}
