//
// Created by toney on 16/4/6.
//
#include <data_cfg_mgr.h>
#include <center_log.h>
#include "game_imple_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "json/json.h"
#include "robot_mgr.h"
#include "robot_oper_mgr.h"

using namespace std;
using namespace svrlib;
using namespace net;
using namespace game_paijiu;

namespace
{
    const static uint32 s_FreeTime              = 3*1000;       // 空闲时间
    const static uint32 s_PlaceJettonTime       = 15*1000;      // 下注时间    
    const static uint32 s_DispatchTime          = 17*1000;      // 发牌时间
    
	//const static uint32 s_SysLostWinProChange = 500;			// 吃币吐币概率变化
	//const static int64  s_UpdateJackpotScore = 100000;			// 更新奖池分数

};

extern vector<vector<uint8> > g_vvAreaIndexLists; // add by har

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 牌九
        {
            pTable = new CGamePaijiuTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛牌九
        {
            pTable = new CGamePaijiuTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房牌九
        {
            pTable = new CGamePaijiuTable(this,tableID,emTABLE_TYPE_PLAYER);
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
CGamePaijiuTable::CGamePaijiuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));

	//个人下注
	for(uint8 i=0;i<AREA_COUNT;++i){
        m_userJettonScore[i].clear();
    }
	//玩家成绩
	m_mpUserWinScore.clear();
	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));

	//庄家信息
	m_pCurBanker            = NULL;
	m_wBankerTime           = 0;
	m_lBankerWinScore       = 0L;
	m_robotBankerWinPro = 0;
	m_playerBankerLosePro = 0;
	// add by har
	m_confBankerAllWinLoseMaxCount = 0;
	m_confBankerAllWinLoseLimitCount = 1;
	m_bankerAllWinLoseComputeCount = 0;
	m_bankerAllWinLoseCount = 0;
	m_bIsConputeBankerAllWinLose = false;
	m_confRobotBankerAreaPlayerWinMax = 100000000;
	m_confRobotBankerAreaPlayerLoseRate = 0;
	ZeroMemory(m_isTableCardPointKill, sizeof(m_isTableCardPointKill));
	// add by har end

	//m_lMaxPollScore = 0;
	//m_lMinPollScore = 0;
	//m_lCurPollScore = 0;
	//m_lFrontPollScore = 0;
	//m_uSysWinPro = 0;
	//m_uSysLostPro = 0;
	m_bIsChairRobotAlreadyJetton = false;
	m_chairRobotPlaceJetton.clear();
	m_bIsRobotAlreadyJetton = false;
	m_RobotPlaceJetton.clear();
	m_tagControlPalyer.Init();
    return;
}
CGamePaijiuTable::~CGamePaijiuTable()
{

}
bool    CGamePaijiuTable::CanEnterTable(CGamePlayer* pPlayer)
{
    if(pPlayer->GetTable() != NULL)
        return false;
    // 限额进入
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin()){
        return false;
    }
    return true;
}
bool    CGamePaijiuTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    if(m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
        return false;         
    if(IsInApplyList(pPlayer->GetUID()))
        return false;    
    
    return true;
}
bool    CGamePaijiuTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
    
    return CGameTable::CanSitDown(pPlayer,chairID);
}
bool    CGamePaijiuTable::CanStandUp(CGamePlayer* pPlayer)
{
    return true;        
}    
bool    CGamePaijiuTable::IsFullTable()
{
    if(m_mpLookers.size() >= 200)
        return true;
    
    return false;
}
void CGamePaijiuTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::paijiu_table_info* ppaijiu = pInfo->mutable_paijiu();
    ppaijiu->set_tableid(GetTableID());
    ppaijiu->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        ppaijiu->set_is_passwd(1);
    }else{
        ppaijiu->set_is_passwd(0);
    }
    ppaijiu->set_hostname(m_conf.hostName);
    ppaijiu->set_basescore(m_conf.baseScore);
    ppaijiu->set_consume(m_conf.consume);
    ppaijiu->set_entermin(m_conf.enterMin);
    ppaijiu->set_duetime(m_conf.dueTime);
    ppaijiu->set_feetype(m_conf.feeType);
    ppaijiu->set_feevalue(m_conf.feeValue);
    ppaijiu->set_card_time(s_PlaceJettonTime);
    ppaijiu->set_table_state(GetGameState());
    ppaijiu->set_sitdown(m_pHostRoom->GetSitDown());
    ppaijiu->set_apply_banker_condition(GetApplyBankerCondition());
    ppaijiu->set_apply_banker_maxscore(GetApplyBankerConditionLimit());
    ppaijiu->set_banker_max_time(m_BankerTimeLimit);
    
}

//配置桌子
bool CGamePaijiuTable::Init()
{
    SetGameState(net::TABLE_STATE_NIUNIU_FREE);

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    m_BankerTimeLimit = 3;
    m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");

    m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
    m_robotChairSize = g_RandGen.RandRange(3, 6);//机器人座位数

	ReAnalysisParam();
	CRobotOperMgr::Instance().PushTable(this);
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGamePaijiuTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	if (jvalue.isMember("bbw")) {
		m_robotBankerWinPro = jvalue["bbw"].asInt();
	}
	if (jvalue.isMember("pbl")) {
		m_playerBankerLosePro = jvalue["pbl"].asInt();
	}
	// add by har
	if (jvalue.isMember("awlmc"))
		m_confBankerAllWinLoseMaxCount = jvalue["awlmc"].asInt();
	if (jvalue.isMember("awllc"))
		m_confBankerAllWinLoseLimitCount = jvalue["awllc"].asInt();
	if (jvalue.isMember("rzapwm"))
		m_confRobotBankerAreaPlayerWinMax = jvalue["rzapwm"].asInt();
	if (jvalue.isMember("rzaplr"))
		m_confRobotBankerAreaPlayerLoseRate = jvalue["rzaplr"].asInt();
	// add by har end

	LOG_ERROR("CGamePaijiuTable::ReAnalysisParam success - roomid:%d,tableid:%d,m_robotBankerWinPro:%d,m_playerBankerLosePro:%d,m_confBankerAllWinLoseMaxCount:%d,m_confBankerAllWinLoseLimitCount:%d,m_confRobotBankerAreaPlayerWinMax:%d,m_confRobotBankerAreaPlayerLoseRate:%d", 
		m_pHostRoom->GetRoomID(), GetTableID(), m_robotBankerWinPro, m_playerBankerLosePro, m_confBankerAllWinLoseMaxCount, 
		m_confBankerAllWinLoseLimitCount, m_confRobotBankerAreaPlayerWinMax, m_confRobotBankerAreaPlayerLoseRate);
	return true;
}


