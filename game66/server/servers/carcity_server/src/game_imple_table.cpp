//
// Created on 19/9/25.
//
#include <data_cfg_mgr.h>
#include "pb/msg_define.pb.h"
#include "game_room.h"
#include "player_mgr.h"
#include "game_imple_table.h"
#include "robot_oper_mgr.h"


using namespace svrlib;

namespace {
	const static uint32 s_FreeTime        = 3  * 1000; // 空闲时间
	const static uint32 s_PlaceJettonTime = 15 * 1000; // 下注时间
	const static uint32 s_DispatchTime    = 15 * 1000; // 发结果时间
};

vector<int> g_szCarIndex[JETTON_INDEX_COUNT] = { // 每种车型对应的客户端圆盘索引
	{7, 15, 23, 31}, // 兰博基尼
    {2, 10, 18, 26}, // 法拉利
    {0, 8,  16, 24}, // 宝马
    {1, 9,  17, 25}, // 奔驰
	{6, 14, 22, 30}, // 奥迪
    {5, 13, 21, 29}, // 大众
	{4, 12, 20, 28}, // 丰田
	{3, 11, 19, 27}, // 本田
};

CGameTable* CGameRoom::CreateTable(uint32 tableID) {
    CGameTable *pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 普通
        {
            pTable = new CGameCarcityTable(this, tableID, emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛
        {
            pTable = new CGameCarcityTable(this, tableID, emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房
        {
            pTable = new CGameCarcityTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

// 二八杠游戏桌子
CGameCarcityTable::CGameCarcityTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
 : CGameTable(pRoom,tableID,tableType) {
	//个人下注
	for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
		m_userJettonScore[i].clear();
	m_tagControlPalyer.Init();
}

/// 重载基类函数 ///
bool CGameCarcityTable::CanEnterTable(CGamePlayer* pPlayer){
	if (pPlayer->GetTable() != NULL)
		return false;
	// 限额进入
	if (IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin())
		return false;
	return true;
}

bool CGameCarcityTable::CanLeaveTable(CGamePlayer* pPlayer) {
	if (IsSetJetton(pPlayer->GetUID()))
		return false;
	return true;
}

bool CGameCarcityTable::IsFullTable() {
	if (m_mpLookers.size() >= 200)
		return true;
	return false;
}

void CGameCarcityTable::GetTableFaceInfo(net::table_face_info* pInfo) {
	net::carcity_table_info* pTableInfo = pInfo->mutable_carcity();
	pTableInfo->set_tableid(GetTableID());
	pTableInfo->set_tablename(m_conf.tableName);
	if (m_conf.passwd.length() > 1)
		pTableInfo->set_is_passwd(1);
	else
		pTableInfo->set_is_passwd(0);
	pTableInfo->set_hostname(m_conf.hostName);
	pTableInfo->set_basescore(m_conf.baseScore);
	pTableInfo->set_consume(m_conf.consume);
	pTableInfo->set_entermin(m_conf.enterMin);
	pTableInfo->set_duetime(m_conf.dueTime);
	pTableInfo->set_feetype(m_conf.feeType);
	pTableInfo->set_feevalue(m_conf.feeValue);
	pTableInfo->set_card_time(s_PlaceJettonTime);
	pTableInfo->set_table_state(GetGameState());

	LOG_DEBUG("roomid:%d,tableid:%d,tableName=%s", GetRoomID(), GetTableID(), m_conf.tableName);
}

// 初始化，桌子对象创建后执行的第一个函数
bool CGameCarcityTable::Init() {
	if (m_pHostRoom == NULL)
		return false;

	SetGameState(TABLE_STATE_CARCITY_FREE);
	
	ReAnalysisParam();
	m_bInitTableSuccess = true;
	CRobotOperMgr::Instance().PushTable(this);
	return true;
}

bool CGameCarcityTable::ReAnalysisParam() {
	LOG_DEBUG("reader json parse success - roomid:%d,tableid:%d", GetRoomID(), GetTableID());

	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue)) {
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", GetRoomID(), param.c_str());
		return true;
	}

	for (int i = AREA_LBGN_MEN; i < JETTON_INDEX_COUNT; ++i) {
		string strPro = CStringUtility::FormatToString("pr%d", i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral()) {
			int pro = jvalue[strPro.c_str()].asInt();
			if (pro < 0) {
				LOG_ERROR("reader json parse error - pro<0 roomid:%d,i:%d,pro:%d", GetRoomID(), i, pro);
			} else
			    m_iArrDispatchCardPro[i] = pro;
		}
		strPro = CStringUtility::FormatToString("mp%d", i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral()) {
			int multiple = jvalue[strPro.c_str()].asInt();
			if (multiple < 1) {
				LOG_ERROR("reader json parse error - multiple<1 roomid:%d,i:%d,multiple:%d", GetRoomID(), i, multiple);
			} else
			    m_winMultiple[i] = multiple;
		}
	}

	LOG_DEBUG("reader json parse success - roomid:%d,tableid:%d,m_winMultiple:%d-%d-%d-%d-%d-%d-%d-%d,m_iArrDispatchCardPro:%d-%d-%d-%d-%d-%d-%d-%d",
		GetRoomID(), GetTableID(), m_winMultiple[0], m_winMultiple[1], m_winMultiple[2], m_winMultiple[3], m_winMultiple[4], m_winMultiple[5], m_winMultiple[6], m_winMultiple[7],
		m_iArrDispatchCardPro[0], m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3], m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5], m_iArrDispatchCardPro[6], m_iArrDispatchCardPro[7]);
	return true;
}

//复位桌子
void CGameCarcityTable::ResetTable() {
	ResetGameData();
}

void CGameCarcityTable::OnTimeTick() {
	OnTableTick();

	uint8 tableState = GetGameState();
	if (m_coolLogic.isTimeOut()) {
		switch (tableState) {
		case TABLE_STATE_CARCITY_FREE:           // 空闲
		{
		}break;
		case TABLE_STATE_CARCITY_PLACE_JETTON:   // 下注时间
		{
			SetGameState(TABLE_STATE_CARCITY_FREE);
			m_coolLogic.beginCooling(s_DispatchTime);

			DispatchTableCard();

			m_bIsRobotAlreadyJetton = false;
			m_RobotPlaceJetton.clear();

			OnGameEnd(INVALID_CHAIR, GER_NORMAL);

			m_brc_table_status = emTABLE_STATUS_END;
			m_brc_table_status_time = 0;

			//同步刷新百人场控制界面的桌子状态信息
			OnBrcControlFlushTableStatus();

		}break;
		default:
			break;
		}
	}

	if (tableState == TABLE_STATE_CARCITY_PLACE_JETTON && m_coolLogic.getPassTick() > 0)
		OnRobotJetton();
}

void CGameCarcityTable::OnRobotTick() {
	uint8 tableState = GetGameState();
	if (tableState == net::TABLE_STATE_CARCITY_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
		OnRobotPlaceJetton();

	if (m_coolLogic.isTimeOut())
	{
		switch (tableState)
		{
		case TABLE_STATE_CARCITY_FREE:           // 空闲
		{
			if (m_bInitTableSuccess) {
				if (OnGameStart()) {
					InitBlingLog();
					InitChessID();
					CalculateDeity();
				}

				m_brc_table_status = emTABLE_STATUS_START;
				m_brc_table_status_time = s_PlaceJettonTime;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();
			} else
				m_coolLogic.beginCooling(s_FreeTime);
		}break;
		default:
			break;
		}
	}
}

//游戏消息
int CGameCarcityTable::OnGameMessage(CGamePlayer *pPlayer, uint16 cmdID, const uint8 *pkt_buf, uint16 buf_len) {
	if (pPlayer == NULL) {
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,pPlayer:%p,cmdID:%d", GetRoomID(), GetTableID(), pPlayer, cmdID);
		return 0;
	}
	if (!pPlayer->IsRobot())
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,uid:%d,cmdID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), cmdID);

	switch (cmdID)
	{
	case net::C2S_MSG_CARCITY_PLACE_JETTON:  // 用户加注
	{
		if (GetGameState() != TABLE_STATE_CARCITY_PLACE_JETTON) {
			LOG_DEBUG("not jetton state can't jetton");
			return 0;
		}
		net::msg_carcity_place_jetton_req msg;
		PARSE_MSG_FROM_ARRAY(msg);
		return OnUserPlaceJetton(pPlayer, msg.jetton_area(), msg.jetton_score());
	}break;
	case net::C2S_MSG_CARCITY_CONTINUOUS_PRESSURE_REQ: // 续押
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

// 用户断线或重连
bool CGameCarcityTable::OnActionUserNetState(CGamePlayer* pPlayer, bool bConnected, bool isJoin) {
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bConnected:%d,isJoin:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), bConnected, isJoin);
	if (bConnected) { //断线重连
		if (isJoin) {
			pPlayer->SetPlayDisconnect(false);
			PlayerSetAuto(pPlayer, 0);
			SendTableInfoToClient(pPlayer);
			SendSeatInfoToClient(pPlayer);
			if (m_mpLookers.find(pPlayer->GetUID()) != m_mpLookers.end())
				NotifyPlayerJoin(pPlayer, true);
			SendLookerListToClient(pPlayer);
			SendGameScene(pPlayer);
			SendPlayLog(pPlayer);
		}
	} else
		pPlayer->SetPlayDisconnect(true);
	return true;
}

//用户坐下
bool CGameCarcityTable::OnActionUserSitDown(uint16 wChairID, CGamePlayer* pPlayer) {
	SendSeatInfoToClient();
	return true;
}
//用户起立
bool CGameCarcityTable::OnActionUserStandUp(uint16 wChairID, CGamePlayer* pPlayer) {
	SendSeatInfoToClient();
	return true;
}

// 游戏开始
bool CGameCarcityTable::OnGameStart() {
	if (m_pHostRoom == NULL)
		return false;
	if (!m_pHostRoom->GetCanGameStart())
		return false;
	
	LOG_DEBUG("game_start - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());

	ClearTurnGameData();
	SetGameState(TABLE_STATE_CARCITY_PLACE_JETTON);
	m_coolLogic.beginCooling(s_PlaceJettonTime);

	m_curr_bet_user.clear();

	net::msg_carcity_start_rep gameStart;
	gameStart.set_time_leave(m_coolLogic.getCoolTick());
	SendMsgToAll(&gameStart, net::S2C_MSG_CARCITY_START);
	OnTableGameStart();
	return true;
}

//游戏结束
bool CGameCarcityTable::OnGameEnd(uint16 chairID, uint8 reason) {
	LOG_DEBUG("game end:table:%d", GetTableID());
	switch (reason)
	{
	case GER_NORMAL:		//常规结束
	{
		AddPlayerToBlingLog();

		for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
			for (map<uint32, int64>::iterator iter = m_userJettonScore[i].begin(); iter != m_userJettonScore[i].end(); ++iter)
				if (iter->second != 0)
					OnAddPlayerJetton(iter->first, iter->second);

		//计算分数
		int64 lBankerWinScore = CalculateScore();
		int64 playerAllWinScore = 0; // 玩家总赢分,奔驰宝马玩家不能上庄
		m_winIndex = g_szCarIndex[m_winCardType][g_RandGen.GetRandi(0, 3)];
		WriteGameInfoLog();
		//结束消息
		net::msg_carcity_game_end msg;
		msg.set_time_leave(m_coolLogic.getCoolTick());
		for (int i = 0; i < JETTON_INDEX_COUNT; ++i)
			if (i == m_winCardType)
			    msg.add_win_multiple(m_winMultiple[i]);
			else
				msg.add_win_multiple(-m_winMultiple[i]);

		int startIndex = g_RandGen.GetRandi(0, 31);
		msg.set_start_index(startIndex);
		msg.set_win_index(m_winIndex);

		LOG_DEBUG("roomid:%d,tableid:%d,lBankerWinScore:%lld,m_winCardType:%d,startIndex:%d,m_winIndex:%d",
			GetRoomID(), GetTableID(), lBankerWinScore, m_winCardType, startIndex, m_winIndex);

		//发送旁观者积分
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL) {
				LOG_DEBUG("send_player_game_end - roomid:%d,tableid:%d", GetRoomID(), GetTableID());
				continue;
			}
			int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
			lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
			m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
			msg.set_user_score(lUserScoreFree);

			if (!pPlayer->IsRobot())
			    LOG_DEBUG("send_player_game_end - roomid:%d,tableid:%d,uid:%d,IsRobot:%d,score:%lld",
				    GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsRobot(), lUserScoreFree);

			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_GAME_END);

			//精准控制统计
			OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);

			if (!pPlayer->IsRobot())
				playerAllWinScore += lUserScoreFree;
		}

		//更新活跃福利数据            
		int64 curr_win = m_mpUserWinScore[m_aw_ctrl_uid];
		UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);

		LOG_DEBUG("OnGameEnd2 - roomid:%d,tableid:%d,playerAllWinScore:%lld", GetRoomID(), GetTableID(), playerAllWinScore);
		m_pHostRoom->UpdateStock(this, playerAllWinScore);

		//同步所有玩家数据到控端
		OnBrcFlushSendAllPlayerInfo();

		//个人下注
		for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
			m_userJettonScore[i].clear();
		SaveBlingLog();
		OnTableGameEnd();

		return true;
	}break;
	case GER_DISMISS:		//游戏解散
	{
		LOG_ERROR("强制游戏解散 roomid:%d,tableid:%d", GetRoomID(), GetTableID());
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

// 玩家进入或离开
void CGameCarcityTable::OnPlayerJoin(bool isJoin, uint16 chairID, CGamePlayer* pPlayer) {
	uint32 uid = 0;
	if (pPlayer != NULL)
		uid = pPlayer->GetUID();
	LOG_DEBUG("player join - uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld", uid, chairID, isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);
	CGameTable::OnPlayerJoin(isJoin, chairID, pPlayer);
	if (isJoin) {
		SendGameScene(pPlayer);
		SendPlayLog(pPlayer);
	} else
		for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
			m_userJettonScore[i].erase(pPlayer->GetUID());

	//刷新控制界面的玩家数据
	if (!pPlayer->IsRobot())
		OnBrcFlushSendAllPlayerInfo();
}

// 发送场景信息(断线重连)
void CGameCarcityTable::SendGameScene(CGamePlayer* pPlayer) {
	LOG_DEBUG("send game scene - roomid:%d,tableid:%d,uid:%d,GameState:%d,lCurScore:%lld",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetGameState(), GetPlayerCurScore(pPlayer));
	switch (GetGameState())
	{
	case TABLE_STATE_CARCITY_FREE:          // 空闲状态
	case TABLE_STATE_CARCITY_PLACE_JETTON:  // 游戏状态
	{
		net::msg_carcity_game_info_play_rep msg;
		for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i) {
			msg.add_all_jetton_score(m_allJettonScore[i]);
			msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
		}
		msg.set_time_leave(m_coolLogic.getCoolTick());
		msg.set_game_status(GetGameState());
		if (GetGameState() == TABLE_STATE_CARCITY_FREE) {
			msg.set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
			msg.set_win_index(m_winIndex);
			for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
				if (i == m_winCardType)
					msg.add_win_multiple(m_winMultiple[i]);
				else
					msg.add_win_multiple(-m_winMultiple[i]);

		}
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_GAME_PLAY_INFO);

		//刷新所有控制界面信息协议---用于断线重连的处理
		if (pPlayer->GetCtrlFlag()) {
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

int64 CGameCarcityTable::CalcPlayerInfo(uint32 uid, int64 winScore, int64 OnlywinScore, bool isBanker) {
	int64 userJettonScore = 0;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex) {
		map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[nAreaIndex].find(uid);
		if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
			continue;
		userJettonScore += it_player_jetton->second;
	}

	int64 fee = GetBrcFee(uid, OnlywinScore, true);
	winScore += fee;
	CalcPlayerGameInfoForBrc(uid, winScore, 0, true, false, userJettonScore);

	LOG_DEBUG("report to lobby:%d winScore:%lld OnlywinScore:%lld fee:%lld", uid, winScore, OnlywinScore, fee);
	return fee;
}

// 重置每局游戏数据
void CGameCarcityTable::ResetGameData() {
	//总下注数
	ZeroMemory(m_allJettonScore, sizeof(m_allJettonScore));
	ZeroMemory(m_playerJettonScore, sizeof(m_playerJettonScore));

	m_curr_bet_user.clear();

	//个人下注
	for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
		m_userJettonScore[i].clear();

	//玩家成绩	
	m_mpUserWinScore.clear();

	m_winCardType = AREA_MAX; // 开奖车型
}

void CGameCarcityTable::ClearTurnGameData() {
	//总下注数
	ZeroMemory(m_allJettonScore, sizeof(m_allJettonScore));
	ZeroMemory(m_playerJettonScore, sizeof(m_playerJettonScore));

	//个人下注
	for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i)
		m_userJettonScore[i].clear();

	//玩家成绩	
	m_mpUserWinScore.clear();
}

// 写入出牌log
void CGameCarcityTable::WriteOutCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount, int32 mulip) {
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["m"] = mulip;
	for (uint32 i = 0; i < cardCount; ++i) {
		logValue["c"].append(cardData[i]);
	}
	m_operLog["card"].append(logValue);
}

