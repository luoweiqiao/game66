#include <data_cfg_mgr.h>

#include "game_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "center_log.h"
#include "dbmysql_mgr.h"
#include "lobby_mgr.h"
#include "active_welfare_mgr.h"

using namespace svrlib;
using namespace std;
using namespace net;

bool less_second(const tagAWPlayerInfo &m1, const tagAWPlayerInfo &m2)
{
    return m1.sum_loss < m2.sum_loss;
}

CGameTable::CGameTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:m_pHostRoom(pRoom)
,m_tableID(tableID)
,m_tableType(tableType)
{
    m_coolLogic.clearCool();
    m_hostIncome = 0;
    m_sysIncome  = 0;
    m_mpLookers.clear();
	m_mpBairenPalyerBet.clear();
	m_vecBairenCount.clear();

	m_mpCurPlayerJettonInfo.clear();
	m_spFrontPlayerJettonInfo = nullptr;
	m_ctrl_welfare = 0;
    m_aw_ctrl_uid = 0;
	m_nrw_ctrl_uid = 0;
	m_iGameCount = 0;
	m_tableCtrlPlayers.clear();
}
CGameTable::~CGameTable()
{
    
}

bool CGameTable::OnTableTick()
{
	if (m_mpControlMultiPalyer.size()>0)
	{
		uint64 uSysTime = getTickCount64();
		vector<uint32> vecPlayer;

		auto it_palyer = m_mpControlMultiPalyer.begin();
		for (; it_palyer != m_mpControlMultiPalyer.end(); it_palyer++)
		{
			tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			if (ControlMultiPalyer.ctime != 0 && ControlMultiPalyer.etime < uSysTime)
			{
				if (GetControlPalyerGame(GetGameType()))
				{
					ControlMultiPalyer.etime = getSysTime();
					CGamePlayer * pGamePlayer = GetGamePlayerByUid(ControlMultiPalyer.uid);
					if (pGamePlayer != NULL)
					{
						ControlMultiPalyer.escore = GetPlayerCurScore(pGamePlayer);
					}
					CDBMysqlMgr::Instance().SaveControlPlayerData(ControlMultiPalyer);
				}
				vecPlayer.push_back(it_palyer->first);
			}
			else if (ControlMultiPalyer.ctime == 0)
			{
				continue;
			}
			else
			{
				// error
			}
		}

		for (uint32 uIndex = 0; uIndex < vecPlayer.size(); uIndex++)
		{
			auto it_palyer_Index = m_mpControlMultiPalyer.find(vecPlayer[uIndex]);
			if (it_palyer_Index != m_mpControlMultiPalyer.end())
			{
				m_mpControlMultiPalyer.erase(it_palyer_Index);
			}
			else
			{
				// error
			}
		}
	}
	if (m_tagControlPalyer.uid != 0)
	{
		uint64 uSysTime = getTickCount64();
		uint64 uOldTime = m_tagControlPalyer.utime;
		uint64 uEndTame = (uOldTime + CONTROL_TIME_OUT);
		//LOG_ERROR("control palyer game end - roomid:%d,tableid:%d,uSysTime:%lld,uOldTime:%lld, uid:%d,count:%d,type:%d",
		//GetRoomID(), GetTableID(), uSysTime, uOldTime, m_tagControlPalyer.uid, m_tagControlPalyer.count, m_tagControlPalyer.type);

		if (uEndTame < uSysTime)
		{
			LOG_ERROR("control palyer game end - roomid:%d,tableid:%d,uSysTime:%lld,uOldTime:%lld,uEndTame:%lld, uid:%d,count:%d,type:%d,id:%lld",
				GetRoomID(), GetTableID(), uSysTime, uOldTime, uEndTame, m_tagControlPalyer.uid, m_tagControlPalyer.count, m_tagControlPalyer.type, m_tagControlPalyer.id);
			// save
			if (m_tagControlPalyer.id != -1)
			{
				if (GetControlPalyerGame(GetGameType()))
				{
					m_tagControlPalyer.etime = getSysTime();
					CGamePlayer * pGamePlayer = GetGamePlayerByUid(m_tagControlPalyer.uid);
					if (pGamePlayer != NULL)
					{
						m_tagControlPalyer.escore = GetPlayerCurScore(pGamePlayer);
					}
					CDBMysqlMgr::Instance().SaveControlPlayerData(m_tagControlPalyer);
				}
			}
			m_tagControlPalyer.Init();
		}
	}


	return true;
}

CGameRoom*  CGameTable::GetHostRoom()
{
    return m_pHostRoom;
}

uint16		CGameTable::GetRoomID()
{
	if (m_pHostRoom != NULL)
	{
		return m_pHostRoom->GetRoomID();
	}
	return 0;
}

uint16		CGameTable::GetGameType()
{
	if (m_pHostRoom != NULL)
	{
		return m_pHostRoom->GetGameType();
	}
	return net::GAME_CATE_MAX_TYPE;
}

bool    CGameTable::EnterTable(CGamePlayer* pPlayer)
{
	if (!CanEnterTable(pPlayer))
	{
		return false;
	}
	pPlayer->SetBankruptRecord(true);

	//百人场玩家进入处理
	OnBrcControlPlayerEnterTable(pPlayer);
	InitMasterUserShowInfo(pPlayer);

	if (GetControlPalyerGame(GetGameType()))
	{
		if (m_tagControlPalyer.id != -1 && m_tagControlPalyer.uid == pPlayer->GetUID())
		{
			if (m_tagControlPalyer.bscore == -1)
			{
				m_tagControlPalyer.bscore = pPlayer->GetAccountValue(emACC_VALUE_COIN);
				m_tagControlPalyer.escore = pPlayer->GetAccountValue(emACC_VALUE_COIN);
			}
		}
		auto it_palyer = m_mpControlMultiPalyer.find(pPlayer->GetUID());
		if (it_palyer != m_mpControlMultiPalyer.end())
		{
			tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			if (ControlMultiPalyer.bscore == -1)
			{
				ControlMultiPalyer.bscore = pPlayer->GetAccountValue(emACC_VALUE_COIN);
				ControlMultiPalyer.escore = pPlayer->GetAccountValue(emACC_VALUE_COIN);
			}
		}
	}
    if(NeedSitDown())
	{//需要手动坐下站起
        pPlayer->SetTable(this);
        LOG_DEBUG("IsNeedSitDown enter table - roomid:%d--tableid:%d,uid:%d,player_num:%d",
			m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerNum());
        
		SendTableInfoToClient(pPlayer);
        OnPlayerJoin(true,0,pPlayer);
		
		InitPlayerBairenCoint(pPlayer);
		//CalculateDeity();
        return true;
    }
	else
	{//自动坐下
        for(uint8 i=0;i<m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == NULL)
            {
                m_vecPlayers[i].pPlayer    = pPlayer;
                m_vecPlayers[i].readyState = 0;
                m_vecPlayers[i].uid        = pPlayer->GetUID();
                m_vecPlayers[i].readyTime  = getSysTime();
                
                pPlayer->SetTable(this);
                LOG_DEBUG("AtNeedSitDown enter table：room_id:%d--table_id:%d,uid:%d,IsAutoReady:%d,player_num:%d",
					m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsAutoReady(), GetPlayerNum());
                
				SendTableInfoToClient(pPlayer);
                OnPlayerJoin(true,i,pPlayer);
				
                if(pPlayer->IsAutoReady())
				{
                    PlayerReady(pPlayer);
                }
				
                return true;
            }
        }
    }
	
    return false;
}

bool    CGameTable::EnterEveryColorTable(CGamePlayer* pPlayer)
{
	if (!CanEnterTable(pPlayer))
	{
		return false;
	}
	pPlayer->SetBankruptRecord(true);
	if (NeedSitDown()) {//需要手动坐下站起
		pPlayer->SetTable(this);
		LOG_DEBUG("enter EnterEveryColorTable - room:%d--tb:%d,uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID());
		OnPlayerJoin(true, 0, pPlayer);
		return true;
	}
	return false;
}

bool    CGameTable::LeaveTable(CGamePlayer* pPlayer,bool bNotify)
{
    if(pPlayer->GetTable() != this)
	{
        LOG_ERROR("error table:%p--%p",pPlayer->GetTable(),this);
        return false;
    }
    if(pPlayer->IsRobot() && GetPlayerCurScore(pPlayer) < GetEnterMin())
    {
        m_robotEnterCooling.beginCooling(5*1000);
    }

	//增加对于百人场精准控制的处理
	OnBrcControlPlayerLeaveTable(pPlayer);
	LeaveMasterUser(pPlayer);

    if(NeedSitDown())
    {   //需要手动坐下站起
        for(uint8 i=0;i<m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == pPlayer)
            {
                m_vecPlayers[i].Reset();
                
                OnActionUserStandUp(i,pPlayer);
                OnPlayerJoin(false,i,pPlayer);
                
                pPlayer->SetTable(NULL);
                LOG_DEBUG("离开桌子:room:%d--tb:%d,uid:%d",m_pHostRoom->GetRoomID(),GetTableID(),pPlayer->GetUID());
                //NotifyPlayerLeave(pPlayer);
				//CalculateDeity();
                return true;
            }
        }
        LOG_DEBUG("旁观者离开桌子:%d",pPlayer->GetUID());
        OnPlayerJoin(false,0,pPlayer);
        pPlayer->SetTable(NULL);
        if(bNotify)
		{
            NotifyPlayerLeave(pPlayer);
        }
		//CalculateDeity();
        return true;
    }
	else
	{
        for(uint8 i=0;i<m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == pPlayer)
            {
                m_vecPlayers[i].Reset();
                pPlayer->SetTable(NULL);
                LOG_DEBUG("离开桌子:room_id:%d--table_id:%d,uid:%d,player_num:%d,i:%d,game_state:%d",
					m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerNum(), i, GetGameState());

                OnPlayerJoin(false,i,pPlayer);
                if(bNotify){
                    NotifyPlayerLeave(pPlayer);
                }
                return true;
            }
        }
    }
	
    return false;
}
// 从新匹配
void    CGameTable::RenewFastJoinTable(CGamePlayer* pPlayer)
{
    if(pPlayer->GetTable() != this)
        return;
    if(pPlayer->IsRobot())
	{
		LOG_DEBUG("play_leave - uid:%d", pPlayer->GetUID());
        LeaveTable(pPlayer);
        return;
    }
    net::msg_fast_join_table_rep rep;
    rep.set_result(0);
    CGameRoom* pRoom = pPlayer->GetRoom();
    if(pRoom != NULL)
    {
        if(pRoom->FastJoinTable(pPlayer)){
            rep.set_result(1);
        }
    }else{
        LOG_DEBUG("the room is null");
    }
    pPlayer->SendMsgToClient(&rep,net::S2C_MSG_FAST_JOIN_TABLE_REP);
}
bool    CGameTable::AddLooker(CGamePlayer* pPlayer)
{
    m_mpLookers[pPlayer->GetUID()] = pPlayer;
    NotifyPlayerJoin(pPlayer,true);
    LOG_DEBUG("add looker tb:%d--%d",GetTableID(),pPlayer->GetUID());
    return true;
}
bool    CGameTable::RemoveLooker(CGamePlayer* pPlayer)
{
    m_mpLookers.erase(pPlayer->GetUID());
    NotifyPlayerJoin(pPlayer,false);
    LOG_DEBUG("remove looker tb:%d--%d",GetTableID(),pPlayer->GetUID());
    return true;   
}
bool    CGameTable::AddEveryColorLooker(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) return false;
	m_mpLookers[pPlayer->GetUID()] = pPlayer;

	LOG_DEBUG("add every color looker tb:%d--%d", GetTableID(), pPlayer->GetUID());
	return true;
}
bool    CGameTable::RemoveEveryColorLooker(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) return false;
	m_mpLookers.erase(pPlayer->GetUID());
	LOG_DEBUG("remove every color looker tb:%d--%d", GetTableID(), pPlayer->GetUID());
	return true;
}

bool    CGameTable::IsExistLooker(uint32 uid)
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.find(uid);
    if(it != m_mpLookers.end()){
        return true;
    }
    return false;
}
bool    CGameTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer==NULL) {
		LOG_DEBUG("player pointer is null");
		return false;
	}
	//if (!pPlayer->IsRobot())
	//{
	//	LOG_DEBUG("player pointer is user - uid:%d", pPlayer->GetUID());
	//	return true;
	//}
	if (pPlayer->GetTable() != NULL) {
		LOG_DEBUG("player table is not null - uid:%d,ptable:%p", pPlayer->GetUID(), pPlayer->GetTable());
		return false;
	}
	if (IsFullTable() || GetGameState() != TABLE_STATE_FREE) {
		LOG_DEBUG("game table limit - uid:%d,is_full:%d,game_state:%d", pPlayer->GetUID(), IsFullTable(), GetGameState());
		return false;
	}
    // 限额进入
    if(GetPlayerCurScore(pPlayer) < GetEnterMin()){
        LOG_DEBUG("min score limit - uid:%d,cur_score:%lld,min_score:%lld", pPlayer->GetUID(),GetPlayerCurScore(pPlayer),GetEnterMin());
        return false;
    }
    if(pPlayer->IsRobot() && m_robotEnterCooling.isTimeOut() == false){// 冷却期机器人不能进入
        LOG_DEBUG("robot is cooling,do not enter table - uid:%d", pPlayer->GetUID());
        return false;
    }

    return true;
}
bool    CGameTable::CanLeaveTable(CGamePlayer* pPlayer)
{
	if (GetGameState() != TABLE_STATE_FREE)
	{
		return false;
	}
    if(pPlayer->IsRobot())
	{//机器人延迟离开
        for(uint16 i=0;i<m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == pPlayer && (getSysTime() - m_vecPlayers[i].readyTime) < 10)
			{
                return false;
            }
        }
    }

    return true;
}
bool    CGameTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,pPlayer:%p,chairID:%d", GetRoomID(), GetTableID(), pPlayer, chairID);
		return false;
	}
	uint16 gameType = m_pHostRoom->GetGameType();
	if (CCommonLogic::IsBaiRenCount(gameType))
	{
		{
			LOG_DEBUG("bairencount - dont sit down - roomid:%d,tableid:%d,uid:%d,isrobot:%d,chairID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsRobot(), chairID);


			return false;
		}
	}

    if(GetPlayerCurScore(pPlayer) < m_pHostRoom->GetSitDown())
	{
		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,chairID:%d,GetSitDown:%lld", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, m_pHostRoom->GetSitDown());

		}
        return false;
    }
    if(!IsExistLooker(pPlayer->GetUID()))
	{
        //LOG_DEBUG("not in looklist:%d",pPlayer->GetUID());
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,chairID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID);

        return false;
    }
    if(chairID >= m_vecPlayers.size())
	{
        //LOG_DEBUG("the seat is more big:%d",chairID);
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,chairID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID);

        return false;
    }
    if(m_vecPlayers[chairID].pPlayer != NULL)
	{
        //LOG_DEBUG("the seat is other player");
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,chairID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID);

        return false;
    }   
    
    return true;
}
bool    CGameTable::CanStandUp(CGamePlayer* pPlayer)
{
	if (CanLeaveTable(pPlayer))
	{
		return true;
	}
    
    return false;
}
bool    CGameTable::NeedSitDown()
{
    uint16 gameType = m_pHostRoom->GetGameType();
    switch(gameType)
    {
    case net::GAME_CATE_BULLFIGHT:
    case net::GAME_CATE_TEXAS:
    case net::GAME_CATE_BACCARAT:
    case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_EVERYCOLOR:
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
        return true;
    default:
        return false;
    }
        
    return false;
}
bool    CGameTable::PlayerReady(CGamePlayer* pPlayer)
{
	if (GetGameState() != TABLE_STATE_FREE)
	{
		return false;
	}

    if(GetPlayerCurScore(pPlayer) < GetEnterMin() || GetPlayerCurScore(pPlayer) > GetEnterMax())
	{
        LOG_DEBUG("can't ready %d",pPlayer->GetUID());
        if(CanLeaveTable(pPlayer))
		{
			LOG_DEBUG("play_leave - uid:%d", pPlayer->GetUID());
            if(LeaveTable(pPlayer,true))
			{
                if(pPlayer->GetRoom() != NULL)
                {
                    pPlayer->GetRoom()->LeaveRoom(pPlayer);
                }
            }
        }
        return false;
    }

	if (m_pHostRoom->GetGameType() != net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
		if (m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_COIN && IsExistBlock(pPlayer))
		{
			RenewFastJoinTable(pPlayer);
			return false;
		}
	}

    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == pPlayer && m_vecPlayers[i].readyState == 0)
        {
            m_vecPlayers[i].readyState = 1;
            m_vecPlayers[i].autoState  = 0;
            m_vecPlayers[i].readyTime  = getSysTime();
            m_vecPlayers[i].overTimes  = 0;
            SendReadyStateToClient();
            OnActionUserOnReady(i,pPlayer);
            return true;
        }
    }
    return false;
}

struct tagGameSortInfo
{
	//CGamePlayer * pPlayer;
	uint32 uid;
	int wincount;
	int64 betscore;
	tagGameSortInfo()
	{
		//pPlayer = NULL;
		uid = 0;
		wincount = 0;
		betscore = 0;
	}
};