void CGamePaijiuTable::ShutDown()
{
    //CalcBankerScore();
}
//复位桌子
void CGamePaijiuTable::ResetTable()
{
    ResetGameData();
}
void CGamePaijiuTable::OnTimeTick()
{
	OnTableTick();
    uint8 tableState = GetGameState();
    if(m_coolLogic.isTimeOut())
    {
        switch(tableState)
        {
        case TABLE_STATE_NIUNIU_FREE:           // 空闲
            {                
                if(OnGameStart()){
					InitChessID();
                    SetGameState(TABLE_STATE_NIUNIU_PLACE_JETTON);       

					m_brc_table_status = emTABLE_STATUS_START;
					m_brc_table_status_time = s_PlaceJettonTime;

					//同步刷新百人场控制界面的桌子状态信息
					OnBrcControlFlushTableStatus();
              
                }else{
                    m_coolLogic.beginCooling(s_FreeTime);                    
                }
            }break;
        case TABLE_STATE_NIUNIU_PLACE_JETTON:   // 下注时间
            {
                SetGameState(TABLE_STATE_NIUNIU_GAME_END);
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
        case TABLE_STATE_NIUNIU_GAME_END:       // 结束游戏
            {
    			//切换庄家
    			ClearTurnGameData();
    			ChangeBanker(false);
                SetGameState(TABLE_STATE_NIUNIU_FREE);
                m_coolLogic.beginCooling(s_FreeTime);

				m_brc_table_status = emTABLE_STATUS_FREE;
				m_brc_table_status_time = 0;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();

				//刷新上庄列表
				OnBrcControlFlushAppleList();
                
            }break;         
        default:
            break;
        }
    }
    if(tableState == TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
    {
        OnRobotOper();
		OnChairRobotJetton();
		//OnChairRobotPlaceJetton();

		OnRobotJetton();
		//OnRobotPlaceJetton();
    }           
    
}

void CGamePaijiuTable::OnRobotTick()
{
	uint8 tableState = GetGameState();
	int64 passtick = m_coolLogic.getPassTick();
	//LOG_DEBUG("on_time_tick_loop - roomid:%d,tableid:%d,m_lGameCount:%lld,tableState:%d,passtick:%lld", GetRoomID(), GetTableID(), m_lGameCount, tableState, passtick);

	if (tableState == net::TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
	{
		OnChairRobotPlaceJetton();
		OnRobotPlaceJetton();
	}
}


// 游戏消息
int CGamePaijiuTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    //LOG_DEBUG("table recv msg:%d--%d", pPlayer->GetUID(),cmdID);
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
    case net::C2S_MSG_PAIJIU_PLACE_JETTON:  // 用户加注
        {
            if(GetGameState() != TABLE_STATE_NIUNIU_PLACE_JETTON){
                LOG_DEBUG("not jetton state can't jetton");
                return 0;
            }
            net::msg_paijiu_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);                      
            return OnUserPlaceJetton(pPlayer,msg.jetton_area(),msg.jetton_score());
        }break;
    case net::C2S_MSG_PAIJIU_APPLY_BANKER:  // 申请庄家
        {
            net::msg_paijiu_apply_banker msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(msg.apply_oper() == 1){
                return OnUserApplyBanker(pPlayer,msg.apply_score(),msg.auto_addscore());
            }else{
                return OnUserCancelBanker(pPlayer);
            }
        }break;
    case net::C2S_MSG_PAIJIU_JUMP_APPLY_QUEUE:// 插队
        {
            net::msg_paijiu_jump_apply_queue_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
            return OnUserJumpApplyQueue(pPlayer);
        }break;
	case net::C2S_MSG_PAIJIU_CONTINUOUS_PRESSURE_REQ://
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
	case net::C2S_MSG_BRC_CONTROL_FORCE_LEAVE_BANKER_REQ://
		{
			net::msg_brc_control_force_leave_banker_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlApplePlayer(pPlayer, msg.uid());
		}break;
	case net::C2S_MSG_BRC_CONTROL_AREA_INFO_REQ://
		{
			net::msg_brc_control_area_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlPlayerBetArea(pPlayer, msg);
		}break;
    default:
        return 0;
    }
    return 0;
}
//用户断线或重连
bool CGamePaijiuTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
{
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
bool CGamePaijiuTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{
    
    SendSeatInfoToClient();
    return true;
}
//用户起立
bool CGamePaijiuTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{

    SendSeatInfoToClient();
    return true;
}
// 游戏开始
bool CGamePaijiuTable::OnGameStart()
{
	if (m_pHostRoom == NULL)
	{
		return false;
	}
	if (m_pHostRoom->GetCanGameStart() == false)
	{
		LOG_DEBUG("start_error - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

		return false;
	}
	LOG_DEBUG("game_start - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

    //LOG_DEBUG("game start:%d-%d",m_pHostRoom->GetRoomID(),GetTableID());
    if(m_pCurBanker == NULL)
	{
        //LOG_ERROR("the banker is null");
		LOG_DEBUG("game_start the banker is null - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

        CheckRobotApplyBanker();
        ChangeBanker(false);
        return false;
    }
    m_coolLogic.beginCooling(s_PlaceJettonTime);
    
    m_curr_bet_user.clear();

    net::msg_paijiu_start_rep gameStart;
    gameStart.set_time_leave(m_coolLogic.getCoolTick());
    gameStart.set_banker_score(m_lBankerScore);
    gameStart.set_banker_id(GetBankerUID());
    gameStart.set_banker_buyin_score(m_lBankerBuyinScore);
 
    SendMsgToAll(&gameStart,net::S2C_MSG_PAIJIU_START);
	OnTableGameStart();
    OnRobotStandUp();
    return true;
}
//游戏结束
bool CGamePaijiuTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end:table:%d,%d",GetTableID(),m_wBankerTime);
    switch(reason)
    {
    case GER_NORMAL:		//常规结束
        {
            InitBlingLog();
			WriteBankerInfo();
            AddPlayerToBlingLog();
            
			//计算分数
			m_curr_banker_win = 0;
			int64 lBankerWinScore = CalculateScore();

			LOG_DEBUG("CGamePaijiuTable::OnGameEnd GER_NORMAL - roomid:%d,tableid:%d,bankUid:%d,chessid:%s,m_lBankerScore:%lld,curScore:%lld,lBankerWinScore:%lld,m_curr_banker_win:%lld",
				GetRoomID(), GetTableID(), GetBankerUID(), GetChessID().c_str(), m_lBankerScore, GetPlayerCurScore(m_pCurBanker), lBankerWinScore, m_curr_banker_win);

			int64 bankerfee = CalcPlayerInfo(GetBankerUID(),lBankerWinScore,m_curr_banker_win,true);


			lBankerWinScore += bankerfee;
			// add by har
			int64 playerAllWinScore = 0; // 玩家总赢分
			if (IsBankerRealPlayer())
				playerAllWinScore = lBankerWinScore;
			// add by har end
			//递增次数
			m_wBankerTime++;

			//结束消息
            net::msg_paijiu_game_end msg;
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                net::msg_cards* pCards = msg.add_table_cards();
                for(uint8 j=0;j<MAX_CARD;++j){
                    pCards->add_cards(m_cbTableCardArray[i][j]);
                }
            }
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                msg.add_card_types(m_cbTableCardType[i]);                
            }
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_win_multiple(m_winMultiple[i]);
            }

            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(lBankerWinScore);
            msg.set_banker_total_score(m_lBankerWinScore);                                 
            msg.set_rand_card(CPaijiuLogic::m_cbCardListData[g_RandGen.RandUInt()%CARD_COUNT]);
			msg.set_settle_accounts_type(m_cbBrankerSettleAccountsType);
			////发送积分
			//for(WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex){
			//	//设置成绩
			//	uint32 uid = GetPlayerID(wUserIndex);
   //             msg.add_player_score(m_mpUserWinScore[uid]);            
			//}
            
			int64 lRobotScoreResult = 0;
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				if (pPlayer->IsRobot())
				{
					lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			//发送旁观者积分
			map<uint32, CGamePlayer*>::iterator it_win_robot_score = m_mpLookers.begin();
			for (; it_win_robot_score != m_mpLookers.end(); ++it_win_robot_score)
			{
				CGamePlayer* pPlayer = it_win_robot_score->second;
				if (pPlayer == NULL)continue;

				if (pPlayer->IsRobot())
				{
					lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			if (lBankerWinScore> 0 && lRobotScoreResult < 0)
			{
				lRobotScoreResult = -lRobotScoreResult;
				int64 fee = -(lRobotScoreResult * m_conf.feeValue / PRO_DENO_10000);
				lRobotScoreResult += fee;
				lRobotScoreResult = -lRobotScoreResult;
			}

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
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_GAME_END);

				//精准控制统计
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
			}

			//发送座位积分
			//for( WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex )
			//{
			//	CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			//	if(pPlayer == NULL)
   //                 continue;

			//	int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
			//	lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
			//	m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
			//	msg.set_user_score(lUserScoreFree);
			//	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_GAME_END);
			//}            
            //发送旁观者积分
            map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
            for(;it != m_mpLookers.end();++it)
            {
                CGamePlayer* pPlayer = it->second;
                if(pPlayer == NULL)continue;

				int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
				lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
				m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
				msg.set_user_score(lUserScoreFree);
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_GAME_END);

				//精准控制统计
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
				// add by har
				if (!pPlayer->IsRobot())
					playerAllWinScore += lUserScoreFree; // add by har end
            }
			//奖池统计
			int64 lPlayerScoreResult = 0;
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				if (!pPlayer->IsRobot())
				{
					lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			map<uint32, CGamePlayer*>::iterator it_lookers = m_mpLookers.begin();
			for (; it_lookers != m_mpLookers.end(); ++it_lookers)
			{
				CGamePlayer* pPlayer = it_lookers->second;
				if (pPlayer == NULL)continue;
				if (!pPlayer->IsRobot())
				{
					lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			//if (m_pCurBanker != NULL && m_pCurBanker->IsRobot())
			//{
			//	m_lCurPollScore -= lPlayerScoreResult;
			//}
			//if (m_pCurBanker != NULL && !m_pCurBanker->IsRobot())
			//{
			//	m_lCurPollScore += lRobotScoreResult;
			//}
			if (m_pCurBanker != NULL && m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lPlayerScoreResult != 0)
			{
				m_pHostRoom->UpdateJackpotScore(-lPlayerScoreResult);
			}
			if (m_pCurBanker != NULL && !m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lRobotScoreResult != 0)
			{
				m_pHostRoom->UpdateJackpotScore(lRobotScoreResult);
			}

			//int64 lDiffMaxScore = m_lCurPollScore - m_lFrontPollScore;
			//int64 lDiffMinScore = m_lFrontPollScore - m_lCurPollScore;

			//LOG_DEBUG("1 roomid:%d,tableid:%d,m_lCurPollScore:%lld,m_lMinPollScore:%lld,m_lMaxPollScore:%lld,m_lFrontPollScore:%lld,m_lCurPollScore:%lld,m_uSysWinPro:%d,m_uSysLostPro:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,lDiffMaxScore:%lld,lDiffMinScore:%lld",
			//	m_pHostRoom->GetRoomID(), GetTableID(), m_lCurPollScore, m_lMinPollScore, m_lMaxPollScore, m_lFrontPollScore, m_lCurPollScore, m_uSysWinPro, m_uSysLostPro, lPlayerScoreResult, lRobotScoreResult, lDiffMaxScore, lDiffMinScore);

			//if (lDiffMaxScore >= s_UpdateJackpotScore) // 奖池增加 吐币增加 吃币减少
			//{
			//	do
			//	{
			//		if (m_uSysLostPro + s_SysLostWinProChange < PRO_DENO_10000)
			//		{
			//			m_uSysLostPro += s_SysLostWinProChange;
			//		}
			//		else if (m_uSysLostPro + s_SysLostWinProChange >= PRO_DENO_10000)
			//		{
			//			m_uSysLostPro = PRO_DENO_10000;
			//		}

			//		if (m_uSysWinPro > s_SysLostWinProChange)
			//		{
			//			m_uSysWinPro -= s_SysLostWinProChange;
			//		}
			//		else if (m_uSysWinPro <= s_SysLostWinProChange)
			//		{
			//			m_uSysWinPro = 0;
			//		}

			//		lDiffMaxScore -= s_UpdateJackpotScore;

			//	} while (lDiffMaxScore >= s_UpdateJackpotScore);

			//	m_lFrontPollScore = m_lCurPollScore - lDiffMaxScore;
			//}

			//if (lDiffMinScore >= s_UpdateJackpotScore) // 奖池减少 吃币增加 吐币减少
			//{
			//	do
			//	{
			//		if (m_uSysWinPro + s_SysLostWinProChange < PRO_DENO_10000)
			//		{
			//			m_uSysWinPro += s_SysLostWinProChange;
			//		}
			//		else if (m_uSysWinPro + s_SysLostWinProChange >= PRO_DENO_10000)
			//		{
			//			m_uSysWinPro = PRO_DENO_10000;
			//		}

			//		if (m_uSysLostPro > s_SysLostWinProChange)
			//		{
			//			m_uSysLostPro -= s_SysLostWinProChange;
			//		}
			//		else if (m_uSysLostPro <= s_SysLostWinProChange)
			//		{
			//			m_uSysLostPro = 0;
			//		}

			//		lDiffMinScore -= s_UpdateJackpotScore;

			//	} while (lDiffMinScore >= s_UpdateJackpotScore);

			//	m_lFrontPollScore = m_lCurPollScore + lDiffMinScore;
			//}

			//LOG_DEBUG("2 roomid:%d,tableid:%d,m_lCurPollScore:%lld,m_lMinPollScore:%lld,m_lMaxPollScore:%lld,m_lFrontPollScore:%lld,m_lCurPollScore:%lld,m_uSysWinPro:%d,m_uSysLostPro:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,lDiffMaxScore:%lld,lDiffMinScore:%lld",
			//	m_pHostRoom->GetRoomID(), GetTableID(), m_lCurPollScore, m_lMinPollScore, m_lMaxPollScore, m_lFrontPollScore, m_lCurPollScore, m_uSysWinPro, m_uSysLostPro, lPlayerScoreResult, lRobotScoreResult, lDiffMaxScore, lDiffMinScore);
			//

            //更新活跃福利数据            
            int64 curr_win = m_mpUserWinScore[m_aw_ctrl_uid];
            UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);

			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,playerAllWinScore:%lld,m_allJettonScore:%d-%d-%d-%d-%d-%d-%d--%d",
				m_pHostRoom->GetRoomID(), GetTableID(), lPlayerScoreResult, lRobotScoreResult, playerAllWinScore,
				m_allJettonScore[0], m_allJettonScore[1], m_allJettonScore[2], m_allJettonScore[3], m_allJettonScore[4], m_allJettonScore[5], m_allJettonScore[6],
				m_allJettonScore[0] + m_allJettonScore[1] + m_allJettonScore[2] + m_allJettonScore[3] + m_allJettonScore[4] + m_allJettonScore[5] + m_allJettonScore[6]);

			//如果当前庄家为真实玩家，需要更新精准控制统计
			if (m_pCurBanker && !m_pCurBanker->IsRobot())
			{
				OnBrcControlSetResultInfo(GetBankerUID(), lBankerWinScore);
			}

			m_mpUserWinScore[GetBankerUID()] = 0;   

			//同步所有玩家数据到控端
			OnBrcFlushSendAllPlayerInfo();

            SaveBlingLog();
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
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
void  CGamePaijiuTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer != NULL) {
		uid = pPlayer->GetUID();
	}
    //LOG_DEBUG("player join:%d--%d",chairID,isJoin);
	LOG_DEBUG("player join - roomid:%d,tableid:%d,uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld", 
		GetRoomID(),GetTableID(),uid, chairID, isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin){
        SendApplyUser(pPlayer);
        SendGameScene(pPlayer);
		SendPlayLog(pPlayer);
    }else{
        OnUserCancelBanker(pPlayer);        
        RemoveApplyBanker(pPlayer->GetUID());
        for(uint8 i=0;i<AREA_COUNT;++i){
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
void    CGamePaijiuTable::SendGameScene(CGamePlayer* pPlayer)
{
    LOG_DEBUG("send game scene:%d", pPlayer->GetUID());
    switch(m_gameState)
    {
    case net::TABLE_STATE_NIUNIU_FREE:          // 空闲状态
        {
            net::msg_paijiu_game_info_free_rep msg;
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
           
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_GAME_FREE_INFO);
        }break;
    case net::TABLE_STATE_NIUNIU_PLACE_JETTON:  // 游戏状态
    case net::TABLE_STATE_NIUNIU_GAME_END:      // 结束状态
        {
            net::msg_paijiu_game_info_play_rep msg;
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_all_jetton_score(m_allJettonScore[i]);
            }            
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_game_status(m_gameState);
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
            }       
			if (GetBankerUID() == pPlayer->GetUID() && m_needLeaveBanker)
			{
				msg.set_need_leave_banker(1);
			}
			else
			{
				msg.set_need_leave_banker(0);
			}
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_GAME_PLAY_INFO);

			//刷新所有控制界面信息协议---用于断线重连的处理
			if (pPlayer->GetCtrlFlag())
			{
				//刷新百人场桌子状态
				m_brc_table_status_time = m_coolLogic.getCoolTick();
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
    SendApplyUser(pPlayer);
	SendFrontJettonInfo(pPlayer);
}
int64    CGamePaijiuTable::CalcPlayerInfo(uint32 uid,int64 winScore,int64 OnlywinScore,bool isBanker)
{
    //LOG_DEBUG("report to lobby:%d  %lld",uid,winScore);
	int64 userJettonScore = 0;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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
	CalcPlayerGameInfoForBrc(uid, winScore, 0, true, isBanker, userJettonScore);	

	if (isBanker) {// 庄家池子减少响应筹码,否则账目不平
		m_lBankerWinScore += fee;
		//当前积分
		m_lBankerScore += fee;
	}
	LOG_DEBUG("report to lobby uid:%d winScore:%lld OnlywinScore:%lld fee:%lld", uid, winScore, OnlywinScore,fee);
	return fee;
}
// 重置游戏数据
void    CGamePaijiuTable::ResetGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));

	//个人下注
	for(uint8 i=0;i<AREA_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//玩家成绩	
	m_mpUserWinScore.clear();       
    m_curr_bet_user.clear();
	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));
	//庄家信息
	m_pCurBanker            = NULL;
	m_wBankerTime           = 0;
	m_lBankerWinScore       = 0L;		


    m_bankerAutoAddScore    = 0;                //自动补币
    m_needLeaveBanker       = false;            //离开庄位
        
    m_wBankerTime = 0;							//做庄次数
    m_wBankerWinTime = 0;                       //胜利次数
	m_lBankerScore = 0;							//庄家积分
	m_lBankerWinScore = 0;						//累计成绩

    m_lBankerBuyinScore = 0;                    //庄家带入
    m_lBankerInitBuyinScore = 0;                //庄家初始带入
    m_lBankerWinMaxScore = 0;                   //庄家最大输赢
    m_lBankerWinMinScore = 0;                   //庄家最惨输赢

}
void    CGamePaijiuTable::ClearTurnGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));

	//个人下注
	for(uint8 i=0;i<AREA_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//玩家成绩	
	m_mpUserWinScore.clear();       
	//扑克信息
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));
	ZeroMemory(m_isTableCardPointKill, sizeof(m_isTableCardPointKill)); // add by har
}
// 写入出牌log
void    CGamePaijiuTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
	uint8 cardType = m_GameLogic.GetCardType(cardData, cardCount);
    Json::Value logValue;
    logValue["p"]       = chairID;
	logValue["cardtype"] = cardType;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}

// 写入加注log
void    CGamePaijiuTable::WriteAddScoreLog(uint32 uid, uint8 area, int64 score, int64 win)
{
	if (score == 0)
		return;

	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["p"] = area;
	logValue["s"] = score;
	logValue["win"] = win;

	m_operLog["op"].append(logValue);
}

// 写入庄家信息
void    CGamePaijiuTable::WriteBankerInfo()
{
	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	m_operLog["banker"] = GetBankerUID();
	m_operLog["cps"] = tmpJackpotScore.lCurPollScore;
	m_operLog["swp"] = tmpJackpotScore.uSysWinPro;
	m_operLog["slp"] = tmpJackpotScore.uSysLostPro;
}

