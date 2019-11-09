
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
using namespace game_war;

namespace
{
    const static uint32 s_FreeTime              = 3*1000;       // 空闲时间
    const static uint32 s_PlaceJettonTime       = 13*1000;       // 下注时间
    const static uint32 s_DispatchTime          = 10*1000;      // 发牌时间    
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 
        {
            pTable = new CGameWarTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 
        {
            pTable = new CGameWarTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 
        {
            pTable = new CGameWarTable(this,tableID,emTABLE_TYPE_PLAYER);
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
CGameWarTable::CGameWarTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));

	//个人下注
	for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
        m_userJettonScore[i].clear();
    }
	//玩家成绩	
	m_mpUserWinScore.clear();
	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));

    m_robotBankerWinPro     = 0;
	m_robotBankerMaxCardPro = 0;
	m_pCurBanker = NULL;
	m_tagControlPalyer.Init();
	m_bInitTableSuccess = false;

	m_bIsChairRobotAlreadyJetton = false;
	m_chairRobotPlaceJetton.clear();
	m_bIsRobotAlreadyJetton = false;
	m_RobotPlaceJetton.clear();

    return;
}
CGameWarTable::~CGameWarTable()
{

}
bool    CGameWarTable::CanEnterTable(CGamePlayer* pPlayer)
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

    return true;
}
bool    CGameWarTable::CanLeaveTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d", GetRoomID(), GetTableID());
		return false;
	}
	if (m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
	{
		return false;
	}
	if (pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());
	}
    return true;
}
bool    CGameWarTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
    
    return CGameTable::CanSitDown(pPlayer,chairID);
}
bool    CGameWarTable::CanStandUp(CGamePlayer* pPlayer)
{
    return true;
}    
bool    CGameWarTable::IsFullTable()
{
    if(m_mpLookers.size() >= 200)
        return true;
    
    return false;
}
void CGameWarTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::war_table_info* pwar = pInfo->mutable_war();
	pwar->set_tableid(GetTableID());
	pwar->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
		pwar->set_is_passwd(1);
    }else{
		pwar->set_is_passwd(0);
    }
	pwar->set_hostname(m_conf.hostName);
	pwar->set_basescore(m_conf.baseScore);
	pwar->set_consume(m_conf.consume);
	pwar->set_entermin(m_conf.enterMin);
	pwar->set_duetime(m_conf.dueTime);
	pwar->set_feetype(m_conf.feeType);
	pwar->set_feevalue(m_conf.feeValue);
	pwar->set_card_time(s_PlaceJettonTime);
	pwar->set_table_state(GetGameState());
	pwar->set_sitdown(m_pHostRoom->GetSitDown());
}

//配置桌子
bool CGameWarTable::Init()
{
	if (m_pHostRoom == NULL) {
		return false;
	}

    SetGameState(net::TABLE_STATE_WAR_FREE);

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    
    m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
    m_robotChairSize = g_RandGen.RandRange(2, 4);//机器人座位数

	m_iArrDispatchCardPro[Pro_Index_BaoZi] = 23;
	m_iArrDispatchCardPro[Pro_Index_ShunJin] = 22;
	m_iArrDispatchCardPro[Pro_Index_JinHua] = 800;
	m_iArrDispatchCardPro[Pro_Index_ShunZi] = 655;
	m_iArrDispatchCardPro[Pro_Index_Double] = 4000;
	m_iArrDispatchCardPro[Pro_Index_Single] = 4500;
	m_sysBankerWinPro = 80;

	ReAnalysisParam();
	m_bInitTableSuccess = true;
	CRobotOperMgr::Instance().PushTable(this);
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGameWarTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", m_pHostRoom->GetRoomID(),param.c_str());
		return true;
	}

	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		string strPro = CStringUtility::FormatToString("pr%d", i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral())
		{
			m_iArrDispatchCardPro[i] = jvalue[strPro.c_str()].asInt();
		}
	}

	if (jvalue.isMember("sbw") && jvalue["sbw"].isIntegral())
	{
		m_sysBankerWinPro = jvalue["sbw"].asInt();
	}


	LOG_ERROR("reader_json -  roomid:%d,tableid:%d,m_sysBankerWinPro:%d,m_iArrDispatchCardPro:%d %d %d %d %d %d",
		GetRoomID(), GetTableID(), m_sysBankerWinPro, m_iArrDispatchCardPro[0], m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3], m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5]);

	return true;
}


void CGameWarTable::ShutDown()
{

}
//复位桌子
void CGameWarTable::ResetTable()
{
    ResetGameData();
}

// 是否能够开始游戏
bool    CGameWarTable::IsCanStartGame()
{
	bool bFlag = true;

	if (m_bInitTableSuccess == false)
	{
		bFlag = false;
	}
	return bFlag;
}

void CGameWarTable::OnTimeTick()
{

	OnTableTick();

    uint8 tableState = GetGameState();
    if(m_coolLogic.isTimeOut())
    {
        switch(tableState)
        {
        case TABLE_STATE_WAR_FREE:           // 空闲
            {                
    //            if(IsCanStartGame())
				//{
    //                SetGameState(TABLE_STATE_WAR_PLACE_JETTON);                
				//	m_coolLogic.beginCooling(s_PlaceJettonTime);
				//	OnGameStart();
				//	InitBlingLog();
				//	InitChessID();
				//	CalculateDeity();

    //            }
				//else
				//{
    //                m_coolLogic.beginCooling(s_FreeTime);
    //            }
            }break;
        case TABLE_STATE_WAR_PLACE_JETTON:   // 下注时间
            {
				SetGameState(TABLE_STATE_WAR_GAME_END);
				m_coolLogic.beginCooling(s_DispatchTime);

				DispatchTableCard();

				m_bIsChairRobotAlreadyJetton = false;
				m_chairRobotPlaceJetton.clear();
				m_bIsRobotAlreadyJetton = false;
				m_RobotPlaceJetton.clear();

                OnGameEnd(INVALID_CHAIR,GER_NORMAL);

				m_brc_table_status = emTABLE_STATUS_END;
				m_brc_table_status_time = 0;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();

            }break;
        case TABLE_STATE_WAR_GAME_END:       // 结束游戏
            {
    			ClearTurnGameData();
                SetGameState(TABLE_STATE_WAR_FREE);
                m_coolLogic.beginCooling(s_FreeTime);
                
				m_brc_table_status = emTABLE_STATUS_FREE;
				m_brc_table_status_time = 0;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();

            }break;         
        default:
            break;
        }
    }
    if(tableState == TABLE_STATE_WAR_PLACE_JETTON && m_coolLogic.getPassTick() > 3000)
    {
        //OnRobotOper();

		OnChairRobotJetton();
		OnRobotJetton();
    }
    
}

void CGameWarTable::OnRobotTick()
{
	uint8 tableState = GetGameState();
	int64 passtick = m_coolLogic.getPassTick();
	//LOG_DEBUG("on_time_tick_loop - roomid:%d,tableid:%d,m_lGameCount:%lld,tableState:%d,passtick:%lld", GetRoomID(), GetTableID(), m_lGameCount, tableState, passtick);

	if (tableState == net::TABLE_STATE_WAR_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
	{
		OnChairRobotPlaceJetton();
		OnRobotPlaceJetton();
	}
	if (m_coolLogic.isTimeOut())
	{
		switch (tableState)
		{
		case TABLE_STATE_WAR_FREE:           // 空闲
		{
			if (IsCanStartGame())
			{
				if (OnGameStart())
				{
					InitBlingLog();
					InitChessID();
					CalculateDeity();
				}

				m_brc_table_status = emTABLE_STATUS_START;
				m_brc_table_status_time = s_PlaceJettonTime;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();
			}
			else
			{
				m_coolLogic.beginCooling(s_FreeTime);
			}
		}break;
		default:
			break;
		}
	}


}


// 游戏消息
int CGameWarTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,pPlayer:%p,cmdID:%d", GetRoomID(), GetTableID(), pPlayer, cmdID);

		return 0;
	}
	if (pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,uid:%d,cmdID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), cmdID);
	}
    switch(cmdID)
    {
    case net::C2S_MSG_WAR_PLACE_JETTON:  // 用户加注
        {
            if(GetGameState() != TABLE_STATE_WAR_PLACE_JETTON)
			{
                LOG_DEBUG("not jetton state can't jetton");
                return 0;
            }
            net::msg_war_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            return OnUserPlaceJetton(pPlayer,msg.jetton_area(),msg.jetton_score());
        }break;
	case net::C2S_MSG_WAR_CONTINUOUS_PRESSURE_REQ://
		{
			net::msg_player_continuous_pressure_jetton_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnUserContinuousPressure(pPlayer, msg);
		}break;
	case net::C2S_MSG_BRC_CONTROL_ENTER_TABLE_REQ://
		{
			net::msg_brc_control_user_enter_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlEnterControlInterface(pPlayer);
		}break;
	case net::C2S_MSG_BRC_CONTROL_LEAVE_TABLE_REQ://
		{
			net::msg_brc_control_user_leave_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlPlayerLeaveInterface(pPlayer);
		}break;
	case net::C2S_MSG_BRC_CONTROL_AREA_INFO_REQ://
		{
			net::msg_brc_control_area_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlPlayerBetArea(pPlayer, msg);
		}break;
    default:
	{
		return 0;
	}
    }
    return 0;
}
//用户断线或重连
bool CGameWarTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
{
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bConnected:%d,isJoin:%d",GetRoomID(),GetTableID(), pPlayer->GetUID(), bConnected, isJoin);
    if(bConnected)//断线重连
    {
        if(isJoin)
        {
            pPlayer->SetPlayDisconnect(false);
            PlayerSetAuto(pPlayer,0);
            SendTableInfoToClient(pPlayer);
            SendSeatInfoToClient(pPlayer);
            if(m_mpLookers.find(pPlayer->GetUID()) != m_mpLookers.end()){
                NotifyPlayerJoin(pPlayer,true);
            }
            SendLookerListToClient(pPlayer);
            SendGameScene(pPlayer);
			SendPlayLog(pPlayer);
        }
    }else{
        pPlayer->SetPlayDisconnect(true);
    }
    return true;
}
//用户坐下
bool CGameWarTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{
    
    SendSeatInfoToClient();
    return true;
}
//用户起立
bool CGameWarTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{

    SendSeatInfoToClient();
    return true;
}