// 写入加注log
void CGameCarcityTable::WriteAddScoreLog(uint32 uid, uint8 area, int64 score) {
	if (score == 0)
		return;
	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["p"] = area;
	logValue["s"] = score;
	m_operLog["op"].append(logValue);
}

// 写入最大牌型
void CGameCarcityTable::WriteMaxCardType(uint32 uid, uint8 cardType) {
	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["mt"] = cardType;
	m_operLog["maxcard"].append(logValue);
}

void CGameCarcityTable::WriteGameInfoLog() {
	// 旁观用户
	for (map<uint32, CGamePlayer*>::iterator it_looker = m_mpLookers.begin(); it_looker != m_mpLookers.end(); ++it_looker) {
		CGamePlayer *pPlayer = it_looker->second;
		if (pPlayer == NULL)
			continue;
		uint32 dwUserID = pPlayer->GetUID();
		int64 lUserJettonScore = 0;
		Json::Value logValue;
		logValue["uid"] = dwUserID;
		for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex) {
			map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
				continue;
			int64 lIndexJettonScore = it_player_jetton->second;
			lUserJettonScore += lIndexJettonScore;
			string strindex = CStringUtility::FormatToString("score_%d", nAreaIndex);
			logValue[strindex.c_str()] = lIndexJettonScore;
		}
		if (lUserJettonScore > 0) {
			logValue["score_a"] = lUserJettonScore;
			m_operLog["userBetInfo"].append(logValue);
		}
	}

	Json::Value logValueMultiple;
	for (int i = 0; i < JETTON_INDEX_COUNT; ++i) {
		if (i == m_winCardType)
		    logValueMultiple["m"].append(m_winMultiple[i]);
		else
			logValueMultiple["m"].append(-m_winMultiple[i]);
		//logValueMultiple["w"].append(m_winIndex[i]);
	}
	logValueMultiple["w"].append(m_winIndex);
	m_operLog["ji"] = logValueMultiple;
}

