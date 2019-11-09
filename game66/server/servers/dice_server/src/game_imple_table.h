
#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "dice_logic.h"
#include "db_struct_define.h"

using namespace svrlib;
using namespace std;
using namespace game_dice;

class CGamePlayer;
class CGameRoom;

//记录信息

struct tagDiceGameRecord
{
	BYTE  cbBigSmall;                       //大小
    BYTE  cbSumPoints;                      //点数和
	BYTE  cbIsWaidice;						//是否豹子
	BYTE  cbDiceRecord[DICE_COUNT];         //点数	
    tagDiceGameRecord(){
		Init();
    }
	void Init()
	{
		cbBigSmall = 0;
		cbSumPoints = 0;
		cbIsWaidice = 0;
		for (uint32 i = 0; i < DICE_COUNT; i++)
		{
			cbDiceRecord[i] = 0;
		}
	}
};


struct tag_dice_area_info
{
	uint32 	jetton_area;		// 下注区域
	uint32 	jetton_multiple;	// 下注赔率
	int64 	jetton_score;		// 下注分数
	int64 	final_score;		// 最终分数
	tag_dice_area_info()
	{
		Init();
	}
	void Init()
	{
		jetton_area = 0;
		jetton_multiple = 0;
		jetton_score = 0;
		final_score = 0;
	}
};

struct tagRobotPlaceJetton
{
	uint32 uid;
	bool bflag;
	CGamePlayer* pPlayer;
	uint8 area;
	uint32 time;
	int64 jetton;
	tagRobotPlaceJetton()
	{
		uid = 0;
		bflag = false;
		pPlayer = NULL;
		area = 0;
		time = 0;
		jetton = 0;
	}
};

struct tagRobotJettonCountArea
{
	uint8 area;
	int count;
	tagRobotJettonCountArea()
	{
		area = 0;
		count = 0;
	}
};


struct tagDiceJackpotInfo
{
	int64	lCurPollScore;			//当前奖池分数
	uint32	uPoolAddRatio;			//奖池增加比率
	uint32	uPoolSubPoint;			//奖池指定点数分配比例
	uint32	uPoolSubOneFive;		//奖池1-5围骰分配比例
	uint32	uPoolSubSixRatio;		//奖池6围骰分配比例

	int64	lWinTotalScore;			//玩家赢金币的数量
	uint64	utime;					//开奖时间
	BYTE	cbTableDice[DICE_COUNT];//开奖这个局的骰子

	uint32	uRobotSubProbability;	//机器人获取奖池概率
	int64	lMaxPollScore;			//最大奖池分数
	
	tagDiceJackpotInfo()
	{
		Init();
	}

	void Init()
	{
		lCurPollScore = 0;
		uPoolAddRatio = 200;
		uPoolSubPoint = 1000;
		uPoolSubOneFive = 3000;
		uPoolSubSixRatio = 5000;

		lWinTotalScore = 0;
		utime = 0;
		for (uint32 i = 0; i < DICE_COUNT; i++)
		{
			cbTableDice[i] = 0;
		}

		uRobotSubProbability = 2000;
		lMaxPollScore = 5000;
	}

	void UpdateCurPoolScore(int64 lScore)
	{
		__sync_add_and_fetch(&lCurPollScore, lScore);
	}
	void SetCurPoolScore(int64 lScore)
	{
		int64 lPoolScore = -lCurPollScore;
		__sync_add_and_fetch(&lCurPollScore, lPoolScore);
		__sync_add_and_fetch(&lCurPollScore, lScore);
	}
};

enum emDICE_JACKPOT_INFO_POS
{
	DICE_JACKPOT_INFO_POS_front = 0,
	DICE_JACKPOT_INFO_POS_add,
	DICE_JACKPOT_INFO_POS_sub,
};

struct tagUserJackpotInfo
{
	uint8 area;
	uint32 ratio;
	int64 pscore;
	int64 wscore;
	tagUserJackpotInfo()
	{
		area = 0;
		ratio = 0;
		pscore = 0;
		wscore = 0;
	}
};

class CGameDiceTable : public CGameTable
{
public:
	CGameDiceTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameDiceTable();

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
	//bool LoadGameDataWeekRank();
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

	virtual bool DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[]);

