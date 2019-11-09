//
// Created by toney on 16/5/22.
//

#include <server_mgr.h>
#include "gobal_robot_mgr.h"
#include "stdafx.h"
#include "player.h"
#include "data_cfg_mgr.h"
#include "game_define.h"
#include "robot.h"

using namespace std;


CGobalRobotMgr::CGobalRobotMgr()
{
    m_checkLogin.beginCooling(20000);
	m_checkLoginEx.beginCooling(10000);
	m_iRobotCityCount = 0;
}
CGobalRobotMgr::~CGobalRobotMgr()
{

}
bool	CGobalRobotMgr::Init()
{
    m_checkLogin.beginCooling(20000);
	m_checkLoginEx.beginCooling(10000);
    LoadRobotPayPoolData();

    CDBMysqlMgr::Instance().ClearRobotLoginState();

    //CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRobotOnlineCfg(m_mpRobotCfg);

	while (strcmp("", g_robot_city_tables[m_iRobotCityCount].c_str())!=0)
	{
		m_iRobotCityCount++;
	}

    return true;
}


void	CGobalRobotMgr::ShutDown()
{

}
void	CGobalRobotMgr::OnTimeTick()
{
    if(m_checkLogin.isTimeOut())
	{
        CheckRobotLogin();
        m_checkLogin.beginCooling(SECONDS_IN_MIN*3*1000);
    }
	if (m_checkLoginEx.isTimeOut())
	{
		CheckTimeRobotLogin();
		m_checkLoginEx.beginCooling(60000);
	}
}


// 尝试登陆一个机器人
bool    CGobalRobotMgr::LoginRobot(stRobotCfg & refCfg)
{
    CPlayer* pRobot = (CPlayer*)CPlayerMgr::Instance().GetPlayer(refCfg.uid);
	if (pRobot != NULL)
	{
		LOG_DEBUG("robot_login_error - gameType:%d,uid:%d,richLv:%d,batchID:%d,logintype:%d", refCfg.gameType, refCfg.uid, refCfg.richLv, refCfg.batchID,refCfg.loginType);
		return false;
	}

    return AddRobot(refCfg);
}

bool    CGobalRobotMgr::LoginTimeRobot(stRobotCfg & refCfg)
{
	CPlayer* pRobot = (CPlayer*)CPlayerMgr::Instance().GetPlayer(refCfg.uid);
	if (pRobot != NULL)
	{
		LOG_DEBUG("robot_login_error - gameType:%d,uid:%d,richLv:%d,batchID:%d,logintype:%d", refCfg.gameType, refCfg.uid, refCfg.richLv, refCfg.batchID, refCfg.loginType);
		return false;
	}
	return AddRobot(refCfg);
}

bool    CGobalRobotMgr::AddRobot(stRobotCfg & refCfg)
{

	LOG_DEBUG("robot_login_gameType:%d,uid:%d,richLv:%d,batchID:%d,logintype:%d", refCfg.gameType, refCfg.uid, refCfg.richLv, refCfg.batchID, refCfg.loginType);

    CLobbyRobot* pRobot = new CLobbyRobot();
    pRobot->SetSession(NULL);
    pRobot->SetUID(refCfg.uid);
	pRobot->SetBatchID(refCfg.batchID);
	int iRandRobotCityIndex = g_RandGen.RandRange(0, m_iRobotCityCount - 1);
	pRobot->SetCity(g_robot_city_tables[iRandRobotCityIndex]);
	//pRobot->SetCity("中国");

    CPlayerMgr::Instance().AddPlayer(pRobot);
    pRobot->OnLogin();
    pRobot->SetRobotCfg(refCfg);


	//LOG_DEBUG("robot_city_info - uid:%d,m_iRobotCityCount:%d,iRandRobotCityIndex:%d,tables:%s,city:%s", refCfg.uid, m_iRobotCityCount, iRandRobotCityIndex, g_robot_city_tables[iRandRobotCityIndex].c_str(), pRobot->GetCity().c_str());

	//COUNT_OCCUPY_MEM_SZIE(this);

    return true;
}

