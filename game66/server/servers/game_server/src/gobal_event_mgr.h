
#ifndef GOBAL_EVENT_MGR_H__
#define GOBAL_EVENT_MGR_H__

#include "svrlib.h"
#include "game_define.h"


using namespace std;
using namespace svrlib;


enum   emTIMER_EVENT_ID
{
	emTIMER_EVENT_REPORT = 1,    // 上报在线人数

};

class CGameSvrEventMgr : public ITimerSink,public AutoDeleteSingleton<CGameSvrEventMgr>
{
public:
	CGameSvrEventMgr();
	~CGameSvrEventMgr();

	bool	Init();
	void	ShutDown();
	void	ProcessTime();

	void	OnTimer(uint8 eventID);


    // 通用初始化
    bool    GameServerInit();    
    // 通用关闭
    bool    GameServerShutDown();    
    // 通用TICK
    bool    GameServerTick();
    
    void    ReportInfo2Lobby();


    
private:
    CTimer* m_pReportTimer;


};











































#endif // GOBAL_EVENT_MGR_H__


