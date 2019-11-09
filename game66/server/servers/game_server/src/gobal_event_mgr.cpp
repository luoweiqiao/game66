

#include "gobal_event_mgr.h"
#include "stdafx.h"
#include "game_room_mgr.h"
#include "center_log.h"
#include "robot_mgr.h"

#include "game_net_mgr.h"
#include "lobby_mgr.h"
#include "svrlib.h"
#include <iostream>
#include "game_room_mgr.h"
#include "robot_mgr.h"
#include "game_server_config.h"
#include "utility/timeFunction.h"
#include "data_cfg_mgr.h"
#include "game_room.h"
#include "center_log.h"
#include "robot_mgr.h"
#include "active_welfare_mgr.h"
#include "new_register_welfare_mgr.h"

using namespace svrlib;

namespace
{  

};
CGameSvrEventMgr::CGameSvrEventMgr()
{
	m_pReportTimer  = NULL;
}
CGameSvrEventMgr::~CGameSvrEventMgr()
{

}
bool	CGameSvrEventMgr::Init()
{
    m_pReportTimer = CApplication::Instance().MallocTimer(this,emTIMER_EVENT_REPORT);
    m_pReportTimer->StartTimer(5*1000,5*1000);// 5秒

	return true;
}
void	CGameSvrEventMgr::ShutDown()
{
	CApplication::Instance().FreeTimer(m_pReportTimer);

}
void	CGameSvrEventMgr::ProcessTime()
{
	static uint64 uProcessTime = 0;

	uint64 uTime	= getSysTime();
	uint64 uTick	= getSystemTick64();
	if (!uProcessTime)
	{
		uProcessTime = uTime;
	}
	if (uTime == uProcessTime)
	{
		return;
	}
	bool bNewDay = (diffTimeDay(uProcessTime, uTime) != 0);
	if(bNewDay)
	{
		//OnNewDay();
		CGameRoomMgr::Instance().OnNewDay();

	}
	g_RandGen.Reset(uTick);
	CRobotMgr::Instance().OnTimeTick();

	uProcessTime = uTime;
}
void	CGameSvrEventMgr::OnTimer(uint8 eventID)
{
	switch(eventID)
	{
	case emTIMER_EVENT_REPORT:
		{
            ReportInfo2Lobby();            
		}break;
	default:
		break;
	}
}
// 通用初始化
bool    CGameSvrEventMgr::GameServerInit()    
{
	// db
	if(CDBMysqlMgr::Instance().Init(GameServerConfig::Instance().DBConf) == false)
	{
		LOG_ERROR("init mysqlmgr fail ");
		return false;
	}
	CDBMysqlMgr::Instance().UpdateServerInfo();
	if(CDataCfgMgr::Instance().Init() == false) {
		LOG_ERROR("init datamgr fail ");
		return false;
	}
	if(!CGameNetMgr::Instance().Init())
	{
		LOG_ERROR("初始化网络失败");
		return false;
	}
	if(!CRedisMgr::Instance().Init(GameServerConfig::Instance().redisConf[0]))
    {
    	LOG_ERROR("redis初始化失败");
        return false;
    }	
	if(!CPlayerMgr::Instance().Init())
	{
		LOG_ERROR("playermgr init fail");
		return false;			
	} 
    if(!this->Init())
    {
        LOG_ERROR("global mgr init fail");
        return false;
    }
	if(!CCenterLogMgr::Instance().Init(CApplication::Instance().GetServerID())){
		LOG_ERROR("初始化日志Log失败");
		return false;
	}    
    if(CGameRoomMgr::Instance().Init() == false)
	{
		LOG_ERROR("初始化房间信息失败");
		return false;
	}
	if(CRobotMgr::Instance().Init() == false)
	{
		LOG_ERROR("初始化机器人管理器失败");
		return false;
	}    

    if (CAcTiveWelfareMgr::Instance().Init() == false)
    {
        LOG_ERROR("初始化活跃福利管理器失败");
        return false;
    }

	if (CNewRegisterWelfareMgr::Instance().Init() == false)
	{
		LOG_ERROR("初始化新注册玩家福利管理器失败");
		return false;
	}
	   
    CRedisMgr::Instance().ClearPlayerOnlineSvrID(CApplication::Instance().GetServerID());
	//CDBMysqlMgr::Instance().UpdateServerInfo();
	//CDBMysqlMgr::Instance().ClearBairenCount();
    return true;
}
// 通用关闭
bool    CGameSvrEventMgr::GameServerShutDown() 
{
	CRobotMgr::Instance().ShutDown();
	CGameRoomMgr::Instance().ShutDown();
	CPlayerMgr::Instance().ShutDown();
	CRedisMgr::Instance().ShutDown();
	CGameNetMgr::Instance().ShutDown();

    CDBMysqlMgr::Instance().ShutDown();
    
    return true;
}
// 通用TICK
bool    CGameSvrEventMgr::GameServerTick()
{    
    CGameNetMgr::Instance().Update();	
    this->ProcessTime();
    CDBMysqlMgr::Instance().ProcessDBEvent(); 
    
    return true;       
}
void    CGameSvrEventMgr::ReportInfo2Lobby()
{
    net::msg_report_svr_info info;
    uint32 players = 0,robots = 0;
    uint32 allcount = CPlayerMgr::Instance().GetRoomOnlines(players,robots);
    info.set_onlines(players);
    info.set_robots(robots);
    
	LOG_DEBUG("allcount:%d,players:%d,robots:%d", allcount, players, robots);

    CLobbyMgr::Instance().SendMsg2Client(&info,net::S2L_MSG_REPORT,0);
}





