// 检测新机器人上线
void    CGobalRobotMgr::CheckRobotLogin()
{
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return;
	}

    //LOG_DEBUG("check robot login");
    tm local_time;
    uint64 uTime	= getTime();
    getLocalTime( &local_time, uTime );
	
	
	map<uint32, vector<stRobotOnlineCfg>> & mpRobotCfg = CDataCfgMgr::Instance().GetRobotAllDayLoadCount();
	
	map<uint32, stRobotOnlineCfg>  mpRobotOnLineCount;
	uint32 allOnlineRobotCount = GetOnLineRobotCount(mpRobotOnLineCount);

	LOG_DEBUG("server_cfg_robot -allOnlineRobotCount:%d, mpRobotCfg.size:%d", allOnlineRobotCount, mpRobotCfg.size());
	for (auto iter_loop = mpRobotCfg.begin(); iter_loop != mpRobotCfg.end(); iter_loop++)
    {
		uint32 gameType = iter_loop->first;
		vector<stRobotOnlineCfg> & refVecCfg = iter_loop->second;
		if (refVecCfg.size() == 0)
		{
			LOG_DEBUG("not_cfg_robot - gameType:%d,size:%d", gameType, mpRobotCfg.size());
			continue;
		}
		bool bLoadRobotStatus = CheckLoadRobotStatus(gameType);

		if (bLoadRobotStatus == true)
		{
			bool bRetSubLoadIndex = true;//CDataCfgMgr::Instance().SubCfgRobotLoadindex(gameType);
			LOG_DEBUG("load_status_error - gameType:%d,bRetSubLoadIndex:%d,size:%d", gameType, bRetSubLoadIndex, mpRobotCfg.size());

			continue;
		}
		//if (refVecCfg.size() > 1)
		//{
		//	std::sort(std::begin(refVecCfg), std::end(refVecCfg), [](const auto &a, const auto &b) { return a.iLoadindex > b.iLoadindex; });
		//}
		int minLoadIndex = refVecCfg[0].iLoadindex;
		uint32 curLoadIndex = 0;
		string strVecCfg;
		for (uint32 i = 0; i < refVecCfg.size(); i++)
		{
			auto & refCfg = refVecCfg[i];
			if (refCfg.iLoadindex < minLoadIndex)
			{
				minLoadIndex = refCfg.iLoadindex;
				curLoadIndex = i;
			}
			strVecCfg += CStringUtility::FormatToString("i:%d,batchID:%d,iLoadindex:%d ", i, refCfg.batchID, refCfg.iLoadindex);
		}

		stRobotOnlineCfg & refCfg = refVecCfg[curLoadIndex];
		refCfg.iLoadindex++;
		if (refCfg.iLoadindex >= 0x7FFFFFFF)
		{
			refCfg.iLoadindex = 0;
		}
		if (CServerMgr::Instance().IsOpenRobot(refCfg.gameType) == 0)
		{
			LOG_DEBUG("server_not_open_robot - batchID:%d,gameType:%d", refCfg.batchID, refCfg.gameType);
			continue;
		}
		LOG_DEBUG("load_robot - refVecCfg.size:%d,gameType:%d,batchID:%d,iLoadindex:%d,minLoadIndex:%d,curLoadIndex:%d,strVecCfg.c_str:%s",
			refVecCfg.size(), refCfg.gameType, refCfg.batchID, refCfg.iLoadindex, minLoadIndex, curLoadIndex, strVecCfg.c_str());
        for(uint8 lv=0;lv<ROBOT_MAX_LEVEL;++lv)
        {
            uint32 cfgRobotCount = refCfg.onlines[lv];
			LOG_DEBUG("start_load - gameType:%d,lv:%d,cfgRobotCount:%d,batchID:%d", refCfg.gameType, lv, cfgRobotCount, refCfg.batchID);
			if (cfgRobotCount == 0)
			{
				continue;
			}
			//map<uint32, stRobotOnlineCfg>  mpBatchCount;
			//uint32 onlineRobotCount = 0;
			//uint32 levelRobotCount = GetRobotCountByBatchID(refCfg.gameType, lv + 1, mpBatchCount);
			//auto find_batchCount = mpBatchCount.find(refCfg.batchID);
			//if (find_batchCount != mpBatchCount.end())
			//{
			//	auto & tempRobotOnlineCfg = find_batchCount->second;
			//	onlineRobotCount = tempRobotOnlineCfg.onlines[lv];
			//}

			uint32 onlineRobotCount = 0;
			auto find_robotOnLineCount = mpRobotOnLineCount.find(refCfg.batchID);
			if (find_robotOnLineCount != mpRobotOnLineCount.end())
			{
				auto & tempRobotOnlineCfg = find_robotOnLineCount->second;
				onlineRobotCount = tempRobotOnlineCfg.onlines[lv];
			}

			LOG_DEBUG("sting_load - gameType:%d,lv:%d,cfgRobotCount:%d,onlineRobotCount:%d,batchID:%d", refCfg.gameType, lv, cfgRobotCount, onlineRobotCount, refCfg.batchID);
			if (onlineRobotCount >= cfgRobotCount)
			{
				continue;
			}
			uint32 needLoadRobotCount = cfgRobotCount - onlineRobotCount;
			LOG_DEBUG("sted_load - gameType:%d,lv:%d,needLoadRobotCount:%d,onlineRobotCount:%d,cfgRobotCount:%d,local_time.tm_wday:%d,batchID:%d,leveltype:%d",
				refCfg.gameType, lv, needLoadRobotCount, onlineRobotCount, cfgRobotCount, local_time.tm_wday, refCfg.batchID, refCfg.leveltype);
            bool bRetMysql = CDBMysqlMgr::Instance().AsyncLoadRobotLogin(refCfg.gameType,lv+1,local_time.tm_wday + 1, needLoadRobotCount, refCfg.batchID,refCfg.loginType, refCfg.leveltype);
			if (bRetMysql == true)
			{
				CGobalRobotMgr::Instance().UpdateLoadRobotStatus(gameType, true);
			}
        }
    }

}

