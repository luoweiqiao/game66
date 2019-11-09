//
// Created by toney on 16/6/28.
//
// 德州桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <unordered_map>
#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "texas_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_texas;

class CGamePlayer;
class CGameRoom;

//德州机器人决策数据
struct stTexasRobotAIInfo
{
    int64   minScore;               // 最小下注值
    int64   maxScore;               // 最大下注值
    int64   poolScore;              // 底池
    uint8   enemyShowhandNum;       // 敌人梭哈人数
    uint8   enemyNum;               // 敌人数量

    uint8   curGenre;               // 当前牌型
    uint8   pubGenre;               // 底牌牌型
    bool    bigPublic;              // 比底牌大
    uint16  maxChairID;             // 最大玩家
    uint16  maxRobotChairID;        // 最大机器人    
    
    uint8   balanceCount;           // 平衡次数
    uint16  chairID;                // 决策玩家
    bool    isMaxGenre;             // 是否最大牌型
    int32   winPro;                 // 胜率(与上家比)
    
    stTexasRobotAIInfo(){
        clear();
    }
    void clear(){
        memset(this,0,sizeof(stTexasRobotAIInfo));
    }
};

// 德州游戏桌子
class CGameTexasTable : public CGameTable
{
public:
    CGameTexasTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameTexasTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
    virtual bool    CanSitDown(CGamePlayer* pPlayer,uint16 chairID);
        
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

public:
    // 扣除开始台费
    virtual void    DeductStartFee();
    //配置桌子
    virtual bool Init();
    virtual void ShutDown();
	virtual bool ReAnalysisParam();

    //复位桌子
    virtual void ResetTable();
    virtual void OnTimeTick();
    //游戏消息
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
    //用户断线或重连
    virtual bool OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin = true);
	//用户坐下
	virtual bool OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer);
	//用户起立
	virtual bool OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer); 
public:
    // 游戏开始
    virtual bool OnGameStart();
    // 游戏结束
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
	// 发送座位信息给客户端 add by har
	virtual void SendSeatInfoToClient(CGamePlayer* pGamePlayer = NULL);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    int64    CalcPlayerInfo(uint16 chairID,int64 winScore);
    int64   GetSitDownScore();

    // 重置游戏数据
    void    ResetGameData();

	int     GetProCardType();

	bool	ProbabilityDispatchPokerCard();

    // 做牌发牌
	void	DispatchCard(uint16 wUserCount);
    
	bool	SetRobotMaxCardType();

	bool	SetRobotMinCardType();

	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

	bool	SetControlNoviceWelfarePalyerWin(uint32 control_uid);

	void    SetCardDataControl();

	// uid: 玩家id
	// uArSortIndex ： 玩家当前座位号
	// uSortLevel : 排序等级，0 最小 1 次小 ...
	bool	SetPlayerCardSortIndex(uint32 uid, uint8 uArSortIndex, uint8 uSortLevel);

	bool    SetWinMaxScorePlayerLost();

	bool	ProgressControlPalyer();

    // 活跃福利控制
    bool    ActiveWelfareCtrl();

	CGamePlayer* HaveWelfareNovicePlayer();

	bool	NoviceWelfareCtrlWinScore();

	// 幸运值控制
	bool SetLuckyCtrl();

	// 幸运值控制---设置输家的牌型
	bool SetLostForLuckyCtrl(uint32 control_uid);

protected:
	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
    // 写入公牌log
    void    WritePublicCardLog(uint8 cardData[],uint8 cardCount);
    // 写入下注log
    void    WriteAddScoreLog(uint16 chairID,int64 score,uint8 round);

    //游戏事件
protected:
	//放弃事件
	bool    OnUserGiveUp(WORD wChairID);
	//加注事件
	bool    OnUserAddScore(WORD wChairID, int64 lScore, bool bGiveUp);
    //带入
    bool    OnUserBuyin(CGamePlayer* pPlayer,int64 score);
	//下一局补充 add by har
	bool OnUserBuyinNext(CGamePlayer* pPlayer, int64 score);
    //亮牌
    bool    OnUserShowCard(WORD wChairID);
	//等待买入筹码的玩家站立 add by har
	void OnUserBuyinWaitStandUp(CGamePlayer* pPlayer);
    void    SendShowCardUser();

    int64   GetBuyinScore(uint32 uid);
    int64   ChangeBuyinScore(uint32 uid,int64& score);

    //没积分的玩家自动站起
    void    StandUpNotScore(); 
    
    //机器人操作