// 游戏开始
bool CGameWarTable::OnGameStart()
{
	if (m_pHostRoom == NULL)
	{
		return false;
	}
	if (m_pHostRoom->GetCanGameStart() == false)
	{
		//LOG_DEBUG("start_error - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

		return false;
	}
	LOG_DEBUG("game_start - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

    //LOG_DEBUG("game start - roomid:%d,tableid:%d,m_robotBankerWinPro:%d",m_pHostRoom->GetRoomID(),GetTableID(), m_robotBankerWinPro);

    m_curr_bet_user.clear();
	SetGameState(TABLE_STATE_WAR_PLACE_JETTON);
	m_coolLogic.beginCooling(s_PlaceJettonTime);
    net::msg_war_start_rep gameStart;
    gameStart.set_time_leave(m_coolLogic.getCoolTick());
    SendMsgToAll(&gameStart,net::S2C_MSG_WAR_START);
	OnTableGameStart();
    //OnRobotStandUp();
    return true;
}

uint32 CGameWarTable::GetBankerUID()
{
	return 0;
}

//游戏结束
bool CGameWarTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end - roomid%d,tableid:%d",GetRoomID(),GetTableID());
    switch(reason)
    {
    case GER_NORMAL:		//常规结束
        {
            AddPlayerToBlingLog();
            
			for (uint8 i = 0; i<JETTON_INDEX_COUNT; ++i)
			{
				auto iter = m_userJettonScore[i].begin();
				for (; iter != m_userJettonScore[i].end(); iter++)
				{
					if (iter->second != 0)
					{
						OnAddPlayerJetton(iter->first, iter->second);
					}
				}
			}


			//计算分数
			int64 lBankerWinScore = CalculateScore();
			int64 playerAllWinScore = 0; // 玩家总赢分,红黑大战玩家不能上庄 add by har

			WriteGameInfoLog();
			//结束消息
            net::msg_war_game_end msg;
			msg.set_time_leave(m_coolLogic.getCoolTick());
            for(uint8 i=0;i<SHOW_CARD_COUNT;++i)
			{
                net::msg_cards* pCards = msg.add_table_cards();
                for(uint8 j=0;j<MAX_CARD_COUNT;++j)
				{
                    pCards->add_cards(m_cbTableCardArray[i][j]);
                }
            }
            for(uint8 i=0;i<SHOW_CARD_COUNT;++i)
			{
                msg.add_card_types(m_cbTableCardType[i]);
            }
            for(uint8 i=0;i<JETTON_INDEX_COUNT;++i)
			{
                msg.add_win_multiple(m_winMultiple[i]);
            }
			for (uint8 i = 0; i<JETTON_INDEX_COUNT; ++i)
			{
				msg.add_win_index(m_winIndex[i]);
			}

			LOG_DEBUG("roomid:%d,tableid:%d,lBankerWinScore:%lld,CardType:%d %d,cbWinArea:%d %d %d,lWinMultiple:%d %d %d",
				GetRoomID(), GetTableID(),lBankerWinScore, m_cbTableCardType[0], m_cbTableCardType[1], m_winIndex[0], m_winIndex[1], m_winIndex[2], m_winMultiple[0], m_winMultiple[1], m_winMultiple[2]);


			//发送积分
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				//设置成绩
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					msg.add_player_score(0);
				}
				else
				{
					int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
					lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
					m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
					//uint32 uid = GetPlayerID(wUserIndex);
					msg.add_player_score(m_mpUserWinScore[pPlayer->GetUID()]);
					// add by har
					if (!pPlayer->IsRobot())
						playerAllWinScore += lUserScoreFree; // add by har end
				}
			}

			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				msg.set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_GAME_END);

				//精准控制统计
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
			}

            //发送旁观者积分
            map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
            for(;it != m_mpLookers.end();++it)
            {
                CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)
				{
					LOG_DEBUG("send_player_game_end - roomid:%d,tableid:%d", GetRoomID(), GetTableID());
					continue;
				}
				int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
				lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
				m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
				msg.set_user_score(lUserScoreFree);

				LOG_DEBUG("send_player_game_end - roomid:%d,tableid:%d,uid:%d,score:%lld", GetRoomID(), GetTableID(), pPlayer->GetUID(), lUserScoreFree);

				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_GAME_END);

				//精准控制统计
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);\

				// add by har
				if (!pPlayer->IsRobot())
					playerAllWinScore += lUserScoreFree; // add by har end
			}

            //更新活跃福利数据            
            int64 curr_win = m_mpUserWinScore[m_aw_ctrl_uid];
            UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);

			LOG_DEBUG("OnGameEnd2 - roomid:%d,tableid:%d,playerAllWinScore:%d", GetRoomID(), GetTableID(), playerAllWinScore);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // 必须写在m_userJettonScore清零的前面 add by har

			//同步所有玩家数据到控端
			OnBrcFlushSendAllPlayerInfo();

			//个人下注
			for (uint8 i = 0; i<JETTON_INDEX_COUNT; ++i) {
				m_userJettonScore[i].clear();
			}
			SaveBlingLog();
			OnTableGameEnd();

			return true;
        }break;
    case GER_DISMISS:		//游戏解散
        {
            LOG_ERROR("强制游戏解散");
            for(uint8 i=0;i<GAME_PLAYER;++i)
            {
                if(m_vecPlayers[i].pPlayer != NULL) {
                    LeaveTable(m_vecPlayers[i].pPlayer);
                }
            }
            ResetTable();
            return true;
        }break;
    case GER_NETWORK_ERROR:		//用户强退
    case GER_USER_LEAVE:
    default:
        break;
    }
    //错误断言
    assert(false);
    return false;
}
//玩家进入或离开
void  CGameWarTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer!=NULL) {
		uid = pPlayer->GetUID();
	}
	//int64 lCurScore = GetPlayerCurScore(pPlayer);
 //   LOG_DEBUG("PlayerJoin - roomid:%d,tableid:%d,uid:%d,isJoin:%d,chairID:%d,lCurScore:%lld", GetRoomID(),GetTableID(),uid, isJoin,chairID, lCurScore);
	LOG_DEBUG("player join - uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld", uid, chairID, isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin)
	{
        SendGameScene(pPlayer);
        SendPlayLog(pPlayer);
    }
	else
	{
        for(uint8 i=0;i<JETTON_INDEX_COUNT;++i)
		{
            m_userJettonScore[i].erase(pPlayer->GetUID());
        }
    }        

	//刷新控制界面的玩家数据
	if (!pPlayer->IsRobot())
	{
		OnBrcFlushSendAllPlayerInfo();
	}
}

// 发送场景信息(断线重连)
void    CGameWarTable::SendGameScene(CGamePlayer* pPlayer)
{
	int64 lCurScore = GetPlayerCurScore(pPlayer);
    LOG_DEBUG("send game scene - uid:%d,m_gameState:%d,lCurScore:%lld", pPlayer->GetUID(), m_gameState,lCurScore);
    switch(m_gameState)
    {
    case net::TABLE_STATE_WAR_FREE:          // 空闲状态
        {
            net::msg_war_game_info_free_rep msg;
            msg.set_time_leave(m_coolLogic.getCoolTick());

           
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_WAR_GAME_FREE_INFO);
        }break;
    case net::TABLE_STATE_WAR_PLACE_JETTON:  // 游戏状态
    case net::TABLE_STATE_WAR_GAME_END:      // 结束状态
        {
            net::msg_war_game_info_play_rep msg;
            for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
                msg.add_all_jetton_score(m_allJettonScore[i]);
            }
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_game_status(m_gameState);
            for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
                msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
            }
			if (m_gameState == net::TABLE_STATE_WAR_GAME_END)
			{
				for (uint8 i = 0; i<SHOW_CARD_COUNT; ++i)
				{
					net::msg_cards* pCards = msg.add_table_cards();
					for (uint8 j = 0; j<MAX_CARD_COUNT; ++j)
					{
						pCards->add_cards(m_cbTableCardArray[i][j]);
					}
				}
				for (uint8 i = 0; i<SHOW_CARD_COUNT; ++i)
				{
					msg.add_card_types(m_cbTableCardType[i]);
				}
			}

            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_WAR_GAME_PLAY_INFO);       

			//刷新所有控制界面信息协议---用于断线重连的处理
			if (pPlayer->GetCtrlFlag())
			{
				//刷新百人场桌子状态
				m_brc_table_status_time = m_coolLogic.getCoolTick();
				LOG_DEBUG("BBB  - uid:%d,m_brc_table_status_time:%d", pPlayer->GetUID(), m_brc_table_status_time);

				OnBrcControlFlushTableStatus(pPlayer);

				//发送控制区域信息
				OnBrcControlFlushAreaInfo(pPlayer);
				//发送所有真实玩家列表
				OnBrcControlSendAllPlayerInfo(pPlayer);
				//发送机器人总下注信息
				OnBrcControlSendAllRobotTotalBetInfo();
				//发送真实玩家总下注信息
				OnBrcControlSendAllPlayerTotalBetInfo();
				//发送申请上庄玩家列表
				OnBrcControlFlushAppleList();
			}
        }break;
    default:
        break;
    }
    SendLookerListToClient(pPlayer);
	SendFrontJettonInfo(pPlayer);
}
int64    CGameWarTable::CalcPlayerInfo(uint32 uid,int64 winScore, int64 OnlywinScore,bool isBanker)
{
	//LOG_DEBUG("report to lobby:%d  %lld",uid,winScore);

	int64 userJettonScore = 0;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
	{
		auto it_player_jetton = m_userJettonScore[nAreaIndex].find(uid);
		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
		{
			continue;
		}
		userJettonScore += it_player_jetton->second;
	}
	//if (winScore == 0 && userJettonScore == 0)
	//{
	//	return 0;
	//}
	int64 fee = GetBrcFee(uid, OnlywinScore, true);
	winScore += fee;
	CalcPlayerGameInfoForBrc(uid, winScore, 0, true, false, userJettonScore);

    if(isBanker)
	{

    }
	LOG_DEBUG("report to lobby:%d winScore:%lld OnlywinScore:%lld fee:%lld", uid, winScore, OnlywinScore, fee);
	return fee;
}
// 重置游戏数据
void    CGameWarTable::ResetGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));
    memset(m_winMultiple,0,sizeof(m_winMultiple));

	//个人下注
	for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//玩家成绩	
	m_mpUserWinScore.clear();       
	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));
}
void    CGameWarTable::ClearTurnGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));

	//个人下注
	for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//玩家成绩	
	m_mpUserWinScore.clear();     

    m_curr_bet_user.clear();

	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));     
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameWarTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
		map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[wAreaIndex].find(pPlayer->GetUID());
		if (it_player_jetton == m_userJettonScore[wAreaIndex].end())
			continue;
		if (it_player_jetton->second == 0)
			continue;
		if (!pPlayer->IsRobot())
			isAllRobot = false;
		isAllPlayer = false;
		return;
	}
}

