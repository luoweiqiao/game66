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
#include "common_logic.h"

using namespace std;
using namespace svrlib;
using namespace game_sangong;
using namespace net;

namespace
{
    const static uint32 s_AddScoreTime       = 10*1000;
    const static uint32 s_FreeTime           = 6*1000;
    const static uint32 s_ShowCardTime       = 10*1000;

};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 三公
        {
            pTable = new CGameSanGongTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛三公
        {
            pTable = new CGameSanGongTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房三公
        {
            pTable = new CGameSanGongTable(this,tableID,emTABLE_TYPE_PLAYER);
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
CGameSanGongTable::CGameSanGongTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();
	m_wBankerUser   =   INVALID_CHAIR;
    m_isNeedBanker  =   false;

	//用户状态
	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
	//扑克变量
	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));

	//下注信息
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));


}
CGameSanGongTable::~CGameSanGongTable()
{

}
bool    CGameSanGongTable::CanEnterTable(CGamePlayer* pPlayer)
{
    if(pPlayer->GetTable() != NULL)
        return false;
    // 限额进入
    if(GetPlayerCurScore(pPlayer) < GetEnterMin() || GetChairPlayerNum() >= m_conf.seatNum){
        return false;
    }
    
    return true;
}
bool    CGameSanGongTable::CanLeaveTable(CGamePlayer* pPlayer)
{       
    if(GetGameState() != TABLE_STATE_FREE)
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
void    CGameSanGongTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::sangong_table_info* psangong = pInfo->mutable_sangong();
    psangong->set_tableid(GetTableID());
    psangong->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        psangong->set_is_passwd(1);
    }else{
        psangong->set_is_passwd(0);
    }
    psangong->set_hostname(m_conf.hostName);
    psangong->set_basescore(m_conf.baseScore);
    psangong->set_consume(m_conf.consume);
    psangong->set_entermin(m_conf.enterMin);
    psangong->set_duetime(m_conf.dueTime);
    psangong->set_feetype(m_conf.feeType);
    psangong->set_feevalue(m_conf.feeValue);
    psangong->set_card_time(s_AddScoreTime);
    psangong->set_table_state(GetGameState());
    psangong->set_seat_num(m_conf.seatNum);
    psangong->set_can_banker(m_isNeedBanker ? 1:0);
    psangong->set_apply_banker_time(s_AddScoreTime);
    psangong->set_show_card_time(s_ShowCardTime);

}

