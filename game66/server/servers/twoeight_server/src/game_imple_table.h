//
// Created on 19/8/12.
//
// 二八杠的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H


#include "game_table.h"
#include "svrlib.h"
#include "twoeight_logic.h"


using namespace std;
using namespace game_twoeight;

class CGamePlayer;
class CGameRoom;

//组件属性
#define GAME_PLAYER 6 // 座位人数

//区域索引
#define ID_SHUN_MEN 0 // 顺
#define ID_TIAN_MEN	1 // 天
#define ID_DI_MEN	2 // 地

#define AREA_COUNT	   3 // 区域数目


//控制区域
enum emAREA {
	AREA_SHUN_MEN = 0, // 顺
	AREA_TIAN_MEN,	   // 天
	AREA_DI_MEN,	   // 地
	AREA_BANK,		   // 庄赢
	AREA_XIAN,		   // 闲赢
	AREA_MAX,		   // 最大区域
};

// 记录信息
struct stTwoeightGameRecord {
	uint8 wins[AREA_COUNT]{0}; // 输赢标记
};

// 机器人下注变量
struct tagRobotPlaceJetton {
	uint32 uid = 0;
	bool bflag = false;
	uint8 area = 0;
	int64 time = 0;
	int64 jetton = 0;
};

// 二八杠游戏桌子
class CGameTwoeightbarTable : public CGameTable {
public:
	CGameTwoeightbarTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
	virtual ~CGameTwoeightbarTable() {}

	/// 重载基类函数 ///
	virtual bool    CanEnterTable(CGamePlayer* pPlayer);
	virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
	virtual bool    CanStandUp(CGamePlayer* pPlayer) { return true; }

	virtual bool    IsFullTable();
	virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

	//配置桌子
	virtual bool Init();
	virtual void ShutDown() {}
	virtual bool ReAnalysisParam();

	//复位桌子
	virtual void ResetTable();
	virtual void OnTimeTick();
	//游戏消息
	virtual int  OnGameMessage(CGamePlayer* pPlayer, uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
	//用户断线或重连
	virtual bool OnActionUserNetState(CGamePlayer* pPlayer, bool bConnected, bool isJoin = true);
	//用户坐下
	virtual bool OnActionUserSitDown(uint16 wChairID, CGamePlayer* pPlayer);
	//用户起立
	virtual bool OnActionUserStandUp(uint16 wChairID, CGamePlayer* pPlayer);

	// 游戏开始
	virtual bool OnGameStart();
	// 游戏结束
	virtual bool OnGameEnd(uint16 chairID, uint8 reason);
	//玩家进入或离开
	virtual void OnPlayerJoin(bool isJoin, uint16 chairID, CGamePlayer* pPlayer);

	virtual void OnNewDay();
	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);
	virtual bool RobotLeavaReadJetton(uint32 uid); // 玩家下线

	// 发送场景信息(断线重连)
	virtual void SendGameScene(CGamePlayer* pPlayer);

	int64    CalcPlayerInfo(uint32 uid, int64 winScore, int64 OnlywinScore, bool isBanker = false);
	// 重置游戏数据
	void    ResetGameData();
	void    ClearTurnGameData();

	// 通过CRobotOperMgr::OnTickRobot函数调用过来
	void OnRobotTick();

protected:
	// 获取单个下注的是机器人还是玩家  add by har
	// 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
	// 写入出牌log
	// isPointKill : 是否点杀 add by har
	void    WriteOutCardLog(uint16 chairID, uint8 cardData[], int32 mulip, bool isPointKill);
	// 写入加注log
	void    WriteAddScoreLog(uint32 uid, uint8 area, int64 score);
	// 写入最大牌型
	void 	WriteMaxCardType(uint32 uid, uint8 cardType);
	// 写入庄家信息
	void    WriteBankerInfo();

