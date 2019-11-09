
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
using namespace game_dice;

namespace
{
    const static uint32 s_FreeTime              = 8*1000;       // 空闲时间
    const static uint32 s_PlaceJettonTime       = 30*1000;      // 下注时间
    const static uint32 s_DispatchTime          = 16*1000;      // 发牌时间
    
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 游戏
        {
            pTable = new CGameDiceTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛
        {
            pTable = new CGameDiceTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人
        {
            pTable = new CGameDiceTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

CGameDiceTable::CGameDiceTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
	m_vecPlayers.clear();

	ResetGameData();
	
    return;
}
CGameDiceTable::~CGameDiceTable()
{

}
bool    CGameDiceTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
    LOG_DEBUG("uid:%d",pPlayer->GetUID());
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
bool    CGameDiceTable::CanLeaveTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
	{
		return false;
	}
	if (IsInApplyList(pPlayer->GetUID()))
	{
		return false;
	}
	if (IsInRobotAlreadyJetton(pPlayer->GetUID(), pPlayer))
	{
		return false;
	}
    return true;
}
bool    CGameDiceTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (GetPlayerCurScore(pPlayer) < m_pHostRoom->GetSitDown())
	{
		LOG_DEBUG("the sitdown condition not have :%lld", m_pHostRoom->GetSitDown());
		return false;
	}
	if (!IsExistLooker(pPlayer->GetUID())) {
		LOG_DEBUG("not in looklist:%d", pPlayer->GetUID());
		return false;
	}
	if (chairID >= m_vecPlayers.size()) {
		LOG_DEBUG("the seat is more big:%d", chairID);
		return false;
	}
	if (m_vecPlayers[chairID].pPlayer != NULL) {
		LOG_DEBUG("the seat is other player");
		return false;
	}
	return true;
}
bool    CGameDiceTable::CanStandUp(CGamePlayer* pPlayer)
{
    return true;
}
bool    CGameDiceTable::IsFullTable()
{
	if (m_mpLookers.size() >= 200)
	{
		return true;
	}
    
    return false;
}
void CGameDiceTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::dice_table_info* pdice = pInfo->mutable_dice();
    pdice->set_tableid(GetTableID());
    pdice->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pdice->set_is_passwd(1);
    }else{
        pdice->set_is_passwd(0);
    }
    pdice->set_basescore(m_conf.baseScore);
    pdice->set_consume(m_conf.consume);
    pdice->set_entermin(m_conf.enterMin);
    pdice->set_duetime(m_conf.dueTime);
    pdice->set_feetype(m_conf.feeType);
    pdice->set_feevalue(m_conf.feeValue);
    pdice->set_hostname(m_conf.hostName);
	pdice->set_free_time(s_FreeTime);
    pdice->set_card_time(s_PlaceJettonTime);
	pdice->set_dispatch_time(s_DispatchTime);
    pdice->set_table_state(GetGameState());
    pdice->set_sitdown(m_pHostRoom->GetSitDown());
    pdice->set_apply_banker_condition(GetApplyBankerCondition());
    pdice->set_apply_banker_maxscore(GetApplyBankerConditionLimit());
    pdice->set_banker_max_time(m_BankerTimeLimit);
}

bool CGameDiceTable::CompareIncomeScoreRank(tagDiceGameDataRank tmpData1, tagDiceGameDataRank tmpData2)
{
	return tmpData1.score > tmpData2.score;
}

bool CGameDiceTable::SortIncomeScoreRank()
{
	if (m_vecIncomeScoreRank.size() == 0)
	{
		return false;
	}

	//for (uint32 i = 0; i < m_vecIncomeScoreRank.size(); i++)
	//{
	//	LOG_DEBUG("begin - uid:%d,score:%lld,nickname:%s,city:%s", m_vecIncomeScoreRank[i].uid, m_vecIncomeScoreRank[i].score, m_vecIncomeScoreRank[i].nick_name.c_str(), m_vecIncomeScoreRank[i].city.c_str());
	//}

	sort(m_vecIncomeScoreRank.begin(), m_vecIncomeScoreRank.end(), CompareIncomeScoreRank);

	//for (uint32 i = 0; i < m_vecIncomeScoreRank.size(); i++)
	//{
	//	LOG_DEBUG("end - uid:%d,score:%lld,nickname:%s,city:%s", m_vecIncomeScoreRank[i].uid, m_vecIncomeScoreRank[i].score, m_vecIncomeScoreRank[i].nick_name.c_str(), m_vecIncomeScoreRank[i].city.c_str());
	//}

	return true;
}


//配置桌子
bool CGameDiceTable::Init()
{
    SetGameState(net::TABLE_STATE_DICE_FREE);
	m_vecPlayers.clear();
    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    //m_BankerTimeLimit = 3;
	m_BankerTimeLimit = 6;// CApplication::Instance().call<int>("dicebankertime");
    //m_robotBankerWinPro = CApplication::Instance().call<int>("bankerwin");
	m_robotBankerWinPro = 500;
    m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
    m_robotChairSize = g_RandGen.RandRange(2, 4);//机器人座位数
    m_coolLogic.beginCooling(s_FreeTime);
	
	m_tagDiceJackpotInfo.Init();

	ReAnalysisParam();
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGameDiceTable::ReAnalysisParam()
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (param.empty() || !reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", m_pHostRoom->GetRoomID(), param.c_str());
		return true;
	}

	int iIsUpdateCurPollScore = 0;
	if (jvalue.isMember("ucp") && jvalue["ucp"].isIntegral())
	{
		iIsUpdateCurPollScore = jvalue["ucp"].asInt();
	}
	if (iIsUpdateCurPollScore == 1)
	{
		if (jvalue.isMember("cps") && jvalue["cps"].isIntegral())
		{
			int64 lCurPollScore = jvalue["cps"].asInt64();
			m_tagDiceJackpotInfo.SetCurPoolScore(lCurPollScore);
		}
	}
	if (jvalue.isMember("par") && jvalue["par"].isIntegral()) {
		m_tagDiceJackpotInfo.uPoolAddRatio = jvalue["par"].asInt();
	}
	if (jvalue.isMember("psp") && jvalue["psp"].isIntegral()) {
		m_tagDiceJackpotInfo.uPoolSubPoint = jvalue["psp"].asInt();
	}
	if (jvalue.isMember("pof") && jvalue["pof"].isIntegral()) {
		m_tagDiceJackpotInfo.uPoolSubOneFive = jvalue["pof"].asInt();
	}
	if (jvalue.isMember("psr") && jvalue["psr"].isIntegral()) {
		m_tagDiceJackpotInfo.uPoolSubSixRatio = jvalue["psr"].asInt();
	}
	if (jvalue.isMember("rbw") && jvalue["rbw"].isIntegral()) {
		m_robotBankerWinPro = jvalue["rbw"].asInt();
	}
	if (jvalue.isMember("rsp") && jvalue["rsp"].isIntegral()) {
		m_tagDiceJackpotInfo.uRobotSubProbability = jvalue["rsp"].asInt();
	}
	if (jvalue.isMember("mps") && jvalue["mps"].isIntegral()) {
		m_tagDiceJackpotInfo.lMaxPollScore = jvalue["mps"].asInt64();
	}

	LOG_ERROR("reader json parse success - roomid:%d,tableid:%d,iIsUpdateCurPollScore:%d,lCurPollScore:%lld,uPoolAddRatio:%d,uPoolSubPoint:%d,uPoolSubOneFive:%d,uPoolSubSixRatio:%d,m_robotBankerWinPro:%d,uRobotSubProbability:%d,lMaxPollScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), iIsUpdateCurPollScore, m_tagDiceJackpotInfo.lCurPollScore, m_tagDiceJackpotInfo.uPoolAddRatio, m_tagDiceJackpotInfo.uPoolSubPoint, m_tagDiceJackpotInfo.uPoolSubOneFive, m_tagDiceJackpotInfo.uPoolSubSixRatio, m_robotBankerWinPro, m_tagDiceJackpotInfo.uRobotSubProbability, m_tagDiceJackpotInfo.lMaxPollScore);
	return true;
}

void CGameDiceTable::ShutDown()
{
    //CalcBankerScore();
}
//复位桌子
void CGameDiceTable::ResetTable()
{
    ResetGameData();
}
void CGameDiceTable::OnTimeTick()
{
	OnTableTick();

    uint8 tableState = GetGameState();
    if(m_coolLogic.isTimeOut())
    {
        switch(tableState)
        {
        case TABLE_STATE_DICE_FREE:           // 空闲
            {
                if(OnGameStart())
				{
					InitChessID();
                    SetGameState(TABLE_STATE_DICE_PLACE_JETTON);
              
                }
				else
				{
                    m_coolLogic.beginCooling(s_FreeTime);
                }
            }break;
        case TABLE_STATE_DICE_PLACE_JETTON:   // 下注时间
            {
                SetGameState(TABLE_STATE_DICE_GAME_END);
                m_coolLogic.beginCooling(s_DispatchTime);
				DispatchTableCard();
                OnGameEnd(INVALID_CHAIR,GER_NORMAL);
            }break;
        case TABLE_STATE_DICE_GAME_END:       // 结束游戏
            {
    			//切换庄家
    			ClearTurnGameData();
    			//ChangeBanker(false);
                SetGameState(TABLE_STATE_DICE_FREE);
                m_coolLogic.beginCooling(s_FreeTime);
                
            }break;
        default:
            break;
        }
    }
    if(tableState == TABLE_STATE_DICE_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
    {
        OnRobotOper();
		OnRobotPlaceJetton();
		RemainAreaRobotJetton();
		OnChairRobotPlaceJetton();
    }
    
}
// 游戏消息
int CGameDiceTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    if(pPlayer==NULL){
        return 0;
    }
    LOG_DEBUG("table recv msg:%d--%d", pPlayer->GetUID(),cmdID);
    switch(cmdID)
    {
    case net::C2S_MSG_DICE_PLACE_JETTON:  // 用户加注
        {
            if(GetGameState() != TABLE_STATE_DICE_PLACE_JETTON){
                LOG_DEBUG("not jetton state can't jetton - uid:%d", pPlayer->GetUID());
                return 0;
            }
            net::msg_dice_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            return OnUserPlaceJetton(pPlayer,msg.jetton_area(),msg.jetton_score());
        }break;
	case net::C2S_MSG_DICE_CANCEL_JETTON:  // 用户取消下注
	{
		if (GetGameState() != net::TABLE_STATE_DICE_PLACE_JETTON) {
			LOG_DEBUG("not jetton state can't cancel jetton - uid:%d", pPlayer->GetUID());
			return 0;
		}
		net::msg_dice_place_jetton_req msg;
		PARSE_MSG_FROM_ARRAY(msg);
		return OnUserCancelJetton(pPlayer);
	}break;
    case net::C2S_MSG_DICE_APPLY_BANKER:  // 申请庄家
        {
			return 1;
            net::msg_dice_apply_banker msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(msg.apply_oper() == 1){
                return OnUserApplyBanker(pPlayer,msg.apply_score(),msg.auto_addscore());
            }else{
                return OnUserCancelBanker(pPlayer);
            }
        }break;
    case net::C2S_MSG_DICE_JUMP_APPLY_QUEUE:// 插队
        {
			return 1;
            net::msg_dice_jump_apply_queue_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
            return OnUserJumpApplyQueue(pPlayer);
        }break;
	case net::C2S_MSG_DICE_JACKPOT_INFO_REQ:
		{
			net::dice_jackpot_info msg;
			// 奖池信息信息
			for (uint8 i = 0; i<DICE_COUNT; ++i)
			{
				msg.add_table_cards(m_tagDiceJackpotInfo.cbTableDice[i]);
			}
			msg.set_utime(m_tagDiceJackpotInfo.utime);
			msg.set_win_total_score(m_tagDiceJackpotInfo.lWinTotalScore);
			msg.set_cur_jackpot_score(m_tagDiceJackpotInfo.lCurPollScore);

			for (uint32 uIndex = 0; uIndex < m_vecIncomeScoreRank.size(); uIndex++)
			{
				net::dice_game_score_rank * pdice_game_score_rank = msg.add_score_rank();
				pdice_game_score_rank->set_uid(m_vecIncomeScoreRank[uIndex].uid);
				pdice_game_score_rank->set_nick_name(m_vecIncomeScoreRank[uIndex].nick_name);
				pdice_game_score_rank->set_income_score(m_vecIncomeScoreRank[uIndex].score);
			}
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_JACKPOT_INFO_REP);
		}break;
	case net::C2S_MSG_DICE_CONTROL_REQ:
		{
			LOG_DEBUG("1 playerControlDice - uid:%d,m_bIsControlDice:%d,dice:%d %d %d",
				pPlayer->GetUID(), m_GameLogic.m_bIsControlDice, m_GameLogic.m_cbControlDice[0], m_GameLogic.m_cbControlDice[1], m_GameLogic.m_cbControlDice[2]);

			return 0;

			m_GameLogic.m_bIsControlDice = true;
			net::dice_control_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			if (msg.table_cards_size() != DICE_COUNT)
			{
				m_GameLogic.m_bIsControlDice = false;
			}
			if (m_GameLogic.m_bIsControlDice)
			{
				for (uint8 i = 0; i<msg.table_cards_size(); ++i)
				{
					m_GameLogic.m_cbControlDice[i] = msg.table_cards(i);
					if (m_GameLogic.m_cbControlDice[i] <= 0 || m_GameLogic.m_cbControlDice[i] >= 7)
					{
						m_GameLogic.m_bIsControlDice = false;
					}
				}
			}

			LOG_DEBUG("2 playerControlDice - uid:%d,m_bIsControlDice:%d,dice:%d %d %d", 
				pPlayer->GetUID(), m_GameLogic.m_bIsControlDice, m_GameLogic.m_cbControlDice[0], m_GameLogic.m_cbControlDice[1], m_GameLogic.m_cbControlDice[2]);

		}break;
	case net::C2S_MSG_DICE_CONTINUOUS_PRESSURE_REQ://
	{
		net::msg_player_continuous_pressure_jetton_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnUserContinuousPressure(pPlayer, msg);
	}break;
    default:
        return 0;
    }
    return 0;
}

bool CGameDiceTable::DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[])
{
	m_GameLogic.m_bIsControlDice = true;
	for (uint8 i = 0; i<DICE_COUNT; ++i)
	{
		m_GameLogic.m_cbControlDice[i] = udice[i];
		if (m_GameLogic.m_cbControlDice[i] <= 0 || m_GameLogic.m_cbControlDice[i] >= 7)
		{
			m_GameLogic.m_bIsControlDice = false;
		}
	}

	LOG_DEBUG("m_bIsControlDice:%d,dice:%d %d %d",
		m_GameLogic.m_bIsControlDice, m_GameLogic.m_cbControlDice[0], m_GameLogic.m_cbControlDice[1], m_GameLogic.m_cbControlDice[2]);

	return true;
}

//用户断线或重连
bool CGameDiceTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
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
bool CGameDiceTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{
    uint32 uid = 0;
	if (pPlayer != NULL)
	{
		uid = pPlayer->GetUID();
	}
    LOG_DEBUG("uid:%d, wChairID:%d",uid,wChairID);
    SendSeatInfoToClient();
    return true;
}
//用户起立
bool CGameDiceTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{
    uint32 uid = 0;
	if (pPlayer != NULL)
	{
		uid = pPlayer->GetUID();
	}
    LOG_DEBUG("uid:%d, wChairID:%d",uid,wChairID);
    SendSeatInfoToClient();
    return true;
}

// 游戏开始
bool CGameDiceTable::OnGameStart()
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

    //LOG_DEBUG("game start - roomid:%d,tableid:%d,looksize:%d",m_pHostRoom->GetRoomID(),GetTableID(), m_mpLookers.size());
    /*
     // if(m_pCurBanker==NULL) 系统坐庄
    if(m_pCurBanker == NULL){
        //LOG_ERROR("the banker is null");
        CheckRobotApplyBanker();
        ChangeBanker(false);
        return false;
    }
     */
    m_uStartCount++;
	if (m_uStartCount == 99999)
	{
		m_uStartCount = 0;
	}
    m_coolLogic.beginCooling(s_PlaceJettonTime);
    
    net::msg_dice_start_rep gameStart;
    gameStart.set_time_leave(m_coolLogic.getCoolTick());
    gameStart.set_banker_score(m_lBankerScore);
    gameStart.set_banker_id(GetBankerUID());
    gameStart.set_banker_buyin_score(m_lBankerBuyinScore);
 
    SendMsgToAll(&gameStart,net::S2C_MSG_DICE_START);   
	OnTableGameStart();
    OnRobotStandUp();
    return true;
}