void    CGobalRobotMgr::CheckTimeRobotLogin()
{
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
		return;

	//LOG_DEBUG("check robot login");
	tm local_time;
	uint64 uTime = getTime();
	getLocalTime(&local_time, uTime);
	uint32 curSecond = local_time.tm_hour * 3600 + local_time.tm_min * 60 + local_time.tm_sec;

	map<uint32, stRobotOnlineCfg> mpRobotCfgEx;
	CDataCfgMgr::Instance().GetRobotBatchInfo(mpRobotCfgEx);
	LOG_DEBUG("server_cfg_robot - size:%d", mpRobotCfgEx.size());
	for (auto iter_beg = mpRobotCfgEx.begin(); iter_beg != mpRobotCfgEx.end(); iter_beg++)
	{
		stRobotOnlineCfg& refCfg = iter_beg->second;

		if (CServerMgr::Instance().IsOpenRobot(refCfg.gameType) == 0)
		{
			LOG_DEBUG("server not open robot - gametype:%d", refCfg.gameType);

			continue;
		}
		uint32 dwRemaindTime = CDataCfgMgr::Instance().BatchServiceRemaindTime(refCfg.enterTime, refCfg.leaveTime, curSecond);

		LOG_DEBUG("robot_load - gametype:%d,enterTime:%d,leaveTime:%d,curSecond:%d,dwRemaindTime:%d", refCfg.gameType, refCfg.enterTime, refCfg.leaveTime, curSecond, dwRemaindTime);

		if (dwRemaindTime > ROBOT_UNLOAD_TIME)
		{
			stRobotOnlineCfg& refCfg = iter_beg->second;

			for (uint8 lv = 0; lv<ROBOT_MAX_LEVEL; ++lv)
			{
				uint32 robotNum = refCfg.onlines[lv];
				LOG_DEBUG("gametype_start_load - gameType:%d,lv:%d,robotNum:%d,batchID:%d", refCfg.gameType, lv, robotNum, refCfg.batchID);

				if (robotNum == 0)
				{
					continue;
				}

				uint32 onlineRobotCount = GetRobotNum(refCfg.gameType, lv + 1, refCfg.batchID);
				LOG_DEBUG("gametype_sting_load - gameType:%d,lv:%d,robotNum:%d,onlineRobotCount:%d,batchID:%d", refCfg.gameType, lv, robotNum, onlineRobotCount, refCfg.batchID);

				if (onlineRobotCount >= robotNum)
				{
					continue;
				}
				uint32 needNum = robotNum - onlineRobotCount;

				LOG_DEBUG("gametype_sting_load - gameType:%d,lv:%d,robotNum:%d,onlineRobotCount:%d,needNum:%d,batchID:%d", refCfg.gameType, lv, robotNum, onlineRobotCount,needNum, refCfg.batchID);

				CDBMysqlMgr::Instance().AsyncLoadTimeIntervalRobotLogin(refCfg.gameType, refCfg.roomID, lv + 1, local_time.tm_wday + 1, needNum, refCfg.batchID, refCfg.loginType);
			}
		}

	}
}


// 加载机器人充值池数据
bool    CGobalRobotMgr::LoadRobotPayPoolData()
{
    LOG_DEBUG("robotmgr login:%d",ROBOT_MGR_ID);

    for(uint32 i=0;i<net::GAME_CATE_MAX_TYPE;++i)
    {
		if (i != net::GAME_CATE_EVERYCOLOR)
		{
			if (!CCommonLogic::IsOpenGame(i))
			{
				continue;
			}
		}


        uint32 uid = ROBOT_MGR_ID + i;
        CPlayer* pRobot = new CLobbyRobot();
        pRobot->SetSession(NULL);
        pRobot->SetUID(uid);
        CPlayerMgr::Instance().AddPlayer(pRobot);
        pRobot->OnLogin();
    }
    return true;
}

