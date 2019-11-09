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

using namespace std;
using namespace svrlib;
using namespace game_zajinhua;
using namespace net;

namespace
{
    const static uint32 s_AddScoreTime       = 15 * 1000;
    const static uint32 s_FreeTime           =  2 * 1000;
	const static uint32 s_readyStartTime     =  5 * 1000;     // 开始倒计时 add by har
	const static uint32 s_gameEndTime        =  5 * 1000;     // 结算 add by har
    const static uint8  s_JettonMultip[]     = {1,2,5, 10};   // 暗注倍数
    const static uint8  s_JettonMultip2[]    = {2,4,10,20};   // 明注倍数
    
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 扎金花
        {
            pTable = new CGameZajinhuaTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛扎金花
        {
            pTable = new CGameZajinhuaTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房扎金花
        {
            pTable = new CGameZajinhuaTable(this,tableID,emTABLE_TYPE_PLAYER);
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
CGameZajinhuaTable::CGameZajinhuaTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();
	//游戏变量
	m_bOperaCount   =   0;
	m_wBankerUser   =   INVALID_CHAIR;
	m_wCurrentUser  =   INVALID_CHAIR;
	m_bGameEnd      =   false;

	//用户状态
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
    ZeroMemory(m_szCardState,sizeof(m_szCardState));
	for(int i=0;i<GAME_PLAYER;i++){
		m_wCompardUser[i].clear();
	}
	//扑克变量
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));

	//下注信息
	m_lCellScore    =   0L;
	m_lCurrentTimes =   0L;		
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_lUserMaxScore,sizeof(m_lUserMaxScore));
	ZeroMemory(m_szNoOperCount, sizeof(m_szNoOperCount)); // add by har
	ZeroMemory(m_szNoOperTrun, sizeof(m_szNoOperTrun));   // add by har
	ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus));   // add by har

    m_cmpRound  = 2;      //比牌轮数
    m_lookRound = 1;      //看牌轮数
    m_maxRound  = 20;     //最大轮数

	m_tagControlPalyer.Init();

	for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		m_cbPlayIsRobot[i] = emPLAYER_IS_NOT_READY;
	}
	m_robotBankerWinPro = 0;
}
CGameZajinhuaTable::~CGameZajinhuaTable()
{
    
    
}
bool    CGameZajinhuaTable::CanEnterTable(CGamePlayer* pPlayer)
{
    if(pPlayer->GetTable() != NULL)
        return false;
    // 限额进入
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin()){
        return false;
    }
	if (pPlayer->IsRobot()) {
		int irobot_num = 0;
		for (uint32 i = 0; i < m_vecPlayers.size(); ++i) {
			CGamePlayer* ptmp_Player = m_vecPlayers[i].pPlayer;
			if (ptmp_Player != NULL && ptmp_Player->IsRobot())
			{
				irobot_num++;
			}
		}
		if (irobot_num >= TABLE_ROBOT_COUNT_ZAJINHUA) {
			return false;
		}
	}
	bool bIsNoviceWelfare = EnterNoviceWelfare(pPlayer);
	//增加新注册用户的判断
	bool bIsNewRegisterWelfare = EnterNewRegisterWelfare(pPlayer);
	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNewRegisterWelfare:%d,bIsNoviceWelfare:%d",
		pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNewRegisterWelfare, bIsNoviceWelfare);
	if (bIsNewRegisterWelfare == true || bIsNoviceWelfare == true)
	{
		return false;
	}
	
    return true;
}
bool    CGameZajinhuaTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    if(GetGameState() != TABLE_STATE_ZAJINHUA_FREE && GetGameState() != TABLE_STATE_ZAJINHUA_READY_START)
    {
        for(uint16 i=0;i<m_vecPlayers.size();++i)
        {
            if(m_vecPlayers[i].pPlayer == pPlayer)
            {
                if(m_cbPlayStatus[i] == TRUE)
                    return false;
				if (m_szGameEndStatus[i] == TRUE)
					return false;
                break;
            }
        }
    }
    return true;
}

// add by har
bool CGameZajinhuaTable::LeaveTable(CGamePlayer* pPlayer, bool bNotify) {
	bool bRet = CGameTable::LeaveTable(pPlayer, bNotify);
	if (bRet) {
		int uPlayerCount = GetPlayerNum();
		if (GetGameState() == TABLE_STATE_ZAJINHUA_READY_START && uPlayerCount < 2) {
			CheckPlayerScoreLeave();
			SetGameState(TABLE_STATE_ZAJINHUA_FREE);
			m_coolLogic.beginCooling(s_FreeTime);
			// 通知客户端游戏状态变化
			net::msg_zajinhua_game_info_free_rep msg;
			SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_GAME_FREE_INFO);
			LOG_DEBUG("CGameZajinhuaTable::LeaveTable FREE roomid:%d,tableid:%d,s_FreeTime:%d,uPlayerCount:%d",
				GetRoomID(), GetTableID(), s_FreeTime, uPlayerCount);
		}
	}
	return bRet;
}

void CGameZajinhuaTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::zajinhua_table_info* pzajinhua = pInfo->mutable_zajinhua();
    pzajinhua->set_tableid(GetTableID());
    pzajinhua->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pzajinhua->set_is_passwd(1);
    }else{
        pzajinhua->set_is_passwd(0);
    }
    pzajinhua->set_hostname(m_conf.hostName);
    pzajinhua->set_basescore(m_conf.baseScore);
    pzajinhua->set_consume(m_conf.consume);
    pzajinhua->set_entermin(m_conf.enterMin);
    pzajinhua->set_duetime(m_conf.dueTime);
    pzajinhua->set_feetype(m_conf.feeType);
    pzajinhua->set_feevalue(m_conf.feeValue);
    pzajinhua->set_card_time(s_AddScoreTime);
    pzajinhua->set_table_state(GetGameState());
    pzajinhua->set_compare_round(GetCompareRound());
    pzajinhua->set_look_round(GetLookRound());
    pzajinhua->set_limit_round(GetMaxRound());
    for(uint16 i=0;i<getArrayLen(s_JettonMultip2);++i){
        pzajinhua->add_ming_multip(s_JettonMultip2[i]);
        pzajinhua->add_blind_multip(s_JettonMultip[i]);
    }    
    
}

//配置桌子
bool CGameZajinhuaTable::Init()
{
    SetGameState(net::TABLE_STATE_ZAJINHUA_FREE);
    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    string param = m_pHostRoom->GetCfgParam();    
    Json::Reader reader;
    Json::Value  jvalue;
    if(!reader.parse(param,jvalue))
    {
        LOG_ERROR("解析游戏参数json错误:%s",param.c_str());
        return true;
    }
    if(jvalue.isMember("cmp")){
        m_cmpRound = jvalue["cmp"].asInt();
    }
    if(jvalue.isMember("look")){
        m_lookRound = jvalue["look"].asInt();
    }
    if(jvalue.isMember("maxrd")){
        m_maxRound  = jvalue["maxrd"].asInt();
    }

	m_iArrDispatchCardPro[Pro_Index_BaoZi] = 23;
	m_iArrDispatchCardPro[Pro_Index_ShunJin] = 22;
	m_iArrDispatchCardPro[Pro_Index_JinHua] = 800;
	m_iArrDispatchCardPro[Pro_Index_ShunZi] = 655;
	m_iArrDispatchCardPro[Pro_Index_Double] = 4000;
	m_iArrDispatchCardPro[Pro_Index_Single] = 4500;

	m_iArrWelfareCardPro[Pro_Index_BaoZi] = 23;
	m_iArrWelfareCardPro[Pro_Index_ShunJin] = 22;
	m_iArrWelfareCardPro[Pro_Index_JinHua] = 800;
	m_iArrWelfareCardPro[Pro_Index_ShunZi] = 655;
	m_iArrWelfareCardPro[Pro_Index_Double] = 4000;
	m_iArrWelfareCardPro[Pro_Index_Single] = 4500;


	memset(m_iArrNRWCardPro, 0x0, sizeof(m_iArrNRWCardPro));
	
	ReAnalysisParam();
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGameZajinhuaTable::ReAnalysisParam()
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		string strPro = CStringUtility::FormatToString("pr%d",i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral()) 
		{
			m_iArrDispatchCardPro[i] = jvalue[strPro.c_str()].asInt();
		}
	}

	if (jvalue.isMember("wro") && jvalue["wro"].isString())
	{
		string str_temp = jvalue["wro"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (str_temp.empty() == false && tmpreader.parse(str_temp, tmpjvalue))
		{
			LOG_ERROR("analysisjson_error - roomid:%d,tableid:%d,tmpjvalue.size:%d,str:%s",
				GetRoomID(), GetTableID(), tmpjvalue.size(), str_temp.c_str());
			if (tmpjvalue.size() == Pro_Index_MAX)
			{
				for (uint32 i = 0; i < tmpjvalue.size(); i++)
				{
					LOG_DEBUG("json_analysis - roomid:%d,tableid:%d,tmpjvalue.size:%d,i:%d,isIntegral:%d", GetRoomID(), GetTableID(), tmpjvalue.size(), i, tmpjvalue[i].isIntegral());
					if (!tmpjvalue[i].isIntegral())
					{
						continue;
					}
					m_iArrWelfareCardPro[i] = tmpjvalue[i].asInt();
				}
			}
		}

	}
	string strWelfareCardPro;
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		strWelfareCardPro = CStringUtility::FormatToString("%d", m_iArrWelfareCardPro[i]);
	}

	if (jvalue.isMember("bbw"))
	{
		m_robotBankerWinPro = jvalue["bbw"].asInt();
	}

	//新注册玩家福利牌型的概率配置
	if (jvalue.isMember("nrwct"))
	{
		string strPro = jvalue["nrwct"].asString();
		if (strPro.size() > 0)
		{
			Json::Reader nrwct_reader;
			Json::Value  nrwct_jvalue;
			if (!nrwct_reader.parse(strPro, nrwct_jvalue)) 
			{
				LOG_ERROR("解析 nrwct:%s json串错误", strPro.c_str());				
			}
			else
			{ 
				for (uint32 i = 0; i < nrwct_jvalue.size(); ++i)
				{
					m_iArrNRWCardPro[i] = nrwct_jvalue[i].asUInt();
				}
				m_iArrNRWCardPro[Pro_Index_Double] = 0;
				m_iArrNRWCardPro[Pro_Index_Single] = 0;
			}
		}		
	}

	LOG_ERROR("reader_json -  roomid:%d,tableid:%d,strWelfareCardPro:%s,m_iArrDispatchCardPro:%d %d %d %d %d %d,m_robotBankerWinPro:%d,m_iArrNRWCardPro:%d %d %d %d %d %d",
		GetRoomID(), GetTableID(), strWelfareCardPro.c_str(), m_iArrDispatchCardPro[0], m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3],
		m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5], m_robotBankerWinPro, m_iArrNRWCardPro[0], m_iArrNRWCardPro[1], m_iArrNRWCardPro[2], 
		m_iArrNRWCardPro[3], m_iArrNRWCardPro[4], m_iArrNRWCardPro[5]);

	return true;
}
void CGameZajinhuaTable::ShutDown()
{

}
//结束游戏，复位桌子
void CGameZajinhuaTable::ResetTable()
{
    //ResetGameData();

    SetGameState(TABLE_STATE_ZAJINHUA_GAME_END /*TABLE_STATE_ZAJINHUA_FREE*/); // modify by har
    ResetPlayerReady();

	// add by har
	m_coolLogic.beginCooling(s_gameEndTime);
	m_coolRobot.beginCooling(g_RandGen.RandRange(4000, 5000));
	OnTableGameEnd(); // add by har end
}

// 将资金不满足条件的玩家踢出 add by har
void CGameZajinhuaTable::CheckPlayerScoreLeave()
{
	if (m_pHostRoom == NULL) {
		LOG_DEBUG("CGameZajinhuaTable::CheckPlayerScoreLeave roomid:%d,tableid:%d,m_pHostRoom:%p", GetRoomID(), GetTableID(), m_pHostRoom);
		return;
	}
	for (uint16 i = 0; i < GAME_PLAYER; ++i) {
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL ) {
			int64 lCurScore = GetPlayerCurScore(pPlayer);
			int64 lMaxScore = m_pHostRoom->GetEnterMax();
			int64 lMinScore = m_pHostRoom->GetEnterMin();
			if (lCurScore >= lMaxScore || lCurScore < lMinScore) {
				uint32 uid = pPlayer->GetUID();
				bool bCanLeaveTable = CanLeaveTable(pPlayer);
				bool bLeaveTable = false;
				bool bCanLeaveRoom = false;
				bool bLeaveRoom = false;
				if (bCanLeaveTable) {
					bLeaveTable = LeaveTable(pPlayer);
					if (bLeaveTable) {
						bCanLeaveRoom = m_pHostRoom->CanLeaveRoom(pPlayer);
						if (bCanLeaveRoom)
							bLeaveRoom = m_pHostRoom->LeaveRoom(pPlayer);
						net::msg_leave_table_rep rep;
						rep.set_result(1);
						pPlayer->SendMsgToClient(&rep, net::S2C_MSG_LEAVE_TABLE_REP);
						m_szNoOperCount[i] = 0;
						m_szNoOperTrun[i] = 0;
					}
				}
				LOG_DEBUG("CGameZajinhuaTable::CheckPlayerScoreLeave roomid:%d,tableid:%d,i:%d,uid:%d,lCurScore:%lld,lMaxScore:%lld,bCanLeaveTable:%d,bLeaveTable:%d,bCanLeaveRoom:%d,bLeaveRoom:%d",
					GetRoomID(), GetTableID(), i, uid, lCurScore, lMaxScore, bCanLeaveTable, bLeaveTable, bCanLeaveRoom, bLeaveRoom);
			}
		}
	}
}

// 将挂机玩家踢出 add by har
void CGameZajinhuaTable::CheckNoOperPlayerLeave() {
	if (m_pHostRoom == NULL) {
		LOG_DEBUG("CGameZajinhuaTable::CheckNoOperPlayerLeave roomid:%d,tableid:%d,m_pHostRoom:%p", GetRoomID(), GetTableID(), m_pHostRoom);
		return;
	}
	for (uint16 i = 0; i < GAME_PLAYER; ++i) {
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer != NULL) {
			if (m_szNoOperTrun[i] > 2) {
				uint32 uid = pPlayer->GetUID();
				bool bCanLeaveTable = CanLeaveTable(pPlayer);
				bool bLeaveTable = false;
				bool bCanLeaveRoom = false;
				bool bLeaveRoom = false;
				if (bCanLeaveTable) {
					bLeaveTable = LeaveTable(pPlayer);
					if (bLeaveTable) {
						bCanLeaveRoom = m_pHostRoom->CanLeaveRoom(pPlayer);
						if (bCanLeaveRoom)
							bLeaveRoom = m_pHostRoom->LeaveRoom(pPlayer);
						net::msg_leave_table_rep rep;
						rep.set_result(1);
						pPlayer->SendMsgToClient(&rep, net::S2C_MSG_LEAVE_TABLE_REP);
						m_szNoOperCount[i] = 0;
						m_szNoOperTrun[i] = 0;
					}
				}
				LOG_DEBUG("CGameZajinhuaTable::CheckNoOperPlayerLeave roomid:%d,tableid:%d,i:%d,uid:%d,szNoOperCount[]:%d,m_szNoOperTrun[]:%d,bCanLeaveTable:%d,bLeaveTable:%d,bCanLeaveRoom:%d,bLeaveRoom:%d",
					GetRoomID(), GetTableID(), i, uid, m_szNoOperCount[i], m_szNoOperTrun[i], bCanLeaveTable, bLeaveTable, bCanLeaveRoom, bLeaveRoom);
			}
		}
	}
}

