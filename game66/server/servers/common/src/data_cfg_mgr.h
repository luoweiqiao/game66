#ifndef DATA_CFG_MGR_H__
#define DATA_CFG_MGR_H__

#include <string>
#include "json/json.h"
#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include "db_struct_define.h"
#include "dbmysql_mgr.h"
#include <vector>
#include "game_define.h"

using namespace std;
using namespace svrlib;

/*************************************************************/
class CDataCfgMgr : public AutoDeleteSingleton<CDataCfgMgr>
{
private:
    struct stShowHandBaseCfg
    {
        int64   minValue;
        int64   maxValue;
        uint32  paramValue;
    };
public:
    CDataCfgMgr();
    virtual ~CDataCfgMgr();

public:
    bool    Init();
    void    ShutDown();
    bool    Reload();

    int32   GetGiveTax(bool sendVip,bool recvVip);
    int32   GetCLoginScore(uint32 days);
	vector<int32> &  GetCloginRewards();

    int32   GetWLoginIngot(uint8 days);
    uint32  GetBankruptCount();
    int32   GetBankruptBase();
    int32   GetBankruptValue();
    int32   GetBankruptType();
    int32   GetSpeakCost();
    int32   GetJumpQueueCost();
    uint32   GetSignGameCount();

    bool    GetExchangeValue(uint8 exchangeType,uint32 id,int64& fromValue,int64& toValue);
    uint32  GetExchangeID(uint8 exchangeType,int64 wantValue);
    bool    GetRobotPayDiamondRmb(int64 wantDiamond,int64 &rmb,int64 &diamond);

    // 获得梭哈底注
    int64   GetShowhandBaseScore(int64 handScore);

    bool    CheckBaseScore(uint8 deal,uint32 score);
    bool    CheckFee(uint8 feeType,uint32 feeValue);
    int64   GetLandPRoomRice(uint32 days);
    int64   CalcEnterMin(int64 baseScore,uint8 deal);

    const stServerCfg& GetCurSvrCfg(){ return m_curSvrCfg; }
    stServerCfg* GetServerCfg(uint32 svrid);
    void    GetLobbySvrsCfg(uint32 group,vector<stServerCfg>& lobbysCfg);

    const   stMissionCfg* GetMissionCfg(uint32 missid);
    const   map<uint32,stMissionCfg>& GetAllMissionCfg(){ return m_Missioncfg; }

	bool    cloginrewardAnalysis(bool bPhpSend, std::string & strAnalysis);