//加注事件
bool CGameCarcityTable::OnUserPlaceJetton(CGamePlayer* pPlayer, uint8 cbJettonArea, int64 lJettonScore) {
	if (!pPlayer->IsRobot())
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,uid:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetGameState(), cbJettonArea, lJettonScore);

	if (GetGameState() != net::TABLE_STATE_CARCITY_PLACE_JETTON) {
		net::msg_carcity_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLACE_JETTON_REP);
		return true;
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	if (!TableJettonLimmit(pPlayer, lJettonScore, lJettonCount)) {
		if (!pPlayer->IsRobot())
			LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());

		net::msg_carcity_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLACE_JETTON_REP);
		return true;
	}

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//合法校验
	if (lUserScore < lJettonCount + lJettonScore) {
		if (!pPlayer->IsRobot())
			LOG_DEBUG("jetton more - roomid:%d,tableid:%d,uid:%d,cbJettonArea:%d,lJettonScore:%lld,lJettonCount:%lld,lUserScore:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), cbJettonArea, lJettonScore, lJettonCount, lUserScore);

		net::msg_carcity_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLACE_JETTON_REP);

		return true;
	}

	//保存下注
	m_allJettonScore[cbJettonArea] += lJettonScore;
	if (!pPlayer->IsRobot()) {
		m_playerJettonScore[cbJettonArea] += lJettonScore;
		m_curr_bet_user.insert(pPlayer->GetUID());
	}
	m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;

	RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);
	AddUserBlingLog(pPlayer);
	net::msg_carcity_place_jetton_rep msg;
	msg.set_jetton_area(cbJettonArea);
	msg.set_jetton_score(lJettonScore);
	msg.set_result(net::RESULT_CODE_SUCCESS);
	//发送消息
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLACE_JETTON_REP);

	net::msg_carcity_place_jetton_broadcast broad;
	broad.set_uid(pPlayer->GetUID());
	broad.set_jetton_area(cbJettonArea);
	broad.set_jetton_score(lJettonScore);
	broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

	SendMsgToAll(&broad, net::S2C_MSG_CARCITY_PLACE_JETTON_BROADCAST);

	//刷新百人场控制界面的下注信息
	OnBrcControlBetDeal(pPlayer);

	return true;
}