bool    CGameTable::PlayerSitDownStandUp(CGamePlayer* pPlayer,bool sitDown,uint16 chairID)
{
    if(sitDown)//坐下
    {
        if(!CanSitDown(pPlayer,chairID))
            return false;
        if(m_vecPlayers[chairID].pPlayer == NULL)
        {
            m_vecPlayers[chairID].pPlayer    = pPlayer;
            m_vecPlayers[chairID].readyState = 0;
            m_vecPlayers[chairID].uid        = pPlayer->GetUID();
            m_vecPlayers[chairID].autoState  = 0;
            m_vecPlayers[chairID].readyTime  = getSysTime();
            m_vecPlayers[chairID].overTimes  = 0;

            LOG_DEBUG("sitdown：room:%d--tb:%d,chairID:%d,uid:%d",m_pHostRoom->GetRoomID(),GetTableID(),chairID,pPlayer->GetUID());
            RemoveLooker(pPlayer);
            OnActionUserSitDown(chairID,pPlayer);            
            return true;
        }else{
            LOG_DEBUG("该位置已经有人占了:%d--%d",pPlayer->GetUID(),chairID);
            return false;
        }      
    }
	else
	{//站起
        if(!CanStandUp(pPlayer))
		{
            LOG_DEBUG("can standup in gameing  roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());
            return false;
        }                
        for(uint8 i=0;i < m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == pPlayer)
            {  
                LOG_DEBUG("standup:room:%d--tb:%d,chairID:%d,uid:%d",m_pHostRoom->GetRoomID(),GetTableID(),i,pPlayer->GetUID());
                m_vecPlayers[i].Reset();    
                AddLooker(pPlayer);
                OnActionUserStandUp(i,pPlayer);
				//CalculateDeity();
                return true;
            }
        }     
                
    }    
    
    return true;
}
bool    CGameTable::ResetPlayerReady()
{
    for(uint8 i=0;i<m_vecPlayers.size();++i){
        m_vecPlayers[i].readyState = 0;
        m_vecPlayers[i].autoState  = 0;
        m_vecPlayers[i].readyTime  = 0;
        m_vecPlayers[i].overTimes  = 0;
        if(m_vecPlayers[i].pPlayer){
            m_vecPlayers[i].pPlayer->SetAutoReady(false);
            m_vecPlayers[i].uid = m_vecPlayers[i].pPlayer->GetUID();
        }
    }
    SendReadyStateToClient();
    return true;
}
bool    CGameTable::IsAllReady()
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == NULL || m_vecPlayers[i].readyState == 0){
            return false;
        }
    }
    return true;
}
bool    CGameTable::PlayerSetAuto(CGamePlayer* pPlayer,uint8 autoType)
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == pPlayer)
        {
            if(m_vecPlayers[i].autoState != autoType){
                m_vecPlayers[i].autoState = (autoType == 1) ? 1 : 0;
                m_vecPlayers[i].overTimes = 0;
                SendReadyStateToClient();
            }
            return true;
        }
    }
    return false;
}
bool    CGameTable::IsReady(CGamePlayer* pPlayer)
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == pPlayer){
            return (m_vecPlayers[i].readyState == 1);
        }
    }
    return false;
}

void    CGameTable::ReadyAllRobot()
{
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_vecPlayers[i].readyState == 0){
                PlayerReady(pPlayer);
            }
        }
    }
}
bool    CGameTable::IsExistIP(uint32 ip)
{
	if (m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_SCORE)
	{
		return false;
	}
	//return false;
    if(ip == 0)return false;
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->GetIP() == ip)
        {
            return true;
        }
    }

    return false;
}

bool    CGameTable::IsExistBlock(CGamePlayer* pPlayer)
{
	if (m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_SCORE)
	{
		return false;
	}
	//return false;
    // 是否有屏蔽玩家
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pTmp = m_vecPlayers[i].pPlayer;
		if (pTmp == NULL || pTmp == pPlayer)
		{
			continue;
		}            
        if(pTmp->IsExistBlocker(pPlayer->GetUID()) || pPlayer->IsExistBlocker(pTmp->GetUID()))
		{
            return true;
        }
        if(pTmp->IsExistBlockerIP(pPlayer->GetIP()) || pPlayer->IsExistBlockerIP(pTmp->GetIP()))
		{
            return true;
        }
    }
    return false;
}
bool    CGameTable::IsFullTable()
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer == NULL)
		{
			return false;
		}            
    }
    return true;
}
uint32  CGameTable::GetPlayerNum()
{
    uint32 num = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer != NULL)
		{
			num++;
		}            
    }
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
	{
        if(it->second != NULL)
		{
            num++;
        }
    }

    return num;
}
uint32  CGameTable::GetChairPlayerNum()
{
    uint32 num = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer != NULL)
		{
			num++;
		}            
    }
    return num;
}

uint32  CGameTable::GetOnlinePlayerNum()
{
    uint32 num = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer != NULL && m_vecPlayers[i].pPlayer->GetPlayerType() == PLAYER_TYPE_ONLINE)
		{
            num++;
        }
    }
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        if(it->second != NULL && it->second->GetPlayerType() == PLAYER_TYPE_ONLINE)
        {
            num++;
        }
    }

    return num;    
}
uint32  CGameTable::GetReadyNum()
{
    uint32 num = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer != NULL && m_vecPlayers[i].readyState == 1)
		{
			num++;
		}            
    }
    return num;
}
bool    CGameTable::IsEmptyTable()
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer != NULL)
		{
			return false;
		}           
    }
    return true;
}
uint32  CGameTable::GetFreeChairNum()
{
    uint32 count = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
		if (m_vecPlayers[i].pPlayer == NULL)
		{
			count++;
		}
    }
    return count;
}
uint32  CGameTable::GetChairNum()
{
    return m_vecPlayers.size();
}
bool    CGameTable::IsAllDisconnect()
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer != NULL && m_vecPlayers[i].pPlayer->GetNetState() == 1)
        {
            return false;
        }
    }
    return true;
}
void    CGameTable::SetGameState(uint8 state)
{
    m_gameState = state;
}
uint8   CGameTable::GetGameState()
{
    return m_gameState;
}
CGamePlayer* CGameTable::GetPlayer(uint16 chairID)
{
	if (chairID < m_vecPlayers.size())
	{
		return m_vecPlayers[chairID].pPlayer;
	}
    return NULL;
}

CGamePlayer * CGameTable::GetGamePlayerByUid(uint32 uid)
{
	CGamePlayer * pGamePlayer = NULL;
	for (uint32 nChairID = 0; nChairID < m_vecPlayers.size(); nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();
		if (dwUserID == uid)
		{
			pGamePlayer = pPlayer;
			break;
		}
	}
	if (pGamePlayer == NULL)
	{
		// 旁观用户
		auto it_looker = m_mpLookers.find(uid);
		if (it_looker != m_mpLookers.end())
		{
			CGamePlayer * pPlayer = it_looker->second;
			if (pPlayer != NULL)
			{
				pGamePlayer = pPlayer;
			}
		}
	}
	return pGamePlayer;
}

uint32  CGameTable::GetPlayerID(uint16 chairID)
{
	if (chairID < m_vecPlayers.size())
	{
		return m_vecPlayers[chairID].uid;
	}
    return 0;
}
uint16  CGameTable::GetChairID(CGamePlayer* pPlayer)
{
    for(uint8 i=0;i<m_vecPlayers.size();++i){
        if(m_vecPlayers[i].pPlayer == pPlayer){
            return i;
        }
    }
    //LOG_ERROR("not this table:%d--%p-%p",pPlayer->GetUID(),pPlayer->GetTable(),this);
    return 0xFF;
}
uint16  CGameTable::GetRandFreeChairID()
{
    vector<uint16> emptyChairs;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer == NULL)
		{
            emptyChairs.push_back(i);
            continue;
        }
    }
    if(emptyChairs.size() > 0)
	{
        return emptyChairs[g_RandGen.RandUInt()%emptyChairs.size()];        
    }       
    return 0xFF;
}
void    CGameTable::SendMsgToLooker(const google::protobuf::Message* msg,uint16 msg_type)
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
		if (pPlayer != NULL)
		{
			pPlayer->SendMsgToClient(msg, msg_type);
		}
    }
}
void    CGameTable::SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type)
{
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer != NULL)
        {
            m_vecPlayers[i].pPlayer->SendMsgToClient(msg,msg_type);
        }
    }
}
void    CGameTable::SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type)
{
    SendMsgToPlayer(msg,msg_type);
    SendMsgToLooker(msg,msg_type);
}
void    CGameTable::SendMsgToClient(uint16 chairID,const google::protobuf::Message* msg,uint16 msg_type)
{
	if (chairID >= m_vecPlayers.size())
	{
		return;
	}        
    CGamePlayer* pGamePlayer = m_vecPlayers[chairID].pPlayer;
    if(pGamePlayer != NULL){
        pGamePlayer->SendMsgToClient(msg,msg_type);
    }
}
void    CGameTable::SendTableInfoToClient(CGamePlayer* pPlayer)
{
    net::msg_table_info_rep msg;
    // 桌子信息
    net::table_face_info* pTableInfo = msg.mutable_table_info();
    GetTableFaceInfo(pTableInfo);
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TABLE_INFO);
    LOG_DEBUG("Send Table Info To Client - roomid:%d,tableid:%d,uid:%lld", GetRoomID(), GetTableID(), pPlayer->GetUID());
}
void    CGameTable::SendReadyStateToClient()
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
    LOG_DEBUG("send ready state to client  roomid:%d,tableid:%d,m_vecPlayers.size:%d", GetRoomID(), GetTableID(), m_vecPlayers.size());
    net::msg_table_ready_rep msg;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        msg.add_readys(m_vecPlayers[i].readyState);
        msg.add_auto_states(m_vecPlayers[i].autoState);
    }
    SendMsgToAll(&msg,S2C_MSG_TABLE_READY_REP);
}
void    CGameTable::SendSeatInfoToClient(CGamePlayer* pGamePlayer)
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
    net::msg_seat_info_rep msg;
	string strChairUID;
    // 座位玩家信息
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        net::seat_info* pSeat = msg.add_players();
        pSeat->set_chairid(i);
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL)
		{
            pSeat->set_uid(pPlayer->GetUID());
			pSeat->set_showuid(pPlayer->GetUID());
            pSeat->set_name(pPlayer->GetPlayerName());
            pSeat->set_sex(pPlayer->GetSex());
            pSeat->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
            pSeat->set_score(pPlayer->GetAccountValue(emACC_VALUE_SCORE));
            pSeat->set_cvalue(pPlayer->GetAccountValue(emACC_VALUE_CVALUE));
            pSeat->set_head_icon(pPlayer->GetHeadIcon());
            pSeat->set_buyin(m_vecPlayers[i].buyinScore);
			pSeat->set_city(pPlayer->GetCity());
			pSeat->set_wincount(pPlayer->GetVecWin(GetGameType()));
			pSeat->set_betscore(pPlayer->GetVecBet(GetGameType()));
			pSeat->set_betcount(pPlayer->GetBetCount(GetGameType()));
			if (pPlayer->IsRobot() == false)
			{
				strChairUID += CStringUtility::FormatToString(" _chairid_%d_uid_%d", i, pPlayer->GetUID());
				LOG_DEBUG("i:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d,gamestate:%d,playernum:%d", i, 
					pPlayer->GetUID(), pSeat->wincount(), pSeat->betscore(), pSeat->betcount(), GetGameState(), GetPlayerNum());
			}
			if (GetIsMasterUser(pPlayer))
			{
				pSeat->set_showuid(pPlayer->GetShowUid());
				pSeat->set_city(pPlayer->GetShowCity());
				pSeat->set_coin(pPlayer->GetShowCoin());
			}
        }
		else
		{
            pSeat->set_uid(0);
            pSeat->set_name("");
            pSeat->set_sex(0);
            pSeat->set_coin(0);
            pSeat->set_score(0);
            pSeat->set_cvalue(0);
            pSeat->set_head_icon(0);
            pSeat->set_buyin(0);
			pSeat->set_city("");
        }
    }
    if(pGamePlayer != NULL){
        pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_SEATS_INFO);
    }else{
        SendMsgToAll(&msg, net::S2C_MSG_SEATS_INFO);
    }
	LOG_DEBUG("Send SeatInfo to Client - roomid:%d,tableid:%d,m_mpLookers.size:%d,msg.size:%d,strChairUID:%s", GetRoomID(),GetTableID(),m_mpLookers.size(), msg.players_size(), strChairUID.c_str());
}
void     CGameTable::SendLookerListToClient(CGamePlayer* pGamePlayer)
{
    uint32 sendNum = 0;        
    net::msg_looker_list_info_rep msg;
    msg.set_is_reset(1);
    
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        net::looker_info* pInfo = msg.add_lookers();
        CGamePlayer* pPlayer = it->second;
        pInfo->set_uid(pPlayer->GetUID());
        pInfo->set_name(pPlayer->GetPlayerName());
        pInfo->set_sex(pPlayer->GetSex());
        pInfo->set_head_icon(pPlayer->GetHeadIcon());     
        pInfo->set_score(pPlayer->GetAccountValue(emACC_VALUE_SCORE));
        pInfo->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
		pInfo->set_city(pPlayer->GetCity());
		pInfo->set_wincount(pPlayer->GetVecWin(GetGameType()));
		pInfo->set_betscore(pPlayer->GetVecBet(GetGameType()));
		pInfo->set_betcount(pPlayer->GetBetCount(GetGameType()));
		//LOG_DEBUG("sendNum:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d", sendNum, pPlayer->GetUID(), pInfo->wincount(), pInfo->betscore(), pInfo->betcount());
        sendNum++;
        if(sendNum > 20)
		{
            if(pGamePlayer != NULL)
			{
                pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_LOOKER_LIST_INFO);
            }
			else
			{
                SendMsgToAll(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
            }
            sendNum = 0;
            msg.set_is_reset(0);
			msg.clear_lookers();
        }
    }
    if(sendNum > 0)
	{
        if(pGamePlayer != NULL)
		{
            pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_LOOKER_LIST_INFO);
        }
		else
		{
            SendMsgToAll(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
        }    
    }
}

void     CGameTable::SendPalyerLookerListToClient(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL)
	{
		return;
	}
	uint32 sendNum = 0;
	net::msg_looker_list_info_rep msg;
	msg.set_is_reset(1);

	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer != NULL && pGamePlayer->GetUID() == pPlayer->GetUID())
		{
			net::looker_info* pInfo = msg.add_lookers();

			pInfo->set_uid(pPlayer->GetUID());
			pInfo->set_name(pPlayer->GetPlayerName());
			pInfo->set_sex(pPlayer->GetSex());
			pInfo->set_head_icon(pPlayer->GetHeadIcon());
			pInfo->set_score(pPlayer->GetAccountValue(emACC_VALUE_SCORE));
			pInfo->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));

			pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_LOOKER_LIST_INFO);

			break;
		}

	}
}

void     CGameTable::NotifyPlayerJoin(CGamePlayer* pPlayer,bool isJoin)
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
    net::msg_notify_player_join_rep msg;
    msg.set_join_leave(isJoin);
    
    net::looker_info* pSeat = msg.mutable_player();

    pSeat->set_uid(pPlayer->GetUID());
    pSeat->set_name(pPlayer->GetPlayerName());
    pSeat->set_sex(pPlayer->GetSex());
    pSeat->set_head_icon(pPlayer->GetHeadIcon());
	pSeat->set_city(pPlayer->GetCity());
    
    SendMsgToAll(&msg,net::S2C_MSG_NOTIFY_PLAYER_JOIN);      
}
void     CGameTable::NotifyPlayerLeave(CGamePlayer* pPlayer)
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
    net::msg_leave_table_rep rep;
    rep.set_result(1);
    pPlayer->SendMsgToClient(&rep,net::S2C_MSG_LEAVE_TABLE_REP);            
}
int64    CGameTable::ChangeScoreValue(uint16 chairID,int64& score,uint16 operType,uint16 subType)
{
    uint8 consumeType = GetConsumeType();
    CGamePlayer* pGamePlayer = GetPlayer(chairID);
    uint32 uid = GetPlayerID(chairID);
    if(pGamePlayer != NULL)
        uid = pGamePlayer->GetUID();
    return ChangeScoreValueByUID(uid,score,operType,subType); 
}
int64   CGameTable::ChangeScoreValueByUID(uint32 uid,int64& score,uint16 operType,uint16 subType)
{
    uint8 consumeType = GetConsumeType();
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    switch(consumeType)
    {
    case net::ROOM_CONSUME_TYPE_SCORE:
    {
        if(pGamePlayer != NULL) {
            pGamePlayer->SyncChangeAccountValue(operType,subType,0,0,0,score,0,0,m_chessid);
        }else{
            CLobbyMgr::Instance().NotifyLobbyChangeAccValue(uid,operType,subType,0,0,0,score,0,0,m_chessid);
        }
    }break;
    case net::ROOM_CONSUME_TYPE_COIN:
    {
        if(pGamePlayer != NULL) {
            pGamePlayer->SyncChangeAccountValue(operType, subType, 0, score, 0, 0, 0,0,m_chessid);
        }else{
            CLobbyMgr::Instance().NotifyLobbyChangeAccValue(uid,operType,subType,0,score,0,0,0,0,m_chessid);
        }
    }break;
    default:
        break;
    }

    return score;    
}
int64   CGameTable::ChangeScoreValueInGame(uint32 uid, int64& score, uint16 operType, uint16 subType,string chessid)
{
	uint8 consumeType = GetConsumeType();
	CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	switch (consumeType)
	{
	case net::ROOM_CONSUME_TYPE_SCORE:
	{
		if (pGamePlayer != NULL) 
		{
			pGamePlayer->SubChangeAccountValue(operType, subType, 0, 0, 0, score, 0, 0, chessid);
		}
		else {
			CLobbyMgr::Instance().UpDateLobbyChangeAccValue(uid, operType, subType, 0, 0, 0, score, 0, 0, chessid);
		}
	}break;
	case net::ROOM_CONSUME_TYPE_COIN:
	{
		if (pGamePlayer != NULL) 
		{
			pGamePlayer->SubChangeAccountValue(operType, subType, 0, score, 0, 0, 0, 0, chessid);
		}
		else
		{
			CLobbyMgr::Instance().UpDateLobbyChangeAccValue(uid, operType, subType, 0, score, 0, 0, 0, 0, chessid);
		}
	}break;
	default:
		break;
	}

	return score;
}

