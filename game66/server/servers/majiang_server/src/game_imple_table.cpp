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
#include "majiang_logic.h"

using namespace std;
using namespace svrlib;
using namespace game_majiang;
using namespace net;

namespace
{
    const static int32 s_OutCardTime  = 8*1000;
};
CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
        case emROOM_TYPE_COMMON:           // 麻将
            {
                pTable = new CGameMaJiangTable(this,tableID,emTABLE_TYPE_SYSTEM);
            }break;
        case emROOM_TYPE_MATCH:            // 比赛麻将
            {
                pTable = new CGameMaJiangTable(this,tableID,emTABLE_TYPE_SYSTEM);
            }break;
        case emROOM_TYPE_PRIVATE:          // 私人房麻将
            {
                pTable = new CGameMaJiangTable(this,tableID,emTABLE_TYPE_PLAYER);
            }break;
        default:
            {
                assert(false);
                return NULL;
            }break;
    }
    return pTable;
}
// 麻将游戏桌子
CGameMaJiangTable::CGameMaJiangTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();
    //游戏变量
    memset(m_siceCount,0,sizeof(m_siceCount));

    m_wBankerUser=INVALID_CHAIR;
    m_wNextBankerUser=INVALID_CHAIR;
    ZeroMemory(m_cbCardIndex,sizeof(m_cbCardIndex));
	ZeroMemory(m_bTrustee, sizeof(m_bTrustee));
	ZeroMemory(m_uTimeOutCount, sizeof(m_uTimeOutCount));
    //出牌信息
    m_cbOutCardData=0;
    m_cbOutCardCount=0;
    m_wOutCardUser=INVALID_CHAIR;
    ZeroMemory(m_cbDiscardCard,sizeof(m_cbDiscardCard));
    ZeroMemory(m_cbDiscardCount,sizeof(m_cbDiscardCount));
    m_cbGangCount=0;
	ZeroMemory(m_cbArrOutCardCount, sizeof(m_cbArrOutCardCount));
	ZeroMemory(m_cbArrOperIndex, sizeof(m_cbArrOperIndex));
	m_cbPlayerOperIndex = 0;
    //发牌信息
    m_cbSendCardData=0;
    m_cbSendCardCount=0;
	m_wSendCardUser = INVALID_CHAIR;
	m_wLastOutCardUser = INVALID_CHAIR;
    m_poolCards.clear();

    ZeroMemory(m_lGameScore,sizeof(m_lGameScore));
    ZeroMemory(m_lEachScore,sizeof(m_lEachScore));
	m_TempWeaveItem.Init();

    //运行变量
    m_cbProvideCard=0;
    m_wResumeUser=INVALID_CHAIR;
    m_wCurrentUser=INVALID_CHAIR;
    m_wProvideUser=INVALID_CHAIR;

    //状态变量
    m_bSendStatus=false;
    m_bGangStatus=false;
    ZeroMemory(m_bEnjoinChiHu,sizeof(m_bEnjoinChiHu));
    ZeroMemory(m_bEnjoinChiPeng,sizeof(m_bEnjoinChiPeng));

    //用户状态
    ResetUserOperInfo();
    ZeroMemory(m_cbListenStatus,sizeof(m_cbListenStatus));
	ZeroMemory(m_bArrTianTingStatus, sizeof(m_bArrTianTingStatus));
	ZeroMemory(m_bArrTingAfterMingGang, sizeof(m_bArrTingAfterMingGang));
	m_wOperHuCardPlayer = INVALID_CHAIR;
	for (uint32 i = 0; i < GAME_PLAYER; i++)
	{
		m_vecArrUserTingCard[i].clear();
		m_vecArrUserHuCard[i].clear();
		m_mpTingNotifyHu[i].clear();
		m_vecTingNotifyHu[i].clear();
	}
    //组合扑克
    ZeroMemory(m_WeaveItemArray,sizeof(m_WeaveItemArray));
    ZeroMemory(m_cbWeaveItemCount,sizeof(m_cbWeaveItemCount));

    //结束信息
    m_cbChiHuCard=0;
    m_fangPaoUser=INVALID_CHAIR;
    ZeroMemory(&m_ChiHuResult,sizeof(m_ChiHuResult));
    ZeroMemory(m_openingHu,sizeof(m_openingHu));
    ZeroMemory(m_hitBirdCount,sizeof(m_hitBirdCount));
    ZeroMemory(m_addHitBirdCount,sizeof(m_addHitBirdCount));

    m_allBirdCards.clear();

    m_tailCard=0;
    ZeroMemory(m_tailCardOper, sizeof(m_tailCardOper));

    //m_gangTingState = emGANG_TING_NULL;
    m_gangTingCards.clear();

    m_gameLogic.SetMjCfg(&m_mjCfg);
    m_gameLogic.SetMjTable(this);
    m_bChangeHandleCards = false;

	ZeroMemory(m_iUserPassHuCount, sizeof(m_iUserPassHuCount));
	ZeroMemory(m_bArrGangFlowerStatus, sizeof(m_bArrGangFlowerStatus));

	m_stockTingCardZiMoPro = 0; // add by har
}
CGameMaJiangTable::~CGameMaJiangTable()
{

}
bool    CGameMaJiangTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return false;
	}
	if (pPlayer->IsRobot())
	{
		int robotCount = 0;
		for (uint8 i = 0; i<m_vecPlayers.size(); ++i)
		{
			if (m_vecPlayers[i].pPlayer != NULL && m_vecPlayers[i].pPlayer->IsRobot())
			{
				robotCount++;
			}
		}
		if (robotCount == 1)
		{
			return false;
		}
	}
	if (m_pHostRoom != NULL && m_pHostRoom->GetConsume() == ROOM_CONSUME_TYPE_COIN && IsExistIP(pPlayer->GetIP()))
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
	// 是否有屏蔽玩家
	//if (IsExistBlock(pPlayer))
	//	return false;
    return CGameTable::CanEnterTable(pPlayer);
}
bool    CGameMaJiangTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    return CGameTable::CanLeaveTable(pPlayer);
}
void    CGameMaJiangTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::majiang_table_info* pmajiang = pInfo->mutable_majiang();
    pmajiang->set_tableid(GetTableID());

    pmajiang->set_hostname(m_conf.hostName);
    pmajiang->set_deal(m_conf.deal);
    pmajiang->set_basescore(m_conf.baseScore);
    pmajiang->set_consume(m_conf.consume);
    pmajiang->set_entermin(m_conf.enterMin);
    pmajiang->set_duetime(m_conf.dueTime);
    pmajiang->set_feetype(m_conf.feeType);
    pmajiang->set_feevalue(m_conf.feeValue);
    pmajiang->set_show_hand_num(m_pHostRoom->GetShowHandNum());
    pmajiang->set_call_time(s_OutCardTime);
    pmajiang->set_card_time(s_OutCardTime);
    pmajiang->set_play_type(0);
    pmajiang->set_seat_num(m_conf.seatNum);

    if(m_mjCfg.supportSuperCard() && m_mjCfg.hasHongZhongCard()){
        pmajiang->add_kind_cards(emPOKER_ZHONG);
    }

}
//配置桌子
bool CGameMaJiangTable::Init()
{
	ResetTable();

	bool bInitFalg = true;
	if (m_conf.seatNum > GAME_PLAYER)
	{
		m_conf.seatNum = GAME_PLAYER;
	}
	m_wPlayerCount = m_conf.seatNum;

    SetGameState(net::TABLE_STATE_FREE);
	m_vecPlayers.resize(m_wPlayerCount);

    for(uint8 i=0;i<m_wPlayerCount;++i)
    {
        m_vecPlayers[i].Reset();
    }
    m_wNextBankerUser=INVALID_CHAIR;

    ResetCoolTime();

	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
		m_playType = MAJIANG_TYPE_TWO_PEOPLE;
	}
    if(!m_mjCfg.InitConfig(m_playType)){// 暂时写死国标 toney
        //LOG_DEBUG("麻将参数配置错误");
		bInitFalg = false;
    }

	LOG_DEBUG("m_wPlayerCount:%d,m_playType:%d,bInitFalg:%d,gameType:%d,net:type:%d", m_wPlayerCount, m_playType, bInitFalg, CDataCfgMgr::Instance().GetCurSvrCfg().gameType, net::GAME_CATE_TWO_PEOPLE_MAJIANG);

	m_iArrDispatchCardPro[Pro_Index_FiveDuiZi] = 100;
	m_iArrDispatchCardPro[Pro_Index_FourDuiZi] = 200;
	m_iArrDispatchCardPro[Pro_Index_ThreeKeZi] = 300;
	m_iArrDispatchCardPro[Pro_Index_TwoKeZi_OneDuiZi] = 300;
	m_iArrDispatchCardPro[Pro_Index_TwoKeZi] = 500;
	m_iArrDispatchCardPro[Pro_Index_TwelveTongHua] = 100;
	m_iArrDispatchCardPro[Pro_Index_ElevenTongHua] = 200;
	m_iArrDispatchCardPro[Pro_Index_TenTongHua] = 300;
	m_iArrDispatchCardPro[Pro_Index_RandSingle] = 8000;

	//m_iArrDispatchCardPro[Pro_Index_FiveDuiZi] = 0;
	//m_iArrDispatchCardPro[Pro_Index_FourDuiZi] = 0;
	//m_iArrDispatchCardPro[Pro_Index_ThreeKeZi] = 0;
	//m_iArrDispatchCardPro[Pro_Index_TwoKeZi_OneDuiZi] = 0;
	//m_iArrDispatchCardPro[Pro_Index_TwoKeZi] = 0;
	//m_iArrDispatchCardPro[Pro_Index_TwelveTongHua] = 1111;
	//m_iArrDispatchCardPro[Pro_Index_ElevenTongHua] = 0;
	//m_iArrDispatchCardPro[Pro_Index_TenTongHua] = 0;
	//m_iArrDispatchCardPro[Pro_Index_RandSingle] = 0;

	memset(m_NRWDispatchCardPro, 0x0, sizeof(m_NRWDispatchCardPro));
	m_uRobotZiMoPro = 500;

	ReAnalysisParam();
	SetMaxChairNum(GAME_PLAYER); // add by har
    return bInitFalg;
}

bool CGameMaJiangTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	//if (jvalue.isMember("mjt")) {
	//	m_playType = jvalue["mjt"].asInt();
	//}

	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		string strPro = CStringUtility::FormatToString("pr%d", i);
		if (jvalue.isMember(strPro.c_str()) && jvalue[strPro.c_str()].isIntegral())
		{
			m_iArrDispatchCardPro[i] = jvalue[strPro.c_str()].asInt();
		}
	}
	string strDispatchCardPro;
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		strDispatchCardPro += CStringUtility::FormatToString("%d ", m_iArrDispatchCardPro[i]);
	}

	if (jvalue.isMember("zmp") && jvalue["zmp"].isIntegral())
	{
		m_uRobotZiMoPro = jvalue["zmp"].asInt();
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
					m_NRWDispatchCardPro[i] = nrwct_jvalue[i].asUInt();
				}
			}
		}
	}

	LOG_ERROR("reader json parse success - roomid:%d,tableid:%d,m_playType:%d,m_uRobotZiMoPro:%d,strDispatchCardPro:%s,param:%s m_NRWDispatchCardPro:%d %d %d %d %d %d %d %d %d",
		GetRoomID(), GetTableID(), m_playType, m_uRobotZiMoPro, strDispatchCardPro.c_str(), param.c_str(), m_NRWDispatchCardPro[0], m_NRWDispatchCardPro[1]
		, m_NRWDispatchCardPro[2], m_NRWDispatchCardPro[3], m_NRWDispatchCardPro[4], m_NRWDispatchCardPro[5], m_NRWDispatchCardPro[6], m_NRWDispatchCardPro[7]
		, m_NRWDispatchCardPro[8]);

	return true;
}

void CGameMaJiangTable::ShutDown()
{

}

//复位桌子
void CGameMaJiangTable::ResetTable()
{
    ResetGameData();

    SetGameState(TABLE_STATE_FREE);
    ResetPlayerReady();

}
void CGameMaJiangTable::OnTimeTick()
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
                if(IsCanStart()){
                    OnGameStart();
                }
            }break;
        case TABLE_STATE_CALL:
            {

            }break;
        case TABLE_STATE_PLAY:
            {
                OnTimeOutOper();
            }break;
        case TABLE_STATE_GAME_END:
            {
                SetGameState(TABLE_STATE_FREE);				
            }break;
        default:
            break;
        }
    }
    if(GetGameState() == TABLE_STATE_PLAY)
    {
        OnListenOutCard();
        OnRobotOutCard();
		OnTrusteeOutCard();
		OnTrusteeOperCard();
    }
}
// 游戏消息
int CGameMaJiangTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    uint16 chairID = GetChairID(pPlayer);
	if (chairID >= m_wPlayerCount)
	{
		LOG_DEBUG("uid:%d,chairID:%d,cmd:%d,curstate:%d", GetPlayerID(chairID), chairID, cmdID, GetGameState());

		return 0;
	}
	LOG_DEBUG("uid:%d,chairID:%d,cmd:%d,curstate:%d:m_uTimeOutCount:%d", GetPlayerID(chairID), chairID, cmdID, GetGameState(), m_uTimeOutCount[chairID]);

    switch(cmdID)
    {
    case net::C2S_MSG_MAJIANG_OUT_CARD_REQ:// 出牌消息
        {
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}

            net::msg_majiang_out_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            LOG_DEBUG("玩家出牌: uid:%d,%d--%s",GetPlayerID(chairID),chairID,m_gameLogic.GetCardName(msg.card_data()).c_str());
			if (m_uTimeOutCount[chairID] == 1)
			{
				m_uTimeOutCount[chairID] = 0;
			}
            return OnUserOutCard(chairID,msg.card_data());
        }break;
    case net::C2S_MSG_MAJIANG_OPER_CARD_REQ:// 操作消息
        {
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}
            net::msg_majiang_oper_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            LOG_DEBUG("玩家操作:%d--%s--%s",chairID,m_gameLogic.GetOperCodeName(msg.oper_code()).c_str(),m_gameLogic.GetCardName(msg.oper_card()).c_str());
            //前端没有偏移
            uint32 operCode = msg.oper_code();
			if (operCode != 0)
			{
				operCode = 1 << operCode;
			}
			if (m_uTimeOutCount[chairID] == 1)
			{
				m_uTimeOutCount[chairID] = 0;
			}
            return OnUserOperCard(chairID,operCode,msg.oper_card());
        }break;
    case net::C2S_MSG_MAJIANG_CHANGE_CARD_REQ:// 修改手牌操作
        {
			//return 0;
            net::msg_majiang_change_hand_card_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            LOG_DEBUG("change_hand_oper - 修改手牌操作:%d--%d",msg.change_user(),msg.change_card_size());
            uint16 changeUser=msg.change_user();

            return 0;// test toney

            if(changeUser<m_wPlayerCount)
            {
                if(GetGameState() == TABLE_STATE_PLAY)
                {
                   if(msg.change_card_size() != m_gameLogic.GetCardCount(m_cbCardIndex[changeUser])){
                        LOG_DEBUG("change_hand_oper - 牌数不对:%d", msg.change_card_size());
                        return 0;
                   }
                }else {
                    if (msg.change_card_size() != MAX_COUNT && msg.change_card_size() != (MAX_COUNT - 1)) {
                        LOG_DEBUG("change_hand_oper - 换牌数目太多:%d", msg.change_card_size());
                        return 0;
                    }
                }
                memset(m_cbCardIndex[changeUser], 0, sizeof(m_cbCardIndex[changeUser]));
                for(uint8 j = 0; j < msg.change_card_size(); ++j) {
                    BYTE card = msg.change_card(j);
                    m_cbCardIndex[changeUser][m_gameLogic.SwitchToCardIndex(card)]++;
                }
            }else{
                LOG_DEBUG("change_hand_oper - 修改牌池的牌")
                m_poolCards.clear();
                for(int32 i=0;i<msg.change_card_size();++i)
                {
                    m_poolCards.push_front(msg.change_card(i));
                }
            }
            FlushDeskCardToClient();
            m_bChangeHandleCards = true;
			LOG_DEBUG("change_hand_oper - chairid:%d,uid:%d",changeUser,GetPlayerID(changeUser))
        }break;
    case net::C2S_MSG_MAJIANG_GET_HAND_CARD_REQ:// 获取手牌信息
        {
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}

            FlushHandCardToClient(chairID);

        }break;
	case net::C2S_MSG_MAJIANG_OPER_TRUSTEE_REQ:// 托管操作
		{
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}

			net::msg_majiang_oper_trustee_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			LOG_DEBUG("托管操作 - uid:%d,chairID:%d,trustee:%d",GetPlayerID(chairID), chairID, msg.trustee());

			OnUserOperTrustee(chairID, msg.trustee());
			return 0;
		}break;
	case net::C2S_MSG_MAJIANG_OPER_AFTER_NOTIFY_HU_REQ:// 提示消息
		{
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}
			net::msg_mjaction_oper_after_notify_hu_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			//前端没有偏移
			return OnUserNotifyHu(chairID);
		}break;
	case net::C2S_MSG_MAJIANG_GET_ALL_CARD_INFO_REQ:// 
		{
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				return 0;
			}

			FlushDeskCardToClient(chairID);
			return 0;
		}break;
	case net::C2S_MSG_MAJIANG_OUT_AFTER_NOTIFY_HU_REQ:// 提示消息
	{
		if (GetGameState() != TABLE_STATE_PLAY)
		{
			return 0;
		}
		net::mjaction_out_after_notify_hu_req msg;
		PARSE_MSG_FROM_ARRAY(msg);
		//前端没有偏移
		return OnUserOutCardNotifyHu(chairID);
	}break;
    default:
        return 0;
    }
    return 0;
}

bool CGameMaJiangTable::MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard)
{
	LOG_DEBUG("start - roomid:%d,tableid:%d,status:%d,strHandCard:%s",GetRoomID(),GetTableID(), GetGameState(), strHandCard.c_str());

	if (GetGameState() != net::TABLE_STATE_FREE)
	{
		LOG_DEBUG("error - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

		return 0;
	}


	string param = strHandCard;
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_DEBUG("error - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

		return true;
	}
	vector<uint32> vecHandCard[GAME_PLAYER];
	for (uint32 i = 0; i < jvalue.size(); i++)
	{
		if (jvalue[i].isArray())
		{
			Json::Value card = jvalue[i];
			for (uint32 j = 0; j < card.size(); j++)
			{
				if (card[j].isIntegral())
				{
					vecHandCard[i].push_back(card[j].asInt());
				}
				else
				{
					LOG_DEBUG("error isIntegral - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());
					return false;
				}
			}
		}
		else
		{
			LOG_DEBUG("error isArray - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());
			return false;
		}
	}
	BYTE cbAllCardIndex[MAX_INDEX] = { 0 };
	int iBrankerCount = 0;
	for (uint16 i = 0; i < m_wPlayerCount; i++)
	{
		for (uint32 j = 0; j < vecHandCard[i].size(); j++)
		{
			BYTE cbCard = vecHandCard[i][j];
			if (m_gameLogic.IsValidCard(cbCard) == false)
			{
				LOG_DEBUG("error IsValidCard - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

				return false;
			}

			BYTE cbCardIndex = m_gameLogic.SwitchToCardIndex(cbCard);
			cbAllCardIndex[cbCardIndex]++;
		}
		if (vecHandCard[i].size() > MAX_COUNT)
		{
			LOG_DEBUG("error >MAX_COUNT - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

			return false;
		}
		if (vecHandCard[i].size() == MAX_COUNT)
		{
			iBrankerCount++;
		}
	}
	if (iBrankerCount>= m_wPlayerCount)
	{
		LOG_DEBUG("error iBrankerCount - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

		return false;
	}
	for (int j = 0; j < MAX_INDEX; j++)
	{
		if (cbAllCardIndex[j] >= 4)
		{
			LOG_DEBUG("error cbAllCardIndex - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());
		}
	}

	ZeroMemory(m_cbCardIndex, sizeof(m_cbCardIndex));

	for (uint16 i = 0; i < m_wPlayerCount; i++)
	{
		for (uint16 j = 0; j < vecHandCard[i].size(); ++j)
		{
			BYTE cbCard = vecHandCard[i][j];
			m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(cbCard)]++;
		}
	}
	FlushDeskCardToClient();
	m_bChangeHandleCards = true;

	LOG_DEBUG("success - roomid:%d,tableid:%d,status:%d,strHandCard:%s", GetRoomID(), GetTableID(), GetGameState(), strHandCard.c_str());

	return true;
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameMaJiangTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	DealSIsRobotOrPlayerJetton(pPlayer, isAllPlayer, isAllRobot);
}

bool CGameMaJiangTable::StartSendHandCard()
{
	ZeroMemory(m_cbCardIndex, sizeof(m_cbCardIndex));

	//先判断玩家是否为新注册玩家福利且为必赢牌局
	bool bSetNRWControlCardData = false;
	bSetNRWControlCardData = SetNRWControlCardData();

	bool bSetStockControl = false;
	bool bSetControlCardData = false;
	if (!bSetNRWControlCardData)
	{
		bSetControlCardData = SetControlCardData();
	}
	if (bSetNRWControlCardData)
	{
		for (WORD i = 0; i < m_wPlayerCount; i++)
		{
			CGamePlayer * pPlayer = GetPlayer(i);
			if (pPlayer != NULL && !pPlayer->IsRobot())
			{
				continue;
			}
			for (uint8 j = 0; j < (MAX_COUNT - 1); ++j)
			{
				BYTE card = PopCardFromPool();
				m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
			}
		}
	}
	else
	{
		bSetStockControl = SetStockWinLose(); // add by har
		if (bSetControlCardData)
		{
			for (WORD i = 0; i < m_wPlayerCount; i++)
			{
				CGamePlayer * pPlayer = GetPlayer(i);
				if (pPlayer != NULL && pPlayer->IsRobot())
				{
					continue;
				}
				for (uint8 j = 0; j < (MAX_COUNT - 1); ++j)
				{
					BYTE card = PopCardFromPool();
					m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
				}
			}
		}
		else
		{
			for (WORD i = 0; i < m_wPlayerCount; i++)
			{
				for (uint8 j = 0; j < (MAX_COUNT - 1); ++j)
				{
					BYTE card = PopCardFromPool();
					m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
				}
			}
		}
	}
	

	//for (WORD i = 0; i < m_wPlayerCount; i++)
	//{
	//	for (uint8 j = 0; j < (MAX_COUNT - 1); ++j)
	//	{
	//		BYTE card = PopCardFromPool();
	//		m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
	//	}
	//}

	m_cbSendCardCount++;
	m_cbSendCardData = PopCardFromPool();
	m_cbCardIndex[m_wBankerUser][m_gameLogic.SwitchToCardIndex(m_cbSendCardData)]++;

	string strHandCardIndex;
	int iHandCardCount = 0;
	for (unsigned int j = 0; j < MAX_INDEX; ++j)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[m_wBankerUser][j]);
		iHandCardCount += m_cbCardIndex[m_wBankerUser][j];
	}

	LOG_DEBUG("branker_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,m_wBankerUser:%d,bSetControlCardData:%d,bSetNRWControlCardData:%d,bSetStockControl:%d,m_stockTingCardZiMoPro:%d,iHandCardCount:%d,m_cbSendCardData:0x%02X - %d,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), m_wBankerUser, GetPlayerID(m_wBankerUser), m_wBankerUser, bSetControlCardData, bSetNRWControlCardData, bSetStockControl, m_stockTingCardZiMoPro, iHandCardCount, m_cbSendCardData, m_gameLogic.SwitchToCardIndex(m_cbSendCardData), strHandCardIndex.c_str());

	return true;
}

bool CGameMaJiangTable::StartChangeHandCard()
{
	bool bIsErrorCard = false;
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		for (uint8 j = 0; j < MAX_INDEX; ++j)
		{
			for (BYTE k = 0; k < m_cbCardIndex[i][j]; k++)
			{
				BYTE cbCard = m_gameLogic.SwitchToCardData(j);
				auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), cbCard);
				if (iter != m_poolCards.end())
				{
					m_poolCards.erase(iter);
				}
				else
				{
					bIsErrorCard = true;
					LOG_DEBUG("error iBrankerCount - roomid:%d,tableid:%d,cbCard:0x%2X",
						GetRoomID(), GetTableID(), cbCard);
				}
			}
		}
	}

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		int cardNum = m_gameLogic.GetCardCount(m_cbCardIndex[i]);
		if (cardNum == MAX_COUNT && i == m_wBankerUser)
		{
			continue;
		}
		if (i == m_wBankerUser)
		{
			if (cardNum > MAX_COUNT)
			{
				int iSubCardCount = cardNum - MAX_COUNT;

				for (uint8 j = 0; j < MAX_INDEX && iSubCardCount>0; ++j)
				{
					if (m_cbCardIndex[i][j] > 0)
					{
						m_cbCardIndex[i][j]--;
						iSubCardCount--;
						BYTE cbCard = m_gameLogic.SwitchToCardData(j);
						m_poolCards.push_back(cbCard);
					}
				}
			}
			else
			{
				int iSubCardCount = MAX_COUNT - cardNum;
				for (uint8 j = 0; j < MAX_COUNT && iSubCardCount>0; ++j)
				{
					BYTE card = PopCardFromPool();
					m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
					iSubCardCount--;
				}
			}
		}
		else
		{
			int cardNum = m_gameLogic.GetCardCount(m_cbCardIndex[i]);
			if (cardNum > (MAX_COUNT - 1))
			{
				int iSubCardCount = cardNum - (MAX_COUNT - 1);

				for (uint8 j = 0; j < (MAX_INDEX - 1) && iSubCardCount>0; ++j)
				{
					if (m_cbCardIndex[i][j] > 0)
					{
						m_cbCardIndex[i][j]--;
						iSubCardCount--;
						BYTE cbCard = m_gameLogic.SwitchToCardData(j);
						m_poolCards.push_back(cbCard);
					}
				}
			}
			else
			{
				int iSubCardCount = (MAX_COUNT - 1) - cardNum;
				for (uint8 j = 0; j < (MAX_COUNT - 1) && iSubCardCount>0; ++j)
				{
					BYTE card = PopCardFromPool();
					m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
					iSubCardCount--;
				}
			}
		}
	}

	for (uint8 i = 0; i<MAX_INDEX; ++i)
	{
		if (m_cbCardIndex[m_wBankerUser][i] > 0)
		{
			m_cbSendCardData = m_gameLogic.SwitchToCardData(i);
			m_cbSendCardCount++;
			break;
		}
	}

	string strHandCardIndex[GAME_PLAYER];
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		for (uint8 j = 0; j < MAX_INDEX; ++j)
		{
			strHandCardIndex[i] += CStringUtility::FormatToString("%d ", m_cbCardIndex[i][j]);
		}
	}
	BYTE cbPoolCardIndex[MAX_INDEX] = { 0 };
	for(auto iter_pool = m_poolCards.begin();  iter_pool != m_poolCards.end(); iter_pool++)
	{
		cbPoolCardIndex[m_gameLogic.SwitchToCardIndex(*iter_pool)]++;
	}
	string strPoolCard;
	BYTE cbPoolCardCount = 0;
	for (uint8 j = 0; j < MAX_INDEX; ++j)
	{
		strPoolCard += CStringUtility::FormatToString("%d ", cbPoolCardIndex[j]);
		cbPoolCardCount += cbPoolCardIndex[j];
	}
	
	BYTE cbArrHandCardData[GAME_PLAYER][MAX_COUNT];
	BYTE cbArrHandCardCount[GAME_PLAYER] = { 0 };
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		cbArrHandCardCount[i] = m_gameLogic.SwitchToCardData(m_cbCardIndex[i], cbArrHandCardData[i], MAX_COUNT);
	}

	string strHandCardData[GAME_PLAYER];
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		for (uint8 j = 0; j < cbArrHandCardCount[i]; ++j)
		{
			strHandCardData[i] += CStringUtility::FormatToString("0x%02X ", cbArrHandCardData[i][j]);
		}
	}

	LOG_DEBUG("change_handcard_pool - roomid:%d,tableid:%d,m_wBankerUser:%d,uid:%d %d,strHandCardIndex:%s - %s,strHandCardData:%s - %s,strPoolCard:%s",
		GetRoomID(), GetTableID(), m_wBankerUser,GetPlayerID(0),GetPlayerID(1), strHandCardIndex[0].c_str(),strHandCardIndex[1].c_str(), strHandCardData[0].c_str(), strHandCardData[1].c_str(), strPoolCard.c_str());


	return true;
}

