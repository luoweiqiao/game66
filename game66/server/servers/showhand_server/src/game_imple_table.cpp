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
using namespace game_show_hand;
using namespace net;

namespace
{
    const static uint32 s_AddScoreTime = 25*1000;
       
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 梭哈
        {
            pTable = new CGameShowHandTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛梭哈
        {
            pTable = new CGameShowHandTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房梭哈
        {
            pTable = new CGameShowHandTable(this,tableID,emTABLE_TYPE_PLAYER);
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
CGameShowHandTable::CGameShowHandTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();
    //游戏变量
    m_bShowHand=false;
    m_wCurrentUser=INVALID_CHAIR;
	m_robotBankerWinPro = 0;
	m_robotFrontFourWinPro = 8000;
	m_robotFrontFourLostPro = 100;

    //下注信息
    m_lDrawMaxScore=0L;
    m_lTurnMaxScore=0L;
    m_lTurnLessScore=0L;
    m_lDrawCellScore=0L;

    //用户状态
    memset(m_cbPlayStatus,0,sizeof(m_cbPlayStatus));
    memset(m_cbOperaScore,0,sizeof(m_cbOperaScore));

    //金币信息
    memset(m_lUserScore,0,sizeof(m_lUserScore));
    memset(m_lTableScore,0,sizeof(m_lTableScore));
    memset(m_lUserMaxScore,0,sizeof(m_lUserMaxScore));
    memset(m_lHistoryScore,0,sizeof(m_lHistoryScore));

    //扑克变量
    m_cbSendCardCount=0;
    memset(m_cbCardCount,0,sizeof(m_cbCardCount));
    memset(m_cbHandCardData,0,sizeof(m_cbHandCardData));

    //组件变量
	m_tagControlPalyer.Init();

	m_bIsControlPlayer = false;
    return;
}
CGameShowHandTable::~CGameShowHandTable()
{

}
// 重载准备状态
bool CGameShowHandTable::IsAllReady()
{
    uint16 readyConut = 0;
    for(uint8 i=0;i<m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == NULL)
            continue;
        if(m_vecPlayers[i].readyState == 0){
            return false;
        }
        readyConut++;
    }
    return readyConut >= 2;
}
void CGameShowHandTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::showhand_table_info* pshowhand = pInfo->mutable_showhand();
    pshowhand->set_tableid(GetTableID());
    pshowhand->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pshowhand->set_is_passwd(1);
    }else{
        pshowhand->set_is_passwd(0);
    }
    pshowhand->set_hostname(m_conf.hostName);
    pshowhand->set_basescore(m_conf.baseScore);
    pshowhand->set_consume(m_conf.consume);
    pshowhand->set_entermin(m_conf.enterMin);
    pshowhand->set_duetime(m_conf.dueTime);
    pshowhand->set_feetype(m_conf.feeType);
    pshowhand->set_feevalue(m_conf.feeValue);
    pshowhand->set_card_time(s_AddScoreTime);
    pshowhand->set_table_state(GetGameState());
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        net::seat_face* pface = pshowhand->add_players();
        pface->set_chairid(i);
        pface->set_head_icon(0);
        pface->set_ready(m_vecPlayers[i].readyState);
        pface->set_uid(0);
        pface->set_name("");
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL)
        {
            pface->set_uid(pPlayer->GetUID());
            pface->set_name(pPlayer->GetPlayerName());
            pface->set_head_icon(pPlayer->GetHeadIcon());
        }
    }
}

//配置桌子
bool CGameShowHandTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }


	m_iArrDispatchCardPro[ShowHand_Pro_Index_TongHuaShun] = 10;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_TieZhi] = 10;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_HuLu] = 10;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_TongHua] = 10;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_ShunZi] = 60;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_ThreeTiao] = 1000;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_TwoDouble] = 2000;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_OneDouble] = 3000;
	m_iArrDispatchCardPro[ShowHand_Pro_Index_Single] = 3000;

	m_iArrWelfareCardPro[ShowHand_Pro_Index_TongHuaShun] = 10;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_TieZhi] = 10;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_HuLu] = 10;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_TongHua] = 10;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_ShunZi] = 60;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_ThreeTiao] = 1000;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_TwoDouble] = 2000;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_OneDouble] = 3000;
	m_iArrWelfareCardPro[ShowHand_Pro_Index_Single] = 3000;

	m_iArrNRWCardPro[ShowHand_Pro_Index_TongHuaShun] = 1000;
	m_iArrNRWCardPro[ShowHand_Pro_Index_TieZhi] = 1000;
	m_iArrNRWCardPro[ShowHand_Pro_Index_HuLu] = 2000;
	m_iArrNRWCardPro[ShowHand_Pro_Index_TongHua] = 3000;
	m_iArrNRWCardPro[ShowHand_Pro_Index_ShunZi] = 3000;
	m_iArrNRWCardPro[ShowHand_Pro_Index_ThreeTiao] = 0;
	m_iArrNRWCardPro[ShowHand_Pro_Index_TwoDouble] = 0;
	m_iArrNRWCardPro[ShowHand_Pro_Index_OneDouble] = 0;
	m_iArrNRWCardPro[ShowHand_Pro_Index_Single] = 0;
	InitHaveWinPreHandCard();
	ReAnalysisParam();
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}


bool CGameShowHandTable::ReAnalysisParam() 
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}

	if (jvalue.isMember("bbw") && jvalue["bbw"].isIntegral())
	{
		m_robotBankerWinPro = jvalue["bbw"].asInt();
	}
	if (jvalue.isMember("ffw") && jvalue["ffw"].isIntegral())
	{
		m_robotFrontFourWinPro = jvalue["ffw"].asInt();
	}
	if (jvalue.isMember("ffl") && jvalue["ffl"].isIntegral())
	{
		m_robotFrontFourLostPro = jvalue["ffl"].asInt();
	}

	for (int i = 0; i < ShowHand_Pro_Index_MAX; i++)
	{
		string strPro = CStringUtility::FormatToString("pr%d", i);
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
				GetRoomID(),GetTableID(), tmpjvalue.size() , str_temp.c_str());
			if (tmpjvalue.size() == ShowHand_Pro_Index_MAX)
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


	if (jvalue.isMember("wro"))
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
			}
		}
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
			}
		}
	}
	LOG_ERROR("reader json parse success - roomid:%d,tableid:%d,m_iArrDispatchCardPro:%d %d %d %d %d %d %d %d %d,m_robotBankerWinPro:%d,m_robotFrontFourWinPro:%d,m_robotFrontFourLostPro:%d,m_iArrNRWCardPro:%d %d %d %d %d %d %d %d %d",
		GetRoomID(), GetTableID(), m_iArrDispatchCardPro[0], m_iArrDispatchCardPro[1], m_iArrDispatchCardPro[2], m_iArrDispatchCardPro[3], m_iArrDispatchCardPro[4], m_iArrDispatchCardPro[5], m_iArrDispatchCardPro[6], m_iArrDispatchCardPro[7], m_iArrDispatchCardPro[8], m_robotBankerWinPro, m_robotFrontFourWinPro, m_robotFrontFourLostPro,
		m_iArrNRWCardPro[0], m_iArrNRWCardPro[1], m_iArrNRWCardPro[2], m_iArrNRWCardPro[3], m_iArrNRWCardPro[4], m_iArrNRWCardPro[5], m_iArrNRWCardPro[6], m_iArrNRWCardPro[7], m_iArrNRWCardPro[8]);
	return true;
}



bool    CGameShowHandTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) {
		LOG_DEBUG("player pointer is null");
		return false;
	}
	if (pPlayer->GetTable() != NULL) {
		LOG_DEBUG("player table is not null - uid:%d,ptable:%p", pPlayer->GetUID(), pPlayer->GetTable());
		return false;
	}
	if (IsFullTable() || GetGameState() != TABLE_STATE_FREE) {
		LOG_DEBUG("game table limit - uid:%d,is_full:%d,game_state:%d", pPlayer->GetUID(), IsFullTable(), GetGameState());
		return false;
	}
	// 限额进入
	if (GetPlayerCurScore(pPlayer) < GetEnterMin()) {
		LOG_DEBUG("min score limit - uid:%d,cur_score:%lld,min_score:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetEnterMin());
		return false;
	}
	if (pPlayer->IsRobot() && m_robotEnterCooling.isTimeOut() == false) {// 冷却期机器人不能进入
		LOG_DEBUG("robot is cooling,do not enter table - uid:%d", pPlayer->GetUID());
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
		if (irobot_num >= TABLE_ROBOT_COUNT_SHOWHAND) {
			return false;
		}
	}
	bool bIsNoviceWelfare = EnterNoviceWelfare(pPlayer);

	//增加新注册用户的判断
	bool bIsNewRegisterWelfare = EnterNewRegisterWelfare(pPlayer);
	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNewRegisterWelfare:%d,bIsNoviceWelfare:%d", 
		pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNewRegisterWelfare, bIsNoviceWelfare);
	if (bIsNewRegisterWelfare == true || bIsNoviceWelfare== true)
	{
		return false;
	}


	//bool bIsKilledScore = EnterAutoKillScore(pPlayer);
	//LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsKilledScore:%d",
	//	pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsKilledScore);

	//if (bIsKilledScore == true)
	//{
	//	return false;
	//}

	return true;
}