int64   CGameTable::NotifyChangeScoreValueInGame(uint32 uid, int64 score, uint16 operType, uint16 subType)
{
	uint8 consumeType = GetConsumeType();
	CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	switch (consumeType)
	{
	case net::ROOM_CONSUME_TYPE_SCORE:
	{
		CLobbyMgr::Instance().NotifyUpDateLobbyChangeAccValue(uid, operType, subType, 0, 0, 0, score, 0, 0, m_chessid);
	}break;
	case net::ROOM_CONSUME_TYPE_COIN:
	{
		CLobbyMgr::Instance().NotifyUpDateLobbyChangeAccValue(uid, operType, subType, 0, score, 0, 0, 0, 0, m_chessid);
	}break;
	default:
		break;
	}

	return score;
}

uint8   CGameTable::GetConsumeType()
{
    return m_conf.consume;
}
int64   CGameTable::GetBaseScore()
{
    return m_conf.baseScore;
}
int64   CGameTable::GetEnterMin()
{
    return m_conf.enterMin;
}
int64   CGameTable::GetEnterMax()
{
    return m_conf.enterMax;
}
uint32   CGameTable::GetExitChip()
{
    return m_conf.exitchip;
}
uint16   CGameTable::GetUproom()
{
    return m_conf.uproom;
}
int64   CGameTable::GetJettonMin()
{
	if (m_pHostRoom != NULL)
	{
		return m_pHostRoom->GetJettonMin();
	}
	return 0;
}

bool CGameTable::TableJettonLimmit(CGamePlayer * pPlayer,int64 lJettonScore,int64 lAllyJetton)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	int64 curScore = GetEnterScore(pPlayer);

	if (GetJettonMin() > curScore)
	{
		return false;
	}
	return true;
}

void CGameTable::AddEnterScore(CGamePlayer * pPlayer)
{
	if (pPlayer != NULL)
	{
		int64 curScore = GetPlayerCurScore(pPlayer);
		auto iter = m_mpFirstEnterScore.find(pPlayer->GetUID());
		if (iter == m_mpFirstEnterScore.end())
		{
			m_mpFirstEnterScore.insert(make_pair(pPlayer->GetUID(), curScore));
		}
		else
		{
			iter->second = curScore;
		}
	}
}
int64 CGameTable::GetEnterScore(CGamePlayer * pPlayer)
{
	int64 lEnterScore = 0;
	if (pPlayer != NULL)
	{
		auto iter = m_mpFirstEnterScore.find(pPlayer->GetUID());
		if (iter != m_mpFirstEnterScore.end())
		{
			lEnterScore = iter->second;
		}
	}
	return lEnterScore;
}

void CGameTable::RemoveEnterScore(CGamePlayer * pPlayer)
{
	if (pPlayer != NULL)
	{
		auto iter = m_mpFirstEnterScore.find(pPlayer->GetUID());
		if (iter != m_mpFirstEnterScore.end())
		{
			m_mpFirstEnterScore.erase(pPlayer->GetUID());
		}
	}
}

void CGameTable::BuyEnterScore(CGamePlayer * pPlayer,int64 lScore)
{
	if (pPlayer != NULL && lScore > 0)
	{
		auto iter = m_mpFirstEnterScore.find(pPlayer->GetUID());
		if (iter != m_mpFirstEnterScore.end())
		{
			iter->second += lScore;
		}
	}
}

void CGameTable::UpdateEnterScore(bool isJoin, CGamePlayer * pPlayer)
{
	if (pPlayer != NULL)
	{
		if (isJoin)
		{
			AddEnterScore(pPlayer);
		}
		else
		{
			RemoveEnterScore(pPlayer);
		}
	}
}

uint32  CGameTable::GetDeal()
{
    return m_conf.deal;
}
bool    CGameTable::IsShow()
{
    return m_conf.isShow != 0;
}
uint32  CGameTable::GetHostID()
{
    return m_conf.hostID;
}
bool    CGameTable::IsRightPasswd(string passwd)
{
	if (GetTableType() != emTABLE_TYPE_PLAYER)
	{
		return true;
	}
        
	if (m_conf.passwd != passwd)
	{
		return false;
	}

    return true;
}
int64   CGameTable::GetPlayerCurScore(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return 0;
	}

    uint8 consumeType = GetConsumeType();
    switch(consumeType)
    {
    case net::ROOM_CONSUME_TYPE_SCORE:
        {
            return pPlayer->GetAccountValue(emACC_VALUE_SCORE);
        }break;
    case net::ROOM_CONSUME_TYPE_COIN:
        {
            return pPlayer->GetAccountValue(emACC_VALUE_COIN);
        }break;
    default:
        break;
    }
    return 0;
}
int64   CGameTable::ChangePlayerCurScore(CGamePlayer* pPlayer,int64 score)
{
	if (pPlayer == NULL)
	{
		return 0;
	}

	LOG_DEBUG("1 uid:%d,lCurScore:%lld,score:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), score);

    uint8 consumeType = GetConsumeType();
    switch(consumeType)
    {
    case net::ROOM_CONSUME_TYPE_SCORE:
        {
			int64 lvalue = pPlayer->ChangeAccountValue(emACC_VALUE_SCORE, score);
			LOG_DEBUG("2 uid:%d,lCurScore:%lld,lvalue:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), lvalue);
			return lvalue;
        }break;
    case net::ROOM_CONSUME_TYPE_COIN:
        {
			int64 lvalue = pPlayer->ChangeAccountValue(emACC_VALUE_COIN, score);
			LOG_DEBUG("3 uid:%d,lCurScore:%lld,lvalue:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), lvalue);
			return lvalue;
        }break;
    default:
        break;
    }
    return 0;    
}

bool CGameTable::StopServer()
{
	return true;
}

void    CGameTable::RenewPrivateTable(uint32 days)
{
    if(m_conf.dueTime < getSysTime()){
        m_conf.dueTime = getSysTime() + days*SECONDS_IN_ONE_DAY;
    }else{
        m_conf.dueTime += days*SECONDS_IN_ONE_DAY;
    }
}
void    CGameTable::LoadPrivateTable(stPrivateTable& data)
{
    m_conf.baseScore    = data.baseScore;
    m_conf.consume      = data.consume;
    m_conf.deal         = data.deal;
    m_conf.dueTime      = data.dueTime;
    m_conf.enterMin     = data.enterMin;
    m_conf.feeType      = data.feeType;
    m_conf.feeValue     = data.feeValue;

    m_conf.hostID       = data.hostID;
    m_hostIncome        = data.hostIncome;
    m_conf.hostName     = data.hostName;
    m_conf.isShow       = data.isShow;
    m_conf.passwd       = data.passwd;
    m_sysIncome         = data.sysIncome;
    m_tableID           = data.tableID;
    m_conf.tableName    = data.tableName;

}
bool    CGameTable::CreatePrivateTable()
{
    stPrivateTable table;
    table.baseScore = m_conf.baseScore;
    table.consume   = m_conf.consume;
    table.createTime = getSysTime();
    table.deal      = m_conf.deal;
    table.dueTime   = m_conf.dueTime;
    table.enterMin  = m_conf.enterMin;
    table.feeType   = m_conf.feeType;
    table.feeValue  = m_conf.feeValue;
    table.gameType  = m_pHostRoom->GetGameType();
    table.hostID    = m_conf.hostID;
    table.hostIncome = m_hostIncome;
    table.hostName  = m_conf.hostName;
    table.isShow    = m_conf.isShow;
    table.passwd    = m_conf.passwd;
    table.sysIncome = m_sysIncome;
    table.tableID   = GetTableID();
    table.tableName = m_conf.tableName;

    m_tableID = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).CreatePrivateTable(table);
    return m_tableID != 0;
}
void    CGameTable::ChangePrivateTableIncome(int64 hostIncome,int64 sysIncome)
{
    if(GetTableType() == emTABLE_TYPE_PLAYER)//私人房
    {
        CDBMysqlMgr::Instance().ChangePrivateTableIncome(GetTableID(),hostIncome,sysIncome);
        CLobbyMgr::Instance().NotifyLobbyChangeAccValue(GetHostID(),emACCTRAN_OPER_TYPE_FEE,GetTableID(),0,hostIncome,0,0,0,0);
        m_hostIncome += hostIncome;
        m_sysIncome  += sysIncome;
    }
    
}
void    CGameTable::UpdatePrivateTableDuetime()
{
    CDBMysqlMgr::Instance().UpdatePrivateTableDuetime(GetTableID(),m_conf.dueTime);
}

void CGameTable::InitChessID()
{
	m_chessid = CStringUtility::FormatToString("%d-%d-%d-%d", GetGameType(), GetRoomID(), GetTableID(), getSysTime());
}

void CGameTable::SetEmptyChessID()
{
	m_chessid = "";
	m_chessid.clear();
}

// 牌局日志
void    CGameTable::InitBlingLog(bool bNeedRead)
{
	if (m_chessid.empty())
	{
		InitChessID();
	}

    m_blingLog.Reset();
    m_blingLog.baseScore = m_conf.baseScore;
    m_blingLog.consume   = m_conf.consume;
    m_blingLog.deal      = m_conf.deal;
    m_blingLog.startTime = getSysTime();
    m_blingLog.gameType  = m_pHostRoom->GetGameType();
    m_blingLog.roomType  = m_pHostRoom->GetRoomType();
    m_blingLog.tableID   = GetTableID();
    m_blingLog.chessid   = m_chessid;
    m_blingLog.roomID    = m_pHostRoom->GetRoomID();
	m_blingLog.welctrl = GetChessWelfare();
	int welfare = 0;
	welfare = m_pHostRoom->GetNoviceWelfare();
	if (welfare == 0)
	{
		welfare = m_pHostRoom->GetNoviceWelfareOwe();
	}
	m_blingLog.welfare = welfare;

    for(uint16 i=0;i<m_vecPlayers.size();++i) {
        stBlingUser user;
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer == NULL || (bNeedRead && m_vecPlayers[i].readyState == 0))
		{
			continue;
		}
        user.uid        = pPlayer->GetUID();
		user.playerType = pPlayer->GetPlayerType();
		user.welfare = pPlayer->IsNoviceWelfare();
        user.oldValue   = GetPlayerCurScore(pPlayer);
        user.safeCoin   = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
        user.chairid    = i;
		user.totalwinc = GetPlayerTotalWinScore(pPlayer);
		user.stockscore = pPlayer->GetPlayerStockScore(GetGameType());
		user.gamecount = pPlayer->GetPlayerGameCount(GetGameType());
        m_blingLog.users.push_back(user);
    }
    m_operLog.clear();
}

//单人游戏牌局
// 牌局日志  
void CGameTable::InitBlingLog(CGamePlayer* pPlayer)
{
	m_chessid = CStringUtility::FormatToString("%d-%d-%d-%d", m_pHostRoom->GetGameType(), m_pHostRoom->GetRoomID(), GetTableID(), getSysTime());

	m_blingLog.Reset();
	m_blingLog.baseScore = m_conf.baseScore;
	m_blingLog.consume = m_conf.consume;
	m_blingLog.deal = m_conf.deal;
	m_blingLog.startTime = getSysTime();
	m_blingLog.gameType = m_pHostRoom->GetGameType();
	m_blingLog.roomType = m_pHostRoom->GetRoomType();
	m_blingLog.tableID = GetTableID();
	m_blingLog.chessid = m_chessid;
	m_blingLog.roomID = m_pHostRoom->GetRoomID();
	m_blingLog.welctrl = GetChessWelfare();
	int welfare = 0;
	welfare = m_pHostRoom->GetNoviceWelfare();
	if (welfare == 0)
	{
		welfare = m_pHostRoom->GetNoviceWelfareOwe();
	}
	m_blingLog.welfare = welfare;

	stBlingUser user;
	user.uid = pPlayer->GetUID();
	user.playerType = pPlayer->GetPlayerType();
	user.welfare = pPlayer->IsNoviceWelfare();
	user.oldValue = GetPlayerCurScore(pPlayer);
	user.safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
	user.chairid = 0;
	user.totalwinc = GetPlayerTotalWinScore(pPlayer);
	user.stockscore = pPlayer->GetPlayerStockScore(GetGameType());
	user.gamecount = pPlayer->GetPlayerGameCount(GetGameType());

	m_blingLog.users.push_back(user);
	m_operLog.clear();
}

void    CGameTable::AddUserBlingLog(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return;
	}
    
    for(uint32 i=0;i<m_blingLog.users.size();++i){
        if(m_blingLog.users[i].uid == pPlayer->GetUID()){
            return;
        }
    }
    stBlingUser user;
    user.uid        = pPlayer->GetUID();
	user.playerType = pPlayer->GetPlayerType();
	user.welfare = pPlayer->IsNoviceWelfare();
    user.oldValue   = GetPlayerCurScore(pPlayer);
    user.safeCoin   = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
    user.chairid    = GetChairID(pPlayer);
	user.totalwinc = GetPlayerTotalWinScore(pPlayer);
	user.gamecount = pPlayer->GetPlayerGameCount(GetGameType());
    m_blingLog.users.push_back(user);    
}
void    CGameTable::FlushUserBlingLog(CGamePlayer* pPlayer,int64 winScore, int64 lJackpotScore, uint8 land)
{
    for(uint32 i=0;i<m_blingLog.users.size();++i)
	{
        if(m_blingLog.users[i].uid == pPlayer->GetUID()){
            m_blingLog.users[i].win      = winScore;
            m_blingLog.users[i].newValue = GetPlayerCurScore(pPlayer);
            m_blingLog.users[i].safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN); 
            m_blingLog.users[i].land     = land;
			m_blingLog.users[i].totalwinc = GetPlayerTotalWinScore(pPlayer);
			m_blingLog.users[i].gamecount = pPlayer->GetPlayerGameCount(GetGameType());
			m_blingLog.users[i].lJackpotScore = lJackpotScore;
            return;
        }
    }      
}
void    CGameTable::FlushUserNoExistBlingLog(uint32 uid, int64 winScore, int64 fee, uint8 land)
{
	for (uint32 i = 0; i<m_blingLog.users.size(); ++i) {
		if (m_blingLog.users[i].uid == uid) {
			m_blingLog.users[i].win = winScore;
			m_blingLog.users[i].newValue = m_blingLog.users[i].newValue + winScore;
			m_blingLog.users[i].land = land;
			m_blingLog.users[i].fee += fee;
			return;
		}
	}
}

void    CGameTable::ChangeUserBlingLogFee(uint32 uid,int64 fee)
{
    for(uint32 i=0;i<m_blingLog.users.size();++i){
        if(m_blingLog.users[i].uid == uid){
            m_blingLog.users[i].fee += fee;
            return;
        }
    }    
}
void    CGameTable::SaveBlingLog()
{
	if (m_operLog.toFastString().size() <= 0)
	{
		return;
	}
    m_blingLog.endTime = getSysTime();
    m_blingLog.operLog << m_operLog.toFastString();
    CCenterLogMgr::Instance().WriteGameBlingLog(m_blingLog);
	LOG_DEBUG("write m_operLog:%s", m_operLog.toFastString().c_str());
}
// 扣除开始台费
int64    CGameTable::DeductStartFee(bool bNeedReady)
{
    LOG_DEBUG("Deduct Start Fee");
    if(m_conf.feeType == TABLE_FEE_TYPE_ALLBASE)
    {
        int64 fee = -(m_conf.baseScore * m_conf.feeValue/PRO_DENO_10000);
        for(uint32 i=0;i<m_vecPlayers.size();++i)
        {
            CGamePlayer* pPlayer = GetPlayer(i);
			if (pPlayer == NULL || (bNeedReady && m_vecPlayers[i].readyState == 0))
			{
				continue;
			}
            ChangeScoreValue(i,fee,emACCTRAN_OPER_TYPE_FEE,GetTableID());
            // 处理桌子收益，房主收益
            m_hostIncome += -fee;
            ChangePrivateTableIncome(-fee,0);
            ChangeUserBlingLogFee(pPlayer->GetUID(),fee);
            LogFee(pPlayer->GetUID(),0,-fee);
        }
        SendSeatInfoToClient();
    }

    return 0;
}
// 扣除结算台费
int64    CGameTable::DeducEndFee(uint32 uid,int64& winScore)
{   
    //LOG_DEBUG("Deduc End Fee");
    int64 fee = 0;
    if(m_conf.feeType == TABLE_FEE_TYPE_WIN)
    {
        if(winScore > 0){        
            fee = -(winScore * m_conf.feeValue/PRO_DENO_10000);
            winScore += fee;
            // 处理桌子收益，房主收益
            m_hostIncome += -fee;
            ChangePrivateTableIncome(-fee,0);
            ChangeUserBlingLogFee(uid,fee);
            LogFee(uid,-fee,0);

            return fee;
        }else{
            fee = -(winScore * m_conf.feeValue/PRO_DENO_10000);
            LogFee(uid,0,fee);
        }        
    }

    return 0;
}
// 上报游戏战报
void    CGameTable::ReportGameResult(uint32 uid, int64 winScore, int64 lExWinScore, bool bBranker, int64 lBetScore)
{
	net::msg_report_game_result msg;
	msg.set_uid(uid);
	msg.set_consume(GetConsumeType());
	msg.set_game_type(m_pHostRoom->GetGameType());
	msg.set_roomid(GetRoomID());
	msg.set_win_score(winScore);
	msg.set_ex_win_score(lExWinScore);
	//CGamePlayer* pPlayerUid = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	//if (pPlayerUid != NULL)
	//{
	msg.set_branker(bBranker);
	msg.set_bet_score(lBetScore);

	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	int welcount = 0;
	uint64 weltime = 0;
	if (pPlayer != NULL)
	{
		welcount = pPlayer->GetWelCount();
		weltime = pPlayer->GetWelTime();
	}
	msg.set_welcount(welcount);
	msg.set_weltime(weltime);

    CLobbyMgr::Instance().SendMsg2Client(&msg,net::S2L_MSG_REPORT_GAME_RESULT,uid);  
    
}
// 玩家贡献度记录
void    CGameTable::LogFee(uint32 uid,int64 feewin,int64 feelose)
{
    CDBMysqlMgr::Instance().ChangeFeeValue(uid,feewin,feelose);

    net::msg_report_fee_log msg;
    msg.set_uid(uid);
    msg.set_fee_win(feewin);
    msg.set_fee_lose(feelose);

    CLobbyMgr::Instance().SendMsg2Client(&msg,net::S2L_MSG_REPORT_FEE_LOG,uid);    
}

