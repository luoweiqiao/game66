//
// Created by toney on 16/4/6.
//
#include <unordered_set>
#include <data_cfg_mgr.h>
#include <center_log.h>
#include "game_imple_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "json/json.h"
#include "robot_mgr.h"

using namespace std;
using namespace svrlib;
using namespace net;
using namespace game_texas;

namespace
{
    const static uint32 s_AddScoreTime = 15*1000;
    const static uint32 s_RestTime     = 10*1000;
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 德州
        {
            pTable = new CGameTexasTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛德州
        {
            pTable = new CGameTexasTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房德州
        {
            pTable = new CGameTexasTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}
// 梭哈游戏桌子
CGameTexasTable::CGameTexasTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();
	//玩家变量
	m_wDUser = INVALID_CHAIR;
	m_wCurrentUser = INVALID_CHAIR;
	
	//玩家状态
	memset(m_cbPlayStatus,0,sizeof(m_cbPlayStatus));
	memset(m_cbFirstEnterStatus, 0, sizeof(m_cbFirstEnterStatus));
	memset(m_cbHaveCard, 0, sizeof(m_cbHaveCard));
	memset(m_cbWaitTrun, 0, sizeof(m_cbWaitTrun));
	//扑克变量
	m_cbSendCardCount = 0;
	memset(m_cbCenterCardData,0,sizeof(m_cbCenterCardData));
	memset(m_cbHandCardData,0,sizeof(m_cbHandCardData));

	//加注变量
	m_lCellScore     = 0L;
	m_lTurnLessScore = 0L;
	m_lAddLessScore  = 0L;
	m_lTurnMaxScore  = 0L;
	m_wOperaCount    = 0;
	m_cbBalanceCount = 0;
	m_lBalanceScore  = 0L;
	memset(m_lTableScore,0,sizeof(m_lTableScore));
	memset(m_lTotalScore,0,sizeof(m_lTotalScore));
	memset(m_cbShowHand, 0,sizeof(m_cbShowHand));    
	m_bIsControlPlayer = false;
    return;
}
CGameTexasTable::~CGameTexasTable()
{

}
bool    CGameTexasTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer->GetTable() != NULL)
	{
		return false;
	}
    // 限额进入
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin())
	{
        return false;
    }
	if (pPlayer->IsRobot() && (GetChairPlayerNum() >= 5))
	{
		return false;
	}

	bool bIsNoEnterNoviceWelfare = EnterNoviceWelfare(pPlayer);

	//LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNoEnterNoviceWelfare:%d",
	//	pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNoEnterNoviceWelfare);

	if (bIsNoEnterNoviceWelfare == true )
	{
		return false;
	}

    return true;
}
bool    CGameTexasTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    if(GetGameState() == TABLE_STATE_PLAY)
    {
        for(uint16 i=0;i<m_vecPlayers.size();++i){
            if(m_vecPlayers[i].pPlayer == pPlayer)
            {
                if(m_cbPlayStatus[i] == TRUE)
                    return false;
                break;
            }
        }
    }
    
    return true;
}
bool    CGameTexasTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
    if(GetBuyinScore(pPlayer->GetUID()) < GetSitDownScore()){
        LOG_DEBUG("buyin积分小于最小带入:%lld--%lld",GetBuyinScore(pPlayer->GetUID()),GetSitDownScore());
        return false;
    }
    if(GetBuyinScore(pPlayer->GetUID()) > GetPlayerCurScore(pPlayer)){
        LOG_DEBUG("buyin积分大于身上积分:%lld--%lld",GetBuyinScore(pPlayer->GetUID()),GetPlayerCurScore(pPlayer));
        return false;
    }
    if(!IsExistLooker(pPlayer->GetUID())){
        LOG_DEBUG("not in looklist:%d",pPlayer->GetUID());
        return false;
    }
    if(chairID >= m_vecPlayers.size()){
        LOG_DEBUG("the seat is more big:%d",chairID);
        return false;
    }
    if(m_vecPlayers[chairID].pPlayer != NULL){
        LOG_DEBUG("the seat is other player");
        return false;
    }
	// add by har
	if (m_mpWaitBuyinScore.find(pPlayer->GetUID()) != m_mpWaitBuyinScore.end()) {
		LOG_WARNING("CGameTexasTable::CanSitDown  wait  roomid:%d,tableid:%d,uid:%d,chairid:%d,waitCharid:%d", 
			GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, m_mpWaitBuyinScore[pPlayer->GetUID()]);
		return false;
	}
	for (auto &ait : m_mpWaitBuyinScore)
		if (chairID == ait.second) {
			LOG_WARNING("CGameTexasTable::CanSitDown  wait2  roomid:%d,tableid:%d,uid:%d,waituid:%d,chairid:%d", 
				GetRoomID(), GetTableID(), pPlayer->GetUID(), ait.first, chairID);
			return false;
		}
	// add by har end
    return true;
}
void CGameTexasTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::texas_table_info* ptexas = pInfo->mutable_texas();
    ptexas->set_tableid(GetTableID());
    ptexas->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        ptexas->set_is_passwd(1);
    }else{
        ptexas->set_is_passwd(0);
    }
    ptexas->set_hostname(m_conf.hostName);
    ptexas->set_basescore(m_conf.baseScore);
    ptexas->set_consume(m_conf.consume);
    ptexas->set_entermin(m_conf.enterMin);
    ptexas->set_duetime(m_conf.dueTime);
    ptexas->set_feetype(m_conf.feeType);
    ptexas->set_feevalue(m_conf.feeValue);
    ptexas->set_card_time(s_AddScoreTime);
    ptexas->set_table_state(GetGameState());
    ptexas->set_sitdown(GetSitDownScore());
    ptexas->set_player_num(GetChairPlayerNum());

}
// 扣除开始台费
void CGameTexasTable::DeductStartFee()
{
    LOG_DEBUG("Deduct Start Fee");
    if(m_conf.feeType == TABLE_FEE_TYPE_ALLBASE)
    {
        int64 fee = -(m_conf.baseScore * m_conf.feeValue/PRO_DENO_10000);
        for(uint32 i=0;i<m_vecPlayers.size();++i)
        {
            CGamePlayer* pPlayer = GetPlayer(i);
            if(pPlayer == NULL)continue;
            
            ChangeScoreValue(i,fee,emACCTRAN_OPER_TYPE_FEE,GetTableID());
            ChangeBuyinScore(pPlayer->GetUID(),fee);
            // 处理桌子收益，房主收益
            m_hostIncome += -fee;
            ChangePrivateTableIncome(-fee,0);
            ChangeUserBlingLogFee(pPlayer->GetUID(),fee);
            LogFee(pPlayer->GetUID(),0,-fee);
        }
        SendSeatInfoToClient();
    }
}
//配置桌子
bool CGameTexasTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
	m_iArrDispatchCardPro[Texas_Pro_Index_KingTongHuaShun] = 0;
	m_iArrDispatchCardPro[Texas_Pro_Index_TongHuaShun] = 20;
	m_iArrDispatchCardPro[Texas_Pro_Index_TieZhi] = 50;
	m_iArrDispatchCardPro[Texas_Pro_Index_HuLu] = 230;
	m_iArrDispatchCardPro[Texas_Pro_Index_TongHua] = 500;
	m_iArrDispatchCardPro[Texas_Pro_Index_ShunZi] = 1000;
	m_iArrDispatchCardPro[Texas_Pro_Index_ThreeTiao] = 1000;
	m_iArrDispatchCardPro[Texas_Pro_Index_TwoDouble] = 1200;
	m_iArrDispatchCardPro[Texas_Pro_Index_OneDouble] = 3000;
	m_iArrDispatchCardPro[Texas_Pro_Index_Single] = 3000;
	m_confArrDispatchCardAllPro = 10000; // add by har

	InitSubWelfareHandCard();

	ReAnalysisParam();
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}
void CGameTexasTable::ShutDown()
{
      
}

bool CGameTexasTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", m_pHostRoom->GetRoomID(), param.c_str());
		return true;
	}
	if (jvalue.isMember("rmc")) {
		m_robotBankerMaxCardPro = jvalue["rmc"].asInt();
	}
	
	m_confArrDispatchCardAllPro = 0;
	for (int i = 0; i < Texas_Pro_Index_MAX; ++i) {
		string strPro = CStringUtility::FormatToString("pr%d", i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral()) {
			m_iArrDispatchCardPro[i] = jvalue[strPro.c_str()].asInt();
			m_confArrDispatchCardAllPro += m_iArrDispatchCardPro[i];
		}
	}
	//LOG_ERROR("reader json parse success - roomid:%d,tableid:%d,m_iArrDispatchCardPro:%d %d %d %d %d %d %d %d %d %d,m_robotBankerMaxCardPro:%d",
	//	GetRoomID(), GetTableID(), m_iArrDispatchCardPro[0], m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3], m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5], m_iArrDispatchCardPro[6], m_iArrDispatchCardPro[7], m_iArrDispatchCardPro[8], m_iArrDispatchCardPro[9], m_robotBankerMaxCardPro);


	LOG_DEBUG("reader json parse success - roomid:%d,tableid:%d,m_robotBankerMaxCardPro:%d,m_confArrDispatchCardAllPro:%d,m_iArrDispatchCardPro:%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
		m_pHostRoom->GetRoomID(), GetTableID(), m_robotBankerMaxCardPro, m_confArrDispatchCardAllPro, m_iArrDispatchCardPro[0],
		m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3], m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5], 
		m_iArrDispatchCardPro[6], m_iArrDispatchCardPro[7], m_iArrDispatchCardPro[8], m_iArrDispatchCardPro[9]);
	return true;
}

//复位桌子
void CGameTexasTable::ResetTable()
{
    SetGameState(TABLE_STATE_FREE);
    ResetGameData();
    StandUpNotScore();
	SendSeatInfoToClient(); // add by har
	OnTableGameEnd(); // add by har
	CheckPlayerScoreManyLeave();
}

