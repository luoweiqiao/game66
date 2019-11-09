
#ifndef __GAME_TABLE_H__
#define __GAME_TABLE_H__

#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "center_log.h"
#include "json/json.h"
#include "poker/poker_logic.h"
#include "db_struct_define.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;
class CGameRoom;

enum BRANKER_SETTLE_ACCOUNTS_TYPE
{
	BRANKER_TYPE_NULL				= 0,	// 正常结算
	BRANKER_TYPE_TAKE_ALL			= 1,	// 庄家通杀
	BRANKER_TYPE_COMPENSATION		= 2,	// 庄家通赔
};

enum GAME_END_TYPE
{
    GER_NORMAL      = 0,		//常规结束
    GER_DISMISS,		        //游戏解散
    GER_USER_LEAVE,	            //用户强退
    GER_NETWORK_ERROR,	        //网络中断

};

enum GAME_CONTROL_PALYER_TYPE
{
	GAME_CONTROL_CANCEL			= 0,	// 取消控制
	GAME_CONTROL_WIN			= 1,	// 控制赢
	GAME_CONTROL_LOST			= 2,	// 控制输
};

enum GAME_CONTROL_MULTI_PALYER_TYPE
{
	GAME_CONTROL_MULTI_CANCEL	= 0,	// 取消控制
	GAME_CONTROL_MULTI_WIN		= 1,	// 控制赢
	GAME_CONTROL_MULTI_LOST		= 2,	// 控制输
	GAME_CONTROL_MULTI_ALL_CANCEL = 3,	// 取消全部控制
};


#define CONTROL_TIME_OUT		(5*60*1000)		//控制超时(5分钟)
#define BRC_MAX_CONTROL_AREA	(20)			//所有百人场最大的下注区域值

struct tagJackpotScore
{
	int64	lMaxPollScore;		//最大奖池分数
	int64	lMinPollScore;		//最小奖池分数
	int64	lCurPollScore;		//当前奖池分数
	int64	lDiffPollScore;		//奖池分数变化
	uint32	uSysWinPro;			//系统赢概率
	uint32	uSysLostPro;		//系统输概率
	uint32	uSysLostWinProChange; //吃币吐币概率变化
	int64	lUpdateJackpotScore;// 更新奖池分数
	int		iUserJackpotControl; //是否使用奖池控制 1使用 否则不适用
	uint64	ulLastRestTime;		//最后重置时间
	int tm_min;   // minutes after the hour - [0, 59]
	int tm_hour;  // hours since midnight - [0, 23]

