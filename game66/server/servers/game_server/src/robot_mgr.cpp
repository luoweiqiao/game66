//
// Created by toney on 16/5/18.
//

#include "robot_mgr.h"
#include "stdafx.h"
#include "game_room.h"
#include "game_room_mgr.h"
#include "data_cfg_mgr.h"
#include "common_logic.h"

namespace
{
    static const uint32 s_FreeRobotNum = 10;
    static const int32  s_RoomMaxRobot = 200;
};

CRobotMgr::CRobotMgr()
{
    m_isOpenRobot = false;
}
CRobotMgr::~CRobotMgr()
{
}
bool CRobotMgr::Init()
{
    m_DispatchCool.beginCooling(3000);
    m_isOpenRobot = CDataCfgMgr::Instance().GetCurSvrCfg().openRobot == 1 ? true : false;
    return true;
}
void CRobotMgr::ShutDown()
{

}
void CRobotMgr::OnTimeTick()
{
    CheckRobotBackPool();
    CheckRobotBackLobby();
    if(m_DispatchCool.isTimeOut()){
        DispatchRobotToTable();
        m_DispatchCool.beginCooling(10000);
    }
}
CGameRobot* CRobotMgr::GetRobot(uint32 uid)
{
    MAP_ROBOTS::iterator it = m_mpRobots.find(uid);
	if (it != m_mpRobots.end())
	{
		return it->second;
	}
    return NULL;
}
// 请求分配一个机器人
bool CRobotMgr::RequestOneRobot(CGameTable* pTable, CGamePlayer* pPlayer)
{
	if (m_mpRobots.empty() || !m_isOpenRobot)
	{
		return false;
	}

	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return false;
	}

	uint32 roomid = 255;
	uint32 tableid = 255;
	uint32 uid = 0;
	if (pTable != NULL && pPlayer != NULL)
	{
		roomid = pTable->GetRoomID();
		tableid = pTable->GetTableID();
		uid = pPlayer->GetUID();
	}

	LOG_DEBUG("get_robot_sta - roomid:%d,tableid:%d,uid:%d,m_mpRobots_size:%d,pTable:%p,pPlayer:%p", roomid, tableid, uid, m_mpRobots.size(), pTable, pPlayer);
	
	int iRobotIndexCount = 0;
	int iEnterRoomCount = 0;
	int iEnterTableCount = 0;

	stRobotOnlineCfg robotCfgScore;
	stRobotOnlineCfg robotCfgCoin;


    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    //uint32 startPos = g_RandGen.RandUInt()%m_mpRobots.size();
    for(;it != m_mpRobots.end();++it)
    {
        //if(startPos > 0){
        //    startPos--;
        //    continue;
        //}
        CGameRobot* pRobot = it->second;
		if (pRobot == NULL)
		{
			iRobotIndexCount++;
			continue;
		}

		stRobotOnlineCfg robotCfgTemp;
		CDataCfgMgr::Instance().GetRobotGameCfg(pRobot->GetUID(), robotCfgTemp);

		if (robotCfgTemp.leveltype == net::ROOM_CONSUME_TYPE_COIN)
		{
			if (pRobot->GetLvCoin() > 0 && pRobot->GetLvCoin() < ROBOT_MAX_LEVEL)
			{
				robotCfgCoin.onlines[pRobot->GetLvCoin()-1];
			}
		}
		else if (robotCfgTemp.leveltype == net::ROOM_CONSUME_TYPE_SCORE)
		{
			if (pRobot->GetLvScore() > 0 && pRobot->GetLvScore() < ROBOT_MAX_LEVEL)
			{
				robotCfgScore.onlines[pRobot->GetLvScore()-1];
			}
		}
		else
		{

		}

        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL)
		{
			iRobotIndexCount++;
            continue;
        }
        if(!pTable->GetHostRoom()->CanEnterRoom(pRobot))
		{
			iEnterRoomCount++;
            continue;
        }
		if (!pTable->CanEnterTable(pRobot))
		{
			iEnterTableCount++;
			continue;
		}

		string strCfgScore;
		string strCfgCoin;

		for (int i = 0; i < ROBOT_MAX_LEVEL; i++)
		{
			strCfgScore += CStringUtility::FormatToString("i:%02d,lv:%02d ",i, robotCfgScore.onlines[i]);
		}
		for (int i = 0; i < ROBOT_MAX_LEVEL; i++)
		{
			strCfgCoin += CStringUtility::FormatToString("i:%02d,lv:%02d ",i, robotCfgCoin.onlines[i]);
		}


        LOG_DEBUG("get_a_robot - roomid:%d,tableid:%d,uid:%d,ruid:%d,iRobotIndexCount:%d,iEnterRoomCount:%d,iEnterTableCount:%d,strCfgScore:%s,strCfgCoin:%s",
			roomid, tableid, uid,pRobot->GetUID(), iRobotIndexCount, iEnterRoomCount, iEnterTableCount, strCfgScore.c_str(), strCfgCoin.c_str());

        pTable->GetHostRoom()->EnterRoom(pRobot);
		if (pTable->EnterTable(pRobot))
		{
			return true;
		}            
    }

	string strCfgScore;
	string strCfgCoin;

	for (int i = 0; i < ROBOT_MAX_LEVEL; i++)
	{
		strCfgScore += CStringUtility::FormatToString("i:%02d,lv:%02d ",i, robotCfgScore.onlines[i]);
	}
	for (int i = 0; i < ROBOT_MAX_LEVEL; i++)
	{
		strCfgCoin += CStringUtility::FormatToString("i:%02d,lv:%02d ",i, robotCfgCoin.onlines[i]);
	}

	LOG_DEBUG("get_robot_end - roomid:%d,tableid:%d,uid:%d,m_mpRobots_size:%d,iRobotIndexCount:%d,iEnterRoomCount:%d,iEnterTableCount:%d,strCfgScore:%s,strCfgCoin:%s",
		roomid, tableid, uid, m_mpRobots.size(), iRobotIndexCount, iEnterRoomCount, iEnterTableCount, strCfgScore.c_str(), strCfgCoin.c_str());

    return false;
}

