#ifndef  _CCREACTOR_H_
#define  _CCREACTOR_H_

#include "poller.h"
#include "svrlib.h"
#include "ICC_TCP_Server.h"
#include "NetworkObject.h"
#include "heap_timer.h"
#define MAX_POLLER 102400


class CCReactor : public AutoDeleteSingleton<CCReactor>
{
public:
    int Init(int maxpoller = MAX_POLLER);
    void Update();
    int RegistServer(ICC_TCP_Server* server);
    int RegistClient(NetworkObject* client);

    int AttachPoller(CPollerObject* poller);

    bool StartTimer(CTimerNotify* timerable, uint32_t interval);

    void StopTimer(CTimerNotify* timerable);
public:

    CCReactor();
    virtual ~CCReactor();

    CPollerUnit*    _pollerunit;
    CTimerMng*      _timer_mng;
    int _maxpoller;
};

#endif