bool	CGameDiceTable::CalcuJackpotScore()
{
	WriteJackpotInfo(DICE_JACKPOT_INFO_POS_front);
	// 奖池增加
	tagDiceJackpotInfo	tagTttDiceJackpotInfo = m_tagDiceJackpotInfo;
	for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
	{
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		if (lPlayerWinScore <= 0)
		{
			continue;
		}
		double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
		int64 lPoolAddScore = (int64)fPoolAddScore;
		//m_mpUserWinScore[pPlayer->GetUID()] = lPlayerWinScore - lPoolAddScore;
		m_mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
		m_tagDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
	}
	auto it_looker = m_mpLookers.begin();
	for (; it_looker != m_mpLookers.end(); ++it_looker)
	{
		CGamePlayer* pPlayer = it_looker->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		if (lPlayerWinScore <= 0)
		{
			continue;
		}
		double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
		int64 lPoolAddScore = (int64)fPoolAddScore;
		//m_mpUserWinScore[pPlayer->GetUID()] = lPlayerWinScore - lPoolAddScore;
		m_mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
		m_tagDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
	}
	WriteJackpotInfo(DICE_JACKPOT_INFO_POS_add);
	// 用户中奖池

	uint32 uWinPoolScoreCountPoint = 0;
	uint32 uWinPoolScoreCountOneFive = 0;
	uint32 uWinPoolScoreCountSix = 0;
	tagDiceJackpotInfo	tagTempDiceJackpotInfo = m_tagDiceJackpotInfo;

	if (m_tagDiceJackpotInfo.lCurPollScore > 0)
	{
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			//是否中奖池
			auto it_player = m_mpUserAreaInfo.find(pPlayer->GetUID());
			if (it_player != m_mpUserAreaInfo.end())
			{
				auto & vec_dice_area_info = it_player->second;
				for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
				{
					int64 lWinPoolScore = 0;
					if (vec_dice_area_info[uInfoIndex].final_score > 0 && m_tagDiceJackpotInfo.lCurPollScore>0)
					{
						if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
						{
							uWinPoolScoreCountPoint++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
							//lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubPoint;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}
						}
						else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
						{
							uWinPoolScoreCountOneFive++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
							//lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubOneFive;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}

						}
						else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
						{
							uWinPoolScoreCountSix++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
							//lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubSixRatio;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}
						}
					}
					//if (lWinPoolScore > 0)
					//{
					//	ChangeScoreValueByUID(uid, lWinPoolScore, emACCTRAN_OPER_TYPE_DICE_WIN_JACKPOT_SCORE, m_pHostRoom->GetGameType());
					//	string title = "骰宝游戏获奖";
					//	string nickname = "系统提示";
					//	string content = CStringUtility::FormatToString("恭喜你在骰宝游戏获得%.2F筹码奖励，请注意查收。", (lWinPoolScore*0.01));
					//	CDBMysqlMgr::Instance().SendMail(0, uid, title, content, nickname);
					//}
				}
			}
		}
	}

	if (m_tagDiceJackpotInfo.lCurPollScore > 0)
	{
		auto it_looker = m_mpLookers.begin();
		for (; it_looker != m_mpLookers.end(); ++it_looker)
		{
			CGamePlayer *pPlayer = it_looker->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			//是否中奖池
			auto it_player = m_mpUserAreaInfo.find(pPlayer->GetUID());
			if (it_player != m_mpUserAreaInfo.end())
			{
				auto & vec_dice_area_info = it_player->second;
				for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
				{
					int64 lWinPoolScore = 0;
					if (vec_dice_area_info[uInfoIndex].final_score > 0 && m_tagDiceJackpotInfo.lCurPollScore>0)
					{
						if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
						{
							uWinPoolScoreCountPoint++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
							//int64 lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubPoint;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}

						}
						else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
						{
							uWinPoolScoreCountOneFive++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
							//int64 lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubOneFive;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}
						}
						else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
						{
							uWinPoolScoreCountSix++;
							tagUserJackpotInfo JackpotInfo;
							//JackpotInfo.pscore = m_tagDiceJackpotInfo.lCurPollScore;

							//double fWinPoolScore = ((double)m_tagDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
							//int64 lWinPoolScore = (int64)fWinPoolScore;
							//m_tagDiceJackpotInfo.UpdateCurPoolScore(-lWinPoolScore);
							//m_mpUserJackpotScore[pPlayer->GetUID()] += lWinPoolScore;

							JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
							JackpotInfo.ratio = tagTempDiceJackpotInfo.uPoolSubSixRatio;
							JackpotInfo.wscore = lWinPoolScore;
							auto it_UserJackpotInfo = m_mpUserJackpotInfo.find(pPlayer->GetUID());
							if (it_UserJackpotInfo != m_mpUserJackpotInfo.end())
							{
								vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
								vecUserJackpotInfo.push_back(JackpotInfo);
							}
							else
							{
								vector<tagUserJackpotInfo> vecUserJackpotInfo;
								vecUserJackpotInfo.push_back(JackpotInfo);
								m_mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
							}

						}
					}
					//if (lWinPoolScore > 0)
					//{
					//	ChangeScoreValueByUID(uid, lWinPoolScore, emACCTRAN_OPER_TYPE_DICE_WIN_JACKPOT_SCORE, m_pHostRoom->GetGameType());
					//	string title = "骰宝游戏获奖";
					//	string nickname = "系统提示";
					//	string content = CStringUtility::FormatToString("恭喜你在骰宝游戏获得%.2F筹码奖励，请注意查收。", (lWinPoolScore*0.01));
					//	CDBMysqlMgr::Instance().SendMail(0, uid, title, content, nickname);
					//}
				}
			}
		}
	}
	// 算人数平分奖池
	
	int64 lPoolScorePoint = 0;
	int64 lPoolScoreOneFive = 0;
	int64 lPoolScoreSix = 0;

	int64 lAverageScorePoint = 0;
	int64 lAverageScoreOneFive = 0;
	int64 lAverageScoreSix = 0;

	if (tagTempDiceJackpotInfo.lCurPollScore > 0)
	{
		double fPoolScorePoint = ((double)tagTempDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
		lPoolScorePoint = (int64)fPoolScorePoint;
		double fPoolScoreOneFive = ((double)tagTempDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
		lPoolScoreOneFive = (int64)fPoolScoreOneFive;
		double fPoolScoreSix = ((double)tagTempDiceJackpotInfo.lCurPollScore * (double)tagTempDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
		lPoolScoreSix = (int64)fPoolScoreSix;
	}
	
	if (lPoolScorePoint > 0 && uWinPoolScoreCountPoint > 0)
	{
		double dAverageScorePoint = (double)lPoolScorePoint / (double)uWinPoolScoreCountPoint;
		lAverageScorePoint = (int64)dAverageScorePoint;
	}
	if (lPoolScoreOneFive > 0 && uWinPoolScoreCountOneFive > 0)
	{
		double fAverageScoreOneFive = (double)lPoolScoreOneFive / (double)uWinPoolScoreCountOneFive;
		lAverageScoreOneFive = (int64)fAverageScoreOneFive;
	}
	if (lPoolScoreSix > 0 && uWinPoolScoreCountSix > 0)
	{
		double fAverageScoreSix = (double)lPoolScoreSix / (double)uWinPoolScoreCountSix;
		lAverageScoreSix = (int64)fAverageScoreSix;
	}

	LOG_DEBUG("1_jackpotscore - lCurPollScore:%lld,lPoolScore:%lld,%lld,%lld,lAverageScore:%lld,%lld,%lld,uWinPoolScore:%d,%d,%d",
		m_tagDiceJackpotInfo.lCurPollScore,lPoolScorePoint, lPoolScoreOneFive, lPoolScoreSix, lAverageScorePoint, lAverageScoreOneFive, lAverageScoreSix, uWinPoolScoreCountPoint, uWinPoolScoreCountOneFive, uWinPoolScoreCountSix);


	//用户中奖池加分
	auto it_begin_UserJackpotInfo = m_mpUserJackpotInfo.begin();
	for (; it_begin_UserJackpotInfo != m_mpUserJackpotInfo.end(); it_begin_UserJackpotInfo++)
	{
		vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_begin_UserJackpotInfo->second;
		for (uint32 i = 0; i < vecUserJackpotInfo.size(); i++)
		{
			if (vecUserJackpotInfo[i].area == CT_POINT_FOUR || vecUserJackpotInfo[i].area == CT_POINT_SEVEN_TEEN)
			{
				vecUserJackpotInfo[i].pscore = lPoolScorePoint;
				vecUserJackpotInfo[i].wscore = lAverageScorePoint;
				m_mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScorePoint;
				m_tagDiceJackpotInfo.UpdateCurPoolScore(-lAverageScorePoint);
			}
			else if (vecUserJackpotInfo[i].area >= CT_TRIPLE_ONE && vecUserJackpotInfo[i].area <= CT_TRIPLE_FIVE)
			{
				vecUserJackpotInfo[i].pscore = lPoolScoreOneFive;
				vecUserJackpotInfo[i].wscore = lAverageScoreOneFive;
				m_mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreOneFive;
				m_tagDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreOneFive);
			}
			else if (vecUserJackpotInfo[i].area == CT_TRIPLE_SIX)
			{
				vecUserJackpotInfo[i].pscore = lPoolScorePoint;
				vecUserJackpotInfo[i].wscore = lPoolScoreSix;
				m_mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreSix;
				m_tagDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreSix);
			}
			
		}
	}

	LOG_DEBUG("2_jackpotscore - lCurPollScore:%lld,lPoolScore:%lld,%lld,%lld,lAverageScore:%lld,%lld,%lld,uWinPoolScore:%d,%d,%d",
		m_tagDiceJackpotInfo.lCurPollScore, lPoolScorePoint, lPoolScoreOneFive, lPoolScoreSix, lAverageScorePoint, lAverageScoreOneFive, lAverageScoreSix, uWinPoolScoreCountPoint, uWinPoolScoreCountOneFive, uWinPoolScoreCountSix);



	WriteJackpotInfo(DICE_JACKPOT_INFO_POS_sub);

	int64 lPlayerWinTotalScore = 0;
	// 写用户分数 下注赢分数 和 中奖池分数
	for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
	{
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 uid = pPlayer->GetUID();
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPoolAddScore	= m_mpUserSubWinScore[pPlayer->GetUID()];
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		int64 lPlayerJackpotScore = m_mpUserJackpotScore[pPlayer->GetUID()];
		int64 lExWinScore = lPlayerJackpotScore - lPoolAddScore;
		int64 lValueFee = CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, m_mpWinScoreForFee[pPlayer->GetUID()], lExWinScore);
		int64 lWinScore = lPlayerWinScore + lValueFee - lPoolAddScore;
		m_mpUserWinScore[pPlayer->GetUID()] = lWinScore;

		LOG_DEBUG("uid:%d,lPoolAddScore:%lld,lPlayerWinScore:%lld,lPlayerJackpotScore:%lld,lExWinScore:%lld,lValueFee:%lld,lWinScore:%lld", uid, lPoolAddScore, lPlayerWinScore, lPlayerJackpotScore, lExWinScore, lValueFee, lWinScore);

		if (lWinScore>0)
		{
			lPlayerWinTotalScore += lWinScore;
		}

		tagDiceGameDataRank tagData;
		tagData.uid = pPlayer->GetUID();
		tagData.nick_name = pPlayer->GetNickName();
		tagData.city = pPlayer->GetCity();
		tagData.score = lWinScore;
		PushPlayerRank(tagData);

		if (lPlayerJackpotScore > 0)
		{
			string title = "骰宝游戏获奖";
			string nickname = "系统提示";
			string content = CStringUtility::FormatToString("恭喜你在骰宝游戏获得%.2F筹码奖励，请注意查收。", (lPlayerJackpotScore*0.01));
			CDBMysqlMgr::Instance().SendMail(0, uid, title, content, nickname);
		}
	}
	auto it_looker_calc = m_mpLookers.begin();
	for (; it_looker_calc != m_mpLookers.end(); ++it_looker_calc)
	{
		CGamePlayer* pPlayer = it_looker_calc->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 uid = pPlayer->GetUID();
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPoolAddScore = m_mpUserSubWinScore[pPlayer->GetUID()];
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		int64 lPlayerJackpotScore = m_mpUserJackpotScore[pPlayer->GetUID()];
		int64 lExWinScore = lPlayerJackpotScore - lPoolAddScore;
		int64 lValueFee = CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, m_mpWinScoreForFee[pPlayer->GetUID()], lExWinScore);
		int64 lWinScore = lPlayerWinScore + lValueFee - lPoolAddScore;
		m_mpUserWinScore[pPlayer->GetUID()] = lWinScore;


		LOG_DEBUG("uid:%d,lPoolAddScore:%lld,lPlayerWinScore:%lld,lPlayerJackpotScore:%lld,lExWinScore:%lld,lValueFee:%lld,lWinScore:%lld", uid, lPoolAddScore, lPlayerWinScore, lPlayerJackpotScore, lExWinScore, lValueFee, lWinScore);

		if (lWinScore>0)
		{
			lPlayerWinTotalScore += lWinScore;
		}

		tagDiceGameDataRank tagData;
		tagData.uid = pPlayer->GetUID();
		tagData.nick_name = pPlayer->GetNickName();
		tagData.city = pPlayer->GetCity();
		tagData.score = lWinScore;
		PushPlayerRank(tagData);

		if (lPlayerJackpotScore > 0)
		{
			string title = "骰宝游戏获奖";
			string nickname = "系统提示";
			string content = CStringUtility::FormatToString("恭喜你在骰宝游戏获得%.2F筹码奖励，请注意查收。", (lPlayerJackpotScore*0.01));
			CDBMysqlMgr::Instance().SendMail(0, uid, title, content, nickname);
		}
	}

	for (uint8 i = 0; i<DICE_COUNT; ++i)
	{
		m_tagDiceJackpotInfo.cbTableDice[i] = m_cbTableDice[i];
	}
	m_tagDiceJackpotInfo.utime = getSysTime();
	m_tagDiceJackpotInfo.lWinTotalScore = lPlayerWinTotalScore;

	return true;
}

bool CGameDiceTable::CalcuPlayerScore(int64 &playerAllWinScore)
{
	for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
	{
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 uid = pPlayer->GetUID();
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPoolAddScore = m_mpUserSubWinScore[pPlayer->GetUID()];
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		int64 lPlayerJackpotScore = m_mpUserJackpotScore[pPlayer->GetUID()];
		int64 lExWinScore = 0;// lPlayerJackpotScore - lPoolAddScore;
		int64 lValueFee = CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, m_mpWinScoreForFee[pPlayer->GetUID()], lExWinScore);
		int64 lWinScore = lPlayerWinScore + lValueFee;
		m_mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		// add by har
		if (!pPlayer->IsRobot())
			playerAllWinScore += lWinScore; // add by har end

		//LOG_DEBUG("uid:%d,lPoolAddScore:%lld,lPlayerWinScore:%lld,lPlayerJackpotScore:%lld,lExWinScore:%lld,lValueFee:%lld,lWinScore:%lld", uid, lPoolAddScore, lPlayerWinScore, lPlayerJackpotScore, lExWinScore, lValueFee, lWinScore);

		tagDiceGameDataRank tagData;
		tagData.uid = pPlayer->GetUID();
		tagData.nick_name = pPlayer->GetNickName();
		tagData.city = pPlayer->GetCity();
		tagData.score = lWinScore;
		PushPlayerRank(tagData);

	}
	auto it_looker_calc = m_mpLookers.begin();
	for (; it_looker_calc != m_mpLookers.end(); ++it_looker_calc)
	{
		CGamePlayer* pPlayer = it_looker_calc->second;
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 uid = pPlayer->GetUID();
		if (!IsSetJetton(pPlayer->GetUID()))
		{
			continue;
		}
		int64 lPoolAddScore = m_mpUserSubWinScore[pPlayer->GetUID()];
		int64 lPlayerWinScore = m_mpUserWinScore[pPlayer->GetUID()];
		int64 lPlayerJackpotScore = m_mpUserJackpotScore[pPlayer->GetUID()];
		int64 lExWinScore = 0;// lPlayerJackpotScore - lPoolAddScore;
		int64 lValueFee = CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, m_mpWinScoreForFee[pPlayer->GetUID()], lExWinScore);
		int64 lWinScore = lPlayerWinScore + lValueFee;
		m_mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		// add by har
		if (!pPlayer->IsRobot())
			playerAllWinScore += lWinScore; // add by har end

		//LOG_DEBUG("uid:%d,lPoolAddScore:%lld,lPlayerWinScore:%lld,lPlayerJackpotScore:%lld,lExWinScore:%lld,lValueFee:%lld,lWinScore:%lld", uid, lPoolAddScore, lPlayerWinScore, lPlayerJackpotScore, lExWinScore, lValueFee, lWinScore);

		tagDiceGameDataRank tagData;
		tagData.uid = pPlayer->GetUID();
		tagData.nick_name = pPlayer->GetNickName();
		tagData.city = pPlayer->GetCity();
		tagData.score = lWinScore;
		PushPlayerRank(tagData);

	}

	return true;
}


bool CGameDiceTable::PushPlayerRank(tagDiceGameDataRank tagData)
{
	bool bIsInRank = false;
	for (uint32 uIndex = 0; uIndex < m_vecIncomeScoreRank.size(); uIndex++)
	{
		if (m_vecIncomeScoreRank[uIndex].uid == tagData.uid)
		{
			if (m_vecIncomeScoreRank[uIndex].score < tagData.score)
			{
				m_vecIncomeScoreRank[uIndex].score = tagData.score;
			}
			bIsInRank = true;
			break;
		}
	}
	if (bIsInRank == false)
	{
		if (m_vecIncomeScoreRank.size() < GAME_DATA_DICE_WEEK_RANK_COUNT && tagData.score > 0)
		{
			m_vecIncomeScoreRank.push_back(tagData);
		}
		else if (m_vecIncomeScoreRank.size() > 0)
		{
			vector<tagDiceGameDataRank>::iterator iter_end = m_vecIncomeScoreRank.end();
			if (iter_end->score < tagData.score)
			{
				if (m_vecIncomeScoreRank.size() >= GAME_DATA_DICE_WEEK_RANK_COUNT)
				{
					m_vecIncomeScoreRank.erase(iter_end);
				}
				m_vecIncomeScoreRank.push_back(tagData);
			}
		}
		else
		{
			//
		}

	}

	SortIncomeScoreRank();

	return true;
}

//游戏结束
bool CGameDiceTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end:table:%d,m_wBankerTime:%d",GetTableID(),m_wBankerTime);
    switch(reason)
    {
    case GER_NORMAL:		//常规结束
        {
            InitBlingLog();
            WriteBankerInfo();

            AddPlayerToBlingLog();

            //if(m_uStartCount!=0 && m_uStartCount%10==0){
            //    for (int nAreaIndex=0;nAreaIndex<AREA_COUNT;nAreaIndex++) {
            //        double fmolecule=double((double)m_uArrAppear[nAreaIndex]/(double)m_uStartCount)*100;
            //        //uint32 umolecule = (m_uArrAppear[nAreaIndex]*10000)/m_uStartCount;
            //        LOG_DEBUG("m_uStartCount:%d,nAreaIndex:%d,AppearCount:%d,probability:%f/100",m_uStartCount,nAreaIndex,m_uArrAppear[nAreaIndex],fmolecule);
            //    }
            //}

			//计算分数
			int64 lBankerWinScore = CalculateScore(); 
			int64 playerAllWinScore = 0; // 玩家总赢分,骰宝玩家不能上庄 add by har
			//CalcuJackpotScore();
			CalcuPlayerScore(playerAllWinScore);
            WriteCardType();
			WriteUserJackpotInfo();

			int64 fee = 0;
			m_lBankerCurWinScore += lBankerWinScore;
            //m_lBankerWinScore += lBankerWinScore;
			//递增次数
			//m_wBankerTime++;

			//结束消息
            net::msg_dice_game_end msg;

            //net::msg_cards* pCards = msg.add_table_cards();

            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerCurWinScore);
            msg.set_banker_total_score(m_lBankerWinScore);

			net::dice_game_end_info* pend_info = msg.mutable_end_info();

			for (uint8 i = 0; i<DICE_COUNT; ++i)
			{
				pend_info->add_table_cards(m_cbTableDice[i]);
			}
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
			{
				int nMultiple;
				bool bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				if (bIsHit)
				{
					pend_info->add_hit_area(nAreaIndex);
				}
			}
			pend_info->set_big_small(m_record.cbBigSmall);
			pend_info->set_sum_point(m_record.cbSumPoints);


			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL || !IsSetJetton(pPlayer->GetUID()))
				{
					pend_info->add_player_score(0);
					pend_info->add_player_cur_score(0);
					continue;
				}
				pend_info->add_player_score(m_mpUserWinScore[pPlayer->GetUID()]);				
				pend_info->add_player_cur_score(GetPlayerCurScore(pPlayer));
			}
			
			for (uint32 uIndex=0; uIndex < AREA_COUNT; uIndex++)
			{
				if (m_ArrAllAreaInfo[uIndex].jetton_score == 0)
				{
					continue;
				}
				net::dice_area_info * pdice_area_info = pend_info->add_total_area_info();
				pdice_area_info->set_jetton_area(m_ArrAllAreaInfo[uIndex].jetton_area);
				pdice_area_info->set_jetton_multiple(m_ArrAllAreaInfo[uIndex].jetton_multiple);
				pdice_area_info->set_jetton_score(m_ArrAllAreaInfo[uIndex].jetton_score);
				pdice_area_info->set_final_score(m_ArrAllAreaInfo[uIndex].final_score);

				//LOG_DEBUG("game end - roomid:%d,table:%d,m_wBankerTime:%d,add_total_area_info:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_wBankerTime, pend_info->total_area_info_size());
			}

			//LOG_DEBUG("game end - roomid:%d,table:%d,m_wBankerTime:%d,add_total_area_info:%d", m_pHostRoom->GetRoomID(),GetTableID(), m_wBankerTime, pend_info->total_area_info_size());
			
			// 奖池信息信息
			net::dice_jackpot_info* pdice_jackpot_info = pend_info->mutable_jackpot_info();
			for (uint8 i = 0; i<DICE_COUNT; ++i)
			{
				pdice_jackpot_info->add_table_cards(m_tagDiceJackpotInfo.cbTableDice[i]);
			}
			pdice_jackpot_info->set_utime(m_tagDiceJackpotInfo.utime);
			pdice_jackpot_info->set_win_total_score(m_tagDiceJackpotInfo.lWinTotalScore);
			pdice_jackpot_info->set_cur_jackpot_score(m_tagDiceJackpotInfo.lCurPollScore);

			for (uint32 uIndex = 0; uIndex < m_vecIncomeScoreRank.size(); uIndex++)
			{
				net::dice_game_score_rank * pdice_game_score_rank = pdice_jackpot_info->add_score_rank();
				pdice_game_score_rank->set_uid(m_vecIncomeScoreRank[uIndex].uid);
				pdice_game_score_rank->set_nick_name(m_vecIncomeScoreRank[uIndex].nick_name);
				pdice_game_score_rank->set_income_score(m_vecIncomeScoreRank[uIndex].score);
			}

			//发送座位积分
			for( WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex )
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					continue;
				}
				//扣除分数
				pend_info->set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
				pend_info->set_self_cur_score(GetPlayerCurScore(pPlayer));
				pend_info->set_jackpot_score(m_mpUserJackpotScore[pPlayer->GetUID()]);
				pend_info->set_total_score(m_mpUserJackpotScore[pPlayer->GetUID()] + m_mpUserWinScore[pPlayer->GetUID()]);
				
				// 自己下注区域信息
				pend_info->clear_self_area_info();
				auto it_player = m_mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != m_mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						net::dice_area_info * pdice_area_info = pend_info->add_self_area_info();
						pdice_area_info->set_jetton_area(vec_dice_area_info[uInfoIndex].jetton_area);
						pdice_area_info->set_jetton_multiple(vec_dice_area_info[uInfoIndex].jetton_multiple);
						pdice_area_info->set_jetton_score(vec_dice_area_info[uInfoIndex].jetton_score);
						pdice_area_info->set_final_score(vec_dice_area_info[uInfoIndex].final_score);
					}
				}

				//LOG_DEBUG("game end - roomid:%d,table:%d,m_wBankerTime:%d,add_self_area_info:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_wBankerTime, pend_info->self_area_info_size());

                pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_GAME_END);
			}

            //发送旁观者积分
            map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
            for(;it != m_mpLookers.end();++it)
            {
                CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)
				{
					continue;
				}
				//扣除分数
				pend_info->set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
				pend_info->set_self_cur_score(GetPlayerCurScore(pPlayer));
				pend_info->set_jackpot_score(m_mpUserJackpotScore[pPlayer->GetUID()]);
				pend_info->set_total_score(m_mpUserJackpotScore[pPlayer->GetUID()] + m_mpUserWinScore[pPlayer->GetUID()]);
				// 自己下注区域信息
				pend_info->clear_self_area_info();
				auto it_player = m_mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != m_mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						net::dice_area_info * pdice_area_info = pend_info->add_self_area_info();
						pdice_area_info->set_jetton_area(vec_dice_area_info[uInfoIndex].jetton_area);
						pdice_area_info->set_jetton_multiple(vec_dice_area_info[uInfoIndex].jetton_multiple);
						pdice_area_info->set_jetton_score(vec_dice_area_info[uInfoIndex].jetton_score);
						pdice_area_info->set_final_score(vec_dice_area_info[uInfoIndex].final_score);
					}
				}

				//LOG_DEBUG("game end - roomid:%d,table:%d,m_wBankerTime:%d,add_self_area_info:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_wBankerTime, pend_info->self_area_info_size());

                pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_GAME_END);
            }
			//m_mpUserWinScore[GetBankerUID()] = 0;
   //         for(uint8 i=0;i<AREA_COUNT;++i)
			//{
   //             m_userJettonScore[i].clear();
   //         }

			//更新活跃福利数据            
			int64 curr_win = m_mpUserWinScore[m_aw_ctrl_uid];
			UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);

            SaveBlingLog();			
            CheckRobotCancelBanker();
			LOG_DEBUG("OnGameEnd2 - roomid:%d,tableid:%d,playerAllWinScore:%d,lBankerWinScore:%d", GetRoomID(), GetTableID(), playerAllWinScore, lBankerWinScore);
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