// 写入出牌log
void    CGameWarTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip)
{
    uint8 cardType = m_GameLogic.GetCardType(cardData,cardCount);
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["m"]       = mulip;
    logValue["cardtype"] = cardType;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// 写入加注log
void    CGameWarTable::WriteAddScoreLog(uint32 uid,uint8 area,int64 score)
{
    if(score == 0)
        return;
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["p"]    = area;
    logValue["s"]    = score;

    m_operLog["op"].append(logValue);
}
// 写入最大牌型
void 	CGameWarTable::WriteMaxCardType(uint32 uid,uint8 cardType)
{
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["mt"]   = cardType;
    m_operLog["maxcard"].append(logValue);
}

void    CGameWarTable::WriteGameInfoLog()
{
	// 椅子用户
	for (int nChairID = 0; nChairID<GAME_PLAYER; nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();
		int64 lUserJettonScore = 0;
		Json::Value logValue;
		logValue["uid"] = dwUserID;
		for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lIndexJettonScore = it_player_jetton->second;
			lUserJettonScore += lIndexJettonScore;
			string strindex = CStringUtility::FormatToString("score_%d", nAreaIndex);
			logValue[strindex.c_str()] = lIndexJettonScore;
		}
		if (lUserJettonScore>0)
		{
			logValue["score_a"] = lUserJettonScore;
			m_operLog["userBetInfo"].append(logValue);
		}
	}

	// 旁观用户
	auto it_looker = m_mpLookers.begin();
	for (; it_looker != m_mpLookers.end(); it_looker++)
	{
		CGamePlayer * pPlayer = it_looker->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();
		int64 lUserJettonScore = 0;
		Json::Value logValue;
		logValue["uid"] = dwUserID;
		for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lIndexJettonScore = it_player_jetton->second;
			lUserJettonScore += lIndexJettonScore;
			string strindex = CStringUtility::FormatToString("score_%d", nAreaIndex);
			logValue[strindex.c_str()] = lIndexJettonScore;
		}
		if (lUserJettonScore>0)
		{
			logValue["score_a"] = lUserJettonScore;
			m_operLog["userBetInfo"].append(logValue);
		}
	}
	//写扑克牌

	for (int i = 0; i < SHOW_CARD_COUNT; i++)
	{
		Json::Value logValue;
		logValue["p"] = i;
		logValue["t"] = m_cbTableCardType[i];
		for (int j = 0; j < MAX_CARD_COUNT; j++)
		{
			logValue["c"].append(m_cbTableCardArray[i][j]);
		}
		m_operLog["card"].append(logValue);
	}

	Json::Value logValueMultiple;
	for (int i = 0; i < JETTON_INDEX_COUNT; i++)
	{
		logValueMultiple["m"].append(m_winMultiple[i]);
		logValueMultiple["w"].append(m_winIndex[i]);
	}
	m_operLog["ji"] = logValueMultiple;
}



//加注事件
bool    CGameWarTable:: OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
    //LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
    //效验参数
	if (pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,uid:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetGameState(), cbJettonArea, lJettonScore);

	}


	if(cbJettonArea > JETTON_INDEX_OTHER || lJettonScore <= 0)
	{
		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("jetton error - roomid:%d,tableid:%d,uid:%d,cbJettonArea:%d,lJettonScore:%lld", GetRoomID(), GetTableID(), pPlayer->GetUID(), cbJettonArea, lJettonScore);

		}
		return false;
	}
	if(GetGameState() != net::TABLE_STATE_WAR_PLACE_JETTON)
	{
        net::msg_war_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//发送消息
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_WAR_PLACE_JETTON_REP);
		return true;
	}


	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	if (TableJettonLimmit(pPlayer, lJettonScore, lJettonCount) == false)
	{
		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());

		}

		net::msg_war_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_PLACE_JETTON_REP);
		return true;
	}

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//合法校验
	if(lUserScore < lJettonCount + lJettonScore) 
    {
		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("jetton more - roomid:%d,tableid:%d,uid:%d,cbJettonArea:%d,lJettonScore:%lld,lJettonCount:%lld,lUserScore:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), cbJettonArea, lJettonScore, lJettonCount, lUserScore);

		}

		net::msg_war_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_PLACE_JETTON_REP);

		return true;
	}

	//保存下注
	m_allJettonScore[cbJettonArea] += lJettonScore;
	if (!pPlayer->IsRobot())
	{
		m_playerJettonScore[cbJettonArea] += lJettonScore;
        m_curr_bet_user.insert(pPlayer->GetUID());
	}
	m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;

	//OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);
	RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);
	AddUserBlingLog(pPlayer);
	net::msg_war_place_jetton_rep msg;
	msg.set_jetton_area(cbJettonArea);
	msg.set_jetton_score(lJettonScore);
	msg.set_result(net::RESULT_CODE_SUCCESS);
	//发送消息
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_PLACE_JETTON_REP);

	net::msg_war_place_jetton_broadcast broad;
	broad.set_uid(pPlayer->GetUID());
	broad.set_jetton_area(cbJettonArea);
	broad.set_jetton_score(lJettonScore);
	broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

	SendMsgToAll(&broad, net::S2C_MSG_WAR_PLACE_JETTON_BROADCAST);

	//刷新百人场控制界面的下注信息
	OnBrcControlBetDeal(pPlayer);

    return true;
}

bool    CGameWarTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	//LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_WAR_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//效验参数
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		if (info.score() <= 0 || info.area()>=JETTON_INDEX_COUNT)
		{
			LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);
			return false;
		}
		lTotailScore += info.score();
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	int64 lUserMaxJettonScore = lJettonCount + lTotailScore + GetJettonMin();
	//合法校验
	//if (lUserScore < lUserMaxJettonScore)
	if (GetJettonMin() > GetEnterScore(pPlayer) || (lUserScore < lJettonCount + lTotailScore))
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserScore:%lld,lUserMaxJettonScore:%lld, lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserScore, lUserMaxJettonScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}
	//成功标识
	//bool bPlaceJettonSuccess = true;

	//for (int i = 0; i < msg.info_size(); i++)
	//{
	//	net::bairen_jetton_info info = msg.info(i);

	//	if (info.score() <= 0 || info.area()>=JETTON_INDEX_COUNT)
	//	{
	//		LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());

	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, info.area());
	//	if (lUserMaxJetton < info.score())
	//	{
	//		LOG_DEBUG("error_pressu - uid:%d,i:%d,lUserMaxJetton:%lld,area:%d,score:%lld", pPlayer->GetUID(), i, lUserMaxJetton, info.area(), info.score());

	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//}

	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		m_allJettonScore[info.area()] += info.score();
		if (!pPlayer->IsRobot())
		{
			m_playerJettonScore[info.area()] += info.score();
            m_curr_bet_user.insert(pPlayer->GetUID());
		}
		m_userJettonScore[info.area()][pPlayer->GetUID()] += info.score();

		RecordPlayerBaiRenJettonInfo(pPlayer, info.area(), info.score());

		BYTE cbJettonArea = info.area();
		int64 lJettonScore = info.score();

		net::msg_war_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_PLACE_JETTON_REP);

		net::msg_war_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_WAR_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_WAR_CONTINUOUS_PRESSURE_REP);

	//刷新百人场控制界面的下注信息
	OnBrcControlBetDeal(pPlayer);

	return true;
}

int     CGameWarTable::GetProCardType()
{
	int iArrDispatchCardPro[Pro_Index_MAX] = { 0 };
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_iArrDispatchCardPro[i];
	}

	int iRandNum = g_RandGen.RandRange(0, PRO_DENO_10000);
	int iProIndex = 0;

	for (; iProIndex < Pro_Index_MAX; iProIndex++)
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
	if (iProIndex >= Pro_Index_MAX)
	{
		iProIndex = 0;
	}
	return iProIndex;
}
bool	CGameWarTable::ProbabilityDispatchPokerCard()
{
	bool bIsFlag = true;
	int iArProCardType[SHOW_CARD_COUNT] = { 0 };
	// 先确定每一个人获取的类型
	for (uint16 i = 0; i < SHOW_CARD_COUNT; ++i)
	{
		iArProCardType[i] = Pro_Index_MAX;
		int iProIndex = GetProCardType();
		if (iProIndex < Pro_Index_MAX)
		{
			iArProCardType[i] = iProIndex;
		}
	}
	// 根据类型获取全部的手牌
	for (int index = 0; index < 1024; index++)
	{
		bIsFlag = m_GameLogic.GetCardTypeData(iArProCardType, m_cbTableCardArray);

		BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
		memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
		if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
		{
			break;
		}
	}

	for (uint16 i = 0; i < SHOW_CARD_COUNT; ++i)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,iArProCardType:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X",
			GetRoomID(), GetTableID(), iArProCardType[i], m_cbTableCardArray[i][0], m_cbTableCardArray[i][1], m_cbTableCardArray[i][2]);
	}

	return bIsFlag;
}

void CGameWarTable::GetAreaInfo(WORD iLostIndex, WORD iWinIndex,BYTE cbType[], BYTE cbWinArea[], int64 lWinMultiple[])
{
	for (int iJettonIndex = 0; iJettonIndex < JETTON_INDEX_COUNT; iJettonIndex++)
	{
		if (iJettonIndex == JETTON_INDEX_TIGER || iJettonIndex == JETTON_INDEX_LEOPARD)
		{
			if (iJettonIndex == iWinIndex)
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 1;
			}
		}
		else if (iJettonIndex == JETTON_INDEX_OTHER)
		{
			int switch_on = cbType[iWinIndex];
			switch (switch_on)
			{
			case CT_DOUBLE:
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 1;
			}break;
			case CT_SHUN_ZI:
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 2;
			}break;
			case CT_JIN_HUA:
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 3;
			}break;
			case CT_SHUN_JIN:
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 5;
			}break;
			case CT_BAO_ZI:
			{
				cbWinArea[iJettonIndex] = TRUE;
				lWinMultiple[iJettonIndex] = 10;
			}break;
			}
		}
	}
}

