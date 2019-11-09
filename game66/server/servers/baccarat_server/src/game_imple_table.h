//
// Created by toney on 16/6/28.
//
// 百人牛牛的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "poker/baccarat_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_baccarat;

class CGamePlayer;
class CGameRoom;

//组件属性
#define GAME_PLAYER					8									//座位人数
#define MAX_SEAT_INDEX              2                                   //最大牌位
#define CARD_NUM                    3
#define INDEX_PLAYER				0									//闲家索引
#define INDEX_BANKER				1									//庄家索引

#define SUPER_SIX_RATE				(0.5)								//supersix
#define SUPER_SIX_NUMBER			(6)									//supersix



//玩家索引
enum emAREA
{
    AREA_XIAN = 0,													//闲家索引
    AREA_PING,														//平家索引
    AREA_ZHUANG,													//庄家索引
    AREA_XIAN_DUI,													//闲对子
    AREA_ZHUANG_DUI,												//庄对子
	AREA_SUPSIX,													//6
	AREA_SMALL,														//小
	AREA_BIG,														//大
    AREA_MAX,														//最大区域
};

#define	MAX_BACCARAT_GAME_RECORD	(10)

//记录信息
struct baccaratGameRecord
{
    BYTE							bPlayerTwoPair;						//对子标识
    BYTE							bBankerTwoPair;						//对子标识
    BYTE							cbPlayerCount;						//闲家点数
    BYTE							cbBankerCount;						//庄家点数
	BYTE							cbIsSmall;							//是否小
	BYTE							cbIsSuperSix;
	int								index;
    baccaratGameRecord(){
        memset(this,0,sizeof(baccaratGameRecord));
    }
};

struct tagRobotPlaceJetton {
	bool bflag;
	CGamePlayer* pPlayer;
	uint8 area;
	int64 time;
	int64 jetton;
	uint32 uid;
	tagRobotPlaceJetton() {
		bflag = false;
		pPlayer = NULL;
		area = AREA_MAX;
		time = 0;
		jetton = 0;
		uid = 0;
	}
};

// 百家乐游戏桌子
class CGameBaccaratTable : public CGameTable
{
public:
    CGameBaccaratTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameBaccaratTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
    virtual bool    CanSitDown(CGamePlayer* pPlayer,uint16 chairID);
    virtual bool    CanStandUp(CGamePlayer* pPlayer); 
    
    virtual bool    IsFullTable();
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);
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
	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
	int64    CalcPlayerInfo(uint32 uid,int64 winScore,int64 OnlywinScore,bool isBanker = false);
    // 重置游戏数据
    void    ResetGameData();
    void    ClearTurnGameData();
    
protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip);
    // 写入加注log
    void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score,int64 win);
    // 写入庄家信息
    void    WriteBankerInfo();

    
    //游戏事件
protected:
	//加注事件
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore);
	//申请庄家
	bool OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore);
    bool OnUserJumpApplyQueue(CGamePlayer* pPlayer);
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg);

	//取消申请
	bool OnUserCancelBanker(CGamePlayer* pPlayer);
    
protected:
	bool  DispatchRandTableCard();
	//发送扑克
	bool DispatchTableCard();
	//发送扑克
	bool  DispatchTableCardBrankerIsRobot();
	//真实玩家当庄发牌
	bool  DispatchTableCardBrankerIsPlayer(uint32  playerBankerLosePro);
	
	bool  DispatchTableCardControlPalyerWin(uint32 control_uid);

	bool  DispatchTableCardControlPalyerLost(uint32 control_uid);

	bool    SetBrankerWin();

	bool    SetBrankerLost();

	bool    SetLeisurePlayerWin();

	bool    SetLeisurePlayerLost();

	// 福利控制
	bool DosWelfareCtrl();
	// 非福利控制
	bool NotWelfareCtrl();
	//bool  KilledPlayerCtrl();

    // 活跃福利控制
    bool ActiveWelfareCtrl();

    // 控制活跃福利玩家赢
    bool  ControlActiveWelfarePalyerWin(uint32 control_uid, int64 max_win);

	//发送庄家
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
	//更换庄家
	bool  ChangeBanker(bool bCancelCurrentBanker);
	//轮换判断
	void  TakeTurns();
    //结算庄家
    void  CalcBankerScore();  
    //自动补币
    void  AutoAddBankerScore();
    //发送游戏记录
    void  SendPlayLog(CGamePlayer* pPlayer);

	// 设置库存输赢  add by har
	// return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();
	// 获取某个玩家的赢分  add by har
	// pPlayer : 玩家
	// cbWinArea : 赢的区域
	// cbBankerCount : 庄家点数
	// lBankerWinScore : 庄家赢分
	// return 非机器人玩家赢分
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, uint8 cbWinArea[AREA_MAX], uint8 cbBankerCount, int64 &lBankerWinScore);


	//下注计算
private:
	//最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea);
	uint8   GetRobotJettonArea(CGamePlayer* pPlayer);

    //庄家站起
    void    StandUpBankerSeat(CGamePlayer* pPlayer);
    bool    IsSetJetton(uint32 uid);
    bool    IsInApplyList(uint32 uid);
    
	//游戏统计