void CGameTexasTable::OnTimeTick()
{
	OnTableTick();

    if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FREE:           // 空闲
            {
                if(GetChairPlayerNum() >= 2){
                    OnGameStart();
                }              
            }break;
        case TABLE_STATE_PLAY:          // 下注时间
            {
                m_vecPlayers[m_wCurrentUser].overTimes++;
                if(m_vecPlayers[m_wCurrentUser].overTimes == 2){// 超时2次托管
                    m_vecPlayers[m_wCurrentUser].autoState = 1;
					LOG_DEBUG("下注时间到,超时2次托管   roomid:%d,tableid:%d,m_wCurrentUser:%d,uid:%d", GetRoomID(), GetTableID(), m_wCurrentUser, GetPlayerID(m_wCurrentUser));
                }
                if(m_lTurnLessScore > 0) {
					LOG_DEBUG("下注时间到,自动弃牌   roomid:%d,tableid:%d,m_wCurrentUser:%d,uid:%d,m_lTurnLessScore:%d", GetRoomID(), GetTableID(), m_wCurrentUser, GetPlayerID(m_wCurrentUser), m_lTurnLessScore);
                    OnUserGiveUp(m_wCurrentUser);
                }else{
					LOG_DEBUG("下注时间到,自动下注   roomid:%d,tableid:%d,m_wCurrentUser:%d,uid:%d,m_lTurnLessScore:%d", GetRoomID(), GetTableID(), m_wCurrentUser, GetPlayerID(m_wCurrentUser), m_lTurnLessScore);
                    OnUserAddScore(m_wCurrentUser,m_lTurnLessScore,false);
                }
            }break;      
        default:
            break;
        }
    }
    if(GetGameState() == TABLE_STATE_FREE){
       CheckAddRobot(); 
    }    
    if(GetGameState() == TABLE_STATE_PLAY && m_coolRobot.isTimeOut())
    {
        CGamePlayer* pPlayer = m_vecPlayers[m_wCurrentUser].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot()){
           OnRobotOper(m_wCurrentUser);
        }
    }
}
// 游戏消息
int CGameTexasTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("收到玩家消息  roomid:%d,tableid:%d,uid:%d-chairID:%d-cmd:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, cmdID);
    
    switch(cmdID)
    {
    case net::C2S_MSG_TEXAS_ADD_SCORE:  // 用户加注
        {
            net::msg_texas_addscore_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
			//状态判断
			if(chairID > GAME_PLAYER || m_cbPlayStatus[chairID]==FALSE) 
                return false;

			//消息处理
            m_vecPlayers[chairID].overTimes = 0;
			return OnUserAddScore(chairID,msg.add_score(),false);
        }break; 
    case net::C2S_MSG_TEXAS_GIVEUP:     // 用户弃牌
        {            
            net::msg_texas_giveup_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
			//状态判断
			if(chairID > GAME_PLAYER || m_cbPlayStatus[chairID]==FALSE) 
                return false;
            m_vecPlayers[chairID].overTimes = 0;
			//消息处理
			return OnUserGiveUp(chairID);           
        }break;
    case net::C2S_MSG_TEXAS_BUYIN:
        {
            net::msg_texas_buyin_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
            return OnUserBuyin(pPlayer,msg.score());
        }break;
	case net::C2S_MSG_TEXAS_BUYIN_NEXT:
	    {
		    net::msg_texas_buyin_next_req msg;
		    PARSE_MSG_FROM_ARRAY(msg);
		    OnUserBuyinNext(pPlayer, msg.score());
			return 0;
	    }break;
    case net::C2S_MSG_TEXAS_SHOW_CARD:
        {
            return OnUserShowCard(chairID);
        }break;
	case net::C2S_MSG_TEXAS_BUYIN_WAIT_STANDUP:
	{
		OnUserBuyinWaitStandUp(pPlayer);
		return 0;
	}break;
    default:
        return 0;
    }
    return 0;
}
// 用户断线或重连
bool CGameTexasTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
{
    if(bConnected)//断线重连
    {
        if(isJoin){
            pPlayer->SetPlayDisconnect(false);
            PlayerSetAuto(pPlayer,0);
            SendTableInfoToClient(pPlayer);
            SendSeatInfoToClient(pPlayer);
            SendGameScene(pPlayer);
        }
    }else{
        pPlayer->SetPlayDisconnect(true);
    }
    return true;
}
//用户坐下
bool CGameTexasTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{    
	m_cbFirstEnterStatus[wChairID] = TRUE;
	m_cbWaitTrun[wChairID] = 0;
    m_vecPlayers[wChairID].buyinScore = m_mpBuyinScore[pPlayer->GetUID()];
    
    SendSeatInfoToClient();

    LOG_DEBUG("玩家坐下 - uid:%d,wChairID:%d,buyinScore:%lld",pPlayer->GetUID(),wChairID,m_vecPlayers[wChairID].buyinScore);
    if(GetGameState() == TABLE_STATE_FREE && GetChairPlayerNum() == 2)
    {
        LOG_DEBUG("两个人开始，延迟2秒");
        m_coolLogic.beginCooling(2000);
    }    
    return true;
}
//用户起立
bool CGameTexasTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{	    
    WORD i=0;
	for(i=0;i<GAME_PLAYER;i++)
	{
		if(i == wChairID)
            continue;
		if(GetPlayer(i) != NULL)
		{
			break;
		}
	}
	//庄家设置
	if(i==GAME_PLAYER)
        m_wDUser=INVALID_CHAIR;

    m_mpBuyinScore.erase(pPlayer->GetUID());
	m_mpNextBuyinScore.erase(pPlayer->GetUID()); // add by har
	m_cbFirstEnterStatus[wChairID] = FALSE;
	m_cbHaveCard[wChairID] = FALSE;
	m_cbWaitTrun[wChairID] = 0;
    SendSeatInfoToClient();
    LOG_DEBUG("玩家站起 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,isrobot:%d", GetRoomID(), GetTableID(), wChairID, pPlayer->GetUID(), pPlayer->IsRobot());
    return true;
}
// 游戏开始
bool CGameTexasTable::OnGameStart()
{
    LOG_DEBUG("game start - roomid:%d,tableid:%d,time:%d",GetRoomID(),GetTableID(),getSysTime());
	//设置状态
	SetGameState(net::TABLE_STATE_PLAY);
    memset(m_cbHandCardData,0,sizeof(m_cbHandCardData));
	string strAllUID;
	
	m_lucky_flag = false;

	//游戏变量
	int wUserCount=0;
	int iFirstCount = 0;
	int iHaveCardCount = 0;

	string strAllUIDStatus;

	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		//获取用户
        CGamePlayer* pPlayer = GetPlayer(i);        
		//无效用户
		if (pPlayer == NULL)
		{
            continue;
		}
		//获取积分
		m_lUserMaxScore[i] = GetBuyinScore(pPlayer->GetUID());
		strAllUIDStatus += CStringUtility::FormatToString("i_%d_u_%d_f_%d_c_%d_m_%d ", i, GetPlayerID(i), m_cbFirstEnterStatus[i], m_cbHaveCard[i], m_lUserMaxScore[i]);

		//if (m_wDUser == INVALID_CHAIR)
		//{
		//	m_cbFirstEnterStatus[i] = FALSE;
		//}
		//设置状态
		//m_cbPlayStatus[i] = TRUE;
		//m_szCardState[i]  = 1;
		if (m_cbFirstEnterStatus[i] == TRUE)
		{
			iFirstCount++;
		}
		if (m_cbHaveCard[i] == TRUE)
		{
			iHaveCardCount++;

			m_cbPlayStatus[i] = TRUE;
			m_szCardState[i] = 1;
		}
		wUserCount++;
	}
	m_lCellScore = GetBaseScore();
	WORD wFrontDUser = m_wDUser;
	if (wFrontDUser == INVALID_CHAIR)
	{
		memset(m_cbFirstEnterStatus, 0, sizeof(m_cbFirstEnterStatus));
	}
	
	//if (wUserCount >= 3 && m_wDUser!= INVALID_CHAIR && wUserCount-iFirstCount>=2)
	WORD wPlayerChairIDFlag[4] = { INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR }, wPlayerCount = 0;
	
	if(wUserCount > iHaveCardCount && iHaveCardCount>=2 && iFirstCount > 0 && wFrontDUser != INVALID_CHAIR)
	{
		//WORD wNextUser = (wFrontDUser + 1) % GAME_PLAYER;
		WORD wNextUser = wFrontDUser;
		int iNextDLoop = 0;
		do
		{
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
			if (GetPlayer(wNextUser) != NULL && m_cbHaveCard[wNextUser] == TRUE)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
				m_cbPlayStatus[wNextUser] = TRUE;
				m_szCardState[wNextUser] = 1;
				break;
			}
			iNextDLoop++;
		} while (iNextDLoop <= GAME_PLAYER); // 0 庄家

		iNextDLoop = 0;
		do
		{
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
			if (GetPlayer(wNextUser) != NULL && m_cbHaveCard[wNextUser] == TRUE)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
				m_cbPlayStatus[wNextUser] = TRUE;
				m_szCardState[wNextUser] = 1;
				break;
			}
			iNextDLoop++;
		} while (iNextDLoop <= GAME_PLAYER); // 1 小忙

		iNextDLoop = 0;
		do
		{
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
			if (GetPlayer(wNextUser) != NULL)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
				m_cbPlayStatus[wNextUser] = TRUE;
				m_szCardState[wNextUser] = 1;
				break;
			}
			iNextDLoop++;
		} while (iNextDLoop <= GAME_PLAYER); // 2 大忙

		//iNextDLoop = 0;
		//do
		//{
		//	wNextUser = (wNextUser + 1) % GAME_PLAYER;
		//	bool bIsNextCurUser_o = (GetPlayer(wNextUser) != NULL && m_cbHaveCard[wNextUser] == TRUE);
		//	bool bIsNextCurUser_t = (wNextUser == wPlayerChairIDFlag[0] || wNextUser == wPlayerChairIDFlag[1] || wNextUser == wPlayerChairIDFlag[2]);
		//	if (bIsNextCurUser_o || bIsNextCurUser_t)
		//	{
		//		wPlayerChairIDFlag[wPlayerCount] = wNextUser;
		//		wPlayerCount++;
		//		m_cbPlayStatus[wNextUser] = TRUE;
		//		m_szCardState[wNextUser] = 1;
		//		break;
		//	}
		//	iNextDLoop++;
		//} while (iNextDLoop <= GAME_PLAYER); // 3 当前用户

		for (WORD i = 0; i < GAME_PLAYER; i++)
		{
			CGamePlayer* pPlayer = GetPlayer(i);
			if (pPlayer == NULL)
			{
				continue;
			}
			//if (m_cbFirstEnterStatus[i] == TRUE && wPlayerChairIDFlag[2] != i)
			//{
			//	m_cbPlayStatus[i] = FALSE;
			//}
			if (m_cbFirstEnterStatus[i] == TRUE && wPlayerChairIDFlag[2] == i)
			{
				m_cbFirstEnterStatus[i] = FALSE;
			}
		}

		string strFrontPlayerStatus;
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL)
			{
				strFrontPlayerStatus += CStringUtility::FormatToString("i_%d_s_%d ",i, m_cbPlayStatus[i]);
			}
		}
		string strWaitTrun;
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL)
			{
				strWaitTrun += CStringUtility::FormatToString("i_%d_w_%d ",i, m_cbWaitTrun[i]);
			}
		}


		//for (WORD i = 0; i<GAME_PLAYER; i++)
		//{
		//	if (GetPlayer(i) != NULL && m_cbPlayStatus[i] == FALSE && m_cbWaitTrun[i] != 0 && wPlayerChairIDFlag[1] != i)
		//	{
		//		m_cbPlayStatus[i] = TRUE;
		//		m_cbFirstEnterStatus[i] = FALSE;
		//	}
		//}

		iNextDLoop = 0;
		WORD wTempNextUser = wNextUser;
		do
		{
			wTempNextUser = (wTempNextUser + 1) % GAME_PLAYER;
			if (wTempNextUser == wPlayerChairIDFlag[0])
			{
				break;
			}
			if (GetPlayer(wTempNextUser) != NULL && m_cbPlayStatus[wTempNextUser] == FALSE && m_cbWaitTrun[wTempNextUser] != 0 && wPlayerChairIDFlag[1] != wTempNextUser)
			{
				m_cbFirstEnterStatus[wTempNextUser] = FALSE;
				m_cbPlayStatus[wTempNextUser] = TRUE;
				m_szCardState[wTempNextUser] = 1;
			}
			iNextDLoop++;
		} while (iNextDLoop <= GAME_PLAYER);


		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL && m_cbPlayStatus[i] == FALSE)
			{
				m_cbWaitTrun[i]++;
			}
		}

		string strWaitTrun_w;
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL)
			{
				strWaitTrun_w += CStringUtility::FormatToString("i_%d_w_%d ", i, m_cbWaitTrun[i]);
			}
		}

		iNextDLoop = 0;
		do
		{
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
			//bool bIsNextCurUser_o = (GetPlayer(wNextUser) != NULL && m_cbPlayStatus[wNextUser] == TRUE);
			//bool bIsNextCurUser_t = (wNextUser == wPlayerChairIDFlag[0] || wNextUser == wPlayerChairIDFlag[1] || wNextUser == wPlayerChairIDFlag[2]);
			//if (bIsNextCurUser_o || bIsNextCurUser_t)
			if (GetPlayer(wNextUser) != NULL && m_cbPlayStatus[wNextUser] == TRUE)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
				break;
			}
			iNextDLoop++;
		} while (iNextDLoop <= GAME_PLAYER); // 3 当前用户


		string strNextPlayerStatus;
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL)
			{
				strNextPlayerStatus += CStringUtility::FormatToString("i_%d_s_%d ", i, m_cbPlayStatus[i]);
			}
		}
		string strAllPlayerChairIDFlag;
		for (int i = 0; i < 4; i++)
		{
			strAllPlayerChairIDFlag += CStringUtility::FormatToString("%d ", wPlayerChairIDFlag[i]);
		}
		LOG_DEBUG("cacl_player_info roomid:%d,tableid:%d,wPlayerCount:%d,strAllPlayerChairIDFlag:%s,strFrontPlayerStatus:%s,strNextPlayerStatus:%s,wUserCount:%d,iHaveCardCount:%d,iFirstCount:%d,wFrontDUser:%d,m_wDUser:%d,strAllUIDStatus:%s,strWaitTrun:%s,strWaitTrun_w:%s",
			GetRoomID(), GetTableID(), wPlayerCount, strAllPlayerChairIDFlag.c_str(), strFrontPlayerStatus.c_str(), strNextPlayerStatus.c_str(), wUserCount, iHaveCardCount, iFirstCount, wFrontDUser, m_wDUser, strAllUIDStatus.c_str(), strWaitTrun.c_str(), strWaitTrun_w.c_str());

		//for (int i = 0; i < 4; i++)
		//{
		//	wPlayerChairIDFlag[i] = INVALID_CHAIR;
		//}

		//wPlayerCount = 0;
		//wNextUser = m_wDUser;
		//do
		//{
		//	wNextUser = (wNextUser + 1) % GAME_PLAYER;
		//	if (m_cbPlayStatus[wNextUser] == TRUE)
		//	{
		//		wPlayerChairIDFlag[wPlayerCount] = wNextUser;
		//		wPlayerCount++;
		//	}			
		//} while (wPlayerCount < 4);
	}
	else
	{
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (GetPlayer(i) != NULL)
			{
				m_cbPlayStatus[i] = TRUE;
				m_szCardState[i]  = 1;
			}
		}
		memset(m_cbFirstEnterStatus, 0, sizeof(m_cbFirstEnterStatus));
		memset(m_cbHaveCard, 0, sizeof(m_cbHaveCard));
		memset(m_cbWaitTrun, 0, sizeof(m_cbWaitTrun));
		wPlayerCount = 0;
		if(m_wDUser == INVALID_CHAIR)
		{
			m_wDUser = 0;
		}
		else
		{
			m_wDUser = (m_wDUser + 1) % GAME_PLAYER;
		}
		//盲注玩家
		//WORD wPlayer[] = { INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR }, wPlayerCount = 0;
		WORD wNextUser = m_wDUser;
		do
		{
			if (m_cbPlayStatus[wNextUser] == TRUE)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
			}
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
		} while (wPlayerCount < 4);
	}

	bool bIsErrorFlag = false;
	if (wPlayerChairIDFlag[0] < GAME_PLAYER || wPlayerChairIDFlag[1] < GAME_PLAYER || wPlayerChairIDFlag[2] < GAME_PLAYER || wPlayerChairIDFlag[3] < GAME_PLAYER)
	{
		if (m_cbPlayStatus[wPlayerChairIDFlag[0]] != TRUE || m_cbPlayStatus[wPlayerChairIDFlag[1]] != TRUE || m_cbPlayStatus[wPlayerChairIDFlag[2]] != TRUE || m_cbPlayStatus[wPlayerChairIDFlag[3]] != TRUE)
		{
			bIsErrorFlag = true;
		}
	}
	else
	{
		bIsErrorFlag = true;
	}
	int iPlayStatusCount = 0;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_cbPlayStatus[i] == TRUE)
		{
			iPlayStatusCount++;
		}
	}
	if (iPlayStatusCount <= 1)
	{
		bIsErrorFlag = true;
	}

	if (bIsErrorFlag)
	{
		string strErrorPlayerChairIDFlag;
		for (int i = 0; i < 4; i++)
		{
			strErrorPlayerChairIDFlag += CStringUtility::FormatToString("%d ", wPlayerChairIDFlag[i]);
		}
		string strAllPlayerStatus;
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			if (m_cbPlayStatus[i] == TRUE)
			{
				strAllPlayerStatus += CStringUtility::FormatToString("%d ", m_cbPlayStatus[i]);
			}
		}

		LOG_DEBUG("error_all_player roomid:%d,tableid:%d,wPlayerChairIDFlag:%s,iPlayStatusCount:%d,iHaveCardCount:%d,iFirstCount:%d,wFrontDUser:%d,m_wDUser:%d,strAllUIDStatus:%s,strAllPlayerStatus:%s",
			GetRoomID(), GetTableID(), strErrorPlayerChairIDFlag.c_str(), iPlayStatusCount,iHaveCardCount, iFirstCount, wFrontDUser, m_wDUser, strAllUIDStatus.c_str(), strAllPlayerStatus.c_str());


		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			CGamePlayer* pPlayer = GetPlayer(i);
			if (pPlayer == NULL)
			{
				continue;
			}
			m_lUserMaxScore[i] = GetBuyinScore(pPlayer->GetUID());
			m_cbPlayStatus[i] = TRUE;
			m_szCardState[i] = 1;
		}
		memset(m_cbFirstEnterStatus, 0, sizeof(m_cbFirstEnterStatus));
		wPlayerCount = 0;
		if (m_wDUser == INVALID_CHAIR)
		{
			m_wDUser = 0;
		}
		else
		{
			m_wDUser = (m_wDUser + 1) % GAME_PLAYER;
		}
		WORD wNextUser = m_wDUser;
		do
		{
			if (m_cbPlayStatus[wNextUser] == TRUE)
			{
				wPlayerChairIDFlag[wPlayerCount] = wNextUser;
				wPlayerCount++;
			}
			wNextUser = (wNextUser + 1) % GAME_PLAYER;
		} while (wPlayerCount < 4);
	}




	string strPlayerChairIDFlag;
	for (int i = 0; i < 4; i++)
	{
		strPlayerChairIDFlag += CStringUtility::FormatToString("%d ", wPlayerChairIDFlag[i]);
	}

    //发牌
	int iAllPlayerCount = 0;
	uint16 wDispatchCardUserCount = 0;
	for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_cbPlayStatus[i] == TRUE)
		{
			wDispatchCardUserCount++;
		}
		iAllPlayerCount++;
	}
    DispatchCard(wDispatchCardUserCount);
	SetCardDataControl();

    for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		BYTE cbEndCardKind = m_GameLogic.FiveFromSeven(m_cbHandCardData[i],MAX_COUNT,m_cbCenterCardData,MAX_CENTERCOUNT,m_cbMaxCardData[i],MAX_CENTERCOUNT);                  
		m_cbEndCardType[i] = cbEndCardKind;
	}

	string strAllPlayerCard;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			strAllPlayerCard += CStringUtility::FormatToString("c_%d_0x%02X_0x%02X ",i, m_cbHandCardData[i][0], m_cbHandCardData[i][1]);
		}
	}

	LOG_DEBUG("all_player_info roomid:%d,tableid:%d,bIsErrorFlag:%d,wPlayerChairIDFlag:%s,iPlayStatusCount:%d,wUserCount:%d,iAllPlayerCount:%d,wDispatchCardUserCount:%d,iHaveCardCount:%d,iFirstCount:%d,wFrontDUser:%d,m_wDUser:%d,strAllUIDStatus:%s,strAllPlayerCard:%s",
		GetRoomID(), GetTableID(), bIsErrorFlag, strPlayerChairIDFlag.c_str(), iPlayStatusCount,wUserCount, iAllPlayerCount, wDispatchCardUserCount, iHaveCardCount, iFirstCount, wFrontDUser, m_wDUser, strAllUIDStatus.c_str(), strAllPlayerCard.c_str());


	//扑克数目
	m_cbSendCardCount = 0;
	m_cbBalanceCount  = 0;

	//首家判断
	//if(m_wDUser == INVALID_CHAIR)
	//{
	//	m_wDUser = 0;
	//}
	//else
	//{
	//	m_wDUser = (m_wDUser + 1) % GAME_PLAYER;
	//}
	////盲注玩家
	//WORD wPlayer[]={INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR,INVALID_CHAIR},wPlayerCount = 0;
	//WORD wNextUser = m_wDUser;
	//do
	//{
	//	if(m_cbPlayStatus[wNextUser] == TRUE)
	//	{
	//		wPlayer[wPlayerCount] = wNextUser;
	//		wPlayerCount++;
	//	}
	//	wNextUser = (wNextUser+1)%GAME_PLAYER;
	//}while(wPlayerCount < 4);

	m_wDUser            = wPlayerChairIDFlag[0];
    m_wMinChipinUser    = wPlayerChairIDFlag[1];
    m_wMaxChipinUser    = wPlayerChairIDFlag[2];
	m_wCurrentUser      = wPlayerChairIDFlag[3];
    LOG_DEBUG("CGameTexasTable::OnGameStart roomid:%d,tableid:%d,D位:%d,小盲:%d,大盲:%d,枪口(m_wCurrentUser):%d",GetRoomID(),GetTableID(),m_wDUser,m_wMinChipinUser,m_wMaxChipinUser,m_wCurrentUser);
	//当前下注
	m_lTableScore[m_wMinChipinUser]   = m_lCellScore;
	m_lTableScore[m_wMaxChipinUser]   = 2*m_lCellScore;
	m_lTotalScore[m_wMinChipinUser]   = m_lCellScore;
	m_lTotalScore[m_wMaxChipinUser]   = 2*m_lCellScore;

    m_lRoundScore[m_wMinChipinUser]   = m_lCellScore;
    m_lRoundScore[m_wMaxChipinUser]   = 2*m_lCellScore;

	//设置变量
	m_lBalanceScore     = 2L*m_lCellScore;
	m_lTurnMaxScore     = m_lUserMaxScore[m_wCurrentUser] - m_lTotalScore[m_wCurrentUser];
	m_lTurnLessScore    = m_lBalanceScore - m_lTotalScore[m_wCurrentUser];
	m_lAddLessScore     = 2L*m_lCellScore + m_lTurnLessScore;

    net::msg_texas_start_rep msg;
    msg.set_d_user(m_wDUser);
    msg.set_current_user(m_wCurrentUser);
    msg.set_min_chipin_user(m_wMinChipinUser);
    msg.set_max_chipin_user(m_wMaxChipinUser);
    msg.set_cell_score(m_lCellScore);
    msg.set_turn_max_score(m_lTurnMaxScore);
    msg.set_turn_less_score(m_lTurnLessScore);
    msg.set_add_less_score(m_lAddLessScore);
    
	memset(m_cbHaveCard, 0, sizeof(m_cbHaveCard));

	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			msg.add_card_chairid(i);
			m_cbHaveCard[i] = TRUE;
		}
	}

    //发送数据
    msg_cards* pcards = msg.mutable_card_datas();
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if(m_cbPlayStatus[i]==TRUE)
		{
			for(uint8 j=0;j<MAX_COUNT;++j){
                pcards->add_cards(m_cbHandCardData[i][j]);			    
			}
            SendMsgToClient(i,&msg,net::S2C_MSG_TEXAS_START);
            pcards->Clear();
		}
	}

	string strSendChairID;
	for (int32 k = 0; k < msg.card_chairid_size(); k++)
	{
		strSendChairID += CStringUtility::FormatToString("%d ", msg.card_chairid(k));
	}


	LOG_DEBUG("have_card_chairid - roomid:%d,tableid:%d,strAllUID:%s,card_chairid_size:%d,strSendChairID:%s,m_lBalanceScore:%d,m_lTotalScore[m_wCurrentUser]:%d,m_wDUser:%d,m_wCurrentUser:%d,m_wMinChipinUser:%d,m_wMaxChipinUser:%d,m_lCellScore:%d,m_lTurnMaxScore:%d,m_lTurnLessScore:%d,m_lTurnMaxScore:%d,m_lAddLessScore:%d",
		GetRoomID(), GetTableID(), strAllUID.c_str(), msg.card_chairid_size(), strSendChairID.c_str(), m_lBalanceScore, m_lTotalScore[m_wCurrentUser], m_wDUser, m_wCurrentUser, m_wMinChipinUser, m_wMaxChipinUser, m_lCellScore, m_lTurnMaxScore, m_lTurnLessScore, m_lTurnMaxScore, m_lAddLessScore);
	

	for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		if (GetPlayer(i) == NULL)
		{
			continue;
		}
		if (m_cbPlayStatus[i] == FALSE)
		{
			SendMsgToClient(i, &msg, net::S2C_MSG_TEXAS_START);
		}
	}
    SendMsgToLooker(&msg,net::S2C_MSG_TEXAS_START);

    m_coolLogic.beginCooling(s_AddScoreTime + 2000);
    m_coolRobot.beginCooling(g_RandGen.RandRange(2000+GetPlayNum()*200,6000));

    InitBlingLog();
    for(WORD i=0;i<GAME_PLAYER;i++){
        if(m_cbPlayStatus[i]==TRUE){
            WriteOutCardLog(i,m_cbHandCardData[i],MAX_COUNT);
        }
    }
    WriteAddScoreLog(m_wMinChipinUser,m_lTotalScore[m_wMinChipinUser],m_cbBalanceCount);
    WriteAddScoreLog(m_wMaxChipinUser,m_lTotalScore[m_wMaxChipinUser],m_cbBalanceCount);

    return true;
}
//游戏结束
bool CGameTexasTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    // LOG_DEBUG("game end:table:%d,%d,time:%d",GetTableID(),reason,getSysTime());
	LOG_DEBUG("game end - roomid:%d,tableid:%d,reason:%d,time:%d", GetRoomID(), GetTableID(), reason, getSysTime());

	switch(reason)
	{
	case GER_NORMAL:		//常规结束
		{
            //LOG_DEBUG("常规结束");
            net::msg_texas_game_end_rep gameend;
            gameend.set_total_end(1);

			BYTE cbEndCardData[GAME_PLAYER][MAX_CENTERCOUNT];
			memset(cbEndCardData,0,sizeof(cbEndCardData));
			try{
				//获取扑克
				for(WORD i=0;i<GAME_PLAYER;i++)
				{
					net::msg_cards* pCards = gameend.add_last_center_carddata();
					if(m_cbPlayStatus[i]==FALSE)
                        continue;
					BYTE cbEndCardKind = m_GameLogic.FiveFromSeven(m_cbHandCardData[i],MAX_COUNT,m_cbCenterCardData,MAX_CENTERCOUNT,cbEndCardData[i],MAX_CENTERCOUNT);
                    for(uint16 j=0;j<MAX_CENTERCOUNT;++j){
                        pCards->add_cards(cbEndCardData[i][j]);
                    }					
				}
                memcpy(m_cbMaxCardData,cbEndCardData,sizeof(m_cbMaxCardData));
			}catch(...)
			{
				LOG_ERROR("用户过滤v最大牌型");
				ASSERT(FALSE);
			}
			//总下注备份
			int64 lTotalScore[GAME_PLAYER];
			memset(lTotalScore,0,sizeof(lTotalScore));
			memcpy(lTotalScore,m_lTotalScore,sizeof(m_lTotalScore));

			//胜利列表
			UserWinList WinnerList[GAME_PLAYER];
			memset(WinnerList,0,sizeof(WinnerList));

			//临时数据
			BYTE bTempData[GAME_PLAYER][MAX_CENTERCOUNT];
			memcpy(bTempData,cbEndCardData,GAME_PLAYER*MAX_CENTERCOUNT);

			WORD wWinCount=0;
			try{
				//用户得分顺序
				for(WORD i=0;i<GAME_PLAYER;i++)
				{
					//查找最大用户
					if(!m_GameLogic.SelectMaxUser(bTempData,WinnerList[i],lTotalScore))
					{
						wWinCount=i;
						break;
					}
					//删除胜利数据
					for(WORD j=0;j<WinnerList[i].bSameCount;j++)
					{
						WORD wRemoveId=WinnerList[i].wWinerList[j];
						ASSERT(bTempData[wRemoveId][0]!=0);
						memset(bTempData[wRemoveId],0,sizeof(BYTE)*MAX_CENTERCOUNT);
					}
				}
			}catch(...)
			{
				LOG_ERROR("用户得分顺序");
				ASSERT(FALSE);
			}
			//强退用户
			for(WORD i=0;i<GAME_PLAYER;i++)
			{
				if(m_cbPlayStatus[i] == FALSE && lTotalScore[i] > 0)
				{					
					WinnerList[wWinCount].wWinerList[WinnerList[wWinCount].bSameCount++] = i;
				}
			}

			//得分变量
			int64 lUserScore[GAME_PLAYER];
			memset(lUserScore,0,sizeof(lUserScore));
            uint32 compareCount = 0;
			try
			{
				//得分情况
				for(int i=0;i<GAME_PLAYER-1;i++)
				{
                    UserWinList& refList = WinnerList[i];
                    int iWinCount = refList.bSameCount;
					if(0 == iWinCount)
                        break;
					//胜利用户得分情况
                    compareCount++;
					for(int j=0;j<iWinCount;j++)
					{
						if(0 == lTotalScore[refList.wWinerList[j]])
                            continue;

						if(j>0 && (lTotalScore[refList.wWinerList[j]]-lTotalScore[refList.wWinerList[j-1]]) == 0)
                            continue;

						//失败用户失分情况
						for(int k=i+1;k<GAME_PLAYER;k++)
						{
							//失败人数
							if(0 == WinnerList[k].bSameCount)
                                break;
							for(int l=0;l<WinnerList[k].bSameCount;l++)
							{
								//用户已赔空
								if(0 == lTotalScore[WinnerList[k].wWinerList[l]])
                                    continue;

								WORD  wLostId   = WinnerList[k].wWinerList[l];
								WORD  wWinId    = refList.wWinerList[j];
								int64 lMinScore = 0;

								//上家得分数目
								int64 lLastScore = (j>0) ? lTotalScore[refList.wWinerList[j-1]] : 0;
								//if(j>0)ASSERT(lLastScore>0);
                                lMinScore = min((lTotalScore[wWinId]-lLastScore),lTotalScore[wLostId]);
								for(int m=j;m<iWinCount;m++)
								{
									//得分数目
									lUserScore[refList.wWinerList[m]] += lMinScore/(iWinCount-j);

								}
								//赔偿数目
								lUserScore[wLostId]  -= lMinScore;
								lTotalScore[wLostId] -= lMinScore;
							}
						}
					}
				}
			}
            catch(...)
			{
				LOG_ERROR("得分数目/赔偿数目");
				ASSERT(FALSE);
			}
            LOG_DEBUG("下注及得分计算:");
            CCommonLogic::LogIntString(m_lTotalScore,GAME_PLAYER);
            CCommonLogic::LogIntString(lUserScore,GAME_PLAYER);
			int64 playerAllWinScore = 0; // add by har
			//统计用户分数(税收)
			for(WORD i=0;i<GAME_PLAYER;i++)
			{
				int64 fee_value = 0;
                if(m_cbPlayStatus[i] == TRUE){
					fee_value = CalcPlayerInfo(i,lUserScore[i]);
                }				
                assert((lUserScore[i]+m_lTotalScore[i]) >= 0);
				lUserScore[i] += fee_value;

                gameend.add_game_scores(lUserScore[i]);
                net::msg_cards* pCards = gameend.add_card_datas();
                for(uint8 j=0;j<MAX_COUNT;++j){
                    pCards->add_cards(m_cbHandCardData[i][j]);
                }
                net::texas_win_chair* pwin = gameend.add_win_chairs();
                for(uint16 k=0;k<WinnerList[i].bSameCount;++k){
                    pwin->add_win_chair(WinnerList[i].wWinerList[k]);
                }
				CGamePlayer *pPlayer = GetPlayer(i);
				if (pPlayer != NULL && !pPlayer->IsRobot())
				    playerAllWinScore += lUserScore[i]; // add by har
			}

			//更新幸运值数据   
			LOG_DEBUG("set current player lucky info. roomid:%d m_lucky_flag:%d m_set_ctrl_lucky_uid size:%d", GetRoomID(), m_lucky_flag, m_set_ctrl_lucky_uid.size());
			if (m_lucky_flag)
			{
				for (uint16 i = 0; i < GAME_PLAYER; ++i)
				{					
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL)
					{						
						auto iter = m_set_ctrl_lucky_uid.find(pGamePlayer->GetUID());
						if (iter != m_set_ctrl_lucky_uid.end())
						{							
							pGamePlayer->SetLuckyInfo(GetRoomID(), lUserScore[i]);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), lUserScore[i]);
						}
					}
				}
			}

            SendMsgToAll(&gameend,net::S2C_MSG_TEXAS_GAME_END);
            SendShowCardUser();
            WritePublicCardLog(m_cbCenterCardData,m_cbSendCardCount);
            SaveBlingLog();

            //更新活跃福利数据  
            for (WORD i = 0; i < GAME_PLAYER; i++)
            {
                CGamePlayer * pGamePlayer = GetPlayer(i);
                if (pGamePlayer != NULL && pGamePlayer->GetUID() == m_aw_ctrl_uid)
                {
                    int64 curr_win = lUserScore[i];
                    UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);
                }
            }

            m_coolLogic.beginCooling(s_RestTime + compareCount*1000);
			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,playerAllWinScore:%lld", GetRoomID(), GetTableID(), playerAllWinScore);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har    
			//结束游戏
            ResetTable();

            //SendSeatInfoToClient(); delete by har
			//OnTableGameEnd();
			return true;
		}
	case GER_NO_PLAYER:		//没有玩家
		{
            //LOG_DEBUG("没有玩家结束");
	        net::msg_texas_game_end_rep gameend;
            gameend.set_total_end(0);
            
			//效验结果
			WORD wUserCount=0;
			for(WORD i=0;i<GAME_PLAYER;i++){
				if(m_cbPlayStatus[i]!=FALSE)
                    wUserCount++;
			}
			if(wUserCount != 1){
				ASSERT(FALSE);
				//LOG_ERROR("没有玩家//效验结果出错");
			}

			//统计分数
			WORD wWinner = INVALID_CHAIR;
			int64 lAllScore = 0L;
			for(WORD i = 0;i<GAME_PLAYER;i++)
			{
                gameend.add_game_scores(0);
				if(m_cbPlayStatus[i] == FALSE)
				{
					if(m_lTotalScore[i] > 0L){
                       gameend.set_game_scores(i,-m_lTotalScore[i]);
					}
					continue;
				}
				wWinner = i;
								
				for(WORD j = 0;j<GAME_PLAYER;j++)
				{
					if(wWinner == j)
                        continue;
					lAllScore += m_lTotalScore[j];
				}
                int64 fee = CalcPlayerInfo(wWinner,lAllScore);
				lAllScore += fee;
                gameend.set_game_scores(i,lAllScore);
				//构造扑克
                for(uint16 j=0;j<GAME_PLAYER;++j){
                    net::msg_cards* pCards = gameend.add_card_datas();
                    for(uint8 k=0;k<MAX_COUNT;++k){
                        pCards->add_cards(m_cbHandCardData[j][k]);
                    }                    
                }
                LOG_DEBUG("下注及得分计算: roomid:%d,tableid:%d", GetRoomID(), GetTableID());
                CCommonLogic::LogIntString(m_lTotalScore,GAME_PLAYER);
                LOG_DEBUG("玩家赢的:%d--:%lld",wWinner,lAllScore);
			}

			//更新幸运值数据   
			LOG_DEBUG("set current player lucky info. roomid:%d m_lucky_flag:%d m_set_ctrl_lucky_uid size:%d", GetRoomID(), m_lucky_flag, m_set_ctrl_lucky_uid.size());
			int64 lucky_Score = 0L;
			if (m_lucky_flag)
			{
				for (uint16 i = 0; i < GAME_PLAYER; ++i)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL)
					{
						auto iter = m_set_ctrl_lucky_uid.find(pGamePlayer->GetUID());
						if (iter != m_set_ctrl_lucky_uid.end())
						{
							if (i == wWinner)
							{
								lucky_Score = lAllScore;
							}
							else
							{
								lucky_Score = -m_lTotalScore[i];
							}
							pGamePlayer->SetLuckyInfo(GetRoomID(), lucky_Score);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), lucky_Score);
						}
					}
				}
			}

			//发送消息
            SendMsgToAll(&gameend,net::S2C_MSG_TEXAS_GAME_END);
            SendShowCardUser();
            WritePublicCardLog(m_cbCenterCardData,m_cbSendCardCount);
            SaveBlingLog();

			//更新活跃福利数据  			
			CGamePlayer * pGamePlayer = GetPlayer(wWinner);
			if (pGamePlayer != NULL && pGamePlayer->GetUID() == m_aw_ctrl_uid)
			{				
				UpdateActiveWelfareInfo(m_aw_ctrl_uid, lAllScore);
			}			

			//结束游戏			
            ResetTable();

            m_coolLogic.beginCooling(s_RestTime);
            //SendSeatInfoToClient();  delete by har
			//OnTableGameEnd();
            return true;
		}
	case GER_USER_LEAVE:
	case GER_NETWORK_ERROR:
		{
			return true;
		}
	}
	return false;
}
//玩家进入或离开
void  CGameTexasTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    LOG_DEBUG("玩家进入桌子 - uid:%d,chairID:%d,isJoin:%d",pPlayer->GetUID(),chairID,isJoin);
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin)
	{
        SendGameScene(pPlayer);
    }
	else
	{
        m_mpBuyinScore.erase(pPlayer->GetUID());
		m_mpNextBuyinScore.erase(pPlayer->GetUID()); // add by har
		m_mpWaitBuyinScore.erase(pPlayer->GetUID()); // add by har
    }        
}

