//
// Created by toney on 16/4/6.
//
// 扎金花的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "zajinhua_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_zajinhua;

class CGamePlayer;
class CGameRoom;

enum emCARD_STATE
{
    emCARD_STATE_NULL = 0,// 暗
    emCARD_STATE_MING,    // 明  
    emCARD_STATE_QI,      // 弃  
    emCARD_STATE_SHU,     // 输
    emCARD_STATE_ALLIN,   // allin

};

enum emPLAYER_IS_ROBOT
{
	emPLAYER_IS_USER = 0,
	emPLAYER_IS_ROBOT = 1,
	emPLAYER_IS_NOT_READY = 2,
};

#define TABLE_ROBOT_COUNT_ZAJINHUA		2	//扎金花每桌机器人限制2个

#define DISPATCH_TYPE_Room		0
#define DISPATCH_TYPE_Welfare	1


// 扎金花游戏桌子
class CGameZajinhuaTable : public CGameTable
{
public:
    CGameZajinhuaTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameZajinhuaTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
	virtual bool    LeaveTable(CGamePlayer* pPlayer, bool bNotify = false); // add by har

    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

public:
    //配置桌子
    virtual bool Init();
	virtual bool ReAnalysisParam();
    virtual void ShutDown();

    //结束游戏，复位桌子
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
	virtual bool    IsReady(CGamePlayer* pPlayer); // add by har
    
    int64    CalcPlayerInfo(uint16 chairID,int64 winScore);
    // 重置游戏数据
    void    ResetGameData();

private:
	// 将资金不满足条件的玩家踢出 add by har
	void CheckPlayerScoreLeave();
	// 将挂机玩家踢出 add by har
	void CheckNoOperPlayerLeave();
	// 每局结束重新设置未操作状态计数 add by har
	void ResetGameEndNoOper();
	// 发送加注消息给客户端 add by har
    // addScoreUser : 下注的用户
    // addScore : 下注的数量
    // addUserIsCompare : 下注的用户是否比牌
	void SendMsgAddScore(uint16 addScoreUser, int64 addScore, bool addUserIsCompare);
	// 设置比牌胜利用户和失败用户 add by har
	void SetCompareCardWinLoseUser(uint16 wFirstChairID, uint16 wNextChairID, uint16 &wWinUser, uint16 &wLostUser);
	// 比牌结束处理 add by har
	void CompareCardEnd(uint16 wFirstChairID, uint16 wNextChairID, uint16 wWinUser, uint16 wLostUser, int64 compareScore = 0);
	// 强制比牌 add by har
	void ForeCompareCard();
	// 获取下一个玩家座位 add by har
	uint16 GetNextPlayerChair();

protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
    // 写入加注log
    void    WriteAddScoreLog(uint16 chairID,int64 score);
    // 写入比牌log
    void    WriteCompare(uint16 chairID1,uint16 chairID2,uint16 winID,int64 score);

	void	WriteJackpotScoreInfo();

    // 是否能够开始游戏
    bool    IsCanStartGame();
    // 检测筹码是否正确
    bool    CheckJetton(uint16 chairID,int64 score);
    // 计算比牌筹码
    int64   CompareJetton(uint16 chairID);
    int64   CallJetton(uint16 chairID);
    int64   AllinJetton(uint16 chairID);



    // 是否只剩比牌操作
    bool    IsOnlyCompare(uint16 chairID);
    // 是否强制开牌
    bool    CheckOpenCard();
    void    CheckNewRound(uint16 chairID);

    uint16  GetCompareRound();
    uint16  GetLookRound();
    uint16  GetMaxRound();   
    uint16  GetPlayNum();
	
	

	// 做牌发牌
	void	DispatchCard();
	// 根据座位准备人数概率发牌
	bool	ProbabilityDispatchPokerCard(int type);
	int     GetProCardType();
	int     GetWelfareCardType();

	bool	SetRobotMaxCard();

	CGamePlayer* HaveWelfareNovicePlayer();

	bool	NoviceWelfareCtrlWinScore();

    //游戏事件
protected:
	//游戏状态
	bool IsUserPlaying(WORD wChairID);
    //用户放弃
    bool OnUserGiveUp(WORD wChairID);
	//看牌事件
	bool OnUserLookCard(WORD wChairID);
	//比牌事件
	bool OnUserCompareCard(WORD wFirstChairID,WORD wNextChairID);
    //比牌结束
    bool OnOverCompareCard();
    //亮牌
    bool OnUserShowCard(WORD wChairID);
    void SendShowCardUser();

	//开牌事件
	bool OnUserOpenCard(WORD wUserID);
	//加注事件
    bool OnUserAddScore(WORD wChairID, int64 lScore, bool bGiveUp, bool bCompareCard);
    //全压
    bool OnUserAllin(WORD chairID);

	uint16 GetNextUser(WORD chairID);

	uint16 GetFrontUser(WORD chairID);

    //机器人操作
protected:
    bool OnRobotOper(uint16 chairID);
    bool OnRobotOper_1(uint16 chairID);

    bool OnRobotCompare(uint16 chairID);
    bool IsMaxCard(uint16 chairID);

    void CheckAddRobot();
    void SetRobotThinkTime();

	uint16 GetMaxCardChair(uint16 chairID);

	uint16 GetMinCardChair(uint16 chairID);

	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

	bool	SetRobotWinScore();