// 玩家续押
bool CGameCarcityTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_CARCITY_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID()) {
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_CARCITY_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//效验参数
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); ++i) {
		net::bairen_jetton_info info = msg.info(i);
		if (info.score() <= 0 || info.area() >= JETTON_INDEX_COUNT) {
			LOG_DEBUG("error_pressu - uid:%d,i:%d,area:%d,score:%lld", pPlayer->GetUID(), i, info.area(), info.score());
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_CARCITY_CONTINUOUS_PRESSURE_REP);
			return false;
		}
		lTotailScore += info.score();
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	int64 lUserMaxJettonScore = lJettonCount + lTotailScore + GetJettonMin();
	//合法校验
	if (GetJettonMin() > GetEnterScore(pPlayer) || (lUserScore < lJettonCount + lTotailScore)) {
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_CARCITY_CONTINUOUS_PRESSURE_REP);
		LOG_DEBUG("error_pressu - uid:%d,lUserScore:%lld,lUserMaxJettonScore:%lld, lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserScore, lUserMaxJettonScore, lJettonCount, lTotailScore, GetJettonMin());
		return false;
	}

	for (int i = 0; i < msg.info_size(); ++i) {
		net::bairen_jetton_info info = msg.info(i);
		m_allJettonScore[info.area()] += info.score();
		if (!pPlayer->IsRobot()) {
			m_playerJettonScore[info.area()] += info.score();
			m_curr_bet_user.insert(pPlayer->GetUID());
		}
		m_userJettonScore[info.area()][pPlayer->GetUID()] += info.score();
		RecordPlayerBaiRenJettonInfo(pPlayer, info.area(), info.score());

		uint8 cbJettonArea = info.area();
		int64 lJettonScore = info.score();

		net::msg_carcity_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLACE_JETTON_REP);

		net::msg_carcity_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_CARCITY_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_CARCITY_CONTINUOUS_PRESSURE_REP);

	//刷新百人场控制界面的下注信息
	OnBrcControlBetDeal(pPlayer);
	return true;
}

// 获取非机器人玩家赢金币数
int64 CGameCarcityTable::GetPlayerWinScore(int winCarType) {
	int64 playerAllWinScore = 0; // 非机器人玩家总赢数

	//计算旁观者积分
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL)
			continue;
		if (pPlayer->IsRobot())
			continue;
		playerAllWinScore += GetSinglePlayerWinScore(pPlayer->GetUID(), winCarType);
	}

	return playerAllWinScore;
}

// 获取玩家(非庄家)赢金币数
int64 CGameCarcityTable::GetSinglePlayerWinScore(uint32 uid, int winCarType) {
	int64 playerScoreWin = 0;
	for (int wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
		map<uint32, int64> umpData = m_userJettonScore[wAreaIndex];
		map<uint32, int64>::iterator it = umpData.find(uid);
		if (it == umpData.end())
		    continue;
		int64 userJettonScore = it->second;
		if (userJettonScore == 0)
			continue;
		if (wAreaIndex == winCarType)
			playerScoreWin += userJettonScore * (m_winMultiple[winCarType] - 1);
		else
			playerScoreWin -= userJettonScore;
	}
	return playerScoreWin;
}