void CGameShowHandTable::ShutDown()
{

}
//复位桌子
void CGameShowHandTable::ResetTable()
{
    ResetGameData();

    SetGameState(TABLE_STATE_FREE);
    ResetPlayerReady();

}
void CGameShowHandTable::OnTimeTick()
{
	OnTableTick();

    if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FREE:
            {
				if(IsAllReady()){
                    OnGameStart();
                }
                CheckAddRobot();
            }break;
        case TABLE_STATE_PLAY:
            {
                OnTimeOutOper();
            }break;
        default:
            break;
        }
    }
	if (GetGameState() == net::TABLE_STATE_FREE)
	{
		CheckAddRobot();
		CheckPlayerScoreManyLeave();
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
int CGameShowHandTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    uint16 chairID = GetChairID(pPlayer);
    switch(cmdID)
    {
    case net::C2S_MSG_SHOWHAND_GIVE_UP:// 用户放弃
        {
            //状态效验
            if(GetGameState() != TABLE_STATE_PLAY)
                return 0;
            //用户效验
            if(chairID != m_wCurrentUser || m_cbPlayStatus[chairID] == 0)
                return 0;
            //消息处理
            return OnUserGiveUp(chairID);
        }break;
    case net::C2S_MSG_SHOWHAND_ADDSCORE:// 用户加注
        {
            //状态效验
            if(GetGameState() != TABLE_STATE_PLAY)
                return 0;

            //用户效验
            if (m_cbPlayStatus[chairID] == 0)
                return 0;
            net::msg_showhand_addscore_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            return OnUserAddScore(chairID,msg.score());
        }break;
    default:
        return 0;
    }
    return 0;
}
// 游戏开始
bool CGameShowHandTable::OnGameStart()
{

    //LOG_DEBUG("game start");
    //设置状态
    SetGameState(TABLE_STATE_PLAY);
	m_lucky_flag = false;

    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer == NULL)
		{
			continue;
		}
        m_cbPlayStatus[i]=TRUE;
    }

	//派发扑克
	m_cbSendCardCount = 2;
	DispatchCard();

    //最低积分
    int64 lUserLessScore=0L;
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        //获取用户
        CGamePlayer* pPlayer = GetPlayer(i);
        //设置变量
        if(pPlayer != NULL)
        {
            if((lUserLessScore <= 0L) || (GetPlayerCurScore(pPlayer) < lUserLessScore))
            {
                lUserLessScore = GetPlayerCurScore(pPlayer);
            }
        }
    }
    InitBlingLog();    
    //服务费
    DeductStartFee(true);

    //底注积分
    //if(GetTableType() == emTABLE_TYPE_SYSTEM) {
        //m_lDrawCellScore = CDataCfgMgr::Instance().GetShowhandBaseScore(lUserLessScore);
    //}else{
        m_lDrawCellScore = GetBaseScore();
    //}

	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

    //变量设置
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        //获取用户
        CGamePlayer* pPlayer = GetPlayer(i);
        //设置变量
        if(pPlayer != NULL)
        {
            //状态设置
            m_cbPlayStatus[i]  = TRUE;
            m_lTableScore[i]   = m_lDrawCellScore;
            m_lUserMaxScore[i] = GetPlayerCurScore(pPlayer);

            //扑克设置
            m_cbCardCount[i]  = m_cbSendCardCount;

			WriteAddCardLog(i, cbUserCardData[i], m_cbCardCount[i],0);
        }
    }

    //下注计算
    RectifyMaxScore();

    //设置变量
    m_wCurrentUser   =  EstimateWinner(1,1);
    m_lTurnLessScore =  m_lUserScore[m_wCurrentUser] + m_lTableScore[m_wCurrentUser];

	LOG_DEBUG("game_start - roomid:%d,tableid:%d,m_wCurrentUser:%d,uids:%d %d",
		GetRoomID(), GetTableID(), m_wCurrentUser, GetPlayerID(0), GetPlayerID(1));


    // 发送消息
    for(uint16 i=0;i<GAME_PLAYER;++i)
    {
        net::msg_showhand_start_rep start_msg;
        start_msg.set_cur_user(m_wCurrentUser);
        start_msg.set_draw_max_score(m_lDrawMaxScore);
        start_msg.set_turn_max_score(m_lTurnMaxScore);
        start_msg.set_turn_less_score(m_lTurnLessScore);
        start_msg.set_cell_score(m_lDrawCellScore);
        //设置扑克
        for(uint16 j = 0; j < GAME_PLAYER; ++j)
        {
            net::msg_cards *pCards = start_msg.add_table_card_data();
            for(int k = 0; k < m_cbCardCount[j];++k)
            {
                if(m_cbPlayStatus[j] == 0){
                    pCards->add_cards(0);
                    continue;
                }
                if(k == 0)
				{
                    if(i == j)
					{// 自己的暗牌
                       pCards->add_cards(m_cbHandCardData[j][k]);
                    }else
					{// 别人的暗牌
                       pCards->add_cards(0);
                    }
                    continue;
                }
                pCards->add_cards(m_cbHandCardData[j][k]);
            }
            start_msg.add_history_score(m_lHistoryScore[i]);
        }

		//LOG_DEBUG("------------------------------------------------------------------------------------");

		//for (WORD k = 0; k<GAME_PLAYER; k++)
		//{
		//	CGamePlayer * pGamePlayer = GetPlayer(k);
		//	uint32 uid = 0;
		//	if (pGamePlayer != NULL) {
		//		uid = pGamePlayer->GetUID();
		//	}
		//	net::msg_cards pCards = start_msg.table_card_data(k);
		//	for (int32 j = 0; j < pCards.cards_size(); j++)
		//	{
		//		LOG_DEBUG("send_hand_card - roomid:%d,tableid:%d,table_card_data_size:%d,cards_size:%d,i:%d,k:%d,j:%d,uid:%d,card_data:0x%02X",
		//			m_pHostRoom->GetRoomID(), GetTableID(), start_msg.table_card_data_size(), pCards.cards_size(),i, k, j, uid, pCards.cards(j));
		//	}
		//}

        SendMsgToClient(i,&start_msg,net::S2C_MSG_SHOWHAND_START);
    }
    
    m_coolLogic.beginCooling(s_AddScoreTime);
    SetRobotThinkTime();
    return true;
}
//游戏结束
bool CGameShowHandTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    //LOG_DEBUG("game end:%d--%d",chairID,reason);

	LOG_DEBUG("game_end - roomid:%d,tableid:%d,uid:%d,chairID:%d,reason:%d,m_wCurrentUser:%d,uids:%d %d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, reason, m_wCurrentUser, GetPlayerID(0), GetPlayerID(1));

    m_coolLogic.beginCooling(3000);
    m_coolRobot.beginCooling(3000);

    switch(reason)
    {
    case GER_NORMAL:		//常规结束
    case GER_NO_PLAYER:		//没有玩家
        {
            //定义变量
            net::msg_showhand_game_over_rep end_msg;
            //计算总注
            int64 lDrawScore=0L;
            for(WORD i=0;i<getArrayLen(m_lTableScore);i++) {
                lDrawScore += m_lTableScore[i];
            }
            //变量定义
            WORD wWinerUser = EstimateWinner(0,MAX_COUNT-1);
            //积分变量
            int64 ScoreInfoArray[GAME_PLAYER];
            memset(ScoreInfoArray,0,sizeof(ScoreInfoArray));

			int64 playerAllWinScore = 0; // 玩家总赢分 add by har

            // 统计积分
            for(WORD i=0;i<GAME_PLAYER;i++)
            {
                //设置扑克
                net::msg_cards* pCards = end_msg.add_card_data();
                if( m_cbPlayStatus[i] == TRUE )
                {
                    //成绩计算
                    int64 lUserScore = (i == wWinerUser)?(lDrawScore - m_lTableScore[i]): -m_lTableScore[i];
                    //设置积分
                    ScoreInfoArray[i] = lUserScore;

                    pCards->add_cards((reason!=GER_NO_PLAYER)?m_cbHandCardData[i][0]:0);
                }else{
                    pCards->add_cards(0);
                    ScoreInfoArray[i] = -m_lTableScore[i];
                }
				//计算梭哈数值
				int64 fee_value = CalcPlayerInfo(i, ScoreInfoArray[i]);
				ScoreInfoArray[i] += fee_value;
                //设置成绩
                end_msg.add_game_score(ScoreInfoArray[i]);
                end_msg.add_history_score(m_lHistoryScore[i]);

				CGamePlayer * pGamePlayer = GetPlayer(i);
				uint32 uid = 0;
				if (pGamePlayer != NULL) {
					uid = pGamePlayer->GetUID();
					// add by har
					if (!pGamePlayer->IsRobot())
						playerAllWinScore += ScoreInfoArray[i]; // add by har end
				}
				
				LOG_DEBUG("game end send - roomid:%d,tableid:%d,i:%d,uid:%d,wWinerUser:%d,ScoreInfoArray:%lld,reason:%d,m_lTurnMaxScore:%lld,m_lTurnLessScore:%lld,m_bShowHand:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
					m_pHostRoom->GetRoomID(), GetTableID(), i,uid, wWinerUser, ScoreInfoArray[i], reason, m_lTurnMaxScore, m_lTurnLessScore, m_bShowHand, m_cbHandCardData[i][0],m_cbHandCardData[i][1],m_cbHandCardData[i][2],m_cbHandCardData[i][3],m_cbHandCardData[i][4]);

            }
			
			end_msg.clear_hand_card_data();
			for (WORD i = 0; i < GAME_PLAYER; i++)
			{
				CGamePlayer * pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL)
				{
					continue;
				}

				net::msg_cards* pCards = end_msg.add_hand_card_data();
				for (uint32 j = 0; j < m_cbCardCount[i]; j++)
				{
					if (j == 0)
					{
						// 发给第1个玩家
						if (reason != GER_NO_PLAYER || i == 0)
						{
							pCards->add_cards(m_cbHandCardData[i][j]);
						}
						else
						{
							pCards->add_cards(0);

						}
					}
					else
					{
						pCards->add_cards(m_cbHandCardData[i][j]);
					}
				}
			}

			for (WORD i = 0; i < GAME_PLAYER; i++)
			{
				CGamePlayer * pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL || i == 1)
				{
					continue;
				}
				pGamePlayer->SendMsgToClient(&end_msg, net::S2C_MSG_SHOWHAND_GAME_END);
			}

			//LOG_DEBUG("------------------------------------------------------------------------------------");


			//for (WORD i = 0; i<GAME_PLAYER; i++)
			//{
			//	CGamePlayer * pGamePlayer = GetPlayer(i);
			//	uint32 uid = 0;
			//	if (pGamePlayer != NULL) {
			//		uid = pGamePlayer->GetUID();
			//	}
			//	net::msg_cards pCards = end_msg.hand_card_data(i);
			//	for (int32 j = 0; j < pCards.cards_size(); j++)
			//	{
			//		LOG_DEBUG("send_hand_card - roomid:%d,tableid:%d,card_data_size:%d,cards_size:%d,i:%d,j:%d,uid:%d,card_data:0x%02X", m_pHostRoom->GetRoomID(), GetTableID(), end_msg.card_data_size(), pCards.cards_size(), i, j, uid, pCards.cards(j));
			//	}
			//}

			end_msg.clear_hand_card_data();
			for (WORD i = 0; i < GAME_PLAYER; i++)
			{
				CGamePlayer * pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL)
				{
					continue;
				}

				net::msg_cards* pCards = end_msg.add_hand_card_data();
				for (uint32 j = 0; j < m_cbCardCount[i]; j++)
				{
					if (j == 0)
					{
						// 发给第2个玩家
						if (reason != GER_NO_PLAYER || i == 1)
						{
							pCards->add_cards(m_cbHandCardData[i][j]);
						}
						else
						{
							pCards->add_cards(0);

						}
					}
					else
					{
						pCards->add_cards(m_cbHandCardData[i][j]);
					}
				}
			}

			for (WORD i = 0; i < GAME_PLAYER; i++)
			{
				CGamePlayer * pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL || i == 0)
				{
					continue;
				}
				pGamePlayer->SendMsgToClient(&end_msg, net::S2C_MSG_SHOWHAND_GAME_END);
			}

			//LOG_DEBUG("------------------------------------------------------------------------------------");

			//for (WORD i = 0; i<GAME_PLAYER; i++)
			//{
			//	CGamePlayer * pGamePlayer = GetPlayer(i);
			//	uint32 uid = 0;
			//	if (pGamePlayer != NULL) {
			//		uid = pGamePlayer->GetUID();
			//	}
			//	net::msg_cards pCards = end_msg.hand_card_data(i);
			//	for (int32 j = 0; j < pCards.cards_size(); j++)
			//	{
			//		LOG_DEBUG("send_hand_card - roomid:%d,tableid:%d,card_data_size:%d,cards_size:%d,i:%d,j:%d,uid:%d,card_data:0x%02X", m_pHostRoom->GetRoomID(), GetTableID(), end_msg.card_data_size(), pCards.cards_size(), i, j, uid, pCards.cards(j));
			//	}
			//}

            // 结束消息
            //SendMsgToAll(&end_msg,net::S2C_MSG_SHOWHAND_GAME_END);

			int64 lUpdatePollScore = 0;
			for (uint16 i = 0; i<GAME_PLAYER; ++i)
			{
				CGamePlayer * pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL) continue;
				if (!pGamePlayer->IsRobot())
				{
					lUpdatePollScore += ScoreInfoArray[i];
				}
			}
			if (m_pHostRoom != NULL && lUpdatePollScore != 0)
			{
				m_pHostRoom->UpdateJackpotScore(-lUpdatePollScore);
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
							pGamePlayer->SetLuckyInfo(GetRoomID(), ScoreInfoArray[i]);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), ScoreInfoArray[i]);
						}
					}
				}
			}

			//更新新注册玩家福利数据   
			if (IsNewRegisterWelfareTable())
			{
				for (WORD i = 0; i < GAME_PLAYER; i++)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL && !pGamePlayer->IsRobot())
					{
						int64 curr_win = ScoreInfoArray[i];
						if (pGamePlayer->GetUID() == m_nrw_ctrl_uid)
						{							
							if (m_nrw_status == 2 && curr_win > 0)
							{
								pGamePlayer->UpdateNRWPlayerScore(curr_win);
								LOG_DEBUG("control current player is lost. but current player is win. uid:%d score:%d", m_nrw_ctrl_uid, curr_win);
							}
							else
							{
								UpdateNewRegisterWelfareInfo(m_nrw_ctrl_uid, curr_win);
							}
						}
						else
						{
							pGamePlayer->UpdateNRWPlayerScore(curr_win);
						}
					}						
				}
			}			

            //更新活跃福利数据  
            for (WORD i = 0; i < GAME_PLAYER; i++)
            {
                CGamePlayer * pGamePlayer = GetPlayer(i);
                if (pGamePlayer != NULL && pGamePlayer->GetUID()== m_aw_ctrl_uid)
                {
                    int64 curr_win = ScoreInfoArray[i];
                    UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);
                }
            }                        

			WriteJackpotScoreInfo();
			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,playerAllWinScore:%lld", GetRoomID(), GetTableID(), playerAllWinScore);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
            //结束游戏
            ResetGameData();
            SetGameState(TABLE_STATE_FREE);
            ResetPlayerReady();
            SendSeatInfoToClient();
            SaveBlingLog();
			OnTableGameEnd();
            return true;
        }break;
    case GER_DISMISS:		//游戏解散
    {
        LOG_ERROR("force game over");
        for(uint8 i=0;i<GAME_PLAYER;++i)
        {
            if(m_vecPlayers[i].pPlayer != NULL) {
                LeaveTable(m_vecPlayers[i].pPlayer);
            }
        }
        ResetTable();
        return true;
    }
    case GER_NETWORK_ERROR:		//用户强退
    case GER_USER_LEAVE:
    default:
        break;
    }
    //错误断言
    assert(false);
    return false;
}
//用户同意
bool    CGameShowHandTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    if(m_coolLogic.isTimeOut()){
        m_coolLogic.beginCooling(500);// 准备后等一秒
    }
    return true;
}
//玩家进入或离开
void  CGameShowHandTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);

    //m_coolRobot.beginCooling(3000);
    m_lHistoryScore[chairID] = 0;
    m_isOnlyRobot = true;
    for(uint8 i=0;i<GAME_PLAYER;++i)
	{
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && !pPlayer->IsRobot())
		{
            m_isOnlyRobot = false;
            break;
        }
    }
    if(GetChairPlayerNum() < GAME_PLAYER)
	{
        m_isOnlyRobot = false;
    }
	SetRobotThinkTime();
}
// 发送场景信息(断线重连)
void    CGameShowHandTable::SendGameScene(CGamePlayer* pPlayer)
{
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("send game scene:%d", chairID);
    if(GetGameState() == net::TABLE_STATE_FREE) {
        return;
    }else{
        //构造数据
        net::msg_showhand_game_info_rep msg;
        //设置变量
        msg.set_cell_score(m_lDrawCellScore);

        //加注信息
        msg.set_draw_max_score(m_lDrawMaxScore);
        msg.set_turn_max_score(m_lTurnMaxScore);
        msg.set_turn_less_score(m_lTurnLessScore);
        for(uint16 i=0;i<GAME_PLAYER;++i){
            msg.add_user_score(m_lUserScore[i]);
            msg.add_table_score(m_lTableScore[i]);
        }

        //状态信息
        msg.set_cur_user(m_wCurrentUser);
        msg.set_show_hand((m_bShowHand==true)?1:0);
        msg.set_wait_time(m_coolLogic.getCoolTick());
        for(uint16 i=0;i<GAME_PLAYER;++i){
            msg.add_play_status(m_cbPlayStatus[i]);
            msg.add_history_score(m_lHistoryScore[i]);
        }

        //历史积分

        //设置扑克
        for (WORD i=0;i<GAME_PLAYER;i++)
        {
            //设置数目
            net::msg_cards* pCards = msg.add_hand_card_data();
            //设置扑克
            if(m_cbPlayStatus[i]==TRUE)
            {
                if(i==chairID){
                    pCards->add_cards(m_cbHandCardData[i][0]);
                }else{
                    pCards->add_cards(0);
                }
                for(uint16 j=1;j<m_cbCardCount[i];++j)
                {
                    pCards->add_cards(m_cbHandCardData[i][j]);
                }
            }
        }
        //发送场景
        SendMsgToClient(chairID,&msg,net::S2C_MSG_SHOWHAND_GAME_INFO);
    }
}
int64    CGameShowHandTable::CalcPlayerInfo(uint16 chairID,int64 winScore)
{
    LOG_DEBUG("report game to lobby:%d  %lld",chairID,winScore);
	//if (winScore == 0)
	//{
	//	return 0;
	//}
    uint32 uid = m_vecPlayers[chairID].uid;
    int64 fee_value = CalcPlayerGameInfo(uid,winScore);
    // 修改梭哈数据
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        uint8 curMaxCard[5];
        pPlayer->GetGameMaxCard(net::GAME_CATE_SHOWHAND,isCoin,curMaxCard,5);
        BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
        memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
        m_gameLogic.SortCardList(cbUserCardData[chairID],m_cbCardCount[chairID]);
        if(m_gameLogic.CompareCard(cbUserCardData[chairID],curMaxCard,m_cbCardCount[chairID]) == false){
            pPlayer->AsyncSetGameMaxCard(net::GAME_CATE_SHOWHAND,isCoin,cbUserCardData[chairID],m_cbCardCount[chairID]);            
        }
    }
    m_lHistoryScore[chairID] += (winScore+ fee_value);
    // 写入手牌log
	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

    WriteOutCardLog(chairID, cbUserCardData[chairID],m_cbCardCount[chairID]);
	return fee_value;
}
// 重置游戏数据
void    CGameShowHandTable::ResetGameData()
{
    //游戏变量
    m_bShowHand    = false;
    m_wCurrentUser = INVALID_CHAIR;

    //下注信息
    m_lDrawMaxScore  = 0L;
    m_lTurnMaxScore  = 0L;
    m_lTurnLessScore = 0L;
    m_lDrawCellScore = 0L;

    //用户状态
    memset(m_cbPlayStatus,0,sizeof(m_cbPlayStatus));
    memset(m_cbOperaScore,0,sizeof(m_cbOperaScore));

    //金币信息
    memset(m_lUserScore,0,sizeof(m_lUserScore));
    memset(m_lTableScore,0,sizeof(m_lTableScore));
    memset(m_lUserMaxScore,0,sizeof(m_lUserMaxScore));

    //扑克变量
    m_cbSendCardCount=0;
    memset(m_cbCardCount,0,sizeof(m_cbCardCount));
    memset(m_cbHandCardData,0,sizeof(m_cbHandCardData));

    m_robotAIInfo.clear();
	m_bIsControlPlayer = false;
}


