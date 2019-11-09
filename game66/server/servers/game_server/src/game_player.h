

#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

#include "svrlib.h"
#include "player_base.h"
#include "game_define.h"
#include "pb/msg_define.pb.h"
#include "game_table.h"
#include "new_register_welfare_mgr.h"

using namespace svrlib;
using namespace std;
using namespace net;
using namespace Network;

class CGameRoom;
class CGameTable;

struct tagMasterShowInfo
{
	uint32 uid;
	string city;
	int64  coin;

	tagMasterShowInfo()
	{
		uid = 5600403;
		city = "北京";
		coin = 100000;
	}
};

class CGamePlayer : public CPlayerBase
{
public:
	CGamePlayer(uint8 type);
	virtual ~CGamePlayer();

	virtual bool  	SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type,bool bSendRobot = false);

	virtual bool 	OnLoginOut(uint32 leaveparam = 0);
	virtual void  	OnLogin();
	virtual void	ReLogin();
	virtual void 	OnGameEnd();

    // 更新游戏服务器ID到redis
	void 	FlushOnlineSvrIDToRedis(uint16 svrID);
    // 通知返回大厅(退出游戏)
	void	NotifyLeaveGame(uint16 gotoSvrid = 0);
	// 请求大厅修改数值
	void	NotifyLobbyChangeAccValue(int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid="");
	// 修改玩家账号数值（增量修改）
	virtual bool    SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid="");
	// 修改玩家账号数值（减少修改）
	virtual void    SubChangeAccountValue(uint16 operType, uint16 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, const string& chessid = "");

	// 能否退出
	bool 	CanBackLobby();

	// 游戏消息处理
	int 	OnGameMessage(uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
	// 重置心跳时间
	void	ResetHeart();

	virtual void   OnTimeTick(uint64 uTime,bool bNewDay);
	// 是否需要回收
	virtual bool   NeedRecover();
	// 是否正在游戏中
	bool 	IsInGamePlaying();
	// 更新数值到桌子
	void 	FlushAccValue2Table();
    // 修改游戏数值并更新到数据库
    void 	AsyncChangeGameValue(uint16 gameType,bool isCoin,int64 winScore, int64 lExWinScore,int rwelfare);
    void    AsyncSetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount);
	void 	AsyncChangeGameValueForFish(bool isCoin, int64 winScore);
	virtual bool    IsCanLook();

public:
	uint8	GetNetState();
	void	SetNetState(uint8 state);

	CGameRoom*	GetRoom(){ return m_pGameRoom; }
	void	SetRoom(CGameRoom* pRoom){ m_pGameRoom = pRoom; }
	uint16  GetRoomID();

	CGameTable*	GetTable(){ return m_pGameTable; }
	void	SetTable(CGameTable* pTable);
	uint32  GetTableID();

	void 	SetAutoReady(bool bAuto){ m_autoReady = bAuto; }
	bool 	IsAutoReady(){ return m_autoReady; }

	void 	SetPlayDisconnect(bool bFlag){ m_playDisconnect = bFlag; }
	bool 	IsPlayDisconnect(){ return m_playDisconnect; }

	void 	AddBlocker(uint32 uid);
	bool 	IsExistBlocker(uint32 uid);
	void 	ClearBlocker();

	void 	AddBlockerIP(uint32 ip);
	bool 	IsExistBlockerIP(uint32 ip);
	void 	ClearBlockerIP();

	void	SetBankruptRecord(bool bFlag) { m_bBankruptRecord = bFlag; }
	bool	GetBankruptRecord() { return m_bBankruptRecord; }

	void 	AsyncChangeStockScore(uint16 gametype,int64 winScore);// 修改游戏玩家库存并更新到数据库