//加注事件
bool    CGamePaijiuTable::OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
	if (pPlayer != NULL && pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("player place jetton:%d--%d--%lld", pPlayer->GetUID(), cbJettonArea, lJettonScore);

	}
    //效验参数
	if(cbJettonArea > ID_HENG_R || lJettonScore <= 0){
        
        LOG_DEBUG("jetton is error:%d--%lld",cbJettonArea,lJettonScore);
		return false;
	}
	if(GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON){
        net::msg_paijiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//发送消息
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_PLACE_JETTON_REP);
		return true;
	}
	//庄家判断
	if(pPlayer->GetUID() == GetBankerUID()){
        LOG_DEBUG("the banker can't jetton");
		return true;
	}

	//变量定义
	int64 lJettonCount = 0L;
	for(int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex) 
        lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//合法校验
	if(lUserScore < lJettonCount + lJettonScore) 
    {
        LOG_DEBUG("the jetton more than you have");
        return true;
	}

	if (TableJettonLimmit(pPlayer, lJettonScore, lJettonCount) == false)
	{
		//bPlaceJettonSuccess = false;
		LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());
		net::msg_paijiu_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_PLACE_JETTON_REP);
		return true;
	}

	//成功标识
	bool bPlaceJettonSuccess=true;
	//合法验证
	if(GetUserMaxJetton(pPlayer, cbJettonArea) >= lJettonScore)
	{
		//保存下注
		m_allJettonScore[cbJettonArea] += lJettonScore;
		m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;	
		if (pPlayer->IsRobot() == false)
		{
			m_curr_bet_user.insert(pPlayer->GetUID());
		}
		if (!pPlayer->IsRobot()) {
			m_playerJettonScore[cbJettonArea] += lJettonScore;
		}
	}else{
	    LOG_DEBUG("the jetton more than limit");
		bPlaceJettonSuccess = false;
	}

	if(bPlaceJettonSuccess)
	{
		//刷新百人场控制界面的下注信息
		OnBrcControlBetDeal(pPlayer);

		RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);
		OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);
        net::msg_paijiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_SUCCESS);        
		//发送消息
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_PLACE_JETTON_REP);

        net::msg_paijiu_place_jetton_broadcast broad;
        broad.set_uid(pPlayer->GetUID());
        broad.set_jetton_area(cbJettonArea);
        broad.set_jetton_score(lJettonScore);
        broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);
        
        SendMsgToAll(&broad,net::S2C_MSG_PAIJIU_PLACE_JETTON_BROADCAST);
	}
	else
	{
        net::msg_paijiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//发送消息
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_PLACE_JETTON_REP);
        
	}    
    return true;
}

bool    CGamePaijiuTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	//LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(),GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//效验参数
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		if (info.score() <= 0 || info.area() >= AREA_COUNT)
		{
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

			LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());

			return false;
		}
		lTotailScore += info.score();
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	//合法校验
	//if (lUserScore < lJettonCount + lTotailScore + GetJettonMin())
	if (GetJettonMin() > GetEnterScore(pPlayer) || (lUserScore < lJettonCount + lTotailScore))
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return true;
	}
	//成功标识
	//bool bPlaceJettonSuccess = true;

	//for (int i = 0; i < msg.info_size(); i++)
	//{
	//	net::bairen_jetton_info info = msg.info(i);

	//	if (info.score() <= 0 || info.area() >= AREA_COUNT)
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

	//		LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());

	//		return false;
	//	}
	//	if (GetUserMaxJetton(pPlayer, info.area()) < info.score())
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

	//		LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());

	//		return false;
	//	}
	//}

	int64 lUserMaxHettonScore = GetUserMaxJetton(pPlayer, 0);
	if (lUserMaxHettonScore < lTotailScore)
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserMaxHettonScore:%lld,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserMaxHettonScore, lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}

	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		m_allJettonScore[info.area()] += info.score();
		//if (!pPlayer->IsRobot())
		//{
		//	m_playerJettonScore[info.area()] += info.score();
		//}
		if (!pPlayer->IsRobot())
		{
			m_playerJettonScore[info.area()] += info.score();
		}

		m_userJettonScore[info.area()][pPlayer->GetUID()] += info.score();
		if (pPlayer->IsRobot() == false)
		{
			m_curr_bet_user.insert(pPlayer->GetUID());
		}
        
		RecordPlayerBaiRenJettonInfo(pPlayer, info.area(), info.score());

		BYTE cbJettonArea = info.area();
		int64 lJettonScore = info.score();

		net::msg_paijiu_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_PLACE_JETTON_REP);

		net::msg_paijiu_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_PAIJIU_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PAIJIU_CONTINUOUS_PRESSURE_REP);

	//刷新百人场控制界面的下注信息
	OnBrcControlBetDeal(pPlayer);

	return true;
}


//申请庄家
bool    CGamePaijiuTable::OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore)
{
    LOG_DEBUG("player apply banker:%d--%lld",pPlayer->GetUID(),bankerScore);
        //构造变量
    net::msg_paijiu_apply_banker_rep msg;
    msg.set_apply_oper(1);
    msg.set_buyin_score(bankerScore);
    
    if(m_pCurBanker == pPlayer){
        LOG_DEBUG("you is the banker");
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
        return false;
    }
    if(IsSetJetton(pPlayer->GetUID())){// 下注不能上庄
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
        return false;
    }    
    //合法判断
	int64 lUserScore = GetPlayerCurScore(pPlayer);
    if(bankerScore > lUserScore){
        LOG_DEBUG("you not have more score:%d",pPlayer->GetUID());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
        return false;    
    }
    
	if(bankerScore < GetApplyBankerCondition() || bankerScore > GetApplyBankerConditionLimit()){
		LOG_DEBUG("you score less than condition %lld--%lld，faild",GetApplyBankerCondition(),GetApplyBankerConditionLimit());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
		return false;
	}

	//存在判断
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == pPlayer->GetUID())
		{
			LOG_DEBUG("you is in apply list");
            msg.set_result(net::RESULT_CODE_ERROR_STATE);    
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
			return false;
		}
	}

	//保存信息 
	m_ApplyUserArray.push_back(pPlayer);
    m_mpApplyUserInfo[pPlayer->GetUID()] = autoAddScore;
    LockApplyScore(pPlayer,bankerScore);        

    msg.set_result(net::RESULT_CODE_SUCCESS);    
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
	//切换判断
	if(GetGameState() == net::TABLE_STATE_NIUNIU_FREE && m_ApplyUserArray.size() == 1)
	{
		ChangeBanker(false);
	} 
    SendApplyUser();

	//刷新控制界面的上庄列表
	OnBrcControlFlushAppleList();

    return true;
}
bool    CGamePaijiuTable::OnUserJumpApplyQueue(CGamePlayer* pPlayer)
{
    LOG_DEBUG("player jump queue:%d",pPlayer->GetUID());
    int64 cost = CDataCfgMgr::Instance().GetJumpQueueCost();  
    net::msg_paijiu_jump_apply_queue_rep msg;
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("the jump cost can't pay:%lld",cost);
        msg.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_JUMP_APPLY_QUEUE);
        return false;
    }    
    //存在判断
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == pPlayer->GetUID())
		{
            if(nUserIdx == 0){
               LOG_DEBUG("you is the first queue");
               return false; 
            }
            m_ApplyUserArray.erase(m_ApplyUserArray.begin()+nUserIdx);
            m_ApplyUserArray.insert(m_ApplyUserArray.begin(),pPlayer);                      
            
            SendApplyUser(NULL);
            msg.set_result(net::RESULT_CODE_SUCCESS);
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_JUMP_APPLY_QUEUE);

			//刷新控制界面的上庄列表
			OnBrcControlFlushAppleList();

            cost = -cost;
            pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_JUMPQUEUE,0,0,cost,0,0,0,0);
			return true;
		}
	}      
    
    return false;
}
//取消申请
bool    CGamePaijiuTable::OnUserCancelBanker(CGamePlayer* pPlayer)
{
    LOG_DEBUG("cance banker:%d",pPlayer->GetUID());

    net::msg_paijiu_apply_banker_rep msg;
    msg.set_apply_oper(0);
    msg.set_result(net::RESULT_CODE_SUCCESS);
        
    //当前庄家 
	if(pPlayer->GetUID() == GetBankerUID() && GetGameState() != net::TABLE_STATE_NIUNIU_FREE)
	{
		//发送消息
		LOG_DEBUG("the game is run,you can't cance banker");
        m_needLeaveBanker = true;
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
		return true;
	}
	//存在判断
	for(WORD i=0; i<m_ApplyUserArray.size(); ++i)
	{
		//获取玩家
		CGamePlayer *pTmp = m_ApplyUserArray[i];		     

		//条件过滤
		if(pTmp != pPlayer) continue;

		//删除玩家
		RemoveApplyBanker(pPlayer->GetUID());       
		if(m_pCurBanker != pPlayer)
		{
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_BANKER);
		}
		else if(m_pCurBanker == pPlayer)
		{
			//切换庄家 
			ChangeBanker(true);
		}
        SendApplyUser();
		return true;
	}
	return false;
}