bool    CGobalRobotMgr::AddRobotPayPoolData(uint16 gameType)
{
	LOG_DEBUG("robotmgr - ROBOT_MGR_ID:%d,gameType:%d", ROBOT_MGR_ID, gameType);
	if (gameType > GAME_CATE_MAX_TYPE || gameType <= 0)
	{
		return false;
	}
	uint32 robotUid = ROBOT_MGR_ID + gameType;
	bool bIs_robot_online = CPlayerMgr::Instance().IsOnline(robotUid);
	LOG_DEBUG("robotmgr - robotUid:%d,bIs_robot_online:%d", robotUid, bIs_robot_online);
	if (bIs_robot_online)
	{
		return false;
	}
	else
	{
		CPlayer* pRobot = new CLobbyRobot();
		pRobot->SetSession(NULL);
		pRobot->SetUID(robotUid);
		CPlayerMgr::Instance().AddPlayer(pRobot);
		pRobot->OnLogin();
	}
	return true;
}

// 机器人充值存入积分财富币
bool    CGobalRobotMgr::StoreScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pPlayer->CanChangeAccountValue(emACC_VALUE_SCORE,-score) || !pPlayer->CanChangeAccountValue(emACC_VALUE_COIN,-coin)){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_SCORE,score) || !pRobotMgr->CanChangeAccountValue(emACC_VALUE_COIN,coin)){
        return false;
    }
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTSTORE,0,0,-coin,0,-score,0,0);
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTSTORE,1,0,coin,0,score,0,0);
    LOG_DEBUG("机器人存金币 - robot store uid:%d-score:%lld-coin:%lld",pPlayer->GetUID(),score,coin);
    
    return true;
}
// 机器人充值钻石
bool    CGobalRobotMgr::PayDiamond(CLobbyRobot* pPlayer,int64 diamond)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_DIAMOND,-diamond)){
        LOG_DEBUG("总账号%d钻石不足，不能充值:%d",pRobotMgr->GetUID(),pRobotMgr->GetAccountValue(emACC_VALUE_DIAMOND));
        return false;
    }
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,0,diamond,0,0,0,0,0);
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,1,-diamond,0,0,0,0,0);
    LOG_DEBUG("机器人:%d 充值钻石:%lld",pPlayer->GetUID(),diamond);
    return true;
}
// 兑换积分
bool    CGobalRobotMgr::TakeScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_SCORE,-score))
	{
        LOG_DEBUG("robot_score_error - mgrUID:%d,uid:%d,score:%lld",pRobotMgr->GetUID(), pPlayer->GetUID(), pRobotMgr->GetAccountValue(emACC_VALUE_SCORE));
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_COIN,-coin))
	{
        LOG_DEBUG("robot_coin_error - mgrUid:%d,uid:%d,coin:%lld",pRobotMgr->GetUID(), pPlayer->GetUID(), pRobotMgr->GetAccountValue(emACC_VALUE_COIN));
        return false;
    }
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,1,0,-coin,0,-score,0,0);
    LOG_DEBUG("get_score - uid:%d,score:%lld,coin:%lld", pPlayer->GetUID(),score,coin);

    return true;
}
// 获得机器人管理员
CPlayer* CGobalRobotMgr::GetRobotMgr(uint16 gameType)
{
    uint32 mgrID = ROBOT_MGR_ID + gameType;
    CPlayer* pRobotMgr = (CPlayer*)CPlayerMgr::Instance().GetPlayer(mgrID);
    if(pRobotMgr == NULL || !pRobotMgr->IsPlaying()){
        LOG_ERROR("机器人总账号未登录或未加载数据成功:%d",mgrID);
        return NULL;
    }    
    return pRobotMgr;
}
// 重新加载机器人数据
void    CGobalRobotMgr::ChangeAllRobotCfg()
{
    vector<CPlayerBase*> allPlayers;
    CPlayerMgr::Instance().GetAllPlayers(allPlayers);
    for(uint32 i=0;i<allPlayers.size();++i){
        if(allPlayers[i]->IsRobot()){
            CLobbyRobot* pRobot = (CLobbyRobot*)allPlayers[i];
            pRobot->SetNeedLoginOut(true);

            net::msg_leave_robot msg;
            msg.set_uid(pRobot->GetUID());
            pRobot->SendMsgToGameSvr(&msg,net::L2S_MSG_LEAVE_ROBOT);
        }
    }
}
// 获得在线机器人数量
uint32  CGobalRobotMgr::GetRobotNum(uint16 gameType,uint8 level,uint32 batchid)
{
    uint32 count = 0;
    vector<CPlayerBase*> allPlayers;
    CPlayerMgr::Instance().GetAllPlayers(allPlayers);
    for(uint32 i=0;i<allPlayers.size();++i)
    {
        if(allPlayers[i]->IsRobot())
        {
            CLobbyRobot* pRobot = (CLobbyRobot*)allPlayers[i];
            if(pRobot->GetGameType() == gameType && pRobot->GetBatchID() == batchid)
            {
                if(pRobot->GetCfgLevel() == level)
                    count++;
            }
        }
    }
    return count;
}