public:

	//活跃福利功能
    uint64   GetCurrAWInfo(uint8 aw_id);
    void    SetCurrAWInfo(uint8 aw_id, uint64 aw_count);
    //void    ClearCurrAWInfo(uint8 aw_id);

    uint8   GetCurrAWID() { return m_curr_aw_id; }
    void    SetCurrAWID(uint8 aw_id) { m_curr_aw_id = aw_id; }

    int64   GetCurrAWBaseLoss() { return m_curr_aw_base_loss; }
    void    SetCurrAWBaseLoss(int64 bass_loss) { m_curr_aw_base_loss = bass_loss; }

    bool    IsEnterActiveWelfareRoomFlag(uint8 game_type);

    int64   GetCurrLoss() { return m_accInfo.coin + m_accInfo.safecoin + m_accInfo.converts_actwle - m_accInfo.recharge_actwle; }

	//新注册用户福利
	bool    IsNewRegisterPlayerWelfareFlag(uint8 game_id);		//只需要判断是否在新手福利期间
	void    UpdateNewRegisterPlayerInfo(int64 win_coin);
	void    UpdateNRWPlayerScore(int64 coin);
	//返回当前玩家是否享受新手福利 参数 1:表示必胜期间 2:表示新账号盈利是否大于最高盈利 0:表示其它情况
	bool    GetNewRegisterWelfareStatus(int &status);
    
	stNewRegisterWelfarePlayerInfo* GetPlayerNRWInfo() { return &m_nrw_info; }

	struct tagUserNewPlayerWelfareValue & GetWelfareValue() { return m_tagWelfareValue; }

public:

	void 	SetCtrlFlag(bool flag) { m_ctrl_flag = flag; }
	bool	GetCtrlFlag() { return m_ctrl_flag; }

	void    SetShowUid(uint32 value) { m_MasterShowInfo.uid = value; }
	void    SetShowCity(string value) { m_MasterShowInfo.city = value; }
	void    SetShowCoin(int64 value) { m_MasterShowInfo.coin = value; }
	void    UpdateShowCoin(int64 value) { m_MasterShowInfo.coin += value; }

	uint32  GetShowUid() { return m_MasterShowInfo.uid; }
	string  GetShowCity() { return m_MasterShowInfo.city; }
	int64  GetShowCoin() { return m_MasterShowInfo.coin; }

public:
	// 读取当前玩家的幸运值配置信息
	void 	GetLuckyCfg();

	//根据roomid获取当前玩家是否触发幸运值 flag:代表控制赢输 true 赢 false 输-----针对非捕鱼类游戏
	bool	GetLuckyFlag(uint16 roomid, bool &flag);

	//根据roomid获取当前玩家是否触发幸运值 flag:代表控制赢输 true 赢 false 输  rate:概率-----针对捕鱼类游戏
	bool	GetLuckyFlagForFish(uint16 roomid, bool &flag, uint32 &rate);

	// 设置当前玩家的幸运值统计信息
	void    SetLuckyInfo(uint16 roomid, int32 add_value);

	// 重置当前玩家的幸运值信息---每天凌晨
	void    ResetLuckyInfo();

protected:
	uint8		m_netState;			// 网络状态
	CGameRoom* 	m_pGameRoom;		// 所在房间
	CGameTable*	m_pGameTable;		// 所在桌子
	uint32 		m_msgHeartTime;		// 消息心跳时间
	bool 		m_autoReady;		// 自动准备
	bool        m_playDisconnect;   // 游戏是否断线中
	bool		m_bBankruptRecord;
	vector<uint32> 	m_blocks;		// 屏蔽匹配人
	vector<uint32> 	m_blockIPs;		// 屏蔽IP

	// 活跃福利功能
    map<uint8, uint64> m_active_welfare_record;    //记录玩家在每个亏损区间的补助记录---用于活跃福利
	uint32      m_curr_aw_id;              // 玩家当前亏损所在的区间ID
	int64       m_curr_aw_base_loss;       // 玩家进入当前亏损区间时的基础值

	// 新注册玩家福利
	stNewRegisterWelfarePlayerInfo	m_nrw_info;		//当前玩家的新注册福利信息
	struct		tagUserNewPlayerWelfareValue m_tagWelfareValue; // 新手福利临时值

	//精准控制
	bool        m_ctrl_flag;		// 是否拥有精准控制权限
	tagMasterShowInfo	m_MasterShowInfo;

	//幸运值
	map<uint8, tagUserLuckyCfg>		m_lucky_info;	//key:roomid 需要区分场次
	bool		m_lucky_read;		//是否读取过该玩家的幸运值配置信息
};



#endif // __GAME_PLAYER_H__