void CGameZajinhuaTable::OnTimeTick()
{
	OnTableTick();

    if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_ZAJINHUA_FREE:
            {
                if(IsCanStartGame()){ 
					// modify by har
					//OnGameStart();
					LOG_DEBUG("CGameZajinhuaTable::OnTimeTick  FREE->READY_START  roomid:%d,tableid:%d,s_readyStartTime:%d",
						GetRoomID(), GetTableID(), s_readyStartTime);
					SetGameState(TABLE_STATE_ZAJINHUA_READY_START);
					m_coolLogic.beginCooling(s_readyStartTime);
					// 通知客户端剩余时间
					net::msg_zajinhua_game_info_play_rep msg;
					msg.set_game_status(GetGameState()); // add by har
					msg.set_oper_time(m_coolLogic.getCoolTick());
					SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_GAME_PLAY_INFO);
					// modify by har end
                }
            }break;
		// add by har
		case TABLE_STATE_ZAJINHUA_READY_START:
		{
			CheckPlayerScoreLeave();
			OnGameStart();
		}break; // add by har end
        case TABLE_STATE_ZAJINHUA_PLAY:
            {
				// 机器人超时弃牌
				CGamePlayer * pPlayer = GetPlayer(m_wCurrentUser);
				if (pPlayer != NULL /*&& pPlayer->IsRobot()*/)
				{
					LOG_DEBUG("CGameZajinhuaTable::OnTimeTick 超时自动弃牌 - roomid:%d,tableid:%d,m_wCurrentUser:%d,uid:%d,m_coolRobot.isTimeOut:%d,m_wCurrentRound:%d,GetLookRound:%d,IsRobot:%d,m_cbPlayStatus[]:%d,m_lTableScore[]:%d,m_szNoOperCount[]:%d,m_szNoOperTrun[]:%d,playernum:%d",
						GetRoomID(), GetTableID(), m_wCurrentUser, GetPlayerID(m_wCurrentUser), m_coolRobot.isTimeOut(), 
						m_wCurrentRound, GetLookRound(), pPlayer->IsRobot(), m_cbPlayStatus[m_wCurrentUser], m_lTableScore[m_wCurrentUser],
						m_szNoOperCount[m_wCurrentUser], m_szNoOperTrun[m_wCurrentUser], GetPlayerNum());
					if (!pPlayer->IsRobot() && m_cbPlayStatus[m_wCurrentUser] == TRUE)
						++m_szNoOperCount[m_wCurrentUser]; // add by har
				}
                OnUserGiveUp(m_wCurrentUser);
            }break;
        case TABLE_STATE_ZAJINHUA_WAIT:
            {
                OnOverCompareCard();
            }break;
		// add by har
		case TABLE_STATE_ZAJINHUA_GAME_END:
		{
			ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus));
			SetGameState(TABLE_STATE_ZAJINHUA_FREE);
			m_coolLogic.beginCooling(s_FreeTime);
			CheckNoOperPlayerLeave();
			CheckPlayerScoreLeave();
		}break; // add by har end
        default:
            break;
        }
    }
    if(GetGameState() == TABLE_STATE_ZAJINHUA_FREE)
	{
       CheckAddRobot(); 
    }
    if(GetGameState() == TABLE_STATE_ZAJINHUA_PLAY && m_coolRobot.isTimeOut())
    {
        CGamePlayer* pPlayer = GetPlayer(m_wCurrentUser); // m_vecPlayers[m_wCurrentUser].pPlayer; modify by har
        if(pPlayer != NULL && pPlayer->IsRobot())
		{
           OnRobotOper(m_wCurrentUser);
        }
    }
}
// 游戏消息
int CGameZajinhuaTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if (pPlayer == NULL)
	{
		return 0;
	}
	uint32 uid = pPlayer->GetUID();
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("收到玩家消息 - roomid:%d,tableid:%d,uid:%d,chairID:%d,cmdID:%d,GetGameState:%d", GetRoomID(),
		GetTableID(), uid, chairID, cmdID, GetGameState());

	// add by har
	if (chairID < GAME_PLAYER) {
		m_szNoOperCount[chairID] = 0;
		m_szNoOperTrun[chairID] = 0;
	} // add by har

    switch(cmdID)
    {
    case net::C2S_MSG_ZAJINHUA_ADD_SCORE:// 加注
        {
		if (GetGameState() != TABLE_STATE_ZAJINHUA_PLAY) {
			    LOG_WARNING("加注，游戏状态不对！ roomid:%d,tableid:%d,uid:%d,GetGameState:%d", GetRoomID(), 
					GetTableID(), uid, GetGameState());
			    return 0;
			}
            net::msg_zajinhua_addscore_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(m_wCurrentUser != chairID){
				LOG_WARNING("加注，不是当前用户: roomid:%d,tableid:%d,uid:%d,m_wCurrentUser:%d,chairID:%d,add_score:%d", 
					GetRoomID(), GetTableID(), uid, m_wCurrentUser, chairID, msg.add_score());
                return false;
            }
			if (m_cbPlayStatus[chairID] == FALSE) {
				LOG_WARNING("加注，用户状态不对: roomid:%d,tableid:%d,uid:%d,chairID:%d,add_score:%d", GetRoomID(), 
					GetTableID(), uid, chairID, msg.add_score());
				return false;
			}
			if (IsOnlyCompare(chairID)) {
				LOG_WARNING("加注，只剩比牌: roomid:%d,tableid:%d,uid:%d,chairID:%d,add_score:%d", GetRoomID(), 
					GetTableID(), uid, chairID, msg.add_score());
				return false;
			}
            if(msg.is_allin() == 1){
                if(GetPlayNum() > 2) {
					LOG_WARNING("加注，大于2人只能allin - roomid:%d,tableid:%d,uid:%d,GetPlayNum:%d,add_score:%d", GetRoomID(),
						GetTableID(), uid, GetPlayNum(), msg.add_score());
                    return false;
                }
                return OnUserAllin(chairID);
            }
            if(m_bAllinState) {
				LOG_WARNING("加注，AllIn状态只能Allin: roomid:%d,tableid:%d,%uid:%d,chairID:%d,add_score:%d", GetRoomID(), 
					GetTableID(), uid, chairID, msg.add_score());
                return false;
            }
			return OnUserAddScore(chairID,msg.add_score(),false,false);
        }break;
    case net::C2S_MSG_ZAJINHUA_COMPARE_CARD:// 用户比牌
        {
            if(GetGameState() != TABLE_STATE_ZAJINHUA_PLAY)
                return 0;
            net::msg_zajinhua_compare_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(m_wCurrentUser != chairID){
                LOG_DEBUG("不是当前用户:%d--%d",m_wCurrentUser,chairID);
                return false;
            }
            uint16 compareUser = msg.compare_user();
			if(m_cbPlayStatus[chairID] == FALSE || m_cbPlayStatus[compareUser] == FALSE)
                return false;

			return OnUserCompareCard(chairID,compareUser);
        }break;
    case net::C2S_MSG_ZAJINHUA_GIVEUP:// 弃牌
        {            
            if(GetGameState() != TABLE_STATE_ZAJINHUA_PLAY)
                return 0;
			if(m_cbPlayStatus[chairID] == FALSE) 
                return 0;
            /*if(m_wCurrentUser != chairID){ delete by har
                LOG_DEBUG("不是当前用户:%d--%d",m_wCurrentUser,chairID);
                return false;
            }*/
			return OnUserGiveUp(chairID);
        }break;
    case net::C2S_MSG_ZAJINHUA_LOOK_CARD:// 看牌
        {
			if (GetGameState() != TABLE_STATE_ZAJINHUA_PLAY)
			{
				return 0;
			}
			if (m_cbPlayStatus[chairID] == FALSE)
			{
				return 0;
			}
            			
            return OnUserLookCard(chairID);
        }break;
    case net::C2S_MSG_ZAJINHUA_OPEN_CARD:// 开牌
        {
            if(GetGameState() != TABLE_STATE_ZAJINHUA_PLAY)
                return 0;
			if(m_cbPlayStatus[chairID] == FALSE) 
                return 0;
            if(m_wCurrentUser != chairID){
                LOG_DEBUG("不是当前用户:%d--%d",m_wCurrentUser,chairID);
                return false;
            }
			return OnUserOpenCard(chairID);
        }break;
    case net::C2S_MSG_ZAJINHUA_SHOW_CARD:// 亮牌
        {
            return OnUserShowCard(chairID);
        }break;

    default:
        return 0;
    }
    return 0;
}
// 游戏开始
bool CGameZajinhuaTable::OnGameStart()
{
	// add by har
	int32 uPlayerCount = GetPlayerNum();

	if (uPlayerCount < 2) {
		// 倒计时的时候有人离开不能继续开始游戏
		SetGameState(TABLE_STATE_ZAJINHUA_FREE);
		m_coolLogic.beginCooling(s_FreeTime);
		LOG_DEBUG("CGameZajinhuaTable::OnGameStart FREE roomid:%d,tableid:%d,s_FreeTime:%d,uPlayerCount:%d", 
			GetRoomID(), GetTableID(), s_FreeTime, uPlayerCount);
		return false;
	} // add by har end

    //LOG_DEBUG("game start");
    //设置状态
    SetGameState(TABLE_STATE_ZAJINHUA_PLAY);
	m_bGameEnd = false;
	m_lucky_flag = false;
    ResetGameData();

	// add by har
	string strAllPlayerUid;
	string strAllPlayerStatus;
	for (WORD i = 0; i < GAME_PLAYER; i++) {
		CGamePlayer *pPlayer = GetPlayer(i);
		if (pPlayer != NULL)
			strAllPlayerUid += CStringUtility::FormatToString("i_%d-uid_%d-npc_%d-npt_%d ", i, pPlayer->GetUID(), m_szNoOperCount[i], m_szNoOperTrun[i]);
		if (m_cbPlayStatus[i] == TRUE)
			strAllPlayerStatus += CStringUtility::FormatToString("i:%d,ps:%d", i, m_cbPlayStatus[i]);
	} // add by har end
    InitBlingLog(false /*true*/); // modify by har
	LOG_DEBUG("CGameZajinhuaTable::OnGameStart roomid:%d,tableid:%d,chessid:%s,uids:%s,status:%s", GetRoomID(), 
		GetTableID(), GetChessID().c_str(), strAllPlayerUid.c_str(), strAllPlayerStatus.c_str());

	for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		m_cbPlayIsRobot[i] = emPLAYER_IS_NOT_READY;
	}
    for(WORD i=0;i<GAME_PLAYER;i++)
	{
		//获取用户
		CGamePlayer *pPlayer = GetPlayer(i);
		if(pPlayer == NULL || !IsReady(pPlayer))
            continue;

		const int64 lUserScore = GetPlayerCurScore(pPlayer);

		if (pPlayer->IsRobot())
		{
			m_cbPlayIsRobot[i] = emPLAYER_IS_ROBOT;
		}
		else
		{
			m_cbPlayIsRobot[i] = emPLAYER_IS_USER;
		}

		//设置变量
		m_cbPlayStatus[i]   = TRUE;
        m_szJoinGame[i]     = TRUE;
		m_lUserMaxScore[i]  = lUserScore;
	}
	//下注变量
	m_lCellScore    = GetBaseScore();
	m_lCurrentTimes = 1;
    m_wCurrentRound = 0;

	//分发扑克
	DispatchCard();

    //用户设置
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if(m_cbPlayStatus[i] == TRUE)
		{
			m_lTableScore[i] = m_lCellScore;
            WriteAddScoreLog(i,m_lCellScore);
		}
	}

	//设置庄家
	if(m_wBankerUser == INVALID_CHAIR)
        m_wBankerUser = g_RandGen.RandUInt()%GAME_PLAYER;

	//庄家离开
	if(m_wBankerUser < GAME_PLAYER && m_cbPlayStatus[m_wBankerUser]==FALSE)
        m_wBankerUser = g_RandGen.RandUInt()%GAME_PLAYER;

	//确定庄家
	uint32 uLoopCount = 0;
	while(m_cbPlayStatus[m_wBankerUser]==FALSE)
	{
		uLoopCount++;
		m_wBankerUser = (m_wBankerUser+1)%GAME_PLAYER;
		if (uLoopCount >= GAME_PLAYER + GAME_PLAYER)
		{
			// 第五个循环完找不到庄家别开始了
			LOG_ERROR("CGameZajinhuaTable::OnGameStart2 roomid:%d,tableid:%d,uLoopCount:%d,m_wBankerUser:%d", 
				GetRoomID(), GetTableID(), uLoopCount, m_wBankerUser);
			return true;
		}
	}
	//m_wCanLookUser = m_wBankerUser;
	//当前用户 庄家下一个已经准备的用户
	m_wCurrentUser = (m_wBankerUser+1)%GAME_PLAYER;
	while(m_cbPlayStatus[m_wCurrentUser] == FALSE)
	{
		m_wCurrentUser = (m_wCurrentUser+1)%GAME_PLAYER;
	}

	//构造数据
    net::msg_zajinhua_start_rep msg;    
    
    msg.set_current_user(m_wCurrentUser);
    msg.set_banker_user(m_wBankerUser);

	for (WORD i = 0; i < GAME_PLAYER; ++i)
		msg.add_play_status(m_szJoinGame[i]); // add by har

	//发送数据
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if(m_cbPlayStatus[i] == TRUE)
		{
            msg.set_user_max_score(m_lUserMaxScore[i]);
		}else{
		    msg.set_user_max_score(m_lUserMaxScore[i]);
		}
        SendMsgToClient(i,&msg,net::S2C_MSG_ZAJINHUA_START);
	}

    //服务费
    DeductStartFee(true);

    m_coolLogic.beginCooling(s_AddScoreTime + 1200 + 500*GetChairPlayerNum()); // 15秒+开始动画时间(1.2秒)+0.5*游戏人数=第一个玩家的最大操作时间
    m_coolRobot.beginCooling(g_RandGen.RandRange(3000,5000));

	LOG_DEBUG("gamestart check_round - roomid:%d,tableid:%d,m_wCurrentRound:%d,m_wBankerUser:%d - %d,m_lCurrentTimes:%d, m_wCurrentUser:%d",
		GetRoomID(), GetTableID(), m_wCurrentRound, m_wBankerUser, GetPlayerID(m_wBankerUser), m_lCurrentTimes, m_wCurrentUser);
	
    return true;
}

// 每局结束重新设置未操作状态计数 add by har
void CGameZajinhuaTable::ResetGameEndNoOper() {
	for (uint16 i = 0; i < GAME_PLAYER; ++i) {
		if (m_szNoOperCount[i] > 0)
			++m_szNoOperTrun[i];
		m_szNoOperCount[i] = 0;
	}
}

//游戏结束
bool CGameZajinhuaTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("CGameZajinhuaTable::OnGameEnd roomid:%d,tableid:%d,chairID:%d--reason:%d", GetRoomID(), 
		GetTableID(), chairID, reason);
	switch(reason)
	{
	case GER_COMPARECARD:	//比牌结束
	case GER_NO_PLAYER:		//没有玩家
		{
			if(m_bGameEnd)
                return true;
			m_bGameEnd=true;

			//定义变量
			net:msg_zajinhua_game_end_rep msg;
	
			//唯一玩家
			WORD wWinner=0,wUserCount=0;
			for(WORD i=0;i<GAME_PLAYER;i++)
			{	
				if(m_cbPlayStatus[i]==TRUE)
				{
					m_szGameEndStatus[i] = TRUE; // add by har
					wUserCount++;
					wWinner=i;
					if(GER_COMPARECARD==reason)
                        ASSERT(m_wBankerUser==i);
					m_wBankerUser=i;
				}
			}
			//胜利者强退
			if(wUserCount==0)
			{
				wWinner=m_wBankerUser;
			}
			//计算总注
			int64 lWinnerScore=0L;
			
			int64 lWinPlayerScore = 0;
			int64 lWinRobotScore = 0;
			int64 playerAllWinScore = 0; // add by har
			for(WORD i=0;i<GAME_PLAYER;i++) 
			{
				if(i==wWinner){
                    msg.add_game_score(0);
                    continue;
				}
                msg.add_game_score(-m_lTableScore[i]);
				lWinnerScore += m_lTableScore[i];
				if (m_cbPlayIsRobot[i] == emPLAYER_IS_USER)
				{
					lWinPlayerScore += m_lTableScore[i];
				}
				if (m_cbPlayIsRobot[i] == emPLAYER_IS_ROBOT)
				{
					lWinRobotScore += m_lTableScore[i];
				}
				CGamePlayer *pGamePlayer = GetPlayer(i);
				if (pGamePlayer != NULL) {
					if (!pGamePlayer->IsRobot())
					    playerAllWinScore -= m_lTableScore[i]; // add by har
					LOG_DEBUG("CGameZajinhuaTable::OnGameEnd0  - roomid:%d,tableid:%d,playerAllWinScore:%lld,m_lTableScore[i]:%lld,i:%d,uid:%d,isRobot:%d",
						GetRoomID(), GetTableID(), playerAllWinScore, m_lTableScore[i], i, pGamePlayer->GetUID(), pGamePlayer->IsRobot());
				}
			}
			if (m_pHostRoom != NULL)
			{
				LOG_DEBUG("CGameZajinhuaTable::OnGameEnd1  - roomid:%d,tableid:%d,m_cbPlayIsRobot:%d,wWinner:%d,lWinnerScore:%lld,lWinRobotScore:%lld,lWinPlayerScore:%lld",
					m_pHostRoom->GetRoomID(), GetTableID(), m_cbPlayIsRobot[wWinner], wWinner, lWinnerScore, lWinRobotScore, lWinPlayerScore);
			}
			int64 fee = 0;
            for(uint16 i=0;i<GAME_PLAYER;++i)
            {
                if(m_szJoinGame[i] == FALSE)
                    continue;
                if(i == wWinner){
                    fee = CalcPlayerInfo(wWinner, lWinnerScore);
                }
            }
			lWinnerScore += fee;
			msg.set_game_score(wWinner, lWinnerScore);
            for(uint16 i = 0;i<GAME_PLAYER;++i)
            {
                net::msg_cards* pCards = msg.add_card_data();
                for(uint16 j=0;j<MAX_COUNT;++j){
                    pCards->add_cards(m_cbHandCardData[i][j]);
                }
            }
            msg.set_end_state(0);

			// 给赢家用户牌型赋值 add by har
			for (uint16 i = 0; i < GAME_PLAYER; ++i) {
				uint8 cardType = 0;
				if (reason == GER_COMPARECARD && i == wWinner)
					cardType = m_gameLogic.GetCardType(m_cbHandCardData[i], MAX_COUNT);
				/*else {
					for (int k = 0; k < GAME_PLAYER; ++k) {
						bool isExit = false;
						for (uint16 j = 0; j < m_wCompardUser[wWinner].size(); ++j) {
							uint16 chairId = m_wCompardUser[wWinner][j];
							if (chairId == i) {
								// TODO: chairId是否最后一轮结束，不是则不加入比牌牌型
								cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairId], MAX_COUNT);
								isExit = true;
								break;
							}
						}
						if (isExit)
							break;
					}
				}*/
				msg.add_card_types(cardType);
			} // add by har end

			//扑克数据
			for(WORD i=0;i<GAME_PLAYER;i++) 
			{                
                msg.clear_compare_user();
                for(uint16 j=0;j<m_wCompardUser[i].size();++j)    
				{
                    msg.add_compare_user(m_wCompardUser[i][j]);
				}
                m_wCompardUser[i].clear();                            
                SendMsgToClient(i,&msg,net::S2C_MSG_ZAJINHUA_GAME_END);
			}
			
			int64 lUpdatePollScore = 0;
			if (m_pHostRoom != NULL && m_cbPlayIsRobot[wWinner]== emPLAYER_IS_ROBOT)
			{
				//int64 fee = -(lWinPlayerScore * m_conf.feeValue / PRO_DENO_10000);
				//lWinPlayerScore += fee;

				lUpdatePollScore = lWinPlayerScore;
				if(lUpdatePollScore != 0)
					m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
				if (m_pHostRoom != NULL && lUpdatePollScore != 0)
				{
					LOG_DEBUG("CGameZajinhuaTable::OnGameEnd02  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
				}

			}
			if (m_pHostRoom != NULL && m_cbPlayIsRobot[wWinner] == emPLAYER_IS_USER)
			{
				int64 fee = -(lWinRobotScore * m_conf.feeValue / PRO_DENO_10000);
				lWinRobotScore += fee;

				lUpdatePollScore = -lWinRobotScore;
				if (lUpdatePollScore != 0)
					m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
				if (m_pHostRoom != NULL && lUpdatePollScore!=0)
				{
					LOG_DEBUG("CGameZajinhuaTable::OnGameEnd3  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
				}
			}
			if (m_pHostRoom != NULL)
			{
				LOG_DEBUG("CGameZajinhuaTable::OnGameEnd4  - roomid:%d,tableid:%d,lUpdatePollScore:%lld,m_cbPlayIsRobot:%d,wWinner:%d,m_wBankerUser:%d,lWinnerScore:%lld,lWinRobotScore:%lld,lWinPlayerScore:%lld,m_cbPlayStatus:%d-%d-%d-%d-%d",
					m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore, m_cbPlayIsRobot[wWinner], wWinner, m_wBankerUser,
					lWinnerScore, lWinRobotScore, lWinPlayerScore, m_cbPlayStatus[0], m_cbPlayStatus[1], m_cbPlayStatus[2], m_cbPlayStatus[3], m_cbPlayStatus[4]);
			}

			//更新幸运值数据   
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
							int64 curr_score = 0L;
							if (i == wWinner)
								curr_score = lWinnerScore;
							else
								curr_score = -m_lTableScore[i];
							pGamePlayer->SetLuckyInfo(GetRoomID(), curr_score);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), curr_score);
						}
					}
				}
			}

			//更新新注册玩家福利数据   
			if (IsNewRegisterWelfareTable())
			{
				//找出当前新注册玩家的位置
				int nrw_chair = INVALID_CHAIR;
				for (uint16 i = 0; i < GAME_PLAYER; ++i)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL && !pGamePlayer->IsRobot())
					{
						nrw_chair = i;
						break;
					}
				}
				LOG_DEBUG("roomid:%d,tableid:%d,nrw_chair:%d wWinner:%d", m_pHostRoom->GetRoomID(), GetTableID(), nrw_chair, wWinner);
				if (nrw_chair != INVALID_CHAIR)
				{
					//获取当前玩家的赢取值
					int64 curr_score = 0L;
					if (nrw_chair == wWinner)
					{
						curr_score = lWinnerScore;							
					}
					else
					{
						curr_score = -m_lTableScore[nrw_chair];
					}

					//判断当前玩家的受控情况
					CGamePlayer * pGamePlayer = GetPlayer(nrw_chair);
					if (pGamePlayer != NULL)
					{
						if(pGamePlayer->GetUID() == m_nrw_ctrl_uid)						
						{
							if (m_nrw_status == 2 && curr_score > 0)
							{
								pGamePlayer->UpdateNRWPlayerScore(curr_score);
								LOG_DEBUG("control current player is lost. but current player is win. uid:%d score:%d", m_nrw_ctrl_uid, curr_score);
							}
							else
							{
								UpdateNewRegisterWelfareInfo(pGamePlayer->GetUID(), curr_score);
							}							
						}
						else
						{
							pGamePlayer->UpdateNRWPlayerScore(curr_score);
						}
					}					
				}				
			}
			
			bool IsWinRobot = true;
			uint32 winUid = 0;
			CGamePlayer *pGameWinPlayer = GetPlayer(wWinner);
            if (pGameWinPlayer != NULL) {
				winUid = pGameWinPlayer->GetUID(); // add by har
				//更新活跃福利数据   
			    if (winUid == m_aw_ctrl_uid)
					UpdateActiveWelfareInfo(m_aw_ctrl_uid, lWinnerScore);
				// 更新库存数据
				IsWinRobot = pGameWinPlayer->IsRobot();
				if (!IsWinRobot)
					playerAllWinScore += lWinnerScore;  // add by har
            }
           
            SendShowCardUser();
            SendSeatInfoToClient();
			WriteJackpotScoreInfo();
			ResetGameEndNoOper(); // add by har
            SaveBlingLog();
				
			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,lWinnerScore:%lld,playerAllWinScore:%lld,winUid:%d,wWinner:%d,IsWinRobot:%d,winfee:%lld",
				GetRoomID(), GetTableID(), lWinnerScore, playerAllWinScore, winUid, wWinner, IsWinRobot, fee);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
			//结束游戏					
            ResetTable();      
            /*m_coolLogic.beginCooling(s_gameEndTime);  delete by har
            m_coolRobot.beginCooling(g_RandGen.RandRange(4000,5000));
			OnTableGameEnd();*/
			return true;
		}break;
	case GER_USER_LEAVE:		//用户强退
	case GER_NETWORK_ERROR:	    //网络中断
		{
		    ResetGameEndNoOper(); // add by har
			if (m_bGameEnd)
			{
				OnTableGameEnd();
				return true;
			}

			//强退处理
			
			bool bret = OnUserGiveUp(chairID);
			OnTableGameEnd();
			return bret;
		}break;
	case GER_OPENCARD:		//开牌结束   
		{
			if(m_bGameEnd)return true;
			m_bGameEnd = true;

			//定义变量
			net::msg_zajinhua_game_end_rep msg;
			//胜利玩家
			WORD wWinner=m_wBankerUser;
			//计算总注
			int64 lWinnerScore=0L;
			int64 lWinPlayerScore = 0;
			int64 lWinRobotScore = 0;
			int64 playerAllWinScore = 0; // add by har
			for(WORD i=0;i<GAME_PLAYER;i++) 
			{
				if (m_cbPlayStatus[i] == TRUE)
					m_szGameEndStatus[i] = TRUE; // add by har
				if(i==wWinner){
                    msg.add_game_score(0);
                    continue;
				}
                msg.add_game_score(-m_lTableScore[i]);				
				lWinnerScore+=m_lTableScore[i];

				if (m_cbPlayIsRobot[i] == emPLAYER_IS_USER)
				{
					lWinPlayerScore += m_lTableScore[i];
				}
				if (m_cbPlayIsRobot[i] == emPLAYER_IS_ROBOT)
				{
					lWinRobotScore += m_lTableScore[i];
				}
				CGamePlayer *pGamePlayer = GetPlayer(i);
				if (pGamePlayer != NULL) {
					if (!pGamePlayer->IsRobot())
						playerAllWinScore -= m_lTableScore[i]; // add by har
				}
			}
			int64 fee = 0;
            for(uint16 i=0;i<GAME_PLAYER;++i)
            {
                if(m_szJoinGame[i] == FALSE)
                    continue;
                if(i == wWinner){
                    fee = CalcPlayerInfo(wWinner, lWinnerScore);
                }
            }
			fee += lWinnerScore;
            msg.set_game_score(wWinner,lWinnerScore);
            for(uint16 i = 0;i<GAME_PLAYER;++i)
            {
                net::msg_cards* pCards = msg.add_card_data();
                for(uint16 j=0;j<MAX_COUNT;++j){
                    pCards->add_cards(m_cbHandCardData[i][j]);
                }                
            }
            msg.set_end_state(1);

			// 给赢家用户牌型赋值 add by har
			for (uint16 i = 0; i < GAME_PLAYER; ++i) {
				uint8 cardType = 0;
				if (i == wWinner)
					cardType = m_gameLogic.GetCardType(m_cbHandCardData[i], MAX_COUNT);
				msg.add_card_types(cardType);
			} // add by har end

		    //扑克数据
			for(WORD i=0;i<GAME_PLAYER;i++) 
			{                
                msg.clear_compare_user();
                for(uint16 j=0;j<m_wCompardUser[i].size();++j)    
				{         
                    msg.add_compare_user(m_wCompardUser[i][j]);
				}
                m_wCompardUser[i].clear();                            
                SendMsgToClient(i,&msg,net::S2C_MSG_ZAJINHUA_GAME_END);
			}

			//更新幸运值数据   
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
							int64 curr_score = 0L;
							if (i == wWinner)
								curr_score = lWinnerScore;
							else
								curr_score = -m_lTableScore[i];
							pGamePlayer->SetLuckyInfo(GetRoomID(), curr_score);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), curr_score);
						}
					}
				}
			}
            SendShowCardUser();
            SendSeatInfoToClient();
			ResetGameEndNoOper(); // add by har
            SaveBlingLog();

			int64 lUpdatePollScore = 0;
			if (m_pHostRoom != NULL && m_cbPlayIsRobot[wWinner] == emPLAYER_IS_ROBOT)
			{
				//int64 fee = -(lWinPlayerScore * m_conf.feeValue / PRO_DENO_10000);
				//lWinPlayerScore += fee;

				lUpdatePollScore = lWinPlayerScore;
				if (lUpdatePollScore != 0)
					m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
				/*if (m_pHostRoom != NULL)
				{
					LOG_DEBUG("update jackpot score  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
				}*/
			}
			if (m_pHostRoom != NULL && m_cbPlayIsRobot[wWinner] == emPLAYER_IS_USER)
			{
				int64 fee = -(lWinRobotScore * m_conf.feeValue / PRO_DENO_10000);
				lWinRobotScore += fee;

				lUpdatePollScore = -lWinRobotScore;
				if (lUpdatePollScore != 0)
					m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
				/*if (m_pHostRoom != NULL)
				{
					LOG_DEBUG("update jackpot score  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
				}*/
			}

			uint32 winUid = 0;
			CGamePlayer *pGameWinPlayer = GetPlayer(wWinner);
			if (pGameWinPlayer != NULL) {
				winUid = pGameWinPlayer->GetUID(); // add by har
				// 更新库存数据
				if (!pGameWinPlayer->IsRobot())
					playerAllWinScore += lWinnerScore;  // add by har
			}
			
			LOG_DEBUG("OnGameEnd2 2 - roomid:%d,tableid:%d,lUpdatePollScore:%lld,wWinner:%d,m_wBankerUser:%d,m_cbPlayIsRobot:%d,winUid:%d,lWinnerScore:%lld,playerAllWinScore:%lld,m_cbPlayStatus:%d-%d-%d-%d-%d",
					GetRoomID(), GetTableID(), lUpdatePollScore, wWinner, m_wBankerUser, m_cbPlayIsRobot[wWinner], winUid, lWinnerScore, playerAllWinScore,
					m_cbPlayStatus[0], m_cbPlayStatus[1], m_cbPlayStatus[2], m_cbPlayStatus[3], m_cbPlayStatus[4]);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har    
			//结束游戏
            ResetTable();
            /*m_coolLogic.beginCooling(s_gameEndTime); delete by har
            m_coolRobot.beginCooling(g_RandGen.RandRange(4000,5000));
			OnTableGameEnd();*/
			return true;
		}break;
	case GER_DISMISS:		//游戏解散
		{
			return true;
		}break;
    default:
        break;
	}
	return false;
}
//用户同意
bool    CGameZajinhuaTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    if(GetReadyNum() == 2 && m_coolLogic.getCoolTick() < 1000){
        m_coolLogic.beginCooling(1500);// 准备后等一秒
    }
    return true;
}
//玩家进入或离开
void  CGameZajinhuaTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);            
    if(isJoin){
        SendGameScene(pPlayer);
    }else{      
        
    }
	// add by har
	if (chairID < GAME_PLAYER) {
		m_szNoOperCount[chairID] = 0;
		m_szNoOperTrun[chairID] = 0;
	} // add by har end
}
// 发送场景信息(断线重连)
void    CGameZajinhuaTable::SendGameScene(CGamePlayer* pPlayer)
{
    uint16 chairID = GetChairID(pPlayer);
	LOG_DEBUG("CGameZajinhuaTable::SendGameScene - roomid:%d,tableid:%d,uid:%d,chairID:%d,isRobot:%d,m_wCurrentRound:%d,GetLookRound:%d,tableState:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, pPlayer->IsRobot(), m_wCurrentRound, GetLookRound(), m_gameState);
	switch(m_gameState)
	{
	case net::TABLE_STATE_ZAJINHUA_FREE: //空闲状态
		{
            net::msg_zajinhua_game_info_free_rep msg;
            
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_ZAJINHUA_GAME_FREE_INFO);     

		}break;
	case net::TABLE_STATE_ZAJINHUA_GAME_END: //结算状态 add by har
	case net::TABLE_STATE_ZAJINHUA_READY_START: // 准备开始开始状态 add by har
    case net::TABLE_STATE_ZAJINHUA_WAIT:
	case net::TABLE_STATE_ZAJINHUA_PLAY:	//游戏状态
		{
            net::msg_zajinhua_game_info_play_rep msg;
			msg.set_game_status(GetGameState()); // add by har
			msg.set_oper_time(m_coolLogic.getCoolTick());
			if (m_gameState == TABLE_STATE_ZAJINHUA_GAME_END || m_gameState == TABLE_STATE_ZAJINHUA_READY_START) {
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_ZAJINHUA_GAME_PLAY_INFO);
				return;
			}
            
            msg.set_current_times(m_lCurrentTimes);
            msg.set_user_max_score(m_lUserMaxScore[chairID]);
            msg.set_banker_user(m_wBankerUser);
            msg.set_current_user(m_wCurrentUser);
            msg.set_current_round(m_wCurrentRound);
            msg.set_compare_state(IsOnlyCompare(chairID));
            msg.set_is_mingpai(m_szMingPai[chairID]);

            for(uint16 i=0;i<GAME_PLAYER;++i)
            {
                msg.add_card_stat(m_szCardState[i]);
                msg.add_table_score(m_lTableScore[i]);
                msg.add_play_status(m_szJoinGame[i]);
            }
            for(uint16 i=0;i<MAX_COUNT;++i)    
            {
                if(m_szCardState[chairID] != emCARD_STATE_NULL){
                    msg.add_hand_card_data(m_cbHandCardData[chairID][i]);
				}else{
                    msg.add_hand_card_data(0);
                }
            }
			msg.set_card_type(m_gameLogic.GetCardType(m_cbHandCardData[chairID], MAX_COUNT)); // add by har
			uint16 roomid = 0;
			if (m_pHostRoom != NULL) {
				roomid = m_pHostRoom->GetRoomID();
			}
			LOG_DEBUG("CGameZajinhuaTable::SendGameScene roomid:%d,tableid:%d,uid:%d,gamestate:%d,m_lCurrentTimes:%d,m_wCurrentUser:%d,m_szJoinGame:%d-%d-%d-%d-%d", 
				roomid, GetTableID(), pPlayer->GetUID(), GetGameState(), m_lCurrentTimes, m_wCurrentUser, 
				m_szJoinGame[0], m_szJoinGame[1], m_szJoinGame[2], m_szJoinGame[3], m_szJoinGame[4]);
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_ZAJINHUA_GAME_PLAY_INFO);
				
		}break;
	}    

}

