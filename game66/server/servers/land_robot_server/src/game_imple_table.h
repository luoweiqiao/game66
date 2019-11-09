//
// Created by toney on 16/4/6.
//
// 斗地主的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "land_logic.h"

#include "OGLordRobot.h"
#include "CardsEvaluating.h"

//#include "DDZRobot.h"

//  1.定义机器人期望输赢为T.定义机器人总输赢为C.
//  2.设置机器人止损线 T3<T2<T1<T.
//	3.当C大于T时候, 可以不控制, 也可以控制让玩家赢一些, 1 / 5概率去调整玩家的牌.
//	4.当C小于T且大于T1时候, 1 / 4概率去调整AI的好牌.
//	5.当C小于T1且大于T2时候, 1 / 2概率去调整AI的好牌.
//	6.当C小于T2且大于T3时候, 3 / 4概率去调整AI的好牌.
//	7.当C小于T3时候, 100 % 概率去调整AI的好牌.

using namespace svrlib;
using namespace std;
using namespace game_land;

class CGamePlayer;
class CGameRoom;

// 斗地主游戏桌子
class CGameLandTable : public CGameTable
{
public:
    CGameLandTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameLandTable();

    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void GetTableFaceInfo(net::table_face_info* pInfo);
public:
    //配置桌子
    virtual bool Init();
    virtual void ShutDown();
	virtual bool ReAnalysisParam();

    //复位桌子
    virtual void ResetTable();
    virtual void OnTimeTick();
    //游戏消息
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);

public:
    // 游戏开始
    virtual bool OnGameStart();
    // 游戏结束
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //用户同意
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
public:
    // 发送手中扑克牌
    void    SendHandleCard(uint16 chairID);
	void    SendHandleCard_2(uint16 chairID);

    // 用户放弃
    bool    OnUserPassCard(uint16 chairID);
    // 用户叫分
    bool    OnUserCallScore(uint16 chairID,uint8 score);
    void    OnCallScoreTimeOut();

    // 用户出牌
    bool    OnUserOutCard(uint16 chairID,uint8 cardData[],uint8 cardCount);
    void    OnOutCardTimeOut();
    // 托管出牌
    void    OnUserAutoCard();

    // 出牌时间跟叫地主时间
    uint32  GetCallScoreTime();
    uint32  GetOutCardTime();

    void    CalcPlayerInfo(CGamePlayer* pPlayer,uint16 chairID,uint8 win,uint8 spring,uint8 land,int64 winScore);
    // 游戏重新开始
    void    ReGameStart();
    // 重置游戏数据
    void    ResetGameData();
    // 添加防黑名单
    void    AddBlockers();

protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 重置扑克
    void    ReInitPoker();
    // 洗牌
    void    ShuffleTableCard(uint8 dealType);
    // 发牌并返回当前玩家位置
    uint16  DispatchUserCard(uint16 startUser,uint32 cardIndex);
    uint16  DispatchUserCardByDeal(const uint8 dealArry[],uint8 dealCount,uint16 startUser,uint32 cardIndex);

	uint16  PushUserCard(uint16 startUser, uint8 validCardData, uint8 & cardIndex);
	uint16  PushUserCardByDeal(const uint8 dealArry[], uint8 dealCount, uint16 startUser, uint8 validCardData, uint8 & cardIndex);

    // 放入出牌池中
    void    PushCardToPool(uint8 cardData[],uint8 cardCount);
    // 检测准备超时的从新排队
    void    CheckReadyTimeOut();

    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
	void    WriteErrorOutCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount);
    void    WriteWinType(uint16 chairID,uint8 winType);
	void    WriteBankerCardLog(uint8 cardData[], uint8 cardCount);
	void    WriteRobotID();
    void    CheckAddRobot();
    // AI 函数
	//游戏开始
	bool    OnSubGameStart();
	//庄家信息
	bool    OnSubBankerInfo();
	//用户出牌
	bool    OnSubOutCard(uint16 chairID);
    //机器人叫分
    bool    OnRobotCallScore();
    //机器人出牌
    bool    OnRobotOutCard();
    //是否需要重置思考时间
    bool    ReCalcRobotThinkTime();
    //设置机器人思考时间
    bool    OnSetRobotThinkTime(bool bRecalc = false);
	bool    ChairIsRobot(uint16 chairID);
	void RobotCardJudge(BYTE handCardData[],int count, std::vector<int> & argHandCards);
	void RobotCardCheck(BYTE cbCardData[], std::vector<int> argHandCards);
	
	void KickPlayerInTable();

	void	CheckPlayerScoreManyLeave();

    //游戏变量
