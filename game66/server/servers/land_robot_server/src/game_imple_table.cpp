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
using namespace game_land;
using namespace net;

namespace
{
    const static int32 s_ReadyTimeOut = 30;
    const static int32 s_ShuffCount[] = {10,10,4,3};
    const static uint8 s_DealSolo[]   = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};// 单张发牌
    const static uint8 s_DealThree[]  = {3,4,5,5};// 3455
    const static uint8 s_DealSeven[]  = {7,10};// 710发牌
};
CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
        case emROOM_TYPE_COMMON:           // 斗地主
            {
                pTable = new CGameLandTable(this,tableID,emTABLE_TYPE_SYSTEM);
            }break;
        case emROOM_TYPE_MATCH:            // 比赛斗地主
            {
                pTable = new CGameLandTable(this,tableID,emTABLE_TYPE_SYSTEM);
            }break;
        case emROOM_TYPE_PRIVATE:          // 私人房斗地主
            {
                pTable = new CGameLandTable(this,tableID,emTABLE_TYPE_PLAYER);
            }break;
        default:
            {
                assert(false);
                return NULL;
            }break;
    }
    return pTable;
}
// 斗地主游戏桌子
CGameLandTable::CGameLandTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

    //炸弹变量
    m_firstUser =   INVALID_CHAIR;
    m_bankerUser=   INVALID_CHAIR;
    m_curUser   =   INVALID_CHAIR;
    memset(m_outCardCount,0,sizeof(m_outCardCount));

    //游戏变量
    m_bombCount = 0;
    memset(m_eachBombCount,0,sizeof(m_eachBombCount));

    //叫分信息
    memset(m_callScore,0,sizeof(m_callScore));

    //出牌信息
    m_turnCardCount=0;
    m_turnWiner=INVALID_CHAIR;
    memset(m_turnCardData,0,sizeof(m_turnCardData));

	m_cbOutCardCount = 0;
	memset(m_outCardData, 0, sizeof(m_outCardData));

	memset(m_cbMemCardMac, 0, sizeof(m_cbMemCardMac));
	


    //扑克信息
    memset(m_bankerCard,0,sizeof(m_bankerCard));
    memset(m_handCardData,0,sizeof(m_handCardData));
    memset(m_handCardCount,0,sizeof(m_handCardCount));

	m_isaddblockers = 0;
	m_argRobotLevel = BEST_LEVEL_2;
	//m_bGameEnd = false;
	//m_coolKickPlayer.clearCool();
}
CGameLandTable::~CGameLandTable()
{

}
bool    CGameLandTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (m_pHostRoom != NULL && m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_COIN && IsExistIP(pPlayer->GetIP()))
	{
		return false;
	}
    // 限制一个桌子1个或者3个玩家
	/*
    bool bFlag = false;
    if(!pPlayer->IsRobot())//玩家
    {
        if(GetOnlinePlayerNum() == GetChairPlayerNum())
		{//全玩家
            bFlag = true;
        }
		else if(GetOnlinePlayerNum() == 0)
		{//全机器人
            bFlag = true;
        }
    }
	else
	{//机器人
        if(GetOnlinePlayerNum() == 1)
		{
            bFlag = true;
        }
        if(GetOnlinePlayerNum() == 0 && GetChairPlayerNum() < 2)
		{
            bFlag = true;
        }
    }
	if (bFlag == false)
	{
		return false;
	}
	*/
    // 是否有屏蔽玩家
	if (m_pHostRoom != NULL && m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_COIN && IsExistBlock(pPlayer))
	{
		return false;
	}

	//增加新注册用户的判断
	bool bIsNewRegisterWelfare = EnterNewRegisterWelfare(pPlayer);
	LOG_DEBUG("uid:%d,roomid:%d,tableid:%d,bIsNewRegisterWelfare:%d", pPlayer->GetUID(), GetRoomID(), GetTableID(), bIsNewRegisterWelfare);
	if (bIsNewRegisterWelfare == true)
	{
		return false;
	}

    return CGameTable::CanEnterTable(pPlayer);
}
bool    CGameLandTable::CanLeaveTable(CGamePlayer* pPlayer)
{
	if (GetGameState() != TABLE_STATE_FREE)
	{
		return false;
	}
    if(pPlayer->IsRobot())
	{//机器人延迟离开
		if (m_coolLogic.getPassTick() < 1000)
		{
			return false;
		}
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
void    CGameLandTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::land_table_info* pland = pInfo->mutable_land();
    pland->set_tableid(GetTableID());
    pland->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pland->set_is_passwd(1);
    }else{
        pland->set_is_passwd(0);
    }
    pland->set_hostname(m_conf.hostName);
    pland->set_deal(m_conf.deal);
    pland->set_basescore(m_conf.baseScore);
    pland->set_consume(m_conf.consume);
    pland->set_entermin(m_conf.enterMin);
    pland->set_duetime(m_conf.dueTime);
    pland->set_feetype(m_conf.feeType);
    pland->set_feevalue(m_conf.feeValue);
    pland->set_show_hand_num(m_pHostRoom->GetShowHandNum());
    pland->set_call_time(GetCallScoreTime());
    pland->set_card_time(GetOutCardTime());

}

//配置桌子
bool CGameLandTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);

    m_vecPlayers.resize(GAME_LAND_PLAYER);
    for(uint8 i=0;i<GAME_LAND_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }

	if (m_pHostRoom == NULL)
	{
		return true;
	}
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("analysis game room param json error:%s", param.c_str());
		return true;
	}
	if (jvalue.isMember("isaddblockers")) {
		m_isaddblockers = jvalue["isaddblockers"].asInt();
	}	
	ReAnalysisParam();
	SetMaxChairNum(GAME_LAND_PLAYER); // add by har
    return true;
}
bool CGameLandTable::ReAnalysisParam()
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", m_pHostRoom->GetRoomID(), param.c_str());
		return true;
	}
	if (jvalue.isMember("isaddblockers") && jvalue["isaddblockers"].isIntegral()) {
		m_isaddblockers = jvalue["isaddblockers"].asInt();
	}
	if (jvalue.isMember("bbw") && jvalue["bbw"].isIntegral())
	{
		m_robotBankerWinPro = jvalue["bbw"].asInt();
	}
	if (jvalue.isMember("rbl") && jvalue["rbl"].isIntegral())
	{
		int argRobotLevel = jvalue["rbl"].asInt();
		if (argRobotLevel >= BASE_LEVEL && argRobotLevel <= BEST_LEVEL_2)
		{
			m_argRobotLevel = argRobotLevel;
		}
	}

	LOG_DEBUG("roomid:%d,tableid:%d,m_isaddblockers:%d,m_robotBankerWinPro:%d,m_argRobotLevel:%d",
		GetRoomID(), GetTableID(), m_isaddblockers, m_robotBankerWinPro, m_argRobotLevel);

	return true;
}


void CGameLandTable::ShutDown()
{
    
    
    
}
//复位桌子
void CGameLandTable::ResetTable()
{
    ResetGameData();

    SetGameState(TABLE_STATE_FREE);
    ResetPlayerReady();
    ReInitPoker();
}
void CGameLandTable::OnTimeTick()
{
	OnTableTick();
    if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FREE:
            {
                CheckAddRobot();
                CheckReadyTimeOut();
                if(IsAllReady()){
                    OnGameStart();
                }
            }break;
        case TABLE_STATE_CALL:
            {
                OnCallScoreTimeOut();
            }break;
        case TABLE_STATE_PLAY:
            {
                OnOutCardTimeOut();
            }break;
        default:
            break;
        }
    }
	//if (m_coolKickPlayer.isTimeOut())
	//{
	//	KickPlayerInTable();
	//}
    if(GetGameState() == TABLE_STATE_CALL)
    {
		if (m_curUser < GAME_LAND_PLAYER)
		{
			CGamePlayer* pPlayer = m_vecPlayers[m_curUser].pPlayer;
			if (pPlayer != NULL && pPlayer->IsRobot() && m_coolRobot.isTimeOut())// 托管
			{
				OnRobotCallScore();
			}
		}
		else
		{
			LOG_DEBUG("cur_chairid_error - roomid:%d,tableid:%d,m_curUser:%d", GetRoomID(), GetTableID(), m_curUser);
		}
    }
    if(GetGameState() == TABLE_STATE_PLAY)
    {
		if (m_curUser < GAME_LAND_PLAYER)
		{
			//if (m_vecPlayers[m_curUser].overTimes == 2)
			//{// 超时2次托管
			//	m_vecPlayers[m_curUser].autoState = 1;
			//	SendReadyStateToClient();
			//}
			CGamePlayer* pPlayer = m_vecPlayers[m_curUser].pPlayer;
			if (pPlayer != NULL && pPlayer->IsRobot() && m_coolRobot.isTimeOut())
			{
				OnRobotOutCard();
				return;
			}
			if (pPlayer != NULL && pPlayer->IsRobot() == false)
			{
				if (m_vecPlayers[m_curUser].autoState == 1 && m_coolLogic.getPassTick() > 1000)// 托管
				{
					OnUserAutoCard();
				}
			}
		}
		else
		{
			LOG_DEBUG("cur_chairid_error - roomid:%d,tableid:%d,m_curUser:%d", GetRoomID(), GetTableID(), m_curUser);
		}
    }
}
// 游戏消息
int CGameLandTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    uint16 chairID = GetChairID(pPlayer);

	LOG_DEBUG("table recv - roomid:%d,tableid:%d,pPlayer:%p,cmdID:%d,chairID:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer, cmdID, chairID,GetPlayerID(chairID));

    switch(cmdID)
    {
    case net::C2S_MSG_LAND_CALL_SCORE_REQ:// 用户叫分
        {
            net::msg_land_call_score_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            //状态效验
            if(GetGameState() != TABLE_STATE_CALL)
            {
				LOG_DEBUG("not_call_state - roomid:%d,tableid:%d,uid:%d,chairID:%d,status:%d", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, GetGameState());
                return 0;
            }
            //消息处理
            return OnUserCallScore(chairID,msg.call_score());
        }break;
    case net::C2S_MSG_LAND_OUT_CARD_REQ:// 用户出牌
        {
            net::msg_land_out_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            //状态效验
            if(GetGameState() != TABLE_STATE_PLAY) {
                LOG_DEBUG("not play state")
                return 0;
            }
            //消息处理
            if(msg.card_data_size() > MAX_LAND_COUNT)
            {
                LOG_ERROR("the card is more 20:%d",chairID);
                return 0;
            }
            uint8 cardCount = 0;
            uint8 cardData[MAX_LAND_COUNT];
            memset(cardData,0,sizeof(cardData));
            for(uint8 i=0;i<msg.card_data_size();++i){
                cardData[i] = msg.card_data(i);
                cardCount++;
            }
            return OnUserOutCard(chairID,cardData,cardCount);
        }break;
    case net::C2S_MSG_LAND_PASS_CARD_REQ:// 用户放弃
        {
            net::msg_land_pass_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            //状态效验
            if(GetGameState() != TABLE_STATE_PLAY) {
                LOG_DEBUG("not play state");
                return 0;
            }
            //消息处理
            return OnUserPassCard(chairID);
        }break;
    case net::C2S_MSG_LAND_REQ_HAND_CARD:// 请求手牌
        {
            net::msg_land_req_hand_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            SendHandleCard(chairID);
            return 0;
        }break;
	case net::C2S_MSG_LAND_REQ_HAND_CARD_2:// 请求手牌
		{
			net::msg_land_req_hand_card_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			SendHandleCard_2(chairID);
			return 0;
		}break;
    default:
        return 0;
    }
    return 0;
}

// 0  1  2      3  4  5    6  7  8
// 0  1  2

// 9 10 11     12 13 14	  15 16 17
// 3  4  5

//18 19 20     21 22 23   24 25 26
// 6  7  8

//27 28 29     30 31 32   33 34 35
// 9 10 11

//36 37 38     39 40 41   42 43 44
//12 13 14

//45 46 47     48 49 50
//15 16 17

//51 52 53



uint16  CGameLandTable::PushUserCard(uint16 startUser, uint8 validCardData, uint8 & cardIndex)
{
	uint8 dealType = GetDeal();
	switch (dealType)
	{
	case net::ROOM_DEAL_TYPE_SOLO:
	{
		return PushUserCardByDeal(s_DealSolo, getArrayLen(s_DealSolo), startUser, validCardData, cardIndex);
	}break;
	case net::ROOM_DEAL_TYPE_THREE:
	{
		return PushUserCardByDeal(s_DealThree, getArrayLen(s_DealThree), startUser, validCardData, cardIndex);
	}break;
	case net::ROOM_DEAL_TYPE_SEVEN:
	{
		return PushUserCardByDeal(s_DealSeven, getArrayLen(s_DealSeven), startUser, validCardData, cardIndex);
	}break;
	default:
		break;
	}
	return PushUserCardByDeal(s_DealSolo, getArrayLen(s_DealSolo), startUser, validCardData,cardIndex);
}
uint16  CGameLandTable::PushUserCardByDeal(const uint8 dealArry[], uint8 dealCount, uint16 startUser, uint8 validCardData, uint8 & cardIndex)
{
	uint8   cardpos = 0;
	uint16  curUser = 0;
	uint8 pollCard[FULL_POKER_COUNT] = { 0 };
	uint8   handCardCount[GAME_LAND_PLAYER] = { 0 };
	//memcpy(handCardCount, m_handCardCount, sizeof(handCardCount));

	for (uint8 m = 0; m<dealCount; ++m)
	{
		uint8 cardCount = dealArry[m];
		for (uint16 i = 0; i < GAME_LAND_PLAYER; ++i)
		{
			uint16 userIndex = (startUser + i) % GAME_LAND_PLAYER;
			for (uint8 j = 0; j<cardCount; ++j)
			{
				pollCard[cardpos] = m_handCardData[userIndex][handCardCount[userIndex]];
				handCardCount[userIndex]++;
				//if (cardpos == cardIndex) {
				//	curUser = userIndex;
				//}
				cardpos++;
			}
		}
	}

	for (int i = 0; i < FULL_POKER_COUNT; i++)
	{
		if (pollCard[i] == validCardData)
		{
			cardIndex = i;
			break;
		}
	}

	return curUser;
}