// add by har
bool CGameZajinhuaTable::IsReady(CGamePlayer* pPlayer) {
	for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
	{
		if (m_vecPlayers[i].pPlayer == pPlayer) {
			return true;
		}
	}
	return false;
}

int64    CGameZajinhuaTable::CalcPlayerInfo(uint16 chairID,int64 winScore)
{
    LOG_DEBUG("report game to lobby:%d  %lld",chairID,winScore);
    uint32 uid = m_vecPlayers[chairID].uid;

	int64 fee = CalcPlayerGameInfo(uid,winScore);
    // 修改扎金花数据
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        uint8 curMaxCard[5];
        pPlayer->GetGameMaxCard(net::GAME_CATE_ZAJINHUA,isCoin,curMaxCard,5);
        BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
        memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
        m_gameLogic.SortCardList(cbUserCardData[chairID],MAX_COUNT);
        if(m_gameLogic.CompareCard(cbUserCardData[chairID],curMaxCard,MAX_COUNT) == false){
            pPlayer->AsyncSetGameMaxCard(net::GAME_CATE_ZAJINHUA,isCoin,cbUserCardData[chairID],MAX_COUNT);            
        }
    }
    // 写入手牌log
    WriteOutCardLog(chairID,m_cbHandCardData[chairID],MAX_COUNT);
	return fee;
}
// 重置游戏数据
void    CGameZajinhuaTable::ResetGameData()
{
	//游戏变量
	m_bOperaCount   = 0;
	m_wCurrentUser  = INVALID_CHAIR;

	//用户状态
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
    ZeroMemory(m_szJoinGame, sizeof(m_szJoinGame));
    ZeroMemory(m_szCardState,sizeof(m_szCardState));
    ZeroMemory(m_szShowCard,sizeof(m_szShowCard));
    ZeroMemory(m_szMingPai, sizeof(m_szMingPai));
    ZeroMemory(m_szMingPaiRound,sizeof(m_szMingPaiRound));
	ZeroMemory(m_szCompareWinUser, sizeof(m_szCompareWinUser));
	
	for(int i=0;i<GAME_PLAYER;i++){
		m_wCompardUser[i].clear();
	}

	//扑克变量
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));

	//下注信息
	m_lCellScore    =   0L;
	m_lCurrentTimes =   0L;	
    m_wCurrentRound =   0;
    m_bAllinState   =   false;
    m_wWinCompareUser = INVALID_CHAIR;

	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));
	ZeroMemory(m_lUserMaxScore,sizeof(m_lUserMaxScore));
    ZeroMemory(m_lAllinScore,sizeof(m_lAllinScore));
	//m_bIsCanLookCard = false;
	//m_wCanLookUser = INVALID_CHAIR;
	//ZeroMemory(m_uAddScoreCount, sizeof(m_uAddScoreCount));
	
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameZajinhuaTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	DealIsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot, m_cbPlayStatus);
}

// 写入出牌log
void    CGameZajinhuaTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
    uint8 cardType = m_gameLogic.GetCardType(cardData,cardCount);
    Json::Value logValue;
    logValue["p"] = chairID;
    logValue["cardtype"] = cardType;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// 写入加注log
void CGameZajinhuaTable::WriteAddScoreLog(uint16 chairID,int64 score)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["s"]       = score;
    logValue["cmp"]     = 0;
    logValue["cp"]       = INVALID_CHAIR;
    logValue["win"]      = INVALID_CHAIR;

    m_operLog["score"].append(logValue);
}
// 写入比牌log
void CGameZajinhuaTable::WriteCompare(uint16 chairID1,uint16 chairID2,uint16 winID,int64 score)
{
    Json::Value logValue;
    logValue["p"]       = chairID1;
    logValue["s"]       = score;
    logValue["cmp"]     = 1;
    logValue["cp"]      = chairID2;
    logValue["win"]     = winID;

    m_operLog["score"].append(logValue);
}
void CGameZajinhuaTable::WriteJackpotScoreInfo()
{
	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	m_operLog["cps"] = tmpJackpotScore.lCurPollScore;
	m_operLog["swp"] = tmpJackpotScore.uSysWinPro;
	m_operLog["slp"] = tmpJackpotScore.uSysLostPro;

}

// 是否能够开始游戏
bool CGameZajinhuaTable::IsCanStartGame()
{
    uint16 bReadCount    = 0;
    uint16 bNotReadCount = 0;
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        if(m_vecPlayers[i].pPlayer != NULL){
			// modify by har
            /*if(m_vecPlayers[i].readyState == 1) {
                bReadCount++;
            }else{
                bNotReadCount++;
            }*/
			++bReadCount; // modify by har end
        }
    }
    /*if(bNotReadCount > 0 && !m_coolLogic.isTimeOut()){ delete by har
        return false;
    }*/
    if(bReadCount >= 2)
        return true;

    return false;
}
// 检测筹码是否正确
bool    CGameZajinhuaTable::CheckJetton(uint16 chairID,int64 score)
{
	uint32 uid = 0;
	if (m_vecPlayers.size() >= chairID) {
		uid = m_vecPlayers[chairID].uid;
	}
	//金币效验
	if (score < 0 || score%m_lCellScore != 0) {
		LOG_WARNING("金币不正确 - uid:%d,score:%lld,m_lCellScore:%lld",uid,score, m_lCellScore);
		return false;
	}
        
	if((score + m_lTableScore[chairID]) > m_lUserMaxScore[chairID]) {
		LOG_WARNING("下注超过最大值 - roomid:%d,tableid:%d,uid:%d,score:%lld,m_lTableScore:%lld,m_lUserMaxScore:%lld",
			GetRoomID(), GetTableID(), uid, score, m_lTableScore[chairID], m_lUserMaxScore[chairID]);
		return false;
    }
	uint32 lTimes = 0;
	uint32 lTemp = score/m_lCellScore;

    if(m_szCardState[chairID] == emCARD_STATE_MING){
        for(int32 i=0;i<getArrayLen(s_JettonMultip2);++i){
            if(lTemp == s_JettonMultip2[i]){
                lTimes = i+1;
                break;
            }
        }
    }else{
        for(int32 i=0;i<getArrayLen(s_JettonMultip);++i){
            if(lTemp == s_JettonMultip[i]){
                lTimes = i+1;
                break;
            }
        }
    }    
	if(!(m_lCurrentTimes <= lTimes)) {
		LOG_WARNING("加注倍数不对 roomid:%d,tableid:%d,uid:%d,m_lCurrentTimes:%d,lTimes:%d,lTemp:%d,m_szCardState[chairID]:%d",
			GetRoomID(), GetTableID(), uid, m_lCurrentTimes, lTimes, lTemp, m_szCardState[chairID]);
        return false;
    }
	m_lCurrentTimes = lTimes;
	//uint32 uid = m_vecPlayers[chairID].uid;

	LOG_DEBUG("jetton check - roomid:%d,tableid:%d,uid:%d,m_lCurrentTimes:%d,lTimes:%d,m_lCellScore:%d,chairID:%d,m_szCardState[chairID]:%d,lTemp:%d",
		GetRoomID(), GetTableID(), uid, m_lCurrentTimes, lTimes, m_lCellScore, chairID, m_szCardState[chairID], lTemp);
    return true;
}
// 计算比牌筹码
int64   CGameZajinhuaTable::CompareJetton(uint16 chairID)
{
    int64 score = 0;
    WORD wPlayerCount = 0;
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			wPlayerCount++;
		}
	}
	int64 lJettonMultip = 0;
    if(m_szCardState[chairID] == emCARD_STATE_MING)
	{
		lJettonMultip = s_JettonMultip2[m_lCurrentTimes - 1];
		score = lJettonMultip*m_lCellScore;
    }
	else
	{
		lJettonMultip = s_JettonMultip[m_lCurrentTimes - 1];
		score = lJettonMultip*m_lCellScore;
    }
	/*if (wPlayerCount > 2)  改成每次比牌消耗筹码和跟注的一样 modify by har
	{
		score *= 2;
	}*/

	LOG_DEBUG("compare_jetton - roomid:%d,tableid:%d,uid:%d,chairID:%d,wPlayerCount:%d,m_szCardState:%d, m_lCurrentTimes:%d, lJettonMultip:%lld, score:%lld",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, wPlayerCount, m_szCardState[chairID], m_lCurrentTimes, lJettonMultip, score);

    return score;
}
int64   CGameZajinhuaTable::CallJetton(uint16 chairID)
{
    int64 score = 0;
    int32 curTimes = m_lCurrentTimes;
    bool  isNeedAddScore = false;
	BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID],MAX_COUNT);
    if(m_lCurrentTimes < 4 &&  g_RandGen.RandRatio(20,100) && cardType >= CT_SHUN_ZI){
        CGamePlayer *pPlayer = GetPlayer(chairID);
        if (pPlayer != NULL && pPlayer->IsRobot()) {
            isNeedAddScore = true;
        }
    }
    if(isNeedAddScore){
        curTimes += g_RandGen.RandRange(1,4-curTimes);
    }
    if(m_szCardState[chairID] == emCARD_STATE_MING){
        score = s_JettonMultip2[curTimes-1]*m_lCellScore;
    }else{
        score = s_JettonMultip[curTimes-1]*m_lCellScore;
    }
    return score;        
}

int64   CGameZajinhuaTable::RobotCallJetton(uint16 chairID)
{
	int64 score = 0;
	uint32 curTimes = m_lCurrentTimes;
	if (curTimes < 1 || curTimes>4)
	{
		curTimes = 1;
	}

	bool bIsNeedCallJetton = false;

	CGamePlayer *pPlayer = GetPlayer(chairID);
	if (pPlayer != NULL && pPlayer->IsRobot())
	{
		bIsNeedCallJetton = true;
	}

	if (bIsNeedCallJetton)
	{
		if (m_szCardState[chairID] == emCARD_STATE_MING)
		{
			score = s_JettonMultip2[curTimes - 1] * m_lCellScore;
		}
		else
		{
			score = s_JettonMultip[curTimes - 1] * m_lCellScore;
		}
	}
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,curTimes:%d,m_lCurrentTimes:%d,score:%lld", GetRoomID(), GetTableID(), GetPlayerID(chairID), curTimes, m_lCurrentTimes, score);
	return score;
}

int64   CGameZajinhuaTable::RobotRaiseJetton(uint16 chairID)
{
	int64 score = 0;
	uint32 curTimes = m_lCurrentTimes;
	if (curTimes < 1 || curTimes>4)
	{
		curTimes = 1;
	}
	bool isNeedAddScore = false;

	CGamePlayer *pPlayer = GetPlayer(chairID);
	if (pPlayer != NULL && pPlayer->IsRobot())
	{
		isNeedAddScore = true;
	}
	if (isNeedAddScore)
	{
		curTimes += g_RandGen.RandRange(1, 4 - curTimes);
		if (curTimes > 4 || curTimes < m_lCurrentTimes)
		{
			curTimes = m_lCurrentTimes;
		}
	}
	if (isNeedAddScore)
	{
		// curTimes = 0 1 2 3
		if (m_szCardState[chairID] == emCARD_STATE_MING)
		{
			score = s_JettonMultip2[curTimes - 1] * m_lCellScore;
		}
		else
		{
			score = s_JettonMultip[curTimes - 1] * m_lCellScore;
		}
	}
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,curTimes:%d,m_lCurrentTimes:%d,score:%lld", GetRoomID(), GetTableID(), GetPlayerID(chairID), curTimes, m_lCurrentTimes, score);
	return score;
}

bool	 CGameZajinhuaTable::OnRobotJettonScore(uint16 chairID, bool bIsAddScore)
{
	bool bIsRobotAllIn = m_bAllinState;

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bIsAddScore:%d,bIsRobotAllIn:%d,chairID:%d,m_szCardState:%d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), bIsAddScore, bIsRobotAllIn, chairID, m_szCardState[chairID]);

	if (bIsRobotAllIn)
	{
		return OnRobotAllIn(chairID);
	}
	if (bIsAddScore)
	{
		return OnUserAddScore(chairID, RobotRaiseJetton(chairID), false, false);
	}
	else
	{
		return OnUserAddScore(chairID, RobotCallJetton(chairID), false, false);
	}
}

bool	 CGameZajinhuaTable::OnRobotAllIn(uint16 chairID)
{
	if (m_szCardState[chairID] != emCARD_STATE_ALLIN)
	{
		return OnUserAllin(chairID);
	}
	else
	{
		return false;
	}
	return false;
}