bool CGamePaijiuTable::SetRobotBrankerWin()
{
	BYTE    cbTableCard[CARD_COUNT];				//桌面扑克

	int64 lBankerWinScore = 0;
	uint32 brankeruid = GetBankerUID();

	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		lBankerWinScore = 0;

		//计算座位积分
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//获取用户
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了 
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		//计算旁观者积分
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			if(pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					m_winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		if (lBankerWinScore >= 0 && IsCurTableCardRuleAllow()) { // 需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("CGamePaijiuTable::SetRobotBrankerWin - roomid:%d,tableid:%d,brankeruid:%d,m_robotBankerWinPro:%d,iRountIndex:%d,lBankerWinScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, m_robotBankerWinPro, iRountIndex, lBankerWinScore);

	if (lBankerWinScore > 0) {
		return true;
	}

	return false;
}

bool CGamePaijiuTable::SetPlayerBrankerWin()
{
	BYTE    cbTableCard[CARD_COUNT];

	int64 lBankerWinScore = 0;
	uint32 brankeruid = GetBankerUID();

	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		lBankerWinScore = 0;

		//计算座位积分
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//获取用户
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了 
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		//计算旁观者积分
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					m_winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		if (lBankerWinScore > 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,m_robotBankerWinPro:%d,iRountIndex:%d,lBankerWinScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, m_robotBankerWinPro, iRountIndex, lBankerWinScore);

	if (lBankerWinScore > 0) {
		return true;
	}

	return false;
}


bool CGamePaijiuTable::SetPlayerBrankerLost()
{
	BYTE    cbTableCard[CARD_COUNT];

	int64 lBankerWinScore = 0;
	uint32 brankeruid = GetBankerUID();

	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		lBankerWinScore = 0;

		//计算座位积分
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//获取用户
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了 
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		//计算旁观者积分
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					m_winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		if (lBankerWinScore <= 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,m_robotBankerWinPro:%d,iRountIndex:%d,lBankerWinScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, m_robotBankerWinPro, iRountIndex, lBankerWinScore);

	if (lBankerWinScore <= 0) {
		return true;
	}

	return false;
}

bool CGamePaijiuTable::SetLeisurePlayerWin()
{
	BYTE    cbTableCard[CARD_COUNT];

	int64 lBankerWinScore = 0;
	uint32 brankeruid = GetBankerUID();

	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		lBankerWinScore = 0;

		//计算座位积分
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//获取用户
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了 
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		//计算旁观者积分
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					m_winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		if (lBankerWinScore <= 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,m_robotBankerWinPro:%d,iRountIndex:%d,lBankerWinScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, m_robotBankerWinPro, iRountIndex, lBankerWinScore);

	if (lBankerWinScore <= 0) {
		return true;
	}

	return false;
}

bool CGamePaijiuTable::SetLeisurePlayerLost()
{
	BYTE    cbTableCard[CARD_COUNT];

	int64 lBankerWinScore = 0;
	uint32 brankeruid = GetBankerUID();

	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		lBankerWinScore = 0;

		//计算座位积分
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//获取用户
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了 
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		//计算旁观者积分
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
			{
				//角判断
				bool bReturnScore = false;
				if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
				if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
				if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
				if (true == bWinFlag[wAreaIndex])// 赢了
				{
					lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				}
				else if (true == bReturnScore) {
					m_winMultiple[wAreaIndex] = 0;
				}
				else// 输了
				{
					lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				}
			}
		}
		if (lBankerWinScore > 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,m_robotBankerWinPro:%d,iRountIndex:%d,lBankerWinScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, m_robotBankerWinPro, iRountIndex, lBankerWinScore);

	if (lBankerWinScore > 0) {
		return true;
	}

	return false;
}


bool CGamePaijiuTable::SetControlPlayerWin(uint32 control_uid)
{
	BYTE    cbTableCard[CARD_COUNT];

	uint32 brankeruid = GetBankerUID();

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;
	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		control_win_score = 0;
		control_lose_score = 0;
		control_result_score = 0;

		for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
		{
			if (m_userJettonScore[wAreaIndex][control_uid] == 0)
				continue;
			//角判断
			bool bReturnScore = false;
			if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
			if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
			if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
			if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
			if (true == bWinFlag[wAreaIndex])// 赢了 
			{
				control_win_score += (m_userJettonScore[wAreaIndex][control_uid]);
			}
			else if (true == bReturnScore) {
				winMultiple[wAreaIndex] = 0;
			}
			else
			{
				control_lose_score -= m_userJettonScore[wAreaIndex][control_uid];
			}
		}
		control_result_score = control_win_score + control_lose_score;
		if (control_result_score > 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d",m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

	if (control_result_score > 0) {
		return true;
	}

	return false;
}

bool CGamePaijiuTable::SetControlPlayerLost(uint32 control_uid)
{
	BYTE    cbTableCard[CARD_COUNT];

	uint32 brankeruid = GetBankerUID();

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;
	int irount_count = 1000;
	int iRountIndex = 0;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
		bWinFlag[ID_DUI_MEN] = bWinDuiMen;
		bWinFlag[ID_DAO_MEN] = bWinDaoMen;
		bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
		bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

		int32 winMultiple[AREA_COUNT];
		memset(winMultiple, 0, sizeof(winMultiple));
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			winMultiple[i] = bWinFlag[i] ? 1 : -1;
		}

		control_win_score = 0;
		control_lose_score = 0;
		control_result_score = 0;

		for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
		{
			if (m_userJettonScore[wAreaIndex][control_uid] == 0)
				continue;
			//角判断
			bool bReturnScore = false;
			if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
			if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
			if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
			if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
			if (true == bWinFlag[wAreaIndex])// 赢了 
			{
				control_win_score += (m_userJettonScore[wAreaIndex][control_uid]);
			}
			else if (true == bReturnScore) {
				winMultiple[wAreaIndex] = 0;
			}
			else
			{
				control_lose_score -= m_userJettonScore[wAreaIndex][control_uid];
			}
		}
		control_result_score = control_win_score + control_lose_score;
		if (control_result_score < 0 && IsCurTableCardRuleAllow()) { //  需要同时满足牌组规则 modify by har
			break;
		} else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

	if (control_result_score < 0) {
		return true;
	}

	return false;
}


//bool CGamePaijiuTable::KilledPlayerCtrl()
//{
//	////桌面扑克
//	//BYTE cbTableCard[CARD_COUNT];
//	//m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
//	////设置扑克
//	//memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
//	uint32 playerBankerLosePro = m_pHostRoom->GetAoPlayerBankerLosePro();
//	uint32 robotBankerWinPro = m_pHostRoom->GetAoRobotBankerWinPro();
//	bool needRobotChangeCard = g_RandGen.RandRatio(robotBankerWinPro, PRO_DENO_10000);
//	bool needPlayerChangeCard = g_RandGen.RandRatio(playerBankerLosePro, PRO_DENO_10000);
//
//	bool bBrankerIsRobot = false;
//	bool bBrankerIsControl = false;
//
//
//	bool bIsControlPlayerIsJetton = false;
//	bool bIsFalgControl = false;
//
//	uint32 control_uid = m_tagControlPalyer.uid;
//	uint32 game_count = m_tagControlPalyer.count;
//	uint32 control_type = m_tagControlPalyer.type;
//
//
//	if (m_pCurBanker != NULL)
//	{
//		bBrankerIsRobot = m_pCurBanker->IsRobot();
//		if (control_uid == m_pCurBanker->GetUID())
//		{
//			bBrankerIsControl = true;
//		}
//	}
//
//	if (bBrankerIsControl && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
//	{
//		if (control_type == GAME_CONTROL_WIN)
//		{
//			bIsFalgControl = SetPlayerBrankerWin();
//		}
//		if (control_type == GAME_CONTROL_LOST)
//		{
//			bIsFalgControl = SetPlayerBrankerLost();
//		}
//		if (bIsFalgControl && m_tagControlPalyer.count>0)
//		{
//			//m_tagControlPalyer.count--;
//			//if (m_tagControlPalyer.count == 0)
//			//{
//			//	m_tagControlPalyer.Init();
//			//}
//			if (m_pHostRoom != NULL)
//			{
//				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
//			}
//		}
//	}
//
//	if (!bBrankerIsControl && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
//	{
//		for (uint8 i = 0; i < AREA_COUNT; ++i)
//		{
//			if (m_userJettonScore[i][control_uid] > 0)
//			{
//				bIsControlPlayerIsJetton = true;
//				break;
//			}
//		}
//	}
//
//	if (bIsControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
//	{
//		if (control_type == GAME_CONTROL_WIN)
//		{
//			bIsFalgControl = SetControlPlayerWin(control_uid);
//		}
//		if (control_type == GAME_CONTROL_LOST)
//		{
//			bIsFalgControl = SetControlPlayerLost(control_uid);
//		}
//		if (bIsFalgControl && m_tagControlPalyer.count>0)
//		{
//			//m_tagControlPalyer.count--;
//			//if (m_tagControlPalyer.count == 0)
//			//{
//			//	m_tagControlPalyer.Init();
//			//}
//			if (m_pHostRoom != NULL)
//			{
//				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
//			}
//		}
//	}
//
//	tagJackpotScore tmpJackpotScore;
//	if (m_pHostRoom != NULL)
//	{
//		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
//	}
//	bool bIsPoolScoreControl = false;
//	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
//	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);
//
//
//	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // 吐币
//	{
//		bIsPoolScoreControl = true;
//		if (bBrankerIsRobot)
//		{
//			SetLeisurePlayerWin();
//		}
//		else
//		{
//			SetPlayerBrankerWin();
//		}
//	}
//	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // 吃币
//	{
//		bIsPoolScoreControl = true;
//		if (bBrankerIsRobot)
//		{
//			SetLeisurePlayerLost();
//		}
//		else
//		{
//			SetPlayerBrankerLost();
//		}
//	}
//
//	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needRobotChangeCard && bBrankerIsRobot)
//	{
//		SetRobotBrankerWin();
//	}
//
//	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needPlayerChangeCard && !bBrankerIsRobot)
//	{
//		//设置真实庄家输
//		SetPlayerBrankerLost();
//	}
//	LOG_DEBUG("robot win - roomid:%d,tableid:%d,bBrankerIsRobot:%d,m_robotBankerWinPro:%d,needRobotChangeCard:%d,bIsControlPlayerIsJetton:%d",
//		GetRoomID(), GetTableID(), bBrankerIsRobot, m_robotBankerWinPro, needRobotChangeCard, bIsControlPlayerIsJetton);
//
//	return true;
//}

// 福利控制
bool CGamePaijiuTable::DosWelfareCtrl()
{
	////桌面扑克
	//BYTE cbTableCard[CARD_COUNT];
	//m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
	////设置扑克
	//memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
	uint32 playerBankerLosePro = m_pHostRoom->GetPlayerBankerLosePro();
	uint32 robotBankerWinPro = m_pHostRoom->GetRobotBankerWinPro();
	bool needRobotChangeCard = g_RandGen.RandRatio(robotBankerWinPro, PRO_DENO_10000);
	bool needPlayerChangeCard = g_RandGen.RandRatio(playerBankerLosePro, PRO_DENO_10000);

	bool bBrankerIsRobot = false;
	bool bBrankerIsControl = false;


	bool bIsControlPlayerIsJetton = false;
	bool bIsFalgControl = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;


	if (m_pCurBanker != NULL)
	{
		bBrankerIsRobot = m_pCurBanker->IsRobot();
		if (control_uid == m_pCurBanker->GetUID())
		{
			bBrankerIsControl = true;
		}
	}

	if (bBrankerIsControl && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetPlayerBrankerWin();
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetPlayerBrankerLost();
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

	if (!bBrankerIsControl && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (uint8 i = 0; i < AREA_COUNT; ++i)
		{
			if (m_userJettonScore[i][control_uid] > 0)
			{
				bIsControlPlayerIsJetton = true;
				break;
			}
		}
	}

	if (bIsControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetControlPlayerWin(control_uid);
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetControlPlayerLost(control_uid);
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

	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	bool bIsPoolScoreControl = false;
	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);


	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // 吐币
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerWin();
		}
		else
		{
			SetPlayerBrankerWin();
		}
	}
	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // 吃币
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerLost();
		}
		else
		{
			SetPlayerBrankerLost();
		}
	}

	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needRobotChangeCard && bBrankerIsRobot)
	{
		SetRobotBrankerWin();
	}

	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needPlayerChangeCard && !bBrankerIsRobot)
	{
		//设置真实庄家输
		SetPlayerBrankerLost();
	}
	//LOG_DEBUG("robot win - roomid:%d,tableid:%d,bBrankerIsRobot:%d,m_robotBankerWinPro:%d,needRobotChangeCard:%d,bIsControlPlayerIsJetton:%d",
	//	GetRoomID(), GetTableID(), bBrankerIsRobot, m_robotBankerWinPro, needRobotChangeCard, bIsControlPlayerIsJetton);

	LOG_DEBUG("roomid:%d,tableid:%d,bBrankerIsControl:%d,bIsControlPlayerIsJetton:%d,bIsPoolScoreControl:%d,bBrankerIsRobot:%d,robotBankerWinPro:%d,needRobotChangeCard:%d,playerBankerLosePro:%d,needPlayerChangeCard:%d,control_uid:%d,control_type:%d,game_count:%d,bIsFalgControl:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), bBrankerIsControl, bIsControlPlayerIsJetton, bIsPoolScoreControl, bBrankerIsRobot, robotBankerWinPro, needRobotChangeCard, playerBankerLosePro, needPlayerChangeCard, control_uid, control_type, game_count, bIsFalgControl);

	return true;
}
// 非福利控制
int CGamePaijiuTable::NotWelfareCtrl()
{
	//BYTE    cbTableCard[CARD_COUNT];
	//重新洗牌
	//m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
	////设置扑克
	//memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
	bool needRobotChangeCard = g_RandGen.RandRatio(m_robotBankerWinPro, PRO_DENO_10000);
	bool needPlayerChangeCard = g_RandGen.RandRatio(m_playerBankerLosePro, PRO_DENO_10000);

	bool bBrankerIsRobot = false;
	bool bBrankerIsControl = false;


	bool bIsControlPlayerIsJetton = false;
	bool bIsFalgControl = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;
	int ret = 0; // add by har

	if (m_pCurBanker != NULL)
	{
		bBrankerIsRobot = m_pCurBanker->IsRobot();
		if (control_uid == m_pCurBanker->GetUID())
		{
			bBrankerIsControl = true;
			ret = 1; // add by har
		}
	}

	if (bBrankerIsControl && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetPlayerBrankerWin();
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetPlayerBrankerLost();
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

	if (!bBrankerIsControl && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (uint8 i = 0; i < AREA_COUNT; ++i)
		{
			if (m_userJettonScore[i][control_uid] > 0)
			{
				bIsControlPlayerIsJetton = true;
				ret = 1; // add by har
				break;
			}
		}
	}

	if (bIsControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetControlPlayerWin(control_uid);
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetControlPlayerLost(control_uid);
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

	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	bool bIsPoolScoreControl = false;
	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);


	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // 吐币
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerWin();
		}
		else
		{
			SetPlayerBrankerWin();
		}
		ret = 2; // add by har
	}
	if (!bBrankerIsControl && !bIsControlPlayerIsJetton  && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // 吃币
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerLost();
		}
		else
		{
			SetPlayerBrankerLost();
		}
		ret = 2; // add by har
	}

	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needRobotChangeCard && bBrankerIsRobot)
	{
		SetRobotBrankerWin(); // 庄家赢真实玩家
	}

	if (!bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && needPlayerChangeCard && !bBrankerIsRobot)
	{
		// 设置真实玩家当庄输
		SetPlayerBrankerLost();
	}

	// add by har
	if (ret == 0) {
		if (needRobotChangeCard)
			ret = 3;
		else if (needPlayerChangeCard)
			ret = 6;
		else if (SetTableCardPointKill())
			ret = 4;
	}
	// add by har end

	if (ret == 0 && SetStockWinLose())
		ret = 7;

	//判断是否满足活跃福利
	bool isAwCtrl = false;
	//if (!bBrankerIsControl && !bIsControlPlayerIsJetton && !bIsPoolScoreControl && !needPlayerChangeCard && !needRobotChangeCard)
	if (ret == 0) { // modify by har 
		isAwCtrl = ActiveWelfareCtrl();
		if (isAwCtrl)
			ret = 5; // add by har
	}

	LOG_DEBUG("CGamePaijiuTable::NotWelfareCtrl - roomid:%d,tableid:%d,bBrankerIsRobot:%d,m_robotBankerWinPro:%d,needRobotChangeCard:%d,bBrankerIsControl:%d,bIsControlPlayerIsJetton:%d,bIsPoolScoreControl:%d,isAwCtrl:%d needPlayerChangeCard:%d,ret=%d",
		GetRoomID(), GetTableID(), bBrankerIsRobot, m_robotBankerWinPro, needRobotChangeCard, bBrankerIsControl, bIsControlPlayerIsJetton, bIsPoolScoreControl, isAwCtrl, needPlayerChangeCard, ret);

	return ret /*true*/; // modify by har
}