// 游戏开始
bool CGameLandTable::OnGameStart()
{
    //LOG_DEBUG("game start");
    // 出牌信息
    m_turnCardCount = 0;
    m_turnWiner     = INVALID_CHAIR;
    memset(m_turnCardData,0,sizeof(m_turnCardData));
    memset(m_pressCount,0,sizeof(m_pressCount));
    memset(m_bankrupts,0, sizeof(m_bankrupts));
    memset(m_sendHand,0,sizeof(m_sendHand));
	m_nrw_ctrl_uid = 0;
	m_cbOutCardCount = 0;
	memset(m_outCardData, 0, sizeof(m_outCardData));
	m_lucky_flag = false;

	for (int j = 0; j < GAME_LAND_PLAYER; j++)
	{
		for (int i = 1; i < MAX_SIG_CARD_COUNT; i++)
		{
			m_cbMemCardMac[j][i] = 4;
			if (i >= 14)
			{
				m_cbMemCardMac[j][i] = 1;
			}
		}
	}

    SetGameState(net::TABLE_STATE_CALL);

    // 混淆扑克
    ShuffleTableCard(GetDeal());

    // 抽取明牌
    uint8   validCardData   = 0;
    uint8   validCardIndex  = 0;
    uint16  startUser       = m_firstUser;
    uint16  curUser         = m_firstUser;

    //抽取扑克
    validCardIndex = g_RandGen.RandUInt()%DISPATCH_COUNT;
    validCardData  = m_randCard[validCardIndex];

    //设置用户
    startUser   =  m_gameLogic.GetCardValue(validCardData)%GAME_LAND_PLAYER;
    curUser = DispatchUserCard(startUser,validCardIndex);

    //设置底牌
    memcpy(m_bankerCard,&m_randCard[DISPATCH_COUNT],sizeof(m_bankerCard));

	bool bControlCardData = SetControlCardData();
	if (bControlCardData && m_evaluateUser < GAME_LAND_PLAYER)
	{
		curUser = m_evaluateUser;
		int randIndex = g_RandGen.RandRange(1, 15);
		validCardData = m_handCardData[m_evaluateUser][randIndex];

	}

	PushUserCard(startUser, validCardData, validCardIndex);


    //设置用户
    m_firstUser   = curUser;
    m_curUser     = curUser;
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d %d %d,m_curUser:%d,m_firstUser:%d,startUser:%d,curUser:%d,m_evaluateUser:%d,GetIsAllRobotOrPlayerJetton:%d,validCardIndex:%d,validCardData:0x%02X",
		GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), m_curUser, m_firstUser, startUser, curUser, m_evaluateUser, GetIsAllRobotOrPlayerJetton(), validCardIndex, validCardData);


    //发送数据
    for(uint16 i=0;i<GAME_LAND_PLAYER;i++)
    {
        //构造消息
        net::msg_land_start_rep gameStart;
        gameStart.set_start_user(startUser);
        gameStart.set_cur_user(curUser);
        gameStart.set_valid_card_data(validCardData);
        gameStart.set_valid_card_index(validCardIndex);

        //构造扑克(闷抓先不发牌 toney)
        for(uint8 j=0;j<m_handCardCount[i];++j){
            gameStart.add_card_data(m_handCardData[i][j]);
        }
        //发送数据
        SendMsgToClient(i,&gameStart,net::S2C_MSG_LAND_START);
    }

    //排列扑克
    for(uint16 i=0;i<GAME_LAND_PLAYER;i++)
    {
        m_gameLogic.SortCardList(m_handCardData[i],m_handCardCount[i],ST_ORDER);
    }
    // 启动叫分定时器
    m_coolLogic.beginCooling(GetCallScoreTime());
    OnSetRobotThinkTime();
    OnSubGameStart();
    return true;
}
//游戏结束
bool CGameLandTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end:%d--%d",chairID,reason);
    m_coolLogic.clearCool();
    OnSetRobotThinkTime();

    switch(reason)
    {
    case GER_NORMAL:		//常规结束
        {
            net::msg_land_game_over_rep gameOver;
            // 游戏积分计算

            // 炸弹信息
            gameOver.set_bomb_count(m_bombCount);
            for(uint8 i=0;i<GAME_LAND_PLAYER;++i)
            {
                gameOver.add_each_bomb_counts(m_eachBombCount[i]);
            }
            //用户扑克
            for(uint16 i=0;i<GAME_LAND_PLAYER;i++)
            {
                //拷贝扑克
                gameOver.add_card_counts(m_handCardCount[i]);
                for(uint8 j=0;j<m_handCardCount[i];++j)
                {
                    gameOver.add_hand_card_data(m_handCardData[i][j]);
                }
                msg_cards* pCards = gameOver.add_all_card_data();
                for(uint8 j=0;j<m_allCardCount[i];++j)
                {
                    pCards->add_cards(m_allCardData[i][j]);
                }                
                PushCardToPool(m_handCardData[i],m_handCardCount[i]);
            }
            
            
            //炸弹统计
            int64 scoreTimes = 1;
            for(uint8 i=0;i<m_bombCount;i++){
                scoreTimes *= 2;
            }
            //春天判断
            uint8 isSpring = 0;
            if(chairID == m_bankerUser)
            {
                //用户定义
                uint16 user1=(m_bankerUser+1)%GAME_LAND_PLAYER;
                uint16 user2=(m_bankerUser+2)%GAME_LAND_PLAYER;
                gameOver.set_chun_tian(0);

                //用户判断
                if((m_outCardCount[user1] == 0) && (m_outCardCount[user2] == 0))
                {
                    scoreTimes *= 2;
                    gameOver.set_chun_tian(1);
                    isSpring = 1;
                }
            }
            //反春天判断
            if(chairID != m_bankerUser)
            {
                gameOver.set_fan_chun_tian(0);
                if(m_outCardCount[m_bankerUser] == 1)
                {
                    scoreTimes *= 2;
                    gameOver.set_fan_chun_tian(1);
                    isSpring = 1;
                }
            }
            //闷抓 
            gameOver.set_blind(0);
            scoreTimes *= m_callScore[m_bankerUser];
            
            //调整倍数

            //统计积分
            int64 calcScore[GAME_LAND_PLAYER];
            int64 loseNum = 0;
            memset(calcScore,0,sizeof(calcScore));

            // 计算输赢分数,最多只赢自己身上携带
            bool  isLandWin    = (m_handCardCount[m_bankerUser] == 0) ? true : false;
            int64 curLandScore = GetPlayerCurScore(GetPlayer(m_bankerUser))/2;// 能结算的金币
            {
                // 计算输的分数
                for(uint16 i = 0;i < GAME_LAND_PLAYER;i++)
                {
                    //变量定义
                    int64 userScore = 0;
                    uint8 isLand = (i == m_bankerUser) ? 1 : 0;
					if (isLand)
					{
						continue;
					}
					if (GetPlayer(i)==NULL)
					{
						continue;
					}
                    //计算积分
                    userScore = GetBaseScore() * scoreTimes;
                    int64 curScore  = GetPlayerCurScore(GetPlayer(i));
                    int64 needScore = MIN(curScore,curLandScore);
                    calcScore[i] = (MIN(userScore,needScore));
                    if(isLandWin){
                        calcScore[i] = -calcScore[i];
                    }
                    loseNum += calcScore[i];
                    DeducEndFee(GetPlayer(i)->GetUID(),calcScore[i]);
                    ChangeScoreValue(i,calcScore[i],emACCTRAN_OPER_TYPE_GAME,m_pHostRoom->GetGameType());//写入积分到数据库
                    if(GetPlayerCurScore(GetPlayer(i)) == 0){
                        m_bankrupts[i] = 1;
                    }
                }
                calcScore[m_bankerUser] = -loseNum;
                // 计算赢的分数
				if (GetPlayer(m_bankerUser) != NULL)
				{
					DeducEndFee(GetPlayer(m_bankerUser)->GetUID(), calcScore[m_bankerUser]);
				}
                ChangeScoreValue(m_bankerUser,calcScore[m_bankerUser],emACCTRAN_OPER_TYPE_GAME,m_pHostRoom->GetGameType());//写入积分到数据库
                if(GetPlayerCurScore(GetPlayer(m_bankerUser)) == 0){
                    m_bankrupts[m_bankerUser] = 1;
                }
            }

            for(uint16 i = 0;i<GAME_LAND_PLAYER;++i)
			{
                gameOver.add_scores(calcScore[i]);
                CGamePlayer* pGamePlayer = GetPlayer(i);
				if (pGamePlayer == NULL)
				{
					continue;
				}
                FlushUserBlingLog(pGamePlayer,calcScore[i],0,(i==m_bankerUser) ? 1 : 0);             
            }
            //发送数据
            SendMsgToAll(&gameOver,net::S2C_MSG_LAND_GAME_OVER);

			int64 playerAllWinScore = 0; // 玩家总赢分 add by har
            for(uint16 i=0;i<GAME_LAND_PLAYER;++i)
			{
                uint8 isLand = (i==m_bankerUser) ? 1 : 0;
                uint8 isWin = 0;
                if(calcScore[i] > 0)
				{
                    isWin = 1;
                }
				CGamePlayer *pPlayer = GetPlayer(i);
				if (pPlayer != NULL) {
					CalcPlayerInfo(pPlayer, i, isWin, isSpring, isLand, calcScore[i]);
					if (!pPlayer->IsRobot())
						playerAllWinScore += calcScore[i]; // add by har
				}
            }

			//更新新注册玩家福利数据   
			if (IsNewRegisterWelfareTable())
			{				
				for (uint16 i = 0; i < GAME_LAND_PLAYER; ++i)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL && !pGamePlayer->IsRobot())
					{
						if (pGamePlayer->GetUID() == m_nrw_ctrl_uid)
						{
							if (m_nrw_status == 2 && calcScore[i] > 0)
							{
								pGamePlayer->UpdateNRWPlayerScore(calcScore[i]);
								LOG_DEBUG("control current player is lost. but current player is win. uid:%d score:%d", m_nrw_ctrl_uid, calcScore[i]);
							}
							else
							{
								UpdateNewRegisterWelfareInfo(m_nrw_ctrl_uid, calcScore[i]);
							}
						}
						else
						{
							pGamePlayer->UpdateNRWPlayerScore(calcScore[i]);
						}
						break;
					}
				}							
			}

			//更新幸运值数据   
			if (m_lucky_flag)
			{
				for (uint16 i = 0; i < GAME_LAND_PLAYER; ++i)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL)
					{
						auto iter = m_set_ctrl_lucky_uid.find(pGamePlayer->GetUID());
						if (iter != m_set_ctrl_lucky_uid.end())
						{
							pGamePlayer->SetLuckyInfo(GetRoomID(), calcScore[i]);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), calcScore[i]);
						}						
					}
				}
			}

            //切换用户
            m_firstUser = chairID;
            //结束游戏
			WriteRobotID();
            ResetGameData();
            SetGameState(TABLE_STATE_FREE);
            ResetPlayerReady();
            //SendReadyStateToClient();
            SendSeatInfoToClient();
			
            SaveBlingLog();
			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,playerAllWinScore:%lld", GetRoomID(), GetTableID(), playerAllWinScore);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
			OnTableGameEnd();
			if (m_isaddblockers == 1)
			{
				AddBlockers();
			}
			//m_bGameEnd = true;
			//m_coolKickPlayer.beginCooling(3000);
			CheckPlayerScoreManyLeave();

            return true;
        }break;
    case GER_DISMISS:		//游戏解散
        {
            LOG_ERROR("froce dis game");
            for(uint8 i=0;i<GAME_LAND_PLAYER;++i)
            {
                if(m_vecPlayers[i].pPlayer != NULL)
				{
					LOG_DEBUG("play_leave - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), m_vecPlayers[i].pPlayer->GetUID());
                    LeaveTable(m_vecPlayers[i].pPlayer);
                }
            }
            ResetTable();
            return true;
        }break;
    case GER_USER_LEAVE:	//用户强退
    case GER_NETWORK_ERROR:	//网络中断
    default:
        return true;
    }
    //错误断言
    assert(false);
    return false;
}