// 设置玩家座位信息 add by har
void CGameTexasTable::SetSeatInfo(net::seat_info *pSeat, CGamePlayer *pPlayer, int chairId) {
	pSeat->set_uid(pPlayer->GetUID());
	pSeat->set_name(pPlayer->GetPlayerName());
	pSeat->set_sex(pPlayer->GetSex());
	pSeat->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
	pSeat->set_score(pPlayer->GetAccountValue(emACC_VALUE_SCORE));
	pSeat->set_cvalue(pPlayer->GetAccountValue(emACC_VALUE_CVALUE));
	pSeat->set_head_icon(pPlayer->GetHeadIcon());
	pSeat->set_buyin(m_vecPlayers[chairId].buyinScore);
	pSeat->set_city(pPlayer->GetCity());
	pSeat->set_wincount(pPlayer->GetVecWin(GetGameType()));
	pSeat->set_betscore(pPlayer->GetVecBet(GetGameType()));
	pSeat->set_betcount(pPlayer->GetBetCount(GetGameType()));
}

// 发送座位信息给客户端 add by har
void CGameTexasTable::SendSeatInfoToClient(CGamePlayer* pGamePlayer) {
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
		return;
	net::msg_seat_info_rep msg;
	int validNum = 0;
	string strChairUID;
	// 座位玩家信息
	for (uint32 i = 0; i < m_vecPlayers.size(); ++i) {
		net::seat_info* pSeat = msg.add_players();
		pSeat->set_chairid(i);
		CGamePlayer *pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL) {
			/*pSeat->set_uid(pPlayer->GetUID());
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
			pSeat->set_betcount(pPlayer->GetBetCount(GetGameType()));*/
			SetSeatInfo(pSeat, pPlayer, i);
			++validNum;
			if (pPlayer->IsRobot())
				strChairUID += CStringUtility::FormatToString(" _chairid_%d_robotuid_%d", i, pPlayer->GetUID());
			else
				strChairUID += CStringUtility::FormatToString(" _chairid_%d_uid_%d", i, pPlayer->GetUID());
		} else {
			bool isSet = false;
			for (auto &ait : m_mpWaitBuyinScore)
				if (ait.second == i) {
					map<uint32, CGamePlayer*>::iterator it2 = m_mpLookers.find(ait.first);
					if (it2 != m_mpLookers.end()) {
						++validNum;
						CGamePlayer *pPlayer = it2->second;
						SetSeatInfo(pSeat, pPlayer, i);
						isSet = true;
						if (pPlayer->IsRobot())
							strChairUID += CStringUtility::FormatToString(" _waitchairid_%d_robotuid_%d", i, ait.first);
						else
							strChairUID += CStringUtility::FormatToString(" _waitchairid_%d_uid_%d", i, ait.first);
						break;
					}
				}
			if (!isSet) {
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
	}
	int uid = -1;
	if (pGamePlayer != NULL) {
		uid = pGamePlayer->GetUID();
		pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_SEATS_INFO);
	} else
		SendMsgToAll(&msg, net::S2C_MSG_SEATS_INFO);
	LOG_DEBUG("CGameTexasTable::SendSeatInfoToClient Send SeatInfo to Client - roomid:%d,tableid:%d,uid:%d,m_mpLookers.size:%d,validNum:%d,GameState:%d,m_wCurrentUser:%d,strChairUID:%s", 
		GetRoomID(), GetTableID(), uid, m_mpLookers.size(), validNum, GetGameState(), m_wCurrentUser, strChairUID.c_str());
}

// 发送场景信息(断线重连)
void  CGameTexasTable::SendGameScene(CGamePlayer* pPlayer)
{
    LOG_DEBUG("CGameTexasTable::SendGameScene  roomid:%d,tableid:%d,uid:%d,GameState:%d,m_wCurrentUser:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), GetGameState(), m_wCurrentUser);

	// add by har
	int32 buyin_state = -1; // 筹码状态
	unordered_map<uint32, int64>::iterator it = m_mpNextBuyinScore.find(pPlayer->GetUID());
	if (it != m_mpNextBuyinScore.end()) {
		if (it->second == 0)
			buyin_state = 10;
	} else {
		unordered_map<uint32, uint32>::iterator it_wait = m_mpWaitBuyinScore.find(pPlayer->GetUID());
		if (it_wait != m_mpWaitBuyinScore.end())
			buyin_state = 0;
	}
	// add by har end

    switch(m_gameState)
	{
	case net::TABLE_STATE_FREE://空闲状态
		{
			//构造数据
			net::msg_texas_game_info_free_rep msg;
			//游戏变量           
            msg.set_cell_maxscore(m_lCellScore);
            msg.set_cell_minscore(m_lCellScore);
			msg.set_buyin_state(buyin_state);  // add by har
			//发送场景
			pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TEXAS_GAME_FREE_INFO);
			return;	
		}break;
	case net::TABLE_STATE_PLAY:	//游戏状态
		{
			//构造数据
			net::msg_texas_game_info_play_rep msg;

			//总下注数目
			int64 lAllScore = 0L;
			for(WORD j = 0;j<GAME_PLAYER;j++)
			{
				lAllScore += m_lTotalScore[j];
				lAllScore -= m_lTableScore[j];
			}
			ASSERT(lAllScore>=0);

            msg.set_cell_score(m_lCellScore);
            msg.set_turn_max_score(m_lTurnMaxScore);
            msg.set_turn_less_score(m_lTurnLessScore);
            msg.set_add_less_score(m_lAddLessScore);
            msg.set_center_score(lAllScore);
            msg.set_cell_max_score(m_lCellScore);
            msg.set_balance_count(m_cbBalanceCount);

            msg.set_d_user(m_wDUser);
            msg.set_current_user(m_wCurrentUser);
            msg.set_min_chipin_user(m_wMinChipinUser);
            msg.set_max_chipin_user(m_wMaxChipinUser);

            msg.set_oper_time(m_coolLogic.getCoolTick());
            
            //加注信息
            for(int32 i=0;i<GAME_PLAYER;++i)
            {
                msg.add_table_scores(m_lTableScore[i]);
                msg.add_total_scores(m_lTotalScore[i]);
                msg.add_play_status(m_szCardState[i]);
            }
	
			//扑克信息
			uint16 wChairID = GetChairID(pPlayer);
            if(wChairID < GAME_PLAYER){
                for(int32 i=0;i<MAX_COUNT;++i){
                    msg.add_hand_cards(m_cbHandCardData[wChairID][i]);
                }
            }
			if(m_cbBalanceCount>0)
			{
                for(int16 i=0;i<m_cbSendCardCount;++i){
                    msg.add_center_cards(m_cbCenterCardData[i]);
                }				
			}
			for (WORD i = 0; i<GAME_PLAYER; i++)
			{
				if (m_cbPlayStatus[i] == TRUE)
				{
					msg.add_card_chairid(i);
				}
			}
			msg.set_buyin_state(buyin_state); // add by har
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TEXAS_GAME_PLAY_INFO);
			return;
		}break;
	}
	//效验结果
	ASSERT(FALSE);
	return;
}
int64    CGameTexasTable::CalcPlayerInfo(uint16 chairID,int64 winScore)
{
    if(winScore == 0)
        return 0;
    uint32 uid = m_vecPlayers[chairID].uid;
    
	int64 fee_value = CalcPlayerGameInfo(uid,winScore);
    int64 buyinScore = ChangeBuyinScore(uid,winScore);
	LOG_DEBUG("report game to lobby   roomid:%d,tableid:%d,uid:%d,chairID:%d,winScore:%lld,fee_value:%d,buyinScore:%d", 
		GetRoomID(), GetTableID(), uid, chairID, winScore, fee_value, buyinScore);

    // 修改德州数据
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        uint8 curMaxCard[5];
        pPlayer->GetGameMaxCard(net::GAME_CATE_TEXAS,isCoin,curMaxCard,5);
        BYTE cbUserCardData[GAME_PLAYER][MAX_CARD_COUNT];
        memcpy(cbUserCardData,m_cbMaxCardData,sizeof(cbUserCardData));
        m_GameLogic.SortCardList(cbUserCardData[chairID],MAX_CARD_COUNT);
        if(m_GameLogic.CompareCard(cbUserCardData[chairID],curMaxCard,MAX_CARD_COUNT) == 2){
            pPlayer->AsyncSetGameMaxCard(net::GAME_CATE_TEXAS,isCoin,cbUserCardData[chairID],MAX_CARD_COUNT);            
        }
    }
	return fee_value;
}
int64   CGameTexasTable::GetSitDownScore()
{
    if(GetTableType() == emTABLE_TYPE_PLAYER){
        return GetBaseScore()*100;
    }
    return m_pHostRoom->GetSitDown();
}
// 重置游戏数据
void    CGameTexasTable::ResetGameData()
{
  	//玩家变量
	m_wCurrentUser = INVALID_CHAIR;

	//玩家状态
	memset(m_cbPlayStatus,0,sizeof(m_cbPlayStatus));
    memset(m_szCardState,0,sizeof(m_szCardState));

	//扑克变量
	m_cbSendCardCount = 0;
	memset(m_cbCenterCardData,0,sizeof(m_cbCenterCardData));
	//memset(m_cbHandCardData,0,sizeof(m_cbHandCardData));

	//加注变量
	m_lCellScore = 0L;
	m_lTurnLessScore = 0L;
	m_lTurnMaxScore = 0L;
	m_lAddLessScore = 0L;
	m_wOperaCount = 0;
	m_cbBalanceCount = 0;
	m_lBalanceScore = 0L;
	memset(m_lTableScore,0,sizeof(m_lTableScore));
    memset(m_lRoundScore,0,sizeof(m_lRoundScore));
	memset(m_lTotalScore,0,sizeof(m_lTotalScore));
	memset(m_cbShowHand,0,sizeof(m_cbShowHand));
    memset(m_cbMaxCardData,0,sizeof(m_cbMaxCardData));
    ZeroMemory(m_szShowCard,sizeof(m_szShowCard));
	m_bIsControlPlayer = false;
}

CGamePlayer* CGameTexasTable::HaveWelfareNovicePlayer()
{
	int count = 0;
	CGamePlayer * pTempPlayer = NULL;
	for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
	{
		if (m_cbPlayStatus[wChairID] == FALSE)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(wChairID);
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
			return NULL;
		}
		else
		{
			pTempPlayer = pPlayer;
			count++;
		}
	}
	if (count == 1)
	{
		return pTempPlayer;
	}
	return NULL;
}