//发送扑克
bool    CGamePaijiuTable::DispatchTableCard()
{
	//桌面扑克
	BYTE cbTableCard[CARD_COUNT];
    //重新洗牌
	m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
	//设置扑克
	memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));

	bool bIsAreaControl = false;
	int newroom = 0;
	bool bHaveNotNovicePlayer = true;
	
	//增加精准控制功能
	bIsAreaControl = OnBrcAreaControl();

	LOG_DEBUG("bIsAreaControl:%d", bIsAreaControl);

	if (!bIsAreaControl && m_pHostRoom != NULL)
	{
		newroom = m_pHostRoom->GetNoviceWelfare();
	}

	int nNotWelfareCtrlRet = 0;
	if (!bIsAreaControl &&newroom == 1) 
	{
		bHaveNotNovicePlayer = HaveNotNovicePlayer();
		if (bHaveNotNovicePlayer == false) {
			DosWelfareCtrl();
			SetChessWelfare(1);
		} else {
			nNotWelfareCtrlRet = NotWelfareCtrl();
		}
	}
	if (!bIsAreaControl &&newroom != 1)
	{
		nNotWelfareCtrlRet = NotWelfareCtrl();
	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	if (!bIsAreaControl)
	{	
		// add by har
		bool static bWinFlag0[AREA_COUNT];
		GetWinFlag(bWinFlag0, m_cbTableCardArray);
		int allWLNeedChangeIndex = -1;
		// 将牌组的大小按小到大排序
		vector<uint8> vSortCardIndexs; // 牌索引数组，按牌面值从小到大排序
		SortCardIndexs(vSortCardIndexs);

		// 打印牌组
		uint8 szCardLogic[MAX_SEAT_INDEX]; // 牌逻辑值
		uint8 szCardType[MAX_SEAT_INDEX]; // 牌型
		uint8 szCardPoint[MAX_SEAT_INDEX]; // 点数
		for (int i = 0; i < MAX_SEAT_INDEX; ++i) {
			szCardType[i] = m_GameLogic.GetCardType(m_cbTableCardArray[i], MAX_CARD);
			szCardPoint[i] = m_GameLogic.GetCardListPip(m_cbTableCardArray[i], MAX_CARD);

			//排序扑克
			uint8 cbFirstCardDataTmp[CARD_COUNT], cbNextCardDataTmp[CARD_COUNT];
			memcpy(cbFirstCardDataTmp, m_cbTableCardArray[i], sizeof(uint8)*MAX_CARD);
			m_GameLogic.SortCardList(cbFirstCardDataTmp, MAX_CARD, ST_LOGIC);
			szCardLogic[i] = m_GameLogic.GetCardLogicValue(cbFirstCardDataTmp[0]);
		}
		LOG_DEBUG("CGamePaijiuTable::DispatchTableCard show card - roomid:%d,tableid:%d,m_cbTableCardArray:%d-%d_%d-%d_%d-%d_%d-%d,szCardType:%d-%d-%d-%d,szCardPoint:%d-%d-%d-%d,szCardLogic:%d-%d-%d-%d",
			GetRoomID(), GetTableID(), m_cbTableCardArray[0][0], m_cbTableCardArray[0][1], m_cbTableCardArray[1][0], m_cbTableCardArray[1][1], 
			m_cbTableCardArray[2][0], m_cbTableCardArray[2][1], m_cbTableCardArray[3][0], m_cbTableCardArray[3][1],
			szCardType[0], szCardType[1], szCardType[2], szCardType[3],
			szCardPoint[0], szCardPoint[1], szCardPoint[2], szCardPoint[3],
			szCardLogic[0], szCardLogic[1], szCardLogic[2], szCardLogic[3]);
		// 打印牌组 end

		if (vSortCardIndexs[0] == 0 || vSortCardIndexs[3] == 0) { // 庄家通输或通赢
			if (nNotWelfareCtrlRet != 4) {
				m_bIsConputeBankerAllWinLose = true;
				++m_bankerAllWinLoseCount;
				if (m_bankerAllWinLoseCount > m_confBankerAllWinLoseLimitCount) {
					int randIndex = g_RandGen.RandRange(1, 2);
					allWLNeedChangeIndex = vSortCardIndexs[randIndex];
					uint8 tmp[MAX_CARD];
					memcpy(tmp, m_cbTableCardArray[0], MAX_CARD);
					memcpy(m_cbTableCardArray[0], m_cbTableCardArray[allWLNeedChangeIndex], MAX_CARD);
					memcpy(m_cbTableCardArray[allWLNeedChangeIndex], tmp, MAX_CARD);
				}
			}
		}

		int oldBankerAllWinLoseComputeCount = m_bankerAllWinLoseComputeCount;
		int oldBankerAllWinLoseCount = m_bankerAllWinLoseCount;
		if (m_bIsConputeBankerAllWinLose) {
			if (nNotWelfareCtrlRet != 4 || (vSortCardIndexs[0] != 0 && vSortCardIndexs[3] != 0)) {
				oldBankerAllWinLoseComputeCount = ++m_bankerAllWinLoseComputeCount;
				if (m_bankerAllWinLoseComputeCount >= m_confBankerAllWinLoseMaxCount) {
					m_bankerAllWinLoseComputeCount = 0;
					m_bankerAllWinLoseCount = 0;
					m_bIsConputeBankerAllWinLose = false;
				}
			}
		}

		bool static bWinFlag[AREA_COUNT];
		GetWinFlag(bWinFlag, m_cbTableCardArray); // add by har end

		LOG_DEBUG("CGamePaijiuTable::DispatchTableCard - roomid:%d,tableid:%d,newroom:%d,GetIsAllRobotOrPlayerJetton:%d,bHaveNotNovicePlayer:%d,ChessWelfare:%d,nNotWelfareCtrlRet:%d,allWLNeedChangeIndex:%d,BankerAllWinLoseComputeCount:%d,BankerAllWinLoseCount:%d,vSortCardIndexs:%d-%d-%d-%d, old_win:%d-%d-%d-%d-%d-%d-%d, new_win:%d-%d-%d-%d-%d-%d-%d",
			GetRoomID(), GetTableID(), newroom, GetIsAllRobotOrPlayerJetton(), bHaveNotNovicePlayer, GetChessWelfare(), nNotWelfareCtrlRet,
			allWLNeedChangeIndex, oldBankerAllWinLoseComputeCount, oldBankerAllWinLoseCount,
			vSortCardIndexs[0], vSortCardIndexs[1], vSortCardIndexs[2], vSortCardIndexs[3], 
			bWinFlag0[0], bWinFlag0[1], bWinFlag0[2], bWinFlag0[3], bWinFlag0[4], bWinFlag0[5], bWinFlag0[6],
			bWinFlag[0], bWinFlag[1], bWinFlag[2], bWinFlag[3], bWinFlag[4], bWinFlag[5], bWinFlag[6]);
	}
	//bool test_cardtype = false;
	//if (test_cardtype)
	//{
	//	m_cbTableCardArray[BANKER_INDEX][0]		= 0x22;
	//	m_cbTableCardArray[BANKER_INDEX][1]		= 0x02;
	//	m_cbTableCardArray[SHUN_MEN_INDEX][0]	= 0x31;
	//	m_cbTableCardArray[SHUN_MEN_INDEX][1]	= 0x33;
	//	m_cbTableCardArray[DUI_MEN_INDEX][0]	= 0x2C;
	//	m_cbTableCardArray[DUI_MEN_INDEX][1]	= 0x0C;
	//	//m_cbTableCardArray[DAO_MEN_INDEX][0]	= 0x38;
	//	//m_cbTableCardArray[DAO_MEN_INDEX][1]	= 0x18;
	//	m_cbTableCardArray[DAO_MEN_INDEX][0] = 0x28;
	//	m_cbTableCardArray[DAO_MEN_INDEX][1] = 0x08;
	//}

	return true;
}



//发送庄家
void    CGamePaijiuTable::SendApplyUser(CGamePlayer* pPlayer)
{
    net::msg_paijiu_apply_list msg;
   	for(uint32 nUserIdx=0; nUserIdx<m_ApplyUserArray.size(); ++nUserIdx)
	{
		CGamePlayer *pTmp = m_ApplyUserArray[nUserIdx];
		//庄家判断
		if(pTmp == m_pCurBanker) 
            continue;        
        msg.add_player_ids(pTmp->GetUID());
        msg.add_apply_score(m_ApplyUserScore[pTmp->GetUID()]);
	}
    LOG_DEBUG("发送庄家列表:%d",msg.player_ids_size());
    if(pPlayer){
       pPlayer->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_APPLY_LIST);
    }else{
       SendMsgToAll(&msg,net::S2C_MSG_PAIJIU_APPLY_LIST);
    }    
}
//更换庄家
bool    CGamePaijiuTable::ChangeBanker(bool bCancelCurrentBanker)
{
   	//切换标识
	bool bChangeBanker = false;

	//取消当前
	if(bCancelCurrentBanker)
	{
        CalcBankerScore();
		TakeTurns();
		bChangeBanker = true;
	}
	//轮庄判断
	else if(m_pCurBanker != NULL)
	{
        //自动补币
        AutoAddBankerScore();                   
		//次数判断
		if(m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime || m_lBankerScore < GetApplyBankerCondition())
		{				
            LOG_DEBUG("the timesout or the score less,you down banker:%d-%lld",m_wBankerTime,m_lBankerScore);
            CalcBankerScore();
			TakeTurns();	
			bChangeBanker       = true;
		}		
	}
	//系统做庄
	else if(m_pCurBanker == NULL && m_ApplyUserArray.size() != 0)
	{
		//轮换判断
		TakeTurns();
		bChangeBanker = true;
	}
	//切换判断
	if(bChangeBanker)
	{
		//设置变量
		m_wBankerTime       = 0;
		m_lBankerWinScore   = 0;

		//发送消息
        net::msg_paijiu_change_banker msg;
        msg.set_banker_user(GetBankerUID());
        msg.set_banker_score(m_lBankerScore);
        
        SendMsgToAll(&msg,net::S2C_MSG_PAIJIU_CHANGE_BANKER);

		SendApplyUser(NULL);
	}

	return bChangeBanker; 
}
//轮换判断
void    CGamePaijiuTable::TakeTurns()
{
    vector<uint32> delIDs;
	for(uint32 i = 0; i < m_ApplyUserArray.size(); i++)
	{
		if(GetGameState() == net::TABLE_STATE_NIUNIU_FREE)
		{
			//获取分数
			CGamePlayer *pPlayer = m_ApplyUserArray[i];
            if(pPlayer->GetNetState() == 0){
                delIDs.push_back(pPlayer->GetUID());
                continue;
            }            
			if(m_ApplyUserScore[pPlayer->GetUID()] >= GetApplyBankerCondition())
			{
				m_pCurBanker            = m_ApplyUserArray[i];
                m_lBankerScore          = m_ApplyUserScore[pPlayer->GetUID()];
                m_lBankerBuyinScore     = m_lBankerScore;      //庄家带入
                m_lBankerInitBuyinScore = m_lBankerBuyinScore;
                
                m_bankerAutoAddScore    = m_mpApplyUserInfo[pPlayer->GetUID()];//自动补币
                m_needLeaveBanker       = false;
                
                RemoveApplyBanker(pPlayer->GetUID());
                StandUpBankerSeat(pPlayer); 

                m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");
				break;
			}		
		}
	}
    for(uint16 i=0;i<delIDs.size();++i){
        RemoveApplyBanker(delIDs[i]);
    }    
}
//结算庄家
void    CGamePaijiuTable::CalcBankerScore() 
{
    if(m_pCurBanker == NULL)return;
    net::msg_paijiu_banker_calc_rep msg;
    msg.set_banker_time(m_wBankerTime);
    msg.set_win_count(m_wBankerWinTime);
    msg.set_buyin_score(m_lBankerBuyinScore);
    msg.set_win_score(m_lBankerWinScore);
    msg.set_win_max(m_lBankerWinMaxScore);
    msg.set_win_min(m_lBankerWinMinScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_BANKER_CALC);
    
    int64 score = m_lBankerWinScore;

    LOG_DEBUG("the turn the banker win:%lld,rest:%lld,buyin:%lld",score,m_lBankerScore,m_lBankerBuyinScore);
    RemoveApplyBanker(m_pCurBanker->GetUID());

    //设置庄家
	m_pCurBanker = NULL;     
    m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
    m_robotChairSize = g_RandGen.RandRange(5, 7);//机器人座位数
    
    ResetGameData();    
}
//自动补币
void    CGamePaijiuTable::AutoAddBankerScore()
{
    //轮庄判断
	if(m_pCurBanker == NULL || m_bankerAutoAddScore == 0 || m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime)
        return;
	
	//int64 lBankerScore = m_lBankerScore;
	////判断金币是否够
	//if(lBankerScore >= m_lBankerInitBuyinScore)
 //       return;
 //   int64 diffScore   = m_lBankerInitBuyinScore - lBankerScore;
 //   int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerBuyinScore;
 //   if(canAddScore < diffScore){
 //       diffScore = canAddScore;
 //   }
 //   if((m_lBankerScore + diffScore) < GetApplyBankerCondition())
 //       return;
    


	int64 diffScore = m_lBankerInitBuyinScore - m_lBankerScore;
	int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerScore;

	LOG_DEBUG("1 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	//判断金币是否够
	if (diffScore <= 0)
	{
		return;
	}
	if (canAddScore <= 0)
	{
		return;
	}
	if (canAddScore < diffScore)
	{
		diffScore = canAddScore;
	}
	if ((m_lBankerBuyinScore + diffScore) < GetApplyBankerCondition())
	{
		return;
	}

    m_lBankerBuyinScore += diffScore;
    m_lBankerScore      += diffScore;
    
    net::msg_paijiu_add_bankerscore_rep msg;
    msg.set_buyin_score(diffScore);
    
	LOG_DEBUG("5 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_PAIJIU_ADD_BANKER_SCORE);
}
//最大下注
int64   CGamePaijiuTable::GetUserMaxJetton(CGamePlayer* pPlayer, BYTE cbJettonArea)
{
	int iTimer = 1;
	//已下注额
	int64 lNowJetton = 0;
	for(int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex){ 
        lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}
	//庄家金币
	int64 lBankerScore = 0;
	if (m_pCurBanker != NULL)
	{
		lBankerScore = m_lBankerScore;
	}
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lBankerScore -= m_allJettonScore[nAreaIndex] * iTimer;
	}

	//个人限制
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton*iTimer) / iTimer;

	//庄家限制
	lMeMaxScore = min(lMeMaxScore,lBankerScore/iTimer);

	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	if (pPlayer != NULL && pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,m_pCurBanker:%p,m_lBankerScore:%lld,iTimer:%d,curScore:%lld,lNowJetton:%lld,lBankerScore:%lld,lMeMaxScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), m_pCurBanker, m_lBankerScore, iTimer, GetPlayerCurScore(pPlayer), lNowJetton, lBankerScore, lMeMaxScore);

	}

	return (lMeMaxScore);
}

