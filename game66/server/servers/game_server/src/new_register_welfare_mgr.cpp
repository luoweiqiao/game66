//
// Created by AAA on 2019-03-04 新注册玩家管理类
//

#include "new_register_welfare_mgr.h"
#include "stdafx.h"
#include "data_cfg_mgr.h"
#include "game_define.h"

using namespace std;

CNewRegisterWelfareMgr::CNewRegisterWelfareMgr()
{	
}
CNewRegisterWelfareMgr::~CNewRegisterWelfareMgr()
{	
}

bool CNewRegisterWelfareMgr::Init()
{
    //启动时加载新注册玩家福利配置表
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadNewRegisterWelfareCfg(m_mpNrwCfg);
    if (!bRet)
    {
        return false;
    }  
    return true;
}

//加载配置数据
bool	CNewRegisterWelfareMgr::ReLoadCfgData()
{
    //重新加载新注册玩家福利配置表
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadNewRegisterWelfareCfg(m_mpNrwCfg);
    if (!bRet)
    {
        return false;
    }
    return true;
}

bool    CNewRegisterWelfareMgr::IsExistGame(uint8 game_id)
{    
    set<uint8>::iterator iter = m_mpNrwCfg.game_list.find(game_id);
   
    // 判断game ID是否存在
    if (iter!= m_mpNrwCfg.game_list.end())
    {        
		return true;
    }	
	return false;  
}

// 记录新注册玩家福利控制的牌局信息
void   CNewRegisterWelfareMgr::WriteNewRegisterWelfareLog(uint32 uid, uint8 game_id, uint32 curr_must_win, uint32 curr_total_win, uint64 curr_win_coin, uint64 total_win_coin, uint64 register_time)
{
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if (pGamePlayer == NULL)
    {
        LOG_ERROR("WriteNewRegisterWelfareLog - get user info fail. uid:%", uid);
        return;
    }
		
	uint8 status = 0;

	//判断局数是否满足
	if (m_mpNrwCfg.total_win <= 0)
	{
		if (curr_must_win >= m_mpNrwCfg.must_win)
		{		
			status = 1;
		}
	}
	else
	{
		if (curr_total_win >= m_mpNrwCfg.total_win)
		{
			status = 1;
		}
	}
	
	//判断有效期
	uint64 curr_time = getSysTime();
	if (curr_time > (SECONDS_IN_ONE_DAY * m_mpNrwCfg.period_day + register_time))
	{
		status = 1;
	}

	LOG_DEBUG("WriteNewRegisterWelfareLog - uid:%d game_id:%d curr_must_win:%d curr_total_win:%d curr_win_coin:%d total_win_coin:%d status:%d register_time:%lld curr_time:%lld",
		uid, game_id, curr_must_win, curr_total_win, curr_win_coin, total_win_coin, status, register_time, curr_time);
	CDBMysqlMgr::Instance().InsertNewRegisterWelfareLog(uid, game_id, curr_must_win, curr_total_win, curr_win_coin, total_win_coin, status, curr_time);
    
}