	tagJackpotScore()
	{
		Init();
	}
	void Init()
	{
		lMaxPollScore = 0;
		lMinPollScore = 0;
		lCurPollScore = 0;
		lDiffPollScore = 0;
		uSysWinPro = 0;
		uSysLostPro = 0;
		uSysLostWinProChange = 0;
		lUpdateJackpotScore = 0;
		iUserJackpotControl = 0;
		ulLastRestTime = 0;
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

struct tagRangeWelfare
{
	uint32 minpos;
	uint32 maxpos;
	int64 smaxjetton;
	int64 smaxwin;
	tagRangeWelfare()
	{
		init();
	}
	void init()
	{
		minpos = 0;
		maxpos = 0;
		smaxjetton = 0;
		smaxwin = 0;
	}
};
struct tagNewPlayerNoviceWelfare
{
	int newroom; // 是否是新手福利房间 百家乐 百人牛牛 牌九 红黑大战 龙虎斗
	int isnewnowe;//是否有新手福利功能 水果机 斗牛
	int64 newmaxjetton; //新手用户最大下注
	int64 newsmaxwin; // 新手的单局最高盈利
	int	maxjettonrate; // 该房间的最大下注倍率
	uint32 nobbw; // 新手机器人赢概率 百人牛牛 牌九
	uint32 normc; // 新手机器人最大牌概率 百人牛牛 
	uint32 nopbl; // 新手真实玩家当庄输概率 百家乐 牌九
	uint32 nosbw; // 新手福利房机器人系统庄家赢的概率 红黑大战 龙虎斗
	
	vector<uint32> newpayrange;
	vector<tagRangeWelfare> welfarerange;
	tagNewPlayerNoviceWelfare()
	{
		init();
	}
	void init()
	{
		newroom = 0;
		isnewnowe = 0;
		newmaxjetton = 0;
		newsmaxwin = 0;
		maxjettonrate = 0;
		nobbw = 0;
		normc = 0;
		newpayrange.clear();
		welfarerange.clear();
	}
};

struct tagAutoKillScore
{
	int autokillsocre;
	int32   defeat;     // 控制局数
	int32   interval;   // 不控制局数
	int32   state;       // 1 正在杀分 0 正常控制
	uint32  aobbw; //杀分机器人赢概率 百人牛牛 牌九
	uint32 aormc; //杀分机器人最大牌概率 百人牛牛
	uint aopbl; // 杀分真实玩家当庄输概率 百家乐 牌九
	uint32 aosbw;//杀分系统庄家赢率 龙虎斗 红黑大战
	uint32 aorbw; //杀分机器人庄家胜率 骰宝
	tagAutoKillScore()
	{
		init();
	}
	void init()
	{
		autokillsocre = 0;
		defeat = 0;
		interval = 0;
		state = 1;
		aobbw = 0;
		aormc = 0;
		aopbl = 0;
		aosbw = 0;
		aorbw = 0;
	}
};
// 座位信息
struct stSeat
{
    CGamePlayer* pPlayer;
    uint8        readyState;  // 准备状态
    uint8        autoState;   // 托管状态
    uint32       readyTime;   // 准备时间
    uint8        overTimes;   // 超时次数
    uint32       uid;         // 德州梭哈用户游戏结束前退出用uid处理
    int64        buyinScore;  // buyin游戏币
    stSeat(){
        Reset();
    }
    void Reset(){
        memset(this,0,sizeof(stSeat));
    }
};

// 活跃福利信息
struct tagAWPlayerInfo
{
    uint32   uid;
    int64    max_win;
    int64    sum_loss;
	uint32   probability;
    tagAWPlayerInfo()
    {
        uid = 0;
        max_win = 0;
        sum_loss = 0;
		probability = 0;
    }
};

// 桌子类型
enum TABLE_TYPE
{
    emTABLE_TYPE_SYSTEM  = 0, // 系统桌子
    emTABLE_TYPE_PLAYER  = 1, // 玩家桌子
};

// 桌子类型
enum BRC_TABLE_STATUS
{
	emTABLE_STATUS_FREE = 1,	// 空闲
	emTABLE_STATUS_START = 2,	// 开始
	emTABLE_STATUS_END = 3,		// 结束
};

// 桌子信息
struct stTableConf
{
    string tableName;   // 桌子名字
    string passwd;      // 密码
    string hostName;    // 房主名字
    uint32 hostID;      // 房主ID
    uint8  deal;        // 发牌方式
    int64  baseScore;   // 底分
    uint8  consume;     // 消费类型
    int64  enterMin;    // 最低进入
    int64  enterMax;    // 最大带入
    uint8  isShow;      // 是否显示
    uint8  feeType;     // 台费类型
    int64  feeValue;    // 台费值
    uint32 dueTime;     // 到期时间
    uint16 seatNum;    	// 座位数
    uint16 uproom;		// 是否赶场，0不敢场 1强制赶场 2非强制敢场
    uint32 exitchip;	// 赶场筹码

    void operator=(const stTableConf& conf)
    {
        hostID    = conf.hostID;
        tableName = conf.tableName;
        passwd    = conf.passwd;
        hostName  = conf.hostName;
        hostID    = conf.hostID;
        deal      = conf.deal;
        baseScore = conf.baseScore;
        consume   = conf.consume;
        enterMin  = conf.enterMin;
        enterMax  = conf.enterMax;
        isShow    = conf.isShow;
        feeType   = conf.feeType;
        feeValue  = conf.feeValue;
        dueTime   = conf.dueTime;
        seatNum   = conf.seatNum;      // 座位数
    }
    stTableConf(){
        hostID    = 0;
        tableName = "送金币";
        passwd    = "";
        hostName  = "隔壁老王";
        hostID    = 0;
        deal      = 0;
        baseScore = 0;
        consume   = 0;
        enterMin  = 0;
        enterMax  = 0;
        isShow    = 0;
        feeType   = 0;
        feeValue  = 0;
        dueTime   = 0;
        seatNum   = 0;  // 座位数
    }
};

struct tagMasterShowUserInfo
{
	int64 lMinScore;
	int64 lMaxScore;
	tagMasterShowUserInfo()
	{
		lMinScore = 300000;
		lMaxScore = 1000000;
	}
};

// 游戏桌子
class CGameTable
{
public:
    CGameTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameTable();

    void            SetTableID(uint32 tableID){ m_tableID = tableID; }
    uint32          GetTableID(){ return m_tableID; };
	uint16			GetRoomID();
	uint16			GetGameType();
    uint8           GetTableType(){ return m_tableType; }
    CGameRoom*      GetHostRoom();

    virtual bool    EnterTable(CGamePlayer* pPlayer);
	virtual bool    EnterEveryColorTable(CGamePlayer* pPlayer);
    virtual bool    LeaveTable(CGamePlayer* pPlayer,bool bNotify = false);
    // 从新匹配
    virtual void    RenewFastJoinTable(CGamePlayer* pPlayer);

    virtual bool    AddLooker(CGamePlayer* pPlayer);
    virtual bool    RemoveLooker(CGamePlayer* pPlayer);
	virtual bool    AddEveryColorLooker(CGamePlayer* pPlayer);
	virtual bool    RemoveEveryColorLooker(CGamePlayer* pPlayer);

    virtual bool    IsExistLooker(uint32 uid);
    
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
    virtual bool    CanSitDown(CGamePlayer* pPlayer,uint16 chairID);
    virtual bool    CanStandUp(CGamePlayer* pPlayer);
    virtual bool    NeedSitDown();
    virtual void    SetRobotEnterCool(uint32 time){ m_robotEnterCooling.beginCooling(time); }

    virtual bool    PlayerReady(CGamePlayer* pPlayer);
    virtual bool    PlayerSitDownStandUp(CGamePlayer* pPlayer,bool sitDown,uint16 chairID);
	virtual bool    ForcePlayerSitDownStandUp(CGamePlayer* pPlayer, bool sitDown, uint16 chairID);

    virtual bool    ResetPlayerReady();
    virtual bool    IsAllReady();
    virtual bool    PlayerSetAuto(CGamePlayer* pPlayer,uint8 autoType);
    virtual bool    IsReady(CGamePlayer* pPlayer);
    virtual void    ReadyAllRobot();
    virtual bool    IsExistIP(uint32 ip);
    virtual bool    IsExistBlock(CGamePlayer* pPlayer);

    virtual uint32  GetPlayerNum();
    virtual uint32  GetChairPlayerNum();
    virtual uint32  GetOnlinePlayerNum();
    virtual uint32  GetReadyNum();
    virtual bool    IsFullTable();
    virtual bool    IsEmptyTable();
    virtual uint32  GetFreeChairNum();

    virtual uint32  GetChairNum();
    virtual bool    IsAllDisconnect();

    virtual void    SetGameState(uint8 state);
    virtual uint8   GetGameState();
    virtual CGamePlayer* GetPlayer(uint16 chairID);
	CGamePlayer * GetGamePlayerByUid(uint32 uid);

    virtual uint32  GetPlayerID(uint16 chairID);
    virtual uint16  GetChairID(CGamePlayer* pPlayer);
    virtual uint16  GetRandFreeChairID();

    virtual void    SendMsgToLooker(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToClient(uint16 chairID,const google::protobuf::Message* msg,uint16 msg_type);

    virtual void    SendTableInfoToClient(CGamePlayer* pPlayer);
    virtual void    SendReadyStateToClient();
    virtual void    SendSeatInfoToClient(CGamePlayer* pGamePlayer = NULL);
    virtual void    SendLookerListToClient(CGamePlayer* pGamePlayer = NULL);
	virtual void    SendPalyerLookerListToClient(CGamePlayer* pGamePlayer = NULL);
    virtual void    NotifyPlayerJoin(CGamePlayer* pPlayer,bool isJoin);
    virtual void    NotifyPlayerLeave(CGamePlayer* pPlayer);
    
    virtual int64   ChangeScoreValue(uint16 chairID,int64& score,uint16 operType,uint16 subType);
    virtual int64   ChangeScoreValueByUID(uint32 uid,int64& score,uint16 operType,uint16 subType);
	virtual int64   ChangeScoreValueInGame(uint32 uid, int64& score, uint16 operType, uint16 subType, string chessid);
	virtual int64   NotifyChangeScoreValueInGame(uint32 uid, int64 score, uint16 operType, uint16 subType);

    virtual uint8   GetConsumeType();
    virtual int64   GetBaseScore();
    virtual int64   GetEnterMin();
    virtual int64   GetEnterMax();
    virtual uint32  GetExitChip();
    virtual uint16  GetUproom();

    virtual uint32  GetDeal();
    virtual bool    IsShow();
    virtual uint32  GetHostID();
    virtual bool    IsRightPasswd(string passwd);

	virtual int64   GetJettonMin();
	virtual bool	TableJettonLimmit(CGamePlayer * pPlayer,int64 lJettonScore, int64 lAllyJetton);

    virtual int64   GetPlayerCurScore(CGamePlayer* pPlayer);
    virtual int64   ChangePlayerCurScore(CGamePlayer* pPlayer,int64 score);
	virtual bool	StopServer();

    void    SetTableConf(stTableConf& conf){ m_conf = conf; }
    stTableConf& GetTableConf(){ return m_conf; }

    // 私人房操作
    virtual void    RenewPrivateTable(uint32 days);
    virtual void    LoadPrivateTable(stPrivateTable& data);
    virtual bool    CreatePrivateTable();
    virtual void    ChangePrivateTableIncome(int64 hostIncome,int64 sysIncome);
    virtual void    UpdatePrivateTableDuetime();
    // 牌局日志
	void InitChessID();
	void SetEmptyChessID();
    virtual void    InitBlingLog(bool bNeedRead = false);
	virtual void	InitBlingLog(CGamePlayer* pPlayer);
    virtual void    AddUserBlingLog(CGamePlayer* pPlayer);
    virtual void    FlushUserBlingLog(CGamePlayer* pPlayer,int64 winScore, int64 lJackpotScore = 0, uint8 land = 0);
	virtual void    FlushUserNoExistBlingLog(uint32 uid, int64 winScore, int64 fee, uint8 land = 0);
    virtual void    ChangeUserBlingLogFee(uint32 uid,int64 fee);
    virtual void    SaveBlingLog();

    // 扣除开始台费
    virtual int64    DeductStartFee(bool bNeedReady);
    // 扣除结算台费
    virtual int64    DeducEndFee(uint32 uid,int64& winScore);
        
    // 上报游戏战报
    virtual void    ReportGameResult(uint32 uid,int64 winScore,int64 lExWinScore, bool bBranker,int64 lBetScore);
    // 玩家贡献度记录
    virtual void    LogFee(uint32 uid,int64 feewin,int64 feelose);
    // 结算玩家信息
    virtual int64   CalcPlayerGameInfo(uint32 uid,int64& winScore,int64 lExWinScore = 0,bool bDeductFee = true,bool bBranker = false,int64 lBetScore = 0);
	
	// 结算玩家信息---百人场
	virtual void   CalcPlayerGameInfoForBrc(uint32 uid, int64& winScore, int64 lExWinScore = 0, bool bDeductFee = true, bool bBranker = false, int64 lBetScore = 0);

	// 获取抽水费用---百人场
	virtual int64   GetBrcFee(uint32 uid, int64 winScore, bool bDeductFee = true);
	virtual bool RobotLeavaReadJetton(uint32 uid) { return true; }

	// 结算玩家信息---捕鱼场
	virtual void   CalcPlayerGameInfoForFish(uint32 uid, int64 winScore);


public:
    // 是否需要回收
    virtual bool    NeedRecover();
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo) = 0;
	virtual int64	GetPlayerTotalWinScore(CGamePlayer *pPlayer);

public:
	void    SaveUserBankruptScore();

public:
	//配置桌子
	virtual bool    Init()         = 0;
    virtual void    ShutDown()     = 0;
    //复位桌子
    virtual void    ResetTable()   = 0;
    virtual void    OnTimeTick()   = 0;
	bool			OnTableTick();
	//游戏消息
	virtual int     OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len) = 0;
	//用户断线或重连
	virtual bool    OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin = true);
	//用户坐下
	virtual bool    OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer);
	//用户起立
	virtual bool    OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer);
	//用户同意
	virtual bool    OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);    
    //玩家进入或离开
    virtual void    OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
	//桌子游戏结束
	virtual void    OnTableGameEnd();
 public:
	 uint16			GetRobotPlayerCount();
	 //重新解析房间Param
	 virtual bool	ReAnalysisParam();
	 //更新控制玩家
	 virtual bool	UpdateControlPlayer(int64 id, uint32 uid, uint32 gamecount, uint32 operatetype);

	 virtual bool	UpdateControlMultiPlayer(int64 id, uint32 uid, uint32 gamecount, uint64 gametime,int64 totalscore, uint32 operatetype);
	 //停止夺宝
	 virtual bool	SetSnatchCoinState(uint32 stop);
	 //机器人夺宝
	 virtual bool	RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount);

	 virtual bool	DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[]);

	 virtual bool MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard);

	 //控制玩家局数同步
	 virtual bool	SynControlPlayer(uint32 uid, int32 gamecount, uint32 operatetype);
	 //获取下注分数
	 virtual int64	GetJettonScore(CGamePlayer *pPlayer = NULL);
	 //更新最大下注分数
	 virtual bool	UpdateMaxJettonScore(CGamePlayer* pPlayer = NULL, int64 lScore = 0);
	 //游戏中通知客户端更新分数
	 virtual bool	UpdataScoreInGame(uint32 uid, uint32 gametype, int64 lScore);

	 virtual void    SaveBroadcastLog(stGameBroadcastLog &BroadcastLog);   //广播日志

	 virtual void    OnNewDay();

	 virtual void    GetGamePlayLogInfo(net::msg_game_play_log* pInfo);
	 virtual void    GetGameEndLogInfo(net::msg_game_play_log* pInfo);

	 virtual uint32  GetRemainTime();

	 void    SendFirstGamePlayLogInfo(CGamePlayer *pPlayer);
	 void    GetAllGameEndLogInfo(net::msg_game_play_log_rep * prep);
	 void    SendGameEndLogInfo();

	 void CaclPlayerBaiRenCount();

	 void OnAddPlayerJetton(uint32 uid, int64 score);
	 void OnSetPlayerWinScore(uint32 uid, int64 score);
	 void OnCaclBairenCount();
	 void InitPlayerBairenCoint(CGamePlayer * pPlayer);
	 bool CalculateDeity();
	 void DivineMathematicianAndRichSitdown(std::vector<std::shared_ptr<struct tagGameSortInfo>> & vecGameSortRank);
	 static bool CompareRankByWinCount(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2);
	 static bool CompareRankByBetScore(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2);

	 bool GetControlPalyerGame(uint16 gameType);

	 void AddControlMultiPalyerCount(uint32 uid)
	 {
		 auto it_palyer = m_mpControlMultiPalyer.find(uid);
		 if (it_palyer != m_mpControlMultiPalyer.end())
		 {
			 tagControlMultiPalyer & ControlMultiPalyer = it_palyer->second;
			 ControlMultiPalyer.rccount++;
		 }
	 }

	 void OnTableGameStart();
	 void RecordPlayerBaiRenJettonInfo(CGamePlayer * pPlayer, BYTE area, int64 score);
	 bool ContinuousPressureBaiRenGame();
	 void RecordFrontJettonInfo();
	 void SendFrontJettonInfo(CGamePlayer * pPlayer);
	 void SendAllFrontJettonInfo();
	 void AddEnterScore(CGamePlayer * pPlayer);
	 virtual int64 GetEnterScore(CGamePlayer * pPlayer);
	 void RemoveEnterScore(CGamePlayer * pPlayer);
	 void UpdateEnterScore(bool isJoin,CGamePlayer * pPlayer);
	 void BuyEnterScore(CGamePlayer * pPlayer, int64 lScore);
	 bool GetIsGameEndKickUser(uint16 gameType);
	 void KickLittleScoreUser();

	 bool HaveNotNovicePlayer();
	 //bool EnterAutoKillScore(CGamePlayer * pPlayer);
	 bool EnterNoviceWelfare(CGamePlayer * pPlayer);
	 //virtual bool GetHaveKilledPlayer();
	 //bool IsHaveAutoKillScorePlayer();
	 string GetChessID();
	 void SetChessWelfare(int value);
	 int GetChessWelfare();
	 int64 GetBankruptScore();
     // 更新活跃福利信息 --- 每一局游戏结束后调用
     void UpdateActiveWelfareInfo(uint32 uid, int64 curr_win);

     // 获取活跃福利当前局所需要控制的玩家列表---排序后结果（按照亏损值从大到小排序）--- 在各子类游戏的活跃控制函数中调用
     void GetActiveWelfareCtrlPlayerList();
	 void CountGameRound(int maxcount);

	 //判断当前玩家是否可以进入新注册玩家的桌子
	 bool EnterNewRegisterWelfare(CGamePlayer * pPlayer);
	 
	 //判断当前桌子是否包含有新注册玩家
	 bool IsNewRegisterWelfareTable();

	 // 更新新注册玩家福利信息 --- 每一局游戏结束后调用
	 void UpdateNewRegisterWelfareInfo(uint32 uid, int64 curr_win);

	 //百人场精准控制
	 virtual bool   OnBrcControlPlayerEnterInterface(CGamePlayer *pPlayer);		//进入控制界面
	 virtual bool   OnBrcControlPlayerLeaveInterface(CGamePlayer *pPlayer);		//离开控制界面

	 virtual bool   OnBrcControlPlayerBetArea(CGamePlayer *pPlayer, net::msg_brc_control_area_info_req & msg);		//控制区域请求
	 virtual void   OnBrcControlGameEnd();		//百人场游戏结束后，发送实际控制区域结果

	 virtual void   OnBrcControlPlayerEnterTable(CGamePlayer *pPlayer);		//玩家进入游戏
	 virtual void   OnBrcControlPlayerLeaveTable(CGamePlayer *pPlayer);		//玩家离开游戏

	 virtual void   OnBrcControlSetResultInfo(uint32 uid, int64 win_score);	//百人场游戏结束后，修改当前玩家的统计信息---每个子类调用

	 virtual bool   OnBrcControlApplePlayer(CGamePlayer *pPlayer, uint32 target_uid);		//强制玩家下庄

	 virtual void   OnBrcControlFlushTableStatus(CGamePlayer *pPlayer = NULL);							//刷新百人场桌子状态

	 bool			OnBrcControlFlushAreaInfo(CGamePlayer *pPlayer);		//百人场控制账号刷新控制区域信息

	 uint32			IsBrcControlPlayer(uint32 uid);							//是否控制玩家 

	 uint32			GetBankerUID();
	 void			RemoveApplyBanker(uint32 uid);
	 bool			LockApplyScore(CGamePlayer* pPlayer, int64 score);
	 bool			UnLockApplyScore(CGamePlayer* pPlayer);
	 virtual void   OnBrcControlFlushAppleList();		//刷新上庄玩家列表
	 virtual bool	ChangeBanker(bool bCancelCurrentBanker) { return false; };
	 virtual void	SendApplyUser(CGamePlayer* pPlayer = NULL) {};
	 virtual void	OnNotityForceApplyUser(CGamePlayer* pPlayer) {};
	 //是否为超权用户
	 bool GetIsMasterUser(CGamePlayer* pPlayer);
	 //是否跟踪用户
	 bool GetIsMasterFollowUser(CGamePlayer* pPlayer, CGamePlayer* pTagPlayer);
	 //是否能够进入桌子
	 bool CanEnterCtrledUserTable(CGamePlayer* pGamePlayer);
	 //离开用户
	 bool LeaveMasterUser(CGamePlayer *pPlayer);
	 //初始化超权信息
	 void InitMasterUserShowInfo(CGamePlayer *pPlayer);
	 //更新超权信息
	 void UpdateMasterUserShowInfo(CGamePlayer *pPlayer, int64 lScore);
	 //更新游戏信息
	 void UpdateMasterGameInfo();
	 //是否是超权游戏
	 bool IsMasterGame();
	 //离开机器人
	 bool LeaveRobotUser();

	 // 获取下注的是否全部为机器人或全部为玩家（针对库存系统）  add by har
	 // return false 其他  true 全部为机器人或全部为玩家
	 bool IsAllRobotOrPlayerJetton();
	 // 设置最大椅子数
	 void SetMaxChairNum(uint16 maxChairNum) { m_maxChairNum = maxChairNum; };
	 
	 //获取当前桌子的幸运值标志，参数返回:赢玩家一个，输玩家为列表 一个或多个
	 bool GetTableLuckyFlag(uint32 &win_uid, set<uint32> &set_lose_uid);

	 // 获取是否全部为玩家或机器人下注
	 bool GetIsAllRobotOrPlayerJetton() { return m_isAllRobotOrPlayerJetton; }