	//vip广播
	bool	VipBroadCastAnalysis(bool bPhpSend, string & strBroadcast);
	int32	GetVipBroadcastStatus() { return m_iVipBroadcastStatus; }
	int64	GetVipBroadcastRecharge() { return m_lVippBroadcastRecharge; }
	string	GetVipBroadcastMsg() { return m_strVipBroadcast; }
	//vip充值
	bool	VipProxyRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge);
	bool	VipAliAccRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge);	
	bool    UnionPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    WeChatPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    AliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    OtherPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    QQPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    WeChatScanPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    JDPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    ApplePayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    LargeAliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool ExclusiveAlipayRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 个人专属支付宝
	bool FixedAlipayRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 定额支付宝
	bool FixedWechatRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 定额微信
	bool FixedUnionpayRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 定额云闪付
	bool ExclusiveFlashRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 专享闪付

	static bool    CompareWelfareValueByPid(tagNewPlayerWelfareValue data1, tagNewPlayerWelfareValue data2);
	void    SetNewPlayerWelfareValue(tagNewPlayerWelfareValue & data);
	tagNewPlayerWelfareValue GetNewPlayerWelfareValue(uint32 uid,uint32 payrmb);
	//微信账号
	int32   GetVipProxyRechargeStatus() { return m_VipProxyRechargeStatus; }
	int64   GetVipProxyRecharge() { return m_lVipProxyRecharge; }
	map<string, tagVipRechargeWechatInfo> & GetVipProxyWeChatInfo() { return m_mpVipProxyWeChatInfo; }
	int64 GetVipProxySpecialRecharge() { return m_lVipProxySpecialRecharge; }
	// 指定渠道是否属于vip代理微信充值特殊渠道
	// siteid : 指定渠道渠道
	bool IsInVipProxyRechargeSpecialCnid(int32 siteid)
	    { return m_usVipProxySpecialRechargeCnid.find(siteid) != m_usVipProxySpecialRechargeCnid.end(); }

	//支付宝账号
	int32   GetVipAliAccRechargeStatus() { return m_VipAliAccRechargeStatus; }
	int64   GetVipAliAccRecharge() { return m_lVipAliAccRecharge; }
	map<string, tagVipRechargeWechatInfo> & GetVipAliAccInfo() { return m_mpVipProxyAliAccInfo; }
	int64 GetVipAliAccSpecialRecharge() { return m_lVipAliAccSpecialRecharge; }
	// 指定渠道是否属于vip代理支付宝充值特殊渠道
	// siteid : 指定渠道渠道
	bool IsInVipAliAccRechargeSpecialCnid(int32 siteid)
	    { return m_usVipAliAccSpecialRechargeCnid.find(siteid) != m_usVipAliAccSpecialRechargeCnid.end(); }

	int32   GetUnionPayRechargeStatus() { return m_UnionPayRechargeStatus; }
	int64   GetUnionPayRecharge() { return m_lUnionPayRecharge; }
	int64 GetUnionPaySpecialRecharge() { return m_lUnionPaySpecialRecharge; }
	// 指定渠道是否属于银联充值特殊渠道
	// siteid : 指定渠道渠道
	bool IsInUnionPayRechargeSpecialCnid(int32 siteid)
	    { return m_usUnionPaySpecialRechargeCnid.find(siteid) != m_usUnionPaySpecialRechargeCnid.end(); }

	int32   GetWeChatPayRechargeStatus() { return m_WeChatPayRechargeStatus; }
	int64   GetWeChatPayRecharge() { return m_lWeChatPayRecharge; }
	int64 GetWeChatPaySpecialRecharge() { return m_lWeChatPaySpecialRecharge; }
	// 指定渠道是否属于微信支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInWeChatPayRechargeSpecialCnid(int32 siteid)
	    { return m_usWeChatPaySpecialRechargeCnid.find(siteid) != m_usWeChatPaySpecialRechargeCnid.end(); }

	int32   GetAliPayRechargeStatus() { return m_AliPayRechargeStatus; }
	int64   GetAliPayRecharge() { return m_lAliPayRecharge; }
	int64 GetAliPaySpecialRecharge() { return m_lAliPaySpecialRecharge; }
	// 指定渠道是否属于支付宝支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInAliPayRechargeSpecialCnid(int32 siteid)
	    { return m_usAliPaySpecialRechargeCnid.find(siteid) != m_usAliPaySpecialRechargeCnid.end(); }
	
	int32   GetOtherPayRechargeStatus() { return m_OtherPayRechargeStatus; }
	int64   GetOtherPayRecharge() { return m_lOtherPayRecharge; }
	int64 GetOtherPaySpecialRecharge() { return m_lOtherPaySpecialRecharge; }
	// 指定渠道是否属于其他支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInOtherPayRechargeSpecialCnid(int32 siteid)
	    { return m_usOtherPaySpecialRechargeCnid.find(siteid) != m_usOtherPaySpecialRechargeCnid.end(); }

	int32   GetQQPayRechargeStatus() { return m_QQPayRechargeStatus; }
	int64   GetQQPayRecharge() { return m_lQQPayRecharge; }
	int64 GetQQPaySpecialRecharge() { return m_lQQPaySpecialRecharge; }
	// 指定渠道是否属于QQ支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInQQPayRechargeSpecialCnid(int32 siteid)
	    { return m_usQQPaySpecialRechargeCnid.find(siteid) != m_usQQPaySpecialRechargeCnid.end(); }

	int32   GetWeChatScanPayRechargeStatus() { return m_WeChatScanPayRechargeStatus; }
	int64   GetWeChatScanPayRecharge() { return m_lWeChatScanPayRecharge; }
	int64 GetWeChatScanPaySpecialRecharge() { return m_lWeChatScanPaySpecialRecharge; }
	// 指定渠道是否属于微信扫码支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInWeChatScanPayRechargeSpecialCnid(int32 siteid)
	    { return m_usWeChatScanPaySpecialRechargeCnid.find(siteid) != m_usWeChatScanPaySpecialRechargeCnid.end(); }

	int32   GetJDPayRechargeStatus() { return m_JDPayRechargeStatus; }
	int64   GetJDPayRecharge() { return m_lJDPayRecharge; }
	int64 GetJDPaySpecialRecharge() { return m_lJDPaySpecialRecharge; }
	// 指定渠道是否属于京东支付特殊渠道
	// siteid : 指定渠道渠道
	bool IsInJDPayRechargeSpecialCnid(int32 siteid)
	    { return m_usJDPaySpecialRechargeCnid.find(siteid) != m_usJDPaySpecialRechargeCnid.end(); }

	int32   GetApplePayRechargeStatus() { return m_ApplePayRechargeStatus; }
	int64   GetApplePayRecharge() { return m_lApplePayRecharge; }

	int32   GetLargeAliPayRechargeStatus() { return m_LargeAliPayRechargeStatus; }
	int64   GetLargeAliPayRecharge() { return m_lLargeAliPayRecharge; }
	int64 GetLargeAliPaySpecialRecharge() { return m_lLargeAliPaySpecialRecharge; }
	// 指定渠道是否属于大额支付宝特殊渠道
	// siteid : 指定渠道渠道
	bool IsInLargeAliPayRechargeSpecialCnid(int32 siteid)
	{ return m_usLargeAliPaySpecialRechargeCnid.find(siteid) != m_usLargeAliPaySpecialRechargeCnid.end(); }

	// 个人专属支付宝
	int32 GetExclusiveAlipayRechargeStatus() { return m_exclusiveAlipayRechargeStatus; }
	int64 GetExclusiveAlipayRecharge() { return m_exclusiveAlipayRecharge; }
	vector<tagExclusiveAlipayInfo>& GetExclusiveAlipayInfo() { return m_vExclusiveInfo; }
	int64 GetExclusiveAlipaySpecialRecharge() { return m_exclusiveAlipaySpecialRecharge; }
	// 指定渠道是否属于专属支付宝特殊渠道
	// siteid : 指定渠道渠道
	bool IsInExclusiveAlipayRechargeSpecialCnid(int32 siteid)
	    { return m_usExclusiveAlipaySpecialRechargeCnid.find(siteid) != m_usExclusiveAlipaySpecialRechargeCnid.end(); }

	// 定额支付宝
	int32 GetFixedAlipayRechargeStatus() { return m_fixedAlipayRechargeStatus; }
	int64 GetFixedAlipayRecharge() { return m_lFixedAlipayRecharge; }
	int64 GetFixedAlipaySpecialRecharge() { return m_lFixedAlipaySpecialRecharge; }
	// 指定渠道是否属于定额支付宝特殊渠道
	// siteid : 指定渠道渠道
	bool IsInFixedAlipayRechargeSpecialCnid(int32 siteid)
	{ return m_usFixedAlipaySpecialRechargeCnid.find(siteid) != m_usFixedAlipaySpecialRechargeCnid.end(); }

	// 定额微信
	int32 GetFixedWechatRechargeStatus() { return m_fixedWechatRechargeStatus; }
	int64 GetFixedWechatRecharge() { return m_lFixedWechatRecharge; }
	int64 GetFixedWechatSpecialRecharge() { return m_lFixedWechatSpecialRecharge; }
	// 指定渠道是否属于定额微信特殊渠道
	// siteid : 指定渠道渠道
	bool IsInFixedWechatRechargeSpecialCnid(int32 siteid)
	{ return m_usFixedWechatSpecialRechargeCnid.find(siteid) != m_usFixedWechatSpecialRechargeCnid.end(); }

	// 定额银联云闪付
	int32 GetFixedUnionpayRechargeStatus() { return m_fixedUnionpayRechargeStatus; }
	int64 GetFixedUnionpayRecharge() { return m_lFixedUnionpayRecharge; }
	int64 GetFixedUnionpaySpecialRecharge() { return m_lFixedUnionpaySpecialRecharge; }
	// 指定渠道是否属于定额微信特殊渠道
	// siteid : 指定渠道渠道
	bool IsInFixedUnionpayRechargeSpecialCnid(int32 siteid)
	{ return m_usFixedUnionpaySpecialRechargeCnid.find(siteid) != m_usFixedUnionpaySpecialRechargeCnid.end(); }

	// 专享闪付
	int32 GetExclusiveFlashRechargeStatus() { return m_exclusiveFlashRechargeStatus; }
	int64 GetExclusiveFlashRecharge() { return m_lExclusiveFlashRecharge; }
	int64 GetExclusiveFlashSpecialRecharge() { return m_lExclusiveFlashSpecialRecharge; }
	// 指定渠道是否属于定额微信特殊渠道
	// siteid : 指定渠道渠道
	bool IsInExclusiveFlashRechargeSpecialCnid(int32 siteid)
	{ return m_usExclusiveFlashSpecialRechargeCnid.find(siteid) != m_usExclusiveFlashSpecialRechargeCnid.end(); }
	// 专享闪付账号信息
	map<string, tagVipRechargeWechatInfo>& GetVipProxyFlashAccInfo() { return m_mpVipProxyFlashAccInfo; }