int64 CGameTable::GetBankruptScore()
{
	int64 lBankruptScore = 0;
	switch (GetGameType())
	{
		case net::GAME_CATE_BULLFIGHT:
		case net::GAME_CATE_BACCARAT:
		case net::GAME_CATE_DICE:
		case net::GAME_CATE_WAR:
		case net::GAME_CATE_FIGHT:
		case net::GAME_CATE_FRUIT_MACHINE:
		case net::GAME_CATE_TWOEIGHT:
		case net::GAME_CATE_CARCITY:
		{
			lBankruptScore = m_pHostRoom->GetJettonMin();
		}break;
		default:
		{
			lBankruptScore = m_pHostRoom->GetEnterMin();
		}break;
	}
	return lBankruptScore;
}

// 结算玩家信息
int64    CGameTable::CalcPlayerGameInfo(uint32 uid,int64& winScore, int64 lExWinScore, bool bDeductFee, bool bBranker, int64 lBetScore)
{
    //LOG_DEBUG("calc player game info:%d  %lld",uid,winScore);
    //ReportGameResult(uid,winScore);
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    int64 fee = 0;
	int64 lPlayerWinScore = winScore + lExWinScore;
    if(pPlayer != NULL)
    {
        if(bDeductFee)
		{
			if (winScore != 0)
			{
				fee = DeducEndFee(uid, winScore);
			}
        }
		lPlayerWinScore = winScore + lExWinScore;
		if (lPlayerWinScore != 0)
		{
			ChangeScoreValueByUID(uid, lPlayerWinScore, emACCTRAN_OPER_TYPE_GAME, m_pHostRoom->GetGameType());
		}
        pPlayer->AsyncChangeGameValue(m_pHostRoom->GetGameType(),isCoin, lPlayerWinScore, lExWinScore, GetChessWelfare());
        FlushUserBlingLog(pPlayer, lPlayerWinScore, lExWinScore);
		OnSetPlayerWinScore(uid, lPlayerWinScore);
		UpdateMasterUserShowInfo(pPlayer, lPlayerWinScore);
		if (lExWinScore > 0)
		{
			pPlayer->ReSetGameCount(GetGameType());
		}
        pPlayer->OnGameEnd();
		
		if (GetPlayerCurScore(pPlayer) >= GetBankruptScore())
		{
			pPlayer->SetBankruptRecord(true);
		}

		if (m_tagControlPalyer.id != -1 && m_tagControlPalyer.uid == pPlayer->GetUID())
		{
			m_tagControlPalyer.escore = GetPlayerCurScore(pPlayer);
		}
		auto it_palyer = m_mpControlMultiPalyer.find(pPlayer->GetUID());
		if (it_palyer != m_mpControlMultiPalyer.end())
		{
			tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			ControlMultiPalyer.escore = GetPlayerCurScore(pPlayer);
		}
    }
	if (uid != 0)
	{
		ReportGameResult(uid, lPlayerWinScore,lExWinScore, bBranker,lBetScore);
	}
    return fee;
}
// 是否需要回收
bool    CGameTable::NeedRecover()
{
    if(GetTableType() == emTABLE_TYPE_SYSTEM){
        return false;
    }
    if(GetTableType() == emTABLE_TYPE_PLAYER)
    {
        if(m_conf.dueTime < getSysTime()){
            if(IsEmptyTable()){
                return true;
            }
        }
    }
    return false;    
}    
//用户断线或重连
bool    CGameTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
{
    if(bConnected)//断线重连
    {
        if(isJoin)
        {
            pPlayer->SetPlayDisconnect(false);
            PlayerSetAuto(pPlayer,0);
            SendTableInfoToClient(pPlayer);
			SendSeatInfoToClient();
            SendGameScene(pPlayer);
        }
    }else{
        pPlayer->SetPlayDisconnect(true);
		SendSeatInfoToClient(); // add by har
    }
    //SendSeatInfoToClient(); 断线重连发了两次桌面信息会导致客户端刷新金币异常，注释掉 delete by har
    SendReadyStateToClient();
    LOG_DEBUG("OnActionUserNetState:%d--%d--%d",pPlayer->GetUID(),bConnected,isJoin);
    return true;
}
//用户坐下
bool    CGameTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{


    SendSeatInfoToClient();
    return true;
}
//用户起立
bool    CGameTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{

    
    SendSeatInfoToClient();
    return true;
}
//用户同意
bool    CGameTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    return true;
}
//玩家进入或离开
void    CGameTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	bool bIsRobot = true;
	if (pPlayer != NULL) {
		uid = pPlayer->GetUID();
		bIsRobot = pPlayer->IsRobot();
	}
	if (bIsRobot == false)
	{
		LOG_DEBUG("all_table_in - roomid:%d,tableid:%d,uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld",
			GetRoomID(), GetTableID(), uid, chairID, isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	}
    if(NeedSitDown())
	{
        if(isJoin)
		{
            AddLooker(pPlayer);
            SendLookerListToClient(pPlayer);
        }
		else
		{
            RemoveLooker(pPlayer);            
        } 
   }
   SendSeatInfoToClient();
   //if(GetGameState() == TABLE_STATE_FREE){
       SendReadyStateToClient();
   //}

	//判断是否为控制玩家
	if (pPlayer->GetCtrlFlag())
	{
		if (isJoin)
		{
			m_tableCtrlPlayers.insert(pPlayer->GetUID());
		}
		else
		{
			m_tableCtrlPlayers.erase(m_tableCtrlPlayers.find(pPlayer->GetUID()));
		}
	}

}
// 发送场景信息(断线重连)
void    CGameTable::SendGameScene(CGamePlayer* pPlayer)
{
    
}

uint16	CGameTable::GetRobotPlayerCount() {
	uint16 ucount = 0;
	for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
	{
		CGamePlayer * pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer!=NULL && pPlayer->IsRobot()){
			ucount++;
		}
	}
	return ucount;
}

bool CGameTable::ReAnalysisParam() {
	return true;
}

bool CGameTable::UpdateControlPlayer(int64 id, uint32 uid, uint32 gamecount, uint32 operatetype){
	if (uid == 0 || gamecount == 0 || operatetype>GAME_CONTROL_LOST || id<0)
	{
		LOG_ERROR("add control palyer failed - roomid:%d,tableid:%d,id:%lld,uid:%d,gamecount:%d,operatetype:%d", m_pHostRoom->GetRoomID(),GetTableID(), id, uid, gamecount, operatetype);
		return false;
	}

	if (m_tagControlPalyer.id != -1)
	{
		if (GetControlPalyerGame(GetGameType()))
		{
			m_tagControlPalyer.etime = getSysTime();
			CGamePlayer * pGamePlayer = GetGamePlayerByUid(uid);
			if (pGamePlayer != NULL)
			{
				m_tagControlPalyer.escore = GetPlayerCurScore(pGamePlayer);
			}
			CDBMysqlMgr::Instance().SaveControlPlayerData(m_tagControlPalyer);
		}
	}

	m_tagControlPalyer.Init();

	if (operatetype == GAME_CONTROL_CANCEL)
	{
		m_tagControlPalyer.Init();
	}
	else if(operatetype <= GAME_CONTROL_LOST)
	{
		m_tagControlPalyer.id = id;
		m_tagControlPalyer.uid = uid;
		m_tagControlPalyer.count = gamecount;
		m_tagControlPalyer.type = operatetype;
		m_tagControlPalyer.utime = getTickCount64();
		if (m_tagControlPalyer.bscore == -1)
		{
			CGamePlayer * pGamePlayer = GetGamePlayerByUid(uid);
			if (pGamePlayer != NULL)
			{
				m_tagControlPalyer.bscore = GetPlayerCurScore(pGamePlayer);
				m_tagControlPalyer.escore = m_tagControlPalyer.bscore;
			}
		}
	}

	LOG_ERROR("add control palyer success - roomid:%d,tableid:%d,uid:%d,gamecount:%d,operatetype:%d,bscore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), uid, gamecount, operatetype, m_tagControlPalyer.bscore);

	return true;
}

bool CGameTable::UpdateControlMultiPlayer(int64 id, uint32 uid, uint32 gamecount,uint64 gametime,int64 totalscore,uint32 operatetype)
{
	bool bIsRetFlag = true;
	if (id <0 || uid == 0 || operatetype>GAME_CONTROL_MULTI_ALL_CANCEL)
	{
		LOG_ERROR("add control palyer failed - roomid:%d,tableid:%d,uid:%d,gamecount:%d,gametime:%lld,operatetype:%d", m_pHostRoom->GetRoomID(), GetTableID(), uid, gamecount, gametime, operatetype);
		//return false;
		bIsRetFlag = false;
	}
	else if (operatetype == GAME_CONTROL_MULTI_ALL_CANCEL)
	{
		if (GetControlPalyerGame(GetGameType()))
		{
			auto it_palyer = m_mpControlMultiPalyer.begin();
			for (; it_palyer != m_mpControlMultiPalyer.end(); it_palyer++)
			{
				tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
				ControlMultiPalyer.etime = getSysTime();
				CGamePlayer * pGamePlayer = GetGamePlayerByUid(ControlMultiPalyer.uid);
				if (pGamePlayer != NULL)
				{
					ControlMultiPalyer.escore = GetPlayerCurScore(pGamePlayer);
				}
				CDBMysqlMgr::Instance().SaveControlPlayerData(ControlMultiPalyer);
			}
		}
		m_mpControlMultiPalyer.clear();
	}
	else if (operatetype == GAME_CONTROL_MULTI_CANCEL)
	{
		auto it_palyer = m_mpControlMultiPalyer.find(uid);
		if (it_palyer != m_mpControlMultiPalyer.end())
		{
			tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			ControlMultiPalyer.etime = getSysTime();
			if (GetControlPalyerGame(GetGameType()))
			{
				CGamePlayer * pGamePlayer = GetGamePlayerByUid(ControlMultiPalyer.uid);
				if (pGamePlayer != NULL)
				{
					ControlMultiPalyer.escore = GetPlayerCurScore(pGamePlayer);
				}
				CDBMysqlMgr::Instance().SaveControlPlayerData(ControlMultiPalyer);
			}
			m_mpControlMultiPalyer.erase(it_palyer);
		}
		else
		{
			bIsRetFlag = false;
		}
	}
	else if (operatetype == GAME_CONTROL_MULTI_WIN || operatetype == GAME_CONTROL_MULTI_LOST)
	{
		tagControlMultiPalyer ControlMultiPalyer;
		ControlMultiPalyer.id = id;
		ControlMultiPalyer.uid = uid;
		ControlMultiPalyer.count = gamecount;
		ControlMultiPalyer.type = operatetype;
		ControlMultiPalyer.ctime = gametime;
		ControlMultiPalyer.stime = getTickCount64();
		ControlMultiPalyer.etime = ControlMultiPalyer.stime + (gametime * 1000);
		ControlMultiPalyer.tscore = totalscore;
		CGamePlayer * pGamePlayer = GetGamePlayerByUid(uid);
		if (pGamePlayer != NULL)
		{
			ControlMultiPalyer.bscore = GetPlayerCurScore(pGamePlayer);
			ControlMultiPalyer.escore = ControlMultiPalyer.bscore;
		}

		auto it_palyer = m_mpControlMultiPalyer.find(uid);
		if (it_palyer != m_mpControlMultiPalyer.end())
		{
			m_mpControlMultiPalyer.erase(it_palyer);
		}

		pair< map<uint32, tagControlMultiPalyer>::iterator, bool > paMultiInsertRet;
		paMultiInsertRet = m_mpControlMultiPalyer.insert(make_pair(uid, ControlMultiPalyer));

		bIsRetFlag = paMultiInsertRet.second;
	}
	else
	{
		bIsRetFlag = false;
	}

	LOG_ERROR("add control palyer success - roomid:%d,tableid:%d,uid:%d,gamecount:%d,gametime:%lld,operatetype:%d,bIsRetFlag:%d", m_pHostRoom->GetRoomID(), GetTableID(), uid, gamecount, gametime, operatetype, bIsRetFlag);
	
	uint64 uSysTime = getTickCount64();

	if (m_mpControlMultiPalyer.size()>0)
	{
		auto it_palyer = m_mpControlMultiPalyer.begin();
		for (; it_palyer != m_mpControlMultiPalyer.end(); it_palyer++)
		{
			tagControlMultiPalyer ControlMultiPalyer = it_palyer->second;

			LOG_ERROR("show control mutil palyer - roomid:%d,tableid:%d,uid:%d,gamecount:%d,ctime:%lld,type:%d,etime:%lld,uSysTime:%lld", m_pHostRoom->GetRoomID(), GetTableID(), ControlMultiPalyer.uid, ControlMultiPalyer.count, ControlMultiPalyer.ctime, ControlMultiPalyer.type, ControlMultiPalyer.etime, uSysTime);
		}
	}
	
	return bIsRetFlag;
}

//停止夺宝
bool	CGameTable::SetSnatchCoinState(uint32 stop)
{
	return false;
}

bool	CGameTable::RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount)
{
	return false;
}

bool	CGameTable::DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[])
{
	return false;
}

bool CGameTable::MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard)
{
	return false;
}

bool CGameTable::SynControlPlayer(uint32 uid, int32 gamecount, uint32 operatetype) {

	if (m_tagControlPalyer.uid == uid && m_tagControlPalyer.type == operatetype && m_tagControlPalyer.count >= gamecount)
	{
		m_tagControlPalyer.rccount++;
		m_tagControlPalyer.count += gamecount;
		if (m_tagControlPalyer.count<=0)
		{
			// save
			if (m_tagControlPalyer.id != -1)
			{
				if (GetControlPalyerGame(GetGameType()))
				{
					m_tagControlPalyer.etime = getSysTime();
					CGamePlayer * pGamePlayer = GetGamePlayerByUid(uid);
					if (pGamePlayer != NULL)
					{
						m_tagControlPalyer.escore = GetPlayerCurScore(pGamePlayer);
					}
					CDBMysqlMgr::Instance().SaveControlPlayerData(m_tagControlPalyer);
				}
			}
			m_tagControlPalyer.Init();
		}
	}
	else
	{
		m_tagControlPalyer.Init();
	}

	LOG_ERROR("syn control palyer success - roomid:%d,tableid:%d,uid:%d,gamecount:%d,operatetype:%d, m_tagControlPalyer.uid:%d,count:%d,type:%d,rccount:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), uid, gamecount, operatetype, m_tagControlPalyer.uid, m_tagControlPalyer.count, m_tagControlPalyer.type, m_tagControlPalyer.rccount);


	return true;
}

bool CGameTable::GetControlPalyerGame(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
	{
		return true;
	}break;
	default:
		return false;
	}
	return false;
}

bool CGameTable::GetIsGameEndKickUser(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_TWOEIGHT:
	{
		return true;
	}break;
	default:
		return false;
	}
	return false;
}

void CGameTable::KickLittleScoreUser()
{
	if (GetIsGameEndKickUser(GetGameType()))
	{
		bool bIsUpdate = false;
		for (uint32 i = 0; i < m_vecPlayers.size(); i++)
		{
			int64 lCurScore = GetPlayerCurScore(m_vecPlayers[i].pPlayer);
			if (m_vecPlayers[i].pPlayer != NULL && lCurScore < 1000)
			{
				ForcePlayerSitDownStandUp(m_vecPlayers[i].pPlayer, false, i);
				bIsUpdate = true;
			}
		}
		if (bIsUpdate)
		{
			SendSeatInfoToClient();
			SendLookerListToClient();
		}
	}
}

void CGameTable::OnTableGameEnd()
{
	//CalculateDeity();
	SetEmptyChessID();
	SaveUserBankruptScore();
	OnCaclBairenCount();
	CaclPlayerBaiRenCount();
	RecordFrontJettonInfo();
	KickLittleScoreUser();
	SetChessWelfare(0);

	//精准控制百人场处理
	OnBrcControlGameEnd();
	LeaveRobotUser();
	UpdateMasterGameInfo();
	SetIsAllRobotOrPlayerJetton(true); // add by har
}

void CGameTable::CountGameRound(int maxcount)
{
	m_iGameCount++;
	if (m_iGameCount > maxcount)
	{
		m_iGameCount = 0;
	}
}

void CGameTable::OnTableGameStart()
{
	SendAllFrontJettonInfo();
	SetChessWelfare(0);
	CountGameRound(72);
}