bool CGameTexasTable::NoviceWelfareCtrlWinScore()
{
	uint32 noviceuid = 0;
	uint32 posrmb = 0;
	struct tagUserNewPlayerWelfareValue tagValue;
	CGamePlayer * pPlayer = HaveWelfareNovicePlayer();

	if (pPlayer != NULL)
	{
		noviceuid = pPlayer->GetUID();
		posrmb = pPlayer->GetPosRmb();
		tagValue = pPlayer->GetWelfareValue();
	}
	int isnewnowe = 0;
	int64 newmaxjetton = 0;
	int64 newsmaxwin = 0;
	if (m_pHostRoom != NULL)
	{
		isnewnowe = m_pHostRoom->GetNoviceWelfareOwe();
		tagRangeWelfare tempTagRange = m_pHostRoom->GetRangeWelfareByPosRmb(noviceuid, posrmb);
		newmaxjetton = tempTagRange.smaxjetton;
		newsmaxwin = tempTagRange.smaxwin;
	}

	bool bIsNoviceWelfareCtrl = false;
	int64 lNoviceWinScore = 0;
	tagNewPlayerWelfareValue NewPlayerWelfareValue;
	bool bIsHitWelfarePro = false;

	uint32 real_welfarepro = 0;
	int fUseHitWelfare = 1;
	if (isnewnowe == 1 && pPlayer != NULL && noviceuid != 0)
	{
		NewPlayerWelfareValue = CDataCfgMgr::Instance().GetNewPlayerWelfareValue(noviceuid, posrmb);
		struct tagUserNewPlayerWelfareValue & tagTempValue = pPlayer->GetWelfareValue();

		fUseHitWelfare = tagTempValue.frontIsHitWelfare;
		if (fUseHitWelfare == 0 && tagTempValue.jettonCount > 0)
		{
			real_welfarepro = NewPlayerWelfareValue.welfarepro + (tagTempValue.jettonCount * NewPlayerWelfareValue.lift_odds);
		}
		else
		{
			real_welfarepro = NewPlayerWelfareValue.welfarepro;
		}

		if (real_welfarepro > PRO_DENO_10000)
		{
			real_welfarepro = PRO_DENO_10000;
		}

		bIsHitWelfarePro = g_RandGen.RandRatio(real_welfarepro, PRO_DENO_10000);

		if (bIsHitWelfarePro)
		{
			bool bRetPlayerWinScore = SetControlNoviceWelfarePalyerWin(noviceuid);
			if (bRetPlayerWinScore)
			{
				bIsNoviceWelfareCtrl = true;
				SetChessWelfare(1);
			}
			else
			{
				bIsNoviceWelfareCtrl = false;
			}
		}
		else
		{
			bIsNoviceWelfareCtrl = false;
		}

		if (bIsNoviceWelfareCtrl)
		{
			tagTempValue.frontIsHitWelfare = 1;
			tagTempValue.jettonCount = 0;
		}
		else
		{
			tagTempValue.jettonCount++;
			tagTempValue.frontIsHitWelfare = 0;
		}
	}

	LOG_DEBUG("dos_wel_ctrl - roomid:%d,tableid:%d,isnewnowe:%d, newmaxjetton:%lld, newsmaxwin:%lld,lNoviceWinScore:%lld, IsNoviceWelfareCtrl:%d, noviceuid:%d,posrmb:%d, ChessWelfare:%d,welfarepro:%d,real_welfarepro:%d,lift_odds:%d,pPlayer:%p, fUseHitWelfare:%d,frontIsHitWelfare:%d,jettonCount:%d,bIsHitWelfarePro:%d",
		GetRoomID(), GetTableID(), isnewnowe, newmaxjetton, newsmaxwin, lNoviceWinScore, bIsNoviceWelfareCtrl, noviceuid, posrmb, GetChessWelfare(), NewPlayerWelfareValue.welfarepro, real_welfarepro, NewPlayerWelfareValue.lift_odds, pPlayer, fUseHitWelfare, tagValue.frontIsHitWelfare, tagValue.jettonCount, bIsHitWelfarePro);

	return bIsNoviceWelfareCtrl;
}


void    CGameTexasTable::SetCardDataControl()
{
	bool bIsFalgControlMulti = false;

	bIsFalgControlMulti = ProgressControlPalyer();

	m_bIsControlPlayer = bIsFalgControlMulti;

	bool bIsFalgControl = false;
	bool bIsControlPlayerIsReady = false;

	bool bIsWinMaxScorePlayerLost = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	bool bIsNoviceWelfareCtrl = false;
	bool bIsLuckyCtrl = false;
    bool bIsAWCtrl = false;
	bool bSetRobotMaxCardType = false;
	bool bStockCtrl = false;

	if (!bIsFalgControlMulti)
	{
		if (control_uid != 0 && game_count > 0 && control_type != GAME_CONTROL_CANCEL)
		{
			for (WORD i = 0; i < GAME_PLAYER; i++)
			{
				CGamePlayer *pPlayer = GetPlayer(i);
				if (pPlayer == NULL)
					continue;
				if (m_cbPlayStatus[i] == FALSE)
				{
					continue;
				}
				if (control_uid == pPlayer->GetUID())
				{
					bIsControlPlayerIsReady = true;
					break;
				}
			}
		}

		if (bIsControlPlayerIsReady && game_count > 0 && control_type != GAME_CONTROL_CANCEL)
		{
			if (control_type == GAME_CONTROL_WIN)
			{
				bIsFalgControl = SetControlPalyerWin(control_uid);
			}
			if (control_type == GAME_CONTROL_LOST)
			{
				bIsFalgControl = SetControlPalyerLost(control_uid);
			}
			if (bIsFalgControl && m_tagControlPalyer.count > 0)
			{
				//m_tagControlPalyer.count--;
				//if (m_tagControlPalyer.count == 0)
				//{
				//	m_tagControlPalyer.Init();
				//}
				if (m_pHostRoom != NULL)
				{
					m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
				}
			}
			m_bIsControlPlayer = bIsFalgControl;
		}

		// 幸运值福利		
		if (!bIsFalgControl)
		{
			bIsLuckyCtrl = SetLuckyCtrl();
		}

		// 新用户福利
		if (!bIsFalgControl && !bIsLuckyCtrl)
		{
			bIsNoviceWelfareCtrl = NoviceWelfareCtrlWinScore();
		}

		if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl)
		{
			//玩家最多能赢
			bIsWinMaxScorePlayerLost = SetWinMaxScorePlayerLost();
		}

		if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsWinMaxScorePlayerLost)
			bStockCtrl = SetStockWinLose();
		
		if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsWinMaxScorePlayerLost && !bStockCtrl)
		{
			//bSetRobotMaxCardType = true;
			bSetRobotMaxCardType = SetRobotMaxCardType();
			if (!bSetRobotMaxCardType)
				bIsAWCtrl = ActiveWelfareCtrl();
		}

	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("robot win - roomid:%d,tableid:%d,m_robotBankerMaxCardPro:%d,bIsControlPlayerIsReady:%d,control_uid:%d,control_type:%d,game_count:%d,bIsFalgControl:%d,bIsLuckyCtrl:%d,bIsWinMaxScorePlayerLost:%d,bIsFalgControlMulti:%d,bIsAWCtrl:%d,bIsNoviceWelfareCtrl:%d,bSetRobotMaxCardType:%d,bStockCtrl:%d,GetIsAllRobotOrPlayerJetton:%d",
		GetRoomID(), GetTableID(), m_robotBankerMaxCardPro, bIsControlPlayerIsReady, control_uid, control_type, game_count, bIsFalgControl, bIsLuckyCtrl, bIsWinMaxScorePlayerLost, bIsFalgControlMulti, bIsAWCtrl, bIsNoviceWelfareCtrl, bSetRobotMaxCardType, bStockCtrl, GetIsAllRobotOrPlayerJetton());

}

bool	CGameTexasTable::ProgressControlPalyer()
{
	bool bIsFalgControlMulti = false;
	vector<tagControlMultiPalyer> vecControlMultiPlayerLost;
	vector<tagControlMultiPalyer> vecControlMultiPlayerWin;
	for (uint32 uIndex = 0; uIndex < GAME_PLAYER; uIndex++)
	{
		CGamePlayer *pPlayer = GetPlayer(uIndex);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_cbPlayStatus[uIndex] == FALSE)
		{
			continue;
		}
		auto it_player = m_mpControlMultiPalyer.find(pPlayer->GetUID());
		if (it_player != m_mpControlMultiPalyer.end())
		{
			tagControlMultiPalyer ControlMultiPalyer = it_player->second;
			ControlMultiPalyer.chairID = uIndex;
			if (ControlMultiPalyer.type == GAME_CONTROL_MULTI_LOST)
			{
				vecControlMultiPlayerLost.push_back(ControlMultiPalyer);
			}
			else if (ControlMultiPalyer.type == GAME_CONTROL_MULTI_WIN)
			{
				vecControlMultiPlayerWin.push_back(ControlMultiPalyer);
			}
			else
			{
				// error
			}
		}
	}
	
	uint32 uLostPlayerUid = 0;
	if (vecControlMultiPlayerLost.size() > 0)
	{
		int64 i64MaxProfits = 0;
		bool bIsFirst = true;

		for (uint32 uLostIndex = 0; uLostIndex < vecControlMultiPlayerLost.size(); uLostIndex++)
		{
			CGamePlayer *pPlayer = GetPlayer(vecControlMultiPlayerLost[uLostIndex].chairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (bIsFirst)
			{
				i64MaxProfits = pPlayer->GetProfits();
				uLostPlayerUid = pPlayer->GetUID();
				bIsFirst = false;
			}
			else
			{
				if (pPlayer->GetProfits() > i64MaxProfits)
				{
					i64MaxProfits = pPlayer->GetProfits();
					uLostPlayerUid = pPlayer->GetUID();
				}
			}
		}
		if (uLostPlayerUid != 0)
		{
			bIsFalgControlMulti = SetControlPalyerLost(uLostPlayerUid);
		}
	}

	uint32 uWinPlayerUid = 0;
	if (!bIsFalgControlMulti)
	{
		if (vecControlMultiPlayerWin.size() > 0)
		{
			int64 i64MaxProfits = 0;
			bool bIsFirst = true;

			for (uint32 uWinIndex = 0; uWinIndex < vecControlMultiPlayerWin.size(); uWinIndex++)
			{
				CGamePlayer *pPlayer = GetPlayer(vecControlMultiPlayerWin[uWinIndex].chairID);
				if (pPlayer == NULL)
				{
					continue;
				}
				if (bIsFirst)
				{
					i64MaxProfits = pPlayer->GetProfits();
					uWinPlayerUid = pPlayer->GetUID();
					bIsFirst = false;
				}
				else
				{
					if (i64MaxProfits > pPlayer->GetProfits())
					{
						i64MaxProfits = pPlayer->GetProfits();
						uWinPlayerUid = pPlayer->GetUID();
					}
				}
			}
			if (uWinPlayerUid != 0)
			{
				bIsFalgControlMulti = SetControlPalyerWin(uWinPlayerUid);
			}
		}
	}

	LOG_DEBUG("roomid:%d,tableid:%d,vecControlMultiPlayer_size:%d %d,uLostPlayerUid:%d,uWinPlayerUid:%d,bIsFalgControlMulti:%d",
		m_pHostRoom->GetRoomID(),GetTableID(),vecControlMultiPlayerLost.size(), vecControlMultiPlayerWin.size(), uLostPlayerUid, uWinPlayerUid, bIsFalgControlMulti);
	return bIsFalgControlMulti;
}

bool    CGameTexasTable::SetWinMaxScorePlayerLost()
{
	bool bIsHaveRobot = false;
	for (uint8 i = 0; i < GAME_PLAYER; i++)
	{
		CGamePlayer *pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
			continue;
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		if (pPlayer->IsRobot())
		{
			bIsHaveRobot = true;
		}
	}
	if (!bIsHaveRobot)
	{
		return true;
	}

	bool bIsHaveWinExceedMaxScore = false;
	int64 lPlayerMaxWinScore = 0;
	uint16 roomid = 0;
	uint16 gametype = net::GAME_CATE_MAX_TYPE;
	if (m_pHostRoom != NULL)
	{
		lPlayerMaxWinScore = m_pHostRoom->GetPlayerMaxWinScore();
		roomid = m_pHostRoom->GetRoomID();
		gametype = m_pHostRoom->GetGameType();
	}
	uint8 uMinLevel = 0;
	for (uint8 i = 0; i<GAME_PLAYER; i++)
	{
		CGamePlayer *pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
			continue;
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		if (pPlayer->IsRobot())
		{
			continue;
		}
		int64 lPlayerTotalWinScore = pPlayer->GetPlayerTotalWinScore(gametype);
		if (lPlayerTotalWinScore >= lPlayerMaxWinScore)
		{
			SetPlayerCardSortIndex(pPlayer->GetUID(),i, uMinLevel);
			uMinLevel++;
			bIsHaveWinExceedMaxScore = true;
		}
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,lPlayerTotalWinScore:%lld,lPlayerMaxWinScore:%lld,uMinLevel:%d,gametype:%d", roomid,GetTableID(), pPlayer->GetUID(), lPlayerTotalWinScore, lPlayerMaxWinScore, uMinLevel, gametype);
	}

	return bIsHaveWinExceedMaxScore;
}

bool CGameTexasTable::SetPlayerCardSortIndex(uint32 uid,uint8 uArSortIndex,uint8 uSortLevel)
{
	uint16 roomid = 0;
	if (m_pHostRoom != NULL)
	{
		roomid = m_pHostRoom->GetRoomID();
	}


	BYTE cbTableCard[GAME_PLAYER][MAX_CARD_COUNT] = { { 0 } };

	memcpy(cbTableCard, m_cbMaxCardData, sizeof(m_cbMaxCardData));

	uint8 uArSortCardIndex[GAME_PLAYER] = { 0,1,2,3,4,5,6,7,8 };

	for (uint8 i = 0; i < GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		for (uint8 j = 0; j < GAME_PLAYER; j++)
		{
			if (i == j || m_cbPlayStatus[j] == FALSE)
			{
				continue;
			}
			bool bIsSwapCardIndex = (m_GameLogic.CompareCard(cbTableCard[i], cbTableCard[j], MAX_CARD_COUNT) == 1);
			if (bIsSwapCardIndex)
			{
				uint8 tmp[MAX_CARD_COUNT];
				memcpy(tmp, cbTableCard[j], MAX_CARD_COUNT);
				memcpy(cbTableCard[j], cbTableCard[i], MAX_CARD_COUNT);
				memcpy(cbTableCard[i], tmp, MAX_CARD_COUNT);

				uint8 uTempIndex = uArSortCardIndex[i];
				uArSortCardIndex[i] = uArSortCardIndex[j];
				uArSortCardIndex[j] = uTempIndex;
			}
		}
	}
	//cbTableCard 按照有牌数据的顺序排序好了
	uint8  cbCountIndex = 0;
	uint8 cbSwpIndex = 0;
	for (; cbSwpIndex < GAME_PLAYER; cbSwpIndex++)
	{
		if (m_cbPlayStatus[cbSwpIndex] == TRUE)
		{
			if (cbCountIndex == uSortLevel)
			{
				break;
			}
			cbCountIndex++;
		}
	}
	LOG_DEBUG("-----------------------------------------------------------------------------------------");
	for (uint8 i = 0; i < GAME_PLAYER; i++)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,uArSortCardIndex:%d,m_cbEndCardType:%d,m_cbHandCardData[0X%02x 0X%02x], m_cbMaxCardData[0X%02x 0X%02x 0X%02x 0X%02x 0X%02x],cbTableCard[0X%02x 0X%02x 0X%02x 0X%02x 0X%02x]", roomid, GetTableID(),uid, i, uArSortCardIndex[i], m_cbEndCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbMaxCardData[i][0], m_cbMaxCardData[i][1], m_cbMaxCardData[i][2], m_cbMaxCardData[i][3], m_cbMaxCardData[i][4], m_cbHandCardData[i][0], m_cbHandCardData[i][1], cbTableCard[i][0], cbTableCard[i][1], cbTableCard[i][2], cbTableCard[i][3], cbTableCard[i][4]);

	}

	uint8 uChairIndex = uArSortCardIndex[cbSwpIndex];
	if (uArSortIndex != uChairIndex)
	{
		uint8 tmp[MAX_COUNT];
		memcpy(tmp, m_cbHandCardData[uChairIndex], MAX_COUNT);
		memcpy(m_cbHandCardData[uChairIndex], m_cbHandCardData[uArSortIndex], MAX_COUNT);
		memcpy(m_cbHandCardData[uArSortIndex], tmp, MAX_COUNT);

		BYTE cbTempMaxCardData[MAX_CARD_COUNT];
		memcpy(cbTempMaxCardData, m_cbMaxCardData[uChairIndex], MAX_CARD_COUNT);
		memcpy(m_cbMaxCardData[uChairIndex], m_cbMaxCardData[uArSortIndex], MAX_CARD_COUNT);
		memcpy(m_cbMaxCardData[uArSortIndex], cbTempMaxCardData, MAX_CARD_COUNT);

		BYTE cbTempEndCardType = m_cbEndCardType[uChairIndex];
		m_cbEndCardType[uChairIndex] = m_cbEndCardType[uArSortIndex];
		m_cbEndCardType[uArSortIndex] = cbTempEndCardType;
	}

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,uArSortIndex:%d,uSortLevel:%d,uChairIndex:%d,cbSwpIndex:%d",roomid,GetTableID(),uid, uArSortIndex, uSortLevel, uChairIndex, cbSwpIndex);

	for (uint8 i = 0; i < GAME_PLAYER; i++)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,uArSortIndex:%d,uSortLevel:%d,i:%d,uArSortCardIndex:%d,m_cbEndCardType:%d,m_cbHandCardData[0X%02x 0X%02x], m_cbMaxCardData[0X%02x 0X%02x 0X%02x 0X%02x 0X%02x],", roomid, GetTableID(),uid, uArSortIndex, uSortLevel, i,uArSortCardIndex[i], m_cbEndCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbMaxCardData[i][0], m_cbMaxCardData[i][1], m_cbMaxCardData[i][2], m_cbMaxCardData[i][3], m_cbMaxCardData[i][4] );

	}


	return true;
}


