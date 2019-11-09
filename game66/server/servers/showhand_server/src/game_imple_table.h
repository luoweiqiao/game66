//
// Created by toney on 16/4/6.
//
// 梭哈的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "show_hand_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_show_hand;

class CGamePlayer;
class CGameRoom;

#define TABLE_ROBOT_COUNT_SHOWHAND	1
//组件属性

//结束原因
#define GER_NO_PLAYER				0x10								//没有玩家

//梭哈机器人决策数据
struct stRobotAIInfo
{
    int64   minScore;           // 最小下注值
    int64   maxScore;           // 最大下注值
    int64   smallScore;         // 轻注
    int64   manyScore;          // 重注
    bool    enemyShowhand;      // 敌人是否梭哈

    bool    isDeskWin;          // 是否桌面最大
    uint16  isHandWin;          // 自己手牌组合是否赢桌面
    uint8   selfGenre;          // 自己牌型
    uint8   selfGenreValue;     // 自己牌型值
    uint8   enemyGenre;         // 敌人桌面牌型
    uint8   enemyMaxDeskValue;  // 桌面最大牌值
    uint8   selfEndGenre;       // 自己最终牌型
    bool    isEndWin;           // 是否最终赢家
    uint8   sendCard;           // 第几张牌
    uint16  chairID;            // 决策玩家
    uint16  winPro;             // 胜率

    stRobotAIInfo(){
        clear();
    }
    void clear(){
        memset(this,0,sizeof(stRobotAIInfo));
    }
};

#define DISPATCH_TYPE_Room		0
#define DISPATCH_TYPE_Welfare	1


// 梭哈游戏桌子
class CGameShowHandTable : public CGameTable
{
public:
    CGameShowHandTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameShowHandTable();

// 重载基类函数
public:
    virtual bool    IsAllReady();
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);
	virtual bool    CanEnterTable(CGamePlayer* pPlayer);
public:
    //配置桌子
    virtual bool Init();
	virtual bool ReAnalysisParam();
    virtual void ShutDown();

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

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    int64    CalcPlayerInfo(uint16 chairID,int64 winScore);
    // 重置游戏数据
    void    ResetGameData();
    // 做牌发牌
	void	DispatchCard();
	int     GetProCardType();

	bool	ProbabilityDispatchPokerCard(int type);

	CGamePlayer* HaveWelfareNovicePlayer();

	void	InitHaveWinPreHandCard();

	bool	NoviceWelfareCtrlWinScore();

	bool	SetControlNoviceWelfarePalyerWin(uint32 control_uid);

protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
    // 写入加注log
    void    WriteAddScoreLog(uint16 chairID,int64 score);

	void    WriteAddCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount, uint8 giveup);

	void	WriteJackpotScoreInfo();

    //游戏事件
protected:
    //超时操作
    void OnTimeOutOper();
    //用户放弃
    bool OnUserGiveUp(WORD wChairID);
    //用户加注
    bool OnUserAddScore(WORD wChairID, int64 lScore);

    //机器人平跟或下注
    bool OnRobotCallSmallScore(uint16 chairID);
    //机器人平跟大注
    bool OnRobotCallManyScore(uint16 chairID);
    //机器人AllIn
    bool OnRobotAllinScore(uint16 chairID);
    //机器人概率弃牌
    bool OnRobotGaiLvGiveUp(uint16 chairID,uint32 pro);

    //辅助函数
protected:
    //调整下注
    void RectifyMaxScore();
    //发送扑克
    bool DispatchUserCard();
    //逻辑辅助
protected:
    //推断输者
    WORD EstimateLoser(BYTE cbStartPos, BYTE cbConcludePos);
    //推断胜者
    WORD EstimateWinner(BYTE cbStartPos, BYTE cbConcludePos);

    WORD EstimateWinnerEx(BYTE cbStartPos, BYTE cbConcludePos);
    //获得最终胜利者
    WORD GetEndWiner();
    WORD GetEnemyChairID(uint16 chairID);

    //判断手牌是否最大
    bool IsHandWiner(uint16 chairID);
    //获得其它人的最大牌面
    BYTE GetOtherCardGenre(uint16 chairID);
    BYTE GetOtherMaxDeskCardValue(uint16 chairID);
    BYTE GetSelfCardGenre(uint16 chairID);
    BYTE GetSelfCardGenreValue(uint16 chairID);

    bool IsOtherShowhand(uint16 chairID);
    //获得最终牌型
    uint8 GetEndGenre(uint16 chairID);
    bool IsCardA(uint8 cardValue);
    bool IsCardQ(uint8 cardValue);

    //机器人操作