bool CGameCarcityTable::SetControlPlayerWinLose(uint32 control_uid, bool isWin) {
	int winCardType = m_winCardType;
	int64 playerWinScore = 0;
	int i = 0;
	// 循环，直到找到满足条件的牌组合
	while (true) {
		playerWinScore = GetSinglePlayerWinScore(control_uid, winCardType);
		if ((isWin && playerWinScore > -1) || (!isWin && playerWinScore < 1)) {
			m_winCardType = winCardType;
			LOG_DEBUG("suc roomid:%d,tableid:%d,isWin:%d,i:%d,playerWinScore:%lld,winCardType:%d",
				GetRoomID(), GetTableID(), isWin, i, playerWinScore, winCardType);
			return true;
		}
		if (++i > 999)
			break;
		winCardType = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
	}

	LOG_ERROR("fail roomid:%d,tableid:%d,isWin:%d,playerWinScore:%lld, winCardType:%d, m_winCardType:%d",
		GetRoomID(), GetTableID(), isWin, playerWinScore, winCardType, m_winCardType);
	return false;
}

// 设置库存输赢
bool CGameCarcityTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;
	int winCardType = m_winCardType;
	int64 playerAllWinScore = 0;
	int i = 0;
	// 循环，直到找到满足条件的牌组合
	while (true) {
		playerAllWinScore = GetPlayerWinScore(winCardType);
		if (CheckStockChange(stockChange, playerAllWinScore, i)) {
			m_winCardType = winCardType;
			LOG_DEBUG("SetStockWinLose suc  roomid:%d,tableid:%d,stockChange:%lld,i:%d,playerAllWinScore:%d,m_winCardType:%d",
				GetRoomID(), GetTableID(), stockChange, i, playerAllWinScore, m_winCardType);
			return true;
		}
		if (++i > 999)
			break;
		// 重新抽签
		winCardType = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
	}

	LOG_ERROR("SetStockWinLose fail! roomid:%d,tableid:%d,playerAllWinScore:%lld,stockChange:%lld,IsBankerRealPlayer:%d", GetRoomID(), GetTableID(), playerAllWinScore, stockChange, IsBankerRealPlayer());
	return false;
}

bool CGameCarcityTable::DosWelfareCtrl() {
	return true;
}

// 非福利控制
void CGameCarcityTable::NotWelfareCtrl() {
	bool bIsFalgControl = false;
	bool bControlPlayerIsJetton = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid != 0 && game_count > 0 && control_type != GAME_CONTROL_CANCEL) {
		for (int i = 0; i < JETTON_INDEX_COUNT; ++i) {
			map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[i].find(control_uid);
			if (it_player_jetton == m_userJettonScore[i].end())
				continue;
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore > 0)
				bControlPlayerIsJetton = true;
		}

		if (bControlPlayerIsJetton && game_count > 0 && control_type != GAME_CONTROL_CANCEL) {
			if (control_type == GAME_CONTROL_WIN)
				bIsFalgControl = SetControlPlayerWinLose(control_uid, true);
			if (control_type == GAME_CONTROL_LOST)
				bIsFalgControl = SetControlPlayerWinLose(control_uid, false);
			if (bIsFalgControl && m_tagControlPalyer.count > 0) {
				if (m_pHostRoom != NULL)
					m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
			}
		}
	}

	// 库存控制
	bool isStockCtrl = false;
	if (!bIsFalgControl)
		isStockCtrl = SetStockWinLose();

	LOG_DEBUG("NotWelfareCtrl - roomid:%d,tableid:%d,control_uid:%d,control_type:%d,game_count:%d,m_winCardType:%d,bIsFalgControl:%d,isStockCtrl:%d",
		GetRoomID(), GetTableID(), control_uid, control_type, game_count, m_winCardType, bIsFalgControl, isStockCtrl);
}

//发送扑克
bool CGameCarcityTable::DispatchTableCard() {
	// 根据概率抽签
	int randIndex = g_RandGen.GetRandi(1, PRO_DENO_10000);
	int count = 0;
	int i = AREA_LBGN_MEN;
	m_winCardType = AREA_HONDA_MEN;
	for (; i < JETTON_INDEX_COUNT; ++i) {
		count += m_iArrDispatchCardPro[i];
		if (randIndex < count) {
			m_winCardType = i;
			break;
		}
	}
	bool bAreaCtrl = OnBrcAreaControl();
	bool isAllRobotOrPlayerJetton = IsAllRobotOrPlayerJetton();
	SetIsAllRobotOrPlayerJetton(isAllRobotOrPlayerJetton);
	LOG_DEBUG("1 - roomid:%d,tableid:%d,randIndex:%d,m_winCardType:%d,count:%d,i:%d,bAreaCtrl:%d,GetIsAllRobotOrPlayerJetton:%d",
		GetRoomID(), GetTableID(), randIndex, m_winCardType, count, i, bAreaCtrl, isAllRobotOrPlayerJetton);

	if (bAreaCtrl)
	    return true;
 
	NotWelfareCtrl();

	LOG_DEBUG("2 - roomid:%d,tableid:%d,bAreaCtrl:%d,ChessWelfare:%d", GetRoomID(), GetTableID(), bAreaCtrl, GetChessWelfare());

	return true;
}

//发送游戏记录
void CGameCarcityTable::SendPlayLog(CGamePlayer *pPlayer) {
	uint32 uid = 0;
	bool bIsRobot = true;
	if (pPlayer != NULL) {
		uid = pPlayer->GetUID();
		bIsRobot = pPlayer->IsRobot();
	}
	net::msg_carcity_play_log_rep msg;
	for (uint16 i = 0; i < m_vecRecord.size(); ++i) {
		net::carcity_play_log *plog = msg.add_logs();
		stCarcityGameRecord &record = m_vecRecord[i];
		std::string strWinIndex;
		for (uint16 j = 0; j < JETTON_INDEX_COUNT; ++j) {
			plog->add_seats_win(record.wins[j]);
			strWinIndex += CStringUtility::FormatToString("%d ", record.wins[j]);
		}
		if (!bIsRobot)
			LOG_DEBUG("send_deail - size:%d,uid:%d,strWinIndex:%s", msg.logs_size(), uid, strWinIndex.c_str());

	}

	if (!bIsRobot)
		LOG_DEBUG("发送牌局记录 size:%d,uid:%d", msg.logs_size(), uid);

	if (pPlayer != NULL)
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CARCITY_PLAY_LOG);
	else
		SendMsgToAll(&msg, net::S2C_MSG_CARCITY_PLAY_LOG);
}

//最大下注
int64 CGameCarcityTable::GetUserMaxJetton(CGamePlayer* pPlayer/*, uint8 cbJettonArea*/) {
	int iTimer = 3;
	//已下注额
	int64 lNowJetton = 0;
	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
		lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	//庄家金币
	int64 lBankerScore = 9223372036854775807;

	for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex)
		lBankerScore -= m_allJettonScore[nAreaIndex] * iTimer;

	//个人限制
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton * iTimer) / iTimer;

	//庄家限制
	lMeMaxScore = min(lMeMaxScore, lBankerScore / iTimer);

	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	return (lMeMaxScore);
}