bool    CGameShowHandTable::SetControlPalyerWin(uint32 control_uid)
{
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));

	WORD maxChairID = EstimateWinner(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar == NULL || pTar->GetUID() == control_uid) {
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
			LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i:%d,maxchairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID);
	return false;
}

bool    CGameShowHandTable::SetControlPalyerLost(uint32 control_uid)
{
	LOG_DEBUG("SetControlPalyerLost control_uid:%d", control_uid);
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));

	uint16 minChairID = EstimateLoser(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(minChairID);
	if (pTar == NULL || pTar->GetUID() == control_uid) {
		LOG_DEBUG("min card is - roomid:%d,tableid:%d,minChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID, control_uid);
		return true;
	}
	LOG_DEBUG("control_uid:%d minChairID:%d", control_uid, minChairID);

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->GetUID() == control_uid)
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);
			LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,minChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, minChairID);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,minChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID);
	return false;
}

bool CGameShowHandTable::SetRobotWin()
{
	//int irount_count = 1000;
	//if (GetOnlinePlayerNum() == 0) {
	//	m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));
	//}
	//else if (GetOnlinePlayerNum() == GAME_PLAYER) {
	//	uint16 maxNum = g_RandGen.RandRange(8000, PRO_DENO_10000) ? 0 : 2;//做牌率
	//	m_needChangeCard = g_RandGen.RandRatio(5000, PRO_DENO_10000);//三张含对子概率
	//	m_makeType5 = maxNum > 0 ? true : false;
	//	m_makeType3 = m_needChangeCard;
	//	//maxNum = 2

	//	for (int i = 0; i<irount_count; ++i)
	//	{
	//		m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));
	//		BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	//		memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));
	//		uint16 cardNum = 0;
	//		for (uint16 j = 0; j<GAME_PLAYER; ++j)
	//		{
	//			if (m_gameLogic.IsMatchedCardType(cbUserCardData[j], MAX_COUNT, m_makeType5, m_makeType3) == false)
	//				cardNum++;
	//			if (cardNum > maxNum)
	//				break;
	//		}
	//		if (cardNum > maxNum) {
	//			continue;
	//		}
	//		else {
	//			break;
	//		}
	//	}
	//}
	//else {
		//bool needChangeCard = g_RandGen.RandRatio(m_robotBankerWinPro, PRO_DENO_10000);

		bool brobotwin = false;

		int iIndex = 0;
		WORD wWinerUser = GAME_PLAYER + 1;
		int irount_count = 1000;
		for (; iIndex<irount_count; ++iIndex)
		{
			m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));
			//if (!needChangeCard) {
			//	break;
			//}
			wWinerUser = EstimateWinner(0, MAX_COUNT - 1);
			for (WORD i = 0; i<GAME_PLAYER; i++)
			{
				if (i == wWinerUser)
				{
					CGamePlayer *pGamePlayer = GetPlayer(i);
					if (pGamePlayer == NULL) {
						break;
					}
					if (pGamePlayer->IsRobot())
					{
						brobotwin = true;

						BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
						memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));
						for (uint16 j = 0; j < GAME_PLAYER; ++j) {
							if (i == j) {
								continue;
							}
							m_gameLogic.GetRandCardType(cbUserCardData[j], MAX_COUNT, m_makeType5, m_makeType3);
						}
						break;
					}
				}
			}
			if (brobotwin)
			{
				break;
			}
		}

		//LOG_DEBUG("robot win - roomid:%d,tableid:%d,m_robotBankerWinPro:%d,needChangeCard:%d,iIndex:%d,wWinerUser:%d,brobotwin:%d,m_cbHandCardData[0][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X],m_cbHandCardData[1][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X]", m_pHostRoom->GetRoomID(), GetTableID(), m_robotBankerWinPro, needChangeCard, iIndex, wWinerUser, brobotwin, m_cbHandCardData[0][0], m_cbHandCardData[0][1], m_cbHandCardData[0][2], m_cbHandCardData[0][3], m_cbHandCardData[0][4], m_cbHandCardData[1][0], m_cbHandCardData[1][1], m_cbHandCardData[1][2], m_cbHandCardData[1][3], m_cbHandCardData[1][4]);

	//}
	return true;
}

bool CGameShowHandTable::SetRobotLostScore()
{
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));

	uint16 minChairID = EstimateLoser(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(minChairID);
	if (pTar == NULL || pTar->IsRobot()) {
		//LOG_DEBUG("min card is - roomid:%d,tableid:%d,minChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->IsRobot())
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);
			//LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,minChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, minChairID);
			return true;
		}
	}
	//LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,minChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID);

	return false;
}

bool CGameShowHandTable::SetRobotWinScore()
{
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));

	WORD maxChairID = EstimateWinner(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar == NULL || pTar->IsRobot()) {
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
			//LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i:%d,maxchairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}
	//LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,maxChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), maxChairID);
	return false;
}

int     CGameShowHandTable::GetProCardType()
{
	int iSumValue = 0;
	int iArrDispatchCardPro[ShowHand_Pro_Index_MAX] = { 0 };
	for (int i = 0; i < ShowHand_Pro_Index_MAX; i++)
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


	for (; iProIndex < ShowHand_Pro_Index_MAX; iProIndex++)
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
	if (iProIndex >= ShowHand_Pro_Index_MAX)
	{
		iProIndex = ShowHand_Pro_Index_Single;
	}
	return iProIndex;
}


int     CGameShowHandTable::GetWelfareCardType()
{
	int iSumValue = 0;
	int iArrDispatchCardPro[ShowHand_Pro_Index_MAX] = { 0 };
	for (int i = 0; i < ShowHand_Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_iArrWelfareCardPro[i];
		iSumValue += m_iArrWelfareCardPro[i];
	}
	LOG_DEBUG("roomid:%d,tableid:%d,iSumValue:%d", GetRoomID(), GetTableID(), iSumValue);

	if (iSumValue <= 0)
	{
		return 0;
	}
	int iRandNum = g_RandGen.RandRange(0, iSumValue);
	int iProIndex = 0;

	for (; iProIndex < ShowHand_Pro_Index_MAX; iProIndex++)
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
	if (iProIndex >= ShowHand_Pro_Index_MAX)
	{
		iProIndex = ShowHand_Pro_Index_Single;
	}
	return iProIndex;
}

bool	CGameShowHandTable::ProbabilityDispatchPokerCard(int type)
{
	bool bIsFlag = true;
	int iArProCardType[GAME_PLAYER] = { 0 };
	// 先确定每一个人获取的类型
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		iArProCardType[i] = ShowHand_Pro_Index_MAX;

		if (m_cbPlayStatus[i] == TRUE)
		{
			int iProIndex = ShowHand_Pro_Index_Single;
			if (type == DISPATCH_TYPE_Room)
			{
				iProIndex = GetProCardType();
			}
			else if (type == DISPATCH_TYPE_Welfare)
			{
				iProIndex = GetWelfareCardType();
			}			
			if (iProIndex < ShowHand_Pro_Index_MAX)
			{
				iArProCardType[i] = iProIndex;
			}

			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbPlayStatus:%d,iProIndex:%d,type:%d",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbPlayStatus[i], iProIndex, type);

		}
	}
	// 根据类型获取全部的手牌
	bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData);

	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2], m_cbHandCardData[i][3], m_cbHandCardData[i][4]);
		}
	}

	return bIsFlag;
}


CGamePlayer* CGameShowHandTable::HaveWelfareNovicePlayer()
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

void CGameShowHandTable::InitHaveWinPreHandCard()
{
	m_mpWinPreWelfareHandCard.clear();
	std::vector<BYTE> vecSubCardData_ten{ 0x0A, 0x01, 0x0B, 0x0C, 0x0D };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x0A, vecSubCardData_ten));

	std::vector<BYTE> vecSubCardData_eleven{ 0x0B, 0x01, 0x0C, 0x0D };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x0B, vecSubCardData_eleven));

	std::vector<BYTE> vecSubCardData_twelve{ 0x0C, 0x01, 0x0D };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x0C, vecSubCardData_twelve));

	std::vector<BYTE> vecSubCardData_thirteen{ 0x0D, 0x01 };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x0D, vecSubCardData_thirteen));

	std::vector<BYTE> vecSubCardData_one{ 0x01 };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x01, vecSubCardData_one));

	std::vector<BYTE> vecSubCardData_eight{ 0x08 };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x08, vecSubCardData_eight));

	std::vector<BYTE> vecSubCardData_nine{ 0x09 };
	m_mpWinPreWelfareHandCard.insert(std::make_pair(0x09, vecSubCardData_nine));

	//for (BYTE i = 1; i <= 9; i++)
	//{
	//	std::vector<BYTE> vecSubCardData;
	//	vecSubCardData.push_back(i);
	//	m_mpWinPreWelfareHandCard.insert(std::make_pair(i, vecSubCardData));
	//}
}

bool	CGameShowHandTable::SetControlNoviceWelfarePalyerWin(uint32 control_uid)
{
	uint16 uCtrlChairID = 255;
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
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

	bool bRetPlayerWinScore = false;
	int iLoopIndex = 0;

	string strTempCardDataSta;
	string strTempCardDataFor;
	string strTempCardDataEnd;


	if (uCtrlChairID < GAME_PLAYER)
	{
		ProbabilityDispatchPokerCard(DISPATCH_TYPE_Welfare);
		for (; iLoopIndex < 1024; iLoopIndex++)
		{
			bool bIsOtherDispatchCard = true;
			bRetPlayerWinScore = SetControlPalyerWin(control_uid);
			if (bRetPlayerWinScore)
			{
				BYTE cbTempCardData[MAX_COUNT] = {0};
				memcpy(cbTempCardData, m_cbHandCardData[uCtrlChairID], MAX_COUNT);
				BYTE cbTempCardValue[MAX_COUNT] = { 0 };
				for (int i = 0; i < MAX_COUNT; i++)
				{
					cbTempCardValue[i] = m_gameLogic.GetCardValue(cbTempCardData[i]);
				}
				BYTE cbTempCardColor[MAX_COUNT] = { 0 };
				for (int i = 0; i < MAX_COUNT; i++)
				{
					cbTempCardColor[i] = m_gameLogic.GetCardColor(cbTempCardData[i]);
				}
				for (int i = 0; i < MAX_COUNT; i++)
				{
					BYTE cbCardValueOne = cbTempCardValue[i];
					auto iter_find = m_mpWinPreWelfareHandCard.find(cbCardValueOne);
					if (iter_find != m_mpWinPreWelfareHandCard.end())
					{
						auto & vecTempCardData = iter_find->second;
						for (int j = 0; j < MAX_COUNT; j++)
						{
							BYTE cbCardValueTwo = cbTempCardValue[j];
							if (j == i)
							{
								continue;
							}
							auto iter_two = find(vecTempCardData.begin(), vecTempCardData.end(), cbCardValueTwo);
							if (iter_two != vecTempCardData.end())
							{
								if (i != j)
								{
									strTempCardDataSta += CStringUtility::FormatToString("control_uid:%d,uCtrlChairID:%d,i:%d,j:%d,cbCardValue:0x%02X_0x%02X,cd:0x%02X_0x%02X_0x%02X_0x%02X_0x%02X ",
										control_uid, uCtrlChairID, i, j, cbCardValueOne, cbCardValueTwo,cbTempCardData[0], cbTempCardData[1], cbTempCardData[2], cbTempCardData[3], cbTempCardData[4]);

									strTempCardDataFor += CStringUtility::FormatToString("control_uid:%d,uCtrlChairID:%d,i:%d,j:%d,cd:0x%02X_0x%02X_0x%02X_0x%02X_0x%02X ",
										control_uid, uCtrlChairID, i, j, m_cbHandCardData[uCtrlChairID][0], m_cbHandCardData[uCtrlChairID][1], m_cbHandCardData[uCtrlChairID][2], m_cbHandCardData[uCtrlChairID][3], m_cbHandCardData[uCtrlChairID][4]);

									bIsOtherDispatchCard = false;
									if (i != 0 && i != 1 && j != 0 && j != 1)
									{
										BYTE cbOneCardData = m_cbHandCardData[uCtrlChairID][0];
										m_cbHandCardData[uCtrlChairID][0] = m_cbHandCardData[uCtrlChairID][i];
										m_cbHandCardData[uCtrlChairID][i] = cbOneCardData;

										BYTE cbTwoCardData = m_cbHandCardData[uCtrlChairID][1];
										m_cbHandCardData[uCtrlChairID][1] = m_cbHandCardData[uCtrlChairID][j];
										m_cbHandCardData[uCtrlChairID][j] = cbTwoCardData;
									}
									else if (i == 0 && i != 1 && j != 0 && j != 1)
									{
										BYTE cbTwoCardData = m_cbHandCardData[uCtrlChairID][1];
										m_cbHandCardData[uCtrlChairID][1] = m_cbHandCardData[uCtrlChairID][j];
										m_cbHandCardData[uCtrlChairID][j] = cbTwoCardData;
									}
									else if (i != 0 && i == 1 && j != 0 && j != 1)
									{
										BYTE cbTwoCardData = m_cbHandCardData[uCtrlChairID][0];
										m_cbHandCardData[uCtrlChairID][0] = m_cbHandCardData[uCtrlChairID][j];
										m_cbHandCardData[uCtrlChairID][j] = cbTwoCardData;
									}
									else if (i != 0 && i != 1 && j == 0 && j != 1)
									{
										BYTE cbOneCardData = m_cbHandCardData[uCtrlChairID][1];
										m_cbHandCardData[uCtrlChairID][1] = m_cbHandCardData[uCtrlChairID][i];
										m_cbHandCardData[uCtrlChairID][i] = cbOneCardData;
									}
									else if (i != 0 && i != 1 && j != 0 && j == 1)
									{
										BYTE cbOneCardData = m_cbHandCardData[uCtrlChairID][0];
										m_cbHandCardData[uCtrlChairID][0] = m_cbHandCardData[uCtrlChairID][i];
										m_cbHandCardData[uCtrlChairID][i] = cbOneCardData;
									}
									strTempCardDataEnd += CStringUtility::FormatToString("control_uid:%d,uCtrlChairID:%d,i:%d,j:%d,cd:0x%02X_0x%02X_0x%02X_0x%02X_0x%02X ",
										control_uid, uCtrlChairID, i, j, m_cbHandCardData[uCtrlChairID][0], m_cbHandCardData[uCtrlChairID][1], m_cbHandCardData[uCtrlChairID][2], m_cbHandCardData[uCtrlChairID][3], m_cbHandCardData[uCtrlChairID][4]);

								}
							}
							if (bIsOtherDispatchCard == false)
							{
								break;
							}
						}
					}
					if (bIsOtherDispatchCard == false)
					{
						break;
					}
				}
			}
			if (bIsOtherDispatchCard)
			{
				//重新发牌
				ProbabilityDispatchPokerCard(DISPATCH_TYPE_Welfare);
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
			strHandCardData += CStringUtility::FormatToString("i:%d,uid:%d,cd:0x%02X_0x%02X_0x%02X_0x%02X_0x%02X ",
				i, GetPlayerID(i), m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2], m_cbHandCardData[i][3], m_cbHandCardData[i][4]);
		}
	}

	LOG_DEBUG("roomid:%d,tableid:%d,strTempCardDataSta_%s,For_%s,End_%s",
		GetRoomID(), GetTableID(), strTempCardDataSta.c_str(), strTempCardDataFor.c_str(), strTempCardDataEnd.c_str());

	LOG_DEBUG("roomid:%d,tableid:%d,control_uid:%d,uCtrlChairID:%d,iLoopIndex:%d,strHandCardData:%s",
		GetRoomID(),GetTableID(), control_uid, uCtrlChairID, iLoopIndex, strHandCardData.c_str());

	return bRetPlayerWinScore;
}


