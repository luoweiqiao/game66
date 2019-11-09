
// 时时彩的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;
class CGameRoom;

#define EVERY_COLOR_AREA_COUNT	7
#define EVERY_COLOR_CARD_COUNT 52

#define EVERY_COLOR_SNATCH_COIN_CARD_COUNT 54

static const int GAME_PLAYER = 1;	//最大游戏人数

// 扑克数值掩码
#define	MASK_COLOR			0xF0	//花色掩码
#define	MASK_VALUE			0x0F	//数值掩码


namespace game_everycolor
{
	//扑克数据
	const BYTE g_cbEveryColorCardListData[EVERY_COLOR_SNATCH_COIN_CARD_COUNT] =
	{
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
		0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
		0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
		0x4E,0x4F
	};

	enum emEveryColorAreaIndex
	{
		AreaIndex_Spade					= 0,	// 黑桃
		AreaIndex_Heart					= 1,	// 梅花
		AreaIndex_Club					= 2,	// 红桃
		AreaIndex_Diamond				= 3,	// 方块
		AreaIndex_Small					= 4,	// 小
		AreaIndex_Seven					= 5,	// 7
		AreaIndex_Big					= 6		// 大
	};

	enum emSnatchCoinType
	{
		SnatchCoinType_Ten				= 0,	// 十元夺宝
		SnatchCoinType_Hundred			= 1,	// 百元夺宝
		SnatchCoinType_Jetton			= 2,	// 大小下注
		SnatchCoinType_Low				= 3		// 非法类型
	};

#define			AreaMultiple_Spade		1
#define 		AreaMultiple_Heart		1
#define 		AreaMultiple_Club		1
#define 		AreaMultiple_Diamond	1
#define 		AreaMultiple_Big		2
#define 		AreaMultiple_Seven		10
#define 		AreaMultiple_Small		2

};

using namespace game_everycolor;

class CGameEveryColorTable : public CGameTable
{
public:
	CGameEveryColorTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameEveryColorTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

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
 
public:
    // 游戏开始
    virtual bool OnGameStart();
    // 游戏结束
    virtual bool OnGameEnd(uint16 chairID, uint8 reason);
    //用户同意
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    // 重置游戏数据
    void    ResetGameData();
protected:
    // 是否能够开始游戏
    bool    IsCanStartGame();
    // 检测筹码是否正确
	bool    CheckJetton(CGamePlayer* pPlayer, int64 lScore);

	int64   GetUserMaxJetton(CGamePlayer* pPlayer);

	bool	GetOpenCard();
	int64	CalculateScore();
	//是否第一次下注成功
	bool    IsExistJettonSuccess(uint32 uid);
	//获取扑克数值
	uint8    GetCardValue(uint8 cbCardData) { return cbCardData&MASK_VALUE; }
	//获取扑克花色
	uint8    GetCardColor(uint8 cbCardData) { return cbCardData&MASK_COLOR; }
	//日志
	void    WriteCardDataInfo();
	void    WritePeriodsInfo();
	void    WriteAddScoreLog(uint32 uid, uint8 area, int64 score);
	void    WriteAreaInfo();
    //游戏事件
protected:
	//加注事件
	bool    OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore);
	bool    OnUserGameRecord(CGamePlayer* pPlayer) { return true; }
	//机器人操作
protected:
	void    OnRobotOper();


	//扑克变量