void CGameLandTable::KickPlayerInTable()
{
	//if (m_bGameEnd == false)
	//{
	//	return;
	//}
	net::msg_land_marry_player msg;
	for (uint32 j = 0; j<m_vecPlayers.size(); ++j)
	{
		CGamePlayer* pPlayer = m_vecPlayers[j].pPlayer;
		if (pPlayer != NULL)
		{
			uint32 uid = pPlayer->GetUID();
			bool bCanLevelTable = CanLeaveTable(pPlayer);
			bool bLevelTable = false;
			if(bCanLevelTable)
			{
				LOG_DEBUG("play_leave - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());

				bLevelTable = LeaveTable(pPlayer, false);
			}
			if (bLevelTable && m_pHostRoom != NULL)
			{
				m_pHostRoom->JoinMarry(pPlayer, GetTableID());

				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_LAND_MARRY_PLAYER);
			}
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,bCanLevelTable:%d,bLevelTable:%d",
				GetRoomID(), GetTableID(), uid, bCanLevelTable, bLevelTable);
		}
	}
	//m_bGameEnd = false;
}


//用户同意
bool    CGameLandTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    if(m_coolLogic.isTimeOut()){
        m_coolLogic.beginCooling(500);// 准备后等一秒
    }
    return true;
}
//玩家进入或离开
void  CGameLandTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    ReInitPoker();
    pPlayer->SetAutoReady(true);
}
// 发送手中扑克牌
void  CGameLandTable::SendHandleCard(uint16 chairID)
{
    if(chairID >= GAME_LAND_PLAYER)
        return;
    //发送数据
    net::msg_land_hand_card_rep msg;
    msg.set_chair_id(chairID);
    for(uint8 j=0;j<m_handCardCount[chairID];++j){
        msg.add_card_data(m_handCardData[chairID][j]);
    }

    //发送数据
    SendMsgToClient(chairID,&msg,net::S2C_MSG_LAND_HAND_CARD);
    m_sendHand[chairID] = 1;
}

void  CGameLandTable::SendHandleCard_2(uint16 chairID)
{
	if (chairID >= GAME_LAND_PLAYER)
		return;
	//发送数据
	net::msg_land_hand_card_rep msg;
	msg.set_chair_id(chairID);
	for (uint8 j = 0; j<m_handCardCount[chairID]; ++j) {
		msg.add_card_data(m_handCardData[chairID][j]);
	}

	//发送数据
	SendMsgToClient(chairID, &msg, net::S2C_MSG_LAND_HAND_CARD_2);
	//m_sendHand[chairID] = 1;
}