//庄家站起
void    CGamePaijiuTable::StandUpBankerSeat(CGamePlayer* pPlayer)
{
    for(uint8 i=0;i < m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == pPlayer)
        {
            m_vecPlayers[i].Reset();            
            LOG_DEBUG("standup banker seat:room:%d--tb:%d,chairID:%d,uid:%d",m_pHostRoom->GetRoomID(),GetTableID(),i,pPlayer->GetUID());
            AddLooker(pPlayer);
            OnActionUserStandUp(i,pPlayer);
        }
    }
}
bool    CGamePaijiuTable::IsSetJetton(uint32 uid)
{
    //if(TABLE_STATE_NIUNIU_PLACE_JETTON != GetGameState())
    //    return false;
	if (GetGameState() == net::TABLE_STATE_NIUNIU_GAME_END)
	{
		return false;
	}
    for(uint8 i=0;i<AREA_COUNT;++i){
        if(m_userJettonScore[i][uid] > 0)
            return true;
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
bool    CGamePaijiuTable::IsInApplyList(uint32 uid)
{
	//存在判断
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == uid){
			return true;
		}
	}    
    return false;        
}    
//计算得分
int64   CGamePaijiuTable::CalculateScore()
{
	//推断玩家
	bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
	DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);
    LOG_DEBUG("win:%d-%d-%d",bWinShunMen, bWinDuiMen, bWinDaoMen);

    //胜利标识
    bool static bWinFlag[AREA_COUNT];
    bWinFlag[ID_SHUN_MEN]=bWinShunMen;
    bWinFlag[ID_JIAO_R]=(true==bWinShunMen && true==bWinDuiMen) ? true : false;
    bWinFlag[ID_HENG_L]=(true==bWinShunMen && true==bWinDaoMen) ? true : false;
    bWinFlag[ID_HENG_R]=(true==bWinShunMen && true==bWinDaoMen) ? true : false;
    bWinFlag[ID_DUI_MEN]=bWinDuiMen;
    bWinFlag[ID_DAO_MEN]=bWinDaoMen;
    bWinFlag[ID_JIAO_L]=(true==bWinDaoMen && true==bWinDuiMen) ? true : false;
    //角标识
    bool static bWinBankerJiaoL,bWinBankerJiaoR,bWinBankerQiao;
    bWinBankerJiaoR=(false==bWinShunMen && false==bWinDuiMen) ? true : false;
    bWinBankerJiaoL=(false==bWinDaoMen && false==bWinDuiMen) ? true : false;
    bWinBankerQiao=(false==bWinShunMen && false==bWinDaoMen) ? true : false;

    for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
        m_cbTableCardType[i] = m_GameLogic.GetCardType(m_cbTableCardArray[i],2);
        WriteOutCardLog(i,m_cbTableCardArray[i],2);
    }
    for(uint8 i=0;i<AREA_COUNT;++i){
        m_winMultiple[i] = bWinFlag[i] ? 1 : -1;

		// 写入点杀log add by har
		Json::Value logValue;
		logValue["pk"] = m_isTableCardPointKill[i] ? 1 : 0;
		m_operLog["area"].append(logValue);
		// add by har end
    }
	//庄家总量
	int64 lBankerWinScore = 0;

	//玩家成绩
	m_mpUserWinScore.clear();
	m_mpWinScoreForFee.clear();

	map<uint32,int64> mpUserLostScore;
	mpUserLostScore.clear();

	//计算座位积分
	for(WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
	{
		//获取用户
		CGamePlayer * pPlayer = GetPlayer(wChairID);
		if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
		{
			int64 lWinScore = 0;
            //角判断
            bool bReturnScore=false;
            if (ID_JIAO_L==wAreaIndex && false==bWinBankerJiaoL) bReturnScore=true;
            if (ID_JIAO_R==wAreaIndex && false==bWinBankerJiaoR) bReturnScore=true;
            if (ID_HENG_L==wAreaIndex && false==bWinBankerQiao) bReturnScore=true;
            if (ID_HENG_R==wAreaIndex && false==bWinBankerQiao) bReturnScore=true;
			if(true == bWinFlag[wAreaIndex])// 赢了 
			{
				m_mpUserWinScore[pPlayer->GetUID()]    += ( m_userJettonScore[wAreaIndex][pPlayer->GetUID()]) ;
				lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
				lWinScore += (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
			}
            else if(true == bReturnScore){
                m_winMultiple[wAreaIndex] = 0;
            }
			else// 输了
			{
				mpUserLostScore[pPlayer->GetUID()] -= m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				m_curr_banker_win += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				lWinScore -= m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
			}
			//lResultScore -= lWinScore;
			WriteAddScoreLog(pPlayer->GetUID(), wAreaIndex, m_userJettonScore[wAreaIndex][pPlayer->GetUID()], lWinScore);
		}
		//总的分数
		m_mpWinScoreForFee[pPlayer->GetUID()] = m_mpUserWinScore[pPlayer->GetUID()];
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
	}
    //计算旁观者积分
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
		{
			int64 lWinScore = 0;
            //角判断
            bool bReturnScore=false;
            if (ID_JIAO_L==wAreaIndex && false==bWinBankerJiaoL) bReturnScore=true;
            if (ID_JIAO_R==wAreaIndex && false==bWinBankerJiaoR) bReturnScore=true;
            if (ID_HENG_L==wAreaIndex && false==bWinBankerQiao) bReturnScore=true;
            if (ID_HENG_R==wAreaIndex && false==bWinBankerQiao) bReturnScore=true;
            if(true == bWinFlag[wAreaIndex])// 赢了
			{
				m_mpUserWinScore[pPlayer->GetUID()]    += ( m_userJettonScore[wAreaIndex][pPlayer->GetUID()]) ;
				lBankerWinScore -= ( m_userJettonScore[wAreaIndex][pPlayer->GetUID()]) ;
				lWinScore += (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
			}else if(true == bReturnScore){
                m_winMultiple[wAreaIndex] = 0;
            }
			else// 输了
			{
				mpUserLostScore[pPlayer->GetUID()] -= m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				m_curr_banker_win += m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				lWinScore -= m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
			}
			//lResultScore -= lWinScore;
			WriteAddScoreLog(pPlayer->GetUID(), wAreaIndex, m_userJettonScore[wAreaIndex][pPlayer->GetUID()], lWinScore);
		}
		//总的分数
		m_mpWinScoreForFee[pPlayer->GetUID()] = m_mpUserWinScore[pPlayer->GetUID()];
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
    }  
	//累计积分
	m_lBankerWinScore    += lBankerWinScore;
	//当前积分
    m_lBankerScore       += lBankerWinScore;
    if(lBankerWinScore > 0)m_wBankerWinTime++;
    m_lBankerWinMaxScore = MAX(lBankerWinScore,m_lBankerWinMaxScore);
    m_lBankerWinMinScore = MIN(lBankerWinScore,m_lBankerWinMinScore);
    
	//游戏记录
	for (uint32 i = 0; i < AREA_COUNT; ++i)
	{
		m_record.wins[i] = m_winMultiple[i]>0 ? 1 : 0;
	}
	m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() > 20) {//保留最近10局
		m_vecRecord.erase(m_vecRecord.begin());
	}

	//m_vecGamePlayRecord.push_back(m_record);
	//if (m_vecGamePlayRecord.size() >= 20) {
	//	m_vecGamePlayRecord.erase(m_vecGamePlayRecord.begin());
	//}
	SendGameEndLogInfo();

	bool bBrankerTakeAll = true;
	bool bBrankerCompensation = true;
	bool bIsUserPlaceJetton = false;
	for (WORD wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		if (true == bWinFlag[wAreaIndex])// 赢了
		{
			bBrankerTakeAll = false;
		}
		else if (m_winMultiple[wAreaIndex] == 0)
		{
			bBrankerTakeAll = false;
			bBrankerCompensation = false;
		}
		else
		{
			bBrankerCompensation = false;
		}
		if (m_allJettonScore[wAreaIndex]>0)
		{
			bIsUserPlaceJetton = true;
		}
	}

	if (bBrankerTakeAll && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_TAKE_ALL;
	}
	else if (bBrankerCompensation && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_COMPENSATION;
	}
	else
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_NULL;
	}

	return lBankerWinScore;
}

// 推断赢家处理 add by har
// cbTableCardArray ：根据此扑克推断
void CGamePaijiuTable::DeduceWinnerDeal(bool &bWinShunMen, bool &bWinDuiMen, bool &bWinDaoMen, uint8 cbTableCardArray[][MAX_CARD]) {
	//大小比较
	bWinShunMen = m_GameLogic.CompareCard(cbTableCardArray[BANKER_INDEX], MAX_CARD, cbTableCardArray[SHUN_MEN_INDEX], MAX_CARD) == 1 ? true : false;
	bWinDuiMen = m_GameLogic.CompareCard(cbTableCardArray[BANKER_INDEX], MAX_CARD, cbTableCardArray[DUI_MEN_INDEX], MAX_CARD) == 1 ? true : false;
	bWinDaoMen = m_GameLogic.CompareCard(cbTableCardArray[BANKER_INDEX], MAX_CARD, cbTableCardArray[DAO_MEN_INDEX], MAX_CARD) == 1 ? true : false;
}

//推断赢家
void    CGamePaijiuTable::DeduceWinner(bool &bWinShunMen, bool &bWinDuiMen, bool &bWinDaoMen)
{
    //大小比较
    /*bWinShunMen = m_GameLogic.CompareCard(m_cbTableCardArray[BANKER_INDEX],2,m_cbTableCardArray[SHUN_MEN_INDEX],2)==1?true:false;
    bWinDuiMen  = m_GameLogic.CompareCard(m_cbTableCardArray[BANKER_INDEX],2,m_cbTableCardArray[DUI_MEN_INDEX],2)==1?true:false;
    bWinDaoMen  = m_GameLogic.CompareCard(m_cbTableCardArray[BANKER_INDEX],2,m_cbTableCardArray[DAO_MEN_INDEX],2)==1?true:false;*/
	DeduceWinnerDeal(bWinShunMen, bWinDuiMen, bWinDaoMen, m_cbTableCardArray); // modify by har
}

// 获取赢的标识 add by har
void CGamePaijiuTable::GetWinFlag(bool bWinFlag[], uint8 cbTableCardArray[][MAX_CARD]) {
	bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
	DeduceWinnerDeal(bWinShunMen, bWinDuiMen, bWinDaoMen, cbTableCardArray);
	//胜利标识
	bWinFlag[ID_SHUN_MEN] = bWinShunMen;
	bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
	bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
	bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
	bWinFlag[ID_DUI_MEN] = bWinDuiMen;
	bWinFlag[ID_DAO_MEN] = bWinDaoMen;
	bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
}

// 将牌组索引按小到大排序 add by harry
void CGamePaijiuTable::SortCardIndexs(vector<uint8> &vSortCardIndexs) {
	vector<uint8> vCardIndexs = { 0, 1, 2, 3 }; // 原始牌面值索引数组
	while (!vCardIndexs.empty()) {
		uint8 minCardIndex = vCardIndexs[0];
		vector<uint8>::iterator erase_it = vCardIndexs.begin();
		if (vCardIndexs.size() != 1) {
			for (vector<uint8>::iterator it = erase_it; it != vCardIndexs.end(); ++it) {
				uint8 seat = *it;
				if (seat == minCardIndex)
					continue;
				if (m_GameLogic.CompareCard(m_cbTableCardArray[seat], MAX_CARD, m_cbTableCardArray[minCardIndex], MAX_CARD) == 1) {
					minCardIndex = seat;
					erase_it = it;
				}
			}
		}
		vSortCardIndexs.push_back(minCardIndex);
		vCardIndexs.erase(erase_it);
	}
}

// 当前牌组是否满足规则 add by harry
bool CGamePaijiuTable::IsCurTableCardRuleAllow() {
	// 将牌组的大小按小到大排序
	vector<uint8> vSortCardIndexs; // 牌索引数组，按牌面值从小到大排序
	SortCardIndexs(vSortCardIndexs);
	if (vSortCardIndexs[0] == 0 || vSortCardIndexs[3] == 0) // 庄家通输或通赢
		if (m_bankerAllWinLoseCount >= m_confBankerAllWinLoseLimitCount) // 超过限制次数则不允许出现通输通赢
			return false;

	return true;
}