bool CGameShowHandTable::NoviceWelfareCtrlWinScore()
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
			tagTempValue.frontIsHitWelfare = 0;
			tagTempValue.jettonCount++;
		}
	}

	LOG_DEBUG("dos_wel_ctrl - roomid:%d,tableid:%d,isnewnowe:%d,newmaxjetton:%lld,newsmaxwin:%lld,lNoviceWinScore:%lld,IsNoviceWelfareCtrl:%d,noviceuid:%d,posrmb:%d,ChessWelfare:%d,welfarepro:%d,real_welfarepro:%d,lift_odds:%d,pPlayer:%p,fUseHitWelfare:%d,frontIsHitWelfare:%d,jettonCount:%d,bIsHitWelfarePro:%d",
		GetRoomID(), GetTableID(), isnewnowe, newmaxjetton, newsmaxwin, lNoviceWinScore, bIsNoviceWelfareCtrl, noviceuid, posrmb, GetChessWelfare(), NewPlayerWelfareValue.welfarepro, real_welfarepro, NewPlayerWelfareValue.lift_odds, pPlayer, fUseHitWelfare, tagValue.frontIsHitWelfare, tagValue.jettonCount, bIsHitWelfarePro);

	return bIsNoviceWelfareCtrl;
}




// 做牌发牌
void CGameShowHandTable::DispatchCard()
{	
	m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));

	ProbabilityDispatchPokerCard(DISPATCH_TYPE_Room);

	bool bIsFalgControl = false;

    //系统控制个人
	bIsFalgControl = ProgressControlPalyer();
	m_bIsControlPlayer = bIsFalgControl;

	//幸运值控制
	bool bIsLuckyCtrl = false;
	if (!bIsFalgControl)
	{
		bIsLuckyCtrl = SetLuckyCtrl();
	}

	//新玩家福利
	bool bIsNoviceWelfareCtrl = false;
	if (!bIsFalgControl && !bIsLuckyCtrl)
	{
		bIsNoviceWelfareCtrl = NoviceWelfareCtrlWinScore();
	}

	// 新注册玩家福利控制
	bool bIsNRWControl = false;
	if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl)
	{
		bIsNRWControl = NewRegisterWelfareCtrl();
	}
	
	bool bIsWinMaxScorePlayerLost = false;
	if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNRWControl && !bIsNoviceWelfareCtrl)
	{
		//玩家最多能赢
		bIsWinMaxScorePlayerLost = SetWinMaxScorePlayerLost();
	}

	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	bool bIsPoolScoreControl = false;
	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);

	if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsNRWControl && !bIsWinMaxScorePlayerLost && tmpJackpotScore.iUserJackpotControl == 1)
	{
		if (tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // 吐币
		{
			bIsPoolScoreControl = true;
			SetRobotLostScore();

		}
		if (tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // 吃币
		{
			bIsPoolScoreControl = true;
			SetRobotWinScore();
		}
	}

	bool bIsSetRobotWin = false;
	bool needChangeCard = g_RandGen.RandRatio(m_robotBankerWinPro, PRO_DENO_10000);
	if (needChangeCard && !bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsNRWControl && !bIsWinMaxScorePlayerLost && !bIsPoolScoreControl)
	{
		bIsSetRobotWin = true;

		SetRobotWinScore();
	}

	//bool IsHaveAotoKillScore = false;
	//if (!bIsSetRobotWin && !bIsFalgControl && !bIsWinMaxScorePlayerLost && !bIsPoolScoreControl)
	//{
	//	IsHaveAotoKillScore = IsHaveAutoKillScorePlayer();

	//	if (IsHaveAotoKillScore)
	//	{
	//		SetRobotWinScore();
	//	}
	//}

	// add by har
	bool bIsStockControl = false;
	if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsNRWControl && !bIsWinMaxScorePlayerLost && !needChangeCard && !bIsPoolScoreControl) {
		bIsStockControl = SetStockWinLose();
	}
	bool isRobot0 = true;
	bool isRobot1 = true;
	CGamePlayer* pTmp0 = GetPlayer(0);
	CGamePlayer* pTmp1 = GetPlayer(1);
	if (pTmp0 != NULL && !pTmp0->IsRobot())
		isRobot0 = false;
	if (pTmp1 != NULL && !pTmp1->IsRobot())
		isRobot1 = false;
	// add by har end

	//活跃福利控制
	bool bIsAWCtrl = false;
	if (!bIsFalgControl && !bIsLuckyCtrl && !bIsNoviceWelfareCtrl && !bIsNRWControl && !bIsWinMaxScorePlayerLost && !needChangeCard && !bIsPoolScoreControl && !bIsStockControl)
	{
		bIsAWCtrl = ActiveWelfareCtrl();
	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("roomid:%d,tableid:%d,bIsFalgControl:%d,bIsLuckyCtrl:%d,bIsAWCtrl:%d,bIsNRWControl:%d,bIsNoviceWelfareCtrl:%d,bIsWinMaxScorePlayerLost:%d,bIsPoolScoreControl:%d,iUserJackpotControl:%d,needChangeCard:%d,bIsSetRobotWin:%d,bIsStockControl:%d,GetIsAllRobotOrPlayerJetton:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X isRobot0:%d - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X isRobot1:%d",
		GetRoomID(), GetTableID(), bIsFalgControl, bIsLuckyCtrl, bIsAWCtrl, bIsNRWControl, bIsNoviceWelfareCtrl, bIsWinMaxScorePlayerLost, bIsPoolScoreControl, tmpJackpotScore.iUserJackpotControl, needChangeCard, bIsSetRobotWin, bIsStockControl, GetIsAllRobotOrPlayerJetton(),
		m_cbHandCardData[0][0], m_cbHandCardData[0][1], m_cbHandCardData[0][2], m_cbHandCardData[0][3], m_cbHandCardData[0][4], isRobot0, m_cbHandCardData[1][0], m_cbHandCardData[1][1], m_cbHandCardData[1][2], m_cbHandCardData[1][3], m_cbHandCardData[1][4], isRobot1);

	bool btestflag = false;
	if (btestflag)
	{
		m_cbHandCardData[0][0] = 0x01;
		m_cbHandCardData[0][1] = 0x11;
		m_cbHandCardData[0][2] = 0x21;
		m_cbHandCardData[0][3] = 0x31;
		m_cbHandCardData[0][4] = 0x0D;

		//m_cbHandCardData[1][0] = 0x0A;
		//m_cbHandCardData[1][1] = 0x1C;
		//m_cbHandCardData[1][2] = 0x2D;
		//m_cbHandCardData[1][3] = 0x38;
		//m_cbHandCardData[1][4] = 0x19;

		//m_cbHandCardData[1][0] = 0x09;
		//m_cbHandCardData[1][1] = 0x0A;
		//m_cbHandCardData[1][2] = 0x0B;
		//m_cbHandCardData[1][3] = 0x0C;
		//m_cbHandCardData[1][4] = 0x0D;

		//m_cbHandCardData[1][0] = 0x0A;
		//m_cbHandCardData[1][1] = 0x1A;
		//m_cbHandCardData[1][2] = 0x2A;
		//m_cbHandCardData[1][3] = 0x19;
		//m_cbHandCardData[1][4] = 0x29;

		m_cbHandCardData[1][0] = 0x01;
		m_cbHandCardData[1][1] = 0x18;
		m_cbHandCardData[1][2] = 0x29;
		m_cbHandCardData[1][3] = 0x0A;
		m_cbHandCardData[1][4] = 0x2B;

	}

 //   for(uint16 j=0;j<GAME_PLAYER;++j)
	//{
	//	//藏牌率
 //       if(m_gameLogic.GetCardLogicValue(m_cbHandCardData[j][1]) == m_gameLogic.GetCardLogicValue(m_cbHandCardData[j][2]) && g_RandGen.RandRatio(6000, PRO_DENO_10000))
 //       {
 //           uint8 tmp = m_cbHandCardData[j][0];
 //           m_cbHandCardData[j][0] = m_cbHandCardData[j][1];
 //           m_cbHandCardData[j][1] = tmp;
 //       }
	//}
    
}


bool    CGameShowHandTable::SetControlPalyerLost_card_type(uint32 control_uid)
{
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));
	int iRandProCardType[] = {
		ShowHand_Pro_Index_TongHuaShun ,
		ShowHand_Pro_Index_TieZhi,
		ShowHand_Pro_Index_HuLu,
		ShowHand_Pro_Index_TongHua,
		ShowHand_Pro_Index_ShunZi,
		ShowHand_Pro_Index_ThreeTiao ,
		ShowHand_Pro_Index_TwoDouble ,
		ShowHand_Pro_Index_OneDouble };

	bool bIsFlag = true;
	int iArProCardType[GAME_PLAYER] = { 0 };
	// 先确定每一个人获取的类型
	int iRandCount = 999;
	for (int iRandIndex = 0; iRandIndex < iRandCount; iRandIndex++)
	{
		for (uint16 i = 0; i < GAME_PLAYER; ++i)
		{
			int index = g_RandGen.RandRange(0, getArrayLen(iRandProCardType) - 1);
			iArProCardType[i] = iRandProCardType[index];
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,index:%d,iArProCardType:%d,m_cbPlayStatus:%d",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, index, iArProCardType[i], m_cbPlayStatus[i]);
		}
		// 根据类型获取全部的手牌
		bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData);

		BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
		memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

		BYTE cbFirstGenre = m_gameLogic.GetCardGenre(cbUserCardData[0], MAX_COUNT);
		BYTE cbNextGenre = m_gameLogic.GetCardGenre(cbUserCardData[1], MAX_COUNT);
		if (cbFirstGenre != CT_SINGLE && cbNextGenre != CT_SINGLE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,cbFirstGenre:%d,cbNextGenre:%d", GetRoomID(), GetTableID(), cbFirstGenre, cbNextGenre);
			break;
		}
	}

	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2], m_cbHandCardData[i][3], m_cbHandCardData[i][4]);
		}
	}


	uint16 minChairID = EstimateLoser(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(minChairID);
	if (pTar == NULL || pTar->GetUID() == control_uid) {
		LOG_DEBUG("min card is - roomid:%d,tableid:%d,minChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID, control_uid);
		return true;
	}

	for (uint16 i = 0; i<GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE || i == minChairID)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL && pTmp->GetUID() == control_uid)
		{
			uint8 tmp[MAX_COUNT];
			memcpy(tmp, m_cbHandCardData[i], MAX_COUNT);
			memcpy(m_cbHandCardData[i], m_cbHandCardData[minChairID], MAX_COUNT);
			memcpy(m_cbHandCardData[minChairID], tmp, MAX_COUNT);
			LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i%d,minChairID:%d", m_pHostRoom->GetRoomID(), GetTableID(), control_uid, i, minChairID);
			return true;
		}
	}
	LOG_DEBUG("changer is no find - roomid:%d,tableid:%d,minChairID:%d,control_uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), minChairID, control_uid);
	return false;
}

