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
#include "poker/niuniu_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_niuniu;

class CGamePlayer;
class CGameRoom;

//组件属性
#define GAME_PLAYER					6									//座位人数

//区域索引
#define ID_TIAN_MEN					0									//天
#define ID_DI_MEN					1									//地
#define ID_XUAN_MEN					2									//玄
#define ID_HUANG_MEN				3									//黄

#define AREA_COUNT					4									//区域数目
#define MAX_SEAT_INDEX              5                                   //牌位

//控制区域
enum emAREA
{
	AREA_TIAN_MEN = 0,							//天
	AREA_DI_MEN,								//地
	AREA_XUAN_MEN,								//玄
	AREA_HUANG_MEN,								//黄
	AREA_BANK,									//庄赢
	AREA_XIAN,									//闲赢
	AREA_MAX,									//最大区域
};

//记录信息
struct bainiuGameRecord
{
	BYTE	wins[AREA_COUNT];//输赢标记
	bainiuGameRecord(){
		memset(this,0, sizeof(bainiuGameRecord));
	}
};
struct tagRobotPlaceJetton
{
	uint32 uid;
	bool bflag;
	//CGamePlayer* pPlayer;
	uint8 area;
	int64 time;
	int64 jetton;
	tagRobotPlaceJetton()
	{
		uid = 0;
		bflag = false;
		//pPlayer = NULL;
		area = 0;
		time = 0;
		jetton = 0;
	}
};

// 百牛游戏桌子
class CGameBaiNiuTable : public CGameTable
{
public:
    CGameBaiNiuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameBaiNiuTable();

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

	virtual void    OnNewDay();
	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);
	virtual bool RobotLeavaReadJetton(uint32 uid);


public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    int64    CalcPlayerInfo(uint32 uid,int64 winScore, int64 OnlywinScore,bool isBanker = false);
    // 重置游戏数据
    void    ResetGameData();
    void    ClearTurnGameData();
    
protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
	// isPointKill : 是否点杀 add by har
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip, bool isPointKill);
	// 写入加注log
	void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score);
	// 写入最大牌型
	void 	WriteMaxCardType(uint32 uid,uint8 cardType);
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
    //发送扑克
	bool  DispatchTableCard();
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

	// 新手福利控制
	bool DosWelfareCtrl();
	// 非福利控制
	// return: 0 不受控制  1 后台控制  2 奖池控制  3 机器人赢  4 点杀 6 库存控制 5 活跃福利
	int NotWelfareCtrl();
    // 活跃福利控制
    bool ActiveWelfareCtrl();

	// 控制被杀分
	//bool KilledPlayerCtrl();
	//下注计算
private:
	//最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea);
	
    //庄家站起
    void    StandUpBankerSeat(CGamePlayer* pPlayer);
    bool    IsSetJetton(uint32 uid);
    bool    IsInApplyList(uint32 uid);
    //是否有真实玩家下注
	bool    IsUserPlaceJetton();
	//游戏统计
private:
	//计算得分
	int64   CalculateScore();
	
	// 推断赢家处理 add by har
	// cbTableCardArray ：根据此扑克推断
	void DeduceWinnerDeal(bool &bWinTian, bool &bWinDi, bool &bWinXuan, bool &bWinHuan, uint8 &TianMultiple, uint8 &diMultiple, uint8 &TianXuanltiple, uint8 &HuangMultiple, uint8 cbTableCardArray[][5]);
	//推断赢家
	void    DeduceWinner(bool &bWinTian, bool &bWinDi, bool &bWinXuan,bool &bWinHuan,BYTE &TianMultiple,BYTE &diMultiple,BYTE &TianXuanltiple,BYTE &HuangMultiple );
    
    
    //申请条件
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //次数限制
    int32   GetBankerTimeLimit();

    //申请庄家队列排序
	bool	CompareApplyBankers(CGamePlayer* pBanker1,CGamePlayer* pBanker2);

    //机器人操作
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area);

    void    OnRobotOper();
    void    OnRobotStandUp();
    void 	CheckRobotCancelBanker();

    void    CheckRobotApplyBanker();
    //设置机器人庄家赢金币
    bool    SetRobotBankerWin(bool bBrankerIsRobot, uint32 robotBankerMaxCardPro = PRO_DENO_10000);
private:
	// 将牌组索引按小到大排序 add by harry
	// cbTableCardArray : 要排序的牌组的引用
	// vSortCardIndexs out : 保存排序好的牌组索引
	void SortCardIndexs(uint8 cbTableCardArray[][5], vector<uint8> &vSortCardIndexs);
	// 获取庄家和非机器人玩家赢金币数 add by har
    // cbTableCardArray : 牌组的引用
	// bankerWinScore out : 庄家赢数，调用前必须初始化为0，否则 lBankerWinScore 的值（如果没有赋初始值）是随机的，返回值不确定！
    // return : 非机器人玩家赢金币数
	int64 GetBankerAndPlayerWinScore(uint8 cbTableCardArray[][5], int64 &lBankerWinScore);
	// 获取非机器人玩家赢金币数 add by har
	// cbTableCardArray : 牌组的引用
	// return : 非机器人玩家赢金币数
	int64 GetPlayerWinScore(uint8 cbTableCardArray[][5]);
	// 获取庄家赢金币数 add by har
	// cbTableCardArray : 牌组的引用
    // return : 庄家赢金币数
	int64 GetBankerWinScore(uint8 cbTableCardArray[][5]);
	// 指定牌组是否满足规则 add by harry
	// cbTableCardArray : 指定牌组的引用
	bool IsCurTableCardRuleAllow(uint8 cbTableCardArray[][5]);
	// 设置机器人赢 add by har
	void SetRobotWin();
	// 设置点杀 add by har
	// return : 是否成功设置了点杀
	bool SetTableCardPointKill();
	// 设置庄家赢或输 add by har
	// isWin : true 赢  false 输
	void SetBankerWinLose(bool isWin);
