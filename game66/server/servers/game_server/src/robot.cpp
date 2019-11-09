//
// Created by toney on 16/5/18.
//

#include "robot.h"
#include "stdafx.h"
#include "game_room.h"
#include "data_cfg_mgr.h"

using namespace svrlib;
using namespace std;

CGameRobot::CGameRobot()
:CGamePlayer(PLAYER_TYPE_ROBOT)
{
    m_loginInScore = 0;
    m_loginInCoin  = 0;
    m_leaveTime  = getSysTime() + g_RandGen.RandRange(SECONDS_IN_MIN*10,SECONDS_IN_30_MIN);
    m_leaveRound = g_RandGen.RandRange(20,30);
}
CGameRobot::~CGameRobot()
{

}
void 	CGameRobot::OnTimeTick(uint64 uTime,bool bNewDay)
{
    if(m_coolBlocker.isTimeOut()){
        ClearBlocker();
        ClearBlockerIP();
        m_coolBlocker.beginCooling(1000*300);
    }
}
void  	CGameRobot::OnLogin()
{
    CGamePlayer::OnLogin();
    m_loginInScore = GetAccountValue(emACC_VALUE_SCORE);
    m_loginInCoin  = GetAccountValue(emACC_VALUE_COIN);
    m_leaveTime  = getSysTime() + g_RandGen.RandRange(SECONDS_IN_MIN*10,SECONDS_IN_30_MIN);
    if(CCommonLogic::IsBaiRenGame(CDataCfgMgr::Instance().GetCurSvrCfg().gameType)){
        m_leaveRound = g_RandGen.RandRange(20,30);
    }else{
        m_leaveRound = g_RandGen.RandRange(20,30);
    }

	//时时彩的夺宝机器人
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		m_leaveTime = getSysTime() + g_RandGen.RandRange(SECONDS_IN_MIN * 5, SECONDS_IN_MIN * 10);

		LOG_DEBUG("robot_leave_time - uid:%d,m_leaveTime:%d,getSysTime:%lld", GetUID(), m_leaveTime, getSysTime());

	}

    LOG_ERROR("rob_en_svr - uid:%d,lv:%d-%d,score:%lld,coin:%lld",
		GetUID(),GetLvScore(),GetLvCoin(),m_loginInScore,m_loginInCoin);
}

void 	CGameRobot::OnGameEnd()
{
    m_leaveRound--;
}
// 是否需要回收
bool 	CGameRobot::NeedRecover()
{

    return false;
}
bool    CGameRobot::NeedBackPool()
{
    if(m_pGameTable != NULL)
    {
		if (m_pGameTable->CanLeaveTable(this))
		{
			if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR) {
				//LOG_DEBUG("维护状态，归队");
				LOG_DEBUG("the server is repair,robot uid:%d need back pool.", GetUID());
				return true;
			}
			int32 dwRemaindTime = CDataCfgMgr::Instance().GetRobotBatchOut(GetBatchID());
			if (dwRemaindTime == 0)
			{
				LOG_DEBUG("robot_leave_time_arrive - uid:%d,batchID:%d,dwRemaindTime:%d", GetUID(), GetBatchID(), dwRemaindTime);
				return true;
			}
			if (m_pGameTable->GetPlayerCurScore(this) < m_pGameTable->GetEnterMin())// 没金币了
			{
				//LOG_DEBUG("金币不足归队:cur:%lld--min:%lld",m_pGameTable->GetPlayerCurScore(this),m_pGameTable->GetEnterMin());
				LOG_DEBUG("robot score lack - uid:%d,cur_score:%lld,min_enter_score:%lld", GetUID(), m_pGameTable->GetPlayerCurScore(this), m_pGameTable->GetEnterMin());
				return true;
			}
			if (GetAccountValue(emACC_VALUE_SCORE) > (m_loginInScore * 2) || GetAccountValue(emACC_VALUE_COIN) > (m_loginInCoin * 2)) {
				//LOG_DEBUG("盈利超过2倍归队");
				LOG_DEBUG("earnings more than doubled,robot uid:%d need back pool.", GetUID());
				return true;
			}
			if (getSysTime() > m_leaveTime)
			{
				LOG_DEBUG("time to go,robot uid:%d need back pool.", GetUID());
				return true;
			}
			if (m_leaveRound <= 0)
			{
				LOG_DEBUG("m_leaveRound is 0,robot uid:%d need back pool.", GetUID());
				return true;
			}
			if (g_RandGen.RandRatio(30, PRO_DENO_100))
			{
				int64 lCurCoin = GetAccountValue(emACC_VALUE_COIN) * 10;
				int64 lLoginCoin = m_loginInCoin * 17;
				if (lCurCoin> lLoginCoin)
				{
					//LOG_DEBUG("盈利超过1.4倍归队:%d",GetUID());
					LOG_DEBUG("pro -- earnings more than doubled,robot need back lobby. -  uid:%d,lCurCoin:%lld,lLoginCoin:%lld", GetUID(), lCurCoin, lLoginCoin);
					return true;
				}
			}
        }
    }
	else
	{
        if(m_pGameRoom != NULL && !m_pGameRoom->IsJoinMarry(this)){
			LOG_DEBUG("it is on the room and not on the table,robot uid:%d need back pool.", GetUID());
            //LOG_DEBUG("在房间不在桌子上,不在排队，归队");
            return true;
        }
    }

    return false;
}