protected:
	bool							m_bInitTableSuccess;
	vector<uint32>					m_userJettonSuccess;							//用户下注成功uid列表
	map<uint32, int64>              m_userJettonScore[EVERY_COLOR_AREA_COUNT];		//用户各区域下注分数
	int64							m_areaJettonScore[EVERY_COLOR_AREA_COUNT];		//区域总下注
	bool							m_areaHitPrize[EVERY_COLOR_AREA_COUNT];			//中奖区域
	map<uint32, int64>              m_mpUserWinScoreFlower;							//用户分数
	map<uint32, int64>              m_mpUserWinScoreBigSmall;						//用户分数
	map<uint32, int64>              m_mpUserLostScoreFlower;						//用户分数
	map<uint32, int64>              m_mpUserLostScoreBigSmall;						//用户分数
	map<uint32, int64>              m_mpUserWinScoreToatl;							//用户分数
	map<uint32, int64>				m_mpUserWinScoreFee;							//收税分数
	BYTE							m_cbCardData;									//牌数据
	
	//------------------------------------------------------------------------------- 
	// 十元夺宝和百元夺宝
	uint32							m_uSnatchCoinStop;								//停止购买
	CCooling						m_coolLogicTen;									//逻辑CD
	CCooling						m_coolLogicHundred;								//逻辑CD
	uint8							m_gameStateTen;									//游戏状态
	uint8							m_gameStateHundred;								//游戏状态

	stGameBlingLog					m_blingLogTen;									//牌局日志
	stGameBlingLog					m_blingLogHundred;								//牌局日志
	Json::Value						m_operLogTen;									//出牌操作LOG
	Json::Value						m_operLogHundred;								//出牌操作LOG

	uint32							m_uGameCountTen;								//游戏局数
	uint32							m_uGameCountHundred;							//游戏局数
	string							m_strPeriodsTen;								//期数
	string							m_strPeriodsHundred;							//期数
	vector<BYTE>                    m_poolCardsTen;									//牌池扑克
	vector<BYTE>                    m_poolCardsHundred;								//牌池扑克
	BYTE							m_cbSnatchCoinCardDataTen;						//牌数据
	BYTE							m_cbSnatchCoinCardDataHundred;					//牌数据
	map<uint32, vector<BYTE>>		m_userSnatchCoinTen;							//用户十元夺宝数据
	map<uint32, vector<BYTE>>		m_userSnatchCoinHundred;						//用户百元夺宝数据
	map<uint32, vector<BYTE>>		m_userSnatchCoinRobotTen;						//机器十元夺宝数据
	map<uint32, vector<BYTE>>		m_userSnatchCoinRobotHundred;					//机器百元夺宝数据
	uint32							m_PlayerWinSnatchCoinTen;						//获得十元夺宝用户uid
	uint32							m_PlayerWinSnatchCoinHundred;					//获得百元夺宝用户uid
	int64							m_WinScoreSnatchCoinTen;						//获得十元夺宝用户uid
	int64							m_WinScoreSnatchCoinHundred;					//获得百元夺宝用户uid

	uint64							m_lLastTimeRobotOperSnatchCoinTen;				//机器人最后操作时间
	uint64							m_lLastTimeRobotOperSnatchCoinHundred;			//机器人最后操作时间
	uint32							m_uSnatchCoinRobotWinPre;						//夺宝机器人赢概率
	uint32							m_uSnatchCoinRobotCanWinTen;					//夺宝机器人赢还是输 0 肯定输 1 肯定赢  2 不使用
	uint32							m_uSnatchCoinRobotCanWinHundred;				//夺宝机器人赢还是输