bool    CGameShowHandTable::SetRobotWinScore_card_type()
{
	//m_gameLogic.RandCardList(m_cbHandCardData[0], sizeof(m_cbHandCardData) / sizeof(m_cbHandCardData[0][0]));
	int iRandProCardType[] = {
		ShowHand_Pro_Index_TongHuaShun ,
		ShowHand_Pro_Index_TieZhi,
		ShowHand_Pro_Index_HuLu,
		ShowHand_Pro_Index_TongHua,
		ShowHand_Pro_Index_ShunZi,
		ShowHand_Pro_Index_ThreeTiao ,
		ShowHand_Pro_Index_TwoDouble ,
		ShowHand_Pro_Index_OneDouble };

	bool bIsFlag = true;
	int iArProCardType[GAME_PLAYER] = { 0 };
	// 先确定每一个人获取的类型
	int iRandCount = 999;
	for (int iRandIndex = 0; iRandIndex < iRandCount; iRandIndex++)
	{
		for (uint16 i = 0; i < GAME_PLAYER; ++i)
		{
			int index = g_RandGen.RandRange(0, getArrayLen(iRandProCardType) - 1);
			iArProCardType[i] = iRandProCardType[index];
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,index:%d,iArProCardType:%d,m_cbPlayStatus:%d",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, index, iArProCardType[i], m_cbPlayStatus[i]);
		}
		// 根据类型获取全部的手牌
		bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData);

		BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
		memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

		BYTE cbFirstGenre = m_gameLogic.GetCardGenre(cbUserCardData[0], MAX_COUNT);
		BYTE cbNextGenre = m_gameLogic.GetCardGenre(cbUserCardData[1], MAX_COUNT);
		if (cbFirstGenre != CT_SINGLE && cbNextGenre != CT_SINGLE )
		{
			LOG_DEBUG("roomid:%d,tableid:%d,cbFirstGenre:%d,cbNextGenre:%d",GetRoomID(), GetTableID(), cbFirstGenre, cbNextGenre);
			break;
		}
	}
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == TRUE)
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
				GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2], m_cbHandCardData[i][3], m_cbHandCardData[i][4]);
		}
	}


	WORD maxChairID = EstimateWinner(0, MAX_COUNT - 1);

	CGamePlayer* pTar = GetPlayer(maxChairID);
	if (pTar == NULL || pTar->IsRobot()) {
		//LOG_DEBUG("max card is - roomid:%d,tableid:%d,maxChairID:%d,control_uid:%d", GetRoomID(), GetTableID(), maxChairID, control_uid);
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
			//LOG_DEBUG("changer card success - roomid:%d,tableid:%d,control_uid:%d,i:%d,maxchairID:%d", GetRoomID(), GetTableID(), control_uid, i, maxChairID);
			return true;
		}
	}

	return false;
}

bool	CGameShowHandTable::ProgressControlPalyer()
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
		if (IsReady(pPlayer))
		{
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
	}

	if (vecControlMultiPlayerLost.size() > 0)
	{
		uint32 uLostPlayerUid = 0;
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
	if (!bIsFalgControlMulti)
	{
		if (vecControlMultiPlayerWin.size() > 0)
		{
			uint32 uWinPlayerUid = 0;
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
	
	if (bIsFalgControlMulti)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,bIsFalgControlMulti:%d", GetRoomID(), GetTableID(), bIsFalgControlMulti);

		return bIsFalgControlMulti;
	}

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
			{
				continue;
			}
			if (pPlayer->GetUID() == control_uid && IsReady(pPlayer))
			{
				bIsControlPlayerIsReady = true;
				break;
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
	}

	LOG_DEBUG("roomid:%d,tableid:%d,control_uid:%d,game_count:%d,control_type:%d,bIsControlPlayerIsReady:%d",
		GetRoomID(), GetTableID(), control_uid, game_count, control_type, bIsControlPlayerIsReady);


	return bIsFalgControl;
}
bool    CGameShowHandTable::SetWinMaxScorePlayerLost()
{
	bool bIsHaveRobot = false;
	for (uint8 i = 0; i < GAME_PLAYER; i++)
	{
		CGamePlayer *pPlayer = GetPlayer(i);
		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_cbPlayStatus[i] == FALSE)
		{
			continue;
		}
		if (pPlayer->IsRobot())
		{
			bIsHaveRobot = true;
		}
	}
	// 没有机器人就是失败 不用设置
	if (!bIsHaveRobot)
	{
		return false;
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
			SetControlPalyerLost(pPlayer->GetUID());

			bIsHaveWinExceedMaxScore = true;
		}
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,lPlayerTotalWinScore:%lld,lPlayerMaxWinScore:%lld,gametype:%d", roomid, GetTableID(), pPlayer->GetUID(), lPlayerTotalWinScore, lPlayerMaxWinScore, gametype);
	}

	return bIsHaveWinExceedMaxScore;
}


// 写入出牌log
void    CGameShowHandTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
    Json::Value logValue;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
	uint8 cardType = m_gameLogic.GetCardGenre(cardData, cardCount);
	logValue["p"] = chairID;
	logValue["cardtype"] = cardType;

    m_operLog["card"].append(logValue);
}

void    CGameShowHandTable::WriteAddCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount,uint8 giveup)
{
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["giveup"] = giveup;
	for (uint32 i = 0; i<cardCount; ++i)
	{
		logValue["c"].append(cardData[i]);
	}
	uint8 cardType = m_gameLogic.GetCardGenre(cardData, cardCount);
	logValue["cardtype"] = cardType;

	m_operLog["addcard"].append(logValue);
}

void CGameShowHandTable::WriteJackpotScoreInfo()
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

// 写入加注log
void    CGameShowHandTable::WriteAddScoreLog(uint16 chairID,int64 score)
{
    if(m_cbSendCardCount < 2)return;

    Json::Value logValue;
    logValue["p"] = chairID;
    logValue["s"] = score;
    if(m_operLog["op"].size() < (uint16)(m_cbSendCardCount-1)){
        m_operLog["op"].resize(m_cbSendCardCount-1);
    }
    m_operLog["op"][m_cbSendCardCount-2].append(logValue);
}
//超时操作
void CGameShowHandTable::OnTimeOutOper()
{
	LOG_DEBUG("timeout_giveup - m_wCurrentUser:%d,uid:%d,uids:%d %d",
		m_wCurrentUser, GetPlayerID(m_wCurrentUser), GetPlayerID(0), GetPlayerID(1));

    CGamePlayer* pPlayer = m_vecPlayers[m_wCurrentUser].pPlayer;
    if(pPlayer != NULL && pPlayer->IsRobot())
	{
        //OnRobotOper(m_wCurrentUser);
		OnUserGiveUp(m_wCurrentUser);
    }
	else
	{
        OnUserGiveUp(m_wCurrentUser);
    }
}
//用户放弃
bool CGameShowHandTable::OnUserGiveUp(WORD wChairID)
{
	if (wChairID >= GAME_PLAYER)
	{
		LOG_DEBUG("up_start_error - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_wCurrentUser:%d,uids:%d %d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_wCurrentUser, GetPlayerID(0), GetPlayerID(1));

		return false;
	}



    //设置变量
	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

    m_cbPlayStatus[wChairID] = 0;
	WriteAddCardLog(wChairID, cbUserCardData[wChairID], m_cbCardCount[wChairID], 1);

    // 获取玩家

    // 扣除分数
    int64 lDeficiencyPoint = -(m_lUserScore[wChairID] + m_lTableScore[wChairID]);



    //写入积分
    //ChangeScoreValue(wChairID,lDeficiencyPoint,emACCTRAN_OPER_TYPE_GAME,m_pHostRoom->GetGameType());//写入积分到数据库

    //人数计算
    WORD wPlayerCount=0;
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        if(m_cbPlayStatus[i]==TRUE) wPlayerCount++;
    }

	LOG_DEBUG("up_starting - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_lUserScore:%lld, m_lTableScore:%lld,m_wCurrentUser:%d,wPlayerCount:%d,lDeficiencyPoint:%lld,uids:%d %d",
		GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_lUserScore[wChairID], m_lTableScore[wChairID], m_wCurrentUser, wPlayerCount, lDeficiencyPoint, GetPlayerID(0), GetPlayerID(1));


    //继续判断
    if(wPlayerCount >= 2)
    {
        //下注调整
        RectifyMaxScore();

        //切换用户
        if(m_wCurrentUser==wChairID)
        {
            //设置用户
            m_wCurrentUser=INVALID_CHAIR;

            //用户搜索
            for (WORD i=1;i<GAME_PLAYER;i++)
            {
                //变量定义
                WORD wCurrentUser=(wChairID+i)%GAME_PLAYER;
                int64 lDrawAddScroe=m_lUserScore[wCurrentUser]+m_lTableScore[wCurrentUser];

                //状态判断
                if(m_cbPlayStatus[wCurrentUser] == 0)
                    continue;

                //用户切换
                if((m_cbOperaScore[wCurrentUser] == 0)||(lDrawAddScroe < m_lTurnLessScore))
                {
                    m_wCurrentUser=wCurrentUser;
                    break;
                }
            }
        }
    }
    else
    {
        //汇集金币
        for (WORD i=0;i<GAME_PLAYER;i++)
        {
            m_lTableScore[i] += m_lUserScore[i];
            m_lUserScore[i] = 0L;
        }

        //设置用户
        m_wCurrentUser=INVALID_CHAIR;
    }

	LOG_DEBUG("up_runing - roomid:%d,tableid:%d,uid:%d,wChairID:%d,m_lUserScore:%lld, m_lTableScore:%lld,m_wCurrentUser:%d,wPlayerCount:%d,lDeficiencyPoint:%lld,uids:%d %d",
		GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, m_lUserScore[wChairID], m_lTableScore[wChairID], m_wCurrentUser, wPlayerCount, lDeficiencyPoint, GetPlayerID(0), GetPlayerID(1));

    //变量定义
    net::msg_showhand_giveup_rep msg;
    //设置变量
    msg.set_giveup_user(wChairID);
    msg.set_cur_user(m_wCurrentUser);
    msg.set_draw_max_score(m_lDrawMaxScore);
    msg.set_turn_max_score(m_lTurnMaxScore);
    SendMsgToAll(&msg,net::S2C_MSG_SHOWHAND_GIVE_UP);
    //结束游戏
    if(wPlayerCount <= 1)
    {
        OnGameEnd(INVALID_CHAIR,GER_NO_PLAYER);
        return true;
    }

    //发送扑克
    if( m_wCurrentUser == INVALID_CHAIR) {
        if(DispatchUserCard())
            return true;
    }
    m_coolLogic.beginCooling(s_AddScoreTime);
    SetRobotThinkTime();
    return true;
}
//用户加注
bool CGameShowHandTable::OnUserAddScore(WORD wChairID, int64 lScore)
{
	if (wChairID >= GAME_PLAYER)
	{
		LOG_DEBUG("add_start_error - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_wCurrentUser:%d,uids:%d %d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_wCurrentUser, GetPlayerID(0), GetPlayerID(1));

		return false;
	}

	int64 lCurTurnReadyScore = lScore + m_lUserScore[wChairID] + m_lTableScore[wChairID];

	LOG_DEBUG("add_starting - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_lUserScore:%lld, m_lTableScore:%lld,m_wCurrentUser:%d,lCurTurnReadyScore:%lld,uids:%d %d",
		GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], m_wCurrentUser, lCurTurnReadyScore, GetPlayerID(0), GetPlayerID(1));

    //状态效验
	if (m_wCurrentUser != wChairID)
	{
		LOG_DEBUG("add_error_cur_user - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_wCurrentUser:%d",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_wCurrentUser);

		return false;
	}
    if((lScore<0L)||(lCurTurnReadyScore < m_lTurnLessScore))
	{
		// 小于本轮最小分数
		LOG_DEBUG("add_error_less_score - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_lUserScore:%lld, m_lTableScore:%lld, lCurTurnReadyScore:%lld,m_lTurnLessScore:%lld",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], lCurTurnReadyScore,m_lTurnLessScore);

        return false;
    }
    //加注效验
    if(lCurTurnReadyScore > m_lTurnMaxScore)
	{
		// 大于本轮最大分数
		LOG_DEBUG("add_error_max_score - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_lUserScore:%lld, m_lTableScore:%lld, lCurTurnReadyScore:%lld,m_lTurnMaxScore:%lld",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], lCurTurnReadyScore, m_lTurnMaxScore);

        return false;
    }
    if(lCurTurnReadyScore > m_lUserMaxScore[wChairID])
	{
		// 大于自己的最大下注分数
		LOG_DEBUG("add_error_over_max_score - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_lUserScore:%lld, m_lTableScore:%lld, lCurTurnReadyScore:%lld,m_lUserMaxScore:%lld",
			GetRoomID(), GetTableID(), GetPlayerID(wChairID), wChairID, lScore, m_lUserScore[wChairID], m_lTableScore[wChairID], lCurTurnReadyScore, m_lUserMaxScore[wChairID]);

        return false;
    }

    //设置变量
    m_cbOperaScore[wChairID] = 1;
    m_lUserScore[wChairID]  += lScore;
    m_lTurnLessScore = m_lUserScore[wChairID] + m_lTableScore[wChairID];

    //状态变量
    m_wCurrentUser = INVALID_CHAIR;
    m_bShowHand = (m_lTurnLessScore == m_lDrawMaxScore);

    //用户搜索
    for(WORD i=1;i<GAME_PLAYER;i++)
    {
        //变量定义
        WORD  wCurrentUser  = (wChairID+i)%GAME_PLAYER;
        int64 lDrawAddScroe = m_lUserScore[wCurrentUser] + m_lTableScore[wCurrentUser];

        //状态判断
        if(m_cbPlayStatus[wCurrentUser] == 0)
            continue;

        //用户切换
        if((m_cbOperaScore[wCurrentUser] == 0) || (lDrawAddScroe < m_lTurnLessScore))
        {
            m_wCurrentUser = wCurrentUser;
            break;
        }
    }

    //变量定义
    net::msg_showhand_addscore_rep msg;
    //设置变量
    msg.set_add_score_user(wChairID);
    msg.set_cur_user(m_wCurrentUser);
    msg.set_turn_less_score(m_lTurnLessScore);
    msg.set_user_score_count(lScore);
    msg.set_show_hand(m_bShowHand ? 1 : 0);
    SendMsgToAll(&msg,net::S2C_MSG_SHOWHAND_ADDSCORE);

	CGamePlayer * pGamePlayer = GetPlayer(wChairID);
	uint32 uid = 0;
	if (pGamePlayer != NULL) {
		uid = pGamePlayer->GetUID();
	}
	LOG_DEBUG("dispatch user card - roomid:%d,tableid:%d,uid:%d,wChairID:%d,lScore:%lld,m_wCurrentUser:%d,m_lTurnMaxScore:%lld,m_lTurnLessScore:%lld,m_lDrawMaxScore:%d,m_bShowHand:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), uid,wChairID, lScore, m_wCurrentUser, m_lTurnMaxScore, m_lTurnLessScore, m_lDrawMaxScore, m_bShowHand);


    //写入日志log
    WriteAddScoreLog(wChairID,lScore);
    //发送扑克
    if (m_wCurrentUser==INVALID_CHAIR){
        if(DispatchUserCard())
            return true;
    }
    m_coolLogic.beginCooling(s_AddScoreTime);
    SetRobotThinkTime();
    return true;
}
//机器人平跟或下注
bool CGameShowHandTable::OnRobotCallSmallScore(uint16 chairID)
{
    if(m_robotAIInfo.minScore == 0)
	{
        return OnUserAddScore(chairID, m_robotAIInfo.smallScore);
    }
	else
	{
        if(m_robotAIInfo.enemyShowhand && !m_robotAIInfo.isHandWin)
		{
			//玩家梭哈 如果机器人梭哈不赢则直接弃牌
			return OnUserGiveUp(chairID);
        }
        return OnUserAddScore(chairID, m_robotAIInfo.minScore);
    }
}
//机器人平跟大注
bool CGameShowHandTable::OnRobotCallManyScore(uint16 chairID)
{
    if(m_robotAIInfo.minScore == 0){
        return OnUserAddScore(chairID, m_robotAIInfo.manyScore);
    }else{
        return OnUserAddScore(chairID, m_robotAIInfo.smallScore);
    }
}
//机器人AllIn
bool CGameShowHandTable::OnRobotAllinScore(uint16 chairID)
{
    if(m_robotAIInfo.isEndWin){
        return OnUserAddScore(chairID,m_robotAIInfo.maxScore);
    }
    if(g_RandGen.RandRatio(80,PRO_DENO_100)){
        return OnUserAddScore(chairID, m_robotAIInfo.maxScore);
    }
    return OnUserAddScore(chairID,m_robotAIInfo.manyScore);
}
//机器人概率弃牌
bool CGameShowHandTable::OnRobotGaiLvGiveUp(uint16 chairID,uint32 pro)
{
	bool bIsRobotGiveUp = true;
	if (m_bIsControlPlayer)
	{
		if (m_robotAIInfo.isEndWin)
		{
			bIsRobotGiveUp = false;
		}
	}

    if(m_robotAIInfo.minScore == 0)
	{
        return OnUserAddScore(chairID,m_robotAIInfo.minScore);
    }
    if(g_RandGen.RandRatio(pro,PRO_DENO_100))
	{
        return OnUserAddScore(chairID,m_robotAIInfo.minScore);
    }
	if (bIsRobotGiveUp && m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre)
	{
		return OnUserGiveUp(chairID);
	}
	return OnUserAddScore(chairID, m_robotAIInfo.minScore);
}
//调整下注
void CGameShowHandTable::RectifyMaxScore()
{
    //设置变量
    m_lDrawMaxScore=0L;
    m_lTurnMaxScore=0L;

    //最大下注
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        //用户判断
        if (m_cbPlayStatus[i]==0)
            continue;

        //变量设置
        if((m_lDrawMaxScore == 0L) || (m_lUserMaxScore[i] < m_lDrawMaxScore)) {
            m_lDrawMaxScore = m_lUserMaxScore[i];
        }
    }

    //当前下注
    if(m_cbSendCardCount <= 2){
        m_lTurnMaxScore = m_lDrawMaxScore/2;
    }else if(m_cbSendCardCount == 3){
        m_lTurnMaxScore = m_lDrawMaxScore;
    }else{
        m_lTurnMaxScore = m_lDrawMaxScore;
    }
    return;
}