protected:
	// 设置是否全部为玩家或机器人下注
	void SetIsAllRobotOrPlayerJetton(bool isAllRobotOrPlayerJetton) { m_isAllRobotOrPlayerJetton = isAllRobotOrPlayerJetton; }
	// 获取单个下注的是机器人还是玩家（针对库存系统）  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {};
	// 处理获取单个下注的是机器人还是玩家（针对对战类库存系统）  add by har
	// cbPlayStatus ： 玩家游戏状态
	void DealIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot, uint8	cbPlayStatus[]);
	// 处理获取单个下注的是机器人还是玩家（针对少数玩家对战类库存系统）  add by har
    // cbPlayStatus ： 玩家游戏状态
	void DealSIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
	// 检查库存改变是否成功  add by har
	bool CheckStockChange(int64 stockChange, int64 playerAllWinScore, int i);
	// 庄家是否是真实玩家  add by har
	bool IsBankerRealPlayer();
	// 百人场获取所有观众机器人（庄除外）  add by har
	void GetAllLookersRobotPlayer(vector<CGamePlayer*> &robots);
	// 百人场获取所有机器人（庄除外）  add by har
	void GetAllRobotPlayer(vector<CGamePlayer*> &robots);

	CGameRoom*                  m_pHostRoom;        // 所属房间
    stTableConf                 m_conf;             // 桌子配置
    vector<stSeat>              m_vecPlayers;       // 玩家
    map<uint32,CGamePlayer*>    m_mpLookers;        // 围观者
    uint8                       m_gameState;        // 游戏状态
    uint32                      m_tableID;          // 桌子ID
    CCooling                    m_coolLogic;        // 逻辑CD
    uint8                       m_tableType;        // 桌子类型(动态桌子，私人桌子);
    int64                       m_hostIncome;       // 桌子收益(私人房)
    int64                       m_sysIncome;        // 系统收益
    stGameBlingLog              m_blingLog;         // 牌局日志
    Json::Value                 m_operLog;          // 出牌操作LOG;
    string                      m_chessid;          // 牌局ID
    CCooling                    m_robotEnterCooling;// 机器人进入冷却
	tagControlPalyer			m_tagControlPalyer;	// 控制玩家变量
	map<uint32, tagControlMultiPalyer> m_mpControlMultiPalyer;

	//map<uint32, struct tagBairenCount> m_mpBairenPalyerBet;
	//vector<map<uint32, struct tagBairenCount>> m_vecBairenCount;

	std::map<uint32, std::shared_ptr<struct tagBairenCount>> m_mpBairenPalyerBet;
	std::vector<std::shared_ptr<std::map<uint32, std::shared_ptr<struct tagBairenCount>>>> m_vecBairenCount;

	std::map<uint32, std::map<BYTE, int64>> m_mpCurPlayerJettonInfo;
	std::shared_ptr<std::map<uint32, std::map<BYTE, int64>>> m_spFrontPlayerJettonInfo;

	map<uint32, int64> m_mpFirstEnterScore;
	int m_ctrl_welfare; // 这一局游戏是否受福利控制 0 不控制 1 控制

    vector<tagAWPlayerInfo>      m_aw_ctrl_player_list; //活跃福利---记录当前局符合活跃玩家的列表，并按照亏损值从大到小排序 --- 在各子类游戏的活跃控制函数中调用
    set<uint32>                  m_curr_bet_user;       //活跃福利---百人游戏表示当前局所有下注的玩家列表 对战游戏表示当前局所有玩家列表
    uint32                       m_aw_ctrl_uid;         //活跃福利---当前局所控的玩家ID
	int							 m_iGameCount;

	uint32                       m_nrw_ctrl_uid;        //新注册玩家福利---当前局所控的玩家ID
	uint32                       m_nrw_status;          //新注册玩家福利---当前局所控玩家的状态，用于记录日志时，进行判断

	uint16 m_maxChairNum = 0; // 最大椅子数

	//百人场精准控制
	set<CGamePlayer*>    m_setControlPlayers;       // 控制玩家列表---保存所有在控制界面
	int8                 m_control_number;			// 控制局数
	uint8				 m_req_control_area[BRC_MAX_CONTROL_AREA];	// 请求控制区域
	uint8				 m_real_control_area[BRC_MAX_CONTROL_AREA];	// 游戏结束时实际控制区域
	uint32				 m_real_control_uid;		// 实际控制玩家ID---多个玩家控制的情况下，取最后一个玩家的控制结果
	map<uint32, tagPlayerResultInfo>	m_mpPlayerResultInfo;	//所有玩家的在当前游戏的输赢结果信息

	vector<CGamePlayer*>			m_ApplyUserArray;			//申请上庄玩家列表
	map<uint32, uint8>              m_mpApplyUserInfo;          //是否自动补币
	CGamePlayer*					m_pCurBanker = NULL;				//当前庄家
	bool                            m_needLeaveBanker;          //离开庄位
	map<uint32, int64>              m_ApplyUserScore;           //申请带入积分

	uint8				m_brc_table_status;			//百人场游戏状态
	uint32				m_brc_table_status_time;	//百人场游戏状态对应的倒计时
	set<uint32>			m_tableCtrlPlayers;			// 控制玩家列表---进入桌子

	bool				m_lucky_flag;				//当前局是否触发幸运值
	set<uint32>			m_set_ctrl_lucky_uid;		//当前局触发幸运值的玩家列表
	bool m_isAllRobotOrPlayerJetton = true; // 是否全部为玩家或机器人下注，针对库存系统
};

#endif //__GAME_TABLE_H__