	bool	SetPalyerWinScore();

    //活跃福利控制
    bool    ActiveWelfareCtrl();

	//新注册玩家福利控制
	bool    NewRegisterWelfareCtrl();

	bool    SetNRWControlPalyerWin(uint32 control_uid);

	//幸运值控制
	bool    SetLuckyCtrl();
	
	// 幸运值控制---设置输家的牌型
	bool SetLostForLuckyCtrl(uint32 control_uid);

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

	//机器人 ai 函数

	bool	IsCanLookCard();

	bool	IsCanCompareCard(WORD wChairID);

	bool	IsRobotCanLookCard();

	bool	IsRobotCanCompareCard();

	bool	HaveRealPalyer(uint16 chairID);

	uint16  GetRealPlayerCount();

	uint16	RealPalyerCount(uint16 chairID);

	bool	HavePlayerLookCard(uint16 chairID);

	bool	HavePlayerNoLookCard(uint16 chairID);

	bool	HavePalyerWinScore(uint16 chairID);

	bool	HavePalyerNoWinScore(uint16 chairID);

	bool	HavePalyerBigger(uint16 chairID);

	bool	HavePalyerBiggerWinScore(uint16 chairID);

	bool	HavePalyerBiggerNoWinScore(uint16 chairID);

	bool	HavePalyerAllIn(uint16 chairID);

	bool	OnRobotCompareRand(uint16 chairID);

	bool	OnRobotCompareRobot(uint16 chairID);

	bool	OnRobotComparePlayer(uint16 chairID);

	int64   RobotCallJetton(uint16 chairID);

	int64   RobotRaiseJetton(uint16 chairID);

	bool	OnRobotAllIn(uint16 chairID);

	bool	OnRobotJettonScore(uint16 chairID, bool bIsAddScore);
	
	bool	OnRobotFoldFrontLook(uint16 chairID);

	bool	OnRobotNoEnoughScoreCompareCard(uint16 chairID);

	//机器人单牌出牌
	bool	RobotSingleOperCard(uint16 chairID);
	//机器人对子出牌
	bool	RobotDoubleOperCard(uint16 chairID);

	bool	RobotShunZiOperCard(uint16 chairID);

	bool	RobotJinHuaOperCard(uint16 chairID);

	bool	RobotShunJinBaoZiOperCard(uint16 chairID);


	//游戏变量
protected:
	BYTE							m_bOperaCount;							//操作次数
	WORD							m_wCurrentUser;							//当前用户
	WORD							m_wBankerUser;							//庄家用户
    uint16                          m_wCurrentRound;                        //当前轮次
    uint16                          m_wWinCompareUser;

	//用户状态
protected:
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//游戏状态
	BYTE							m_cbPlayIsRobot[GAME_PLAYER];			//玩家是否是机器人
    BYTE                            m_szJoinGame[GAME_PLAYER];              //是否参与游戏
	bool							m_bGameEnd;								//结束状态
    bool                            m_bAllinState;                          //allin状态

private:
	uint8                           m_szNoOperCount[GAME_PLAYER];           //无操作状态计数 add by har
	uint8                           m_szNoOperTrun[GAME_PLAYER];            //无操作状态局计数 add by har
	uint8                           m_szGameEndStatus[GAME_PLAYER];         //游戏结算期间是否必须留在房间里 add by har

	//扑克变量
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_COUNT];//桌面扑克
	vector<WORD>					m_wCompardUser[GAME_PLAYER];			 
	// 散牌	对子	顺子	同花	同花顺	豹子 6种牌型概率
	int								m_iArrDispatchCardPro[Pro_Index_MAX];	//发牌型概率
	int								m_iArrWelfareCardPro[Pro_Index_MAX];	//发牌型概率

	//下注信息
protected:
	//WORD							m_wCanLookUser;							//是否能看牌以这个用户下注次数为标准
	//bool							m_bIsCanLookCard;						//是否能看牌
	//uint16						m_uAddScoreCount[GAME_PLAYER];			//下注次数
	int64                           m_lAllinScore[GAME_PLAYER];             //allin下注
    int64						    m_lTableScore[GAME_PLAYER];				//下注数目
	int64                           m_lUserMaxScore[GAME_PLAYER];			//最大下注
	int64						    m_lCellScore;							//单元下注
	uint32						    m_lCurrentTimes;						//当前倍数
	uint8							m_szCardState[GAME_PLAYER];				//看明下注
    uint8                           m_szShowCard[GAME_PLAYER];              //是否亮牌
    uint8                           m_szMingPai[GAME_PLAYER];               //是否明牌
    uint8                           m_szMingPaiRound[GAME_PLAYER];          //明牌轮数
	uint8                           m_szCompareWinUser[GAME_PLAYER];        //明牌轮数

    uint16                          m_cmpRound;     //比牌轮数
    uint16                          m_lookRound;    //看牌轮数
    uint16                          m_maxRound;     //最大轮数
    
protected:
    CZajinhuaLogic                  m_gameLogic;                            //游戏逻辑
    CCooling                        m_coolRobot;                            //机器人CD
	uint32                          m_robotBankerWinPro;					//做牌概率

	// 散牌	对子	顺子	同花	同花顺	豹子 6种牌型概率
	int								m_iArrNRWCardPro[Pro_Index_MAX];	//新注册玩家福利牌型概率
	
};

#endif //SERVER_GAME_IMPLE_TABLE_H