//发送扑克
bool CGameShowHandTable::DispatchUserCard()
{
    //汇集金币
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        m_lTableScore[i] += m_lUserScore[i];
        m_lUserScore[i] = 0L;

		LOG_DEBUG("start_user_card - roomid:%d,tableid:%d,i:%d,uid:%d,m_cbSendCardCount:%d,m_wCurrentUser:%d,lTableScore:%lld",
			GetRoomID(), GetTableID(), i,GetPlayerID(i), m_cbSendCardCount, m_wCurrentUser, m_lTableScore[i]);

    }

    //结束判断
    if(m_cbSendCardCount == MAX_COUNT)
    {
        OnGameEnd(INVALID_CHAIR,GER_NORMAL);
        return true;
    }
    //换牌
    /*if(m_cbSendCardCount == 3 && m_needChangeCard)//三张牌的时候检测是否换牌
    {
        if(m_bShowHand){
            for(uint16 i=0;i<GAME_PLAYER;++i)
            {
                CGamePlayer* pPlayer = GetPlayer(i);
                if(pPlayer == NULL)continue;
                if(pPlayer->IsRobot()){
                    ChangeLeftCard(i,35,3);
                }else{
                    ChangeLeftCard(i,50,5);
                }                
            }            
        }else{
            for(uint16 i=0;i<GAME_PLAYER;++i)
            {
                CGamePlayer* pPlayer = GetPlayer(i);
                if(pPlayer == NULL)continue;
                if(pPlayer->IsRobot()){
                    ChangeLeftCard(i,20,3);
                }else{
                    ChangeLeftCard(i,20,5);
                }                
            }       
        }        
    }*/

    //派发扑克
    BYTE cbSourceCount=m_cbSendCardCount;
    m_cbSendCardCount=(m_bShowHand==false)?(m_cbSendCardCount+1):MAX_COUNT;

    //当前用户
    if(m_bShowHand==false)
    {
        //状态变量
        memset(m_cbOperaScore,0,sizeof(m_cbOperaScore));

        //设置用户
        m_wCurrentUser = EstimateWinner(1,m_cbSendCardCount-1);

        //下注设置(第三张开始梭哈)
        m_lTurnMaxScore  = (m_cbSendCardCount>=3) ? m_lDrawMaxScore : m_lDrawMaxScore/2;
        m_lTurnLessScore = m_lUserScore[m_wCurrentUser]+m_lTableScore[m_wCurrentUser];
    }else{
        //设置变量
        m_wCurrentUser   = INVALID_CHAIR;
        m_lTurnMaxScore  = m_lDrawMaxScore;
        m_lTurnLessScore = m_lDrawMaxScore;
    }
    //构造数据
    net::msg_showhand_sendcard_rep msg;
    //设置变量
    msg.set_cur_user(m_wCurrentUser);
    msg.set_turn_max_score(m_lTurnMaxScore);
    msg.set_start_chairid(EstimateWinner(1,cbSourceCount-1));

	BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
	memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

    //发送扑克
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        net::msg_cards* pCards = msg.add_card_data();
        //状态判断
        if(m_cbPlayStatus[i]==FALSE)
            continue;

        //设置数目
        m_cbCardCount[i] = m_cbSendCardCount;

        //派发扑克
        for(BYTE j=0;j<(m_cbSendCardCount-cbSourceCount);j++)
        {
            pCards->add_cards(m_cbHandCardData[i][cbSourceCount+j]);
        }

		CGamePlayer * pGamePlayer = GetPlayer(i);
		uint32 uid = 0;
		if (pGamePlayer != NULL)
		{
			uid = pGamePlayer->GetUID();
		}

		WriteAddCardLog(i, cbUserCardData[i], m_cbCardCount[i],0);

		LOG_DEBUG("dispatch_user_card - roomid:%d,tableid:%d,uid:%d,cbSourceCount:%d,m_cbSendCardCount:%d,m_wCurrentUser:%d,m_lTurnMaxScore:%lld,m_lTurnLessScore:%lld,m_bShowHand:%d",
			GetRoomID(), GetTableID(),uid, cbSourceCount, m_cbSendCardCount, m_wCurrentUser, m_lTurnMaxScore, m_lTurnLessScore, m_bShowHand);

    }
    SendMsgToAll(&msg,net::S2C_MSG_SHOWHAND_SEND_CARD);



    //结束处理
    if(m_wCurrentUser == INVALID_CHAIR){
        OnGameEnd(INVALID_CHAIR,GER_NORMAL);
        return true;
    }
    return false;
}

//推断输者
WORD CGameShowHandTable::EstimateLoser(BYTE cbStartPos, BYTE cbConcludePos)
{
    //保存扑克
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

    //寻找玩家
    WORD wLoser=0;
    for(;wLoser<GAME_PLAYER;wLoser++)
    {
        if(m_cbPlayStatus[wLoser] == 1)
        {
            m_gameLogic.SortCardList(cbUserCardData[wLoser]+cbStartPos,cbConcludePos-cbStartPos+1);
            break;
        }
    }

    //对比玩家
    for(WORD i=(wLoser+1);i<GAME_PLAYER;i++)
    {
        //用户过滤
        if(m_cbPlayStatus[i] == 0)
            continue;

        //排列扑克
        m_gameLogic.SortCardList(cbUserCardData[i]+cbStartPos,cbConcludePos-cbStartPos+1);

        //对比扑克
        if(m_gameLogic.CompareCard(cbUserCardData[i]+cbStartPos,cbUserCardData[wLoser]+cbStartPos,cbConcludePos-cbStartPos+1)==false)
        {
            wLoser=i;
        }
    }

    return wLoser;
}

//推断胜者
WORD CGameShowHandTable::EstimateWinner(BYTE cbStartPos, BYTE cbConcludePos)
{
    //保存扑克
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

    //寻找玩家
    WORD wWinner=0;
    for(;wWinner<GAME_PLAYER;wWinner++)
    {
        if (m_cbPlayStatus[wWinner] == 1)
        {
            m_gameLogic.SortCardList(cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1);
            break;
        }

		CGamePlayer * pGamePlayer = GetPlayer(wWinner);
		uint32 uid = 0;
		if (pGamePlayer != NULL) {
			uid = pGamePlayer->GetUID();
		}

		//LOG_DEBUG("Estimate Winner find - roomid:%d,tableid:%d,cbStartPos:%d,cbConcludePos:%d,wWinner:%d,uid:%d,cbUserCardData[0][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X],cbUserCardData[1][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X]",
		//	m_pHostRoom->GetRoomID(), GetTableID(), cbStartPos, cbConcludePos, wWinner, uid, cbUserCardData[0][0], cbUserCardData[0][1], cbUserCardData[0][2], cbUserCardData[0][3], cbUserCardData[0][4], cbUserCardData[1][0], cbUserCardData[1][1], cbUserCardData[1][2], cbUserCardData[1][3], cbUserCardData[1][4]);

    }


    //对比玩家
    for(WORD i=(wWinner+1);i<GAME_PLAYER;i++)
    {
        //用户过滤
        if(m_cbPlayStatus[i] == 0)
            continue;

        //排列扑克
        m_gameLogic.SortCardList(cbUserCardData[i]+cbStartPos,cbConcludePos-cbStartPos+1);

        //对比扑克
        if(m_gameLogic.CompareCard(cbUserCardData[i]+cbStartPos,cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1)==true)
        {
            wWinner=i;
        }
		CGamePlayer * pGamePlayer = GetPlayer(wWinner);
		uint32 uid = 0;
		if (pGamePlayer != NULL) {
			uid = pGamePlayer->GetUID();
		}
		//LOG_DEBUG("Estimate Winner cmp - roomid:%d,tableid:%d,i:%d,cbStartPos:%d,cbConcludePos:%d,wWinner:%d,uid:%d,cbUserCardData[0][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X],cbUserCardData[1][0x%02X,0x%02X,0x%02X,0x%02X,0x%02X]",
		//	m_pHostRoom->GetRoomID(), GetTableID(),i, cbStartPos, cbConcludePos, wWinner, uid, cbUserCardData[0][0], cbUserCardData[0][1], cbUserCardData[0][2], cbUserCardData[0][3], cbUserCardData[0][4], cbUserCardData[1][0], cbUserCardData[1][1], cbUserCardData[1][2], cbUserCardData[1][3], cbUserCardData[1][4]);
    }

    return wWinner;
}