void CGameTable::SaveUserBankruptScore()
{
	for (uint32 i = 0; i<m_blingLog.users.size(); ++i)
	{
		bool bIsJetton = false;
		if (m_blingLog.users[i].win != 0)
		{
			bIsJetton = true;
		}
		if (m_blingLog.users[i].fee != 0)
		{
			bIsJetton = true;
		}
		if (m_blingLog.users[i].playerType == PLAYER_TYPE_ONLINE && m_pHostRoom != NULL && bIsJetton)
		{
			CGamePlayer * pGamePlayer = GetGamePlayerByUid(m_blingLog.users[i].uid);
			LOG_DEBUG("uid:%d,pGamePlayer:%p", m_blingLog.users[i].uid,pGamePlayer);
			if (m_blingLog.users[i].newValue < GetBankruptScore() && pGamePlayer!=NULL && pGamePlayer->GetBankruptRecord())
			{
				pGamePlayer->SetBankruptRecord(false);

				CDBMysqlMgr::Instance().AsyncSaveUserBankruptScore(m_blingLog.users[i].uid, m_pHostRoom->GetGameType(), GetRoomID(), m_blingLog.users[i].oldValue, m_blingLog.users[i].newValue, m_pHostRoom->GetEnterMin(), getSysTime());
			}
		}
	}
}
int64 CGameTable::GetJettonScore(CGamePlayer *pPlayer) {
	return 0;
}

bool CGameTable::UpdateMaxJettonScore(CGamePlayer *pPlayer, int64 lScore) {
	return false;
}

bool	CGameTable::UpdataScoreInGame(uint32 uid, uint32 gametype, int64 lScore)
{
	if (lScore == 0)	return false;
	net::msg_ingame_update_score msg;
	msg.set_uid(uid);
	msg.set_game_type(gametype);
	msg.set_diff_score(lScore);

	SendMsgToAll(&msg, net::S2C_MSG_INGAME_UPDATE_SCORE);
	return true;
}

int64	CGameTable::GetPlayerTotalWinScore(CGamePlayer *pPlayer)
{
	int64 lPlayerTotalWinScore = 0;
	if (pPlayer != NULL)
	{
		lPlayerTotalWinScore = pPlayer->GetPlayerTotalWinScore(GetGameType());
	}
	return lPlayerTotalWinScore;
}
//发送游戏广播到PHP
void CGameTable::SaveBroadcastLog(stGameBroadcastLog &BroadcastLog) {
	CCenterLogMgr::Instance().WriteGameBroadcastLog(BroadcastLog);
}

void CGameTable::OnAddPlayerJetton(uint32 uid, int64 score)
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()))
	{
		auto iter = m_mpBairenPalyerBet.find(uid);
		if (iter != m_mpBairenPalyerBet.end())
		{
			iter->second->betscore += score;
		}
		else
		{
			struct tagBairenCount BairenCount;
			BairenCount.chessid = m_chessid;
			BairenCount.uid = uid;
			BairenCount.svrid = 255;
			BairenCount.gametype = GetGameType();
			BairenCount.roomid = GetRoomID();
			BairenCount.tableid = GetTableID();
			BairenCount.uid = uid;
			BairenCount.winscore = 0;
			BairenCount.betscore = score;
			std::shared_ptr<struct tagBairenCount> spBairenCount = std::make_shared<struct tagBairenCount>(BairenCount);
			pair< std::map<uint32, std::shared_ptr<struct tagBairenCount>>::iterator, bool > paBairenCountRet;
			paBairenCountRet = m_mpBairenPalyerBet.insert(make_pair(uid, spBairenCount));
			bool bIsRetFlag = paBairenCountRet.second;
		}

		CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
		if (pPlayer != NULL)
		{
			pPlayer->SetVecBet(GetGameType(), score);
			if (pPlayer->IsRobot() == false)
			{
				LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,score:%lld,wincount:%d,betscore:%lld,betcount:%d",
					GetRoomID(), GetTableID(), pPlayer->GetUID(), score, pPlayer->GetVecWin(GetGameType()), pPlayer->GetVecBet(GetGameType()), pPlayer->GetBetCount(GetGameType()));
			}
		}
	}
}

void CGameTable::RecordPlayerBaiRenJettonInfo(CGamePlayer * pPlayer, BYTE area, int64 score)
{
	if (ContinuousPressureBaiRenGame()==false || pPlayer == NULL || pPlayer->IsRobot())
	{
		return;
	}
	uint32  uid = pPlayer->GetUID();

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,area:%d,score:%lld",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), area, score);

	auto iter = m_mpCurPlayerJettonInfo.find(uid);
	if (iter != m_mpCurPlayerJettonInfo.end())
	{
		auto iter_area = iter->second.find(area);
		if (iter_area != iter->second.end())
		{
			iter_area->second += score;
		}
		else
		{
			iter->second.insert(make_pair(area, score));
		}
	}
	else
	{
		std::map<BYTE, int64> mpAreaInfo;
		pair< std::map<BYTE, int64>::iterator, bool > paAreaInfoRet;
		paAreaInfoRet = mpAreaInfo.insert(make_pair(area, score));
		bool bIsRetFlag = paAreaInfoRet.second;
		if (bIsRetFlag)
		{
			m_mpCurPlayerJettonInfo.insert(make_pair(uid, mpAreaInfo));
		}
	}
}

bool CGameTable::ContinuousPressureBaiRenGame()
{
	switch (GetGameType())
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
	{
		return true;
	}break;
	default:
		return false;
	}
	return false;
}

void CGameTable::RecordFrontJettonInfo()
{
	if (ContinuousPressureBaiRenGame())
	{
		m_spFrontPlayerJettonInfo = nullptr;
		m_spFrontPlayerJettonInfo = std::make_shared<std::map<uint32, std::map<BYTE, int64>>>(m_mpCurPlayerJettonInfo);
		m_mpCurPlayerJettonInfo.clear();
	}
}

void CGameTable::SendAllFrontJettonInfo()
{
	if (ContinuousPressureBaiRenGame())
	{
		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
		{
			if (m_vecPlayers[i].pPlayer != NULL)
			{
				SendFrontJettonInfo(m_vecPlayers[i].pPlayer);
			}
		}
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer != NULL)
			{
				SendFrontJettonInfo(pPlayer);
			}
		}
	}
}

void CGameTable::SendFrontJettonInfo(CGamePlayer * pPlayer)
{
	if (ContinuousPressureBaiRenGame())
	{
		if (m_spFrontPlayerJettonInfo != nullptr && pPlayer != NULL && pPlayer->IsRobot() == false)
		{
			net::msg_continuous_pressure_jetton_info_rep msg;
			auto iter_uid = m_spFrontPlayerJettonInfo->find(pPlayer->GetUID());
			if (iter_uid != m_spFrontPlayerJettonInfo->end())
			{
				auto iter_area = iter_uid->second.begin();
				for (; iter_area != iter_uid->second.end(); iter_area++)
				{
					if (iter_area->second == 0)
					{
						continue;
					}
					//LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,area:%d,score:%lld",	GetRoomID(),GetTableID(), pPlayer->GetUID(), iter_area->first, iter_area->second);
					net::bairen_jetton_info * pinfo = msg.add_info();
					pinfo->set_area(iter_area->first);
					pinfo->set_score(iter_area->second);
				}
			}
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAIREN_GAME_CONTINUOUS_PRESSURE_REP);
		}
	}
}
void CGameTable::OnCaclBairenCount()
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()) && m_mpBairenPalyerBet.size() > 0)
	{
		auto mpBairenPalyerBet = std::make_shared<std::map<uint32, std::shared_ptr<struct tagBairenCount>>>(m_mpBairenPalyerBet);
		m_vecBairenCount.push_back(mpBairenPalyerBet);
		if (m_vecBairenCount.size() > MAX_STATISTICS_GAME_COUNT)
		{
			//auto iter_vec = m_vecBairenCount.begin();
			//if (iter_vec != m_vecBairenCount.end())
			//{
			//	auto tempMpPlayerBet = (*iter_vec);
			//	if (tempMpPlayerBet != nullptr && tempMpPlayerBet->size() > 0)
			//	{
			//		auto iter_mp = tempMpPlayerBet->begin();
			//		if (iter_mp != tempMpPlayerBet->end() && iter_mp->second != nullptr)
			//		{
			//			CDBMysqlMgr::Instance().AsyncRemoveBairenCount(*(iter_mp->second));
			//		}
			//	}
			//}
			m_vecBairenCount.erase(m_vecBairenCount.begin());
		}
		//for (auto iter = m_mpBairenPalyerBet.begin(); iter != m_mpBairenPalyerBet.end() && iter->second != nullptr; iter++)
		//{
		//	CDBMysqlMgr::Instance().AsyncInsertBairenCount(*(iter->second));
		//}
		m_mpBairenPalyerBet.clear();
	}
}

void CGameTable::OnSetPlayerWinScore(uint32 uid, int64 score)
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()))
	{
		auto iter = m_mpBairenPalyerBet.find(uid);
		if (iter != m_mpBairenPalyerBet.end() && iter->second != nullptr)
		{
			iter->second->winscore += score;
		}

		CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
		if (pPlayer != NULL)
		{
			//pPlayer->SetVecWin(GetGameType(), score);
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,score:%lld,wincount:%d,betscore:%lld,betcount:%d",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), score, pPlayer->GetVecWin(GetGameType()), pPlayer->GetVecBet(GetGameType()), pPlayer->GetBetCount(GetGameType()));
		}
	}
}

void CGameTable::InitPlayerBairenCoint(CGamePlayer * pPlayer)
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()))
	{
		pPlayer->ClearVecWin(GetGameType());
		pPlayer->ClearVecBet(GetGameType());
		for (auto iter_vec = m_vecBairenCount.begin(); iter_vec != m_vecBairenCount.end(); iter_vec++)
		{
			auto tempMpPlayerBet = (*iter_vec);
			if (tempMpPlayerBet != nullptr && tempMpPlayerBet->size() > 0)
			{
				auto iter_find = tempMpPlayerBet->find(pPlayer->GetUID());
				if (iter_find != tempMpPlayerBet->end() && iter_find->second != nullptr)
				{
					pPlayer->SetVecWin(GetGameType(), iter_find->second->winscore > 0);
					pPlayer->SetVecBet(GetGameType(), iter_find->second->betscore);
				}
			}
		}
		//LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d", 
		//	GetRoomID(),GetTableID(), pPlayer->GetUID(), pPlayer->GetVecWin(GetGameType()), pPlayer->GetVecBet(GetGameType()), pPlayer->GetBetCount(GetGameType()));

	}
}

bool    CGameTable::CalculateDeity()
{
	if (m_vecPlayers.size() == 0)
	{
		return false;
	}
	if (CCommonLogic::IsBaiRenCount(GetGameType()) == false)
	{
		return false;
	}
	std::vector<std::shared_ptr<struct tagGameSortInfo>> vecGameSortRank;
	if (CCommonLogic::IsBaiRenCount(GetGameType()))
	{
		CGamePlayer * pGamePlayer = NULL;
		int iMaxCount = 0;
		for (uint32 uChairID = 0; uChairID < m_vecPlayers.size(); uChairID++)
		{
			if (m_vecPlayers[uChairID].pPlayer != NULL)
			{
				int tempCount = m_vecPlayers[uChairID].pPlayer->GetVecWin(GetGameType());
				int64 tempScore = m_vecPlayers[uChairID].pPlayer->GetVecBet(GetGameType());
				struct tagGameSortInfo tagSort;
				//tagSort.pPlayer = m_vecPlayers[uChairID].pPlayer;
				tagSort.uid = m_vecPlayers[uChairID].pPlayer->GetUID();
				tagSort.wincount = tempCount;
				tagSort.betscore = tempScore;
				auto sptagSort = std::make_shared<struct tagGameSortInfo>(tagSort);
				vecGameSortRank.emplace_back(sptagSort);
			}
		}
		auto iter_lookers = m_mpLookers.begin();
		for (; iter_lookers != m_mpLookers.end(); ++iter_lookers)
		{
			CGamePlayer* pPlayer = iter_lookers->second;
			if (pPlayer != NULL)
			{
				int tempCount = pPlayer->GetVecWin(GetGameType());
				int64 tempScore = pPlayer->GetVecBet(GetGameType());
				struct tagGameSortInfo tagSort;
				//tagSort.pPlayer = pPlayer;
				tagSort.uid = pPlayer->GetUID();
				tagSort.wincount = tempCount;
				tagSort.betscore = tempScore;
				auto sptagSort = std::make_shared<struct tagGameSortInfo>(tagSort);
				vecGameSortRank.emplace_back(sptagSort);
			}
		}
	}
	DivineMathematicianAndRichSitdown(vecGameSortRank);
	return true;
}

bool CGameTable::CompareRankByWinCount(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2)
{
	return sptagSort1->wincount > sptagSort2->wincount;
}
bool CGameTable::CompareRankByBetScore(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2)
{
	return sptagSort1->betscore > sptagSort2->betscore;
}

bool    CGameTable::ForcePlayerSitDownStandUp(CGamePlayer* pPlayer, bool sitDown, uint16 chairID)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,chairID:%d,pPlayer:%p", GetRoomID(), GetTableID(), chairID, pPlayer);

		return false;
	}
	if (sitDown)//坐下
	{
		m_vecPlayers[chairID].pPlayer = pPlayer;
		m_vecPlayers[chairID].readyState = 0;
		m_vecPlayers[chairID].uid = pPlayer->GetUID();
		m_vecPlayers[chairID].autoState = 0;
		m_vecPlayers[chairID].readyTime = getSysTime();
		m_vecPlayers[chairID].overTimes = 0;

		LOG_DEBUG("sitdown：room:%d--tb:%d,chairID:%d,uid:%d", GetRoomID(), GetTableID(), chairID, pPlayer->GetUID());
		RemoveLooker(pPlayer);
		//OnActionUserSitDown(chairID, pPlayer);
		return true;
	}
	else
	{//站起
		for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		{
			if (m_vecPlayers[i].pPlayer == pPlayer)
			{
				LOG_DEBUG("standup:room:%d--tb:%d,chairID:%d,uid:%d", GetRoomID(), GetTableID(), i, pPlayer->GetUID());
				m_vecPlayers[i].Reset();
				AddLooker(pPlayer);
				//OnActionUserStandUp(i, pPlayer);
				return true;
			}
		}

	}

	return true;
}

void CGameTable::DivineMathematicianAndRichSitdown(std::vector<std::shared_ptr<struct tagGameSortInfo>> & vecGameSortRank)
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()) == false)
	{
		return;
	}
	if (vecGameSortRank.size() == 0)
	{
		return;
	}
	for (uint32 i = 0; i < m_vecPlayers.size(); i++)
	{
		if (m_vecPlayers[i].pPlayer != NULL)
		{
			ForcePlayerSitDownStandUp(m_vecPlayers[i].pPlayer, false, i);
		}
	}
	sort(vecGameSortRank.begin(), vecGameSortRank.end(), CompareRankByWinCount);
	auto iter_vec_divine = vecGameSortRank.begin();

	CGamePlayer * pGamePlayerDivine = NULL;
	if (iter_vec_divine != vecGameSortRank.end())
	{
		auto spvec_tag = (*iter_vec_divine);
		if (spvec_tag != nullptr)
		{
			//pGamePlayerDivine = spvec_tag->pPlayer;
			pGamePlayerDivine = GetGamePlayerByUid(spvec_tag->uid);
			if (pGamePlayerDivine != NULL)
			{
				ForcePlayerSitDownStandUp(pGamePlayerDivine, true, m_vecPlayers.size() - 1); // 大富豪(赢局数最多)
			}
		}
	}
	sort(vecGameSortRank.begin(), vecGameSortRank.end(), CompareRankByBetScore);
	auto iter_vec_rich = vecGameSortRank.begin();
	uint32 indexChairID = 0;
	for (; iter_vec_rich != vecGameSortRank.end() && indexChairID < (m_vecPlayers.size() - 1); iter_vec_rich++)
	{
		auto spvec_tag = (*iter_vec_rich);
		if (spvec_tag != nullptr)
		{
			//CGamePlayer * pGamePlayerRich = spvec_tag->pPlayer;
			CGamePlayer * pGamePlayerRich = GetGamePlayerByUid(spvec_tag->uid);
			if (pGamePlayerRich != NULL && pGamePlayerRich != pGamePlayerDivine)
			{
				ForcePlayerSitDownStandUp(pGamePlayerRich, true, indexChairID);
				indexChairID++;
			}
		}
	}

	LOG_DEBUG("------------------------------------------------------------------------------------------------------------");
	for (uint32 i = 0; i < m_vecPlayers.size(); i++)
	{
		if (m_vecPlayers[i].pPlayer != NULL)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d",
				GetRoomID(),GetTableID(),i, m_vecPlayers[i].pPlayer->GetUID(), m_vecPlayers[i].pPlayer->GetVecWin(GetGameType()),
				m_vecPlayers[i].pPlayer->GetVecBet(GetGameType()), m_vecPlayers[i].pPlayer->GetBetCount(GetGameType()));
		}
	}
	/*
	auto iter_vec_rich_temp = vecGameSortRank.begin();
	int index = 0;
	for (; iter_vec_rich_temp != vecGameSortRank.end(); iter_vec_rich_temp++)
	{
		auto spvec_tag = (*iter_vec_rich_temp);
		if (spvec_tag != nullptr)
		{
			CGamePlayer * pGamePlayerRich = GetGamePlayerByUid(spvec_tag->uid);
			if (pGamePlayerRich != NULL && pGamePlayerRich != pGamePlayerDivine)
			{
				LOG_DEBUG("roomid:%d,tableid:%d,index:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d", GetRoomID(),GetTableID(),index++, pGamePlayerRich->GetUID(), pGamePlayerRich->GetVecWin(GetGameType()), pGamePlayerRich->GetVecBet(GetGameType()), pGamePlayerRich->GetBetCount(GetGameType()));
			}
		}
	}
	*/

	
	for (uint32 i = 0; i < m_vecPlayers.size(); i++)
	{
		if (i == m_vecPlayers.size() - 1)
		{
			if (m_vecPlayers[i].pPlayer != NULL)
			{
				Json::Value logValue;
				logValue["uid"] = m_vecPlayers[i].pPlayer->GetUID();
				logValue["wincount"] = m_vecPlayers[i].pPlayer->GetVecWin(GetGameType());
				logValue["betscore"] = m_vecPlayers[i].pPlayer->GetVecBet(GetGameType());
				logValue["betcount"] = m_vecPlayers[i].pPlayer->GetBetCount(GetGameType());
				m_operLog["divine"].append(logValue);
			}
		}
	}
	auto iter_vec_rich_temp = vecGameSortRank.begin();
	int index = 0;
	for (; iter_vec_rich_temp != vecGameSortRank.end(); iter_vec_rich_temp++)
	{
		auto spvec_tag = (*iter_vec_rich_temp);
		if (spvec_tag != nullptr)
		{
			CGamePlayer * pGamePlayerRich = GetGamePlayerByUid(spvec_tag->uid);
			if (pGamePlayerRich != NULL && pGamePlayerRich != pGamePlayerDivine)
			{
				//LOG_DEBUG("roomid:%d,tableid:%d,index:%d,uid:%d,wincount:%d,betscore:%lld,betcount:%d", GetRoomID(), GetTableID(), index++, pGamePlayerRich->GetUID(), pGamePlayerRich->GetVecWin(GetGameType()), pGamePlayerRich->GetVecBet(GetGameType()), pGamePlayerRich->GetBetCount(GetGameType()));
				Json::Value logValue;
				logValue["uid"] = pGamePlayerRich->GetUID();
				logValue["wincount"] = pGamePlayerRich->GetVecWin(GetGameType());
				logValue["betscore"] = pGamePlayerRich->GetVecBet(GetGameType());
				logValue["betcount"] = pGamePlayerRich->GetBetCount(GetGameType());
				m_operLog["divine"].append(logValue);
				index++;
			}
		}
		if (index==10)
		{
			break;
		}
	}
	SendSeatInfoToClient();
	SendLookerListToClient();
}