bool CGameCarcityTable::IsSetJetton(uint32 uid) {
	for (int i = 0; i < JETTON_INDEX_COUNT; ++i) {
		map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[i].find(uid);
		if (it_player_jetton == m_userJettonScore[i].end())
			continue;
		int64 lUserJettonScore = it_player_jetton->second;
		if (lUserJettonScore > 0)
			return true;
	}

	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); ++i)
		if (uid == m_RobotPlaceJetton[i].uid)
			return true;
	return false;
}

//计算得分
int64 CGameCarcityTable::CalculateScore() {
	map<uint32, int64> mpUserLostScore;
	mpUserLostScore.clear();
	m_mpUserWinScore.clear();
	m_mpWinScoreForFee.clear();

	int64 lBankerWinScore = 0;

	uint16 iLostIndex = 0, iWinIndex = 0;

	for (int i = 0; i < JETTON_INDEX_COUNT; ++i)
		if (i == m_winCardType)
			m_record.wins[i] = TRUE;
		else
			m_record.wins[i] = FALSE;

	m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() >= 72)
		m_vecRecord.erase(m_vecRecord.begin());

	SendGameEndLogInfo();

	// 旁观用户
	for (map<uint32, CGamePlayer*>::iterator it_looker = m_mpLookers.begin(); it_looker != m_mpLookers.end(); ++it_looker) {
		CGamePlayer *pPlayer = it_looker->second;
		if (pPlayer == NULL)
			continue;
		uint32 dwUserID = pPlayer->GetUID();

		for (int nAreaIndex = 0; nAreaIndex < JETTON_INDEX_COUNT; ++nAreaIndex) {
			map<uint32, int64>::iterator it_player_jetton = m_userJettonScore[nAreaIndex].find(dwUserID);
			if (it_player_jetton == m_userJettonScore[nAreaIndex].end())
				continue;
			int64 lUserJettonScore = it_player_jetton->second;
			if (lUserJettonScore == 0)
				continue;

			int64 lTempWinScore = 0;
			if (nAreaIndex == m_winCardType) {
				lTempWinScore = lUserJettonScore * (m_winMultiple[nAreaIndex] - 1);
				m_mpUserWinScore[dwUserID] += lTempWinScore;
				lBankerWinScore -= lTempWinScore;
			} else {
					lTempWinScore = -lUserJettonScore;
					mpUserLostScore[dwUserID] -= lUserJettonScore;
					lBankerWinScore += lUserJettonScore;
			}
		}
		m_mpWinScoreForFee[dwUserID] = m_mpUserWinScore[dwUserID];
		m_mpUserWinScore[dwUserID] += mpUserLostScore[dwUserID];

		if (!pPlayer->IsRobot())
			LOG_DEBUG("score result looker - uid:%d,mroomid,tableid:%d,_mpUserWinScore:%lld", dwUserID, GetRoomID(), GetTableID(), m_mpUserWinScore[dwUserID]);
	}
	return lBankerWinScore;
}

// 申请上庄条件
int64 CGameCarcityTable::GetApplyBankerCondition() {
	return GetBaseScore();
}

int64 CGameCarcityTable::GetApplyBankerConditionLimit() {
	return GetBaseScore() * 20;
}

// 机器人押注准备实现
void CGameCarcityTable::OnRobotJettonDeal(CGamePlayer *pPlayer, bool isChairPlayer) {
	if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		return;

	uint8 cbJettonArea = GetRobotJettonArea();
	uint8 cbOldJettonArea = cbJettonArea;
	int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
	if (lUserRealJetton == 0)
		return;

	int iJettonCount = g_RandGen.GetRandi(1, 6);
	cbJettonArea = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);

	int iJettonTypeCount = g_RandGen.GetRandi(1, 2);
	int iJettonStartCount = g_RandGen.GetRandi(2, 3);
	int64 lOldRealJetton = -1;
	int64 iJettonOldTime = -1;

	bool bIsContinuouslyJetton = false;
	int iPreRatio = g_RandGen.GetRandi(5, 10);
	if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100)) {
		bIsContinuouslyJetton = true;
		if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			iJettonCount = g_RandGen.GetRandi(5, 18);
	}
	for (int iIndex = 0; iIndex < iJettonCount; ++iIndex) {
		if (bIsContinuouslyJetton == false) {
			cbJettonArea = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
			lUserRealJetton = GetRobotJettonScore(pPlayer, 0);
			if (lOldRealJetton == -1)
				lOldRealJetton = lUserRealJetton;
			if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				lUserRealJetton = lOldRealJetton;
			if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex) {
				lUserRealJetton = lUserRealJetton;
				lOldRealJetton = lUserRealJetton;
			} else
				lUserRealJetton = lOldRealJetton;
		}
		if (lUserRealJetton == 0)
			continue;

		tagRobotPlaceJetton robotPlaceJetton;
		robotPlaceJetton.uid = pPlayer->GetUID();

		int64 uMaxDelayTime = s_PlaceJettonTime;
		robotPlaceJetton.time = g_RandGen.GetRandi(100, uMaxDelayTime - 500);
		iJettonOldTime = robotPlaceJetton.time;

		if (bIsContinuouslyJetton) {
			robotPlaceJetton.time = iJettonOldTime + 100;
			iJettonOldTime = robotPlaceJetton.time;
		}

		if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			continue;

		robotPlaceJetton.area = cbJettonArea;
		robotPlaceJetton.jetton = lUserRealJetton;
		robotPlaceJetton.bflag = false;
		m_RobotPlaceJetton.push_back(robotPlaceJetton);
	}
}

// 观众机器人押注准备
void CGameCarcityTable::OnRobotJetton() {
	if (m_bIsRobotAlreadyJetton)
		return;
	m_bIsRobotAlreadyJetton = true;
	m_RobotPlaceJetton.clear();
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
			continue;
		OnRobotJettonDeal(it->second, false);
	}
	LOG_DEBUG("robot_jetton - roomid:%d,tableid:%d,m_chairRobotPlaceJetton.size:%lld,m_mpLookers.size:%lld",
		GetRoomID(), GetTableID(), m_RobotPlaceJetton.size(), m_mpLookers.size());
}