//推断胜者
WORD CGameShowHandTable::EstimateWinnerEx(BYTE cbStartPos, BYTE cbConcludePos)
{
    //保存扑克
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

    //寻找玩家
    WORD wWinner=0;
    for(;wWinner<GAME_PLAYER;wWinner++)
    {
        if(m_cbPlayStatus[wWinner] == 1)
        {
            //排列扑克
            m_gameLogic.SortCardList(cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1);
            break;
        }
    }

    //对比玩家
    WORD wId = wWinner;
    for(WORD i = 0;i < GAME_PLAYER-1;i++)
    {
        wId = (wId+1)%GAME_PLAYER;

        //用户过滤
        if(m_cbPlayStatus[wId] == 0)
            continue;

        //排列扑克
        m_gameLogic.SortCardList(cbUserCardData[wId]+cbStartPos,cbConcludePos-cbStartPos+1);

        //对比扑克
        if(m_gameLogic.CompareCard(cbUserCardData[wId]+cbStartPos,cbUserCardData[wWinner]+cbStartPos,cbConcludePos-cbStartPos+1)==true)
        {
            wWinner=wId;
        }
    }

    return wWinner;
}
//获得最终胜利者
WORD CGameShowHandTable::GetEndWiner()
{
    return EstimateWinner(0,MAX_COUNT-1);
}
WORD CGameShowHandTable::GetEnemyChairID(uint16 chairID)
{
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        //用户过滤
        if(m_cbPlayStatus[i] == 0 || i == chairID)
            continue;

        return i;
    }
    return chairID;
}
//判断手牌是否最大
bool CGameShowHandTable::IsHandWiner(uint16 chairID)
{
    //保存扑克
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
    //寻找玩家
    WORD wWinner=0;
    for(;wWinner<GAME_PLAYER;wWinner++)
    {
        if(m_cbPlayStatus[wWinner] == 1)
		{
			 
            m_gameLogic.SortCardList(cbUserCardData[wWinner],m_cbSendCardCount);
			//m_gameLogic.SortCardList(cbUserCardData[wWinner], MAX_COUNT);
            break;
        }
    }
	
    //对比玩家
    for(WORD i=(wWinner+1);i<GAME_PLAYER;i++)
    {
        //用户过滤
		if (m_cbPlayStatus[i] == 0)
		{
			continue;
		}

        //排列扑克
        m_gameLogic.SortCardList(cbUserCardData[i],m_cbSendCardCount);
		//m_gameLogic.SortCardList(cbUserCardData[i], MAX_COUNT);
        //对比扑克
        if(m_gameLogic.CompareCard(cbUserCardData[i],cbUserCardData[wWinner],m_cbSendCardCount)==true)
        {
			//if (m_gameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wWinner], MAX_COUNT) == true)

            wWinner=i;
        }
    }

    return (wWinner == chairID);
}
//获得其它人的最大牌面
BYTE CGameShowHandTable::GetOtherCardGenre(uint16 chairID)
{
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
    uint8 genre = 0;
    for(uint16 i=0;i<GAME_PLAYER;++i)
	{
		if (i == chairID)
		{
			continue;
		}
        m_gameLogic.SortCardList(cbUserCardData[i],m_cbCardCount[i]);
        uint8 tmp = m_gameLogic.GetCardGenre(cbUserCardData[i],m_cbCardCount[i]);
        genre = MAX(genre,tmp);
    }
    return genre;
}
BYTE CGameShowHandTable::GetOtherMaxDeskCardValue(uint16 chairID)
{
    uint8 cardValue = 0;
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(i == chairID)
            continue;
        for(uint8 j=1;j<m_cbCardCount[i];++j){
            uint8 tmp = m_gameLogic.GetCardValue(m_cbHandCardData[i][j]);
            cardValue = MAX(tmp,cardValue);
        }
    }
    return cardValue;
}
BYTE CGameShowHandTable::GetSelfCardGenre(uint16 chairID)
{
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

    m_gameLogic.SortCardList(cbUserCardData[chairID],m_cbCardCount[chairID]);
    return m_gameLogic.GetCardGenre(cbUserCardData[chairID],m_cbCardCount[chairID]);
}
BYTE CGameShowHandTable::GetSelfCardGenreValue(uint16 chairID)
{
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));

    m_gameLogic.SortCardList(cbUserCardData[chairID],m_cbCardCount[chairID]);
    return m_gameLogic.GetCardGenreValue(cbUserCardData[chairID],m_cbCardCount[chairID]);
}
bool CGameShowHandTable::IsOtherShowhand(uint16 chairID)
{
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(chairID == i)
            continue;
        if((m_lTableScore[i] + m_lUserScore[i]) >= m_lTurnMaxScore)
            return true;
    }
    return false;
}
//获得最终牌型
uint8 CGameShowHandTable::GetEndGenre(uint16 chairID)
{
    //保存扑克
    BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
    //寻找玩家
    m_gameLogic.SortCardList(cbUserCardData[chairID],m_cbCardCount[chairID]);
    return m_gameLogic.GetCardGenre(cbUserCardData[chairID],m_cbCardCount[chairID]);
}
bool CGameShowHandTable::IsCardA(uint8 cardValue)
{
    return cardValue == 14;
}
bool CGameShowHandTable::IsCardQ(uint8 cardValue)
{
    return cardValue >= 12;
}
void CGameShowHandTable::OnRobotOper(uint16 chairID)
{
 //   if(m_isOnlyRobot)
	//{
 //       OnUserAddScore(chairID,m_robotAIInfo.minScore);
 //       return;
 //   }
    //初始化机器人AI信息
    m_robotAIInfo.minScore = m_lTurnLessScore - m_lUserScore[chairID] - m_lTableScore[chairID]; // 最小下注
    m_robotAIInfo.maxScore = m_lTurnMaxScore - m_lUserScore[chairID] - m_lTableScore[chairID];  // 最大下注
    int64 minCell = m_lDrawCellScore;
    m_robotAIInfo.smallScore = MAX(minCell, m_robotAIInfo.minScore);
    int64 maxCell = m_lDrawCellScore * 4;
    m_robotAIInfo.manyScore = MIN(maxCell, m_robotAIInfo.maxScore);
    m_robotAIInfo.manyScore = MAX(m_robotAIInfo.manyScore, m_robotAIInfo.smallScore);
    m_robotAIInfo.enemyShowhand = IsOtherShowhand(chairID);

    //if(m_robotAIInfo.chairID != chairID || m_robotAIInfo.sendCard != m_cbSendCardCount)
	//{
    m_robotAIInfo.isDeskWin = (EstimateWinner(1, m_cbSendCardCount - 1) == chairID) ? true : false;
    m_robotAIInfo.isEndWin = (GetEndWiner() == chairID) ? true : false;
    m_robotAIInfo.isHandWin = IsHandWiner(chairID);
    m_robotAIInfo.enemyGenre = GetOtherCardGenre(chairID);
    m_robotAIInfo.enemyMaxDeskValue = GetOtherMaxDeskCardValue(chairID);
    m_robotAIInfo.selfGenre = GetSelfCardGenre(chairID);
    m_robotAIInfo.selfGenreValue = GetSelfCardGenreValue(chairID);
    m_robotAIInfo.selfEndGenre = GetEndGenre(chairID);

    m_robotAIInfo.chairID = chairID;
    m_robotAIInfo.sendCard = m_cbSendCardCount;
    if(m_cbSendCardCount >= 3 && m_cbSendCardCount <= 4)
	{
        uint16 enemyChairID = GetEnemyChairID(chairID);
        m_robotAIInfo.winPro = m_gameLogic.GetWinPro(m_cbHandCardData[chairID],m_cbSendCardCount,m_cbHandCardData[enemyChairID],m_cbSendCardCount,m_makeType5,m_makeType3);
    }
    if(m_cbSendCardCount == 5)
	{
        uint16 enemyChairID = GetEnemyChairID(chairID);
        m_robotAIInfo.winPro = m_gameLogic.GetWinPro(m_cbHandCardData[chairID],m_cbSendCardCount,m_cbHandCardData[enemyChairID]+1, m_cbSendCardCount-1,m_makeType5,m_makeType3);
    }
    //}

	LOG_DEBUG("roomid:%d,tableid:%d,chessid:%s,uid:%d,minScore:%d,maxScore:%d,smallScore:%d,manyScore:%d,enemyShowhand:%d,isDeskWin:%d,isHandWin:%d,selfGenre:%d,selfGenreValue:%d,enemyGenre:%d,enemyMaxDeskValue:%d,selfEndGenre:%d,isEndWin:%d,m_robotFrontFourWinPro:%d,m_robotFrontFourLostPro:%d,sendCard:%d,chairID:%d,winPro:%d",
		GetRoomID(),GetTableID(),GetChessID().c_str(),GetPlayerID(chairID),m_robotAIInfo.minScore, m_robotAIInfo.maxScore, m_robotAIInfo.smallScore, m_robotAIInfo.manyScore, m_robotAIInfo.enemyShowhand, m_robotAIInfo.isDeskWin, m_robotAIInfo.isHandWin, m_robotAIInfo.selfGenre, m_robotAIInfo.selfGenreValue, m_robotAIInfo.enemyGenre, m_robotAIInfo.enemyMaxDeskValue, m_robotAIInfo.selfEndGenre, m_robotAIInfo.isEndWin, m_robotFrontFourWinPro, m_robotFrontFourLostPro, m_robotAIInfo.sendCard, m_robotAIInfo.chairID, m_robotAIInfo.winPro);

	if (m_robotAIInfo.enemyShowhand)
	{
		if (m_cbSendCardCount == 5)
		{
			if (m_robotAIInfo.isEndWin)
			{
				OnUserAddScore(chairID, m_robotAIInfo.minScore);
				return;
			}
			else
			{
				OnUserGiveUp(chairID);
				return;
			}
		}
		else
		{
			if (m_robotAIInfo.isEndWin)
			{
				if (g_RandGen.RandRatio(m_robotFrontFourWinPro, PRO_DENO_10000))
				{
					OnUserAddScore(chairID, m_robotAIInfo.minScore);
					return;
				}
			}
			else
			{
				if (g_RandGen.RandRatio(m_robotFrontFourLostPro, PRO_DENO_10000))
				{
					OnUserAddScore(chairID, m_robotAIInfo.minScore);
					return;
				}
			}
		}
		OnUserGiveUp(chairID);
		return;
	}


    if(m_cbSendCardCount == 2){
        OnRobotOper2(chairID);
        return;
    }
    if(m_cbSendCardCount == 3){
        OnRobotOperEx(chairID);
        return;
    }
    if(m_cbSendCardCount == 4){
        OnRobotOperEx(chairID);
        return;
    }
    if(m_cbSendCardCount == 5){
        OnRobotOperEx5(chairID);
        return;
    }
}
//2张牌策略
bool CGameShowHandTable::OnRobotOper2(uint16 chairID)
{
    if(m_robotAIInfo.isDeskWin)// 自己领先下注或者跟注
    {
        if(m_robotAIInfo.selfGenre == CT_ONE_DOUBLE || IsCardA(m_robotAIInfo.selfGenreValue)){
            return OnRobotCallManyScore(chairID);
        }
        if(m_robotAIInfo.isHandWin && g_RandGen.RandRatio(50,PRO_DENO_100)){
            return OnRobotCallManyScore(chairID);
        }
        return OnRobotCallSmallScore(chairID);
    }
    else
    {
        if(m_robotAIInfo.selfGenre >= CT_ONE_DOUBLE)// 成对子跟注或下注
        {
            if(m_robotAIInfo.isHandWin && g_RandGen.RandRatio(20,PRO_DENO_100)){
                return OnRobotCallManyScore(chairID);
            }
            return OnRobotCallSmallScore(chairID);
        }
        if(!IsCardQ(m_robotAIInfo.selfGenreValue) && m_robotAIInfo.minScore >= m_robotAIInfo.smallScore)
		{
            return OnRobotGaiLvGiveUp(chairID,10);
        }
        if(m_robotAIInfo.isHandWin){
            return OnUserAddScore(chairID,m_robotAIInfo.minScore);
        }
        // 其它判断
        return OnRobotGaiLvGiveUp(chairID,50);
    }
    return OnRobotGaiLvGiveUp(chairID,50);
}
bool CGameShowHandTable::OnRobotOperEx(uint16 chairID)
{
	bool bIsRobotGiveUp = true;
	if (m_bIsControlPlayer)
	{
		if (m_robotAIInfo.isEndWin)
		{
			bIsRobotGiveUp = false;
		}
	}

	if (m_robotAIInfo.enemyShowhand)
	{
		if (m_robotAIInfo.isEndWin)
		{
			if (g_RandGen.RandRatio(80, PRO_DENO_100))
			{
				return OnUserAddScore(chairID, m_robotAIInfo.minScore);
			}
		}
		else
		{
			if (g_RandGen.RandRatio(5, PRO_DENO_100))
			{
				return OnUserAddScore(chairID, m_robotAIInfo.minScore);
			}
		}
		return OnUserGiveUp(chairID);
	}

    uint32 pro = m_robotAIInfo.winPro;//机器人胜率
    if(pro < 50)//50
    {
        if(m_robotAIInfo.minScore == 0)
		{// 对方check跟注
            return OnUserAddScore(chairID,m_robotAIInfo.minScore);
        }
        if(m_robotAIInfo.enemyShowhand)
		{//敌人梭哈概率大于40%且自己赢，50%抓鸡，否则弃牌
            if(pro > 40 && m_robotAIInfo.isEndWin && g_RandGen.RandRatio(55,PRO_DENO_100))
			{
                return OnUserAddScore(chairID,m_robotAIInfo.minScore);
            }

			if (g_RandGen.RandRatio(90, PRO_DENO_100))
			{
				if (bIsRobotGiveUp && m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre)
				{
					return OnUserGiveUp(chairID);
				}
			}
			else
			{
				return OnUserAddScore(chairID, m_robotAIInfo.minScore);
			}
        }
        if(m_robotAIInfo.isDeskWin && !m_robotAIInfo.enemyShowhand)
		{//桌面大且对面没梭哈
            return OnUserAddScore(chairID,m_robotAIInfo.minScore);
        }        
        if(pro >= 35 && (!m_robotAIInfo.isEndWin || g_RandGen.RandRatio(50,100)))
		{//概率弃牌
            return OnRobotGaiLvGiveUp(chairID,pro);
        }
        if(pro < 35)
		{//小于30概率加注就弃牌
			if (bIsRobotGiveUp && m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre)
			{
				return OnUserGiveUp(chairID);
			}
        }//平跟
        return OnUserAddScore(chairID,m_robotAIInfo.minScore);
    }
    if(pro >= 45 && pro <= 65){//概率加大小注 45   65
        if(g_RandGen.RandRatio(pro,PRO_DENO_100))
		{
            return OnRobotCallManyScore(chairID);
        }
        if(m_robotAIInfo.isEndWin && g_RandGen.RandRatio((pro-20),PRO_DENO_100))
		{
            return OnRobotAllinScore(chairID);
        }
        return OnRobotCallSmallScore(chairID);
    }
    if(pro > 65 && pro <= 75)
	{// 概率梭哈
        if(m_robotAIInfo.isEndWin && g_RandGen.RandRatio(pro,100))
		{
            return OnRobotAllinScore(chairID);
        }
        return OnRobotCallManyScore(chairID);
    }
    if(m_robotAIInfo.isEndWin && pro > 75)
	{// 梭哈 50
        return OnRobotAllinScore(chairID);
    }
    return OnUserAddScore(chairID,m_robotAIInfo.minScore);
}
bool CGameShowHandTable::OnRobotOperEx5(uint16 chairID)
{
	if (m_robotAIInfo.enemyShowhand)
	{
		if (m_robotAIInfo.isEndWin)
		{
			//if (g_RandGen.RandRatio(80, PRO_DENO_100))
			//{
			//	return OnUserAddScore(chairID, m_robotAIInfo.minScore);
			//}
			return OnUserAddScore(chairID, m_robotAIInfo.minScore);
		}
		else
		{
			//if (g_RandGen.RandRatio(5, PRO_DENO_100))
			//{
			//	return OnUserAddScore(chairID, m_robotAIInfo.minScore);
			//}
			return OnUserGiveUp(chairID);
		}
		return OnUserGiveUp(chairID);
	}
    if(m_robotAIInfo.enemyShowhand)//对手梭哈
    {
        if(!m_robotAIInfo.isEndWin){
            return OnUserGiveUp(chairID);
        }
        if(m_robotAIInfo.selfGenre < CT_ONE_DOUBLE && g_RandGen.RandRatio(90,PRO_DENO_100))  // 90
		{
			if (m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre && !m_robotAIInfo.isEndWin)
			{
				return OnUserGiveUp(chairID);
			}
		}
        if(m_robotAIInfo.winPro > 70 && m_robotAIInfo.isDeskWin && m_robotAIInfo.minScore < m_lTableScore[chairID]) // 70
		{
            return OnUserAddScore(chairID,m_robotAIInfo.minScore);
        }
        if(m_robotAIInfo.winPro < 30)
		{
			if (m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre && !m_robotAIInfo.isEndWin)
			{
				return OnUserGiveUp(chairID);
			}
		}
        if(m_robotAIInfo.isDeskWin && m_robotAIInfo.minScore < m_lTableScore[chairID])
		{
            if(g_RandGen.RandRatio(m_robotAIInfo.winPro,PRO_DENO_100))
			{
                return OnUserAddScore(chairID,m_robotAIInfo.minScore);
            }
			if (m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre && !m_robotAIInfo.isEndWin)
			{
				return OnUserGiveUp(chairID);
			}
		}
		if (m_robotAIInfo.selfGenre <= m_robotAIInfo.enemyGenre)
		{
			if (m_robotAIInfo.winPro < 30)
			{
				return OnUserGiveUp(chairID);
			}
		}
		if (m_robotAIInfo.isEndWin)
		{
			return OnUserAddScore(chairID, m_robotAIInfo.minScore);
		}
    }
    else
    {        
        if(m_robotAIInfo.winPro > 70)
		{
            if(m_robotAIInfo.isEndWin && g_RandGen.RandRatio(70,PRO_DENO_100))
			{
                return OnUserAddScore(chairID,m_robotAIInfo.maxScore);
            }
            return OnRobotCallManyScore(chairID);
        }         
        if(m_robotAIInfo.winPro < 50)
		{
            if(!m_robotAIInfo.isEndWin && m_robotAIInfo.minScore > 0)
			{
                return OnUserGiveUp(chairID);
            }
            if(m_robotAIInfo.winPro == 0)
			{
                return OnUserGiveUp(chairID);
            }
            return OnUserAddScore(chairID,m_robotAIInfo.minScore);
        }
        if(!m_robotAIInfo.isEndWin)
		{
            return OnRobotGaiLvGiveUp(chairID,m_robotAIInfo.winPro);
        }
		else
		{
            return OnRobotCallManyScore(chairID);
        }
    } 
    return OnUserGiveUp(chairID);
}
void CGameShowHandTable::CheckAddRobot()
{
	if (m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
	{
		return;
	}

    if(GetChairPlayerNum() == 1)
    {
        for(uint32 i=0;i<m_vecPlayers.size();++i)
		{
            CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
            if(pPlayer != NULL && !pPlayer->IsRobot())// 只有一个玩家
            {
                CRobotMgr::Instance().RequestOneRobot(this, pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(2000,3000));
                return;
            }
        }
    }
    SetRobotThinkTime();
    for(uint32 i=0;i<m_vecPlayers.size();++i)
	{
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_vecPlayers[i].readyState == 0)
			{
                if(m_isOnlyRobot && m_lHistoryScore[i] > GetBaseScore()*50)
				{
                    LeaveTable(pPlayer);
                    LOG_DEBUG("win and leave:%d",pPlayer->GetUID());
                    break;
                }
                PlayerReady(pPlayer);
                break;
            }
			else
			{
                if((getSysTime() - m_vecPlayers[i].readyTime) > 150)
				{
                    LeaveTable(pPlayer);
                    LOG_DEBUG("time out ready and leave:%d",pPlayer->GetUID());
                }
            }
        }
    }
}
void CGameShowHandTable::SetRobotThinkTime()
{
    if(m_isOnlyRobot)
	{
        //m_coolRobot.beginCooling(15000);
		m_coolRobot.beginCooling(g_RandGen.RandRange(1000, 3000));
        return;
    }
    if(m_bShowHand && GetGameState() == TABLE_STATE_PLAY)
	{
        m_coolRobot.beginCooling(g_RandGen.RandRange(3000, 5000));
        return;
    }
    if(g_RandGen.RandRatio(70,100))
	{
        m_coolRobot.beginCooling(g_RandGen.RandRange(1000, 3000));
    }
	else
	{
        m_coolRobot.beginCooling(g_RandGen.RandRange(2000, 3000));
    }
}
//修改剩下的牌
void CGameShowHandTable::ChangeLeftCard(uint16 chairID,uint32 pro,uint16 cardNum)
{
    if(g_RandGen.RandRatio(pro,100) == false)
        return;
    LOG_DEBUG("换牌");     
    vector<uint8> tmp;    
    vector<uint8> randCards;
    m_gameLogic.GetLeftCard(m_cbHandCardData[0],sizeof(m_cbHandCardData)/sizeof(m_cbHandCardData[0][0]),tmp);
    for(uint16 i=0;i<cardNum;++i)
    {
        uint16 pos = g_RandGen.RandUInt()%tmp.size();
        randCards.push_back(tmp[pos]);
        tmp.erase(tmp.begin()+pos);
    }    
    for(uint8 i=m_cbSendCardCount;i<MAX_COUNT;++i){//手牌放进去
        randCards.push_back(m_cbHandCardData[chairID][i]);        
    }
    //随机洗牌
    for(uint16 i=0;i<10;++i)
    {
        uint16 pos1 = g_RandGen.RandUInt()%randCards.size();
        uint16 pos2 = g_RandGen.RandUInt()%randCards.size();
        uint8 tmp = randCards[pos1];
        randCards[pos1] = randCards[pos2];
        randCards[pos2] = tmp;        
    }    
    for(uint8 i=m_cbSendCardCount,j=0;i<MAX_COUNT;++i,++j){
        m_cbHandCardData[chairID][i] = randCards[j];        
    }    
}