int64 CGameDiceTable::CalculateScore()
{
    map<uint32,int64> mpUserLostScore;
	mpUserLostScore.clear();
    m_mpUserWinScore.clear();
	m_mpWinScoreForFee.clear();

	int nMultiple = 0;
	bool bIsHit = false;
    int64 lBankerWinScore = 0;

	m_GameLogic.ComputeDiceResult();

	int nSumPoint = m_GameLogic. m_nSumPoint;
	
	m_record.cbBigSmall = m_GameLogic.m_enBigSmall;
	m_record.cbSumPoints = m_GameLogic.m_nSumPoint;
	m_record.cbIsWaidice = m_GameLogic.IsWaidice();
	memcpy(m_record.cbDiceRecord,m_cbTableDice,sizeof(m_record.cbDiceRecord));

	m_vecRecord.push_back(m_record);
    if(m_vecRecord.size() > 40)
	{
        m_vecRecord.erase(m_vecRecord.begin());
    }

	//m_vecGamePlayRecord.push_back(m_record);
	//if (m_vecGamePlayRecord.size() >= 40) {
	//	m_vecGamePlayRecord.erase(m_vecGamePlayRecord.begin());
	//}
	SendGameEndLogInfo();

	// 椅子用户
	for (int nChairID=0; nChairID<GAME_PLAYER; nChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(nChairID);
		if (pPlayer == NULL)
		{
			continue;
		}
		uint32 dwUserID = pPlayer->GetUID();

		for (int nAreaIndex=0;nAreaIndex<AREA_COUNT;nAreaIndex++)
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

			bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex,nMultiple);
			int64 lTempWinScore = 0;
			if (bIsHit)
			{
                //lTempWinScore = (lUserJettonScore * m_GameLogic.m_nCompensateRatio[nAreaIndex]);
				lTempWinScore = (lUserJettonScore * nMultiple);
				m_mpUserWinScore[dwUserID] += lTempWinScore;
                lBankerWinScore -= lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
                mpUserLostScore[dwUserID] -= lUserJettonScore;
                lBankerWinScore += lUserJettonScore;
			}

			tag_dice_area_info dice_area_info;
			dice_area_info.jetton_area = nAreaIndex;
			dice_area_info.jetton_multiple = nMultiple;
			dice_area_info.jetton_score = lUserJettonScore;
			dice_area_info.final_score = lTempWinScore;


			m_ArrAllAreaInfo[nAreaIndex].jetton_area +=	dice_area_info.jetton_area;
			m_ArrAllAreaInfo[nAreaIndex].jetton_multiple += dice_area_info.jetton_multiple;
			m_ArrAllAreaInfo[nAreaIndex].jetton_score += dice_area_info.jetton_score;
			m_ArrAllAreaInfo[nAreaIndex].final_score += (-dice_area_info.final_score);


			auto it_player = m_mpUserAreaInfo.find(dwUserID);
			if (it_player != m_mpUserAreaInfo.end())
			{
				it_player->second.push_back(dice_area_info);
			}
			else
			{
				vector<tag_dice_area_info> vec_dice_area_info;
				vec_dice_area_info.push_back(dice_area_info);
				m_mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
			}

			//LOG_DEBUG("score result chair - nChairID:%d,uid:%d,nAreaIndex:%d,lUserJettonScore:%d,bIsHit:%d,lTempWinScore:%d,m_mpUserWinScore:%d,mpUserLostScore:%d,nMultiple:%d,lBankerWinScore:%d,m_mpUserAreaInfo.size:%d,",
			//	nChairID, dwUserID, nAreaIndex, lUserJettonScore,bIsHit, lTempWinScore, m_mpUserWinScore[dwUserID], mpUserLostScore[dwUserID], nMultiple, lBankerWinScore, m_mpUserAreaInfo.size());
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

		for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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

			bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
			int64 lTempWinScore = 0;
			if (bIsHit)
			{
				//lTempWinScore = (lUserJettonScore * m_GameLogic.m_nCompensateRatio[nAreaIndex]);
				lTempWinScore = (lUserJettonScore * nMultiple);
				m_mpUserWinScore[dwUserID] += lTempWinScore;
				lBankerWinScore -= lTempWinScore;
			}
			else
			{
				lTempWinScore = -lUserJettonScore;
				mpUserLostScore[dwUserID] -= lUserJettonScore;
				lBankerWinScore += lUserJettonScore;
			}

			tag_dice_area_info dice_area_info;
			dice_area_info.jetton_area = nAreaIndex;
			dice_area_info.jetton_multiple = nMultiple;
			dice_area_info.jetton_score = lUserJettonScore;
			dice_area_info.final_score = lTempWinScore;
			
			m_ArrAllAreaInfo[nAreaIndex].jetton_area += dice_area_info.jetton_area;
			m_ArrAllAreaInfo[nAreaIndex].jetton_multiple += dice_area_info.jetton_multiple;
			m_ArrAllAreaInfo[nAreaIndex].jetton_score += dice_area_info.jetton_score;
			m_ArrAllAreaInfo[nAreaIndex].final_score += (-dice_area_info.final_score);
			
			auto it_player = m_mpUserAreaInfo.find(dwUserID);
			if (it_player != m_mpUserAreaInfo.end())
			{
				it_player->second.push_back(dice_area_info);
			}
			else
			{
				vector<tag_dice_area_info> vec_dice_area_info;
				vec_dice_area_info.push_back(dice_area_info);
				m_mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
			}

			//LOG_DEBUG("score result looker - uid:%d,nAreaIndex:%d,lUserJettonScore:%d,bIsHit:%d,lTempWinScore:%d,m_mpUserWinScore:%d,mpUserLostScore:%d,nMultiple:%d,lBankerWinScore:%d,m_mpUserAreaInfo.size:%d,jetton_score:%lld,final_score:%lld",
			//	dwUserID, nAreaIndex, lUserJettonScore, bIsHit, lTempWinScore, m_mpUserWinScore[dwUserID], mpUserLostScore[dwUserID], nMultiple, lBankerWinScore, m_mpUserAreaInfo.size(), m_ArrAllAreaInfo[nAreaIndex].jetton_score, m_ArrAllAreaInfo[nAreaIndex].final_score);
		}
		m_mpWinScoreForFee[dwUserID] = m_mpUserWinScore[dwUserID];
		m_mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];

		LOG_DEBUG("score result looker - uid:%d,m_mpUserWinScore:%lld m_mpWinScoreForFee:%lld", dwUserID, m_mpUserWinScore[dwUserID], m_mpWinScoreForFee[dwUserID]);

	}
    return lBankerWinScore;
}
int64    CGameDiceTable::CalcPlayerInfo(uint32 uid,int64 winScore,int64 OnlywinScore,int64 lPlayerJackpotScore,bool isBanker)
{
	//if (uid == 0 || winScore == 0)
	//{
	//	LOG_DEBUG("report to lobby error - uid:%d,winScore:%lld", uid, winScore);
	//	return 0;
	//}
    //LOG_DEBUG("report to lobby - uid:%d,winScore:%lld",uid,winScore);
	int64 fee = GetBrcFee(uid, OnlywinScore, isBanker ? false : true);
	winScore += fee;
	CalcPlayerGameInfoForBrc(uid, winScore, lPlayerJackpotScore, isBanker ? false : true);
	
	LOG_DEBUG("report to lobby - uid:%d,winScore:%lld OnlywinScore:%lld lPlayerJackpotScore:%lld fee:%lld", uid, winScore, OnlywinScore, lPlayerJackpotScore, fee);
   
	return fee;

}
//玩家进入或离开
void  CGameDiceTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer != NULL)
	{
		uid = pPlayer->GetUID();
	}
    LOG_DEBUG("player join - uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld", uid,chairID,isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin){
        SendApplyUser(pPlayer);
        SendGameScene(pPlayer);
        SendPlayLog(pPlayer);
    }else{
        //OnUserCancelBanker(pPlayer);        
        //RemoveApplyBanker(uid);
        for(uint8 i=0;i<AREA_COUNT;++i){
            m_userJettonScore[i].erase(uid);
        }
    }        
}
// 发送场景信息(断线重连)
void    CGameDiceTable::SendGameScene(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL)
	{
		return;
	}
    LOG_DEBUG("send game scene - uid:%d", pGamePlayer->GetUID());
    switch(m_gameState)
    {
    case net::TABLE_STATE_DICE_FREE:          // 空闲状态
        {
            net::msg_dice_game_info_free_rep msg;
            msg.set_time_leave(m_coolLogic.getCoolTick());
			msg.set_game_status(m_gameState);
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerCurWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
           
			pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_GAME_FREE_INFO);
        }break;
    case net::TABLE_STATE_DICE_PLACE_JETTON:  // 游戏状态
    case net::TABLE_STATE_DICE_GAME_END:      // 结束状态
        {
            net::msg_dice_game_info_play_rep msg;
			// 庄家信息
			msg.set_banker_id(GetBankerUID());
			msg.set_banker_time(m_wBankerTime);
			msg.set_banker_win_score(m_lBankerCurWinScore);
			msg.set_banker_score(m_lBankerScore);
			msg.set_banker_buyin_score(m_lBankerBuyinScore);

			// 游戏信息
			msg.set_time_leave(m_coolLogic.getCoolTick());
			msg.set_game_status(m_gameState);

			// 游戏状态
			//if (m_gameState == net::TABLE_STATE_DICE_PLACE_JETTON)
			{
				for (uint8 i = 0; i<AREA_COUNT; ++i)
				{
					msg.add_all_jetton_score(m_allJettonScore[i]);
				}

				for (uint8 i = 0; i<AREA_COUNT; ++i)
				{
					auto it_palyer = m_userJettonScore[i].find(pGamePlayer->GetUID());
					if (it_palyer != m_userJettonScore[i].end())
					{
						msg.add_self_jetton_score(it_palyer->second);
					}
					else
					{
						msg.add_self_jetton_score(0);
					}
				}
			}

			// 结束状态
			//if (m_gameState == net::TABLE_STATE_DICE_GAME_END)
			{
				net::dice_game_end_info* pend_info = msg.mutable_end_info();

				for (uint8 i = 0; i<DICE_COUNT; ++i)
				{
					pend_info->add_table_cards(m_cbTableDice[i]);
				}
				pend_info->set_big_small(m_record.cbBigSmall);
				pend_info->set_sum_point(m_record.cbSumPoints);

				int64 lGamePlayerWinScore = 0;
				int64 lGamePlayerJackpotScore = 0;

				auto it_pGamePlayerWinScore = m_mpUserWinScore.find(pGamePlayer->GetUID());
				if (it_pGamePlayerWinScore != m_mpUserWinScore.end())
				{
					lGamePlayerWinScore = it_pGamePlayerWinScore->second;
				}
				auto it_pGamePlayerJackpotScore = m_mpUserJackpotScore.find(pGamePlayer->GetUID());
				if (it_pGamePlayerJackpotScore != m_mpUserJackpotScore.end())
				{
					lGamePlayerJackpotScore = it_pGamePlayerJackpotScore->second;
				}
				pend_info->set_user_score(lGamePlayerWinScore);
				pend_info->set_self_cur_score(GetPlayerCurScore(pGamePlayer));
				pend_info->set_jackpot_score(lGamePlayerJackpotScore);
				pend_info->set_total_score(lGamePlayerWinScore + lGamePlayerJackpotScore);

				for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
				{
					CGamePlayer *pPlayer = GetPlayer(wUserIndex);
					if (pPlayer == NULL || !IsSetJetton(pPlayer->GetUID()))
					{
						pend_info->add_player_score(0);
						pend_info->add_player_cur_score(0);
						continue;
					}
					lGamePlayerWinScore = 0;
					lGamePlayerJackpotScore = 0;

					auto it_pGamePlayerWinScore = m_mpUserWinScore.find(pPlayer->GetUID());
					if (it_pGamePlayerWinScore != m_mpUserWinScore.end())
					{
						lGamePlayerWinScore = it_pGamePlayerWinScore->second;
					}
					auto it_pGamePlayerJackpotScore = m_mpUserJackpotScore.find(pPlayer->GetUID());
					if (it_pGamePlayerJackpotScore != m_mpUserJackpotScore.end())
					{
						lGamePlayerJackpotScore = it_pGamePlayerJackpotScore->second;
					}

					pend_info->add_player_score(lGamePlayerWinScore);
					pend_info->add_player_cur_score(GetPlayerCurScore(pPlayer));
				}
				
				pend_info->set_self_cur_score(GetPlayerCurScore(pGamePlayer));

				// 自己下注区域信息
				pend_info->clear_self_area_info();
				auto it_player = m_mpUserAreaInfo.find(pGamePlayer->GetUID());
				if (it_player != m_mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						net::dice_area_info * pdice_area_info = pend_info->add_self_area_info();
						pdice_area_info->set_jetton_area(vec_dice_area_info[uInfoIndex].jetton_area);
						pdice_area_info->set_jetton_multiple(vec_dice_area_info[uInfoIndex].jetton_multiple);
						pdice_area_info->set_jetton_score(vec_dice_area_info[uInfoIndex].jetton_score);
						pdice_area_info->set_final_score(vec_dice_area_info[uInfoIndex].final_score);
					}
				}

				for (uint32 uIndex = 0; uIndex < AREA_COUNT; uIndex++)
				{
					if (m_ArrAllAreaInfo[uIndex].jetton_score == 0)
					{
						continue;
					}
					net::dice_area_info * pdice_area_info = pend_info->add_total_area_info();
					pdice_area_info->set_jetton_area(m_ArrAllAreaInfo[uIndex].jetton_area);
					pdice_area_info->set_jetton_multiple(m_ArrAllAreaInfo[uIndex].jetton_multiple);
					pdice_area_info->set_jetton_score(m_ArrAllAreaInfo[uIndex].jetton_score);
					pdice_area_info->set_final_score(m_ArrAllAreaInfo[uIndex].final_score);
				}

				// 奖池信息信息
				net::dice_jackpot_info* pdice_jackpot_info = pend_info->mutable_jackpot_info();
				for (uint8 i = 0; i<DICE_COUNT; ++i)
				{
					pdice_jackpot_info->add_table_cards(m_tagDiceJackpotInfo.cbTableDice[i]);
				}
				pdice_jackpot_info->set_utime(m_tagDiceJackpotInfo.utime);
				pdice_jackpot_info->set_win_total_score(m_tagDiceJackpotInfo.lWinTotalScore);
				pdice_jackpot_info->set_cur_jackpot_score(m_tagDiceJackpotInfo.lCurPollScore);

				for (uint32 uIndex = 0; uIndex < m_vecIncomeScoreRank.size(); uIndex++)
				{
					net::dice_game_score_rank * pdice_game_score_rank = pdice_jackpot_info->add_score_rank();
					pdice_game_score_rank->set_uid(m_vecIncomeScoreRank[uIndex].uid);
					pdice_game_score_rank->set_nick_name(m_vecIncomeScoreRank[uIndex].nick_name);
					pdice_game_score_rank->set_income_score(m_vecIncomeScoreRank[uIndex].score);
				}
			}

			pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_GAME_PLAY_INFO);


        }break;
    default:
        break;                    
    }
    SendLookerListToClient(pGamePlayer);
    SendApplyUser(pGamePlayer);
	SendFrontJettonInfo(pGamePlayer);
}

// 重置游戏数据
void    CGameDiceTable::ResetGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));
	memset(m_robotJettonScore, 0, sizeof(m_robotJettonScore));
    memset(m_winMultiple,0,sizeof(m_winMultiple));

	m_curr_bet_user.clear();

	//个人下注
	for(uint8 i=0;i<AREA_COUNT;++i)
	{
	    m_userJettonScore[i].clear();
	}
	//玩家成绩	
	m_mpUserWinScore.clear();
	m_mpUserSubWinScore.clear();
	m_mpUserJackpotScore.clear();
	m_mpUserJackpotInfo.clear();
	m_mpUserAreaInfo.clear();
	//扑克信息
    memset(m_cbTableDice,0,sizeof(m_cbTableDice));
	//庄家信息
	m_pCurBanker            = NULL;

	m_robotBankerWinPro = 500;

    m_bankerAutoAddScore    = 0;                //自动补币
    m_needLeaveBanker       = false;            //离开庄位

	m_wBankerTime = 0;							//做庄次数
    m_wBankerWinTime = 0;                       //胜利次数
	m_lBankerScore = 0;							//庄家积分
	m_lBankerCurWinScore = 0;
	m_lBankerWinScore = 0;						//累计成绩

    m_lBankerBuyinScore = 0;                    //庄家带入
    m_lBankerInitBuyinScore = 0;                //庄家初始带入
    m_lBankerWinMaxScore = 0;                   //庄家最大输赢
    m_lBankerWinMinScore = 0;                   //庄家最惨输赢
	m_bIsRobotAlreadyJetton = false;
	
	m_uStartCount = 0;
	memset(m_uArrAppear, 0, sizeof(m_uArrAppear));
	m_vecIncomeScoreRank.clear();
	m_robotJettonArea.clear();
	m_GameLogic.ResetGameData();
}
void    CGameDiceTable::ClearTurnGameData()
{
	//总下注数
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));
	memset(m_robotJettonScore, 0, sizeof(m_robotJettonScore));

	m_curr_bet_user.clear();

	//个人下注
	for(uint8 i=0;i<AREA_COUNT;++i){
	    m_userJettonScore[i].clear();
		m_ArrAllAreaInfo[i].Init();
	}
	//玩家成绩
	m_mpUserWinScore.clear();
	m_mpUserSubWinScore.clear();
	m_mpUserJackpotScore.clear();
	m_mpUserJackpotInfo.clear();
	m_mpUserAreaInfo.clear();

	m_record.Init();
	//扑克信息
	memset(m_cbTableDice, 0, sizeof(m_cbTableDice));
	memset(m_uArrAppear, 0, sizeof(m_uArrAppear));
	m_vecIncomeScoreRank.clear();
	m_lBankerCurWinScore = 0;
	m_bIsRobotAlreadyJetton = false;
	m_robotJettonArea.clear();
	m_GameLogic.ResetGameData();
	m_lRemainAreaLastRobotJettonTime = getSysTime();
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameDiceTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		map<uint32, int64>::iterator it_palyer = m_userJettonScore[wAreaIndex].find(pPlayer->GetUID());
		if (it_palyer == m_userJettonScore[wAreaIndex].end())
			continue;
		if (it_palyer->second == 0)
			continue;
		if (!pPlayer->IsRobot())
			isAllRobot = false;
		isAllPlayer = false;
		return;
	}
}