// 机器人弃牌前看牌
bool	 CGameZajinhuaTable::OnRobotFoldFrontLook(uint16 chairID)
{
	LOG_DEBUG("机器人弃牌前看牌 - roomid:%d,tableid:%d, chairID:%d,uid:%d,m_szMingPai:%d,m_wCurrentRound:%d,GetLookRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), m_szMingPai[chairID], m_wCurrentRound, GetLookRound());

	if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
	{
		return OnUserLookCard(chairID);
	}
	return OnUserGiveUp(chairID);
}

bool	CGameZajinhuaTable::OnRobotNoEnoughScoreCompareCard(uint16 chairID)
{
	//return false;
	if (IsRobotCanCompareCard())
	{
		uint16 wPlayerCount = GetPlayNum();
		int64 compareScore = CompareJetton(chairID);
		int64 lUserScore = m_lUserMaxScore[chairID] - m_lTableScore[chairID];
		int64 lMinScore = (wPlayerCount - 1) * compareScore;
		int64 lMaxScore = (wPlayerCount) * compareScore;
		if (lUserScore >= lMinScore && lUserScore < lMaxScore)
		{
			return OnRobotCompareRand(chairID);
		}
	}
	return false;
}
int64   CGameZajinhuaTable::AllinJetton(uint16 chairID)
{
    int32 diffRound = GetMaxRound() - m_wCurrentRound;
	int64 lJettonMultip = s_JettonMultip2[getArrayLen(s_JettonMultip2) - 1];
	int64 score = lJettonMultip* diffRound*m_lCellScore;
	score = min(score, (m_lUserMaxScore[chairID] - m_lTableScore[chairID]));

	LOG_DEBUG("allin_score - roomid:%d,tableid:%d, chairID:%d,uid:%d,diffRound:%d,m_wCurrentRound:%d,GetMaxRound:%d,score:%lld,m_lCellScore:%lld,lJettonMultip:%lld,m_lUserMaxScore:%dll,m_lTableScore:%lld",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), diffRound, m_wCurrentRound, GetMaxRound(), score, m_lCellScore, lJettonMultip, m_lUserMaxScore[chairID], m_lTableScore[chairID]);

	return score;
}
// 是否只剩比牌操作
bool    CGameZajinhuaTable::IsOnlyCompare(uint16 chairID)
{
	// 改为金币不足明注最大下注数时只剩比牌 modify by har
	/*uint16 wPlayerCount = GetPlayNum();
    int64 score = s_JettonMultip2[getArrayLen(s_JettonMultip2)-1]*2*(wPlayerCount-1)*m_lCellScore;
    if(m_lUserMaxScore[chairID] < (m_lTableScore[chairID] + score))
    {
		LOG_DEBUG("CGameZajinhuaTable::IsOnlyCompare 只剩比牌 roomid:%d,tableid:%d,chairID:%d,wPlayerCount:%d,score:%d,m_lCellScore:%d,m_lUserMaxScore[chairID]:%d,m_lTableScore[chairID]:%d",
			GetRoomID(), GetTableID(), chairID, wPlayerCount, score, m_lCellScore, m_lUserMaxScore[chairID], m_lTableScore[chairID]);
        return true;
    }*/
	if (m_lUserMaxScore[chairID] < m_lTableScore[chairID]+ s_JettonMultip2[getArrayLen(s_JettonMultip2)-1]*m_lCellScore) {
		LOG_DEBUG("CGameZajinhuaTable::IsOnlyCompare 只剩比牌 roomid:%d,tableid:%d,uid:%d,chairID:%d,m_lCurrentTimes:%d,m_lCellScore:%d,m_szCardState[chairID]:%d,m_lUserMaxScore[chairID]:%d,m_lTableScore[chairID]:%d",
			GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, m_lCurrentTimes, m_lCellScore, m_szCardState[chairID], m_lUserMaxScore[chairID], m_lTableScore[chairID]);
		return true;
	} // modify by har end
    return false;
}
// 是否强制开牌
bool    CGameZajinhuaTable::CheckOpenCard()
{
	if (m_wCurrentRound >= GetMaxRound() && m_wCurrentUser != INVALID_CHAIR && m_cbPlayStatus[m_wCurrentUser] != FALSE) {
		if (OnUserOpenCard(m_wCurrentUser))
			return true;
	} else {
		ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus)); // add by har
	}
    return false;
}
void    CGameZajinhuaTable::CheckNewRound(uint16 chairID)
{
    uint16 tmp = 0;
    for(uint16 i=1;i<GAME_PLAYER;++i)
    {
        tmp = (m_wBankerUser+i)%GAME_PLAYER;
        if(m_cbPlayStatus[tmp] == TRUE)
		{
            if(tmp == m_wCurrentUser)
			{
                m_wCurrentRound++;

				LOG_DEBUG("check_round  - roomid:%d,tableid:%d,uid:%d,m_wCurrentRound:%d,m_lCurrentTimes:%d,LookRound:%d,CompareRound:%d,GetMaxRound:%d,i:%d,m_wBankerUser:%d,m_wCurrentUser:%d",
					GetRoomID(), GetTableID(),GetPlayerID(chairID), m_wCurrentRound, m_lCurrentTimes, GetLookRound(), GetCompareRound(), GetMaxRound(), i,m_wBankerUser, m_wCurrentUser);

                return;
            }
			else
			{
                return;
            }
        }
    }
}
uint16  CGameZajinhuaTable::GetCompareRound()
{
    return m_cmpRound;
}
uint16  CGameZajinhuaTable::GetLookRound()
{
    return m_lookRound;
}
uint16  CGameZajinhuaTable::GetMaxRound()   
{
    return m_maxRound;
}
uint16  CGameZajinhuaTable::GetPlayNum()
{
    //人数统计
    WORD wPlayerCount=0;
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        if(m_cbPlayStatus[i]==TRUE)
            wPlayerCount++;
    }
    return wPlayerCount;
}

uint16  CGameZajinhuaTable::GetRealPlayerCount()
{
	//真实人数统计
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && !pPlayer->IsRobot())
		{
			wPlayerCount++;
		}
	}
	return wPlayerCount;
}

int     CGameZajinhuaTable::GetProCardType()
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
		iProIndex = Pro_Index_Single;
	}
	return iProIndex;
}

int     CGameZajinhuaTable::GetWelfareCardType()
{
	int iArrDispatchCardPro[Pro_Index_MAX] = { 0 };
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_iArrWelfareCardPro[i];
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
		iProIndex = Pro_Index_Single;
	}
	return iProIndex;
}


bool	CGameZajinhuaTable::ProbabilityDispatchPokerCard(int type)
{
	bool bIsFlag = true;
	int iArProCardType[GAME_PLAYER] = { 0 };
	// 先确定每一个人获取的类型
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		iArProCardType[i] = Pro_Index_MAX;
		if (m_cbPlayStatus[i] == TRUE)
		{
			//int iProIndex = GetProCardType();
			int iProIndex = Pro_Index_Single;
			if (type == DISPATCH_TYPE_Room)
			{
				iProIndex = GetProCardType();
			}
			else if (type == DISPATCH_TYPE_Welfare)
			{
				iProIndex = GetWelfareCardType();
			}
			if (iProIndex < Pro_Index_MAX)
			{
				iArProCardType[i] = iProIndex;
			}
		}
	}
	// 根据类型获取全部的手牌
	bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType,m_cbHandCardData);

	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		//if (m_cbPlayStatus[i] == TRUE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,type:%d,uid:%d,i:%d,iArProCardType:%d,m_cbPlayStatus:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X",
				GetRoomID(),GetTableID(), type,GetPlayerID(i), i, iArProCardType[i], m_cbPlayStatus[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2]);
		}
	}

	return bIsFlag;
}

bool CGameZajinhuaTable::SetRobotMaxCard()
{
	int pro = m_robotBankerWinPro;
	bool bChange = g_RandGen.RandRatio(pro, PRO_DENO_10000);
	LOG_ERROR("robot max card - roomid:%d,tableid:%d,pro:%d,bChange:%d", GetRoomID(), GetTableID(), pro, bChange);
	if (!bChange)
	{
		return false;
	}
	uint16 maxChairID = GetMaxCardChair(INVALID_CHAIR);
	if (maxChairID == INVALID_CHAIR)
	{
		//LOG_DEBUG("max card is error - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	}
	//LOG_DEBUG("begin max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar != NULL && pTar->IsRobot()) {
		//LOG_DEBUG("max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->IsRobot())
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);
			//LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,maxchairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}
	//LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	return false;

}



CGamePlayer* CGameZajinhuaTable::HaveWelfareNovicePlayer()
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

bool CGameZajinhuaTable::NoviceWelfareCtrlWinScore()
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
			ProbabilityDispatchPokerCard(DISPATCH_TYPE_Welfare);
			bool bRetPlayerWinScore = SetControlPalyerWin(noviceuid);

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
			tagTempValue.frontIsHitWelfare = 0;
			tagTempValue.jettonCount++;
		}
	}

	LOG_DEBUG("dos_wel_ctrl - roomid:%d,tableid:%d,isnewnowe:%d, newmaxjetton:%lld, newsmaxwin:%lld,lNoviceWinScore:%lld, IsNoviceWelfareCtrl:%d, noviceuid:%d,posrmb:%d, ChessWelfare:%d,welfarepro:%d,real_welfarepro:%d,lift_odds:%d,pPlayer:%p, fUseHitWelfare:%d,frontIsHitWelfare:%d,jettonCount:%d,bIsHitWelfarePro:%d",
		GetRoomID(), GetTableID(), isnewnowe, newmaxjetton, newsmaxwin, lNoviceWinScore, bIsNoviceWelfareCtrl, noviceuid, posrmb, GetChessWelfare(), NewPlayerWelfareValue.welfarepro, real_welfarepro, NewPlayerWelfareValue.lift_odds, pPlayer, fUseHitWelfare, tagValue.frontIsHitWelfare, tagValue.jettonCount, bIsHitWelfarePro);

	return bIsNoviceWelfareCtrl;
}