bool    CGameWarTable::SetControlPalyerWin(uint32 control_uid)
{
	int iLoopCount = 99999;
	int iLoopIndex = 0;
	for (; iLoopIndex < iLoopCount; iLoopIndex++)
	{
		int64 lUserLostScore = 0;
		int64 lUserWinScore = 0;

		BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
		int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
		BYTE cbTableCardType[SHOW_CARD_COUNT] = { 0 };

		BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
		memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));


		cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
		cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
		BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

		WORD iLostIndex = 0, iWinIndex = 0;
		if (result == TRUE)
		{
			iWinIndex = JETTON_INDEX_TIGER;
			iLostIndex = JETTON_INDEX_LEOPARD;
		}
		else
		{
			iWinIndex = JETTON_INDEX_LEOPARD;
			iLostIndex = JETTON_INDEX_TIGER;
		}

		GetAreaInfo(iLostIndex, iWinIndex, cbTableCardType, cbWinArea, lWinMultiple);

		// 椅子用户
		//for (int nChairID = 0; nChairID<GAME_PLAYER; nChairID++)
		//{
		//	CGamePlayer * pPlayer = GetPlayer(nChairID);
		//	if (pPlayer == NULL)
		//	{
		//		continue;
		//	}
		//	uint32 dwUserID = pPlayer->GetUID();
		//	if (dwUserID != control_uid)
		//	{
		//		continue;
		//	}
		//	for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
		//	{
		//		auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
		//		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
		//		{
		//			continue;
		//		}
		//		int64 lUserJettonScore = it_player_jetton->second;
		//		if (lUserJettonScore == 0)
		//		{
		//			continue;
		//		}
		//		int64 lTempWinScore = 0;
		//		if (cbWinArea[nAreaIndex] == TRUE)
		//		{
		//			lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
		//			lUserWinScore += lTempWinScore;
		//		}
		//		else
		//		{
		//			lTempWinScore = -lUserJettonScore;
		//			lUserLostScore -= lUserJettonScore;
		//		}
		//	}
		//	lUserWinScore += lUserLostScore;
		//}
		//if (lUserWinScore > 0)
		//{
		//	return true;
		//}
		// 旁观用户
		//auto it_looker = m_mpLookers.find(control_uid);
		//if (it_looker == m_mpLookers.end())
		//{
		//	LOG_DEBUG("contt_player 1 - roomid:%d,tableid:%d,uid:%d,iLoopIndex:%d,", GetRoomID(), GetTableID(), control_uid, iLoopIndex);
		//	return false;
		//}
		//CGamePlayer * pPlayer = it_looker->second;
		//if (pPlayer == NULL)
		//{
		//	LOG_DEBUG("contt_player 2 - roomid:%d,tableid:%d,uid:%d,iLoopIndex:%d,", GetRoomID(), GetTableID(), control_uid, iLoopIndex);
		//	return false;
		//}

		uint32 dwUserID = control_uid;// pPlayer->GetUID();
		for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore == 0)
			{
				continue;
			}
			int64 lTempWinScore = 0;
			if (cbWinArea[nAreaIndex] == TRUE)
			{
				lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
				lUserWinScore += lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
				lUserLostScore -= lUserJettonScore;
			}
		}
		lUserWinScore += lUserLostScore;
		
		if (lUserWinScore > 0)
		{
			return true;
		}
		else
		{
			for (int index = 0; index < 1024; index++)
			{
				m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
				BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
				memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
				if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
				{
					break;
				}
			}
		}
	}
	LOG_DEBUG("contt_player 3 - roomid:%d,tableid:%d,uid:%d,iLoopIndex:%d,", GetRoomID(), GetTableID(), control_uid, iLoopIndex);
	return false;
}
bool    CGameWarTable::SetControlPalyerLost(uint32 control_uid)
{
	int iLoopCount = 1024;
	for (int iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
	{
		int64 lUserLostScore = 0;
		int64 lUserWinScore = 0;

		BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
		int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
		BYTE cbTableCardType[SHOW_CARD_COUNT] = { 0 };

		BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
		memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));


		cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
		cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
		BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

		WORD iLostIndex = 0, iWinIndex = 0;
		if (result == TRUE)
		{
			iWinIndex = JETTON_INDEX_TIGER;
			iLostIndex = JETTON_INDEX_LEOPARD;
		}
		else
		{
			iWinIndex = JETTON_INDEX_LEOPARD;
			iLostIndex = JETTON_INDEX_TIGER;
		}

		GetAreaInfo(iLostIndex, iWinIndex, cbTableCardType, cbWinArea, lWinMultiple);

		// 椅子用户
		//for (int nChairID = 0; nChairID<GAME_PLAYER; nChairID++)
		//{
		//	CGamePlayer * pPlayer = GetPlayer(nChairID);
		//	if (pPlayer == NULL)
		//	{
		//		continue;
		//	}
		//	uint32 dwUserID = pPlayer->GetUID();
		//	if (dwUserID != control_uid)
		//	{
		//		continue;
		//	}
		//	for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
		//	{
		//		auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
		//		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
		//		{
		//			continue;
		//		}
		//		int64 lUserJettonScore = it_player_jetton->second;
		//		if (lUserJettonScore == 0)
		//		{
		//			continue;
		//		}
		//		int64 lTempWinScore = 0;
		//		if (cbWinArea[nAreaIndex] == TRUE)
		//		{
		//			lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
		//			lUserWinScore += lTempWinScore;
		//		}
		//		else
		//		{
		//			lTempWinScore = -lUserJettonScore;
		//			lUserLostScore -= lUserJettonScore;
		//		}
		//	}
		//	lUserWinScore += lUserLostScore;
		//}
		//if (lUserWinScore < 0)
		//{
		//	return true;
		//}
		// 旁观用户
		//auto it_looker = m_mpLookers.find(control_uid);
		//CGamePlayer * pPlayer = it_looker->second;
		//if (pPlayer == NULL)
		//{
		//	return false;
		//}
		uint32 dwUserID = control_uid;// pPlayer->GetUID();
		for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore == 0)
			{
				continue;
			}
			int64 lTempWinScore = 0;
			if (cbWinArea[nAreaIndex] == TRUE)
			{
				lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
				lUserWinScore += lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
				lUserLostScore -= lUserJettonScore;
			}
		}
		lUserWinScore += lUserLostScore;

		if (lUserWinScore < 0)
		{
			return true;
		}
		else
		{
			for (int index = 0; index < 1024; index++)
			{
				m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
				BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
				memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
				if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
				{
					break;
				}
			}
		}
	}
	return false;
}

bool CGameWarTable::SetSystemBrankerWinPlayerScore()
{
	int iLoopCount = 1024;
	for (int iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
	{
		map<uint32, int64> mpUserLostScore;
		map<uint32, int64> mpUserWinScore;

		mpUserLostScore.clear();
		mpUserWinScore.clear();

		int64 lBankerWinScore = 0;

		BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
		int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
		BYTE cbTableCardType[SHOW_CARD_COUNT] = { 0 };

		BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
		memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));

		
		cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
		cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
		BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

		WORD iLostIndex = 0, iWinIndex = 0;
		if (result == TRUE)
		{
			iWinIndex = JETTON_INDEX_TIGER;
			iLostIndex = JETTON_INDEX_LEOPARD;
		}
		else
		{
			iWinIndex = JETTON_INDEX_LEOPARD;
			iLostIndex = JETTON_INDEX_TIGER;
		}

		GetAreaInfo(iLostIndex, iWinIndex, cbTableCardType, cbWinArea, lWinMultiple);

		// 椅子用户
		for (int nChairID = 0; nChairID<GAME_PLAYER; nChairID++)
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
			uint32 dwUserID = pPlayer->GetUID();

			for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
			{
				auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
				if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
				{
					continue;
				}
				int64 lUserJettonScore = it_player_jetton->second;
				if (lUserJettonScore == 0)
				{
					continue;
				}
				int64 lTempWinScore = 0;
				if (cbWinArea[nAreaIndex] == TRUE)
				{
					lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
					mpUserWinScore[dwUserID] += lTempWinScore;
					lBankerWinScore -= lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
					lBankerWinScore += lUserJettonScore;
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 旁观用户
		auto it_looker = m_mpLookers.begin();
		for (; it_looker != m_mpLookers.end(); it_looker++)
		{
			CGamePlayer * pPlayer = it_looker->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (pPlayer->IsRobot())
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();

			for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
			{
				auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
				if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
				{
					continue;
				}
				int64 lUserJettonScore = it_player_jetton->second;
				if (lUserJettonScore == 0)
				{
					continue;
				}

				int64 lTempWinScore = 0;
				if (cbWinArea[nAreaIndex] == TRUE)
				{
					lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
					mpUserWinScore[dwUserID] += lTempWinScore;
					lBankerWinScore -= lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
					lBankerWinScore += lUserJettonScore;
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}
		if (lBankerWinScore > 0)
		{
			return true;
		}
		else
		{
			for (int index = 0; index < 1024; index++)
			{
				m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
				BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
				memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
				if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
				{
					break;
				}
			}
		}
	}

	return false;
}

bool	CGameWarTable::ProgressControlPalyer()
{
	bool bIsFalgControl = false;

	bool bControlPlayerIsJetton = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (int i = 0; i < JETTON_INDEX_COUNT; i++)
		{
			auto it_player_jetton = m_userJettonScore[i].find(control_uid);
			if (it_player_jetton == m_userJettonScore[i].end())
			{
				continue;
			}
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore >0)
			{
				bControlPlayerIsJetton = true;
			}
		}

		if (bControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
		{
			if (control_type == GAME_CONTROL_WIN)
			{
				bIsFalgControl = SetControlPalyerWin(control_uid);
			}
			if (control_type == GAME_CONTROL_LOST)
			{
				bIsFalgControl = SetControlPalyerLost(control_uid);
			}
			if (bIsFalgControl && m_tagControlPalyer.count>0)
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
		}
	}

	LOG_DEBUG("roomid:%d,tableid:%d,control_uid:%d,game_count:%d,control_type:%d,bControlPlayerIsJetton:%d,bIsFalgControl:%d",
		GetRoomID(), GetTableID(), control_uid, game_count, control_type, bControlPlayerIsJetton, bIsFalgControl);


	return bIsFalgControl;
}



// 福利控制
bool CGameWarTable::DosWelfareCtrl()
{
	bool bIsFalgControl = ProgressControlPalyer();
	uint32 sysBankerWinPro = m_pHostRoom->GetSysBankerWinPro();
	bool needChangeCard = g_RandGen.RandRatio(sysBankerWinPro, PRO_DENO_10000);
	bool bIsSysWinScore = false;
	if (bIsFalgControl == false && needChangeCard)
	{
		bIsSysWinScore = SetSystemBrankerWinPlayerScore();
	}

	LOG_DEBUG("roomid:%d,tableid:%d,sysBankerWinPro:%d,bIsFalgControl:%d,needChangeCard:%d,bIsSysWinScore:%d", GetRoomID(), GetTableID(), sysBankerWinPro, bIsFalgControl, needChangeCard, bIsSysWinScore);

	return true;
}
// 非福利控制
bool CGameWarTable::NotWelfareCtrl()
{
	bool bIsFalgControl = ProgressControlPalyer();

	bool needChangeCard = g_RandGen.RandRatio(m_sysBankerWinPro, PRO_DENO_10000);
	bool bIsSysWinScore = false;
	if (bIsFalgControl == false && needChangeCard)
	{
		bIsSysWinScore = SetSystemBrankerWinPlayerScore();
	}

	// add by har
	bool bIsStockControl = false;
	if (!bIsFalgControl && !bIsSysWinScore)
		bIsStockControl = SetStockWinLose(); // add by har end

	//判断是否满足活跃福利
	bool isAwCtrl = false;
	if (!bIsFalgControl && !bIsSysWinScore && !bIsStockControl)
		isAwCtrl = ActiveWelfareCtrl();

	LOG_DEBUG("roomid:%d,tableid:%d,m_sysBankerWinPro:%d,bIsFalgControl:%d,needChangeCard:%d,bIsSysWinScore:%d,isAwCtrl:%d,bIsStockControl:%d",
		GetRoomID(), GetTableID(), m_sysBankerWinPro, bIsFalgControl, needChangeCard, bIsSysWinScore, isAwCtrl, bIsStockControl);

	return true;
}

//bool CGameWarTable::KilledPlayerCtrl()
//{
//	bool bIsFalgControl = ProgressControlPalyer();
//
//	bool needChangeCard = g_RandGen.RandRatio(m_pHostRoom->GetAoSysBankerWinPro(), PRO_DENO_10000);
//	bool bIsSysWinScore = false;
//	if (bIsFalgControl == false && needChangeCard)
//	{
//		bIsSysWinScore = SetSystemBrankerWinPlayerScore();
//	}
//
//	LOG_DEBUG("roomid:%d,tableid:%d,bIsFalgControl:%d,needChangeCard:%d,bIsSysWinScore:%d", GetRoomID(), GetTableID(), bIsFalgControl, needChangeCard, bIsSysWinScore);
//
//	return true;
//}

//发送扑克
bool    CGameWarTable::DispatchTableCard()
{
    //重新洗牌
    //m_GameLogic.RandCardList(m_cbTableCardArray[0],sizeof(m_cbTableCardArray)/sizeof(m_cbTableCardArray[0][0]));
	ProbabilityDispatchPokerCard();

	bool bAreaIsControl = false;
	int newroom = 0;
	bool bHaveNotNovicePlayer = true;
	
	//增加精准控制区域
	bAreaIsControl = OnBrcAreaControl();

	if (!bAreaIsControl && m_pHostRoom != NULL)
	{
		newroom = m_pHostRoom->GetNoviceWelfare();
	}
	if (!bAreaIsControl && newroom == 1)
	{
		bHaveNotNovicePlayer = HaveNotNovicePlayer();
		if (bHaveNotNovicePlayer == false)
		{
			DosWelfareCtrl();
			SetChessWelfare(1);
		}
		else
		{
            NotWelfareCtrl();            			
		}
	}	
	if (!bAreaIsControl && newroom != 1)
	{
        NotWelfareCtrl();        		
	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("dos_wel_ctrl - roomid:%d,tableid:%d,bAreaIsControl:%d, newroom:%d, bHaveNotNovicePlayer:%d,ChessWelfare:%d,GetIsAllRobotOrPlayerJetton:%d",
		GetRoomID(), GetTableID(), bAreaIsControl, newroom, bHaveNotNovicePlayer, GetChessWelfare(), GetIsAllRobotOrPlayerJetton());

	for (uint16 i = 0; i < SHOW_CARD_COUNT; ++i)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X",	GetRoomID(), GetTableID(),  m_cbTableCardArray[i][0], m_cbTableCardArray[i][1], m_cbTableCardArray[i][2]);
	}

    return true;
}
//发送游戏记录
void  CGameWarTable::SendPlayLog(CGamePlayer* pPlayer)
{
    net::msg_war_play_log_rep msg;
    for(uint16 i=0;i<m_vecRecord.size();++i)
    {
        net::war_play_log* plog = msg.add_logs();
        warGameRecord& record = m_vecRecord[i];
        for(uint16 j=0;j<JETTON_INDEX_COUNT;j++){
            plog->add_seats_win(record.wins[j]);
        }
		plog->set_card(record.card);
		plog->set_index(record.index);
    }
    LOG_DEBUG("发送牌局记录:%d",msg.logs_size());
    if(pPlayer != NULL) {
        pPlayer->SendMsgToClient(&msg, net::S2C_MSG_WAR_PLAY_LOG);
    }else{
        SendMsgToAll(&msg,net::S2C_MSG_WAR_PLAY_LOG);
    }
}
//最大下注
int64   CGameWarTable::GetUserMaxJetton(CGamePlayer* pPlayer, BYTE cbJettonArea)
{
	int iTimer = 3;
	//已下注额
	int64 lNowJetton = 0;
	for(int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
	{
        lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}
	//庄家金币
	int64 lBankerScore = 9223372036854775807;
	if (m_pCurBanker != NULL)
	{
		lBankerScore = lBankerScore;
	}

	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
	{
		lBankerScore -= m_allJettonScore[nAreaIndex] * iTimer;
	}

	//个人限制
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton*iTimer)/iTimer;

	//庄家限制
	lMeMaxScore = min(lMeMaxScore,lBankerScore/iTimer);

	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	return (lMeMaxScore);
}


bool    CGameWarTable::IsSetJetton(uint32 uid)
{
	//for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
	//{
	//	if (m_userJettonScore[i][uid] > 0)
	//		return true;
	//}
	for (int i = 0; i < JETTON_INDEX_COUNT; i++)
	{
		auto it_player_jetton = m_userJettonScore[i].find(uid);
		if (it_player_jetton == m_userJettonScore[i].end())
		{
			continue;
		}
		int64 lUserJettonScore = it_player_jetton->second;
		if (lUserJettonScore >0)
		{
			//bControlPlayerIsJetton = true;
			return true;
		}
	}

	for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); i++)
	{
		uint32 temp_uid = 0;
		if (m_chairRobotPlaceJetton[i].pPlayer)
		{
			temp_uid = m_chairRobotPlaceJetton[i].pPlayer->GetUID();
		}
		if (uid == temp_uid) {
			return true;
		}
	}
	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); i++)
	{
		uint32 temp_uid = 0;
		if (m_RobotPlaceJetton[i].pPlayer)
		{
			temp_uid = m_RobotPlaceJetton[i].pPlayer->GetUID();
		}
		if (uid == temp_uid) {
			return true;
		}
	}
    return false;    
}