// 写入出牌log
void    CGameDiceTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip)
{
    uint8 cardType = 0;//m_GameLogic.GetCardType(cardData,cardCount);
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
void    CGameDiceTable::WriteAddScoreLog(uint32 uid,uint8 area,int64 score)
{
	if (score == 0)
	{
		return;
	}
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["p"]    = area;
    logValue["s"]    = score;

    m_operLog["op"].append(logValue);
}
// 写入牌型
void 	CGameDiceTable::WriteCardType()
{
    Json::Value logValue;
    string strDescribe = CStringUtility::FormatToString("%d,%d,%d",m_cbTableDice[0],m_cbTableDice[1],m_cbTableDice[2]);
    int nMultiple=0;
    bool bIsHit = false;
    string strAppear;
    LOG_DEBUG("m_nSumPoint:%d,m_enBigSmall:%d. m_nDice:%d,%d,%d. m_nCountNum:%d,%d,%d,%d,%d,%d. m_enNumber:%d,%d,%d,%d,%d,%d.",m_GameLogic.m_nSumPoint,m_GameLogic.m_enBigSmall,m_GameLogic.m_nDice[0],m_GameLogic.m_nDice[1],m_GameLogic.m_nDice[2],m_GameLogic.m_nCountNum[0],m_GameLogic.m_nCountNum[1],m_GameLogic.m_nCountNum[2],m_GameLogic.m_nCountNum[3],m_GameLogic.m_nCountNum[4],m_GameLogic.m_nCountNum[5],m_GameLogic.m_enNumber_1,m_GameLogic.m_enNumber_2,m_GameLogic.m_enNumber_3,m_GameLogic.m_enNumber_4,m_GameLogic.m_enNumber_5,m_GameLogic.m_enNumber_6);

    for (int nAreaIndex=0;nAreaIndex<AREA_COUNT;nAreaIndex++)
	{
        bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex,nMultiple);
        //LOG_DEBUG("nAreaIndex:%d,bIsHit:%d",nAreaIndex,bIsHit);
        if(bIsHit)
		{
            m_uArrAppear[nAreaIndex]++;
            string strTemp = CStringUtility::FormatToString("%d,", nAreaIndex);
            strAppear += strTemp;
        }
    }

    logValue["dice"]  = strDescribe.c_str();
    logValue["type"]   = strAppear.c_str();
    m_operLog["diceinfo"].append(logValue);


	//下注信息
	Json::Value logAreaInfo;
	auto it_player = m_mpUserAreaInfo.begin();
	for (; it_player != m_mpUserAreaInfo.end(); it_player++)
	{
		string strPlayerUid = CStringUtility::FormatToString("%d", it_player->first);
		vector<tag_dice_area_info> & vec_dice_area_info = it_player->second;
		for (uint32 uIndex = 0; uIndex < vec_dice_area_info.size(); uIndex++)
		{
			tag_dice_area_info & dice_area_info = vec_dice_area_info[uIndex];
			Json::Value logPlayerAreaInfo;
			logPlayerAreaInfo["area"] = dice_area_info.jetton_area;
			logPlayerAreaInfo["mult"] = dice_area_info.jetton_multiple;
			logPlayerAreaInfo["jscore"] = dice_area_info.jetton_score;
			logPlayerAreaInfo["fscore"] = dice_area_info.final_score;

			logAreaInfo[strPlayerUid.c_str()].append(logPlayerAreaInfo);
		}
	}
	m_operLog["areainfo"].append(logAreaInfo);

}

void    CGameDiceTable::WriteUserJackpotInfo()
{
	if (m_mpUserJackpotInfo.size() == 0)
	{
		return;
	}
	Json::Value logJackpotInfo;
	auto it_UserJackpotInfo = m_mpUserJackpotInfo.begin();
	for (; it_UserJackpotInfo != m_mpUserJackpotInfo.end(); it_UserJackpotInfo++)
	{
		string strPlayerUid = CStringUtility::FormatToString("%d", it_UserJackpotInfo->first);
		vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
		for (uint32 i = 0; i < vecUserJackpotInfo.size(); i++)
		{
			Json::Value logPlayerJackpotInfo;
			logPlayerJackpotInfo["area"] = vecUserJackpotInfo[i].area;
			logPlayerJackpotInfo["ratio"] = vecUserJackpotInfo[i].ratio;
			logPlayerJackpotInfo["pscore"] = vecUserJackpotInfo[i].pscore;
			logPlayerJackpotInfo["wscore"] = vecUserJackpotInfo[i].wscore;
			logPlayerJackpotInfo["tscore"] = m_mpUserJackpotScore[it_UserJackpotInfo->first];
			logJackpotInfo[strPlayerUid.c_str()].append(logPlayerJackpotInfo);
		}
	}
	m_operLog["jackpotinfo"].append(logJackpotInfo);
}

// 写入庄家信息
void    CGameDiceTable::WriteBankerInfo()
{
    m_operLog["banker"] = GetBankerUID();
}

void    CGameDiceTable::WriteJackpotInfo(uint16 pos)
{
	tagDiceJackpotInfo	DiceJackpotInfo = m_tagDiceJackpotInfo;
	string strPos;
	if (pos == DICE_JACKPOT_INFO_POS_front)
	{
		strPos = "jif";
	}
	else if (pos == DICE_JACKPOT_INFO_POS_add)
	{
		strPos = "jia";
	}
	else if (pos == DICE_JACKPOT_INFO_POS_sub)
	{
		strPos = "jis";
	}
	else
	{
		strPos = "jie";
	}

	m_operLog[strPos.c_str()] = DiceJackpotInfo.lCurPollScore;
}

//加注事件
bool    CGameDiceTable:: OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
    //LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
    //效验参数
    if(cbJettonArea==CT_ERROR ||
       cbJettonArea==CT_POINT_THREE ||
       cbJettonArea==CT_POINT_EIGHT_TEEN ||
       cbJettonArea==CT_LIMIT_CICLE_DICE ||
       lJettonScore <= 0     )
	{
        LOG_DEBUG("jetton is error,uid:%d,bIsRobot:%d,cbJettonArea:%d,lJettonScore:%lld", pPlayer->GetUID(), pPlayer->IsRobot(), cbJettonArea,lJettonScore);
        return false;
    }
	if (pPlayer != NULL && pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("jetton is start,uid:%d,bIsRobot:%d,cbJettonArea:%d,lJettonScore:%lld", pPlayer->GetUID(), pPlayer->IsRobot(), cbJettonArea, lJettonScore);

	}

	net::msg_dice_place_jetton_rep msg;
	msg.set_jetton_area(cbJettonArea);
	msg.set_jetton_score(lJettonScore);


	if(GetGameState() != net::TABLE_STATE_DICE_PLACE_JETTON)
	{
        msg.set_result(net::RESULT_CODE_FAIL);        
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_PLACE_JETTON_REP);        
		return false;
	}
	//庄家判断
	if(pPlayer->GetUID() == GetBankerUID())
	{
        LOG_DEBUG("the banker can't jetton - uid:%d", pPlayer->GetUID());
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_PLACE_JETTON_REP);

		return false;
	}

	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	if (TableJettonLimmit(pPlayer, lJettonScore, lJettonCount) == false)
	{
		//bPlaceJettonSuccess = false;
		LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());

		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_PLACE_JETTON_REP);

		return false;
	}

	//成功标识
	bool bPlaceJettonSuccess=true;
	//合法验证
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, cbJettonArea,lJettonScore);
	if(lUserMaxJetton >= lJettonScore)
	{
		//保存下注
		m_allJettonScore[cbJettonArea] += lJettonScore;
        if(!pPlayer->IsRobot())
		{
            m_playerJettonScore[cbJettonArea] += lJettonScore;
			m_curr_bet_user.insert(pPlayer->GetUID());
        }
		if (pPlayer->IsRobot())
		{
			m_robotJettonScore[cbJettonArea] += lJettonScore;
		}
		m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;		
	}
	else
	{
	    LOG_DEBUG("the jetton more than limit - uid:%d,IsRobot:%d,lUserMaxJetton:%lld,lJettonScore:%lld", pPlayer->GetUID(), pPlayer->IsRobot(), lUserMaxJetton, lJettonScore);

		bPlaceJettonSuccess = false;
	}


	if(bPlaceJettonSuccess)
	{
		RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);

		OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);
		//发送消息
        msg.set_result(net::RESULT_CODE_SUCCESS);        
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_PLACE_JETTON_REP);

        net::msg_dice_place_jetton_broadcast broad;
        broad.set_uid(pPlayer->GetUID());
        broad.set_jetton_area(cbJettonArea);
        broad.set_jetton_score(lJettonScore);
        broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);
        
        SendMsgToAll(&broad,net::S2C_MSG_DICE_PLACE_JETTON_BROADCAST);

		LOG_DEBUG("jetton is success,uid:%d,bIsRobot:%d,cbJettonArea:%d,lJettonScore:%lld", pPlayer->GetUID(), pPlayer->IsRobot(), cbJettonArea, lJettonScore);
		return true;
	}
	else
	{
        msg.set_result(net::RESULT_CODE_FAIL);
		//发送消息
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_PLACE_JETTON_REP);
		return false;
	}
    return false;
}

bool    CGameDiceTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	//LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_DICE_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//效验参数
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		BYTE cbJettonArea = info.area();
		int64 lJettonScore = info.score();
		if (cbJettonArea == CT_ERROR ||
			cbJettonArea == CT_POINT_THREE ||
			cbJettonArea == CT_POINT_EIGHT_TEEN ||
			cbJettonArea == CT_LIMIT_CICLE_DICE ||
			lJettonScore <= 0)
		{
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);
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
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("the jetton more than you have - uid:%d", pPlayer->GetUID());

		return true;
	}
	//成功标识
	//bool bPlaceJettonSuccess = true;

	//for (int i = 0; i < msg.info_size(); i++)
	//{
	//	net::bairen_jetton_info info = msg.info(i);

	//	BYTE cbJettonArea = info.area();
	//	int64 lJettonScore = info.score();
	//	if (cbJettonArea == CT_ERROR ||
	//		cbJettonArea == CT_POINT_THREE ||
	//		cbJettonArea == CT_POINT_EIGHT_TEEN ||
	//		cbJettonArea == CT_LIMIT_CICLE_DICE ||
	//		lJettonScore <= 0)
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//	
	//	if (GetUserMaxJetton(pPlayer, info.area(), info.score()) < info.score())
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//}

	int64 lUserMaxHettonScore = GetUserMaxJetton(pPlayer, 0,0);
	if (lUserMaxHettonScore < lTotailScore)
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserMaxHettonScore:%lld,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserMaxHettonScore, lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}

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

		net::msg_dice_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_PLACE_JETTON_REP);

		net::msg_dice_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_DICE_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_DICE_CONTINUOUS_PRESSURE_REP);

	return true;
}


//用户取消下注
bool    CGameDiceTable::OnUserCancelJetton(CGamePlayer* pPlayer)
{
	bool bIsHavePlaceJetton = false;
	uint32 uid = pPlayer->GetUID();


	int64 playerJettonScore[AREA_COUNT] = { 0 };
	for (uint8 i = 0; i<AREA_COUNT; ++i)
	{
		auto it_palyer = m_userJettonScore[i].find(uid);
		if (it_palyer != m_userJettonScore[i].end())
		{
			playerJettonScore[i] = it_palyer->second;
			RecordPlayerBaiRenJettonInfo(pPlayer, i, -playerJettonScore[i]);

			m_allJettonScore[i] -= playerJettonScore[i];
			if (!pPlayer->IsRobot())
			{
				m_playerJettonScore[i] -= playerJettonScore[i];
			}
			if (pPlayer->IsRobot())
			{
				m_robotJettonScore[i] -= playerJettonScore[i];
			}
			m_userJettonScore[i].erase(it_palyer);

			bIsHavePlaceJetton = true;
		}

		LOG_DEBUG("jetton_cancel_area  - uid:%d,bIsHavePlaceJetton:%d,i:%d,playerJettonScore:%lld", uid, bIsHavePlaceJetton,i, playerJettonScore[i]);

	}

	LOG_DEBUG("jetton_cancel_success  - uid:%d,bIsHavePlaceJetton:%d,getGameState:%d", uid, bIsHavePlaceJetton, GetGameState());

	if (GetGameState() != net::TABLE_STATE_DICE_PLACE_JETTON || bIsHavePlaceJetton == false)
	{
		net::msg_dice_cancel_jetton_rep msg;
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_CANCEL_JETTON_REP);
		return true;
	}
	else
	{
		net::msg_dice_cancel_jetton_rep msg;
		msg.set_result(net::RESULT_CODE_SUCCESS);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_CANCEL_JETTON_REP);
	}

	if (bIsHavePlaceJetton)
	{
		net::msg_dice_cancel_jetton_broadcast broad;

		for (uint8 i = 0; i<AREA_COUNT; ++i)
		{
			broad.add_all_jetton_score(m_allJettonScore[i]);
		}
		SendMsgToAll(&broad, net::S2C_MSG_DICE_CANCEL_JETTON_BROADCAST);
	}

	return true;
}

//申请庄家
bool    CGameDiceTable::OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore)
{
    LOG_DEBUG("player apply banker:%d--%lld",pPlayer->GetUID(),bankerScore);
    //构造变量
    net::msg_dice_apply_banker_rep msg;
    msg.set_apply_oper(1);
    msg.set_buyin_score(bankerScore);
    
    if(m_pCurBanker == pPlayer){
        LOG_DEBUG("you is the banker");
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER); 
        return false;
    }
    if(IsSetJetton(pPlayer->GetUID())){// 下注不能上庄
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);
        return false;
    }    
    //合法判断
	int64 lUserScore = GetPlayerCurScore(pPlayer);
    if(bankerScore > lUserScore){
        LOG_DEBUG("you not have more score:%d",pPlayer->GetUID());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);    
        return false;    
    }
    
	if(bankerScore < GetApplyBankerCondition() || bankerScore > GetApplyBankerConditionLimit()){
		LOG_DEBUG("you score less than condition %lld--%lld，faild",GetApplyBankerCondition(),GetApplyBankerConditionLimit());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);  
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
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);
			return false;
		}
	}

	//保存信息 
	m_ApplyUserArray.push_back(pPlayer);
    m_mpApplyUserInfo[pPlayer->GetUID()] = autoAddScore;
    LockApplyScore(pPlayer,bankerScore);

    msg.set_result(net::RESULT_CODE_SUCCESS);    
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);
	//切换判断
	if(GetGameState() == net::TABLE_STATE_DICE_FREE && m_ApplyUserArray.size() == 1)
	{
		ChangeBanker(false);
	}
    FlushApplyUserSort();
    SendApplyUser();
    return true;
}
bool    CGameDiceTable::OnUserJumpApplyQueue(CGamePlayer* pPlayer)
{
    LOG_DEBUG("player jump queue:%d",pPlayer->GetUID());
    int64 cost = CDataCfgMgr::Instance().GetJumpQueueCost();  
    net::msg_dice_jump_apply_queue_rep msg;
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("the jump cost can't pay:%lld",cost);
        msg.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_JUMP_APPLY_QUEUE);
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
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_JUMP_APPLY_QUEUE);

            cost = -cost;
            pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_JUMPQUEUE,0,0,cost,0,0,0,0);
			return true;
		}
	}      
    
    return false;
}
//取消申请
bool    CGameDiceTable::OnUserCancelBanker(CGamePlayer* pPlayer)
{
    LOG_DEBUG("cance banker:%d",pPlayer->GetUID());

    net::msg_dice_apply_banker_rep msg;
    msg.set_apply_oper(0);
    msg.set_result(net::RESULT_CODE_SUCCESS);

    //前三局不能下庄
    if(pPlayer->GetUID() == GetBankerUID() && m_wBankerTime < 3)
        return false;
    //当前庄家 
	if(pPlayer->GetUID() == GetBankerUID() && GetGameState() != net::TABLE_STATE_DICE_FREE)
	{
		//发送消息
		LOG_DEBUG("the game is run,you can't cance banker");
        m_needLeaveBanker = true;
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);            
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
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_BANKER);			
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

//bool CGameDiceTable::KilledPlayerCtrl()
//{
//	uint32 robotBankerWinPro = m_pHostRoom->GetAoRbwRobotBankerWinPro();
//
//	bool bNeedChangeCard = g_RandGen.RandRatio(robotBankerWinPro, PRO_DENO_10000);
//	bool bIsBrankerWin = false;
//	if (bNeedChangeCard)
//	{
//		bIsBrankerWin = SetRobotBrankerWin();
//	}
//	return true;
//}

bool CGameDiceTable::NotWelfareCtrl()
{
	uint32 robotBankerWinPro = m_robotBankerWinPro;
	bool bIsBrankerWin = false;
	bool bNeedChangeCard = g_RandGen.RandRatio(robotBankerWinPro, PRO_DENO_10000);

	if (bNeedChangeCard)
	{
		bIsBrankerWin = SetRobotBrankerWin();
	}
	return true;
}