bool CRobotMgr::RequestXRobot(int count, CGameTable* pTable)
{
	if (m_mpRobots.empty() || !m_isOpenRobot || count <= 0 || pTable==NULL)
	{
		return false;
	}

	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return false;
	}

	MAP_ROBOTS::iterator it = m_mpRobots.begin();
	int robotIndex = 0;
	for (; it != m_mpRobots.end(); ++it)
	{
		robotIndex++;
		CGameRobot* pRobot = it->second;
		if (pRobot == NULL)
		{
			continue;
		}
		if (pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL)
		{
			continue;
		}
		if (!pTable->GetHostRoom()->CanEnterRoom(pRobot) || !pTable->CanEnterTable(pRobot))
		{
			continue;
		}
		//LOG_DEBUG("get a robot - uid:%d", pRobot->GetUID());
		bool bEnterRoom = pTable->GetHostRoom()->EnterRoom(pRobot);
		bool bEnterTable = false;
		if (bEnterRoom)
		{
			bEnterTable = pTable->EnterTable(pRobot);
		}
		LOG_DEBUG("get_x_robot - robotIndex:%d,bEnterRoom:%d,bEnterTable:%d,m_mpRobots.size:%d,count:%d,roomid:%d,tableid:%d,ruid:%d",
			robotIndex,bEnterRoom, bEnterTable, m_mpRobots.size(), count, pTable->GetRoomID(), pTable->GetTableID(), pRobot->GetUID());

		if (bEnterTable)
		{
			count--;
			if (count <= 0)
			{
				return true;
			}
		}
	}

	LOG_DEBUG("get_xe_robot - robotIndex:%d,m_mpRobots.size:%d,count:%d,roomid:%d,tableid:%d",
		robotIndex, m_mpRobots.size(), count, pTable->GetRoomID(), pTable->GetTableID());

	return false;
}


bool    CRobotMgr::AddRobot(CGameRobot* pRobot)
{
	if (GetRobot(pRobot->GetUID()) != NULL)
	{
		return false;
	}
    m_mpRobots.insert(make_pair(pRobot->GetUID(),pRobot));

    LOG_DEBUG("add robot uid:%d,batchID:%d,m_mpRobots.size:%d",pRobot->GetUID(), pRobot->GetBatchID(),m_mpRobots.size());
    CPlayerMgr::Instance().AddPlayer(pRobot);

    return true;
}