int64	CGameWarTable::GetAreaMultiple(int nAreaIndex, int iLostIndex, int iWinIndex)
{
	int64 lMultiple = 0;
	if (nAreaIndex == JETTON_INDEX_TIGER || nAreaIndex == JETTON_INDEX_LEOPARD)
	{
		if (nAreaIndex == iWinIndex)
		{
			lMultiple = 1;
		}
	}
	else if (nAreaIndex == JETTON_INDEX_OTHER)
	{
		int switch_on = m_cbTableCardType[iWinIndex];
		switch (switch_on)
		{
			case CT_DOUBLE:
			{
				lMultiple = 1;
			}break;
			case CT_SHUN_ZI:
			{
				lMultiple = 2;
			}break;
			case CT_JIN_HUA:
			{
				lMultiple = 3;
			}break;
			case CT_SHUN_JIN:
			{
				lMultiple = 5;
			}break;
			case CT_BAO_ZI:
			{
				lMultiple = 15;
			}break;
		}
	}
	return lMultiple;
}

//计算得分
int64   CGameWarTable::CalculateScore()
{
	map<uint32, int64> mpUserLostScore;
	mpUserLostScore.clear();
	m_mpUserWinScore.clear();
	m_mpWinScoreForFee.clear();
	memset(m_winMultiple, 0, sizeof(m_winMultiple));
	
	int64 lBankerWinScore = 0;

	BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
	memset(cbWinArea, 0, sizeof(cbWinArea));
	int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
	memset(lWinMultiple, 0, sizeof(lWinMultiple));
	BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = {0};
	memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));
	m_cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
	m_cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
	BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
	WORD iLostIndex = 0, iWinIndex = 0;
	if (result == TRUE)
	{
		iWinIndex = JETTON_INDEX_TIGER;
		iLostIndex = JETTON_INDEX_LEOPARD;
	}
	else
	{
		iWinIndex = JETTON_INDEX_LEOPARD;
		iLostIndex = JETTON_INDEX_TIGER;
	}

	GetAreaInfo(iLostIndex, iWinIndex, m_cbTableCardType, cbWinArea, lWinMultiple);

	LOG_DEBUG("roomid:%d,tableid:%d,iLostIndex:%d, iWinIndex:%d,result:%d,CardType:%d %d,cbWinArea:%d %d %d,lWinMultiple:%lld %lld %lld",
		GetRoomID(),GetTableID(),iLostIndex, iWinIndex, result, m_cbTableCardType[0], m_cbTableCardType[1], cbWinArea[0], cbWinArea[1], cbWinArea[2], lWinMultiple[0], lWinMultiple[1], lWinMultiple[2]);

	for (int i = 0; i < JETTON_INDEX_COUNT; i++)
	{
		m_winMultiple[i] = lWinMultiple[i];
		m_winIndex[i] = cbWinArea[i];
	}
	memcpy(m_record.wins, cbWinArea, sizeof(m_record.wins));
	m_record.card = m_cbTableCardType[iWinIndex];
	m_record.index = m_iGameCount;
	if (m_record.index == 0)
	{
		m_vecRecord.clear();
	}
	//memcpy(m_record.card, cbTableCardArray, sizeof(m_record.card));
	m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() >= 72)
	{
		m_vecRecord.erase(m_vecRecord.begin());
	}

	//m_vecGamePlayRecord.push_back(m_record);
	//if (m_vecGamePlayRecord.size() >= 72) {
	//	m_vecGamePlayRecord.erase(m_vecGamePlayRecord.begin());
	//}
	SendGameEndLogInfo();

	// 椅子用户
	for (int nChairID = 0; nChairID<GAME_PLAYER; nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();

		for (int nAreaIndex = 0; nAreaIndex<JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore == 0)
			{
				continue;
			}
			int64 lTempWinScore = 0;
			if (cbWinArea[nAreaIndex] == TRUE)
			{
				lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
				m_mpUserWinScore[dwUserID] += lTempWinScore;
				lBankerWinScore -= lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
				mpUserLostScore[dwUserID] -= lUserJettonScore;
				lBankerWinScore += lUserJettonScore;
			}
		}
		m_mpWinScoreForFee[dwUserID] = m_mpUserWinScore[dwUserID];
		m_mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
	}

	// 旁观用户
	auto it_looker = m_mpLookers.begin();
	for (; it_looker != m_mpLookers.end(); it_looker++)
	{
		CGamePlayer * pPlayer = it_looker->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();

		for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
		{
			auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			{
				continue;
			}
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore == 0)
			{
				continue;
			}

			int64 lTempWinScore = 0;
			if (cbWinArea[nAreaIndex] == TRUE)
			{
				lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
				m_mpUserWinScore[dwUserID] += lTempWinScore;
				lBankerWinScore -= lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
				mpUserLostScore[dwUserID] -= lUserJettonScore;
				lBankerWinScore += lUserJettonScore;
			}
		}
		m_mpWinScoreForFee[dwUserID] = m_mpUserWinScore[dwUserID];
		m_mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];

		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("score result looker - uid:%d,mroomid,tableid:%d,_mpUserWinScore:%lld", dwUserID, GetRoomID(), GetTableID(), m_mpUserWinScore[dwUserID]);

		}

	}
	return lBankerWinScore;
}



//申请条件
int64   CGameWarTable::GetApplyBankerCondition()
{
    return GetBaseScore();
}
int64   CGameWarTable::GetApplyBankerConditionLimit()
{
    return GetBaseScore()*20;
}


void    CGameWarTable::OnRobotOper()
{
    //LOG_DEBUG("robot place jetton");
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
		uint8 area = GetRobotJettonArea();
        if(g_RandGen.RandRatio(50,PRO_DENO_100))
            continue;

		int64 minJetton = GetRobotJettonScore(pPlayer, area);
		if (minJetton == 0)
		{
			continue;
		}
		if (!OnUserPlaceJetton(pPlayer, area, minJetton))
		{
			break;
		}
    }
    for(uint32 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
		uint8 area = GetRobotJettonArea();
		if(g_RandGen.RandRatio(85,PRO_DENO_100))
            continue;
		int64 minJetton = GetRobotJettonScore(pPlayer, area);
		if (minJetton == 0)
		{
			continue;
		}
        if(!OnUserPlaceJetton(pPlayer,area,minJetton))
            break;
    }

}