// 用户放弃
bool CGameLandTable::OnUserPassCard(uint16 chairID)
{
    //效验状态
    if(chairID != m_curUser || m_turnCardCount == 0){
        LOG_ERROR("not you oper:%d",chairID);
        return false;
    }
	
	LOG_DEBUG("1 m_turnCardCount:%d,m_curUser:%d,m_turnWiner:%d,uid: %d - %d", m_turnCardCount, m_curUser, m_turnWiner, GetPlayerID(m_curUser), GetPlayerID(m_turnWiner));

	//设置变量
    m_curUser = (m_curUser+1)%GAME_LAND_PLAYER;
    if(m_curUser == m_turnWiner)
	{
        m_turnCardCount = 0;
        m_pressCount[m_curUser]++;// 无人出牌，压制
    }

	LOG_DEBUG("cur_out_card_info - uid:%d,chairID:%d,m_curUser:%d,bankerUid:%d,m_bankerUser:%d", GetPlayerID(chairID), chairID, m_curUser, GetPlayerID(m_bankerUser), m_bankerUser);

	LOG_DEBUG("2 m_turnCardCount:%d,m_curUser:%d,m_turnWiner:%d,uid: %d - %d", m_turnCardCount, m_curUser, m_turnWiner, GetPlayerID(m_curUser), GetPlayerID(m_turnWiner));

    uint8 isOver = m_turnCardCount == 0 ? 1 : 0;

    net::msg_land_pass_card_rep passCard;
    passCard.set_pass_card_user(chairID);
    passCard.set_cur_user(m_curUser);
    passCard.set_turn_over(isOver);

    SendMsgToAll(&passCard,net::S2C_MSG_LAND_PASS_CARD);
    m_coolLogic.beginCooling(GetOutCardTime());
    OnSetRobotThinkTime(true);

	//LOG_DEBUG("1 roomid:%d,tableid:%d,chairID:%d,robot:%d,uid:%d", GetRoomID(), GetTableID(), chairID, ChairIsRobot(chairID), GetPlayerID(chairID));
	int argSeat = chairID;
	std::vector<int> argHandCards;
	for (WORD wChairID = 0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	{
		//LOG_DEBUG("roomid:%d,tableid:%d,wChairID:%d,robot:%d,uid:%d", GetRoomID(), GetTableID(), wChairID, ChairIsRobot(wChairID), GetPlayerID(wChairID));
		//if (ChairIsRobot(wChairID) == false)
		//{
		//	continue;
		//}
		bool bInTakeOutCard = m_LordRobot[wChairID].RbtInTakeOutCard(argSeat, argHandCards);
		
		string strCard;
		for (unsigned int i = 0; i < argHandCards.size(); ++i)
		{
			strCard += CStringUtility::FormatToString("%d ", argHandCards[i]);
		}
		LOG_DEBUG("land_robot_ai_log 收到出牌 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInTakeOutCard:%d,argSeat:%d,strCard:%s", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInTakeOutCard, argSeat, strCard.c_str());


		//LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInTakeOutCard:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInTakeOutCard);
	}

	//LOG_DEBUG("2 roomid:%d,tableid:%d,chairID:%d,robot:%d,uid:%d", GetRoomID(), GetTableID(), chairID, ChairIsRobot(chairID), GetPlayerID(chairID));


    return true;
}
// 用户叫分
bool CGameLandTable::OnUserCallScore(uint16 chairID,uint8 score)
{
    //LOG_DEBUG("call score %d--%d",chairID,score);

	LOG_DEBUG("start_call_state - roomid:%d,tableid:%d,uid:%d,chairID:%d,m_curUser:%d,m_bankerUser:%d,m_firstUser:%d,next:%d,status:%d,score:%d,m_curCallScore:%d",
		GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, m_curUser, m_bankerUser, m_firstUser, ((chairID + 1) % GAME_LAND_PLAYER), GetGameState(), score, m_curCallScore);

	if (GetGameState() != TABLE_STATE_CALL)
	{
		LOG_DEBUG("not_call_state - roomid:%d,tableid:%d,uid:%d,chairID:%d,status:%d", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, GetGameState());
		return 0;
	}

    //效验状态
    if(chairID != m_curUser) {
        LOG_DEBUG("you can't oper call:%d--%d",m_curUser,chairID);
        return false;
    }
    //效验参数
    if(score > 3 || score <= m_curCallScore){
        LOG_DEBUG("地主叫分错误:%d",score);
        score = 0;
    }
    if(score > m_curCallScore){
        m_curCallScore = score;
    }
    //设置叫分
    m_callScore[chairID] = score;
    
    //设置状态,3分或者最后一家
    if(m_curCallScore == 3)
    {
        m_bankerUser = m_curUser;
        /*if(m_sendHand[chairID] == 1){// 看牌过的玩家不能闷抓
            score = 1;
        }*/
    } 
    if(m_firstUser == (chairID+1)%GAME_LAND_PLAYER)//最后一家
    {
        uint16 maxScoreChairID = m_firstUser;
        for(uint16 i=0;i<GAME_LAND_PLAYER;++i)
        {
            uint16 pos = (m_firstUser+i)%GAME_LAND_PLAYER;
            if(m_callScore[pos] > m_callScore[maxScoreChairID]){
                maxScoreChairID = pos;
            }            
        }
        if(m_callScore[maxScoreChairID] > 0){
            m_bankerUser = maxScoreChairID;
        }                    
    }    

    //设置用户
    if(m_bankerUser != INVALID_CHAIR || m_firstUser == (chairID+1)%GAME_LAND_PLAYER)
    {
        m_curUser = INVALID_CHAIR;
    }else{
        m_curUser = (chairID+1)%GAME_LAND_PLAYER;
    }

    net::msg_land_call_score_rep callScore;
    callScore.set_call_user(chairID);
    callScore.set_cur_user(m_curUser);
    callScore.set_call_score(score);
    SendMsgToAll(&callScore,net::S2C_MSG_LAND_CALL_SCORE);

	for (WORD wChairID = 0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	{
		//if (ChairIsRobot(wChairID) == false)
		//{
		//	continue;
		//}
		bool bInCallScore = m_LordRobot[wChairID].RbtInCallScore(chairID, score);

		LOG_DEBUG("land_robot_ai_log 收到叫分 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInCallScore:%d,score:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInCallScore, score);


		LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInCallScore:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInCallScore);

	}

    //无人叫分重新开始
    if(m_bankerUser == INVALID_CHAIR && m_firstUser == (chairID+1)%GAME_LAND_PLAYER)
    {
        ReGameStart();
        return true;
    }
    if(m_bankerUser != INVALID_CHAIR) 
    {   
        // 叫地主
        // 设置状态
        SetGameState(TABLE_STATE_PLAY);
        // 开启超时定时器
        m_coolLogic.beginCooling(GetOutCardTime());
        //设置变量
        if(m_bankerUser == INVALID_CHAIR)
            m_bankerUser = m_firstUser;

        //发送底牌
        m_handCardCount[m_bankerUser] += getArrayLen(m_bankerCard);
        memcpy(&m_handCardData[m_bankerUser][NORMAL_COUNT],m_bankerCard,sizeof(m_bankerCard));

        //排列扑克
        m_gameLogic.SortCardList(m_handCardData[m_bankerUser], m_handCardCount[m_bankerUser], ST_ORDER);

        //设置用户
        m_turnWiner = m_bankerUser;
        m_curUser   = m_bankerUser;

        //发送消息
        net::msg_land_banker_info_rep bankerInfo;
        bankerInfo.set_banker_user(m_bankerUser);
        bankerInfo.set_cur_user(m_curUser);
        bankerInfo.set_call_score(m_callScore[m_bankerUser]);
        uint8 iSize = getArrayLen(m_bankerCard);
        for(uint8 i=0;i<iSize;++i){
            bankerInfo.add_banker_card(m_bankerCard[i]);
        }

        //SendMsgToAll(&bankerInfo,net::S2C_MSG_LAND_BANKER_INFO);

		//去掉手牌

		for (int i = 0; i < GAME_LAND_PLAYER; i++)
		{
			for (uint8 j = 0; j<m_handCardCount[i]; ++j)
			{
				BYTE cbIndex = m_gameLogic.GetCardIndex(m_handCardData[i][j]);
				if (m_cbMemCardMac[i][cbIndex]>0)
				{
					m_cbMemCardMac[i][cbIndex]--;
				}						
			}
		}

		//for (int i = 0; i < GAME_LAND_PLAYER; i++)
		//{
		//	for (int m = 0; m < MAX_SIG_CARD_COUNT; m++)
		//	{
		//		LOG_DEBUG("brank info - roomid:%d,tableid:%d,m_cbMemCardMac[%d][%d]:%d",m_pHostRoom->GetRoomID(),GetTableID(),i,m, m_cbMemCardMac[i][m]);
		//	}
		//}

		for (int i = 0; i < GAME_LAND_PLAYER; i++)
		{
			bankerInfo.clear_mem_card_mac();

			for (int m = 1; m < MAX_SIG_CARD_COUNT; m++)
			{
				bankerInfo.add_mem_card_mac(m_cbMemCardMac[i][m]);
			}
			SendMsgToClient(i, &bankerInfo, net::S2C_MSG_LAND_BANKER_INFO);
		}

        InitBlingLog();
        DeductStartFee(false);
		WriteBankerCardLog(m_bankerCard, 3);

        for(uint8 i=0;i<GAME_LAND_PLAYER;++i) {
            WriteOutCardLog(i, m_handCardData[i], m_handCardCount[i]);
        }
        OnSubBankerInfo();
        OnSetRobotThinkTime();
        return true;
    }
    m_coolLogic.beginCooling(GetCallScoreTime());
    OnSetRobotThinkTime();
    return true;
}
void CGameLandTable::OnCallScoreTimeOut()
{
    LOG_DEBUG("time out call score");
    if(m_curUser == INVALID_CHAIR)
	{
		LOG_DEBUG("cur_chairid_error - roomid:%d,tableid:%d,m_curUser:%d", GetRoomID(), GetTableID(), m_curUser);

        ReGameStart();

        return;
    }

	int bidNumber = 0;
	bool bOutGetCallScore = m_LordRobot[m_curUser].RbtOutGetCallScore(bidNumber);
	OnUserCallScore(m_curUser, bidNumber);

	LOG_DEBUG("land_robot_ai_log  超时叫分 - roomid:%d,tableid:%d,m_curUser:%d,uid:%d,bOutGetCallScore:%d，bidNumber:%d", GetRoomID(), GetTableID(), m_curUser, GetPlayerID(m_curUser), bOutGetCallScore, bidNumber);


    //OnUserCallScore(m_curUser,0);
}
// 用户出牌
bool CGameLandTable::OnUserOutCard(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
    //效验状态
    if(chairID != m_curUser) {
        LOG_ERROR("you can't oper outcard:%d",chairID);
        return false;
    }
    m_gameLogic.SortCardList(cardData,cardCount,ST_ORDER);
    //获取类型
    uint8 cardType = m_gameLogic.GetCardType(cardData,cardCount);

    //类型判断
    if(cardType == CT_ERROR)
    {
        LOG_ERROR("the card type is error:%d",cardType);
        CCommonLogic::LogCardString(cardData,cardCount);
        return false;
    }
    //出牌判断
    if(m_turnCardCount != 0)
    {
        //对比扑克
        if(m_gameLogic.CompareCard(m_turnCardData,cardData,m_turnCardCount,cardCount)==false)
        {
            //LOG_ERROR("the outcard is not handcard:%d",chairID);
            //CCommonLogic::LogCardString(cardData,cardCount);
            //CCommonLogic::LogCardString(m_turnCardData,m_turnCardCount);

			string strCard;
			strCard.clear();
			for (int i = 0; i < cardCount; ++i)
			{
				strCard += CStringUtility::FormatToString("0x%02X ", cardData[i]);
			}
			string strTurnCard;
			strTurnCard.clear();
			for (int i = 0; i < m_turnCardCount; ++i)
			{
				strTurnCard += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
			}
			LOG_DEBUG("uid:%d,cardCount:%d,m_turnCardCount:%d,cardType:%d,strCard:%s - %s", GetPlayerID(chairID), cardCount, m_turnCardCount, cardType, strCard.c_str(), strTurnCard.c_str());


            return false;
        }
    }

	string strOutCards;
	for (unsigned int i = 0; i < cardCount; ++i)
	{
		strOutCards += CStringUtility::FormatToString("0x%02X ", cardData[i]);
	}

	string strHandCards;
	for (BYTE i = 0; i < m_handCardCount[chairID]; ++i)
	{
		strHandCards += CStringUtility::FormatToString("0x%02X ", m_handCardData[chairID][i]);
	}

	LOG_DEBUG("1 roomid:%d,tableid:%d,uid:%d,chairID:%d,robot:%d,count:%d,card:%s", GetRoomID(),GetTableID(), GetPlayerID(chairID), chairID,ChairIsRobot(chairID), cardCount, strOutCards.c_str());
	LOG_DEBUG("2 roomid:%d,tableid:%d,uid:%d,chairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, ChairIsRobot(chairID), m_handCardCount[chairID], strHandCards.c_str());


    //删除扑克
    if(m_gameLogic.RemoveCardList(cardData,cardCount,m_handCardData[chairID],m_handCardCount[chairID])==false)
    {
        //LOG_ERROR("removecard error:%d",chairID);

		LOG_DEBUG("removecard_error - roomid:%d,tableid:%d,uid:%d,chairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, ChairIsRobot(chairID), cardCount, strOutCards.c_str());
		LOG_DEBUG("removecard_error - roomid:%d,tableid:%d,uid:%d,chairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, ChairIsRobot(chairID), m_handCardCount[chairID], strHandCards.c_str());

		
		WriteErrorOutCardLog(chairID, cardData, cardCount);

        return false;
    }


	string strCard;
	strCard.clear();
	for (int i = 0; i < cardCount; ++i)
	{
		strCard += CStringUtility::FormatToString("0x%02X ", cardData[i]);
	}
	string strTurnCard;
	strTurnCard.clear();
	for (int i = 0; i < m_turnCardCount; ++i)
	{
		strTurnCard += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
	}
	LOG_DEBUG("1 uid:%d,m_turnCardCount:%d,cardType:%d,strCard:%s - %s", GetPlayerID(chairID), m_turnCardCount, cardType, strCard.c_str(), strTurnCard.c_str());


    //出牌变量
    m_outCardCount[chairID]++;

    //设置变量
    m_turnCardCount = cardCount;
    m_handCardCount[chairID] -= cardCount;
    memcpy(m_turnCardData,cardData,sizeof(uint8)*cardCount);

	strTurnCard.clear();
	for (int i = 0; i < m_turnCardCount; ++i)
	{
		strTurnCard += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
	}
	LOG_DEBUG("2 uid:%d,m_turnCardCount:%d,cardType:%d,strCard:%s - %s", GetPlayerID(chairID), m_turnCardCount, cardType, strCard.c_str(), strTurnCard.c_str());


	memcpy(m_outCardData + m_cbOutCardCount, cardData, sizeof(uint8)*cardCount);
	m_cbOutCardCount += cardCount;

    //炸弹判断
    if((cardType == CT_BOMB_CARD)||(cardType == CT_MISSILE_CARD))
    {
        m_bombCount++;
        m_eachBombCount[chairID]++;
    }

    //切换用户
    m_turnWiner = chairID;
    if(m_handCardCount[chairID]!=0)
    {
        m_curUser = (m_curUser+1)%GAME_LAND_PLAYER;
    }else{
        m_curUser = INVALID_CHAIR;
    }

	LOG_DEBUG("cur_out_card_info - uid:%d,chairID:%d,m_curUser:%d,bankerUid:%d,m_bankerUser:%d", GetPlayerID(chairID), chairID, m_curUser, GetPlayerID(m_bankerUser), m_bankerUser);

    // 发送数据
    net::msg_land_out_card_rep outCard;
    outCard.set_out_card_user(chairID);
    outCard.set_cur_user(m_curUser);
    for(uint8 i=0;i<m_turnCardCount;++i){
        outCard.add_card_data(m_turnCardData[i]);
    }
    //SendMsgToAll(&outCard,net::S2C_MSG_LAND_OUT_CARD);

	//去掉手牌

	for (int i = 0; i < GAME_LAND_PLAYER; i++)
	{
		if (i != chairID)
		{
			for (uint8 j = 0; j<cardCount; ++j)
			{
				BYTE cbIndex = m_gameLogic.GetCardIndex(cardData[j]);
				if (m_cbMemCardMac[i][cbIndex]>0)
				{
					m_cbMemCardMac[i][cbIndex]--;
				}				
			}
		}
	}

	for (int i = 0; i < GAME_LAND_PLAYER; i++)
	{
		outCard.clear_mem_card_mac();
		for (int m = 1; m < MAX_SIG_CARD_COUNT; m++)
		{
			outCard.add_mem_card_mac(m_cbMemCardMac[i][m]);
		}
		SendMsgToClient(i, &outCard, net::S2C_MSG_LAND_OUT_CARD);
	}

    // 写入Log
    WriteOutCardLog(chairID,cardData,cardCount);


    //结束判断
    if(m_curUser == INVALID_CHAIR){
        OnGameEnd(chairID,GER_NORMAL);
    }else{
        m_coolLogic.beginCooling(GetOutCardTime());
        OnSetRobotThinkTime(true);
    }
    PushCardToPool(cardData,cardCount);
    
	//LOG_DEBUG("cur_out_card_end 1 - roomid:%d,tableid:%d,uid:%d,chairID:%d,m_curUser:%d,bankerUid:%d,m_bankerUser:%d", GetRoomID(), GetTableID(), GetPlayerID(chairID), chairID, m_curUser, GetPlayerID(m_bankerUser), m_bankerUser);

    OnSubOutCard(chairID);

	//LOG_DEBUG("cur_out_card_end 2 - roomid:%d,tableid:%d,uid:%d,chairID:%d,m_curUser:%d,bankerUid:%d,m_bankerUser:%d", GetRoomID(),GetTableID(),GetPlayerID(chairID), chairID, m_curUser, GetPlayerID(m_bankerUser), m_bankerUser);

    return true;
}
void    CGameLandTable::OnOutCardTimeOut()
{
	LOG_DEBUG("cur_chairid_auto - roomid:%d,tableid:%d,m_curUser:%d,uid:%d", GetRoomID(), GetTableID(), m_curUser,GetPlayerID(m_curUser));

	if (m_curUser < GAME_LAND_PLAYER)
	{
		//if (m_vecPlayers[m_curUser].overTimes == 2)
		{// 超时2次托管
			m_vecPlayers[m_curUser].autoState = 1;
			SendReadyStateToClient();
		}
		OnUserAutoCard();

		//LOG_DEBUG("outcard time out");
		//m_vecPlayers[m_curUser].overTimes++;
		//if (m_turnCardCount == 0)// 自动出牌
		//{
		//	uint8 outCard[MAX_LAND_COUNT];
		//	memset(outCard, 0, sizeof(outCard));
		//	outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];// 出最小一张
		//	OnUserOutCard(m_curUser, outCard, 1);
		//}
		//else {
		//	OnUserPassCard(m_curUser);
		//}
	}
	else
	{
		LOG_DEBUG("cur_chairid_error - roomid:%d,tableid:%d,m_curUser:%d", GetRoomID(), GetTableID(), m_curUser);
	}
}



// 托管出牌
void    CGameLandTable::OnUserAutoCard()
{
	WORD wMeChairID = m_curUser;
	uint8 outcards[MAX_LAND_COUNT] = { 0 };
	int  count = 0;

	string strHandCardsEx;
	for (BYTE i = 0; i < m_handCardCount[wMeChairID]; ++i)
	{
		strHandCardsEx += CStringUtility::FormatToString("0x%02X ", m_handCardData[wMeChairID][i]);
	}
	string strTurnCardEx;
	strTurnCardEx.clear();
	for (int i = 0; i < m_turnCardCount; ++i)
	{
		strTurnCardEx += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
	}
	if (m_handCardCount[wMeChairID] == 1)
	{
		bool bOutCardRet = false;
		bool bPassCardRet = false;

		outcards[0] = m_handCardData[wMeChairID][0];
		count = 1;

		bOutCardRet = OnUserOutCard(m_curUser, (uint8*)outcards, count);
		if (bOutCardRet == false)
		{
			bPassCardRet = OnUserPassCard(m_curUser);
		}

		LOG_DEBUG("最后一张牌 last_a_card_out- roomid:%d,tableid:%d,wChairID:%d,uid:%d, bOutCardRet:%d, bPassCardRet:%d,m_turnCardCount:%d,m_handCardCount:%d,strHandCardsEx:%s,strTurnCardEx:%s",
			GetRoomID(), GetTableID(), wMeChairID, GetPlayerID(wMeChairID), bOutCardRet, bPassCardRet, m_turnCardCount, m_handCardCount[wMeChairID], strHandCardsEx.c_str(), strTurnCardEx.c_str());

		return;
	}


 //   if(m_turnCardCount == 0)// 自动出牌
 //   {
	//	WORD wMeChairID = m_curUser;

	//	uint8 outcards[MAX_LAND_COUNT] = { 0 };
	//	int  count = 0;
	//	std::vector<int> vecCards;
	//	bool bOutGetTakeOutCard = m_LordRobot[wMeChairID].RbtOutGetTakeOutCard(vecCards);
	//	RobotCardCheck(outcards, vecCards);
	//	if (vecCards.size() >0)
	//	{
	//		OnUserOutCard(m_curUser, outcards, vecCards.size());
	//	}
	//	else
	//	{
	//		uint8 outCard[MAX_LAND_COUNT];
	//		memset(outCard,0,sizeof(outCard));
	//		outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser]-1];// 出最小一张
	//		OnUserOutCard(m_curUser, outCard, 1);
	//	}
 //   }
	//else
	//{
        //搜索扑克
        //tagOutCardResult OutCardResult;
        //if(m_gameLogic.SearchOutCard(m_handCardData[m_curUser],m_handCardCount[m_curUser],m_turnCardData,m_turnCardCount,OutCardResult))
        //{
        //    OnUserOutCard(m_curUser,OutCardResult.cbResultCard,OutCardResult.cbCardCount);
        //}
		//else
		//{
        //    OnUserPassCard(m_curUser);
        //}

		//WORD wMeChairID = m_curUser;
		//uint8 outcards[MAX_LAND_COUNT] = { 0 };
		//int  count = 0;
		std::vector<int> vecCards;
		bool bOutGetTakeOutCard = m_LordRobot[wMeChairID].RbtOutGetTakeOutCard(vecCards);
		RobotCardCheck(outcards, vecCards);
		count = (int)vecCards.size();

		string strCard;
		for (unsigned int i = 0; i < vecCards.size(); ++i)
		{
			strCard += CStringUtility::FormatToString("%d ", vecCards[i]);
		}
		LOG_DEBUG("land_robot_ai_log 托管出牌 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bOutGetTakeOutCard:%d,strCard:%s", GetRoomID(), GetTableID(), wMeChairID, GetPlayerID(wMeChairID), bOutGetTakeOutCard, strCard.c_str());
		string strOutCards;
		for (unsigned int i = 0; i < vecCards.size(); ++i)
		{
			strOutCards += CStringUtility::FormatToString("0x%02X ", outcards[i]);
		}
		string strHandCards;
		for (BYTE i = 0; i < m_handCardCount[wMeChairID]; ++i)
		{
			strHandCards += CStringUtility::FormatToString("0x%02X ", m_handCardData[wMeChairID][i]);
		}
		string strTurnCard;
		strTurnCard.clear();
		for (int i = 0; i < m_turnCardCount; ++i)
		{
			strTurnCard += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
		}
		LOG_DEBUG("1 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), vecCards.size(), strCard.c_str());
		LOG_DEBUG("2 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), vecCards.size(), strOutCards.c_str());
		LOG_DEBUG("3 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), m_handCardCount[wMeChairID], strHandCards.c_str());
		LOG_DEBUG("4 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,m_turnWiner:%d - %d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), m_turnWiner, GetPlayerID(m_turnWiner), m_turnCardCount, strTurnCard.c_str());

		if (count == 0)
		{
			LOG_DEBUG("机器人不出 robot_not_out_card - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,m_turnCardCount:%d",
				GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, m_turnCardCount);
			if (m_turnCardCount == 0)
			{
				uint8 outCard[MAX_LAND_COUNT];
				memset(outCard, 0, sizeof(outCard));
				outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];// 出最小一张
				OnUserOutCard(m_curUser, outCard, 1);

				LOG_DEBUG("出最小一张 out_card_faild - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,m_turnCardCount:%d,outCard:0x%02X",
					GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, m_turnCardCount, outCard[0]);
			}
			else
			{
				//放弃出牌
				OnUserPassCard(m_curUser);
				LOG_DEBUG("放弃出牌 robot_not_out_card - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,m_turnCardCount:%d",
					GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, m_turnCardCount);
			}

		}
		else
		{
			//char outcards2[MAX_LAND_COUNT];
			//m_ddzAIRobot[wMeChairID].AICards2SelfCards_1(count,outcards,outcards2);
			if (OnUserOutCard(m_curUser, (uint8*)outcards, count) == false)
			{
				LOG_DEBUG("出牌失败 out_card_faild - roomid:%d,tableid:%d,uid:%d,m_curUser:%d", GetRoomID(), GetTableID(), GetPlayerID(m_curUser), m_curUser);
				
				if (m_turnCardCount == 0)
				{
					uint8 outCard[MAX_LAND_COUNT];
					memset(outCard, 0, sizeof(outCard));
					outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];// 出最小一张
					OnUserOutCard(m_curUser, outCard, 1);

					LOG_DEBUG("出最小一张 out_card_faild - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,m_turnCardCount:%d,outCard:0x%02X",
						GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, m_turnCardCount, outCard[0]);
				}
				else
				{
					//放弃出牌
					OnUserPassCard(m_curUser);
					LOG_DEBUG("放弃出牌 robot_not_out_card - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,m_turnCardCount:%d",
						GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, m_turnCardCount);
				}

				//OnUserPassCard(m_curUser);
			}
		}
    //}
}
// 出牌时间跟叫地主时间
uint32  CGameLandTable::GetCallScoreTime()
{
    return 15*1000;
}
uint32  CGameLandTable::GetOutCardTime()
{
    if(m_pHostRoom->GetDeal() == net::ROOM_DEAL_TYPE_SOLO){
        return 20*1000;
    }else{
        return 20*1000;
    }
}
// 发送场景信息(断线重连)
void    CGameLandTable::SendGameScene(CGamePlayer* pPlayer)
{
    LOG_DEBUG("send game scene:%d", pPlayer->GetUID());
    uint16 chairID = GetChairID(pPlayer);
    if(GetGameState() == net::TABLE_STATE_FREE) {
        return;
    }else{
        net::msg_land_game_info_rep msg;
        msg.set_bomb_count(m_bombCount);
        msg.set_banker_user(m_bankerUser);
        msg.set_cur_user(m_curUser);
        msg.set_banker_score(m_callScore[m_bankerUser]);
        msg.set_turn_winer(m_turnWiner);
        msg.set_first_user(m_firstUser);
        for(uint8 i=0;i<m_turnCardCount;++i){
            msg.add_turn_card_data(m_turnCardData[i]);
        }
        for(uint8 i=0;i<getArrayLen(m_bankerCard);++i){
            msg.add_banker_card(m_bankerCard[i]);
        }
        for(uint8 i=0;i<GAME_LAND_PLAYER;++i){
            msg.add_hand_card_count(m_handCardCount[i]);
            msg.add_call_score(m_callScore[i]);
        }
        for(uint8 i=0;i<m_handCardCount[chairID];++i){
            msg.add_hand_card_data(m_handCardData[chairID][i]);
        }
		//for (uint8 i = 0; i<m_cbOutCardCount; ++i) {
		//	bool bAddOutData = true;
		//	for (uint8 j = 0; j<getArrayLen(m_bankerCard); ++j)
		//	{
		//		if (m_bankerCard[j] == m_outCardData[i])
		//		{
		//			bAddOutData = false;
		//		}
		//	}
		//	if (bAddOutData)
		//	{
		//		msg.add_out_card_data(m_outCardData[i]);
		//	}
		//}
		
		for (int m = 1; m < MAX_SIG_CARD_COUNT; m++)
		{
			msg.add_mem_card_mac(m_cbMemCardMac[chairID][m]);
		}

        msg.set_game_state(GetGameState());
        msg.set_wait_time(m_coolLogic.getCoolTick());
        
        SendMsgToClient(chairID,&msg,net::S2C_MSG_LAND_GAME_INFO);
    }
    SendReadyStateToClient();
}
void    CGameLandTable::CalcPlayerInfo(CGamePlayer* pPlayer,uint16 chairID,uint8 win,uint8 spring,uint8 land,int64 winScore)
{
    LOG_DEBUG("report game to lobby:%d--%d",pPlayer->GetUID(),win);
    if(pPlayer == NULL)
        return;

    net::msg_report_game_result msg;
    msg.set_uid(pPlayer->GetUID());
    msg.set_win_score(winScore);
    msg.set_consume(GetConsumeType());
    msg.set_game_type(m_pHostRoom->GetGameType());
	msg.set_welcount(pPlayer->GetWelCount());
	msg.set_weltime(pPlayer->GetWelTime());

    net::land_game_result* pland = msg.mutable_land();
    pland->set_is_land(land);
    pland->set_deal(m_conf.deal);
    pland->set_enter_min(m_conf.enterMin);
    pland->set_basescore(m_conf.baseScore);
    pland->set_press_count(m_pressCount[chairID]);
    pland->set_bankrupt_count(0);
    pland->set_is_spring(spring);
    
    if(win==1){
        uint8 bankrupt = 0;
        for(uint16 i=0;i<GAME_LAND_PLAYER;++i){
            bankrupt += m_bankrupts[i];
        }
        pland->set_bankrupt_count(bankrupt);
    }

    pPlayer->SendMsgToClient(&msg,net::S2L_MSG_REPORT_GAME_RESULT);

    // 修改斗地主数据
    bool isCoin = (GetConsumeType() == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    int32 lose = (win==1) ? 0 : 1;
    pPlayer->ChangeLandValue(isCoin,win,lose,land,spring,winScore);
    uint8 winType = win + spring;
    WriteWinType(chairID,winType);
    CDBMysqlMgr::Instance().ChangeLandValue(pPlayer->GetUID(),isCoin,win,lose,land,spring,pPlayer->GetGameMaxScore(m_pHostRoom->GetGameType(),isCoin));
}
// 游戏重新开始
void    CGameLandTable::ReGameStart()
{


    LOG_DEBUG("restart game");
    ResetGameData();
    SetGameState(TABLE_STATE_FREE);
    ResetPlayerReady();
    SendSeatInfoToClient();
    OnGameStart();
}
// 重置游戏数据
void    CGameLandTable::ResetGameData()
{
    //游戏变量
    m_bombCount    = 0;
    m_bankerUser   = INVALID_CHAIR;
    m_curUser      = INVALID_CHAIR;
	m_evaluateUser = INVALID_CHAIR;
    memset(m_outCardCount,0,sizeof(m_outCardCount));
    memset(m_eachBombCount,0,sizeof(m_eachBombCount));
    //叫分信息
    memset(m_callScore,0,sizeof(m_callScore));
    m_curCallScore = 0;

    //出牌信息
    m_turnCardCount = 0;
    m_turnWiner     = INVALID_CHAIR;
    memset(m_turnCardData,0,sizeof(m_turnCardData));

	m_cbOutCardCount = 0;
	memset(m_outCardData, 0, sizeof(m_outCardData));

    //扑克信息
    memset(m_bankerCard,0,sizeof(m_bankerCard));
    memset(m_handCardData,0,sizeof(m_handCardData));
    memset(m_handCardCount,0,sizeof(m_handCardCount));

    memset(m_pressCount,0,sizeof(m_pressCount));     // 压制次数
    memset(m_bankrupts,0,sizeof(m_bankrupts));      // 是否破产
    memset(m_sendHand,0,sizeof(m_sendHand));

}
// 添加防黑名单
void    CGameLandTable::AddBlockers()
{
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer == NULL)
            continue;
        pPlayer->ClearBlocker();
        pPlayer->ClearBlockerIP();
        for(uint32 j=0;j<m_vecPlayers.size();++j)
        {
            CGamePlayer* pTmp = m_vecPlayers[j].pPlayer;
            if(pTmp != NULL)
			{
                pPlayer->AddBlocker(pTmp->GetUID());
                pPlayer->AddBlockerIP(pTmp->GetIP());
            }
        }
    }
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameLandTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	DealSIsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot);
}

// 重置扑克
void    CGameLandTable::ReInitPoker()
{
    m_gameLogic.RandCardList(m_randCard,getArrayLen(m_randCard));
    m_gameLogic.ShuffleCard(m_randCard,getArrayLen(m_randCard));
}
// 洗牌
void    CGameLandTable::ShuffleTableCard(uint8 dealType)
{
    if(m_outCards.size() == FULL_POKER_COUNT)
    {
        LOG_DEBUG("imple shuffle card");
        uint8 shuffCount = dealType < getArrayLen(s_ShuffCount) ? s_ShuffCount[dealType] : 10;
        for(uint8 i=0;i<shuffCount;++i)
        {
            uint8 pos = g_RandGen.RandRange(10,30);
            vector<uint8> tmpCards;
            for(uint8 j=0;j<pos;++j)
            {
                tmpCards.push_back(m_outCards.front());
                m_outCards.erase(m_outCards.begin());
            }
            pos = g_RandGen.RandUInt()%m_outCards.size();
            m_outCards.insert(m_outCards.begin()+pos,tmpCards.begin(),tmpCards.end());
        }
        for(uint8 i=0;i<m_outCards.size();++i){
            m_randCard[i] = m_outCards[i];
        }
        if(dealType == net::ROOM_DEAL_TYPE_SOLO){
            m_gameLogic.ShuffleCard(m_randCard,getArrayLen(m_randCard));
        }
    }else{
        ReInitPoker();
    }
    m_outCards.clear();
}
// 发牌并返回当前玩家位置
uint16  CGameLandTable::DispatchUserCard(uint16 startUser,uint32 cardIndex)
{
    uint8 dealType = GetDeal();
    switch(dealType)
    {
    case net::ROOM_DEAL_TYPE_SOLO:
        {
            return DispatchUserCardByDeal(s_DealSolo,getArrayLen(s_DealSolo),startUser,cardIndex);
        }break;
    case net::ROOM_DEAL_TYPE_THREE:
        {
            return DispatchUserCardByDeal(s_DealThree,getArrayLen(s_DealThree),startUser,cardIndex);
        }break;
    case net::ROOM_DEAL_TYPE_SEVEN:
        {
            return DispatchUserCardByDeal(s_DealSeven,getArrayLen(s_DealSeven),startUser,cardIndex);
        }break;
    default:
        break;
    }
    return DispatchUserCardByDeal(s_DealSolo,getArrayLen(s_DealSolo),startUser,cardIndex);
}
uint16  CGameLandTable::DispatchUserCardByDeal(const uint8 dealArry[],uint8 dealCount,uint16 startUser,uint32 cardIndex)
{
    uint8   cardpos = 0;
    uint16  curUser = 0;
    memset(m_handCardCount,0,sizeof(m_handCardCount));
    memset(m_handCardData,0,sizeof(m_handCardData));
    for(uint8 m=0;m<dealCount;++m)
    {
        uint8 cardCount = dealArry[m];
        for(uint16 i = 0; i < GAME_LAND_PLAYER; ++i)
        {
            uint16 userIndex = (startUser+i)%GAME_LAND_PLAYER;
            for(uint8 j=0;j<cardCount;++j)
            {
                m_handCardData[userIndex][m_handCardCount[userIndex]] = m_randCard[cardpos];
                m_handCardCount[userIndex]++;
                if(cardpos == cardIndex){
                    curUser = userIndex;
                }
                cardpos++;
            }
        }
    }
    return curUser;
}
// 放入出牌池中
void    CGameLandTable::PushCardToPool(uint8 cardData[],uint8 cardCount)
{
    for(uint8 i=0;i<cardCount;++i)
    {
        m_outCards.push_back(cardData[i]);
    }
}
// 检测准备超时的从新排队
void    CGameLandTable::CheckReadyTimeOut()
{
    if(GetTableType() == emTABLE_TYPE_PLAYER)
        return;

    for(uint32 i = 0; i < m_vecPlayers.size();++i){
        stSeat& refSeat = m_vecPlayers[i];
        if(refSeat.pPlayer != NULL && refSeat.readyState == 1
        && refSeat.readyTime + s_ReadyTimeOut < getSysTime())
        {
             CGamePlayer* pPlayer = refSeat.pPlayer;
             if(!pPlayer->IsRobot()){
				 LOG_DEBUG("play_leave - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());
                 this->LeaveTable(pPlayer);             
                 pPlayer->SetAutoReady(true);
                 m_pHostRoom->JoinMarry(pPlayer,GetTableID());
             }else{
				 LOG_DEBUG("play_leave - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());

                 this->LeaveTable(pPlayer);
             }
             LOG_DEBUG("time out and change the table:table:%d-->%d",GetTableID(),pPlayer->GetUID());
        }
    }

}
// 写入出牌log
void    CGameLandTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);

}

void    CGameLandTable::WriteErrorOutCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount)
{
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["e"] = 1;
	for (uint32 i = 0; i<cardCount; ++i) {
		logValue["c"].append(cardData[i]);
	}
	m_operLog["card"].append(logValue);

}

void    CGameLandTable::WriteWinType(uint16 chairID,uint8 winType)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["wintype"] = winType;
    m_operLog["wintype"].append(logValue);
}
void    CGameLandTable::WriteBankerCardLog(uint8 cardData[], uint8 cardCount)
{
	Json::Value logValue;
	for (uint32 i = 0; i<cardCount; ++i) {
		logValue["c"].append(cardData[i]);
	}
	m_operLog["bankercard"].append(logValue);

}

void CGameLandTable::WriteRobotID()
{

	//for (WORD wChairID = 0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	//{
	//	if (ChairIsRobot(wChairID) == false)
	//	{
	//		continue;
	//	}
	//	Json::Value logValue;
	//	//logValue["batch_id"] = m_DDZRobotHttp[wChairID].GetRobotID().c_str();
	//	logValue["chairID"] = wChairID;
	//	logValue["uid"] = GetPlayerID(wChairID);
	//	m_operLog["rinfo"].append(logValue);
	//}
	m_operLog["eu"] = m_evaluateUser;
}


void    CGameLandTable::CheckAddRobot()
{
    if(m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
        return;
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
            for(uint32 j=0;j<m_vecPlayers.size();++j)
            {
                if (m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_COIN && pPlayer->IsExistBlocker(m_vecPlayers[j].uid))
                {
                    RenewFastJoinTable(pPlayer);
                    return;
                }
            }
            if(m_vecPlayers[i].readyState == 0){
                PlayerReady(pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(1500,5000));
                return;
            }else{
                if((getSysTime() - m_vecPlayers[i].readyTime) > 30)
				{
					LOG_DEBUG("play_leave - roomid:%d,tableid:%d,uid:%d",GetRoomID(),GetTableID(), pPlayer->GetUID());
                    LeaveTable(pPlayer);
                    m_coolRobot.beginCooling(g_RandGen.RandRange(1000,5000));
                    return;
                }
            }
        }
    }
	string strPlayerUid;
	for (uint32 i = 0; i < m_vecPlayers.size(); ++i)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL)
		{
			strPlayerUid += CStringUtility::FormatToString("i:%02d,uid:%d ",i, pPlayer->GetUID());
		}
	}
    if(GetChairPlayerNum() >= 1 && GetChairPlayerNum() < 3)
    {
        for(uint32 i=0;i<m_vecPlayers.size();++i)
        {
            CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
            if(pPlayer != NULL && !pPlayer->IsRobot())// 有玩家存在
            {
				LOG_DEBUG("get_a_robot - roomid:%d,tableid:%d,uid:%d,strPlayerUid:%s", GetRoomID(), GetTableID(), pPlayer->GetUID(), strPlayerUid.c_str());

                CRobotMgr::Instance().RequestOneRobot(this, pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(1500,3000));
                return;
            }
        }
    }

}

