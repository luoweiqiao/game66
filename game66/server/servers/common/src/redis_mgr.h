
#ifndef REDIS_MGR_H__
#define REDIS_MGR_H__

#include "dbmysql/hiredisop.h"
#include "svrlib.h"
#include "config/config.h"
#include <string>
#include "db_struct_define.h"

using namespace std;
using namespace svrlib;

class CRedisMgr : public ITimerSink,public CHiredisOp,public AutoDeleteSingleton<CRedisMgr>
{
public:
	CRedisMgr();
	~CRedisMgr();

	virtual void OnTimer(uint8 eventID);
	
	bool	Init(stRedisConf& conf);
	bool    SetPassword(string pass);
	void	ShutDown();
	
	// 设置玩家在线服务器ID
	void	SetPlayerOnlineSvrID(uint32 uid,uint16 svrID);
	uint16  GetPlayerOnlineSvrID(uint32 uid);
	void 	DelPlayerOnlineSvrID(uint32 uid);
	void 	ClearPlayerOnlineSvrID(uint16 svrID);
	// 保存玩家屏蔽玩家信息
	void 	SavePlayerBlockers(uint32 uid,vector<uint32>& vecRef);
	void 	LoadPlayerBlockers(uint32 uid,vector<uint32>& vecRef);
	void 	SavePlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef);
	void 	LoadPlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef);

	// 设置币商UID
	void 	AddVipPlayer(uint32 uid,int32 vip);
	void 	RemoveVipPlayer(uint32 uid);
	bool  	IsVipPlayer(uint32 uid);
    // 签到设备限制
    void    AddSignInDev(string dev,uint32 uid);
    void    ClearSignInDev();
    bool    IsHaveSignInDev(string dev);
    
	// 封号玩家
	bool 	IsBlackList(uint32 uid);
	// 玩家登陆Key
	string	GetPlayerLoginKey(uint32 uid);
	void 	RenewalLoginKey(uint32 uid);

	uint32	GetOperCount(){ return m_count; }
	void 	ClearOperCount(){ m_count = 0; }

    // 写入svrlog
    void    WriteSvrLog(string logStr);
    
	void    WriteUserSnatchGameData(uint32 uid, uint32 snatchtype, string logStr);

	void    ClearUserSnatchGameData(uint32 snatchtype);

    // 活跃福利
    void    GetPlayerActiveWelfareInfo(uint32 uid, map<uint8,uint64>& mapRef);
    void    SetPlayerActiveWelfareInfo(uint32 uid, uint8 aw_id, uint64 count);
    void    ClearAllPlayerActiveWelfareInfo();

	// 新注册玩家福利
	void    GetPlayerNewRegisterWelfareInfo(uint32 uid, stNewRegisterWelfarePlayerInfo	&nrw_info);
	void    SetPlayerNewRegisterWelfareInfo(uint32 uid, stNewRegisterWelfarePlayerInfo	&nrw_info);

	// 百人场精准控制信息
	void    GetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo	&pri_info);
	void    SetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo	pri_info);

private:
	uint16  m_svrID;
    CTimer* m_pTimer;
    char	m_szKey[128];
	uint32  m_count;

};

#endif // REDIS_MGR_H__

























































































































