int64 CGameWarTable::GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area)
{
	int64 lUserRealJetton = 100;
	int64 lUserMinJetton = 100;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, area);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);
	//LOG_DEBUG("uid:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", pPlayer->GetUID(), lUserMinJetton, lUserMaxJetton, lUserRealJetton);

	if (lUserCurJetton < 2000)
	{
		lUserRealJetton = 0;
	}
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000)
	{
		if (g_RandGen.RandRatio(35, PRO_DENO_100))
		{
			lUserRealJetton = 100;
		}
		else if (g_RandGen.RandRatio(57, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else
		{
			lUserRealJetton = 5000;
		}
	}
	else if (lUserCurJetton >= 50000 && lUserCurJetton < 200000)
	{
		if (g_RandGen.RandRatio(15, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(47, PRO_DENO_100))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(35, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else
		{
			lUserRealJetton = 50000;
		}
	}
	else if (lUserCurJetton >= 200000 && lUserCurJetton < 2000000)
	{
		if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(5500, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else
		{
			lUserRealJetton = 50000;
		}
	}
	else if (lUserCurJetton >= 2000000)
	{
		if (g_RandGen.RandRatio(500, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else
		{
			lUserRealJetton = 50000;
		}
	}
	else
	{
		lUserRealJetton = 100;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 50000)
	{
		lUserRealJetton = 10000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 10000)
	{
		lUserRealJetton = 5000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 5000)
	{
		lUserRealJetton = 1000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 1000)
	{
		lUserRealJetton = 100;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100)
	{
		lUserRealJetton = 0;
	}
	if (lUserRealJetton < lUserMinJetton)
	{
		lUserRealJetton = 0;
	}
	return lUserRealJetton;
}


int64 CGameWarTable::GetRobotJettonScoreRand(CGamePlayer* pPlayer, uint8 area)
{
	int64 lUserRealJetton = 100;
	int switch_on = g_RandGen.RandRange(0, 3);
	switch (switch_on)
	{
	case 0:
	{
		lUserRealJetton = 100;
	}break;
	case 1:
	{
		lUserRealJetton = 1000;
	}break;
	case 2:
	{
		lUserRealJetton = 5000;
	}break;
	case 3:
	{
		lUserRealJetton = 10000;
	}break;
	default:
		break;
	}

	return lUserRealJetton;
}
void    CGameWarTable::OnRobotStandUp()
{
    // 保持一两个座位给玩家    
    vector<uint16> emptyChairs;
    vector<uint16> robotChairs;
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL){
            emptyChairs.push_back(i);
            continue;
        }
        if(pPlayer->IsRobot()){
            robotChairs.push_back(i);
        }        
    }
    
    if(GetChairPlayerNum() > m_robotChairSize && robotChairs.size() > 0)// 机器人站起
    {
        uint16 chairID = robotChairs[g_RandGen.RandUInt()%robotChairs.size()];
        CGamePlayer* pPlayer = GetPlayer(chairID);
        if(pPlayer != NULL && pPlayer->IsRobot() && CanStandUp(pPlayer))
        {
            PlayerSitDownStandUp(pPlayer,false,chairID);
            return;
        }                          
    }
    if(GetChairPlayerNum() < m_robotChairSize && emptyChairs.size() > 0)//机器人坐下
    {
        map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
        for(;it != m_mpLookers.end();++it)
        {
            CGamePlayer* pPlayer = it->second;
            if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
                continue;

            uint16 chairID = emptyChairs[g_RandGen.RandUInt()%emptyChairs.size()];
            if(CanSitDown(pPlayer,chairID)){
                PlayerSitDownStandUp(pPlayer,true,chairID);
                return;
            }      
        }            
    }
}

/*void    CGameWarTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots)
{
	robots.clear();
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
		robots.push_back(pPlayer);
	}

	for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
	{
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
		robots.push_back(pPlayer);
	}

	//LOG_DEBUG("robots.size:%d", robots.size());
}*/



void    CGameWarTable::AddPlayerToBlingLog()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)
            continue;
        for(uint8 i=0;i<JETTON_INDEX_COUNT;++i){
            if(m_userJettonScore[i][pPlayer->GetUID()] > 0){
                AddUserBlingLog(pPlayer);
                break;
            }
        }           
    }
    AddUserBlingLog(m_pCurBanker);
}







bool    CGameWarTable::IsInTableRobot(uint32 uid, CGamePlayer * pPlayer)
{
	for (uint32 i = 0; i<GAME_PLAYER; ++i)
	{
		if (pPlayer != NULL && pPlayer == GetPlayer(i) && pPlayer->GetUID() == uid)
		{
			return true;
		}
	}

	auto iter_player = m_mpLookers.find(uid);
	if (iter_player != m_mpLookers.end())
	{
		if (pPlayer != NULL && pPlayer == iter_player->second && pPlayer->GetUID() == iter_player->first)
		{
			return true;
		}
	}

	return false;
}


bool CGameWarTable::OnChairRobotJetton()
{
	if (m_bIsChairRobotAlreadyJetton)
	{
		return false;
	}
	m_bIsChairRobotAlreadyJetton = true;
	m_chairRobotPlaceJetton.clear();
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(5, 9);
		uint8 cbJettonArea = GetRobotJettonArea();
		uint8 cbOldJettonArea = cbJettonArea;
		int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
		if (lUserRealJetton == 0)
		{
			continue;
		}
		int iJettonTypeCount = g_RandGen.RandRange(1, 2);
		int iJettonStartCount = g_RandGen.RandRange(2, 3);
		int64 lOldRealJetton = -1;
		int64 iJettonOldTime = -1;

		bool bIsContinuouslyJetton = false;
		int iPreRatio = g_RandGen.RandRange(5, 10);
		if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100))
		{
			bIsContinuouslyJetton = true;

			if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			{
				iJettonCount = g_RandGen.RandRange(5, 18);
			}
		}
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			//if (g_RandGen.RandRatio(95, PRO_DENO_100))
			//{
			//	continue;
			//}

			if (bIsContinuouslyJetton == false)
			{
				//cbJettonArea = GetRobotJettonArea();

				if (g_RandGen.RandRatio(10, PRO_DENO_100))
				{
					cbJettonArea = JETTON_INDEX_OTHER;
				}
				else
				{
					cbJettonArea = cbOldJettonArea;
				}


				lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
				if (lOldRealJetton == -1)
				{
					lOldRealJetton = lUserRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				{
					lUserRealJetton = lOldRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex)
				{
					lUserRealJetton = lUserRealJetton;
					lOldRealJetton = lUserRealJetton;
				}
				else
				{
					lUserRealJetton = lOldRealJetton;
				}
			}
			if (lUserRealJetton == 0)
			{
				continue;
			}
			if (cbJettonArea == JETTON_INDEX_OTHER)
			{
				if (lUserRealJetton == 50000 || lUserRealJetton == 200000)
				{
					lUserRealJetton = GetRobotJettonScoreRand(pPlayer, cbJettonArea);
				}
			}
			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			int64 uRemainTime = m_coolLogic.getCoolTick();
			int64 passtick = m_coolLogic.getPassTick();
			int64 uMaxDelayTime = s_PlaceJettonTime;

			if (iJettonOldTime >= uMaxDelayTime - 500)
			{
				iJettonOldTime = g_RandGen.RandRange(100, 5000);
			}
			if (iJettonOldTime == -1)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
				iJettonOldTime = robotPlaceJetton.time;
				if (bIsContinuouslyJetton == true)
				{
					robotPlaceJetton.time = g_RandGen.RandRange(100, 4000);
					iJettonOldTime = robotPlaceJetton.time;
				}
			}
			else
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 100;
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			{
				continue;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;
			m_chairRobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}
	LOG_DEBUG("chair_robot_jetton - roomid:%d,tableid:%d,m_chairRobotPlaceJetton.size:%d", GetRoomID(), GetTableID(), m_chairRobotPlaceJetton.size());

	return true;
}

void	CGameWarTable::OnChairRobotPlaceJetton()
{
	if (m_chairRobotPlaceJetton.size() == 0)
	{
		return;
	}
	vector<tagRobotPlaceJetton>	vecRobotPlaceJetton;
	for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); i++)
	{
		if (m_chairRobotPlaceJetton.size() == 0)
		{
			return;
		}
		tagRobotPlaceJetton robotPlaceJetton = m_chairRobotPlaceJetton[i];
		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;

		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_chairRobotPlaceJetton.size() == 0)
		{
			return;
		}
		int64 passtick = m_coolLogic.getPassTick();
		int64 uMaxDelayTime = s_PlaceJettonTime;
		//passtick = passtick / 1000;
		bool bflag = false;
		bool bIsInTable = false;
		bool bIsJetton = false;
		for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
		{
			if (vecRobotPlaceJetton[i].pPlayer != NULL && vecRobotPlaceJetton[i].pPlayer->GetUID() == robotPlaceJetton.pPlayer->GetUID())
			{
				bIsJetton = true;
			}
		}

		if (passtick > robotPlaceJetton.time && uMaxDelayTime - passtick > 800 && m_chairRobotPlaceJetton[i].bflag == false && bIsJetton == false)
		{
			bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			if (bIsInTable)
			{
				bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_chairRobotPlaceJetton[i].bflag = bflag;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
			//LOG_DEBUG("chair_robot_jetton - roomid:%d,tableid:%d,uid:%d,time:%d,passtick:%d,bIsInTable:%d,bIsJetton:%d,bflag:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), robotPlaceJetton.time, passtick, bIsInTable, bIsJetton, bflag);
		}
		if (m_chairRobotPlaceJetton[i].bflag == true)
		{
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}

	for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
	{
		auto iter_begin = m_chairRobotPlaceJetton.begin();
		for (; iter_begin != m_chairRobotPlaceJetton.end(); iter_begin++)
		{
			if (iter_begin->bflag == true)
			{
				m_chairRobotPlaceJetton.erase(iter_begin);
				break;
			}
		}
	}
}