// 游戏开始
bool CGameMaJiangTable::OnGameStart()
{
    // 获取当前局活跃福利玩家的数量
    SetActiveWelfareCtrlList();

	// 获取当前局幸运值的控制玩家
	m_lucky_win_uid = 0;
	m_set_ctrl_lucky_uid.clear();
	GetLuckyPlayerUid();
	m_lucky_flag = false;
	LOG_DEBUG("幸运值当前控制玩家:%d", m_lucky_win_uid);

    m_wBankerUser = m_wNextBankerUser;
	m_NRWRobotZiMo = false;

    //混乱扑克
    InitRandCard();
    //掷骰子
    m_siceCount[0] = g_RandGen.RandRange(1,6);
    m_siceCount[1] = g_RandGen.RandRange(1,6);
    //初始化庄家
    if(m_wNextBankerUser == INVALID_CHAIR)
	{
        m_wBankerUser = m_siceCount[0]%m_wPlayerCount;// 随机庄家
        //m_wBankerUser = 1;// test
    }
    //LOG_DEBUG("当前庄家:%d",m_wBankerUser);

    //发牌
    if(m_bChangeHandleCards == false)
	{		
		StartSendHandCard();
    }
	else
	{// 换牌
		StartChangeHandCard();
    }

    m_cbProvideCard=0;
    m_wProvideUser=INVALID_CHAIR;
    m_wCurrentUser=m_wBankerUser;
    //动作分析
    if(!ProcessOpeningHu())// 处理起手胡
    {
        CheckStartAction(true);
		CheckUserTingCard(m_wBankerUser);
    }
	else
	{
        m_cbProvideCard=0;
        m_wProvideUser=INVALID_CHAIR;
        m_wCurrentUser=INVALID_CHAIR;
    }

	m_wLastOutCardUser = GetNextUser(m_wBankerUser);
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("game start: roomid:%d,tableid:%d,m_wBankerUser:%d,GetIsAllRobotOrPlayerJetton:%d,m_cbSendCardData:0x%02X",
		m_pHostRoom->GetRoomID(), GetTableID(), m_wBankerUser, GetIsAllRobotOrPlayerJetton(), m_cbSendCardData);

    //构造数据
    net::msg_majiang_start_rep msg;
    msg.add_sice_count(m_siceCount[0]);
    msg.add_sice_count(m_siceCount[1]);
    msg.set_cur_round(0);
    msg.set_banker_user(m_wBankerUser);
    msg.set_cur_user(m_wCurrentUser);
    msg.set_left_card_count(m_poolCards.size());

    for(uint8 i=0;i<m_wPlayerCount;++i)
	{
        BYTE cardNum = m_gameLogic.GetCardCount(m_cbCardIndex[i]);
        msg.add_card_num(cardNum);
    }

	DWORD tmp = m_gameLogic.AnalyseTingCard15(m_wBankerUser, m_cbCardIndex[m_wBankerUser], m_WeaveItemArray[m_wBankerUser], m_cbWeaveItemCount[m_wBankerUser], m_mpTingNotifyHu[m_wBankerUser]);

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		msg.add_passhu_count(m_iUserPassHuCount[i]);
		//bool bIsHuOper = false;
		//vector<net::mjaction> actions;
		//SwitchUserAction(i, m_cbSendCardData, actions);
		//for (uint8 j = 0; j < actions.size(); ++j)
		//{
		//	if (actions[j].code() == 7)
		//	{
		//		bIsHuOper = true;
		//	}
		//}
		//if (bIsHuOper)
		//{
		//	msg.add_passhu_count(m_iUserPassHuCount[i]);
		//}
		//else
		//{
		//	msg.add_passhu_count(-1);
		//}
	}
    //发送数据
    for (WORD i=0;i<m_wPlayerCount;i++)
    {
        //设置变量
        msg.clear_user_action();
		bool bIsHuOper = false;
        vector<net::mjaction> actions;
        SwitchUserAction(i,m_cbSendCardData,actions);
        for(uint8 j=0;j<actions.size();++j){
            net::mjaction* pa=msg.add_user_action();
            *pa = actions[j];
			if (actions[j].code() == 7)
			{
				bIsHuOper = true;
			}
        }
		//if (bIsHuOper)
		//{
		//	msg.set_passhu_count(m_iUserPassHuCount[i]);
		//}
		//else
		//{
		//	msg.set_passhu_count(-1);
		//}
        msg.clear_card_data();
		BYTE cardData[MAX_COUNT] = {0};

		string strHandCardIndex;
		int iHandCardCount = 0;
		for (unsigned int j = 0; j < MAX_INDEX; ++j)
		{
			strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[i][j]);
			iHandCardCount += m_cbCardIndex[i][j];
		}

		LOG_DEBUG("send_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,m_wBankerUser:%d,iHandCardCount:%d,strHandCardIndex:%s",
			GetRoomID(), GetTableID(), i, GetPlayerID(i), m_wBankerUser, iHandCardCount, strHandCardIndex.c_str());


        BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[i],cardData,MAX_COUNT);
        for(uint32 j=0;j<cardNum;++j){
            msg.add_card_data(cardData[j]);
        }

		net::mjaction_out_front_notify_hu * pout_front_notify_hu = msg.add_notify_hu();

		LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,cardData:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
			GetPlayerID(i), m_cbListenStatus[i], m_wCurrentUser, cardData, m_mpTingNotifyHu[i].size());

		auto iter_begin = m_mpTingNotifyHu[i].begin();
		for (; iter_begin != m_mpTingNotifyHu[i].end(); iter_begin++)
		{
			pout_front_notify_hu->add_out_card(iter_begin->first);
			net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

			vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
			for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
			{
				pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
				pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
				pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

				LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_wCurrentUser:%d,operCard:0x%02X,out_card:0x%02X - hu_card:0x%02X",
					GetPlayerID(i), m_wCurrentUser, cardData, iter_begin->first, vecTingNotifyHu[uIndex].cbCard);
			}
		}

		uint32 uid = 0;
		CGamePlayer * pPlayer = GetPlayer(i);
		if (pPlayer != NULL)
		{
			uid = pPlayer->GetUID();
		}
		//LOG_DEBUG("handcard - roomid:%d,tableid:%d,i:%d,uid:%d,m_cbCardIndex:%d %d %d %d %d %d %d %d %d %d %d %d %d %d", m_pHostRoom->GetRoomID(), GetTableID(), i, uid, m_cbCardIndex[i][0], m_cbCardIndex[i][1], m_cbCardIndex[i][2], m_cbCardIndex[i][3], m_cbCardIndex[i][4], m_cbCardIndex[i][5], m_cbCardIndex[i][6], m_cbCardIndex[i][7], m_cbCardIndex[i][8], m_cbCardIndex[i][9], m_cbCardIndex[i][10], m_cbCardIndex[i][11], m_cbCardIndex[i][12], m_cbCardIndex[i][13]);
		LOG_DEBUG("handcard - roomid:%d,tableid:%d,i:%d,uid:%d,m_cbListenStatus:%d,cardData:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", m_pHostRoom->GetRoomID(), GetTableID(),i, uid, m_cbListenStatus[i],cardData[0], cardData[1], cardData[2], cardData[3], cardData[4], cardData[5], cardData[6], cardData[7], cardData[8], cardData[9], cardData[10], cardData[11], cardData[12], cardData[13]);

        SendMsgToClient(i,&msg,S2C_MSG_MAJIANG_START);

		// 发完清除数据
		pout_front_notify_hu->clear_out_card();
		pout_front_notify_hu->clear_notify_hu();
    }

    SetGameState(net::TABLE_STATE_PLAY);
    ResetCoolTime();

    InitBlingLog(true);
    DeductStartFee(true);

	WriteUserHandCard(HAND_CARD_PSO_GAME_START);


	//if (m_bChangeHandleCards == false)
	//{
	//	DispatchCardData(m_wCurrentUser,true,true);
	//}

    return true;
}
//游戏结束
bool CGameMaJiangTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end - roomid:%d,tableid:%d,chairID:%d,reason:%d,m_fangPaoUser:%d,m_wProvideUser:%d", m_pHostRoom->GetRoomID(), GetTableID(), chairID,reason,m_fangPaoUser,m_wProvideUser);
    m_coolLogic.clearCool();
    switch(reason)
    {
    case GER_NORMAL:		//常规结束
        {
			FlushDeskCardToClient();

            if(m_fangPaoUser == chairID){
                m_fangPaoUser = INVALID_CHAIR;
            }

            //CalcGangScore();//算杠分
            CalcHuScore();

            SetNextBanker();

            net::msg_majiang_game_over_rep msg;
			for (uint16 i = 0; i<m_wPlayerCount; ++i)
			{
				msg.add_passhu_count(m_iUserPassHuCount[i]);
			}
            msg.set_provide_user(m_fangPaoUser);
            msg.set_chi_hu_card(m_cbChiHuCard);
            msg.set_banker_user(m_wBankerUser);
            if(m_wProvideUser==INVALID_CHAIR){
                msg.set_end_type(0);//流局
                LOG_DEBUG("流局");
            }else if(m_ChiHuResult[m_wProvideUser].IsHu()){
                msg.set_end_type(2);//自摸
                LOG_DEBUG("自摸");
            }else{
                msg.set_end_type(1);//点炮
                LOG_DEBUG("点炮");
            }
			int64 playerAllWinScore = 0; // 玩家总赢分 add by har
            for(int32 i=0;i<m_wPlayerCount;++i)
            {
                net::mjhu_info* phu = msg.add_hu_infos();
                vector<BYTE> actions;
                m_openingHu[i].getAllActions(actions);
                for(uint8 j=0;j<actions.size();++j){
                    phu->add_open_hu(actions[j]);
                }

                m_ChiHuResult[i].HuKind.getAllActions(actions);
                for(uint8 j=0;j<actions.size();++j){
                    phu->add_hu_kind(actions[j]);
					LOG_DEBUG("hu_card_type - chairID:%d,uid:%d,j:%d,kind:%d", chairID,GetPlayerID(chairID), j, actions[j]);
                }
				WriteGameEndLog(i, OPERATE_CODE_HU_TYPE, actions);

				m_ChiHuResult[i].HuKind.getAllTypeFan(actions);
				for (uint8 j = 0; j<actions.size(); ++j) {
					phu->add_hu_score(actions[j]);
					LOG_DEBUG("hu_card_score - chairID:%d,uid:%d,j:%d,fan:%d", chairID,GetPlayerID(chairID),j, actions[j]);
				}
				WriteGameEndLog(i, OPERATE_CODE_HU_SCORE, actions);

                m_gameLogic.GetAllAction(m_ChiHuResult[i].dwChiHuRight,actions);
                for(uint8 j=0;j<actions.size();++j){
                    phu->add_hu_right(actions[j]);
                }

                //phu->set_score(m_lGameScore[i]);
                for(uint8 j=0;j<m_hitBirdCount[i];++j){
                    phu->add_hit_bird(m_birdCards[i][j]);
                }
                phu->set_hu_card(m_ChiHuResult[i].ChiHuCard);

				int64 score = m_lGameScore[i];
				int64 fee_score = CalcPlayerGameInfo(GetPlayerID(i), score);
				FlushUserBlingLog(GetPlayer(i), score,0, (i == m_wBankerUser) ? 1 : 0);
				m_lGameScore[i] += fee_score;

                phu->set_score(m_lGameScore[i]);

				LOG_DEBUG("hu_score - i:%d,uid:%d,m_lGameScore:%lld", i, GetPlayerID(i), m_lGameScore[i]);

				WriteGameEndLog(i, m_fangPaoUser, m_cbChiHuCard, m_lGameScore[i]);

                BYTE cbCardData[MAX_COUNT];
                BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[i],cbCardData,MAX_COUNT);
                for(uint32 j=0;j<cardNum;++j){
                    phu->add_card_datas(cbCardData[j]);
                }
                for(uint8 j=0;j<m_cbWeaveItemCount[i];++j){
                    net::weave_item* pItem = phu->add_weave_items();
                    pItem->set_provide_user(m_WeaveItemArray[i][j].wProvideUser);
                    pItem->set_center_card(m_WeaveItemArray[i][j].cbCenterCard);
                    pItem->set_public_card(m_WeaveItemArray[i][j].cbPublicCard);
                    pItem->set_weave_kind(m_gameLogic.GetOneAction(m_WeaveItemArray[i][j].WeaveKind));
                }
                phu->set_bird_score(m_hitBirdCount[i] + m_addHitBirdCount[i]);
				CGamePlayer *pPlayer = GetPlayer(i);
				if (pPlayer != NULL && !pPlayer->IsRobot())
					playerAllWinScore += m_lGameScore[i];
                //LOG_DEBUG("玩家结算:%d--%s-%s-%s-%s",i,m_gameLogic.PrintOpenHuType(m_openingHu[i]).c_str(),m_gameLogic.PrintHuKindType(m_ChiHuResult[i].HuKind).c_str()
                //,m_gameLogic.PrintHuRightType(m_ChiHuResult[i].dwChiHuRight).c_str(),m_gameLogic.GetCardName(m_ChiHuResult[i].ChiHuCard).c_str());
            }
            //全部鸟牌
            for(uint32 i=0;i<m_allBirdCards.size();++i){
                msg.add_all_bird(m_allBirdCards[i]);
            }

            SendMsgToAll(&msg,S2C_MSG_MAJIANG_GAME_OVER_REP);
			WriteUserHandCard(HAND_CARD_PSO_GAME_END);

			//更新幸运值数据   
			if (m_lucky_flag)
			{
				for (uint16 i = 0; i < m_wPlayerCount; ++i)
				{
					CGamePlayer * pGamePlayer = GetPlayer(i);
					if (pGamePlayer != NULL)
					{
						LOG_DEBUG("set current player lucky info. uid:%d roomid:%d m_set_ctrl_lucky_uid size:%d", pGamePlayer->GetUID(), GetRoomID(), m_set_ctrl_lucky_uid.size());
					
						auto iter = m_set_ctrl_lucky_uid.find(pGamePlayer->GetUID());
						if (iter != m_set_ctrl_lucky_uid.end())
						{
							pGamePlayer->SetLuckyInfo(GetRoomID(), m_lGameScore[i]);
							LOG_DEBUG("set current player lucky info. uid:%d roomid:%d score:%d", pGamePlayer->GetUID(), GetRoomID(), m_lGameScore[i]);
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
						int64 curr_win = m_lGameScore[i];
						if (pGamePlayer->GetUID() == m_nrw_ctrl_uid)
						{
							if (m_nrw_status == 2 && curr_win > 0)
							{
								pGamePlayer->UpdateNRWPlayerScore(curr_win);
								LOG_DEBUG("control current player is lost. but current player is win. uid:%d score:%d", m_nrw_ctrl_uid, curr_win);
							}
							else
							{
								UpdateNewRegisterWelfareInfo(pGamePlayer->GetUID(), curr_win);
							}
						}
						else
						{
							pGamePlayer->UpdateNRWPlayerScore(curr_win);
						}
					}
				}
			}

            // 更新活跃福利数据
            if(m_aw_ctrl_uid!=0)
            {
                CGamePlayer * pGamePlayer = GetGamePlayerByUid(m_aw_ctrl_uid);
                int32 aw_chair_id = GetChairID(pGamePlayer);
                if (aw_chair_id != INVALID_CHAIR)
                {
                    int64 curr_win = m_lGameScore[aw_chair_id];
                    UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);
                }
            }           

            //修改积分
            //for(WORD i=0;i<m_wPlayerCount;i++)
            //{
            //    int64 score = m_lGameScore[i];
            //    CalcPlayerGameInfo(GetPlayerID(i),score);
            //    FlushUserBlingLog(GetPlayer(i),score,(i==m_wBankerUser) ? 1 : 0);
            //}
            //结束游戏
            ResetGameData();

            SetGameState(TABLE_STATE_GAME_END);
            m_coolLogic.beginCooling(500);
            m_coolRobot.beginCooling(500);

            ResetPlayerReady();
            SendSeatInfoToClient();

            SaveBlingLog();
			m_operLog.clear();
			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,playerAllWinScore:%lld", GetRoomID(), GetTableID(), playerAllWinScore);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
			OnTableGameEnd();

			CheckPlayerScoreManyLeave();
			//AddBlockers();
            return true;
        }break;
    case GER_DISMISS:		//游戏解散
        {
            //LOG_ERROR("froce dis game");
            for(uint8 i=0;i<m_wPlayerCount;++i)
            {
                if(m_vecPlayers[i].pPlayer != NULL){
                    LeaveTable(m_vecPlayers[i].pPlayer);
                }
            }
            ResetTable();
			//m_operLog.clear();
            return true;
        }break;
    case GER_USER_LEAVE:	//用户强退
    case GER_NETWORK_ERROR:	//网络中断
    default:
		{
			//m_operLog.clear();
			return true;
		}
       
    }
    //错误断言
    assert(false);
    return false;
}
// 添加防黑名单
void    CGameMaJiangTable::AddBlockers()
{
	for (uint32 i = 0; i<m_vecPlayers.size(); ++i)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer == NULL)
			continue;
		pPlayer->ClearBlocker();
		pPlayer->ClearBlockerIP();
		for (uint32 j = 0; j<m_vecPlayers.size(); ++j)
		{
			CGamePlayer* pTmp = m_vecPlayers[j].pPlayer;
			if (pTmp != NULL) {
				pPlayer->AddBlocker(pTmp->GetUID());
				pPlayer->AddBlockerIP(pTmp->GetIP());
			}
		}
	}
}
//用户同意
bool    CGameMaJiangTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    if(m_coolLogic.isTimeOut()){
        m_coolLogic.beginCooling(500);// 准备后等一秒
    }
    return true;
}
//玩家进入或离开
void  CGameMaJiangTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    m_coolRobot.beginCooling(10*1000);
    //RandChangeSeat(chairID);// test
}
// 发送场景信息(断线重连)
void    CGameMaJiangTable::SendGameScene(CGamePlayer* pPlayer)
{
    LOG_DEBUG("send game scene: uid:%d", pPlayer->GetUID());
    uint16 chairID = GetChairID(pPlayer);
    if(GetGameState() == net::TABLE_STATE_FREE)
    {
        return;
    }
    else if(GetGameState() == net::TABLE_STATE_PLAY || GetGameState() == net::TABLE_STATE_CALL)
    {
        net::msg_majiang_game_info_rep msg;
        msg.set_cur_round(0);
        msg.set_banker_user(m_wBankerUser);
        msg.set_cur_user(m_wCurrentUser);
        msg.set_game_state(GetGameState());

        msg.set_turn_winer(m_wOutCardUser);
        msg.set_turn_card_data(m_cbOutCardData);
        msg.set_resume_user(m_wResumeUser);
		msg.set_send_card_user(m_wSendCardUser);
		//msg.set_passhu_count(m_iUserPassHuCount[chairID]);

		for (WORD i = 0; i < m_wPlayerCount; i++)
		{
			msg.add_passhu_count(m_iUserPassHuCount[i]);
		}

		//for (WORD i = 0; i < m_wPlayerCount; i++)
		//{
		//	bool bIsHuOper = false;
		//	vector<net::mjaction> actions;
		//	SwitchUserAction(i, m_cbProvideCard, actions);
		//	for (uint8 j = 0; j < actions.size(); ++j)
		//	{
		//		if (actions[j].code() == 7)
		//		{
		//			bIsHuOper = true;
		//		}
		//	}
		//	if (bIsHuOper)
		//	{
		//		msg.add_passhu_count(m_iUserPassHuCount[i]);
		//	}
		//	else
		//	{
		//		msg.add_passhu_count(-1);
		//	}
		//	LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,bIsHuOper:%d,action.size:%d,m_iUserPassHuCount:%d,size_hu:%d,passhu_count:%d",
		//		GetRoomID(),GetTableID(),i,GetPlayerID(i), bIsHuOper, actions.size(), m_iUserPassHuCount[i],msg.passhu_count_size(), msg.passhu_count(i));
		//}



		LOG_DEBUG("send_scene - uid:%d,chairID:%d,m_wCurrentUser:%d,m_mpTingNotifyHu[chairID].size:%d,turn_card_data:0x%02X,m_cbOutCardData:0x%02X",
			pPlayer->GetUID(), chairID,m_wCurrentUser, m_mpTingNotifyHu[chairID].size(), msg.turn_card_data(), m_cbOutCardData);

		vector<net::mjaction> actions;
		SwitchUserAction(chairID, m_cbProvideCard, actions);
		
		for (uint32 j = 0; j<actions.size(); ++j)
		{
			net::mjaction* pa = msg.add_cur_oper();
			*pa = actions[j];
		}

		for (int32 i = 0; i < m_wPlayerCount; ++i)
		{
			//提示胡牌
			net::mjaction_out_front_notify_hu * pout_front_notify_hu = msg.add_notify_hu();
			if (chairID != i)
			{
				continue;
			}
			auto iter_begin = m_mpTingNotifyHu[i].begin();
			for (; iter_begin != m_mpTingNotifyHu[i].end(); iter_begin++)
			{
				pout_front_notify_hu->add_out_card(iter_begin->first);
				net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

				vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
				for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
				{
					pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
					pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
					pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

					LOG_DEBUG("else_if_ting_card - uid:%d,card:0x%02X - hu_card:0x%02X", GetPlayerID(i), iter_begin->first, vecTingNotifyHu[uIndex].cbCard);
				}
			}
		}


        msg.set_left_card_count(m_poolCards.size());
        msg.set_send_card_data(((m_cbSendCardData!=0)&&(m_wProvideUser==chairID))?m_cbSendCardData:0x00);

		// 暗杠 自摸 不帮加进去 

        //弃牌,组合
        for(int32 i=0;i<m_wPlayerCount;++i)
		{
			BYTE cbDiscardCount = m_cbDiscardCount[i];


            for(uint32 j=0;j<m_cbDiscardCount[i];++j){
				//LOG_DEBUG("send_scene_1 - uid:%d,i:%d,j:%d,m_cbDiscardCount:%d,m_cbDiscardCard:0x%02X", pPlayer->GetUID(), i,j, m_cbDiscardCount[i], m_cbDiscardCard[i][j]);

                msg.add_discard_cards(m_cbDiscardCard[i][j]);
            }

			if (actions.size() > 0 && chairID != i && m_wOutCardUser != chairID && chairID != m_wCurrentUser)
			{
				cbDiscardCount += 1;
				msg.add_discard_cards(m_cbOutCardData);
			}

			//LOG_DEBUG("send_scene_2 - uid:%d,i:%d,chairID:%d,m_wOutCardUser:%d,m_wCurrentUser:%d,m_cbDiscardCount:%d,cbDiscardCount:%d,actions.size:%d",
			//	pPlayer->GetUID(), i, chairID, m_wOutCardUser, m_wCurrentUser, m_cbDiscardCount[i], cbDiscardCount, actions.size() );


			msg.add_discard_count(cbDiscardCount);

            msg.add_weave_count(m_cbWeaveItemCount[i]);
            for(uint32 j=0;j<m_cbWeaveItemCount[i];++j){
                net::weave_item* pItem = msg.add_weave_items();
                pItem->set_provide_user(m_WeaveItemArray[i][j].wProvideUser);
                pItem->set_center_card(m_WeaveItemArray[i][j].cbCenterCard);
                pItem->set_public_card(m_WeaveItemArray[i][j].cbPublicCard);
                pItem->set_weave_kind(m_gameLogic.GetOneAction(m_WeaveItemArray[i][j].WeaveKind));
            }

            //手牌数据
            BYTE cbCardData[MAX_COUNT];
            BYTE cardCount = m_gameLogic.SwitchToCardData(m_cbCardIndex[i],cbCardData,MAX_COUNT);
            msg.add_hand_card_count(cardCount);
            if(i==chairID)
            {
                for(uint32 k=0;k<cardCount;++k){
                    msg.add_hand_card_data(cbCardData[k]);
                }
            }
            msg.add_listen_states(m_cbListenStatus[i]);
			msg.add_uid(GetPlayerID(i));
			msg.add_trustee(m_bTrustee[i]);
        }

        msg.set_wait_time(m_coolLogic.getCoolTick());
        SendMsgToClient(chairID,&msg,net::S2C_MSG_MAJIANG_GAME_INFO);
    }
}

bool    CGameMaJiangTable::IsCanStart()
{
    for(uint8 i=0;i<m_wPlayerCount;++i)
    {
        if(m_vecPlayers[i].pPlayer == NULL || m_vecPlayers[i].readyState == 0)
		{
            return false;
        }
    }
    return true;
}
// 重置游戏数据
void    CGameMaJiangTable::ResetGameData()
{
    //游戏变量
    memset(m_siceCount,0,sizeof(m_siceCount));

    ZeroMemory(m_cbCardIndex,sizeof(m_cbCardIndex));
	ZeroMemory(m_bTrustee, sizeof(m_bTrustee));
	ZeroMemory(m_uTimeOutCount, sizeof(m_uTimeOutCount));

    //出牌信息
    m_cbOutCardData=0;
    m_cbOutCardCount=0;
    m_wOutCardUser=INVALID_CHAIR;
    ZeroMemory(m_cbDiscardCard,sizeof(m_cbDiscardCard));
    ZeroMemory(m_cbDiscardCount,sizeof(m_cbDiscardCount));
    m_cbGangCount=0;
    m_cbMustLeft =0;// 保留牌数
	ZeroMemory(m_cbArrOutCardCount, sizeof(m_cbArrOutCardCount));
	ZeroMemory(m_cbArrOperIndex, sizeof(m_cbArrOperIndex));
	m_cbPlayerOperIndex = 0;
    //发牌信息
    m_cbSendCardData=0;
    m_cbSendCardCount=0;
	m_wSendCardUser = INVALID_CHAIR;
    m_poolCards.clear();

    ZeroMemory(m_lGameScore,sizeof(m_lGameScore));
    ZeroMemory(m_lEachScore,sizeof(m_lEachScore));
	m_TempWeaveItem.Init();

    //运行变量
    m_cbProvideCard=0;
    m_wResumeUser=INVALID_CHAIR;
    m_wCurrentUser=INVALID_CHAIR;
    m_wProvideUser=INVALID_CHAIR;

    //状态变量
    m_bSendStatus=false;
    m_bGangStatus=false;
    ZeroMemory(m_bEnjoinChiHu,sizeof(m_bEnjoinChiHu));
    ZeroMemory(m_bEnjoinChiPeng,sizeof(m_bEnjoinChiPeng));

    //用户状态
    ResetUserOperInfo();
    ZeroMemory(m_cbListenStatus,sizeof(m_cbListenStatus));
	ZeroMemory(m_bArrTianTingStatus, sizeof(m_bArrTianTingStatus));
	ZeroMemory(m_bArrTingAfterMingGang, sizeof(m_bArrTingAfterMingGang));
	m_wOperHuCardPlayer = INVALID_CHAIR;
	for (uint32 i = 0; i < GAME_PLAYER; i++)
	{
		m_vecArrUserTingCard[i].clear();
		m_vecArrUserHuCard[i].clear();
		m_mpTingNotifyHu[i].clear();
	}
	
    //组合扑克
    ZeroMemory(m_WeaveItemArray,sizeof(m_WeaveItemArray));
    ZeroMemory(m_cbWeaveItemCount,sizeof(m_cbWeaveItemCount));

    //结束信息
    m_cbChiHuCard=0;
    m_fangPaoUser=INVALID_CHAIR;
    ZeroMemory(&m_ChiHuResult,sizeof(m_ChiHuResult));

    ZeroMemory(m_openingHu,sizeof(m_openingHu));
    ZeroMemory(m_hitBirdCount,sizeof(m_hitBirdCount));
    ZeroMemory(m_addHitBirdCount,sizeof(m_addHitBirdCount));

    m_allBirdCards.clear();

    m_tailCard=0;
    ZeroMemory(m_tailCardOper, sizeof(m_tailCardOper));

    //m_gangTingState = emGANG_TING_NULL;
    m_gangTingCards.clear();
    m_bChangeHandleCards = false;
	m_playType = MAJIANG_TYPE_NULL;

	ZeroMemory(m_iUserPassHuCount, sizeof(m_iUserPassHuCount));
	ZeroMemory(m_bArrGangFlowerStatus, sizeof(m_bArrGangFlowerStatus));

	m_stockTingCardZiMoPro = 0; // add by har
}
// 流局结束
void    CGameMaJiangTable::GameOverNoWin()
{
    m_cbChiHuCard=0;
    m_wProvideUser=INVALID_CHAIR;
    m_fangPaoUser=INVALID_CHAIR;
    OnGameEnd(m_wProvideUser,GER_NORMAL);

}
// 杠听后无人操作继续流程
void    CGameMaJiangTable::GangTingNoHuEnd(uint16 chairID)
{
    //m_gangTingState = emGANG_TING_NULL;
    m_bGangStatus   = false;
    for(uint8 i=0;i<m_gangTingCards.size();++i){
        m_cbDiscardCount[chairID]++;
        m_cbDiscardCard[chairID][m_cbDiscardCount[chairID]-1]=m_gangTingCards[i];
    }
    m_gangTingCards.clear();
    FlushDeskCardToClient();

    m_wCurrentUser  = GetNextUser(chairID);
    LOG_DEBUG("杠听后无人能胡---------------------,继续发牌:%d",m_wCurrentUser);
    DispatchCardData(m_wCurrentUser);

}
// 重置用户操作信息
void    CGameMaJiangTable::ResetUserOperInfo()
{
    ZeroMemory(m_bResponse,sizeof(m_bResponse));
    ZeroMemory(m_dwUserAction,sizeof(m_dwUserAction));
    ZeroMemory(m_cbOperateCard,sizeof(m_cbOperateCard));
    ZeroMemory(m_dwPerformAction,sizeof(m_dwPerformAction));
}
//获得已经出牌数量(胡绝张)
uint32    CGameMaJiangTable::GetShowCardNum(BYTE card)
{
    uint32 count=0;
    for(uint32 i=0;i<m_wPlayerCount;++i){
        //出掉的牌
        for(uint32 j=0;j<m_cbDiscardCount[i];++j){
            if(m_cbDiscardCard[i][j]==card){
                count++;
            }
        }
        //吃碰杠的牌
        for(uint32 j=0;j<m_cbWeaveItemCount[i];++j)
        {
            stWeaveItem& item=m_WeaveItemArray[i][j];
            switch(item.WeaveKind)
            {
            case ACTION_GANG:
                {
                    if(item.cbCenterCard==card && item.cbPublicCard==TRUE){
                        count+=4;
                    }
                }break;
            case ACTION_PENG:
                {
                    if(item.cbCenterCard==card){
                        count+=3;
                    }
                }break;
            default:
                {
                    BYTE tmp = m_gameLogic.GetEatCenterCard(item.WeaveKind,item.cbCenterCard);
                    if(tmp==card || (tmp+1)==card || (tmp-1)==card){
                        count++;
                    }
                }break;
            }
        }
    }
    return count;
}
//获得牌池牌数量
uint32    CGameMaJiangTable::GetCardPoolSize()
{
    return m_poolCards.size();
}
// 用户出牌
bool    CGameMaJiangTable::OnUserOutCard(uint16 chairID,BYTE cardData,bool bListen)
{
	LOG_DEBUG("用户出牌 - uid:%d,chaidID:%d,cardData:0x%02X ", GetPlayerID(chairID), chairID, cardData);

	if (chairID >= m_wPlayerCount)
	{
		return false;
	}
    //效验参数
    if(chairID!=m_wCurrentUser || m_wOutCardUser != INVALID_CHAIR)
	{
        //LOG_DEBUG("非当前用户,不能出牌:%d--%d",m_wCurrentUser,chairID);

		string strHandCardIndex;
		int iHandCardCount = 0;
		for (unsigned int j = 0; j < MAX_INDEX; ++j)
		{
			strHandCardIndex += CStringUtility::FormatToString("%d, ", m_cbCardIndex[chairID][j]);
			iHandCardCount += m_cbCardIndex[chairID][j];
		}

		LOG_DEBUG("out_card_error - uid:%d - %d %d,chairID:%d,m_wCurrentUser:%d,cbOutCardData:0x%02X,iHandCardCount:%d,strHandCardIndex:%s",
			GetPlayerID(chairID), GetPlayerID(0), GetPlayerID(1), chairID, m_wCurrentUser, cardData, iHandCardCount, strHandCardIndex.c_str());


        return false;
    }
    if(m_gameLogic.IsValidCard(cardData)==false || GetCardCount(chairID,cardData)==0)
	{
        //LOG_DEBUG("出牌数据非法:%s--%d",m_gameLogic.GetCardName(cardData).c_str(),GetCardCount(chairID,cardData));

		string strHandCardIndex;
		int iHandCardCount = 0;
		for (unsigned int j = 0; j < MAX_INDEX; ++j)
		{
			strHandCardIndex += CStringUtility::FormatToString("%d, ", m_cbCardIndex[chairID][j]);
			iHandCardCount += m_cbCardIndex[chairID][j];
		}

		LOG_DEBUG("out_card_error - uid:%d - %d %d,chairID:%d,m_wCurrentUser:%d,cbOutCardData:0x%02X,iHandCardCount:%d,strHandCardIndex:%s",
			GetPlayerID(chairID), GetPlayerID(0), GetPlayerID(1), chairID, m_wCurrentUser, cardData, iHandCardCount, strHandCardIndex.c_str());


        return false;
    }
    //听牌状态只能出摸的牌
    if(m_cbListenStatus[chairID])
	{
        if(cardData != m_cbSendCardData)
		{
            LOG_DEBUG("听牌状态下,只能出摸的牌");
            return false;
        }
    }
	if (bListen)
	{
		m_cbListenStatus[chairID] = TRUE;
		NotifyListen(chairID);

		bool bIsHaveGang = false;
		for (uint32 i = 0; i < m_cbWeaveItemCount[chairID]; i++)
		{
			if (m_WeaveItemArray[chairID][i].WeaveKind == ACTION_GANG)
			{
				bIsHaveGang = true;
				break;
			}
		}
		if (bIsHaveGang == false)
		{
			if (m_cbArrOutCardCount[chairID] == 0 && m_wBankerUser == chairID)
			{
				m_bArrTianTingStatus[chairID] = true;
			}
			if (m_cbArrOutCardCount[chairID] == 0 && m_wBankerUser != chairID)
			{
				if (cardData == m_cbSendCardData)
				{
					m_bArrTianTingStatus[chairID] = true;
				}
			}
		}
	}

	//if (m_cbListenStatus[chairID] == FALSE)
	//{
	//	// 没听牌 点过了 需要重新计算 是否出的是可以出的牌
	//	map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
	//	DWORD tmp = m_gameLogic.AnalyseTingCard15(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], mpTingNotifyHu);
	//	if (mpTingNotifyHu.size() > 0)
	//	{
	//		auto iter = mpTingNotifyHu.find(cardData);
	//		if (iter== mpTingNotifyHu.end())
	//		{
	//			m_mpTingNotifyHu[chairID].clear();
	//		}
	//	}
	//}
	if (bListen)
	{
		// 第一次听牌 需要保留数据 不再变
		auto iter = m_mpTingNotifyHu[chairID].find(cardData);
		if (iter != m_mpTingNotifyHu[chairID].end())
		{
			vector<tagAnalyseTingNotifyHu> vecNotifyHu = iter->second;
			m_mpTingNotifyHu[chairID].clear();
			m_mpTingNotifyHu[chairID].insert(make_pair(cardData, vecNotifyHu));
		}
		else
		{
			m_mpTingNotifyHu[chairID].clear();
		}
	}

	if (bListen==false && m_cbListenStatus[chairID] == TRUE)
	{
		// 不是第一次听牌 	更新胡牌数量
	}
	if (m_cbListenStatus[chairID] == FALSE)
	{
		// 可以听牌但是不点听牌
		auto iter = m_mpTingNotifyHu[chairID].find(cardData);
		if (iter != m_mpTingNotifyHu[chairID].end())
		{
			vector<tagAnalyseTingNotifyHu> vecNotifyHu = iter->second;
			m_mpTingNotifyHu[chairID].clear();
			m_mpTingNotifyHu[chairID].insert(make_pair(cardData, vecNotifyHu));
		}
		else
		{
			m_mpTingNotifyHu[chairID].clear();
		}
	}
    //删除扑克
    if(m_gameLogic.RemoveCard(m_cbCardIndex[chairID],cardData)==false)
    {
        LOG_DEBUG("出牌错误,没有这张牌:%x",cardData);
        return false;
    }
	BYTE cbArrOutCard[] = { cardData };
	WriteOutCardLog(chairID, OPERATE_CODE_OUT_CARD, cbArrOutCard, 1);
	memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));
	
	//DWORD tmp = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], m_vecTingNotifyHu[chairID]);

	// 出牌之后只留出牌之后的那个
	//UpdateNotifyHuCard(chairID, cardData);
	UpdateNotifyHuCardCount(cardData, 1);
    //设置变量
	m_TempWeaveItem.Init();
    m_bSendStatus=true;
    m_bEnjoinChiHu[chairID]=true;
	m_bArrGangFlowerStatus[chairID] = false;
    for(WORD i=0;i<m_wPlayerCount;i++)
    {
		if (i == chairID)
		{
			continue;
		}
		else
		{
			m_bEnjoinChiHu[i] = false;
		}
    }
    m_dwUserAction[chairID] = ACTION_NULL;
    m_dwPerformAction[chairID] = ACTION_NULL;

    //出牌记录
    m_cbOutCardCount++;
    m_wOutCardUser  = chairID;
    m_cbOutCardData = cardData;
    m_fangPaoUser=chairID;
	m_cbArrOutCardCount[chairID]++;
	m_wLastOutCardUser = chairID;

    //广播出牌
    net::msg_majiang_out_card_rep outCardMsg;
    outCardMsg.set_out_card_data(cardData);
    outCardMsg.set_out_card_user(chairID);

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		net::mjaction_out_front_notify_hu * pout_front_notify_hu = outCardMsg.add_notify_hu();

		LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,cardData:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
			GetPlayerID(i), m_cbListenStatus[i], m_wCurrentUser, cardData, m_mpTingNotifyHu[i].size());

		auto iter_begin = m_mpTingNotifyHu[i].begin();
		for (; iter_begin != m_mpTingNotifyHu[i].end(); iter_begin++)
		{
			pout_front_notify_hu->add_out_card(iter_begin->first);
			net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

			vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
			for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
			{
				pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
				pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
				pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

				LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_wCurrentUser:%d,operCard:0x%02X,out_card:0x%02X - hu_card:0x%02X",
					GetPlayerID(i), m_wCurrentUser, cardData, iter_begin->first, vecTingNotifyHu[uIndex].cbCard);

				vector<BYTE> actions;
				vecTingNotifyHu[uIndex].chiHuResult.HuKind.getAllActions(actions);
				for (uint8 j = 0; j<actions.size(); ++j) {
					LOG_DEBUG("ting_card_m_mpTingNotifyHu_hu_card_type - chairID:%d,uid:%d,j:%d,kind:%d", chairID, GetPlayerID(chairID), j, actions[j]);
				}
				vecTingNotifyHu[uIndex].chiHuResult.HuKind.getAllTypeFan(actions);
				for (uint8 j = 0; j<actions.size(); ++j) {
					LOG_DEBUG("ting_card_m_mpTingNotifyHu_hu_card_score - chairID:%d,uid:%d,j:%d,fan:%d", chairID, GetPlayerID(chairID), j, actions[j]);
				}
			}
		}

		SendMsgToClient(i, &outCardMsg, net::S2C_MSG_MAJIANG_OUT_CARD_REP);
		pout_front_notify_hu->clear_out_card();
		pout_front_notify_hu->clear_notify_hu();
	}
	

    //SendMsgToAll(&outCardMsg,S2C_MSG_MAJIANG_OUT_CARD_REP);

    LOG_DEBUG("通知玩家出牌 - uid:%d,chaidID:%d,cardData:0x%02X - %s", GetPlayerID(chairID),chairID, cardData,m_gameLogic.GetCardName(cardData).c_str());

    //用户切换
    m_wProvideUser  = chairID;
    m_cbProvideCard = cardData;
    m_wCurrentUser  = GetNextUser(chairID);
	m_wSendCardUser = INVALID_CHAIR;

    //响应判断
    bool bAroseAction = EstimateUserRespond(chairID,cardData,EstimatKind_OutCard);

    //抢杆设置
    if(m_bGangStatus==true)
    {
        WORD i=0;
        for(i=0;i<m_wPlayerCount;i++)
        {
            if((m_dwUserAction[i]&ACTION_CHI_HU)!=0) break;
        }
        if(i==m_wPlayerCount)
        {
            m_bGangStatus=false;
            LOG_DEBUG("杠牌后无人胡,取消杠状态:%d",m_wCurrentUser);
        }
    }
    //派发扑克
    if(bAroseAction==false) {
        LOG_DEBUG("出牌后无人有操作,继续发牌:%d",m_wCurrentUser);
        DispatchCardData(m_wCurrentUser);
    }

    ResetCoolTime();
    return true;
}
bool    CGameMaJiangTable::OnUserNotifyHu(uint16 chairID)
{
	for (uint32 i = 0; i < m_wPlayerCount; ++i)
	{
		if (m_cbListenStatus[i] == FALSE)
		{
			DWORD tmp = m_gameLogic.AnalyseTingCard15(i, m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], m_mpTingNotifyHu[i]);
		}
	}
	net::msg_mjaction_oper_after_notify_hu_rep msg_notify_hu;
	msg_notify_hu.set_oper_chairid(chairID);
	for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
	{
		net::mjaction_out_front_notify_hu * pout_front_notify_hu = msg_notify_hu.add_notify_hu();

		LOG_DEBUG("ting_card_m_mpTingNotifyHu msg_notify_hu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,m_mpTingNotifyHu.size:%d ----------------",
			GetPlayerID(uIndex), m_cbListenStatus[uIndex], m_wCurrentUser, m_mpTingNotifyHu[uIndex].size());

		auto iter_begin = m_mpTingNotifyHu[uIndex].begin();
		for (; iter_begin != m_mpTingNotifyHu[uIndex].end(); iter_begin++)
		{
			pout_front_notify_hu->add_out_card(iter_begin->first);
			net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

			vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
			for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
			{
				pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
				pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
				pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

				LOG_DEBUG("ting_card_m_mpTingNotifyHu msg_notify_hu - uid:%d,m_wCurrentUser:%d,out_card:0x%02X - hu_card:0x%02X,cbCount:%d",
					GetPlayerID(uIndex), m_wCurrentUser, iter_begin->first, vecTingNotifyHu[uIndex].cbCard, vecTingNotifyHu[uIndex].cbCount);
			}
		}

		SendMsgToClient(uIndex, &msg_notify_hu, net::S2C_MSG_MAJIANG_OPER_AFTER_NOTIFY_HU_REP);

		pout_front_notify_hu->clear_out_card();
		pout_front_notify_hu->clear_notify_hu();

	}

	//SendMsgToAll(&msg_notify_hu, S2C_MSG_MAJIANG_OPER_AFTER_NOTIFY_HU_REP);

	return true;
}

