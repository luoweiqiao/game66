
#ifndef PLAYER_BASE_H__
#define PLAYER_BASE_H__


#include "svrlib.h"
#include "db_struct_define.h"
#include "pb/msg_define.pb.h"

using namespace svrlib;
using namespace std;
using namespace Network;


// 用户权限
// 0x00000001 标识过数据库
#define UR_NOVICE_WELFARE 0x00000002				//新手福利



enum PLAYER_TYPE
{
	PLAYER_TYPE_ONLINE = 0,		// 在线玩家
	PLAYER_TYPE_ROBOT,	   		// 机器人
};
enum PLAYER_STATE
{
	PLAYER_STATE_NULL,			//    空状态
	PLAYER_STATE_LOAD_DATA, 	// 	  拉取数据
	PLAYER_STATE_PLAYING,		//    游戏状态
	PLAYER_STATE_LOGINOUT,		//    下线
};
class CPlayerBase
{
public:
	CPlayerBase(uint8 type);
	virtual ~CPlayerBase();
	
	bool	SetBaseInfo(stPlayerBaseInfo& info);	
    bool    SetAccountInfo(stAccountInfo& info);
    bool    SetGameInfo(uint16 gameType,const stGameBaseInfo& info);
    
    bool    IsLoadData(uint8 dataType);
    void    SetLoadState(uint8 dataType,uint8 state);
    bool	IsLoadOver();	

    void  	SetPlayerState(uint8 state);
	uint8 	GetPlayerState();
	bool	IsPlaying();
            
	uint32	GetUID();
	void	SetUID(uint32 uid);
        
	string	GetCity() { return m_strCity; }
	void	SetCity(string city) { m_strCity = city; }

	string	GetNickName() { return m_baseInfo.name; }

	uint8	GetPlayerType(){ return m_bPlayerType; }
    bool    IsRobot();
	string 	GetPlayerName();
	void 	SetPlayerName(string name);
	uint8	GetSex();
	void 	SetSex(uint8 sex);
	uint16	GetHeadIcon();
    uint32  GetDayGameCount(){ return m_baseInfo.dayGameCount; }
    void    SetDayGameCount(uint32 num){ m_baseInfo.dayGameCount = num; }
    
    
  	virtual void  	SetSession(NetworkObject* pSession);
	virtual NetworkObject* GetSession();
	virtual void	SetIP(string strIP);
	virtual uint32	GetIP();
	virtual string  GetIPStr();

	virtual int32	GetVipLevel();

	
	virtual bool  	SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type);
	virtual bool  	SendMsgToClient(const void *msg, uint16 msg_len, uint16 msg_type);

    virtual bool 	OnLoginOut(uint32 leaveparam = 0);
	virtual void  	OnLogin();
	virtual void	OnGetAllData();
	virtual void	ReLogin();
	virtual void 	OnTimeTick(uint64 uTime,bool bNewDay);
	// 是否需要回收
	virtual bool 	NeedRecover();
	virtual bool    IsCanLook();

	void	GetPlayerBaseData(net::base_info* pInfo);
	void	SetPlayerBaseData(const net::base_info& info);

    void    GetGameData(uint16 gameType,net::game_data_info* pInfo);
    void    SetGameData(uint16 gameType,const net::game_data_info& pInfo);
    
	void 	SetPlayerGameData(const net::msg_enter_into_game_svr& info);
	void	GetPlayerGameData(uint16 gameType,net::msg_enter_into_game_svr* pInfo);

	// 修改玩家账号数值（增量修改）
	bool 	CanChangeAccountValue(uint8 valueType,int64 value);
	int64   ChangeAccountValue(uint8 valueType,int64 value);
	int64 	GetAccountValue(uint8 valueType);
	void 	SetAccountValue(uint8 valueType,int64 value);
	// 修改玩家账号数值（增量修改）
	virtual bool SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid="") = 0;
	// 原子操作账号数值
	virtual bool AtomChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin);

	// 存取保险箱
	bool 	TakeSafeBox(int64 takeCoin);
	void 	SetSafePasswd(const string& passwd);
	bool 	CheckSafePasswd(const string& passwd);

	void 	SetSafeBoxState(uint8 state,bool checkpwd = true);
	uint8   GetSafeBoxState();
	// 钻石兑换积分财富币
	uint8 	ExchangeScore(uint32 exid,uint8 extype);

	bool 	IsVip(){ return m_accInfo.vip > 0 ; }
	void	SetVip(uint32 vip){ m_accInfo.vip = vip; }

	uint32	GetCLogin(){ return m_baseInfo.clogin; }
	uint32  GetWLogin(){ return m_baseInfo.weekLogin; }
	uint32  GetBankrupt(){ return m_baseInfo.bankrupt; }
	void 	AddBankrupt(){ m_baseInfo.bankrupt++; }

	//领奖记录
	void	SetRewardBitFlag(uint32 flag);
	void	UnsetRewardBitFlag(uint32 flag);
	bool	IsSetRewardBitFlag(uint32 flag);
	void 	SendBaseValue2Client();
	void    CloginCleanup();

	// 修改斗地主数值(增量修改)
	void    ChangeLandValue(bool isCoin,int32 win,int32 lose,int32 land,int32 spring,int64 winscore);

    // 修改游戏数值
	void 	ChangeGameValue(uint16 gameType,bool isCoin,int32 win,int32 lose,int64 winscore);
	int64	GetGameMaxScore(uint16 gameType,bool isCoin);    
	// 设置最大牌型
	void 	SetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount);
	void 	GetGameMaxCard(uint16 gameType,bool isCoin,uint8* cardData,uint8 cardCount);
	int64	GetPlayerWeekWinScore(uint16 gameType);
	int64	GetPlayerDayWin(uint16 gameType);
	int64	GetPlayerTotalWinScore(uint16 gameType);
	int64	GetPlayerGameCount(uint16 gameType);
	int64	GetPlayerStockScore(uint16 gameType);


	int64	GetRecharge() { return m_accInfo.recharge; }
	int64	GetConverts() { return m_accInfo.converts; }
	int64	GetProfits() { return m_accInfo.recharge - m_accInfo.converts; }
	void	AddRecharge(int64 value) { m_accInfo.recharge += value; }
	bool	SendVipBroadCast();
	bool	SendVipProxyRecharge();
	bool	SendVipAliAccRecharge();

	bool	SendUnionPayRecharge();
	bool	SendWeChatPayRecharge();
	bool	SendAliPayRecharge();
	bool	SendOtherPayRecharge();
	bool	SendQQPayRecharge();
	bool	SendWeChatScanPayRecharge();
	bool	SendJDPayRecharge();
	bool	SendApplePayRecharge();
	bool	SendLargeAliPayRecharge();
	void SendExclusiveAlipayRecharge(); // 发送个人专属支付宝信息
	void SendFixedAlipayRecharge(); // 发送定额支付宝信息
	void SendFixedWechatRecharge(); // 发送定额微信信息
	void SendFixedUnionpayRecharge(); // 发送定额银联云闪付信息
	void SendExclusiveFlashRecharge(); // 发送专享闪付信息

	// 设置用户水果机库存 
	int64    ChangeStockScore(uint16 gametype,int64 nStockScore);

	void	ReSetGameCount(uint16 gameType);


	int GetVecWin(uint16 gameType);
	int64 GetVecBet(uint16 gameType);
	void SetVecWin(uint16 gameType, int win);
	void SetVecBet(uint16 gameType, int64 score);
	int GetBetCount(uint16 gameType);
	void ClearVecWin(uint16 gameType);
	void ClearVecBet(uint16 gameType);