protected:
    uint16  m_firstUser;                        // 首叫用户
    uint16  m_bankerUser;                       // 庄家用户
    uint16  m_curUser;                          // 当前用户
    uint8   m_outCardCount[GAME_LAND_PLAYER];   // 出牌次数
    uint8   m_sendHand[GAME_LAND_PLAYER];       // 是否看牌
	uint16  m_evaluateUser;                          // 

    // 炸弹信息
protected:
    uint8   m_bombCount;                        // 炸弹个数
    uint8   m_eachBombCount[GAME_LAND_PLAYER];  // 炸弹个数
    // 叫分信息
protected:
    uint8   m_callScore[GAME_LAND_PLAYER];      // 叫分数
    uint8   m_curCallScore;                     // 当前叫分
    uint8   m_pressCount[GAME_LAND_PLAYER];     // 压制次数
    uint8   m_bankrupts[GAME_LAND_PLAYER];      // 是否破产

    // 出牌信息
protected:
    uint16  m_turnWiner;                        // 胜利玩家
    uint8   m_turnCardCount;                    // 出牌数目
    uint8   m_turnCardData[MAX_LAND_COUNT];     // 出牌数据
	uint8   m_cbOutCardCount;                     // 出牌数目
	uint8   m_outCardData[FULL_POKER_COUNT];      // 出牌数据

    // 扑克信息
protected:
    uint8   m_bankerCard[3];                        // 游戏底牌
    uint8   m_handCardCount[GAME_LAND_PLAYER];      // 扑克数目
    uint8   m_handCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];   // 手上扑克

    uint8   m_allCardCount[GAME_LAND_PLAYER];      // 扑克数目
    uint8   m_allCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];   // 手上扑克
protected:
    CLandLogic      m_gameLogic;                    // 游戏逻辑
    uint8           m_randCard[FULL_POKER_COUNT];   // 洗牌数据
    vector<uint8>   m_outCards;                     // 已出的牌
    CCooling        m_coolRobot;                    // 机器人CD
    //CDDZAIManager   m_ddzAIRobot[GAME_LAND_PLAYER]; // 百度斗地主机器人

	//DDZRobotHttp	m_DDZRobotHttp[GAME_LAND_PLAYER];

	//COGLordRbtAIClv m_OGLordRbtAIClv[GAME_LAND_PLAYER];
	OGLordRobot m_LordRobot[GAME_LAND_PLAYER];
	// m_LordRobot[GAME_LAND_PLAYER];
	int m_argRobotLevel;
	

	//OGLordRobot m_AutoLordRobot[GAME_LAND_PLAYER];
	uint32                          m_robotBankerWinPro;					//做牌概率


	bool    SetControlCardData();
	bool	ProgressControlPalyer();
	bool	SetControlPalyerWin(uint32 control_uid);
	bool	SetControlPalyerLost(uint32 control_uid);

	//新注册玩家福利控制
	bool    NewRegisterWelfareCtrl();

	//幸运值控制
	bool    SetLuckyCtrl();

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

protected:
	uint16          m_isaddblockers;
	uint16			m_cbMemCardMac[GAME_LAND_PLAYER][FULL_POKER_COUNT];

	//bool			m_bGameEnd;
	//CCooling		m_coolKickPlayer;
};

#endif //SERVER_GAME_IMPLE_TABLE_H