bool CGameShowHandTable::ActiveWelfareCtrl()
{
    LOG_DEBUG("enter ActiveWelfareCtrl ctrl player count:%d.", m_aw_ctrl_player_list.size());

    //获限当前局参与游戏的玩家列表
    for (uint32 uIndex = 0; uIndex < GAME_PLAYER; uIndex++)
    {
        CGamePlayer *pPlayer = GetPlayer(uIndex);
        if (pPlayer == NULL)
        {
            continue;
        }
        if (IsReady(pPlayer) && pPlayer->IsRobot()==false)
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

bool CGameShowHandTable::NewRegisterWelfareCtrl()
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
			ret = SetNRWControlPlayerWin(control_uid);
		}
		if (control_status == 2)        //如果当前赢取大于最大赢取时,设置机器人必赢
		{
			bool ret = SetRobotWin();
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

bool    CGameShowHandTable::SetNRWControlPlayerWin(uint32 control_uid)
{
	LOG_DEBUG("enter function tid:%d control_uid:%d.", GetTableID(), control_uid);
	bool bIsFlag = true;

	//先确定当前控制玩家的牌型 [同花顺、铁枝、葫芦、同花、顺子] 根据配置的概率
	int iSumValue = 0;
	int iArrNRWCardPro[ShowHand_Pro_Index_MAX] = { 0 };
	for (int i = 0; i < ShowHand_Pro_Index_MAX; i++)
	{
		iArrNRWCardPro[i] = m_iArrNRWCardPro[i];
		iSumValue += m_iArrNRWCardPro[i];
	}
	if (iSumValue <= 0)
	{
		return false;
	}

	int iRandNum = g_RandGen.RandRange(0, iSumValue);
	LOG_DEBUG("tableid:%d iSumValue:%d iRandNum:%d", GetTableID(), iSumValue, iRandNum);
			
	int iProIndex = 0;
	for (; iProIndex < ShowHand_Pro_Index_MAX; iProIndex++)
	{
		//LOG_DEBUG("iProIndex:%d,iRandNum:%d rate:%d", iProIndex, iRandNum, iArrNRWCardPro[iProIndex]);
		if (iArrNRWCardPro[iProIndex] == 0)
		{
			continue;
		}
		if (iRandNum <= iArrNRWCardPro[iProIndex])
		{
			break;
		}
		else
		{
			iRandNum -= iArrNRWCardPro[iProIndex];
		}		
	}
	if (iProIndex >= ShowHand_Pro_Index_MAX)
	{
		iProIndex = ShowHand_Pro_Index_Single;
	}	
	LOG_DEBUG("get control player cardtype tid:%d control_uid:%d card_type:%d", GetTableID(), control_uid, iProIndex);

	//再确定其它人获取牌的类型
	int iArProCardType[GAME_PLAYER] = { 0 };
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		if (m_cbPlayStatus[i] == FALSE)
			continue;
		CGamePlayer* pTmp = GetPlayer(i);
		if (pTmp != NULL)
		{
			if (pTmp->GetUID() == control_uid)
			{
				iArProCardType[i] = iProIndex;
			}
			else
			{
				uint32 tmp = rand() % (ShowHand_Pro_Index_MAX - iProIndex);
				iArProCardType[i] = iProIndex + tmp + 1;
				if (iArProCardType[i] >= ShowHand_Pro_Index_MAX)
				{
					iArProCardType[i] = ShowHand_Pro_Index_Single;
				}
			}
		}
		else
		{
			continue;
		}
	}

	LOG_DEBUG("get all player cardtype. roomid:%d,tableid:%d,cbFirstCardType:%d,cbNextCardType:%d", GetRoomID(), GetTableID(), iArProCardType[0], iArProCardType[1]);

	// 根据每个人的牌型获取对应的牌型数据
	int iRandCount = 999;
	for (int iRandIndex = 0; iRandIndex < iRandCount; iRandIndex++)
	{		
		// 根据类型获取全部的手牌
		bIsFlag = m_gameLogic.GetCardTypeData(iArProCardType, m_cbHandCardData);

		BYTE cbUserCardData[GAME_PLAYER][MAX_COUNT];
		memcpy(cbUserCardData, m_cbHandCardData, sizeof(cbUserCardData));

		BYTE cbFirstGenre = m_gameLogic.GetCardGenre(cbUserCardData[0], MAX_COUNT);
		BYTE cbNextGenre = m_gameLogic.GetCardGenre(cbUserCardData[1], MAX_COUNT);

		LOG_DEBUG("xxxx cbFirstGenre:%d,cbNextGenre:%d iArProCardType[0]:%d iArProCardType[1]:%d", cbFirstGenre, cbNextGenre, ShowHand_Pro_Index_MAX - iArProCardType[0], ShowHand_Pro_Index_MAX - iArProCardType[1]);

		if (cbFirstGenre == (ShowHand_Pro_Index_MAX-iArProCardType[0]) && cbNextGenre == (ShowHand_Pro_Index_MAX-iArProCardType[1]))
		{			
			LOG_DEBUG("xxxx find all player cardtype is success. roomid:%d,tableid:%d,cbFirstGenre:%d,cbNextGenre:%d iRandIndex:%d", GetRoomID(), GetTableID(), cbFirstGenre, cbNextGenre, iRandIndex);
			break;
		}
	}
	// 根据类型获取全部的手牌
	for (uint16 i = 0; i < GAME_PLAYER; ++i)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,i:%d,iArProCardType:%d,m_cbPlayStatus:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X",
			m_pHostRoom->GetRoomID(), GetTableID(), GetPlayerID(i), i, iArProCardType[i], m_cbPlayStatus[i], m_cbHandCardData[i][0], m_cbHandCardData[i][1], m_cbHandCardData[i][2]);
	}
	return true;	//直接返回真
}

bool CGameShowHandTable::SetLuckyCtrl()
{
	uint32 win_uid = 0;
	set<uint32> set_lose_uid;
	set_lose_uid.clear();

	bool flag = GetTableLuckyFlag(win_uid, set_lose_uid);

	LOG_DEBUG("flag:%d win_uid:%d set_lose_uid size:%d.", flag, win_uid, set_lose_uid.size());

	if (!flag)
	{
		return false;
	}

	m_set_ctrl_lucky_uid.clear();
	m_lucky_flag = false;

	//如果设置了当前赢取玩家
	if (win_uid != 0)
	{
		bool ret = SetControlPalyerWin(win_uid);
		if (ret)
		{
			LOG_DEBUG("set current win player - uid:%d", win_uid);
			m_set_ctrl_lucky_uid = set_lose_uid;
			m_set_ctrl_lucky_uid.insert(win_uid);
			m_lucky_flag = true;
			return true;
		}
		else
		{
			LOG_DEBUG("set current win player fail- uid:%d", win_uid);
			return false;
		}
	}

	//如果设置了当前输的玩家
	uint32 ctrl_uid = 0;
	if (set_lose_uid.size() == 1)
	{
		for (uint32 uid : set_lose_uid)
		{
			ctrl_uid = uid;
		}
		bool ret = SetControlPalyerLost(ctrl_uid);
		if (ret)
		{
			LOG_DEBUG("set current lose player - uid:%d", ctrl_uid);
			m_set_ctrl_lucky_uid.insert(ctrl_uid);
			m_lucky_flag = true;
			return true;
		}
		else
		{
			LOG_DEBUG("set current lose player fail- uid:%d", ctrl_uid);
			return false;
		}
	}
	
	LOG_DEBUG("the no player match lucky tid:%d.", GetTableID());
	return false;
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameShowHandTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	DealIsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot, m_cbPlayStatus);
}

// 设置库存输赢  add by har
bool CGameShowHandTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;
	LOG_DEBUG("SetStockWinLose before roomid:%d,tableid:%d,m_cbHandCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
		GetRoomID(), GetTableID(), m_cbHandCardData[0][0], m_cbHandCardData[0][1], m_cbHandCardData[0][2], m_cbHandCardData[0][3], m_cbHandCardData[0][4], m_cbHandCardData[1][0], m_cbHandCardData[1][1], m_cbHandCardData[1][2], m_cbHandCardData[1][3], m_cbHandCardData[1][4]);
	bool bRet;
	if (stockChange == -1)
		bRet = SetRobotWinScore();
	else
		bRet = SetRobotLostScore();
	LOG_DEBUG("SetStockWinLose suc roomid:%d,tableid:%d,stockChange:%lld,bRet:%d",
		GetRoomID(), GetTableID(), stockChange, bRet);
	return bRet;
}

void   CGameShowHandTable::CheckPlayerScoreManyLeave()
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