public:
    //游戏开始
    virtual bool OnGameStart();
    //游戏结束
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    
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
	void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score);
	// 写入牌型
	void 	WriteCardType();
	// 写入庄家信息
	void    WriteBankerInfo();
	void	WriteUserJackpotInfo();
	void    WriteJackpotInfo(uint16 pos);
    //游戏事件
protected:
	//加注事件
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore);
	//用户取消下注
	bool OnUserCancelJetton(CGamePlayer* pPlayer);
	//申请庄家
	bool OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore);
    bool OnUserJumpApplyQueue(CGamePlayer* pPlayer);
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg);

	//取消申请
	bool OnUserCancelBanker(CGamePlayer* pPlayer);
    
protected:
    //发送扑克
	bool  DispatchTableCard();
	//bool KilledPlayerCtrl();
	bool NotWelfareCtrl();

	//发送庄家
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
	//排序庄家
	void  FlushApplyUserSort();
	//更换庄家
	bool  ChangeBanker(bool bCancelCurrentBanker);
	//轮换判断
	void  TakeTurns();
    //结算庄家
    void  CalcBankerScore();  
    //自动补币
    void  AutoAddBankerScore();
	void  SendPlayLog(CGamePlayer* pPlayer);

	CGamePlayer * GetPlayerByUid(uint32 uid);

	//下注计算
private:
	//最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea,int64 lJettonScore);
    uint32  GetBankerUID();
    void    RemoveApplyBanker(uint32 uid);
    bool    LockApplyScore(CGamePlayer* pPlayer,int64 score);
    bool    UnLockApplyScore(CGamePlayer* pPlayer);
    //庄家站起
    void    StandUpBankerSeat(CGamePlayer* pPlayer);
    bool    IsSetJetton(uint32 uid);
    bool    IsInApplyList(uint32 uid);
	bool    IsInRobotAlreadyJetton(uint32 uid, CGamePlayer* pPlayer);

	//游戏统计
private:
	//计算得分
	bool	CalcuJackpotScore();
	int64   CalculateScore();
	int64   CalcPlayerInfo(uint32 uid,int64 winScore,int64 OnlywinScore,int64 lPlayerJackpotScore = 0,bool isBanker = false);

	// playerAllWinScore : 玩家总赢分
	bool	CalcuPlayerScore(int64 &playerAllWinScore);

    //申请条件
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //次数限制
    int32   GetBankerTimeLimit();

    //申请庄家队列排序
	bool	CompareApplyBankers(CGamePlayer* pBanker1,CGamePlayer* pBanker2);

	static  bool CompareIncomeScoreRank(tagDiceGameDataRank tmpData1, tagDiceGameDataRank tmpData2);
	bool SortIncomeScoreRank();
	bool PushPlayerRank(tagDiceGameDataRank tagData);

	
    //机器人操作
protected:	
	void    OnRobotOper();
	void	OnRobotPlaceJetton();
	void	RemainAreaRobotJetton();
	void    GetAllRobotPlayer(vector<CGamePlayer*> & robots);
	int64	GetRobotJettonScore(CGamePlayer* pPlayer);
	uint8	GetRandJettonArea(int switch_on);	
	void	GetJettonArea();
	bool	RobotJettonJackpotArea();

	bool	PushRobotPlaceJetton(CGamePlayer* pPlayer);
	bool    IsInTableRobot(uint32 uid, CGamePlayer * pPlayer);
	
	bool	RobotContinueJetton(CGamePlayer* pPlayer, uint8 area);


    
    void    OnRobotStandUp();
    void 	CheckRobotCancelBanker();

    void    CheckRobotApplyBanker();
    //设置机器人庄家赢金币
    void    SetRobotBankerWin();
    //设置机器人庄家输金币
    void    SetRobotBankerLose();

	void    AddPlayerToBlingLog();
    //test 
    //void    TestMultiple();

public:

	bool	SetRobotBrankerWin();

	//真实玩家不能赢奖池的金币
	bool    SetPlayerNoWinJackpot();

	bool	ProgressRobotSubJackpot();

	bool	ProgressControlPalyer();

	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

	// 活跃福利控制
	bool ActiveWelfareCtrl();

	// 控制活跃福利玩家赢
	bool  ControlActiveWelfarePalyerWin(uint32 control_uid, int64 max_win);


    //游戏变量
