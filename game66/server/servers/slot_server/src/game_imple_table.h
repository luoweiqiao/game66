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
#include "slot_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_slot;

class CGamePlayer;
class CGameRoom;

struct tagSlotRobotWinJackpotScore
{
	int64 lMinJackpotScore;
	int64 lMaxJackpotScore;
	uint32 uProHit_777;
	uint32 uProHit_7777;
	uint32 uProHit_77777;
	tagSlotRobotWinJackpotScore()
	{
		lMinJackpotScore = 0;
		lMaxJackpotScore = 0;
		uProHit_777 = 0;
		uProHit_7777 = 0;
		uProHit_77777 = 0;
	}
};

static const int iMaxMasterRandProCount = 7;

struct tagSlotMasterCtrlInfo
{
	uint32 suid;
	uint32 uid;
	int64 lMinMultiple;		// 最低倍数
	int64 lMaxMultiple;		// 最高倍数
	int64 lRanMinMultiple;	// 最低倍数
	int64 lRanMaxMultiple;	// 最高倍数
	int   iAllLostCount;	// 连输次数
	int   iLostCount;		// 连输次数
	int   iAllWinCount;		// 连赢次数
	int   iWinCount;		// 连赢次数
	int	  iFreeSpin;		// 免费次数
	int	  jackpotIndex;		// 彩金倍数
	int   iAllPreCount;		// 执行次数
	int   iPreCount;		// 执行次数

	tagSlotMasterCtrlInfo()
	{
		Init();
	}
	void Init()
	{
		suid = 0;
		uid = 0;
		lMinMultiple = 0;
		lMaxMultiple = 0;
		lRanMinMultiple = 0;
		lRanMaxMultiple = 0;
		iAllLostCount = 0;
		iLostCount = 0;
		iAllWinCount = 0;
		iWinCount = 0;
		iFreeSpin = 0;
		jackpotIndex = 0;
		iAllPreCount = 0;
		iPreCount = 0;
		
	}
};

struct tagSlotMasterShowPlayerInfo
{
	uint32 uid;
	uint32 attac_line;
	uint32 attac_score;
	uint32 lost_count;
	uint32 win_count;
	tagSlotMasterShowPlayerInfo()
	{
		Init();
	}
	void Init()
	{
		uid = 0;
		attac_line = 0;
		attac_score = 0;
		lost_count = 0;
		win_count = 0;
	}
};
//用户游戏数据
typedef struct
{
	uint32 	CurLineNo;  //当前押注的线号
	uint32	CurBetScore;//当前押注的金币ID
	uint32 	FreeTimes; //当前免费的次数
	int64   Free_winscore;//免费赢金币和
	bool bIsMasterCtrl;
}FruitUserData;

// 游戏桌子
class CGameSlotTable : public CGameTable
{
public:
	CGameSlotTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameSlotTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);
	virtual bool    IsFullTable();
	virtual bool    ReAnalysisParam();
public:
    //配置桌子
    virtual bool Init();
    virtual void ShutDown();

    //复位桌子
    virtual void ResetTable();
    virtual void OnTimeTick();
    //游戏消息
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
 
public:
    //// 游戏开始
    //virtual bool OnGameStart();
    //// 游戏结束
    //virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    ////用户同意
    //virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
	//virtual int64	GetJettonScore(CGamePlayer* pPlayer);
	//virtual bool	UpdateMaxJettonScore(CGamePlayer* pPlayer, int64 lScore);

	virtual bool	StopServer();
public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    int64    BroadcasPlayerInfo(uint32 uid, uint16 actType,int64 winScore);
    // 重置游戏数据
    void    ResetGameData();
	// 游戏开始摇一摇
	bool OnGameUserSpin(CGamePlayer* pPlayer, uint32 nline, uint32 nbet);
	// 免费转
	bool OnGameUserFreeSpin(CGamePlayer* pPlayer);
	//计算相连的个数
	int64 GetLineMultiple(uint32 GamePic[MAXCOLUMN], picLinkinfo &picLink);
	// 更新用户押注数据
	void AddFruitUserData(uint32 uid, FruitUserData & UserData);
	// 删除用户押注数据
	void DelFruitUserData(uint32 uid);
	// 获取对应的图片集
	void GetSpinPics(CGamePlayer* pPlayer,uint32 nline, uint32 nbet, uint32 mGamePic[]);
	//根据权重算法,获取拿到彩金库存比例
	uint32 GetWeightJackpotIndex();
	// 获取有彩金生成概率
	int64 GetWinJackpotScore(uint32 nline, uint32 nbet, uint32  nIndex);
	//显示打印
	void display();
	int64 GetWinHandselGameCount() {
		return m_lWinHandselGameCount;
	}
	bool ProgressMasterCtrl(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int & out_hitIndex);
	bool ProgressControlPalyer(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[]);
	bool ProgressNoviceWelfareCtrl(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[]);
	bool ProgressJackpotScore(CGamePlayer *pPlayer, uint32 nline, uint32 nbet,int & hitIndex, uint32 mGamePic[][MAXCOLUMN]);
	bool CheckUserSpinLine(CGamePlayer* pPlayer, uint32 nline, uint32 nbet);
	uint32 GetRobotLineCount(CGamePlayer* pPlayer);
	uint32 GetRobotJettonScore(CGamePlayer* pPlayer);

	bool SetControlPalyerWin(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int64 & out_winscore,int64 & lWinMultiple);

	bool SetControlPalyerFreeCount(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int freeCount, int64 lMinWinMultiple,int64 lMaxWinMultiple);

	bool SetControlPalyerLost(uint32 nline, uint32 mGamePic[]);

	bool ProgressActiveWelfareCtrl(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[]);

	bool GetPalyerMaxWinScorePic(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int64 & out_winscore);

	void TatalLineUserBet(CGamePlayer* pPlayer, uint32 nline, uint32 nbet, int64 lWinScore);

	void GetRandMutail(tagSlotMasterCtrlInfo & tempInfo);