//配置桌子
bool    CGameSanGongTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);
    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    if(m_conf.deal == 3){
        m_isNeedBanker = false;
    }else{
        m_isNeedBanker = true;
    }
    return true;
}
void    CGameSanGongTable::ShutDown()
{

}
//复位桌子
void    CGameSanGongTable::ResetTable()
{
    ResetGameData();

    SetGameState(TABLE_STATE_FREE);
    ResetPlayerReady();
    SendSeatInfoToClient();
}
void    CGameSanGongTable::OnTimeTick()
{
    if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FREE:
            {
                if(IsCanStartGame()){
                    OnGameStart();
                }                
            }break;
        case TABLE_STATE_CALL:
            {
                InitBanker(true);
            }break;
        case TABLE_STATE_PLAY:
            {
                SendCardToClient();
            }break;
        case TABLE_STATE_WAIT:
            {
                OnGameEnd(INVALID_CHAIR,GER_NORMAL);
            }break;
        default:
            break;
        }
    }
    if(GetGameState() == TABLE_STATE_FREE){
        OnRobotReady();
        CheckAddRobot();
    }else if(GetGameState() == TABLE_STATE_PLAY){
        OnRobotOper();
    }else if(GetGameState() == TABLE_STATE_CALL){
        OnRobotApplyBanker();
    }else if(GetGameState() == TABLE_STATE_WAIT){
        OnRobotChangeCard();
    }
}
// 游戏消息
int     CGameSanGongTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("收到玩家消息:%d--%d",chairID,cmdID);
    switch(cmdID)
    {
    case net::C2S_MSG_SANGONG_APPLY_BANKER:// 申请庄家
        {
            if(GetGameState() != TABLE_STATE_CALL) {
                LOG_DEBUG("不是抢庄状态:%d",GetGameState());
                return 0;
            }
            net::msg_sangong_apply_banker msg;
            PARSE_MSG_FROM_ARRAY(msg);

            return OnUserApplyBanker(chairID,msg.score());
        }break;
    case net::C2S_MSG_SANGONG_PLACE_JETTON_REQ:// 加注
        {
            if(GetGameState() != TABLE_STATE_PLAY) {
                LOG_DEBUG("不是加注状态:%d",GetGameState());
                return 0;
            }
            net::msg_sangong_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);

            if(m_cbPlayStatus[chairID] == FALSE)
                return false;

            return OnUserAddScore(chairID,msg.jetton_score());
        }break;
    case net::C2S_MSG_SANGONG_CHANGE_CARD:// 摆牌
        {
            if(GetGameState() != TABLE_STATE_WAIT) {
                LOG_DEBUG("不是摆牌状态:%d",GetGameState());
                return 0;
            }
            net::msg_sangong_change_card msg;
            PARSE_MSG_FROM_ARRAY(msg);
            return OnUserChangeCard(chairID);
        }break;
    default:
        return 0;
    }
    return 0;
}
// 游戏开始
bool    CGameSanGongTable::OnGameStart()
{
    LOG_DEBUG("game start");
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		CGamePlayer *pPlayer = GetPlayer(i);
		if(pPlayer == NULL || !IsReady(pPlayer)) 
            continue;
		m_cbPlayStatus[i]   = TRUE;
	}
	//分发扑克
    DispatchCard();
    
	//构造数据
    net::msg_sangong_start_rep msg;
    msg.set_can_apply_banker(0);
    msg.set_banker_id(m_wBankerUser);
    if(m_isNeedBanker && m_wBankerUser == INVALID_CHAIR){
        msg.set_can_apply_banker(1);
    }
    SendMsgToAll(&msg,net::S2C_MSG_SANGONG_START);
    InitBlingLog(true);    
    //服务费
    DeductStartFee(true);
    //设置状态
    if(m_isNeedBanker && m_wBankerUser == INVALID_CHAIR){
        SetGameState(TABLE_STATE_CALL);
        m_coolLogic.beginCooling(s_AddScoreTime + (GetChairPlayerNum()-2)*1000);
    }else{
        InitBanker(false);
    }
    SetRobotThinkTime();
    return true;
}
//游戏结束
bool    CGameSanGongTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end:%d--%d",chairID,reason);

	switch(reason)
	{
	case GER_NORMAL:
		{
            CalculateScore();
            net::msg_sangong_game_end msg;
            for(uint16 i=0;i<GAME_PLAYER;++i){
                msg.add_card_types(m_cbTableCardType[i]);
                msg.add_win_multiple(m_winMultiple[i]);
                msg.add_player_score(m_lWinScore[i]);

                net::msg_cards* pcards = msg.add_table_cards();
                for(uint8 j=0;j<5;++j){
                    pcards->add_cards(m_cbHandCardData[i][j]);
                }
            }
            SendMsgToAll(&msg,net::S2C_MSG_SANGONG_GAME_END);

            SaveBlingLog();

            SetGameState(TABLE_STATE_NIUNIU_FREE);
            m_coolLogic.beginCooling(s_FreeTime + (GetPlayNum()-GetChangeCardNum())*1000);
            m_coolRobot.beginCooling(s_FreeTime + (GetPlayNum()-GetChangeCardNum())*1000);
            ResetTable();

		}break;
    default:
        break;
	}
	return false;
}
//用户同意
bool    CGameSanGongTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    if(GetReadyNum() == 2 && m_coolLogic.getCoolTick() < 1000){
        m_coolLogic.beginCooling(1500);// 准备后等一秒
    }
    return true;
}
//玩家进入或离开
void    CGameSanGongTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);            
    if(isJoin){
        SendGameScene(pPlayer);
    }else{      
        if(chairID == m_wBankerUser){
            m_wBankerUser = INVALID_CHAIR;
        }
    } 
}
// 发送场景信息(断线重连)
void    CGameSanGongTable::SendGameScene(CGamePlayer* pPlayer)
{
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("send game scene:%d", chairID);
	switch(m_gameState)
	{
	case net::TABLE_STATE_FREE: //空闲状态
		{
            net::msg_sangong_game_info_free_rep msg;
            msg.set_banker_id(m_wBankerUser);
            msg.set_time_leave(m_coolLogic.getCoolTick());

            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_SANGONG_GAME_FREE_INFO);

		}break;
    case net::TABLE_STATE_CALL:
    case net::TABLE_STATE_WAIT:
	case net::TABLE_STATE_PLAY:	//游戏状态
		{
            net::msg_sangong_game_info_play_rep msg;
            msg.set_game_status(GetGameState());
            msg.set_banker_id(m_wBankerUser);
            msg.set_time_leave(m_coolLogic.getCoolTick());
            for(uint16 i=0;i<GAME_PLAYER;++i){
                msg.add_all_jetton_score(m_lTableScore[i]);
                msg.add_show_cards(m_szShowCardState[i]);
                msg.add_player_status(m_cbPlayStatus[i]);
                msg.add_apply_list(m_szApplyBanker[i]);

                net::msg_cards* pcards = msg.add_table_cards();
                for(uint8 j=0;j<MAX_COUNT;++j){
                    pcards->add_cards(m_cbHandCardData[i][j]);
                }
                msg.add_turn_max_score(m_lTurnMaxScore[i]);
                msg.add_card_types(m_cbTableCardType[i]);
            }
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_SANGONG_GAME_PLAY_INFO);
		}break;
	}    

}
void    CGameSanGongTable::CalcPlayerInfo(uint16 chairID,int64 winScore)
{
    LOG_DEBUG("report game to lobby:%d  %lld",chairID,winScore);
    uint32 uid = m_vecPlayers[chairID].uid;

    CalcPlayerGameInfo(uid,winScore);
    // 修改三公数据
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        uint8 curMaxCard[5];
        //pPlayer->GetGameMaxCard(net::GAME_CATE_NIUNIU,isCoin,curMaxCard,5);
        //BYTE cbUserCardData[GAME_PLAYER][5];
        //memcpy(cbUserCardData,m_cbHandCardData,sizeof(cbUserCardData));
        //m_gameLogic.SortCardList(cbUserCardData[chairID],5);
        //if(m_gameLogic.CompareCard(cbUserCardData[chairID],curMaxCard,5) == false){
        //    pPlayer->AsyncSetGameMaxCard(net::GAME_CATE_ZAJINHUA,isCoin,cbUserCardData[chairID],MAX_COUNT);
       // }
    }
    // 写入手牌log
    WriteOutCardLog(chairID,m_cbHandCardData[chairID],3,m_lTableScore[chairID]);
}
// 重置游戏数据
void    CGameSanGongTable::ResetGameData()
{

	ZeroMemory(m_cbPlayStatus,sizeof(m_cbPlayStatus));
    ZeroMemory(m_szApplyBanker,sizeof(m_szApplyBanker));
    ZeroMemory(m_szShowCardState,sizeof(m_szShowCardState));

	ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
	ZeroMemory(m_lTableScore,sizeof(m_lTableScore));

    ZeroMemory(m_cbTableCardType,sizeof(m_cbTableCardType));         //桌面牌型
    ZeroMemory(m_winMultiple,sizeof(m_winMultiple));                 //输赢倍数
    ZeroMemory(m_lWinScore,sizeof(m_lWinScore));                     //输赢分数
    ZeroMemory(m_lTurnMaxScore,sizeof(m_lTurnMaxScore));             //最大下注

}
// 做牌发牌
void CGameSanGongTable::DispatchCard()
{	
	if(GetOnlinePlayerNum() == 0){
		m_gameLogic.RandCardList(m_cbHandCardData[0],sizeof(m_cbHandCardData)/sizeof(m_cbHandCardData[0][0]));
	}else{		
		uint16 maxNum = g_RandGen.RandRange(70,100) ? 2 : 6;		
		for(uint32 i=0;i<1000;++i)
		{
			m_gameLogic.RandCardList(m_cbHandCardData[0],sizeof(m_cbHandCardData)/sizeof(m_cbHandCardData[0][0]));
			uint16 cardNum = 0;
			for(uint16 j=0;j<GAME_PLAYER;++j)
			{
                if(m_cbPlayStatus[j] == FALSE)
                    continue;
			    uint8 cardType = m_gameLogic.GetCardType(m_cbHandCardData[j],MAX_COUNT);
			    if(cardType <= OX_VALUE5){
					cardNum++;
				}
				if(cardNum > maxNum)
					break;
			}
            if(cardNum > maxNum){
    			continue;
            }else{
                break;
            }
		}		
	}	
}
// 写入出牌log
void    CGameSanGongTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int64 score)
{
    Json::Value logValue;
    logValue["p"] = chairID;
    logValue["s"] = score;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// 写入庄家位log
void    CGameSanGongTable::WriteBankerLog(uint16 chairID)
{
    m_operLog["banker"] = chairID;
}
// 是否能够开始游戏
bool    CGameSanGongTable::IsCanStartGame()
{
    uint16 bCount = 0;
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_vecPlayers[i].pPlayer != NULL && m_vecPlayers[i].readyState == 1){
            bCount++;
        }        
    }    
    
    return bCount >= 2;
}
// 检测筹码是否正确
bool    CGameSanGongTable::CheckJetton(uint16 chairID,int64 score)
{
    if(!m_isNeedBanker)
        return true;

    if(score < 0 || score > m_lTurnMaxScore[chairID]){
        LOG_ERROR("下注错误%lld,%lld",score,m_lTurnMaxScore[chairID]);
        return false;
    }
    return true;
}
// 获得机器人的铁壁
int64   CGameSanGongTable::GetRobotJetton(uint16 chairID)
{
    if(g_RandGen.RandRatio(20,100))
        return m_lTurnMaxScore[chairID]/8;
    if(g_RandGen.RandRatio(30,100))
        return m_lTurnMaxScore[chairID]/4;
    if(g_RandGen.RandRatio(30,100))
        return m_lTurnMaxScore[chairID]/2;

    return m_lTurnMaxScore[chairID];
}
uint16  CGameSanGongTable::GetPlayNum()
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
void    CGameSanGongTable::InitBanker(bool isApply)
{
    net::msg_sangong_banker_result_rep msg;
    if(isApply){
        vector<uint16> bankers;
        for (uint16 i = 0; i < GAME_PLAYER; ++i) {
            if (m_szApplyBanker[i] == TRUE) {
                bankers.push_back(i);
            }
        }
        if(bankers.size() > 0)
        {
            m_wBankerUser = bankers[g_RandGen.RandUInt()%bankers.size()];
            for (uint16 i = 0; i < GAME_PLAYER; ++i) {
                if (m_cbPlayStatus[i] == TRUE && m_szApplyBanker[i] == TRUE) {
                    msg.add_apply_list(i);
                }
            }
        }else{
            m_wBankerUser = g_RandGen.RandUInt() % GAME_PLAYER;
            while (m_cbPlayStatus[m_wBankerUser] == FALSE) {
                m_wBankerUser = (m_wBankerUser + 1) % GAME_PLAYER;
            }
            for (uint16 i = 0; i < GAME_PLAYER; ++i) {
                if (m_cbPlayStatus[i] == TRUE) {
                    msg.add_apply_list(i);
                }
            }
        }
    }
    //庄家积分
    int64 lBankerScore = GetPlayerCurScore(GetPlayer(m_wBankerUser));
    //玩家人数
    WORD wUserCount=0;
    for (WORD i=0;i<GAME_PLAYER;i++) {
        if (m_cbPlayStatus[i] == TRUE)
            wUserCount++;
    }
    //最大下注
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        if(m_cbPlayStatus[i]!=TRUE || i==m_wBankerUser)
            continue;
        //获取积分
        int64 lScore = GetPlayerCurScore(GetPlayer(i));
        //下注变量 客户要求
        m_lTurnMaxScore[i] = min(lBankerScore/(wUserCount-1)/5,lScore/5);
    }
    for(uint16 i=0;i<GAME_PLAYER;++i){
        msg.add_turn_max_score(m_lTurnMaxScore[i]);
    }

    msg.set_banker_id(m_wBankerUser);
    msg.set_is_apply(isApply ? 1:0);
    SendMsgToAll(&msg,net::S2C_MSG_SANGONG_BANKER_RESULT);

    SetGameState(TABLE_STATE_PLAY);
    m_coolLogic.beginCooling(s_AddScoreTime + 500*GetPlayNum());
    m_coolRobot.beginCooling(g_RandGen.RandRange(2000+500*GetPlayNum(),4500));
    WriteBankerLog(m_wBankerUser);
}
void    CGameSanGongTable::SendCardToClient()
{
    if(m_isNeedBanker) {
        for (uint16 i = 0; i < GAME_PLAYER; ++i){
            if (m_cbPlayStatus[i] == TRUE && m_lTableScore[i] == 0){
                OnUserAddScore(i,m_lTurnMaxScore[i]/8);
            }
        }
    }else{
        for (uint16 i = 0; i < GAME_PLAYER; ++i){
            if(m_cbPlayStatus[i] == TRUE && m_lTableScore[i] == 0){
                OnUserAddScore(i,GetBaseScore());
            }
        }
    }
    for(uint16 i=0;i<GAME_PLAYER;++i){
        msg_sangong_send_card_rep msg;
        if(m_cbPlayStatus[i] == TRUE){
            for(uint8 j=0;j<MAX_COUNT;++j){
                msg.add_cards(m_cbHandCardData[i][j]);
            }
        }
        SendMsgToClient(i,&msg,net::S2C_MSG_SANGONG_SENDCARD_REP);
    }
    SetGameState(TABLE_STATE_WAIT);
    m_coolLogic.beginCooling(s_ShowCardTime + 1500*GetPlayNum());
    m_coolRobot.beginCooling(2000 + 1000*GetPlayNum() + g_RandGen.RandRange(1000,3000));

}
// 结算分数
void   CGameSanGongTable::CalculateScore()
{
    LOG_DEBUG("结算分数");
    if(m_isNeedBanker)//带庄家的计算
    {
        for(uint16 i=0;i<GAME_PLAYER;++i)
        {
            if(m_cbPlayStatus[i] == FALSE || i == m_wBankerUser)
                continue;
            bool bWin = m_gameLogic.CompareCard(m_cbHandCardData[m_wBankerUser],m_cbHandCardData[i],MAX_COUNT,m_winMultiple[i]);
            m_cbTableCardType[i] = m_gameLogic.GetCardType(m_cbHandCardData[i],MAX_COUNT);
            if(bWin == false){
                m_lWinScore[i] += m_lTableScore[i]*m_winMultiple[i];
                m_lWinScore[m_wBankerUser] -= m_lTableScore[i]*m_winMultiple[i];
            }else{
                m_lWinScore[i] -= m_lTableScore[i]*m_winMultiple[i];
                m_lWinScore[m_wBankerUser] += m_lTableScore[i]*m_winMultiple[i];
            }
        }
        m_cbTableCardType[m_wBankerUser] = m_gameLogic.GetCardType(m_cbHandCardData[m_wBankerUser],MAX_COUNT);
    }else{//通比三公
        uint16 winChairID = INVALID_CHAIR;
        for(uint16 i=0;i<GAME_PLAYER;++i)
        {
            if(m_cbPlayStatus[i] == FALSE)
                continue;
            m_cbTableCardType[i] = m_gameLogic.GetCardType(m_cbHandCardData[i],MAX_COUNT);
            if(winChairID == INVALID_CHAIR){
                m_gameLogic.CompareCard(m_cbHandCardData[i],m_cbHandCardData[i],MAX_COUNT,m_winMultiple[i]);
                winChairID = i;
                continue;
            }
            bool bWin = m_gameLogic.CompareCard(m_cbHandCardData[winChairID],m_cbHandCardData[i],5,m_winMultiple[i]);
            if(bWin){
                winChairID = i;
            }
        }
        for(uint16 i=0;i<GAME_PLAYER;++i){
            if(m_cbPlayStatus[i] == FALSE || i == winChairID)
                continue;
            m_lWinScore[i] -= min(m_lTableScore[i],m_lTableScore[winChairID]) * m_winMultiple[winChairID];
            m_lWinScore[winChairID] += (-m_lWinScore[i]);
        }
    }
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_cbPlayStatus[i] == FALSE)
            continue;
        CalcPlayerInfo(i,m_lWinScore[i]);
        CCommonLogic::LogCardString(m_cbHandCardData[i],MAX_COUNT);
        LOG_DEBUG("牌型:%d",m_cbTableCardType[i]);
    }

}
// 检测提前结束
void    CGameSanGongTable::CheckOverTime()
{
    uint8 gameState = GetGameState();
    bool bFlag = false;
    switch(gameState)
    {
    case TABLE_STATE_CALL:
        {
            for(uint16 i=0;i<GAME_PLAYER;++i){
                if(m_cbPlayStatus[i] == TRUE && m_szApplyBanker[i] == FALSE){
                    goto EXIT;
                }
            }
            bFlag = true;
        }break;
    case TABLE_STATE_PLAY:
        {
            for(uint16 i=0;i<GAME_PLAYER;++i){
                if(i == m_wBankerUser)
                    continue;
                if(m_cbPlayStatus[i] == TRUE && m_lTableScore[i] == 0){
                    goto EXIT;
                }
            }
            bFlag = true;
        }break;
    case TABLE_STATE_WAIT:
        {
            for(uint16 i=0;i<GAME_PLAYER;++i){
                if(m_cbPlayStatus[i] == TRUE && m_szShowCardState[i] == 0){
                    goto EXIT;
                }
            }
            bFlag = true;
        }break;
    default:
        break;
    }
EXIT:
    if(bFlag){
        LOG_DEBUG("清楚cd：%d",GetGameState());
        m_coolLogic.clearCool();
    }
}
//游戏状态
bool    CGameSanGongTable::IsUserPlaying(WORD wChairID)
{
	ASSERT(wChairID<GAME_PLAYER);
	return (m_cbPlayStatus[wChairID]==TRUE)?true:false;    
}
//加注事件
bool    CGameSanGongTable::OnUserAddScore(WORD wChairID, int64 lScore)
{
    LOG_DEBUG("玩家加注:%d--%lld",wChairID,lScore);
    net::msg_sangong_place_jetton_rep rep;
    rep.set_jetton_score(lScore);

    if(wChairID == m_wBankerUser || !CheckJetton(wChairID,lScore)) {
        rep.set_result(RESULT_CODE_FAIL);
        SendMsgToClient(wChairID,&rep,net::S2C_MSG_SANGONG_PLACE_JETTON_REP);
        return false;
    }
    m_lTableScore[wChairID] = lScore;
    rep.set_result(net::RESULT_CODE_SUCCESS);
    SendMsgToClient(wChairID,&rep,net::S2C_MSG_SANGONG_PLACE_JETTON_REP);

    net::msg_sangong_place_jetton_broadcast broad;
    broad.set_jetton_score(lScore);
    broad.set_chairid(wChairID);

    SendMsgToAll(&broad,net::S2C_MSG_SANGONG_PLACE_JETTON_BROADCAST);
    CheckOverTime();
	return true;    
}
//申请庄家
bool    CGameSanGongTable::OnUserApplyBanker(WORD wChairID,int32 score)
{
    LOG_DEBUG("申请庄家:%d--%d",wChairID,score);
    if(!m_isNeedBanker){
        LOG_DEBUG("不需要申请庄家");
        return false;
    }
    if(m_szApplyBanker[wChairID] != FALSE)
        return false;

    if(score > 0) {
        m_szApplyBanker[wChairID] = TRUE;
    }else{
        m_szApplyBanker[wChairID] = 2;
    }

    net::msg_sangong_apply_banker_rep msg;
    msg.set_chairid(wChairID);
    msg.set_score(score);
    msg.set_result(net::RESULT_CODE_SUCCESS);
    SendMsgToAll(&msg,net::S2C_MSG_SANGONG_APPLY_BANKER_REP);

    CheckOverTime();
    return true;
}
//摆牌
bool    CGameSanGongTable::OnUserChangeCard(WORD wChairID)
{
    LOG_DEBUG("玩家摆牌:%d",wChairID);
    m_szShowCardState[wChairID] = TRUE;
    m_cbTableCardType[wChairID] = m_gameLogic.GetCardType(m_cbHandCardData[wChairID],MAX_COUNT);

    SendChangeCard(wChairID,NULL);

    CheckOverTime();
    return true;
}
//发送摆牌
void    CGameSanGongTable::SendChangeCard(WORD wChairID,CGamePlayer* pPlayer)
{
    net::msg_sangong_change_card_rep msg;
    msg.set_oper_id(wChairID);
    msg.set_result(1);
    msg.set_card_type(m_cbTableCardType[wChairID]);
    for(uint8 i=0;i<MAX_COUNT;++i){
        msg.add_cards(m_cbHandCardData[wChairID][i]);
    }
    if(pPlayer == NULL) {
        SendMsgToAll(&msg, net::S2C_MSG_SANGONG_CHANGE_CARD_REP);
    }else{
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_SANGONG_CHANGE_CARD_REP);
    }
}
int32   CGameSanGongTable::GetChangeCardNum()
{
    int32 num = 0;
    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_szShowCardState[i] == TRUE)
            num++;
    }
    return num;
}
bool    CGameSanGongTable::OnRobotOper()
{
    if(!m_coolRobot.isTimeOut())
        return false;

    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_cbPlayStatus[i] == FALSE || i == m_wBankerUser)
            continue;
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;
        if(m_lTableScore[i] == 0){
            int64 jetton = GetRobotJetton(i);
            OnUserAddScore(i,jetton);
            break;
        }
    }
    m_coolRobot.beginCooling(g_RandGen.RandRange(500,1000));
    CheckOverTime();
    return true;
}
bool    CGameSanGongTable::OnRobotApplyBanker()
{
    if(!m_coolRobot.isTimeOut())
        return false;

    for(uint16 i=0;i<GAME_PLAYER;++i){
        if(m_cbPlayStatus[i] == FALSE)
            continue;
        if(m_szApplyBanker[i] != FALSE)
            continue;
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;
        OnUserApplyBanker(i,g_RandGen.RandRange(0,1));
        break;
    }
    m_coolRobot.beginCooling(g_RandGen.RandRange(500,1000));
    CheckOverTime();
    return true;
}
bool    CGameSanGongTable::OnRobotReady()
{
    if(m_coolLogic.getCoolTick() < 1000){
        ReadyAllRobot();
        return true;
    }
    if(!m_coolRobot.isTimeOut())
        return false;
    for(uint32 i=0;i<m_vecPlayers.size();++i){
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_vecPlayers[i].readyState == 0){
                PlayerReady(pPlayer);
                break;
            }
        }
    }
    return true;
}
bool    CGameSanGongTable::OnRobotChangeCard()
{
    if(!m_coolRobot.isTimeOut())
        return false;
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_cbPlayStatus[i] == TRUE && m_szShowCardState[i] == FALSE){
                OnUserChangeCard(i);
                break;
            }
        }
    }
    m_coolRobot.beginCooling(g_RandGen.RandRange(500,1000));
    return true;
}
void    CGameSanGongTable::CheckAddRobot()
{
    if(m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
        return;

    CheckRobotLeave();
    for(uint32 i=0;i<m_vecPlayers.size();++i){
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            if(m_vecPlayers[i].readyState == 0){
                PlayerReady(pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(1000,2000));
                return;
            }else{
                if((getSysTime() - m_vecPlayers[i].readyTime) > 150){
                    LeaveTable(pPlayer);
                    LOG_DEBUG("time out ready and leave:%d",pPlayer->GetUID());
                    m_coolRobot.beginCooling(g_RandGen.RandRange(2000,4000));
                    return;
                }
            }
        }
    }
    if(GetChairPlayerNum() >= 1 && GetChairPlayerNum() <= 3)
    {
        for(uint32 i=0;i<m_vecPlayers.size();++i){
            CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
            if(pPlayer != NULL && !pPlayer->IsRobot())
            {
                CRobotMgr::Instance().RequestOneRobot(this, pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(1000,3000));
                return;
            }
        }
    }
    SetRobotThinkTime();
}
void    CGameSanGongTable::CheckRobotLeave()
{
    for(uint16 i=0;i<m_vecPlayers.size();++i){
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot()){
            if(g_RandGen.RandRatio(5,100) && CanLeaveTable(pPlayer)){
                LeaveTable(pPlayer);
            }
        }
    }
}
void    CGameSanGongTable::SetRobotThinkTime()
{
    if(GetGameState() == TABLE_STATE_FREE) {
        m_coolRobot.beginCooling(g_RandGen.RandRange(1000, 2000));
    }else{
        m_coolRobot.beginCooling(g_RandGen.RandRange(2000, 3000));
    }
}

