private:
	//计算得分
	int64   CalculateScore();
	//推断赢家
	// return 庄家点数 add by har
	uint8 DeduceWinner(BYTE* pWinArea);
    
    
    //申请条件
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //次数限制
    int32   GetBankerTimeLimit();
    
    //机器人操作
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer,uint8 area);
    void    OnRobotOper();
	void    OnChairRobotOper();
    void    OnRobotStandUp();
    
    void    CheckRobotApplyBanker();

    void    AddPlayerToBlingLog();
	//void	OnRobotPlaceJetton();
	//void OnChairRobotPlaceJetton();
	//bool	PushRobotPlaceJetton(tagRobotPlaceJetton &robotPlaceJetton);
	//初始化洗牌
	void    InitRandCard();
	//牌池洗牌
	void 	RandPoolCard();
	//摸一张牌
	BYTE    PopCardFromPool();
	//做牌局数
	int		m_imake_card_count;
	//不牌局数
	int		m_inotmake_card_count;
	//已经几局没做牌
	int		m_inotmake_card_round;
	//最大做牌数
	int		m_imax_notmake_round;

	int64   m_lGameRount;
    //游戏变量
protected:
    //总下注数
protected:
	int64						    m_allJettonScore[AREA_MAX];		                //全体总注
	int64							m_playerJettonScore[AREA_MAX];					//玩家总注
    map<uint32,int64>               m_userJettonScore[AREA_MAX];                    //个人总注
    map<uint32,int64>			    m_mpUserWinScore;			                    //玩家成绩

	map<uint32, int64>			    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水
	int64							m_curr_banker_win;								//当前庄家的赢取金额---只统计赢不包括输，用于计算抽水
	//扑克信息
protected:


    //扑克信息
protected:
    BYTE							m_cbCardCount[2];						//扑克数目
    BYTE							m_cbTableCardArray[2][3];				//桌面扑克
    BYTE                            m_cbTableCardType[MAX_SEAT_INDEX];      //桌面牌型
    BYTE                            m_cbWinArea[AREA_MAX];

	vector<BYTE>                    m_poolCards;                            //牌池扑克

	//庄家信息
protected:	
    //vector<CGamePlayer*>			m_ApplyUserArray;						//申请玩家
    //map<uint32,uint8>               m_mpApplyUserInfo;                      //是否自动补币
    
    //map<uint32,int64>               m_ApplyUserScore;                       //申请带入积分
    
	//CGamePlayer*					m_pCurBanker;						    //当前庄家
    uint8                           m_bankerAutoAddScore;                   //自动补币
    //bool                            m_needLeaveBanker;                      //离开庄位
        
    uint16							m_wBankerTime;							//做庄次数
    uint16                          m_wBankerWinTime;                       //胜利次数
	int64						    m_lBankerScore;							//庄家积分
	int64						    m_lBankerWinScore;						//累计成绩

	int64							m_lBankerShowScore;						//庄家显示分数
    int64                           m_lBankerBuyinScore;                    //庄家带入
    int64                           m_lBankerInitBuyinScore;                //庄家初始带入
    int64                           m_lBankerWinMaxScore;                   //庄家最大输赢
    int64                           m_lBankerWinMinScore;                   //庄家最惨输赢
	uint16							m_cbBrankerSettleAccountsType;			//庄家结算类型

    
	//控制变量
protected:
	uint32                          m_playerBankerLosePro;                    //真实玩家庄概率
	
	//int64							m_lMaxPollScore;						  //最大奖池分数
	//int64							m_lMinPollScore;						  //最小奖池分数
	//int64							m_lCurPollScore;						  //当前奖池分数
	//int64							m_lFrontPollScore;						  //上一轮奖池变化一次
	//uint32							m_uSysWinPro;							  //系统赢概率
	//uint32							m_uSysLostPro;							  //系统输概率

	//组件变量
protected:
	CBaccaratLogic					m_GameLogic;							//游戏逻辑
    uint32                          m_BankerTimeLimit;                      //庄家次数


    uint32                          m_robotApplySize;                       //机器人申请人数
    uint32                          m_robotChairSize;                       //机器人座位数
	//vector<tagRobotPlaceJetton>		m_robotPlaceJetton;						//下注的机器人
	//vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//下注的机器人
	//bool m_bIsChairRobotAlreadyJetton;
	CCooling						m_coolRobotJetton;						//机器人下注CD
    baccaratGameRecord              m_record;
    vector<baccaratGameRecord>		m_vecRecord;	                        //游戏记录
	vector<baccaratGameRecord>		m_vecGamePlayRecord;	                //游戏记录

	bool IsInTableRobot(uint32 uid, CGamePlayer * pPlayer);

	bool OnChairRobotJetton();
	void OnChairRobotPlaceJetton();
	bool							m_bIsChairRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//下注的机器人

																			//
	bool OnRobotJetton();
	void OnRobotPlaceJetton();
	bool							m_bIsRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_RobotPlaceJetton;				//下注的机器人

	uint8 m_cbJettonArea;

public:
	void OnRobotTick();

//百人场精准控制
public:
	void   OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer);			//发送在线所有真实玩家下注详情
	void   OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer);	//通知单个玩家信息
	void   OnBrcControlSendAllRobotTotalBetInfo();						//发送所有机器人总下注信息
	void   OnBrcControlSendAllPlayerTotalBetInfo();						//发送所有真实玩家总下注信息
	bool   OnBrcControlEnterControlInterface(CGamePlayer* pPlayer);		//进入控制界面
	void   OnBrcControlBetDeal(CGamePlayer* pPlayer);					//下注处理
	bool   OnBrcAreaControl();											//百人场区域控制
	void   OnBrcFlushSendAllPlayerInfo();								//刷新在线所有真实玩家信息---玩家进入/退出桌子时调用

	//当申请上庄玩家被强制下庄后，需要通知该上庄玩家
	void  OnNotityForceApplyUser(CGamePlayer* pPlayer);
};

#endif //SERVER_GAME_IMPLE_TABLE_H