protected:
	// 设置库存输赢  add by har
	// return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();
	// 获取某个玩家的赢分  add by har
	// pPlayer : 玩家
	// return 非机器人玩家赢分
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, int64 &lBankerWinScore);
	// 获取非机器人玩家赢分 add by har
	// return : 非机器人玩家赢分
	int64 GetBankerAndPlayerWinScore();

    //总下注数
protected:
	int64						    m_allJettonScore[AREA_COUNT];		            //全体总注 包括机器人
	int64							m_playerJettonScore[AREA_COUNT];				//玩家下注 不包括机器人
	int64							m_robotJettonScore[AREA_COUNT];					//玩家下注 不包括真实玩家

    map<uint32,int64>               m_userJettonScore[AREA_COUNT];                  //个人总注
    map<uint32,int64>			    m_mpUserWinScore;			                    //玩家成绩
	map<uint32, int64>			    m_mpUserSubWinScore;							//奖池成绩
	map<uint32, int64>			    m_mpUserJackpotScore;							//奖池成绩
	map<uint32, vector<tagUserJackpotInfo>> m_mpUserJackpotInfo;					//奖池信息
	
	map<uint32, vector<tag_dice_area_info>>	m_mpUserAreaInfo;						//用户区域信息
	tag_dice_area_info				m_ArrAllAreaInfo[AREA_COUNT];					//所有区域信息

	map<uint32, int64>			    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水

	//扑克信息
protected:
    // BYTE							m_cbTableCardArray[DICE_COUNT][DICE_POINT_COUNT];		    //桌面扑克
    // BYTE                         m_cbTableCardType[DICE_COUNT];          //桌面牌型

    BYTE							m_cbTableDice[DICE_COUNT];				//桌面骰子
    int32                           m_winMultiple[AREA_COUNT];              //输赢倍数
    
	//庄家信息
protected:	
    vector<CGamePlayer*>			m_ApplyUserArray;						//申请玩家
    map<uint32,uint8>               m_mpApplyUserInfo;                      //是否自动补币
    
    map<uint32,int64>               m_ApplyUserScore;                       //申请带入积分
    
	CGamePlayer*					m_pCurBanker;						    //当前庄家
    uint8                           m_bankerAutoAddScore;                   //自动补币
    bool                            m_needLeaveBanker;                      //离开庄位

    uint16							m_wBankerTime;							//做庄次数
    uint16                          m_wBankerWinTime;                       //胜利次数
	int64						    m_lBankerScore;							//庄家积分
	int64						    m_lBankerCurWinScore;					//本轮成绩
	int64						    m_lBankerWinScore;						//累计成绩

    int64                           m_lBankerBuyinScore;                    //庄家带入
    int64                           m_lBankerInitBuyinScore;                //庄家初始带入
    int64                           m_lBankerWinMaxScore;                   //庄家最大输赢
    int64                           m_lBankerWinMinScore;                   //庄家最惨输赢

	tagDiceJackpotInfo				m_tagDiceJackpotInfo;
protected:
	CDiceLogic						m_GameLogic;							//游戏逻辑
    CCooling                        m_coolRobot;                            //机器人CD    
    uint32                          m_BankerTimeLimit;                      //庄家次数    
    uint32                          m_robotBankerWinPro;                    //机器人庄家赢概率
    uint32                          m_robotApplySize;                       //机器人申请人数
    uint32                          m_robotChairSize;                       //机器人座位数
	bool							m_bIsRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_robotPlaceJetton;						//下注的机器人
	vector<tagRobotJettonCountArea>	m_robotJettonArea;						//机器人需要下注的区域

	uint64							m_lRemainAreaLastRobotJettonTime;
protected:
	//bool							m_bIsChairRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//下注的机器人
	bool							OnChairRobotJetton();
	void							OnChairRobotPlaceJetton();

protected:
	tagDiceGameRecord              	m_record;
	vector<tagDiceGameRecord>		m_vecRecord;	                        //游戏记录
	vector<tagDiceGameRecord>		m_vecGamePlayRecord;	                //游戏记录

	//
	uint32							m_uStartCount;
	uint32							m_uArrAppear[AREA_COUNT];

	vector<tagDiceGameDataRank>	m_vecIncomeScoreRank;
};

#endif //SERVER_GAME_IMPLE_TABLE_H