// 设置点杀 add by har
bool CGamePaijiuTable::SetTableCardPointKill() {
	if (m_pCurBanker == NULL || !m_pCurBanker->IsRobot())
		return false;

	int playerMaxJettons[AREA_COUNT] = { 0 }; // 单个玩家7个区域最大下注
	bool isRobotBankerAreaPlayerLoseChange = false; // 是否触发单杀换牌
	// 获取6个区域玩家的押注金额
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		// 计算座位押注
		for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID) {
			//获取用户
			CGamePlayer *pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL || pPlayer->IsRobot()) continue;
			if (playerMaxJettons[wAreaIndex] < m_userJettonScore[wAreaIndex][pPlayer->GetUID()])
				playerMaxJettons[wAreaIndex] = m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
		}
		//计算旁观者积分
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
			CGamePlayer *pPlayer = it->second;
			if (pPlayer == NULL || pPlayer->IsRobot()) continue;
			if (playerMaxJettons[wAreaIndex] < m_userJettonScore[wAreaIndex][pPlayer->GetUID()])
				playerMaxJettons[wAreaIndex] = m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
		}
	}

	//胜利标识
	bool static bWinFlag[AREA_COUNT];
	GetWinFlag(bWinFlag, m_cbTableCardArray);

	bool isNeedChange = false;
	bool static bRobotWinFlag[AREA_COUNT];
	for (int i = 0; i < AREA_COUNT; ++i) {
		bRobotWinFlag[i] = false;
		if (playerMaxJettons[i] > m_confRobotBankerAreaPlayerWinMax) {
			bRobotWinFlag[i] = g_RandGen.RandRatio(m_confRobotBankerAreaPlayerLoseRate, PRO_DENO_10000);
			if (bRobotWinFlag[i]) {
				isRobotBankerAreaPlayerLoseChange = true;
				m_isTableCardPointKill[i] = true;
				if (bWinFlag[i])
					isNeedChange = true;
			}
		}
	}

	LOG_DEBUG("CGamePaijiuTable::SetTableCardPointKill - roomi:%d,tableid:%d,isRobotBankerAreaPlayerLoseChange:%d,isNeedChange:%d,playerMaxJettons:%d-%d-%d-%d-%d-%d-%d,bWinFlag:%d-%d-%d-%d-%d-%d-%d,bRobotWinFlag:%d-%d-%d-%d-%d-%d-%d",
		GetRoomID(), GetTableID(), isRobotBankerAreaPlayerLoseChange, isNeedChange, 
		playerMaxJettons[0], playerMaxJettons[1], playerMaxJettons[2], playerMaxJettons[3], playerMaxJettons[4], playerMaxJettons[5], playerMaxJettons[6], 
		bWinFlag[0], bWinFlag[1], bWinFlag[2], bWinFlag[3], bWinFlag[4], bWinFlag[5], bWinFlag[6],
		bRobotWinFlag[0], bRobotWinFlag[1], bRobotWinFlag[2], bRobotWinFlag[3], bRobotWinFlag[4], bRobotWinFlag[5], bRobotWinFlag[6]);
	if (!isRobotBankerAreaPlayerLoseChange)
		return false;

	if (!isNeedChange)
		return true;

	int vvAreaIndexLists_size = g_vvAreaIndexLists.size();
	int randIndex = g_RandGen.RandRange(0, vvAreaIndexLists_size - 1);
	uint8 cbTableCardArray[MAX_SEAT_INDEX][MAX_CARD];
	for (int i = 0; i < vvAreaIndexLists_size; ++i) {
		int index = randIndex + i;
		if (index >= vvAreaIndexLists_size)
			index = index - vvAreaIndexLists_size;
		ZeroMemory(cbTableCardArray, sizeof(cbTableCardArray));

		vector<uint8> &areaIndexs = g_vvAreaIndexLists[index];
		for (int j = 0; j < MAX_SEAT_INDEX; ++j)
			memcpy(cbTableCardArray[j], m_cbTableCardArray[areaIndexs[j]], MAX_CARD);

		//胜利标识
		bool static bWinFlag1[AREA_COUNT];
		GetWinFlag(bWinFlag1, cbTableCardArray);

		bool isSuc = true;
		for (int i = 0; i < AREA_COUNT; ++i) {
			if (bRobotWinFlag[i] && bWinFlag1[i]) {
				isSuc = false;
				break;
			}
		}
		if (isSuc) {
			memcpy(m_cbTableCardArray, cbTableCardArray, sizeof(m_cbTableCardArray));
			break;
		}
	}
		
	return true;
}

//申请条件
int64   CGamePaijiuTable::GetApplyBankerCondition()
{
    return GetBaseScore();
}
int64   CGamePaijiuTable::GetApplyBankerConditionLimit()
{
    return GetBaseScore()*20;
}
//次数限制
int32   CGamePaijiuTable::GetBankerTimeLimit()
{
    return m_BankerTimeLimit;
}

void    CGamePaijiuTable::OnRobotOper()
{
	/*
    LOG_DEBUG("robot place jetton");
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;        
        uint8 area = g_RandGen.RandRange(ID_HENG_L,ID_HENG_R);
        if(g_RandGen.RandRatio(50,PRO_DENO_100))
            continue;
		//int64 minJetton = GetUserMaxJetton(pPlayer,area)/10;
		//if (minJetton > 100)
		//{
		//	minJetton = (minJetton / 100) * 100;
		//}
		//else
		//{
		//	continue;
		//}
		int64 minJetton = GetRobotJettonScore(pPlayer, area);
		if (minJetton == 0)
		{
			continue;
		}
        if(!OnUserPlaceJetton(pPlayer,area,minJetton))
            break;
    }
	*/
	/*
    for(uint32 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
		int iJettonCount = g_RandGen.RandRange(1, 9);
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			uint8 area = g_RandGen.RandRange(ID_HENG_L, ID_HENG_R);
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
				continue;
			//int64 minJetton = GetUserMaxJetton(pPlayer,area)/10;
			//if (minJetton > 100)
			//{
			//	minJetton = (minJetton / 100) * 100;
			//}
			//else
			//{
			//	continue;
			//}
			int64 minJetton = GetRobotJettonScore(pPlayer, area);
			if (minJetton == 0)
			{
				continue;
			}
			if (!OnUserPlaceJetton(pPlayer, area, minJetton))
				break;
		}
    }
	*/
}

int64 CGamePaijiuTable::GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area)
{
	int64 lUserRealJetton = 1000;
	int64 lUserMinJetton = 1000;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, area);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);
	if (lUserCurJetton < 2000)
	{
		lUserRealJetton = 0;
	}
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000)
	{
		if (g_RandGen.RandRatio(77, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
		{
			lUserRealJetton = 5000;
		}
		else
		{
			lUserRealJetton = 10000;
		}
	}
	else if (lUserCurJetton >= 50000 && lUserCurJetton < 200000)
	{
		if (g_RandGen.RandRatio(60, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(20, PRO_DENO_100))
		{
			lUserRealJetton = 10000;
		}
		else
		{
			lUserRealJetton = 50000;
		}
	}
	else if (lUserCurJetton >= 200000 && lUserCurJetton < 2000000)
	{
		if (g_RandGen.RandRatio(3500, PRO_DENO_10000))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(2300, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3200, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(800, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else
		{
			lUserRealJetton = 100000;
		}
	}
	else if (lUserCurJetton >= 2000000)
	{
		if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(4150, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(2000, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else
		{
			lUserRealJetton = 100000;
		}
	}
	else
	{
		lUserRealJetton = 1000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100000)
	{
		lUserRealJetton = 50000;
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
		lUserRealJetton = 0;
	}
	if (lUserRealJetton < lUserMinJetton)
	{
		lUserRealJetton = 0;
	}

	return lUserRealJetton;
}

void    CGamePaijiuTable::OnRobotStandUp()
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
void    CGamePaijiuTable::CheckRobotApplyBanker()
{   
    if(m_pCurBanker != NULL || m_ApplyUserArray.size() >= m_robotApplySize)
        return;
    LOG_DEBUG("robot apply banker");
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;
        int64 curScore = GetPlayerCurScore(pPlayer);
        if(curScore < GetApplyBankerCondition())
            continue;
        
        int64 buyinScore = GetApplyBankerCondition()*2;
        if(curScore < buyinScore)
        {
            OnUserApplyBanker(pPlayer,curScore,0);            
        }else{
            buyinScore = g_RandGen.RandRange(buyinScore,curScore);
            buyinScore = (buyinScore/10000)*10000;
            
            OnUserApplyBanker(pPlayer,buyinScore,1);     
        }        
        if(m_ApplyUserArray.size() > m_robotApplySize)
            break;        
    }        
}
void    CGamePaijiuTable::AddPlayerToBlingLog()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)
            continue;
        for(uint8 i=0;i<AREA_COUNT;++i){
            if(m_userJettonScore[i][pPlayer->GetUID()] > 0){
                AddUserBlingLog(pPlayer);
                break;
            }
        }           
    }              
	AddUserBlingLog(m_pCurBanker);
}

//发送游戏记录
void  CGamePaijiuTable::SendPlayLog(CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer!=NULL)
	{
		uid = pPlayer->GetUID();
	}
	net::msg_paijiu_play_log_rep msg;
	for (uint16 i = 0; i<m_vecRecord.size(); ++i)
	{
		net::paijiu_play_log* plog = msg.add_logs();
		paijiuGameRecord& record = m_vecRecord[i];
		for (uint16 j = 0; j<AREA_COUNT; j++) {
			plog->add_seats_win(record.wins[j]);
		}
	}
	LOG_DEBUG("发送牌局记录 size:%d,uid:%d", msg.logs_size(),uid);
	if (pPlayer != NULL) {
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_PLAY_LOG);
	}
	else {
		SendMsgToAll(&msg, net::S2C_MSG_PAIJIU_PLAY_LOG);
	}
}




bool    CGamePaijiuTable::IsInTableRobot(uint32 uid, CGamePlayer * pPlayer)
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


bool CGamePaijiuTable::OnChairRobotJetton()
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
		uint8 cbJettonArea = g_RandGen.RandRange(ID_HENG_L, ID_HENG_R);
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
				cbJettonArea = g_RandGen.RandRange(ID_HENG_L, ID_HENG_R);

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

void	CGamePaijiuTable::OnChairRobotPlaceJetton()
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

bool CGamePaijiuTable::OnRobotJetton()
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
		uint8 cbJettonArea = g_RandGen.RandRange(ID_HENG_L, ID_HENG_R);
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
				cbJettonArea = g_RandGen.RandRange(ID_HENG_L, ID_HENG_R);

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


void	CGamePaijiuTable::OnRobotPlaceJetton()
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

bool CGamePaijiuTable::ActiveWelfareCtrl()
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

        if(SetAWPlayerWin(control_uid, iter->max_win))
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

bool CGamePaijiuTable::SetAWPlayerWin(uint32 control_uid, int64 max_win)
{
    LOG_DEBUG("enter SetAWPlayerWin function. control_uid:%d max_win:%lld", control_uid, max_win);

    BYTE    cbTableCard[CARD_COUNT];

    uint32 brankeruid = GetBankerUID();

    int64 control_win_score = 0;
    int64 control_lose_score = 0;
    int64 control_result_score = 0;
    int irount_count = 1000;
    int iRountIndex = 0;
    bool IsFind = false;
    for (; iRountIndex < irount_count; iRountIndex++)
    {
        //推断玩家
        bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
        DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

        //胜利标识
        bool static bWinFlag[AREA_COUNT];
        bWinFlag[ID_SHUN_MEN] = bWinShunMen;
        bWinFlag[ID_JIAO_R] = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
        bWinFlag[ID_HENG_L] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
        bWinFlag[ID_HENG_R] = (true == bWinShunMen && true == bWinDaoMen) ? true : false;
        bWinFlag[ID_DUI_MEN] = bWinDuiMen;
        bWinFlag[ID_DAO_MEN] = bWinDaoMen;
        bWinFlag[ID_JIAO_L] = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
        //角标识
        bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
        bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
        bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
        bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

        int32 winMultiple[AREA_COUNT];
        memset(winMultiple, 0, sizeof(winMultiple));
        for (uint8 i = 0; i < AREA_COUNT; ++i) {
            winMultiple[i] = bWinFlag[i] ? 1 : -1;
        }

        control_win_score = 0;
        control_lose_score = 0;
        control_result_score = 0;

        for (WORD wAreaIndex = 0; wAreaIndex <= ID_HENG_R; ++wAreaIndex)
        {
            if (m_userJettonScore[wAreaIndex][control_uid] == 0)
                continue;
            //角判断
            bool bReturnScore = false;
            if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
            if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
            if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
            if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
            if (true == bWinFlag[wAreaIndex])// 赢了 
            {
                control_win_score += (m_userJettonScore[wAreaIndex][control_uid]);
            }
            else if (true == bReturnScore) {
                winMultiple[wAreaIndex] = 0;
            }
            else
            {
                control_lose_score -= m_userJettonScore[wAreaIndex][control_uid];
            }
        }
        control_result_score = control_win_score + control_lose_score;
        if (control_result_score > 0 && control_result_score <= max_win)
        {
            LOG_DEBUG("search success control_uid:%d max_win:%lld control_result_score:%lld", control_uid, max_win, control_result_score);
            IsFind = true;
            break;
        }
        else {
            //重新洗牌
            m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
            //设置扑克
            memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
        }
    }
    return IsFind;
}


void CGamePaijiuTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_paijiu_play_log_rep * pplay = pInfo->mutable_paijiu();
	for (uint16 i = 0; i < m_vecRecord.size(); ++i)
	{
		net::paijiu_play_log* plog = pplay->add_logs();
		paijiuGameRecord& record = m_vecRecord[i];
		for (uint16 j = 0; j < AREA_COUNT; j++) {
			plog->add_seats_win(record.wins[j]);
		}
	}
}

void CGamePaijiuTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_paijiu_play_log_rep * pplay = pInfo->mutable_paijiu();
	net::paijiu_play_log* plog = pplay->add_logs();
	paijiuGameRecord& record = m_record;
	for (uint16 j = 0; j < AREA_COUNT; j++) {
		plog->add_seats_win(record.wins[j]);
	}
}

void CGamePaijiuTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer)
{
	if (!pPlayer)
	{
		return;
	}
	LOG_DEBUG("send brc control all true player info list uid:%d.", pPlayer->GetUID());

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
		for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
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
		for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

void CGamePaijiuTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer)
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
			for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
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
			for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
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

void CGamePaijiuTable::OnBrcControlSendAllRobotTotalBetInfo()
{
	LOG_DEBUG("notice brc control all robot totol bet info.");

	net::msg_brc_control_total_robot_bet_info rep;
	for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_allJettonScore[wAreaIndex] - m_playerJettonScore[wAreaIndex]);
		LOG_DEBUG("wAreaIndex:%d m_allJettonScore[%d]:%lld m_playerJettonScore[%d]:%lld", wAreaIndex, wAreaIndex, m_allJettonScore[wAreaIndex], wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_ROBOT_BET_INFO);
	}
}

void CGamePaijiuTable::OnBrcControlSendAllPlayerTotalBetInfo()
{
	LOG_DEBUG("notice brc control all player totol bet info.");

	net::msg_brc_control_total_player_bet_info rep;
	for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_PLAYER_BET_INFO);
	}
}

bool CGamePaijiuTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer)
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
		OnBrcControlFlushAppleList();

		return true;
	}
	return false;
}

void CGamePaijiuTable::OnBrcControlBetDeal(CGamePlayer* pPlayer)
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

bool CGamePaijiuTable::OnBrcAreaControl()
{
	LOG_DEBUG("brc area control.");

	if (m_real_control_uid == 0)
	{
		LOG_DEBUG("brc area control the control uid is zero.");
		return false;
	}

	//获取当前控制区域
	bool ctrl_area_a = false;
	vector<uint8> ctrl_area_a_list;	//A 区域 顺门 对门 倒门 支持多个

	uint8 ctrl_area_b = AREA_MAX;	//B 区域 庄赢/庄输/左角/右角/横
	for (uint8 i = 0; i < AREA_MAX; ++i)
	{
		if (m_req_control_area[i] == 1)
		{
			if (i == AREA_SHUN_MEN || i == AREA_DUI_MEN || i == AREA_DAO_MEN)	//A 区域控制
			{
				ctrl_area_a_list.push_back(i);
				ctrl_area_a = true;
			}
			else    //B 区域控制
			{
				ctrl_area_b = i;
			}
		}
	}
	if (!ctrl_area_a && ctrl_area_b == AREA_MAX)
	{
		LOG_DEBUG("brc area control the ctrl_area is none.");
		return false;
	}

	//判断当前执行的控制是A区域还是B区域
	if (ctrl_area_a)
	{
		return OnBrcAreaControlForA(ctrl_area_a_list);
	}

	if (ctrl_area_b != AREA_MAX)
	{
		return OnBrcAreaControlForB(ctrl_area_b);
	}

	return false;
}

void CGamePaijiuTable::OnBrcFlushSendAllPlayerInfo()
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
		for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
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
		for (WORD wAreaIndex = ID_HENG_L; wAreaIndex < AREA_COUNT; ++wAreaIndex)
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

bool CGamePaijiuTable::OnBrcAreaControlForA(vector<uint8> &area_list)
{
	LOG_DEBUG("brc area control for A. size:%d", area_list.size());

	for (uint8 i = 0; i < area_list.size(); i++)
	{
		LOG_DEBUG("brc area control for A. id:%d", area_list[i]);
	}

	BYTE    cbTableCard[CARD_COUNT];				//桌面扑克

	//多次遍历结果
	int irount_count = 5000;
	int iRountIndex = 0;
	bool find_flag = false;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//3个区域都是赢，庄家通赔
		if (area_list.size() == 3)
		{
			if (bWinShunMen && bWinDuiMen && bWinDaoMen)
			{
				find_flag = true;
				break;
			}
		}
		//2个区域控制
		if (area_list.size() == 2)
		{
			//顺门 + 对门
			if (area_list[0] == AREA_SHUN_MEN && area_list[1] == AREA_DUI_MEN)
			{
				if (bWinShunMen && bWinDuiMen && !bWinDaoMen)
				{
					find_flag = true;
					break;
				}
			}
			//顺门 + 倒门
			if (area_list[0] == AREA_SHUN_MEN && area_list[1] == AREA_DAO_MEN)
			{
				if (bWinShunMen && bWinDaoMen && !bWinDuiMen)
				{
					find_flag = true;
					break;
				}
			}
			//对门 + 顺门
			if (area_list[0] == AREA_DUI_MEN && area_list[1] == AREA_SHUN_MEN)
			{
				if (bWinDuiMen && bWinShunMen && !bWinDaoMen)
				{
					find_flag = true;
					break;
				}
			}
			//对门 + 倒门
			if (area_list[0] == AREA_DUI_MEN && area_list[1] == AREA_DAO_MEN)
			{
				if (bWinDuiMen && bWinDaoMen && !bWinShunMen)
				{
					find_flag = true;
					break;
				}
			}
			//倒门 + 顺门
			if (area_list[0] == AREA_DAO_MEN && area_list[1] == AREA_SHUN_MEN)
			{
				if (bWinDaoMen && bWinShunMen && !bWinDuiMen)
				{
					find_flag = true;
					break;
				}
			}
			//倒门 + 对门
			if (area_list[0] == AREA_DAO_MEN && area_list[1] == AREA_DUI_MEN)
			{
				if (bWinDaoMen && bWinDuiMen && !bWinShunMen)
				{
					find_flag = true;
					break;
				}
			}
		}

		//1个区域控制
		if (area_list.size() == 1)
		{
			if (area_list[0] == AREA_SHUN_MEN && bWinShunMen && !bWinDuiMen && !bWinDaoMen)
			{
				find_flag = true;
				break;
			}
			if (area_list[0] == AREA_DUI_MEN && bWinDuiMen && !bWinShunMen && !bWinDaoMen)
			{
				find_flag = true;
				break;
			}
			if (area_list[0] == AREA_DAO_MEN && bWinDaoMen && !bWinShunMen && !bWinDuiMen)
			{
				find_flag = true;
				break;
			}
		}

		if (find_flag)
		{
			break;
		}
		else
		{
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	if (find_flag)
	{
		LOG_DEBUG("get area ctrl A is success - roomid:%d,tableid:%d", m_pHostRoom->GetRoomID(), GetTableID());
		return true;
	}
	else
	{
		LOG_DEBUG("get area ctrl A is fail - roomid:%d,tableid:%d", m_pHostRoom->GetRoomID(), GetTableID());
		return false;
	}

	return false;
}

bool CGamePaijiuTable::OnBrcAreaControlForB(uint8 ctrl_area_b)
{
	LOG_DEBUG("brc area control for B. ctrl_area_b:%d", ctrl_area_b);

	//判断是否设置为庄赢或庄输
	if (ctrl_area_b == AREA_BANK)
	{
		return SetPlayerBrankerWin();
	}

	if (ctrl_area_b == AREA_XIAN)
	{
		return SetPlayerBrankerLost();
	}

	BYTE    cbTableCard[CARD_COUNT];				//桌面扑克

	//多次遍历结果
	int irount_count = 1000;
	int iRountIndex = 0;
	bool find_flag = false;
	for (; iRountIndex < irount_count; iRountIndex++)
	{
		//推断玩家
		bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
		DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

		//角标识
		bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
		bWinBankerJiaoR = (true == bWinShunMen && true == bWinDuiMen) ? true : false;
		bWinBankerJiaoL = (true == bWinDaoMen && true == bWinDuiMen) ? true : false;
		bWinBankerQiao = (true == bWinShunMen && true == bWinDaoMen) ? true : false;

		//判断横
		if (ctrl_area_b == AREA_HENG && bWinBankerQiao && !bWinBankerJiaoL && !bWinBankerJiaoR)
		{
			find_flag = true;
		}

		//判断左角
		if (ctrl_area_b == AREA_JIAO_L && bWinBankerJiaoL && !bWinBankerQiao && !bWinBankerJiaoR)
		{
			find_flag = true;
		}

		//判断右角
		if (ctrl_area_b == AREA_JIAO_R && bWinBankerJiaoR && !bWinBankerQiao && !bWinBankerJiaoL)
		{
			find_flag = true;
		}

		if (find_flag)
		{
			break;
		}
		else {
			//重新洗牌
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//设置扑克
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	if (find_flag)
	{
		LOG_DEBUG("get area ctrl B is success - roomid:%d,tableid:%d,ctrl_area_b:%d", m_pHostRoom->GetRoomID(), GetTableID(), ctrl_area_b);
		return true;
	}
	else
	{
		LOG_DEBUG("get area ctrl B is fail - roomid:%d,tableid:%d,ctrl_area_b:%d", m_pHostRoom->GetRoomID(), GetTableID(), ctrl_area_b);
		return false;
	}
}

void CGamePaijiuTable::OnNotityForceApplyUser(CGamePlayer* pPlayer)
{
	LOG_DEBUG("Notity Force Apply uid:%d.", pPlayer->GetUID());

	net::msg_paijiu_apply_banker_rep msg;
	msg.set_apply_oper(0);
	msg.set_result(net::RESULT_CODE_SUCCESS);

	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_PAIJIU_APPLY_BANKER);
}

// 获取单个下注的是机器人还是玩家  add by har
void CGamePaijiuTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
			continue;
		if (pPlayer->IsRobot())
			isAllPlayer = false;
		else
		    isAllRobot = false;
		return;
	}
}

// 获取某个玩家的赢分  add by har
int64 CGamePaijiuTable::GetSinglePlayerWinScore(CGamePlayer *pPlayer, bool bWinFlag[AREA_COUNT], int64 &lBankerWinScore) {
	if (pPlayer == NULL)
		return 0;
	//角标识
	bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
	bWinBankerJiaoR = (false == bWinFlag[ID_SHUN_MEN] && false == bWinFlag[ID_DUI_MEN]) ? true : false;
	bWinBankerJiaoL = (false == bWinFlag[ID_DAO_MEN] && false == bWinFlag[ID_DUI_MEN]) ? true : false;
	bWinBankerQiao = (false == bWinFlag[ID_SHUN_MEN] && false == bWinFlag[ID_DAO_MEN]) ? true : false;
	uint32 playerUid = pPlayer->GetUID();
	int64 playerScoreWin = 0; // 该玩家总赢分
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		int64 scoreWin = m_userJettonScore[wAreaIndex][playerUid];
		if (scoreWin == 0)
			continue;
		//角判断
		bool bReturnScore = false;
		if (ID_JIAO_L == wAreaIndex && false == bWinBankerJiaoL) bReturnScore = true;
		if (ID_JIAO_R == wAreaIndex && false == bWinBankerJiaoR) bReturnScore = true;
		if (ID_HENG_L == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
		if (ID_HENG_R == wAreaIndex && false == bWinBankerQiao) bReturnScore = true;
		if (true == bWinFlag[wAreaIndex]) // 赢了 
			playerScoreWin += scoreWin;
		else if (false == bReturnScore) // 输了
			playerScoreWin -= scoreWin;
	}
	lBankerWinScore -= playerScoreWin;
	if (pPlayer->IsRobot())
		return 0;
	return playerScoreWin;
}

// 获取非机器人玩家赢分 add by har
int64 CGamePaijiuTable::GetBankerAndPlayerWinScore() {
	int64 playerAllWinScore = 0; // 非机器人玩家总赢数

	//推断玩家
	bool static bWinShunMen, bWinDuiMen, bWinDaoMen;
	DeduceWinner(bWinShunMen, bWinDuiMen, bWinDaoMen);

	//胜利标识
	bool static bWinFlag[AREA_COUNT];
	GetWinFlag(bWinFlag, m_cbTableCardArray);
	//角标识
	bool static bWinBankerJiaoL, bWinBankerJiaoR, bWinBankerQiao;
	bWinBankerJiaoR = (false == bWinShunMen && false == bWinDuiMen) ? true : false;
	bWinBankerJiaoL = (false == bWinDaoMen && false == bWinDuiMen) ? true : false;
	bWinBankerQiao = (false == bWinShunMen && false == bWinDaoMen) ? true : false;

	int64 lBankerWinScore = 0;

	//计算座位积分
	for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
		playerAllWinScore += GetSinglePlayerWinScore(GetPlayer(wChairID), bWinFlag, lBankerWinScore);
	//计算旁观者积分
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it)
		playerAllWinScore += GetSinglePlayerWinScore(it->second, bWinFlag, lBankerWinScore);

	if (IsBankerRealPlayer())
		playerAllWinScore += lBankerWinScore;

	return playerAllWinScore;
}

// 设置库存输赢  add by har
bool CGamePaijiuTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	uint8 cbTableCard[CARD_COUNT];
	int64 playerAllWinScore = 0;
	int i = 0;
	// 循环，直到找到满足条件的牌组合
	while (true) {
		playerAllWinScore = GetBankerAndPlayerWinScore();
		if (IsCurTableCardRuleAllow() && CheckStockChange(stockChange, playerAllWinScore, i)) {
			LOG_DEBUG("SetStockWinLose suc  roomid:%d,tableid:%d,stockChange:%lld,i:%d,playerAllWinScore:%lld,IsBankerRealPlayer:%d",
				GetRoomID(), GetTableID(), stockChange, i, playerAllWinScore, IsBankerRealPlayer());
			return true;
		}
		if (++i > 999)
			break;
		//重新洗牌
		m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
		//设置扑克
		memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
	}
	LOG_ERROR("SetStockWinLose fail roomid:%d,tableid:%d,playerAllWinScore:%lld,stockChange:%d", GetRoomID(), GetTableID(), playerAllWinScore, stockChange);
	return false;
}