bool    CGameRobot::NeedBackLobby()
{
	if (m_pGameTable != NULL)
	{
		return false;
	}
	int32 dwRemaindTime = CDataCfgMgr::Instance().GetRobotBatchOut(GetBatchID());
	if (dwRemaindTime == 0)
	{
		LOG_DEBUG("robot_leave_time_arrive - uid:%d,batchID:%d,dwRemaindTime:%d", GetUID(), GetBatchID(), dwRemaindTime);
		return true;
	}
    if(IsBankrupt()){
        //LOG_DEBUG("破产回归大厅:%d",GetUID());
		LOG_DEBUG("bankrupt,robot uid:%d need back lobby.", GetUID());
        return true;
    }
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR) {
        //LOG_DEBUG("维护状态，回归大厅");
		LOG_DEBUG("the server is repair,robot uid:%d need back lobby.", GetUID());
        return true;
    }
    if(GetAccountValue(emACC_VALUE_SCORE) > (m_loginInScore*2) || GetAccountValue(emACC_VALUE_COIN) > (m_loginInCoin*2)){
        //LOG_DEBUG("盈利超过2倍归队:%d",GetUID());
		LOG_DEBUG("earnings more than doubled,robot uid:%d need back lobby.", GetUID());
        return true;
    }
    if(getSysTime() > m_leaveTime){
        //LOG_DEBUG("离开游戏时间已到:%d",GetUID(), m_leaveTime);
		LOG_DEBUG("time to go,robot uid:%d need back lobby.", GetUID());
        return true;
    }
	if (m_leaveRound <= 0) {
		LOG_DEBUG("m_leaveRound is 0,robot uid:%d need back pool.", GetUID());
		return true;
	}
	if (g_RandGen.RandRatio(30, PRO_DENO_100))
	{
		int64 lCurCoin = GetAccountValue(emACC_VALUE_COIN) * 10;
		int64 lLoginCoin = m_loginInCoin * 17;
		if (lCurCoin > lLoginCoin)
		{
			//LOG_DEBUG("盈利超过1.4倍归队:%d",GetUID());
			LOG_DEBUG("pro -- earnings more than doubled,robot need back lobby. -  uid:%d,lCurCoin:%lld,lLoginCoin:%lld", GetUID(), lCurCoin, lLoginCoin);
			return true;
		}
	}
    return false;
}
// 是否破产
bool    CGameRobot::IsBankrupt()
{
    /*if(m_loginInScore >= 4L && GetAccountValue(emACC_VALUE_SCORE) < (int64)(m_loginInScore/4)){
        LOG_DEBUG("积分破产uid:%d :%lld-->%lld",GetUID(),m_loginInScore,GetAccountValue(emACC_VALUE_SCORE));
        return true;
    }*/

	int robot_batchid = GetBatchID();
	uint32 robot_uid = GetUID();
	stRobotOnlineCfg robotCfg;
	CDataCfgMgr::Instance().GetRobotGameCfg(robot_batchid, robotCfg);

	if (robotCfg.leveltype == net::ROOM_CONSUME_TYPE_COIN)
	{
		if (m_loginInCoin >= 4L && GetAccountValue(emACC_VALUE_COIN) < (int64)(m_loginInCoin / 4))
		{
			LOG_DEBUG("robot_bankrupt_coin - uid:%d,m_loginInCoin:%lld,cur_score:%lld", GetUID(), m_loginInCoin, GetAccountValue(emACC_VALUE_COIN));
			return true;
		}
	}
	else if (robotCfg.leveltype == net::ROOM_CONSUME_TYPE_SCORE)
	{
		if (m_loginInScore >= 4L && GetAccountValue(emACC_VALUE_SCORE) < (int64)(m_loginInScore / 4))
		{
			LOG_DEBUG("robot_bankrupt_score - uid:%d,m_loginInScore:%lld,cur_score:%lld",
				GetUID(), m_loginInScore, GetAccountValue(emACC_VALUE_SCORE));
			return true;
		}
	}
	else
	{
		if (m_loginInCoin >= 4L && GetAccountValue(emACC_VALUE_COIN) < (int64)(m_loginInCoin / 4))
		{
			LOG_DEBUG("robot_bankrupt_coin_2 - uid:%d,m_loginInCoin:%lld,cur_score:%lld", GetUID(), m_loginInCoin, GetAccountValue(emACC_VALUE_COIN));
			return true;
		}
	}
	//LOG_DEBUG("robot_enter_ing - uid:%d,batchid:%d,dwRemaindTime:%d,bGetCfg:%d,gameType:%d,robotType:%d,rrid,%d,leveltype:%d,r_size:%d - %d",
	//	robot_uid, robot_batchid, dwRemaindTime, bGetCfg, gameType, robotCfg.gameType, robotCfg.roomID, robotCfg.leveltype, vecRoomsCoin.size(), vecRoomsScore.size());


    return false;
}

// 回归队列
void    CGameRobot::BackPool()
{
    //LOG_DEBUG("机器人回归队列:%d",GetUID());
	LOG_DEBUG("robot back pool - uid:%d", GetUID());
    if(m_pGameTable != NULL)
    {
        m_pGameTable->SetRobotEnterCool(5000);
		LOG_DEBUG("play_leave - uid:%d", GetUID());
		if (m_pGameTable->LeaveTable(this) == false)
		{
			return;
		}
    }
    if(m_pGameRoom != NULL)
    {
		if (m_pGameRoom->LeaveRoom(this) == false)
		{
			return;
		}
    }

}