bool    CGameMaJiangTable::OnUserOutCardNotifyHu(uint16 chairID)
{
	for (uint32 i = 0; i < m_wPlayerCount; ++i)
	{
		if (chairID == i)
		{
			DWORD tmp = m_gameLogic.AnalyseTingCard19(i, m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], m_vecTingNotifyHu[i]);
		}
	}
	net::mjaction_out_after_notify_hu_rep msg_notify_hu;
	msg_notify_hu.set_last_out_card_user(m_wLastOutCardUser);
	for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
	{
		if (chairID != uIndex)
		{
			continue;
		}
		net::mjaction_notify_hu* pnotify_hu = msg_notify_hu.add_notify_hu();
		for (uint32 uIndexNotify = 0; uIndexNotify < m_vecTingNotifyHu[uIndex].size(); uIndexNotify++)
		{
			pnotify_hu->add_card(m_vecTingNotifyHu[uIndex][uIndexNotify].cbCard);
			pnotify_hu->add_score(m_vecTingNotifyHu[uIndex][uIndexNotify].score);
			pnotify_hu->add_count(m_vecTingNotifyHu[uIndex][uIndexNotify].cbCount);

			LOG_DEBUG("ting_card_m_TingNotifyHu msg_notify_hu - uid:%d,m_wCurrentUser:%d,hu_card:0x%02X,cbCount:%d",
				GetPlayerID(uIndex), m_wCurrentUser, m_vecTingNotifyHu[uIndex][uIndexNotify].cbCard, m_vecTingNotifyHu[uIndex][uIndexNotify].cbCount);
		}

		SendMsgToClient(uIndex, &msg_notify_hu, net::S2C_MSG_MAJIANG_OUT_AFTER_NOTIFY_HU_REP);

		//net::mjaction_out_front_notify_hu * pout_front_notify_hu = msg_notify_hu.add_notify_hu();
		//LOG_DEBUG("ting_card_m_TingNotifyHu msg_notify_hu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,m_mpTingNotifyHu.size:%d ----------------",
		//	GetPlayerID(uIndex), m_cbListenStatus[uIndex], m_wCurrentUser, m_mpTingNotifyHu[uIndex].size());
		//auto iter_begin = m_mpTingNotifyHu[uIndex].begin();
		//for (; iter_begin != m_mpTingNotifyHu[uIndex].end(); iter_begin++)
		//{
		//	pout_front_notify_hu->add_out_card(iter_begin->first);
		//	net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();
		//	vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
		//	for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
		//	{
		//		pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
		//		pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
		//		pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);
		//		LOG_DEBUG("ting_card_m_TingNotifyHu msg_notify_hu - uid:%d,m_wCurrentUser:%d,out_card:0x%02X - hu_card:0x%02X,cbCount:%d",
		//			GetPlayerID(uIndex), m_wCurrentUser, iter_begin->first, vecTingNotifyHu[uIndex].cbCard, vecTingNotifyHu[uIndex].cbCount);
		//	}
		//}


		//pout_front_notify_hu->clear_out_card();
		//pout_front_notify_hu->clear_notify_hu();

	}

	//SendMsgToAll(&msg_notify_hu, S2C_MSG_MAJIANG_OPER_AFTER_NOTIFY_HU_REP);

	return true;
}

void CGameMaJiangTable::AddUserPassHuCount(uint16 chairID)
{
	memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));
	if (m_iUserPassHuCount[chairID] < 3)
	{
		m_iUserPassHuCount[chairID]++;
		m_iUserPassHuFlag[chairID] = 1;
	}
}

// 用户操作
bool    CGameMaJiangTable::OnUserOperCard(uint16 chairID,uint32 operCode,BYTE operCard)
{
	vector<BYTE> allActions;
	if (chairID < m_wPlayerCount)
	{
		m_gameLogic.GetAllAction(m_dwUserAction[chairID], allActions);
	}
	string strAllActions;
	for (unsigned int a = 0; a < allActions.size(); ++a)
	{
		strAllActions += CStringUtility::FormatToString("%d ", allActions[a]);
	}

	LOG_DEBUG("用户操作 - uid:%d,chairID:%d,operCode:%d,m_wCurrentUser:%d,operCard:0x%02X,strAllActions:%s",	GetPlayerID(chairID), chairID, operCode, m_wCurrentUser, operCard, strAllActions.c_str());
    //效验用户
	if ((chairID != m_wCurrentUser) && (m_wCurrentUser != INVALID_CHAIR))
	{
		return false;
	}
    //效验状态
	if (m_gameLogic.IsValidCard(operCard) == false && operCode!= ACTION_NULL)
	{
		LOG_DEBUG("操作数据非法 - uid:%d,operCard:0x%02X,operCode:%d",GetPlayerID(chairID), operCard, operCode);

		return false;
	}
    if(m_bResponse[chairID]==true){
        LOG_DEBUG("已经操作过了 uid:%d,%d", GetPlayerID(chairID), chairID);
        return true;
    }
    if((operCode!=ACTION_NULL)&&((m_dwUserAction[chairID]&operCode)==0)){
        LOG_DEBUG("没有这个操作 uid:%d,:%d--%d", GetPlayerID(chairID), chairID,operCode);
        return true;
    }
	OnUserNotifyHu(chairID);
	bool bIsPassHuOper = false;
	if (operCode == ACTION_NULL && (m_dwUserAction[chairID] & ACTION_CHI_HU) != 0)
	{
		AddUserPassHuCount(chairID);
		bIsPassHuOper = true;
	}

    //被动动作
    if(m_wCurrentUser==INVALID_CHAIR)
    {

        //变量定义
        WORD wTargetUser    = chairID;
        DWORD TargetAction  = operCode;

        //设置变量
        m_bResponse[chairID]=true;
        m_dwPerformAction[chairID] = operCode;
        m_cbOperateCard[chairID]=m_cbProvideCard;

        //起手胡处理
        //if(OnUserOpenHuOper(chairID,operCode))return true;
        //海底牌处理
        //if(OnUserNeedTailOper(chairID,operCode))return true;

        //执行判断
        for(WORD i=0;i<m_wPlayerCount;i++)
        {
            //获取动作
            DWORD UserAction=(m_bResponse[i]==false)?m_dwUserAction[i]:m_dwPerformAction[i];

            //优先级别
            BYTE cbUserActionRank       =   m_gameLogic.GetUserActionRank(UserAction);
            BYTE cbTargetActionRank     =   m_gameLogic.GetUserActionRank(TargetAction);

            //动作判断
            if (cbUserActionRank>cbTargetActionRank)
            {
                wTargetUser=i;
                TargetAction=UserAction;
            }
        }

		LOG_DEBUG("被动操作 - uid:%d,m_wCurrentUser:%d,chairID:%d,operCode:%d,wTargetUser:%d,TargetAction:%d,m_bResponse:%d,",
			GetPlayerID(chairID), m_wCurrentUser, chairID, operCode, wTargetUser, TargetAction, m_bResponse[wTargetUser]);


        if(m_bResponse[wTargetUser]==false)	return true;

        //吃胡等待
        if(TargetAction==ACTION_CHI_HU)
        {
            for(WORD i=0;i<m_wPlayerCount;i++)
            {
				LOG_DEBUG("operuid:%d,uid:%d,i:%d,m_bResponse:%d,m_dwUserAction:%d", GetPlayerID(chairID),GetPlayerID(i), i, m_bResponse[i], m_dwUserAction[i]);

				if ((m_bResponse[i] == false) && (m_dwUserAction[i] & ACTION_CHI_HU))
				{
					return true;
				}
            }
        }
        //吃胡执行判断
        if(TargetAction==ACTION_CHI_HU)
        {
            for(WORD i=0;i<m_wPlayerCount;i++)
            {
                //吃牌判断
                WORD wFirstUser=(m_wProvideUser+i+1)%m_wPlayerCount;

				LOG_DEBUG("operuid:%d,uid:%d,wFirstUser:%d,m_bResponse:%d,m_dwPerformAction:%d", GetPlayerID(chairID), GetPlayerID(wFirstUser), wFirstUser, m_bResponse[wFirstUser], m_dwPerformAction[wFirstUser]);

                if(m_dwPerformAction[wFirstUser]&ACTION_CHI_HU)
                {
                    //数据校验
                    if((m_bResponse[wFirstUser] == false)) return false;
                    //找到首家用户 就退出循环
                    wTargetUser = wFirstUser;
                    break;
                }
            }
        }
        //放弃操作
        if(TargetAction==ACTION_NULL)
        {
            //if(m_gangTingState == emGANG_TING_SELF_HU){//杠听胡放弃
            //    EstimateGangTingQiang(m_wResumeUser);
            //    return true;
            //}
            //if(m_gangTingState == emGANG_TING_QIANG){//抢杠胡放弃
            //    EstimateGangTing(m_wResumeUser);
            //    return true;
            //}
            //if(m_gangTingState == emGANG_TING_OTHER_HU){//三方杠听胡放弃
            //    GangTingNoHuEnd(m_wResumeUser);
            //    return true;
            //}

			LOG_DEBUG("放弃操作 - uid:%d,chairID:%d,wTargetUser:%d,m_wResumeUser:%d,WeaveKind:%d,wOperateUser:%d,cbCenterCard:0x%02X,m_dwUserAction:%d",
				GetPlayerID(chairID), chairID, wTargetUser, m_wResumeUser, m_TempWeaveItem.WeaveKind, m_TempWeaveItem.wOperateUser, m_TempWeaveItem.cbCenterCard, m_dwUserAction[chairID]);

			// 如果不胡检查上次是否有玩家杠牌 如果有则继续杠牌
			if (m_TempWeaveItem.WeaveKind == ACTION_GANG && (m_dwUserAction[chairID]&ACTION_CHI_HU) != 0)
			{
				// 完成上一次的杠操作
				BYTE cbCardIndex = m_gameLogic.SwitchToCardIndex(m_TempWeaveItem.cbCenterCard);
				BYTE cbWeaveIndex = 0xFF;
				DWORD dwConcreteAction = 0;
				if (m_TempWeaveItem.wOperateUser >= m_wPlayerCount || cbCardIndex > MAX_INDEX)
				{
					LOG_DEBUG("放弃操作错误 - uid:%d,chairID:%d,wTargetUser:%d,m_wResumeUser:%d,wOperateUser:%d,cbCardIndex:%d",
						GetPlayerID(chairID), chairID, wTargetUser, m_wResumeUser, m_TempWeaveItem.wOperateUser, cbCardIndex);

					return false;
				}

				//杠牌处理
				if (m_cbCardIndex[m_TempWeaveItem.wOperateUser][cbCardIndex] == 1)
				{
					//寻找组合
					for (BYTE i = 0; i < m_cbWeaveItemCount[m_TempWeaveItem.wOperateUser]; i++)
					{
						DWORD WeaveKind = m_WeaveItemArray[m_TempWeaveItem.wOperateUser][i].WeaveKind;
						BYTE cbCenterCard = m_WeaveItemArray[m_TempWeaveItem.wOperateUser][i].cbCenterCard;
						if ((cbCenterCard == m_TempWeaveItem.cbCenterCard) && (WeaveKind == ACTION_PENG))
						{
							cbWeaveIndex = i;
							dwConcreteAction = 13;
							break;
						}
					}

					//效验动作
					//LOG_ERROR("cbWeaveIndex is null");
					if (cbWeaveIndex == 0xFF)
					{
						LOG_DEBUG("放弃操作错误 - uid:%d,chairID:%d,wTargetUser:%d,m_wResumeUser:%d,wOperateUser:%d,cbCardIndex:%d",
							GetPlayerID(chairID), chairID, wTargetUser, m_wResumeUser, m_TempWeaveItem.wOperateUser, cbCardIndex);

						return false;
					}
					m_WeaveItemArray[m_TempWeaveItem.wOperateUser][cbWeaveIndex].cbPublicCard = m_TempWeaveItem.cbPublicCard;
					m_WeaveItemArray[m_TempWeaveItem.wOperateUser][cbWeaveIndex].wProvideUser = m_TempWeaveItem.wProvideUser;
					m_WeaveItemArray[m_TempWeaveItem.wOperateUser][cbWeaveIndex].WeaveKind = m_TempWeaveItem.WeaveKind;
					m_WeaveItemArray[m_TempWeaveItem.wOperateUser][cbWeaveIndex].cbCenterCard = m_TempWeaveItem.cbCenterCard;

					//杠牌数目
					m_cbGangCount++;

					//删除扑克
					m_cbCardIndex[m_TempWeaveItem.wOperateUser][cbCardIndex] = 0;

					UpdateNotifyHuCardCount(m_TempWeaveItem.wOperateUser, m_TempWeaveItem.cbCenterCard, 4);

					//设置状态
					//if(operCode==ACTION_GANG)
					m_bGangStatus = true;
					m_bArrGangFlowerStatus[m_TempWeaveItem.wOperateUser] = true;
					
					net::msg_majiang_oper_result_rep operResultRep;
					operResultRep.set_oper_user(m_TempWeaveItem.wOperateUser);
					operResultRep.set_provide_user(m_TempWeaveItem.wProvideUser);
					net::mjaction* pa = operResultRep.mutable_action();
					pa->set_card(m_TempWeaveItem.cbCenterCard);
					//pa->set_code(m_gameLogic.GetOneAction(m_TempWeaveItem.WeaveKind));
					if (m_TempWeaveItem.WeaveKind == ACTION_GANG)
					{
						pa->set_code(dwConcreteAction);
					}
					else
					{
						pa->set_code(m_gameLogic.GetOneAction(m_TempWeaveItem.WeaveKind));
					}
					for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
					{
						operResultRep.add_passhu_count(m_iUserPassHuCount[uIndex]);

						//if (uIndex == chairID)
						//{
						//	if (bIsPassHuOper)
						//	{
						//		operResultRep.add_passhu_count(m_iUserPassHuCount[chairID]);
						//	}
						//	else
						//	{
						//		operResultRep.add_passhu_count(-1);
						//	}
						//}
						//else
						//{
						//	operResultRep.add_passhu_count(-1);
						//}
					}

					//SendMsgToAll(&operResultRep, S2C_MSG_MAJIANG_OPER_RESULT_REP);

					//提示胡牌
					if (m_cbListenStatus[m_TempWeaveItem.wOperateUser] == FALSE)
					{
						DWORD tmp = m_gameLogic.AnalyseTingCard15(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], m_mpTingNotifyHu[chairID]);
					}

					for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
					{
						net::mjaction_out_front_notify_hu * pout_front_notify_hu = operResultRep.add_notify_hu();

						LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,operCard:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
							GetPlayerID(uIndex), m_cbListenStatus[uIndex], m_wCurrentUser, operCard, m_mpTingNotifyHu[uIndex].size());

						auto iter_begin = m_mpTingNotifyHu[uIndex].begin();
						for (; iter_begin != m_mpTingNotifyHu[uIndex].end(); iter_begin++)
						{
							pout_front_notify_hu->add_out_card(iter_begin->first);
							net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

							vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
							for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
							{
								pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
								pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
								pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

								LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_wCurrentUser:%d,operCard:0x%02X,out_card:0x%02X - hu_card:0x%02X,cbCount:%d",
									GetPlayerID(uIndex), m_wCurrentUser, operCard, iter_begin->first, vecTingNotifyHu[uIndex].cbCard, vecTingNotifyHu[uIndex].cbCount);
							}
						}

						SendMsgToClient(uIndex, &operResultRep, net::S2C_MSG_MAJIANG_OPER_RESULT_REP);
						pout_front_notify_hu->clear_out_card();
						pout_front_notify_hu->clear_notify_hu();
					}

					GangPaiOper(m_TempWeaveItem.wOperateUser, m_TempWeaveItem.cbCenterCard, m_TempWeaveItem.cbPublicCard);

					LOG_DEBUG("继续杠操作 - uid:%d,m_wCurrentUser:%d,chairID:%d,operCode:%d,wTargetUser:%d,TargetAction:%d,m_bResponse:%d,wOperateUser:%d,m_fangPaoUser:%d",
						GetPlayerID(chairID), m_wCurrentUser, chairID, operCode, wTargetUser, TargetAction, m_bResponse[wTargetUser], m_TempWeaveItem.wOperateUser, m_fangPaoUser);

					m_TempWeaveItem.Init();
					//m_fangPaoUser = INVALID_CHAIR;

					BYTE cbArrPassCard[] = { operCard };
					WriteOutCardLog(chairID, OPERATE_CODE_NULL, cbArrPassCard, 1);

					ResetUserOperInfo();

					if (m_TempWeaveItem.cbPublicCard==TRUE)
					{
						BYTE cbMingGangRemoveCard[] = { m_TempWeaveItem.cbCenterCard,m_TempWeaveItem.cbCenterCard,m_TempWeaveItem.cbCenterCard };
						WriteOutCardLog(m_TempWeaveItem.wOperateUser, OPERATE_CODE_AN_GANG, cbMingGangRemoveCard, 3);
					}
					else
					{
						BYTE cbAnGangRemoveCard[] = { m_TempWeaveItem.cbCenterCard,m_TempWeaveItem.cbCenterCard,m_TempWeaveItem.cbCenterCard,m_TempWeaveItem.cbCenterCard };
						WriteOutCardLog(m_TempWeaveItem.wOperateUser, OPERATE_CODE_AN_GANG, cbAnGangRemoveCard, 4);
					}
					memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

					return true;
				}
			}
			
            //用户状态
			BYTE cbArrPassCard[] = { operCard };
			WriteOutCardLog(chairID, OPERATE_CODE_NULL, cbArrPassCard, 1);
			memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

            ResetUserOperInfo();
			
            //m_gangTingState=emGANG_TING_NULL;
            //发送扑克
            LOG_DEBUG("大家放弃操作,发牌 - uid:%d,chairID:%d,wTargetUser:%d,m_wResumeUser:%d", GetPlayerID(chairID), chairID,wTargetUser,m_wResumeUser);
            DispatchCardData(m_wResumeUser);
            //FlushDeskCardToClient();

			//ResetCoolTime();
            return true;
        }

        //变量定义
        BYTE cbTargetCard=m_cbOperateCard[wTargetUser];

        //出牌变量
        m_cbOutCardData=0;
        m_bSendStatus=true;
        m_wOutCardUser=INVALID_CHAIR;
        m_bEnjoinChiHu[wTargetUser]=false;

        //胡牌操作
        if(TargetAction==ACTION_CHI_HU)
        {
			BYTE cbArrHuCard[] = { operCard };
			WriteOutCardLog(wTargetUser, OPERATE_CODE_CHI_HU, cbArrHuCard, 1);
			memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

            ChiHuOper(wTargetUser,false);
            return true;
        }

        ResetUserOperInfo();

		LOG_DEBUG("uid:%d,chairID:%d,operCode:%d,operCard:0x%02X,wTargetUser:%d,wTargetUserUID:%d,m_cbWeaveItemCount:%d",GetPlayerID(chairID), chairID, operCode, operCard, wTargetUser, GetPlayerID(wTargetUser), m_cbWeaveItemCount[wTargetUser]);
        //组合扑克
        ASSERT(m_cbWeaveItemCount[wTargetUser]<MAX_WEAVE);
        WORD wIndex=m_cbWeaveItemCount[wTargetUser]++;
        m_WeaveItemArray[wTargetUser][wIndex].cbPublicCard=TRUE;
        m_WeaveItemArray[wTargetUser][wIndex].cbCenterCard=cbTargetCard;
        m_WeaveItemArray[wTargetUser][wIndex].WeaveKind=TargetAction;
        m_WeaveItemArray[wTargetUser][wIndex].wProvideUser=(m_wProvideUser==INVALID_CHAIR)?wTargetUser:m_wProvideUser;
		DWORD dwConcreteAction = 0;
        //删除扑克
        switch (TargetAction)
        {
        case ACTION_EAT_LEFT:		//左吃操作
            {
                //删除扑克
                BYTE cbRemoveCard[]={BYTE(cbTargetCard+1),BYTE(cbTargetCard+2)};
                m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard + 1, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard + 2, 1);
				WriteOutCardLog(wTargetUser, OPERATE_CODE_EAT_LEFT, cbRemoveCard, 2);

            }break;
        case ACTION_EAT_RIGHT:		//右吃操作
            {
                //删除扑克
                BYTE cbRemoveCard[]={BYTE(cbTargetCard-2),BYTE(cbTargetCard-1)};
                m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard - 1, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard - 2, 1);
				WriteOutCardLog(wTargetUser, OPERATE_CODE_EAT_RIGHT, cbRemoveCard, 2);

            }break;
        case ACTION_EAT_CENTER:	    //中吃操作
            {
                //删除扑克
                BYTE cbRemoveCard[]={BYTE(cbTargetCard-1),BYTE(cbTargetCard+1)};
                m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard - 1, 1);
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard + 1, 1);
				WriteOutCardLog(wTargetUser, OPERATE_CODE_EAT_CENTER, cbRemoveCard, 2);

            }break;
        case ACTION_PENG:		    //碰牌操作
            {
                //删除扑克
                BYTE cbRemoveCard[]={cbTargetCard,cbTargetCard};
                m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
				UpdateNotifyHuCardCount(wTargetUser, cbTargetCard, 3);
				WriteOutCardLog(wTargetUser, OPERATE_CODE_PENG, cbRemoveCard, 2);

            }break;
        case ACTION_GANG:		//杠牌操作
        case ACTION_GANG_TING:  //杠听操作
            {
				UpdateNotifyHuCardCount(wTargetUser,cbTargetCard, 4);

				BYTE cbRemoveCard[] = { cbTargetCard,cbTargetCard,cbTargetCard };
                //杠牌设置
                if((m_cbSendCardCount==1)&&(m_cbOutCardData==0))
                {
                    //删除扑克
                    //BYTE cbRemoveCard[]={cbTargetCard,cbTargetCard,cbTargetCard};
                    m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
                }
                else
                {
                    //删除扑克
                    //BYTE cbRemoveCard[]={cbTargetCard,cbTargetCard,cbTargetCard};
                    m_gameLogic.RemoveCard(m_cbCardIndex[wTargetUser],cbRemoveCard,getArrayLen(cbRemoveCard));
                }
				dwConcreteAction = 11;
				BYTE cbAnGangRemoveCard[] = { cbTargetCard,cbTargetCard,cbTargetCard };
				WriteOutCardLog(wTargetUser, OPERATE_CODE_MING_GANG, cbAnGangRemoveCard, 3);
				m_bArrGangFlowerStatus[wTargetUser] = true;
                m_cbGangCount++;
            }break;

        }
		memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

        net::msg_majiang_oper_result_rep operResMsg;
        net::mjaction* pa = operResMsg.mutable_action();
        pa->set_card(cbTargetCard);
        //pa->set_code(m_gameLogic.GetOneAction(TargetAction));
		if (TargetAction == ACTION_GANG)
		{
			pa->set_code(dwConcreteAction);
		}
		else
		{
			pa->set_code(m_gameLogic.GetOneAction(TargetAction));
		}
		for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
		{
			operResMsg.add_passhu_count(m_iUserPassHuCount[uIndex]);
			//if (uIndex == chairID)
			//{
			//	if (bIsPassHuOper)
			//	{
			//		operResMsg.add_passhu_count(m_iUserPassHuCount[chairID]);
			//	}
			//	else
			//	{
			//		operResMsg.add_passhu_count(-1);
			//	}
			//}
			//else
			//{
			//	operResMsg.add_passhu_count(-1);
			//}
		}
		//if (bIsPassHuOper)
		//{
		//	operResMsg.set_passhu_count(m_iUserPassHuCount[chairID]);
		//}
		//else
		//{
		//	operResMsg.set_passhu_count(-1);
		//}
        operResMsg.set_oper_user(wTargetUser);
        operResMsg.set_provide_user((m_wProvideUser==INVALID_CHAIR)?wTargetUser:m_wProvideUser);

		//提示胡牌
		if (m_cbListenStatus[wTargetUser] == FALSE)
		{
			DWORD tmp = m_gameLogic.AnalyseTingCard15(wTargetUser, m_cbCardIndex[wTargetUser], m_WeaveItemArray[wTargetUser], m_cbWeaveItemCount[wTargetUser], m_mpTingNotifyHu[wTargetUser]);
		}

		for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
		{
			net::mjaction_out_front_notify_hu * pout_front_notify_hu = operResMsg.add_notify_hu();

			LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,operCard:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
				GetPlayerID(uIndex), m_cbListenStatus[uIndex], m_wCurrentUser, operCard, m_mpTingNotifyHu[uIndex].size());


			auto iter_begin = m_mpTingNotifyHu[uIndex].begin();
			for (; iter_begin != m_mpTingNotifyHu[uIndex].end(); iter_begin++)
			{
				pout_front_notify_hu->add_out_card(iter_begin->first);
				net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

				vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
				for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
				{
					pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
					pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
					pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

					LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_wCurrentUser:%d,operCard:0x%02X,out_card:0x%02X - hu_card:0x%02X,cbCount:%d",
						GetPlayerID(uIndex), m_wCurrentUser, operCard, iter_begin->first, vecTingNotifyHu[uIndex].cbCard, vecTingNotifyHu[uIndex].cbCount);
				}
			}

			SendMsgToClient(uIndex, &operResMsg, net::S2C_MSG_MAJIANG_OPER_RESULT_REP);
			pout_front_notify_hu->clear_out_card();
			pout_front_notify_hu->clear_notify_hu();
		}

        //SendMsgToAll(&operResMsg,S2C_MSG_MAJIANG_OPER_RESULT_REP);

        //设置状态
        if(TargetAction==ACTION_GANG || TargetAction==ACTION_GANG_TING) {
            LOG_DEBUG("设置杠状态:%d",TargetAction);
            m_bGangStatus = true;
        }

        //设置用户
        m_wCurrentUser=wTargetUser;

        //杠牌处理
        if(TargetAction==ACTION_GANG)
        {
			if (m_cbListenStatus[wTargetUser] == TRUE)
			{
				m_bArrTingAfterMingGang[wTargetUser] = true;
			}
			
			

            GangPaiOper(wTargetUser,cbTargetCard,true);
            return true;
        }
        //杠听处理
        //if(TargetAction==ACTION_GANG_TING)
        //{
        //    //检查抢杠
        //    bool bAroseAction=EstimateUserRespond(wTargetUser,cbTargetCard,EstimatKind_GangTing);

        //    if(bAroseAction==false){
        //        EstimateGangTing(chairID);
        //        return true;
        //    }else{
        //        LOG_DEBUG("有人抢杠听");
        //        m_gangTingState = emGANG_TING_QIANG;
        //    }
        //    return true;
        //}

        //动作判断
        if(m_poolCards.size()>1)
        {
            //杠牌判断
            stGangCardResult GangCardResult;
            m_dwUserAction[m_wCurrentUser]|=m_gameLogic.AnalyseGangCard(m_cbCardIndex[m_wCurrentUser],m_WeaveItemArray[m_wCurrentUser],m_cbWeaveItemCount[m_wCurrentUser],GangCardResult);

            //结果处理
            if(GangCardResult.cbCardCount>0)
            {
                //设置变量
                m_dwUserAction[m_wCurrentUser]|=ACTION_GANG;
            }
        }
		CheckUserTingCard(wTargetUser);
        //发送动作
        SendOperNotify();
        FlushDeskCardToClient();
		//吃碰刷新时间
		ResetCoolTime();
        return true;
    }

    //主动动作
    if(m_wCurrentUser == chairID)
    {
        LOG_DEBUG("主动操作:%d",chairID);
        //海底牌处理
        //if(OnUserNeedTailOper(chairID,operCode))return true;

        if(operCode == ACTION_NULL)
        {
            //if(m_gangTingState == emGANG_TING_SELF_HU) {//杠听胡放弃
            //    EstimateGangTingQiang(m_wResumeUser);
            //    return true;
            //}
			BYTE cbArrPassCard[] = { operCard };
			WriteOutCardLog(chairID, OPERATE_CODE_NULL, cbArrPassCard, 1);
			memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

            m_dwUserAction[chairID] = ACTION_NULL;
            LOG_DEBUG("主动放弃操作:%d 更新操作",chairID);
            SendOperNotify();
			// 胡 、听和 补杠 暗杠点过的时间刷新
			ResetCoolTime();
            return true;
        }

        if((operCode != ACTION_NULL) && (operCode != ACTION_CHI_HU) &&
            (m_gameLogic.IsValidCard(operCard) == false)) {
            LOG_DEBUG("操作牌非法:%d",operCard);
			BYTE cbArrLawCard[] = { operCard };
			WriteOutCardLog(chairID, OPERATE_CODE_LAW, cbArrLawCard, 1);
			memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

            return false;
        }

        //设置变量
        m_bSendStatus                     =   true;
        m_bEnjoinChiHu[m_wCurrentUser]    =   false;
        m_dwUserAction[m_wCurrentUser]    =   ACTION_NULL;
        m_dwPerformAction[m_wCurrentUser] =   ACTION_NULL;

        bool bPublic=false;

        //执行动作
        switch(operCode)
        {
        case ACTION_LISTEN:     //听牌操作
            {
                BYTE cbCardIndex=m_gameLogic.SwitchToCardIndex(operCard);
                vector<BYTE> tingCards;
				vector<BYTE> huCards;
				BYTE cbTempCardIndex[MAX_INDEX] = { 0 };
				//BYTE cbCardIndex = m_gameLogic.SwitchToCardIndex(operCard);
				memcpy(cbTempCardIndex, m_cbCardIndex[chairID],sizeof(cbTempCardIndex));
				if (cbCardIndex<MAX_INDEX && cbTempCardIndex[cbCardIndex]>0)
				{
					cbTempCardIndex[cbCardIndex]--;
				}
				BYTE cbArrListenCard[] = { operCard };
				WriteOutCardLog(chairID, OPERATE_CODE_LISTEN, cbArrListenCard, 1);
				memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

				auto it_TingNotifyHu = m_mpTingNotifyHu[chairID].find(operCard);
				if (it_TingNotifyHu != m_mpTingNotifyHu[chairID].end())
				{
					vector<tagAnalyseTingNotifyHu> vecTingNotifyHu = it_TingNotifyHu->second;
					for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
					{
						huCards.push_back(vecTingNotifyHu[uIndex].cbCard);
					}
					OnUserRecordTingCard(chairID, huCards);
					OnUserOutCard(chairID, operCard, true);
					//SendTingNotifyHuCard(chairID);
					return true;
				}
				else
				{
					//
				}

                //m_gameLogic.AnalyseTingCard12(m_cbCardIndex[chairID],m_WeaveItemArray[chairID],m_cbWeaveItemCount[chairID],tingCards, huCards);
				//m_gameLogic.AnalyseTingCard11(cbTempCardIndex, m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], huCards);

				//LOG_DEBUG("听牌操作 - uid:%d,chairID:%d,operCode:%d,operCard:0x%02X,cbCardIndex:%d,tingCardsize:%d,huCards.size:%d,m_cbWeaveItemCount:%d", GetPlayerID(chairID), chairID, operCode, operCard, cbCardIndex, tingCards.size(), huCards.size(), m_cbWeaveItemCount[chairID]);

				

    //            for(uint32 j=0;j<tingCards.size();++j)
				//{
    //                if(tingCards[j] == operCard)
				//	{
				//		OnUserRecordTingCard(chairID, huCards);
    //                    OnUserOutCard(chairID,operCard,true);
    //                    return true;
    //                }
    //            }
                LOG_DEBUG("听牌操作错误 uid:%d,:%d",GetPlayerID(chairID),operCard);
                return false;
            }break;
        case ACTION_GANG:			//杠牌操作
        case ACTION_GANG_TING:      //杠听操作
            {
				
                //变量定义
                BYTE cbWeaveIndex=0xFF;
                BYTE cbCardIndex=m_gameLogic.SwitchToCardIndex(operCard);
				DWORD dwConcreteAction = 0;
                //杠牌处理
                if (m_cbCardIndex[chairID][cbCardIndex]==1)
                {
                    //寻找组合
                    for (BYTE i=0;i<m_cbWeaveItemCount[chairID];i++)
                    {
                        DWORD WeaveKind=m_WeaveItemArray[chairID][i].WeaveKind;
                        BYTE cbCenterCard=m_WeaveItemArray[chairID][i].cbCenterCard;
                        if ((cbCenterCard==operCard)&&(WeaveKind==ACTION_PENG))
                        {
							dwConcreteAction = 13;
                            bPublic=true;
                            cbWeaveIndex=i;
                            break;
                        }
                    }

                    //效验动作
                    //LOG_ERROR("cbWeaveIndex is null");
					if (cbWeaveIndex == 0xFF)
					{
						return false;
					}

					// 抢杠胡
					bool bAroseAction = EstimateUserRespond(chairID, operCard, EstimatKind_QiangGangHu);
					if (bAroseAction)
					{
						m_TempWeaveItem.Init();
						m_TempWeaveItem.WeaveKind = operCode;
						m_TempWeaveItem.cbCenterCard = operCard;
						m_TempWeaveItem.cbPublicCard = TRUE;
						m_TempWeaveItem.wProvideUser = chairID;
						m_TempWeaveItem.wOperateUser = chairID;

						//m_fangPaoUser = chairID;
						return true;
					}
					if (m_cbListenStatus[chairID] == TRUE)
					{
						m_bArrTingAfterMingGang[chairID] = true;
					}
					
                    //组合扑克
                    m_WeaveItemArray[chairID][cbWeaveIndex].cbPublicCard    = TRUE;
                    m_WeaveItemArray[chairID][cbWeaveIndex].wProvideUser    = chairID;
                    m_WeaveItemArray[chairID][cbWeaveIndex].WeaveKind       = operCode;
                    m_WeaveItemArray[chairID][cbWeaveIndex].cbCenterCard    = operCard;

					UpdateNotifyHuCardCount(chairID, operCard, 1);
                }
                else
                {
                    //扑克效验
                    if(m_cbCardIndex[chairID][cbCardIndex]!=4) 	return false;

                    //设置变量
                    bPublic=false;
                    cbWeaveIndex=m_cbWeaveItemCount[chairID]++;
                    m_WeaveItemArray[chairID][cbWeaveIndex].cbPublicCard    =   FALSE;
                    m_WeaveItemArray[chairID][cbWeaveIndex].wProvideUser    =   chairID;
                    m_WeaveItemArray[chairID][cbWeaveIndex].WeaveKind       =   operCode;
                    m_WeaveItemArray[chairID][cbWeaveIndex].cbCenterCard    =   operCard;

					

					UpdateNotifyHuCardCount(chairID, operCard, 4);
                }

				m_bArrGangFlowerStatus[chairID] = true;

                //杠牌数目
                m_cbGangCount++;

                //删除扑克
                m_cbCardIndex[chairID][cbCardIndex]=0;
				if (bPublic)
				{
					BYTE cbRemoveCard[] = { operCard,operCard,operCard };
					WriteOutCardLog(chairID, OPERATE_CODE_MING_GANG, cbRemoveCard, 3);
					//dwConcreteAction = 11;
				}
				else
				{
					BYTE cbRemoveCard[] = { operCard,operCard,operCard,operCard };
					WriteOutCardLog(chairID, OPERATE_CODE_AN_GANG, cbRemoveCard, 4);
					dwConcreteAction = 12;
				}
				memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));


				

                //设置状态
                //if(operCode==ACTION_GANG)
                m_bGangStatus=true;


                net::msg_majiang_oper_result_rep operResultRep;
                operResultRep.set_oper_user(chairID);
                operResultRep.set_provide_user(chairID);
                net::mjaction* pa = operResultRep.mutable_action();
                pa->set_card(operCard);
				if (operCode == ACTION_GANG)
				{
					pa->set_code(dwConcreteAction);
				}
				else
				{
					pa->set_code(m_gameLogic.GetOneAction(operCode));
				}

				for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
				{
					operResultRep.add_passhu_count(m_iUserPassHuCount[uIndex]);
					//if (uIndex == chairID)
					//{
					//	if (bIsPassHuOper)
					//	{
					//		operResultRep.add_passhu_count(m_iUserPassHuCount[chairID]);
					//	}
					//	else
					//	{
					//		operResultRep.add_passhu_count(-1);
					//	}
					//}
					//else
					//{
					//	operResultRep.add_passhu_count(-1);
					//}
				}

				//if (bIsPassHuOper)
				//{
				//	operResultRep.set_passhu_count(m_iUserPassHuCount[chairID]);
				//}
				//else
				//{
				//	operResultRep.set_passhu_count(-1);
				//}
				//提示胡牌
				
				if (m_cbListenStatus[chairID] == FALSE)
				{
					DWORD tmp = m_gameLogic.AnalyseTingCard15(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], m_mpTingNotifyHu[chairID]);
				}

				for (WORD uIndex = 0; uIndex < m_wPlayerCount; uIndex++)
				{
					net::mjaction_out_front_notify_hu * pout_front_notify_hu = operResultRep.add_notify_hu();

					LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_wCurrentUser:%d,operCard:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
						GetPlayerID(uIndex), m_cbListenStatus[uIndex], m_wCurrentUser, operCard, m_mpTingNotifyHu[uIndex].size());


					auto iter_begin = m_mpTingNotifyHu[uIndex].begin();
					for (; iter_begin != m_mpTingNotifyHu[uIndex].end(); iter_begin++)
					{
						pout_front_notify_hu->add_out_card(iter_begin->first);
						net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

						vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
						for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
						{
							pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
							pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
							pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

							LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_wCurrentUser:%d,operCard:0x%02X,out_card:0x%02X - hu_card:0x%02X,cbCount:%d",
								GetPlayerID(uIndex), m_wCurrentUser, operCard, iter_begin->first, vecTingNotifyHu[uIndex].cbCard, vecTingNotifyHu[uIndex].cbCount);
						}
					}

					SendMsgToClient(uIndex, &operResultRep, net::S2C_MSG_MAJIANG_OPER_RESULT_REP);
					pout_front_notify_hu->clear_out_card();
					pout_front_notify_hu->clear_notify_hu();
				}


                //SendMsgToAll(&operResultRep,S2C_MSG_MAJIANG_OPER_RESULT_REP);

                //效验动作
                bool bAroseAction=false;
                //杠牌处理
                if(operCode == ACTION_GANG)
                {
                    GangPaiOper(chairID,operCard,bPublic);
                }
                //杠听处理
                //if(operCode == ACTION_GANG_TING)
                //{
                //    //LOG_DEBUG("自摸杠听:%d--%s",chairID,m_gameLogic.GetCardName(operCard).c_str());
                //    //检查抢杠
                //    if(bPublic == true)bAroseAction=EstimateUserRespond(chairID,operCard,EstimatKind_GangTing);

                //    if(bAroseAction==false){
                //        EstimateGangTing(chairID);
                //        return true;
                //    }else{
                //        //LOG_DEBUG("有人抢杠听");
                //        m_gangTingState = emGANG_TING_QIANG;
                //    }
                //}

                return true;
            }break;
        case ACTION_CHI_HU:		//吃胡操作
            {
				BYTE cbArrHuCard[] = { operCard };
				WriteOutCardLog(chairID, OPERATE_CODE_CHI_HU, cbArrHuCard, 1);
				memset(m_iUserPassHuFlag, 0, sizeof(m_iUserPassHuFlag));

                ChiHuOper(chairID,true);
                return true;
            }break;
        }

        return true;
    }


    return false;
}