public:
	// 重置游戏数据十元夺宝
	void    ResetGameDataTen();
	// 重置游戏数据百元夺宝
	void    ResetGameDataHundred();
	//初始化洗牌
	void    InitRandCardTen();
	//初始化洗牌
	void    InitRandCardHundred();
	//牌池洗牌
	void 	RandPoolCardTen();
	//牌池洗牌
	void 	RandPoolCardHundred();
	//摸一张牌
	BYTE    PopCardFromPoolTen();
	//摸一张牌
	BYTE    PopCardFromPoolHundred();
	//进入夺宝
	bool    OnUserEnterSnatchCoin(CGamePlayer* pPlayer, BYTE cbType);
	//夺宝事件
	bool    OnUserSnatchCoin(CGamePlayer* pPlayer, BYTE cbType, BYTE cbCount);
	//是否第一次夺宝成功
	bool    IsExistSnatchCoinTenSuccess(uint32 uid);
	//是否第一次夺宝成功
	bool    IsExistSnatchCoinHundredSuccess(uint32 uid);
	// 游戏开始
	bool OnGameStartTen();
	// 游戏开始
	bool OnGameStartHundred();
	// 发送夺宝游戏状态
	bool SendSnatchCoinGameStatus(BYTE cbType);
	// 游戏结束
	bool OnGameEndTen(uint16 chairID, uint8 reason);
	// 游戏结束
	bool OnGameEndHundred(uint16 chairID, uint8 reason);

	void    InitBlingLogTen();
	void    InitBlingLogHundred();
	void    AddUserBlingLogTen(CGamePlayer* pPlayer);
	void    AddUserBlingLogHundred(CGamePlayer* pPlayer);
	//写期数日志
	void    WritePeriodsInfoTen();
	//写期数日志
	void    WritePeriodsInfoHundred();

	void    WriteCardDataInfoTen();

	void    WriteCardDataInfoHundred();

	uint32  GetPlayerType(BYTE cbType, uint32 uid);

	void    WriteSnatchCoinCardData(CGamePlayer * pPlayer, BYTE cbType);


	//十元夺宝开奖
	void    OpenLotterySnatchCoinTen();
	void	OpenLotterySnatchCoinHundred();
	void    CalculateSnatchCoinTen();
	void	CalculateSnatchCoinHundred();

	void    FlushUserBlingLogTen(CGamePlayer* pPlayer,int64 win);
	void    FlushUserBlingLogHundred(CGamePlayer* pPlayer, int64 win);
	void    FlushUserBlingLogByUidTen(uint32 uid, int64 winScore, int64 fee);
	void    FlushUserBlingLogByUidHundred(uint32 uid, int64 winScore, int64 fee);
	void    SaveBlingLogTen();
	void    SaveBlingLogHundred();
	void	SaveUserBankruptScoreTen();
	void	SaveUserBankruptScoreHundred();


	//机器操作
	void	GetAllRobotPlayer(vector<CGamePlayer*> & robots);

	void	OnRobotSnatchCoin(BYTE cbType, BYTE cbCount,int iRobotCount,vector<CGamePlayer*> & robots);

	void    OnRobotOperSnatchCoinTen();

	void    OnRobotOperSnatchCoinHundred();

	bool	RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount);

	void    ExcRobotSnatchCoin(BYTE cbType, BYTE cbCount, uint32 robotcount, vector<CGamePlayer*> & robots);

	//夺宝控制
	//十元
	bool	SetSnatchCoinRandOpenCardTen();

	bool    SetRobotSnatchCoinLostTen(vector<BYTE> & vecRobotCards);

	bool	SetRobotSnatchCoinWinTen(vector<BYTE> & vecRobotCards);

	bool	SetSnatchCoinPreOpenCardTen(vector<BYTE> & vecRobotCards);
	//百元
	bool	SetSnatchCoinRandOpenCardHundred();

	bool    SetRobotSnatchCoinLostHundred(vector<BYTE> & vecRobotCards);

	bool	SetRobotSnatchCoinWinHundred(vector<BYTE> & vecRobotCards);

	bool	SetSnatchCoinPreOpenCardHundred(vector<BYTE> & vecRobotCards);


	//停止夺宝
	bool	SetSnatchCoinState(uint32 stop);

	void	SendSnatchCoinGameState(uint32 status);

	bool	SaveSnatchCoinGameData();

	bool	LoadSnatchCoinGameData(bool & bReTen, bool & bReHundred);

	//-------------------------------------------------------------------------------

public:
	uint32							m_uGameCount;					//游戏局数
	int								m_iFlowerColorTax;				//花色税收
	int								m_ibigSmallTax;					//大小税收
	int								m_ibigSmallSwitch;				//大小开关
	int64							m_lJackpotFlower;				//花色奖池
	int64							m_lFrontJackpotBigSmall;		//上轮大小奖池
	int64							m_lJackpotBigSmall;				//大小奖池
	int64							m_lRandMaxScore;				//随机上限
	int64							m_lRandMinScore;				//随机下限
	int								m_iSysWinPro;					//吃币
	int								m_iSysLostPro;					//吐币
	string							m_strPeriods;					//期数

	int32							m_uSysLostWinProChange;			// 吃币吐币概率变化
	int64							m_lUpdateJackpotScore;			// 更新奖池分数

};

#endif //SERVER_GAME_IMPLE_TABLE_H

