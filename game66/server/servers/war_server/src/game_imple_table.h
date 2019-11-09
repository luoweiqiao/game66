

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "war_logic.h"
#include "robot_oper_mgr.h"

using namespace svrlib;
using namespace std;
using namespace game_war;

class CGamePlayer;
class CGameRoom;

//组件属性
#define GAME_PLAYER					6				//座位人数


//记录信息
struct warGameRecord
{
	BYTE	wins[JETTON_INDEX_COUNT];//输赢标记
	BYTE	card;//桌面扑克
	int								index;
	warGameRecord()
	{
		memset(this,0, sizeof(warGameRecord));
	}
};

struct tagRobotPlaceJetton
{
	uint32 uid;
	bool bflag;
	CGamePlayer* pPlayer;
	uint8 area;
	int64 time;
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

//百人场控制区域类型
enum emBRC_CONTROL_AREA_TYPE
{
	//区域类型
	AREA_SINGLE = 1,			//单牌类型
	AREA_DOUBLE,				//对子类型
	AREA_SHUN_ZI,				//顺子类型
	AREA_JIN_HUA,				//金花类型
	AREA_SHUN_JIN,				//顺金类型
	AREA_BAO_ZI,				//豹子类型
	AREA_BLACK,					//黑区域
	AREA_RED,					//红区域
	AREA_MAX,					//最大区域
};

class CGameWarTable : public CGameTable
{
public:
	CGameWarTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameWarTable();

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

	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);

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
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip);
	// 写入加注log
	void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score);
	// 写入最大牌型
	void 	WriteMaxCardType(uint32 uid,uint8 cardType);

	void    WriteGameInfoLog();

    //游戏事件
protected:
	//加注事件
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore);
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg);

    
protected:


    //发送扑克
	bool  DispatchTableCard();
	void  SendPlayLog(CGamePlayer* pPlayer);

	// 福利控制
	bool DosWelfareCtrl();
	// 非福利控制
	bool NotWelfareCtrl();
	//bool KilledPlayerCtrl();

    // 活跃福利控制
    bool ActiveWelfareCtrl();

	//下注计算
private:
	//最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea);
    bool    IsSetJetton(uint32 uid);
	//游戏统计
private:
	void GetAreaInfo(WORD iLostIndex, WORD iWinIndex, BYTE cbType[], BYTE cbWinArea[], int64 lWinMultiple[]);

	//计算得分
	int64   CalculateScore();
    
    
    //申请条件
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //机器人操作
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area);

    void    OnRobotOper();
    void    OnRobotStandUp();

    void    AddPlayerToBlingLog();

	//void    GetAllRobotPlayer(vector<CGamePlayer*> & robots);
	uint32 GetBankerUID();

	int64	GetAreaMultiple(int nAreaIndex, int iLostIndex, int iWinIndex);
	bool    IsCanStartGame();

    //游戏变量
protected:
	bool	m_bInitTableSuccess;
    //总下注数
protected:
	int64						    m_allJettonScore[JETTON_INDEX_COUNT];		            //全体总注
	int64							m_playerJettonScore[JETTON_INDEX_COUNT];				//玩家下注

    map<uint32,int64>               m_userJettonScore[JETTON_INDEX_COUNT];                  //个人总注
    map<uint32,int64>			    m_mpUserWinScore;										//玩家成绩

	map<uint32, int64>			    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水
	
	//扑克信息
protected:
    BYTE							m_cbTableCardArray[SHOW_CARD_COUNT][MAX_CARD_COUNT];	//桌面扑克
    BYTE                            m_cbTableCardType[SHOW_CARD_COUNT];						//桌面牌型
    int32                           m_winMultiple[JETTON_INDEX_COUNT];                      //输赢倍数
    int32							m_winIndex[JETTON_INDEX_COUNT];
	//控制变量
protected:

	CGamePlayer *					m_pCurBanker;

	int		m_iArrDispatchCardPro[Pro_Index_MAX];
	bool	ProbabilityDispatchPokerCard();
	int     GetProCardType();

	uint32							m_sysBankerWinPro;						//系统庄家赢概率
	bool							SetSystemBrankerWinPlayerScore();

	uint32                          m_robotBankerWinPro;                    //机器人庄家赢概率
	uint32                          m_robotBankerMaxCardPro;                //机器人庄家最大牌概率

	uint32                          m_robotApplySize;                       //机器人申请人数
	uint32                          m_robotChairSize;                       //机器人座位数      
	int64 GetRobotJettonScoreRand(CGamePlayer* pPlayer, uint8 area);


	bool	ProgressControlPalyer();
	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

    bool    SetControlPalyerWinForAW(uint32 control_uid, int64 max_win);

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();
	// 获取某个玩家的赢分  add by har
	// pPlayer : 玩家
	// return 非机器人玩家赢分
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, uint8 cbWinArea[JETTON_INDEX_COUNT], int64 lWinMultiple[JETTON_INDEX_COUNT], int64 &lBankerWinScore);
	// 获取非机器人玩家赢分 add by har
	// return : 非机器人玩家赢分
	int64 GetBankerAndPlayerWinScore();

	//组件变量
protected:
	CWarLogic						m_GameLogic;							//游戏逻辑
    

	warGameRecord              		m_record;
	vector<warGameRecord>			m_vecRecord;	                        //游戏记录
	vector<warGameRecord>			m_vecGamePlayRecord;	                //游戏记录

public:
	void OnRobotTick();

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
	uint8 m_cbFrontRobotJettonArea;
	uint8 GetRobotJettonArea();

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

	//更换庄家
	bool  ChangeBanker(bool bCancelCurrentBanker);
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
};

#endif //SERVER_GAME_IMPLE_TABLE_H