void CGameLandTable::RobotCardJudge(BYTE handCardData[], int count, std::vector<int> & argHandCards)
{
	argHandCards.clear();
	for (int i = 0; i < count; i++)
	{
		int iCardNum = handCardData[i];
		if (iCardNum == 0)
		{
			continue;
		}
		int val = -1;
		switch (iCardNum)
		{
		case 0x01: {val = 11; }break;
		case 0x11: {val = 24; }break;
		case 0x21: {val = 37; }break;
		case 0x31: {val = 50; }break;

		case 0x02: {val = 12; }break;
		case 0x12: {val = 25; }break;
		case 0x22: {val = 38; }break;
		case 0x32: {val = 51; }break;

		case 0x03: {val =  0; }break;
		case 0x13: {val = 13; }break;
		case 0x23: {val = 26; }break;
		case 0x33: {val = 39; }break;

		case 0x04: {val =  1; }break;
		case 0x14: {val = 14; }break;
		case 0x24: {val = 27; }break;
		case 0x34: {val = 40; }break;

		case 0x05: {val =  2; }break;
		case 0x15: {val = 15; }break;
		case 0x25: {val = 28; }break;
		case 0x35: {val = 41; }break;

		case 0x06: {val =  3; }break;
		case 0x16: {val = 16; }break;
		case 0x26: {val = 29; }break;
		case 0x36: {val = 42; }break;

		case 0x07: {val =  4; }break;
		case 0x17: {val = 17; }break;
		case 0x27: {val = 30; }break;
		case 0x37: {val = 43; }break;

		case 0x08: {val =  5; }break;
		case 0x18: {val = 18; }break;
		case 0x28: {val = 31; }break;
		case 0x38: {val = 44; }break;

		case 0x09: {val =  6; }break;
		case 0x19: {val = 19; }break;
		case 0x29: {val = 32; }break;
		case 0x39: {val = 45; }break;

		case 0x0A: {val =  7; }break;
		case 0x1A: {val = 20; }break;
		case 0x2A: {val = 33; }break;
		case 0x3A: {val = 46; }break;

		case 0x0B: {val =  8; }break;
		case 0x1B: {val = 21; }break;
		case 0x2B: {val = 34; }break;
		case 0x3B: {val = 47; }break;

		case 0x0C: {val =  9; }break;
		case 0x1C: {val = 22; }break;
		case 0x2C: {val = 35; }break;
		case 0x3C: {val = 48; }break;

		case 0x0D: {val = 10; }break;
		case 0x1D: {val = 23; }break;
		case 0x2D: {val = 36; }break;
		case 0x3D: {val = 49; }break;

		case 0x4E: {val = 52; }break;
		case 0x4F: {val = 53; }break;
		default: {break; }
		}
		if (val == -1)
		{
			continue;
		}
		if (i >= MAX_LAND_COUNT)
		{
			break;
		}
		argHandCards.push_back(val);
	}
}

