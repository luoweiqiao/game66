/*
 * application.h
 *
 *  Created on: 2011-11-14
 *      Author: toney
 */

#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include "utility/timer_mgr.h"
#include "fundamental/noncopyable.h"
#include "luatinker/luaConsole.h"
#include "utility/timeFunction.h"
#include "utility/singleton.h"

#include <map>

using namespace svrlib;

extern int FrameworkMain(int argc, char* argv[]);

class CApplication : public CLuaConsole,public AutoDeleteSingleton<CApplication>
{
public:
	bool PreInit()
	{
		return m_timermgr.Init();
	}
	void OverShutDown()
	{
		m_timermgr.ShutDown();
	}
	void PreTick()
	{
		// 驱动时钟
		setSystemTick64();// 更新tick	
		setSysTime();		
		m_timermgr.Tick();
	}	
    bool Initialize();
	void ShutDown();

    CTimer* MallocTimer(ITimerSink* pSink,uint8 eventID)
    {
        return m_timermgr.MallocTimer(pSink,eventID);
    }
    void    FreeTimer(CTimer* pTimer)
    {
        return m_timermgr.FreeTimer(pTimer);
    }
    
    void 	ConfigurationChanged();	

	void	Tick();
		
	void  	SetServerID(unsigned int svrid){ m_uiServerID = svrid; }
	uint32  GetServerID(){ return m_uiServerID; }
	void 	SetStatus(uint8 status){ m_status = status; }
	uint8 	GetStatus(){ return m_status; }
	CApplication(){
		m_status = 0;
	}
    ~CApplication(){
		m_status = 0;
    }
private:
 	CWheelTimerMgr	 m_timermgr;
	unsigned int 	 m_uiServerID;
	uint8 			 m_status;	// 服务器状态
};

#endif /* CAPPLICATION_H */






