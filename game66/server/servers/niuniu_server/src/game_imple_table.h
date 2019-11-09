
// 桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "niuniu_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_niuniu;

class CGamePlayer;
class CGameRoom;

struct tagRobotOperItem
{
	uint32 uid;
	uint16 chairID;
	bool bflag;
	int64 count;
	int64 time;	
	tagRobotOperItem()
	{
		uid = 0;
		chairID = 255;
		bflag = false;		
		count = 0;
		time = 0;
	}
};

// 游戏桌子
class CGameNiuniuTable : public CGameTable
{
public:
	CGameNiuniuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameNiuniuTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

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
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //用户同意
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    int64    CalcPlayerInfo(uint16 chairID,int64 winScore);
    // 重置游戏数据
    void    ResetGameData();
	void OnRobotTick();
	bool   OnSendMasterCard();
protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int64 score);
    // 写入庄家位log
    void    WriteBankerLog(uint16 chairID);
	void    WriteGameInfo();

    // 是否能够开始游戏
    bool    IsCanStartGame();
    // 检测筹码是否正确
    bool    CheckJetton(uint16 chairID,int64 score);
    // 获得机器人的铁壁
    int64   GetRobotJetton(uint16 chairID);

    uint16  GetPlayNum();

    void    InitBanker();
    void    SendCardToClient();

    // 结算分数
	// szWinScore : 保存赢分  add by har
	// isComputeFee : 是否计算抽水  add by har
	// return : 非机器人玩家总赢分  add by har
    int64 CalculateScore(int64 szWinScore[GAME_PLAYER], bool isComputeFee);

    // 检测提前结束
    void    CheckOverTime();
	// 检查挂机玩家踢出
    void    CheckNoOperPlayerLeave();
	void	CheckPlayerScoreLessNotify();
	void	CheckPlayerScoreLessLeave();
	void	CheckPlayerScoreManyLeave();
	// 做牌发牌
	void	DispatchCard();

    //游戏事件
protected:
	//游戏状态
	bool    IsUserPlaying(WORD wChairID);
    
	//加注事件
    bool    OnUserAddScore(WORD wChairID, int64 lScore);

	bool    OnMasterUserOper(CGamePlayer* pPlayer, vector<BYTE> vecChairID, vector<BYTE> vecCardData);


    //申请庄家
    bool    OnUserApplyBanker(WORD wChairID,int32 score);
    //摆牌
    bool    OnUserChangeCard(WORD wChairID,BYTE cards[]);
    //发送摆牌
    void    SendChangeCard(WORD wChairID,CGamePlayer* pPlayer);
    int32   GetChangeCardNum();

    //机器人操作
protected:

    bool    OnRobotReady();
    bool    OnRobotChangeCard();

    void    CheckAddRobot();
    void    SetRobotThinkTime();

    //设置机器人赢金币
    bool    SetRobotWinCard();

	void    SetCardDataControl();

	bool    SetControlPalyerWin(uint32 control_uid);

	bool    SetControlPalyerLost(uint32 control_uid);

	bool	SetPlayerWinScore();

	bool	SetPlayerLostScore();

	// 是否有用户超出下注
	bool HaveOverWelfareMaxJettonScore(int64 newmaxjetton);
	// 有不是福利玩家
	CGamePlayer* HaveWelfareNovicePlayer();
	int64 GetNoviceWinScore(uint32 noviceuid);
	
	// 活跃福利控制
    bool ActiveWelfareCtrl();
	
	bool SetControlPalyerWinForAW(uint32 control_uid, int64 max_win);

	// 幸运值控制---设置输家的牌型
	bool SetLostForLuckyCtrl(uint32 control_uid);

	// 幸运值控制
	bool SetLuckyCtrl();

	// 设置库存输赢  add by har
	// return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();