//发送扑克
bool    CGameDiceTable::DispatchTableCard()
{
    //重新洗牌
	tagDiceJackpotInfo tempDiceJackpotInfo = m_tagDiceJackpotInfo;
    m_GameLogic.ShakeRandDice(m_cbTableDice,DICE_COUNT);
	uint32 robotBankerWinPro = m_robotBankerWinPro;

	//int autokillsocre = 0;
	//int autokillstatus = 0;
	//bool HaveKilledPlayer = false;
	//int iAutoKillDefeat = CDataCfgMgr::Instance().GetBaiRenAutoKillDefeat();
	//int iAutoKillInterval = CDataCfgMgr::Instance().GetBaiRenAutoKillInterval();
	//if (m_pHostRoom != NULL)
	//{
	//	autokillsocre = m_pHostRoom->GetAutoKillSocre();
	//	autokillstatus = m_pHostRoom->GetAutoKillStatus();
	//}


	bool bIsFalgControlPlayer = false;
	bool bNeedChangeCard = g_RandGen.RandRatio(robotBankerWinPro, PRO_DENO_10000);
	bool bIsBrankerWin = false;
	bool bIsPlayerNoWinJackpot = false;
	bool bRobotSubJackpotPro = g_RandGen.RandRatio(tempDiceJackpotInfo.uRobotSubProbability, PRO_DENO_10000);
	bool bIsRobotSubJackpot = false;
	bool bAw = false;
	bool bIsStockControl = false;

	if (m_GameLogic.m_bIsControlDice == false)
	{
		bIsFalgControlPlayer = ProgressControlPalyer();
		if (!bIsFalgControlPlayer)
		{
			//bIsPlayerNoWinJackpot = SetPlayerNoWinJackpot();
		}
		if (bRobotSubJackpotPro &&!bIsFalgControlPlayer && !bIsPlayerNoWinJackpot && tempDiceJackpotInfo.lMaxPollScore <= tempDiceJackpotInfo.lCurPollScore)
		{
			bIsRobotSubJackpot = ProgressRobotSubJackpot();
		}

		// add by har
		if (!bIsFalgControlPlayer && !bIsRobotSubJackpot)
			bIsStockControl = SetStockWinLose(); // add by har end

		if (!bIsFalgControlPlayer && !bIsRobotSubJackpot && !bIsStockControl)
			bAw = ActiveWelfareCtrl();

		//if (bIsFalgControlPlayer == false)
		//{
		//	if (autokillsocre == 1)
		//	{
		//		HaveKilledPlayer = GetHaveKilledPlayer();
		//		if (HaveKilledPlayer)
		//		{
		//			if (autokillstatus == 1)
		//			{
		//				if (iAutoKillDefeat > m_pHostRoom->GetAutoKillDefeat())
		//				{
		//					KilledPlayerCtrl();
		//					m_pHostRoom->AddAutoKillDefeat();
		//					if (iAutoKillDefeat <= m_pHostRoom->GetAutoKillDefeat())
		//					{
		//						m_pHostRoom->SetAutoKillStatus(0);
		//					}
		//				}
		//				else
		//				{
		//					NotWelfareCtrl();
		//					m_pHostRoom->AddAutoKillInterval();
		//					if (iAutoKillInterval <= m_pHostRoom->GetAutoKillInterval())
		//					{
		//						m_pHostRoom->SetAutoKillStatus(1);
		//					}
		//				}
		//			}
		//			else
		//			{
		//				NotWelfareCtrl();
		//				m_pHostRoom->AddAutoKillInterval();
		//				if (iAutoKillInterval <= m_pHostRoom->GetAutoKillInterval())
		//				{
		//					m_pHostRoom->SetAutoKillStatus(1);
		//				}
		//			}
		//		}
		//		else
		//		{
		//			NotWelfareCtrl();
		//		}
		//	}
		//	else
		//	{
		//		if (!bIsFalgControlPlayer &&bNeedChangeCard && !bIsPlayerNoWinJackpot && !bIsRobotSubJackpot)
		//		{
		//			bIsBrankerWin = SetRobotBrankerWin();
		//		}
		//	}
		//}
		//else
		//{
		//	if (!bIsFalgControlPlayer &&bNeedChangeCard && !bIsPlayerNoWinJackpot && !bIsRobotSubJackpot)
		//	{
		//		bIsBrankerWin = SetRobotBrankerWin();
		//	}
		//}

	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("roomid:%d,tableid:%d,m_bIsControlDice:%d,bIsRobotSubJackpot:%d,bIsFalgControlPlayer:%d,bIsPlayerNoWinJackpot:%d,bRobotSubJackpotPro:%d,lMaxPollScore:%lld,lCurPollScore:%lld, bIsRobotSubJackpot:%d,bNeedChangeCard:%d,bIsBrankerWin:%d,robotBankerWinPro:%d,bAw:%d,bIsStockControl:%d,GetIsAllRobotOrPlayerJetton:%d,m_cbTableDice: %d,%d,%d",
		GetRoomID(), GetTableID(), m_GameLogic.m_bIsControlDice, bIsRobotSubJackpot, bIsFalgControlPlayer, bIsPlayerNoWinJackpot, bRobotSubJackpotPro, tempDiceJackpotInfo.lMaxPollScore, tempDiceJackpotInfo.lCurPollScore, bIsRobotSubJackpot, bNeedChangeCard, bIsBrankerWin, robotBankerWinPro, bAw, bIsStockControl, GetIsAllRobotOrPlayerJetton(), m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

    return true;
}

CGamePlayer * CGameDiceTable::GetPlayerByUid(uint32 uid)
{
	CGamePlayer * pGamePlayer = NULL;
	for (int nChairID = 0; nChairID < GAME_PLAYER; nChairID++)
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
		auto it_looker_calc_jetton = m_mpLookers.begin();
		for (; it_looker_calc_jetton != m_mpLookers.end(); it_looker_calc_jetton++)
		{
			CGamePlayer * pPlayer = it_looker_calc_jetton->second;
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
	}
	return pGamePlayer;
}


bool CGameDiceTable::ProgressRobotSubJackpot()
{
	// 机器人需要往有开奖池的区域下注
	bool bIsRobotJettonJackpotArea = false;
	bool bArrRobotJettonJackpotArea[AREA_COUNT] = { 0 };
	int iRobotJettonJackpotAreaCount = 0;
	for (uint32 uAreaIndex = 0; uAreaIndex < AREA_COUNT; uAreaIndex++)
	{
		if (uAreaIndex == CT_POINT_FOUR && m_robotJettonScore[uAreaIndex]>0)
		{
			bIsRobotJettonJackpotArea = true;
			bArrRobotJettonJackpotArea[uAreaIndex] = true;
			iRobotJettonJackpotAreaCount++;
		}
		else if (uAreaIndex == CT_POINT_SEVEN_TEEN&& m_robotJettonScore[uAreaIndex] > 0)
		{
			bIsRobotJettonJackpotArea = true;
			bArrRobotJettonJackpotArea[uAreaIndex] = true;
			iRobotJettonJackpotAreaCount++;
		}
		else if (uAreaIndex >= CT_TRIPLE_ONE && uAreaIndex <= CT_TRIPLE_SIX&& m_robotJettonScore[uAreaIndex] > 0)
		{
			bIsRobotJettonJackpotArea = true;
			bArrRobotJettonJackpotArea[uAreaIndex] = true;
			iRobotJettonJackpotAreaCount++;
		}
	}
	LOG_DEBUG("bIsRobotJettonJackpotArea:%d,iRobotJettonJackpotAreaCount:%d,m_cbTableDice:%d,%d,%d.", bIsRobotJettonJackpotArea, iRobotJettonJackpotAreaCount, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

	if (iRobotJettonJackpotAreaCount > 0)
	{
		int nMultiple = 0;
		uint32 uLoopCount = 9999;
		uint32 uIndex = 0;

		for (; uIndex < uLoopCount; uIndex++)
		{
			m_GameLogic.ComputeDiceResult();

			bool bIsWinJackpot = false;

			for (uint32 nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
			{
				bool bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				if (bArrRobotJettonJackpotArea[nAreaIndex] == true && bIsHit == true)
				{
					bIsWinJackpot = true;
				}
			}

			if (bIsWinJackpot == true)
			{
				//LOG_DEBUG("11 - uIndex:%d,iRobotJettonJackpotAreaCount:%d,m_cbTableDice:%d,%d,%d.", uIndex, iRobotJettonJackpotAreaCount, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

				return true;
			}
			else
			{
				m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
			}
		}
	}
	return false;
}

bool CGameDiceTable::ProgressControlPalyer()
{
	bool bIsFalgControl = false;
	bool bIsControlPlayerIsJetton = false;
	
	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		bIsControlPlayerIsJetton = IsSetJetton(control_uid);
		
		if (bIsControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
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

	return bIsFalgControl;
}


bool CGameDiceTable::SetControlPalyerWin(uint32 control_uid)
{
	map<uint32, int64> mpUserLostScore;
	map<uint32, int64> mpUserWinScore;
	map<uint32, int64> mpUserSubWinScore;
	map<uint32, int64> mpUserJackpotScore;

	map<uint32, vector<tag_dice_area_info>>	mpUserAreaInfo;

	uint32 uLoopCount = 9999;
	for (uint32 uIndex = 0; uIndex < uLoopCount; uIndex++)
	{
		mpUserLostScore.clear();
		mpUserWinScore.clear();
		mpUserAreaInfo.clear();
		mpUserSubWinScore.clear();
		mpUserJackpotScore.clear();

		int nMultiple = 0;
		bool bIsHit = false;
		int64 lBankerWinScore = 0;

		m_GameLogic.ComputeDiceResult();


		// 椅子用户
		for (int nChairID = 0; nChairID < GAME_PLAYER; nChairID++)
		{
			CGamePlayer * pPlayer = GetPlayer(nChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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
				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}

				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 旁观用户
		auto it_looker_calc_jetton = m_mpLookers.begin();
		for (; it_looker_calc_jetton != m_mpLookers.end(); it_looker_calc_jetton++)
		{
			CGamePlayer * pPlayer = it_looker_calc_jetton->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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

				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}
				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 奖池计算 ------------------------------------------------

		/*
		// 奖池增加
		tagDiceJackpotInfo	tagTttDiceJackpotInfo = m_tagDiceJackpotInfo;
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}
		auto it_looker_add_pool = m_mpLookers.begin();
		for (; it_looker_add_pool != m_mpLookers.end(); ++it_looker_add_pool)
		{
			CGamePlayer* pPlayer = it_looker_add_pool->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}

		// 用户中奖池

		uint32 uWinPoolScoreCountPoint = 0;
		uint32 uWinPoolScoreCountOneFive = 0;
		uint32 uWinPoolScoreCountSix = 0;

		map<uint32, vector<tagUserJackpotInfo>> mpUserJackpotInfo;
		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
						}
					}
				}
			}
		}

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			auto it_looker_calc_pool = m_mpLookers.begin();
			for (; it_looker_calc_pool != m_mpLookers.end(); ++it_looker_calc_pool)
			{
				CGamePlayer *pPlayer = it_looker_calc_pool->second;
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}

						}
					}
				}
			}
		}


		// 算人数平分奖池

		int64 lPoolScorePoint = 0;
		int64 lPoolScoreOneFive = 0;
		int64 lPoolScoreSix = 0;

		int64 lAverageScorePoint = 0;
		int64 lAverageScoreOneFive = 0;
		int64 lAverageScoreSix = 0;

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			double fPoolScorePoint = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
			lPoolScorePoint = (int64)fPoolScorePoint;
			double fPoolScoreOneFive = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
			lPoolScoreOneFive = (int64)fPoolScoreOneFive;
			double fPoolScoreSix = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
			lPoolScoreSix = (int64)fPoolScoreSix;
		}

		if (lPoolScorePoint > 0 && uWinPoolScoreCountPoint > 0)
		{
			double dAverageScorePoint = (double)lPoolScorePoint / (double)uWinPoolScoreCountPoint;
			lAverageScorePoint = (int64)dAverageScorePoint;
		}
		if (lPoolScoreOneFive > 0 && uWinPoolScoreCountOneFive > 0)
		{
			double fAverageScoreOneFive = (double)lPoolScoreOneFive / (double)uWinPoolScoreCountOneFive;
			lAverageScoreOneFive = (int64)fAverageScoreOneFive;
		}
		if (lPoolScoreSix > 0 && uWinPoolScoreCountSix > 0)
		{
			double fAverageScoreSix = (double)lPoolScoreSix / (double)uWinPoolScoreCountSix;
			lAverageScoreSix = (int64)fAverageScoreSix;
		}


		//用户中奖池加分
		auto it_begin_UserJackpotInfo = mpUserJackpotInfo.begin();
		for (; it_begin_UserJackpotInfo != mpUserJackpotInfo.end(); it_begin_UserJackpotInfo++)
		{
			vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_begin_UserJackpotInfo->second;
			for (uint32 i = 0; i < vecUserJackpotInfo.size(); i++)
			{
				if (vecUserJackpotInfo[i].area == CT_POINT_FOUR || vecUserJackpotInfo[i].area == CT_POINT_SEVEN_TEEN)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lAverageScorePoint;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScorePoint;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScorePoint);
				}
				else if (vecUserJackpotInfo[i].area >= CT_TRIPLE_ONE && vecUserJackpotInfo[i].area <= CT_TRIPLE_FIVE)
				{
					vecUserJackpotInfo[i].pscore = lPoolScoreOneFive;
					vecUserJackpotInfo[i].wscore = lAverageScoreOneFive;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreOneFive;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreOneFive);
				}
				else if (vecUserJackpotInfo[i].area == CT_TRIPLE_SIX)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lPoolScoreSix;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreSix;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreSix);
				}
			}
		}

		// 写用户分数 下注赢分数 和 中奖池分数
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			int64 lValueFee = 0;
			int64 lWinScore = lPlayerWinScore + lPlayerJackpotScore + lValueFee - lPoolAddScore;
			mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		}
		auto it_looker_calc_total = m_mpLookers.begin();
		for (; it_looker_calc_total != m_mpLookers.end(); ++it_looker_calc_total)
		{
			CGamePlayer* pPlayer = it_looker_calc_total->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			int64 lValueFee = 0;
			int64 lWinScore = lPlayerWinScore + lPlayerJackpotScore + lValueFee - lPoolAddScore;
			mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		}
		*/
		// 如果真实玩家赢金币总分数大于0则可以退出
		if (mpUserWinScore[control_uid] > 0)
		{
			return true;
		}
		else
		{
			m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
		}
	}

	return false;
}

bool CGameDiceTable::SetControlPalyerLost(uint32 control_uid)
{
	map<uint32, int64> mpUserLostScore;
	map<uint32, int64> mpUserWinScore;
	map<uint32, int64> mpUserSubWinScore;
	map<uint32, int64> mpUserJackpotScore;

	map<uint32, vector<tag_dice_area_info>>	mpUserAreaInfo;

	uint32 uLoopCount = 9999;
	for (uint32 uIndex = 0; uIndex < uLoopCount; uIndex++)
	{
		mpUserLostScore.clear();
		mpUserWinScore.clear();
		mpUserAreaInfo.clear();
		mpUserSubWinScore.clear();
		mpUserJackpotScore.clear();

		int nMultiple = 0;
		bool bIsHit = false;
		int64 lBankerWinScore = 0;

		m_GameLogic.ComputeDiceResult();


		// 椅子用户
		for (int nChairID = 0; nChairID < GAME_PLAYER; nChairID++)
		{
			CGamePlayer * pPlayer = GetPlayer(nChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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
				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}

				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 旁观用户
		auto it_looker_calc_jetton = m_mpLookers.begin();
		for (; it_looker_calc_jetton != m_mpLookers.end(); it_looker_calc_jetton++)
		{
			CGamePlayer * pPlayer = it_looker_calc_jetton->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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

				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}
				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 奖池计算 ------------------------------------------------

		/*
		// 奖池增加
		tagDiceJackpotInfo	tagTttDiceJackpotInfo = m_tagDiceJackpotInfo;
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}
		auto it_looker_add_pool = m_mpLookers.begin();
		for (; it_looker_add_pool != m_mpLookers.end(); ++it_looker_add_pool)
		{
			CGamePlayer* pPlayer = it_looker_add_pool->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}

		// 用户中奖池

		uint32 uWinPoolScoreCountPoint = 0;
		uint32 uWinPoolScoreCountOneFive = 0;
		uint32 uWinPoolScoreCountSix = 0;

		map<uint32, vector<tagUserJackpotInfo>> mpUserJackpotInfo;
		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
						}
					}
				}
			}
		}

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			auto it_looker_calc_pool = m_mpLookers.begin();
			for (; it_looker_calc_pool != m_mpLookers.end(); ++it_looker_calc_pool)
			{
				CGamePlayer *pPlayer = it_looker_calc_pool->second;
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}

						}
					}
				}
			}
		}


		// 算人数平分奖池

		int64 lPoolScorePoint = 0;
		int64 lPoolScoreOneFive = 0;
		int64 lPoolScoreSix = 0;

		int64 lAverageScorePoint = 0;
		int64 lAverageScoreOneFive = 0;
		int64 lAverageScoreSix = 0;

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			double fPoolScorePoint = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
			lPoolScorePoint = (int64)fPoolScorePoint;
			double fPoolScoreOneFive = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
			lPoolScoreOneFive = (int64)fPoolScoreOneFive;
			double fPoolScoreSix = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
			lPoolScoreSix = (int64)fPoolScoreSix;
		}

		if (lPoolScorePoint > 0 && uWinPoolScoreCountPoint > 0)
		{
			double dAverageScorePoint = (double)lPoolScorePoint / (double)uWinPoolScoreCountPoint;
			lAverageScorePoint = (int64)dAverageScorePoint;
		}
		if (lPoolScoreOneFive > 0 && uWinPoolScoreCountOneFive > 0)
		{
			double fAverageScoreOneFive = (double)lPoolScoreOneFive / (double)uWinPoolScoreCountOneFive;
			lAverageScoreOneFive = (int64)fAverageScoreOneFive;
		}
		if (lPoolScoreSix > 0 && uWinPoolScoreCountSix > 0)
		{
			double fAverageScoreSix = (double)lPoolScoreSix / (double)uWinPoolScoreCountSix;
			lAverageScoreSix = (int64)fAverageScoreSix;
		}


		//用户中奖池加分
		auto it_begin_UserJackpotInfo = mpUserJackpotInfo.begin();
		for (; it_begin_UserJackpotInfo != mpUserJackpotInfo.end(); it_begin_UserJackpotInfo++)
		{
			vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_begin_UserJackpotInfo->second;
			for (uint32 i = 0; i < vecUserJackpotInfo.size(); i++)
			{
				if (vecUserJackpotInfo[i].area == CT_POINT_FOUR || vecUserJackpotInfo[i].area == CT_POINT_SEVEN_TEEN)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lAverageScorePoint;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScorePoint;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScorePoint);
				}
				else if (vecUserJackpotInfo[i].area >= CT_TRIPLE_ONE && vecUserJackpotInfo[i].area <= CT_TRIPLE_FIVE)
				{
					vecUserJackpotInfo[i].pscore = lPoolScoreOneFive;
					vecUserJackpotInfo[i].wscore = lAverageScoreOneFive;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreOneFive;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreOneFive);
				}
				else if (vecUserJackpotInfo[i].area == CT_TRIPLE_SIX)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lPoolScoreSix;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreSix;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreSix);
				}
			}
		}

		// 写用户分数 下注赢分数 和 中奖池分数
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			int64 lValueFee = 0;
			int64 lWinScore = lPlayerWinScore + lPlayerJackpotScore + lValueFee - lPoolAddScore;
			mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		}
		auto it_looker_calc_total = m_mpLookers.begin();
		for (; it_looker_calc_total != m_mpLookers.end(); ++it_looker_calc_total)
		{
			CGamePlayer* pPlayer = it_looker_calc_total->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			int64 lValueFee = 0;
			int64 lWinScore = lPlayerWinScore + lPlayerJackpotScore + lValueFee - lPoolAddScore;
			mpUserWinScore[pPlayer->GetUID()] = lWinScore;
		}
		*/
		// 如果真实玩家赢金币总分数大于0则可以退出
		if (mpUserWinScore[control_uid] < 0)
		{
			return true;
		}
		else
		{
			m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
		}
	}

	return false;
}



bool CGameDiceTable::SetPlayerNoWinJackpot()
{
	bool bIsPlayerJettonJackpot = false;
	bool bIsHavePlayerJettonJackpot[AREA_COUNT] = { 0 };
	for (uint32 nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
	{
		if (m_playerJettonScore[nAreaIndex] >0 && nAreaIndex == CT_POINT_FOUR)
		{
			bIsPlayerJettonJackpot = true;
			bIsHavePlayerJettonJackpot[nAreaIndex] = true;

			//LOG_DEBUG("aa - nAreaIndex:%d,bIsPlayerJettonJackpot:%d,m_cbTableDice:%d,%d,%d.", nAreaIndex, bIsPlayerJettonJackpot, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

		}
		else if (m_playerJettonScore[nAreaIndex] >0 && nAreaIndex == CT_POINT_SEVEN_TEEN)
		{
			bIsPlayerJettonJackpot = true;
			bIsHavePlayerJettonJackpot[nAreaIndex] = true;

			//LOG_DEBUG("bb - nAreaIndex:%d,bIsPlayerJettonJackpot:%d,m_cbTableDice:%d,%d,%d.", nAreaIndex, bIsPlayerJettonJackpot, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

		}
		else if (m_playerJettonScore[nAreaIndex] > 0 && (nAreaIndex >= CT_TRIPLE_ONE && nAreaIndex <= CT_TRIPLE_SIX))
		{
			bIsPlayerJettonJackpot = true;
			bIsHavePlayerJettonJackpot[nAreaIndex] = true;

			//LOG_DEBUG("cc - nAreaIndex:%d,bIsPlayerJettonJackpot:%d,m_cbTableDice:%d,%d,%d.", nAreaIndex, bIsPlayerJettonJackpot, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

		}
	}

	int nMultiple = 0;
	uint32 uLoopCount = 9999;
	uint32 uIndex = 0;

	if (bIsPlayerJettonJackpot == true)
	{
		for (; uIndex < uLoopCount; uIndex++)
		{
			m_GameLogic.ComputeDiceResult();

			bool bIsPlayerNoWinJackpot = true;

			for (uint32 nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
			{
				bool bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				if (bIsHavePlayerJettonJackpot[nAreaIndex] == true && bIsHit == true)
				{
					bIsPlayerNoWinJackpot = false;
				}
			}

			if (bIsPlayerNoWinJackpot == true)
			{
				LOG_DEBUG("11 - uIndex:%d,bIsPlayerJettonJackpot:%d,m_cbTableDice:%d,%d,%d.", uIndex, bIsPlayerJettonJackpot, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

				return true;
			}
			else
			{
				m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
			}
		}
	}

	LOG_DEBUG("22 - uIndex:%d,bIsPlayerJettonJackpot:%d,m_cbTableDice:%d,%d,%d.",uIndex, bIsPlayerJettonJackpot, m_cbTableDice[0], m_cbTableDice[1], m_cbTableDice[2]);

	return false;
}
bool CGameDiceTable::SetRobotBrankerWin()
{
	map<uint32, int64> mpUserLostScore;
	map<uint32, int64> mpUserWinScore;
	//map<uint32, int64> mpUserSubWinScore;
	//map<uint32, int64> mpUserJackpotScore;

	map<uint32, vector<tag_dice_area_info>>	mpUserAreaInfo;

	uint32 uLoopCount = 1000;
	for (uint32 uIndex = 0; uIndex < uLoopCount; uIndex++)
	{
		mpUserLostScore.clear();
		mpUserWinScore.clear();
		mpUserAreaInfo.clear();
		//mpUserSubWinScore.clear();
		//mpUserJackpotScore.clear();

		int nMultiple = 0;
		bool bIsHit = false;
		int64 lBankerWinScore = 0;

		m_GameLogic.ComputeDiceResult();


		// 椅子用户
		for (int nChairID = 0; nChairID < GAME_PLAYER; nChairID++)
		{
			CGamePlayer * pPlayer = GetPlayer(nChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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
				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}

				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 旁观用户
		auto it_looker_calc_jetton = m_mpLookers.begin();
		for (; it_looker_calc_jetton != m_mpLookers.end(); it_looker_calc_jetton++)
		{
			CGamePlayer * pPlayer = it_looker_calc_jetton->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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

				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}
				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 奖池计算 ------------------------------------------------

		/*
		// 奖池增加
		tagDiceJackpotInfo	tagTttDiceJackpotInfo = m_tagDiceJackpotInfo;
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}
		auto it_looker_add_pool = m_mpLookers.begin();
		for (; it_looker_add_pool != m_mpLookers.end(); ++it_looker_add_pool)
		{
			CGamePlayer* pPlayer = it_looker_add_pool->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			if (lPlayerWinScore <= 0)
			{
				continue;
			}
			double fPoolAddScore = ((double)lPlayerWinScore * (double)tagTttDiceJackpotInfo.uPoolAddRatio / (double)PRO_DENO_10000);
			int64 lPoolAddScore = (int64)fPoolAddScore;
			mpUserSubWinScore[pPlayer->GetUID()] = lPoolAddScore;
			tagTttDiceJackpotInfo.UpdateCurPoolScore(lPoolAddScore);
		}

		// 用户中奖池

		uint32 uWinPoolScoreCountPoint = 0;
		uint32 uWinPoolScoreCountOneFive = 0;
		uint32 uWinPoolScoreCountSix = 0;

		map<uint32, vector<tagUserJackpotInfo>> mpUserJackpotInfo;
		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
						}
					}
				}
			}
		}

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			auto it_looker_calc_pool = m_mpLookers.begin();
			for (; it_looker_calc_pool != m_mpLookers.end(); ++it_looker_calc_pool)
			{
				CGamePlayer *pPlayer = it_looker_calc_pool->second;
				if (pPlayer == NULL)
				{
					continue;
				}
				if (!IsSetJetton(pPlayer->GetUID()))
				{
					continue;
				}
				uint32 uid = pPlayer->GetUID();
				//是否中奖池
				auto it_player = mpUserAreaInfo.find(pPlayer->GetUID());
				if (it_player != mpUserAreaInfo.end())
				{
					auto & vec_dice_area_info = it_player->second;
					for (uint32 uInfoIndex = 0; uInfoIndex < vec_dice_area_info.size(); uInfoIndex++)
					{
						int64 lWinPoolScore = 0;
						if (vec_dice_area_info[uInfoIndex].final_score > 0 && tagTttDiceJackpotInfo.lCurPollScore>0)
						{
							if (vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_FOUR || vec_dice_area_info[uInfoIndex].jetton_area == CT_POINT_SEVEN_TEEN)
							{
								uWinPoolScoreCountPoint++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubPoint;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area >= CT_TRIPLE_ONE && vec_dice_area_info[uInfoIndex].jetton_area <= CT_TRIPLE_FIVE)
							{
								uWinPoolScoreCountOneFive++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubOneFive;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}
							else if (vec_dice_area_info[uInfoIndex].jetton_area == CT_TRIPLE_SIX)
							{
								uWinPoolScoreCountSix++;
								tagUserJackpotInfo JackpotInfo;
								JackpotInfo.area = vec_dice_area_info[uInfoIndex].jetton_area;
								JackpotInfo.ratio = tagTttDiceJackpotInfo.uPoolSubSixRatio;
								JackpotInfo.wscore = lWinPoolScore;
								auto it_UserJackpotInfo = mpUserJackpotInfo.find(pPlayer->GetUID());
								if (it_UserJackpotInfo != mpUserJackpotInfo.end())
								{
									vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_UserJackpotInfo->second;
									vecUserJackpotInfo.push_back(JackpotInfo);
								}
								else
								{
									vector<tagUserJackpotInfo> vecUserJackpotInfo;
									vecUserJackpotInfo.push_back(JackpotInfo);
									mpUserJackpotInfo.insert(make_pair(pPlayer->GetUID(), vecUserJackpotInfo));
								}
							}

						}
					}
				}
			}
		}
		

		// 算人数平分奖池

		int64 lPoolScorePoint = 0;
		int64 lPoolScoreOneFive = 0;
		int64 lPoolScoreSix = 0;

		int64 lAverageScorePoint = 0;
		int64 lAverageScoreOneFive = 0;
		int64 lAverageScoreSix = 0;

		if (tagTttDiceJackpotInfo.lCurPollScore > 0)
		{
			double fPoolScorePoint = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubPoint / (double)PRO_DENO_10000);
			lPoolScorePoint = (int64)fPoolScorePoint;
			double fPoolScoreOneFive = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubOneFive / (double)PRO_DENO_10000);
			lPoolScoreOneFive = (int64)fPoolScoreOneFive;
			double fPoolScoreSix = ((double)tagTttDiceJackpotInfo.lCurPollScore * (double)tagTttDiceJackpotInfo.uPoolSubSixRatio / (double)PRO_DENO_10000);
			lPoolScoreSix = (int64)fPoolScoreSix;
		}

		if (lPoolScorePoint > 0 && uWinPoolScoreCountPoint > 0)
		{
			double dAverageScorePoint = (double)lPoolScorePoint / (double)uWinPoolScoreCountPoint;
			lAverageScorePoint = (int64)dAverageScorePoint;
		}
		if (lPoolScoreOneFive > 0 && uWinPoolScoreCountOneFive > 0)
		{
			double fAverageScoreOneFive = (double)lPoolScoreOneFive / (double)uWinPoolScoreCountOneFive;
			lAverageScoreOneFive = (int64)fAverageScoreOneFive;
		}
		if (lPoolScoreSix > 0 && uWinPoolScoreCountSix > 0)
		{
			double fAverageScoreSix = (double)lPoolScoreSix / (double)uWinPoolScoreCountSix;
			lAverageScoreSix = (int64)fAverageScoreSix;
		}


		//用户中奖池加分
		auto it_begin_UserJackpotInfo = mpUserJackpotInfo.begin();
		for (; it_begin_UserJackpotInfo != mpUserJackpotInfo.end(); it_begin_UserJackpotInfo++)
		{
			vector<tagUserJackpotInfo> & vecUserJackpotInfo = it_begin_UserJackpotInfo->second;
			for (uint32 i = 0; i < vecUserJackpotInfo.size(); i++)
			{
				if (vecUserJackpotInfo[i].area == CT_POINT_FOUR || vecUserJackpotInfo[i].area == CT_POINT_SEVEN_TEEN)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lAverageScorePoint;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScorePoint;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScorePoint);
				}
				else if (vecUserJackpotInfo[i].area >= CT_TRIPLE_ONE && vecUserJackpotInfo[i].area <= CT_TRIPLE_FIVE)
				{
					vecUserJackpotInfo[i].pscore = lPoolScoreOneFive;
					vecUserJackpotInfo[i].wscore = lAverageScoreOneFive;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreOneFive;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreOneFive);
				}
				else if (vecUserJackpotInfo[i].area == CT_TRIPLE_SIX)
				{
					vecUserJackpotInfo[i].pscore = lPoolScorePoint;
					vecUserJackpotInfo[i].wscore = lPoolScoreSix;
					mpUserJackpotScore[it_begin_UserJackpotInfo->first] += lAverageScoreSix;
					tagTttDiceJackpotInfo.UpdateCurPoolScore(-lAverageScoreSix);
				}
			}
		}
		*/
		// 结算下注分数和奖池分数

		int64 lPlayerWinTotalScore = 0;
		int64 lRealPlayerWinTotalScore = 0;

		// 写用户分数 下注赢分数 和 中奖池分数
		for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
		{
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			//int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			//int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			//int64 lExWinScore = lPlayerJackpotScore - lPoolAddScore;
			//int64 lValueFee = 0;// CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, lExWinScore);
			//int64 lWinScore = lPlayerWinScore + lValueFee - lPoolAddScore;
			//mpUserWinScore[pPlayer->GetUID()] = lWinScore;

			//if (lWinScore>0)
			//{
			//	lPlayerWinTotalScore += lWinScore;
			//}
			if (!pPlayer->IsRobot())
			{
				//lRealPlayerWinTotalScore += (lPlayerWinScore + lValueFee - lPoolAddScore + lPlayerJackpotScore);
				lRealPlayerWinTotalScore += lPlayerWinScore;
			}
		}
		auto it_looker_calc_total = m_mpLookers.begin();
		for (; it_looker_calc_total != m_mpLookers.end(); ++it_looker_calc_total)
		{
			CGamePlayer* pPlayer = it_looker_calc_total->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 uid = pPlayer->GetUID();
			if (!IsSetJetton(pPlayer->GetUID()))
			{
				continue;
			}
			//int64 lPoolAddScore = mpUserSubWinScore[pPlayer->GetUID()];
			int64 lPlayerWinScore = mpUserWinScore[pPlayer->GetUID()];
			//int64 lPlayerJackpotScore = mpUserJackpotScore[pPlayer->GetUID()];
			//int64 lExWinScore = lPlayerJackpotScore - lPoolAddScore;
			//int64 lValueFee = 0;// CalcPlayerInfo(pPlayer->GetUID(), lPlayerWinScore, lExWinScore);
			//int64 lWinScore = lPlayerWinScore + lValueFee - lPoolAddScore;
			//mpUserWinScore[pPlayer->GetUID()] = lWinScore;

			//if (lWinScore>0)
			//{
			//	lPlayerWinTotalScore += lWinScore;
			//}
			if (!pPlayer->IsRobot())
			{
				//lRealPlayerWinTotalScore += (lPlayerWinScore + lValueFee - lPoolAddScore + lPlayerJackpotScore);
				lRealPlayerWinTotalScore += lPlayerWinScore;
			}
		}

		//for (uint8 i = 0; i<DICE_COUNT; ++i)
		//{
		//	tagTttDiceJackpotInfo.cbTableDice[i] = m_cbTableDice[i];
		//}
		//tagTttDiceJackpotInfo.utime = getSysTime();
		//tagTttDiceJackpotInfo.lWinTotalScore = lPlayerWinTotalScore;

		// 如果真实玩家赢金币总分数小于0则可以退出
		if (lRealPlayerWinTotalScore <= 0)
		{
			return true;
		}
		else
		{
			m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
		}
	}

	return false;
}


//发送庄家
void    CGameDiceTable::SendApplyUser(CGamePlayer* pPlayer)
{
    net::msg_dice_apply_list msg;
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
       pPlayer->SendMsgToClient(&msg,net::S2C_MSG_DICE_APPLY_LIST);
    }else{
       SendMsgToAll(&msg,net::S2C_MSG_DICE_APPLY_LIST);
    }    
}
//排序庄家
void    CGameDiceTable::FlushApplyUserSort()
{
    if(m_ApplyUserArray.size() > 1)
    {
        for(uint32 i = 0; i < m_ApplyUserArray.size(); ++i)
        {
            for(uint32 j=i+1;j < m_ApplyUserArray.size();++j)
            {
                if(i != j && CompareApplyBankers(m_ApplyUserArray[i], m_ApplyUserArray[j]))
                {
                    CGamePlayer* pTmp = m_ApplyUserArray[i];
                    m_ApplyUserArray[i] = m_ApplyUserArray[j];
                    m_ApplyUserArray[j] = pTmp;
                }
            }
        }
    }
}
//更换庄家
bool    CGameDiceTable::ChangeBanker(bool bCancelCurrentBanker)
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
        net::msg_dice_change_banker msg;
        msg.set_banker_user(GetBankerUID());
        msg.set_banker_score(m_lBankerScore);
        
        SendMsgToAll(&msg,net::S2C_MSG_DICE_CHANGE_BANKER);

        SendApplyUser(NULL);
	}

	return bChangeBanker; 
}
//轮换判断
void    CGameDiceTable::TakeTurns()
{
    vector<uint32> delIDs;
	for(uint32 i = 0; i < m_ApplyUserArray.size(); i++)
	{
		if(GetGameState() == net::TABLE_STATE_DICE_FREE)
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

				m_BankerTimeLimit = 6;// CApplication::Instance().call<int>("dicebankertime");
				break;
			}		
		}
	}
    for(uint16 i=0;i<delIDs.size();++i){
        RemoveApplyBanker(delIDs[i]);
    }    
}
//结算庄家
void    CGameDiceTable::CalcBankerScore()
{
    if(m_pCurBanker == NULL)
        return;
    net::msg_dice_banker_calc_rep msg;
    msg.set_banker_time(m_wBankerTime);
    msg.set_win_count(m_wBankerWinTime);
    msg.set_buyin_score(m_lBankerBuyinScore);
    msg.set_win_score(m_lBankerWinScore);
    msg.set_win_max(m_lBankerWinMaxScore);
    msg.set_win_min(m_lBankerWinMinScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_DICE_BANKER_CALC);
    
    int64 score = m_lBankerWinScore;
    int32 pro = 0;
    switch(m_wBankerTime)
    {
    case 3:
        pro = 8;
        break;
    case 4:
        pro = 6;
        break;
    case 5:
        pro = 5;
        break;
    default:
        break;
    }
    if(score > 200 && pro > 0)
    {
        int64 decScore = score*pro/PRO_DENO_100;
        LOG_DEBUG("提前下庄扣分:%lld",decScore);
        m_pCurBanker->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_FEE,2,0,-decScore,0,0,0,0);
    }

    LOG_DEBUG("the turn the banker win:%lld,rest:%lld,buyin:%lld",score,m_lBankerScore,m_lBankerBuyinScore);
    RemoveApplyBanker(m_pCurBanker->GetUID());

    //设置庄家
	m_pCurBanker = NULL;     
    m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
    m_robotChairSize = g_RandGen.RandRange(5, 7);//机器人座位数
}
//自动补币
void    CGameDiceTable::AutoAddBankerScore()
{
    //轮庄判断
	if(m_pCurBanker == NULL || m_bankerAutoAddScore == 0 || m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime)
        return;
	
	int64 lBankerScore = m_lBankerScore;
	//判断金币是否够
	if(lBankerScore >= m_lBankerInitBuyinScore)
        return;
    int64 diffScore   = m_lBankerInitBuyinScore - lBankerScore;
    int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerBuyinScore;
    if(canAddScore < diffScore){
        diffScore = canAddScore;
    }
    if((m_lBankerScore + diffScore) < GetApplyBankerCondition())
        return;
    
    m_lBankerBuyinScore += diffScore;
    m_lBankerScore      += diffScore;
    
    net::msg_dice_add_bankerscore_rep msg;
    msg.set_buyin_score(diffScore);
    
    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_DICE_ADD_BANKER_SCORE);             
}
//发送游戏记录
void  CGameDiceTable::SendPlayLog(CGamePlayer* pPlayer)
{
    net::msg_dice_play_log_rep msg;
    for(uint16 i=0;i<m_vecRecord.size();++i)
    {
        net::dice_play_log* plog = msg.add_logs();
        tagDiceGameRecord& record = m_vecRecord[i];

        plog->set_big_small(record.cbBigSmall);
        plog->set_sum_point(record.cbSumPoints);
        for(uint16 j=0;j<DICE_COUNT;j++){
			plog->add_cards(record.cbDiceRecord[j]);
        }
    }
    //LOG_DEBUG("发送牌局记录:%d",msg.logs_size());
	uint32 uid = 0;
    if(pPlayer != NULL) {
		uid = pPlayer->GetUID();
        pPlayer->SendMsgToClient(&msg, net::S2C_MSG_DICE_PLAY_LOG);
    }else{
        SendMsgToAll(&msg,net::S2C_MSG_DICE_PLAY_LOG);
    }
	LOG_DEBUG("发送牌局记录 -  uid:%d,size:%d", uid, msg.logs_size());
}
//最大下注
int64   CGameDiceTable::GetUserMaxJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
	if (pPlayer == NULL)
	{
		return 0;
	}
	int iTimer = 1;
	//已下注额
	int64 lNowJetton = 0;
	uint32 uid = pPlayer->GetUID();
	bool bIsRobot = pPlayer->IsRobot();
	for(int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		auto it_palyer = m_userJettonScore[nAreaIndex].find(uid);
		if (it_palyer != m_userJettonScore[nAreaIndex].end())
		{
			lNowJetton += it_palyer->second;
		}
        //lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}
	//庄家金币
	int64 lBankerScore = 9223372036854775807;	
	if (m_pCurBanker != NULL)
	{
		lBankerScore = m_lBankerScore;
	}
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lBankerScore -= m_allJettonScore[nAreaIndex] * m_GameLogic.m_nCompensateRatio[nAreaIndex];
	}

	//个人限制
	//int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton*iTimer)/iTimer;
	int64 lMeMaxScore = GetPlayerCurScore(pPlayer) - lNowJetton;
	//庄家限制
	//lMeMaxScore = min(lMeMaxScore,lBankerScore/iTimer);
	lMeMaxScore = min(lMeMaxScore, lBankerScore);
	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	LOG_DEBUG("user max jetton, uid:%d,bIsRobot:%d,m_pCurBanker:%p,lBankerScore:%lld,curScore:%lld,lNowJetton:%lld,lMeMaxScore:%lld,cbJettonArea:%d,lJettonScore:%lld", uid, bIsRobot,m_pCurBanker, lBankerScore, GetPlayerCurScore(pPlayer), lNowJetton, lMeMaxScore, cbJettonArea, lJettonScore);

	return (lMeMaxScore);
}
uint32  CGameDiceTable::GetBankerUID()
{
	if (m_pCurBanker != NULL)
	{
		return m_pCurBanker->GetUID();
	}
    return 0;
}
void    CGameDiceTable::RemoveApplyBanker(uint32 uid)
{
    LOG_DEBUG("remove apply banker:%d",uid);
    for(uint32 i=0; i<m_ApplyUserArray.size(); ++i)
	{
		//条件过滤
		CGamePlayer* pPlayer = m_ApplyUserArray[i];
		if(pPlayer->GetUID() == uid){
    		//删除玩家
    		m_ApplyUserArray.erase(m_ApplyUserArray.begin()+i);
            m_mpApplyUserInfo.erase(uid);
            UnLockApplyScore(pPlayer);
    		break;
		}
	}
    FlushApplyUserSort();
}
bool    CGameDiceTable::LockApplyScore(CGamePlayer* pPlayer,int64 score)
{
	if (GetPlayerCurScore(pPlayer) < score)
	{
		return false;
	}
    
    ChangePlayerCurScore(pPlayer,-score);     
    m_ApplyUserScore[pPlayer->GetUID()] = score;

    return true;
}
bool    CGameDiceTable::UnLockApplyScore(CGamePlayer* pPlayer)
{
    int64 lockScore = m_ApplyUserScore[pPlayer->GetUID()];
    ChangePlayerCurScore(pPlayer,lockScore);
    
    return true;
}    
//庄家站起
void    CGameDiceTable::StandUpBankerSeat(CGamePlayer* pPlayer)
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
bool    CGameDiceTable::IsSetJetton(uint32 uid)
{
	//if (TABLE_STATE_DICE_PLACE_JETTON != GetGameState())
	//{
	//	return false;
	//}
    
    for(uint8 i=0;i<AREA_COUNT;++i)
	{
		auto it_palyer = m_userJettonScore[i].find(uid);
		if (it_palyer != m_userJettonScore[i].end())
		{
			if (it_palyer->second > 0)
			{
				return true;
			}
		}
    }    
    return false;    
}    
bool    CGameDiceTable::IsInApplyList(uint32 uid)
{
	//存在判断
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		CGamePlayer * pPlayer = m_ApplyUserArray[nUserIdx];
		if (pPlayer != NULL)
		{
			uint32 id = pPlayer->GetUID();
			if (id == uid)
			{
				return true;
			}
		}
	}    
    return false;
}

bool    CGameDiceTable::IsInRobotAlreadyJetton(uint32 uid, CGamePlayer* pPlayer)
{
	for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
	{
		if (m_robotPlaceJetton[i].uid == uid && m_robotPlaceJetton[i].pPlayer == pPlayer)
		{
			return true;
		}
	}

	return false;
}


//申请条件
int64   CGameDiceTable::GetApplyBankerCondition()
{
    return GetBaseScore();
}
int64   CGameDiceTable::GetApplyBankerConditionLimit()
{
    return GetBaseScore()*20;
}
//次数限制
int32   CGameDiceTable::GetBankerTimeLimit()
{
    return m_BankerTimeLimit;
}
//申请庄家队列排序
bool	CGameDiceTable::CompareApplyBankers(CGamePlayer* pBanker1,CGamePlayer* pBanker2)
{
	if (m_ApplyUserScore[pBanker1->GetUID()] < m_ApplyUserScore[pBanker2->GetUID()])
	{
		return true;
	}

    return false;
}

void    CGameDiceTable::OnRobotOper()
{
	if (m_bIsRobotAlreadyJetton)
	{
		return;
	}
	vector<CGamePlayer*> robots;
	GetAllRobotPlayer(robots);
	GetJettonArea();
	OnChairRobotJetton();
	

    for(uint32 i=0;i<robots.size();++i)
    {
        CGamePlayer* pPlayer = robots[i];
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		bool bIsRobotJettonSuccess = PushRobotPlaceJetton(pPlayer);

		if (bIsRobotJettonSuccess == false)
		{
			continue;
			//break;
		}
    }
	LOG_DEBUG("-------------------- robots.size:%d,m_robotJettonArea.size:%d,m_robotPlaceJetton.size:%d", robots.size(), m_robotJettonArea.size(), m_robotPlaceJetton.size());
	
	m_lRemainAreaLastRobotJettonTime = getSysTime() + g_RandGen.RandRange(3, 4) * 1000;

	m_bIsRobotAlreadyJetton = true;
}

bool	CGameDiceTable::RobotContinueJetton(CGamePlayer* pPlayer,uint8 area)
{
	bool bIsRestrictScore = false;
	if (area== CT_POINT_FOUR || area == CT_POINT_SEVEN_TEEN || area == CT_ANY_CICLE_DICE)
	{
		bIsRestrictScore == true;
	}
	else if (CT_TRIPLE_ONE >= CT_TRIPLE_ONE && area <= CT_TRIPLE_SIX)
	{
		bIsRestrictScore == true;
	}

	LOG_DEBUG("uid:%d,area:%d,bIsRestrictScore:%d,m_robotJettonScore[area]:%lld", pPlayer->GetUID(), area, bIsRestrictScore, m_robotJettonScore[area]);

	if (bIsRestrictScore)
	{
		if (m_robotJettonScore[area] >= 10000)
		{
			return false;
		}
	}
	return true;
}

void	CGameDiceTable::OnRobotPlaceJetton()
{
	if (m_robotPlaceJetton.size() == 0)
	{
		return;
	}
	vector<tagRobotPlaceJetton>	vecRobotPlaceJetton;

	for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
	{
		if (m_robotPlaceJetton.size() == 0)
		{
			return;
		}
		tagRobotPlaceJetton robotPlaceJetton = m_robotPlaceJetton[i];
		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;

		if (pPlayer == NULL) {
			continue;
		}
		if (m_robotPlaceJetton.size() == 0)
		{
			return;
		}
		uint32 passtick = m_coolLogic.getPassTick();
		passtick = passtick / 1000;
		if (robotPlaceJetton.time <= passtick && m_robotPlaceJetton[i].bflag == false)
		{
			bool bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			bool bIsRobotContinueJetton = RobotContinueJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area);
			if (bIsInTable && bIsRobotContinueJetton)
			{
				bool bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_robotPlaceJetton[i].bflag = true;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
		else if (m_robotPlaceJetton[i].bflag == true)
		{
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
		//LOG_DEBUG("uid:%d,time:%d,passtick:%d", pPlayer->GetUID(), robotPlaceJetton.time, passtick);
	}

	for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
	{
		auto iter_begin = m_robotPlaceJetton.begin();
		for (; iter_begin != m_robotPlaceJetton.end(); iter_begin++)
		{
			if (iter_begin->bflag == true)
			{
				m_robotPlaceJetton.erase(iter_begin);
				break;
			}
		}
	}
}

void	CGameDiceTable::RemainAreaRobotJetton()
{
	if (m_robotJettonArea.size() == 0)
	{
		return;
	}
	uint64 lCurTime  = getSysTime();
	if (m_lRemainAreaLastRobotJettonTime > lCurTime)
	{
		return;
	}

	vector<CGamePlayer*> robots;
	GetAllRobotPlayer(robots);
	uint64 lDelayTime = g_RandGen.RandRange(3, 4) * 1000;
	m_lRemainAreaLastRobotJettonTime = lCurTime + lDelayTime;

	int iRemainAreaJettonCount = g_RandGen.RandRange(2, 4);
	for (int iIndex = 0; iIndex < iRemainAreaJettonCount; iIndex++)
	{
		if (m_robotJettonArea.size() == 0)
		{
			return;
		}
		int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
		if (m_robotJettonArea.size() == 1)
		{
			iJettonAreaPos = 0;
		}
		int iJettonRobotPos = g_RandGen.RandRange(0, robots.size() - 1);
		if (m_robotJettonArea.size() == 1)
		{
			iJettonRobotPos = 0;
		}
		CGamePlayer* pPlayer = robots[iJettonRobotPos];
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		uint8 cbJettonArea = m_robotJettonArea[iJettonAreaPos].area;
		int64 lJettonScore = GetRobotJettonScore(pPlayer);
		if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
		{
			continue;
		}
		if (lJettonScore == 0)
		{
			continue;
		}
		bool bIsRobotContinueJetton = RobotContinueJetton(pPlayer, cbJettonArea);
		bool bflag = true;
		if (bIsRobotContinueJetton)
		{
			bflag = OnUserPlaceJetton(pPlayer, cbJettonArea, lJettonScore);
		}
		
		if (bflag)
		{
			if (m_robotJettonArea.size() > 0)
			{
				if (m_robotJettonArea[iJettonAreaPos].count > 0)
				{
					m_robotJettonArea[iJettonAreaPos].count--;
					if (m_robotJettonArea[iJettonAreaPos].count <= 0)
					{
						m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
					}
				}
				else
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
		}
	}



	/*
	for (uint32 i = 0; i < robots.size(); ++i)
	{
		CGamePlayer* pPlayer = robots[i];
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		uint8 cbJettonArea = m_robotJettonArea[0].area;
		int64 lJettonScore = GetRobotJettonScore(pPlayer);
		if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
		{
			continue;
		}
		if (lJettonScore == 0)
		{
			continue;
		}
		bool bflag = OnUserPlaceJetton(pPlayer, cbJettonArea, lJettonScore);
		if (bflag)
		{
			m_robotJettonArea[0].count--;
			if (m_robotJettonArea[0].count <= 0)
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin());
			}
		}
	}
	*/
}

int64 CGameDiceTable::GetRobotJettonScore(CGamePlayer* pPlayer)
{
	int64 lUserRealJetton = 100;
	int64 lUserMinJetton = 100;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer,0,0);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);

	if (lUserCurJetton < 2000)
	{
		lUserRealJetton = 0;
	}
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000)
	{
		if (g_RandGen.RandRatio(77, PRO_DENO_100))
		{
			lUserRealJetton = 100;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
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
		if (g_RandGen.RandRatio(60, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(17, PRO_DENO_100))
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
		if (g_RandGen.RandRatio(3470, PRO_DENO_10000))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(2500, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3500, PRO_DENO_10000))
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
		if (g_RandGen.RandRatio(6300, PRO_DENO_10000))
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

	//LOG_DEBUG("uid:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", pPlayer->GetUID(), lUserMinJetton, lUserMaxJetton, lUserRealJetton);
	//int switch_on_ex = g_RandGen.RandRange(0, 5);
	//switch (switch_on_ex)
	//{
	//case 0:
	//{
	//	lUserRealJetton = 100;
	//}break;
	//case 1:
	//{
	//	lUserRealJetton = 1000;
	//}break;
	//case 2:
	//{
	//	lUserRealJetton = 5000;
	//}break;
	//case 3:
	//{
	//	lUserRealJetton = 10000;
	//}break;
	//case 4:
	//{
	//	lUserRealJetton = 50000;
	//}break;
	//case 5:
	//{
	//	//lUserRealJetton = 200000;
	//	lUserRealJetton = 100;
	//}break;
	//default:
	//{
	//	lUserRealJetton = 100;
	//}break;
	//}

	//if (lUserCurJetton>0 && lUserCurJetton <= 50000)
	//{
	//	if (lUserRealJetton > 5000)
	//	{
	//		lUserRealJetton = 5000;
	//	}
	//}
	//if (lUserCurJetton>50000 && lUserCurJetton <= 500000)
	//{
	//	if (lUserRealJetton < 1000)
	//	{
	//		lUserRealJetton = 10;
	//	}
	//	if (lUserRealJetton > 50000)
	//	{
	//		lUserRealJetton = 50000;
	//	}
	//}
	//if (lUserCurJetton>500000 && lUserCurJetton <= 2000000)
	//{
	//	if (lUserRealJetton < 5000)
	//	{
	//		lUserRealJetton = 5000;
	//	}
	//	if (lUserRealJetton > 200000)
	//	{
	//		//lUserRealJetton = 200000;
	//		lUserRealJetton = 100;
	//	}
	//}
	//if (lUserCurJetton>2000000)
	//{
	//	if (lUserRealJetton < 10000)
	//	{
	//		lUserRealJetton = 10000;
	//	}
	//	if (lUserRealJetton > 500000)
	//	{
	//		//lUserRealJetton = 500000;
	//		lUserRealJetton = 100;
	//	}
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 200000)
	//{
	//	lUserRealJetton = 50000;
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 50000)
	//{
	//	lUserRealJetton = 10000;
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 10000)
	//{
	//	lUserRealJetton = 5000;
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 5000)
	//{
	//	lUserRealJetton = 1000;
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 1000)
	//{
	//	lUserRealJetton = 100;
	//}
	//if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100)
	//{
	//	lUserRealJetton = 0;
	//}
	////lUserRealJetton = (lUserRealJetton / 100) * 100;
	//if (lUserRealJetton < lUserMinJetton)
	//{
	//	lUserRealJetton = 0;
	//}
	return lUserRealJetton;
}