protected:
    void OnRobotOper(uint16 chairID);
    //2张牌策略
    bool OnRobotOper2(uint16 chairID);
    //3张牌策略
    bool OnRobotOperEx(uint16 chairID);
    bool OnRobotOperEx5(uint16 chairID);

    void CheckAddRobot();
    void SetRobotThinkTime();

    //修改剩下的牌
    void ChangeLeftCard(uint16 chairID,uint32 pro,uint16 cardNum);
    
	bool	ProgressControlPalyer();

    //活跃福利控制
    bool    ActiveWelfareCtrl();

	//新注册玩家福利控制
	bool    NewRegisterWelfareCtrl();

	bool    SetNRWControlPlayerWin(uint32 control_uid);

	//幸运值控制
	bool    SetLuckyCtrl();

// 控制牌
	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

	bool    SetControlPalyerLost_card_type(uint32 control_uid);


	bool    SetWinMaxScorePlayerLost();

	bool    SetRobotWin();

	bool    SetRobotLostScore();
	bool    SetRobotWinScore();
	bool    SetRobotWinScore_card_type();

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

	int     GetWelfareCardType();

	void	CheckPlayerScoreManyLeave();

    //游戏变量
protected:
    bool							m_bShowHand;						//梭哈标志
    WORD							m_wCurrentUser;						//当前用户

    //下注信息
protected:
    int64						    m_lDrawMaxScore;					//最大下注
    int64						    m_lTurnMaxScore;					//最大下注
    int64	    					m_lTurnLessScore;					//最小下注
    int64   						m_lDrawCellScore;					//底注积分

    //用户状态
protected:
    BYTE							m_cbPlayStatus[GAME_PLAYER];		//游戏状态
    BYTE							m_cbOperaScore[GAME_PLAYER];		//操作标志

    //金币信息
protected:
    int64	    					m_lUserScore[GAME_PLAYER];			//用户下注
    int64   						m_lTableScore[GAME_PLAYER];			//桌面下注
    int64   						m_lUserMaxScore[GAME_PLAYER];		//最大下注
    int64                           m_lHistoryScore[GAME_PLAYER];       //历史输赢

    //扑克变量
protected:
    BYTE							m_cbSendCardCount;					//发牌数目
    BYTE							m_cbCardCount[GAME_PLAYER];			//扑克数目
    BYTE							m_cbHandCardData[GAME_PLAYER][5];	//桌面扑克
protected:
    CShowHandLogic                  m_gameLogic;                        //游戏逻辑
    CCooling                        m_coolRobot;                        //机器人CD
    stRobotAIInfo                   m_robotAIInfo;                      //机器人AI信息
	bool							m_bIsControlPlayer;
    bool                            m_isOnlyRobot;                      //是否纯机器人局
    bool                            m_needChangeCard;                   //需要换牌
    bool                            m_makeType5;
    bool                            m_makeType3;
	uint32                          m_robotBankerWinPro;                //做牌概率
	uint32                          m_robotFrontFourWinPro;             //做牌概率 前四张对手梭哈后若机器人赢跟注概率
	uint32                          m_robotFrontFourLostPro;            //做牌概率 前四张对手梭哈后若机器人输跟注概率


	int								m_iArrDispatchCardPro[ShowHand_Pro_Index_MAX];	//发牌型概率

	int								m_iArrWelfareCardPro[ShowHand_Pro_Index_MAX];	//发牌型概率

	// 同花顺、铁枝、葫芦、同花、顺子 5种牌型概率
	int								m_iArrNRWCardPro[ShowHand_Pro_Index_MAX];	//新注册玩家福利牌型概率

	std::map<BYTE, std::vector<BYTE>>	m_mpWinPreWelfareHandCard;



};

#endif //SERVER_GAME_IMPLE_TABLE_H