// 做牌发牌
void    CGameTexasTable::DispatchCard(uint16 wUserCount) {
	int maxCardType = 0;
	int iRandNum = g_RandGen.RandRange(0, m_confArrDispatchCardAllPro);
	int randIndex = iRandNum;
	for (int iProIndex = CT_TONG_HUA_SHUN; iProIndex > CT_ONE_LONG; --iProIndex) {
		if (iRandNum <= m_iArrDispatchCardPro[iProIndex]) {
			maxCardType = iProIndex;
			break;
		} else {
			iRandNum -= m_iArrDispatchCardPro[iProIndex];
		}
	}

	int playNum = 0;
	int curMaxCardType = 0; // 最大牌型
	int maxCardTypeNum = 0; // 最大牌型数量
	int maxCardIndex = -1, secondCardIndex = -1; // 第一和第二大牌索引
	int c = 0;
	uint64 uSysTime1 = getTickCount64();
	for (; c < PRO_DENO_10000; ++c) {
		//混乱扑克
		BYTE cbRandCard[FULL_COUNT];
		memset(cbRandCard, 0, sizeof(cbRandCard));
		m_GameLogic.RandCardList(cbRandCard, wUserCount*MAX_COUNT + MAX_CENTERCOUNT);

		//用户扑克
		WORD wCardCount = 0;
		for (WORD i = 0; i < GAME_PLAYER; i++)
			if (m_cbPlayStatus[i] == TRUE)
				memcpy(&m_cbHandCardData[i], &cbRandCard[(wCardCount++)*MAX_COUNT], sizeof(BYTE)*MAX_COUNT);
		//中心扑克
		memcpy(m_cbCenterCardData, &cbRandCard[wUserCount*MAX_COUNT], getArrayLen(m_cbCenterCardData));

		playNum = 0;
		// 满足最大的牌型只有1个且必须等于cardType
		curMaxCardType = 0; // 最大牌型
		maxCardTypeNum = 0; // 最大牌型数量
		uint8 szSortCards[GAME_PLAYER][MAX_CARD_COUNT]; // 排序好的最大牌型

		for (uint16 i = 0; i < GAME_PLAYER; ++i) {
			if (m_cbPlayStatus[i] == FALSE)
				continue;
			++playNum;
			uint8 cbEndCardKind = m_GameLogic.FiveFromSeven(m_cbHandCardData[i], MAX_COUNT, m_cbCenterCardData, MAX_CENTERCOUNT, m_cbMaxCardData[i], MAX_CENTERCOUNT);
			m_cbEndCardType[i] = cbEndCardKind;
			if (cbEndCardKind > curMaxCardType) {
				curMaxCardType = cbEndCardKind;
				maxCardTypeNum = 1;
				ZeroMemory(szSortCards, sizeof(szSortCards));
				memcpy(szSortCards[0], m_cbMaxCardData[i], MAX_CARD_COUNT);
			} else if (curMaxCardType == cbEndCardKind) {
				memcpy(szSortCards[maxCardTypeNum], m_cbMaxCardData[i], MAX_CARD_COUNT);
				++maxCardTypeNum;
			}
		}

		if (maxCardType == 0)
			break;

		if (curMaxCardType != maxCardType)
			if (!(maxCardType == CT_TONG_HUA_SHUN && curMaxCardType == CT_KING_TONG_HUA_SHUN))
				continue;

		if (maxCardTypeNum == 1)
			break;

		if (maxCardTypeNum == 0)
			continue;
		
		// 排序
		if (m_GameLogic.CompareCard(szSortCards[0], szSortCards[1], MAX_CARD_COUNT) == 2) {
			maxCardIndex = 0;
			secondCardIndex = 1;
		} else {
			maxCardIndex = 1;
			secondCardIndex = 0;
		}
		for (int i = 2; i < maxCardTypeNum; ++i) {
			uint8 ret = m_GameLogic.CompareCard(szSortCards[i], szSortCards[maxCardIndex], MAX_CARD_COUNT);
			if (ret == 2) {
				secondCardIndex = maxCardIndex;
				maxCardIndex = i;
			} else if (ret == 1) {
				secondCardIndex = i;
			}
		}

		if (m_GameLogic.CompareCard(szSortCards[maxCardIndex], szSortCards[secondCardIndex], MAX_CARD_COUNT) == 2)
			break;
	}
	uint64 changeTime = getTickCount64() - uSysTime1;
	LOG_DEBUG("CGameTexasTable::DispatchCard roomid:%d,tableid:%d,playNum:%d,maxCardType:%d,curMaxCardType:%d,maxCardTypeNum:%d,randIndex:%d,c:%d,changeTime:%dms,maxCardIndex:%d,secondCardIndex:%d,m_cbEndCardType:%d-%d-%d-%d-%d-%d-%d-%d-%d",
		GetRoomID(), GetTableID(), playNum, maxCardType, curMaxCardType, maxCardTypeNum, randIndex, c, changeTime, maxCardIndex, secondCardIndex,
		m_cbEndCardType[0], m_cbEndCardType[1], m_cbEndCardType[2], m_cbEndCardType[3], m_cbEndCardType[4], m_cbEndCardType[5], m_cbEndCardType[6], m_cbEndCardType[7], m_cbEndCardType[8]);

	//ProbabilityDispatchPokerCard();



    return;

	/*if(GetOnlinePlayerNum() == 0){  delete by har
        return;                 
	}else{		
		uint16 maxNum = g_RandGen.RandRange(100,100) ? 2 : 9;		
		for(uint32 m=0;m<1000;++m)
		{
            memset(cbRandCard,0,sizeof(cbRandCard));
        	m_GameLogic.RandCardList(cbRandCard,wUserCount*MAX_COUNT+MAX_CENTERCOUNT);            
        	//用户扑克
        	WORD wCardCount=0;
        	for(WORD i=0;i<GAME_PLAYER;i++){
        		if(m_cbPlayStatus[i]==TRUE){
        			memcpy(&m_cbHandCardData[i],&cbRandCard[(wCardCount++)*MAX_COUNT],sizeof(BYTE)*MAX_COUNT);
        		}
        	} 
            //中心扑克
	        memcpy(m_cbCenterCardData,&cbRandCard[wUserCount*MAX_COUNT],getArrayLen(m_cbCenterCardData));
            
			uint16 cardNum = 0;
			for(uint16 j=0;j<GAME_PLAYER;++j)
			{
                if(m_cbPlayStatus[j] == FALSE)
                    continue;                
			    if(m_GameLogic.CanPlayFirstCardValue(m_cbHandCardData[j],MAX_COUNT) == false){
					cardNum++;
				}
				if(cardNum > maxNum)
					break;
			}
            if(cardNum > maxNum){
    			continue;//手牌不合标准，继续洗
            }
            //前三张两家大于对子
            cardNum = 0;
			for(uint16 j=0;j<GAME_PLAYER;++j)
			{
                if(m_cbPlayStatus[j] == FALSE)
                    continue;                
                uint8 tmp[MAX_CENTERCOUNT];
                memcpy(tmp,m_cbHandCardData[j],MAX_COUNT);
                memcpy(&tmp[2],m_cbCenterCardData,3);
                //排列扑克
	            m_GameLogic.SortCardList(tmp,5);
			    if(m_GameLogic.GetCardType(tmp,5) >= CT_ONE_LONG){
					cardNum++;
				}
				if(cardNum > 2)
					break;
			}
            if(cardNum > 2){
    			break;
            }                          
		}		
	}*/
}

void CGameTexasTable::InitSubWelfareHandCard()
{
	m_mpSubWelfareHandCard.clear();
	std::vector<BYTE> vecSubCardData_one;
	for (BYTE i = 2; i <= 8; i++)
	{
		vecSubCardData_one.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x01, vecSubCardData_one));

	std::vector<BYTE> vecSubCardData_three;
	for (BYTE i = 2; i <= 2; i++)
	{
		vecSubCardData_three.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x03, vecSubCardData_three));

	std::vector<BYTE> vecSubCardData_four;
	for (BYTE i = 2; i <= 3; i++)
	{
		vecSubCardData_four.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x04, vecSubCardData_four));

	std::vector<BYTE> vecSubCardData_five;
	for (BYTE i = 2; i <= 4; i++)
	{
		vecSubCardData_five.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x05, vecSubCardData_five));

	std::vector<BYTE> vecSubCardData_six;
	for (BYTE i = 2; i <= 5; i++)
	{
		vecSubCardData_six.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x06, vecSubCardData_six));

	std::vector<BYTE> vecSubCardData_seven;
	for (BYTE i = 2; i <= 6; i++)
	{
		vecSubCardData_seven.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x07, vecSubCardData_seven));

	std::vector<BYTE> vecSubCardData_eight;
	for (BYTE i = 2; i <= 7; i++)
	{
		vecSubCardData_eight.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x08, vecSubCardData_eight));

	std::vector<BYTE> vecSubCardData_nine;
	for (BYTE i = 2; i <= 8; i++)
	{
		vecSubCardData_nine.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x09, vecSubCardData_nine));

	std::vector<BYTE> vecSubCardData_ten;
	for (BYTE i = 2; i <= 9; i++)
	{
		vecSubCardData_ten.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x0A, vecSubCardData_ten));

	std::vector<BYTE> vecSubCardData_eleven;
	for (BYTE i = 2; i <= 9; i++)
	{
		vecSubCardData_eleven.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x0B, vecSubCardData_eleven));

	std::vector<BYTE> vecSubCardData_twelve;
	for (BYTE i = 2; i <= 9; i++)
	{
		vecSubCardData_twelve.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x0C, vecSubCardData_twelve));

	std::vector<BYTE> vecSubCardData_thirteen;
	for (BYTE i = 2; i <= 9; i++)
	{
		vecSubCardData_thirteen.push_back(i);
	}
	m_mpSubWelfareHandCard.insert(std::make_pair(0x0D, vecSubCardData_thirteen));
}

bool	CGameTexasTable::SetRobotMaxCardType()
{
	uint32 robotBankerMaxCardPro = m_robotBankerMaxCardPro;
	bool bIsMaxCard = g_RandGen.RandRatio(robotBankerMaxCardPro, PRO_DENO_10000);
	if (!bIsMaxCard)
	{
		return false;
	}
	uint16 maxChairID = INVALID_CHAIR;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		if (maxChairID == INVALID_CHAIR)
		{
			maxChairID = i;
			continue;
		}
		if (m_GameLogic.CompareCard(m_cbMaxCardData[i], m_cbMaxCardData[maxChairID], MAX_CARD_COUNT) == 2)
		{
			maxChairID = i;
		}
	}
	if (maxChairID == INVALID_CHAIR)
	{
		return false;
	}

	CGamePlayer* pGamePlayer = GetPlayer(maxChairID);
	if (pGamePlayer != NULL && pGamePlayer->IsRobot()) {
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pRobot = GetPlayer(i);
		if (pRobot != NULL && pRobot->IsRobot()) {
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);

			BYTE cbTempMaxCardData[MAX_CARD_COUNT];
			memcpy(cbTempMaxCardData, m_cbMaxCardData[i], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[i], m_cbMaxCardData[maxChairID], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[maxChairID], cbTempMaxCardData, MAX_CARD_COUNT);

			BYTE cbTempEndCardType = m_cbEndCardType[i];
			m_cbEndCardType[i] = m_cbEndCardType[maxChairID];
			m_cbEndCardType[maxChairID] = cbTempEndCardType;

			return true;
		}
	}
	return false;
}

bool	CGameTexasTable::SetRobotMinCardType()
{
	uint16 minChairID = INVALID_CHAIR;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		if (minChairID == INVALID_CHAIR)
		{
			minChairID = i;
			continue;
		}
		if (m_GameLogic.CompareCard(m_cbMaxCardData[i], m_cbMaxCardData[minChairID], MAX_CARD_COUNT) == 1)
		{
			minChairID = i;
		}
	}
	if (minChairID == INVALID_CHAIR)
	{
		return false;
	}

	CGamePlayer* pGamePlayer = GetPlayer(minChairID);
	if (pGamePlayer != NULL && pGamePlayer->IsRobot()) {
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pRobot = GetPlayer(i);
		if (pRobot != NULL && pRobot->IsRobot()) {
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);
			return true;
		}
	}
	return false;
}

bool	CGameTexasTable::SetControlNoviceWelfarePalyerWin(uint32 control_uid)
{
	uint16 uCtrlChairID = 255;
	uint16 wDispatchCardUserCount = 0;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		else
		{
			wDispatchCardUserCount++;
		}
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (control_uid == pPlayer->GetUID())
		{
			uCtrlChairID = i;
		}
	}
	string strTagInfo;
	bool bRetPlayerWinScore = false;
	int iLoopIndex = 0;
	if (uCtrlChairID < GAME_PLAYER)
	{
		for (; iLoopIndex < 1024; iLoopIndex++)
		{
			strTagInfo.clear();
			bool bIsAgainDispatchCard = false;
			bRetPlayerWinScore = SetControlPalyerWin(control_uid);
			if (bRetPlayerWinScore)
			{
				BYTE cbCardDataOne = m_cbHandCardData[uCtrlChairID][0];
				BYTE cbCardDataTwo = m_cbHandCardData[uCtrlChairID][1];

				BYTE cbOneValue = m_GameLogic.GetCardValue(cbCardDataOne);
				BYTE cbTwoValue = m_GameLogic.GetCardValue(cbCardDataTwo);

				BYTE cbOneColor = m_GameLogic.GetCardColor(cbCardDataOne);
				BYTE cbTwoColor = m_GameLogic.GetCardColor(cbCardDataTwo);


				if (cbOneColor != cbTwoColor)
				{
					auto iter_find = m_mpSubWelfareHandCard.find(cbOneValue);
					if (iter_find != m_mpSubWelfareHandCard.end())
					{
						auto & vecTempCardData = iter_find->second;
						auto iter_two = find(vecTempCardData.begin(), vecTempCardData.end(), cbTwoValue);
						if (iter_two != vecTempCardData.end())
						{
							bIsAgainDispatchCard = true;
						}
					}
				}


				if (bIsAgainDispatchCard == false)
				{
					if (cbOneColor != cbTwoColor)
					{
						auto iter_find = m_mpSubWelfareHandCard.find(cbTwoValue);
						if (iter_find != m_mpSubWelfareHandCard.end())
						{
							auto & vecTempCardData = iter_find->second;
							auto iter_two = find(vecTempCardData.begin(), vecTempCardData.end(), cbOneValue);
							if (iter_two != vecTempCardData.end())
							{
								bIsAgainDispatchCard = true;
							}
						}
					}
				}

				strTagInfo += CStringUtility::FormatToString("cbCardDataOne:0x%02X,", cbCardDataOne);
				strTagInfo += CStringUtility::FormatToString("cbCardDataTwo:0x%02X,", cbCardDataTwo);
				strTagInfo += CStringUtility::FormatToString("cbOneValue:%d,", cbOneValue);
				strTagInfo += CStringUtility::FormatToString("cbTwoValue:%d,", cbTwoValue);
				strTagInfo += CStringUtility::FormatToString("cbOneColor:%d,", cbOneColor);
				strTagInfo += CStringUtility::FormatToString("cbTwoColor:%d,", cbTwoColor);
				strTagInfo += CStringUtility::FormatToString("bIsAgaCard:%d,", bIsAgainDispatchCard);
			}
			else
			{
				bIsAgainDispatchCard = true;
			}

			if (bIsAgainDispatchCard)
			{
				//重新发牌
				DispatchCard(wDispatchCardUserCount);
			}
			else
			{
				break;
			}
		}
	}

	string strHandCardData;
	for (int i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			strHandCardData += CStringUtility::FormatToString("i:%d,uid:%d,cd:0x%02X_0x%02X ",i, GetPlayerID(i), m_cbHandCardData[i][0], m_cbHandCardData[i][1]);
		}
	}

	LOG_DEBUG("roomid:%d,tableid:%d,control_uid:%d,uCtrlChairID:%d,iLoopIndex:%d,strTagInfo:%s,strHandCardData:%s",
		GetRoomID(), GetTableID(), control_uid, uCtrlChairID, iLoopIndex, strTagInfo.c_str(), strHandCardData.c_str());


	return bRetPlayerWinScore;
}


bool	CGameTexasTable::SetControlPalyerWin(uint32 control_uid)
{
	uint16 maxChairID = INVALID_CHAIR;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		if (maxChairID == INVALID_CHAIR)
		{
			maxChairID = i;
			continue;
		}
		if (m_GameLogic.CompareCard(m_cbMaxCardData[i], m_cbMaxCardData[maxChairID], MAX_CARD_COUNT) == 2)
		{
			maxChairID = i;
		}
	}
	if (maxChairID == INVALID_CHAIR)
	{
		uint16 i = 0;
		LOG_DEBUG("0 - i:%d,control_uid:%d,m_cbHandCardData[0x%02X 0x%02X],m_cbMaxCardData[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X],m_cbEndCardType[%d]",
			i, control_uid, m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbMaxCardData[i][0], m_cbMaxCardData[i][1], m_cbMaxCardData[i][2], m_cbMaxCardData[i][3], m_cbMaxCardData[i][4], m_cbEndCardType[i]);

		return false;
	}

	CGamePlayer* pGamePlayer = GetPlayer(maxChairID);
	if (pGamePlayer != NULL && pGamePlayer->GetUID() == control_uid) {
		uint16 i = 0;
		LOG_DEBUG("1 - i:%d,control_uid:%d,m_cbHandCardData[0x%02X 0x%02X],m_cbMaxCardData[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X],m_cbEndCardType[%d]",
			i, control_uid, m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbMaxCardData[i][0], m_cbMaxCardData[i][1], m_cbMaxCardData[i][2], m_cbMaxCardData[i][3], m_cbMaxCardData[i][4], m_cbEndCardType[i]);

		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer != NULL && pPlayer->GetUID() == control_uid) {
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);

			BYTE cbTempMaxCardData[MAX_CARD_COUNT];
			memcpy(cbTempMaxCardData, m_cbMaxCardData[i], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[i], m_cbMaxCardData[maxChairID], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[maxChairID], cbTempMaxCardData, MAX_CARD_COUNT);

			BYTE cbTempEndCardType = m_cbEndCardType[i];
			m_cbEndCardType[i] = m_cbEndCardType[maxChairID];
			m_cbEndCardType[maxChairID] = cbTempEndCardType;

			LOG_DEBUG("2 - i:%d,control_uid:%d,m_cbHandCardData[0x%02X 0x%02X],m_cbMaxCardData[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X],m_cbEndCardType[%d]",
				i, control_uid, m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbMaxCardData[i][0], m_cbMaxCardData[i][1], m_cbMaxCardData[i][2], m_cbMaxCardData[i][3], m_cbMaxCardData[i][4], m_cbEndCardType[i]);


			return true;
		}
	}
	return false;
}

bool	CGameTexasTable::SetControlPalyerLost(uint32 control_uid)
{
	uint16 minChairID = INVALID_CHAIR;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		if (minChairID == INVALID_CHAIR)
		{
			minChairID = i;
			continue;
		}
		if (m_GameLogic.CompareCard(m_cbMaxCardData[i], m_cbMaxCardData[minChairID], MAX_CARD_COUNT) == 1)
		{
			minChairID = i;
		}
	}
	if (minChairID == INVALID_CHAIR)
	{
		return false;
	}

	CGamePlayer* pGamePlayer = GetPlayer(minChairID);
	if (pGamePlayer != NULL && pGamePlayer->GetUID() == control_uid) {
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer != NULL && pPlayer->GetUID() == control_uid) {
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);

			BYTE cbTempMaxCardData[MAX_CARD_COUNT];
			memcpy(cbTempMaxCardData, m_cbMaxCardData[i], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[i], m_cbMaxCardData[minChairID], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[minChairID], cbTempMaxCardData, MAX_CARD_COUNT);

			BYTE cbTempEndCardType = m_cbEndCardType[i];
			m_cbEndCardType[i] = m_cbEndCardType[minChairID];
			m_cbEndCardType[minChairID] = cbTempEndCardType;


			return true;
		}
	}
	return false;
}

