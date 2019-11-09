

#ifndef __PLAYER_MGR_H__
#define __PLAYER_MGR_H__


#include "player_base.h"
#include <map>
#include "svrlib.h"

#include <string>
#include "utility/stl_hash_map.h"


// 在线玩家管理器
using namespace svrlib;
using namespace std;
using namespace Network;

class CPlayerMgr : public ITimerSink,public AutoDeleteSingleton<CPlayerMgr>
{		
public:
    CPlayerMgr();
    ~CPlayerMgr();

    virtual void OnTimer(uint8 eventID);

    bool        Init();
	void		ShutDown();
	void        OnTimeTick();

    bool            IsOnline(uint32 uid);
    CPlayerBase*    GetPlayer(uint32 uid);
	
    bool        AddPlayer(CPlayerBase* pPlayer);
    bool        RemovePlayer(CPlayerBase* pPlayer);

	void		SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type);
	void		SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uid);

	uint32		GetOnlines(uint32& players,uint32& robots);
	uint32		GetRoomOnlines(uint32& players, uint32& robots);

    void        GetAllPlayers(vector<CPlayerBase*>& refVec);
    void        CheckRecoverPlayer();

	bool		SendVipBroadCast();
	bool		SendVipProxyRecharge();
	bool		SendVipAliAccRecharge();

	bool		SendUnionPayRecharge();
	bool		SendWeChatPayRecharge();
	bool		SendAliPayRecharge();
	bool		SendOtherPayRecharge();
	bool		SendQQPayRecharge();
	bool		SendWeChatScanPayRecharge();
	bool		SendJDPayRecharge();
	bool		SendApplePayRecharge();
	bool		SendLargeAliPayRecharge();
	void SendExclusiveAlipayRecharge();
	void SendFixedAlipayRecharge();
	void SendFixedWechatRecharge();
	void SendFixedUnionpayRecharge();
	void SendExclusiveFlashRecharge();
	void		CloginCleanup();

	//清除所有玩家的活跃福利统计数据
	void		ActiveWelfareCleanup();

	stl_hash_map<uint32, CPlayerBase*> & GetAllPlayers() { return m_mpPlayers; }
private:
	typedef 	stl_hash_map<uint32, CPlayerBase*> 		 	MAPPLAYERS;		
    MAPPLAYERS  m_mpPlayers;
    CTimer*     m_pFlushTimer;
    CTimer*     m_pRecoverTimer;
};




#endif // __PLAYER_MGR_H__