//机器人操作
protected:
	void            OnRobotOper();
	void			GetAllRobotPlayer(vector<CGamePlayer*> & robots);
	uint32			GetMinJettonScore();

	//TLock           m_tl;//共享锁
	CCooling        m_coolRobot;                    // 机器人CD

	CCooling		m_coolRobotLooker; // 机器人CD
	CCooling		m_coolRobotChair; // 机器人CD

protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
	//初始化配置
	void InitSlotParam();
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
    // 写入加注log
    void    WriteAddScoreLog(uint16 chairID,int64 score);
	// 写入图片信息
	void   WritePicLog(uint32 GamePic[], uint32 picCount);
    uint16  GetPlayNum();
	//结算
	int64   CalcPlayerInfo(CGamePlayer* pPlayer, int64 lJettonScore, int64 winScore, int64 lJackpotScore);
	int64 GetPlayerCurSortScore(uint32 uid);
	bool SortFrontPlayerByScore(CGamePlayer* pPlayer, bool isJoin);
	void SendSortPlayerByScore(CGamePlayer* pSendPlayer);

	void SendMasterPlayerInfo(CGamePlayer* pPlayer);
	void SendMasterShowCtrlInfo(CGamePlayer* pPlayer,struct tagSlotMasterCtrlInfo tagInfo);

	void SendAllMasterUser(const google::protobuf::Message* msg, uint32 msg_type);

	void RefreshAllMasterPlayerUser(uint32 type, CGamePlayer* pPlayer);

	// 获取一次随机图组赢分  add by har
	// nline : 线的数量
	// nbet : 单条线的押注数
	// mGamePic out : 保存随机生成的图组 
    // return  随机生成的图组的赢分
	int64 GetRandomWinScore(uint32 nline, uint32 nbet, uint32 mGamePic[]);
	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[]);
	//游戏变量
protected:
	BYTE							m_bOperaCount;							//操作次数
    uint16                          m_wCurrentRound;                        //当前轮次
	bool							m_bSaveRoomParam;
	void							SaveRoomParam(int flag);
	std::vector<uint32>				m_vecSortFront;
protected:
	CSlotLogic                     m_gameLogic;                            //游戏逻辑
	bool                           m_bBroadcast;                            //游戏是否接收广播
	int64	                       m_lBroadcastWinScore;                    //发送广播需要的金币数量
	//uint32                       m_nGamePic[MAXROW][MAXCOLUMN];          //水果机矩阵3*5
	ServerLinePosVecType           m_oServerLinePos;                       //线对应的点
	PicRateConfigVecType           m_oPicRateConfig;                       //图片对应的赔率
	int64                          m_lSysScore;                            //普通库存
	int64                          m_lFeeScore;                            //抽水库存
	int64                          m_lJackpotScore;                        //彩金库
	uint32                         m_nCurLineNo;                           //当前押注的线号
	uint32                         m_nCurBetScore;                         //当前押注的金币
	uint32                         m_nFreeTimes;                           //当前免费的次数
	int64                          m_FeeRate;                            //抽水比例
	int64                          m_JackpotRate;                        //彩金比例
	int32                          m_ReUserRate[2];                      //个人返奖比例
	int32                          m_ReSysRate[5];                       // 系统返奖比例
	vector<int64>				   m_oSysRepertoryRange;                  //系统库存区间配置
//彩金配置
	int32                          m_nJackpotChangeRate;                  //彩金库存变化率(万分比)
	int32                          m_nJackpotChangeUnit;                  //彩金库存变化单位
	int32                          m_MinJackpotReviseRate;                //彩金库存最小修正率(万分比)
	int32                          m_MaxJackpotReviseRate;                //彩金库存最大修正率(万分比)
	int64                          m_JackpotBetRange[2];                     //获取彩金押注范围条件
	int64                          m_JackpotRangeScore[2];                  //获得彩金范围
	vector<uint32>                 m_VecJackpotWeigh;                       //彩金权重
	vector<int32>                  m_VecJackpotRatio;                       //彩金比例
	int64						   m_lWinHandselGameCount;					//赢彩金游戏局数
	uint32						   m_uWinHandselPre;						//赢彩金概率

	std::vector<tagSlotRobotWinJackpotScore> m_vecRobotWinJackpotScore;
//用户数据
	map<uint32, FruitUserData>    m_oFruitUserData;                        //用户游戏押注相关信息

	vector<uint32>                 m_vecUserJettonCellScore;               //用户能够下注的分数

	map<uint32, tagSlotMasterCtrlInfo>    m_mpMasterCtrlInfo;
	map<uint32, tagSlotMasterShowPlayerInfo>    m_mpMasterPlayerInfo;
	vector<uint32> m_vecMasterList;
	int m_iArrMasterRandProCount[iMaxMasterRandProCount];
	
};

#endif //SERVER_GAME_IMPLE_TABLE_H

