
#ifndef SERVER_MGR_H__
#define SERVER_MGR_H__


#include <vector>
#include "svrlib.h"
#include "robot.h"

using namespace std;
using namespace svrlib;
using namespace Network;

struct  stGServer
{
	uint16 svrID;
	uint16 gameType;
    uint8  gameSubType;
	uint8  status;
    uint8  openRobot;
    uint32 playerNum;
    uint32 robotNum;
	NetworkObject* pNetObj; 
	stGServer()
	{
		memset(this,0,sizeof(stGServer));
	}	
};

class CServerMgr : public ITimerSink,public CProtobufMsgHanlde,public AutoDeleteSingleton<CServerMgr>
{
public:
	CServerMgr();
	~CServerMgr();

	virtual void OnTimer(uint8 eventID);

	bool	Init();
	void	ShutDown();
        
	bool   AddServer(NetworkObject* pNetObj,uint16 svrID,uint16 gameType,uint8 gameSubType,uint8 openRobot);
	void   RemoveServer(NetworkObject* pNetObj);
	
	void   UpdateOpenRobot(uint16 svrID, uint8 openRobot);

	stGServer* GetServerBySocket(NetworkObject* pNetObj);
	stGServer* GetServerBySvrID(uint16 svrID);

	void   SendMsg2Server(uint16 svrID,const google::protobuf::Message* msg,uint16 msg_type,uint32 uin);    
	void   SendMsg2AllServer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin);

	void   SendMsg2Server(uint16 svrID,const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin);    
	void   SendMsg2AllServer(const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin);

    // 获取服务器列表
    void   SendSvrsInfo2Client(uint32 uid);
    void   SendSvrsPlayInfo2Client(uint32 uid);
    
	void   SendSvrsInfoToAll();
	// 发送维护公告
	void   SendSvrRepairContent(NetworkObject* pNetObj);
    // 发送停服广播
    void   SendSvrRepairAll(); 
    bool   IsNeedRepair(uint16 svrID);
    
	uint16 GetGameTypeSvrID(uint32 gameType);

	bool   IsOpenRobot(uint32 gameType);


	// 服务器重连检测玩家所在服务器状态
	void   SyncPlayerCurSvrID(uint16 svrID);
        
	// 停服通知
	void   NotifyStopService(uint32 btime,uint32 etime,vector<uint16>& svrids,string content);
    // 获得机器人服务器ID
    uint16 GetRobotSvrID(CLobbyRobot* pRobot);
    
    // 服务器机器人数量排序
    static  bool CompareServerRobotNum(stGServer* pSvr1,stGServer* pSvr2);
    // 更改房间param
	void ChangeRoomParam(uint32 gametype, uint32 roomid, string param);
    // 控制玩家
	void ChangeContorlPlayer(int64 id,uint32 gametype,uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount);

	void ChangeContorlMultiPlayer(int64 id,uint32 gametype, uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount, uint64 gametime,int64 totalscore);
	// 停止夺宝
	void StopSnatchCoin(uint32 gametype, uint32 roomid, uint32 stop);
	// 机器人夺宝
	void RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount);

	void ContorlDiceGameCard(uint32 gametype, uint32 roomid, uint32 uDice[]);

	void ConfigMajiangHandCard(uint32 gametype, uint32 roomid, string strHandCard);

	void UpdateServerRoomRobot(int gametype, int roomid, int robot);

	void	ReloadRobotOnlineCfg(int optype,int gametype,int roomid,int leveltype, int batchid, int logintype, int entertimer, int leavetimer, vector<int> & vecOnline);

	void UpdateNewPlayerNoviceWelfareRight(uint32 uid, uint32 userright,uint32 posrmb, uint64 postime);

	void UpdateNewPlayerNoviceWelfareValue(tagNewPlayerWelfareValue & NewPlayerWelfareValue);

	// 通知服务器刷新自动杀分配置
	void UpdateServerAutoKillCfg(string & strjvalueRecharge);
	// 通知服务器刷新自动杀分玩家列表
	void UpdateServerAutoKillUsers(string & strjvalueRecharge);

    // 通知服务器刷新活跃福利配置
    void UpdateServerActiveWelfareCfg(string & json_msg);

	// 通知服务器重置所有在线玩家的活跃福利信息
	void ResetPlayerActiveWelfareInfo(string & json_msg);

	// 通知服务器刷新新帐号福利配置
	void UpdateServerNewRegisterWelfareCfg(string & json_msg);

	//精准控制配置数据发生改变，通知玩家所在的服务器，返回大厅
	void StopContrlPlayer(uint16 svrid, uint32 uid);

	void SynCtrlUserCfg(net::msg_syn_ctrl_user_cfg * pmsg);

	// 通知游戏服修改房间库存配置  add by har
	bool NotifyGameSvrsChangeRoomStockCfg(uint32 gameType, stStockCfg &st);
	
	void SynLuckyCfg(net::msg_syn_lucky_cfg * pmsg);

	void SynFishCfg(net::msg_syn_fish_cfg * pmsg);

	void ResetLuckyCfg(net::msg_reset_lucky_cfg * pmsg);

	// 设置玩家登录时间
	void SetPlayerLoginTime(uint32 uid);

protected:
	void   CheckRepairServer();

private:
	typedef stl_hash_map<uint32,stGServer>  MAP_SERVERS;
	MAP_SERVERS		m_mpServers;
    CTimer*			m_pTimer;
	uint32 			m_bTimeStopSvr;
	uint32 			m_eTimeStopSvr;
	string 			m_stopSvrContent;
	vector<uint16>	m_stopSvrs;
	CCooling		m_coolBroad;

	// 非机器人登陆耗时统计相关
	uint64 m_loginAllTime  = 0; // 登陆耗时总时间(毫秒)
	uint64 m_loginAllCount = 0; // 登陆总次数
	int64  m_maxLoginTime  = 0; // 最大延时
	int64  m_minLoginTime  = 0; // 最小延时
	map<uint32, uint64> m_mpPlayerLoginTime; // 玩家id -> 登录时间（毫秒）
	// 登陆耗时统计相关 end
};


#endif // SERVER_MGR_H__ 