bool CGameWarTable::OnRobotJetton()
{
	//return false;
	if (m_bIsRobotAlreadyJetton)
	{
		return false;
	}
	m_bIsRobotAlreadyJetton = true;
	m_RobotPlaceJetton.clear();
	int64 iJettonOldTime = -1;
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
			continue;
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(1, 6);
		uint8 cbJettonArea = GetRobotJettonArea();
		uint8 cbOldJettonArea = cbJettonArea;
		int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
		if (lUserRealJetton == 0)
		{
			continue;
		}
		int iJettonTypeCount = g_RandGen.RandRange(1, 2);
		int iJettonStartCount = g_RandGen.RandRange(2, 3);
		int64 lOldRealJetton = -1;

		bool bIsContinuouslyJetton = false;
		int iPreRatio = g_RandGen.RandRange(5, 10);
		if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100))
		{
			bIsContinuouslyJetton = true;

			if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			{
				iJettonCount = g_RandGen.RandRange(5, 18);
			}
		}

		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			if (bIsContinuouslyJetton == false)
			{
				//cbJettonArea = GetRobotJettonArea();

				if (g_RandGen.RandRatio(10, PRO_DENO_100))
				{
					cbJettonArea = JETTON_INDEX_OTHER;
				}
				else
				{
					cbJettonArea = cbOldJettonArea;
				}

				lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
				if (lOldRealJetton == -1)
				{
					lOldRealJetton = lUserRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				{
					lUserRealJetton = lOldRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex)
				{
					lUserRealJetton = lUserRealJetton;
					lOldRealJetton = lUserRealJetton;
				}
				else
				{
					lUserRealJetton = lOldRealJetton;
				}
			}
			if (lUserRealJetton == 0)
			{
				continue;
			}
			if (cbJettonArea == JETTON_INDEX_OTHER)
			{
				if (lUserRealJetton == 50000 || lUserRealJetton == 200000)
				{
					lUserRealJetton = GetRobotJettonScoreRand(pPlayer, cbJettonArea);
				}
			}


			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			int64 uRemainTime = m_coolLogic.getCoolTick();

			int64 passtick = m_coolLogic.getPassTick();
			int64 uMaxDelayTime = s_PlaceJettonTime;

			if (iJettonOldTime >= uMaxDelayTime - 500)
			{
				iJettonOldTime = g_RandGen.RandRange(100, 5000);
			}
			if (iJettonOldTime == -1)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, 5000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			else
			{
				robotPlaceJetton.time = iJettonOldTime + g_RandGen.RandRange(100, 2000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 100;
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			{
				continue;
			}

			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;
			m_RobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}

	return true;
}


void	CGameWarTable::OnRobotPlaceJetton()
{
	if (m_RobotPlaceJetton.size() == 0)
	{
		return;
	}
	vector<tagRobotPlaceJetton>	vecRobotPlaceJetton;

	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); i++)
	{
		if (m_RobotPlaceJetton.size() == 0)
		{
			return;
		}
		tagRobotPlaceJetton robotPlaceJetton = m_RobotPlaceJetton[i];
		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;

		if (pPlayer == NULL) {
			continue;
		}
		if (m_RobotPlaceJetton.size() == 0)
		{
			return;
		}
		int64 passtick = m_coolLogic.getPassTick();
		int64 uMaxDelayTime = s_PlaceJettonTime;
		//passtick = passtick / 1000;
		bool bflag = false;
		bool bIsInTable = false;
		bool bIsJetton = false;
		for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
		{
			if (vecRobotPlaceJetton[i].pPlayer != NULL && vecRobotPlaceJetton[i].pPlayer->GetUID() == robotPlaceJetton.pPlayer->GetUID())
			{
				bIsJetton = true;
			}
		}
		if (robotPlaceJetton.time <= passtick && uMaxDelayTime - passtick > 500 && m_RobotPlaceJetton[i].bflag == false && bIsJetton == false)
		{
			bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			if (bIsInTable)
			{
				bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_RobotPlaceJetton[i].bflag = bflag;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
			//LOG_DEBUG("look_robot_jetton - roomid:%d,tableid:%d,uid:%d,time:%d,passtick:%d,bIsInTable:%d,bIsJetton:%d,bflag:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), robotPlaceJetton.time, passtick, bIsInTable, bIsJetton, bflag);
		}
	}

	for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
	{
		auto iter_begin = m_RobotPlaceJetton.begin();
		for (; iter_begin != m_RobotPlaceJetton.end(); iter_begin++)
		{
			if (iter_begin->bflag == true)
			{
				m_RobotPlaceJetton.erase(iter_begin);
				break;
			}
		}
	}
}

uint8 CGameWarTable::GetRobotJettonArea()
{
	uint8 cbJettonArea = g_RandGen.RandRange(JETTON_INDEX_TIGER, JETTON_INDEX_OTHER);

	//if (m_cbFrontRobotJettonArea == JETTON_INDEX_LEOPARD)
	//{
	//	if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//	{
	//		cbJettonArea = JETTON_INDEX_TIGER;
	//	}
	//	else if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//	{
	//		cbJettonArea = JETTON_INDEX_LEOPARD;
	//	}
	//	else
	//	{
	//		cbJettonArea = JETTON_INDEX_OTHER;
	//	}
	//}
	//else if (m_cbFrontRobotJettonArea == JETTON_INDEX_TIGER)
	//{
	//	if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//	{
	//		cbJettonArea = JETTON_INDEX_LEOPARD;
	//	}
	//	else if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//	{
	//		cbJettonArea = JETTON_INDEX_TIGER;
	//	}
	//	else
	//	{
	//		cbJettonArea = JETTON_INDEX_OTHER;
	//	}
	//}
	//else
	//{
	//	if (g_RandGen.RandRatio(5000, PRO_DENO_10000))
	//	{
	//		if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//		{
	//			cbJettonArea = JETTON_INDEX_TIGER;
	//		}
	//		else if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//		{
	//			cbJettonArea = JETTON_INDEX_LEOPARD;
	//		}
	//		else
	//		{
	//			cbJettonArea = JETTON_INDEX_OTHER;
	//		}
	//	}
	//	else
	//	{
	//		if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//		{
	//			cbJettonArea = JETTON_INDEX_LEOPARD;
	//		}
	//		else if (g_RandGen.RandRatio(8000, PRO_DENO_10000))
	//		{
	//			cbJettonArea = JETTON_INDEX_TIGER;
	//		}
	//		else
	//		{
	//			cbJettonArea = JETTON_INDEX_OTHER;
	//		}
	//	}
	//}

	if (g_RandGen.RandRatio(50, PRO_DENO_100))
	{
		cbJettonArea = JETTON_INDEX_TIGER;
	}
	else
	{
		cbJettonArea = JETTON_INDEX_LEOPARD;
	}

	if (g_RandGen.RandRatio(5, PRO_DENO_100))
	{
		cbJettonArea = JETTON_INDEX_OTHER;
	}

	if (cbJettonArea > JETTON_INDEX_OTHER)
	{
		cbJettonArea = g_RandGen.RandRange(JETTON_INDEX_TIGER, JETTON_INDEX_OTHER);
	}
	m_cbFrontRobotJettonArea = cbJettonArea;
	return cbJettonArea;
}

bool CGameWarTable::ActiveWelfareCtrl()
{
    LOG_DEBUG("enter ActiveWelfareCtrl ctrl player count:%d.", m_aw_ctrl_player_list.size());

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
			break;
		}
		LOG_DEBUG("The current player in config rate - control_uid:%d tmp:%d probability:%d", control_uid, tmp, probability)

        if (SetControlPalyerWinForAW(control_uid, iter->max_win))
        {
            LOG_DEBUG("search success current player - uid:%d max_win:%d", control_uid, iter->max_win);
            m_aw_ctrl_uid = control_uid;   //设置当前活跃福利所控的玩家ID
            return true;
        }
        else
        {
			break;
        }
    }
    LOG_DEBUG("the all ActiveWelfareCtrl player is search fail. return false.");
    return false;
}

bool    CGameWarTable::SetControlPalyerWinForAW(uint32 control_uid, int64 max_win)
{
    LOG_DEBUG("enter SetControlPalyerWinForAW function. control_uid:%d max_win:%lld", control_uid, max_win);
    int iLoopCount = 99999;
    int iLoopIndex = 0;
    for (; iLoopIndex < iLoopCount; iLoopIndex++)
    {
        int64 lUserLostScore = 0;
        int64 lUserWinScore = 0;

        BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
        int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
        BYTE cbTableCardType[SHOW_CARD_COUNT] = { 0 };

        BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
        memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));


        cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
        cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
        BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

        WORD iLostIndex = 0, iWinIndex = 0;
        if (result == TRUE)
        {
            iWinIndex = JETTON_INDEX_TIGER;
            iLostIndex = JETTON_INDEX_LEOPARD;
        }
        else
        {
            iWinIndex = JETTON_INDEX_LEOPARD;
            iLostIndex = JETTON_INDEX_TIGER;
        }

        GetAreaInfo(iLostIndex, iWinIndex, cbTableCardType, cbWinArea, lWinMultiple);
        
        uint32 dwUserID = control_uid;// pPlayer->GetUID();
        for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; nAreaIndex++)
        {
            auto it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
            if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
            {
                continue;
            }
            int64 lUserJettonScore = it_player_jetton->second;
            if (lUserJettonScore == 0)
            {
                continue;
            }
            int64 lTempWinScore = 0;
            if (cbWinArea[nAreaIndex] == TRUE)
            {
                lTempWinScore = (lUserJettonScore * lWinMultiple[nAreaIndex]);
                lUserWinScore += lTempWinScore;
            }
            else
            {
                lTempWinScore = -lUserJettonScore;
                lUserLostScore -= lUserJettonScore;
            }
        }
        lUserWinScore += lUserLostScore;

        if (lUserWinScore > 0 && lUserWinScore <= max_win)
        {
            LOG_DEBUG("search find success. control_uid:%d lUserWinScore:%lld max_win:%lld", control_uid, lUserWinScore, max_win);
            return true;
        }
        else
        {
            for (int index = 0; index < 1024; index++)
            {
                m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
                BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
                memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
                if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
                {
                    break;
                }
            }
        }
    }    
    return false;
}

void CGameWarTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_war_play_log_rep* pplay = pInfo->mutable_war();
	for (uint16 i = 0; i<m_vecRecord.size(); ++i)
	{
		net::war_play_log* plog = pplay->add_logs();
		warGameRecord& record = m_vecRecord[i];
		for (uint16 j = 0; j<JETTON_INDEX_COUNT; j++) {
			plog->add_seats_win(record.wins[j]);
		}
		plog->set_card(record.card);
		plog->set_index(record.index);
	}
}

void CGameWarTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_war_play_log_rep* pplay = pInfo->mutable_war();
	net::war_play_log* plog = pplay->add_logs();
	warGameRecord& record = m_record;
	for (uint16 j = 0; j<JETTON_INDEX_COUNT; j++) {
		plog->add_seats_win(record.wins[j]);
	}
	plog->set_card(record.card);
	plog->set_index(record.index);
}

void CGameWarTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer)
{
	if (!pPlayer)
	{
		return;
	}
	LOG_DEBUG(" ++++ send brc control all true player info list uid:%d.", pPlayer->GetUID());

	net::msg_brc_control_all_player_bet_info rep;

	//计算座位玩家
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//获取用户
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		LOG_DEBUG(" ++++ uid:%d name:%s", tmp_pPlayer->GetUID(), tmp_pPlayer->GetPlayerName());

		//统计信息
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	//计算旁观玩家
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		LOG_DEBUG(" ++++ uid:%d name:%s", tmp_pPlayer->GetUID(), tmp_pPlayer->GetPlayerName());

		//统计信息
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

void CGameWarTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer)
{
	if (!pPlayer)
	{
		return;
	}

	if (pPlayer->IsRobot())   return;

	uint32 uid = pPlayer->GetUID();
	LOG_DEBUG("notice brc control single true player bet info uid:%d.", uid);

	net::msg_brc_control_single_player_bet_info rep;

	//计算座位玩家
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//获取用户
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL) continue;
		if (uid == tmp_pPlayer->GetUID())
		{
			net::brc_control_player_bet_info *info = rep.mutable_player_bet_info();
			info->set_uid(uid);
			info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			info->set_name(pPlayer->GetPlayerName());

			//统计信息
			auto iter = m_mpPlayerResultInfo.find(uid);
			if (iter != m_mpPlayerResultInfo.end())
			{
				info->set_curr_day_win(iter->second.day_win_coin);
				info->set_win_number(iter->second.win);
				info->set_lose_number(iter->second.lose);
				info->set_total_win(iter->second.total_win_coin);
			}
			//下注信息	
			uint64 total_bet = 0;
			for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
			{
				info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
				total_bet += m_userJettonScore[wAreaIndex][uid];
			}
			info->set_total_bet(total_bet);
			info->set_ismaster(IsBrcControlPlayer(uid));
			break;
		}
	}

	//计算旁观玩家
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)continue;
		if (uid == tmp_pPlayer->GetUID())
		{
			net::brc_control_player_bet_info *info = rep.mutable_player_bet_info();
			info->set_uid(uid);
			info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			info->set_name(pPlayer->GetPlayerName());

			//统计信息
			auto iter = m_mpPlayerResultInfo.find(uid);
			if (iter != m_mpPlayerResultInfo.end())
			{
				info->set_curr_day_win(iter->second.day_win_coin);
				info->set_win_number(iter->second.win);
				info->set_lose_number(iter->second.lose);
				info->set_total_win(iter->second.total_win_coin);
			}
			//下注信息	
			uint64 total_bet = 0;
			for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
			{
				info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
				total_bet += m_userJettonScore[wAreaIndex][uid];
			}
			info->set_total_bet(total_bet);
			info->set_ismaster(IsBrcControlPlayer(uid));
			break;
		}
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_SINGLE_PLAYER_BET_INFO);
	}
}

