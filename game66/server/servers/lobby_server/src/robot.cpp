//
// Created by toney on 16/5/29.
//

#include <gobal_robot_mgr.h>
#include "robot.h"
#include "stdafx.h"
#include "json/json.h"

namespace
{
	static const int64 s_RichLvScore[] = { 0,				// 0
											350,			// 1
											715,			// 2
											1520,			// 3
											5580,			// 4
											8000,			// 5
											13500,			// 6
											25000,			// 7
											50000,			// 8
											100000,			// 9
											200000,			// 10
											350000,			// 11
											500000,			// 12
											1000000,		// 13
											2000000,		// 14
											5000000,		// 15
											8000000,		// 16
											10000000 };		// 17

	static const int64 s_RichLvCoin[] = { 0,				// 0
											350,			// 1
											715,			// 2
											1520,			// 3
											5580,			// 4
											8000,			// 5
											13500,			// 6
											25000,			// 7
											50000,			// 8
											100000,			// 9
											200000,			// 10
											350000,			// 11
											500000,			// 12
											1000000,		// 13
											2000000,		// 14
											5000000,		// 15
											8000000,		// 16
											10000000 };		// 17


    static const int32 s_SafeBoxPro         = 0;     // 保险箱倍数
    static const int32 s_CoinPro            = 2;     // 财富币倍数
    static const int32 s_ScorePro           = 2;     // 积分倍数    
};