void CGameCarcityTable::OnRobotPlaceJetton() {
	if (m_RobotPlaceJetton.empty())
		return;

	int64 passtick = m_coolLogic.getPassTick(); // 下注开始后经历的时间(毫秒)
	int delNum = 0; // 要删除的下注项数量
	std::set<uint32> usRobotPlaceJetton; // 每个玩家每次定时器tick只能下注一次
	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); ++i) {
		tagRobotPlaceJetton &robotPlaceJetton = m_RobotPlaceJetton[i];
		if (robotPlaceJetton.bflag) {
			++delNum;
			continue;
		}
		CGamePlayer *pPlayer = static_cast<CGamePlayer *>(CPlayerMgr::Instance().GetPlayer(robotPlaceJetton.uid));
		if (pPlayer == NULL)
			continue;

		if (passtick > robotPlaceJetton.time && s_PlaceJettonTime - passtick > 500 &&
			    usRobotPlaceJetton.find(robotPlaceJetton.uid) == usRobotPlaceJetton.end()) {
			if (IsInTableRobot(robotPlaceJetton.uid, pPlayer)) {
				robotPlaceJetton.bflag = OnUserPlaceJetton(pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
				if (robotPlaceJetton.bflag)
					++delNum;
			}
			usRobotPlaceJetton.insert(robotPlaceJetton.uid);
		}
	}

	uint32 j = 0;
	for (int i = 0; i < delNum; ++i) {
		for (vector<tagRobotPlaceJetton>::iterator iter_begin = m_RobotPlaceJetton.begin() + j; j < m_RobotPlaceJetton.size(); ++iter_begin, ++j)
			if (iter_begin->bflag) {
				iter_swap(iter_begin, m_RobotPlaceJetton.end() - 1);
				m_RobotPlaceJetton.pop_back();
				break;
			}
		if (j >= m_RobotPlaceJetton.size())
			break;
	}
}

uint8 CGameCarcityTable::GetRobotJettonArea() {
	uint8 cbJettonArea = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
	m_cbFrontRobotJettonArea = cbJettonArea;
	return cbJettonArea;
}

bool CGameCarcityTable::IsInTableRobot(uint32 uid, CGamePlayer *pPlayer) {
	map<uint32, CGamePlayer*>::iterator iter_player = m_mpLookers.find(uid);
	if (iter_player != m_mpLookers.end())
		if (pPlayer != NULL && pPlayer == iter_player->second && pPlayer->GetUID() == iter_player->first)
			return true;

	return false;
}

int64 CGameCarcityTable::GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area) {
	int64 lUserRealJetton = 100;
	int64 lUserMinJetton = 100;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer/*, area*/);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);
	
	if (lUserCurJetton < 2000)
		lUserRealJetton = 0;
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000) {
		if (g_RandGen.RandRatio(35, PRO_DENO_100))
			lUserRealJetton = 100;
		else if (g_RandGen.RandRatio(57, PRO_DENO_100))
			lUserRealJetton = 1000;
		else
			lUserRealJetton = 5000;
	} else if (lUserCurJetton >= 50000 && lUserCurJetton < 200000) {
		if (g_RandGen.RandRatio(15, PRO_DENO_100))
			lUserRealJetton = 1000;
		else if (g_RandGen.RandRatio(47, PRO_DENO_100))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(35, PRO_DENO_100))
			lUserRealJetton = 1000;
		else
			lUserRealJetton = 50000;
	} else if (lUserCurJetton >= 200000 && lUserCurJetton < 2000000) {
		if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(5500, PRO_DENO_10000))
			lUserRealJetton = 10000;
		else
			lUserRealJetton = 50000;
	} else if (lUserCurJetton >= 2000000) {
		if (g_RandGen.RandRatio(500, PRO_DENO_10000))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
			lUserRealJetton = 10000;
		else
			lUserRealJetton = 50000;
	} else
		lUserRealJetton = 100;

	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 50000)
		lUserRealJetton = 10000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 10000)
		lUserRealJetton = 5000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 5000)
		lUserRealJetton = 1000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 1000)
		lUserRealJetton = 100;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100)
		lUserRealJetton = 0;
	if (lUserRealJetton < lUserMinJetton)
		lUserRealJetton = 0;
	return lUserRealJetton;
}

int64 CGameCarcityTable::GetRobotJettonScoreRand(CGamePlayer* pPlayer, uint8 area) {
	int64 lUserRealJetton = 100;
	int switch_on = g_RandGen.GetRandi(0, 3);
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

void CGameCarcityTable::AddPlayerToBlingLog() {
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL)
			continue;
		for (uint8 i = 0; i < JETTON_INDEX_COUNT; ++i) {
			if (m_userJettonScore[i][pPlayer->GetUID()] > 0) {
				AddUserBlingLog(pPlayer);
				break;
			}
		}
	}
}

void CGameCarcityTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo) {
	net::msg_carcity_play_log_rep* pplay = pInfo->mutable_carcity();
	for (uint16 i = 0; i < m_vecRecord.size(); ++i) {
		net::carcity_play_log* plog = pplay->add_logs();
		stCarcityGameRecord& record = m_vecRecord[i];
		for (uint16 j = 0; j < JETTON_INDEX_COUNT; ++j)
			plog->add_seats_win(record.wins[j]);
	}
}

void CGameCarcityTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo) {
	net::msg_carcity_play_log_rep* pplay = pInfo->mutable_carcity();
	net::carcity_play_log* plog = pplay->add_logs();
	stCarcityGameRecord& record = m_record;
	for (uint16 j = 0; j < JETTON_INDEX_COUNT; ++j)
		plog->add_seats_win(record.wins[j]);
}

void CGameCarcityTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer) {
	if (pPlayer == NULL)
		return;
	LOG_DEBUG("send brc control all true player info list uid:%d.", pPlayer->GetUID());

	net::msg_brc_control_all_player_bet_info rep;

	//计算旁观玩家
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)
			continue;
		if (tmp_pPlayer->IsRobot())
			continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//统计信息
		map<uint32, tagPlayerResultInfo>::iterator iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end()) {
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (uint16 wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

void CGameCarcityTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer) {
	if (pPlayer == NULL)
		return;

	if (pPlayer->IsRobot())
		return;

	uint32 uid = pPlayer->GetUID();
	LOG_DEBUG("notice brc control single true player bet info uid:%d.", uid);
	net::msg_brc_control_single_player_bet_info rep;

	//计算旁观玩家
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)
			continue;
		if (uid == tmp_pPlayer->GetUID()) {
			net::brc_control_player_bet_info *info = rep.mutable_player_bet_info();
			info->set_uid(uid);
			info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			info->set_name(pPlayer->GetPlayerName());

			//统计信息
			map<uint32, tagPlayerResultInfo>::iterator iter = m_mpPlayerResultInfo.find(uid);
			if (iter != m_mpPlayerResultInfo.end()) {
				info->set_curr_day_win(iter->second.day_win_coin);
				info->set_win_number(iter->second.win);
				info->set_lose_number(iter->second.lose);
				info->set_total_win(iter->second.total_win_coin);
			}
			//下注信息	
			uint64 total_bet = 0;
			for (uint16 wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
				info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
				total_bet += m_userJettonScore[wAreaIndex][uid];
			}
			info->set_total_bet(total_bet);
			info->set_ismaster(IsBrcControlPlayer(uid));
			break;
		}
	}

	for (CGamePlayer* it : m_setControlPlayers)
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_SINGLE_PLAYER_BET_INFO);
}

void CGameCarcityTable::OnBrcControlSendAllRobotTotalBetInfo() {
	LOG_DEBUG("notice brc control all robot totol bet info.");

	net::msg_brc_control_total_robot_bet_info rep;
	for (uint16 wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
		rep.add_area_info(m_allJettonScore[wAreaIndex] - m_playerJettonScore[wAreaIndex]);
		//LOG_DEBUG("wAreaIndex:%d m_allJettonScore[%d]:%lld m_playerJettonScore[%d]:%lld", wAreaIndex, wAreaIndex, m_allJettonScore[wAreaIndex], wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (CGamePlayer* it : m_setControlPlayers)
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_ROBOT_BET_INFO);
}

void CGameCarcityTable::OnBrcControlSendAllPlayerTotalBetInfo() {
	LOG_DEBUG("notice brc control all player totol bet info.");

	net::msg_brc_control_total_player_bet_info rep;
	for (uint16 wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex)
		rep.add_area_info(m_playerJettonScore[wAreaIndex]);

	for (CGamePlayer* it : m_setControlPlayers)
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_PLAYER_BET_INFO);
}

bool CGameCarcityTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer) {
	if (pPlayer == NULL)
		return false;

	LOG_DEBUG("brc control enter control interface. uid:%d", pPlayer->GetUID());

	bool ret = OnBrcControlPlayerEnterInterface(pPlayer);
	if (ret) {
		//刷新百人场桌子状态
		m_brc_table_status_time = m_coolLogic.getCoolTick();
		OnBrcControlFlushTableStatus(pPlayer);

		//发送所有真实玩家列表
		OnBrcControlSendAllPlayerInfo(pPlayer);
		//发送机器人总下注信息
		OnBrcControlSendAllRobotTotalBetInfo();
		//发送真实玩家总下注信息
		OnBrcControlSendAllPlayerTotalBetInfo();

		return true;
	}
	return false;
}

void CGameCarcityTable::OnBrcControlBetDeal(CGamePlayer* pPlayer) {
	if (pPlayer == NULL)
		return;

	LOG_DEBUG("brc control bet deal. uid:%d", pPlayer->GetUID());
	if (pPlayer->IsRobot())
		//发送机器人总下注信息
		OnBrcControlSendAllRobotTotalBetInfo();
	else {
		//通知单个玩家下注信息
		OnBrcControlNoticeSinglePlayerInfo(pPlayer);
		//发送真实玩家总下注信息
		OnBrcControlSendAllPlayerTotalBetInfo();
	}
}

bool CGameCarcityTable::OnBrcAreaControl() {
	LOG_DEBUG("brc area control. roomid:%d,tableid:%d,m_real_control_uid:%d,m_winCardType:%d",
		GetRoomID(), GetTableID(), m_real_control_uid, m_winCardType);

	if (m_real_control_uid == 0) {
		LOG_DEBUG("brc area control the control uid is zero.");
		return false;
	}

	//获取当前控制区域
	for (uint8 i = AREA_LBGN_MEN; i < JETTON_INDEX_COUNT; ++i)
		if (m_req_control_area[i] == 1) {
			m_winCardType = i;
			LOG_DEBUG("find area ctrl is true. ctrl_area:%d", m_winCardType);
			return true;
		}

	if (m_req_control_area[AREA_BANK] == 1) {
		for (int i = 0; i < 1000; ++i) {
			int winCarType = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
			if (GetPlayerWinScore(winCarType) < 1) {
				m_winCardType = winCarType;
				LOG_DEBUG("find area ctrl is suc  AREA_BANK  winCarType=%d", winCarType);
				return true;
			}
		}
		LOG_DEBUG("find area ctrl is fail AREA_BANK");
	}

	if (m_req_control_area[AREA_XIAN] == 1) {
		for (int i = 0; i < 1000; ++i) {
			int winCarType = g_RandGen.GetRandi(AREA_LBGN_MEN, AREA_HONDA_MEN);
			if (GetPlayerWinScore(winCarType) > -1) {
				m_winCardType = winCarType;
				LOG_DEBUG("find area ctrl is suc  AREA_XIAN  winCarType=%d", winCarType);
				return true;
			}
		}
		LOG_DEBUG("find area ctrl is fail AREA_XIAN");
	}

	LOG_DEBUG("find area ctrl is fail");
	return false;
}

void CGameCarcityTable::OnBrcFlushSendAllPlayerInfo() {
	LOG_DEBUG("send brc flush all true player info list.");

	net::msg_brc_control_all_player_bet_info rep;

	// 计算旁观玩家
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)
			continue;
		if (tmp_pPlayer->IsRobot())
			continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//统计信息
		map<uint32, tagPlayerResultInfo>::iterator iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end()) {
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//下注信息	
		uint64 total_bet = 0;
		for (uint16 wAreaIndex = AREA_LBGN_MEN; wAreaIndex < JETTON_INDEX_COUNT; ++wAreaIndex) {
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	for (CGamePlayer *it : m_setControlPlayers)
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

// 获取单个下注的是机器人还是玩家
void CGameCarcityTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
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