// 写入出牌log
void    CGameTexasTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);

	CGamePlayer *pPlayer = GetPlayer(chairID);
	if (pPlayer == NULL || m_cbPlayStatus[chairID] == FALSE)
		return;
	int64 lPlayerTotalWinScore = pPlayer->GetPlayerTotalWinScore(CDataCfgMgr::Instance().GetCurSvrCfg().gameType);
	m_operLog["ptws"] = lPlayerTotalWinScore;
}
// 写入公牌log
void    CGameTexasTable::WritePublicCardLog(uint8 cardData[],uint8 cardCount)
{
    for(uint32 i=0;i<cardCount;++i){
         m_operLog["pc"].append(cardData[i]);
    }
}
// 写入下注log
void    CGameTexasTable::WriteAddScoreLog(uint16 chairID,int64 score,uint8 round)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["s"]       = score;
    logValue["r"]       = round;

    m_operLog["score"].append(logValue);
}
//放弃事件
bool    CGameTexasTable::OnUserGiveUp(WORD wChairID)
{
    if(wChairID != m_wCurrentUser){
        LOG_WARNING("玩家投降, 不是当前玩家   roomid:%d,tableid:%d,wChairID:%d,m_wCurrentUser:%d", 
			GetRoomID(), GetTableID(), wChairID, m_wCurrentUser);
        return false;
    }
    
	//重置状态
	m_cbPlayStatus[wChairID] = FALSE;
	m_cbShowHand[wChairID]   = FALSE;
    m_szCardState[wChairID]  = 2;

	//发送消息
    net::msg_texas_giveup_rep msg;
    msg.set_giveup_user(wChairID);
    msg.set_lost_score(-m_lTotalScore[wChairID]);
    
    SendMsgToAll(&msg,net::S2C_MSG_TEXAS_GIVEUP);

	//结算积分
    CalcPlayerInfo(wChairID,-m_lTotalScore[wChairID]);
    
	//清空下注
	m_lTableScore[wChairID] = 0L;

	//人数统计
	WORD wPlayerCount=0;
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if(m_cbPlayStatus[i]==TRUE)
            wPlayerCount++;
	}
	LOG_DEBUG("玩家投降  roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentUser:%d,wPlayerCount:%d", 
		GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_wCurrentUser, wPlayerCount);
	//判断结束
	if(wPlayerCount >= 2)
	{
		if(m_wCurrentUser == wChairID){ 
            OnUserAddScore(wChairID,0L,true);
		}
	}else{
        WriteAddScoreLog(wChairID,-1,m_cbBalanceCount);
        OnGameEnd(INVALID_CHAIR,GER_NO_PLAYER);
	}

	return true;
}
//加注事件
bool    CGameTexasTable::OnUserAddScore(WORD wChairID, int64 lScore, bool bGiveUp)
{    
    //LOG_DEBUG("玩家加注:%d,%lld,当前下注:%lld,allin:%lld,最大下注:%lld",wChairID,lScore,m_lTotalScore[wChairID],m_lTurnMaxScore,m_lUserMaxScore[wChairID]);
	if (m_wCurrentUser != wChairID) {
		LOG_WARNING("加注  m_wCurrentUser!=wChairID  roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentUser:%d,lScore:%d,bGiveUp:%d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_wCurrentUser, lScore, bGiveUp);
		return false;
	}   

	//校验金币
	if (lScore + m_lTotalScore[wChairID] > m_lUserMaxScore[wChairID]) {
		LOG_WARNING("加注  金币超过上限  roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%d,bGiveUp:%d,m_lTotalScore:%d,m_lUserMaxScore:%d,m_wCurrentUser:%d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, bGiveUp, m_lTotalScore[wChairID], m_lUserMaxScore[wChairID], m_wCurrentUser);
		return false;
	}

	if (lScore < 0) {
		LOG_WARNING("加注  lScore<0  roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%d,bGiveUp:%d,m_wCurrentUser:%d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, bGiveUp, m_wCurrentUser);
		return false;
	}

    if(lScore < m_lTurnLessScore && !bGiveUp) {
		LOG_WARNING("加注  未达到平衡注  roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%d,bGiveUp:%d,m_lTurnLessScore:%d,m_wCurrentUser:%d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, bGiveUp, m_lTurnLessScore, m_wCurrentUser);
        return false;
    }

	//累计金币
	m_lTableScore[wChairID] += lScore;
	m_lTotalScore[wChairID] += lScore;
    m_lRoundScore[wChairID] += lScore;

    if(bGiveUp){
        WriteAddScoreLog(wChairID,-1,m_cbBalanceCount);
    }else{
        WriteAddScoreLog(wChairID, lScore, m_cbBalanceCount);
    }
	//平衡下注
	if(m_lTableScore[wChairID] > m_lBalanceScore)
	{
		m_lBalanceScore = m_lTableScore[wChairID];
	}
	//梭哈判断
	if(m_lTotalScore[wChairID] == m_lUserMaxScore[wChairID])
	{
		m_cbShowHand[wChairID] = TRUE;
	}

	//用户切换
	WORD wNextPlayer=INVALID_CHAIR;
	for(WORD i=1;i<GAME_PLAYER;i++)
	{
		//设置变量
		m_wOperaCount++;
		wNextPlayer=(m_wCurrentUser+i)%GAME_PLAYER;

		//继续判断
		if ((m_cbPlayStatus[wNextPlayer]==TRUE) && (m_cbShowHand[wNextPlayer]==FALSE))
            break;
	}
	ASSERT(wNextPlayer < GAME_PLAYER);

	//完成判断
	bool bFinishTurn=false;
	if(m_wOperaCount>=GAME_PLAYER)
	{
        WORD i=0;
		for(i=0;i<GAME_PLAYER;i++)
		{
			//过滤未平衡 和未梭哈用户
			if((m_cbPlayStatus[i]==TRUE)&&(m_lTableScore[i]<m_lBalanceScore)&&(m_cbShowHand[i]==FALSE)) 
				break;
		}
		if(i==GAME_PLAYER) 
			bFinishTurn=true;
	}

	//A家show190,B放弃,C还选择?
	if(!bFinishTurn)
	{
		WORD wPlayCount = 0,wShowCount = 0;
		for(BYTE i=0;i<GAME_PLAYER;i++)
		{
			if(m_cbPlayStatus[i]==TRUE)
			{
				if(m_cbShowHand[i]==TRUE)
				{
					wShowCount++;
				}
				wPlayCount++;
			}
		}
		if(wPlayCount-1 == wShowCount && m_lTableScore[wNextPlayer] >= m_lBalanceScore) 
            bFinishTurn = true;
	}

	//继续加注
	if(!bFinishTurn)
	{
		//当前用户
		m_wCurrentUser = wNextPlayer;

		//最小值为平衡下注 -桌面下注  和 剩余金币中取小 可能梭哈
		m_lTurnLessScore = min((m_lBalanceScore - m_lTableScore[m_wCurrentUser]),(m_lUserMaxScore[m_wCurrentUser] - m_lTotalScore[m_wCurrentUser]));
		m_lTurnMaxScore = m_lUserMaxScore[m_wCurrentUser]-m_lTotalScore[m_wCurrentUser];
		if(m_lTotalScore[m_wCurrentUser] == m_lCellScore)
		{
			int64 bTemp = (m_lBalanceScore == m_lCellScore*2)?(m_lCellScore*2):((m_lBalanceScore-m_lCellScore*2)*2);
			m_lAddLessScore = m_lCellScore + bTemp;
		}else{
		    m_lAddLessScore = (m_lBalanceScore==0)?(2*m_lCellScore):(max((m_lBalanceScore-m_lTableScore[m_wCurrentUser])*2,2*m_lCellScore));
		}
		//构造数据
        net::msg_texas_addscore_rep msg;
        msg.set_current_user(m_wCurrentUser);
        msg.set_addscore_user(wChairID);
        msg.set_addscore_count(lScore);
        msg.set_turn_less_score(m_lTurnLessScore);
        msg.set_turn_max_score(m_lTurnMaxScore);
        msg.set_add_less_score(m_lAddLessScore);

        SendMsgToAll(&msg,net::S2C_MSG_TEXAS_ADD_SCORE);

        m_coolLogic.beginCooling(s_AddScoreTime);
        m_coolRobot.beginCooling(g_RandGen.RandRange(1000,3000));
		LOG_DEBUG("加注响应 !bFinishTurn roomid:%d,tableid:%d,wChairID:%d,uid:%d,m_wCurrentUser:%d,curuid:%d,lScore:%d,m_lTurnLessScore:%d,m_lTurnMaxScore:%d,m_lAddLessScore:%d,m_lBalanceScore:%d,m_lTableScore[m_wCurrentUser]:%d,m_lUserMaxScore[m_wCurrentUser]:%d,m_lTotalScore[m_wCurrentUser]:%d",
			GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), m_wCurrentUser, GetPlayerID(m_wCurrentUser), lScore,
			m_lTurnLessScore, m_lTurnMaxScore, m_lAddLessScore, m_lBalanceScore, m_lTableScore[m_wCurrentUser], 
			m_lUserMaxScore[m_wCurrentUser], m_lTotalScore[m_wCurrentUser]);
		return true;
	}

	//平衡次数
	m_cbBalanceCount++;
	m_wOperaCount = 0;
    memset(m_lRoundScore,0,sizeof(m_lRoundScore));
    
	//第1次下注平衡后就开始发给三张公牌
	//第2次下注平衡后就开始发第四张公牌
	//第3次下注平衡后就开始发第五张公牌
	//第4次下注平衡后就结束游戏 

	//小盲下注
	WORD wDUser = m_wMinChipinUser;
	for(BYTE i=0;i<GAME_PLAYER;i++)
	{
		wDUser=(m_wMinChipinUser+i)%GAME_PLAYER;
		if(m_cbPlayStatus[wDUser]==TRUE && m_cbShowHand[wDUser]==FALSE) 
            break;
	}

	//重值变量
	m_lBalanceScore     = 0L;
	m_lTurnLessScore    = 0L;
	m_lTurnMaxScore     = m_lUserMaxScore[wDUser]-m_lTotalScore[wDUser];
	m_lAddLessScore     = 2*m_lCellScore;

	//构造数据
    net::msg_texas_addscore_rep msg;
    msg.set_current_user(INVALID_CHAIR);
    msg.set_addscore_user(wChairID);
    msg.set_addscore_count(lScore);
    msg.set_turn_less_score(m_lTurnLessScore);
    msg.set_turn_max_score(m_lTurnMaxScore);
    msg.set_add_less_score(m_lAddLessScore);

    SendMsgToAll(&msg,net::S2C_MSG_TEXAS_ADD_SCORE);
	LOG_DEBUG("加注响应 bFinishTurn roomid:%d,tableid:%d,wChairID:%d,uid:%d,wDUser:%d,duid:%d,lScore:%d,m_lTurnLessScore:%d,m_lTurnMaxScore:%d,m_lAddLessScore:%d,m_lBalanceScore:%d,m_lUserMaxScore[wDUser]:%d,m_lTotalScore[wDUser]:%d",
		GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), wDUser, GetPlayerID(wDUser), lScore, m_lTurnLessScore,
		m_lTurnMaxScore, m_lAddLessScore, m_lBalanceScore, m_lUserMaxScore[wDUser], m_lTotalScore[wDUser]);

	//清理数据
	memset(m_lTableScore,0,sizeof(m_lTableScore));
	m_lBalanceScore = 0L;
	//结束判断
	if(m_cbBalanceCount == 4) 
	{
		OnGameEnd(INVALID_CHAIR,GER_NORMAL);
		return true;
	}
	//梭哈用户统计
	WORD wShowHandCount = 0,wPlayerCount = 0;
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if (m_cbShowHand[i]==TRUE)	
            wShowHandCount++;
		if (m_cbPlayStatus[i]==TRUE) 
            wPlayerCount++;
	}

	//只剩一玩家没梭或者全梭
	if((wShowHandCount >= wPlayerCount -1) && m_cbBalanceCount < 4)
	{
		//构造数据
		net::msg_texas_sendcard_rep msg;
        msg.set_public_card(m_cbBalanceCount);
        msg.set_current_user(INVALID_CHAIR);
        msg.set_send_card_count(MAX_CENTERCOUNT);
        for(uint16 i=0;i<MAX_CENTERCOUNT;++i)
        {
            msg.add_center_cards(m_cbCenterCardData[i]);
        }
        SendMsgToAll(&msg,net::S2C_MSG_TEXAS_SEND_CARD);
        m_cbSendCardCount = MAX_CENTERCOUNT;
        
		//结束游戏
		OnGameEnd(INVALID_CHAIR,GER_NORMAL);
		return true;
	}

	//盲注玩家
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		//临时变量
		BYTE cbNextUser =(m_wMinChipinUser+i)%GAME_PLAYER;

		//获取用户
		CGamePlayer* pPlayer = GetPlayer(cbNextUser);

		//无效用户 梭哈用户过滤
		if(pPlayer == NULL || m_cbPlayStatus[cbNextUser] == FALSE || m_cbShowHand[cbNextUser] == 1) 
			continue;

		m_wCurrentUser = cbNextUser;
		break;
	}

	net::msg_texas_sendcard_rep sendcardmsg;
    sendcardmsg.set_public_card(0);
    sendcardmsg.set_current_user(m_wCurrentUser);
    m_cbSendCardCount = 3 +(m_cbBalanceCount-1);
    sendcardmsg.set_send_card_count(m_cbSendCardCount);
    for(uint16 i=0;i<m_cbSendCardCount;++i)
    {
        sendcardmsg.add_center_cards(m_cbCenterCardData[i]);
    }
    SendMsgToAll(&sendcardmsg,net::S2C_MSG_TEXAS_SEND_CARD); 

    m_coolLogic.beginCooling(s_AddScoreTime);
    m_coolRobot.beginCooling(g_RandGen.RandRange(3000,6000));
    return true;
}    
//带入
bool    CGameTexasTable::OnUserBuyin(CGamePlayer* pPlayer,int64 score)
{
    uint16 chairID = GetChairID(pPlayer);
    net::msg_texas_buyin_rep msg;
    msg.set_score(score);
    if(chairID != INVALID_CHAIR){
        LOG_WARNING("已经在座位了不能buyin roomid:%d,tableid:%d,uid:%d,chairID:%d,buyin:%d", GetRoomID(), GetTableID(), 
			pPlayer->GetUID(), chairID, score);
        msg.set_result(net::RESULT_CODE_GAMEING);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TEXAS_BUYIN);
        return false;
    }
    if(score < m_pHostRoom->GetSitDown() || GetPlayerCurScore(pPlayer) < score)
    {
		LOG_WARNING("积分不够buyin roomid:%d,tableid:%d,uid:%d,buyin:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), score);
        msg.set_result(net::RESULT_CODE_NOT_COND);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TEXAS_BUYIN);
        return false; 
    }
	// add by har
	unordered_map<uint32, uint32>::iterator iter = m_mpWaitBuyinScore.find(pPlayer->GetUID());
	uint32 nChairId = INVALID_CHAIR;
	if (iter != m_mpWaitBuyinScore.end()) {
		m_mpWaitBuyinScore.erase(pPlayer->GetUID());
		nChairId = iter->second;
	} // add by har end
    ChangeBuyinScore(pPlayer->GetUID(),score);
    LOG_DEBUG("player buyin suc  roomid:%d,tableid:%d,uid:%d,buyin:%d,nChairId:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), score, nChairId);
    msg.set_result(net::RESULT_CODE_SUCCESS);
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_TEXAS_BUYIN);
    return true; // 客户端请求买入后会紧接着向服务端发送请求坐下的消息C2S_MSG_SITDOWN_STANDUP，服务端在CGameTexasTable::OnActionUserSitDown函数里处理
}

//下一局补充 add by har
bool CGameTexasTable::OnUserBuyinNext(CGamePlayer* pPlayer, int64 score) {
	uint16 chairID = GetChairID(pPlayer);
	net::msg_texas_buyin_next_rep msg;
	msg.set_score(score);
	if (score == -1) {
		m_mpNextBuyinScore.erase(pPlayer->GetUID());
		LOG_DEBUG("buyin next -1 roomid:%d,tableid:%d,uid:%d,score:%d,SitDown:%d,CurScore:%d", GetRoomID(),
			GetTableID(), pPlayer->GetUID(), score, m_pHostRoom->GetSitDown(), GetPlayerCurScore(pPlayer));
		msg.set_result(net::RESULT_CODE_SUCCESS);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TEXAS_BUYIN_NEXT);
		return true;
	}
	if (chairID == INVALID_CHAIR || (score != 0 && (score < m_pHostRoom->GetSitDown() || GetPlayerCurScore(pPlayer) < score || score > 10 * m_pHostRoom->GetSitDown()))) { // modify by har
		LOG_WARNING("buyin next不满足条件 roomid:%d,tableid:%d,uid:%d,score:%d,SitDown:%d,CurScore:%d", GetRoomID(),
			GetTableID(), pPlayer->GetUID(), score, m_pHostRoom->GetSitDown(), GetPlayerCurScore(pPlayer));
		msg.set_result(net::RESULT_CODE_NOT_COND);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TEXAS_BUYIN_NEXT);
		return false;
	}

	m_mpNextBuyinScore[pPlayer->GetUID()] = score;
	LOG_DEBUG("buyin next  roomid:%d,tableid:%d,uid:%d,score:%d,SitDown:%d,CurScore:%d", GetRoomID(),
		GetTableID(), pPlayer->GetUID(), score, m_pHostRoom->GetSitDown(), GetPlayerCurScore(pPlayer));
	msg.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TEXAS_BUYIN_NEXT);
	return true;
}

//亮牌
bool CGameTexasTable::OnUserShowCard(WORD wChairID)
{
    if(GetGameState() == TABLE_STATE_PLAY )
    {
        m_szShowCard[wChairID] = TRUE;
        return true;
    }
    net::msg_texas_show_card_rep msg;
    msg.set_show_chairid(wChairID);
    for(uint8 i=0;i<MAX_COUNT;i++)
    {
        msg.add_cards(m_cbHandCardData[wChairID][i]);
    }
    SendMsgToAll(&msg,net::S2C_MSG_TEXAS_SHOW_CARD);
    return true;
}