bool    CGameMaJiangTable::OnUserOperTrustee(uint16 chairID, uint32 trustee)
{
	LOG_DEBUG("操作托管 - uid:%d,chaidID:%d,trustee:%d ", GetPlayerID(chairID), chairID, trustee);

	CGamePlayer * pPlayer = GetPlayer(chairID);
	if (pPlayer == NULL)
	{
		LOG_DEBUG("操作托管 失败 - uid:%d,chaidID:%d,trustee:%d ", GetPlayerID(chairID), chairID, trustee);

		return false;
	}
	msg_majiang_oper_trustee_rep rep;
	rep.set_trustee(trustee);
	if (trustee != 0 || !m_bTrustee[chairID])
	{
		rep.set_result(RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_MAJIANG_OPER_TRUSTEE_REP);
		return false;
	}
	else
	{
		m_bTrustee[chairID] = false;
		rep.set_result(net::RESULT_CODE_SUCCESS);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_MAJIANG_OPER_TRUSTEE_REP);
	}

	//托管广播
	net::msg_majiang_oper_trustee_broadcast broad;
	broad.set_uid(GetPlayerID(chairID));
	broad.set_trustee(trustee);

	SendMsgToAll(&broad, net::S2C_MSG_MAJIANG_OPER_TRUSTEE_BROADCAST);

	return true;
}
bool    CGameMaJiangTable::OnUserRecordTingCard(uint16 chairID, vector<BYTE> tingCards)
{
	if (chairID >= m_wPlayerCount)
	{
		return false;
	}
	m_vecArrUserTingCard[chairID].clear();

	for (uint32 j = 0; j < tingCards.size(); ++j) 
	{
		LOG_DEBUG("uid:%d,chairID:%d,j:%d,tingCards:0x%02X", GetPlayerID(chairID), chairID, j, tingCards[j]);

		m_vecArrUserTingCard[chairID].push_back(tingCards[j]);
	}

	for (uint32 i = 0; i < m_wPlayerCount; i++)
	{
		for (uint32 j = 0; j<m_vecArrUserTingCard[i].size(); j++)
		{
			LOG_DEBUG("uid:%d,i:%d,j:%d,card:0x%02X", GetPlayerID(i), i, j, m_vecArrUserTingCard[i][j]);
		}
	}

	return true;
}

bool	CGameMaJiangTable::CheckUserTingCardIsChange(uint16 chairID, BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount)
{
	if (chairID >= m_wPlayerCount)
	{
		return true;
	}
	if (m_vecArrUserTingCard[chairID].size() == 0)
	{
		return false;
	}
	//return true;
	vector<BYTE> huCards;
	m_gameLogic.AnalyseTingCard11(cbCardIndex, WeaveItem, cbWeaveCount, huCards);

	//LOG_DEBUG("------------------------------------------------------------------------------------ uid:%d,size:%d,%d", GetPlayerID(chairID),m_vecArrUserTingCard[chairID].size(), huCards.size());
	//for (uint32 i = 0; i < m_wPlayerCount; i++)
	//{
	//	for (uint32 j = 0; j<m_vecArrUserTingCard[i].size(); j++)
	//	{
	//		LOG_DEBUG("uid:%d,i:%d,j:%d,card:0x%02X", GetPlayerID(i), i, j, m_vecArrUserTingCard[i][j]);
	//	}
	//	for (uint32 k = 0; k<huCards.size(); k++)
	//	{
	//		LOG_DEBUG("uid:%d,i:%d,k:%d,card:0x%02X", GetPlayerID(i), i, k, huCards[k]);
	//	}
	//}

	if (huCards.size() == 0 || huCards.size() != m_vecArrUserTingCard[chairID].size())
	{
		return true;
	}

	for (uint32 j = 0; j < m_vecArrUserTingCard[chairID].size(); ++j)
	{
		bool bFlagIsExist = false;
		for (uint32 i = 0; i < huCards.size(); ++i)
		{
			if (m_vecArrUserTingCard[chairID][j] == huCards[i])
			{
				bFlagIsExist = true;
				break;
			}
		}
		if (bFlagIsExist == false)
		{
			return true;
		}
	}


	//for (uint32 i = 0; i < m_wPlayerCount; i++)
	//{
	//	for (uint32 j = 0; j<m_vecArrUserTingCard[i].size(); j++)
	//	{
	//		LOG_DEBUG("uid:%d,i:%d,j:%d,card:0x%02X", GetPlayerID(i), i,j, m_vecArrUserTingCard[i][j]);
	//	}
	//	for (uint32 k = 0; k<tingCards.size(); k++)
	//	{
	//		LOG_DEBUG("uid:%d,i:%d,k:%d,card:0x%02X", GetPlayerID(i), i, k, tingCards[k]);
	//	}
	//}

	return false;
}

// 用户要海底操作
bool    CGameMaJiangTable::OnUserNeedTailOper(uint16 chairID, uint32 operCode)
{
    //海底牌处理
    if(m_dwUserAction[chairID]&ACTION_NEED_TAIL){//要海底
        if(operCode == ACTION_NEED_TAIL)//要海底
        {
            m_tailCardOper[chairID] = 1;
            //检查是否能胡
            if(EstimateNeedTailOper(chairID))
                return true;
        }else{
            m_tailCardOper[chairID] = 2;
            m_wCurrentUser = GetNextUser(m_wCurrentUser);
            if(EstimateUserTailRespond()){
                return true;
            }else{//流局
                GameOverNoWin();
                return true;
            }
        }
    }
    return false;
}
// 用户起手胡操作
bool    CGameMaJiangTable::OnUserOpenHuOper(uint16 chairID,uint32 operCode)
{
    //起手胡特殊处理,大家平级
    if(m_dwUserAction[chairID]&ACTION_QIPAI_HU)
    {
        m_dwUserAction[chairID] = ACTION_NULL;
        if(operCode == ACTION_QIPAI_HU)
        {
            net::msg_majiang_opening_hu_rep msg;
            vector<BYTE> actions;
            m_openingHu[chairID].getAllActions(actions);
            for(uint32 i=0;i<actions.size();++i){
                msg.add_min_hu_type(actions[i]);
            }
            msg.set_chair_id(chairID);
            BYTE cardData[MAX_COUNT];
            BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[chairID],cardData,MAX_COUNT);
            for(uint32 j=0;j<cardNum;++j){
                msg.add_card_data(cardData[j]);
            }
            SendMsgToAll(&msg,S2C_MSG_MAJIANG_OPENING_HU_REP);
        }else{
            m_openingHu[chairID].reset();// 不做起手胡
        }
        for(uint8 i=0;i<m_wPlayerCount;++i)
        {
            if(m_bResponse[i] == false && m_dwUserAction[i] != ACTION_NULL){
                LOG_DEBUG("还有玩家未操作起手胡:%d",i);
                return true;
            }
        }
        CheckStartAction(false);
        return true;
    }

    return false;
}
// 胡牌操作处理
bool   CGameMaJiangTable::ChiHuOper(uint16 chairID,bool bAction)
{
    LOG_DEBUG("吃胡操作 - uid:%d,chairID:%d,bAction:%d,m_cbChiHuCard:0x%02X,m_cbProvideCard:0x%02X,WeaveKind:%d,m_wCurrentUser:%d,",GetPlayerID(chairID),chairID,bAction, m_cbChiHuCard, m_cbProvideCard, m_TempWeaveItem.WeaveKind, m_wCurrentUser);
    //if(m_gangTingState == emGANG_TING_NULL || m_gangTingState == emGANG_TING_QIANG){
    //    
    //}
	m_cbChiHuCard = m_cbProvideCard;
	m_wOperHuCardPlayer = chairID;
    if(bAction)//主动操作
    {
        //吃牌权位
        DWORD dwChiHuRight=0;

		if (m_bGangStatus == true)
		{
			dwChiHuRight |= CHR_GANG_FLOWER;
		}
		if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 1))
		{
			dwChiHuRight |= CHR_DI;			
		}
		if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 0))
		{
			dwChiHuRight |= CHR_TIAN;
			m_wProvideUser = m_wCurrentUser;
		}
		if (m_bArrTianTingStatus[chairID] == true)
		{
			dwChiHuRight |= CHR_TIAN_TING;
		}
		CalcPlayerHuResult(chairID,dwChiHuRight,true);

        OnGameEnd(chairID,GER_NORMAL);

    }
	else
	{//被动操作

        //吃牌权位
        DWORD dwChiHuRight=0;
		if (m_TempWeaveItem.WeaveKind == ACTION_GANG)
		{
			//LOG_DEBUG("抢杠胡 - uid:%d,chairID:%d,bAction:%d,m_cbChiHuCard:0x%02X,m_cbProvideCard:0x%02X,WeaveKind:%d,m_wCurrentUser:%d,", GetPlayerID(chairID), chairID, bAction, m_cbChiHuCard, m_cbProvideCard, m_TempWeaveItem.WeaveKind, m_wCurrentUser);

			dwChiHuRight |= CHR_QIANG_GANG;
		}
		if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 1))
		{
			dwChiHuRight |= CHR_RENHU;
		}
		if (m_bArrTianTingStatus[chairID] == true)
		{
			dwChiHuRight |= CHR_TIAN_TING;
		}
		if (m_cbListenStatus[chairID] == TRUE && m_bArrTingAfterMingGang[chairID] == false)
		{
			dwChiHuRight |= CHR_LI_ZHI;
		}
        //胡牌判断
        for(WORD i=0;i<m_wPlayerCount;i++)
        {
            //过虑判断
			if ((m_dwPerformAction[i] & ACTION_CHI_HU) == 0)
			{
				continue;
			}

            /*if(i == m_wProvideUser && m_gangTingState == 0){
                LOG_DEBUG("放炮者过滤:%d", m_wProvideUser);
                continue;
            }*/
            if(i != chairID && !m_mjCfg.supportTongPao()) {
                LOG_DEBUG("不支持通炮的话,只有一个人胡牌:%d",i);
                continue;
            }
            CalcPlayerHuResult(i,dwChiHuRight,false);
        }
        //CheckHuTail();
        //结束游戏
        OnGameEnd(chairID,GER_NORMAL);
    }

    return true;
}
// 杠牌操作处理
bool    CGameMaJiangTable::GangPaiOper(uint16 chairID,BYTE cbCard,bool bPublic)
{
    LOG_DEBUG("杠牌处理 uid:%d,chairID:%d,cbCard:%d,bPublic:%d",GetPlayerID(chairID),chairID,cbCard,bPublic);

    bool bAroseAction = false;
    //if(bPublic){
    //    bAroseAction = EstimateUserRespond(chairID, cbCard, EstimatKind_GangCard);
    //}
    FlushDeskCardToClient();
    //发送扑克
    if(bAroseAction == false){
        //LOG_DEBUG("玩家杠牌后摸牌:%d",chairID);
		//LOG_DEBUG("玩家杠牌后摸牌 uid:%d,chairID:%d,cbCard:%d,bPublic:%d", GetPlayerID(chairID), chairID, cbCard, bPublic);
        DispatchCardData(chairID, false);
    }

    return true;
}

// 发送操作
bool    CGameMaJiangTable::SendOperNotify()
{
	net::msg_majiang_oper_notify_rep msgRep;
	msgRep.set_resume_user(m_wResumeUser);
	msgRep.set_cur_user(m_wCurrentUser);

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		msgRep.add_passhu_count(m_iUserPassHuCount[i]);
		//bool bIsHuOper = false;
		//vector<net::mjaction> actions;
		//SwitchUserAction(i, m_cbProvideCard, actions);
		//for (uint8 j = 0; j < actions.size(); ++j)
		//{
		//	//net::mjaction* pa = msgRep.add_action();
		//	//*pa = actions[j];
		//	if (actions[j].code() == 7)
		//	{
		//		bIsHuOper = true;
		//	}
		//}
		//if (bIsHuOper)
		//{
		//	msgRep.add_passhu_count(m_iUserPassHuCount[i]);
		//}
		//else
		//{
		//	msgRep.add_passhu_count(-1);
		//}
	}

    //发送提示
    for(WORD i=0;i<m_wPlayerCount;i++)
    {
		//bool bIsHuOper = false;
        vector<net::mjaction> actions;
        SwitchUserAction(i,m_cbProvideCard,actions);
        for(uint32 j=0;j<actions.size();++j)
		{
            net::mjaction* pa = msgRep.add_action();
            *pa=actions[j];
		}

        SendMsgToClient(i,&msgRep,S2C_MSG_MAJIANG_OPER_NOTIFY_REP);
		msgRep.clear_action();

        LOG_DEBUG("通知玩家操作 - roomid:%d,tableid:%d,uid:%d,i:%d,action:%d",GetRoomID(),GetTableID(),GetPlayerID(i),i,m_dwUserAction[i]);

    }
    //ResetCoolTime();
    return true;
}

//uint32 CGameMaJiangTable::UpdateNotifyHuCard(uint8 chairID,BYTE cbCard)
//{
//	if (m_mpTingNotifyHu[chairID].size()>0)
//	{
//		auto iter = m_mpTingNotifyHu[chairID].find(cbCard);
//		if (iter != m_mpTingNotifyHu[chairID].end())
//		{
//			vector<tagAnalyseTingNotifyHu> vecNotifyHu = iter->second;
//			m_mpTingNotifyHu[chairID].clear();
//			m_mpTingNotifyHu[chairID].insert(make_pair(cbCard, vecNotifyHu));
//		}
//		else
//		{
//			m_mpTingNotifyHu[chairID].clear();
//		}
//	}
//	return 0;
//}

bool CGameMaJiangTable::UpdateNotifyHuCardCount(BYTE cbCard,BYTE cbCount)
{
	for (uint32 i = 0; i < m_wPlayerCount; i++)
	{
		if (m_mpTingNotifyHu[i].size()>0)
		{
			auto iter = m_mpTingNotifyHu[i].begin();
			for (; iter != m_mpTingNotifyHu[i].end(); iter++)
			{
				vector<tagAnalyseTingNotifyHu> & vecNotifyHu = iter->second;
				for (uint32 j = 0; j < vecNotifyHu.size(); j++)
				{
					if (vecNotifyHu[j].cbCard == cbCard)
					{
						if (vecNotifyHu[j].cbCount >= cbCount)
						{
							vecNotifyHu[j].cbCount -= cbCount;
						}
					}
				}
			}
		}
	}

	return true;
}

bool CGameMaJiangTable::UpdateNotifyHuCardCount(uint16 chairID, BYTE cbCard, BYTE cbCount)
{
	for (uint32 i = 0; i < m_wPlayerCount; i++)
	{
		if (chairID == i)
		{
			continue;
		}
		if (m_mpTingNotifyHu[i].size() == 1)
		{
			auto iter = m_mpTingNotifyHu[i].begin();
			for (; iter != m_mpTingNotifyHu[i].end(); iter++)
			{
				vector<tagAnalyseTingNotifyHu> & vecNotifyHu = iter->second;
				for (uint32 j = 0; j < vecNotifyHu.size(); j++)
				{
					if (vecNotifyHu[j].cbCard == cbCard)
					{
						if (vecNotifyHu[j].cbCount >= cbCount)
						{
							vecNotifyHu[j].cbCount -= cbCount;
						}
					}
				}
			}
		}
	}

	return true;
}

// 通知听牌
bool    CGameMaJiangTable::NotifyListen(uint16 chairID)
{
    net::msg_majiang_listen_card_rep msg;
    msg.set_listen_user(chairID);

    m_cbListenStatus[chairID] = 1;

    SendMsgToAll(&msg,S2C_MSG_MAJIANG_LISTEN_CARD_REP);
    LOG_DEBUG("通知听牌:%d",chairID);

    return true;
}
// 刷新牌面数据给前端
bool    CGameMaJiangTable::FlushDeskCardToClient()
{
    if(GetGameState() != TABLE_STATE_PLAY)return false;

    for(uint16 chairID=0;chairID<m_wPlayerCount;++chairID) {
        net::msg_majiang_flush_desk_cards_rep msg;
        msg.set_left_card_count(m_poolCards.size());
        msg.set_send_card_data(m_cbSendCardData);
		msg.set_cur_user(m_wCurrentUser);
		msg.set_send_card_user(m_wSendCardUser);
        //弃牌,组合
        for (int32 i = 0; i < m_wPlayerCount; ++i) {
            msg.add_discard_count(m_cbDiscardCount[i]);
            for (uint32 j = 0; j < m_cbDiscardCount[i]; ++j) {
                msg.add_discard_cards(m_cbDiscardCard[i][j]);
            }

            msg.add_weave_count(m_cbWeaveItemCount[i]);
            for (uint32 j = 0; j < m_cbWeaveItemCount[i]; ++j) {
                net::weave_item *pItem = msg.add_weave_items();
                pItem->set_provide_user(m_WeaveItemArray[i][j].wProvideUser);
                pItem->set_center_card(m_WeaveItemArray[i][j].cbCenterCard);
                pItem->set_public_card(m_WeaveItemArray[i][j].cbPublicCard);
                pItem->set_weave_kind(m_gameLogic.GetOneAction(m_WeaveItemArray[i][j].WeaveKind));
            }

            //手牌数据
            BYTE cbCardData[MAX_COUNT];
            BYTE cardCount = m_gameLogic.SwitchToCardData(m_cbCardIndex[i], cbCardData, MAX_COUNT);
            msg.add_hand_card_count(cardCount);
            if (i == chairID) {
                for (uint32 k = 0; k < cardCount; ++k) {
                    msg.add_hand_card_data(cbCardData[k]);
                }
            }
        }
        SendMsgToClient(chairID, &msg, S2C_MSG_MAJIANG_FLUSH_DESK_REP);
    }
    LOG_DEBUG("刷新桌面牌面到客户端");
    return true;
}
// 刷新手牌数据给前端
bool    CGameMaJiangTable::FlushHandCardToClient(uint16 chairID)
{
    LOG_DEBUG("刷新手牌到前端:%d",chairID);
    net::msg_majiang_get_hand_card_rep msg;

    //手牌数据
    BYTE cbCardData[MAX_COUNT];
    BYTE cardCount = m_gameLogic.SwitchToCardData(m_cbCardIndex[chairID], cbCardData, MAX_COUNT);
    msg.set_left_card_count(m_poolCards.size());
    for (uint32 k = 0; k < cardCount; ++k) {
        msg.add_hand_card_data(cbCardData[k]);
    }

    SendMsgToClient(chairID,&msg,S2C_MSG_MAJIANG_GET_HAND_CARD_REP);

    return true;
}

bool    CGameMaJiangTable::FlushDeskCardToClient(uint16 wChairID)
{
	if (GetGameState() != TABLE_STATE_PLAY)
		return false;

	for (uint16 chairID = 0; chairID<m_wPlayerCount; ++chairID)
	{
		if (chairID != wChairID)
		{
			continue;
		}
		net::msg_majiang_flush_desk_cards_rep msg;
		msg.set_left_card_count(m_poolCards.size());
		msg.set_send_card_data(m_cbSendCardData);
		msg.set_cur_user(m_wCurrentUser);
		msg.set_send_card_user(m_wSendCardUser);
		//弃牌,组合
		for (int32 i = 0; i < m_wPlayerCount; ++i) {
			msg.add_discard_count(m_cbDiscardCount[i]);
			for (uint32 j = 0; j < m_cbDiscardCount[i]; ++j) {
				msg.add_discard_cards(m_cbDiscardCard[i][j]);
			}

			msg.add_weave_count(m_cbWeaveItemCount[i]);
			for (uint32 j = 0; j < m_cbWeaveItemCount[i]; ++j) {
				net::weave_item *pItem = msg.add_weave_items();
				pItem->set_provide_user(m_WeaveItemArray[i][j].wProvideUser);
				pItem->set_center_card(m_WeaveItemArray[i][j].cbCenterCard);
				pItem->set_public_card(m_WeaveItemArray[i][j].cbPublicCard);
				pItem->set_weave_kind(m_gameLogic.GetOneAction(m_WeaveItemArray[i][j].WeaveKind));
			}

			//手牌数据
			BYTE cbCardData[MAX_COUNT];
			BYTE cardCount = m_gameLogic.SwitchToCardData(m_cbCardIndex[i], cbCardData, MAX_COUNT);
			msg.add_hand_card_count(cardCount);
			if (i == chairID) {
				for (uint32 k = 0; k < cardCount; ++k) {
					msg.add_hand_card_data(cbCardData[k]);
				}
			}
		}
		SendMsgToClient(chairID, &msg, S2C_MSG_MAJIANG_GET_ALL_CARD_INFO_REQ);
	}
	LOG_DEBUG("刷新桌面牌面到客户端");
	return true;
}

// 显示公共牌
bool    CGameMaJiangTable::ShowPublicCards(uint16 chairID,uint32 showType,const vector<BYTE>& cards)
{
    LOG_DEBUG("显示公共牌:%d--%d--%d",chairID,showType,cards.size());
    net::msg_majiang_show_public_cards_rep msg;
    msg.set_show_user(chairID);
    msg.set_show_type(m_gameLogic.GetOneAction(showType));
    msg.set_left_card_count(m_poolCards.size());
    for(uint32 i=0;i<cards.size();++i){
        msg.add_show_cards(cards[i]);
    }
    SendMsgToAll(&msg,S2C_MSG_MAJIANG_SHOW_PUB_CARD_REP);

    return true;
}

bool    CGameMaJiangTable::SetRobotTingCardZiMo(uint16 chairID, BYTE & cbOutSendCardData, bool isNeedTingCard)
{
	if (chairID >= m_wPlayerCount)
	{
		return false;
	}
	CGamePlayer * pPlayer = GetPlayer(chairID);
	if (pPlayer == NULL)
	{
		return false;
	}
	if (pPlayer->IsRobot() == false)
	{
		return false;
	}
	if (isNeedTingCard && m_cbListenStatus[chairID] == FALSE)
	{
		return false;
	}
	bool needChangeCard = g_RandGen.RandRatio(m_uRobotZiMoPro, PRO_DENO_10000);
	if (needChangeCard == false)
	{
		return false;
	}

	vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;

	DWORD action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
	LOG_DEBUG("SetRobotTingCardZiMo roomid:%d,tableid:%d,chairID:%d,action:%d,vecNotifyHuCard_size:%d",GetRoomID(),
		GetTableID(), chairID, action, vecNotifyHuCard.size());
	if (action != ACTION_LISTEN || vecNotifyHuCard.size() == 0)
	{
		return false;
	}
	for (uint32 i = 0; i < vecNotifyHuCard.size(); i++)
	{
		// remove card pool
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
			cbOutSendCardData = vecNotifyHuCard[i].cbCard;
			return true; // break; // modify by har
		}		
	}

	return false; // true; // modify by har
}

