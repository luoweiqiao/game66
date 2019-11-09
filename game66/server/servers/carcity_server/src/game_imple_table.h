//
// Created on 19/9/25.
//
// 奔驰宝马的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H


#include "game_table.h"
#include "svrlib.h"


using namespace std;

class CGamePlayer;
class CGameRoom;

//组件属性


#define JETTON_INDEX_COUNT 8  // 区域数目


//控制区域(索引)
enum emAREA {
	AREA_LBGN_MEN = 0, // 兰博基尼
	AREA_FERRARI_MEN,  // 法拉利
	AREA_BMW_MEN,      // 宝马
	AREA_BENZ_MEN,	   // 奔驰
	AREA_AUDI_MEN,     // 奥迪
	AREA_VOLKS_MEN,	   // 大众
	AREA_TOYOTA_MEN,   // 丰田
	AREA_HONDA_MEN,	   // 本田
	AREA_BANK,		   // 真实玩家输
	AREA_XIAN,		   // 真实玩家赢
	AREA_MAX,		   // 最大区域
};

// 记录信息
struct stCarcityGameRecord {
	uint8 wins[JETTON_INDEX_COUNT]{0}; // 输赢标记
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
class CGameCarcityTable : public CGameTable {
public:
	CGameCarcityTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
	virtual ~CGameCarcityTable() {}

	/// 重载基类函数 ///
	virtual bool    CanEnterTable(CGamePlayer* pPlayer);
	virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
	virtual bool    CanSitDown(CGamePlayer* pPlayer, uint16 chairID) { return false; } // 不允许坐下
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
	bool OnGameStart();
	// 游戏结束
	virtual bool OnGameEnd(uint16 chairID, uint8 reason);
	//玩家进入或离开
	virtual void OnPlayerJoin(bool isJoin, uint16 chairID, CGamePlayer* pPlayer);

	virtual void GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	virtual void GetGameEndLogInfo(net::msg_game_play_log* pInfo);

	// 发送场景信息(断线重连)
	virtual void SendGameScene(CGamePlayer* pPlayer);

	int64    CalcPlayerInfo(uint32 uid, int64 winScore, int64 OnlywinScore, bool isBanker = false);
	// 重置游戏数据
	void    ResetGameData();
	void    ClearTurnGameData();

	// 通过CRobotOperMgr::OnTickRobot函数调用过来
	void OnRobotTick();

protected:
	// 获取单个下注的是机器人还是玩家
	// 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
	// 写入出牌log
	void WriteOutCardLog(uint16 chairID, uint8 cardData[], uint8 cardCount, int32 mulip);
	// 写入加注log
	void    WriteAddScoreLog(uint32 uid, uint8 area, int64 score);
	// 写入最大牌型
	void 	WriteMaxCardType(uint32 uid, uint8 cardType);
	void    WriteGameInfoLog();

	/// 游戏事件 ///
	//加注事件
	// return: true 删除机器人加注项 false 不删除机器人加注项
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, uint8 cbJettonArea, int64 lJettonScore);
	// 玩家续押
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req &msg);


	//发送扑克
	bool  DispatchTableCard();
	void  SendPlayLog(CGamePlayer* pPlayer);

	// 新手福利控制
	bool DosWelfareCtrl();
	// 非福利控制
	void NotWelfareCtrl();

	/// 下注计算 ///
	// 玩家当前允许的最大下注
	int64   GetUserMaxJetton(CGamePlayer* pPlayer/*, uint8 cbJettonArea*/);

	bool    IsSetJetton(uint32 uid);

	/// 游戏统计 ///
	// 计算得分
	int64   CalculateScore();


	// 申请上庄条件
	int64   GetApplyBankerCondition();
	int64   GetApplyBankerConditionLimit();

	/// 机器人操作 ///
	int64	GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area);
	int64 GetRobotJettonScoreRand(CGamePlayer* pPlayer, uint8 area);

	// 获取庄家和非机器人玩家赢金币数
	// winCarType : 赢的车型
	// return : 非机器人玩家赢金币数
	int64 GetPlayerWinScore(int winCarType);
	// 获取玩家(非庄家)赢金币数
	// uid : 玩家id
	// winCarType : 赢的车型
	// return : 非庄玩家赢金币数
	int64 GetSinglePlayerWinScore(uint32 uid, int winCarType);

	// 设置控制玩家赢或输
	// isWin : true 赢  false 输
	// return : true 设置成功  false 设置失败
	bool SetControlPlayerWinLose(uint32 control_uid, bool isWin);
	// 设置库存输赢
	// return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

	bool GetPlayerGameRest(uint32 uid);

	bool	GetJettonSortIndex(uint32 uid, uint8 uArSortIndex[]);

	void    AddPlayerToBlingLog();


	/// 游戏变量 ///
	bool m_bInitTableSuccess = false;

	//总下注数
	int64						    m_allJettonScore[JETTON_INDEX_COUNT]{0};		        //全体总注
	int64							m_playerJettonScore[JETTON_INDEX_COUNT]{0};				//玩家下注

	map<uint32, int64>    m_userJettonScore[JETTON_INDEX_COUNT];          //个人总注
	map<uint32, int64>    m_mpUserWinScore;			                    //玩家成绩

	map<uint32, int64>    m_mpWinScoreForFee;								//玩家成绩---只统计赢不包括输---用于计算抽水

	int m_winCardType = AREA_MAX; // 开奖车型
	int m_winIndex = -1; // 赢的坐标

	/// 扑克信息 ///
	int32 m_winMultiple[JETTON_INDEX_COUNT]; // 输赢倍数,各类型车的倍数


	/// 控制变量 ///
	int	m_iArrDispatchCardPro[JETTON_INDEX_COUNT]; // 各类型车抽出概率
	//int	m_cardMultiple[JETTON_INDEX_COUNT]; // 各类型车的倍数

	/// 组件变量 ///
     

	stCarcityGameRecord            m_record;
	vector<stCarcityGameRecord>	m_vecRecord;	                        //游戏记录
	vector<stCarcityGameRecord>	m_vecGamePlayRecord;	                //游戏记录

	bool IsInTableRobot(uint32 uid, CGamePlayer *pPlayer);
	void OnRobotJettonDeal(CGamePlayer *pPlayer, bool isChairPlayer); // 机器人押注准备实现
	void OnRobotJetton();      // 机器人押注准备
	void OnRobotPlaceJetton(); // 定时器触发机器人下注
	uint8 GetRobotJettonArea();
	bool						m_bIsRobotAlreadyJetton = false; // 是否机器人准备完毕
	vector<tagRobotPlaceJetton>	m_RobotPlaceJetton;				 // 机器人下注数据项列表
	uint8 m_cbFrontRobotJettonArea;



	/// 百人场精准控制 ///
	void   OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer);			//发送在线所有真实玩家下注详情
	void   OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer);	//通知单个玩家信息
	void   OnBrcControlSendAllRobotTotalBetInfo();						//发送所有机器人总下注信息
	void   OnBrcControlSendAllPlayerTotalBetInfo();						//发送所有真实玩家总下注信息
	bool   OnBrcControlEnterControlInterface(CGamePlayer* pPlayer);		//进入控制界面
	void   OnBrcControlBetDeal(CGamePlayer* pPlayer);					//下注处理
	bool   OnBrcAreaControl();											//百人场区域控制
	void   OnBrcFlushSendAllPlayerInfo();								//刷新在线所有真实玩家信息---玩家进入/退出桌子时调用
};

#endif //SERVER_GAME_IMPLE_TABLE_H