void CGameWarTable::OnBrcControlSendAllRobotTotalBetInfo()
{
	LOG_DEBUG("notice brc control all robot totol bet info.");

	net::msg_brc_control_total_robot_bet_info rep;
	for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_allJettonScore[wAreaIndex] - m_playerJettonScore[wAreaIndex]);
		LOG_DEBUG("wAreaIndex:%d m_allJettonScore[%d]:%lld m_playerJettonScore[%d]:%lld", wAreaIndex, wAreaIndex, m_allJettonScore[wAreaIndex], wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_ROBOT_BET_INFO);
	}
}

void CGameWarTable::OnBrcControlSendAllPlayerTotalBetInfo()
{
	LOG_DEBUG("notice brc control all player totol bet info.");

	net::msg_brc_control_total_player_bet_info rep;
	for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_playerJettonScore[wAreaIndex]);
		LOG_DEBUG("m_playerJettonScore[%d]:%lld", wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_PLAYER_BET_INFO);
	}
}

bool CGameWarTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer)
{
	if (!pPlayer)
		return false;

	LOG_DEBUG("brc control enter control interface. uid:%d", pPlayer->GetUID());

	bool ret = OnBrcControlPlayerEnterInterface(pPlayer);
	if (ret)
	{
		//刷新百人场桌子状态
		m_brc_table_status_time = m_coolLogic.getCoolTick();
		OnBrcControlFlushTableStatus(pPlayer);

		//发送所有真实玩家列表
		OnBrcControlSendAllPlayerInfo(pPlayer);
		//发送机器人总下注信息
		OnBrcControlSendAllRobotTotalBetInfo();
		//发送真实玩家总下注信息
		OnBrcControlSendAllPlayerTotalBetInfo();
		//发送申请上庄玩家列表
		//OnBrcControlFlushAppleList();

		return true;
	}
	return false;
}

void CGameWarTable::OnBrcControlBetDeal(CGamePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	LOG_DEBUG("brc control bet deal. uid:%d", pPlayer->GetUID());
	if (pPlayer->IsRobot())
	{
		//发送机器人总下注信息
		OnBrcControlSendAllRobotTotalBetInfo();
	}
	else
	{
		//通知单个玩家下注信息
		OnBrcControlNoticeSinglePlayerInfo(pPlayer);
		//发送真实玩家总下注信息
		OnBrcControlSendAllPlayerTotalBetInfo();
	}
}

bool CGameWarTable::OnBrcAreaControl()
{
	LOG_DEBUG("brc area control.");

	if (m_real_control_uid == 0)
	{
		LOG_DEBUG("brc area control the control uid is zero.");
		return false;
	}

	//获取当前控制区域
	uint8 ctrl_area_type = AREA_MAX;	//控制区域
	uint8 ctrl_card_type = AREA_MAX;	//控制牌型
	for (uint8 i = 0; i < AREA_MAX - 2; ++i)
	{
		if (m_req_control_area[i] == 1)
		{
			ctrl_card_type = i;
			break;
		}
	}
	for (uint8 i = AREA_MAX - 2; i < AREA_MAX; ++i)
	{
		if (m_req_control_area[i] == 1)
		{
			ctrl_area_type = i;
			break;
		}
	}
	if (ctrl_area_type == AREA_MAX && ctrl_card_type == AREA_MAX)
	{
		LOG_DEBUG("brc area control the ctrl_area is error. ctrl_area_type:%d ctrl_card_type:%d", ctrl_area_type, ctrl_card_type);
		return false;
	}

	int iLoopCount = 5000;
	for (int iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
	{
		int64 lUserLostScore = 0;
		int64 lUserWinScore = 0;

		BYTE cbWinArea[JETTON_INDEX_COUNT] = { 0 };
		int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
		BYTE cbTableCardType[SHOW_CARD_COUNT] = { 0 };

		BYTE cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
		memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));

		cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
		cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
		BYTE result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

		WORD iLostIndex = 0, iWinIndex = 0;
		if (result == TRUE)
		{
			iWinIndex = JETTON_INDEX_TIGER;
			iLostIndex = JETTON_INDEX_LEOPARD;
		}
		else
		{
			iWinIndex = JETTON_INDEX_LEOPARD;
			iLostIndex = JETTON_INDEX_TIGER;
		}

		//先判断区域，再判断牌型
		bool find_flag = false;

		//设置黑方赢
		if (ctrl_area_type == AREA_BLACK && iWinIndex == JETTON_INDEX_TIGER)
		{
			if (ctrl_card_type == AREA_MAX)
			{
				find_flag = true;
			}
			else
			{
				if (cbTableCardType[JETTON_INDEX_TIGER] == ctrl_card_type)
				{
					find_flag = true;
				}
			}
		}

		//设置红方赢
		if (ctrl_area_type == AREA_RED && iWinIndex == JETTON_INDEX_LEOPARD)
		{
			if (ctrl_card_type == AREA_MAX)
			{
				find_flag = true;
			}
			else
			{
				if (cbTableCardType[JETTON_INDEX_LEOPARD] == ctrl_card_type)
				{
					find_flag = true;
				}
			}
		}

		//只设置赢取方的牌型
		if (ctrl_area_type == AREA_MAX && ctrl_card_type != AREA_MAX)
		{
			if (iWinIndex == JETTON_INDEX_TIGER && cbTableCardType[JETTON_INDEX_TIGER] == ctrl_card_type)
			{
				find_flag = true;
			}
			if (iWinIndex == JETTON_INDEX_LEOPARD && cbTableCardType[JETTON_INDEX_LEOPARD] == ctrl_card_type)
			{
				find_flag = true;
			}
		}

		if (!find_flag)
		{
			for (int index = 0; index < 1024; index++)
			{
				m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
				BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
				memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
				if (m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT) == false)
				{
					break;
				}
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

void CGameWarTable::OnBrcFlushSendAllPlayerInfo()
{
	LOG_DEBUG("send brc flush all true player info list.");

	net::msg_brc_control_all_player_bet_info rep;

	//计算座位玩家
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//获取用户
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//统计信息
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	//计算旁观玩家
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//统计信息
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = 0; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
	}
}

bool CGameWarTable::ChangeBanker(bool bCancelCurrentBanker)
{
	return true;
}

void CGameWarTable::SendApplyUser(CGamePlayer* pPlayer)
{
	return;
}

// 设置库存输赢  add by har
bool CGameWarTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 playerAllWinScore = GetBankerAndPlayerWinScore();
	// 循环，直到找到满足条件的牌组合
	for (int i = 0; i < 1000; ++i) {
		if (CheckStockChange(stockChange, playerAllWinScore, i))
			return true;
		//重新洗牌
		for (int index = 0; index < 1024; ++index) {
			m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
			BYTE cbArTempCardData[SHOW_CARD_COUNT][MAX_COUNT] = { 0 };
			memcpy(cbArTempCardData, m_cbTableCardArray, sizeof(cbArTempCardData));
			if (!m_GameLogic.IsSameCard(cbArTempCardData[0], cbArTempCardData[1], MAX_COUNT))
				break;
		}
		playerAllWinScore = GetBankerAndPlayerWinScore();
	}
	LOG_ERROR("SetStockWinLose fail roomid:%d,tableid:%d,playerAllWinScore:%d,stockChange:%d", GetRoomID(), GetTableID(), playerAllWinScore, stockChange);
	return false;
}

// 获取某个玩家的赢分  add by har
int64 CGameWarTable::GetSinglePlayerWinScore(CGamePlayer *pPlayer, uint8 cbWinArea[JETTON_INDEX_COUNT], int64 lWinMultiple[JETTON_INDEX_COUNT], int64 &lBankerWinScore) {
	if (pPlayer == NULL)
		return 0;

	int64 playerWinScore = 0; // 该玩家赢分
	uint32 dwUserID = pPlayer->GetUID();
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex) {
		map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			continue;
		int64 lUserJettonScore = it_player_jetton->second;
		if (lUserJettonScore == 0)
			continue;
		int64 lTempWinScore = 0;
		if (cbWinArea[nAreaIndex] == TRUE)
			playerWinScore += (lUserJettonScore * lWinMultiple[nAreaIndex]);
		else
			playerWinScore -= lUserJettonScore;
	}
	lBankerWinScore -= playerWinScore;
	if (pPlayer->IsRobot())
		return 0;
	return playerWinScore;
}
// 获取非机器人玩家赢分 add by har
int64 CGameWarTable::GetBankerAndPlayerWinScore() {
	int64 playerAllWinScore = 0; // 非机器人玩家总赢数
	int64 lBankerWinScore = 0;
	uint8 cbWinArea[JETTON_INDEX_COUNT] = { 0 };
	int64 lWinMultiple[JETTON_INDEX_COUNT] = { 0 };
	uint8 cbTableCardType[SHOW_CARD_COUNT] = { 0 };

	uint8 cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT] = { 0 };
	memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));


	cbTableCardType[JETTON_INDEX_TIGER] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_TIGER], MAX_CARD_COUNT);
	cbTableCardType[JETTON_INDEX_LEOPARD] = m_GameLogic.GetCardType(cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);
	uint8 result = m_GameLogic.CompareCard(cbTableCardArray[JETTON_INDEX_TIGER], cbTableCardArray[JETTON_INDEX_LEOPARD], MAX_CARD_COUNT);

	uint16 iLostIndex = 0, iWinIndex = 0;
	if (result == TRUE) {
		iWinIndex = JETTON_INDEX_TIGER;
		iLostIndex = JETTON_INDEX_LEOPARD;
	}
	else {
		iWinIndex = JETTON_INDEX_LEOPARD;
		iLostIndex = JETTON_INDEX_TIGER;
	}

	GetAreaInfo(iLostIndex, iWinIndex, cbTableCardType, cbWinArea, lWinMultiple);

	// 椅子用户
	for (int nChairID = 0; nChairID < GAME_PLAYER; ++nChairID)
		playerAllWinScore += GetSinglePlayerWinScore(GetPlayer(nChairID), cbWinArea, lWinMultiple, lBankerWinScore);
	// 旁观用户
	for (map<uint32, CGamePlayer*>::iterator it_looker = m_mpLookers.begin(); it_looker != m_mpLookers.end(); ++it_looker)
		playerAllWinScore += GetSinglePlayerWinScore(it_looker->second, cbWinArea, lWinMultiple, lBankerWinScore);

	if (IsBankerRealPlayer())
		playerAllWinScore += lBankerWinScore;
	return playerAllWinScore;
}