// 等待买入筹码的玩家站立 add by har
void CGameTexasTable::OnUserBuyinWaitStandUp(CGamePlayer* pPlayer) {
	net::msg_texas_buyin_wait_standup_rep rep;
	unordered_map<uint32, uint32>::iterator iter = m_mpWaitBuyinScore.find(pPlayer->GetUID());
	if (iter == m_mpWaitBuyinScore.end()) {
		LOG_WARNING("CGameTexasTable::OnUserBuyinWaitStandUp  warn roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());
		rep.set_reason(0);
	} else {
		rep.set_reason(1);
		LOG_DEBUG("CGameTexasTable::OnUserBuyinWaitStandUp  roomid:%d,tableid:%d,uid:%d,chairid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), iter->second);
		m_mpWaitBuyinScore.erase(iter);
		SendSeatInfoToClient();
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TEXAS_BUYIN_WAIT_STANDUP);
}

void CGameTexasTable::SendShowCardUser()
{
    for(uint8 j=0;j<GAME_PLAYER;++j)
    {
        if(m_szShowCard[j] == FALSE)
            continue;

        net::msg_texas_show_card_rep msg;
        msg.set_show_chairid(j);
        for (uint8 i = 0; i < MAX_COUNT; i++) {
            msg.add_cards(m_cbHandCardData[j][i]);
        }
        SendMsgToAll(&msg, net::S2C_MSG_TEXAS_SHOW_CARD);
    }
}

int64   CGameTexasTable::GetBuyinScore(uint32 uid)
{
    return m_mpBuyinScore[uid];
}
int64   CGameTexasTable::ChangeBuyinScore(uint32 uid,int64& score)
{
    if(score > 0)
    {
        m_mpBuyinScore[uid] += score;
    }else{
        if(m_mpBuyinScore[uid] < (-score))
        {
            score = -m_mpBuyinScore[uid];
            m_mpBuyinScore[uid] = 0;
        }else{            
            m_mpBuyinScore[uid] += score;
        }        
    }
    for(uint16 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].uid == uid){
            m_vecPlayers[i].buyinScore = m_mpBuyinScore[uid];
        }        
    }        
    return m_mpBuyinScore[uid];
}
//没积分的玩家自动站起
void    CGameTexasTable::StandUpNotScore()
{
	net::msg_texas_standup_table_rep rep;
	rep.set_reason(1);
	net::msg_texas_buyin_next_suc_rep rep2; // add by har
	unordered_set<uint32> usUid; // m_mpWaitBuyinScore里要保留的玩家id add by har
    for(uint8 i=0; i < m_vecPlayers.size(); ++i) {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL) {
			int64 buyinScore = m_vecPlayers[i].buyinScore;
			// add by har
			int64 curScore = GetPlayerCurScore(pPlayer);
			unordered_map<uint32, int64>::iterator it = m_mpNextBuyinScore.find(pPlayer->GetUID());
			if (it != m_mpNextBuyinScore.end()) {
				int64 oldBuyinScore = buyinScore;
				// 自动补充/补齐筹码
				int64 needScore; // 理论要补齐到的筹码
				int64 nextBuyinScore = it->second;
				int64 maxBuyinScore = 10 * m_pHostRoom->GetSitDown();
				if (nextBuyinScore == 0){
					needScore = maxBuyinScore;
				} else {
					needScore = buyinScore + nextBuyinScore;
					if (needScore > maxBuyinScore)
						needScore = maxBuyinScore;
					m_mpNextBuyinScore.erase(it);
				}
				int64 needBuyinScore = min(curScore, needScore); // 实际要补齐到的筹码
				int64 changeScore = needBuyinScore - buyinScore;
				if (changeScore > 0) {
					buyinScore = ChangeBuyinScore(pPlayer->GetUID(), changeScore);
					rep2.set_score(needBuyinScore);
					pPlayer->SendMsgToClient(&rep2, net::S2C_MSG_TEXAS_BUYIN_NEXT_SUC);
				}
				LOG_DEBUG("CGameTexasTable::StandUpNotScore auto  room:%d,tbleid:%d,chairID:%d,uid:%d,curScore:%d,oldBuyinScore:%d,buyinScore:%d,needScore:%d,nextBuyinScore:%d,needBuyinScore:%d,changeScore:%d,BaseScore:%d,SitDown:%d",
					GetRoomID(), GetTableID(), i, pPlayer->GetUID(), curScore, oldBuyinScore, buyinScore, needScore, nextBuyinScore, needBuyinScore, changeScore, GetBaseScore(), m_pHostRoom->GetSitDown());
				
			} // add by har end
			bool isNeedSave = false;
			if (buyinScore < GetBaseScore() * 2) {
				// add by har
				if (!pPlayer->IsRobot() && curScore >= m_pHostRoom->GetSitDown()) { // 必须是非机器人才能插入m_mpWaitBuyinScore，否则会导致后面该机器人加注小于0错误导致无法正常结束牌局！
					if (m_mpWaitBuyinScore.find(pPlayer->GetUID()) == m_mpWaitBuyinScore.end()) {
						m_mpWaitBuyinScore[pPlayer->GetUID()] = i;
						usUid.insert(pPlayer->GetUID());
						isNeedSave = true;
						LOG_DEBUG("CGameTexasTable::StandUpNotScore usUid.insert seat:roomid:%d,tableid:%d,chairID:%d,uid:%d,curScore:%d,buyinScore:%d,BaseScore:%d,SitDown:%d",
							GetRoomID(), GetTableID(), i, pPlayer->GetUID(), curScore, buyinScore, GetBaseScore(), m_pHostRoom->GetSitDown());
					}
				} // add by har end
				m_vecPlayers[i].Reset();
				//LOG_DEBUG("CGameTexasTable::StandUpNotScore standup banker seat:room:%d--tb:%d,chairID:%d,uid:%d,curScore:%d,buyinScore:%d,BaseScore:%d,SitDown:%d",
				//	m_pHostRoom->GetRoomID(), GetTableID(), i, pPlayer->GetUID(), curScore, buyinScore, GetBaseScore(), m_pHostRoom->GetSitDown());
				AddLooker(pPlayer);
				OnActionUserStandUp(i, pPlayer);
				if (!isNeedSave)
				    pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TEXAS_STANDUP_TABLE);
			}
			
			if (isNeedSave) { // add by har
				pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TEXAS_BUYIN_WAIT);
				LOG_DEBUG("CGameTexasTable::StandUpNotScore wait seat:roomid:%d,tableid:%d,chairID:%d,uid:%d,curScore:%d,buyinScore:%d,BaseScore:%d,SitDown:%d",
					GetRoomID(), GetTableID(), i, pPlayer->GetUID(), curScore, buyinScore, GetBaseScore(), m_pHostRoom->GetSitDown());
			}
        }
    }
	// add by har
	for (unordered_map<uint32, uint32>::iterator iter = m_mpWaitBuyinScore.begin(); iter != m_mpWaitBuyinScore.end();)
		if (usUid.find(iter->first) == usUid.end()) {
			LOG_DEBUG("CGameTexasTable::StandUpNotScore m_mpWaitBuyinScore.erase :roomid:%d,tableid:%d,chairID:%d,uid:%d",
				GetRoomID(), GetTableID(), iter->second, iter->first); // iter-> 必须写在iter++前面，否则会宕机！
			m_mpWaitBuyinScore.erase(iter++); //满足删除条件，删除当前结点，并指向下面一个结点
		} else
			++iter; //条件不满足，指向下面一个结点
	// add by har end
}
void    CGameTexasTable::OnRobotOper(uint16 chairID)
{
	CalcRobotAIInfo(chairID); // 必须放在前面，否则m_robotAIInfo的信息没有赋值，导致机器人加注未达到平衡注，导致牌局无法正常结束！ add by har
    if(!IsHavePlayerOnline())//没有玩家
    {
        if(g_RandGen.RandRatio(50,100)){
			int32 nRandRange = g_RandGen.RandRange(0, 1);
			LOG_DEBUG("机器人操作  没有玩家  roomid:%d,tableid:%d,chairID:%d,uid:%d,winPro:%d,nRandRange:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_robotAIInfo.winPro, nRandRange);
            OnRobotAddScore(chairID, nRandRange);
        }else{
			LOG_DEBUG("机器人操作  没有玩家，弃牌  roomid:%d,tableid:%d,chairID:%d,uid:%d,winPro:%d,balanceCount:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_robotAIInfo.winPro, m_robotAIInfo.balanceCount);
			OnRobotGiveUp(chairID, 0);
        }
        return;
    }
    
    //CalcRobotAIInfo(chairID); delete by har
    LOG_DEBUG("机器人操作   roomid:%d,tableid:%d,chairID:%d,uid:%d,winPro:%d,balanceCount:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_robotAIInfo.winPro, m_robotAIInfo.balanceCount);
    switch(m_robotAIInfo.balanceCount)
    {
    case 0:// 起手
        {
           OnRobotOper0(chairID);
        }break;
    case 1:// 3张公牌
    case 2:// 4张公牌
        {
           OnRobotOperEx(chairID); 
        }break;
    case 3:// 5张公牌
        {
           OnRobotOper5(chairID); 
        }break;
    case 4:// 结束    
        {            
        }break;
    default:
        break;
    }              
}
bool    CGameTexasTable::OnRobotOper0(uint16 chairID)
{
    LOG_DEBUG("robot oper0  roomid:%d,tableid:%d,chairID:%d,uid:%d,winPro:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_robotAIInfo.winPro);
    return OnRobotAddScoreByFirstScore(chairID,m_robotAIInfo.winPro);
}
bool    CGameTexasTable::OnRobotOperEx(uint16 chairID)
{
    LOG_DEBUG("robot operex  roomid:%d,tableid:%d,chairID:%d,uid:%d,winPro:%d,type:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_robotAIInfo.winPro, m_robotAIInfo.curGenre);
    //不是最大的机器人弃牌
    if(chairID != m_robotAIInfo.maxRobotChairID)
	{
        if(m_robotAIInfo.minScore > 0 && g_RandGen.RandRatio(50,100))
		{
            return OnRobotGiveUp(chairID,0);
        }     
    }    
    if(m_robotAIInfo.winPro == 0xFF)
    {
        if(m_robotAIInfo.curGenre >= CT_THREE_TIAO)
        {
            return OnRobotAddScore(chairID,g_RandGen.RandRange(3,4));
        }
        if(m_robotAIInfo.curGenre >= CT_TWO_LONG && g_RandGen.RandRatio(30,PRO_DENO_100))
        {
            return OnRobotAddScore(chairID,g_RandGen.RandRange(1,2));
        }
        return OnRobotAddScore(chairID,0);
    }        
    
    if(m_robotAIInfo.minScore > m_robotAIInfo.poolScore/2)
    {
        if(m_robotAIInfo.maxChairID != chairID)
		{                 
            return OnRobotGiveUp(chairID,m_robotAIInfo.winPro);   
        }
    }   

    if(m_robotAIInfo.winPro > 95 && m_robotAIInfo.curGenre >= CT_TWO_LONG){
        return OnRobotAddScore(chairID,g_RandGen.RandRange(4,5));
    }
    if(m_robotAIInfo.winPro > 85 && m_robotAIInfo.curGenre >= CT_ONE_LONG){
        return OnRobotAddScore(chairID,g_RandGen.RandRange(3,5));
    }
    if(m_robotAIInfo.winPro > 75){
        return OnRobotAddScore(chairID,g_RandGen.RandRange(2,4));
    }
    if(m_robotAIInfo.winPro > 60){
        return OnRobotAddScore(chairID,g_RandGen.RandRange(1,3));
    }
    if(m_robotAIInfo.winPro < 35){
        return OnRobotAddScore(chairID,0);
    }
	if (m_robotAIInfo.maxChairID == chairID)
	{
		return OnRobotAddScore(chairID, 1);
	}
	else
	{
		return OnRobotGiveUp(chairID, m_robotAIInfo.winPro);
	}
}
bool    CGameTexasTable::OnRobotOper5(uint16 chairID)
{
    LOG_DEBUG("robot oper5:%d--%d",chairID,m_robotAIInfo.winPro);
	if (m_robotAIInfo.bigPublic == false && m_robotAIInfo.minScore > 0)
	{
		return OnRobotGiveUp(chairID, 0);
	}
    
    if(m_robotAIInfo.minScore > (m_robotAIInfo.poolScore/4))
	{      
        if(!m_robotAIInfo.isMaxGenre)
        {
            if(m_robotAIInfo.pubGenre == m_cbEndCardType[chairID])
                return OnRobotGiveUp(chairID,0); 
            
            return OnRobotGiveUp(chairID,5);
        }
    }  
    if(m_robotAIInfo.pubGenre == m_cbEndCardType[chairID]){
        if(m_robotAIInfo.isMaxGenre){
            return OnUserAddScore(chairID,m_robotAIInfo.minScore,false); 
        }else{
            return OnRobotGiveUp(chairID,0);
        }
    }
    //不是最大的机器人弃牌
    if(chairID != m_robotAIInfo.maxRobotChairID){
        if(m_robotAIInfo.minScore > 0){
            return OnRobotGiveUp(chairID,0);
        }else{            
            return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);   
        }        
    }
    //机器人是最大的牌
    if(chairID == m_robotAIInfo.maxChairID)
    {
        // 同花并且是最大的allin
        if(m_cbEndCardType[chairID] >= CT_THREE_TIAO)
        {
            return OnUserAddScore(chairID,m_robotAIInfo.maxScore,false);
        }
        // 两对以上
        if(m_cbEndCardType[chairID] >= CT_TWO_LONG)
        {
            if(g_RandGen.RandRatio(30,PRO_DENO_100)){
                return OnRobotAddScore(chairID,4);
            }
            return OnRobotAddScore(chairID,g_RandGen.RandRange(1,4));
        }                 
        
    }else{
        if(m_robotAIInfo.minScore > (m_lCellScore*20))
        {
            if(g_RandGen.RandRatio(90,100)){
                return OnRobotGiveUp(chairID,0);
            }            
        }
        // 两对以上
        if(m_cbEndCardType[chairID] >= CT_TWO_LONG)
        {
            return OnRobotAddScore(chairID,g_RandGen.RandRange(0,1));
        } 
        if(m_cbEndCardType[chairID] < CT_ONE_LONG){
            return OnRobotGiveUp(chairID,0);
        }
        uint32 showhand = GetAllinPlayerNum();
        if(showhand > 0)
        {
            if(m_robotAIInfo.minScore < m_lTotalScore[chairID]/3){
                if(g_RandGen.RandRatio(10,100)){
					return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
                }
            }
            return OnRobotGiveUp(chairID,10);
        }
        if(m_cbEndCardType[chairID] <= CT_TWO_LONG && m_robotAIInfo.minScore > m_lTotalScore[chairID]/3)
        {
            return OnRobotGiveUp(chairID,0);
        }
        
        return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);          
    }        
    return OnUserAddScore(chairID,m_robotAIInfo.minScore,false); 
}
bool    CGameTexasTable::OnRobotGiveUp(uint16 chairID,int32 pro)
{
	bool bIsRobotGiveUp = true;
	if (m_bIsControlPlayer)
	{
		if (m_robotAIInfo.maxChairID == chairID)
		{
			bIsRobotGiveUp = false;
		}
	}

	if (bIsRobotGiveUp && pro < 30)
	{
		return OnUserGiveUp(chairID);
	}

    if(bIsRobotGiveUp && m_robotAIInfo.minScore > 0 && !g_RandGen.RandRatio(pro,PRO_DENO_100))
    {
        return OnUserGiveUp(chairID);
    }
    return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
}
bool    CGameTexasTable::OnRobotAllIn(uint16 chairID)
{   
    LOG_DEBUG("机器人allin:%d",chairID);
    
    return OnUserAddScore(chairID,m_robotAIInfo.maxScore,false);
}
bool    CGameTexasTable::OnRobotAddScore(uint16 chairID,uint8 level)
{
    int64 score = m_robotAIInfo.minScore;
    switch(level)
    {
    case 0:
        {
        }break;
    case 1:
        {
            score = (m_robotAIInfo.poolScore - m_lRoundScore[chairID])/4;
        }break;
    case 2:
        {
            score = (m_robotAIInfo.poolScore - m_lRoundScore[chairID])/3;
        }break;
    case 3:
        {
            score = (m_robotAIInfo.poolScore - m_lRoundScore[chairID])/2;
        }break;
    case 4:
        {
            score = (m_robotAIInfo.poolScore - m_lRoundScore[chairID]);
        }break;
    case 5:
        {
            if(g_RandGen.RandRatio(50, PRO_DENO_100) || m_robotAIInfo.maxChairID == chairID)
            {
                return OnUserAddScore(chairID, m_robotAIInfo.maxScore, false);
            } else {
                return OnUserAddScore(chairID, score, false);
            }
        }break;
    default:
        break;
    }
    if(score > m_robotAIInfo.minScore){
        score = m_robotAIInfo.minScore + ((score-m_robotAIInfo.minScore)/m_lAddLessScore)*m_lAddLessScore;
    }
    score = max(score,m_robotAIInfo.minScore);
    score = min(score,m_robotAIInfo.maxScore);
	LOG_DEBUG("机器人加注  roomid:%d,tableid:%d,chairID:%d,uid:%d,level:%d,score:%d,minScore:%d,maxScore:%d,m_lAddLessScore:%d,m_lRoundScore:%d,m_lCellScore:%d,poolScore:%d,maxChairID:%d,winPro:%d,m_lTableScore:%d", 
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), level, score, m_robotAIInfo.minScore, m_robotAIInfo.maxScore,
		m_lAddLessScore, m_lRoundScore[chairID], m_lCellScore, m_robotAIInfo.poolScore, m_robotAIInfo.maxChairID, m_robotAIInfo.winPro,
		m_lTableScore[chairID]);
    if(score > m_lRoundScore[chairID]*2 && score > 10*m_lCellScore && score > m_robotAIInfo.poolScore/3){// 反打
        if(chairID != m_robotAIInfo.maxChairID){
            return OnRobotGiveUp(chairID,50);
        }else{
            if(m_robotAIInfo.winPro > 90){
                return OnUserAddScore(chairID,m_robotAIInfo.maxScore,false);
            }
			else if (m_robotAIInfo.winPro > 50) {
				return OnUserAddScore(chairID, m_robotAIInfo.minScore, false);
			}
        }
    }
    if(score < (m_lTableScore[chairID]/2) && g_RandGen.RandRatio(70,PRO_DENO_100)){
        return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
    }
    if(m_lRoundScore[chairID] > (score/4) || m_lRoundScore[chairID] > m_lCellScore*100){
        return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
    }
    return OnUserAddScore(chairID,score,false);
}
bool    CGameTexasTable::OnRobotAddScoreByFirstScore(uint16 chairID,uint32 score)
{
    LOG_DEBUG("机器人加注  roomid:%d,tableid:%d,chairID:%d,uid:%d,score:%d", GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), score);

    if(score < 200)
    {
        //不是最大的机器人弃牌
        if(chairID != m_robotAIInfo.maxRobotChairID)
		{
            if(m_robotAIInfo.minScore > 0 && g_RandGen.RandRatio(50,100))
			{
                return OnRobotGiveUp(chairID,0);
            }     
        }
		if (chairID != m_robotAIInfo.maxRobotChairID)
		{
			if (m_robotAIInfo.minScore > m_lCellScore * 5 || (m_robotAIInfo.minScore > 0 && g_RandGen.RandRatio(25, 100)))
			{
				return OnUserGiveUp(chairID);
			}
		}

        return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
    }
    if(score <= 1200)
	{
        return OnRobotAddScore(chairID,score/300);
    }
    if(g_RandGen.RandRatio(50,PRO_DENO_100)){
        if(g_RandGen.RandRatio(50,PRO_DENO_100)){
            return OnUserAddScore(chairID,m_robotAIInfo.maxScore,false);
        }
        return OnRobotAddScore(chairID,4);
    }else{
        return OnUserAddScore(chairID,m_robotAIInfo.minScore,false);
    }
}
void    CGameTexasTable::CalcRobotAIInfo(uint16 chairID)
{
    m_robotAIInfo.clear();
    m_robotAIInfo.chairID       = chairID;
    m_robotAIInfo.balanceCount  = m_cbBalanceCount;
    m_robotAIInfo.minScore      = m_lTurnLessScore;
    m_robotAIInfo.maxScore      = m_lTurnMaxScore;
    m_robotAIInfo.enemyNum      = GetPlayNum() - 1;

    for(uint16 i=0;i<GAME_PLAYER;++i){
        m_robotAIInfo.poolScore += m_lTotalScore[i];
        m_robotAIInfo.enemyShowhandNum += m_cbShowHand[i];
    }

    m_robotAIInfo.curGenre  = m_GameLogic.GetCurCardType(m_cbHandCardData[chairID],MAX_COUNT,m_cbCenterCardData,m_cbSendCardCount);
    m_robotAIInfo.bigPublic = m_GameLogic.CompareCard(m_cbMaxCardData[chairID],m_cbCenterCardData,MAX_CENTERCOUNT) > 1 ? true : false;                   
    m_robotAIInfo.pubGenre  = m_GameLogic.GetCurCardType(m_cbCenterCardData,MAX_COUNT,&m_cbCenterCardData[MAX_COUNT],MAX_CENTERCOUNT-MAX_COUNT);
    
    uint8 selfGenre = m_cbEndCardType[chairID];
    m_robotAIInfo.isMaxGenre = true;
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_cbPlayStatus[i] == TRUE && i != chairID && m_cbEndCardType[i] > selfGenre){
            m_robotAIInfo.isMaxGenre = false;
            break;
        }        
    }    
    m_robotAIInfo.maxChairID        = INVALID_CHAIR;
    m_robotAIInfo.maxRobotChairID   = INVALID_CHAIR;        
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        if(m_cbPlayStatus[i]==FALSE)
            continue;
        if(m_robotAIInfo.maxChairID == INVALID_CHAIR)
        {
            m_robotAIInfo.maxChairID = i;
            continue;
        }
        if(m_GameLogic.CompareCard(m_cbMaxCardData[i],m_cbMaxCardData[m_robotAIInfo.maxChairID],MAX_CARD_COUNT) == 2)
        {
            m_robotAIInfo.maxChairID = i;
        }        
    }
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        if(m_cbPlayStatus[i]==FALSE)
            continue;
        CGamePlayer* pTmp = GetPlayer(i);
        if(pTmp == NULL || !pTmp->IsRobot())
            continue;        
        if(m_robotAIInfo.maxRobotChairID == INVALID_CHAIR)
        {
            m_robotAIInfo.maxRobotChairID = i;
            continue;
        }
        if(m_GameLogic.CompareCard(m_cbMaxCardData[i],m_cbMaxCardData[m_robotAIInfo.maxRobotChairID],MAX_CARD_COUNT) == 2)
        {
            m_robotAIInfo.maxRobotChairID = i;
        }        
    }
    
    if(m_cbSendCardCount >= 3 && m_cbSendCardCount < 5){
        m_robotAIInfo.winPro = GetWinPro(chairID);
    }
    if(m_cbSendCardCount == 0){
        m_robotAIInfo.winPro = m_GameLogic.GetFirstCardValue(m_cbHandCardData[chairID],MAX_COUNT);
    }
}
//获得牌面胜率
uint32  CGameTexasTable::GetWinPro(uint16 chairID)
{
    uint32 winCount   = 0;
    uint32 totalCount = 0;
    uint8  selectFlag[FULL_COUNT];
    memset(selectFlag,0,sizeof(selectFlag));
    uint8 tmpCenterData[MAX_CENTERCOUNT];
    memset(tmpCenterData,0,sizeof(tmpCenterData));
    memcpy(tmpCenterData,m_cbCenterCardData,m_cbSendCardCount);

    uint16 enemyID = INVALID_CHAIR;
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        if(m_cbPlayStatus[i] == FALSE || i == chairID)
            continue;
        if(enemyID == INVALID_CHAIR || m_lRoundScore[i] > m_lRoundScore[enemyID]
        || (m_vecPlayers[i].pPlayer->IsRobot() == false && m_lRoundScore[i] > 0))
        {
            enemyID = i;
            continue;
        }
    }
    if(m_lRoundScore[enemyID] == 0)//没人加注
    {
        return 0xFF;
    }
    // 设置手牌标记
    for(uint8 k=0;k<GAME_PLAYER;++k)
    {
        if(k != chairID && k != enemyID)
            continue;
        for(uint8 i=0; i<MAX_COUNT; ++i)
        {
            for(uint8 j=0; j<FULL_COUNT; ++j) {
                if (m_cbHandCardData[k][i] == CTexasLogic::m_cbCardData[j]) {
                    selectFlag[j] = 1;
                    continue;
                }
            }
        }
    }
    for(uint8 i=0;i<m_cbSendCardCount;++i){
        for(uint8 j=0;j<FULL_COUNT;++j){
            if(m_cbCenterCardData[i] == CTexasLogic::m_cbCardData[j]){
                selectFlag[j] = 1;
                continue;
            }
        }
    }
    // 在剩下的牌里面遍历选择牌型组合
    vector<BYTE> pokerPool;
    for(uint8 i=0;i<FULL_COUNT;++i){
        if(selectFlag[i] == 0){
            pokerPool.push_back(CTexasLogic::m_cbCardData[i]);
        }
    }
    // 获取所有组合
    vector< vector<BYTE> > centerComb;
    int combCount = (MAX_CENTERCOUNT - m_cbSendCardCount);
    if(combCount > 0)
    {
        m_GameLogic.Combination(pokerPool,combCount,centerComb);
        // 遍历公牌组合
        for(uint32 i=0;i<centerComb.size();++i)
        {
            for(uint32 n = 0;n < centerComb[i].size();++n)
            {
                tmpCenterData[m_cbSendCardCount+n] = centerComb[i][n];
            }
            //扑克数据
            BYTE cbEndCardData[2][MAX_CENTERCOUNT];
            memset(cbEndCardData,0,sizeof(cbEndCardData));
            //获取扑克
            m_GameLogic.FiveFromSeven(m_cbHandCardData[chairID],MAX_COUNT,tmpCenterData,MAX_CENTERCOUNT,cbEndCardData[0],MAX_CENTERCOUNT);
            m_GameLogic.FiveFromSeven(m_cbHandCardData[enemyID],MAX_COUNT, tmpCenterData, MAX_CENTERCOUNT,cbEndCardData[1],MAX_CENTERCOUNT);
            if(m_GameLogic.CompareCard(cbEndCardData[0], cbEndCardData[1], MAX_CENTERCOUNT) == 2)
            {
                winCount++;
            }
            totalCount++;
        }
    }
    //LOG_DEBUG("胜率计算结果:win:%d--total:%d",winCount,totalCount);
    if(totalCount > 1){
        LOG_DEBUG("胜率计算结果:win:%d",winCount * 100/totalCount);
        return winCount * 100/totalCount;
    }
    return 0;
}


