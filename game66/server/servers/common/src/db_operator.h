
#ifndef DB_OPERATOR_H__
#define DB_OPERATOR_H__

#include <string>
#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include "dbmysql/dbmysql.h"
#include "dbmysql/db_wrap.h"
#include "db_struct_define.h"
#include <vector>
#include <unordered_map>
#include "game_define.h"

using namespace std;
using namespace svrlib;

/*************************************************************/
class CDBOperator : public CDBWrap
{
public:
	CDBOperator(){}
	virtual ~CDBOperator(){}
	
public: 
	// 加载房间配置信息
	bool LoadRoomCfg(uint16 gameType,uint8 gameSubType,vector<stRoomCfg>& vecRooms);
	// 加载库存奖池房间配置信息  add by har
	bool LoadRoomStockCfg(uint16 gameType, unordered_map<uint16, stStockCfg>& vecRooms);
	// 加载机器人在线配置
	bool LoadRobotOnlineCfg(map<uint32,stRobotOnlineCfg>& mpRobotCfg);

	bool TimeLoadRobotOnlineEx(map<uint32, stRobotOnlineCfg>& mpRobotCfg);

    // 加载任务配置信息
    bool LoadMissionCfg(map<uint32,stMissionCfg>& mapMissions);
	// 加载系统配置
	bool LoadSysCfg(map<string,string>& mapSysCfg);
    // 加载服务器配置信息
	bool LoadSvrCfg(map<uint32,stServerCfg>& mapSvrCfg);
	// 加载兑换数据
	bool LoadExchangeDiamondCfg(map<uint32,stExchangeCfg>& mapExcfg);
	bool LoadExchangeCoinCfg(map<uint32,stExchangeCfg>& mapExcfg);
	bool LoadExchangeScoreCfg(map<uint32,stExchangeCfg>& mapExcfg);

	// 加载私人房
	bool LoadPrivateTable(uint16 gameType, vector<stPrivateTable>& tables);
	// 创建私人房信息
	uint32  CreatePrivateTable(stPrivateTable& table);
	// 判断用户是否存在
	bool IsExistPlayerID(uint32 id);

	// 加载夺宝游戏数据
	bool SynLoadSnatchCoinGameData(vector<tagSnatchCoinGameData>& vecGameData);

	// 加载vip代理账号信息
	// mapInfo out : 保存代理账号信息
	// type : 账号类型
	bool LoadVipProxyRecharge(map<string, tagVipRechargeWechatInfo> &mapInfo, int type);
	//bool LoadVipAliAccRecharge(map<string, tagVipRechargeWechatInfo> & mapInfo);

	bool LoadGameDataDiceWeekRank(vector<tagDiceGameDataRank> & vecGameData);

	bool GetAccountInfoByUid(uint32 uid, stAccountInfo & data);

	bool GetUserInfoByUid(uint32 uid, string & nickname, string & city);
	// 加载自动杀分玩家列表
	//bool LoadAutoKillUsers(vector<uint32>& vecAutoKillUsers);
	bool LoadNewPlayerWelfareValue(vector<tagNewPlayerWelfareValue> & vecNewPlayerWelfareValue);

    // 加载活跃福利配置表
    bool LoadActiveWelfareCfg(map<uint8, stActiveWelfareCfg> &mpCfg);

	// 加载新注册玩家福利配置表
	bool LoadNewRegisterWelfareCfg(stNewRegisterWelfareCfg &mpCfg);

	// 加载精准控制配置信息
	bool LoadUserControlCfg(map<uint32, tagUserControlCfg> & mapInfo);

	void LoadExclusiveAlipayRecharge(vector<tagExclusiveAlipayInfo> &vExclusiveInfo);

	// 加载玩家的幸运值配置
	bool LoadLuckyCfg(uint32 uid, uint32 gameType, map<uint8, tagUserLuckyCfg> &mpLuckyCfg);

	// 更新玩家的幸运值配置表
	bool UpdateLuckyCfg(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyCfg);

	// 更新玩家的幸运值日志表
	bool UpdateLuckyLog(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyCfg);

	// 加载捕鱼配置表
	bool LoadFishInfoCfg(map<uint8, tagFishInfoCfg> &mpCfg);
};




#endif // DB_OPERATOR_H__