	/// 游戏事件 ///
	//加注事件
	// return: true 删除机器人加注项 false 不删除机器人加注项
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, uint8 cbJettonArea, int64 lJettonScore);
	//申请庄家
	bool OnUserApplyBanker(CGamePlayer* pPlayer, int64 bankerScore, uint8 autoAddScore);
	bool OnUserJumpApplyQueue(CGamePlayer* pPlayer); // 坐庄插队
	// 玩家续押
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req &msg);

	// 取消申请庄家
	bool OnUserCancelBanker(CGamePlayer* pPlayer);


	//发送扑克
	bool  DispatchTableCard();
	//发送庄家
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
	//排序庄家,按上分数从大到小排序
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

	// 新手福利控制
	bool DosWelfareCtrl();
	// 非福利控制
	// return: 0 不受控制  1 后台控制  2 奖池控制  3 机器人赢  4 点杀 6 库存控制 5 活跃福利
	int NotWelfareCtrl();
	// 活跃福利控制
	//bool ActiveWelfareCtrl();

	/// 下注计算 ///
	// 玩家当前允许的最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer/*, uint8 cbJettonArea*/);

	// 庄家站起
	void    StandUpBankerSeat(CGamePlayer* pPlayer);
	bool    IsSetJetton(uint32 uid);
	bool    IsInApplyList(uint32 uid);
	// 是否有真实玩家下注
	bool    IsUserPlaceJetton();
	
	/// 游戏统计 ///
	// 计算得分
	int64   CalculateScore();

	// 推断赢家处理 add by har
	// bWinFlag out : 区域胜利标识
	// cbMultiple out : 区域牌型倍数
	// cbTableCardArray ：根据此扑克推断
	void DeduceWinnerDeal(bool bWinFlag[AREA_COUNT], int cbMultiple[AREA_COUNT], uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]);
	// 推断赢家
	void DeduceWinner(bool bWinFlag[AREA_COUNT], int cbMultiple[AREA_COUNT]);


	// 申请上庄条件
	int64   GetApplyBankerCondition();
	int64   GetApplyBankerConditionLimit();

	// 次数限制
	int32   GetBankerTimeLimit();

	//申请庄家队列排序
	bool	CompareApplyBankers(CGamePlayer* pBanker1, CGamePlayer* pBanker2);

	/// 机器人操作 ///
	int64	GetRobotJettonScore(CGamePlayer* pPlayer/*, uint8 area*/);

	void    OnRobotStandUp();
	void 	CheckRobotCancelBanker();

	void    CheckRobotApplyBanker(); // 检查机器人申请庄家


	// 指定牌组是否满足规则
	// cbTableCardArray : 指定牌组的引用
	bool IsCurTableCardRuleAllow(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]);
	// 获得单个玩家总赢分实现
	int64 GetSinglePlayerWinScoreDeal(uint32 playerUid, int cbMultiple[AREA_COUNT], bool bWinFlag[AREA_COUNT], int64 &lBankerWinScore);
	// 获取庄家和非机器人玩家赢金币数
    // cbTableCardArray : 牌组的引用
    // bankerWinScore out : 庄家赢数
    // return : 非机器人玩家赢金币数
	int64 GetBankerAndPlayerWinScore(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM], int64 &lBankerWinScore);
	// 获取玩家(非庄家)赢金币数
	// cbTableCardArray : 牌组的引用
	// uid : 玩家id
	// return : 非庄玩家赢金币数
	int64 GetSinglePlayerWinScore(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM], uint32 uid);

	// 设置庄家赢或输 add by har
    // isWin : true 赢  false 输
	// return : true 设置成功  false 设置失败
	bool SetBankerWinLose(bool isWin);
	// 设置控制玩家赢或输
	// isWin : true 赢  false 输
	// return : true 设置成功  false 设置失败
	bool SetControlPlayerWinLose(uint32 control_uid, bool isWin);
	// 设置点杀
	// return : 是否成功设置了点杀
	bool SetTableCardPointKill();
	// 设置库存输赢
	// return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

	bool GetPlayerGameRest(uint32 uid);

	// 取得桌面4组牌的小到大排序实现
    // cbTableCardArray:要排序的MAX_SEAT_INDEX组牌
    // uArSortIndex out ： 保存牌组大小从小到大的排序数组
	void GetCardSortIndexImpl(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM], uint8 uArSortIndex[MAX_SEAT_INDEX]);

	// 取得桌面4组牌的小到大排序，默认使用m_cbTableCardArray牌组
	// uArSortIndex out ： 保存牌组大小从小到大的排序数组
	void GetCardSortIndex(uint8 uArSortIndex[MAX_SEAT_INDEX]);

	bool	GetJettonSortIndex(uint32 uid, uint8 uArSortIndex[]);

	void    AddPlayerToBlingLog();


	/// 游戏变量 ///
	//总下注数
	int64						    m_allJettonScore[AREA_COUNT]{0};		        //全体总注
	int64							m_playerJettonScore[AREA_COUNT]{0};				//玩家下注

	unordered_map<uint32, int64>    m_userJettonScore[AREA_COUNT];                  //个人总注
	unordered_map<uint32, int64>    m_mpUserWinScore;			                    //玩家成绩

	unordered_map<uint32, int64>    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水
	int64							m_curr_banker_win;								//当前庄家的赢取金额---只统计赢不包括输，用于计算抽水

	/// 扑克信息 ///
	uint8							m_cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]{0};		//桌面扑克
	uint8                           m_cbTableCardType[MAX_SEAT_INDEX];              //桌面牌型
	int32                           m_winMultiple[AREA_COUNT];                      //输赢倍数

	//庄家信息
	uint8                           m_bankerAutoAddScore;                   //自动补币
	
	uint16							m_wBankerTime = 0;						//做庄次数
	uint16                          m_wBankerWinTime;                       //胜利次数
	int64						    m_lBankerScore;							//庄家积分
	int64						    m_lBankerWinScore = 0L;					//累计成绩

	int64                           m_lBankerBuyinScore;                    //庄家带入
	int64                           m_lBankerInitBuyinScore;                //庄家初始带入
	int64                           m_lBankerWinMaxScore;                   //庄家最大输赢
	int64                           m_lBankerWinMinScore;                   //庄家最惨输赢
	uint16							m_cbBrankerSettleAccountsType;			//庄家结算类型

    /// 通输通赢相关成员变量 ///
	int                             m_confBankerAllWinLoseMaxCount = 0;    // 庄家最近通输通赢最大局数
	int                             m_confBankerAllWinLoseLimitCount = 1;  // 庄家最近通输通赢限制次数
	int                             m_bankerAllWinLoseComputeCount = 0; // 庄家最近通输通赢次数计数局数
	int                             m_bankerAllWinLoseCount = 0; // 庄家最近通输通赢次数
	bool                            m_bIsConputeBankerAllWinLose = false; // 是否开始计数通输通赢局数
	int                             m_confRobotBankerAreaPlayerWinMax = 100000000; // 机器人当庄区域玩家允许赢下注最大值
	int                             m_confRobotBankerAreaPlayerLoseRate = 0; // 机器人当庄区域玩家下注超过允许赢下注最大值，玩家输的概率
	bool                            m_isTableCardPointKill[MAX_SEAT_INDEX]{0}; // 是否点杀

	/// 控制变量 ///
	int								m_iMaxJettonRate = 0; // 最大下注倍数

	/// 组件变量 ///
	//CTwoeightLogic					m_GameLogic;							//游戏逻辑
	uint32                          m_BankerTimeLimit;                      //庄家次数

	uint32                          m_robotBankerWinPro = 0;                    //机器人庄家赢概率
	uint32                          m_robotBankerMaxCardPro = 0;                //机器人庄家最大牌概率
	uint32                          m_robotApplySize;                       //机器人申请人数
	uint32                          m_robotChairSize;                       //机器人座位数      

	stTwoeightGameRecord            m_record;
	vector<stTwoeightGameRecord>	m_vecRecord;	                        //游戏记录
	vector<stTwoeightGameRecord>	m_vecGamePlayRecord;	                //游戏记录

	bool IsInTableRobot(uint32 uid, CGamePlayer *pPlayer);
	void OnRobotJettonDeal(CGamePlayer *pPlayer, bool isChairPlayer); // 机器人押注准备实现
	void OnRobotJetton();      // 机器人押注准备
	void OnRobotPlaceJetton(); // 定时器触发机器人下注
	bool						m_bIsRobotAlreadyJetton = false; // 是否机器人准备完毕
	vector<tagRobotPlaceJetton>	m_RobotPlaceJetton;				 // 机器人下注数据项列表

	/// 计数相关 ///
	uint32						m_uBairenTotalCount = 0;
	vector<uint32>				m_vecAreaWinCount;
	vector<uint32>				m_vecAreaLostCount;
	void AddGameCount();
	void InitAreaSize(uint32 count);
	void OnTablePushAreaWin(uint32 index, int win);

	/// 百人场精准控制 ///
	void   OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer);			//发送在线所有真实玩家下注详情
	void   OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer);	//通知单个玩家信息
	void   OnBrcControlSendAllRobotTotalBetInfo();						//发送所有机器人总下注信息
	void   OnBrcControlSendAllPlayerTotalBetInfo();						//发送所有真实玩家总下注信息
	bool   OnBrcControlEnterControlInterface(CGamePlayer* pPlayer);		//进入控制界面
	void   OnBrcControlBetDeal(CGamePlayer* pPlayer);					//下注处理
	bool   OnBrcAreaControl();											//百人场区域控制
	bool   OnBrcAreaControlForA(uint8 ctrl_area_a);						//百人场A区域控制 庄赢/庄输
	bool   OnBrcAreaControlForB(set<uint8> &area_list);					//百人场B区域控制 天/地/玄/黄 支持多选
	void   OnBrcFlushSendAllPlayerInfo();								//刷新在线所有真实玩家信息---玩家进入/退出桌子时调用

	//当申请上庄玩家被强制下庄后，需要通知该上庄玩家
	void  OnNotityForceApplyUser(CGamePlayer* pPlayer);
};

#endif //SERVER_GAME_IMPLE_TABLE_H