void CGameLandTable::RobotCardCheck(BYTE cbCardData[], std::vector<int> argHandCards)
{
	for (unsigned int i = 0; i < argHandCards.size(); i++)
	{
		int iCardNum = argHandCards[i];
		if (iCardNum < 0 || iCardNum>53)
		{
			continue;
		}
		int val = -1;
		switch (iCardNum)
		{
		case 11: {cbCardData[i] = 0x01; }break;
		case 24: {cbCardData[i] = 0x11; }break;
		case 37: {cbCardData[i] = 0x21; }break;
		case 50: {cbCardData[i] = 0x31; }break;

		case 12: {cbCardData[i] = 0x02; }break;
		case 25: {cbCardData[i] = 0x12; }break;
		case 38: {cbCardData[i] = 0x22; }break;
		case 51: {cbCardData[i] = 0x32; }break;

		case 0:  {cbCardData[i] = 0x03; }break;
		case 13: {cbCardData[i] = 0x13; }break;
		case 26: {cbCardData[i] = 0x23; }break;
		case 39: {cbCardData[i] = 0x33; }break;

		case  1: {cbCardData[i] = 0x04; }break;
		case 14: {cbCardData[i] = 0x14; }break;
		case 27: {cbCardData[i] = 0x24; }break;
		case 40: {cbCardData[i] = 0x34; }break;

		case  2: {cbCardData[i] = 0x05; }break;
		case 15: {cbCardData[i] = 0x15; }break;
		case 28: {cbCardData[i] = 0x25; }break;
		case 41: {cbCardData[i] = 0x35; }break;

		case  3: {cbCardData[i] = 0x06; }break;
		case 16: {cbCardData[i] = 0x16; }break;
		case 29: {cbCardData[i] = 0x26; }break;
		case 42: {cbCardData[i] = 0x36; }break;

		case  4: {cbCardData[i] = 0x07; }break;
		case 17: {cbCardData[i] = 0x17; }break;
		case 30: {cbCardData[i] = 0x27; }break;
		case 43: {cbCardData[i] = 0x37; }break;

		case  5: {cbCardData[i] = 0x08; }break;
		case 18: {cbCardData[i] = 0x18; }break;
		case 31: {cbCardData[i] = 0x28; }break;
		case 44: {cbCardData[i] = 0x38; }break;

		case  6: {cbCardData[i] = 0x09; }break;
		case 19: {cbCardData[i] = 0x19; }break;
		case 32: {cbCardData[i] = 0x29; }break;
		case 45: {cbCardData[i] = 0x39; }break;

		case  7: {cbCardData[i] = 0x0A; }break;
		case 20: {cbCardData[i] = 0x1A; }break;
		case 33: {cbCardData[i] = 0x2A; }break;
		case 46: {cbCardData[i] = 0x3A; }break;

		case  8: {cbCardData[i] = 0x0B; }break;
		case 21: {cbCardData[i] = 0x1B; }break;
		case 34: {cbCardData[i] = 0x2B; }break;
		case 47: {cbCardData[i] = 0x3B; }break;

		case  9: {cbCardData[i] = 0x0C; }break;
		case 22: {cbCardData[i] = 0x1C; }break;
		case 35: {cbCardData[i] = 0x2C; }break;
		case 48: {cbCardData[i] = 0x3C; }break;

		case 10: {cbCardData[i] = 0x0D; }break;
		case 23: {cbCardData[i] = 0x1D; }break;
		case 36: {cbCardData[i] = 0x2D; }break;
		case 49: {cbCardData[i] = 0x3D; }break;

		case 52: {cbCardData[i] = 0x4E; }break;
		case 53: {cbCardData[i] = 0x4F; }break;

		default: {break; }
		}
		if (i >= MAX_LAND_COUNT)
		{
			break;
		}
	}
}