void CGameTable::OnNewDay()
{

}


void CGameTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo)
{

}

void CGameTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo)
{

}

uint32  CGameTable::GetRemainTime()
{
	return m_coolLogic.getCoolTick();
}

void CGameTable::SendFirstGamePlayLogInfo(CGamePlayer * pPlayer)
{
	if (pPlayer == NULL)
	{
		return;
	}

	net::msg_game_play_log_rep rep;
	rep.set_gametype(GetGameType());
	rep.set_roomid(GetRoomID());
	rep.set_gamestate(GetGameState());
	rep.set_remain_time(GetRemainTime());
	net::msg_game_play_log* pInfo = rep.mutable_play_log();
	GetGamePlayLogInfo(pInfo);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_FIRST_SEND_GAME_PLAY_LOG_REP);

	uint16 gameType = GetGameType();
	uint32 rep_size_bainiu = rep.play_log().bainiu().logs_size();
	uint32 rep_size_baccarat = rep.play_log().baccarat().logs_size();
	uint32 rep_size_paijiu = rep.play_log().paijiu().logs_size();
	uint32 rep_size_dice = rep.play_log().dice().logs_size();
	uint32 rep_size_war = rep.play_log().war().logs_size();
	uint32 rep_size_fight = rep.play_log().fight().logs_size();
	
	string strRepSize;

	strRepSize += CStringUtility::FormatToString("rep_size_bainiu:%d,", rep_size_bainiu);
	strRepSize += CStringUtility::FormatToString("rep_size_baccarat:%d,", rep_size_baccarat);
	strRepSize += CStringUtility::FormatToString("rep_size_paijiu:%d,", rep_size_paijiu);
	strRepSize += CStringUtility::FormatToString("rep_size_dice:%d,", rep_size_dice);
	strRepSize += CStringUtility::FormatToString("rep_size_war:%d,", rep_size_war);
	strRepSize += CStringUtility::FormatToString("rep_size_fight:%d,", rep_size_fight);

	LOG_DEBUG("send_game_play_client - uid:%d,gameType:%d,roomid:%d,tableid:%d,state:%d,remain:%d,strRepSize:%s",
		pPlayer->GetUID(), gameType,GetRoomID(),GetTableID(), GetGameState(), GetRemainTime(), strRepSize.c_str());

}

void CGameTable::GetAllGameEndLogInfo(net::msg_game_play_log_rep * prep)
{
	prep->set_gametype(GetGameType());
	prep->set_roomid(GetRoomID());
	prep->set_gamestate(GetGameState());
	prep->set_remain_time(GetRemainTime());
	net::msg_game_play_log* pInfo = prep->mutable_play_log();
	GetGameEndLogInfo(pInfo);
}

void CGameTable::SendGameEndLogInfo()
{
	net::msg_game_play_log_rep rep;
	GetAllGameEndLogInfo(&rep);
	stl_hash_map<uint32, CPlayerBase*> & mpPlayers = CPlayerMgr::Instance().GetAllPlayers();

	for (auto iter = mpPlayers.begin(); iter != mpPlayers.end(); ++iter)
	{
		CGamePlayer* pPlayer = (CGamePlayer*)iter->second;
		if (pPlayer != NULL)
		{
			if (pPlayer->GetRoom() == NULL)
			{
				pPlayer->SendMsgToClient(&rep, net::S2C_MSG_NEXTS_SEND_GAME_PLAY_LOG_REP);
				LOG_DEBUG("send_game_end_client - uid:%d,roomid:%d,tableid:%d,state:%d,remain:%d",
					pPlayer->GetUID(), GetRoomID(), GetTableID(), GetGameState(), GetRemainTime());

			}
		}
	}
}

void CGameTable::CaclPlayerBaiRenCount()
{
	if (CCommonLogic::IsBaiRenCount(GetGameType()) == false)
	{
		return;
	}
	for (uint32 nChairID = 0; nChairID < m_vecPlayers.size(); nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		InitPlayerBairenCoint(pPlayer);
	}
	auto iter = m_mpLookers.begin();
	for (; iter != m_mpLookers.end(); iter++)
	{
		CGamePlayer * pPlayer = iter->second;
		if (pPlayer != NULL)
		{
			InitPlayerBairenCoint(pPlayer);
		}
	}
}

bool CGameTable::HaveNotNovicePlayer()
{
	if (CCommonLogic::IsNewPlayerNoviceWelfare(GetGameType()) == false)
	{
		return true;
	}
	for (uint32 nChairID = 0; nChairID < m_vecPlayers.size(); nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (pPlayer->IsRobot())
		{
			continue;
		}
		if (pPlayer->IsNoviceWelfare() == false)
		{
			return true;
		}
	}
	auto iter = m_mpLookers.begin();
	for (; iter != m_mpLookers.end(); iter++)
	{
		CGamePlayer * pPlayer = iter->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		if (pPlayer->IsRobot())
		{
			continue;
		}
		if (pPlayer->IsNoviceWelfare() == false)
		{
			return true;
		}
	}
	return false;
}


//bool CGameTable::GetHaveKilledPlayer()
//{
//	if (CCommonLogic::IsBaiRenAutoKillScore(GetGameType()) == false)
//	{
//		return false;
//	}
//	for (uint32 nChairID = 0; nChairID < m_vecPlayers.size(); nChairID++)
//	{
//		CGamePlayer * pPlayer = GetPlayer(nChairID);
//		if (pPlayer == NULL)
//		{
//			continue;
//		}
//		if (pPlayer->IsRobot())
//		{
//			continue;
//		}
//		bool bIsAutoKilling = CDataCfgMgr::Instance().IsAutoKillingUsers(pPlayer->GetUID());
//		if (bIsAutoKilling)
//		{
//			return true;
//		}
//	}
//	auto iter = m_mpLookers.begin();
//	for (; iter != m_mpLookers.end(); iter++)
//	{
//		CGamePlayer * pPlayer = iter->second;
//		if (pPlayer == NULL)
//		{
//			continue;
//		}
//		if (pPlayer->IsRobot())
//		{
//			continue;
//		}
//		bool bIsAutoKilling = CDataCfgMgr::Instance().IsAutoKillingUsers(pPlayer->GetUID());
//		if (bIsAutoKilling)
//		{
//			return true;
//		}
//	}
//	return false;
//}

bool CGameTable::EnterNoviceWelfare(CGamePlayer * pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (pPlayer->IsRobot())
	{
		return false;
	}
	bool bIsNoviceWelfare = pPlayer->IsNoviceWelfare();

	bool bIsNoCanEnter = false;
	if (bIsNoviceWelfare == true)
	{
		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
		{
			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
			if (pTPlayer != NULL && pTPlayer->IsRobot() == false)
			{
				bIsNoCanEnter = true;
				break;
			}
		}

		if (GetGameType() == GAME_CATE_TEXAS && m_mpLookers.size() > 0)
		{
			for (auto iter = m_mpLookers.begin(); iter != m_mpLookers.end(); iter++)
			{
				CGamePlayer * pTPlayer = iter->second;
				if (pTPlayer != NULL && pTPlayer->IsRobot() == false)
				{
					bIsNoCanEnter = true;
					break;
				}
			}
		}
	}
	else
	{
		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
		{
			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
			if (pTPlayer != NULL && pTPlayer->IsRobot() == false && pTPlayer->IsNoviceWelfare() == true)
			{
				bIsNoCanEnter = true;
				break;
			}
		}

		if (GetGameType() == GAME_CATE_TEXAS && m_mpLookers.size() > 0)
		{
			for (auto iter = m_mpLookers.begin(); iter != m_mpLookers.end(); iter++)
			{
				CGamePlayer * pTPlayer = iter->second;
				if (pTPlayer != NULL && pTPlayer->IsRobot() == false && pTPlayer->IsNoviceWelfare() == true)
				{
					bIsNoCanEnter = true;
					break;
				}
			}
		}
	}

	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNoviceWelfare:%d,bIsNoCanEnter:%d",
		pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNoviceWelfare, bIsNoCanEnter);

	return bIsNoCanEnter;
}

string CGameTable::GetChessID()
{
	return m_chessid;
}


void CGameTable::SetChessWelfare(int value)
{
	m_ctrl_welfare = value;
	m_blingLog.welctrl = m_ctrl_welfare;
}

int CGameTable::GetChessWelfare()
{
	return m_ctrl_welfare;
}

void CGameTable::UpdateActiveWelfareInfo(uint32 uid, int64 curr_win)
{
    LOG_DEBUG("enter UpdateActiveWelfareInfo roomid:%d,tableid:%d uid:%d curr_win:%lld", m_pHostRoom->GetRoomID(), GetTableID(), uid, curr_win);
        
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if (pGamePlayer == NULL)
    {
        LOG_DEBUG("UpdateActiveWelfareInfo - get user info fail. uid:%d", uid);
        return;
    }

    uint8 aw_id = pGamePlayer->GetCurrAWID();
    
    if (curr_win > 0)
    {
        pGamePlayer->SetCurrAWInfo(aw_id, curr_win);

        //记录活跃福利日志表
        CAcTiveWelfareMgr::Instance().WriteActiveWelfareLog(uid, aw_id, m_pHostRoom->GetGameType(), curr_win);
    }

    m_aw_ctrl_uid = 0;

    return;
}

void CGameTable::GetActiveWelfareCtrlPlayerList()
{
    m_aw_ctrl_player_list.clear();

    uint16 aw_rate = m_pHostRoom->GetAWRate();

    LOG_DEBUG("enter get active welfare ctrl player list - roomid:%d,tableid:%d aw_rate:%d", m_pHostRoom->GetRoomID(), GetTableID(), aw_rate);

    //选择当前桌子上的活跃福利玩家列表
    for (uint32 uid : m_curr_bet_user)
    {
        CGamePlayer* player = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
        if (player == NULL)
        {
            LOG_ERROR("get user info fail. uid:%", uid);
            continue;
        }

        if (!player->IsRobot() && player->IsEnterActiveWelfareRoomFlag(m_pHostRoom->GetGameType()))
        {
			uint32 probability = 0;
			uint32 max_win = 0;
            bool ret = CAcTiveWelfareMgr::Instance().GetActiveWelfareCfgInfo(player->GetUID(), player->GetCurrAWID(), m_pHostRoom->GetGameType(), max_win, probability);
            if (ret)
            {
                tagAWPlayerInfo info;
                info.uid = player->GetUID();
                info.sum_loss = player->GetCurrLoss();
                info.max_win = max_win;
				info.probability = probability;
                m_aw_ctrl_player_list.push_back(info);
                LOG_DEBUG("add aw player info success - uid:%d sum_loss:%d max_win:%d probability:%d", info.uid, info.sum_loss, info.max_win, info.probability);
            }
            else
            {
                LOG_DEBUG("add aw player info fail. player info - uid:%d curr_aw_id:%d game_type:%d", player->GetUID(), player->GetCurrAWID(), m_pHostRoom->GetGameType());
            }
        }
    }

    //判断亏损玩家人数/总下注人数是否达到触发活跃福利功能的百分比
    uint16 bet_count = m_curr_bet_user.size();
    if (bet_count <= 0)
    {
        LOG_DEBUG("the bet player count is zero. reutrn");
		m_aw_ctrl_player_list.clear();
        return ;
    }

    uint16 aw_count = m_aw_ctrl_player_list.size();
    uint16 rate = aw_count * 100 / bet_count;
    if (rate < aw_rate)
    {
        LOG_DEBUG("the aw player count not reach rate - aw_rate:%d rate:%d bet_count:%d aw_count:%d", aw_rate, rate, bet_count, aw_count);
		m_aw_ctrl_player_list.clear();
        return ;
    }

    //将活跃福利玩家列表按照最大亏损进行排序
    sort(m_aw_ctrl_player_list.begin(), m_aw_ctrl_player_list.end(), less_second);  
}

//bool CGameTable::EnterAutoKillScore(CGamePlayer * pPlayer)
//{
//	if (pPlayer == NULL)
//	{
//		return false;
//	}
//	if (pPlayer->IsRobot())
//	{
//		return false;
//	}
//	bool bIsKilledScore = pPlayer->IsKilledScore(GetGameType());
//	bool bIsNoEnterFirst = false;
//	if (bIsKilledScore)
//	{
//		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
//		{
//			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
//			if (pTPlayer != NULL && pTPlayer->IsRobot() == false)
//			{
//				bIsNoEnterFirst = true;
//				break;
//			}
//		}
//	}
//	else
//	{
//		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
//		{
//			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
//			if (pTPlayer != NULL && pTPlayer->IsRobot() == false && pTPlayer->IsKilledScore(GetGameType()) == true)
//			{
//				bIsNoEnterFirst = true;
//				break;
//			}
//		}
//	}
//	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsKilledScore:%d,bIsNoEnterFirst:%d",
//		pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsKilledScore, bIsNoEnterFirst);
//
//	return bIsNoEnterFirst;
//}


//bool CGameTable::IsHaveAutoKillScorePlayer()
//{
//	for (uint32 wChairID = 0; wChairID<m_vecPlayers.size(); wChairID++)
//	{
//		CGamePlayer * pPlayer = GetPlayer(wChairID);
//		if (pPlayer == NULL)
//		{
//			continue;
//		}
//		if (pPlayer->IsRobot())
//		{
//			continue;
//		}
//		if (pPlayer->IsKilledScore(GetGameType()) == true)
//		{
//			stAutoKillConfrontationCfg cfg = pPlayer->GetKilledScoreCfg();
//			if (cfg.defeat >= pPlayer->GetDefeat())
//			{
//				pPlayer->AddInterval();
//				return false;
//			}
//			if (cfg.interval >= pPlayer->GetInterval())
//			{
//				pPlayer->AddDefeat();
//				return true;
//			}
//		}
//	}
//	return false;
//}

bool CGameTable::EnterNewRegisterWelfare(CGamePlayer * pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (pPlayer->IsRobot())
	{
		return false;
	}
	bool bIsNewRegisterWelfare = pPlayer->IsNewRegisterPlayerWelfareFlag(m_pHostRoom->GetGameType());

	bool bIsNoEnter = false;
	if (bIsNewRegisterWelfare == true)
	{
		for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		{
			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
			if (pTPlayer != NULL && pTPlayer->IsRobot() == false)
			{
				bIsNoEnter = true;
				break;
			}
		}
	}
	else
	{
		for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		{
			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
			if (pTPlayer != NULL && pTPlayer->IsRobot() == false && pTPlayer->IsNewRegisterPlayerWelfareFlag(m_pHostRoom->GetGameType()) == true)
			{
				bIsNoEnter = true;
				break;
			}
		}
	}	
	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNewRegisterWelfare:%d,bIsNoEnterFirst:%d",
		pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNewRegisterWelfare, bIsNoEnter);
	return bIsNoEnter;
}

void CGameTable::UpdateNewRegisterWelfareInfo(uint32 uid, int64 curr_win)
{
	LOG_DEBUG("enter function roomid:%d,tableid:%d uid:%d curr_win:%lld", m_pHostRoom->GetRoomID(), GetTableID(), uid, curr_win);

	CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if (pGamePlayer == NULL)
	{
		LOG_DEBUG("get user info fail. uid:%d", uid);
		return;
	}

	//更新福利累计值
	pGamePlayer->UpdateNewRegisterPlayerInfo(curr_win);

	if (curr_win > 0)
	{
		stNewRegisterWelfarePlayerInfo *pInfo = pGamePlayer->GetPlayerNRWInfo();
		if (pInfo)
		{
			//记录新注册玩家福利日志表
			CNewRegisterWelfareMgr::Instance().WriteNewRegisterWelfareLog(uid, m_pHostRoom->GetGameType(), pInfo->curr_must_win_number, pInfo->curr_total_win_number, curr_win, pInfo->curr_total_win_coin, pGamePlayer->GetRtime());
		}
		else
		{
			LOG_ERROR("get user new register info fail. uid:%d", uid);
		}
	}
	else
	{
		LOG_DEBUG("the nrw ctrl player is lose uid:%d", m_nrw_ctrl_uid);
	}
		
	m_nrw_ctrl_uid = 0;
	m_nrw_status = -1;
	return;
}