bool CGameDiceTable::OnChairRobotJetton()
{
	//if (m_bIsChairRobotAlreadyJetton)
	//{
	//	return;
	//}
	m_chairRobotPlaceJetton.clear();
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(1, 9);
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				continue;
			}
			//robots.push_back(pPlayer);

			if (m_robotJettonArea.size() == 0)
			{
				return false;
			}
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}
			uint8 cbJettonArea = m_robotJettonArea[iJettonAreaPos].area;

			if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				continue;
			}

			//LOG_DEBUG("size:%d %d,i:%d,uid:%d,switch_on:%d,cbJettonArea:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", m_robotJettonArea.size(), i, pPlayer->GetUID(), switch_on,cbJettonArea, lUserMinJetton, lUserMaxJetton, lUserRealJetton);

			int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
			if (lUserRealJetton == 0)
			{
				//LOG_DEBUG("1111111111111111111111111111111");
				continue;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			uint32 uRemainTime = m_coolLogic.getCoolTick();
			uRemainTime = uRemainTime / 1000;

			uint32 passtick = m_coolLogic.getPassTick();
			passtick = passtick / 1000;
			uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
			robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
			if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
			{
				continue;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;

			for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); i++)
			{
				if (m_chairRobotPlaceJetton[i].uid == robotPlaceJetton.uid && m_chairRobotPlaceJetton[i].area == robotPlaceJetton.area)
				{
					return false;
				}
			}
			m_chairRobotPlaceJetton.push_back(robotPlaceJetton);
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
			else
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
			}
		}
	}
	//m_bIsChairRobotAlreadyJetton = true;
	return true;
}