bool    CGameLandTable::ChairIsRobot(uint16 chairID)
{
	CGamePlayer * pPlayer = GetPlayer(chairID);
	if (pPlayer != NULL)
	{
		return pPlayer->IsRobot();
	}
	return false;
}
// AI 函数
//游戏开始
bool    CGameLandTable::OnSubGameStart()
{
	std::vector<std::vector<int> > argAllHandCard;
	for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
	{
		std::vector<int> argHandCards;
		RobotCardJudge(m_handCardData[wChairID], MAX_LAND_COUNT, argHandCards);
		argAllHandCard.push_back(argHandCards);
	}
	if (argAllHandCard.size() != GAME_LAND_PLAYER)
	{
		return false;
	}

	for(WORD wChairID=0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	{
		m_gameLogic.SetUserCard(wChairID,m_handCardData[wChairID],NORMAL_COUNT);
        //叫牌扑克
	    m_gameLogic.SetLandScoreCardData(wChairID,m_handCardData[wChairID],MAX_LAND_COUNT);

        //设置百度机器人牌数据
        //m_ddzAIRobot[wChairID].Init();

        //m_ddzAIRobot[wChairID].setSelfPosition(wChairID);
        //char   handCardData[17];
        //m_ddzAIRobot[wChairID].SelfCards2AICards_1(17,(char*)m_handCardData[wChairID],handCardData);
        //m_ddzAIRobot[wChairID].setSelfCards(handCardData);

		//if (ChairIsRobot(wChairID) == false)
		//{
		//	continue;
		//}
		bool bResetData = m_LordRobot[wChairID].RbtResetData();
		bool bInSetLevel = false;
		if (ChairIsRobot(wChairID) == true)
		{
			bInSetLevel = m_LordRobot[wChairID].RbtInSetLevel(m_argRobotLevel);
		}
		else
		{
			bInSetLevel = m_LordRobot[wChairID].RbtInSetLevel(TRUST_LEVEL);
		}

		int argSeat = wChairID;

		bool bInInitCard = m_LordRobot[wChairID].RbtInInitCard(argSeat, argAllHandCard[wChairID]);
		bool bInNtfCardInfo = m_LordRobot[wChairID].RbtInNtfCardInfo(argAllHandCard);
		//LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInSetLevel:%d,bInInitCard:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInSetLevel, bInInitCard);


		string strCard;
		for (unsigned int i = 0; i < argAllHandCard[wChairID].size(); ++i)
		{
			strCard += CStringUtility::FormatToString("%d ", argAllHandCard[wChairID][i]);
		}
		LOG_DEBUG("land_robot_ai_log 游戏开始 - roomid:%d,tableid:%d,wChairID:%d,m_argRobotLevel:%d,robot:%d,uid:%d,bResetData:%d,bInSetLevel:%d,bInInitCard:%d,bInNtfCardInfo:%d,argSeat:%d,strCard:%s", GetRoomID(), GetTableID(), wChairID, m_argRobotLevel, ChairIsRobot(wChairID), GetPlayerID(wChairID), bResetData, bInSetLevel, bInInitCard, bInNtfCardInfo, argSeat,strCard.c_str());

    }

    return true;
}


//庄家信息
bool    CGameLandTable::OnSubBankerInfo()
{
    //设置底牌
    memcpy(m_allCardCount,m_handCardCount,sizeof(m_allCardCount));
    memcpy(m_allCardData,m_handCardData,sizeof(m_allCardData));

    //for(WORD wChairID=0; wChairID<GAME_LAND_PLAYER; ++wChairID)
    //{
    //    //设置百度机器人牌数据
    //    m_ddzAIRobot[wChairID].setBankerPosition(m_bankerUser);
    //    char   bankerCard[3];
    //    m_ddzAIRobot[wChairID].SelfCards2AICards_1(3,(char*)m_bankerCard,bankerCard);
    //    m_ddzAIRobot[wChairID].setBottomCards(bankerCard);
    //}

	int argSeat = m_bankerUser;
	std::vector<int> argHandCards;
	RobotCardJudge(m_bankerCard, 3, argHandCards);

	for (WORD wChairID = 0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	{
		//if (ChairIsRobot(wChairID) == false)
		//{
		//	continue;
		//}
		bool bInSetLord = m_LordRobot[wChairID].RbtInSetLord(argSeat, argHandCards);

		string strCard;
		for (unsigned int i = 0; i < argHandCards.size(); ++i)
		{
			strCard += CStringUtility::FormatToString("%d ", argHandCards[i]);
		}
		LOG_DEBUG("land_robot_ai_log 设置地主 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInSetLord:%d,argSeat:%d,strCard:%s", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInSetLord, argSeat,strCard.c_str());

		//LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInSetLord:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInSetLord);

		//LOG_DEBUG("cur_out_card_info - uid:%d,chairID:%d,m_curUser:%d,bankerUid:%d,m_bankerUser:%d", GetPlayerID(wChairID), wChairID, m_curUser, GetPlayerID(m_bankerUser), m_bankerUser);
	}


    return true;
}
//用户出牌
bool    CGameLandTable::OnSubOutCard(uint16 chairID)
{
	//设置变量
    //char cards[20];
    //m_ddzAIRobot[0].SelfCards2AICards_1(m_turnCardCount,(char*)m_turnCardData,cards);

    ////设置百度机器人出牌
    //for(WORD wChairID=0; wChairID<GAME_LAND_PLAYER; ++wChairID)
    //{
    //    m_ddzAIRobot[wChairID].playerOutCards(chairID,m_turnCardCount,cards);
    //}
	//uint8   turnCardCount;
	uint8   turnCardData[MAX_LAND_COUNT] = {0};
	for (uint8 i = 0; i < m_turnCardCount; i++)
	{
		turnCardData[i] = m_turnCardData[i];
	}

	//LOG_DEBUG("1 roomid:%d,tableid:%d,chairID:%d,m_turnCardCount:%d,robot:%d,uid:%d", GetRoomID(), GetTableID(), chairID, m_turnCardCount, ChairIsRobot(chairID), GetPlayerID(chairID));

	int argSeat = chairID;
	std::vector<int> argHandCards;
	RobotCardJudge(turnCardData, MAX_LAND_COUNT, argHandCards);
	for (WORD wChairID = 0; wChairID<GAME_LAND_PLAYER; ++wChairID)
	{
		//LOG_DEBUG("roomid:%d,tableid:%d,wChairID:%d,robot:%d,uid:%d",GetRoomID(),GetTableID(), wChairID, ChairIsRobot(wChairID),GetPlayerID(wChairID));
		//if (ChairIsRobot(wChairID) == false)
		//{
		//	continue;
		//}
		bool bInTakeOutCard = m_LordRobot[wChairID].RbtInTakeOutCard(argSeat, argHandCards);

		//LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInTakeOutCard:%d", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInTakeOutCard);

		string strCard;
		for (unsigned int i = 0; i < argHandCards.size(); ++i)
		{
			strCard += CStringUtility::FormatToString("%d ", argHandCards[i]);
		}
		LOG_DEBUG("land_robot_ai_log 收到出牌 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bInTakeOutCard:%d,argSeat:%d,strCard:%s", GetRoomID(), GetTableID(), wChairID, GetPlayerID(wChairID), bInTakeOutCard, argSeat, strCard.c_str());
	}



	//LOG_DEBUG("2 roomid:%d,tableid:%d,chairID:%d,m_turnCardCount:%d,robot:%d,uid:%d", GetRoomID(), GetTableID(), chairID, m_turnCardCount, ChairIsRobot(chairID), GetPlayerID(chairID));

    return true;
}
//机器人叫分
bool    CGameLandTable::OnRobotCallScore()
{
    //百度机器人
	int bidNumber = 0;// m_ddzAIRobot[m_curUser].GetBidNumber();
	
	bool bOutGetCallScore = m_LordRobot[m_curUser].RbtOutGetCallScore(bidNumber);
	OnUserCallScore(m_curUser, bidNumber);

	LOG_DEBUG("land_robot_ai_log  机器叫分 - roomid:%d,tableid:%d,m_curUser:%d,uid:%d,bOutGetCallScore:%d，bidNumber:%d", GetRoomID(), GetTableID(), m_curUser, GetPlayerID(m_curUser), bOutGetCallScore, bidNumber);

	LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,m_curUser:%d,uid:%d,bOutGetCallScore:%d", GetRoomID(), GetTableID(), m_curUser, GetPlayerID(m_curUser), bOutGetCallScore);

 //   LOG_DEBUG("uid:%d,bidNumber:%d",GetPlayerID(m_curUser),bidNumber);
 //   

    return true;
}
//机器人出牌
bool    CGameLandTable::OnRobotOutCard()
{
    //百人机器人出牌
    WORD wMeChairID = m_curUser;
	int  count = 0;
	uint8 outcards[MAX_LAND_COUNT] = { 0 };

	string strHandCards;
	for (BYTE i = 0; i < m_handCardCount[wMeChairID]; ++i)
	{
		strHandCards += CStringUtility::FormatToString("0x%02X ", m_handCardData[wMeChairID][i]);
	}
	string strTurnCard;
	strTurnCard.clear();
	for (int i = 0; i < m_turnCardCount; ++i)
	{
		strTurnCard += CStringUtility::FormatToString("0x%02X ", m_turnCardData[i]);
	}
	if (m_handCardCount[wMeChairID] == 1)
	{
		bool bOutCardRet = false;
		bool bPassCardRet = false;

		outcards[0] = m_handCardData[wMeChairID][0];
		count = 1;

		bOutCardRet = OnUserOutCard(m_curUser, (uint8*)outcards, count);
		if (bOutCardRet == false)
		{
			bPassCardRet = OnUserPassCard(m_curUser);
		}

		LOG_DEBUG("最后一张牌 last_a_card_out- roomid:%d,tableid:%d,wChairID:%d,uid:%d, bOutCardRet:%d, bPassCardRet:%d,m_turnCardCount:%d,m_handCardCount:%d,strHandCards:%s,strTurnCard:%s", 
			GetRoomID(), GetTableID(), wMeChairID, GetPlayerID(wMeChairID), bOutCardRet, bPassCardRet, m_turnCardCount, m_handCardCount[wMeChairID], strHandCards.c_str(), strTurnCard.c_str());

		return true;
	}

    int  outCardType,outCardValue;
    //m_ddzAIRobot[wMeChairID].selfOutCards(count,outcards,outCardType,outCardValue);

	std::vector<int> vecCards;

	bool bOutGetTakeOutCard = m_LordRobot[wMeChairID].RbtOutGetTakeOutCard(vecCards);

	//LOG_DEBUG("land_robot_ai - roomid:%d,tableid:%d,wMeChairID:%d,uid:%d,bOutGetTakeOutCard:%d", GetRoomID(), GetTableID(), wMeChairID, GetPlayerID(wMeChairID), bOutGetTakeOutCard);

	RobotCardCheck(outcards, vecCards);

	string strCard;	
	for (unsigned int i = 0; i < vecCards.size(); ++i)
	{
		strCard += CStringUtility::FormatToString("%d ", vecCards[i]);
	}

	LOG_DEBUG("land_robot_ai_log 机器出牌 - roomid:%d,tableid:%d,wChairID:%d,uid:%d,bOutGetTakeOutCard:%d,strCard:%s", GetRoomID(), GetTableID(), wMeChairID, GetPlayerID(wMeChairID), bOutGetTakeOutCard, strCard.c_str());


	string strOutCards;
	for (unsigned int i = 0; i < vecCards.size(); ++i)
	{
		strOutCards += CStringUtility::FormatToString("0x%02X ", outcards[i]);
	}

	//string strHandCards;
	//for (BYTE i = 0; i < m_handCardCount[wMeChairID]; ++i)
	//{
	//	strHandCards += CStringUtility::FormatToString("0x%02X ", m_handCardData[wMeChairID][i]);
	//}



	LOG_DEBUG("1 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s",GetRoomID(),GetTableID(),GetPlayerID(wMeChairID), wMeChairID,ChairIsRobot(wMeChairID), vecCards.size(),strCard.c_str());
	LOG_DEBUG("2 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), vecCards.size(), strOutCards.c_str());
	LOG_DEBUG("3 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), m_handCardCount[wMeChairID], strHandCards.c_str());
	LOG_DEBUG("4 roomid:%d,tableid:%d,uid:%d,wMeChairID:%d,robot:%d,m_turnWiner:%d - %d,count:%d,card:%s", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID, ChairIsRobot(wMeChairID), m_turnWiner, GetPlayerID(m_turnWiner), m_turnCardCount, strTurnCard.c_str());

	count = (int)vecCards.size();
    if(count == 0)
    {
		LOG_DEBUG("机器人不出 robot_not_out_card - roomid:%d,tableid:%d,uid:%d,wMeChairID:%d", GetRoomID(), GetTableID(), GetPlayerID(wMeChairID), wMeChairID);

        //先出牌不能为空
        if(m_turnCardCount == 0)
        {
			LOG_DEBUG("先出牌不能为空 first_out_card_empty - roomid:%d,tableid:%d,uid:%d,m_curUser:%d", GetRoomID(), GetTableID(), GetPlayerID(m_curUser), m_curUser);

            OnUserAutoCard();
            return true;
        }
        //放弃出牌
        OnUserPassCard(m_curUser);
    }else{
        //char outcards2[MAX_LAND_COUNT];
        //m_ddzAIRobot[wMeChairID].AICards2SelfCards_1(count,outcards,outcards2);
        if(OnUserOutCard(m_curUser,(uint8*)outcards,count) == false)
        {
            LOG_DEBUG("出牌失败 out_card_faild - roomid:%d,tableid:%d,uid:%d,m_curUser:%d", GetRoomID(), GetTableID(), GetPlayerID(m_curUser),m_curUser);
            OnUserAutoCard();
        }
    }

    return true;
}
//是否需要重置思考时间
bool    CGameLandTable::ReCalcRobotThinkTime()
{
    if(m_coolRobot.getCoolTick() < 2000)
        return true;
    CGamePlayer* pPlayer = m_vecPlayers[m_curUser].pPlayer;
    if(pPlayer == NULL || !pPlayer->IsRobot()){
        return true;
    }            
    if(m_turnCardCount == 0)// 自动出牌
    {
        m_coolRobot.beginCooling(g_RandGen.RandRange(500,1900));
    }else{
        //搜索扑克
        tagOutCardResult OutCardResult;
        if(m_gameLogic.SearchOutCard(m_handCardData[m_curUser],m_handCardCount[m_curUser],m_turnCardData,m_turnCardCount,OutCardResult))
        {
            return true;
        }else{
            m_coolRobot.beginCooling(g_RandGen.RandRange(500,1500));
        }
    }
    return true;
}
//设置机器人思考时间
bool    CGameLandTable::OnSetRobotThinkTime(bool bRecalc)
{
    if(GetGameState() == TABLE_STATE_CALL){
        if(m_curUser == m_firstUser){
            uint32 tmp = 0;
            if(m_conf.deal == net::ROOM_DEAL_TYPE_SOLO){
                tmp = 11000;
            }else if(m_conf.deal == net::ROOM_DEAL_TYPE_THREE){
                tmp = 7000;
            }else{
                tmp = 6000;
            }
            m_coolRobot.beginCooling(g_RandGen.RandRange(tmp,tmp+2000));
        }else{
            m_coolRobot.beginCooling(g_RandGen.RandRange(2000,4000));
        }
    }else{
        if(g_RandGen.RandRatio(70,100)){
            m_coolRobot.beginCooling(g_RandGen.RandRange(1000,3000)); 
        }else{
            m_coolRobot.beginCooling(g_RandGen.RandRange(3000,5000)); 
        }
    }    
    if(bRecalc){
       ReCalcRobotThinkTime();
    }
    
    return true;
}

bool CGameLandTable::SetControlCardData()
{
	bool bControlPalyer = ProgressControlPalyer();
	if (bControlPalyer)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,bControlPalyer is true.", GetRoomID(), GetTableID());
		return true;
	}

	// 幸运值控制
	bool bIsLuckyControl = SetLuckyCtrl();
	if (bIsLuckyControl)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,bIsLuckyControl is true.", GetRoomID(), GetTableID());
		return true;
	}

	// 新注册玩家福利控制
	bool bIsNRWControl = NewRegisterWelfareCtrl();
	if (bIsNRWControl)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,bIsNRWControl is true.", GetRoomID(), GetTableID());
		return true;
	}

	bool bRobotBankerWinPro = g_RandGen.RandRatio(m_robotBankerWinPro, PRO_DENO_10000);
	int argSeat = GAME_LAND_PLAYER;
	bool argSeatRobot = false;
	if (bRobotBankerWinPro)
	{
		std::vector<std::vector<int> > argAllHandCard;
		for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
		{
			std::vector<int> argHandCards;
			RobotCardJudge(m_handCardData[wChairID], MAX_LAND_COUNT, argHandCards);
			argAllHandCard.push_back(argHandCards);
		}

		std::vector<int> argHandCards;
		RobotCardJudge(m_bankerCard, 3, argHandCards);
		argAllHandCard.push_back(argHandCards);
		argSeat = evaluate_cards(argAllHandCard);

		if (argSeat < GAME_LAND_PLAYER)
		{
			argSeatRobot = ChairIsRobot(argSeat);
			if (argSeatRobot == false)
			{
				// 换牌
				for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
				{
					if (ChairIsRobot(wChairID) == false)
					{
						continue;
					}
					if (argSeat == wChairID)
					{
						continue;
					}
					uint8 tmp[MAX_LAND_COUNT];
					memcpy(tmp, m_handCardData[wChairID], MAX_LAND_COUNT);
					memcpy(m_handCardData[wChairID], m_handCardData[argSeat], MAX_LAND_COUNT);
					memcpy(m_handCardData[argSeat], tmp, MAX_LAND_COUNT);
					m_evaluateUser = wChairID;
					break;
				}
			}
			else
			{
				m_evaluateUser = argSeat;
			}
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d %d %d,m_evaluateUser:%d,argSeatRobot:%d",
				GetRoomID(),GetTableID(),GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), m_evaluateUser, argSeatRobot);
			return true;
		}
	}

	// 库存控制 add by har
	if (SetStockWinLose()) {
		LOG_DEBUG("roomid:%d,tableid:%d, SetStockWinLose is true.", GetRoomID(), GetTableID());
		return true;
	}

	return false;
}