uint32  CGobalRobotMgr::GetRobotCountByBatchID(uint16 gameType, uint8 level,map<uint32, stRobotOnlineCfg> & mpBatchCount)
{
	uint32 count = 0;
	vector<CPlayerBase*> allPlayers;
	mpBatchCount.clear();
	CPlayerMgr::Instance().GetAllPlayers(allPlayers);
	for (uint32 i = 0; i<allPlayers.size(); ++i)
	{
		if (allPlayers[i] == NULL)
		{
			continue;
		}
		if (allPlayers[i]->IsRobot())
		{
			CLobbyRobot* pRobot = (CLobbyRobot*)allPlayers[i];			
			if (pRobot != NULL && pRobot->GetGameType() == gameType && pRobot->GetCfgLevel() > 0 && pRobot->GetCfgLevel() <= ROBOT_MAX_LEVEL)
			{
				int  cfgLevel = pRobot->GetCfgLevel();
				cfgLevel -= 1;
				if (cfgLevel >= 0 && cfgLevel < ROBOT_MAX_LEVEL)
				{
					auto iter_find = mpBatchCount.find(pRobot->GetBatchID());
					if (iter_find != mpBatchCount.end())
					{
						auto & tempRobotOnlineCfg = iter_find->second;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
					}
					else
					{
						stRobotOnlineCfg tempRobotOnlineCfg;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
						mpBatchCount.insert(make_pair(pRobot->GetBatchID(), tempRobotOnlineCfg));
					}
				}
				if (pRobot->GetCfgLevel() == level)
				{
					count++;
				}
			}
		}
	}
	return count;
}

uint32  CGobalRobotMgr::GetOnLineRobotCount(map<uint32, stRobotOnlineCfg> & mpBatchCount)
{	
	//vector<CPlayerBase*> allPlayers;
	//CPlayerMgr::Instance().GetAllPlayers(allPlayers);
	uint32 count = 0;
	mpBatchCount.clear();
	stl_hash_map<uint32, CPlayerBase*> & mpPlayers = CPlayerMgr::Instance().GetAllPlayers();
	for (auto iter = mpPlayers.begin(); iter != mpPlayers.end(); ++iter)
	{
		if (iter->second == NULL)
		{
			continue;
		}
		if (iter->second->IsRobot())
		{
			count++;
			CLobbyRobot* pRobot = (CLobbyRobot*)iter->second;
			if (pRobot != NULL && pRobot->GetCfgLevel() > 0 && pRobot->GetCfgLevel() <= ROBOT_MAX_LEVEL)
			{
				int  cfgLevel = pRobot->GetCfgLevel();
				cfgLevel -= 1;
				if (cfgLevel >= 0 && cfgLevel < ROBOT_MAX_LEVEL)
				{
					auto iter_find = mpBatchCount.find(pRobot->GetBatchID());
					if (iter_find != mpBatchCount.end())
					{
						auto & tempRobotOnlineCfg = iter_find->second;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
					}
					else
					{
						stRobotOnlineCfg tempRobotOnlineCfg;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
						mpBatchCount.insert(make_pair(pRobot->GetBatchID(), tempRobotOnlineCfg));
					}
				}
			}
		}
	}
	return count;
}


bool CGobalRobotMgr::UpdateLoadRobotStatus(uint32 gameType,bool flag)
{
	bool ret = false;
	if (gameType >= GAME_CATE_LAND && gameType < net::GAME_CATE_MAX_TYPE)
	{
		ret = true;
		m_bArrLoadAllDayRobotStatus[gameType] = flag;
	}
	return ret;
}

bool CGobalRobotMgr::CheckLoadRobotStatus(uint32 gameType)
{
	bool ret = true;
	if (gameType >= GAME_CATE_LAND && gameType < net::GAME_CATE_MAX_TYPE)
	{
		ret = m_bArrLoadAllDayRobotStatus[gameType];
	}
	return ret;
}