void	CGameDiceTable::OnChairRobotPlaceJetton()
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

		if (pPlayer == NULL) {
			continue;
		}
		if (m_chairRobotPlaceJetton.size() == 0)
		{
			return;
		}
		uint32 passtick = m_coolLogic.getPassTick();
		passtick = passtick / 1000;
		if (robotPlaceJetton.time <= passtick && m_chairRobotPlaceJetton[i].bflag == false)
		{
			bool bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			bool bIsRobotContinueJetton = RobotContinueJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area);
			if (bIsInTable && bIsRobotContinueJetton)
			{
				bool bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_chairRobotPlaceJetton[i].bflag = true;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
		else if (m_chairRobotPlaceJetton[i].bflag == true)
		{
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
		//LOG_DEBUG("uid:%d,time:%d,passtick:%d", pPlayer->GetUID(), robotPlaceJetton.time, passtick);
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

void    CGameDiceTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots) {
	/*robots.clear();
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		robots.push_back(pPlayer);
	}*/
	GetAllLookersRobotPlayer(robots);
}

uint8	CGameDiceTable::GetRandJettonArea(int switch_on)
{
	uint8  enTempDiceType = CT_ERROR;

	switch (switch_on)
	{
		// 大小
	case 0:
	{
		enTempDiceType = CT_SUM_SMALL;
	}break;
	case 1:
	{
		enTempDiceType = CT_SUM_BIG;
	}break;
	// 点数 4 - 17
	case 2:
	{
		enTempDiceType = CT_POINT_FOUR;
	}break;
	case 3:
	{
		enTempDiceType = CT_POINT_FIVE;
	}break;
	case 4:
	{
		enTempDiceType = CT_POINT_SIX;
	}break;
	case 5:
	{
		enTempDiceType = CT_POINT_SEVEN;
	}break;
	case 6:
	{
		enTempDiceType = CT_POINT_EIGHT;
	}break;
	case 7:
	{
		enTempDiceType = CT_POINT_NINE;
	}break;
	case 8:
	{
		enTempDiceType = CT_POINT_TEN;
	}break;
	case 9:
	{
		enTempDiceType = CT_POINT_ELEVEN;
	}break;
	case 10:
	{
		enTempDiceType = CT_POINT_TWELVE;
	}break;
	case 11:
	{
		enTempDiceType = CT_POINT_THIR_TEEN;
	}break;
	case 12:
	{
		enTempDiceType = CT_POINT_FOUR_TEEN;
	}break;
	case 13:
	{
		enTempDiceType = CT_POINT_FIF_TEEN;
	}break;
	case 14:
	{
		enTempDiceType = CT_POINT_SIX_TEEN;
	}break;
	case 15:
	{
		enTempDiceType = CT_POINT_SEVEN_TEEN;
	}break;
	// 任意围骰
	case 16:
	{
		enTempDiceType = CT_ANY_CICLE_DICE;
	}break;
	// 一个1点 - 6点
	case 17:
	{
		enTempDiceType = CT_ONE;
	}break;
	case 18:
	{
		enTempDiceType = CT_TWO;
	}break;
	case 19:
	{
		enTempDiceType = CT_THREE;
	}break;
	case 20:
	{
		enTempDiceType = CT_FOUR;
	}break;
	case 21:
	{
		enTempDiceType = CT_FIVE;
	}break;
	case 22:
	{
		enTempDiceType = CT_SIX;
	}break;
	// 两个1点 - 6点
	case 23:
	{
		enTempDiceType = CT_TWICE_ONE;
	}break;
	case 24:
	{
		enTempDiceType = CT_TWICE_TWO;
	}break;
	case 25:
	{
		enTempDiceType = CT_TWICE_THREE;
	}break;
	case 26:
	{
		enTempDiceType = CT_TWICE_FOUR;
	}break;
	case 27:
	{
		enTempDiceType = CT_TWICE_FIVE;
	}break;
	case 28:
	{
		enTempDiceType = CT_TWICE_SIX;
	}break;
	// 三个1点 - 6点
	case 29:
	{
		enTempDiceType = CT_TRIPLE_ONE;
	}break;
	case 30:
	{
		enTempDiceType = CT_TRIPLE_TWO;
	}break;
	case 31:
	{
		enTempDiceType = CT_TRIPLE_THREE;
	}break;
	case 32:
	{
		enTempDiceType = CT_TRIPLE_FOUR;
	}break;
	case 33:
	{
		enTempDiceType = CT_TRIPLE_FIVE;
	}break;
	case 34:
	{
		enTempDiceType = CT_TRIPLE_SIX;
	}break;
	// 错误类型
	default:
	{
		enTempDiceType = CT_ERROR;
	}break;
	}
	return enTempDiceType;
}

void	CGameDiceTable::GetJettonArea()
{
	// 小和大每次必定下
	tagRobotJettonCountArea CountArea_Small;
	CountArea_Small.area = CT_SUM_SMALL;
	CountArea_Small.count = g_RandGen.RandRange(7, 16);
	m_robotJettonArea.push_back(CountArea_Small);

	tagRobotJettonCountArea CountArea_Big;
	CountArea_Big.area = CT_SUM_BIG;
	CountArea_Big.count = g_RandGen.RandRange(7, 16);
	m_robotJettonArea.push_back(CountArea_Big);

	// 下注的区域每局4-17，14个位置下注任意9个以上,9-14个位置
	int point_area_count = g_RandGen.RandRange(9, 14);
	for (uint8 cbPonitArea = CT_POINT_FOUR; cbPonitArea <= CT_POINT_SEVEN_TEEN; cbPonitArea++)
	{
		if (point_area_count > 0 && g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			tagRobotJettonCountArea CountArea;
			CountArea.area = cbPonitArea;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			point_area_count--;
		}
	}
	if (point_area_count > 0)
	{
		for (uint8 cbPonitArea = CT_POINT_FOUR; cbPonitArea <= CT_POINT_SEVEN_TEEN; cbPonitArea++)
		{
			bool bIsInJettonArea = false;
			for (uint32 uJettonArea = 0; uJettonArea < m_robotJettonArea.size(); uJettonArea++)
			{
				if (cbPonitArea == m_robotJettonArea[uJettonArea].area)
				{
					bIsInJettonArea = true;
					break;
				}
			}
			if (bIsInJettonArea)
			{
				continue;
			}
			tagRobotJettonCountArea CountArea;
			CountArea.area = cbPonitArea;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			point_area_count--;
			if (point_area_count <= 0)
			{
				break;
			}
		}
	}

	// 最下面的指定单骰赔率1-3的  指定双骰1赔10   4-6个位置下注
	int twice_area_count = g_RandGen.RandRange(4, 6);
	for (uint8 cbTwiceArea = CT_ONE; cbTwiceArea <= CT_TWICE_SIX; cbTwiceArea++)
	{
		if (twice_area_count > 0 && g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			tagRobotJettonCountArea CountArea;
			CountArea.area = cbTwiceArea;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			twice_area_count--;
		}
	}
	if (twice_area_count > 0)
	{
		for (uint8 cbTwiceArea = CT_ONE; cbTwiceArea <= CT_TWICE_SIX; cbTwiceArea++)
		{
			bool bIsInJettonArea = false;
			for (uint32 uJettonArea = 0; uJettonArea < m_robotJettonArea.size(); uJettonArea++)
			{
				if (cbTwiceArea == m_robotJettonArea[uJettonArea].area)
				{
					bIsInJettonArea = true;
					break;
				}
			}
			if (bIsInJettonArea)
			{
				continue;
			}
			tagRobotJettonCountArea CountArea;
			CountArea.area = cbTwiceArea;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			twice_area_count--;
			if (twice_area_count <= 0)
			{
				break;
			}
		}
	}

	// 豹子区域任意围骰+指定围骰7个区域每次任意3-7个
	int triple_area_count = g_RandGen.RandRange(3, 7);
	if (triple_area_count > 0)
	{
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			tagRobotJettonCountArea CountArea;
			CountArea.area = CT_ANY_CICLE_DICE;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			triple_area_count--;
		}
	}
	if (triple_area_count > 0)
	{
		for (uint8 cbTripleArea = CT_TRIPLE_ONE; cbTripleArea <= CT_TRIPLE_SIX; cbTripleArea++)
		{
			if (triple_area_count > 0 && g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				tagRobotJettonCountArea CountArea;
				CountArea.area = cbTripleArea;
				CountArea.count = g_RandGen.RandRange(1, 3);
				m_robotJettonArea.push_back(CountArea);
				triple_area_count--;
			}
		}
	}

	if (triple_area_count > 0)
	{
		for (uint8 cbTripleArea = CT_POINT_FOUR; cbTripleArea <= CT_POINT_SEVEN_TEEN; cbTripleArea++)
		{
			bool bIsInJettonArea = false;
			for (uint32 uJettonArea = 0; uJettonArea < m_robotJettonArea.size(); uJettonArea++)
			{
				if (cbTripleArea == m_robotJettonArea[uJettonArea].area)
				{
					bIsInJettonArea = true;
					break;
				}
			}
			if (bIsInJettonArea)
			{
				continue;
			}
			tagRobotJettonCountArea CountArea;
			CountArea.area = cbTripleArea;
			CountArea.count = g_RandGen.RandRange(1, 3);
			m_robotJettonArea.push_back(CountArea);
			twice_area_count--;
			if (twice_area_count <= 0)
			{
				break;
			}
		}
	}

	RobotJettonJackpotArea();

	//for (uint32 uIndex = 0; uIndex < m_robotJettonArea.size(); uIndex++)
	//{
	//	LOG_DEBUG("size:%d,area:%d", m_robotJettonArea.size(), m_robotJettonArea[uIndex]);
	//}
}