// 做牌发牌
void CGameZajinhuaTable::DispatchCard()
{
	m_gameLogic.RandCardList(m_cbHandCardData[0],sizeof(m_cbHandCardData)/sizeof(m_cbHandCardData[0][0]));

	ProbabilityDispatchPokerCard(DISPATCH_TYPE_Room);

    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        m_gameLogic.SortCardList(m_cbHandCardData[i],MAX_COUNT,false);
    }
	SetRobotMaxCard();

    //控制玩家处理
	bool bIsFalgControl = false;
	bool bIsControlPlayerIsReady = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (WORD i = 0; i<GAME_PLAYER; i++)
		{
			CGamePlayer *pPlayer = GetPlayer(i);
			if (pPlayer == NULL)
				continue;
			if (control_uid == pPlayer->GetUID() && IsReady(pPlayer))
			{
				bIsControlPlayerIsReady = true;
				break;
			}
		}
	}

	if (bIsControlPlayerIsReady && game_count>0 && control_type != GAME_CONTROL_CANCEL)
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

	// 幸运值控制
	bool bLuckyCtrl = false;
	if (!bIsFalgControl)
	{
		bLuckyCtrl = SetLuckyCtrl();
	}

	// 新注册玩家福利控制
	bool bIsNRWControl = false;
	if (!bIsFalgControl && !bLuckyCtrl)
	{
		bIsNRWControl = NewRegisterWelfareCtrl();
	}

	// 新玩家福利
	bool bIsNoviceWelfareCtrl = false;
	if ( !bIsFalgControl && !bLuckyCtrl && !bIsNRWControl)
	{
		bIsNoviceWelfareCtrl = NoviceWelfareCtrlWinScore();
	}

	bool isStockCtrl = false;
	if (!bIsFalgControl && !bLuckyCtrl && !bIsNRWControl && !bIsNoviceWelfareCtrl)
		isStockCtrl = SetStockWinLose();

    // 活跃福利控制
    bool bIsAWControl = false;
    if(!bIsFalgControl && !bLuckyCtrl && !bIsNRWControl && !bIsNoviceWelfareCtrl && !isStockCtrl)
    {
        bIsAWControl = ActiveWelfareCtrl();        
    }
    
	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	bool bIsPoolScoreControl = false;
	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);

	if (!bIsControlPlayerIsReady && !bLuckyCtrl && !bIsAWControl && !bIsNRWControl&& !bIsNoviceWelfareCtrl && !isStockCtrl && tmpJackpotScore.iUserJackpotControl == 1)
	{
		if (tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // 吐币
		{
			bIsPoolScoreControl = true;
			SetPalyerWinScore();
		}
		if (tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // 吃币
		{
			bIsPoolScoreControl = true;
			SetRobotWinScore();
		}
	}
	bool IsHaveAotoKillScore = false;

	//if (!bIsControlPlayerIsReady && !bIsPoolScoreControl)
	//{
	//	IsHaveAotoKillScore = IsHaveAutoKillScorePlayer();
	//	if (IsHaveAotoKillScore)
	//	{
	//		SetRobotWinScore();
	//	}
	//}

	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("conter game card - roomid:%d,tableid:%d,bIsControlPlayerIsReady:%d,bLuckyCtrl:%d bIsAWControl:%d,bIsNRWControl:%d,bIsNoviceWelfareCtrl:%d,bIsSysWinPro:%d,bIsSysLostPro:%d,bIsPoolScoreControl:%d,lCurPollScore:%lld,lMaxPollScore:%lld,lMinPollScore:%lld,isStockCtrl:%d,GetIsAllRobotOrPlayerJetton:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), bIsControlPlayerIsReady, bLuckyCtrl, bIsAWControl, bIsNRWControl, bIsNoviceWelfareCtrl, bIsSysWinPro, bIsSysLostPro, bIsPoolScoreControl, tmpJackpotScore.lCurPollScore, tmpJackpotScore.lMaxPollScore,tmpJackpotScore.lMinPollScore, isStockCtrl, GetIsAllRobotOrPlayerJetton());

    // test toney
    if(false) {
        BYTE temp1[] = {0x01, 0x03, 0x02};
        BYTE temp2[] = {0x07, 0x17, 0x07};
        BYTE temp3[] = {0x15, 0x05, 0x25};
        BYTE temp4[] = {0x27, 0x02, 0x22};
        BYTE temp5[] = {0x16, 0x14, 0x1C};

        for(uint16 i = 0; i < GAME_PLAYER; ++i)
        {
            uint8 testType = g_RandGen.RandUInt() % 7;
            if(testType == 0)
            {
                memcpy(&m_cbHandCardData[i][0], temp1, sizeof(m_cbHandCardData[i]));
            }else if(testType == 1){
                memcpy(&m_cbHandCardData[i][0], temp2, sizeof(m_cbHandCardData[i]));
            }else if(testType == 2){
                memcpy(&m_cbHandCardData[i][0], temp3, sizeof(m_cbHandCardData[i]));
            }else if(testType == 3){
                memcpy(&m_cbHandCardData[i][0], temp4, sizeof(m_cbHandCardData[i]));
            }else if(testType == 4){
                memcpy(&m_cbHandCardData[i][0], temp5, sizeof(m_cbHandCardData[i]));
            }
        }

    }
}

bool    CGameZajinhuaTable::SetControlPalyerWin(uint32 control_uid)
{

	uint16 maxChairID = GetMaxCardChair(INVALID_CHAIR);
	if (maxChairID == INVALID_CHAIR)
	{
		LOG_DEBUG("max card is error - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	}
	LOG_DEBUG("begin max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar != NULL && pTar->GetUID() == control_uid) {
		LOG_DEBUG("max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->GetUID() == control_uid)
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);
			LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,maxchairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	return false;
}



bool    CGameZajinhuaTable::SetControlPalyerLost(uint32 control_uid)
{

	uint16 minChairID = GetMinCardChair(INVALID_CHAIR);
	if (minChairID == INVALID_CHAIR)
	{
		LOG_DEBUG("min card is error - maxChairID:%d,control_uid:%d", minChairID, control_uid);
	}
	CGamePlayer* pTar = GetPlayer(minChairID);
	if (pTar != NULL && pTar->GetUID() == control_uid) {
		LOG_DEBUG("min card is - minChairID:%d,control_uid:%d", minChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->GetUID() == control_uid)
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbHandCardData[i], 5);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], 5);
			memcpy(m_cbHandCardData[minChairID], tmp, 5);
			LOG_DEBUG("changer card success - control_uid:%d,i%d,minChairID:%d", control_uid, i, minChairID);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - minChairID:%d", minChairID);
	return false;
}

bool    CGameZajinhuaTable::SetRobotWinScore()
{

	uint16 maxChairID = GetMaxCardChair(INVALID_CHAIR);
	if (maxChairID == INVALID_CHAIR)
	{
		//LOG_DEBUG("max card is error - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	}
	//LOG_DEBUG("begin max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar != NULL && pTar->IsRobot()) {
		//LOG_DEBUG("max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->IsRobot())
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);
			//LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,maxchairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}
	//LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, control_uid);
	return false;
}

bool    CGameZajinhuaTable::SetPalyerWinScore()
{

	uint16 maxChairID = GetMaxCardChair(INVALID_CHAIR);
	if (maxChairID == INVALID_CHAIR)
	{
		LOG_DEBUG("max card is error - roomid:%d,tableid:%d,maxChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID);
	}
	LOG_DEBUG("begin max card is - roomid:%d,tableid:%d,maxChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar != NULL && !pTar->IsRobot()) {
		LOG_DEBUG("max card is - roomid:%d,tableid:%d,maxChairID:%d,uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID, pTar->GetUID());
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == maxChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[maxChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[maxChairID], tmp, MAX_COUNT);
			LOG_DEBUG("changer card success - roomid:%d,tableid:%d,i%d,maxchairID:%d,uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), i, maxChairID, pTmp->GetUID());
			return true;
		}
	}
	LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID);
	return false;
}

//游戏状态
bool CGameZajinhuaTable::IsUserPlaying(WORD wChairID)
{
	ASSERT(wChairID<GAME_PLAYER);
	return (m_cbPlayStatus[wChairID]==TRUE)?true:false;    
}
//用户放弃
bool CGameZajinhuaTable::OnUserGiveUp(WORD wChairID)
{
	LOG_DEBUG("玩家弃牌 - roomid:%d,tableid:%d,wChairID:%d,uid:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID));
	//设置数据
	//m_cbPlayStatus[wChairID] = FALSE;
    m_szCardState[wChairID]  = emCARD_STATE_QI;

    WriteAddScoreLog(wChairID,-1);

	//发送消息
    net::msg_zajinhua_giveup_rep msg;
    msg.set_giveup_user(wChairID);
    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_GIVEUP);
    
	//修改积分
	int64 fee = CalcPlayerInfo(wChairID,-m_lTableScore[wChairID]);
    
	//CGamePlayer * pUseGamePlayer = GetPlayer(wChairID);
	//int64 lUpdatePollScore = m_lTableScore[wChairID] + fee;
	//if (m_pHostRoom != NULL && lUpdatePollScore != 0 && pUseGamePlayer != NULL && !pUseGamePlayer->IsRobot())
	//{
	//	m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
	//	if (m_pHostRoom != NULL)
	//	{
	//		LOG_DEBUG("update jackpot score  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
	//	}
	//}
	//人数统计
	WORD wPlayerCount = GetPlayNum();
	wPlayerCount -= 1;
	//判断结束
	if(wPlayerCount>=2)
	{
		if (m_wCurrentUser == wChairID)
		{
			OnUserAddScore(wChairID, 0L, true, false);
		}
		else
		{
			m_cbPlayStatus[wChairID] = FALSE;
		}
            
	}else{
		m_cbPlayStatus[wChairID] = FALSE;
	    OnGameEnd(INVALID_CHAIR,GER_NO_PLAYER);
		m_szGameEndStatus[wChairID] = TRUE; // add by har
	}
    
    return true;
}



bool CGameZajinhuaTable::IsCanLookCard()
{
	if (m_wCurrentRound >= GetLookRound())
	{
		return true;
	}
	return false;
}

bool CGameZajinhuaTable::IsCanCompareCard(WORD wChairID)
{
	bool bIsCompare = true;
	if (m_wCurrentRound < GetCompareRound())
	{
		bIsCompare = false;
	}
	if (m_bAllinState)
	{
		if (m_lAllinScore[wChairID] == 0)
		{
			bIsCompare = false;
		}
	}
	return bIsCompare;
}

bool CGameZajinhuaTable::IsRobotCanLookCard()
{
	if (m_wCurrentRound >= GetLookRound())
	{
		return true;
	}
	return false;
}
bool CGameZajinhuaTable::IsRobotCanCompareCard()
{
	if (m_wCurrentRound >= GetCompareRound())
	{
		return true;
	}
	return false;
}


//看牌事件
bool CGameZajinhuaTable::OnUserLookCard(WORD wChairID)
{
	LOG_DEBUG("onuserlookcard - roomid:%d,tableid:%d,uid:%d,m_wCurrentRound:%d,GetLookRound:%d", GetRoomID(), GetTableID(), GetPlayerID(wChairID), m_wCurrentRound, GetLookRound());
	if (m_szCardState[wChairID] == emCARD_STATE_MING)
	{
		return false;
	}

	if (IsCanLookCard() == false)
	{
		LOG_DEBUG("onuserlookcard fail - roomid:%d,tableid:%d,uid:%d,m_wCurrentRound:%d,GetLookRound:%d", GetRoomID(), GetTableID(), GetPlayerID(wChairID), m_wCurrentRound, GetLookRound());
		return false;
	}
	m_szCardState[wChairID] = emCARD_STATE_MING;
    m_szMingPai[wChairID]   = TRUE;
    m_szMingPaiRound[wChairID] = m_wCurrentRound;
    WriteAddScoreLog(wChairID,0);

    net::msg_zajinhua_look_card_rep msg;
    msg.set_look_card_user(wChairID);
	// modify by har
    /*for(uint8 i=0;i<MAX_COUNT;++i)
		msg.add_card_data(m_cbHandCardData[wChairID][i]);
    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_LOOK_CARD);*/
	CGamePlayer* pCurPlayer = NULL;
	for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		if (wChairID == i)
			pCurPlayer = m_vecPlayers[i].pPlayer;
		else
		    if (m_vecPlayers[i].pPlayer != NULL)
		        m_vecPlayers[i].pPlayer->SendMsgToClient(&msg, net::S2C_MSG_ZAJINHUA_LOOK_CARD);
	uint8 cardType = 0;
	if (pCurPlayer != NULL) {
		// 由于GetCardType会改变m_cbHandCardData[wChairID]，必须放到添加牌数据的前面！
		cardType = m_gameLogic.GetCardType(m_cbHandCardData[wChairID], MAX_COUNT);
		for (uint8 i = 0; i < MAX_COUNT; ++i)
			msg.add_card_data(m_cbHandCardData[wChairID][i]);
	    msg.set_card_type(cardType);
		pCurPlayer->SendMsgToClient(&msg, net::S2C_MSG_ZAJINHUA_LOOK_CARD);
	} // modify by har end
	LOG_DEBUG("onuserlookcard success - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentRound:%d,GetLookRound:%d,cardType:%d,m_cbHandCardData[]:%d-%d-%d",
		GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_wCurrentRound, GetLookRound(), cardType,
		m_cbHandCardData[wChairID][0], m_cbHandCardData[wChairID][1], m_cbHandCardData[wChairID][2]);
	
	SetRobotThinkTime();
    return true;
}

// 发送加注消息给客户端 add by har
void CGameZajinhuaTable::SendMsgAddScore(uint16 addScoreUser, int64 addScore, bool addUserIsCompare) {
	net::msg_zajinhua_addscore_rep msg;
	msg.set_current_times(m_lCurrentTimes);
	msg.set_add_score_count(addScore);
	msg.set_add_score_user(addScoreUser);
	msg.set_compare_state(addUserIsCompare ? TRUE : FALSE);
	msg.set_current_user(m_wCurrentUser);
	msg.set_current_round(m_wCurrentRound);
	msg.set_only_compare(IsOnlyCompare(m_wCurrentUser) ? TRUE : FALSE);
	msg.set_is_allin(0);
	SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_ADD_SCORE);
}

// 设置比牌胜利用户和失败用户 add by har
void CGameZajinhuaTable::SetCompareCardWinLoseUser(uint16 wFirstChairID, uint16 wNextChairID, uint16 &wWinUser, uint16 &wLostUser) {
	//比较大小
	uint8 result = m_gameLogic.CompareCard(m_cbHandCardData[wFirstChairID], m_cbHandCardData[wNextChairID], MAX_COUNT);
	if (result == TRUE) {
		wWinUser = wFirstChairID;
		wLostUser = wNextChairID;
	} else {
		wWinUser = wNextChairID;
		wLostUser = wFirstChairID;
	}
	//设置数据
	m_wCompardUser[wLostUser].push_back(wWinUser);
	m_wCompardUser[wWinUser].push_back(wLostUser);
	m_cbPlayStatus[wLostUser] = FALSE;
	m_szGameEndStatus[wLostUser] = TRUE;
	m_szCardState[wLostUser] = emCARD_STATE_SHU;
	m_szCompareWinUser[wWinUser] = 1;
}

// 比牌结束处理 add by har
void CGameZajinhuaTable::CompareCardEnd(uint16 wFirstChairID, uint16 wNextChairID, uint16 wWinUser, uint16 wLostUser, int64 compareScore) {
	WriteCompare(wFirstChairID, wNextChairID, wWinUser, compareScore); 
	if (GetPlayNum() < 2)
		m_wCurrentUser = INVALID_CHAIR;
	net::msg_zajinhua_compare_card_rep msg;
	msg.set_current_user(m_wCurrentUser);
	msg.set_lost_user(wLostUser);
	msg.add_compare_user(wFirstChairID);
	msg.add_compare_user(wNextChairID);

	SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_COMPARE_CARD);
	//修改积分
	CalcPlayerInfo(wLostUser, -m_lTableScore[wLostUser]);

	m_wWinCompareUser = wWinUser;
	SetGameState(TABLE_STATE_ZAJINHUA_WAIT);
	m_coolLogic.beginCooling(2800);
}

// 强制比牌 add by har
void CGameZajinhuaTable::ForeCompareCard() {
	if (CheckOpenCard())
		return;

	if (!IsOnlyCompare(m_wCurrentUser))
		return;

	uint16 wFirstChairID = m_wCurrentUser;
	uint16 wNextChairID = GetNextPlayerChair();

	// 失败和胜利用户
	uint16 wLostUser, wWinUser;
	SetCompareCardWinLoseUser(wFirstChairID, wNextChairID, wWinUser, wLostUser);
	
	m_wCurrentUser = wWinUser;
	if (wFirstChairID != m_wCurrentUser)
		CheckNewRound(m_wCurrentUser);

	SendMsgAddScore(wFirstChairID, 0, true);

	LOG_DEBUG("CGameZajinhuaTable::ForeCompareCard  roomid:%d,tableid:%d,firstUid:%d,nextUid:%d,wFirstChairID=%d,wNextChairID=%d,wWinUser=%d,wLostUser=%d,m_wCurrentUser=%d,m_wCurrentRound=%d", 
		GetRoomID(), GetTableID(), GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID), wFirstChairID, wNextChairID, wWinUser, wLostUser, m_wCurrentUser, m_wCurrentRound);

	CompareCardEnd(wFirstChairID, wNextChairID, wWinUser, wLostUser);
}

// 获取下一个玩家座位 add by har
uint16 CGameZajinhuaTable::GetNextPlayerChair() {
	for (uint16 i = 1; i < GAME_PLAYER; ++i) {
		//设置变量
		uint16 nextChair = (m_wCurrentUser + i) % GAME_PLAYER;
		if (m_cbPlayStatus[nextChair] == TRUE)
			return nextChair;
	}
	return INVALID_CHAIR;
}

//比牌事件
bool CGameZajinhuaTable::OnUserCompareCard(WORD wFirstChairID,WORD wNextChairID)
{
	int64 compareScore = CompareJetton(wFirstChairID);

	bool bIsCompareCard = IsCanCompareCard(wFirstChairID);

	LOG_DEBUG("compare_card_start - roomid:%d,tableid:%d,wFirstChairID:%d-->wNextChairID:%d - uid:%d ->:%d,compareScore:%lld,m_lTableScore:%lld,m_lUserMaxScore:%lld,m_cbPlayStatus:%d,bIsCompareCard:%d",
		GetRoomID(), GetTableID(), wFirstChairID, wNextChairID, GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID), compareScore, m_lTableScore[wFirstChairID], m_lUserMaxScore[wFirstChairID], m_cbPlayStatus[wNextChairID], bIsCompareCard);

    if((compareScore + m_lTableScore[wFirstChairID]) > m_lUserMaxScore[wFirstChairID])
	{
		//比牌积分不足
		LOG_DEBUG("compare_card_fail_score - roomid:%d,tableid:%d,wFirstChairID:%d-->wNextChairID:%d - uid:%d ->:%d,compareScore:%lld,m_lTableScore:%lld,m_lUserMaxScore:%lld,m_cbPlayStatus:%d,bIsCompareCard:%d",
			GetRoomID(), GetTableID(), wFirstChairID, wNextChairID, GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID), compareScore, m_lTableScore[wFirstChairID], m_lUserMaxScore[wFirstChairID], m_cbPlayStatus[wNextChairID], bIsCompareCard);

        return false;
    }
    if(wFirstChairID == wNextChairID || wNextChairID >= GAME_PLAYER || m_cbPlayStatus[wNextChairID] != TRUE || !bIsCompareCard)
	{
		//比牌对象错误
		LOG_DEBUG("compare_card_fail_user - roomid:%d,tableid:%d,wFirstChairID:%d-->wNextChairID:%d - uid:%d ->:%d,compareScore:%lld,m_lTableScore:%lld,m_lUserMaxScore:%lld,m_cbPlayStatus:%d,bIsCompareCard:%d",
			GetRoomID(), GetTableID(), wFirstChairID, wNextChairID, GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID), compareScore, m_lTableScore[wFirstChairID], m_lUserMaxScore[wFirstChairID], m_cbPlayStatus[wNextChairID], bIsCompareCard);

        return false;
    }

	// 失败和胜利用户
	WORD wLostUser, wWinUser;

	// modify by har
	//比较大小
	/*BYTE result = m_gameLogic.CompareCard(m_cbHandCardData[wFirstChairID],m_cbHandCardData[wNextChairID],MAX_COUNT);

	if(result == TRUE)
	{
		wWinUser=wFirstChairID;
		wLostUser=wNextChairID;
	}
	else
	{
		wWinUser=wNextChairID;
		wLostUser=wFirstChairID;
	}

	//设置数据
	m_wCompardUser[wLostUser].push_back(wWinUser);
	m_wCompardUser[wWinUser].push_back(wLostUser);
	m_cbPlayStatus[wLostUser] = FALSE;
	m_szGameEndStatus[wLostUser] = TRUE;
    m_szCardState[wLostUser]  = emCARD_STATE_SHU;
	m_szCompareWinUser[wWinUser] = 1;*/
	SetCompareCardWinLoseUser(wFirstChairID, wNextChairID, wWinUser, wLostUser); // modify by har end

	//人数统计
	//WORD wPlayerCount = GetPlayNum();  // modify by har

    OnUserAddScore(wFirstChairID,compareScore,false,true);
    
	LOG_DEBUG("CGameZajinhuaTable::OnUserCompareCard  roomid:%d,tableid:%d,firstUid:%d,nextUid:%d,wFirstChairID=%d,wNextChairID=%d,wWinUser=%d,wLostUser=%d,m_wCurrentUser=%d,m_wCurrentRound=%d,compareScore:%d,m_cbPlayStatus:%d-%d-%d-%d-%d",
		GetRoomID(), GetTableID(), GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID), wFirstChairID, wNextChairID, wWinUser, 
		wLostUser, m_wCurrentUser, m_wCurrentRound, compareScore, m_cbPlayStatus[0], m_cbPlayStatus[1], m_cbPlayStatus[2], m_cbPlayStatus[3], m_cbPlayStatus[4]);
	// modify by har
	/*WriteCompare(wFirstChairID,wNextChairID,wWinUser,compareScore);
	if (wPlayerCount < 2)
	{
		m_wCurrentUser = INVALID_CHAIR;
	}
	net::msg_zajinhua_compare_card_rep msg;
    msg.set_current_user(m_wCurrentUser);
    msg.set_lost_user(wLostUser);
    msg.add_compare_user(wFirstChairID);
    msg.add_compare_user(wNextChairID);
    
    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_COMPARE_CARD);   
	//修改积分
    int64 fee = CalcPlayerInfo(wLostUser,-m_lTableScore[wLostUser]);
    m_wWinCompareUser = wWinUser;
    SetGameState(TABLE_STATE_ZAJINHUA_WAIT);
    m_coolLogic.beginCooling(2800); */ 
	CompareCardEnd(wFirstChairID, wNextChairID, wWinUser, wLostUser, compareScore); // modify by har end

    return true;
}
//比牌结束
bool CGameZajinhuaTable::OnOverCompareCard()
{
    WORD wPlayerCount = GetPlayNum();
    m_coolLogic.beginCooling(s_AddScoreTime);
    SetRobotThinkTime();
    SetGameState(TABLE_STATE_ZAJINHUA_PLAY);
    //结束游戏
	if(wPlayerCount < 2)
	{
		m_wBankerUser = m_wWinCompareUser;
		OnGameEnd(GAME_PLAYER,GER_COMPARECARD);
        return false;
	}
	//CheckOpenCard(); // delete by har
	ForeCompareCard(); // add by har
    return true;
}
//亮牌
bool CGameZajinhuaTable::OnUserShowCard(WORD wChairID)
{
    if(GetGameState() == TABLE_STATE_ZAJINHUA_PLAY || GetGameState() == TABLE_STATE_ZAJINHUA_WAIT)
    {
        m_szShowCard[wChairID] = TRUE;
        return true;
    }
    net::msg_zajinhua_show_card_rep msg;
    msg.set_show_chairid(wChairID);
    for(uint8 i=0;i<MAX_COUNT;i++)
    {
        msg.add_cards(m_cbHandCardData[wChairID][i]);
    }
    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_SHOW_CARD);
    return true;
}
void CGameZajinhuaTable::SendShowCardUser()
{
    for(uint8 j=0;j<GAME_PLAYER;++j)
    {
        if(m_szShowCard[j] == FALSE)
            continue;

        net::msg_zajinhua_show_card_rep msg;
        msg.set_show_chairid(j);
        for (uint8 i = 0; i < MAX_COUNT; i++) {
            msg.add_cards(m_cbHandCardData[j][i]);
        }
        SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_SHOW_CARD);
    }
}
//开牌事件
bool CGameZajinhuaTable::OnUserOpenCard(WORD wUserID)
{
    LOG_DEBUG("玩家开牌:%d",wUserID);
    
	if (m_wCurrentUser != wUserID)
	{
		ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus)); // add by har
		return false;
	}
	if (m_cbPlayStatus[wUserID] == FALSE)
	{
		ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus)); // add by har
		return false;
	}

	//清理数据
	m_wCurrentUser  = INVALID_CHAIR;

	//保存扑克
	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

	//比牌玩家
	WORD wWinner = wUserID;

	//查找最大玩家
	for(WORD i=1;i < GAME_PLAYER;i++)
	{
		WORD w = (wUserID+i)%GAME_PLAYER;

		//用户过滤
		if(m_cbPlayStatus[w]==FALSE) 
            continue;

		//对比扑克
		if(m_gameLogic.CompareCard(cbUserCardData[w],cbUserCardData[wWinner],MAX_COUNT) >= TRUE)
		{
            m_cbPlayStatus[wWinner] = FALSE;
			m_szGameEndStatus[wWinner] = TRUE; // add by har
            m_szCardState[wWinner]  = emCARD_STATE_SHU;
            int64 fee = CalcPlayerInfo(wWinner,-m_lTableScore[wWinner]);

			//CGamePlayer * pWinGamePlayer = GetPlayer(w);
			//CGamePlayer * pUseGamePlayer = GetPlayer(wWinner);
			//int64 lUpdatePollScore = m_lTableScore[wWinner] + fee;
			//if (m_pHostRoom != NULL && lUpdatePollScore != 0 && pUseGamePlayer != NULL && pWinGamePlayer!=NULL && pWinGamePlayer->IsRobot() && !pUseGamePlayer->IsRobot())
			//{
			//	m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
			//	if (m_pHostRoom != NULL)
			//	{
			//		LOG_DEBUG("update jackpot score  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), lUpdatePollScore);
			//	}
			//}
			//if (m_pHostRoom != NULL && lUpdatePollScore != 0 && pUseGamePlayer != NULL && pWinGamePlayer != NULL && !pWinGamePlayer->IsRobot() && pUseGamePlayer->IsRobot())
			//{
			//	m_pHostRoom->UpdateJackpotScore(-lUpdatePollScore);
			//	if (m_pHostRoom != NULL)
			//	{
			//		LOG_DEBUG("update jackpot score  - roomid:%d,tableid:%d,lUpdatePollScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), -lUpdatePollScore);
			//	}
			//}
            
			wWinner=w;
		}else{
            m_cbPlayStatus[w] = FALSE;
			m_szGameEndStatus[w] = TRUE; // add by har
            m_szCardState[w]  = emCARD_STATE_SHU;
            int64 fee = CalcPlayerInfo(w,-m_lTableScore[w]);

			CGamePlayer * pWinGamePlayer = GetPlayer(wWinner);
			CGamePlayer * pUseGamePlayer = GetPlayer(w);
			int64 lUpdatePollScore = m_lTableScore[w] + fee;
			if (m_pHostRoom != NULL && lUpdatePollScore != 0 && pUseGamePlayer != NULL && pWinGamePlayer != NULL && pWinGamePlayer->IsRobot() && !pUseGamePlayer->IsRobot())
			{
				m_pHostRoom->UpdateJackpotScore(lUpdatePollScore);
			}
			if (m_pHostRoom != NULL && lUpdatePollScore != 0 && pUseGamePlayer != NULL && pWinGamePlayer != NULL && !pWinGamePlayer->IsRobot() && pUseGamePlayer->IsRobot())
			{
				m_pHostRoom->UpdateJackpotScore(-lUpdatePollScore);
			}
        }

        m_wCompardUser[w].push_back(wWinner);
        m_wCompardUser[wWinner].push_back(w);
	}
	if(m_cbPlayStatus[wWinner]==FALSE) {
		ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus)); // add by har
        return false;
    }

	//胜利玩家
	m_wBankerUser = wWinner;

    net::msg_zajinhua_open_card msg;
    msg.set_winner(wWinner);

    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_OPEN_CARD);
    
	OnGameEnd(GAME_PLAYER,GER_OPENCARD);
	return true; 
}
//加注事件
bool CGameZajinhuaTable::OnUserAddScore(WORD wChairID, int64 lScore, bool bGiveUp, bool bCompareCard)
{
	uint32 uid = 0;
	if (m_vecPlayers.size() >= wChairID) {
		uid = m_vecPlayers[wChairID].uid;
	}

	LOG_DEBUG("useraddscore start - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentRound:%d,m_lCurrentTimes:%d,lScore:%lld,bGiveUp:%d,bCompareCard:%d,m_wCurrentUser:%d",
		GetRoomID(), GetTableID(), uid, wChairID, m_wCurrentRound, m_lCurrentTimes, lScore, bGiveUp, bCompareCard, m_wCurrentUser);

	bool isForce = false; // 是否变成强制比牌 add by har
	if (bGiveUp == false)				//设置数据
	{
		//状态效验
		if (m_wCurrentUser != wChairID)
		{
			LOG_WARNING("useraddscore failed 1 - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentUser:%d,lScore:%lld,bGiveUp:%d,bCompareCard:%d",
				GetRoomID(), GetTableID(), uid, wChairID, m_wCurrentUser, lScore, bGiveUp, bCompareCard);
			return false;
		}
		if (!bCompareCard) {
			if (!CheckJetton(wChairID, lScore))
			{
				LOG_WARNING("useraddscore failed 2 - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,bGiveUp:%d,bCompareCard:%d",
					GetRoomID(), GetTableID(), uid, wChairID, lScore, bGiveUp, bCompareCard);
				return false;
			}
		}
		//用户注金
		m_lTableScore[wChairID] += lScore;
		if (!bCompareCard)
			WriteAddScoreLog(wChairID, lScore);
		isForce = IsOnlyCompare(wChairID); // add by har
		if (isForce)
			LOG_DEBUG("加注/比牌后只剩比牌 - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld",
				GetRoomID(), GetTableID(), uid, wChairID, lScore);
	}
	//设置用户
	WORD wPlayerCount = GetPlayNum();
	if (bGiveUp)
	{
		wPlayerCount -= 1;
	}
	//用户切换
	//WORD wNextPlayer = INVALID_CHAIR;
    if(wPlayerCount >= 2)
    {
    	/*for(WORD i=1;i<GAME_PLAYER;i++)  delete by har
    	{
    		//设置变量
    		wNextPlayer=(m_wCurrentUser+i)%GAME_PLAYER;
    		if(m_cbPlayStatus[wNextPlayer]==TRUE)
                break;
    	}
        m_wCurrentUser = wNextPlayer;*/ 
		m_wCurrentUser = GetNextPlayerChair(); // add by har
    }
    //CheckNewRound(wChairID); // delete by har

	if (bGiveUp) {
		m_cbPlayStatus[wChairID] = FALSE;
		m_szGameEndStatus[wChairID] = TRUE; // add by har
	}

	// modify by har
	if (isForce && m_cbPlayStatus[wChairID] == TRUE) // 必须判断 m_cbPlayStatus[wChairID]==TRUE，因为wChairID有可能比牌已经比输了！
		m_wCurrentUser = wChairID; 
	else
		CheckNewRound(wChairID);

	uint32 lastCompare = false;
	if (bCompareCard || isForce)
		lastCompare = true;
	/*net::msg_zajinhua_addscore_rep msg;
	msg.set_current_times(m_lCurrentTimes);
	msg.set_current_user(m_wCurrentUser);
	msg.set_add_score_count(lScore);
	msg.set_add_score_user(wChairID);
	msg.set_compare_state((bCompareCard) ? TRUE : FALSE);
	msg.set_current_round(m_wCurrentRound);
	msg.set_only_compare(IsOnlyCompare(m_wCurrentUser));
	msg.set_is_allin(0);
	SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_ADD_SCORE);*/ 
	SendMsgAddScore(wChairID, lScore, lastCompare); // modify by har end

    m_coolLogic.beginCooling(s_AddScoreTime);
    SetRobotThinkTime();

	NotifyChangeScoreValueInGame(uid, lScore, 0, 0);


	LOG_DEBUG("useraddscore check_round - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentRound:%d,m_lCurrentTimes:%d,lScore:%lld,bGiveUp:%d,bCompareCard:%d,m_wCurrentUser:%d",
		GetRoomID(), GetTableID(), uid, wChairID, m_wCurrentRound, m_lCurrentTimes, lScore, bGiveUp, bCompareCard, m_wCurrentUser);


	//if (lScore > 0)
	//{
	//	m_uAddScoreCount[wChairID]++;
	//}
	//else if (m_szCardState[wChairID] == emCARD_STATE_QI && lScore == 0)
	//{
	//	m_uAddScoreCount[wChairID]++;
	//}

	//if (m_wCanLookUser == wChairID && m_uAddScoreCount[wChairID] >= GetLookRound())
	//{
	//	m_bIsCanLookCard = true;
	//}

	//if (m_wCurrentRound >= GetLookRound())
	//{
	//	m_bIsCanLookCard = true;
	//}

	//LOG_DEBUG("useraddscore - uid:%d,wChairID:%d,m_wCanLookUser:%d,m_bIsCanLookCard:%d,GetLookRound():%d,m_uAddScoreCount[wChairID]:%d",
	//	uid, wChairID, m_wCanLookUser, m_bIsCanLookCard, GetLookRound(), m_uAddScoreCount[wChairID]);

	
	//if (m_bIsCanLookCard == false && m_szCardState[m_wCanLookUser] == emCARD_STATE_QI && lScore == 0)
	//{
	//	LOG_DEBUG("usergiveup_1 - uid:%d,wChairID:%d,m_wCanLookUser:%d,lScore:%lld,bGiveUp:%d,wPlayerCount:%d", uid, wChairID, m_wCanLookUser, lScore, bGiveUp, wPlayerCount);
	//	//之前是一庄家下注几轮为标准 如果庄家弃牌就以上一个用户为标准
	//	if (wPlayerCount >= 2)
	//	{
	//		uint16 wCanLookUser = GetFrontUser(m_wCanLookUser);
	//		//uint16 wCanLookNext = GetNextUser(wCanLookUser);
	//		if (wCanLookUser < GAME_PLAYER && m_cbPlayStatus[wCanLookUser] == TRUE && m_wCanLookUser != wCanLookUser)
	//		{
	//			m_wCanLookUser = wCanLookUser;
	//		}
	//		LOG_DEBUG("usergiveup_2 - uid:%d,wChairID:%d,m_wCanLookUser:%d,m_cbPlayStatus[wCanLookUser]:%d,wCanLookUser:%d", uid, wChairID, m_wCanLookUser, m_cbPlayStatus[wCanLookUser], wCanLookUser);
	//	}
	//}

	if (bCompareCard && !bGiveUp)
		return true; // add by har

	if (!bCompareCard)
		ForeCompareCard(); // add by har
		//CheckOpenCard(); // delete by har
	else
		ZeroMemory(m_szGameEndStatus, sizeof(m_szGameEndStatus)); // add by har

	return true;    
}

