
#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "svrlib.h"
#include "player_base.h"
#include "game_define.h"
#include "pb/msg_define.pb.h"
#include "mission_mgr.h"

using namespace svrlib;
using namespace std;
using namespace net;
using namespace Network;

class CPlayer : public CPlayerBase
{
public:	
	CPlayer(uint8 type);
	virtual ~CPlayer();
	
	virtual bool 	OnLoginOut(uint32 leaveparam = 0);
	virtual void  	OnLogin();
	virtual void	OnGetAllData();
	virtual void	ReLogin();
	virtual void 	OnTimeTick(uint64 uTime,bool bNewDay);
	// 是否需要回收
	virtual bool    NeedRecover();
	// 返回大厅回调
	virtual void    BackLobby();
	// 游戏战报
	virtual void 	OnGameEnd(uint16 gameType);


	bool 	CanModifyData();
	//--- 每日清理
	void	DailyCleanup(int32 iOfflineDay);
    void    SignIn();
    
    //--- 每周清理
    void	WeeklyCleanup();
    //--- 每月清理
    void	MonthlyCleanup();
	
	// 信息同步
	void	NotifyEnterGame();
    void    NotifyLeaveGame(uint32 code);
	bool	SendAllPlayerData2Client();	
	bool	SendAccData2Client();
	bool 	UpdateAccValue2Client();
	bool 	UpdateBaseValue2Client();
    // 同步游戏数据
    bool    UpdateGameInfo2Client(uint16 gameType);
    // 通知返回大厅
    void    NotifyClientBackLobby();
    
	// 构建初始化
	void	BuildInit();	    
public:
    uint16  GetCurSvrID();
    void    SetCurSvrID(uint16 svrID,bool bSetRedis);
	void 	SyncCurSvrIDFromRedis(uint16 svrID);

	// 是否在大厅中
	bool 	IsInLobby();
	bool  	SendMsgToGameSvr(const google::protobuf::Message* msg,uint16 msg_type);
	bool  	SendMsgToGameSvr(const void *msg, uint16 msg_len, uint16 msg_type);

    // 通知网络状态
 	void	NotifyNetState2GameSvr(uint8 state);
	// 通知修改玩家信息
	void 	NotifyChangePlayerInfo2GameSvr();
 	// 进入游戏服务器
 	bool	EnterGameSvr(uint16 svrID);
    // 刷新修改数值到游戏服
    void    FlushChangeAccData2GameSvr(int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin);
	void    UpDateChangeAccData2GameSvr(int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, uint32 oper_type, uint32 sub_type);
	void    UpDateChangeAccData2GameSvrEveryColor(int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, uint32 oper_type, uint32 sub_type);

	// 修改玩家账号数值（增量修改）
	virtual bool  SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid="");

	bool    PhpAtomChangeAccountValue(uint16 operType, uint16 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin);

	// 保存登陆奖励状态
	void 	SaveLoginInfo();
	// 领取破产补助
	bool 	GetBankruptHelp();

	CMissionMgr& GetMissionMgr(){ return m_missionMgr; }

    uint32  GetSpeakCDTime();
    void    Speak();

	// 登陆key
	void 	SetLoginKey(const string & key){ m_loginKey = key; }
	string  GetLoginKey(){ return m_loginKey; }
    // 登录设备ID
    void    SetLoginDeviceID(string device){ m_loginDevice = device; }
    string  GetLoginDeviceID(){ return m_loginDevice; }
    
	// 设置回收
	void 	SetNeedRecover(bool bNeed){ m_needRecover = bNeed; }

	//进入时时彩游戏服
	bool	EnterEveryColorGameSvr(uint16 svrID);
	//离开时时彩游戏服
	bool	LeaveEveryColorGameSvr(uint16 svrID);

	uint16  GetSecondSvrID()	{	return m_secondSvrID;	}
	void    SetSecondSvrID(uint16 svrID)	{		m_secondSvrID = svrID;	}
	void	NotifyNetState2SecondGameSvr(uint8 state);

	// 精准控制校验串
	void 	SetCheckCode(const string & code) { m_checkcode = code; }
	string  GetCheckCode() { return m_checkcode; }

protected:
	uint16  		m_secondSvrID;
    uint16  		m_curSvrID;     	// 当前所在服务器ID
	uint32 			m_disconnectTime;	// 断线时间
	CMissionMgr		m_missionMgr;		// 任务管理器
    uint32          m_lastSpeakTime;    // 最后喇叭时间
    uint32          m_loginTime;        // 在线计时时间
    string 			m_loginKey;			// 登陆key
    string          m_loginDevice;      // 登录设备
	uint32 			m_loadTime;			// 加载数据时间
	bool 			m_needRecover;		// 需要下线回收
	string 			m_checkcode;		// 精准控制校验串
};



#endif // __PLAYER_H__