// 派发扑克
bool    CGameMaJiangTable::DispatchCardData(uint16 curUser,bool bNotGang)
{
    //状态效验
    if(curUser==INVALID_CHAIR || curUser>= m_wPlayerCount)
	{
        LOG_ERROR("发牌用户错误:%d",curUser);
        return false;
    }

    //丢弃扑克
    if((m_wOutCardUser!=INVALID_CHAIR)&&(m_cbOutCardData!=0))
    {
        m_cbDiscardCount[m_wOutCardUser]++;
        m_cbDiscardCard[m_wOutCardUser][m_cbDiscardCount[m_wOutCardUser]-1]=m_cbOutCardData;
    }

    //设置变量
    m_cbOutCardData         =   0;
    m_wCurrentUser          =   curUser;
    m_wOutCardUser          =   INVALID_CHAIR;
    m_fangPaoUser           =   INVALID_CHAIR;
    m_bEnjoinChiHu[curUser] =   false;
	
    ResetUserOperInfo();
	m_TempWeaveItem.Init();

    //海底牌处理
 //   if(m_mjCfg.supportTail() && bNotGang)
	//{//支持海底
 //       if(m_poolCards.size() == m_cbMustLeft){//最后一张海底牌
 //           m_tailCard = PopCardFromPool();
 //           if(EstimateUserTailRespond()){
 //               return true;
 //           }else{
 //               GameOverNoWin();
 //               return true;
 //           }
 //       }
 //   }
    //发牌处理
    if(m_bSendStatus==true)
    {
        //是否没牌了荒庄结束
        if(m_poolCards.size() < (uint32)(m_cbMustLeft) || m_poolCards.size() == 0)
        {
            GameOverNoWin();
            return true;
        }

        //发送扑克
		BYTE cbOutSendCardData = 0xff;
        m_cbSendCardCount++;
		
		//幸运值胡牌控制
		bool LuckyTingCardZiMo = false;
		if (m_lucky_win_uid!=0)
		{
			CGamePlayer * pPlayer = GetGamePlayerByUid(m_lucky_win_uid);
			if (pPlayer != NULL)				
			{
				uint16 ctrl_chairID = GetChairID(pPlayer);
				if (ctrl_chairID == curUser && m_cbListenStatus[curUser])
				{					
					LuckyTingCardZiMo = SetLuckyTingCardZiMo(curUser, cbOutSendCardData);
					if (LuckyTingCardZiMo)
					{
						m_cbSendCardData = cbOutSendCardData;
						LOG_DEBUG("set current lucky win player - uid:%d", m_lucky_win_uid);
						//m_set_ctrl_lucky_uid.insert(m_lucky_win_uid);
						m_lucky_flag = true;
					}					
				}				
			}
		}

		//新账号福利胡牌控制
		bool NRWTingCardZiMo = false;
		if (!LuckyTingCardZiMo && IsNewRegisterWelfareTable())
		{
			uint32 control_uid = 0;
			int control_status = 0;		
			CGamePlayer * pPlayer = GetPlayer(curUser);
			if (pPlayer != NULL && !pPlayer->IsRobot() && m_cbListenStatus[curUser])				
			{
				if (pPlayer->GetNewRegisterWelfareStatus(control_status))
				{
					control_uid = pPlayer->GetUID();
				}
							
				//判断当前是否有可控玩家
				if (control_uid != 0)
				{				
					m_nrw_status = control_status;

					//如果当前赢取大于最大赢取时,设置机器人必赢
					if (control_status == 2)
					{
						m_NRWRobotZiMo = true;
						m_nrw_ctrl_uid = pPlayer->GetUID();   //设置当前活跃福利所控的玩家ID
						LOG_DEBUG("set robot zimo is true - uid:%d control_status:%d", control_uid, control_status);							
					}
					else
					{
						bool ret = SetNRWTingCardZiMo(curUser, cbOutSendCardData);
						if (ret)
						{
							m_cbSendCardData = cbOutSendCardData;
							m_nrw_ctrl_uid = pPlayer->GetUID();   //设置当前活跃福利所控的玩家ID
							NRWTingCardZiMo = true;
							LOG_DEBUG("1 set current player zimo success - uid:%d control_status:%d", control_uid, control_status);
						}
						else
						{
							LOG_DEBUG("1 set current player zimo fail - uid:%d control_status:%d", control_uid, control_status);
						}
					}
				}
				LOG_DEBUG("the no player match new register welfare tid:%d.", GetTableID());				
			}
			else
			{
				if (pPlayer != NULL && pPlayer->IsRobot() && m_cbListenStatus[curUser] && m_NRWRobotZiMo)
				{
					bool ret = SetNRWRobotTingCardZiMo(curUser, cbOutSendCardData);
					if (ret)
					{
						m_cbSendCardData = cbOutSendCardData;
						NRWTingCardZiMo = true;
						LOG_DEBUG("1 set robot zimo success - uid:%d", pPlayer->GetUID());
					}
					else
					{
						LOG_DEBUG("1 set robot zimo fail - uid:%d", pPlayer->GetUID());
					}
				}
			}
		}

		// 库存控制 add by har
		bool bStockCtrl = false;
		if (!LuckyTingCardZiMo && !NRWTingCardZiMo) {
			bStockCtrl = SetStockTingCardZiMo(curUser, cbOutSendCardData);
			if (bStockCtrl)
				m_cbSendCardData = cbOutSendCardData;
		}

		//活跃福利胡牌控制
		bool AWTingCardZiMo = false;
        if (!LuckyTingCardZiMo && !NRWTingCardZiMo && !bStockCtrl)
		{			
			CGamePlayer * pPlayer = GetPlayer(curUser);
			if (pPlayer != NULL && !pPlayer->IsRobot() && m_cbListenStatus[curUser])
			{
				uint64 aw_max_win = 0;
				if (GetInActiveWelfareCtrlList(pPlayer->GetUID(), aw_max_win))
				{
					AWTingCardZiMo = SetAWTingCardZiMo(curUser, cbOutSendCardData, aw_max_win);
					if (AWTingCardZiMo)
					{
						m_cbSendCardData = cbOutSendCardData;
						m_aw_ctrl_uid = pPlayer->GetUID();   //设置当前活跃福利所控的玩家ID
					}
					else
					{
						m_aw_ctrl_uid = 0;
					}
				}
			}
		}
		
        //机器人胡牌控制
		bool RobotTingCardZiMo = false;
        if (!LuckyTingCardZiMo && !NRWTingCardZiMo && !AWTingCardZiMo && !bStockCtrl)
        {
            RobotTingCardZiMo = SetRobotTingCardZiMo(curUser, cbOutSendCardData);
            if (RobotTingCardZiMo == true && cbOutSendCardData != 0xff)
            {
                m_cbSendCardData = cbOutSendCardData;
            }
            else
            {
                m_cbSendCardData = PopCardFromPool();
            }
        }

		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d LuckyTingCardZiMo:%d NRWTingCardZiMo:%d AWTingCardZiMo:%d RobotTingCardZiMo:%d,m_stockTingCardZiMoPro:%d,bStockCtrl:%d",
			GetRoomID(), GetTableID(), GetPlayerID(curUser), LuckyTingCardZiMo, NRWTingCardZiMo, AWTingCardZiMo, RobotTingCardZiMo, m_stockTingCardZiMoPro, bStockCtrl);
		
		BYTE cbArrSendCard[] = { m_cbSendCardData };
		WriteOutCardLog(curUser, OPERATE_CODE_SEND_CARD, cbArrSendCard, 1);

        //插入数据
        m_cbCardIndex[curUser][m_gameLogic.SwitchToCardIndex(m_cbSendCardData)]++;

        //设置变量
        m_wProvideUser = curUser;
		m_wSendCardUser = curUser;
        m_cbProvideCard = m_cbSendCardData;
		m_vecTingNotifyHu[curUser].clear();
		//LOG_DEBUG("---------------------------------------------------------------------------------");
		//for (uint32 i = 0; i < MAX_INDEX; i++)
		//{
		//	if (m_cbCardIndex[curUser][i]>0)
		//	{
		//		LOG_DEBUG("1- uid:%d,i:%d,m_cbCardIndex:%d,m_cbWeaveItemCount:%d",GetPlayerID(curUser),i, m_cbCardIndex[curUser][i], m_cbWeaveItemCount[curUser]);
		//	}
		//}
		

        //杠牌判断
        if(m_poolCards.size() > 1 && (m_cbListenStatus[curUser] == 0 || m_mjCfg.IsCanGangAfterListen()))
        {
			stGangCardResult GangCardResult;
			DWORD dwUserAction = m_gameLogic.AnalyseGangCard(m_cbCardIndex[curUser], m_WeaveItemArray[curUser], m_cbWeaveItemCount[curUser], GangCardResult);
			if (dwUserAction == ACTION_GANG && GangCardResult.cbCardData[0] >0 && 0x37 >= GangCardResult.cbCardData[0])
			{
				BYTE cbWeaveItemCount = m_cbWeaveItemCount[curUser];
				stWeaveItem	tagWeaveItemArray[MAX_WEAVE];
				//memcpy(tagWeaveItemArray, m_WeaveItemArray[curUser], sizeof(tagWeaveItemArray[MAX_WEAVE]));
				for (uint32 i = 0; i < MAX_WEAVE; i++)
				{
					tagWeaveItemArray[i] = m_WeaveItemArray[curUser][i];
				}
				BYTE cbTempCardIndex[MAX_INDEX];
				memset(cbTempCardIndex, 0, sizeof(cbTempCardIndex));
				//memcpy(cbTempCardIndex,m_cbCardIndex[curUser],sizeof(cbTempCardIndex));
				for (uint32 i = 0; i < MAX_INDEX; i++)
				{
					cbTempCardIndex[i] = m_cbCardIndex[curUser][i];
				}
				//for (uint32 i = 0; i < MAX_INDEX; i++)
				//{
				//	if (cbTempCardIndex[i]>0)
				//	{
				//		LOG_DEBUG("2- uid:%d,i:%d,cbTempCardIndex:%d,cbWeaveItemCount:%d,card:0x%02X", GetPlayerID(curUser),i, cbTempCardIndex[i], cbWeaveItemCount, GangCardResult.cbCardData[0]);
				//	}
				//}

				//删除扑克
				BYTE cbCrdIndex = m_gameLogic.SwitchToCardIndex(GangCardResult.cbCardData[0]);
				
				if (cbTempCardIndex[cbCrdIndex] ==4)
				{
					WORD wIndex = cbWeaveItemCount++;
					tagWeaveItemArray[wIndex].cbPublicCard = TRUE;
					tagWeaveItemArray[wIndex].cbCenterCard = GangCardResult.cbCardData[0];
					tagWeaveItemArray[wIndex].WeaveKind = ACTION_GANG;
					tagWeaveItemArray[wIndex].wProvideUser = curUser;


					BYTE cbRemoveCard[] = { GangCardResult.cbCardData[0],GangCardResult.cbCardData[0],GangCardResult.cbCardData[0],GangCardResult.cbCardData[0] };
					m_gameLogic.RemoveCard(cbTempCardIndex, cbRemoveCard, getArrayLen(cbRemoveCard));
				}
				else if (cbTempCardIndex[cbCrdIndex] == 1)
				{
					//寻找组合
					BYTE cbWeaveIndex = 0xFF;
					for (BYTE i = 0; i<cbWeaveItemCount; i++)
					{
						DWORD WeaveKind = tagWeaveItemArray[i].WeaveKind;
						BYTE cbCenterCard = tagWeaveItemArray[i].cbCenterCard;
						if ((cbCenterCard == GangCardResult.cbCardData[0]) && (WeaveKind == ACTION_PENG))
						{
							cbWeaveIndex = i;
							break;
						}
					}
					//效验动作
					if (cbWeaveIndex != 0xFF)
					{
						tagWeaveItemArray[cbWeaveIndex].cbPublicCard = TRUE;
						tagWeaveItemArray[cbWeaveIndex].cbCenterCard = GangCardResult.cbCardData[0];
						tagWeaveItemArray[cbWeaveIndex].WeaveKind = ACTION_GANG;
						tagWeaveItemArray[cbWeaveIndex].wProvideUser = curUser;
					}
					BYTE cbRemoveCard[] = { GangCardResult.cbCardData[0] };
					m_gameLogic.RemoveCard(cbTempCardIndex, cbRemoveCard, getArrayLen(cbRemoveCard));
				}
				//for (uint32 i = 0; i < MAX_INDEX; i++)
				//{
				//	if (cbTempCardIndex[i]>0)
				//	{
				//		LOG_DEBUG("3- uid:%d,i:%d,cbTempCardIndex:%d,cbWeaveItemCount:%d,card:0x%02X", GetPlayerID(curUser), i, cbTempCardIndex[i], cbWeaveItemCount, GangCardResult.cbCardData[0]);
				//	}
				//}

				//if (m_poolCards.size() > 0)
				//{
				//	BYTE cbTempGangGetCard = m_poolCards.back();
				//	cbTempCardIndex[m_gameLogic.SwitchToCardIndex(cbTempGangGetCard)]++;
				//	LOG_DEBUG("4- uid:%d,cbWeaveItemCount:%d,cbTempGangGetCard:0x%02X", GetPlayerID(curUser),  cbWeaveItemCount, cbTempGangGetCard);
				//}

				//for (uint32 i = 0; i < MAX_INDEX; i++)
				//{
				//	if (cbTempCardIndex[i]>0)
				//	{
				//		LOG_DEBUG("5- uid:%d,i:%d,cbTempCardIndex:%d,cbWeaveItemCount:%d,card:0x%02X", GetPlayerID(curUser), i, cbTempCardIndex[i], cbWeaveItemCount, GangCardResult.cbCardData[0]);
				//	}
				//}

				bool bIsChange = CheckUserTingCardIsChange(curUser, cbTempCardIndex, tagWeaveItemArray, cbWeaveItemCount);
				if (bIsChange == false)
				{
					m_dwUserAction[curUser] |= dwUserAction;
					m_cbProvideCard = GangCardResult.cbCardData[0];
					if (m_dwUserAction[curUser] & ACTION_GANG)
					{
						if (m_cbListenStatus[curUser] == FALSE && m_mjCfg.supportOpenGang())//长沙麻将杠听
						{
							DWORD tmp = m_gameLogic.AnalyseBuGangTing(m_cbCardIndex[curUser], m_WeaveItemArray[curUser], m_cbWeaveItemCount[curUser], GangCardResult.cbCardData[0]);
							if (tmp & ACTION_LISTEN)
							{
								m_dwUserAction[curUser] |= ACTION_GANG_TING;// 增加杠听操作
							}
						}
					}
				}
				string strGangCardData;
				for (unsigned int a = 0; a < GangCardResult.cbCardCount; ++a)
				{
					strGangCardData += CStringUtility::FormatToString("0x%02X ", GangCardResult.cbCardData[a]);
				}
				string strHandCardIndex;
				for (unsigned int a = 0; a < MAX_INDEX; ++a)
				{
					strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[curUser][a]);
				}
				vector<BYTE> allActions;
				m_gameLogic.GetAllAction(m_dwUserAction[curUser], allActions);
				string strAllActions;
				for (unsigned int a = 0; a < allActions.size(); ++a)
				{
					strAllActions += CStringUtility::FormatToString("%d ", allActions[a]);
				}
				LOG_DEBUG("robot_gang_card- roomid:%d,tableid:%d,curUser:%d,uid:%d,bIsChange:%d,strGangCardData:%s,strHandCardIndex:%s,strAllActions:%s",
					GetRoomID(), GetTableID(), curUser, GetPlayerID(curUser), bIsChange, strGangCardData.c_str(), strHandCardIndex.c_str(), strAllActions.c_str());
			}
        }
		// 听牌操作
		CheckUserTingCard(curUser);

        //牌型权位
        DWORD dwChiHuRight=0;
        if (m_bGangStatus==true) dwChiHuRight|=CHR_GANG_FLOWER;

        //胡牌判断
        BYTE cbTempCardIndex[MAX_INDEX];
        memcpy(cbTempCardIndex,m_cbCardIndex[m_wCurrentUser],sizeof(cbTempCardIndex));
        if(m_gameLogic.RemoveCard(cbTempCardIndex,m_cbSendCardData)==false)
        {
            ASSERT(FALSE);
        }
        stChiHuResult ChiHuResult;
        m_dwUserAction[curUser]|=m_gameLogic.AnalyseChiHuCard(cbTempCardIndex,m_WeaveItemArray[curUser],m_cbWeaveItemCount[curUser],m_cbSendCardData,dwChiHuRight,ChiHuResult,true);

		net::msg_majiang_send_card_rep sendCardRep;
		sendCardRep.set_cur_user(curUser);
		sendCardRep.set_card_data((m_bSendStatus == true) ? m_cbSendCardData : 0x00);
		sendCardRep.set_is_not_gang(bNotGang);
		sendCardRep.set_left_card_count(m_poolCards.size());

		for (uint32 i = 0; i<m_wPlayerCount; ++i)
		{
			sendCardRep.add_passhu_count(m_iUserPassHuCount[i]);

			//bool bIsHuOper = false;
			//vector<net::mjaction> actions;
			//if (i == curUser)
			//{
			//	SwitchUserAction(curUser, m_cbSendCardData, actions);
			//	for (uint32 j = 0; j < actions.size(); ++j)
			//	{
			//		//net::mjaction *pa = sendCardRep.add_action();
			//		//*pa = actions[j];
			//		if (actions[j].code() == 7)
			//		{
			//			bIsHuOper = true;
			//		}
			//		LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,actions.size:%d,bIsHuOper:%d,j:%d,code:%d",
			//			GetRoomID(), GetTableID(), i, GetPlayerID(i), actions.size(), bIsHuOper, j, actions[j].code());
			//	}
			//	if (bIsHuOper)
			//	{
			//		sendCardRep.add_passhu_count(m_iUserPassHuCount[i]);
			//	}
			//	else
			//	{
			//		sendCardRep.add_passhu_count(-1);
			//	}
			//	LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,actions.size:%d,bIsHuOper:%d,passhu_count:%d",
			//		GetRoomID(), GetTableID(), i, GetPlayerID(i), actions.size(), bIsHuOper, m_iUserPassHuCount[i]);
			//}
			//else
			//{
			//	if (m_iUserPassHuCount[i] > 0)
			//	{
			//		sendCardRep.add_passhu_count(m_iUserPassHuCount[i]);
			//	}
			//	else
			//	{
			//		sendCardRep.add_passhu_count(-1);
			//	}
			//}
		}

        for(uint32 i=0;i<m_wPlayerCount;++i)
        {
			/*
			bool bIsHuOper = false;
            vector<net::mjaction> actions;

            if(i==curUser)
			{
                SwitchUserAction(curUser, m_cbSendCardData, actions);
                for (uint32 j = 0; j < actions.size(); ++j)
				{
                    net::mjaction *pa = sendCardRep.add_action();
                    *pa = actions[j];
					if (actions[j].code() == 7)
					{
						bIsHuOper = true;
					}
					LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,actions.size:%d,bIsHuOper:%d,j:%d,code:%d",
						GetRoomID(), GetTableID(), i, GetPlayerID(i), actions.size(), bIsHuOper, j, actions[j].code());
				}
				if (bIsHuOper)
				{
					sendCardRep.set_passhu_count(m_iUserPassHuCount[i]);
				}
				else
				{
					sendCardRep.set_passhu_count(-1);
				}
				LOG_DEBUG("roomid:%d,tableid:%d,i:%d,uid:%d,actions.size:%d,bIsHuOper:%d,passhu_count:%d",
					GetRoomID(), GetTableID(), i, GetPlayerID(i), actions.size(), bIsHuOper, sendCardRep.passhu_count());


                BYTE cardData[MAX_COUNT];
                BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[i],cardData,MAX_COUNT);
                for(uint32 j=0;j<cardNum;++j)
				{
                    sendCardRep.add_cur_hand_card(cardData[j]);
                }

				//提示胡牌
				//net::mjaction_out_front_notify_hu * pout_front_notify_hu = sendCardRep.mutable_notify_hu();
				//if (m_cbListenStatus[curUser] == FALSE)
				//{
				//	DWORD tmp = m_gameLogic.AnalyseTingCard15(curUser, m_cbCardIndex[curUser], m_WeaveItemArray[curUser], m_cbWeaveItemCount[curUser], m_mpTingNotifyHu[curUser]);
				//}
				//LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbListenStatus:%d,m_cbSendCardData:0x%02X,m_mpTingNotifyHu.size:%d ----------------",
				//	GetPlayerID(curUser), m_cbListenStatus[curUser],m_cbSendCardData, m_mpTingNotifyHu[curUser].size());
				//auto iter_begin = m_mpTingNotifyHu[curUser].begin();
				//for (; iter_begin != m_mpTingNotifyHu[curUser].end(); iter_begin++)
				//{
				//	pout_front_notify_hu->add_out_card(iter_begin->first);
				//	net::mjaction_notify_hu* pnotify_hu = pout_front_notify_hu->add_notify_hu();

				//	vector<tagAnalyseTingNotifyHu> & vecTingNotifyHu = iter_begin->second;
				//	for (uint32 uIndex = 0; uIndex < vecTingNotifyHu.size(); uIndex++)
				//	{
				//		pnotify_hu->add_card(vecTingNotifyHu[uIndex].cbCard);
				//		pnotify_hu->add_score(vecTingNotifyHu[uIndex].score);
				//		pnotify_hu->add_count(vecTingNotifyHu[uIndex].cbCount);

				//		LOG_DEBUG("ting_card_m_mpTingNotifyHu - uid:%d,m_cbSendCardData:0x%02X,out_card:0x%02X - hu_card:0x%02X",
				//			GetPlayerID(curUser), m_cbSendCardData, iter_begin->first, vecTingNotifyHu[uIndex].cbCard);
				//	}
				//}
            }
            sendCardRep.set_card_data((m_bSendStatus == true) ? m_cbSendCardData : 0x00);
            sendCardRep.set_is_not_gang(bNotGang);
            sendCardRep.set_left_card_count(m_poolCards.size());
			*/
			if (i == curUser)
			{
				vector<net::mjaction> actions;
				SwitchUserAction(curUser, m_cbSendCardData, actions);
				for (uint32 j = 0; j < actions.size(); ++j)
				{
					net::mjaction *pa = sendCardRep.add_action();
					*pa = actions[j];
				}
				BYTE cardData[MAX_COUNT];
				BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[i], cardData, MAX_COUNT);
				for (uint32 j = 0; j < cardNum; ++j)
				{
					sendCardRep.add_cur_hand_card(cardData[j]);
				}
			}
			else
			{
				sendCardRep.clear_action();
				sendCardRep.clear_cur_hand_card();
			}
            SendMsgToClient(i,&sendCardRep, S2C_MSG_MAJIANG_SEND_CARD_REP);
        }
    }else{
        SendOperNotify();
    }

    ResetCoolTime();

    return true;
}
//响应判断
bool    CGameMaJiangTable::EstimateUserRespond(WORD wCenterUser, BYTE cbCenterCard, enEstimatKind EstimatKind)
{
    LOG_DEBUG("检查用户响应 - uid:%d,wCenterUser:%d,cbCenterCard:%d,EstimatKind:%d", GetPlayerID(wCenterUser),wCenterUser,cbCenterCard,EstimatKind);
    //变量定义
    bool bAroseAction=false;

    ResetUserOperInfo();

    //动作判断
    for(WORD i=0;i<m_wPlayerCount;i++)
    {
		CGamePlayer * pGamePlayer = GetPlayer(i);
		uint32 uid = 0;
		if (pGamePlayer != NULL)
		{
			uid = pGamePlayer->GetUID();
		}
        //用户过滤
        if(wCenterUser==i) continue;

        //出牌类型
        if(EstimatKind==EstimatKind_OutCard)
        {
            //吃碰判断
            if(m_bEnjoinChiPeng[i] == false && m_cbListenStatus[i] == 0){
                //碰牌判断
                m_dwUserAction[i] |= m_gameLogic.EstimatePengCard(m_cbCardIndex[i], cbCenterCard);

                //吃牌判断
                WORD wEatUser = GetNextUser(wCenterUser);
                if(wEatUser == i && m_mjCfg.supportEat())
				{// 仅检查支持吃牌的玩法
					bool bIsEatCard = true;
					if (cbCenterCard >= emPOKER_DONG && cbCenterCard <= emPOKER_BEI)
					{
						bIsEatCard = m_mjCfg.supportEatWind();
					}
					if (cbCenterCard >= emPOKER_ZHONG && cbCenterCard <= emPOKER_BAI)
					{
						bIsEatCard = m_mjCfg.supportEatWord();
					}
					if (bIsEatCard)
					{
						m_dwUserAction[i] |= m_gameLogic.EstimateEatCard(m_cbCardIndex[i], cbCenterCard);
					}
                }
            }
            //杠牌判断
            if(m_poolCards.size() > 1 && (m_cbListenStatus[i] == 0 || m_mjCfg.IsCanGangAfterListen()))
            {
				DWORD dwUserAction = m_gameLogic.EstimateGangCard(m_cbCardIndex[i], cbCenterCard);
				if (dwUserAction == ACTION_GANG)
				{
					BYTE cbWeaveItemCount = m_cbWeaveItemCount[i];
					WORD wIndex = cbWeaveItemCount++;

					stWeaveItem	tagWeaveItemArray[MAX_WEAVE];
					//memcpy(tagWeaveItemArray, m_WeaveItemArray[i], sizeof(tagWeaveItemArray[MAX_WEAVE]));
					for (int iIndex = 0; iIndex < MAX_WEAVE; iIndex++)
					{
						tagWeaveItemArray[iIndex] = m_WeaveItemArray[i][iIndex];
					}

					tagWeaveItemArray[wIndex].cbPublicCard = TRUE;
					tagWeaveItemArray[wIndex].cbCenterCard = cbCenterCard;
					tagWeaveItemArray[wIndex].WeaveKind = ACTION_GANG;
					tagWeaveItemArray[wIndex].wProvideUser = wCenterUser;

					BYTE cbTempCardIndex[MAX_INDEX];
					memset(cbTempCardIndex, 0, sizeof(cbTempCardIndex));
					memcpy(cbTempCardIndex, m_cbCardIndex[i], sizeof(cbTempCardIndex));

					//删除扑克
					BYTE cbRemoveCard[] = { cbCenterCard,cbCenterCard,cbCenterCard };
					m_gameLogic.RemoveCard(cbTempCardIndex, cbRemoveCard, getArrayLen(cbRemoveCard));

					//if (m_poolCards.size() > 0)
					//{
					//	BYTE cbTempGangGetCard = m_poolCards.back();
					//	cbTempCardIndex[m_gameLogic.SwitchToCardIndex(cbTempGangGetCard)]++;
					//	LOG_DEBUG("4- uid:%d,cbWeaveItemCount:%d,cbTempGangGetCard:0x%02X", GetPlayerID(i), cbWeaveItemCount, cbTempGangGetCard);
					//}

					bool bIsChange = CheckUserTingCardIsChange(i, cbTempCardIndex,tagWeaveItemArray, cbWeaveItemCount);

					// 杠牌后没有改变可以听
					if (bIsChange == false)
					{
						m_dwUserAction[i] |= dwUserAction;
						if (m_dwUserAction[i] & ACTION_GANG)
						{
							if (m_mjCfg.supportOpenGang())//长沙麻将杠听
							{
								DWORD tmp = m_gameLogic.AnalyseTingCard13(m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i]);
								if (tmp & ACTION_LISTEN) {
									m_dwUserAction[i] |= ACTION_GANG_TING;// 增加杠听操作
								}
							}
						}
					}
				}
            }
        }
        //不支持抢杠胡的,
        if(EstimatKind==EstimatKind_GangCard || EstimatKind==EstimatKind_GangTing){
            m_fangPaoUser=wCenterUser;
            if(m_mjCfg.supportQiangGang()==false){
                LOG_DEBUG("不支持抢杠胡");
                //结果判断
                if(m_dwUserAction[i]!=ACTION_NULL)
                    bAroseAction=true;
                continue;
            }
        }
        //仅自摸
        if(EstimatKind==EstimatKind_OutCard && m_mjCfg.onlyZimo()){//是否仅支持自摸
            //结果判断
            if(m_dwUserAction[i]!=ACTION_NULL)
                bAroseAction=true;
            continue;
        }

        //胡牌判断
        if(m_bEnjoinChiHu[i]==false || EstimatKind==EstimatKind_GangCard || EstimatKind==EstimatKind_GangTing || EstimatKind == EstimatKind_QiangGangHu)
        {
            //牌型权位
            DWORD dwChiHuRight=0;
            //if(m_bGangStatus==true)
            //    dwChiHuRight|=CHR_QIANG_GANG;
			if (m_TempWeaveItem.WeaveKind == ACTION_GANG)
			{
				dwChiHuRight |= CHR_QIANG_GANG;
			}
			if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 1))
			{
				dwChiHuRight |= CHR_DI;
			}
			if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 0))
			{
				dwChiHuRight |= CHR_TIAN;
			}

            //吃胡判断
            stChiHuResult ChiHuResult;
            BYTE cbWeaveCount=m_cbWeaveItemCount[i];
            m_dwUserAction[i]|=m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[i],m_WeaveItemArray[i],cbWeaveCount,cbCenterCard,dwChiHuRight,ChiHuResult);

            //平胡不接炮
            if(m_mjCfg.onlyChiDaHu() && ChiHuResult.HuKind.isOnlyType(HUTYPE_PING_HU) && dwChiHuRight == 0){
                m_dwUserAction[i] = m_dwUserAction[i]&(~ACTION_CHI_HU);
            }

            //吃胡限制
            if((m_dwUserAction[i]&ACTION_CHI_HU)!=0)
                m_bEnjoinChiHu[i]=true;
        }else{
            LOG_DEBUG("吃胡限制:%d",i);
        }

        //结果判断
        if(m_dwUserAction[i]!=ACTION_NULL)
            bAroseAction=true;
    }

    //结果处理
    if(bAroseAction==true)
    {
        //设置变量
        m_wProvideUser=wCenterUser;
        m_cbProvideCard=cbCenterCard;
        m_wResumeUser=m_wCurrentUser;
        m_wCurrentUser=INVALID_CHAIR;

        //发送提示
        SendOperNotify();

        return true;
    }

    return false;
}
// 海底操作处理
bool    CGameMaJiangTable::EstimateUserTailRespond()
{
    LOG_DEBUG("通知玩家要海底:%d",m_wCurrentUser);
    //用户状态
    ResetUserOperInfo();

    m_bGangStatus=false;

    //动作判断
    if(m_tailCardOper[m_wCurrentUser] == 0){
        m_dwUserAction[m_wCurrentUser] |= ACTION_NEED_TAIL;

        //发送提示
        SendOperNotify();
        return true;
    }else{
        return false;
    }

    return false;
}
// 要海底后操作
bool    CGameMaJiangTable::EstimateNeedTailOper(uint16 chairID)
{
    //吃胡判断
    LOG_DEBUG("玩家要海底操作:%d",chairID);

    DWORD dwChiHuRight=0;
    BYTE cbWeaveCount=m_cbWeaveItemCount[chairID];
    m_dwUserAction[chairID] |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[chairID],m_WeaveItemArray[chairID],cbWeaveCount,m_tailCard,dwChiHuRight,m_ChiHuResult[chairID],true);

    //胡牌操作
    if(m_dwUserAction[chairID]&ACTION_CHI_HU)
    {
        //结束信息
        m_cbChiHuCard=m_tailCard;
        m_wProvideUser=chairID;
        m_cbCardIndex[chairID][m_gameLogic.SwitchToCardIndex(m_cbChiHuCard)]++;
        //检查牌型
        m_ChiHuResult[chairID].dwChiHuRight |= CHR_HAIDILAOYUE;

        LOG_DEBUG("海底胡牌结束:%d--%s",chairID,m_gameLogic.PrintHuRightType(m_ChiHuResult[chairID].dwChiHuRight).c_str());
        m_fangPaoUser=chairID;
        //结束游戏
        OnGameEnd(chairID,GER_NORMAL);
        return true;
    }else{//不能胡牌,则出牌
        LOG_DEBUG("要海底后不能胡牌,出牌");
        //设置变量
        m_bSendStatus=true;
        m_bEnjoinChiHu[chairID]=true;
        for(WORD i=0;i<m_wPlayerCount;i++)
        {
            if (i==chairID) continue;
            else
                m_bEnjoinChiHu[i] = false;
        }
        m_dwUserAction[chairID] = ACTION_NULL;
        m_dwPerformAction[chairID] = ACTION_NULL;

        //出牌记录
        m_cbOutCardCount++;
        m_wOutCardUser  = chairID;
        m_cbOutCardData = m_tailCard;
        m_fangPaoUser=chairID;

        //广播出牌
        net::msg_majiang_out_card_rep outCardMsg;
        outCardMsg.set_out_card_data(m_cbOutCardData);
        outCardMsg.set_out_card_user(m_wOutCardUser);
        SendMsgToAll(&outCardMsg,S2C_MSG_MAJIANG_OUT_CARD_REP);

        //公共亮牌
        vector<BYTE> cards;
        cards.push_back(m_tailCard);
        ShowPublicCards(chairID,ACTION_NEED_TAIL,cards);

        //用户切换
        m_wProvideUser  = chairID;
        m_cbProvideCard = m_cbOutCardData;
        m_wCurrentUser  = GetNextUser(chairID);
        m_bGangStatus=false;
        //响应判断
        bool bAroseAction = EstimateUserRespond(chairID,m_cbOutCardData,EstimatKind_Tail);

        //抢杆设置
        if(m_bGangStatus==true)
        {
            WORD i=0;
            for(i=0;i<m_wPlayerCount;i++)
            {
                if((m_dwUserAction[i]&ACTION_CHI_HU)!=0) break;
            }
            if(i==m_wPlayerCount)
            {
                m_bGangStatus=false;
            }
        }

        FlushDeskCardToClient();

        //派发扑克
        if(bAroseAction==false) {
            LOG_DEBUG("要海底后无人操作.发牌流局:%d",m_wCurrentUser);
            DispatchCardData(m_wCurrentUser);
        }

        return true;
    }

    return false;
}
// 杠听后操作
bool    CGameMaJiangTable::EstimateGangTing(uint16 chairID)
{
    LOG_DEBUG("杠听后操作:%d",chairID);
    // 杠听后,翻开补张牌,看看能不能胡,能胡,则胡最大的  杠上开花
    // 否则检测其它人有没有胡操作,杠上炮
    uint8 gangCardNum = m_mjCfg.GetOpenGangCards();
    m_gangTingCards.clear();
    for(uint8 i=0;i<gangCardNum && m_poolCards.size() > 0;++i){
        BYTE card = PopCardFromPool();
        m_gangTingCards.push_back(card);
    }
    if(m_gangTingCards.size() > 0){
        //m_gangTingState = emGANG_TING_SELF_HU;
        m_cbListenStatus[chairID] = 1;
        NotifyListen(chairID);
        // 亮牌
        ShowPublicCards(chairID,ACTION_GANG_TING,m_gangTingCards);
    }else{
        m_wCurrentUser  = GetNextUser(chairID);
        LOG_DEBUG("杠听后没牌了,流局发牌:%d",m_wCurrentUser);
        DispatchCardData(m_wCurrentUser);
        return false;
    }

    ResetUserOperInfo();

    // 检查自己能否胡牌
    for(uint32 i=0;i<m_gangTingCards.size();++i)
    {
        //牌型权位
        DWORD dwChiHuRight = 0;
        dwChiHuRight |= CHR_GANG_FLOWER;
        if((m_cbSendCardCount==1)&&(m_cbOutCardCount==1))
            dwChiHuRight|=CHR_DI;
        if((m_cbSendCardCount==1)&&(m_cbOutCardCount==0))
            dwChiHuRight|=CHR_TIAN;
        //吃胡判断
        BYTE cbWeaveCount = m_cbWeaveItemCount[chairID];
        BYTE cbCenterCard = m_gangTingCards[i];
        stChiHuResult ChiHuResult;
        m_dwUserAction[chairID] |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[chairID], m_WeaveItemArray[chairID], cbWeaveCount, cbCenterCard, dwChiHuRight, ChiHuResult);
        if (m_dwUserAction[chairID] & ACTION_CHI_HU) {
            //设置变量
            m_wProvideUser = chairID;
            m_cbProvideCard = cbCenterCard;
            m_wResumeUser = chairID;
            m_wCurrentUser = INVALID_CHAIR;

            //发送提示
            SendOperNotify();
            LOG_DEBUG("杠听后能胡牌:%d",chairID);
            return true;
        }
    }
    LOG_DEBUG("杠听后自己不能胡,开启抢杠胡流程:%d",chairID);
    EstimateGangTingQiang(chairID);

    return false;
}
bool    CGameMaJiangTable::EstimateGangTingQiang(uint16 chairID)
{
    LOG_DEBUG("抢杠听操作:%d",chairID);
    //变量定义
    bool bAroseAction=false;

    //用户状态
    ResetUserOperInfo();

    //动作判断
    for (WORD i=0;i<m_wPlayerCount;i++)
    {
        //用户过滤
        if(chairID==i) continue;
        //胡牌判断
        //牌型权位
        DWORD dwChiHuRight=0;
        dwChiHuRight|=CHR_GANG_SHANG_PAO;
        if((m_cbSendCardCount==1)&&(m_cbOutCardCount==1))
            dwChiHuRight|=CHR_DI;
        if((m_cbSendCardCount==1)&&(m_cbOutCardCount==0))
            dwChiHuRight|=CHR_TIAN;

        //吃胡判断
        stChiHuResult ChiHuResult;
        BYTE cbWeaveCount=m_cbWeaveItemCount[i];
        for(uint32 j=0;j<m_gangTingCards.size();++j)
        {
            BYTE cbCenterCard = m_gangTingCards[j];
            m_dwUserAction[i] |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[i], m_WeaveItemArray[i], cbWeaveCount, cbCenterCard, dwChiHuRight, ChiHuResult);

            //吃胡限制
            if((m_dwUserAction[i] & ACTION_CHI_HU) != 0){
                break;
            }
        }

        //结果判断
        if(m_dwUserAction[i]!=ACTION_NULL)
            bAroseAction=true;
    }
    //结果处理
    if(bAroseAction==true)
    {
        //设置变量
        m_wProvideUser=chairID;
        m_cbProvideCard=0;
        m_wResumeUser=chairID;
        m_wCurrentUser=INVALID_CHAIR;

        //发送提示
        SendOperNotify();
        //m_gangTingState = emGANG_TING_OTHER_HU;
        return true;
    }

    GangTingNoHuEnd(chairID);

    return false;
}
// 计算玩家胡牌结果
void    CGameMaJiangTable::CalcPlayerHuResult(uint16 chairID,uint32 dwChiHuRight,bool bAction)
{
	DWORD dwIsHuCard = ACTION_NULL;
    //普通胡牌
    if(m_cbChiHuCard != 0)
    {
        LOG_DEBUG("普通胡牌 - uid:%d,chairid:%d--%s", GetPlayerID(chairID),chairID,m_gameLogic.GetCardName(m_cbChiHuCard).c_str());
        BYTE cbWeaveItemCount = m_cbWeaveItemCount[chairID];
        stWeaveItem *pWeaveItem = m_WeaveItemArray[chairID];
        BYTE cbTempCardIndex[MAX_INDEX];
        memcpy(cbTempCardIndex,m_cbCardIndex[chairID],sizeof(cbTempCardIndex));
		if (bAction)
		{
			if (m_gameLogic.RemoveCard(cbTempCardIndex, m_cbChiHuCard) == false)
			{
				ASSERT(FALSE);
			}
		}
		m_gameLogic.AnalyseTingCard11(cbTempCardIndex, pWeaveItem, cbWeaveItemCount, m_vecArrUserHuCard[chairID]);
		
        if(!bAction)
		{
            LOG_DEBUG("被动胡牌 - uid:%d,chairID:%d",GetPlayerID(chairID),chairID);
			dwIsHuCard = m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[chairID], pWeaveItem, cbWeaveItemCount, m_cbChiHuCard,dwChiHuRight, m_ChiHuResult[chairID], false);

            if(m_ChiHuResult[chairID].IsHu())
			{
				 m_cbCardIndex[chairID][m_gameLogic.SwitchToCardIndex(m_cbChiHuCard)]++;				
            }
        }
		else
		{
            LOG_DEBUG("主动胡牌 - uid:%d,chairID:%d", GetPlayerID(chairID), chairID);

			dwIsHuCard = m_gameLogic.AnalyseChiHuCard(cbTempCardIndex,pWeaveItem,cbWeaveItemCount,m_cbChiHuCard,dwChiHuRight,m_ChiHuResult[chairID],true);
        }
    }
	else
	{
		LOG_DEBUG("胡牌错误 - uid:%d,chairID:%d,m_cbChiHuCard:0x%02X", GetPlayerID(chairID), chairID, m_cbChiHuCard);

  //      if(m_gangTingState == emGANG_TING_SELF_HU || m_gangTingState == emGANG_TING_OTHER_HU )
		//{
  //          CheckGangTingHu(chairID);
  //      }
		//else
		//{
  //          ASSERT(false);
  //      }
    }

}