protected:
    bool   InitSysConf();
	// 读取开关和充值金额下限 add by har
	bool StatusRechargeAnalysis(Json::Value &jValueData, int32 &rechargeStatus, int64 &recharge, int64 &specialRecharge, unordered_set<int32> &usSpecialRechargeCnid);
	// 读取vip代理账号信息 add by har
	bool VipProxyAccountInfoAnalysis(Json::Value &jValueData, Json::Reader &reader, map<string, tagVipRechargeWechatInfo> &mpVipProxyAccountInfo);

public:
	map<uint32, stRobotOnlineCfg>    m_mpRobotCfg;	
	map<uint32, stRobotOnlineCfg>    m_mpRobotCfgEx;
	map<uint32, vector<stRobotOnlineCfg>>    m_mpRobotCfgAllCount;
	uint32  BatchServiceRemaindTime(uint32 dwEnterTime, uint32 dwLeaveTime, uint32 dwTodayTickCount);

	void GetRobotAllDayInfo(map<uint32, stRobotOnlineCfg> & refCfg);
	map<uint32, vector<stRobotOnlineCfg>> & GetRobotAllDayLoadCount();
	bool SubCfgRobotLoadindex(uint32 gameType);

	uint32 GetRobotCfgCountByBatch(uint32 lv, uint32 batchid);

	void GetRobotBatchInfo(map<uint32, stRobotOnlineCfg> & refCfg);
	int32  GetRobotBatchOut(uint32 batchID);
	bool GetRobotGameCfg(int batchid,stRobotOnlineCfg & cfg);
	void LoadRobotOnlineCount();

	void UpdateRobotLvOnline(int gametype,int roomid,int leveltype, int batchid, vector<int> & vecOnline);
	void UpdateTimeRobotOnline(int gametype, int roomid,int leveltype, int batchid, int logintype, int entertimer, int leavetimer, vector<int> & vecOnline);
	bool DeleteRobotOnlineByBatchID(int batchid);
	void TotailRobotAllDayCount();