protected:
    //设置机器人庄家输金币
    void    SetRobotBankerLose();

	bool    SetPlayerBrankerWin();
	
	bool    SetPlayerBrankerLost();

	bool	SetControlPlayerWin(uint32 control_uid);

	bool	SetControlPlayerLost(uint32 control_uid);

	bool    SetLeisurePlayerWin();

	bool    SetLeisurePlayerLost();

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

	bool	GetPlayerGameRest(uint32 uid);

	bool	GetCardSortIndex(uint8 uArSortIndex[]);

	bool	GetJettonSortIndex(uint32 uid,uint8 uArSortIndex[]);

    void    AddPlayerToBlingLog();
    //test 
    void    TestMultiple();

	//void    GetAllRobotPlayer(vector<CGamePlayer*> & robots);

        
    //游戏变量
protected:
    //总下注数
protected:
	int64						    m_allJettonScore[AREA_COUNT];		            //全体总注
	int64							m_playerJettonScore[AREA_COUNT];				//玩家下注

    map<uint32,int64>               m_userJettonScore[AREA_COUNT];                  //个人总注
    map<uint32,int64>			    m_mpUserWinScore;			                    //玩家成绩

	map<uint32, int64>			    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水
	int64							m_curr_banker_win;								//当前庄家的赢取金额---只统计赢不包括输，用于计算抽水

	//扑克信息
protected:
    BYTE							m_cbTableCardArray[MAX_SEAT_INDEX][5];		    //桌面扑克
    BYTE                            m_cbTableCardType[MAX_SEAT_INDEX];              //桌面牌型
    int32                           m_winMultiple[AREA_COUNT];                      //输赢倍数
    
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

    int64                           m_lBankerBuyinScore;                    //庄家带入
    int64                           m_lBankerInitBuyinScore;                //庄家初始带入
    int64                           m_lBankerWinMaxScore;                   //庄家最大输赢
    int64                           m_lBankerWinMinScore;                   //庄家最惨输赢
	uint16							m_cbBrankerSettleAccountsType;			//庄家结算类型

// add by har
private:
	int                             m_confBankerAllWinLoseMaxCount;    // 庄家最近通输通赢最大局数
	int                             m_confBankerAllWinLoseLimitCount;  // 庄家最近通输通赢限制次数
	int                             m_bankerAllWinLoseComputeCount; // 庄家最近通输通赢次数计数局数
	int                             m_bankerAllWinLoseCount; // 庄家最近通输通赢次数
	bool                            m_bIsConputeBankerAllWinLose; // 是否开始计数通输通赢局数
	int                             m_confRobotBankerAreaPlayerWinMax; // 机器人当庄区域玩家允许赢下注最大值
	int                             m_confRobotBankerAreaPlayerLoseRate; // 机器人当庄区域玩家下注超过允许赢下注最大值，玩家输的概率
	bool                            m_isTableCardPointKill[MAX_SEAT_INDEX]; // 是否点杀
	// add by har end

    
	//控制变量
protected:
	int								m_iMaxJettonRate; // 最大下注倍数
	tagNiuMultiple					m_tagNiuMultiple;

	//int64							m_lMaxPollScore;						  //最大奖池分数
	//int64							m_lMinPollScore;						  //最小奖池分数
	//int64							m_lCurPollScore;						  //当前奖池分数
	//int64							m_lFrontPollScore;						  //
	//uint32							m_uSysWinPro;							  //系统赢概率
	//uint32							m_uSysLostPro;							  //系统输概率


	//组件变量
protected:
	CNiuNiuLogic					m_GameLogic;							//游戏逻辑
    uint32                          m_BankerTimeLimit;                      //庄家次数
    
    uint32                          m_robotBankerWinPro;                    //机器人庄家赢概率
	uint32                          m_robotBankerMaxCardPro;                //机器人庄家最大牌概率
    uint32                          m_robotApplySize;                       //机器人申请人数
    uint32                          m_robotChairSize;                       //机器人座位数      

	bainiuGameRecord              	m_record;
	vector<bainiuGameRecord>		m_vecRecord;	                        //游戏记录
	vector<bainiuGameRecord>		m_vecGamePlayRecord;	                //游戏记录
	

	bool IsInTableRobot(uint32 uid, CGamePlayer * pPlayer);
	bool OnChairRobotJetton();
	void OnChairRobotPlaceJetton();
	bool							m_bIsChairRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//下注的坐在椅子上的机器人

	//
	bool OnRobotJetton();
	void OnRobotPlaceJetton();
	bool							m_bIsRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_RobotPlaceJetton;				//下注的机器人
	int64							m_lGameCount;
public:
		void OnRobotTick();

		uint32						m_uBairenTotalCount;
		vector<uint32>				m_vecAreaWinCount;
		vector<uint32>				m_vecAreaLostCount;
		void AddGameCount();
		void InitAreaSize(uint32 count);
		void OnTablePushAreaWin(uint32 index, int win);

//百人场精准控制
public:
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
	bool   SetControlBankerScore(bool isWin);							//设置庄家总体为赢/输 通过参数控制
	
	//当申请上庄玩家被强制下庄后，需要通知该上庄玩家
	void  OnNotityForceApplyUser(CGamePlayer* pPlayer);
};

#endif //SERVER_GAME_IMPLE_TABLE_H