bool	CGameLandTable::ProgressControlPalyer()
{
	bool bIsFalgControlMulti = false;
	vector<tagControlMultiPalyer> vecControlMultiPlayerLost;
	vector<tagControlMultiPalyer> vecControlMultiPlayerWin;
	for (uint32 uIndex = 0; uIndex < GAME_LAND_PLAYER; uIndex++)
	{
		CGamePlayer *pPlayer = GetPlayer(uIndex);
		if (pPlayer == NULL)
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

	if (bIsFalgControlMulti)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,vecControlMultiPlayer_size:%d %d,uLostPlayerUid:%d,uWinPlayerUid:%d,bIsFalgControlMulti:%d,m_evaluateUser:%d",
			GetRoomID(), GetTableID(), vecControlMultiPlayerLost.size(), vecControlMultiPlayerWin.size(), uLostPlayerUid, uWinPlayerUid, bIsFalgControlMulti, m_evaluateUser);
		return bIsFalgControlMulti;
	}


	bool bIsFalgControl = false;

	bool bIsHaveControlPlayer = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (WORD i = 0; i<GAME_LAND_PLAYER; i++)
		{
			CGamePlayer *pPlayer = GetPlayer(i);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (pPlayer->GetUID() == control_uid && IsReady(pPlayer))
			{
				bIsHaveControlPlayer = true;
				break;
			}
		}

		if (bIsHaveControlPlayer && game_count>0 && control_type != GAME_CONTROL_CANCEL)
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


	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d %d %d,control_uid:%d,game_count:%d,control_type:%d,bIsHaveControlPlayer:%d,m_evaluateUser:%d",
		GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), control_uid, game_count, control_type, bIsHaveControlPlayer, m_evaluateUser);


	return bIsFalgControl;

}

bool	CGameLandTable::SetControlPalyerWin(uint32 control_uid)
{
	std::vector<std::vector<int> > argAllHandCard;
	for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
	{
		std::vector<int> argHandCards;
		RobotCardJudge(m_handCardData[wChairID], MAX_LAND_COUNT, argHandCards);
		argAllHandCard.push_back(argHandCards);
	}

	int argSeat = GAME_LAND_PLAYER;
	bool argSeatRobot = false;

	std::vector<int> argHandCards;
	RobotCardJudge(m_bankerCard, 3, argHandCards);
	argAllHandCard.push_back(argHandCards);
	argSeat = evaluate_cards(argAllHandCard);

	if (argSeat < GAME_LAND_PLAYER)
	{
		m_evaluateUser = argSeat;
		for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
		{
			CGamePlayer *pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (wChairID == argSeat && pPlayer->GetUID() == control_uid)
			{
				// 本来就是好牌
				m_evaluateUser = wChairID;
				break;
			}
			else
			{
				// 本来不是好牌 换牌
				if (pPlayer->GetUID() == control_uid && wChairID != argSeat)
				{
					uint8 tmp[MAX_LAND_COUNT];
					memcpy(tmp, m_handCardData[wChairID], MAX_LAND_COUNT);
					memcpy(m_handCardData[wChairID], m_handCardData[argSeat], MAX_LAND_COUNT);
					memcpy(m_handCardData[argSeat], tmp, MAX_LAND_COUNT);
					m_evaluateUser = wChairID;
					break;
				}
			}
		}

		LOG_DEBUG("lost_nochange 2 - roomid:%d,tableid:%d,uid:%d %d %d,control_uid:%d,m_evaluateUser:%d,argSeat:%d",
			GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), control_uid, m_evaluateUser, argSeat);

		return true;
	}

	return false;
}


bool	CGameLandTable::SetControlPalyerLost(uint32 control_uid)
{
	std::vector<std::vector<int> > argAllHandCard;
	for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
	{
		std::vector<int> argHandCards;
		RobotCardJudge(m_handCardData[wChairID], MAX_LAND_COUNT, argHandCards);
		argAllHandCard.push_back(argHandCards);
	}

	int argSeat = GAME_LAND_PLAYER;
	bool argSeatRobot = false;

	std::vector<int> argHandCards;
	RobotCardJudge(m_bankerCard, 3, argHandCards);
	argAllHandCard.push_back(argHandCards);
	argSeat = evaluate_cards(argAllHandCard);

	
	if (argSeat < GAME_LAND_PLAYER)
	{
		m_evaluateUser = argSeat;
		for (WORD wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
		{
			CGamePlayer *pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)
			{
				continue;
			}
			if (wChairID == argSeat && pPlayer->GetUID() == control_uid)
			{
				// 本来就是好牌
				int add_pre = 1;
				if (g_RandGen.RandRatio(50, 100))
				{
					add_pre = 2;
				}
				m_evaluateUser = (wChairID + add_pre) % GAME_LAND_PLAYER; // 设置输 把好牌给下一个玩家
				if (m_evaluateUser < GAME_LAND_PLAYER && m_evaluateUser != argSeat)
				{
					uint8 tmp[MAX_LAND_COUNT];
					memcpy(tmp, m_handCardData[m_evaluateUser], MAX_LAND_COUNT);
					memcpy(m_handCardData[m_evaluateUser], m_handCardData[argSeat], MAX_LAND_COUNT);
					memcpy(m_handCardData[argSeat], tmp, MAX_LAND_COUNT);

					LOG_DEBUG("lost_change 1 - roomid:%d,tableid:%d,uid:%d %d %d,control_uid:%d,m_evaluateUser:%d,argSeat:%d,add_pre:%d",
						GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), control_uid, m_evaluateUser, argSeat, add_pre);

					return true;
				}
			}
		}

		LOG_DEBUG("lost_nochange 2 - roomid:%d,tableid:%d,uid:%d %d %d,control_uid:%d,m_evaluateUser:%d,argSeat:%d",
			GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), GetPlayerID(2), control_uid, m_evaluateUser, argSeat);

		return true;
	}

	return false;
}

bool CGameLandTable::NewRegisterWelfareCtrl()
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

		//如果当前赢取大于最大赢取时,设置机器人必赢
		if (control_status == 2)
		{
			bool ret = SetControlPalyerLost(control_uid);
			if (ret)
			{
				LOG_DEBUG("2 search success current player - uid:%d control_status:%d", control_uid, control_status);
				m_nrw_ctrl_uid = control_uid;   //设置当前新注册福利所控的玩家ID				
				return true;
			}
			else
			{
				LOG_DEBUG("2 search fail current player - uid:%d control_status:%d", control_uid, control_status);
			}
		}
		else
		{
			bool ret = SetControlPalyerWin(control_uid);
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
	}
	else
	{
		LOG_DEBUG("the nrw control_uid is null.tid:%d.", GetTableID());
	}
	LOG_DEBUG("the no player match new register welfare tid:%d.", GetTableID());
	return false;
}

bool CGameLandTable::SetLuckyCtrl()
{
	uint32 win_uid=0;
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
			m_set_ctrl_lucky_uid.insert(win_uid);

			//增加输家列表
			for (uint32 uid : set_lose_uid)
			{
				m_set_ctrl_lucky_uid.insert(uid);
			}

			m_lucky_flag = true;   
			return true;
		}
		else
		{
			LOG_DEBUG("set current win player fail- uid:%d", win_uid);
			return false;
		}
	}

	//如果设置了当前输的玩家---需要区分输家个数 1个或者2个
	uint32 ctrl_uid = 0;

	//1个输家情况
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

	//2个输家情况---找出赢取玩家即可
	if (set_lose_uid.size() == 2)
	{		
		for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		{
			CGamePlayer* pTPlayer = m_vecPlayers[i].pPlayer;
			if (pTPlayer != NULL)
			{
				auto iter = set_lose_uid.find(pTPlayer->GetUID());
				if (iter == set_lose_uid.end())
				{
					ctrl_uid = pTPlayer->GetUID();
					break;
				}
				else
				{
					continue;
				}
			}
		}

		if (ctrl_uid != 0)
		{
			m_set_ctrl_lucky_uid.insert(ctrl_uid);
			m_lucky_flag = true;

			bool ret = SetControlPalyerWin(ctrl_uid);
			if (ret)
			{
				LOG_DEBUG("set current win player - uid:%d", ctrl_uid);
				for (uint32 uid : set_lose_uid)
				{
					m_set_ctrl_lucky_uid.insert(uid);
				}				
				m_lucky_flag = true;
				return true;
			}
			else
			{
				LOG_DEBUG("set current win player fail- uid:%d", ctrl_uid);
				return false;
			}
		}
	}
		
	LOG_DEBUG("the no player match lucky tid:%d.", GetTableID());
	return false;
}

// 设置库存输赢  add by har
bool CGameLandTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	vector<uint16> playerChairs; // 玩家座位号数组
	vector<uint16> robotChairs;  // 机器人座位号数组
	for (uint16 i = 0; i < GAME_LAND_PLAYER; ++i) {
		CGamePlayer *pTmp = GetPlayer(i);
		if (pTmp != NULL) {
			if (pTmp->IsRobot())
				robotChairs.push_back(i);
			else
				playerChairs.push_back(i);
		}
	}
	if (robotChairs.size() == 0 || playerChairs.size() == 0) {
		LOG_ERROR("SetStockWinLose fail1 roomid:%d,tableid:%d,stockChange:%lld,m_bankerUser:%d,player_size:%d,robot_size:%d",
			GetRoomID(), GetTableID(), stockChange, m_bankerUser, playerChairs.size(), robotChairs.size());
		return false;
	}

	uint16 winChair  = INVALID_CHAIR; // 赢家座位号
	uint16 lostChair = INVALID_CHAIR; // 输家座位号
	if (stockChange == -1)
		//if (robotChairs.size() == 1)
			winChair = robotChairs[0];
		//else
		//	lostChair = playerChairs[0];
	else
		//if (playerChairs.size() == 1)
			winChair = playerChairs[0];
		//else
		//	lostChair = robotChairs[0];

	if (lostChair == INVALID_CHAIR){
		CGamePlayer *pTmp = GetPlayer(winChair);
		SetControlPalyerWin(pTmp->GetUID());
	} else {
		CGamePlayer *pTmp = GetPlayer(lostChair);
		SetControlPalyerLost(pTmp->GetUID());
	}

	LOG_DEBUG("SetStockWinLose suc roomid:%d,tableid:%d,stockChange:%lld,m_bankerUser:%d,player_size:%d,robot_size:%d,winChair;%d,lostChair:%d",
		GetRoomID(), GetTableID(), stockChange, m_bankerUser, playerChairs.size(), robotChairs.size(), winChair, lostChair);
	return true;
}

void   CGameLandTable::CheckPlayerScoreManyLeave()
{
	if (m_pHostRoom == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,m_pHostRoom:%p", GetRoomID(), GetTableID(), m_pHostRoom);
		return;
	}
	for (uint16 i = 0; i < GAME_LAND_PLAYER; ++i)
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