protected:
    void    OnRobotOper(uint16 chairID);
    bool    OnRobotOper0(uint16 chairID);
    bool    OnRobotOperEx(uint16 chairID);    
    bool    OnRobotOper5(uint16 chairID);
    bool    OnRobotGiveUp(uint16 chairID,int32 pro);
    bool    OnRobotAllIn(uint16 chairID);
    bool    OnRobotAddScore(uint16 chairID,uint8 level);
    bool    OnRobotAddScoreByFirstScore(uint16 chairID,uint32 score);

    void    CalcRobotAIInfo(uint16 chairID);
    //获得牌面胜率
    uint32  GetWinPro(uint16 chairID);

    
    bool    RobotBuyin(CGamePlayer* pRobot);
    void    CheckAddRobot();

    uint32  GetAllinPlayerNum();
    uint32  GetPlayNum();

    //是否有玩家在桌子
    bool    IsHavePlayerOnline();

	// 设置玩家座位信息 add by har
	void SetSeatInfo(net::seat_info *pSeat, CGamePlayer *pPlayer, int chairId);

	void InitSubWelfareHandCard();

	void	CheckPlayerScoreManyLeave();

protected:
	WORD							m_wDUser;								//D玩家
    WORD                            m_wMinChipinUser;                       //小盲
    WORD                            m_wMaxChipinUser;                       //大盲

	WORD							m_wCurrentUser;							//当前玩家

	//玩家状态
protected:
	BYTE							m_cbFirstEnterStatus[GAME_PLAYER];
	BYTE							m_cbHaveCard[GAME_PLAYER];
	BYTE							m_cbWaitTrun[GAME_PLAYER];
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//游戏状态
    BYTE                            m_szCardState[GAME_PLAYER];             //牌状态

	//加注信息
protected:
	int64						    m_lCellScore;							//单元下注
	int64						    m_lTurnLessScore;						//最小下注
	int64						    m_lAddLessScore;						//加最小注
	int64						    m_lTurnMaxScore;						//最大下注
	int64						    m_lBalanceScore;						//平衡下注
	WORD							m_wOperaCount;							//操作次数
	BYTE							m_cbBalanceCount;						//平衡次数
	int64						    m_lTableScore[GAME_PLAYER];				//桌面下注
	int64                           m_lRoundScore[GAME_PLAYER];
	int64						    m_lTotalScore[GAME_PLAYER];				//累计下注
	int64						    m_lUserMaxScore[GAME_PLAYER];			//最大下注
	BYTE							m_cbShowHand[GAME_PLAYER];				//梭哈用户
    uint8                           m_szShowCard[GAME_PLAYER];              //是否亮牌

	//扑克信息
protected:
	BYTE							m_cbSendCardCount;						        //发牌数目
	BYTE							m_cbCenterCardData[MAX_CENTERCOUNT];	        //中心扑克
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_COUNT];       //手上扑克

    BYTE                            m_cbEndCardType[GAME_PLAYER];                   //最终牌型
    BYTE							m_cbMaxCardData[GAME_PLAYER][MAX_CARD_COUNT];   //最大牌型
    int32                           m_cbWinPro[GAME_PLAYER];                        // 模拟胜率  
    
	std::map<BYTE, std::vector<BYTE>>	m_mpSubWelfareHandCard;
	//组件变量
protected:
	CTexasLogic					m_GameLogic;							//游戏逻辑
	CCooling                    m_coolRobot;                            //机器人CD
    map<uint32,int64>           m_mpBuyinScore;                         //申请带入积分
    stTexasRobotAIInfo          m_robotAIInfo;                          //机器人AI信息
	bool						m_bIsControlPlayer;
	uint32						m_robotBankerMaxCardPro;				//机器人最大牌概率
	int							m_iArrDispatchCardPro[Texas_Pro_Index_MAX];	//发牌型概率
	// add by har
	int                         m_confArrDispatchCardAllPro;            // 最大牌型总概率
	unordered_map<uint32, int64>  m_mpNextBuyinScore;                   // uid->下一轮要补充的积分,0表示自动补齐至最大
	unordered_map<uint32, uint32> m_mpWaitBuyinScore;                   // 等待购买筹码状态 uid->座位号
	// add by har end
};

#endif //SERVER_GAME_IMPLE_TABLE_H