public:

	//判断当前玩家是否对游戏可控
	bool	GetUserControlFlag(uint32 uid, uint32 game_id, string device_id, string check_code);

	//获取精准控制配置信息
	void    GetUserControlCfg(map<uint32, tagUserControlCfg> &mpInfo);

	//更新精准控制配置信息
	bool	UpdateUserControlInfo(uint32 uid, uint32 oper_type, tagUserControlCfg info);

	// 用户是否为被跟踪用户
	bool	GetIsUserControl(uint32 uid, uint32 gameType, uint32 & suid);

	void    InitMasterShowInfo();

	string  GetMasterRandCity();

	uint32  GetMasterRandUid();

	//幸运值
	void	LoadLuckyConfig(uint32 uid, uint32 gameType, map<uint8, tagUserLuckyCfg> &mpLuckyCfg);
	void	UpdateLuckyInfo(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyInfo);

protected:
    typedef map<uint32,stMissionCfg> MAP_MISSCFG;
    typedef map<string,string>       MAP_SYSCFG;
    typedef map<uint32,stServerCfg>  MAP_SVRCFG;

    MAP_MISSCFG  m_Missioncfg;
    MAP_SYSCFG   m_mpSysCfg;
    MAP_SVRCFG   m_mpSvrCfg;
    stServerCfg  m_curSvrCfg;

    int32          m_givetax[2];       // 赠送税收
    vector<int32>  m_cloginrewards;    // 连续登陆奖励积分
    vector<int32>  m_wloginrewards;    // 周累计登陆奖励
    int32          m_bankrupt[4];      // 破产补助
    int32          m_proomprice;       // 私人房日租金
    int32          m_speakCost;        // 喇叭收费
    int32          m_jumpQueueCost;    // 插队收费
    int32          m_signGameCount;    // 签到游戏局数
    
    
    map<uint32,vector<uint32> > m_mplandbasescores; // 斗地主私人房底分配置
    map<uint32,vector<uint32> > m_mplandentermin;   // 斗地主私人房进入配置
    map<uint32,vector<uint32> > m_mplandfees;       // 斗地主台费配置
    map<uint32,uint32>          m_mplandenterparam; // 最小进入系数

    map<uint32,stExchangeCfg>   m_mpExchangeDiamond;// 钻石兑换
    map<uint32,stExchangeCfg>   m_mpExchangeCoin;   // 财富币兑换
    map<uint32,stExchangeCfg>   m_mpExchangeScore;  // 积分兑换

    vector<stShowHandBaseCfg>   m_vecShowhandBaseCfg;       // 梭哈底注计算配置
    vector<int64>               m_vecShowhandBaseScoreCfg;  // 梭哈私人房底分配置
    vector<int64>               m_vecShowhandMinBuyinCfg;   // 梭哈私人房最小带入配置

    vector<int64>               m_vecTexasBaseScoreCfg;     // 德州底注计算配置
    vector<int64>               m_vecTexasMinBuyinCfg;      // 德州最小带入配置

	int32						m_iVipBroadcastStatus;		// vip广播状态 0 开启 1 关闭
	int64						m_lVippBroadcastRecharge;	// vip广播金额
	string						m_strVipBroadcast;			// vip广播消息

	int32						m_VipProxyRechargeStatus;	// 0 开启 1 关闭
	int64						m_lVipProxyRecharge;		// vip金额
	map<string, tagVipRechargeWechatInfo>          m_mpVipProxyWeChatInfo;		// vip微信信息
	int64                m_lVipProxySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usVipProxySpecialRechargeCnid;   // 特殊渠道号列表

	int32						m_VipAliAccRechargeStatus;	// 0 开启 1 关闭
	int64						m_lVipAliAccRecharge;		// vip金额
	map<string, tagVipRechargeWechatInfo>          m_mpVipProxyAliAccInfo;		// vip支付宝信息
	int64                m_lVipAliAccSpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usVipAliAccSpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_UnionPayRechargeStatus;	// 0 开启 1 关闭
	int64						m_lUnionPayRecharge;		// 银联金额
	int64                m_lUnionPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usUnionPaySpecialRechargeCnid;   // 特殊渠道号列表

	int32						m_WeChatPayRechargeStatus;
	int64						m_lWeChatPayRecharge;
	int64                m_lWeChatPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usWeChatPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_AliPayRechargeStatus;
	int64						m_lAliPayRecharge;
	int64                m_lAliPaySpecialRecharge;	     // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usAliPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_OtherPayRechargeStatus;
	int64						m_lOtherPayRecharge;
	int64                m_lOtherPaySpecialRecharge;	   // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usOtherPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_QQPayRechargeStatus;
	int64						m_lQQPayRecharge;
	int64                m_lQQPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usQQPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_WeChatScanPayRechargeStatus;
	int64						m_lWeChatScanPayRecharge;
	int64                m_lWeChatScanPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usWeChatScanPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_JDPayRechargeStatus;
	int64						m_lJDPayRecharge;
	int64                m_lJDPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usJDPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32						m_ApplePayRechargeStatus;
	int64						m_lApplePayRecharge;

	int32						m_LargeAliPayRechargeStatus;
	int64						m_lLargeAliPayRecharge;
	int64                m_lLargeAliPaySpecialRecharge;	    // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usLargeAliPaySpecialRechargeCnid;  // 特殊渠道号列表

	int32 m_exclusiveAlipayRechargeStatus;	         // 个人专属支付宝 0 开启 1 关闭
	int64 m_exclusiveAlipayRecharge;	             // 充值数大于等于最低充值金额才显示个人专属支付宝
	vector<tagExclusiveAlipayInfo> m_vExclusiveInfo; // 专属支付宝信息
	int64 m_exclusiveAlipaySpecialRecharge;	         // 充值数大于等于最低充值金额才显示个人专属支付宝特殊渠道
	unordered_set<int32> m_usExclusiveAlipaySpecialRechargeCnid; // 特殊渠道号列表

	int32				 m_fixedAlipayRechargeStatus;        // 0 开启  1 关闭
	int64				 m_lFixedAlipayRecharge;
	int64                m_lFixedAlipaySpecialRecharge;	     // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usFixedAlipaySpecialRechargeCnid; // 特殊渠道号列表

	int32				 m_fixedWechatRechargeStatus;        // 0 开启  1 关闭
	int64				 m_lFixedWechatRecharge;
	int64                m_lFixedWechatSpecialRecharge;	     // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usFixedWechatSpecialRechargeCnid; // 特殊渠道号列表

	int32				 m_fixedUnionpayRechargeStatus;        // 0 开启  1 关闭
	int64				 m_lFixedUnionpayRecharge;
	int64                m_lFixedUnionpaySpecialRecharge;	   // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usFixedUnionpaySpecialRechargeCnid; // 特殊渠道号列表

	map<string, tagVipRechargeWechatInfo>  m_mpVipProxyFlashAccInfo; // vip代理闪付账号信息
	int32				 m_exclusiveFlashRechargeStatus;        // 0 开启  1 关闭
	int64				 m_lExclusiveFlashRecharge;
	int64                m_lExclusiveFlashSpecialRecharge;	  // 充值数大于等于最低充值金额才显示特殊渠道
	unordered_set<int32> m_usExclusiveFlashSpecialRechargeCnid; // 特殊渠道号列表

	vector<tagNewPlayerWelfareValue>    m_vecNewPlayerWelfareValue; // 新手福利数值
	//vector<uint32>				m_vecAutoKillingUsers;      // 自动杀分玩家列表

	//stAutoKillCfg				m_AutoKillingCfg;      		// 自动杀分配置

	// 精准控制---配置信息
	map<uint32, tagUserControlCfg>		m_mpUserControlCfg;
	vector<string>				m_vecMasterCity;
	vector<uint32>				m_vecMasterUid;

};




#endif //