// 检查是否胡海底
bool    CGameMaJiangTable::CheckHuTail()
{
    if(!m_mjCfg.supportTail() || m_tailCard==0 || m_cbChiHuCard != m_tailCard) {
        LOG_DEBUG("海底牌:%s--胡牌:%s",m_gameLogic.GetCardName(m_tailCard).c_str(),m_gameLogic.GetCardName(m_cbChiHuCard).c_str());
        return false;
    }
    for(uint16 i=0;i<m_wPlayerCount;++i){
        if(!m_ChiHuResult[i].HuKind.isNull()){
            m_ChiHuResult[i].dwChiHuRight |= CHR_QIANG_HAIDI;
        }
    }

    return false;
}
// 计算杠听胡(胡最大的牌)
bool    CGameMaJiangTable::CheckGangTingHu(uint16 chairID)
{
    LOG_DEBUG("检查杠听胡:%d--%d--%d",chairID,m_gangTingState,m_gangTingCards.size());
    //牌型权位
    DWORD dwChiHuRight=0;
    if(m_gangTingState == emGANG_TING_OTHER_HU){
        dwChiHuRight |= CHR_GANG_SHANG_PAO;
    }else{
        dwChiHuRight |= CHR_GANG_FLOWER;
    }
    if(chairID == m_wProvideUser){// 杠上开花算不算自摸? test
        dwChiHuRight |= CHR_ZI_MO;
    }
    //杠听下没有天地胡

    for(uint32 i=0;i<m_gangTingCards.size();++i)
    {
        //吃胡判断
        stChiHuResult ChiHuResult;
        BYTE cbCenterCard = m_gangTingCards[i];
        m_dwUserAction[chairID] |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], cbCenterCard, dwChiHuRight, ChiHuResult);
        //吃胡限制
        if((m_dwUserAction[chairID] & ACTION_CHI_HU) != 0)
        {
            if(CalcBaseFanCount(chairID,ChiHuResult) > CalcBaseFanCount(chairID,m_ChiHuResult[chairID])){
                m_ChiHuResult[chairID] = ChiHuResult;
            }
        }
    }
    if(!m_ChiHuResult[chairID].HuKind.isNull()) {
        m_cbCardIndex[chairID][m_gameLogic.SwitchToCardIndex(m_ChiHuResult[chairID].ChiHuCard)]++;
    }else{
        LOG_DEBUG("杠听胡失败********************* %d",chairID);
    }
    return true;
}
uint16  CGameMaJiangTable::GetNextUser(uint16 chairID)
{
    return (chairID+1)%m_wPlayerCount;
}
void    CGameMaJiangTable::ResetCoolTime() {
    if(GetGameState() == TABLE_STATE_PLAY){
        //LOG_DEBUG("重置逻辑CD时间");
        m_coolLogic.beginCooling(s_OutCardTime);
        m_coolRobot.beginCooling(s_OutCardTime);
    }
}
void    CGameMaJiangTable::SwitchUserAction(uint16 chairID,BYTE tarCard,vector<net::mjaction>& actions)
{
    actions.clear();
    vector<BYTE> acts;
    if(m_dwUserAction[chairID] == ACTION_NULL)return;

	uint32 uid = 0;
	CGamePlayer * pGamePlayer = GetPlayer(chairID);
	if (pGamePlayer != NULL)
	{
		uid = pGamePlayer->GetUID();
	}
    m_gameLogic.GetAllAction(m_dwUserAction[chairID],acts);
    for(uint32 i=0;i<acts.size();++i)
    {
		LOG_DEBUG("actions - uid:%d,i:%d,acts:%d,m_dwUserAction:%d", uid, i, acts[i], m_dwUserAction[chairID]);

        if(acts[i] == 5 || acts[i] == 10)//杠牌(杠听)特殊处理
        {
            if(m_wCurrentUser == chairID)
			{//暗杠
                //杠牌判断
                stGangCardResult GangCardResult;

				//for (BYTE j = 0; j<MAX_INDEX; j++)
				//{
				//	LOG_DEBUG("gang_card - uid:%d,j:%d,index:%d,king:%d,card:0x%02X", uid, j,m_cbCardIndex[chairID][i], m_gameLogic.IsKingCardIndex(i), m_gameLogic.SwitchToCardData(i));

				//}
                m_gameLogic.AnalyseGangCard(m_cbCardIndex[chairID],m_WeaveItemArray[chairID],m_cbWeaveItemCount[chairID],GangCardResult);
                for(uint32 j=0;j<GangCardResult.cbCardCount;++j)
				{
                    net::mjaction a;
                    a.set_card(GangCardResult.cbCardData[j]);
                    //a.set_code(acts[i]);
					a.set_code(GangCardResult.cbGangType[j]);
                    actions.push_back(a);

					LOG_DEBUG("gang_card_buan - uid:%d,type:%d,card:0x%02X", uid,a.code(),a.card());

                }
            }
			else 
			{
                //明杠
                if (m_gameLogic.EstimateGangCard(m_cbCardIndex[chairID], tarCard))
				{
                    net::mjaction a;
                    a.set_card(tarCard);
                    //a.set_code(acts[i]);
					a.set_code(11);
                    actions.push_back(a);
					LOG_DEBUG("gang_card_ming - uid:%d,type:%d,card:0x%02X", uid, a.code(), a.card());

                }
            }
            continue;
        }
		else if(acts[i] == 7 && m_gangTingState != emGANG_TING_NULL && m_gangTingCards.size() > 0) //杠听或者抢杠听吃胡
		{
            for(uint32 j=0;j<m_gangTingCards.size();++j)
            {
                //吃胡判断
                stChiHuResult ChiHuResult;
                BYTE cbCenterCard = m_gangTingCards[j];
                uint32 action = 0;
                action |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[chairID], m_WeaveItemArray[chairID],m_cbWeaveItemCount[chairID], cbCenterCard, 0,ChiHuResult);
                //吃胡限制
                if((action & ACTION_CHI_HU) != 0)
                {
                    net::mjaction a;
                    a.set_code(acts[i]);
                    a.set_card(cbCenterCard);
                    actions.push_back(a);

					LOG_DEBUG("else_if_ting_hu - uid:%d,code:%d,card:0x%02X", uid, a.code(), a.card());
                }
            }
        }
		else if(acts[i] == 6)
		{//听牌
			vector<BYTE> tingCards;
			DWORD tmp = m_gameLogic.AnalyseTingCard14(m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], tingCards);
			for (uint32 j = 0; j<tingCards.size(); ++j)
			{
				net::mjaction a;
				a.set_code(acts[i]);
				a.set_card(tingCards[j]);
				actions.push_back(a);

				LOG_DEBUG("ting_card - uid:%d,tingCards:0x%02X", uid, tingCards[j]);

			}
        }
        else if(acts[i] == 7)
		{// 胡
            net::mjaction a;
            a.set_card(tarCard);
            a.set_code(acts[i]);
            actions.push_back(a);

			LOG_DEBUG("code_7 - uid:%d,code:%d,card:0x%02X", uid,a.code(),a.card());
        }
		else
		{
			net::mjaction a;
			a.set_card(tarCard);
			a.set_code(acts[i]);
			actions.push_back(a);

			LOG_DEBUG("else - uid:%d,code:%d,card:0x%02X", uid, a.code(), a.card());
		}
    }
    LOG_DEBUG("转换玩家操作:uid:%d,%d--%d",uid,m_dwUserAction[chairID],actions.size());
}
//获得牌数
BYTE    CGameMaJiangTable::GetCardCount(uint16 chairID,BYTE card)
{
    return m_cbCardIndex[chairID][m_gameLogic.SwitchToCardIndex(card)];
}
//选择一张出牌
BYTE    CGameMaJiangTable::GetAIOutCard(uint16 chairID,int & iOutIndex)
{
    BYTE outCard=0xff;
	iOutIndex = 0;
	//有风打风
	for (int i = 27; i<27 + 7; i++)
	{
		if (m_cbCardIndex[chairID][i] == 1)
		{
			outCard = m_gameLogic.SwitchToCardData(i);
			iOutIndex = 1;
			return outCard;
		}
	}
	//打19孤牌
	for (int i = 0; i<1; i++)
	{
		if (m_cbCardIndex[chairID][i * 9] == 1)
		{
			if ((m_cbCardIndex[chairID][i * 9 + 1] == 0)&& (m_cbCardIndex[chairID][i * 9 + 2] == 0))
			{
				outCard = m_gameLogic.SwitchToCardData(i * 9);
				iOutIndex = 2;
				return outCard;
			}
		}
		if (m_cbCardIndex[chairID][i * 9 + 8] == 1)
		{
			if ((m_cbCardIndex[chairID][i * 9 + 7] == 0)&& (m_cbCardIndex[chairID][i * 9 + 6] == 0))
			{
				outCard = m_gameLogic.SwitchToCardData(i * 9 + 8);
				iOutIndex = 3;
				return outCard;
			}
		}
	}

	//打普通卡张
	for (int i = 0; i<9; i++)
	{
		if (i == 0 || i == 8)
		{
			continue;
		}
		if (m_cbCardIndex[chairID][i] == 1)
		{
			if (m_cbCardIndex[chairID][i + 1] == 0 && m_cbCardIndex[chairID][i - 1] == 0)
			{
				outCard = m_gameLogic.SwitchToCardData(i);
				iOutIndex = 4;
				return outCard;
			}
		}
	}

    //单张不靠
    for(uint8 j=0;j<MAX_INDEX;++j)
	{
        if(m_cbCardIndex[chairID][j] ==1 && m_cbCardIndex[chairID][j+1]==0)
		{//单张
            outCard=m_gameLogic.SwitchToCardData(j);
			iOutIndex = 5;
            return outCard;
        }
		else
		{
            j +=2;
        }
    }

	// 拆单靠牌
	for (int i = 0; i<9; i++)
	{
		if (i == 0 && m_cbCardIndex[chairID][i] == 1)
		{
			if (m_cbCardIndex[chairID][i + 1] != 0 && m_cbCardIndex[chairID][i + 2] == 0)
			{
				outCard = m_gameLogic.SwitchToCardData(i);
				iOutIndex = 6;
				return outCard;
			}
			continue;
		}
		if (i == 8 && m_cbCardIndex[chairID][i] == 1)
		{
			if (m_cbCardIndex[chairID][i - 1] != 0 && m_cbCardIndex[chairID][i - 2] == 0)
			{
				outCard = m_gameLogic.SwitchToCardData(i);
				iOutIndex = 7;
				return outCard;
			}
			continue;
		}
		if (m_cbCardIndex[chairID][i] == 1)
		{
			if (m_cbCardIndex[chairID][i + 1] != 0 && m_cbCardIndex[chairID][i - 1] == 0)
			{
				outCard = m_gameLogic.SwitchToCardData(i);
				iOutIndex = 8;
				return outCard;
			}
		}
	}

	map<BYTE,uint32> mpFengCardCount;
	for (int i = 27; i<27 + 7; i++)
	{
		if (m_cbCardIndex[chairID][i] == 2)
		{
			BYTE cbCard = m_gameLogic.SwitchToCardData(i);
			uint32 uiCount = m_gameLogic.GetAnalyseChiHuCardPoolCount(chairID, cbCard);
			mpFengCardCount.insert(make_pair(cbCard, uiCount));
		}
	}
	if (mpFengCardCount.size() > 0)
	{
		uint32 uMaxCountValue = 0;
		BYTE cbMaxCountCard = 0xff;
		for (auto iter_feng_dui = mpFengCardCount.begin(); iter_feng_dui != mpFengCardCount.end(); iter_feng_dui++)
		{
			if (iter_feng_dui->second > uMaxCountValue)
			{
				cbMaxCountCard = iter_feng_dui->first;
				uMaxCountValue = iter_feng_dui->second;
			}
		}
		if (uMaxCountValue != 0 && cbMaxCountCard != 0xff)
		{
			outCard = cbMaxCountCard;
			iOutIndex = 9;
			return outCard;
		}
	}

	map<BYTE, uint32> mpWanCardCount;
	for (int i = 0; i<9; i++)
	{
		if (m_cbCardIndex[chairID][i] == 2)
		{
			BYTE cbCard = m_gameLogic.SwitchToCardData(i);
			uint32 uiCount = m_gameLogic.GetAnalyseChiHuCardPoolCount(chairID, cbCard);
			mpWanCardCount.insert(make_pair(cbCard, uiCount));
		}
	}
	if (mpWanCardCount.size() > 0)
	{
		uint32 uMaxCountValue = 0;
		BYTE cbMaxCountCard = 0xff;
		for (auto iter_wan_dui = mpWanCardCount.begin(); iter_wan_dui != mpWanCardCount.end(); iter_wan_dui++)
		{
			if (iter_wan_dui->second > uMaxCountValue)
			{
				cbMaxCountCard = iter_wan_dui->first;
				uMaxCountValue = iter_wan_dui->second;
			}
		}
		if (uMaxCountValue != 0 && cbMaxCountCard != 0xff)
		{
			outCard = cbMaxCountCard;
			iOutIndex = 10;
			return outCard;
		}
	}

	// 打单张
	for (uint8 j = 0; j<MAX_INDEX; ++j)
	{
		if (m_cbCardIndex[chairID][j] == 1)
		{//单张
			outCard = m_gameLogic.SwitchToCardData(j);
			iOutIndex = 11;
			return outCard;
		}
	}
    //for(uint8 j=0;j<MAX_INDEX;++j){
    //    if(m_cbCardIndex[chairID][j] > 0 && !m_gameLogic.IsKingCardIndex(j)){
    //        outCard=m_gameLogic.SwitchToCardData(j);
    //    }
    //}
	if (outCard==0xff)
	{
		for (uint8 j = 0; j<MAX_INDEX; ++j) {
			if (m_cbCardIndex[chairID][j] > 0 ) {
				outCard = m_gameLogic.SwitchToCardData(j);
			}
		}
	}
	iOutIndex = 12;
    return outCard;
}
//test 随机换位
void    CGameMaJiangTable::RandChangeSeat(uint16 chairID)
{
    uint8 newPos = g_RandGen.RandUInt()%m_wPlayerCount;
    if(newPos==chairID){
        newPos=GetNextUser(newPos);
    }
    std::swap(m_vecPlayers[chairID],m_vecPlayers[newPos]);
    SendSeatInfoToClient();
    SendReadyStateToClient();

}
void    CGameMaJiangTable::CheckAddRobot()
{
    if(m_pHostRoom->GetRobotCfg() == 0 || !m_coolRobot.isTimeOut())
        return;
    for(uint32 i=0;i<m_vecPlayers.size();++i)
    {
        CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
        if(pPlayer != NULL && pPlayer->IsRobot())
        {
			for (uint32 j = 0; j<m_vecPlayers.size(); ++j)
			{
				if (pPlayer->IsExistBlocker(m_vecPlayers[j].uid))
				{
					RenewFastJoinTable(pPlayer);
					return;
				}
			}
            if(m_vecPlayers[i].readyState == 0)
			{
                PlayerReady(pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(100,500));
                return;
            }
			else
			{
                if((getSysTime() - m_vecPlayers[i].readyTime) > 200)
				{
                    if(CanLeaveTable(pPlayer))
					{
                        LeaveTable(pPlayer);
                        m_coolRobot.beginCooling(g_RandGen.RandRange(100, 500));
                        return;
                    }
                }
            }
        }
    }
    if(GetPlayerNum() >= 1 && GetPlayerNum() < m_conf.seatNum)
    {
        for(uint32 i=0;i<m_vecPlayers.size();++i)
        {
            CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
            if(pPlayer != NULL && !pPlayer->IsRobot())// 有玩家存在
            {
                CRobotMgr::Instance().RequestOneRobot(this, pPlayer);
                m_coolRobot.beginCooling(g_RandGen.RandRange(100,500));
                return;
            }
        }
    }
}
//机器人出牌
bool    CGameMaJiangTable::OnRobotOutCard()
{
	if (m_coolLogic.getPassTick() < 2000)
	{
		return false;
	}

    for(uint16 i=0;i<m_wPlayerCount;++i)
	{
        CGamePlayer* player = GetPlayer(i);
        if(player==NULL || !player->IsRobot())
            continue;
        if(m_bResponse[i] == true)
            continue;
        if(m_dwUserAction[i] != ACTION_NULL)
		{
			vector<BYTE> allActions;
			m_gameLogic.GetAllAction(m_dwUserAction[i], allActions);
			uint32 operCode = m_gameLogic.GetOneAIAction(m_dwUserAction[i], m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], m_cbProvideCard, i, m_cbListenStatus);

			string strAllActions;
			for (unsigned int a = 0; a < allActions.size(); ++a)
			{
				strAllActions += CStringUtility::FormatToString("%d ", allActions[a]);
			}

            //LOG_DEBUG("当前机器人操作:%d--%s",i,m_gameLogic.GetOperCodeName(operCode).c_str());
			LOG_DEBUG("robot_oper_card - roomid:%d,tableid:%d,uid:%d - %d %d,m_dwUserAction:%d,operCode:%d,i:%d,m_wCurrentUser:%d, m_cbProvideCard:0x%02X, m_cbSendCardData:0x%02X,strAllActions:%s",
				GetRoomID(),GetTableID(), player->GetUID(),GetPlayerID(0),GetPlayerID(1), m_dwUserAction,operCode, i,m_wCurrentUser, m_cbProvideCard, m_cbSendCardData, strAllActions.c_str());

            if(i != m_wCurrentUser)
			{
                if(operCode != 0)
                    operCode = 1<<operCode;
                BYTE operCard = m_cbProvideCard;
                if(operCard==0)
                    operCard = m_cbSendCardData;
                OnUserOperCard(i,operCode,operCard);
                continue;
            }
			if (operCode == 5)
			{
				if (operCode != 0)
					operCode = 1 << operCode;
				BYTE operCard = m_cbProvideCard;
				if (operCard == 0)
					operCard = m_cbSendCardData;
				OnUserOperCard(i, operCode, operCard);
			}
			if (operCode == 6)
			{
				if (operCode != 0)
					operCode = 1 << operCode;
				BYTE operCard = 0xff;
				BYTE cbCountCard = 0;
				map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
				m_gameLogic.AnalyseTingCard17(i, m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], mpTingNotifyHu);

				string strOutCardTing;

				auto iter_notify_hu = mpTingNotifyHu.begin();
				for (; iter_notify_hu != mpTingNotifyHu.end(); iter_notify_hu++)
				{
					BYTE cbCurHuCount = 0;
					for (uint32 h = 0; h < iter_notify_hu->second.size(); h++)
					{
						cbCurHuCount += iter_notify_hu->second[h].cbCount;
					}
					if (cbCountCard < cbCurHuCount)
					{
						operCard = iter_notify_hu->first;
						cbCountCard = cbCurHuCount;
					}
					strOutCardTing += CStringUtility::FormatToString("cbCurHuCount:%d-0x%02X ", cbCurHuCount, iter_notify_hu->first);
				}
				if (operCard != 0xff && cbCountCard != 0)
				{
					LOG_DEBUG("robot_ting_card - roomid:%d,tableid:%d,uid:%d - %d %d,m_dwUserAction:%d,operCode:%d,i:%d,m_wCurrentUser:%d, operCard:0x%02X, mpTingNotifyHu.size():%d,strAllActions:%s,strOutCardTing:%s",
						GetRoomID(), GetTableID(), player->GetUID(), GetPlayerID(0), GetPlayerID(1), m_dwUserAction, operCode, i, m_wCurrentUser, operCard, mpTingNotifyHu.size(), strAllActions.c_str(), strOutCardTing.c_str());
					OnUserOperCard(i, operCode, operCard);
					continue;
				}
			}
			if (operCode == 7)
			{
				if (operCode != 0)
					operCode = 1 << operCode;
				BYTE operCard = m_cbProvideCard;
				if (operCard == 0)
					operCard = m_cbSendCardData;
				OnUserOperCard(i, operCode, operCard);
				continue;
			}
        }
        if(m_wOutCardUser == INVALID_CHAIR && i==m_wCurrentUser)
        {
            // 等别人操作完
            for(uint32 j=0;j<m_wPlayerCount;++j)
			{
                if(j != i && m_dwUserAction[j] != ACTION_NULL && m_bResponse[j]==false)
                    return false;
            }
			int iOutIndex;
			BYTE cbOutCardData = GetAIOutCard(i, iOutIndex);
            //LOG_DEBUG("机器人自动出牌:%d,cardData:0X%02x",m_wCurrentUser, cardData);

			string strHandCardIndex;
			int iHandCardCount = 0;
			for (unsigned int j = 0; j < MAX_INDEX; ++j)
			{
				strHandCardIndex += CStringUtility::FormatToString("%d, ", m_cbCardIndex[i][j]);
				iHandCardCount += m_cbCardIndex[i][j];
			}

			LOG_DEBUG("robot_auto_out_card - uid:%d - %d %d,m_wCurrentUser:%d,cbOutCardData:0x%02X,iOutIndex:%d,iHandCardCount:%d,strHandCardIndex:%s",
				GetPlayerID(m_wCurrentUser), GetPlayerID(0), GetPlayerID(1), m_wCurrentUser, cbOutCardData, iOutIndex, iHandCardCount, strHandCardIndex.c_str());


            OnUserOutCard(i, cbOutCardData);
            return true;
        }
    }
    return true;
}
//超时自动放弃
bool    CGameMaJiangTable::OnTimeOutOper()
{
    for(uint16 i=0;i<m_wPlayerCount;++i)
	{
        CGamePlayer* player = GetPlayer(i);
		if (player == NULL)
		{
			continue;
		}
		if (m_bResponse[i] == true)
		{
			continue;
		}
        //if(m_dwUserAction[i] != ACTION_NULL && i != m_wCurrentUser)
		if (m_dwUserAction[i] != ACTION_NULL)
		{
            uint32 operCode = m_gameLogic.GetOneAction(m_dwUserAction[i]);
            LOG_DEBUG("超时自动放弃操作 - uid:%d,i:%d,operCode:%d,m_uTimeOutCount:%d",GetPlayerID(i),i, operCode, m_uTimeOutCount[i]);
			m_uTimeOutCount[i]++;
			if (m_uTimeOutCount[i] >= 2)
			{
				m_bTrustee[i] = true;

				//托管广播
				net::msg_majiang_oper_trustee_broadcast broad;
				broad.set_uid(GetPlayerID(i));
				broad.set_trustee(1);
				SendMsgToAll(&broad, net::S2C_MSG_MAJIANG_OPER_TRUSTEE_BROADCAST);

			}
			OnUserOperCard(i, ACTION_NULL, 0);
            continue;
        }
        if(m_wOutCardUser == INVALID_CHAIR && i == m_wCurrentUser)
		{
			string strHandCardIndex;
			int iHandCardCount = 0;
			for (unsigned int j = 0; j < MAX_INDEX; ++j)
			{
				strHandCardIndex += CStringUtility::FormatToString("%d, ", m_cbCardIndex[i][j]);
				iHandCardCount += m_cbCardIndex[i][j];
			}
			int iOutIndex;
			BYTE cbOutCardData = GetAIOutCard(i, iOutIndex);

			LOG_DEBUG("time_out_card - uid:%d - %d %d,m_wCurrentUser:%d,m_uTimeOutCount:%d,cbOutCardData:0x%02X,iOutIndex:%d,iHandCardCount:%d,strHandCardIndex:%s",
				GetPlayerID(m_wCurrentUser), GetPlayerID(0), GetPlayerID(1), m_wCurrentUser, m_uTimeOutCount[i], cbOutCardData, iOutIndex,iHandCardCount, strHandCardIndex.c_str());

			m_uTimeOutCount[i]++;
			if (m_uTimeOutCount[i] >= 2)
			{
				m_bTrustee[i] = true;

				//托管广播
				net::msg_majiang_oper_trustee_broadcast broad;
				broad.set_uid(GetPlayerID(i));
				broad.set_trustee(1);
				SendMsgToAll(&broad, net::S2C_MSG_MAJIANG_OPER_TRUSTEE_BROADCAST);
			}
			OnUserOutCard(i, cbOutCardData);
            return true;
        }
    }

    return true;
}
//听牌自动出牌
bool    CGameMaJiangTable::OnListenOutCard()
{
	if (m_coolLogic.getPassTick() < 500)
	{
		return false;
	}
	if (m_wOutCardUser != INVALID_CHAIR)
	{
		return false;
	}
	if (m_wCurrentUser == INVALID_CHAIR)
	{
		return false;
	}		
	if (m_cbListenStatus[m_wCurrentUser] == 0)
	{
		return false;
	}
	if (m_dwUserAction[m_wCurrentUser] != ACTION_NULL)
	{
		return false;
	}        
    if(m_bResponse[m_wCurrentUser] == false && m_wCurrentUser <= m_wPlayerCount)
    {
        LOG_DEBUG("听牌自动出牌 - m_wCurrentUser:%d uid:%d",m_wCurrentUser,GetPlayerID(m_wCurrentUser));
        OnUserOutCard(m_wCurrentUser,m_cbSendCardData);
    }
    return true;
}

//托管自动出牌
bool    CGameMaJiangTable::OnTrusteeOutCard()
{
	if (m_coolLogic.getPassTick() < 2000)
	{
		return false;
	}
	if (m_wOutCardUser != INVALID_CHAIR)
	{
		return false;
	}
	if (m_wCurrentUser == INVALID_CHAIR)
	{
		return false;
	}
	if (m_wCurrentUser >= m_wPlayerCount)
	{
		return false;
	}
	if (m_bTrustee[m_wCurrentUser] == false)
	{
		return false;
	}
	if (m_dwUserAction[m_wCurrentUser] != ACTION_NULL)
	{
		return false;
	}
	if (m_bResponse[m_wCurrentUser] == false)
	{
		LOG_DEBUG("托管自动出牌 - m_wCurrentUser:%d uid:%d", m_wCurrentUser, GetPlayerID(m_wCurrentUser));
		OnUserOutCard(m_wCurrentUser, m_cbSendCardData);
	}
	return true;
}

//托管自动操作
bool    CGameMaJiangTable::OnTrusteeOperCard()
{
	if (m_coolLogic.getPassTick() < 2000)
	{
		return false;
	}
	for (uint16 i = 0; i<m_wPlayerCount; ++i)
	{
		CGamePlayer* player = GetPlayer(i);
		if (player == NULL)
		{
			continue;
		}
		if (m_bTrustee[i] == false)
		{
			continue;
		}
		if (m_bResponse[i] == true)
		{
			continue;
		}
		if (m_dwUserAction[i] != ACTION_NULL)
		{


			vector<BYTE> allActions;
			m_gameLogic.GetAllAction(m_dwUserAction[i], allActions);
			//uint32 operCode = m_gameLogic.GetOneAIAction(m_dwUserAction[i], m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], m_cbProvideCard, i, m_cbListenStatus);
			string strAllActions;
			for (unsigned int a = 0; a < allActions.size(); ++a)
			{
				strAllActions += CStringUtility::FormatToString("%d ", allActions[a]);
			}
			//LOG_DEBUG("当前机器人操作:%d--%s",i,m_gameLogic.GetOperCodeName(operCode).c_str());
			LOG_DEBUG("trustee_oper_card - roomid:%d,tableid:%d,uid:%d - %d %d,m_dwUserAction:%d,i:%d,m_wCurrentUser:%d, m_cbProvideCard:0x%02X, m_cbSendCardData:0x%02X,strAllActions:%s",
				GetRoomID(), GetTableID(), player->GetUID(), GetPlayerID(0), GetPlayerID(1), m_dwUserAction, i, m_wCurrentUser, m_cbProvideCard, m_cbSendCardData, strAllActions.c_str());
			vector<BYTE>::iterator iter_hu = find(allActions.begin(), allActions.end(), 7);
			if (iter_hu != allActions.end())
			{
				BYTE operCard = m_cbProvideCard;
				if (operCard == 0)
					operCard = m_cbSendCardData;
				OnUserOperCard(i, ACTION_CHI_HU, operCard);
				continue;
			}
			vector<BYTE>::iterator iter_gang = find(allActions.begin(), allActions.end(), 5);
			if (iter_gang != allActions.end())
			{
				BYTE operCard = m_cbProvideCard;
				if (operCard == 0)
					operCard = m_cbSendCardData;
				OnUserOperCard(i, ACTION_GANG, operCard);
				continue;
			}
			LOG_DEBUG("托管自动放弃操作 - uid:%d,i:%d,m_dwUserAction:%d", GetPlayerID(i), i, m_dwUserAction[i]);
			OnUserOperCard(i, ACTION_NULL, 0);
			continue;
		}
	}
	return true;
}

//初始化洗牌
void    CGameMaJiangTable::InitRandCard()
{
    m_poolCards.clear();
    vector<BYTE> tmpVec;
    // 万,索,同
    for(uint8 i=0;i<9;++i)
	{
        for(uint8 j=0;j<4;++j)
		{
            tmpVec.push_back(emPOKER_WAN1 + i);
            tmpVec.push_back(emPOKER_SUO1 + i);
            tmpVec.push_back(emPOKER_TONG1 + i);
        }
    }
    // 国标麻将带字牌
    if(m_mjCfg.playType() == MAJIANG_TYPE_GUOBIAO)
	{
        for(uint8 j=0;j<4;++j)
		{
            tmpVec.push_back(emPOKER_DONG);
            tmpVec.push_back(emPOKER_NAN);
            tmpVec.push_back(emPOKER_XI);
            tmpVec.push_back(emPOKER_BEI);
            tmpVec.push_back(emPOKER_ZHONG);
            tmpVec.push_back(emPOKER_FA);
            tmpVec.push_back(emPOKER_BAI);
        }
    }
    // 长沙宁乡麻将
    m_cbMustLeft = 0;
    if(m_mjCfg.playType() == MAJIANG_TYPE_CHANGSHA || m_mjCfg.playType() == MAJIANG_TYPE_NINGXIANG)
    {
        if(m_mjCfg.supportTail())
		{// 海底牌
            m_cbMustLeft += 1;
        }
    }
	else if(m_mjCfg.playType() == MAJIANG_TYPE_HONGZHONG)
	{
            if(m_mjCfg.hasHongZhongCard())
			{
                for(uint8 i=0;i<4;++i)
				{
                    tmpVec.push_back(emPOKER_ZHONG);
                }
            }
    }
    //二人转转特殊(万牌,字牌)
    if(m_mjCfg.playType() == MAJIANG_TYPE_TWO_PEOPLE && m_conf.seatNum == 2)
	{
        tmpVec.clear();
        for(uint8 i=0;i<9;++i)
		{
            for(uint8 j=0;j<4;++j)
			{
                tmpVec.push_back(emPOKER_WAN1 + i);
            }
        }
        for(uint8 j=0;j<4;++j)
		{
            tmpVec.push_back(emPOKER_DONG);
            tmpVec.push_back(emPOKER_NAN);
            tmpVec.push_back(emPOKER_XI);
            tmpVec.push_back(emPOKER_BEI);
            tmpVec.push_back(emPOKER_ZHONG);
            tmpVec.push_back(emPOKER_FA);
            tmpVec.push_back(emPOKER_BAI);
        }
    }

    //设置宝牌(目前只有转转麻将红中宝)
    if(m_mjCfg.hasHongZhongCard() && m_mjCfg.supportSuperCard())
	{
        LOG_DEBUG("设置红中宝牌");
        m_gameLogic.SetKingCardIndex(m_gameLogic.SwitchToCardIndex(emPOKER_ZHONG));
    }

	//for (uint8 i = 0; i<tmpVec.size(); ++i)
	//{
	//	LOG_DEBUG("roomid:%d,tableid:%d,size:%d,i:%02d,card:0x%02x,index:%02d", 
	//		GetRoomID(), GetTableID(), tmpVec.size(), i, tmpVec[i], m_gameLogic.SwitchToCardIndex(tmpVec[i]));
	//}

    // 洗牌
    for(uint8 i=0;i<tmpVec.size();++i)
	{
		unsigned int uindex = g_RandGen.RandUInt() % tmpVec.size();
		if (uindex != i && uindex < tmpVec.size())
		{
			std::swap(tmpVec[i], tmpVec[uindex]);
		}
    }
    for(uint8 i=0;i<tmpVec.size();++i)
	{
        m_poolCards.push_back(tmpVec[i]);
    }
    LOG_DEBUG("初始化牌池:m_poolCards:%d,m_cbMustLeft:%d",m_poolCards.size(),m_cbMustLeft);
}
//摸一张牌
BYTE    CGameMaJiangTable::PopCardFromPool()
{
    BYTE card = 0;
    if(m_poolCards.size() > 0)
	{
        card = m_poolCards.back();
        m_poolCards.pop_back();
    }
    return card;
}
//处理起手胡
bool    CGameMaJiangTable::ProcessOpeningHu()
{
    // 检查起手胡
    if(!m_mjCfg.supportOpeningHu())
        return false;

    for(uint16 i=0;i<m_wPlayerCount;++i)
    {
        m_gameLogic.CheckMinHu(m_cbCardIndex[i],m_openingHu[i]);
        if(!m_openingHu[i].isNull())
        {
            m_dwUserAction[i] |= ACTION_QIPAI_HU;
            LOG_DEBUG("玩家起手胡:%d--%s",i,m_gameLogic.PrintOpenHuType(m_openingHu[i]).c_str());
        }else{
            m_dwUserAction[i] = ACTION_NULL;
        }
    }
    for(uint16 i=0;i<m_wPlayerCount;++i)
    {
        if(m_dwUserAction[i] != ACTION_NULL){
            return true;
        }
    }
    return false;
}
//检查起手动作
void    CGameMaJiangTable::CheckStartAction(bool bInit)
{
    //LOG_DEBUG("------>>>>>>>>>>>检查庄家起手操作");
    bool bAroseAction = false;
    m_cbProvideCard=m_cbSendCardData;
    m_wProvideUser=m_wBankerUser;
    m_wCurrentUser=m_wBankerUser;

    ResetUserOperInfo();

    for (WORD i = 0; i < m_wPlayerCount; i++)
    {
        //庄家判断
        if(i == m_wBankerUser){
            //杠牌判断
            stGangCardResult GangCardResult;
            m_dwUserAction[i] |= m_gameLogic.AnalyseGangCard(m_cbCardIndex[i], m_WeaveItemArray[i], m_cbWeaveItemCount[i], GangCardResult);
			if ((m_dwUserAction[i] & ACTION_GANG) && GangCardResult.cbCardCount>0)
			{
				m_cbProvideCard = GangCardResult.cbCardData[0];
			}

            BYTE cbTempCardIndex[MAX_INDEX];
            memcpy(cbTempCardIndex, m_cbCardIndex[i], sizeof(cbTempCardIndex));
            if(m_gameLogic.RemoveCard(cbTempCardIndex, m_cbSendCardData) == false) {
                ASSERT(FALSE);
            }
            //胡牌判断
            stChiHuResult ChiHuResult;
            m_dwUserAction[i] |= m_gameLogic.AnalyseChiHuCard(cbTempCardIndex, m_WeaveItemArray[i], 0, m_cbSendCardData, 0, ChiHuResult, true);

        }else{// 普通玩家检查特殊胡
            //胡牌判断
            /*stChiHuResult ChiHuResult;
            m_dwUserAction[i] |= m_gameLogic.AnalyseChiHuCard(m_cbCardIndex[i], m_WeaveItemArray[i], 0, m_cbSendCardData, 0,
                                                              ChiHuResult, false);
            */
        }

        //状态设置
        if((bAroseAction == false) && (i != m_wBankerUser) && (m_dwUserAction[i] != ACTION_NULL)) {
            bAroseAction   = true;
            m_wResumeUser  = m_wCurrentUser;
            m_wCurrentUser = INVALID_CHAIR;
            m_bSendStatus  = false;// 已经摸牌了
        }
    }
    if(bInit==false){
        SendOperNotify();
    }
}

