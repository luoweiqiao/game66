

#ifndef TIMER_MGR_H__
#define TIMER_MGR_H__

#include "utility/basicTypes.h"
#include "utility/stl_hash_map.h"
#include "network/timerbase.h"

#include <vector>


extern "C"
{
#include "stw_timer.h"
}

using namespace std;
using namespace svrlib;

// 定时器的轮数跟精度
#define STW_NUMBER_BUCKETS     ( 1024 ) // (32~4096)
#define STW_RESOLUTION         ( 10 ) 

class  CWheelTimerMgr;
class  CTimer
{
public:
	CTimer()
	{
		memset(this,0,sizeof(CTimer));
	}
	~CTimer()
	{
		StopTimer();
	}
    bool StartTimer(uint32 delay,uint32 periodic_delay);
    bool StartDayTimer(uint32 tm_hour,uint32 tm_min,uint32 tm_sec);
    bool StopTimer();
    
public:	
	stw_tmr_t  	    stw_tmr;
	uint8	   	    eventID;	
	ITimerSink*     pSink;
    CWheelTimerMgr* pTimerMgr;
	bool 			isRuning;
};

// 长时间定时器，会有一定的误差，主要优点是插入删除速度快
class CWheelTimerMgr
{
public:
	CWheelTimerMgr();
	~CWheelTimerMgr();

	bool   Init();
	void   ShutDown();
	void   Tick();

    CTimer* MallocTimer(ITimerSink* pSink,uint8 eventID);
    void    FreeTimer(CTimer* pTimer);
    
	// 添加定时器	
	bool   AddTimer(uint32 delay,uint32 periodic_delay,CTimer* pTimer);
	bool   KillTimer(CTimer* pTimer);
    
private:


private:
	uint64  	m_lastTick;
	stw_t*  	stw_sys_handle;

};












#endif // TIMER_MGR_H__


