

#include "utility/timer_mgr.h"
#include "utility/timeFunction.h"
#include <time.h>

extern "C"
{
#include "utility/stw_timer.h"
}

using namespace std;
using namespace svrlib;

void	 CallBack(stw_tmr_t *tmr, void *parm)
{
	CTimer* pTimer = (CTimer*)parm;
	if(pTimer->pSink)
	{
		pTimer->pSink->OnTimer(pTimer->eventID);
	}	
}

bool CTimer::StartTimer(uint32 delay,uint32 periodic_delay)
{
	StopTimer();
    isRuning = this->pTimerMgr->AddTimer(delay,periodic_delay,this);
    return true;
}
bool CTimer::StartDayTimer(uint32 tm_hour,uint32 tm_min,uint32 tm_sec)
{
	time_t now = getSysTime();      
	struct tm* mytime = localtime( &now );    
	uint32 nNowSecond = mytime->tm_sec + 60 * mytime->tm_min +3600 * mytime->tm_hour;
	uint32 nStartSecond = tm_hour*60*60 + tm_min*60 + tm_sec;	
	uint32 SECONDS_IN_ONE_DAY = 24*3600;
	uint32 diffTime = 0;
	if(nStartSecond > nNowSecond)
	{        
		diffTime = nStartSecond - nNowSecond;
	}else{        
		diffTime = (SECONDS_IN_ONE_DAY - nNowSecond) + nStartSecond;	
	}	    
	return this->StartTimer(diffTime*1000,SECONDS_IN_ONE_DAY*1000);
}
bool CTimer::StopTimer()
{
	if(isRuning) {
		this->pTimerMgr->KillTimer(this);
		isRuning = false;
	}
    return true;
}
    
CWheelTimerMgr::CWheelTimerMgr()
:m_lastTick(0)
,stw_sys_handle(0)
{
}
CWheelTimerMgr::~CWheelTimerMgr()
{
}
bool   CWheelTimerMgr::Init()
{
   int iRet = CSTWTimer::stw_timer_create(STW_NUMBER_BUCKETS, 
                                          STW_RESOLUTION, 
                                          "Game Timer Wheel",
                                          &stw_sys_handle); 	
   
	if(iRet != RC_STW_OK)
	{
		return false;
	}
	
	m_lastTick = getTickCount64();	
	return true;
}
void   CWheelTimerMgr::ShutDown()
{
	CSTWTimer::stw_timer_destroy(stw_sys_handle);	
}
void   CWheelTimerMgr::Tick()
{
	uint64 curTime = getTickCount64();
	while(curTime > (m_lastTick + STW_RESOLUTION))
	{
		m_lastTick += STW_RESOLUTION;
		if(stw_sys_handle)
		{
        	CSTWTimer::stw_timer_tick(stw_sys_handle);
    	}
	}
}
CTimer* CWheelTimerMgr::MallocTimer(ITimerSink* pSink,uint8 eventID)
{
    CTimer* pTimer = new CTimer();
	pTimer->eventID     = eventID;
	pTimer->pSink       = pSink;		
    pTimer->pTimerMgr   = this;
    
	stw_tmr_t* pTmr = &pTimer->stw_tmr;
	CSTWTimer::stw_timer_prepare(pTmr);

    return pTimer;
}
void    CWheelTimerMgr::FreeTimer(CTimer* pTimer)
{
	if(pTimer == NULL)
		return;
    KillTimer(pTimer);
	delete pTimer;
	pTimer = NULL;		
}
bool   CWheelTimerMgr::AddTimer(uint32 delay,uint32 periodic_delay,CTimer* pTimer)
{	
	stw_tmr_t* pTmr = &pTimer->stw_tmr;
	CSTWTimer::stw_timer_prepare(pTmr);
	
	int32 ret = CSTWTimer::stw_timer_start(stw_sys_handle, pTmr, delay, periodic_delay, CallBack, pTimer);
    if(ret == RC_STW_OK)
    {  
	    return true;
    }    
    return false;
}
bool   CWheelTimerMgr::KillTimer(CTimer* pTimer)
{
    CSTWTimer::stw_timer_stop(stw_sys_handle, &pTimer->stw_tmr);
	return true;
}