uint16 CGameZajinhuaTable::GetNextUser(WORD chairID)
{
	uint32 uNextLoopCount = 0;
	uint16 uNextUser = (chairID + 1) % GAME_PLAYER;
	while (m_cbPlayStatus[uNextUser] == FALSE)
	{
		uNextLoopCount++;
		uNextUser = (uNextUser + 1) % GAME_PLAYER;
		if (uNextLoopCount >= GAME_PLAYER + GAME_PLAYER)
		{
			// 第五个循环完找不到庄家别开始了
			break;
		}
	}
	return uNextUser;
}

uint16 CGameZajinhuaTable::GetFrontUser(WORD chairID)
{
	uint16 uFrontUser = INVALID_CHAIR;
	if (chairID == 0)
	{
		uFrontUser = GAME_PLAYER - 1;
	}
	else
	{
		uFrontUser = chairID - 1;
	}

	uint32 uLoopCount = 0;
	while (m_cbPlayStatus[uFrontUser] == FALSE)
	{
		if (uFrontUser == 0)
		{
			uFrontUser = GAME_PLAYER - 1;
		}
		else
		{
			uFrontUser = uFrontUser - 1;
		}

		uLoopCount++;
		if (uLoopCount >= GAME_PLAYER + GAME_PLAYER)
		{
			// 第五个循环完找不到庄家别开始了
			break;
		}
	}


	return uFrontUser;
}

//全压
bool CGameZajinhuaTable::OnUserAllin(WORD chairID)
{
    int64 lScore = AllinJetton(chairID);
	WORD wPlayerCount = GetPlayNum();

	LOG_DEBUG("user_allin - roomid:%d,tableid:%d,uid:%d,chairID:%d,lScore:%lld,m_wCurrentUser:%d,wPlayerCount:%d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, lScore, m_wCurrentUser, wPlayerCount);

    //状态效验
	if (m_wCurrentUser != chairID)
	{
		LOG_WARNING("user_allin，不是当前用户 - roomid:%d,tableid:%d,uid:%d,chairID:%d,m_wCurrentUser:%d",
			GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, m_wCurrentUser);
		return false;
	}
    //设置用户    
	if (wPlayerCount > 2)
	{
		LOG_WARNING("user_allin，游戏人数不对 - roomid:%d,tableid:%d,uid:%d,wPlayerCount:%d, m_cbPlayStatus:%d-%d-%d-%d-%d",
			GetRoomID(), GetTableID(), GetPlayerID(chairID), wPlayerCount, m_cbPlayStatus[0], 
			m_cbPlayStatus[1], m_cbPlayStatus[2], m_cbPlayStatus[3], m_cbPlayStatus[4]);
		return false;
	}

	//检测是否需要发送比牌包
	uint16 wFirstChairID = INVALID_CHAIR;
	for(uint16 i=0;i<GAME_PLAYER;++i)
		if(m_lAllinScore[i] != 0 && i != chairID) {
			wFirstChairID = i;
			lScore = m_lAllinScore[i]; // add by har
			LOG_DEBUG("user_allin lScore change - roomid:%d,tableid:%d,wFirstChairID:%d,lScore:%lld",
				GetRoomID(), GetTableID(), wFirstChairID, lScore);
			break;
		}
	//用户切换
	WORD wNextPlayer = INVALID_CHAIR;
	if (wPlayerCount >= 2) {
		for (WORD i = 1; i < GAME_PLAYER; ++i) {
			//设置变量
			wNextPlayer = (m_wCurrentUser + i) % GAME_PLAYER;
			if (m_cbPlayStatus[wNextPlayer] == TRUE)
				break;
		}
		m_wCurrentUser = wNextPlayer;
	}
	if (wFirstChairID == INVALID_CHAIR) {
		int64 lNScore = AllinJetton(m_wCurrentUser);
		if (lNScore < lScore) {
			lScore = lNScore; // add by har
			LOG_DEBUG("user_allin lScore change - roomid:%d,tableid:%d,m_wCurrentUser:%d,lScore:%lld",
				GetRoomID(), GetTableID(), m_wCurrentUser, lScore);
		}
	}

    //用户注金
    m_lTableScore[chairID] += lScore;
    m_lAllinScore[chairID] = lScore;
    m_szCardState[chairID] = emCARD_STATE_ALLIN;
    m_bAllinState = true;
    WriteAddScoreLog(chairID,lScore);

    CheckNewRound(chairID);

    net::msg_zajinhua_addscore_rep msg;
    msg.set_current_times(m_lCurrentTimes);
    msg.set_current_user(m_wCurrentUser);
    msg.set_add_score_count(lScore);
    msg.set_add_score_user(chairID);
    msg.set_compare_state(( FALSE ));
    msg.set_current_round(m_wCurrentRound);
    msg.set_only_compare(IsOnlyCompare(m_wCurrentUser));
    msg.set_is_allin(1);
    SendMsgToAll(&msg,net::S2C_MSG_ZAJINHUA_ADD_SCORE);

	uint16 wNextChairID = chairID;
    if(wFirstChairID != INVALID_CHAIR) 
    {
        //比较大小
        BYTE result = m_gameLogic.CompareCard(m_cbHandCardData[wFirstChairID], m_cbHandCardData[wNextChairID],MAX_COUNT);

        //胜利用户
        WORD wLostUser, wWinUser;
        if(result == TRUE){
            wWinUser = wFirstChairID;
            wLostUser = wNextChairID;
        }else{
            wWinUser = wNextChairID;
            wLostUser = wFirstChairID;
        }

        //设置数据
        m_wCompardUser[wLostUser].push_back(wWinUser);
        m_wCompardUser[wWinUser].push_back(wLostUser);
        m_cbPlayStatus[wLostUser] = FALSE;
		m_szGameEndStatus[wLostUser] = TRUE; // add by har
        m_szCardState[wLostUser] = emCARD_STATE_SHU;

        //人数统计
        WORD wPlayerCount = 0;
        for (WORD i = 0; i < GAME_PLAYER; i++) {
            if (m_cbPlayStatus[i] == TRUE)
                wPlayerCount++;
        }
        if (wPlayerCount < 2)
            m_wCurrentUser = INVALID_CHAIR;

        net::msg_zajinhua_compare_card_rep msg;
        msg.set_current_user(m_wCurrentUser);
        msg.set_lost_user(wLostUser);
        msg.add_compare_user(wFirstChairID);
        msg.add_compare_user(wNextChairID);

        SendMsgToAll(&msg, net::S2C_MSG_ZAJINHUA_COMPARE_CARD);
        //修改积分
        int64 fee = CalcPlayerInfo(wLostUser, -m_lTableScore[wLostUser]);

        m_wWinCompareUser = wWinUser;
		LOG_DEBUG("allin玩家比牌 - roomid:%d,tableid:%d,wFirstChairID:%d,wNextChairID:%d,uid:%d,nextuid:%d,wWinUser:%d,wPlayerCount:%d,m_wCurrentUser:%d,fee:%d",
			GetRoomID(), GetTableID(), wFirstChairID, wNextChairID, GetPlayerID(wFirstChairID), GetPlayerID(wNextChairID),
			wWinUser, wPlayerCount, m_wCurrentUser, fee);
        SetGameState(TABLE_STATE_ZAJINHUA_WAIT);
        m_coolLogic.beginCooling(2800);
    }else{
        m_coolLogic.beginCooling(s_AddScoreTime);
        SetRobotThinkTime();
    }
    return true;
}

uint16	CGameZajinhuaTable::RealPalyerCount(uint16 chairID)
{
	uint16 count = 0;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			count++;
		}
	}
	return count;
}

// 有真实玩家
bool	CGameZajinhuaTable::HaveRealPalyer(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			return true;
		}
	}
	return false;
}

// 是否有真实玩家比当前的牌大
bool	CGameZajinhuaTable::HavePalyerBigger(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			if (m_gameLogic.CompareCard(m_cbHandCardData[i], m_cbHandCardData[chairID], MAX_COUNT) == TRUE)
			{
				//有真实玩家比这个用户达
				return true;
			}
		}
	}
	return false;
}

bool	CGameZajinhuaTable::HavePalyerWinScore(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			int64 daywin = 0;
			if (m_pHostRoom != NULL)
			{
				daywin = pTmp->GetPlayerDayWin(m_pHostRoom->GetGameType());
			}
			if (daywin > 0)
			{
				return true;
			}
		}
	}
	return false;
}

bool	CGameZajinhuaTable::HavePalyerNoWinScore(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			int64 daywin = 0;
			if (m_pHostRoom != NULL)
			{
				daywin = pTmp->GetPlayerDayWin(m_pHostRoom->GetGameType());
			}
			if (daywin <= 0)
			{
				return true;
			}
		}
	}
	return false;
}


// 是否有真实玩家比当前的牌大
bool	CGameZajinhuaTable::HavePalyerBiggerWinScore(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			int64 daywin = 0;
			if (m_pHostRoom != NULL)
			{
				daywin = pTmp->GetPlayerDayWin(m_pHostRoom->GetGameType());
			}
			if (daywin > 0 && m_gameLogic.CompareCard(m_cbHandCardData[i], m_cbHandCardData[chairID], MAX_COUNT) == TRUE)
			{
				//有真实玩家比这个用户大
				return true;
			}
		}
	}
	return false;
}

bool	CGameZajinhuaTable::HavePalyerBiggerNoWinScore(uint16 chairID)
{
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && !pTmp->IsRobot())
		{
			int64 daywin = 0;
			if (m_pHostRoom != NULL)
			{
				daywin = pTmp->GetPlayerDayWin(m_pHostRoom->GetGameType());
			}
			if (daywin <= 0 && m_gameLogic.CompareCard(m_cbHandCardData[i], m_cbHandCardData[chairID], MAX_COUNT) == TRUE)
			{
				//有真实玩家比这个用户大
				return true;
			}
		}
	}
	return false;
}

bool  CGameZajinhuaTable::HavePlayerLookCard(uint16 chairID)
{
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && !pPlayer->IsRobot() && m_szMingPai[i] == TRUE)
		{
			return true;
		}
	}
	return false;
}

bool  CGameZajinhuaTable::HavePlayerNoLookCard(uint16 chairID)
{
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && !pPlayer->IsRobot() && m_szMingPai[i] == FALSE)
		{
			return true;
		}
	}
	return false;
}

bool  CGameZajinhuaTable::HavePalyerAllIn(uint16 chairID)
{
	WORD wPlayerCount = 0;
	for (WORD i = 0; i<GAME_PLAYER; i++)
	{
		if (m_cbPlayStatus[i] == FALSE || i == chairID)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && !pPlayer->IsRobot() && m_lAllinScore[i] != 0)
		{
			return true;
		}
	}
	return false;
}