bool    CRobotMgr::RemoveRobot(CGameRobot* pRobot)
{
    uint32 uid = pRobot->GetUID();
	uint32 batchID = pRobot->GetBatchID();
	bool bLeaveGame = pRobot->OnLoginOut();
	if (bLeaveGame)
	{
		m_mpRobots.erase(pRobot->GetUID());
		CPlayerMgr::Instance().RemovePlayer(pRobot);
		SAFE_DELETE(pRobot);
	}

    LOG_DEBUG("remove robot - uid:%d,bLeaveGame:%d,batchID:%d,m_mpRobots.size:%d",uid, bLeaveGame, batchID,m_mpRobots.size());
    return true;
}

// 空闲机器人数量
uint32  CRobotMgr::GetFreeRobotNum()
{
    uint32 num = 0;
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL){
            continue;
        }
        num++;
    }
    return num;
}
void    CRobotMgr::GetFreeRobot(vector<CGameRobot*>& robots)
{
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL){
            continue;
        }
		//LOG_DEBUG("push free robot - uid:%d,robots.size:%d", pRobot->GetUID(), robots.size());
        robots.push_back(pRobot);
    }
}
// 派发机器人占桌
void    CRobotMgr::DispatchRobotToTable()
{
	if (m_isOpenRobot == false || CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return;
	}

    vector<CGameRobot*> robots;
    GetFreeRobot(robots);
    uint32 disNum = m_mpRobots.size()/4;
    uint16 gameType = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;

    if(!CCommonLogic::IsBaiRenGame(gameType))
	{
        disNum = MAX(s_FreeRobotNum,disNum);
        if (robots.size() <= disNum)
		{
            //LOG_DEBUG("no more robots - robots.size:%d,disNum:%d,gameType:%d", robots.size(),disNum, gameType);
            return;
        }

    }
	else
	{
        disNum = 0;
    }
	//robots.erase(robots.begin(), robots.begin() + disNum);

    vector<CGameRoom*> vecRoomsCoin;
	vector<CGameRoom*> vecRoomsScore;
    bool bRetGetRoom = CGameRoomMgr::Instance().GetAllRobotRoom(vecRoomsCoin, vecRoomsScore);
    if (bRetGetRoom == false)
	{
        LOG_DEBUG("no_game_room - bRetGetRoom:%d,robots.size:%d,rooms.size:%d - %d,disNum:%d", bRetGetRoom, robots.size(), vecRoomsCoin.size(), vecRoomsScore.size(), disNum);
        return;
    }

    while(robots.size() > 0)
    {
		CGameRobot* pGamePlayer = robots[0];
		robots.erase(robots.begin());
		if (pGamePlayer == NULL)
		{
			continue;
		}
		int32 dwRemaindTime = CDataCfgMgr::Instance().GetRobotBatchOut(pGamePlayer->GetBatchID());
		if (dwRemaindTime != -1)
		{
			if (dwRemaindTime < ROBOT_UNLOAD_TIME)
			{
				LOG_DEBUG("robot_leave_time_arrive - uid:%d,batchID:%d,dwRemaindTime:%d",
					pGamePlayer->GetUID(), pGamePlayer->GetBatchID(), dwRemaindTime);

				continue;
			}
		}
		int robot_batchid = pGamePlayer->GetBatchID();
		uint32 robot_uid = pGamePlayer->GetUID();
		stRobotOnlineCfg robotCfg;
		bool bGetCfg = CDataCfgMgr::Instance().GetRobotGameCfg(robot_batchid, robotCfg);
		LOG_DEBUG("robot_enter_ing - uid:%d,batchid:%d,dwRemaindTime:%d,bGetCfg:%d,gameType:%d,robotType:%d,rrid,%d,leveltype:%d,r_size:%d - %d",
			robot_uid, robot_batchid, dwRemaindTime, bGetCfg, gameType, robotCfg.gameType, robotCfg.roomID, robotCfg.leveltype, vecRoomsCoin.size(), vecRoomsScore.size());
		if (bGetCfg == false || robotCfg.gameType != gameType)
		{
			continue;
		}
		CGameRoom* pRoom = NULL;
		if (CCommonLogic::IsBaiRenRobotGame(gameType))
		{
			for (uint32 roomindex = 0; roomindex < vecRoomsCoin.size(); roomindex++)
			{
				CGameRoom* pTempRoom = vecRoomsCoin[roomindex];
				if (pTempRoom == NULL)
				{
					continue;
				}
				if (robotCfg.roomID == pTempRoom->GetRoomID())
				{
					pRoom = pTempRoom;
				}
			}
		}
		else
		{
			if (robotCfg.leveltype == net::ROOM_CONSUME_TYPE_SCORE)
			{
				if (vecRoomsScore.size() == 0)
				{
					continue;
				}
				if (vecRoomsScore.size() > 1)
				{
					pRoom = vecRoomsScore[g_RandGen.RandRange(0, vecRoomsScore.size() - 1)];
				}
				else
				{
					pRoom = vecRoomsScore[0];
				}
			}
			else if (robotCfg.leveltype == net::ROOM_CONSUME_TYPE_COIN)
			{
				if (vecRoomsCoin.size() == 0)
				{
					continue;
				}
				if (vecRoomsCoin.size() > 1)
				{
					pRoom = vecRoomsCoin[g_RandGen.RandRange(0, vecRoomsCoin.size() - 1)];
				}
				else
				{
					pRoom = vecRoomsCoin[0];
				}
			}
			else
			{
				LOG_DEBUG("robot_enter_room_error - uid:%d,batchid:%d,dwRemaindTime:%d,bGetCfg:%d,gameType:%d,robotType:%d,rrid,%d,leveltype:%d,r_size:%d - %d",
					robot_uid, robot_batchid, dwRemaindTime, bGetCfg, gameType, robotCfg.gameType, robotCfg.roomID, robotCfg.leveltype, vecRoomsCoin.size(), vecRoomsScore.size());
			}
		}
		if (pRoom == NULL)
		{
			LOG_DEBUG("robot_enter_room_null - uid:%d,batchid:%d,dwRemaindTime:%d,bGetCfg:%d,gameType:%d,robotType:%d,rrid,%d,leveltype:%d,r_size:%d - %d",
				robot_uid, robot_batchid, dwRemaindTime, bGetCfg, gameType, robotCfg.gameType, robotCfg.roomID, robotCfg.leveltype, vecRoomsCoin.size(), vecRoomsScore.size());

			continue;
		}
        if(pRoom->GetPlayerNum() > s_RoomMaxRobot || pRoom->IsNeedMarry())
		{
            //break;
			continue;
        }
		if (gameType == net::GAME_CATE_EVERYCOLOR)
		{
			//continue;
			bool bEnterTable = false;
			bool benter_room = pRoom->EnterEveryColorRoom(pGamePlayer);
			if (benter_room)
			{
				bEnterTable = pRoom->FastJoinEveryColorTable(pGamePlayer);
			}
			LOG_DEBUG("robot enter every color room - uid:%d,roomID:%d,bEnterRoom:%d,bEnterTable:%d", pGamePlayer->GetUID(), pRoom->GetRoomID(), benter_room, bEnterTable);
		}
		else
		{
			bool benter_room = pRoom->EnterRoom(pGamePlayer);
			if (benter_room)
			{
				if (pRoom->FastJoinTable(pGamePlayer))
				{
					//LOG_DEBUG("enter game table success - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum, m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
				}
				else
				{
					pRoom->LeaveRoom(pGamePlayer);
					//LOG_DEBUG("leave game room - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum, m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
				}
			}
			else
			{
				//LOG_DEBUG("not enter game room - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum,m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
			}
		}
    }
}
void    CRobotMgr::CheckRobotBackPool()
{
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->NeedBackPool())
        {
            pRobot->BackPool();
            continue;
        }
    }
}
void    CRobotMgr::CheckRobotBackLobby()
{
    vector<CGameRobot*> vecRobots;
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->NeedBackLobby())
        {
            vecRobots.push_back(pRobot);
            continue;
        }
    }
    for(uint32 i=0;i<vecRobots.size();++i)
    {
        RemoveRobot(vecRobots[i]);
    }
    vecRobots.clear();
}