bool CGameTable::IsNewRegisterWelfareTable()
{
	// 先判断当前房间是否享受新账户福利功能
	if (!m_pHostRoom->GetNrwRoom())
	{
		LOG_DEBUG("the current room is not new register welfare room. room_id:%d tid:%d", GetRoomID(), GetTableID());
		return false;
	}

	for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
	{
		CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
		if (pTPlayer != NULL && pTPlayer->IsNewRegisterPlayerWelfareFlag(m_pHostRoom->GetGameType()))
		{
			return true;
		}
	}	
	return false;
}

// 结算玩家信息---百人场
void    CGameTable::CalcPlayerGameInfoForBrc(uint32 uid, int64& winScore, int64 lExWinScore, bool bDeductFee, bool bBranker, int64 lBetScore)
{
	LOG_DEBUG("calc player game info uid:%d winScore:%lld lExWinScore:%lld bDeductFee:%d bBranker:%d lBetScore:%lld"
		,uid,winScore,lExWinScore,bDeductFee,bBranker,lBetScore);
	//ReportGameResult(uid,winScore);
	if (winScore == 0)
		return;

	bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	int64 lPlayerWinScore = winScore + lExWinScore;
	if (pPlayer != NULL)
	{		
		lPlayerWinScore = winScore + lExWinScore;
		if (lPlayerWinScore != 0)
		{
			ChangeScoreValueByUID(uid, lPlayerWinScore, emACCTRAN_OPER_TYPE_GAME, m_pHostRoom->GetGameType());
		}
		pPlayer->AsyncChangeGameValue(m_pHostRoom->GetGameType(), isCoin, lPlayerWinScore, lExWinScore, GetChessWelfare());
		FlushUserBlingLog(pPlayer, lPlayerWinScore, lExWinScore);
		OnSetPlayerWinScore(uid, lPlayerWinScore);
		if (lExWinScore > 0)
		{
			pPlayer->ReSetGameCount(GetGameType());
		}
		pPlayer->OnGameEnd();

		if (GetPlayerCurScore(pPlayer) >= GetBankruptScore())
		{
			pPlayer->SetBankruptRecord(true);
		}

		if (m_tagControlPalyer.id != -1 && m_tagControlPalyer.uid == pPlayer->GetUID())
		{
			m_tagControlPalyer.escore = GetPlayerCurScore(pPlayer);
		}
		auto it_palyer = m_mpControlMultiPalyer.find(pPlayer->GetUID());
		if (it_palyer != m_mpControlMultiPalyer.end())
		{
			tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			ControlMultiPalyer.escore = GetPlayerCurScore(pPlayer);
		}
	}
	if (uid != 0)
	{
		ReportGameResult(uid, lPlayerWinScore, lExWinScore, bBranker, lBetScore);
	}
	return;
}

// 获取抽水费用---百人场
int64    CGameTable::GetBrcFee(uint32 uid, int64 winScore, bool bDeductFee)
{	
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	int64 fee = 0;	
	if (pPlayer != NULL)
	{
		if (bDeductFee)
		{
			if (winScore != 0)
			{
				fee = DeducEndFee(uid, winScore);
			}
		}		
	}	
	LOG_DEBUG("get brc fee. uid:%d winScore:%lld fee:%lld", uid, winScore, fee);
	return fee;
}

//百人场精准控制---进入界面
bool CGameTable::OnBrcControlPlayerEnterInterface(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return false;
	}

	LOG_DEBUG("brc control player enter interface. control uid:%d", pPlayer->GetUID());

	uint8 result = RESULT_CODE_SUCCESS;

	//判断是否有控制权限
	if (!pPlayer->GetCtrlFlag())
	{
		LOG_DEBUG("brc control the curr uid:%d is not allow operator info.", pPlayer->GetUID());
		result = RESULT_CODE_FAIL;
	}
	else
	{
		m_setControlPlayers.insert(pPlayer);
	}

	//返回结果消息
	net::msg_brc_control_user_enter_table_rep rep;
	rep.set_result(result);
	rep.set_times(m_control_number);
	for (int i = 0; i < BRC_MAX_CONTROL_AREA; i++)
	{
		rep.add_area(m_req_control_area[i]);
		LOG_DEBUG("brc control area uid:%d i:%d value:%d.", pPlayer->GetUID(), i, m_req_control_area[i]);
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ENTER_TABLE_REP);

	if (result == RESULT_CODE_SUCCESS)
		return true;
	else
		return false;
}

//百人场精准控制---离开界面
bool CGameTable::OnBrcControlPlayerLeaveInterface(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return false;
	}

	LOG_DEBUG("brc control player leave interface. control uid:%d", pPlayer->GetUID());

	uint8 result = RESULT_CODE_SUCCESS;

	m_setControlPlayers.erase(pPlayer);

	//返回结果消息
	net::msg_brc_control_user_leave_table_rep rep;
	rep.set_result(result);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_LEAVE_TABLE_REP);
	return true;
}

//百人场精准控制---控制区域请求
bool CGameTable::OnBrcControlPlayerBetArea(CGamePlayer *pPlayer, net::msg_brc_control_area_info_req & msg)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return false;
	}

	LOG_DEBUG("brc control area info. control uid:%d times:%d", pPlayer->GetUID(), msg.times());

	uint8 result = RESULT_CODE_SUCCESS;

	//判断是否有控制权限
	if (!pPlayer->GetCtrlFlag())
	{
		LOG_DEBUG("brc control the curr uid:%d is not allow operator info.", pPlayer->GetUID());
		result = RESULT_CODE_FAIL;
	}

	//判断当前是否处于下注阶段
	if (GetGameState() != TABLE_STATE_NIUNIU_PLACE_JETTON)
	{
		LOG_DEBUG("brc control the table is not bet status. state:%d", GetGameState());
		result = RESULT_CODE_FAIL;
	}

	if (result == RESULT_CODE_SUCCESS)
	{
		m_control_number = msg.times();
		m_real_control_uid = pPlayer->GetUID();
		memset(m_req_control_area, 0x0, sizeof(m_req_control_area));
		for (int i = 0; i < msg.area_size(); i++)
		{
			m_req_control_area[i] = msg.area(i);
			LOG_DEBUG("brc control area uid:%d i:%d value:%d.", pPlayer->GetUID(), i, msg.area(i));
		}
	}

	//返回结果消息
	net::msg_brc_control_area_info_rep rep;
	rep.set_result(result);
	rep.set_times(msg.times());
	for (int i = 0; i < msg.area_size(); i++)
	{
		rep.add_area(msg.area(i));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_AREA_INFO_REP);
	return true;
}

//百人场精准控制---牌局结束时，发送实际控制结果到客户端
void CGameTable::OnBrcControlGameEnd()
{
	//精准控制百人场处理
	//if (m_real_control_uid != 0)
	{
		//发送当前局的控制结果
		LOG_DEBUG("send brc control game result. curr_control_uid:%d control player number:%d", m_real_control_uid, m_setControlPlayers.size());
		net::msg_brc_control_game_end_info rep;
		rep.set_uid(m_real_control_uid);
		for (int i = 0; i < BRC_MAX_CONTROL_AREA; i++)
		{
			rep.add_area_info(m_req_control_area[i]);
			LOG_DEBUG("brc control area uid:%d i:%d value:%d.", m_real_control_uid, i, m_req_control_area[i]);
		}

		for (auto &it : m_setControlPlayers)
		{
			it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_GAME_END_INFO);
		}

		m_control_number--;

		if (m_control_number <= 0)
		{
			memset(m_req_control_area, 0x0, sizeof(m_req_control_area));
			m_real_control_uid = 0;
			m_control_number = 0;
		}
		memset(m_real_control_area, 0x0, sizeof(m_real_control_area));
	}

	LOG_DEBUG("brc control game result. real_control_uid:%d control_number:%d ControlPlayers number:%d", m_real_control_uid, m_control_number, m_setControlPlayers.size());
	return;
}

//百人场精准控制---玩家进入游戏
void CGameTable::OnBrcControlPlayerEnterTable(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return;
	}

	//判断当前玩家的百人场统计信息是否存在
	auto iter = m_mpPlayerResultInfo.find(pPlayer->GetUID());
	if (iter != m_mpPlayerResultInfo.end())
	{
		iter->second.win = 0;
		iter->second.lose = 0;

		//判断时间是否跨天,如果跨天,则需要清除累计的当日赢取值
		uint64 uTime = getTime();
		bool bNewDay = (diffTimeDay(iter->second.last_update_time, uTime) != 0);
		if (bNewDay)
		{
			iter->second.day_win_coin = 0;
			iter->second.last_update_time = uTime;
		}

		//同步redis
		CRedisMgr::Instance().SetBrcPlayerResultInfo(pPlayer->GetUID(), m_pHostRoom->GetGameType(), iter->second);

		LOG_DEBUG("get brc info. uid:%d win:%d lose:%d day_win_coin:%lld last_update_time:%llu total_win_coin:%lld",
			pPlayer->GetUID(), iter->second.win, iter->second.lose, iter->second.day_win_coin, iter->second.last_update_time, iter->second.total_win_coin);
	}
	else
	{
		tagPlayerResultInfo info;

		//获取redis数据
		CRedisMgr::Instance().GetBrcPlayerResultInfo(pPlayer->GetUID(), m_pHostRoom->GetGameType(), info);

		//判断时间是否跨天,如果跨天,则需要清除累计的当日赢取值
		uint64 uTime = getTime();
		bool bNewDay = (diffTimeDay(info.last_update_time, uTime) != 0);
		if (bNewDay)
		{
			info.day_win_coin = 0;
			info.last_update_time = uTime;
			//同步redis
			CRedisMgr::Instance().SetBrcPlayerResultInfo(pPlayer->GetUID(), m_pHostRoom->GetGameType(), info);
		}
		m_mpPlayerResultInfo[pPlayer->GetUID()] = info;
		LOG_DEBUG("get brc info. uid:%d win:%d lose:%d day_win_coin:%lld last_update_time:%llu total_win_coin:%lld",
			pPlayer->GetUID(), info.win, info.lose, info.day_win_coin, info.last_update_time, info.total_win_coin);
	}
	return;
}

//百人场精准控制---玩家离开游戏
void CGameTable::OnBrcControlPlayerLeaveTable(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return;
	}

	m_setControlPlayers.erase(pPlayer);

	//判断是否为当前控制玩家，如果是，则控制的信息失效
	if (m_real_control_uid == pPlayer->GetUID())
	{
		m_real_control_uid = 0;
		m_control_number = 0;
		memset(m_req_control_area, 0x0, sizeof(m_req_control_area));
		LOG_DEBUG("real control player is leave table. uid:%d", pPlayer->GetUID());
	}

	//清空当前玩家的输赢次数
	auto iter = m_mpPlayerResultInfo.find(pPlayer->GetUID());
	if (iter != m_mpPlayerResultInfo.end())
	{
		iter->second.win = 0;
		iter->second.lose = 0;

		//同步redis
		CRedisMgr::Instance().SetBrcPlayerResultInfo(pPlayer->GetUID(), m_pHostRoom->GetGameType(), iter->second);
	}
	return;
}

//百人场精准控制---百人场游戏结束后，修改当前玩家的统计信息---每个子类调用
void CGameTable::OnBrcControlSetResultInfo(uint32 uid, int64 win_score)
{
	LOG_DEBUG("brc set result info. uid:%d win_score:%lld ", uid, win_score);
	if (win_score == 0)
		return;

	//清空当前玩家的输赢次数
	auto iter = m_mpPlayerResultInfo.find(uid);
	if (iter != m_mpPlayerResultInfo.end())
	{
		//判断时间是否跨天,如果跨天,则需要清除累计的当日赢取值
		uint64 uTime = getTime();
		bool bNewDay = (diffTimeDay(iter->second.last_update_time, uTime) != 0);
		if (bNewDay)
		{
			iter->second.win = 0;
			iter->second.lose = 0;
		}

		if (win_score > 0)
			iter->second.win++;
		else
			iter->second.lose++;

		iter->second.total_win_coin += win_score;
		iter->second.day_win_coin += win_score;
		iter->second.last_update_time = uTime;

		//同步redis
		CRedisMgr::Instance().SetBrcPlayerResultInfo(uid, m_pHostRoom->GetGameType(), iter->second);
	}
	return;
}

//百人场精准控制---强制玩家下庄
bool CGameTable::OnBrcControlApplePlayer(CGamePlayer *pPlayer, uint32 target_uid)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return false;
	}

	LOG_DEBUG("brc control force apply player. control uid:%d target:%d", pPlayer->GetUID(), target_uid);

	uint8 result = RESULT_CODE_SUCCESS;

	//判断是否有控制权限
	if (!pPlayer->GetCtrlFlag())
	{
		LOG_DEBUG("brc control the curr uid:%d is not allow operator info.", pPlayer->GetUID());
		result = RESULT_CODE_FAIL;
	}

	//判断是否为庄家 
	if (target_uid == GetBankerUID() && GetGameState() != net::TABLE_STATE_NIUNIU_FREE)
	{
		//发送消息
		LOG_DEBUG("the game is run,you can't cance banker");
		m_needLeaveBanker = true;
	}

	bool isflushlist = false;	//是否刷新上庄玩家列表

	//存在判断
	for (WORD i = 0; i < m_ApplyUserArray.size(); ++i)
	{
		//获取玩家
		CGamePlayer *pTmp = m_ApplyUserArray[i];

		//条件过滤
		if (pTmp == NULL) continue;
		if (pTmp->GetUID() != target_uid) continue;

		//删除玩家
		RemoveApplyBanker(target_uid);
		if (m_pCurBanker != pTmp)
		{
			isflushlist = true;
			if (!pTmp->IsRobot())
			{
				OnNotityForceApplyUser(pTmp);
			}
		}
		else if (m_pCurBanker == pPlayer)
		{
			//切换庄家 
			ChangeBanker(true);
		}
		SendApplyUser();
	}

	//返回结果消息
	net::msg_brc_control_force_leave_banker_rep rep;
	rep.set_uid(target_uid);
	rep.set_result(result);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_FORCE_LEAVE_BANKER_REP);

	//判断是否刷新上庄玩家列表
	if (isflushlist)
	{
		OnBrcControlFlushAppleList();
	}
	return true;
}

uint32 CGameTable::GetBankerUID()
{
	if (m_pCurBanker)
		return m_pCurBanker->GetUID();
	return 0;
}

void CGameTable::RemoveApplyBanker(uint32 uid)
{
	LOG_DEBUG("remove apply banker:%d", uid);
	for (uint32 i = 0; i < m_ApplyUserArray.size(); ++i)
	{
		//条件过滤
		CGamePlayer* pPlayer = m_ApplyUserArray[i];
		if (pPlayer->GetUID() == uid) {
			//删除玩家
			m_ApplyUserArray.erase(m_ApplyUserArray.begin() + i);
			m_mpApplyUserInfo.erase(uid);
			UnLockApplyScore(pPlayer);
			LOG_DEBUG("remove_apply_banker_success uid:%d", uid);

			//刷新控制界面的上庄列表
			OnBrcControlFlushAppleList();

			break;
		}
	}
}

bool CGameTable::LockApplyScore(CGamePlayer* pPlayer, int64 score)
{
	if (pPlayer == NULL) {
		return false;
	}

	if (GetPlayerCurScore(pPlayer) < score)
		return false;

	ChangePlayerCurScore(pPlayer, -score);
	m_ApplyUserScore[pPlayer->GetUID()] = score;

	return true;
}

bool CGameTable::UnLockApplyScore(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) {
		return false;
	}

	int64 lockScore = m_ApplyUserScore[pPlayer->GetUID()];
	ChangePlayerCurScore(pPlayer, lockScore);

	return true;
}

void CGameTable::OnBrcControlFlushAppleList()
{
	//发送当前局的控制结果
	LOG_DEBUG("send brc control apple player list size:%d.", m_ApplyUserArray.size());
	net::msg_brc_control_apple_banker_list rep;

	//增加当庄玩家
	net::brc_control_player_coin_info *bank_info = rep.mutable_bank_info();
	if (m_pCurBanker != NULL)
	{
		bank_info->set_uid(m_pCurBanker->GetUID());
		bank_info->set_coin(m_pCurBanker->GetAccountValue(emACC_VALUE_COIN));
		if (m_pCurBanker->IsRobot())
			bank_info->set_player_type(1);
		else
			bank_info->set_player_type(0);
		//LOG_DEBUG("banker uid:%d coin:%lld.", m_pCurBanker->GetUID(), m_pCurBanker->GetAccountValue(emACC_VALUE_COIN));
	}

	//增加申请玩家
	for (uint32 i = 0; i < m_ApplyUserArray.size(); ++i)
	{
		//条件过滤
		CGamePlayer* pPlayer = m_ApplyUserArray[i];
		if (pPlayer)
		{
			//过滤掉庄家
			if (m_pCurBanker && m_pCurBanker->GetUID() == pPlayer->GetUID())
			{
				//LOG_DEBUG("the uid is banker then skip.");
				continue;
			}
			net::brc_control_player_coin_info *user_info = rep.add_user_info();
			user_info->set_uid(pPlayer->GetUID());
			//info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			user_info->set_coin(m_ApplyUserScore[pPlayer->GetUID()]);
			if (pPlayer->IsRobot())
				user_info->set_player_type(1);
			else
				user_info->set_player_type(0);

			//LOG_DEBUG("apple uid:%d coin:%lld user_info size:%d.", pPlayer->GetUID(), m_ApplyUserScore[pPlayer->GetUID()], rep.user_info_size());
		}
	}

	for (auto &it : m_setControlPlayers)
	{
		//LOG_DEBUG("send user_info size:%d.",rep.user_info_size());
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_APPLE_BANKER_INFO);
	}
}