protected:
	int64							m_lJettonMultiple[JETTON_MULTIPLE_COUNT];//下注倍数
	int								m_iApplyBankerPro[NIU_MULTIPLE_COUNT];//
	int								m_iRobotJettonPro[JETTON_MULTIPLE_COUNT];//

	int32							m_iArrDispatchCardPro[CT_SPECIAL_MAX_TYPE];
	bool							m_bIsNoviceWelfareCtrl;// 是否有福利控制
	bool							m_bIsProgressControlPalyer;
	bool							m_bIsMasterUserOper;
protected:
	int64 GetPlayerAddScore(uint32 chairID);
	int64 GetPlayerRobMultiple(uint32 chairID);

	void	SendReadyStartGame();
	uint32	GetPlayGameCount();
	void    SendFourCardToClient();
	int     GetProCardType();
	bool	ProbabilityDispatchPokerCard();
	void	OnRobotApplyBankerScore(uint16 chairID);
	void	OnRobotJettonScore(uint16 chairID);
	//bool    OnRobotApplyBanker();
	bool	OnTimeOutApplyBanker();
	
	bool SetRobotBrankerCanWinScore();
	bool SetPlayerBrankerLostScore();
	bool OnRobotBrankerSubPlayerCardType();
	bool OnPlayerBrankerSubPlayerCardType();

	bool ProCtrlRobotWinScore(uint32 robotWinPro);
	bool NoviceWelfareCtrlWinScore();
	bool OnTimeOutChangeCard();
	bool ProgressControlPalyer();

	void OnRobotReadyApplyBanker();
	void OnRobotRealApplyBankerScore(uint16 chairID);
	bool OnRobotInApplyBanker();
	int64 GetRobotInApplyMultiple(uint16 chairID);

	void OnRobotReadyJetton();	
	void OnRobotRealJettonScore(uint16 chairID);
	bool OnRobotInJetton();
	int64 GetRobotInJettonMultiple(uint16 chairID);
	
	//游戏变量
protected:
	WORD							m_wBankerUser;							//庄家用户
    bool                            m_isNeedBanker;                         //是否需要庄家用户
	uint16							m_cbBrankerSettleAccountsType;			//庄家结算类型
	std::vector<tagRobotOperItem>	m_vecRobotApplyBanker;
	std::vector<tagRobotOperItem>	m_vecRobotJetton;
	//用户状态
protected:
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//游戏状态
    BYTE                            m_szApplyBanker[GAME_PLAYER];           //申请庄家状态
    BYTE                            m_szShowCardState[GAME_PLAYER];         //摆牌状态
	BYTE							m_cbTimeOutShowCard[GAME_PLAYER];
    BYTE                            m_szNoOperCount[GAME_PLAYER];           //无操作状态计数
	BYTE                            m_szNoOperTrun[GAME_PLAYER];           //无操作状态计数

	//扑克变量
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][NIUNIU_CARD_NUM];       //桌面扑克
	BYTE							m_cbChangeCardData[GAME_PLAYER][NIUNIU_CARD_NUM];       //桌面扑克

	//下注信息
protected:
	int64                           m_lRobMultiple[GAME_PLAYER];            //抢庄倍数

    int64						    m_lTableScore[GAME_PLAYER];				//下注数目
    BYTE                            m_cbTableCardType[GAME_PLAYER];         //桌面牌型
    BYTE                            m_winMultiple[GAME_PLAYER];             //输赢倍数
    int64                           m_lWinScore[GAME_PLAYER];               //输赢分数
    int64                           m_lTurnMaxScore[GAME_PLAYER];           //最大下注
	vector<BYTE>					m_vecChangerChairID;
protected:
    CNiuNiuLogic                    m_gameLogic;                            //游戏逻辑
    CCooling                        m_coolRobot;                            //机器人CD
	uint32                          m_robotWinPro;							//机器人赢概率

	bool                            m_isNeedCheckStock; // 是否需要检查库存

};

#endif //SERVER_GAME_IMPLE_TABLE_H