bool	CGameZajinhuaTable::RobotSingleOperCard(uint16 chairID)
{
	uint16 max_chairID = GetMaxCardChair(chairID);
	BYTE cbSingValue = m_gameLogic.GetSingCardValue(m_cbHandCardData[chairID], MAX_COUNT);


	LOG_DEBUG("机器人单牌操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,max_chairID:%d,m_bAllinState:%d,cbSingValue:%d,HavePlayerNoLookCard:%d,HavePlayerLookCard:%d,RealPalyerCount:%d,m_wCurrentRound:%d,GetLookRound:%d,GetCompareRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), max_chairID, m_bAllinState, cbSingValue, HavePlayerNoLookCard(chairID), HavePlayerLookCard(chairID), RealPalyerCount(chairID), m_wCurrentRound, GetLookRound(), GetCompareRound());

	if (m_bAllinState)
	{
		if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (max_chairID == chairID)
		{
			return OnRobotAllIn(chairID);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	if (OnRobotNoEnoughScoreCompareCard(chairID))
	{
		return true;
	}
	// 同桌有机器人比玩家牌型大时（即玩家的牌不是最大时），其他机器人可以加注，加注概率为50%
	if (IsRobotCanLookCard() == false)
	{
		// 如果未能看牌的时候不能弃牌
		bool bIsRobotRandAddScore = false;
		if (HavePalyerBigger(chairID) == false)
		{
			if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(10, PRO_DENO_100))
			{
				bIsRobotRandAddScore = true;
			}
			if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
			{
				bIsRobotRandAddScore = true;
			}
		}
		return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
	}
	else
	{
		if (cbSingValue < 14)
		{
			if (HavePlayerNoLookCard(chairID))
			{
				if (IsRobotCanCompareCard())
				{
					if (RealPalyerCount(chairID) == 1)
					{
						return OnRobotCompareRand(chairID);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
				if (cbSingValue <= 11)
				{
					return OnRobotFoldFrontLook(chairID);
				}
				else
				{
					if (g_RandGen.RandRatio(80, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if(m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else if (HavePlayerLookCard(chairID))
			{
				if (cbSingValue <= 11)
				{
					return OnRobotFoldFrontLook(chairID);
				}
				else
				{
					if (IsRobotCanCompareCard())
					{
						if (RealPalyerCount(chairID) == 1)
						{
							return OnRobotCompareRand(chairID);
						}
						else
						{
							return OnRobotFoldFrontLook(chairID);
						}
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else
			{
				return OnRobotFoldFrontLook(chairID);
			}
		}
		else
		{// cbSingValue == 14
			if (HavePlayerNoLookCard(chairID))
			{
				bool bIsRobotLookCard = false;
				if (m_wCurrentRound == GetLookRound())
				{
					if (g_RandGen.RandRatio(50, PRO_DENO_100))
					{
						bIsRobotLookCard = true;
					}
				}
				else
				{
					bIsRobotLookCard = true;
				}
				if (bIsRobotLookCard && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				if (IsRobotCanCompareCard())
				{
					return OnRobotCompareRand(chairID);
				}
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else if (HavePlayerLookCard(chairID))
			{
				if (m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				if (m_wCurrentRound == GetLookRound())
				{
					if (g_RandGen.RandRatio(30, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
				else
				{
					//第二圈玩家2个人以内跟注则机器人比牌，2个人及以上弃牌，就算比对方大，也弃牌
					if (RealPalyerCount(chairID) == 1 && IsRobotCanCompareCard())
					{
						return OnRobotCompareRobot(chairID);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bool bIsRobotRandAddScore = false;
					if (max_chairID == chairID)
					{
						if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
						if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
					}
					return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
		}
		return false;
	}

	return false;
}

bool	CGameZajinhuaTable::RobotDoubleOperCard(uint16 chairID)
{
	uint16 max_chairID = GetMaxCardChair(chairID);
	uint16 uRealPlayerCount = GetRealPlayerCount();
	BYTE cbDoubleValue = m_gameLogic.GetDoubleValue(m_cbHandCardData[chairID], MAX_COUNT);

	LOG_DEBUG("机器人对子操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,max_chairID:%d,m_bAllinState:%d,cbDoubleValue:%d,HavePlayerNoLookCard:%d,HavePlayerLookCard:%d,uRealPlayerCount:%d,HavePalyerWinScore:%d,HavePalyerNoWinScore:%d,m_wCurrentRound:%d,GetLookRound:%d,GetCompareRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), max_chairID, m_bAllinState,cbDoubleValue, HavePlayerNoLookCard(chairID), HavePlayerLookCard(chairID), uRealPlayerCount, HavePalyerWinScore(chairID), HavePalyerNoWinScore(chairID), m_wCurrentRound, GetLookRound(), GetCompareRound());

	if (m_bAllinState)
	{
		if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (max_chairID == chairID)
		{
			return OnRobotAllIn(chairID);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	if (OnRobotNoEnoughScoreCompareCard(chairID))
	{
		return true;
	}
	if (IsRobotCanLookCard() == false)
	{
		// 如果未能看牌的时候不能弃牌
		bool bIsRobotRandAddScore = false;
		if (HavePalyerBigger(chairID) == false)
		{
			if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(10, PRO_DENO_100))
			{
				bIsRobotRandAddScore = true;
			}
			if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
			{
				bIsRobotRandAddScore = true;
			}
		}
		return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
	}

	if (cbDoubleValue > 0 && cbDoubleValue <= 6)
	{
		if (HavePlayerNoLookCard(chairID))
		{
			//跟注概率：100%，固定闷牌圈数后最多闷2 - 4圈后开启看牌比牌模式
			bool bIsLook = false;
			bool bIsCompare = false;
			if (m_wCurrentRound >= GetLookRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsLook = true;
				}
			}
			if (m_wCurrentRound >= GetCompareRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsCompare = true;
				}
			}
			if ((m_wCurrentRound >= GetLookRound() + 3))
			{
				bIsLook = true;
			}
			if ((m_wCurrentRound >= GetCompareRound() + 3))
			{
				bIsCompare = true;
			}

			if (bIsLook && IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			if (bIsCompare)
			{
				return OnRobotCompareRand(chairID);
			}
			bool bIsRobotRandAddScore = false;
			if (max_chairID == chairID)
			{
				if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
				if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
			}
			return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
		}
		else if (HavePlayerLookCard(chairID))
		{
			if (m_wCurrentRound == GetLookRound())
			{
				// 第一圈看牌概率70%，跟注
				bool bIsLook = false;
				if (g_RandGen.RandRatio(70, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else if (m_wCurrentRound == GetLookRound() + 1)
			{
				// 第二圈看牌概率100%，同桌2个玩家以内跟注概率100%，同桌3个玩家及以上跟注概率50%，另有50%弃牌
				bool bIsLook = false;
				if (g_RandGen.RandRatio(100, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				if (uRealPlayerCount <= 2)
				{
					bool bIsRobotRandAddScore = false;
					if (max_chairID == chairID)
					{
						if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
						if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
					}
					return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
				}
				else
				{
					if (g_RandGen.RandRatio(50, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else
			{
				//  if (m_wCurrentRound == GetLookRound() + 3)
				// 第三圈，判断所有玩家之中是否有一个当天闷三张是否游戏赢金币
				if (HavePalyerWinScore(chairID))
				{
					// 有玩家赢金币，开启透视看牌 - 机器人比玩家牌大，跟住到底 - 机器人比玩家牌小，弃牌
					if (max_chairID == chairID)
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
				else if (HavePalyerNoWinScore(chairID))
				{
					// 没有玩家赢金币，判断跟住玩家人数 - 2人及以上，弃牌 - 1人，直接进入比牌 - 只剩下机器人，牌型小的机器人弃牌
					if (uRealPlayerCount == 1 && IsRobotCanCompareCard())
					{
						return OnRobotCompareRand(chairID);
					}
					else if (uRealPlayerCount >= 2)
					{
						return OnRobotFoldFrontLook(chairID);
					}
					else
					{
						if (max_chairID == chairID)
						{
							bool bIsRobotRandAddScore = false;
							if (max_chairID == chairID)
							{
								if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
								if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
							}
							return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
						}
						else
						{
							return OnRobotFoldFrontLook(chairID);
						}
					}
				}
				else
				{
					if (max_chairID == chairID)
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
		}
		else
		{
			if (max_chairID == chairID)
			{
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else
			{
				return OnRobotFoldFrontLook(chairID);
			}
		}
	}
	else if (cbDoubleValue > 6 && cbDoubleValue <= 10)
	{
		if (HavePlayerNoLookCard(chairID))
		{
			//跟注概率：100%，固定闷牌圈数后最多闷2 - 4圈后开启看牌比牌模式
			bool bIsLook = false;
			bool bIsCompare = false;

			if (m_wCurrentRound >= GetLookRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsLook = true;
				}
			}
			if (m_wCurrentRound >= GetCompareRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsCompare = true;
				}
			}
			if ((m_wCurrentRound >= GetLookRound() + 3))
			{
				bIsLook = true;
			}
			if ((m_wCurrentRound >= GetCompareRound() + 3))
			{
				bIsCompare = true;
			}

			if (bIsLook && IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			if (bIsCompare)
			{
				return OnRobotCompareRand(chairID);
			}
			bool bIsRobotRandAddScore = false;
			if (max_chairID == chairID)
			{
				if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
				if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
			}
			return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
		}
		else if (HavePlayerLookCard(chairID))
		{
			if (m_wCurrentRound == GetLookRound())
			{
				// 第一圈看牌概率50%，跟注
				bool bIsLook = false;
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else if (m_wCurrentRound == GetLookRound() + 1)
			{
				// 第二圈看牌概率100%，同桌2个玩家以内跟注概率100%，同桌3个玩家及以上跟注概率60%，另有40%弃牌
				bool bIsLook = false;
				if (g_RandGen.RandRatio(100, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				if (uRealPlayerCount <= 2)
				{
					bool bIsRobotRandAddScore = false;
					if (max_chairID == chairID)
					{
						if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
						if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
						{
							bIsRobotRandAddScore = true;
						}
					}
					return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
				}
				else
				{
					if (g_RandGen.RandRatio(60, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else
			{
				//  if (m_wCurrentRound == GetLookRound() + 3)
				// 第三圈，判断所有玩家之中是否有一个当天闷三张是否游戏赢金币
				if (HavePalyerWinScore(chairID))
				{
					// 有玩家赢金币，开启透视看牌 - 机器人比玩家牌大，跟住到底 - 机器人比玩家牌小，弃牌
					if (max_chairID == chairID)
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
				else if (HavePalyerNoWinScore(chairID))
				{
					// 没有玩家赢金币，判断跟住玩家人数 - 2人及以上，弃牌 - 1人，直接进入比牌 - 只剩下机器人，牌型小的机器人弃牌
					if (uRealPlayerCount == 1 && IsRobotCanCompareCard())
					{
						return OnRobotCompareRand(chairID);
					}
					else if (uRealPlayerCount >= 2)
					{
						return OnRobotFoldFrontLook(chairID);
					}
					else
					{
						if (max_chairID == chairID)
						{
							bool bIsRobotRandAddScore = false;
							if (max_chairID == chairID)
							{
								if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
								if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
							}
							return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
						}
						else
						{
							return OnRobotFoldFrontLook(chairID);
						}
					}
				}
				else
				{
					if (max_chairID == chairID)
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
		}
		else
		{
			if (max_chairID == chairID)
			{
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else
			{
				return OnRobotFoldFrontLook(chairID);
			}
		}
	}
	else if (cbDoubleValue > 10 && cbDoubleValue <= 14)
	{
		if (HavePlayerNoLookCard(chairID))
		{
			//跟注概率：100%，固定闷牌圈数后最多闷2 - 4圈后开启看牌比牌模式
			bool bIsLook = false;
			bool bIsCompare = false;

			if (m_wCurrentRound >= GetLookRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsLook = true;
				}
			}
			if (m_wCurrentRound >= GetCompareRound() + 1)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsCompare = true;
				}
			}
			if ((m_wCurrentRound >= GetLookRound() + 3))
			{
				bIsLook = true;
			}
			if ((m_wCurrentRound >= GetCompareRound() + 3))
			{
				bIsCompare = true;
			}

			if (bIsLook && IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			if (bIsCompare)
			{
				return OnRobotCompareRand(chairID);
			}
			bool bIsRobotRandAddScore = false;
			if (max_chairID == chairID)
			{
				if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
				if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
			}
			return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
		}
		else if (HavePlayerLookCard(chairID))
		{
			if (m_wCurrentRound == GetLookRound())
			{
				// 第一圈看牌概率30%，跟注
				bool bIsLook = false;
				if (g_RandGen.RandRatio(30, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else if (m_wCurrentRound == GetLookRound() + 1)
			{
				// 第二圈看牌概率100%，跟注概率100%
				bool bIsLook = false;
				if (g_RandGen.RandRatio(100, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}			
			else if (m_wCurrentRound == GetLookRound() + 2)
			{
				// 第三圈跟注概率100%，同桌2个玩家以内跟注概率80%，同桌3个玩家及以上跟注概率70%，另有30%弃牌
				bool bIsLook = false;
				if (g_RandGen.RandRatio(100, PRO_DENO_100))
				{
					bIsLook = true;
				}
				if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
				{
					return OnUserLookCard(chairID);
				}
				if (uRealPlayerCount <= 2)
				{
					if (g_RandGen.RandRatio(80, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
				else
				{
					if (g_RandGen.RandRatio(70, PRO_DENO_100))
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else
			{
				//  if (m_wCurrentRound == GetLookRound() + 3)
				// 第四圈，判断所有玩家之中是否有一个当天闷三张是否游戏赢金币
				if (HavePalyerWinScore(chairID))
				{
					// 有玩家赢金币，开启透视看牌
					//能否比牌  能，比牌
					if (IsRobotCanCompareCard())
					{
						// 自己比玩家牌大，优先和机器人比，没有机器人和玩家比 自己比玩家牌小，和机器人比
						if (max_chairID == chairID)
						{
							return OnRobotCompareRobot(chairID);
						}
						else
						{
							return OnRobotComparePlayer(chairID);
						}
					}
					else
					{
						// 机器人比玩家牌大，跟住到底 机器人比玩家牌小，弃牌
						if (max_chairID == chairID)
						{
							bool bIsRobotRandAddScore = false;
							if (max_chairID == chairID)
							{
								if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
								if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
							}
							return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
						}
						else
						{
							return OnRobotFoldFrontLook(chairID);
						}
					}
				}
				else if (HavePalyerNoWinScore(chairID))
				{
					// 没有玩家赢金币，判断跟住玩家人数 - 3人及以上，弃牌 - 2人，直接进入比牌 - 只剩下机器人，牌型小的机器人弃牌
					if (uRealPlayerCount >= 3)
					{
						return OnRobotFoldFrontLook(chairID);
					}
					else if (uRealPlayerCount <= 2 && IsRobotCanCompareCard())
					{
						return OnRobotCompareRand(chairID);
					}
					else
					{
						if (max_chairID == chairID)
						{
							bool bIsRobotRandAddScore = false;
							if (max_chairID == chairID)
							{
								if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
								if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
								{
									bIsRobotRandAddScore = true;
								}
							}
							return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
						}
						else
						{
							return OnRobotFoldFrontLook(chairID);
						}
					}
				}
				else
				{
					if (max_chairID == chairID)
					{
						bool bIsRobotRandAddScore = false;
						if (max_chairID == chairID)
						{
							if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
							if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
							{
								bIsRobotRandAddScore = true;
							}
						}
						return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
		}
		else
		{
			if (max_chairID == chairID)
			{
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else
			{
				return OnRobotFoldFrontLook(chairID);
			}
		}
	}
	else
	{
		if (max_chairID == chairID)
		{
			bool bIsRobotRandAddScore = false;
			if (max_chairID == chairID)
			{
				if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
				if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
				{
					bIsRobotRandAddScore = true;
				}
			}
			return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	return false;
}

bool	CGameZajinhuaTable::RobotShunZiOperCard(uint16 chairID)
{
	uint16 max_chairID = GetMaxCardChair(chairID);
	uint16 uRealPlayerCount = GetRealPlayerCount();

	LOG_DEBUG("机器人顺子操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,max_chairID:%d,m_bAllinState:%d,HavePlayerNoLookCard:%d,HavePlayerLookCard:%d,HavePalyerNoWinScore:%d,HavePalyerWinScore:%d,uRealPlayerCount:%d,m_wCurrentRound:%d,GetLookRound:%d,GetCompareRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), max_chairID, m_bAllinState, HavePlayerNoLookCard(chairID), HavePlayerLookCard(chairID), HavePalyerNoWinScore(chairID), HavePalyerWinScore(chairID), uRealPlayerCount, m_wCurrentRound, GetLookRound(), GetCompareRound());
	
	if (m_bAllinState)
	{
		if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (max_chairID == chairID)
		{
			return OnRobotAllIn(chairID);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	if (OnRobotNoEnoughScoreCompareCard(chairID))
	{
		return true;
	}
	bool bIsRobotAddScore = false;
	if (max_chairID == chairID)
	{
		if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(10, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
		if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
	}
	if (HavePlayerNoLookCard(chairID))
	{
		// 当前桌有玩家，但是未看牌	跟注概率：100%，固定闷牌圈数后最多闷3 - 5圈后开启看牌比牌模式
		bool bIsLook = false;
		bool bIsCompare = false;
		if (m_wCurrentRound >= GetLookRound() + 2)
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				bIsLook = true;
			}
		}
		if (m_wCurrentRound >= GetCompareRound() + 2)
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				bIsCompare = true;
			}
		}
		if ((m_wCurrentRound >= GetLookRound() + 4))
		{
			bIsLook = true;
		}
		if ((m_wCurrentRound >= GetCompareRound() + 4))
		{
			bIsCompare = true;
		}

		if (bIsLook && IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (bIsCompare)
		{
			return OnRobotCompareRand(chairID);
		}
		return OnRobotJettonScore(chairID, bIsRobotAddScore);
	}
	else if (HavePlayerLookCard(chairID))
	{
		if (m_wCurrentRound == GetLookRound())
		{
			// 第一圈看牌概率20%，跟注
			bool bIsLook = false;
			if (g_RandGen.RandRatio(20, PRO_DENO_100))
			{
				bIsLook = true;
			}
			if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else if (m_wCurrentRound == GetLookRound() + 1)
		{
			// 第二圈看牌概率100%，跟注概率100%
			bool bIsLook = false;
			if (g_RandGen.RandRatio(100, PRO_DENO_100))
			{
				bIsLook = true;
			}
			if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else if (m_wCurrentRound == GetLookRound() + 2)
		{
			// 第三圈跟注概率100%，同桌2个玩家以内跟注概率80%，同桌3个玩家及以上跟注概率70%，另有30%弃牌
			bool bIsLook = false;
			if (g_RandGen.RandRatio(100, PRO_DENO_100))
			{
				bIsLook = true;
			}
			if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
			{
				return OnUserLookCard(chairID);
			}
			if (uRealPlayerCount <= 2)
			{
				if (g_RandGen.RandRatio(80, PRO_DENO_100))
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
			else
			{
				if (g_RandGen.RandRatio(70, PRO_DENO_100))
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
		}
		else
		{
			//  if (m_wCurrentRound == GetLookRound() + 3)
			// 第四圈，判断所有玩家之中是否有一个当天闷三张是否游戏赢金币
			if (HavePalyerWinScore(chairID))
			{
				// 有玩家赢金币，开启透视看牌
				//能否比牌  能，比牌
				if (IsRobotCanCompareCard())
				{
					// 自己比玩家牌大，优先和机器人比，没有机器人和玩家比 自己比玩家牌小，和机器人比
					if (max_chairID == chairID)
					{
						return OnRobotCompareRobot(chairID);
					}
					else
					{
						return OnRobotCompareRobot(chairID);
					}
				}
				else
				{
					// 机器人比玩家牌大，跟住到底 机器人比玩家牌小，弃牌
					if (max_chairID == chairID)
					{
						return OnRobotJettonScore(chairID, bIsRobotAddScore);
					}
					else
					{
						return OnRobotFoldFrontLook(chairID);
					}
				}
			}
			else if (HavePalyerNoWinScore(chairID))
			{
				// 没有玩家赢金币，判断跟住玩家人数 - 4人，任一一个玩家比牌 - 低于4人，继续跟注
				if (uRealPlayerCount >= 4 && IsRobotCanCompareCard())
				{
					return OnRobotCompareRand(chairID);
				}
				else
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
			}
			else
			{
				if (max_chairID == chairID)
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
		}
	}
	else
	{
		if (m_wCurrentRound < GetLookRound())
		{
			bool bIsAddScore = false;
			if (HavePalyerBigger(chairID) == false)
			{
				if (g_RandGen.RandRatio(50, PRO_DENO_100))
				{
					bIsAddScore = true;
				}
			}
			return OnRobotJettonScore(chairID, bIsAddScore);
		}
		else
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				bool bIsRobotRandAddScore = false;
				if (max_chairID == chairID)
				{
					if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(5, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
					if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
					{
						bIsRobotRandAddScore = true;
					}
				}
				return OnRobotJettonScore(chairID, bIsRobotRandAddScore);
			}
			else
			{
				return OnRobotFoldFrontLook(chairID);
			}
		}
	}
	return false;
}

bool	CGameZajinhuaTable::RobotJinHuaOperCard(uint16 chairID)
{
	uint16 max_chairID = GetMaxCardChair(chairID);
	uint16 uRealPlayerCount = GetRealPlayerCount();

	LOG_DEBUG("机器人金花操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,max_chairID:%d,m_bAllinState:%d,HavePlayerNoLookCard:%d,HavePlayerLookCard:%d,uRealPlayerCount:%d,m_wCurrentRound:%d,GetLookRound:%d,GetCompareRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), max_chairID, m_bAllinState, HavePlayerNoLookCard(chairID), HavePlayerLookCard(chairID), uRealPlayerCount, m_wCurrentRound, GetLookRound(), GetCompareRound());

	if (m_bAllinState)
	{
		if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (max_chairID == chairID)
		{
			return OnRobotAllIn(chairID);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	if (OnRobotNoEnoughScoreCompareCard(chairID))
	{
		return true;
	}
	bool bIsRobotAddScore = false;
	if (max_chairID == chairID)
	{
		if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(10, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
		if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
	}

	if (HavePlayerNoLookCard(chairID))
	{
		// 闷牌圈数不限，不和玩家以及机器人比牌，跟注概率100%
		return OnRobotJettonScore(chairID, bIsRobotAddScore);
	}
	else if (HavePlayerLookCard(chairID))
	{
		bool bIsLook = false;
		if (m_wCurrentRound >= GetLookRound() + 4)
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				bIsLook = true;
			}
		}
		if ((m_wCurrentRound >= GetLookRound() + 9))
		{
			bIsLook = true;
		}
		if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		
		if (m_wCurrentRound < GetLookRound() + 3)
		{
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else if (m_wCurrentRound >= GetLookRound() + 3)
		{
			if (HavePalyerWinScore(chairID))
			{
				if (max_chairID == chairID)
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
			else if (HavePalyerNoWinScore(chairID))
			{
				// 没有玩家赢金币，判断跟住玩家人数 - 4人，任一一个玩家比牌 - 低于4人，继续跟注
				if (uRealPlayerCount >= 4 && IsRobotCanCompareCard())
				{
					return OnRobotCompareRand(chairID);
				}
				else
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
			}
			else
			{
				if (max_chairID == chairID)
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}

		}
	}
	else
	{
		if (max_chairID == chairID)
		{
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	return false;
}

bool	CGameZajinhuaTable::RobotShunJinBaoZiOperCard(uint16 chairID)
{
	uint16 max_chairID = GetMaxCardChair(chairID);
	uint16 uRealPlayerCount = GetRealPlayerCount();

	LOG_DEBUG("机器人金豹子操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,max_chairID:%d,m_bAllinState:%d,HavePlayerNoLookCard:%d,HavePlayerLookCard:%d,uRealPlayerCount:%d,m_wCurrentRound:%d,GetLookRound:%d,GetCompareRound:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), max_chairID, m_bAllinState, HavePlayerNoLookCard(chairID), HavePlayerLookCard(chairID), uRealPlayerCount, m_wCurrentRound, GetLookRound(), GetCompareRound());

	if (m_bAllinState)
	{
		if (IsRobotCanLookCard() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}
		if (max_chairID == chairID)
		{
			return OnRobotAllIn(chairID);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	if (OnRobotNoEnoughScoreCompareCard(chairID))
	{
		return true;
	}
	bool bIsRobotAddScore = false;
	if (max_chairID == chairID)
	{
		if (m_szMingPai[chairID] == FALSE && g_RandGen.RandRatio(10, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
		if (m_szMingPai[chairID] == TRUE && g_RandGen.RandRatio(25, PRO_DENO_100))
		{
			bIsRobotAddScore = true;
		}
	}

	if (HavePlayerNoLookCard(chairID))
	{
		// 闷牌圈数不限，不和玩家以及机器人比牌，跟注概率100%
		return OnRobotJettonScore(chairID, bIsRobotAddScore);
	}
	else if (HavePlayerLookCard(chairID))
	{
		bool bIsLook = false;
		if (m_wCurrentRound >= GetLookRound() + 4)
		{
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
			{
				bIsLook = true;
			}
		}
		if ((m_wCurrentRound >= GetLookRound() + 14))
		{
			bIsLook = true;
		}
		if (bIsLook && m_wCurrentRound >= GetLookRound() && m_szMingPai[chairID] == FALSE)
		{
			return OnUserLookCard(chairID);
		}

		if (m_wCurrentRound < GetLookRound() + 3)
		{
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else if (m_wCurrentRound >= GetLookRound() + 3)
		{
			if (HavePalyerWinScore(chairID))
			{
				if (max_chairID == chairID)
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}
			else if (HavePalyerNoWinScore(chairID))
			{
				// 没有玩家赢金币，判断跟住玩家人数 - 4人，任一一个玩家比牌 - 低于4人，继续跟注
				if (uRealPlayerCount >= 4 && IsRobotCanCompareCard())
				{
					return OnRobotCompareRand(chairID);
				}
				else
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
			}
			else
			{
				if (max_chairID == chairID)
				{
					return OnRobotJettonScore(chairID, bIsRobotAddScore);
				}
				else
				{
					return OnRobotFoldFrontLook(chairID);
				}
			}

		}
	}
	else
	{
		if (max_chairID == chairID)
		{
			return OnRobotJettonScore(chairID, bIsRobotAddScore);
		}
		else
		{
			return OnRobotFoldFrontLook(chairID);
		}
	}
	return false;
}


bool CGameZajinhuaTable::OnRobotOper(uint16 chairID)
{
	
	BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID], MAX_COUNT);

	bool bIsResultFlag = false;

	if (cardType == CT_SINGLE)
	{
		bIsResultFlag = RobotSingleOperCard(chairID);
	}
	else if (cardType == CT_DOUBLE)
	{
		bIsResultFlag = RobotDoubleOperCard(chairID);
	}
	else if (cardType == CT_SHUN_ZI)
	{
		bIsResultFlag = RobotShunZiOperCard(chairID);
	}
	else if (cardType == CT_JIN_HUA)
	{
		bIsResultFlag = RobotJinHuaOperCard(chairID);
	}
	else if (cardType == CT_SHUN_JIN || cardType == CT_BAO_ZI)
	{
		bIsResultFlag = RobotShunJinBaoZiOperCard(chairID);
	}
	else
	{
		bIsResultFlag = OnRobotFoldFrontLook(chairID);
	}
	
	LOG_DEBUG("机器人操作 - roomid:%d,tableid:%d, chairID:%d,uid:%d,cardType:%d,bIsResultFlag:%d,m_wCurrentRound:%d,GetLookRound:%d,m_wCurrentUser:%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), cardType, bIsResultFlag, m_wCurrentRound, GetLookRound(), m_wCurrentUser);

	return true;

	/*
	if (IsOnlyCompare(chairID))
	{
		return OnRobotCompare(chairID);
	}
    if(m_bAllinState)
    {
        uint8 isMingPai = TRUE;
        for(uint16 i=0;i<GAME_PLAYER;++i)
        {
            if(m_cbPlayStatus[i] == TRUE && i != chairID)
            {
                isMingPai = m_szMingPai[i];
                break;
            }
        }
        BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID],MAX_COUNT);
        if(isMingPai == FALSE)// 闷allin
        {
            if(cardType >= CT_DOUBLE){
                return OnUserAllin(chairID);
            }
            if(IsMaxCard(chairID) && g_RandGen.RandRatio(80,PRO_DENO_100))
                return OnUserAllin(chairID);
        }else{
            if(AllinJetton(chairID) < m_lTableScore[chairID] && g_RandGen.RandRatio(60,PRO_DENO_100))
                return OnUserAllin(chairID);

            if(cardType <= CT_DOUBLE && g_RandGen.RandRatio(60, PRO_DENO_100)) {
                return OnUserGiveUp(chairID);
            }
            if(IsMaxCard(chairID) && g_RandGen.RandRatio(70, PRO_DENO_100)) {
                return OnUserAllin(chairID);
            }
        }
        return OnUserGiveUp(chairID);
    }
    else
    {
        if(m_szMingPai[chairID] == FALSE && m_wCurrentRound > GetLookRound())// 未明牌
        {
            uint16 lookNum = 0;
            for(uint16 i = 0; i < GAME_PLAYER; ++i){
                if (m_cbPlayStatus[i] == TRUE && m_szCardState[i] != emCARD_STATE_NULL) {
                    lookNum++;
                }
            }
            if(lookNum > 0 && g_RandGen.RandRatio(99,PRO_DENO_100)){
                return OnUserLookCard(chairID);
            }
            if(g_RandGen.RandRatio(30+m_wCurrentRound*5,PRO_DENO_100)){
                return OnUserLookCard(chairID);
            }
        }
        BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID],MAX_COUNT);
        if(m_wCurrentRound > GetCompareRound())//大于2轮才能看牌比牌
        {
            if(m_szMingPai[chairID] == TRUE)
            {
                if(cardType == CT_DOUBLE){//对子
                    if((!IsMaxCard(chairID)) && (!m_szCompareWinUser[chairID]))//概率比牌
                    {
                        if(g_RandGen.RandRatio(85, PRO_DENO_100)){
                            return OnUserGiveUp(chairID);
                        }
                        return OnRobotCompare(chairID);
                    }
                    return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                }else if(cardType == CT_SHUN_ZI){//顺子
                    if((IsMaxCard(chairID) && g_RandGen.RandRatio(75,100))){
                        return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }else if(cardType == CT_JIN_HUA){//金花
                    if(IsMaxCard(chairID) && g_RandGen.RandRatio(65,PRO_DENO_100)){
                        return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }else if(cardType >= CT_SHUN_JIN){//顺金，豹子
                    if(IsMaxCard(chairID)){
                        return OnUserAddScore(chairID, CallJetton(chairID), false, false);
                    }else{
						if ((!m_szCompareWinUser[chairID])) {
							if (cardType == CT_SHUN_JIN && g_RandGen.RandRatio(55, PRO_DENO_100)) {
								return OnUserGiveUp(chairID);
							}
							else if (cardType == CT_BAO_ZI && g_RandGen.RandRatio(10, PRO_DENO_100)) {
								return OnUserGiveUp(chairID);
							}
							else {
								return OnRobotCompare(chairID);
							}
						}
						else {
							return OnRobotCompare(chairID);
						}
                    }
                }
                else{//单牌
                    BYTE singValue = m_gameLogic.GetSingCardValue(m_cbHandCardData[chairID],MAX_COUNT);
                    if(IsMaxCard(chairID))
                    {
                        if(g_RandGen.RandRatio(90, PRO_DENO_100))
                            return OnRobotCompare(chairID);
                    }else{
                        if(GetPlayNum() > 2){
							if (g_RandGen.RandRatio(99, PRO_DENO_100) && (!m_szCompareWinUser[chairID]))
								return OnUserGiveUp(chairID);
                        }
                        if(singValue < 13){
							if (g_RandGen.RandRatio(99, PRO_DENO_100) && (!m_szCompareWinUser[chairID]))
								return OnUserGiveUp(chairID);
                        }
                    }
                    if(singValue < 13 && g_RandGen.RandRatio(99,PRO_DENO_100) && (!m_szCompareWinUser[chairID])){
                        return OnUserGiveUp(chairID);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }
            }
            if(GetPlayNum() == 2 && g_RandGen.RandRatio(50,PRO_DENO_100))//概率allin
            {
                if(cardType <= CT_DOUBLE){
                    return OnRobotCompare(chairID);
                }
                return OnUserAllin(chairID);
            }
        }else{
            if(m_szMingPai[chairID] == TRUE)
            {
                //if(cardType < CT_DOUBLE && !IsMaxCard(chairID)){
                //    return OnUserGiveUp(chairID);
                //}
				if (cardType == CT_DOUBLE) {//对子
					if ((!IsMaxCard(chairID)) && (!m_szCompareWinUser[chairID]))//概率比牌
					{
						if (g_RandGen.RandRatio(85, PRO_DENO_100)) {
							return OnUserGiveUp(chairID);
						}
					}
				}
				else if (cardType == CT_SHUN_ZI) {//顺子
					if ((!IsMaxCard(chairID)) && (!m_szCompareWinUser[chairID])) {
						if (g_RandGen.RandRatio(75, PRO_DENO_100)) {
							return OnUserGiveUp(chairID);
						}
					}
				}
				else if (cardType == CT_JIN_HUA) {//金花
					if ((!IsMaxCard(chairID)) && (!m_szCompareWinUser[chairID])) {
						if (g_RandGen.RandRatio(65, PRO_DENO_100)) {
							return OnUserGiveUp(chairID);
						}
					}
				}
				else if (cardType >= CT_SHUN_JIN) {//顺金，豹子
					if (IsMaxCard(chairID)) {
						return OnUserAddScore(chairID, CallJetton(chairID), false, false);
					}
					else {
						if ((!m_szCompareWinUser[chairID])) {
							if (cardType == CT_SHUN_JIN && g_RandGen.RandRatio(55, PRO_DENO_100)) {
								return OnUserGiveUp(chairID);
							}
							if (cardType == CT_BAO_ZI && g_RandGen.RandRatio(10, PRO_DENO_100)) {
								return OnUserGiveUp(chairID);
							}
						}
					}
				}
				else {//单牌
					BYTE singValue = m_gameLogic.GetSingCardValue(m_cbHandCardData[chairID], MAX_COUNT);
					if (IsMaxCard(chairID))
					{
						if (g_RandGen.RandRatio(90, PRO_DENO_100))
							return OnUserAddScore(chairID, CallJetton(chairID), false, false);
					}
					else {
						if (GetPlayNum() > 2) {
							if (g_RandGen.RandRatio(99, PRO_DENO_100) && (!m_szCompareWinUser[chairID]))
								return OnUserGiveUp(chairID);
						}
						if (singValue < 13) {
							if (g_RandGen.RandRatio(99, PRO_DENO_100) && (!m_szCompareWinUser[chairID]))
								return OnUserGiveUp(chairID);
						}
					}
					if (singValue < 13 && g_RandGen.RandRatio(99, PRO_DENO_100) && (!m_szCompareWinUser[chairID])) {
						return OnUserGiveUp(chairID);
					}
				}
            }
        }
        return OnUserAddScore(chairID, CallJetton(chairID), false, false);
    }
	*/
}
bool CGameZajinhuaTable::OnRobotOper_1(uint16 chairID)
{
    LOG_DEBUG("机器人操作:%d",chairID);
    if(IsOnlyCompare(chairID))
    {
        return OnRobotCompare(chairID);
    }
    if(m_bAllinState)
    {
        uint8 isMingPai = TRUE;
        for(uint16 i=0;i<GAME_PLAYER;++i)
        {
            if(m_cbPlayStatus[i] == TRUE && i != chairID)
            {
                isMingPai = m_szMingPai[i];
                break;
            }
        }
        BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID],MAX_COUNT);
        if(isMingPai == FALSE)// 闷allin
        {
            if(cardType >= CT_DOUBLE){
                return OnUserAllin(chairID);
            }
            if(IsMaxCard(chairID) && g_RandGen.RandRatio(80,PRO_DENO_100))
                return OnUserAllin(chairID);
        }else{
            if(AllinJetton(chairID) < m_lTableScore[chairID] && g_RandGen.RandRatio(60,PRO_DENO_100))
                return OnUserAllin(chairID);

            if(cardType <= CT_DOUBLE && g_RandGen.RandRatio(60, PRO_DENO_100)) {
                return OnUserGiveUp(chairID);
            }
            if(IsMaxCard(chairID) && g_RandGen.RandRatio(70, PRO_DENO_100)) {
                return OnUserAllin(chairID);
            }
        }
        return OnUserGiveUp(chairID);
    }
    else
    {
        if(m_szMingPai[chairID] == FALSE)// 未明牌
        {
            uint16 lookNum = 0;
            for(uint16 i = 0; i < GAME_PLAYER; ++i) {
                if (m_cbPlayStatus[i] == TRUE && m_szCardState[i] != emCARD_STATE_NULL) {
                    lookNum++;
                }
            }
            if(lookNum > 0 && g_RandGen.RandRatio(50+m_wCurrentRound*5,PRO_DENO_100)){
                return OnUserLookCard(chairID);
            }
            if(g_RandGen.RandRatio(10+m_wCurrentRound*5,PRO_DENO_100)){
                return OnUserLookCard(chairID);
            }
        }
        BYTE cardType = m_gameLogic.GetCardType(m_cbHandCardData[chairID],MAX_COUNT);
        if(m_wCurrentRound > 2)//大于2轮才能看牌比牌
        {
            if(m_szMingPai[chairID] == TRUE)
            {
                if(cardType == CT_DOUBLE){//对子
                    if(!IsMaxCard(chairID) || g_RandGen.RandRatio(65, PRO_DENO_100))//概率比牌
                    {
                        return OnRobotCompare(chairID);
                    }
                    return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                }else if(cardType == CT_SHUN_ZI){//顺子
                    if((IsMaxCard(chairID) && g_RandGen.RandRatio(85,100)) || m_szMingPaiRound[chairID] == m_wCurrentRound){
                        return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }else if(cardType == CT_JIN_HUA){//金花
                    if(!IsMaxCard(chairID) && g_RandGen.RandRatio(80,PRO_DENO_100)){
                        return OnUserGiveUp(chairID);
                    }
                    if(g_RandGen.RandRatio(80,100) || m_szMingPaiRound[chairID] == m_wCurrentRound){
                        return OnUserAddScore(chairID,CallJetton(chairID), false, false);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }else if(cardType >= CT_SHUN_JIN){//顺金，豹子
                    if(IsMaxCard(chairID)){
                        return OnUserAddScore(chairID, CallJetton(chairID), false, false);
                    }else{
                        if(g_RandGen.RandRatio(90,PRO_DENO_100)){
                            return OnUserGiveUp(chairID);
                        }else{
                            return OnRobotCompare(chairID);
                        }
                    }
                }
                else{//单牌
                    BYTE singValue = m_gameLogic.GetSingCardValue(m_cbHandCardData[chairID],MAX_COUNT);
                    if(IsMaxCard(chairID) && g_RandGen.RandRatio(80,PRO_DENO_100))
                        return OnRobotCompare(chairID);
                    if(singValue < 13 && g_RandGen.RandRatio(80,PRO_DENO_100)){
                        return OnUserGiveUp(chairID);
                    }else{
                        return OnRobotCompare(chairID);
                    }
                }
            }
            if(GetPlayNum() == 2 && g_RandGen.RandRatio(50,PRO_DENO_100))//概率allin
            {
                if(cardType <= CT_DOUBLE){
                    return OnRobotCompare(chairID);
                }
                return OnUserAllin(chairID);
            }
        }else{
            if(m_szMingPai[chairID] == TRUE)
            {
                if(cardType < CT_DOUBLE && !IsMaxCard(chairID))
				{
                    return OnUserGiveUp(chairID);
                }
            }
        }
        return OnUserAddScore(chairID, CallJetton(chairID), false, false);
    }
}
bool CGameZajinhuaTable::OnRobotCompare(uint16 chairID)
{
	if (!m_szCompareWinUser[chairID])
	{
		uint16 cmp_chairid = GetMaxCardChair(chairID);
		if (cmp_chairid != chairID) {
			return OnUserCompareCard(chairID, cmp_chairid);
		}
	}
    for(uint16 i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_cbPlayStatus[i] == TRUE && i != chairID) {
            return OnUserCompareCard(chairID, i);
        }
    }
    return false;
}

bool CGameZajinhuaTable::OnRobotCompareRand(uint16 chairID)
{
	bool bIsRobotAllIn = m_bAllinState;

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bIsRobotAllIn:%d,chairID:%d,m_szCardState:%d,m_cbPlayStatus:%d-%d-%d-%d-%d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), bIsRobotAllIn, chairID, m_szCardState[chairID], m_cbPlayStatus[0],
		m_cbPlayStatus[1], m_cbPlayStatus[2], m_cbPlayStatus[3], m_cbPlayStatus[4]);

	if (bIsRobotAllIn)
	{
		if (m_szCardState[chairID] != emCARD_STATE_ALLIN)
		{
			return OnRobotAllIn(chairID);
		}
	}


	uint16 cmp_chairid = INVALID_CHAIR;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (i == chairID)
		{
			continue;
		}
		if (m_cbPlayStatus[i] == TRUE)
		{
			cmp_chairid = i;
		}
	} // 注意，比牌的对象不一定是下一个加注的玩家！
	return OnUserCompareCard(chairID, cmp_chairid);
}

bool CGameZajinhuaTable::OnRobotCompareRobot(uint16 chairID)
{
	bool bIsRobotAllIn = m_bAllinState;

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bIsRobotAllIn:%d,chairID:%d,m_szCardState:%d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), bIsRobotAllIn, chairID, m_szCardState[chairID]);

	if (bIsRobotAllIn)
	{
		if (m_szCardState[chairID] != emCARD_STATE_ALLIN)
		{
			return OnRobotAllIn(chairID);
		}
	}

	uint16 cmp_chairid = INVALID_CHAIR;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (i == chairID)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && pPlayer->IsRobot())
		{
			cmp_chairid = i;
		}
	}
	if (cmp_chairid == INVALID_CHAIR)
	{
		for (uint16 i = 0; i < GAME_PLAYER; ++i)
		{
			if (i == chairID)
			{
				continue;
			}
			if (m_cbPlayStatus[i] == TRUE)
			{
				cmp_chairid = i;
			}
		}
	}
	return OnUserCompareCard(chairID, cmp_chairid);
}

bool CGameZajinhuaTable::OnRobotComparePlayer(uint16 chairID)
{
	uint16 cmp_chairid = INVALID_CHAIR;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (i == chairID)
		{
			continue;
		}
		CGamePlayer * pPlayer = GetPlayer(i);
		if (m_cbPlayStatus[i] == TRUE && pPlayer != NULL && !pPlayer->IsRobot())
		{
			cmp_chairid = i;
		}
	}
	if (cmp_chairid == INVALID_CHAIR)
	{
		for (uint16 i = 0; i < GAME_PLAYER; ++i)
		{
			if (i == chairID)
			{
				continue;
			}
			if (m_cbPlayStatus[i] == TRUE)
			{
				cmp_chairid = i;
			}
		}
	}
	return OnUserCompareCard(chairID, cmp_chairid);
}


uint16 CGameZajinhuaTable::GetMaxCardChair(uint16 chairID)
{
	//if (IsMaxCard(chairID) {
	//	return chairID;
	//}
	uint16 tmp_chairID = chairID;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] != TRUE)
			continue;
		tmp_chairID = i;
		for (uint16 j = i+1; j<GAME_PLAYER; ++j)
		{
			if ( m_cbPlayStatus[j] != TRUE)
				continue;
			if (m_gameLogic.CompareCard(m_cbHandCardData[j], m_cbHandCardData[tmp_chairID], MAX_COUNT) == TRUE){
				tmp_chairID = j;
			}
		}
		return tmp_chairID;
	}
	return tmp_chairID;
}

uint16 CGameZajinhuaTable::GetMinCardChair(uint16 chairID)
{
	//if (IsMaxCard(chairID) {
	//	return chairID;
	//}
	uint16 tmp_chairID = chairID;
	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] != TRUE)
			continue;
		tmp_chairID = i;
		for (uint16 j = i + 1; j<GAME_PLAYER; ++j)
		{
			if (m_cbPlayStatus[j] != TRUE)
				continue;
			if (m_gameLogic.CompareCard(m_cbHandCardData[j], m_cbHandCardData[tmp_chairID], MAX_COUNT) == FALSE) {
				tmp_chairID = j;
			}
		}
		return tmp_chairID;
	}
	return tmp_chairID;
}

bool CGameZajinhuaTable::IsMaxCard(uint16 chairID)
{
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        if(chairID == i || m_cbPlayStatus[i] != TRUE)
            continue;
        if(m_gameLogic.CompareCard(m_cbHandCardData[chairID],m_cbHandCardData[i],MAX_COUNT) != TRUE)
            return false;
    }
    return true;
}
void CGameZajinhuaTable::CheckAddRobot()
{
	if (m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
	{
		return;
	}

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
			m_coolRobot.beginCooling(g_RandGen.RandRange(2000, 4000));
			return;
		}
	}

    /*for(uint32 i=0;i<m_vecPlayers.size();++i){  delete by har
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_vecPlayers[i].readyState == 0){
                PlayerReady(pPlayer);
                break;
            }else{
                if((getSysTime() - m_vecPlayers[i].readyTime) > 150){
                    LeaveTable(pPlayer);
                    LOG_DEBUG("time out ready and leave:%d",pPlayer->GetUID());
                }
            }
        }
    }*/

  //  if(GetChairPlayerNum() >= 1 && GetChairPlayerNum() <= 3)
  //  {
  //      for(uint32 i=0;i<m_vecPlayers.size();++i)
		//{
  //          CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
  //          if(pPlayer != NULL && !pPlayer->IsRobot())
  //          {
  //              CRobotMgr::Instance().RequestOneRobot(this);
  //              m_coolRobot.beginCooling(g_RandGen.RandRange(2000,4000));
  //              return;
  //          }
  //      }
  //  }
    //SetRobotThinkTime(); // delete by har
}
void CGameZajinhuaTable::SetRobotThinkTime()
{
    m_coolRobot.beginCooling(g_RandGen.RandRange(1000,2000));
}

bool CGameZajinhuaTable::ActiveWelfareCtrl()
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
        if (IsReady(pPlayer) && pPlayer->IsRobot() == false)
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

bool CGameZajinhuaTable::NewRegisterWelfareCtrl()
{
	LOG_DEBUG("enter function player count:%d.", m_vecPlayers.size());

	//判断当前桌子是否为新注册玩家福利桌子
	if (!IsNewRegisterWelfareTable())
	{
		LOG_DEBUG("the current table is not new register welfare table. room_id:%d tid:%d", GetRoomID(), GetTableID());
		return false;
	}

	//查找当前桌子满足新注册福利的玩家
	uint32 control_uid = 0;
	int control_status = 0;
	for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
	{
		CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
		if (pTPlayer != NULL && !(pTPlayer->IsRobot()) && pTPlayer->GetNewRegisterWelfareStatus(control_status))
		{
			control_uid = pTPlayer->GetUID();
			break;
		}
	}
	
	//判断当前是否有可控玩家
	if (control_uid != 0)
	{
		m_nrw_status = control_status;
		bool ret = false;
		if (control_status == 0)		//不需要指定玩家牌型
		{
			ret = SetControlPalyerWin(control_uid);
		}
		if (control_status == 1)        //需要指定玩家的牌型
		{
			ret = SetNRWControlPalyerWin(control_uid);
		}
		if (control_status == 2)		//如果当前赢取大于最大赢取时,设置机器人必赢
		{
			ret = SetControlPalyerLost(control_uid);
		}
		if (ret)
		{
			LOG_DEBUG("1 search success current player - uid:%d control_status:%d", control_uid, control_status);
			m_nrw_ctrl_uid = control_uid;   //设置当前新注册福利所控的玩家ID
			return true;
		}
		else
		{
			LOG_DEBUG("1 search fail current player - uid:%d control_status:%d", control_uid, control_status);
		}
	}
	else
	{
		LOG_DEBUG("the nrw control_uid is null.tid:%d.", GetTableID());
	}	
	LOG_DEBUG("the no player match new register welfare tid:%d.", GetTableID());
	return false;
}

bool    CGameZajinhuaTable::SetNRWControlPalyerWin(uint32 control_uid)
{
	LOG_DEBUG("enter function tid:%d control_uid:%d.", GetTableID(), control_uid);
	bool bIsFlag = true;
	
	//先确定当前控制玩家的牌型 [豹子、同花顺、同花、顺子] 根据配置的概率
	int iRandNum = g_RandGen.RandRange(0, PRO_DENO_10000);
	int iProIndex = 0;
	for (; iProIndex < Pro_Index_MAX; iProIndex++)
	{
		if (m_iArrNRWCardPro[iProIndex] == 0)
		{
			continue;
		}
		if (iRandNum <= m_iArrNRWCardPro[iProIndex])
		{
			break;
		}
		else
		{
			iRandNum -= m_iArrNRWCardPro[iProIndex];
		}
	}
	if (iProIndex >= Pro_Index_MAX)
	{
		iProIndex = Pro_Index_Single;
	}
	LOG_DEBUG("get control player cardtype tid:%d control_uid:%d card_type:%d.", GetTableID(), control_uid, iProIndex);
		
	//再确定其它人获取牌的类型
	int iArProCardType[GAME_PLAYER] = { Pro_Index_Single };
	//uint16 maxChairID = GetMaxCardChair(INVALID_CHAIR);
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL )
		{
			if (pTmp->GetUID() == control_uid)
			{
				iArProCardType[i] = iProIndex;
				LOG_DEBUG("tid:%d i:%d iProIndex:%d.", GetTableID(), i, iArProCardType[i]);
			}
			else
			{
				uint32 tmp = rand() % (Pro_Index_MAX - iProIndex);
				iArProCardType[i] = iProIndex + tmp +1;
				if (iArProCardType[i] >= Pro_Index_MAX)
				{
					iArProCardType[i] = Pro_Index_Single;
				}
				LOG_DEBUG("tid:%d i:%d iProIndex:%d.", GetTableID(), i, iArProCardType[i]);
			}
		}
		else
		{
			continue;
		}		
	}
	// 根据类型获取全部的手牌
	bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData);
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbPlayStatus:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X",
				m_pHostRoom->GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbPlayStatus[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2]);
	}
	return true;	
}

bool CGameZajinhuaTable::SetLuckyCtrl()
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
							LOG_DEBUG("the lucky win player. uid:%d chairid:%d HandCardData:0x%02X 0x%02X 0x%02X",
								win_uid, chairid, m_cbHandCardData[chairid][0], m_cbHandCardData[chairid][1], m_cbHandCardData[chairid][2]);
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
							LOG_DEBUG("the lucky lose player. uid:%d chairid:%d HandCardData:0x%02X 0x%02X 0x%02X",
								lose_uid, chairid, m_cbHandCardData[chairid][0], m_cbHandCardData[chairid][1], m_cbHandCardData[chairid][2]);
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

bool    CGameZajinhuaTable::SetLostForLuckyCtrl(uint32 control_uid)
{
	uint16 minChairID = GetMinCardChair(INVALID_CHAIR);
	if (minChairID == INVALID_CHAIR)
	{
		LOG_DEBUG("min card is error - maxChairID:%d,control_uid:%d", minChairID, control_uid);
	}
	CGamePlayer* pTar = GetPlayer(minChairID);
	if (pTar != NULL && pTar->GetUID() == control_uid) {
		LOG_DEBUG("min card is - minChairID:%d,control_uid:%d", minChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;

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

		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->GetUID() == control_uid)
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);
			LOG_DEBUG("changer card success - control_uid:%d,i:%d,minChairID:%d HandCardData:0x%02X 0x%02X 0x%02X", 
				control_uid, i, minChairID, m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2]);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - minChairID:%d", minChairID);
	return false;
}

// 设置库存输赢  add by har
bool CGameZajinhuaTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	if (stockChange == -1) {
		if (SetRobotWinScore()) {
			LOG_DEBUG("SetStockWinLose suc RobotWin - roomid:%d,tableid:%d,stockChange:%lld",
				GetRoomID(), GetTableID(), stockChange);
			return true;
		}
		LOG_ERROR("SetStockWinLose fail1 - roomid:%d,tableid:%d,stockChange:%lld",
			GetRoomID(), GetTableID(), stockChange);
	} else {
		if (SetPalyerWinScore()) {
			LOG_DEBUG("SetStockWinLose suc PlayerWin - roomid:%d,tableid:%d,stockChange:%lld",
				GetRoomID(), GetTableID(), stockChange);
			return true;
		}
		LOG_ERROR("SetStockWinLose fail2 - roomid:%d,tableid:%d,stockChange:%lld",
			GetRoomID(), GetTableID(), stockChange);
	}
	LOG_ERROR("SetStockWinLose fail3 - roomid:%d,tableid:%d,stockChange:%lld",
		GetRoomID(), GetTableID(), stockChange);
	return false;
}