CLobbyRobot::CLobbyRobot()
:CPlayer(PLAYER_TYPE_ROBOT)
{
    m_robotCool.beginCooling(10000);
    m_loginOutTime = getSysTime() + SECONDS_IN_ONE_HOUR;
    m_needLoginOut = false;
    m_leaveRound = g_RandGen.RandRange(100,200);
}
CLobbyRobot::~CLobbyRobot()
{

}
bool 	CLobbyRobot::OnLoginOut(uint32 leaveparam)
{
    CPlayer::OnLoginOut(leaveparam);
    CDBMysqlMgr::Instance().SetRobotLoginState(GetUID(),0);
	return true;
}
void	CLobbyRobot::OnGetAllData()
{
    CPlayer::OnGetAllData();
    TidyCoin();

	LOG_DEBUG("机器人上线 gameType:%d,uid:%d--score:%lld--%lld,--coin:%lld--%lld", m_robotCfg.gameType, GetUID(), GetAccountValue(emACC_VALUE_SCORE), GetLoginOutScore(), GetAccountValue(emACC_VALUE_COIN), GetLoginOutCoin());

    m_loginOutTime = getSysTime() +  g_RandGen.RandRange(SECONDS_IN_MIN*10,SECONDS_IN_30_MIN);
    m_robotCool.beginCooling(g_RandGen.RandRange(60000,300000));
    m_leaveRound = g_RandGen.RandRange(50,100);
    CDBMysqlMgr::Instance().AddRobotLoginCount(GetUID());
    CDBMysqlMgr::Instance().SetRobotLoginState(GetUID(),1);
}
void 	CLobbyRobot::OnTimeTick(uint64 uTime,bool bNewDay)
{
    CPlayer::OnTimeTick(uTime,bNewDay);

    if(m_robotCool.isTimeOut()){
        CheckEnterToGameSvr();
        m_robotCool.beginCooling(g_RandGen.RandRange(10000,20000));
    }
}
// 是否需要回收
bool  CLobbyRobot::NeedRecover()
{
    if(GetPlayerState() == PLAYER_STATE_LOAD_DATA && (getSysTime()-m_loadTime) > SECONDS_IN_MIN)
	{
        LOG_ERROR("加载数据超时下线 - uid:%d",GetUID());
        return true;
    }
    if(!IsInLobby() || GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        return false;
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
        return true;
    if(GetUID() >= ROBOT_MGR_ID)
	{
        return false;
    }
	int32 dwRemaindTime = CDataCfgMgr::Instance().GetRobotBatchOut(GetBatchID());
	if (dwRemaindTime==0)
	{
		LOG_DEBUG("robot_leave_time_arrive - uid:%d,gametype:%d,batchID:%d,dwRemaindTime:%d", GetUID(), GetGameType(), GetBatchID(), dwRemaindTime);
		return true;
	}
    if(/*GetAccountValue(emACC_VALUE_SCORE) < GetLoginOutScore()||*/ GetAccountValue(emACC_VALUE_COIN) < GetLoginOutCoin())
	{
        LOG_DEBUG("机器人%d破产了回收下线 - gameType:%d,uid:%d--score:%lld--%lld,--coin:%lld--%lld",m_robotCfg.gameType,GetUID(),GetAccountValue(emACC_VALUE_SCORE),GetLoginOutScore(),GetAccountValue(emACC_VALUE_COIN),GetLoginOutCoin());
        return true;
    }
    if(getSysTime() > m_loginOutTime)
	{
        LOG_DEBUG("机器人在玩超过时间,下线休息 - uid:%d",GetUID());
        return true;
    }
    if(m_leaveRound <= 0)
	{
        LOG_DEBUG("机器人游戏局数满了,下线 - uid:%d,gametype:%d",GetUID(),GetGameType());
        return true;
    }

    if(m_needLoginOut){
        return true;
    }

    return false;
}
// 返回大厅回调
void  CLobbyRobot::BackLobby()
{
    TidyCoin();
    m_robotCool.beginCooling(SECONDS_IN_MIN*1000);
}
// 游戏战报
void  CLobbyRobot::OnGameEnd(uint16 gameType)
{
    m_leaveRound--;
}
void  CLobbyRobot::SetRobotCfg(stRobotCfg& cfg)
{
    m_robotCfg = cfg;
}
int64   CLobbyRobot::GetRichLvScore()
{
    if(m_robotCfg.scoreLv >= getArrayLen(s_RichLvScore))
        return s_RichLvScore[0];
    return s_RichLvScore[m_robotCfg.scoreLv];
}
int64   CLobbyRobot::GetRichLvCoin()
{
    if(m_robotCfg.richLv  >= getArrayLen(s_RichLvCoin))
        return s_RichLvCoin[0];
    return s_RichLvCoin[m_robotCfg.richLv];
}
int64   CLobbyRobot::GetLoginOutScore()
{
    if(m_robotCfg.scoreLv >= getArrayLen(s_RichLvScore))
        return s_RichLvScore[0];
    return s_RichLvScore[m_robotCfg.scoreLv];
}
int64   CLobbyRobot::GetLoginOutCoin()
{
    if(m_robotCfg.richLv  >= getArrayLen(s_RichLvCoin))
        return s_RichLvCoin[0];
    return s_RichLvCoin[m_robotCfg.richLv];
}
uint16  CLobbyRobot::GetGameType()
{
    return m_robotCfg.gameType;
}

uint16  CLobbyRobot::GetCfgLevel()
{
	uint8  cfgLevel = 0;
	if (m_robotCfg.leveltype == net::ROOM_CONSUME_TYPE_SCORE)
	{
		cfgLevel = m_robotCfg.scoreLv;
	}
	else if (m_robotCfg.leveltype == net::ROOM_CONSUME_TYPE_COIN)
	{
		cfgLevel = m_robotCfg.richLv;
	}
    return cfgLevel;
}
uint16  CLobbyRobot::GetLoginType()
{
	return m_robotCfg.loginType;
}
bool    CLobbyRobot::CanEnterGameType(uint16 gameType)
{
    if(m_robotCfg.gameType == 0)
        return true;
    return m_robotCfg.gameType == gameType;
}
// 获得机器人是否需要存入积分财富币
bool    CLobbyRobot::IsNeedStore(int64 &score,int64 &coin)
{
    score = 0;
    coin  = 0;
    int64 richLvScore = GetRichLvScore();
    int64 richLvCoin  = GetRichLvCoin();
    int64 needCoin    = richLvCoin*s_CoinPro;
    int64 needScore   = richLvScore*s_ScorePro;

	if (GetAccountValue(emACC_VALUE_SCORE) > needScore)
	{
		score = (GetAccountValue(emACC_VALUE_SCORE) - needScore);
	}
	//if (GetAccountValue(emACC_VALUE_COIN) > needCoin)
	//{
	//	coin = (GetAccountValue(emACC_VALUE_COIN) - needCoin);
	//}

	LOG_DEBUG("1- uid:%d,score:%lld,coin:%lld,richLvCoin:%lld,needCoin:%lld,curCoin:%lld", GetUID(), score, coin, richLvCoin, needCoin, GetAccountValue(emACC_VALUE_COIN));


	//2.5倍以上的金币全部存入机器人池子
	if (GetAccountValue(emACC_VALUE_COIN) * 10 >= richLvCoin * 25)
	{
		coin = GetAccountValue(emACC_VALUE_COIN);

		LOG_DEBUG("2- uid:%d,score:%lld,coin:%lld,richLvCoin:%lld,needCoin:%lld,curCoin:%lld", GetUID(), score, coin, richLvCoin, needCoin, GetAccountValue(emACC_VALUE_COIN));


		return (score != 0 || coin != 0);
	}

	// 2倍一定会存金币，存金币的比例为身上金币的3分之1到7分之1，向下取整
	bool bIsStore = false;
	if (GetAccountValue(emACC_VALUE_COIN) * 10 >= richLvCoin * 17 && GetAccountValue(emACC_VALUE_COIN) * 10 < richLvCoin * 21)
	{
		if (g_RandGen.RandRatio(30, PRO_DENO_100))
		{
			bIsStore = true;
		}
	}
	if (GetAccountValue(emACC_VALUE_COIN) * 10 >= richLvCoin * 21)
	{
		bIsStore = true;
	}
	if (bIsStore)
	{
		int64 denominator = g_RandGen.RandRange(4, 9);
		coin = GetAccountValue(emACC_VALUE_COIN) / denominator;
		LOG_DEBUG("3- uid:%d,score:%lld,coin:%lld,richLvCoin:%lld,needCoin:%lld,curCoin:%lld,denominator:%lld",
			GetUID(), score, coin, richLvCoin, needCoin, GetAccountValue(emACC_VALUE_COIN), denominator);
	}

    return (score != 0 || coin != 0);
}

// 需要兑换的积分（机器人取金币）
bool    CLobbyRobot::IsNeedExchange(int64 &score,int64 &coin)
{
    score = 0;
    coin  = 0;
    int64 richLvScore = GetRichLvScore();
    int64 richLvCoin  = GetRichLvCoin();
	
	if (GetAccountValue(emACC_VALUE_SCORE) < richLvScore)
	{
		score = richLvScore - GetAccountValue(emACC_VALUE_SCORE);
	}
	//if (GetAccountValue(emACC_VALUE_COIN) < richLvCoin)
	//{
	//	coin = richLvCoin - GetAccountValue(emACC_VALUE_COIN);
	//}

	//低于当前财富币等级额度，低于当前额度，重置财富币等级金额为当前等级额度的1.01到1.19倍
	int64 denominator = g_RandGen.RandRange(101, 119);
	int64 lProRandCoin = 0;
	if (GetAccountValue(emACC_VALUE_COIN) <= richLvCoin)
	{
		lProRandCoin = richLvCoin * denominator - GetAccountValue(emACC_VALUE_COIN) * 100;
		if (lProRandCoin >= 100)
		{
			coin = lProRandCoin / 100;
		}
	}

	LOG_DEBUG("uid:%d,score:%lld,coin:%lld,richLvCoin:%lld,gameType:%d,curScore:%lld,curCoin:%lld,lProRandCoin:%lld",
		GetUID(), score, coin, richLvCoin, m_robotCfg.gameType, GetAccountValue(emACC_VALUE_SCORE), GetAccountValue(emACC_VALUE_COIN), lProRandCoin);


	//LOG_DEBUG("uid:%d,score:%lld,coin:%lld,richLvCoin:%lld,gameType:%d,curCoin:%lld", GetUID(), score, coin, richLvCoin, m_robotCfg.gameType, GetAccountValue(emACC_VALUE_COIN));

    return (score != 0 || coin != 0);
}
// 获得机器人存取保险箱财富币
bool    CLobbyRobot::IsNeedTakeSafeBox(int64 &coin)
{
    int64 richCoin = GetRichLvCoin();
    int64 needCoin = richCoin * s_CoinPro;
    int64 safeCoin = richCoin * s_SafeBoxPro;
    if(GetAccountValue(emACC_VALUE_COIN) > needCoin)
    {
        if(GetAccountValue(emACC_VALUE_SAFECOIN) > safeCoin){
            return false;
        }else{
            coin = -(GetAccountValue(emACC_VALUE_COIN) - needCoin);
            return true;
        }
    }
    if(GetAccountValue(emACC_VALUE_COIN) < richCoin)
	{
        if(GetAccountValue(emACC_VALUE_SAFECOIN) >= richCoin)
		{
            coin = richCoin;
            return true;
        }
        coin = GetAccountValue(emACC_VALUE_SAFECOIN);
        return true;
    }
    return false;
}
// 处理金币存取
bool    CLobbyRobot::TidyCoin()
{
    if(GetUID() >= ROBOT_MGR_ID){
        return false;
    }
    //LOG_DEBUG("机器人 %d 整理金币",GetUID());
    //财富币存取
    //int64 takeCoin = 0;
    //if(IsNeedTakeSafeBox(takeCoin)){
    //    TakeSafeBox(takeCoin);
    //}
    // 存入盈利

	LOG_DEBUG("robot_cur_score_1  - uid:%d,gameType:%d,cur_score:%lld,cur_coin:%lld", GetUID(), m_robotCfg.gameType, GetAccountValue(emACC_VALUE_SCORE),GetAccountValue(emACC_VALUE_COIN));

    int64 score = 0,coin = 0;
    if(IsNeedStore(score,coin))
    {
        if(!CGobalRobotMgr::Instance().StoreScoreCoin(this,score,coin))
		{
			LOG_ERROR("robot StoreScoreCoin error - uid:%d,gameType:%d", GetUID(), m_robotCfg.gameType);
        }
    }
    // 兑换积分财富币
    if(IsNeedExchange(score, coin))
    {
        LOG_DEBUG("uid:%d,gameType:%d, IsNeedExchange - score:%lld,coin:%lld", GetUID(), m_robotCfg.gameType,score, coin);
        if(score > 0)
        {
            if(CGobalRobotMgr::Instance().TakeScoreCoin(this,score,0))
            {                    
                SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,0,0,0,0,score,0,0);
            }                
        }
        if(coin > 0)
        {
            if(CGobalRobotMgr::Instance().TakeScoreCoin(this,0,coin))
            {
                SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,1,0,coin,0,0,0,0);
            }                
        }
    }
    
	LOG_DEBUG("robot_cur_score_2  - uid:%d,gameType:%d,cur_score:%lld,cur_coin:%lld", GetUID(), m_robotCfg.gameType, GetAccountValue(emACC_VALUE_SCORE),GetAccountValue(emACC_VALUE_COIN));

    return true;
}
// 是否需要进入游戏服务器
bool    CLobbyRobot::CheckEnterToGameSvr()
{
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
        return false;
    if(GetUID() >= ROBOT_MGR_ID){
        return false;
    }

    if(IsPlaying() && IsInLobby())
    {
		bool bNeedRecover = NeedRecover();
		uint32 svrid = CServerMgr::Instance().GetRobotSvrID(this);

		LOG_DEBUG("robot_enter_GameSvr - uid:%d,batchid:%d,gameType:%d,svrid:%d,m_curSvrID:%d,bNeedRecover:%d", GetUID(), GetBatchID(), m_robotCfg.gameType, svrid, m_curSvrID, bNeedRecover);

        if(!bNeedRecover)
        {
            if(svrid == 0)
			{
                LOG_DEBUG("robot enter game server id is 0 - uid:%d,batchid:%d,gameType:%d",GetUID(), GetBatchID(), m_robotCfg.gameType);
                return false;
            }
			if (m_robotCfg.gameType==net::GAME_CATE_EVERYCOLOR)
			{
				LOG_DEBUG("robot enter game EnterEveryColorGameSvr - uid:%d,batchid:%d,gameType:%d", GetUID(), GetBatchID(), m_robotCfg.gameType);

				return EnterEveryColorGameSvr(svrid);
			}
			else
			{

				return EnterGameSvr(svrid);
			}
        }
    }
    return false;
}