bool CGameTable::GetIsMasterUser(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	bool bIsMaster = pPlayer->GetCtrlFlag();

	return bIsMaster;
}

bool CGameTable::GetIsMasterFollowUser(CGamePlayer* pPlayer, CGamePlayer* pTagPlayer)
{
	bool bIsMaster = false;
	map<uint32, tagUserControlCfg> mpCfgInfo;
	CDataCfgMgr::Instance().GetUserControlCfg(mpCfgInfo);
	tagUserControlCfg tagCtrlCfg;
	uint32 uid = 0;
	if (pPlayer != NULL && pTagPlayer != NULL)
	{
		uid = pPlayer->GetUID();
		auto find_iter = mpCfgInfo.find(uid);
		if (find_iter != mpCfgInfo.end())
		{
			tagCtrlCfg = find_iter->second;
			auto find_set = tagCtrlCfg.cgid.find(GetGameType());
			if (find_set != tagCtrlCfg.cgid.end() && pTagPlayer->GetUID() == tagCtrlCfg.tuid)
			{
				bIsMaster = true;
			}
		}
	}

	return bIsMaster;
}

bool CGameTable::CanEnterCtrledUserTable(CGamePlayer* pGamePlayer)
{
	bool bIsCanEnterTable = false;
	bool bIsHaveCtrledUser = false;
	bool bIsCtrlConflict = false;
	uint32 uFreeCount = 0;
	uint32 suid;
	bool bIsMaster = GetIsMasterUser(pGamePlayer);
	if (bIsMaster) // 如果是超级用户不受这个条件限制进入桌子
	{
		bIsCanEnterTable = true;
	}
	else
	{
		// 统计空闲椅子数量
		for (uint32 i = 0; i < m_vecPlayers.size(); i++)
		{
			CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
			if (pPlayer == NULL)
			{
				uFreeCount++;
			}
		}

		// 是否已经有了被控制用户
		for (uint32 i = 0; i < m_vecPlayers.size(); i++)
		{
			CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (bIsHaveCtrledUser == false)
			{
				if (CDataCfgMgr::Instance().GetIsUserControl(pPlayer->GetUID(), GetGameType(), suid))
				{
					bIsHaveCtrledUser = true;
					break;
				}
			}
		}
	}

	if (bIsHaveCtrledUser)
	{
		if (CDataCfgMgr::Instance().GetIsUserControl(pGamePlayer->GetUID(), GetGameType(), suid))
		{
			bIsCtrlConflict = true;
		}
	}

	// 里面有被控制的人 并且空闲大于2 并且没有冲突控制 可以进入桌子
	if (bIsHaveCtrledUser == true && uFreeCount >= 2 && bIsCtrlConflict == false)
	{
		bIsCanEnterTable = true;
	}
	// 里面没有被控制的人 并且空闲大于2 并且没有冲突控制 可以进入桌子
	if (bIsHaveCtrledUser == false && uFreeCount >= 2 && bIsCtrlConflict == false)
	{
		bIsCanEnterTable = true;
	}
	// 控制冲突不能够再进去
	if (bIsCtrlConflict)
	{
		bIsCanEnterTable = false;
	}

	//LOG_DEBUG("sta - roomid:%d,tableid:%d,uid:%d,bIsMaster:%d,bIsHaveCtrledUser:%d,bIsCtrlConflict:%d,uFreeCount:%d,bIsCanEnterTable:%d",
	//	GetRoomID(), GetTableID(), pGamePlayer->GetUID(), bIsMaster,bIsHaveCtrledUser, bIsCtrlConflict, uFreeCount, bIsCanEnterTable);

	return bIsCanEnterTable;
}

bool CGameTable::IsMasterGame()
{
	switch (GetGameType())
	{
	case net::GAME_CATE_NIUNIU:
	{
		return true;
	}break;
	default:
	{
		return false;
	}break;
	}
	return false;
}

bool CGameTable::LeaveRobotUser()
{
	if (IsMasterGame() == false)
	{
		return false;
	}
	// 如果被跟踪玩家在桌子上，超权用户不在桌子上则需要离开一个机器人
	int iNeedLeaveRobot = 0;
	uint32 suid;
	for (uint32 i = 0; i < m_vecPlayers.size(); i++)
	{
		CGamePlayer* pTagPlayer = m_vecPlayers[i].pPlayer;
		if (pTagPlayer == NULL)
		{
			continue;
		}
		if (CDataCfgMgr::Instance().GetIsUserControl(pTagPlayer->GetUID(), GetGameType(), suid))
		{
			// 是否存在相对应的超级用户
			bool bIsMasterInTable = false;
			for (uint32 j = 0; j < m_vecPlayers.size(); j++)
			{
				CGamePlayer* pPlayer = m_vecPlayers[j].pPlayer;
				if (pPlayer == NULL)
				{
					continue;
				}
				if (i == j)
				{
					continue;
				}
				if (GetIsMasterFollowUser(pPlayer, pTagPlayer))
				{
					bIsMasterInTable = true;
					break;
				}
			}
			if (bIsMasterInTable == false)
			{
				iNeedLeaveRobot++;
			}
		}
	}
	if (iNeedLeaveRobot > 0)
	{
		for (uint32 i = 0; i < m_vecPlayers.size(); i++)
		{
			CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (pPlayer->IsRobot())
			{
				CGameTable * pTable = pPlayer->GetTable();
				if (pTable != NULL)
				{
					if (pTable->CanLeaveTable(pPlayer) && pTable->LeaveTable(pPlayer))
					{
						iNeedLeaveRobot--;
						if (iNeedLeaveRobot <= 0)
						{
							break;
						}
					}
				}

			}
		}
	}
	return true;
}

bool CGameTable::LeaveMasterUser(CGamePlayer *pTagPlayer)
{
	if (pTagPlayer == NULL)
	{
		return false;
	}
	if (IsMasterGame() == false)
	{
		return false;
	}
	if (pTagPlayer->IsRobot())
	{
		return false;
	}
	uint32 result_leave = 0;
	uint32 suid = 0;
	bool bIsUserControl = CDataCfgMgr::Instance().GetIsUserControl(pTagPlayer->GetUID(), GetGameType(), suid);
	if (bIsUserControl)
	{
		// 踢出超级玩家
		for (uint32 i = 0; i < m_vecPlayers.size(); i++)
		{
			CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (pPlayer->GetUID() == pTagPlayer->GetUID())
			{
				continue;
			}
			if (pPlayer->GetUID() == suid)
			{
				result_leave = 1;
				CGameTable * pMasterTable = pPlayer->GetTable();
				if (pMasterTable != NULL)
				{
					if (pMasterTable->CanLeaveTable(pPlayer) && pMasterTable->LeaveTable(pPlayer, true))
					{
						result_leave = 2;
					}
					else
					{
						result_leave = 3;
					}
				}
				CGameRoom * pMasterRoom = pPlayer->GetRoom();
				if (pMasterRoom != NULL)
				{
					if (pMasterRoom->CanLeaveRoom(pPlayer) && pMasterRoom->LeaveRoom(pPlayer))
					{
						result_leave = 4;
					}
					else
					{
						result_leave = 5;
					}
				}
				break;
			}
		}
	}
	LOG_DEBUG("roomid:%d,tableid:%d,suid:%d,tuid:%d,bIsUserControl:%d,result_leave:%d",
		GetRoomID(), GetTableID(), suid, pTagPlayer->GetUID(), bIsUserControl, result_leave);
	return true;
}

void CGameTable::InitMasterUserShowInfo(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		return;
	}
	if (IsMasterGame() == false)
	{
		return;
	}
	if (GetIsMasterUser(pPlayer))
	{
		pPlayer->SetShowUid(CDataCfgMgr::Instance().GetMasterRandUid());
		pPlayer->SetShowCity(CDataCfgMgr::Instance().GetMasterRandCity());
		int64 lShowCoin = g_RandGen.RandRange(m_pHostRoom->GetShowMasterMinScore(), m_pHostRoom->GetShowMasterMaxScore());
		pPlayer->SetShowCoin(lShowCoin);
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,suid:%d,scity:%s,coin:%lld,scoin:%lld,RangeScore:%lld-%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->GetShowUid(), pPlayer->GetShowCity().c_str(), GetPlayerCurScore(pPlayer), pPlayer->GetShowCoin(), m_pHostRoom->GetShowMasterMinScore(), m_pHostRoom->GetShowMasterMaxScore());

	}

}


void CGameTable::UpdateMasterUserShowInfo(CGamePlayer *pPlayer, int64 lScore)
{
	if (pPlayer == NULL)
	{
		return;
	}
	if (IsMasterGame() == false)
	{
		return;
	}
	if (GetIsMasterUser(pPlayer))
	{
		pPlayer->UpdateShowCoin(lScore);
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,suid:%d,scity:%s,lScore:%lld,coin:%lld,scoin:%lld,RangeScore:%lld-%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->GetShowUid(), pPlayer->GetShowCity().c_str(), lScore, GetPlayerCurScore(pPlayer), pPlayer->GetShowCoin(), m_pHostRoom->GetShowMasterMinScore(), m_pHostRoom->GetShowMasterMaxScore());

	}
}

void CGameTable::UpdateMasterGameInfo()
{
	if (IsMasterGame() == false)
	{
		return;
	}
	SendSeatInfoToClient();
}

void CGameTable::OnBrcControlFlushTableStatus(CGamePlayer *pPlayer)
{
	//发送当前局的控制结果
	LOG_DEBUG("send brc control flush table status. status:%d time:%d", m_brc_table_status, m_brc_table_status_time);
	net::msg_brc_control_game_status_info rep;

	//增加当庄玩家
	rep.set_status(m_brc_table_status);
	rep.set_time(m_brc_table_status_time);

	if (pPlayer == NULL)
	{
		for (auto &it : m_setControlPlayers)
		{
			it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_GAME_STATUS_INFO);
		}
	}
	else
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_GAME_STATUS_INFO);
	}
}

//百人场精准控制---刷新控制区域
bool CGameTable::OnBrcControlFlushAreaInfo(CGamePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_ERROR("PLAYER IS NULL");
		return false;
	}

	//返回结果消息
	net::msg_brc_control_flush_area_info rep;
	rep.set_times(m_control_number);
	for (int i = 0; i < BRC_MAX_CONTROL_AREA; i++)
	{
		rep.add_area(m_req_control_area[i]);
		LOG_DEBUG("brc control area uid:%d i:%d value:%d.", pPlayer->GetUID(), i, m_req_control_area[i]);
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_FLUSH_AREA_INFO);
	return true;
}

// 获取下注的是否全部为机器人或全部为玩家  add by har
bool CGameTable::IsAllRobotOrPlayerJetton() {
	bool isAllPlayer = true; // 是否全部是玩家
	bool isAllRobot = true; // 是否全部是机器人
	for (uint16 wUserIndex = 0; wUserIndex < m_maxChairNum; ++wUserIndex) {
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer != NULL) {
			IsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot);
			if (!isAllPlayer && !isAllRobot)
				return false;
		}
	}
		
	//旁观者
	for (map<uint32, CGamePlayer*>::iterator it_win_robot_score = m_mpLookers.begin(); it_win_robot_score != m_mpLookers.end(); ++it_win_robot_score)
		if (it_win_robot_score->second != NULL) {
			IsRobotOrPlayerJetton(it_win_robot_score->second, isAllPlayer, isAllRobot);
			if (!isAllPlayer && !isAllRobot)
				return false;
		}

	if (m_pCurBanker != NULL) {
		if (m_pCurBanker->IsRobot())
			isAllPlayer = false;
		else
			isAllRobot = false;
	}
	if (isAllPlayer || isAllRobot)
		return true;
	return false;
}

// 处理获取单个下注的是机器人还是玩家（针对对战类库存系统）  add by har
void CGameTable::DealIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot, uint8	cbPlayStatus[]) {
	for (uint16 i = 0; i < m_maxChairNum; ++i) {
		if (cbPlayStatus[i] == FALSE)
			continue;
		CGamePlayer *pPlayer2 = GetPlayer(i);
		if (pPlayer2 != NULL && pPlayer2 == pPlayer) {
			if (pPlayer->IsRobot())
				isAllPlayer = false;
			else
				isAllRobot = false;
			return;
		}
	}
}

// 处理获取单个下注的是机器人还是玩家（针对少数玩家对战类库存系统）  add by har
void CGameTable::DealSIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 i = 0; i < m_maxChairNum; ++i) {
		CGamePlayer *pPlayer2 = GetPlayer(i);
		if (pPlayer2 != NULL && pPlayer2 == pPlayer && m_vecPlayers[i].readyState != 0) {
			if (pPlayer->IsRobot())
				isAllPlayer = false;
			else
				isAllRobot = false;
			return;
		}
	}
}

// 检查库存改变是否成功  add by har
bool CGameTable::CheckStockChange(int64 stockChange, int64 playerAllWinScore, int i) {
	if (stockChange == -1 && playerAllWinScore < 1) {
		LOG_DEBUG("CheckStockChange suc roomid:%d,tableid:%d,i:%d,playerAllWinScore:%d,stockChange:%d", GetRoomID(), GetTableID(), i, playerAllWinScore, stockChange);
		return true;
	}
	if (stockChange == -2 && playerAllWinScore > -1) {
		LOG_DEBUG("CheckStockChange suc2 roomid:%d,tableid:%d,i:%d,playerAllWinScore:%d,stockChange:%d", GetRoomID(), GetTableID(), i, playerAllWinScore, stockChange);
		return true;
	}
	if (stockChange > 0 && playerAllWinScore > -1 && playerAllWinScore <= stockChange) {
		LOG_DEBUG("CheckStockChange suc3 roomid:%d,tableid:%d,i:%d,playerAllWinScore:%d,stockChange:%d", GetRoomID(), GetTableID(), i, playerAllWinScore, stockChange);
		return true;
	}
	return false;
}

// 庄家是否是真实玩家  add by har
bool CGameTable::IsBankerRealPlayer() {
	return m_pCurBanker != NULL && !m_pCurBanker->IsRobot();
}

// 百人场获取所有观众机器人（庄除外）  add by har
void CGameTable::GetAllLookersRobotPlayer(vector<CGamePlayer*> &robots) {
	robots.clear(); 
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer *pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
			continue;
		robots.push_back(pPlayer);
	}
}

// 百人场获取所有机器人（庄除外）  add by har
void CGameTable::GetAllRobotPlayer(vector<CGamePlayer*> &robots) {
	GetAllLookersRobotPlayer(robots);
	for (uint16 wUserIndex = 0; wUserIndex < m_maxChairNum; ++wUserIndex) {
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
			continue;
		robots.push_back(pPlayer);
	}
}

//获取当前桌子的幸运值标志，参数返回:赢玩家一个，输玩家为列表 一个或多个
bool CGameTable::GetTableLuckyFlag(uint32 &win_uid, set<uint32> &set_lose_uid)
{
	//本局幸运值控制玩家个数
	uint8 ctrl_player_num = 0;
	uint8 ctrl_win_num = 0;
	uint8 ctrl_lose_num = 0;
	set_lose_uid.clear();

	//桌子位置所有玩家
	for (uint32 i = 0; i < m_vecPlayers.size(); i++)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer == NULL)
		{
			continue;
		}
		bool is_win = false;
		if (pPlayer->GetLuckyFlag(GetRoomID(), is_win))
		{
			if (is_win)
			{
				win_uid = pPlayer->GetUID();
				ctrl_win_num++;
			}
			else
			{
				set_lose_uid.insert(pPlayer->GetUID());
				ctrl_lose_num++;
			}
			ctrl_player_num++;
		}
	}
	LOG_DEBUG("ctrl_player_num:%d ctrl_win_num:%d ctrl_lose_num:%d m_vecPlayers.size:%d.", ctrl_player_num, ctrl_win_num, ctrl_lose_num, m_vecPlayers.size());
	
	//只有一个玩家赢的情况下，返回真
	if (ctrl_win_num == 1)
	{
		LOG_DEBUG("the ctrl_win_num is 1 return true.");
		return true;
	}

	//赢家个数大于1的情况下，返回假
	if (ctrl_win_num > 1)
	{
		LOG_DEBUG("the ctrl_win_num is more than 1 return false.");
		return false;
	}

	//如果当前控输的玩家数小于当前桌子人数，返回真
	if (ctrl_lose_num > 0 && ctrl_lose_num < m_vecPlayers.size())
	{
		LOG_DEBUG("the ctrl_lose_num is less then m_vecPlayers.size return true.");
		return true;
	}		
	return false;	
}

// 结算玩家信息---捕鱼场
void    CGameTable::CalcPlayerGameInfoForFish(uint32 uid, int64 winScore)
{
	LOG_DEBUG("calc player fish game info uid:%d winScore:%lld", uid, winScore);
	if (winScore == 0)
		return;

	bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if (pPlayer != NULL)
	{
		if (winScore != 0)
		{
			//更新用户基础表数据
			ChangeScoreValueByUID(uid, winScore, emACCTRAN_OPER_TYPE_GAME, m_pHostRoom->GetGameType());

			//更新玩家游戏表数据
			pPlayer->AsyncChangeGameValueForFish(true, winScore);

			//同步数据到大厅
			ReportGameResult(uid, winScore, 0, false, 0);
		}		
		FlushUserBlingLog(pPlayer, winScore, 0);
	}	
	return;
}

//判断当前玩家是否为百人场的控制玩家
uint32	CGameTable::IsBrcControlPlayer(uint32 uid)
{
	set<uint32>::iterator it = m_tableCtrlPlayers.find(uid);
	if (it == m_tableCtrlPlayers.end())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}