bool CGameMaJiangTable::CheckUserTingCard(uint32 chairID)
{
	if (chairID > m_vecPlayers.size())
	{
		return false;
	}
	//听牌操作
	if (m_cbListenStatus[chairID] == FALSE && m_mjCfg.supportTing())
	{
		//for (DWORD iCardIndex = 0; iCardIndex < MAX_INDEX; iCardIndex++)
		//{
		//	LOG_DEBUG("m_cbCardIndex -uid:%d,iCardIndex:%d,m_cbCardIndex:%d,", GetPlayerID(chairID), iCardIndex, m_cbCardIndex[chairID][iCardIndex]);
		//}

		vector<BYTE> tingCards;
		DWORD tmp = m_gameLogic.AnalyseTingCard14(m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], tingCards );
		if (tmp & ACTION_LISTEN)
		{
			m_dwUserAction[chairID] |= ACTION_LISTEN;// 增加听操作
		}
		DWORD dwTing = (tmp & ACTION_LISTEN);

		//LOG_DEBUG("uid:%d,tmp:%d,dwTing:%d,", GetPlayerID(chairID), tmp, dwTing);
		//for (DWORD iCardIndex = 0; iCardIndex < tingCards.size(); iCardIndex++)
		//{
		//	LOG_DEBUG("uid:%d,iCardIndex:%d,size:%d,card:0x%02X,tmp:%d,", GetPlayerID(chairID), iCardIndex, tingCards.size(), tingCards[iCardIndex], tmp);
		//}
	}
	return true;
}

//计算番数
int32   CGameMaJiangTable::CalcBaseFanCount(uint16 chairID,stChiHuResult huResult)
{
    bool   isBanker = (chairID==m_wBankerUser) ? true : false;  // 是否庄家
    bool   isZiMo   = huResult.dwChiHuRight&CHR_ZI_MO ? true : false;
    int32  fan = 0;
    if(!huResult.IsHu()){//没有胡牌
        LOG_DEBUG("没有胡牌:%d",chairID);
        return 0;
    }

    if(m_mjCfg.playType() == MAJIANG_TYPE_TWO_PEOPLE)// 转转麻将
    {
        fan += isZiMo ? 2 : 1; //自摸2分,点胡1分
        LOG_DEBUG("转转自摸:%d--%d",isZiMo,fan);
        if(m_mjCfg.useMasterPoint() && isBanker){
            fan += 1;//庄闲分
            LOG_DEBUG("转转麻将庄闲算分%d",fan);
        }
    }else if(m_mjCfg.playType() == MAJIANG_TYPE_CHANGSHA) {// 长沙麻将
        //大胡
        for(uint32 i=HUTYPE_JI_HU;i<HUTYPE_MAX_TYPE;i++)
        {
            if(huResult.IsKind(i))
            {
                if(i==HUTYPE_JI_HU){
                    fan += isZiMo ? 2 : 1;
                    LOG_DEBUG("基胡或自摸:%d--%d", isZiMo, fan);
                }else if(i==HUTYPE_SUPER_QI_DUI){
                    fan += 12;
                    LOG_DEBUG("豪华七小对:%d",fan);
                }else if(i==HUTYPE_SUPER2_QI_DUI){
                    fan += 18;
                    LOG_DEBUG("双豪华七小对:%d",fan);
                }else if(i==HUTYPE_MEN_QING){
                    fan += 4;
                    LOG_DEBUG("门清:%d",fan);
                }else{
                    fan += 6;
                    LOG_DEBUG("大胡:%d--%d",i,fan);
                }
            }
        }
        //权位
        for(uint32 i=CHR_QIANG_GANG;i<=CHR_QIANG_HAIDI;i=i<<1){
            if(huResult.IsRight(i)){
                fan += 6;
                LOG_DEBUG("权位分:%d---%s",fan,m_gameLogic.PrintHuRightType(huResult.dwChiHuRight).c_str());
            }
        }
        if(m_mjCfg.useMasterPoint() && isBanker){
            fan += 1;
            LOG_DEBUG("长沙庄家加一分:%d",fan);
        }
    }else if(m_mjCfg.playType() == MAJIANG_TYPE_NINGXIANG){// 宁乡麻将
        //大胡
        for(uint32 i=HUTYPE_JI_HU;i<HUTYPE_MAX_TYPE;i++)
        {
            if(huResult.IsKind(i))
            {
                if(i==HUTYPE_JI_HU){
                    fan += isZiMo ? 4 : 4;
                    LOG_DEBUG("基胡或自摸:%d--%d", isZiMo, fan);
                }else if(i==HUTYPE_SUPER_QI_DUI){
                    fan += 16;
                    LOG_DEBUG("豪华七小对:%d",fan);
                }else if(i==HUTYPE_SUPER2_QI_DUI){
                    fan += 24;
                    LOG_DEBUG("双豪华七小对:%d",fan);
                }else if(i==HUTYPE_SUPER3_QI_DUI){
                    fan += 32;
                    LOG_DEBUG("三豪华七小对:%d",fan);
                }else{
                    fan += 8;
                    LOG_DEBUG("大胡:%d--%d",i,fan);
                }
            }
        }
        //权位
        for(uint32 i=CHR_QIANG_GANG;i<=CHR_QIANG_HAIDI;i=i<<1){
            if(huResult.IsRight(i)){
                fan += 8;
                LOG_DEBUG("权位分:%d---%s",fan,m_gameLogic.PrintHuRightType(huResult.dwChiHuRight).c_str());
            }
        }
        if(m_mjCfg.useMasterPoint() && isBanker){
            fan += 1;
            LOG_DEBUG("宁乡庄家加一分:%d",fan);
        }

    }else if(m_mjCfg.playType() == MAJIANG_TYPE_HONGZHONG){// 红中麻将
        fan += isZiMo ? 2 : 1; //自摸2分,点胡1分
        LOG_DEBUG("红中麻将自摸:%d--%d",isZiMo,fan);
        if(m_mjCfg.useMasterPoint() && isBanker){
            fan += 1;//庄闲分
            LOG_DEBUG("红中麻将庄闲算分%d",fan);
        }
    }





    LOG_DEBUG("算番结果:chairID:%d--%d",chairID,fan);
    return fan;
}
//计算扎鸟
bool    CGameMaJiangTable::CalcHitBird(uint32 provideID,uint32 winID,bool isMulPao)
{
    LOG_DEBUG("计算扎鸟:%d--%d--%d",provideID,winID,isMulPao);

    ZeroMemory(m_birdCards,sizeof(m_birdCards));
    uint16 birdCount = m_mjCfg.GetBirdCount();// 扎鸟数
    if(m_mjCfg.playType() == MAJIANG_TYPE_HONGZHONG)
    {
        if(m_mjCfg.IsOneBirdAll())
        {   //一码全中
            birdCount = 1;
        }
        //无红中加码
        uint8 addBird = m_mjCfg.NoHongZhongAddBird();
        if(GetKingCardCount(winID) == 0){
            birdCount += addBird;
            LOG_DEBUG("无红中加码:%d--%d",addBird,birdCount);
        }
        for(uint16 i = 0; i < birdCount && m_poolCards.size() > 0; ++i)
        {
            BYTE birdCard = PopCardFromPool();
            m_allBirdCards.push_back(birdCard);

            BYTE cardValue = GetBirdValue(birdCard);
            BYTE hitChairID = 0;
            if(m_mjCfg.IsOneBirdAll())
            {
                vector<uint16> wins;
                GetAllHuUser(wins);
                for(uint8 j=0;j<wins.size();++j) {
                    m_birdCards[wins[j]][m_hitBirdCount[wins[j]]++] = birdCard;
                    m_addHitBirdCount[wins[j]] += (cardValue - 1);
                }
            }else {
                if (!isMulPao) {
                    hitChairID = (cardValue + winID + m_wPlayerCount - 1) % m_wPlayerCount;
                } else {
                    hitChairID = (cardValue + provideID + m_wPlayerCount - 1) % m_wPlayerCount;
                }
                if (!m_ChiHuResult[hitChairID].IsHu())
                    continue;
                m_birdCards[hitChairID][m_hitBirdCount[hitChairID]++] = birdCard;
            }
        }
    }else {
        for (uint16 i = 0; i < birdCount && m_poolCards.size() > 0; ++i) {
            BYTE birdCard = PopCardFromPool();
            BYTE cardValue = GetBirdValue(birdCard);
            m_allBirdCards.push_back(birdCard);

            if (!isMulPao) {
                BYTE hitChairID = (cardValue + winID + m_wPlayerCount - 1) % m_wPlayerCount;
                m_birdCards[hitChairID][m_hitBirdCount[hitChairID]++] = birdCard;
            } else {
                BYTE hitChairID = (cardValue + provideID + m_wPlayerCount - 1) % m_wPlayerCount;
                m_birdCards[hitChairID][m_hitBirdCount[hitChairID]++] = birdCard;
            }
        }
    }
    return false;
}
//计算杠的分数
bool    CGameMaJiangTable::CalcGangScore()
{
    if(m_mjCfg.supportImmediaGangPoint() == false && m_wProvideUser == INVALID_CHAIR) {
        //LOG_DEBUG("流局不算杠分");
        return false;
    }
    //不支持杠分
    if(!m_mjCfg.supportGangPoint())
        return false;

    for(BYTE i=0;i<m_wPlayerCount;++i)
    {
        for(BYTE j=0;j<m_cbWeaveItemCount[i];++j)
        {
            stWeaveItem& refItem = m_WeaveItemArray[i][j];
            if(refItem.WeaveKind == ACTION_GANG)
			{//杠
                if(refItem.wProvideUser == i)
                {
                    int32 score = (refItem.cbPublicCard == FALSE) ? 2 : 1;//暗杠每人输2分,补杠每人输1分
                    for(BYTE k=0;k<m_wPlayerCount;++k)
                    {
                        if(k!=i)
                        {
                            SetWinScore(i,k,score);
                        }
                    }
                }else{//点杠放杠者输3分
                    SetWinScore(i,refItem.wProvideUser,3);
                }
            }
        }
    }
    //LOG_DEBUG("计算杠分");
    return true;
}
//计算胡牌分数
bool    CGameMaJiangTable::CalcHuScore()
{
    if(m_mjCfg.playType() == MAJIANG_TYPE_GUOBIAO || m_mjCfg.playType() == MAJIANG_TYPE_TWO_PEOPLE){
        return CalcGuoBiaoHuScore();
    }

    // 有大胡的不算基胡
    for(uint16 i=0;i<m_wPlayerCount;++i)
    {
        if(!m_ChiHuResult[i].IsHu())
            continue;

        LOG_DEBUG("%d 算番前牌型:%s--%s",i,m_gameLogic.PrintHuKindType(m_ChiHuResult[i].HuKind).c_str(),m_gameLogic.PrintHuRightType(m_ChiHuResult[i].dwChiHuRight).c_str());

        // 屏蔽不算分不显示的类型
        m_mjCfg.ShowHuType(m_ChiHuResult[i].HuKind);
        m_ChiHuResult[i].dwChiHuRight = m_ChiHuResult[i].dwChiHuRight&m_mjCfg.ShowHuRight();

        if(!m_ChiHuResult[i].HuKind.isOnlyType(HUTYPE_JI_HU)
        || m_ChiHuResult[i].dwChiHuRight > CHR_ZI_MO){
            m_ChiHuResult[i].HuKind.del(HUTYPE_JI_HU);
            LOG_DEBUG("有大胡不记基本胡");
        }

        LOG_DEBUG("%d 算番牌型:%s--%s",i,m_gameLogic.PrintHuKindType(m_ChiHuResult[i].HuKind).c_str(),m_gameLogic.PrintHuRightType(m_ChiHuResult[i].dwChiHuRight).c_str());

    }

    //基本分数
    uint16 winID = 0;
    BYTE winNum = 0;
    for(BYTE i=0;i<m_wPlayerCount;++i)
    {
        int32 fan = CalcBaseFanCount(i,m_ChiHuResult[i]);
        if(fan==0)continue;
        winID = i;
        winNum++;
        if(m_ChiHuResult[i].IsRight(CHR_ZI_MO))
        {//自摸输3家
            for(BYTE j=0;j<m_wPlayerCount;++j){
                if(j!=i){
                    SetWinScore(i,j,fan);
                }
            }
        }else{
            SetWinScore(i,m_fangPaoUser,fan);
        }
        //庄闲算分
        if(m_mjCfg.useMasterPoint() && i != m_wBankerUser && (m_wProvideUser==m_wBankerUser || m_ChiHuResult[i].IsRight(CHR_ZI_MO)))
        {
            LOG_DEBUG("庄闲算分:%d--%d",i,m_wBankerUser);
            SetWinScore(i,m_wBankerUser,1);
        }
    }
    //起手胡
    if(m_mjCfg.supportOpeningHu())
    {
        for(uint16 i=0;i<m_wPlayerCount;++i){
            vector<BYTE> actions;
            m_openingHu[i].getAllActions(actions);
            if(actions.size()>0){
                for(uint8 j=0;j<m_wPlayerCount;++j){
                    if(j!=i){
                        LOG_DEBUG("起手胡算分:%d--%d--%d",i,j,actions.size()*m_mjCfg.openingHuFen());
                        int64 score = actions.size()*m_mjCfg.openingHuFen();
                        SetWinScore(i,j,score);
                    }
                }
            }
        }
    }
    //扎鸟分数
    CalcHitBird(m_wProvideUser,winID,(winNum>1)?true:false);

    for(BYTE i = 0; i < m_wPlayerCount; ++i){
        if (m_ChiHuResult[i].IsHu()) {//胡牌
            if (m_ChiHuResult[i].IsRight(CHR_ZI_MO)) {//自摸每家多输鸟数分
                for (BYTE j = 0; j < m_wPlayerCount; ++j) {
                    if (j != i) {
                        uint32 birdCount = (m_hitBirdCount[i] + m_addHitBirdCount[i]) +
                                           (m_hitBirdCount[j] + m_addHitBirdCount[j]);
                        if(m_mjCfg.hitBirdCalcType() == 0){
                            SetWinScore(i, j, birdCount * m_mjCfg.hitBirdFen());
                        }else if(m_mjCfg.hitBirdCalcType() == 1){
                            m_lEachScore[i][j] *= (birdCount+1);
                        }else{//中鸟翻倍
                            m_lEachScore[i][j] *= pow(2,birdCount);
                        }
                    }
                }
            } else {
                uint32 birdCount = (m_hitBirdCount[i] + m_addHitBirdCount[i]) +
                                   (m_hitBirdCount[m_fangPaoUser] + m_addHitBirdCount[m_fangPaoUser]);
                if(m_mjCfg.hitBirdCalcType() == 0){
                    SetWinScore(i, m_fangPaoUser, birdCount * m_mjCfg.hitBirdFen());
                }else if(m_mjCfg.hitBirdCalcType() == 1){
                    m_lEachScore[i][m_fangPaoUser] *= (birdCount+1);
                }else{
                    m_lEachScore[i][m_fangPaoUser] *= pow(2,birdCount);
                }
            }
        }
    }
    //统计分数
    for(uint16 i=0;i<m_wPlayerCount;++i)
    {
        for(uint16 j=0;j<m_wPlayerCount;++j)
        {
            m_lGameScore[i] += m_lEachScore[i][j] - (m_lEachScore[j][i]);
        }
    }
    for(uint16 i=0;i<m_wPlayerCount;++i) {
        m_lGameScore[i] *= GetBaseScore();
    }
    return true;
}
bool    CGameMaJiangTable::CalcGuoBiaoHuScore()
{
	LOG_DEBUG("playerwinscore1 uid:%d %d,m_fangPaoUser:%d,m_lEachScore:%lld %lld, %lld %lld,m_lGameScore:%lld %lld", GetPlayerID(0), GetPlayerID(1), m_fangPaoUser, m_lEachScore[0][0], m_lEachScore[0][1], m_lEachScore[1][0], m_lEachScore[1][1], m_lGameScore[0], m_lGameScore[1]);

    //基本分数
    for(BYTE i=0;i<m_wPlayerCount;++i)
    {
        m_gameLogic.RemoveGuoBiaoFan(m_ChiHuResult[i]);
		int32 fan = m_gameLogic.CalcGuoBiaoFan(m_ChiHuResult[i]);
		bool bIsZiMo = m_ChiHuResult[i].IsRight(CHR_ZI_MO);
		bool bIsQaingGangHu = m_ChiHuResult[i].HuKind.isExist(HUTYPE_QIANG_GANG_HU);
		bool bIsHuJueZhang = m_ChiHuResult[i].HuKind.isExist(HUTYPE_HU_JUEZHANG);
		LOG_DEBUG("playerwinscore2 - uid:%d %d,%d,i:%d,m_fangPaoUser:%d,bIsZiMo:%d,bIsQaingGangHu:%d,bIsHuJueZhang:%d,m_TempWeaveItem.wOperateUser:%d,fan:%d,lEachScore:%lld,",
			GetPlayerID(0), GetPlayerID(1), GetPlayerID(i), i, m_fangPaoUser, bIsZiMo, bIsQaingGangHu, bIsHuJueZhang, m_TempWeaveItem.wOperateUser, fan, m_lEachScore[i][0]);

		if (fan == 0)
		{
			continue;
		}
        if(bIsZiMo==true)
        {
			//LOG_DEBUG("playerwinscore3 - uid:%d,i:%d,fan:%d,lEachScore:%lld,", GetPlayerID(i), i, fan, m_lEachScore[i][0]);

			//自摸输3家
            for(BYTE j=0;j<m_wPlayerCount;++j)
			{
				if (j != i)
				{
					SetWinScore(i, j, fan);
					//LOG_DEBUG("playerwinscore2 - uid:%d,i:%d,fan:%d,lEachScore:%lld,",GetPlayerID(i), i,fan,m_lEachScore[i][j]);
				}
            }
        }
		else if(bIsZiMo==false && m_fangPaoUser<m_wPlayerCount)
		{
            SetWinScore(i,m_fangPaoUser,fan);
        }
		else
		{
			for (BYTE j = 0; j<m_wPlayerCount; ++j)
			{
				if (j != i)
				{
					SetWinScore(i, j, fan);
				}
			}
		}
		//else if (bIsQaingGangHu == true && m_TempWeaveItem.wOperateUser < m_wPlayerCount)
		//{
		//	//SetWinScore(i, m_TempWeaveItem.wOperateUser, fan);
		//	for (BYTE j = 0; j<m_wPlayerCount; ++j)
		//	{
		//		if (j != i)
		//		{
		//			SetWinScore(i, j, fan);
		//		}
		//	}
		//}
		//else if (bIsHuJueZhang == true && m_TempWeaveItem.wOperateUser < m_wPlayerCount)
		//{
		//	//SetWinScore(i, m_TempWeaveItem.wOperateUser, fan);
		//}
    }

    //统计分数
    for(uint16 i=0;i<m_wPlayerCount;++i)
    {
        for(uint16 j=0;j<m_wPlayerCount;++j)
        {
			m_lGameScore[i] += m_lEachScore[i][j];
			m_lGameScore[j] -= m_lEachScore[i][j];
        }
    }

	LOG_DEBUG("a_ m_wPlayerCount:%d,m_iUserPassHuCount:%d,uid:%d %d,m_lEachScore:%lld %lld, %lld %lld,m_lGameScore:%lld %lld", m_wPlayerCount, m_iUserPassHuCount[0], m_iUserPassHuCount[1], GetPlayerID(0), GetPlayerID(1), m_lEachScore[0][0], m_lEachScore[0][1], m_lEachScore[1][0], m_lEachScore[1][1], m_lGameScore[0], m_lGameScore[1]);

    for(uint16 i=0;i<m_wPlayerCount;++i)
	{
        m_lGameScore[i] *= GetBaseScore();
    }

	for (uint16 i = 0; i<m_wPlayerCount; ++i)
	{
		if (m_lGameScore[i] > 0)
		{
			for (int j = 0; j < m_iUserPassHuCount[i]; j++)
			{
				for (int k = 0; k < m_wPlayerCount; k++)
				{
					m_lGameScore[k] *= 2;
				}
			}
		}
	}

	LOG_DEBUG("1_ m_wPlayerCount:%d,m_iUserPassHuCount:%d,uid:%d %d,m_lEachScore:%lld %lld, %lld %lld,m_lGameScore:%lld %lld", m_wPlayerCount, m_iUserPassHuCount[0], m_iUserPassHuCount[1], GetPlayerID(0), GetPlayerID(1), m_lEachScore[0][0], m_lEachScore[0][1], m_lEachScore[1][0], m_lEachScore[1][1], m_lGameScore[0], m_lGameScore[1]);

	//玩家最多只能输携带的金币
	int64 lLostAllScore = 0;
	for (uint16 i = 0; i < m_wPlayerCount; ++i)
	{
		if (m_ChiHuResult[i].IsHu())
		{
			continue;
		}
		int64 lCurScore = GetPlayerCurScore(GetPlayer(i));
		lLostAllScore += lCurScore;
		int64 lLostScore = 0;
		if (m_lGameScore[i] < 0)
		{
			lLostScore = -m_lGameScore[i];
		}

		int64 lRealScore = MIN(lCurScore, lLostScore);

		if (m_lGameScore[i] < 0)
		{
			m_lGameScore[i] = -lRealScore;
		}
	}
	// 赢家最多只能赢输家的身上携带
	for (uint16 i = 0; i < m_wPlayerCount; ++i)
	{
		if (m_ChiHuResult[i].IsHu() == false)
		{
			continue;
		}
		int64 lCurScore = GetPlayerCurScore(GetPlayer(i));
		int64 lWinScore = m_lGameScore[i];
		int64 lRealScore = MIN(lCurScore, lWinScore);
		lRealScore = MIN(lRealScore, lLostAllScore);
		m_lGameScore[i] = lRealScore;
	}

	LOG_DEBUG("2_ uid:%d %d,m_lEachScore:%lld %lld, %lld %lld,m_lGameScore:%lld %lld,lLostAllScore:%lld,curscore:%lld %lld",
		GetPlayerID(0), GetPlayerID(1), m_lEachScore[0][0], m_lEachScore[0][1], m_lEachScore[1][0], m_lEachScore[1][1], m_lGameScore[0], m_lGameScore[1], lLostAllScore, GetPlayerCurScore(GetPlayer(0)), GetPlayerCurScore(GetPlayer(1)));
	
	// 赢多少 就只能输多少
	for (uint16 i = 0; i < m_wPlayerCount - 1; ++i)
	{
		int64 lFirstScore = m_lGameScore[i];
		int64 lScendScore = m_lGameScore[i+1];
		if (m_lGameScore[i] < 0)
		{
			lFirstScore = -m_lGameScore[i];
		}
		if (m_lGameScore[i + 1] < 0)
		{
			lScendScore = -m_lGameScore[i + 1];
		}
		int64 lRealScore = MIN(lFirstScore, lScendScore);
		if (m_lGameScore[i] < 0)
		{
			m_lGameScore[i] = -lRealScore;
		}
		if (m_lGameScore[i + 1] < 0)
		{
			m_lGameScore[i + 1] = -lRealScore;
		}
	}
	LOG_DEBUG("3_ uid:%d %d,m_lEachScore:%lld %lld, %lld %lld,m_lGameScore:%lld %lld,lLostAllScore:%lld,curscore:%lld %lld",
		GetPlayerID(0), GetPlayerID(1), m_lEachScore[0][0], m_lEachScore[0][1], m_lEachScore[1][0], m_lEachScore[1][1], m_lGameScore[0], m_lGameScore[1], lLostAllScore, GetPlayerCurScore(GetPlayer(0)), GetPlayerCurScore(GetPlayer(1)));

    return true;
}

//设置下轮庄家
bool    CGameMaJiangTable::SetNextBanker()
{
    if(m_mjCfg.playType() == MAJIANG_TYPE_CHANGSHA || m_mjCfg.playType() == MAJIANG_TYPE_NINGXIANG)
    {
        if(m_wProvideUser == INVALID_CHAIR)
        {
            LOG_DEBUG("流局连庄");
            if(m_tailCard != 0)
            {
                for(uint16 i=0;i<m_wPlayerCount;++i)
                {
                    if(m_tailCardOper[i] == 1){
                        m_wNextBankerUser = i;
                        LOG_DEBUG("要海底牌的人坐庄:%d",i);
                        break;
                    }
                }
            }else{
                m_wNextBankerUser = m_wBankerUser;
                LOG_DEBUG("流局连庄:%d",m_wNextBankerUser);
            }
        }else{
            uint16 winID = 0;
            BYTE winNum = 0;
            for(BYTE i=0;i<m_wPlayerCount;++i){
                if(!m_ChiHuResult[i].IsHu())continue;
                winID = i;
                winNum++;
            }
            if(winNum > 1){
                m_wNextBankerUser = m_fangPaoUser;
            }else{
                m_wNextBankerUser = winID;
            }
        }
    }else{
        if(m_wProvideUser == INVALID_CHAIR){//流局连庄
            m_wNextBankerUser = m_wBankerUser;
            LOG_DEBUG("流局连庄:%d",m_wNextBankerUser);
        }else{
            uint16 winID = 0;
            BYTE winNum = 0;
            for(BYTE i=0;i<m_wPlayerCount;++i){
                if(!m_ChiHuResult[i].IsHu())continue;
                winID = i;
                winNum++;
            }
            if(winNum > 1){
                m_wNextBankerUser = m_wProvideUser;
            }else{
                m_wNextBankerUser = winID;
            }
        }
    }
    LOG_DEBUG("下局庄家:%d",m_wNextBankerUser);
    return true;
}
//获得所有胡牌玩家
void    CGameMaJiangTable::GetAllHuUser(vector<uint16>& huUsers)
{
    huUsers.clear();
    for(BYTE i=0;i<m_wPlayerCount;++i)
    {
        if(!m_ChiHuResult[i].IsHu())
            continue;
        huUsers.push_back(i);
    }
}
//检查手牌是否有赖子
uint8   CGameMaJiangTable::GetKingCardCount(uint16 chairID)
{
     uint8 count = 0;
     for(uint8 i=0;i<MAX_INDEX;++i)
     {
         if(m_cbCardIndex[chairID][i] > 0 && m_gameLogic.IsKingCardIndex(i)){
             count += m_cbCardIndex[chairID][i];
         }
     }
     return count;
}
//抓鸟点数
uint8   CGameMaJiangTable::GetBirdValue(uint8 card)
{
    if(card == emPOKER_ZHONG)return 1;

    return m_gameLogic.GetCardValue(card);
}
//设置赢分
void    CGameMaJiangTable::SetWinScore(uint16 winID,uint16 loseID,int64 score)
{
    m_lEachScore[winID][loseID] += score;
}

// 写入出牌log
void    CGameMaJiangTable::WriteOutCardLog(uint16 chairID,uint32 opercode, uint8 cardData[], uint8 cardCount)
{
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["oc"] = opercode;
	logValue["i"] = m_cbPlayerOperIndex;// m_cbArrOperIndex[chairID];
	for (uint32 i = 0; i<cardCount; ++i)
	{
		logValue["c"].append(cardData[i]);
	}
	if (opercode == OPERATE_CODE_NULL)
	{
		vector<BYTE> acts;
		m_gameLogic.GetAllAction(m_dwUserAction[chairID], acts);
		for (uint32 i = 0; i < acts.size(); ++i)
		{
			logValue["a"].append(acts[i]);
		}
	}
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		logValue["pu"].append(m_iUserPassHuCount[i]);
	}
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		logValue["pf"].append(m_iUserPassHuFlag[i]);
	}
	
	m_operLog["out"].append(logValue);
	//m_cbArrOperIndex[chairID]++;
	m_cbPlayerOperIndex++;
}

void    CGameMaJiangTable::WriteGameEndLog(uint16 chairID, uint32 opercode, vector<BYTE> actions)
{
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["oc"] = opercode;
	for (uint32 i = 0; i<actions.size(); ++i) {
		logValue["k"].append(actions[i]);
	}
	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		logValue["pu"].append(m_iUserPassHuCount[i]);
	}
	m_operLog["end"].append(logValue);
}

void    CGameMaJiangTable::WriteGameEndLog(uint16 chairID, uint16 fangpao, uint8 cardData, int64 score)
{
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["f"] = fangpao;
	logValue["c"] = cardData;
	logValue["s"] = score;
	m_operLog["end"].append(logValue);
}

void    CGameMaJiangTable::WriteUserHandCard(uint32 code)
{
	string pos = "error";
	if (code == HAND_CARD_PSO_GAME_START)
	{
		pos.clear();
		pos = "gsta";
	}
	else if (code == HAND_CARD_PSO_GAME_END)
	{
		pos.clear();
		pos = "gend";
	}


	for (uint8 i = 0; i < m_wPlayerCount; i++)
	{
		Json::Value logValueHandCard;
		logValueHandCard["p"] = i;
		
		BYTE cardData[MAX_COUNT] = { 0 };
		BYTE cardNum = m_gameLogic.SwitchToCardData(m_cbCardIndex[i], cardData, MAX_COUNT);
		for (uint32 j = 0; j<cardNum; ++j)
		{
			logValueHandCard["c"].append(cardData[j]);
		}

		for (uint32 j = 0; j<m_cbWeaveItemCount[i]; ++j)
		{
			Json::Value logValueWeaveItem;
			logValueWeaveItem["pu"] = m_WeaveItemArray[i][j].wProvideUser;
			logValueWeaveItem["cc"] = m_WeaveItemArray[i][j].cbCenterCard;
			logValueWeaveItem["pc"] = m_WeaveItemArray[i][j].cbPublicCard;
			BYTE cbWeaveKind = m_WeaveItemArray[i][j].WeaveKind;
			logValueWeaveItem["wk"] = cbWeaveKind;

			logValueHandCard["weave"].append(logValueWeaveItem);
		}


		m_operLog[pos.c_str()].append(logValueHandCard);
	}
}

int CGameMaJiangTable::GetProCardType()
{
	string strDispatchCardPro;
	int iSumValue = 0;
	int iArrDispatchCardPro[Pro_Index_MAX] = { 0 };
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_iArrDispatchCardPro[i];
		iSumValue += m_iArrDispatchCardPro[i];
		strDispatchCardPro += CStringUtility::FormatToString("%d ", m_iArrDispatchCardPro[i]);
	}
	if (iSumValue <= 0)
	{
		return Pro_Index_RandSingle;
	}
	int iRandNum = g_RandGen.RandRange(0, iSumValue);
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
		iProIndex = Pro_Index_RandSingle;
	}
	LOG_DEBUG("roomid:%d,tableid:%d,iSumValue:%d,iRandNum:%d,iProIndex:%d,strDispatchCardPro:%s", GetRoomID(), GetTableID(), iSumValue, iRandNum, iProIndex, strDispatchCardPro.c_str());
	return iProIndex;
}


bool CGameMaJiangTable::SetControlCardData()
{
	bool bIsHaveRobot = false;
	uint16 uRobotChairID = 255;

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		CGamePlayer * pPlayer = GetPlayer(i);
		if (pPlayer != NULL && pPlayer->IsRobot())
		{
			bIsHaveRobot = true;
			uRobotChairID = i;
		}
	}

	int	iProIndex = GetProCardType();
		
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d - %d,bIsHaveRobot:%d,uRobotChairID:%d,m_wPlayerCount:%d,iProIndex:%d",
		GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), bIsHaveRobot, uRobotChairID, m_wPlayerCount, iProIndex);

	if (bIsHaveRobot == false || uRobotChairID >= m_wPlayerCount || iProIndex == Pro_Index_RandSingle)
	{
		return false;
	}
	bool bFlag = false;
	if (iProIndex == Pro_Index_FiveDuiZi)
	{
		bFlag = GetHandCard_FiveDuiZi(uRobotChairID);
	}
	if (iProIndex == Pro_Index_FourDuiZi)
	{
		bFlag = GetHandCard_FourDuiZi(uRobotChairID);
	}
	if (iProIndex == Pro_Index_ThreeKeZi)
	{
		bFlag = GetHandCard_ThreeKeZi(uRobotChairID);
	}
	if (iProIndex == Pro_Index_TwoKeZi_OneDuiZi)
	{
		bFlag = GetHandCard_TwoKeZi_OneDuiZi(uRobotChairID);
	}
	if (iProIndex == Pro_Index_TwoKeZi)
	{
		bFlag = GetHandCard_TwoKeZi(uRobotChairID);
	}
	if (iProIndex == Pro_Index_TwelveTongHua)
	{
		bFlag = GetHandCard_TwelveTongHua(uRobotChairID);
	}
	if (iProIndex == Pro_Index_ElevenTongHua)
	{
		bFlag = GetHandCard_ElevenTongHua(uRobotChairID);
	}
	if (iProIndex == Pro_Index_TenTongHua)
	{
		bFlag = GetHandCard_TenTongHua(uRobotChairID);
	}
	return bFlag;
}