public:
	uint32 GetBatchID() {		return m_batchID;	}
	void   SetBatchID(uint32 batchID) { m_batchID = batchID; }
    
	uint32 GetLvScore() { return m_lvScore; }
	void   SetLvScore(uint32 value) { m_lvScore = value; }

	uint32 GetLvCoin() { return m_lvCoin; }
	void   SetLvCoin(uint32 value) { m_lvCoin = value; }

	int    GetPosTimeDayCount();
	bool   IsCanEnterWelfareRoom();
	bool   IsNoviceWelfare();
	void   SetNoviceWelfare();
	bool   GetNoviceWelfare();

	
	uint64 GetWelTime() { return m_accInfo.weltime; }
	void   SetWelTime(uint64 value) { m_accInfo.weltime = value; }
	void   UpdateWelCount(int value);
	int    GetWelCount(){ return m_accInfo.welcount; }
	void   SetWelCount(int value) { m_accInfo.welcount = value; }

	void   UpdateWelCountByTime();

	uint32 GetPosRmb() { return m_accInfo.posrmb; }

	uint32 SetPosRmb(uint32 value) { return m_accInfo.posrmb = value; }
	uint32 SetPosTime(uint64 value) { return m_accInfo.postime = value; }
	
	//活跃福利功能
	void	AddActwleRecharge(int64 value) { m_accInfo.recharge_actwle += value; }
	void	AddActwleConverts(int64 value) { m_accInfo.converts_actwle += value; }
	
	uint64	GetActwleRecharge() { return m_accInfo.recharge_actwle; }
	uint64	GetActwleConverts() { return m_accInfo.converts_actwle; }

	void	ResetActwleRecharge() { m_accInfo.recharge_actwle = m_accInfo.coin + m_accInfo.safecoin; }
	void	ResetActwleConverts() { m_accInfo.converts_actwle = 0; }

	//新注册玩家福利功能
	uint64  GetRtime() { return m_baseInfo.rtime; }
	void    SetIsPay(uint8 status) { m_baseInfo.ispay = status;	}
	//bool   IsKilledScore(uint16 gametype);
	//stAutoKillConfrontationCfg   GetKilledScoreCfg();

	//int GetDefeat() { return m_defeat; }
	//int GetInterval() { return m_interval; }
	//void AddDefeat() { m_defeat++; }
	//void AddInterval() { m_interval++; }
protected:
	// 是否显示vip充值微信账号信息
	bool IsShowVipRechargeWeChatInfo(tagVipRechargeWechatInfo &tagInfo, tm &tmTime);
	// 设置微信账号信息
	void SetVipRechargeWechatinfo(net::vip_recharge_wechatinfo *pwechatinfo, tagVipRechargeWechatInfo &tagInfo);

	int32   m_defeat;   // 对战场控制局数
	int32   m_interval; // 对战场不控制局数

	uint32					m_batchID;
	uint32					m_lvScore;
	uint32					m_lvCoin;
	uint32					m_uid;
	uint8					m_bPlayerType;
    NetworkObject* 			m_pSession;
	uint8			  		m_bPlayerState;    
	string					m_strCity;
	uint32					m_loginIP;
	string					m_strLoginIP;
    stPlayerBaseInfo        m_baseInfo;                               // 基础信息
    stAccountInfo           m_accInfo;                                // 帐号信息
    stGameBaseInfo          m_gameInfo[net::GAME_CATE_MAX_TYPE];      // 游戏信息    

    uint8                   m_loadGameState[net::GAME_CATE_MAX_TYPE]; // 游戏数据加载状态
    uint8                   m_loadState[emACCDATA_TYPE_MAX];          // 数据加载状态 
};


#endif // PLAYER_BASE_H__