bool    CGameTexasTable::RobotBuyin(CGamePlayer* pRobot)
{    
    m_mpBuyinScore[pRobot->GetUID()] = 0;
    int64 score    = m_pHostRoom->GetSitDown()*g_RandGen.RandRange(1,10);
    int64 curScore = GetPlayerCurScore(pRobot);
    if(curScore < m_pHostRoom->GetSitDown())
        return false;    
    score = min(score,curScore);    
    ChangeBuyinScore(pRobot->GetUID(),score); 
               
    return true;
}
void    CGameTexasTable::CheckAddRobot()
{
	if (m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
	{
		return;
	}
    //uint16 playerNum = GetChairPlayerNum();
    //if(playerNum >= 1 && playerNum < 5)
    //{
    //    for(uint32 i=0;i<m_vecPlayers.size();++i)
    //    {
    //        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
    //        if(pPlayer != NULL && !pPlayer->IsRobot())// 只有一个玩家
    //        {
    //            CRobotMgr::Instance().RequestOneRobot(this);             
    //            break;
    //        }
    //    } 
    //}

	for (uint32 i = 0; i < m_vecPlayers.size(); ++i)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL && !pPlayer->IsRobot())
		{
			int freeCount = GetFreeChairNum();
			int minCount = 1;
			if (freeCount >= 2)
			{
				minCount = 2;
			}
			if (freeCount > 1)
			{
				if (freeCount < GAME_PLAYER)
				{
					int iRobotCount = g_RandGen.RandRange(minCount, freeCount);
					CRobotMgr::Instance().RequestXRobot(iRobotCount, this);
				}
			}
			//m_coolRobot.beginCooling(g_RandGen.RandRange(2000, 4000));
			return;
		}
	}

    //如果有机器人旁观者，能坐下就坐下，不能坐下就滚蛋
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;            
        if(g_RandGen.RandRatio(30,100) || !RobotBuyin(pPlayer))
            continue;
        bool bSitDown = false;
        uint16 chairID = GetRandFreeChairID();
        if(chairID != INVALID_CHAIR){
            bSitDown = PlayerSitDownStandUp(pPlayer,true,chairID);               
        }          
        if(bSitDown)
            break;
    }
    //没有坐下的机器人滚蛋
    it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;            
        if(CanLeaveTable(pPlayer) && LeaveTable(pPlayer))
            break;            
    }    
    //托管玩家站起
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && !pPlayer->IsRobot() && m_vecPlayers[i].autoState == 1)
        {
            PlayerSitDownStandUp(pPlayer,false,i);
        }
    }

    m_coolRobot.beginCooling(g_RandGen.RandRange(1000,2000));
}
uint32  CGameTexasTable::GetAllinPlayerNum()
{
    uint32 num = 0;
    for(int32 i=0;i<GAME_PLAYER;++i){
        if(m_cbShowHand[i] == TRUE)
            num++;
    }
    return num;
}
uint32  CGameTexasTable::GetPlayNum()
{
    uint32 num = 0;
    for(int32 i=0;i<GAME_PLAYER;++i){
        if(m_cbPlayStatus[i] == TRUE)
            num++;
    }
    return num;
}
//是否有玩家在桌子
bool    CGameTexasTable::IsHavePlayerOnline()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer != NULL && !pPlayer->IsRobot())
            return true;            
    }  
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && !pPlayer->IsRobot())
        {
            return true;
        }
    }    
    
    return false;          
}

int     CGameTexasTable::GetProCardType()
{
	int iSumValue = 0;
	int iArrDispatchCardPro[Texas_Pro_Index_MAX] = { 0 };
	for (int i = 0; i < Texas_Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_iArrDispatchCardPro[i];
		iSumValue += m_iArrDispatchCardPro[i];
	}
	LOG_DEBUG("roomid:%d,tableid:%d,iSumValue:%d", GetRoomID(), GetTableID(), iSumValue);

	if (iSumValue <= 0)
	{
		return 0;
	}
	int iRandNum = g_RandGen.RandRange(0, iSumValue);
	int iProIndex = 0;


	for (; iProIndex < Texas_Pro_Index_MAX; iProIndex++)
	{
		if (iArrDispatchCardPro[iProIndex] == 0)
		{
			continue;
		}
		if (iRandNum <= iArrDispatchCardPro[iProIndex])
		{
			break;
		}
		else
		{
			iRandNum -= iArrDispatchCardPro[iProIndex];
		}
	}

	return iProIndex;
}

bool	CGameTexasTable::ProbabilityDispatchPokerCard()
{
	BYTE cbCenterCardData[MAX_CENTERCOUNT] = { 0 };
	memcpy(cbCenterCardData, m_cbCenterCardData, sizeof(cbCenterCardData));
	bool bIsFlag = true;
	int iArProCardType[GAME_PLAYER] = { 0 };
	// 先确定每一个人获取的类型
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		iArProCardType[i] = Texas_Pro_Index_MAX;

		if (m_cbPlayStatus[i] == TRUE)
		{
			int iProIndex = GetProCardType();
			if (iProIndex < Texas_Pro_Index_MAX)
			{
				iArProCardType[i] = iProIndex;
			}

			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbPlayStatus:%d,iProIndex:%d",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbPlayStatus[i], iProIndex);

		}
	}
	LOG_DEBUG("roomid:%d,tableid:%d,cbCenterCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
		GetRoomID(), GetTableID(), cbCenterCardData[0], cbCenterCardData[1], cbCenterCardData[2], cbCenterCardData[3], cbCenterCardData[4]);

	// 根据类型获取全部的手牌
	bIsFlag = m_GameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData, cbCenterCardData);


	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1]);
		}
	}

	return bIsFlag;
}

bool CGameTexasTable::ActiveWelfareCtrl()
{
    LOG_DEBUG("enter ActiveWelfareCtrl ctrl player count:%d.", m_aw_ctrl_player_list.size());

    //获限当前局参与游戏的玩家列表
    m_curr_bet_user.clear();
    for (uint32 uIndex = 0; uIndex < GAME_PLAYER; uIndex++)
    {
        CGamePlayer *pPlayer = GetPlayer(uIndex);
        if (pPlayer == NULL)
        {
            continue;
        }
        LOG_DEBUG("table player - uid:%d Robot:%d Status:%d", pPlayer->GetUID(), pPlayer->IsRobot(), m_cbPlayStatus[uIndex]);
        if ( pPlayer->IsRobot() == false && m_cbPlayStatus[uIndex] == TRUE)
        {
            m_curr_bet_user.insert(pPlayer->GetUID());
        }
    }
    //获取当前局活跃福利的控制玩家列表
    GetActiveWelfareCtrlPlayerList();

    vector<tagAWPlayerInfo>::iterator iter = m_aw_ctrl_player_list.begin();
    for (; iter != m_aw_ctrl_player_list.end(); iter++)
    {
        uint32 control_uid = iter->uid;

		//判断当前控制玩家是否在配置概率范围内
		uint32 tmp = rand() % 100;
		uint32 probability = iter->probability;
		if (tmp > probability)
		{
			LOG_DEBUG("The current player is not in config rate - control_uid:%d tmp:%d probability:%d", control_uid, tmp, probability)
			continue;
		}
		LOG_DEBUG("The current player in config rate - control_uid:%d tmp:%d probability:%d", control_uid, tmp, probability)

        bool ret = SetControlPalyerWin(control_uid);
        if (ret)
        {
            LOG_DEBUG("search success current player - uid:%d ", control_uid);
            m_aw_ctrl_uid = control_uid;   //设置当前活跃福利所控的玩家ID
            return true;
        }
        else
        {
            LOG_DEBUG("search fail current player - uid:%d", control_uid);
        }
    }
    LOG_DEBUG("the all ActiveWelfareCtrl player is search fail. return false.");
    return false;
}

bool CGameTexasTable::SetLuckyCtrl()
{
	uint32 win_uid = 0;
	set<uint32> set_lose_uid;
	set_lose_uid.clear();

	bool flag = GetTableLuckyFlag(win_uid, set_lose_uid);

	LOG_DEBUG("flag:%d win_uid:%d set_lose_uid size:%d.", flag, win_uid, set_lose_uid.size());

	if (!flag)
	{
		m_lucky_flag = false;
		return false;
	}

	m_set_ctrl_lucky_uid.clear();
	m_lucky_flag = true;

	//设置当前赢家手牌
	uint8 chairid = 0;
	bool bIsCtrl = false;
	if (win_uid != 0)
	{
		CGamePlayer * pGamePlayer = GetGamePlayerByUid(win_uid);
		if (pGamePlayer != NULL)
		{
			chairid = GetChairID(pGamePlayer);
			if (chairid != INVALID_CHAIR && m_cbPlayStatus[chairid] == true)
			{
				bIsCtrl = SetControlPalyerWin(win_uid);
				if (bIsCtrl)
				{
					CGamePlayer * pGamePlayer = GetGamePlayerByUid(win_uid);
					if (pGamePlayer != NULL)
					{
						chairid = GetChairID(pGamePlayer);
						if (chairid != INVALID_CHAIR)
						{
							LOG_DEBUG("the lucky win player. uid:%d chairid:%d MaxCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X HandCardData:0x%02X 0x%02X EndCardType:%d",
								win_uid, chairid, m_cbMaxCardData[chairid][0], m_cbMaxCardData[chairid][1], m_cbMaxCardData[chairid][2], m_cbMaxCardData[chairid][3],
								m_cbMaxCardData[chairid][4], m_cbHandCardData[chairid][0], m_cbHandCardData[chairid][1], m_cbEndCardType[chairid]);
						}
					}
					m_set_ctrl_lucky_uid.insert(win_uid);
					LOG_DEBUG("the set lucky win player is success uid:%d.", win_uid);
				}
				else
				{
					LOG_DEBUG("the set lucky win player is fail uid:%d.", win_uid);
				}
			}
			else
			{
				LOG_DEBUG("the chair is error or chair player is not game. uid:%d chair:%d.", win_uid, chairid);
			}
		}	
		else
		{
			LOG_DEBUG("get player info is fail.uid:%d", win_uid);
		}
	}

	//设置当前输家手牌
	uint32 lose_uid = 0;
	for (uint32 uid : set_lose_uid)
	{
		lose_uid = uid;
		CGamePlayer * pGamePlayer = GetGamePlayerByUid(lose_uid);
		if (pGamePlayer != NULL)
		{
			chairid = GetChairID(pGamePlayer);
			if (chairid != INVALID_CHAIR && m_cbPlayStatus[chairid] == true)
			{
				bIsCtrl = SetLostForLuckyCtrl(lose_uid);
				if (bIsCtrl)
				{
					CGamePlayer * pGamePlayer = GetGamePlayerByUid(lose_uid);
					if (pGamePlayer != NULL)
					{
						chairid = GetChairID(pGamePlayer);
						if (chairid != INVALID_CHAIR)
						{
							LOG_DEBUG("the lucky lose player. uid:%d chairid:%d MaxCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X HandCardData:0x%02X 0x%02X EndCardType:%d",
								lose_uid, chairid, m_cbMaxCardData[chairid][0], m_cbMaxCardData[chairid][1], m_cbMaxCardData[chairid][2], m_cbMaxCardData[chairid][3],
								m_cbMaxCardData[chairid][4], m_cbHandCardData[chairid][0], m_cbHandCardData[chairid][1], m_cbEndCardType[chairid]);
						}
					}
					m_set_ctrl_lucky_uid.insert(lose_uid);
					LOG_DEBUG("the set lucky lose player is success uid:%d.", lose_uid);
				}
				else
				{
					LOG_DEBUG("the set lucky lose player is fail uid:%d.", lose_uid);
				}
			}
			else
			{
				LOG_DEBUG("the chair is error or chair player is not game. uid:%d chair:%d.", lose_uid, chairid);
			}
		}
		else
		{
			LOG_DEBUG("get player info is fail.uid:%d", lose_uid);
		}
	}	
	LOG_DEBUG("the set lucky success. tid:%d.", GetTableID());
	return true;
}

bool	CGameTexasTable::SetLostForLuckyCtrl(uint32 control_uid)
{
	uint16 minChairID = INVALID_CHAIR;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		
		if (minChairID == INVALID_CHAIR)
		{
			minChairID = i;
			continue;
		}

		//过滤掉已经换牌的玩家
		CGamePlayer * pPlayer = GetPlayer(i);
		if (!pPlayer)
		{
			auto iter = m_set_ctrl_lucky_uid.find(pPlayer->GetUID());
			if (iter != m_set_ctrl_lucky_uid.end())
			{
				continue;
			}
		}

		if (m_GameLogic.CompareCard(m_cbMaxCardData[i], m_cbMaxCardData[minChairID], MAX_CARD_COUNT) == 1)
		{
			minChairID = i;
		}
	}

	if (minChairID == INVALID_CHAIR)
	{
		return false;
	}

	CGamePlayer* pGamePlayer = GetPlayer(minChairID);
	if (pGamePlayer != NULL && pGamePlayer->GetUID() == control_uid) {
		return true;
	}

	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer != NULL && pPlayer->GetUID() == control_uid) {
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);

			BYTE cbTempMaxCardData[MAX_CARD_COUNT];
			memcpy(cbTempMaxCardData, m_cbMaxCardData[i], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[i], m_cbMaxCardData[minChairID], MAX_CARD_COUNT);
			memcpy(m_cbMaxCardData[minChairID], cbTempMaxCardData, MAX_CARD_COUNT);

			BYTE cbTempEndCardType = m_cbEndCardType[i];
			m_cbEndCardType[i] = m_cbEndCardType[minChairID];
			m_cbEndCardType[minChairID] = cbTempEndCardType;


			return true;
		}
	}
	return false;
}


// 设置库存输赢  add by har
bool CGameTexasTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	if (stockChange == -1) {
		for (uint16 i = 0; i < GAME_PLAYER; ++i) {
			if (m_cbPlayStatus[i] == FALSE)
				continue;
			CGamePlayer* pPlayer = GetPlayer(i);
			if (pPlayer != NULL && pPlayer->IsRobot()) {
				if (SetControlPalyerWin(pPlayer->GetUID())) {
					LOG_DEBUG("SetStockWinLose suc RobotWin - roomid:%d,tableid:%d,stockChange:%lld,i:%d",
						GetRoomID(), GetTableID(), stockChange, i);
					return true;
				}
			}
		}
		LOG_ERROR("SetStockWinLose RobotWin fail1 - roomid:%d,tableid:%d,stockChange:%lld",
			GetRoomID(), GetTableID(), stockChange);
	} else {
		for (uint16 i = 0; i < GAME_PLAYER; ++i) {
			if (m_cbPlayStatus[i] == FALSE)
				continue;
			CGamePlayer* pPlayer = GetPlayer(i);
			if (pPlayer != NULL && !pPlayer->IsRobot()) {
				if (SetControlPalyerWin(pPlayer->GetUID())) {
					LOG_DEBUG("SetStockWinLose suc PlayerWin - roomid:%d,tableid:%d,stockChange:%lld,i:%d",
						GetRoomID(), GetTableID(), stockChange, i);
					return true;
				}
			}
		}
		LOG_ERROR("SetStockWinLose PlayerWin fail2 - roomid:%d,tableid:%d,stockChange:%lld",
			GetRoomID(), GetTableID(), stockChange);
	}
	LOG_ERROR("SetStockWinLose fail3 - roomid:%d,tableid:%d,stockChange:%lld",
		GetRoomID(), GetTableID(), stockChange);
	return false;
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameTexasTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	DealIsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot, m_cbPlayStatus);
}

void   CGameTexasTable::CheckPlayerScoreManyLeave()
{
	if (m_pHostRoom == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,m_pHostRoom:%p", GetRoomID(), GetTableID(), m_pHostRoom);
		return;
	}
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL)
		{
			int64 lCurScore = GetPlayerCurScore(pPlayer);
			int64 lMaxScore = m_pHostRoom->GetEnterMax();
			if (lCurScore >= lMaxScore)
			{
				uint32 uid = pPlayer->GetUID();
				bool bCanLeaveTable = CanLeaveTable(pPlayer);
				bool bLeaveTable = false;
				bool bCanLeaveRoom = false;
				bool bLeaveRoom = false;
				if (bCanLeaveTable)
				{
					bLeaveTable = LeaveTable(pPlayer);
					if (bLeaveTable)
					{
						bCanLeaveRoom = m_pHostRoom->CanLeaveRoom(pPlayer);
						if (bCanLeaveRoom)
						{
							bLeaveRoom = m_pHostRoom->LeaveRoom(pPlayer);
						}
						net::msg_leave_table_rep rep;
						rep.set_result(1);
						pPlayer->SendMsgToClient(&rep, net::S2C_MSG_LEAVE_TABLE_REP);						
					}
				}
				LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,lCurScore:%lld,lMaxScore:%lld,bCanLeaveTable:%d,bLeaveTable:%d,bCanLeaveRoom:%d,bLeaveRoom:%d",
					GetRoomID(), GetTableID(), i, uid, lCurScore, lMaxScore, bCanLeaveTable, bLeaveTable, bCanLeaveRoom, bLeaveRoom);
			}
		}
	}
}