bool CGameDiceTable::RobotJettonJackpotArea()
{
	tagDiceJackpotInfo tempDiceJackpotInfo = m_tagDiceJackpotInfo;

	if (tempDiceJackpotInfo.lMaxPollScore > tempDiceJackpotInfo.lCurPollScore)
	{
		return false;
	}

	bool bIsHavePointFour = false;
	bool bIsHavePointSevenTeen = false;

	for (uint32 uJettonArea = 0; uJettonArea < m_robotJettonArea.size(); uJettonArea++)
	{
		if (m_robotJettonArea[uJettonArea].area == CT_POINT_FOUR)
		{
			bIsHavePointFour = true;
		}
		if (m_robotJettonArea[uJettonArea].area == CT_POINT_SEVEN_TEEN)
		{
			bIsHavePointSevenTeen = true;
		}
	}

	//把 4 、17点放进去
	if (bIsHavePointFour == false)
	{
		// 小和大每次必定下
		tagRobotJettonCountArea CountArea_Small;
		CountArea_Small.area = CT_POINT_FOUR;
		CountArea_Small.count = g_RandGen.RandRange(1, 3);
		m_robotJettonArea.push_back(CountArea_Small);
	}
	if (bIsHavePointSevenTeen == false)
	{
		tagRobotJettonCountArea CountArea_Big;
		CountArea_Big.area = CT_POINT_SEVEN_TEEN;
		CountArea_Big.count = g_RandGen.RandRange(1, 3);
		m_robotJettonArea.push_back(CountArea_Big);
	}
	return true;
}


bool	CGameDiceTable::PushRobotPlaceJetton(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (m_robotJettonArea.size() == 0)
	{
		return false;
	}
	uint32 uJettonAreaCount = g_RandGen.RandRange(1, 3);
	if (uJettonAreaCount == 1 && g_RandGen.RandRatio(40, PRO_DENO_100))
	{
		BYTE cbJettonArea = CT_SUM_SMALL;
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			cbJettonArea = CT_SUM_BIG;
		}

		int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
		if (lUserRealJetton == 0)
		{
			return false;
		}
		tagRobotPlaceJetton robotPlaceJetton;
		robotPlaceJetton.uid = pPlayer->GetUID();
		robotPlaceJetton.pPlayer = pPlayer;
		uint32 uRemainTime = m_coolLogic.getCoolTick();
		uRemainTime = uRemainTime / 1000;

		uint32 passtick = m_coolLogic.getPassTick();
		passtick = passtick / 1000;
		uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
		robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
		if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
		{
			return false;
		}
		robotPlaceJetton.area = cbJettonArea;
		robotPlaceJetton.jetton = lUserRealJetton;
		robotPlaceJetton.bflag = false;

		for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
		{
			if (m_robotPlaceJetton[i].uid == robotPlaceJetton.uid && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
			{
				return false;
			}
		}
		m_robotPlaceJetton.push_back(robotPlaceJetton);

		if (m_robotJettonArea.size() > 0)
		{
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size()-1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
		}
	}
	else if(uJettonAreaCount == 1 || uJettonAreaCount == 2)
	{
		for (uint32 uIndex = 0; uIndex < uJettonAreaCount; uIndex++)
		{
			if (m_robotJettonArea.size() == 0)
			{
				return false;
			}
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}

			//int switch_on = g_RandGen.RandRange(0, 35);
			uint8 cbJettonArea = m_robotJettonArea[iJettonAreaPos].area;// GetRandJettonArea(switch_on);// g_RandGen.RandRange(CT_SUM_SMALL, CT_TRIPLE_SIX);
														   //bool bIsJettonFaileRation = g_RandGen.RandRatio(75, PRO_DENO_100);

			if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				continue;
			}

			//LOG_DEBUG("size:%d %d,i:%d,uid:%d,switch_on:%d,cbJettonArea:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", m_robotJettonArea.size(), i, pPlayer->GetUID(), switch_on,cbJettonArea, lUserMinJetton, lUserMaxJetton, lUserRealJetton);

			int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
			if (lUserRealJetton == 0)
			{
				//LOG_DEBUG("1111111111111111111111111111111");
				return false;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			uint32 uRemainTime = m_coolLogic.getCoolTick();
			uRemainTime = uRemainTime / 1000;

			uint32 passtick = m_coolLogic.getPassTick();
			passtick = passtick / 1000;
			uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
			robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
			if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
			{
				return false;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;

			for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
			{
				if (m_robotPlaceJetton[i].uid == robotPlaceJetton.uid && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
				{
					return false;
				}
			}
			m_robotPlaceJetton.push_back(robotPlaceJetton);
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
			else
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
			}
		}
	}
	else if (uJettonAreaCount == 3)
	{
		BYTE cbJettonArea = CT_SUM_SMALL;
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			cbJettonArea = CT_SUM_BIG;
		}

		int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
		if (lUserRealJetton == 0)
		{
			return false;
		}
		tagRobotPlaceJetton robotPlaceJetton;
		robotPlaceJetton.uid = pPlayer->GetUID();
		robotPlaceJetton.pPlayer = pPlayer;
		uint32 uRemainTime = m_coolLogic.getCoolTick();
		uRemainTime = uRemainTime / 1000;

		uint32 passtick = m_coolLogic.getPassTick();
		passtick = passtick / 1000;
		uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
		robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
		if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
		{
			return false;
		}
		robotPlaceJetton.area = cbJettonArea;
		robotPlaceJetton.jetton = lUserRealJetton;
		robotPlaceJetton.bflag = false;

		for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
		{
			if (m_robotPlaceJetton[i].uid == robotPlaceJetton.uid && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
			{
				return false;
			}
		}
		m_robotPlaceJetton.push_back(robotPlaceJetton);
		if (m_robotJettonArea.size() > 0)
		{
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
		}
		uJettonAreaCount--;
		for (uint32 uIndex = 0; uIndex < uJettonAreaCount; uIndex++)
		{
			if (m_robotJettonArea.size() == 0)
			{
				return false;
			}
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}

			//int switch_on = g_RandGen.RandRange(0, 35);
			uint8 cbJettonArea = m_robotJettonArea[iJettonAreaPos].area;// GetRandJettonArea(switch_on);// g_RandGen.RandRange(CT_SUM_SMALL, CT_TRIPLE_SIX);
																		//bool bIsJettonFaileRation = g_RandGen.RandRatio(75, PRO_DENO_100);

			if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				continue;
			}

			//LOG_DEBUG("size:%d %d,i:%d,uid:%d,switch_on:%d,cbJettonArea:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", m_robotJettonArea.size(), i, pPlayer->GetUID(), switch_on,cbJettonArea, lUserMinJetton, lUserMaxJetton, lUserRealJetton);

			int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
			if (lUserRealJetton == 0)
			{
				//LOG_DEBUG("1111111111111111111111111111111");
				return false;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			uint32 uRemainTime = m_coolLogic.getCoolTick();
			uRemainTime = uRemainTime / 1000;

			uint32 passtick = m_coolLogic.getPassTick();
			passtick = passtick / 1000;
			uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
			robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
			if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
			{
				return false;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;

			for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
			{
				if (m_robotPlaceJetton[i].uid == robotPlaceJetton.uid && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
				{
					return false;
				}
			}
			m_robotPlaceJetton.push_back(robotPlaceJetton);
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
			else
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
			}
		}

	}
	else
	{

		for (uint32 uIndex = 0; uIndex < uJettonAreaCount; uIndex++)
		{
			if (m_robotJettonArea.size() == 0)
			{
				return false;
			}
			int iJettonAreaPos = g_RandGen.RandRange(0, m_robotJettonArea.size() - 1);
			if (m_robotJettonArea.size() == 1)
			{
				iJettonAreaPos = 0;
			}

			//int switch_on = g_RandGen.RandRange(0, 35);
			uint8 cbJettonArea = m_robotJettonArea[iJettonAreaPos].area;// GetRandJettonArea(switch_on);// g_RandGen.RandRange(CT_SUM_SMALL, CT_TRIPLE_SIX);
																		//bool bIsJettonFaileRation = g_RandGen.RandRatio(75, PRO_DENO_100);

			if (cbJettonArea == CT_ERROR || cbJettonArea == CT_POINT_THREE || cbJettonArea == CT_POINT_EIGHT_TEEN || cbJettonArea == CT_LIMIT_CICLE_DICE)
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				continue;
			}

			//LOG_DEBUG("size:%d %d,i:%d,uid:%d,switch_on:%d,cbJettonArea:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", m_robotJettonArea.size(), i, pPlayer->GetUID(), switch_on,cbJettonArea, lUserMinJetton, lUserMaxJetton, lUserRealJetton);

			int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
			if (lUserRealJetton == 0)
			{
				//LOG_DEBUG("1111111111111111111111111111111");
				return false;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			uint32 uRemainTime = m_coolLogic.getCoolTick();
			uRemainTime = uRemainTime / 1000;

			uint32 passtick = m_coolLogic.getPassTick();
			passtick = passtick / 1000;
			uint32 uMaxDelayTime = s_PlaceJettonTime / 1000 - 3;
			robotPlaceJetton.time = g_RandGen.RandRange(3, uRemainTime);
			if (robotPlaceJetton.time <= 3 || robotPlaceJetton.time > uMaxDelayTime)
			{
				return false;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;

			for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
			{
				if (m_robotPlaceJetton[i].uid == robotPlaceJetton.uid && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
				{
					return false;
				}
			}
			m_robotPlaceJetton.push_back(robotPlaceJetton);
			if (m_robotJettonArea[iJettonAreaPos].count > 0)
			{
				m_robotJettonArea[iJettonAreaPos].count--;
				if (m_robotJettonArea[iJettonAreaPos].count <= 0)
				{
					m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
				}
			}
			else
			{
				m_robotJettonArea.erase(m_robotJettonArea.begin() + iJettonAreaPos);
			}
		}
	}

	return true;
}

bool    CGameDiceTable::IsInTableRobot(uint32 uid, CGamePlayer * pPlayer)
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

void    CGameDiceTable::OnRobotStandUp()
{
    // 保持一两个座位给玩家    
    vector<uint16> emptyChairs;
    vector<uint16> robotChairs;
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL)
		{
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
			if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
			{
				continue;
			}

            uint16 chairID = emptyChairs[g_RandGen.RandUInt()%emptyChairs.size()];
            if(CanSitDown(pPlayer,chairID))
			{
                PlayerSitDownStandUp(pPlayer,true,chairID);
                return;
            }
        }
    }
}
void 	CGameDiceTable::CheckRobotCancelBanker()
{
    if(m_pCurBanker != NULL && m_pCurBanker->IsRobot())
    {
        if(m_wBankerTime > 3 && m_lBankerWinScore > m_lBankerBuyinScore/2)
        {
            if(g_RandGen.RandRatio(65,100))
			{
                OnUserCancelBanker(m_pCurBanker);
            }
        }
    }

}
void    CGameDiceTable::CheckRobotApplyBanker()
{
	if (m_pCurBanker != NULL || m_ApplyUserArray.size() >= m_robotApplySize)
	{
		return;
	}
    //LOG_DEBUG("robot apply banker");
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
        int64 curScore = GetPlayerCurScore(pPlayer);
		if (curScore < GetApplyBankerCondition())
		{
			continue;
		}
        
        int64 buyinScore = GetApplyBankerCondition()*2;

        if(curScore < buyinScore)
        {
            buyinScore = curScore;
            buyinScore = (buyinScore/10000)*10000;
            OnUserApplyBanker(pPlayer,buyinScore,0);

        }else{
            buyinScore = g_RandGen.RandRange(buyinScore,curScore);
            buyinScore = (buyinScore/10000)*10000;

            OnUserApplyBanker(pPlayer,buyinScore,1);     
        }        
		if (m_ApplyUserArray.size() > m_robotApplySize)
		{
			break;
		}
    }        
}
//设置机器人庄家赢金币
void    CGameDiceTable::SetRobotBankerWin()
{
    //挑一服最大的牌给庄家机器人最小的给玩家机器人
	if (m_pCurBanker == NULL)
	{
		return;
	}
}
//设置机器人庄家输金币
void    CGameDiceTable::SetRobotBankerLose()
{
    //挑一服最小的牌给庄家机器人

}
void    CGameDiceTable::AddPlayerToBlingLog()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL)
		{
			continue;
		}
        for(uint8 i=0;i<AREA_COUNT;++i)
		{
			auto it_palyer = m_userJettonScore[i].find(pPlayer->GetUID());
			if (it_palyer != m_userJettonScore[i].end())
			{
				if (it_palyer->second > 0)
				{
					AddUserBlingLog(pPlayer);
					break;
				}

				//if (m_userJettonScore[i][pPlayer->GetUID()] > 0)
				//{
				//	AddUserBlingLog(pPlayer);
				//	break;
				//}
			}
        }           
    }

    AddUserBlingLog(m_pCurBanker);
}


void CGameDiceTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_dice_play_log_rep* pplay = pInfo->mutable_dice();
	for (uint16 i = 0; i<m_vecRecord.size(); ++i)
	{
		net::dice_play_log* plog = pplay->add_logs();
		tagDiceGameRecord& record = m_vecRecord[i];
	
		plog->set_big_small(record.cbBigSmall);
		plog->set_sum_point(record.cbSumPoints);
		for (uint16 j = 0; j<DICE_COUNT; j++) {
			plog->add_cards(record.cbDiceRecord[j]);
		}
		plog->set_is_waidice(record.cbIsWaidice);
	}
}

void CGameDiceTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo)
{
	net::msg_dice_play_log_rep* pplay = pInfo->mutable_dice();
	net::dice_play_log* plog = pplay->add_logs();
	tagDiceGameRecord& record = m_record;
	plog->set_big_small(record.cbBigSmall);
	plog->set_sum_point(record.cbSumPoints);
	for (uint16 j = 0; j<DICE_COUNT; j++) {
		plog->add_cards(record.cbDiceRecord[j]);
	}
	plog->set_is_waidice(record.cbIsWaidice);
}

bool CGameDiceTable::ActiveWelfareCtrl()
{
	LOG_DEBUG("enter ActiveWelfareCtrl ctrl player count:%d.", m_aw_ctrl_player_list.size());

	//获取当前局活跃福利的控制玩家列表
	GetActiveWelfareCtrlPlayerList();

	vector<tagAWPlayerInfo>::iterator iter = m_aw_ctrl_player_list.begin();
	for (; iter != m_aw_ctrl_player_list.end(); iter++)
	{
		uint32 control_uid = iter->uid;
		uint64 max_win = iter->max_win;

		//判断当前控制玩家是否在配置概率范围内
		uint32 tmp = rand() % 100;
		uint32 probability = iter->probability;
		if (tmp > probability)
		{
			LOG_DEBUG("The current player is not in config rate - control_uid:%d tmp:%d probability:%d", control_uid, tmp, probability)
			break;
		}
		LOG_DEBUG("The current player in config rate - control_uid:%d tmp:%d probability:%d", control_uid, tmp, probability)

		bool ret = ControlActiveWelfarePalyerWin(control_uid, max_win);
		if (ret)
		{
			LOG_DEBUG("search success current player - uid:%d max_win:%d ", control_uid, max_win);
			m_aw_ctrl_uid = control_uid;   //设置当前活跃福利所控的玩家ID
			return true;
		}
		else
		{
			LOG_DEBUG("search fail current player - uid:%d max_win:%d", control_uid, max_win);
			break;
		}
	}
	LOG_DEBUG("the all ActiveWelfareCtrl player is search fail. return false.");
	return false;
}

bool CGameDiceTable::ControlActiveWelfarePalyerWin(uint32 control_uid, int64 max_win)
{
	map<uint32, int64> mpUserLostScore;
	map<uint32, int64> mpUserWinScore;
	map<uint32, int64> mpUserSubWinScore;
	map<uint32, int64> mpUserJackpotScore;

	map<uint32, vector<tag_dice_area_info>>	mpUserAreaInfo;

	uint32 uLoopCount = 1000;
	for (uint32 uIndex = 0; uIndex < uLoopCount; uIndex++)
	{
		mpUserLostScore.clear();
		mpUserWinScore.clear();
		mpUserAreaInfo.clear();
		mpUserSubWinScore.clear();
		mpUserJackpotScore.clear();

		int nMultiple = 0;
		bool bIsHit = false;
		int64 lBankerWinScore = 0;

		m_GameLogic.ComputeDiceResult();

		// 椅子用户
		for (int nChairID = 0; nChairID < GAME_PLAYER; nChairID++)
		{
			CGamePlayer * pPlayer = GetPlayer(nChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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
				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}

				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}

		// 旁观用户
		auto it_looker_calc_jetton = m_mpLookers.begin();
		for (; it_looker_calc_jetton != m_mpLookers.end(); it_looker_calc_jetton++)
		{
			CGamePlayer * pPlayer = it_looker_calc_jetton->second;
			if (pPlayer == NULL)
			{
				continue;
			}
			uint32 dwUserID = pPlayer->GetUID();
			for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; nAreaIndex++)
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

				bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
				int64 lTempWinScore = 0;
				if (bIsHit)
				{
					lTempWinScore = (lUserJettonScore * nMultiple);
					mpUserWinScore[dwUserID] += lTempWinScore;
				}
				else
				{
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
				}
				if (!pPlayer->IsRobot())
				{
					lBankerWinScore -= lTempWinScore;
				}

				tag_dice_area_info dice_area_info;
				dice_area_info.jetton_area = nAreaIndex;
				dice_area_info.jetton_multiple = nMultiple;
				dice_area_info.jetton_score = lUserJettonScore;
				dice_area_info.final_score = lTempWinScore;

				auto it_player = mpUserAreaInfo.find(dwUserID);
				if (it_player != mpUserAreaInfo.end())
				{
					it_player->second.push_back(dice_area_info);
				}
				else
				{
					vector<tag_dice_area_info> vec_dice_area_info;
					vec_dice_area_info.push_back(dice_area_info);
					mpUserAreaInfo.insert(make_pair(dwUserID, vec_dice_area_info));
				}
			}
			mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];
		}
		
		// 如果真实玩家赢金币总分数大于0则可以退出
		if (mpUserWinScore[control_uid] > 0 && mpUserWinScore[control_uid]<= max_win)
		{
			return true;
		}
		else
		{
			m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
		}
	}

	return false;
}

// 设置库存输赢  add by har
bool CGameDiceTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 playerAllWinScore = 0;
	for (int iLoopIndex = 0; iLoopIndex < 1000; ++iLoopIndex) {
		playerAllWinScore = GetBankerAndPlayerWinScore();
		if (CheckStockChange(stockChange, playerAllWinScore, iLoopIndex))
			return true;
		// 重新摇骰子
		m_GameLogic.ShakeRandDice(m_cbTableDice, DICE_COUNT);
	}

	LOG_ERROR("SetStockWinLose fail roomid:%d,tableid:%d,playerAllWinScore:%d,stockChange:%d", GetRoomID(), GetTableID(), playerAllWinScore, stockChange);
	return false;
}

// 获取某个玩家的赢分  add by har
int64 CGameDiceTable::GetSinglePlayerWinScore(CGamePlayer *pPlayer, int64 &lBankerWinScore) {
	if (pPlayer == NULL)
		return 0;
	int nMultiple = 0;
	int64 playerWinScore = 0; // 该玩家赢分
	uint32 dwUserID = pPlayer->GetUID();
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex) {
		map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			continue;
		int64 lUserJettonScore = it_player_jetton->second;
		if (lUserJettonScore == 0)
			continue;

		bool bIsHit = m_GameLogic.CompareHitPrize(nAreaIndex, nMultiple);
		int64 lTempWinScore = 0;
		if (bIsHit)
			playerWinScore += (lUserJettonScore * nMultiple);
		else
			playerWinScore -= lUserJettonScore;
	}

	lBankerWinScore -= playerWinScore;
	if (pPlayer->IsRobot())
		return 0;
	return playerWinScore;
}

// 获取非机器人玩家赢分 add by har
int64 CGameDiceTable::GetBankerAndPlayerWinScore() {
	int64 playerAllWinScore = 0; // 非机器人玩家总赢数
	int64 lBankerWinScore = 0;
	int nMultiple = 0;
	bool bIsHit = false;
	m_GameLogic.ComputeDiceResult();

	// 椅子用户
	for (int nChairID = 0; nChairID < GAME_PLAYER; ++nChairID)
		playerAllWinScore += GetSinglePlayerWinScore(GetPlayer(nChairID), lBankerWinScore);
	// 旁观用户
	for (map<uint32, CGamePlayer*>::iterator it_looker_calc_jetton = m_mpLookers.begin(); it_looker_calc_jetton != m_mpLookers.end(); ++it_looker_calc_jetton)
		playerAllWinScore += GetSinglePlayerWinScore(it_looker_calc_jetton->second, lBankerWinScore);

	if (IsBankerRealPlayer())
		playerAllWinScore += lBankerWinScore;
	return playerAllWinScore;
}