//for (uint8 i = 0; i<9; ++i)
//{
//	for (uint8 j = 0; j<4; ++j)
//	{
//		tmpVec.push_back(emPOKER_WAN1 + i);
//	}
//}
//for (uint8 j = 0; j<4; ++j)
//{
//	tmpVec.push_back(emPOKER_DONG);
//	tmpVec.push_back(emPOKER_NAN);
//	tmpVec.push_back(emPOKER_XI);
//	tmpVec.push_back(emPOKER_BEI);
//	tmpVec.push_back(emPOKER_ZHONG);
//	tmpVec.push_back(emPOKER_FA);
//	tmpVec.push_back(emPOKER_BAI);
//}

/*
[roomid:2,tableid:1,size:64,i:00,card:0x01,index:00
[roomid:2,tableid:1,size:64,i:01,card:0x01,index:00
[roomid:2,tableid:1,size:64,i:02,card:0x01,index:00
[roomid:2,tableid:1,size:64,i:03,card:0x01,index:00
[roomid:2,tableid:1,size:64,i:04,card:0x02,index:01
[roomid:2,tableid:1,size:64,i:05,card:0x02,index:01
[roomid:2,tableid:1,size:64,i:06,card:0x02,index:01
[roomid:2,tableid:1,size:64,i:07,card:0x02,index:01
[roomid:2,tableid:1,size:64,i:08,card:0x03,index:02
[roomid:2,tableid:1,size:64,i:09,card:0x03,index:02
[roomid:2,tableid:1,size:64,i:10,card:0x03,index:02
[roomid:2,tableid:1,size:64,i:11,card:0x03,index:02
[roomid:2,tableid:1,size:64,i:12,card:0x04,index:03
[roomid:2,tableid:1,size:64,i:13,card:0x04,index:03
[roomid:2,tableid:1,size:64,i:14,card:0x04,index:03
[roomid:2,tableid:1,size:64,i:15,card:0x04,index:03
[roomid:2,tableid:1,size:64,i:16,card:0x05,index:04
[roomid:2,tableid:1,size:64,i:17,card:0x05,index:04
[roomid:2,tableid:1,size:64,i:18,card:0x05,index:04
[roomid:2,tableid:1,size:64,i:19,card:0x05,index:04
[roomid:2,tableid:1,size:64,i:20,card:0x06,index:05
[roomid:2,tableid:1,size:64,i:21,card:0x06,index:05
[roomid:2,tableid:1,size:64,i:22,card:0x06,index:05
[roomid:2,tableid:1,size:64,i:23,card:0x06,index:05
[roomid:2,tableid:1,size:64,i:24,card:0x07,index:06
[roomid:2,tableid:1,size:64,i:25,card:0x07,index:06
[roomid:2,tableid:1,size:64,i:26,card:0x07,index:06
[roomid:2,tableid:1,size:64,i:27,card:0x07,index:06
[roomid:2,tableid:1,size:64,i:28,card:0x08,index:07
[roomid:2,tableid:1,size:64,i:29,card:0x08,index:07
[roomid:2,tableid:1,size:64,i:30,card:0x08,index:07
[roomid:2,tableid:1,size:64,i:31,card:0x08,index:07
[roomid:2,tableid:1,size:64,i:32,card:0x09,index:08
[roomid:2,tableid:1,size:64,i:33,card:0x09,index:08
[roomid:2,tableid:1,size:64,i:34,card:0x09,index:08
[roomid:2,tableid:1,size:64,i:35,card:0x09,index:08
[roomid:2,tableid:1,size:64,i:36,card:0x31,index:27
[roomid:2,tableid:1,size:64,i:37,card:0x32,index:28
[roomid:2,tableid:1,size:64,i:38,card:0x33,index:29
[roomid:2,tableid:1,size:64,i:39,card:0x34,index:30
[roomid:2,tableid:1,size:64,i:40,card:0x35,index:31
[roomid:2,tableid:1,size:64,i:41,card:0x36,index:32
[roomid:2,tableid:1,size:64,i:42,card:0x37,index:33
[roomid:2,tableid:1,size:64,i:43,card:0x31,index:27
[roomid:2,tableid:1,size:64,i:44,card:0x32,index:28
[roomid:2,tableid:1,size:64,i:45,card:0x33,index:29
[roomid:2,tableid:1,size:64,i:46,card:0x34,index:30
[roomid:2,tableid:1,size:64,i:47,card:0x35,index:31
[roomid:2,tableid:1,size:64,i:48,card:0x36,index:32
[roomid:2,tableid:1,size:64,i:49,card:0x37,index:33
[roomid:2,tableid:1,size:64,i:50,card:0x31,index:27
[roomid:2,tableid:1,size:64,i:51,card:0x32,index:28
[roomid:2,tableid:1,size:64,i:52,card:0x33,index:29
[roomid:2,tableid:1,size:64,i:53,card:0x34,index:30
[roomid:2,tableid:1,size:64,i:54,card:0x35,index:31
[roomid:2,tableid:1,size:64,i:55,card:0x36,index:32
[roomid:2,tableid:1,size:64,i:56,card:0x37,index:33
[roomid:2,tableid:1,size:64,i:57,card:0x31,index:27
[roomid:2,tableid:1,size:64,i:58,card:0x32,index:28
[roomid:2,tableid:1,size:64,i:59,card:0x33,index:29
[roomid:2,tableid:1,size:64,i:60,card:0x34,index:30
[roomid:2,tableid:1,size:64,i:61,card:0x35,index:31
[roomid:2,tableid:1,size:64,i:62,card:0x36,index:32
[roomid:2,tableid:1,size:64,i:63,card:0x37,index:33
[初始化牌池:m_poolCards:64,m_cbMustLeft:0]

BYTE    CGameMaJiangTable::PopCardFromPool()
{
BYTE card = 0;
if(m_poolCards.size() > 0)
{
card = m_poolCards.back();
m_poolCards.pop_back();
}
return card;
}

*/

//for (WORD i = 0; i < m_wPlayerCount; i++)
//{
//	for (uint8 j = 0; j < (MAX_COUNT - 1); ++j)
//	{
//		BYTE card = PopCardFromPool();
//		m_cbCardIndex[i][m_gameLogic.SwitchToCardIndex(card)]++;
//	}
//}

bool	CGameMaJiangTable::GetHandCard_FiveDuiZi(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		vecCardIndex.push_back(index);
	}
	uint32 iTargetCount = 5;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());

	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		for (uint32 j = 0; j < 2; j++)
		{
			BYTE card = vecTargetCard[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}
	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndex.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		BYTE card = cbArrCardData[vecCardIndex[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndex.erase(vecCardIndex.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,vecHandCard.size():%d,strHandCardData:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), vecHandCard.size(), strHandCardData.c_str());

	return true;
}
bool	CGameMaJiangTable::GetHandCard_FourDuiZi(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		vecCardIndex.push_back(index);
	}
	uint32 iTargetCount = 4;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());

	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		for (uint32 j = 0; j < 2; j++)
		{
			BYTE card = vecTargetCard[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());


	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndex.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		BYTE card = cbArrCardData[vecCardIndex[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndex.erase(vecCardIndex.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount,vecHandCard.size(), strHandCardData.c_str(),strHandCardIndex.c_str());

	return true;
}
bool	CGameMaJiangTable::GetHandCard_ThreeKeZi(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		vecCardIndex.push_back(index);
	}
	uint32 iTargetCount = 3;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		for (uint32 j = 0; j < 3; j++)
		{
			BYTE card = vecTargetCard[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());


	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndex.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		BYTE card = cbArrCardData[vecCardIndex[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndex.erase(vecCardIndex.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());


	return true;
}
bool	CGameMaJiangTable::GetHandCard_TwoKeZi_OneDuiZi(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		vecCardIndex.push_back(index);
	}
	uint32 iTargetCount = 2;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	uint32 iTargetCount_2 = 1;
	std::vector<int> vecTargetCard_2;
	for (uint32 i = 0; i < iTargetCount_2 && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard_2.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard_2.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard_2[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard_2.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard_2.size(), strTargetCard.c_str());
	}
	if (vecTargetCard_2.size() != iTargetCount_2)
	{
		return false;
	}


	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		for (uint32 j = 0; j < 3; j++)
		{
			BYTE card = vecTargetCard[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}

	for (uint32 i = 0; i < vecTargetCard_2.size(); i++)
	{
		for (uint32 j = 0; j < 2; j++)
		{
			BYTE card = vecTargetCard_2[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());


	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndex.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		BYTE card = cbArrCardData[vecCardIndex[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndex.erase(vecCardIndex.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());


	return true;
}
bool	CGameMaJiangTable::GetHandCard_TwoKeZi(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		vecCardIndex.push_back(index);
	}
	uint32 iTargetCount = 2;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		for (uint32 j = 0; j < 3; j++)
		{
			BYTE card = vecTargetCard[i];
			vecHandCard.push_back(card);
			cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
			iCardIndex++;
		}
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());


	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndex.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		BYTE card = cbArrCardData[vecCardIndex[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndex.erase(vecCardIndex.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());

	return true;
}
bool	CGameMaJiangTable::GetHandCard_TwelveTongHua(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndex.push_back(index);
		}
	}
	uint32 iTargetCount = 12;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		BYTE card = vecTargetCard[i];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		iCardIndex++;
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());

	BYTE cbArrCardDataFeng[] = { 0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLenFeng = getArrayLen(cbArrCardDataFeng);
	std::vector<int> vecCardIndexFeng;
	for (int index = 0; index < iArrCardLenFeng; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndexFeng.push_back(index);
		}
	}
	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndexFeng.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndexFeng.size() - 1);
		BYTE card = cbArrCardDataFeng[vecCardIndexFeng[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndexFeng.erase(vecCardIndexFeng.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());


	return true;
}
bool	CGameMaJiangTable::GetHandCard_ElevenTongHua(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndex.push_back(index);
		}
	}
	uint32 iTargetCount = 11;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		BYTE card = vecTargetCard[i];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		iCardIndex++;
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());

	BYTE cbArrCardDataFeng[] = { 0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLenFeng = getArrayLen(cbArrCardDataFeng);
	std::vector<int> vecCardIndexFeng;
	for (int index = 0; index < iArrCardLenFeng; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndexFeng.push_back(index);
		}
	}
	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndexFeng.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndexFeng.size() - 1);
		BYTE card = cbArrCardDataFeng[vecCardIndexFeng[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndexFeng.erase(vecCardIndexFeng.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());

	return true;
}
bool	CGameMaJiangTable::GetHandCard_TenTongHua(uint16 chairID)
{
	BYTE cbArrCardData[] = { 0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 };
	int iArrCardLen = getArrayLen(cbArrCardData);
	std::vector<int> vecCardIndex;
	for (int index = 0; index < iArrCardLen; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndex.push_back(index);
		}
	}
	uint32 iTargetCount = 10;
	std::vector<int> vecTargetCard;
	for (uint32 i = 0; i < iTargetCount && vecCardIndex.size()>0; i++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndex.size() - 1);
		vecTargetCard.push_back(cbArrCardData[vecCardIndex[uindex]]);
		vecCardIndex.erase(vecCardIndex.begin() + uindex);

		string strTargetCard;
		for (unsigned int i = 0; i < vecTargetCard.size(); ++i)
		{
			strTargetCard += CStringUtility::FormatToString("0x%02X ", vecTargetCard[i]);
		}
		string strCardIndex;
		for (unsigned int i = 0; i < vecCardIndex.size(); ++i)
		{
			strCardIndex += CStringUtility::FormatToString("%d ", vecCardIndex[i]);
		}
		LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iArrCardLen:%d,uindex:%d,vecCardIndex.szie():%d - %s,vecTargetCard.size():%d - %s",
			GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iArrCardLen, uindex, vecCardIndex.size(), strCardIndex.c_str(), vecTargetCard.size(), strTargetCard.c_str());
	}
	if (vecTargetCard.size() != iTargetCount)
	{
		return false;
	}

	// add hand card
	int iCardIndex = 0;
	BYTE cbCardIndex[MAX_INDEX] = { 0 };
	std::vector<BYTE> vecHandCard;
	for (uint32 i = 0; i < vecTargetCard.size(); i++)
	{
		BYTE card = vecTargetCard[i];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		iCardIndex++;
	}

	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iCardIndex:%d,iArrCardLen:%d,vecCardIndex.szie():%d,vecHandCard.size():%d",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iCardIndex, iArrCardLen, vecCardIndex.size(), vecHandCard.size());

	BYTE cbArrCardDataFeng[] = { 0x31,0x32,0x33,0x34,0x35,0x36,0x37 };
	int iArrCardLenFeng = getArrayLen(cbArrCardDataFeng);
	std::vector<int> vecCardIndexFeng;
	for (int index = 0; index < iArrCardLenFeng; index++)
	{
		for (int j = 0; j < 4; j++)
		{
			vecCardIndexFeng.push_back(index);
		}
	}
	// add hand card
	for (; iCardIndex < (MAX_COUNT - 1) && vecCardIndexFeng.size()>0; iCardIndex++)
	{
		unsigned int uindex = g_RandGen.RandRange(0, vecCardIndexFeng.size() - 1);
		BYTE card = cbArrCardDataFeng[vecCardIndexFeng[uindex]];
		vecHandCard.push_back(card);
		cbCardIndex[m_gameLogic.SwitchToCardIndex(card)]++;
		vecCardIndexFeng.erase(vecCardIndexFeng.begin() + uindex);
	}
	// remove card pool
	for (uint32 i = 0; i < vecHandCard.size(); i++)
	{
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecHandCard[i]);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
		}
	}

	BYTE iRobotCardCount = 0;
	for (int i = 0; i < MAX_INDEX; i++)
	{
		m_cbCardIndex[chairID][i] = cbCardIndex[i];
		iRobotCardCount += cbCardIndex[i];
	}

	string strHandCardData;
	for (unsigned int i = 0; i < vecHandCard.size(); ++i)
	{
		strHandCardData += CStringUtility::FormatToString("0x%02X ", vecHandCard[i]);
	}
	string strHandCardIndex;
	for (unsigned int i = 0; i < MAX_INDEX; ++i)
	{
		strHandCardIndex += CStringUtility::FormatToString("%d ", m_cbCardIndex[chairID][i]);
	}
	LOG_DEBUG("robot_hand_card- roomid:%d,tableid:%d,chairID:%d,uid:%d,iRobotCardCount:%d,vecHandCard.size():%d,strHandCardData:%s,strHandCardIndex:%s",
		GetRoomID(), GetTableID(), chairID, GetPlayerID(chairID), iRobotCardCount, vecHandCard.size(), strHandCardData.c_str(), strHandCardIndex.c_str());

	return true;
}

bool CGameMaJiangTable::SetActiveWelfareCtrlList()
{
    LOG_DEBUG("enter SetActiveWelfareCtrlList function.");

    //设置当前桌子游戏玩家列表
    for (uint16 i = 0; i < m_wPlayerCount; ++i)
    {       
        CGamePlayer* pTmp = GetPlayer(i);
        if (pTmp != NULL && !pTmp->IsRobot())
        {
            m_curr_bet_user.insert(pTmp->GetUID());
        }       
    }

    //获取当前局活跃福利的控制玩家列表
    GetActiveWelfareCtrlPlayerList();
        
    LOG_DEBUG("get aw ctrl player count:%d.", m_aw_ctrl_player_list.size());

    return true;
}

bool CGameMaJiangTable::GetInActiveWelfareCtrlList(uint32 uid, uint64 &aw_max_win)
{
    LOG_DEBUG("enter GetInActiveWelfareCtrlList ctrl player count:%d uid:%d.", m_aw_ctrl_player_list.size(), uid);
    aw_max_win = 0;
    vector<tagAWPlayerInfo>::iterator iter = m_aw_ctrl_player_list.begin();
    for (; iter != m_aw_ctrl_player_list.end(); iter++)
    {        		
        if (iter->uid== uid)
        {
			//判断当前控制玩家是否在配置概率范围内
			uint32 tmp = rand() % 100;
			uint32 probability = iter->probability;
			if (tmp > probability)
			{
				LOG_DEBUG("The current player is not in config rate - control_uid:%d tmp:%d probability:%d", uid, tmp, probability)
			    continue;
			}

	        LOG_DEBUG("the current player is in aw control list. uid:%d max_win:%d", uid, iter->max_win);
            aw_max_win = iter->max_win;
            return true;
        }       
    }
    LOG_DEBUG("the current player is not in aw control list. return false.");
    return false;
}

bool    CGameMaJiangTable::SetAWTingCardZiMo(uint16 chairID, BYTE & cbOutSendCardData, uint64 aw_max_win)
{
    LOG_DEBUG("enter SetAWTingCardZiMo function. chairID:%d aw_max_win:%llu.", chairID, aw_max_win);

    if (chairID >= m_wPlayerCount)
    {
        return false;
    }
       
    vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;

    DWORD action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
    if (action != ACTION_LISTEN || vecNotifyHuCard.size() == 0)
    {
        return false;
    }
    for (uint32 i = 0; i < vecNotifyHuCard.size(); i++)
    {
        // remove card pool
		uint32 fan = 1;
		if (m_iUserPassHuCount[chairID] >= 1)
		{
			fan = (vecNotifyHuCard[i].score + 3)*m_iUserPassHuCount[chairID] * 2;
		}
		else
		{
			fan = vecNotifyHuCard[i].score + 3;
		}
        uint64 curr_win = fan * GetBaseScore();  //+3 代表：自摸+听牌
        auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
        if (iter != m_poolCards.end() && curr_win<=aw_max_win)
        {
            m_poolCards.erase(iter);
            cbOutSendCardData = vecNotifyHuCard[i].cbCard;           
            LOG_DEBUG("search success aw control info. chairID:%d curr_win:%llu aw_max_win:%llu fan:%d basescore:%d passhu:%d score:%d.", chairID, curr_win, aw_max_win, fan, GetBaseScore(), m_iUserPassHuCount[chairID], vecNotifyHuCard[i].score);
            return true;
        }
    }
    LOG_DEBUG("search fail aw control info. chairID:%d aw_max_win:%llu passhu:%d.", chairID, aw_max_win, m_iUserPassHuCount[chairID]);
    return false;
}

bool    CGameMaJiangTable::SetNRWTingCardZiMo(uint16 chairID, BYTE & cbOutSendCardData)
{
	LOG_DEBUG("enter SetAWTingCardZiMo function. chairID:%d.", chairID);

	if (chairID >= m_wPlayerCount)
	{
		return false;
	}

	vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;

	DWORD action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
	if (action != ACTION_LISTEN || vecNotifyHuCard.size() == 0)
	{
		return false;
	}
	for (uint32 i = 0; i < vecNotifyHuCard.size(); i++)
	{
		// remove card pool
		uint32 fan = 1;
		if (m_iUserPassHuCount[chairID] >= 1)
		{
			fan = (vecNotifyHuCard[i].score + 3)*m_iUserPassHuCount[chairID] * 2;
		}
		else
		{
			fan = vecNotifyHuCard[i].score + 3;
		}
		uint64 curr_win = fan * GetBaseScore();  //+3 代表：自摸+听牌
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
			cbOutSendCardData = vecNotifyHuCard[i].cbCard;
			LOG_DEBUG("search success aw control info. chairID:%d curr_win:%llu fan:%d basescore:%d passhu:%d score:%d.", chairID, curr_win, fan, GetBaseScore(), m_iUserPassHuCount[chairID], vecNotifyHuCard[i].score);
			return true;
		}
	}
	LOG_DEBUG("search fail aw control info. chairID:%d passhu:%d.", chairID,  m_iUserPassHuCount[chairID]);
	return false;
}

bool    CGameMaJiangTable::SetNRWRobotTingCardZiMo(uint16 chairID, BYTE & cbOutSendCardData)
{
	LOG_DEBUG("SetNRWRobotTingCardZiMo chairID:%d.", chairID);
	if (chairID >= m_wPlayerCount)
	{
		return false;
	}
	CGamePlayer * pPlayer = GetPlayer(chairID);
	if (pPlayer == NULL)
	{
		return false;
	}
	if (pPlayer->IsRobot() == false)
	{
		return false;
	}
	if (m_cbListenStatus[chairID] == FALSE)
	{
		return false;
	}
	
	vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;

	DWORD action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
	if (action != ACTION_LISTEN || vecNotifyHuCard.size() == 0)
	{
		return false;
	}
	for (uint32 i = 0; i < vecNotifyHuCard.size(); i++)
	{		
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
			cbOutSendCardData = vecNotifyHuCard[i].cbCard;
			break;
		}
	}
	return true;
}

int CGameMaJiangTable::GetNRWProCardType()
{
	string strDispatchCardPro;
	int iSumValue = 0;
	int iArrDispatchCardPro[Pro_Index_MAX] = { 0 };
	for (int i = 0; i < Pro_Index_MAX; i++)
	{
		iArrDispatchCardPro[i] = m_NRWDispatchCardPro[i];
		iSumValue += m_NRWDispatchCardPro[i];
		strDispatchCardPro += CStringUtility::FormatToString("%d ", m_NRWDispatchCardPro[i]);
	}
	if (iSumValue <= 0)
	{
		return Pro_Index_RandSingle;
	}
	int iRandNum = g_RandGen.RandRange(0, iSumValue);
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
		iProIndex = Pro_Index_RandSingle;
	}
	LOG_DEBUG("NRW roomid:%d,tableid:%d,iSumValue:%d,iRandNum:%d,iProIndex:%d,strDispatchCardPro:%s", GetRoomID(), GetTableID(), iSumValue, iRandNum, iProIndex, strDispatchCardPro.c_str());
	return iProIndex;
}

bool CGameMaJiangTable::SetNRWControlCardData()
{
	LOG_DEBUG("enter SetNRWControlCardData function. room_id:%d tid:%d", GetRoomID(), GetTableID());
	bool bIsHaveRealPlayer = false;
	uint16 uChairID = 255;

	//判断当前桌子是否为新注册玩家福利桌子
	if (!IsNewRegisterWelfareTable())
	{
		LOG_DEBUG("the current table is not new register welfare table. room_id:%d tid:%d", GetRoomID(), GetTableID());
		return false;
	}

	for (WORD i = 0; i < m_wPlayerCount; i++)
	{
		CGamePlayer * pPlayer = GetPlayer(i);
		if (pPlayer != NULL && !pPlayer->IsRobot())
		{
			int control_status = 0;
			pPlayer->GetNewRegisterWelfareStatus(control_status);
			if (control_status == 1)
			{
				bIsHaveRealPlayer = true;
				uChairID = i;
			}			
		}
	}

	int iProIndex = GetNRWProCardType();
	
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d - %d,bIsHaveRealPlayer:%d,uChairID:%d,m_wPlayerCount:%d,iProIndex:%d",
		GetRoomID(), GetTableID(), GetPlayerID(0), GetPlayerID(1), bIsHaveRealPlayer, uChairID, m_wPlayerCount, iProIndex);

	if (bIsHaveRealPlayer == false || uChairID >= m_wPlayerCount || iProIndex == Pro_Index_RandSingle)
	{
		return false;
	}
	bool bFlag = false;
	if (iProIndex == Pro_Index_FiveDuiZi)
	{
		bFlag = GetHandCard_FiveDuiZi(uChairID);
	}
	if (iProIndex == Pro_Index_FourDuiZi)
	{
		bFlag = GetHandCard_FourDuiZi(uChairID);
	}
	if (iProIndex == Pro_Index_ThreeKeZi)
	{
		bFlag = GetHandCard_ThreeKeZi(uChairID);
	}
	if (iProIndex == Pro_Index_TwoKeZi_OneDuiZi)
	{
		bFlag = GetHandCard_TwoKeZi_OneDuiZi(uChairID);
	}
	if (iProIndex == Pro_Index_TwoKeZi)
	{
		bFlag = GetHandCard_TwoKeZi(uChairID);
	}
	if (iProIndex == Pro_Index_TwelveTongHua)
	{
		bFlag = GetHandCard_TwelveTongHua(uChairID);
	}
	if (iProIndex == Pro_Index_ElevenTongHua)
	{
		bFlag = GetHandCard_ElevenTongHua(uChairID);
	}
	if (iProIndex == Pro_Index_TenTongHua)
	{
		bFlag = GetHandCard_TenTongHua(uChairID);
	}
	return bFlag;
}

void CGameMaJiangTable::GetLuckyPlayerUid()
{
	uint32 win_uid = 0;
	set<uint32> set_lose_uid;
	set_lose_uid.clear();

	bool flag = GetTableLuckyFlag(win_uid, set_lose_uid);

	LOG_DEBUG("flag:%d win_uid:%d set_lose_uid size:%d.", flag, win_uid, set_lose_uid.size());

	if (!flag)
	{
		m_lucky_win_uid = 0;
		return ;
	}

	//如果设置了赢家的情况
	if (win_uid != 0)
	{
		m_lucky_win_uid = win_uid;
		m_set_ctrl_lucky_uid.clear();
		m_set_ctrl_lucky_uid = set_lose_uid;
		m_set_ctrl_lucky_uid.insert(m_lucky_win_uid);
		return;		
	}

	//如果设置了输家的情况
	if (set_lose_uid.size() == 1)
	{
		for (WORD i = 0; i < m_wPlayerCount; i++)
		{
			CGamePlayer * pPlayer = GetPlayer(i);
			if (pPlayer != NULL)
			{
				auto iter = set_lose_uid.find(pPlayer->GetUID());
				if (iter == set_lose_uid.end())
				{
					m_lucky_win_uid = pPlayer->GetUID();
					break;
				}
				else
				{
					continue;
				}
			}
		}
		m_set_ctrl_lucky_uid.clear();
		m_set_ctrl_lucky_uid = set_lose_uid;		
	}
}

bool    CGameMaJiangTable::SetLuckyTingCardZiMo(uint16 chairID, BYTE & cbOutSendCardData)
{
	LOG_DEBUG("enter SetLuckyTingCardZiMo function. chairID:%d.", chairID);

	if (chairID >= m_wPlayerCount)
	{
		return false;
	}

	vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;

	DWORD action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
	if (action != ACTION_LISTEN || vecNotifyHuCard.size() == 0)
	{
		return false;
	}
	for (uint32 i = 0; i < vecNotifyHuCard.size(); i++)
	{
		// remove card pool
		uint32 fan = 1;
		if (m_iUserPassHuCount[chairID] >= 1)
		{
			fan = (vecNotifyHuCard[i].score + 3)*m_iUserPassHuCount[chairID] * 2;
		}
		else
		{
			fan = vecNotifyHuCard[i].score + 3;
		}
		uint64 curr_win = fan * GetBaseScore();  //+3 代表：自摸+听牌
		auto iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
		if (iter != m_poolCards.end())
		{
			m_poolCards.erase(iter);
			cbOutSendCardData = vecNotifyHuCard[i].cbCard;
			LOG_DEBUG("search success lucky control info. chairID:%d curr_win:%llu fan:%d basescore:%d passhu:%d score:%d.", chairID, curr_win, fan, GetBaseScore(), m_iUserPassHuCount[chairID], vecNotifyHuCard[i].score);
			return true;
		}
	}
	LOG_DEBUG("search fail lucky control info. chairID:%d passhu:%d.", chairID, m_iUserPassHuCount[chairID]);
	return false;
}

// 设置库存输赢  add by har
bool CGameMaJiangTable::SetStockWinLose() {
	if (IsAllRobotOrPlayerJetton())
		return false;

	stStockCfg &roomStockCfg = m_pHostRoom->GetRoomStockCfg();
	int64 jackpotSub = labs(roomStockCfg.jackpot - roomStockCfg.jackpotMin);
	if (roomStockCfg.jackpot < roomStockCfg.killPointsLine) {
		int64 oldPlayerWinRate = roomStockCfg.playerWinRate + PRO_DENO_10000 * jackpotSub / roomStockCfg.jackpotCoefficient;
		int64 playerWinRate = oldPlayerWinRate;
		if (playerWinRate < 0)
			playerWinRate = 0;
		else if (playerWinRate > PRO_DENO_10000)
			playerWinRate = PRO_DENO_10000;
		m_stockTingCardZiMoPro = -playerWinRate;
		LOG_DEBUG("CGameMaJiangTable::IsStockChangeCard true  roomid:%d,tableid:%d,m_stockTingCardZiMoPro:%d,playerWinRate:%lld,oldPlayerWinRate:%lld,jackpotSub:%lld,jackpot:%lld,killPointsLine:%lld,jackpotMin:%lld,confPlayerWinRate:%d,jackpotCoefficient:%d",
			GetRoomID(), GetTableID(), m_stockTingCardZiMoPro, playerWinRate, oldPlayerWinRate, jackpotSub, roomStockCfg.jackpot, roomStockCfg.killPointsLine, roomStockCfg.jackpotMin, roomStockCfg.playerWinRate, roomStockCfg.jackpotCoefficient);
		return true;
	}

	if (roomStockCfg.jackpot < roomStockCfg.jackpotMin) {
		LOG_DEBUG("CGameMaJiangTable::IsStockChangeCard false roomid:%d,tableid:%d,jackpot:%lld,jackpotMin:%lld",
			GetRoomID(), GetTableID(), roomStockCfg.jackpot, roomStockCfg.jackpotMin);
		return false;
	}

	int64 oldPlayerWinRate2 = roomStockCfg.jackpotRate + PRO_DENO_10000 * jackpotSub / roomStockCfg.jackpotCoefficient;
	int64 playerWinRate2 = oldPlayerWinRate2; 
	if (playerWinRate2 < 0)
		playerWinRate2 = 0;
	else if (playerWinRate2 > PRO_DENO_10000)
		playerWinRate2 = PRO_DENO_10000;
	m_stockTingCardZiMoPro = playerWinRate2;
	LOG_DEBUG("CGameMaJiangTable::IsStockChangeCard true2 roomid:%d,tableid:%d,jackpotMaxRate:%d,jackpotSub:%lld,m_stockTingCardZiMoPro:%d,playerWinRate2:%lld,oldPlayerWinRate2:%lld",
		GetRoomID(), GetTableID(), roomStockCfg.jackpotMaxRate, jackpotSub, m_stockTingCardZiMoPro, playerWinRate2, oldPlayerWinRate2);

    return true;
}

// 设置库存玩家听牌自摸发牌  add by har
bool CGameMaJiangTable::SetStockTingCardZiMo(uint16 chairID, uint8 &cbOutSendCardData) {
	if (m_stockTingCardZiMoPro == 0)
		return false;

	uint32 action = ACTION_NULL;
	bool robotEnd = false;
	vector<tagAnalyseTingNotifyHu > vecNotifyHuCard;
	if (m_stockTingCardZiMoPro < 0) { // 机器人赢
		uint32 tempPro = m_uRobotZiMoPro;
		m_uRobotZiMoPro = -m_stockTingCardZiMoPro;
		robotEnd = SetRobotTingCardZiMo(chairID, cbOutSendCardData, false);
		m_uRobotZiMoPro = tempPro;
	} else { // 玩家赢
		CGamePlayer *pPlayer = GetPlayer(chairID);
		if (pPlayer != NULL && !pPlayer->IsRobot()) {
			if (g_RandGen.RandRatio(m_stockTingCardZiMoPro, PRO_DENO_10000)) {
				action = m_gameLogic.AnalyseTingCard16(chairID, m_cbCardIndex[chairID], m_WeaveItemArray[chairID], m_cbWeaveItemCount[chairID], vecNotifyHuCard);
				if (action == ACTION_LISTEN && vecNotifyHuCard.size() > 0) {
					for (uint32 i = 0; i < vecNotifyHuCard.size(); ++i) {
						list<uint8>::iterator iter = std::find(m_poolCards.begin(), m_poolCards.end(), vecNotifyHuCard[i].cbCard);
						if (iter != m_poolCards.end()) {
							m_poolCards.erase(iter);
							cbOutSendCardData = vecNotifyHuCard[i].cbCard;
							break;
						}
					}
				}
			}
		}
	}
	if (cbOutSendCardData != 0xff) {
		LOG_DEBUG("SetStockTingCardZiMo suc. roomid:%d,tableid:%d,chairID:%d,m_stockTingCardZiMoPro:%d,m_uRobotZiMoPro:%d,cbOutSendCardData:%d,robotEnd:%d,action:%d,vecNotifyHuCard_size:%d,m_wPlayerCount:%d,m_cbListenStatus[chairID]:%d",
			GetRoomID(), GetTableID(), chairID, m_stockTingCardZiMoPro, m_uRobotZiMoPro, cbOutSendCardData, robotEnd, action, vecNotifyHuCard.size(), m_wPlayerCount, m_cbListenStatus[chairID]);
		return true;
	}
	LOG_ERROR("SetStockTingCardZiMo fail. roomid:%d,tableid:%d,chairID:%d,m_stockTingCardZiMoPro:%d,m_uRobotZiMoPro:%d,robotEnd:%d,action:%d,vecNotifyHuCard_size:%d,m_wPlayerCount:%d,m_cbListenStatus[chairID]:%d",
		GetRoomID(), GetTableID(), chairID, m_stockTingCardZiMoPro, m_uRobotZiMoPro, robotEnd, action, vecNotifyHuCard.size(), m_wPlayerCount, m_cbListenStatus[chairID]);
	return false;
}

void   CGameMaJiangTable::CheckPlayerScoreManyLeave()
{
	LOG_DEBUG("CheckPlayerScoreManyLeave roomid:%d,tableid:%d m_wPlayerCount:%d",	GetRoomID(), GetTableID(), m_wPlayerCount);
	if (m_pHostRoom == NULL)
	{
		LOG_DEBUG("roomid:%d,tableid:%d,m_pHostRoom:%p", GetRoomID(), GetTableID(), m_pHostRoom);
		return; 
	}

	for (uint16 i = 0; i < m_wPlayerCount; ++i)
	{
		CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
		if (pPlayer != NULL)
		{
			int64 lCurScore = GetPlayerCurScore(GetPlayer(i));
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