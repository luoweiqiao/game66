/*
* game_server.cpp
*
*  modify on: 2015-12-2
*      Author: toney
*/
#include "game_define.h"
#include "framework/application.h"
#include "game_net_mgr.h"
#include "dbmysql_mgr.h"
#include "data_cfg_mgr.h"

#include "svrlib.h"
#include <iostream>
#include "stdafx.h"
#include "lobby_server_config.h"
#include "async_dbcallback.h"
#include "utility/timeFunction.h"
#include "center_log.h"
#include "gobal_robot_mgr.h"
#include "utility/profile_manager.h"
#include "json/json.h"

using namespace svrlib;
using namespace std;

bool CApplication::Initialize()
{	
	defLuaConfig(getLua());
		
	// 加载lua 配置   
    if(this->call<bool>("lobby_config",m_uiServerID,&LobbyServerConfig::Instance()) == false)    
	{
		LOG_ERROR("load lobby_config fail ");
		return false;
	}
	//PROFILE_INIT();

	LOG_DEBUG("load config is:id:%d",m_uiServerID);
	// db
	if(CDBMysqlMgr::Instance().Init(LobbyServerConfig::Instance().DBConf) == false)
	{
		LOG_ERROR("init mysqlmgr fail ");
		return false;
	}
	if (CDataCfgMgr::Instance().Init() == false) {
		LOG_ERROR("init datamgr fail ");
		return false;
	}
	if(!CGameNetMgr::Instance().Init())
	{
		LOG_ERROR("初始化网络失败");
		return false;
	}
	if(!CRedisMgr::Instance().Init(LobbyServerConfig::Instance().redisConf[0]))
    {
    	LOG_ERROR("redis初始化失败");
        return false;
    }	
	if(!CPlayerMgr::Instance().Init())
	{
		LOG_ERROR("playermgr init fail");
		return false;			
	} 
    if(!CGobalEventMgr::Instance().Init())
    {
        LOG_ERROR("global mgr init fail");
        return false;
    }
    if(!CServerMgr::Instance().Init())
    {
        LOG_ERROR("服务器管理器初始化失败");
        return false;
    }
	if(!CCenterLogMgr::Instance().Init(m_uiServerID)){
		LOG_ERROR("初始化日志Log失败");
		return false;
	}
	if(!CGobalRobotMgr::Instance().Init()){
		LOG_ERROR("初始化机器人管理器失败");
		return false;
	}

    CDBMysqlMgr::Instance().SetAsyncDBCallBack(new CLobbyAsyncDBCallBack());

	CDBMysqlMgr::Instance().UpdateServerInfo();
    CDBMysqlMgr::Instance().ClearPlayerOnlineInfo(CApplication::Instance().GetServerID());

    return true;
}

void  CApplication::ShutDown()
{
	CGobalEventMgr::Instance().SaveAllPlayerAndLoginOut();
	CPlayerMgr::Instance().ShutDown();
	CRedisMgr::Instance().ShutDown();
	//CGameNetMgr::Instance().ShutDown();

    CDBMysqlMgr::Instance().ShutDown();

	//PROFILE_SHUTDOWN("lobby",GetServerID());
}

/**
* 本函数将在程序启动时和每次配置改变时被调用。
* 第一次调用将在Initialize()之前
*/
void CApplication::ConfigurationChanged()
{
	// 重加载配置
	LOG_ERROR("configuration changed");
    
    CDataCfgMgr::Instance().Reload();
}
void CApplication::Tick()
{
	int64 tick1 = getTickCount64();
	int64 tick2 = 0;
	CGameNetMgr::Instance().Update();
	tick2 = getTickCount64();
	if((tick2-tick1)>100){
		LOG_ERROR("network cost time:%lld",tick2-tick1);
	}

    CGobalEventMgr::Instance().ProcessTime();
    CDBMysqlMgr::Instance().ProcessDBEvent();

	uint32 redisCount = CRedisMgr::Instance().GetOperCount();
	if(redisCount > 100){
		LOG_ERROR("redis opercount:%d",redisCount);
	}
	CRedisMgr::Instance().ClearOperCount();

}

int main(int argc, char * argv[])
{
	return FrameworkMain(argc, argv);
}
