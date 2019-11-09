
#ifndef __GAME_ROOM_H__
#define __GAME_ROOM_H__

#include <game_player.h>
#include <queue>
#include "svrlib.h"
#include "db_struct_define.h"
#include "pb/msg_define.pb.h"
#include "game_table.h"
#include "fish_info_mgr.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;
class CGameTable;

#define ROOM_GATE_START_TIME		(5*60*1000)		//(5分钟)


// 房间类型
enum emROOM_TYPE
{
    emROOM_TYPE_COMMON  = 0,// 普通房
    emROOM_TYPE_PRIVATE,    // 私人房
    emROOM_TYPE_MATCH,      // 比赛房
};

class CGameRoom : public ITimerSink
{
public:
    CGameRoom();
    virtual ~CGameRoom();

    virtual void OnTimer(uint8 eventID);

    bool    Init(uint16 gameType);
    void    ShutDown();
    void    OnTimeTick();

    bool    EnterRoom(CGamePlayer* pGamePlayer);
	bool    EnterEveryColorRoom(CGamePlayer* pGamePlayer);
    bool    LeaveRoom(CGamePlayer* pGamePlayer);

    uint8   EnterTable(CGamePlayer* pGamePlayer,uint32 tableID,string passwd);
    bool    FastJoinTable(CGamePlayer* pGamePlayer);
	bool    FastJoinEveryColorTable(CGamePlayer* pGamePlayer);

    void    SetRoomCfg(stRoomCfg & cfg){ m_roomCfg = cfg; }
	void	SetRoomCfgParam(string param) { m_roomCfg.param = param; }
	// 设置房间库存配置 add by har
	void    SetRoomStockCfg(stStockCfg &cfg) { m_roomStockCfg = cfg; }
	// 获取房间库存配置 add by har
	stStockCfg& GetRoomStockCfg() { return m_roomStockCfg; }
	// 修改房间库存配置 add by har
	void    ChangeRoomStockCfg(stStockCfg &cfg);
    uint16  GetRoomID(){ return m_roomCfg.roomID; }
    uint8   GetConsume(){ return m_roomCfg.consume; }
    uint8   GetDeal(){ return m_roomCfg.deal; }
    int64   GetEnterMin(){ return m_roomCfg.enter_min; }
    int64   GetEnterMax(){ return m_roomCfg.enter_max; }
    int64   GetRobotMaxScore(){ return m_roomCfg.robotMaxScore; }
	int64   GetRobotMinScore() { return m_roomCfg.robotMinScore; }
	int64   GetJettonMin() { return m_roomCfg.jettonMinScore; }
    int64   GetExitChip(){ return m_roomCfg.exitchip; }

    int32   GetBaseScore(){ return m_roomCfg.baseScore; }
    uint8   GetRoomType(){ return m_roomCfg.roomType; }
    uint8   GetShowHandNum(){ return m_roomCfg.showhandNum; }
    uint8   GetRobotCfg(){ return m_roomCfg.robot; }
	void    SetRobotCfg(uint8 robot) { m_roomCfg.robot = robot; }
    int64   GetSitDown(){ return m_roomCfg.sitdown; }
    string  GetCfgParam(){ return m_roomCfg.param; }
	int32   GetSortID() { return m_roomCfg.sortID; }

    bool    IsNeedMarry();

    int32   GetPlayerNum(){ return m_playerNum; }
    uint16  GetGameType(){ return m_gameType; }

    bool    CanEnterRoom(CGamePlayer* pGamePlayer);
    bool    CanLeaveRoom(CGamePlayer* pGamePlayer);

    int64   GetPlayerCurScore(CGamePlayer* pPlayer);

    void    GetRoomInfo(net::room_info* pRoom);
    void    SendTableListToPlayer(CGamePlayer* pGamePlayer,uint32 tableID);
    void    QueryTableListToPlayer(CGamePlayer* pGamePlayer,uint32 start,uint32 end);

    CGameTable* CreateTable(uint32 tableID);
    // 继承房间配置信息
    void    InheritRoomCfg(CGameTable* pTable);

    CGameTable* MallocTable();
    void        FreeTable(CGameTable* pTable);

    CGameTable* GetTable(uint32 tableID);

    CGameTable* CreatePlayerTable(CGamePlayer* pPlayer,stTableConf & cfg);

    // 检测回收空桌子
    void    CheckRecover();
    // 检测是否需要生成新桌子
    void    CheckNewTable();
    // 匹配桌子
    void    MarryTable();
    // 进入空闲桌子
    bool    JoinNoFullTable(CGamePlayer* pPlayer,uint32 excludeID);
    // 加入匹配
    void    JoinMarry(CGamePlayer* pPlayer,uint32 excludeID);
    void    LeaveMarry(CGamePlayer* pPlayer);
    bool    IsJoinMarry(CGamePlayer* pPlayer);

    uint32  GetFreeTableNum();
    // 获取所有桌子
    void    GetAllFreeTable(vector<CGameTable*>& tables);

    // 桌子列表比较函数
    static  bool CompareTableList(CGameTable* pTable1,CGameTable* pTable2);
    // 桌子排队比较函数
    static  bool CompareTableMarry(CGameTable* pTable1,CGameTable* pTable2);
    // 桌子ID排序
    static  bool CompareTableID(CGameTable* pTable1,CGameTable* pTable2);
	// 检查param
	// return : 0 成功，非0 错误码  modify by har
	int CheckRoommParambyGameType(uint32 gametype, string param);
	// 重新解析RoomParam
	void ReAnalysisRoomParam();
	//更新控制玩家
	void UpdateControlPlayer(int64 id, uint32 uid, uint32 gamecount,uint32 operatetype);

	void UpdateControlMultiPlayer(int64 id, uint32 uid, uint32 gamecount, uint64 gametime,int64 totalscore, uint32 operatetype);
	//停止夺宝
	bool SetSnatchCoinState(uint32 stop);
	//机器人夺宝
	bool RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount);

	bool DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[]);

	bool MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard);

	//控制玩家局数同步
	bool SynControlPlayer(uint32 tableID, uint32 uid, int32 gamecount, uint32 operatetype);
	bool StopServer();
	//游戏记录
	void SendFirstGamePlayLogInfo(CGamePlayer * pPlayer);
	bool CheckRoomCanGameStart();
	bool GetCanGameStart() { return m_bCanGameStart; }

protected:
    void    CalcShowOnline();
	bool	ReAnalysisParam();
public:
	tagJackpotScore GetJackpotScoreInfo() { return m_tagJackpotScore; }
	void UpdateJackpotScore(int64 lScore);
	void ResetJackpotScore();
	int64 GetPlayerMaxWinScore() { return m_lPlayerMaxWinScore; }
	void OnNewDay();
	int GetNoviceWelfare() { return m_tagNewPlayerNoviceWelfare.newroom; }
	int GetNoviceWelfareOwe() { return m_tagNewPlayerNoviceWelfare.isnewnowe; }

	int GetNoviceMaxJetton() { return m_tagNewPlayerNoviceWelfare.newmaxjetton; }
	int GetNoviceMaxWin() { return m_tagNewPlayerNoviceWelfare.newsmaxwin; }
	int GetMaxJettonRate() { return m_tagNewPlayerNoviceWelfare.maxjettonrate; }
	uint32 GetRobotBankerWinPro() { return m_tagNewPlayerNoviceWelfare.nobbw; }
	uint32 GetRobotBankerMaxCardPro() { return m_tagNewPlayerNoviceWelfare.normc; }
	uint32 GetPlayerBankerLosePro() { return m_tagNewPlayerNoviceWelfare.nopbl; }
	uint32 GetSysBankerWinPro() { return m_tagNewPlayerNoviceWelfare.nosbw; }
	tagRangeWelfare GetRangeWelfareByPosRmb(uint32 uid, uint32 posrmb);

	//int32 GetAutoKillSocre() { return m_tagAutoKillScore.autokillsocre; }
	//int32 GetAutoKillStatus() { return m_tagAutoKillScore.state; }
	//void SetAutoKillStatus(int status) { m_tagAutoKillScore.state = status; }
	//void AddAutoKillDefeat() { m_tagAutoKillScore.interval = 0; m_tagAutoKillScore.defeat++; }
	//void AddAutoKillInterval() { m_tagAutoKillScore.defeat = 0;  m_tagAutoKillScore.interval++; }
	//int32 GetAutoKillDefeat() { return m_tagAutoKillScore.defeat; }
	//int32 GetAutoKillInterval() { return  m_tagAutoKillScore.interval; }
	//uint32 GetAoRobotBankerWinPro() { return m_tagAutoKillScore.aobbw; }
	//uint32 GetAoRobotBankerMaxCardPro() { return m_tagAutoKillScore.aormc; }
	//uint32 GetAoPlayerBankerLosePro() { return m_tagAutoKillScore.aopbl; }
	//uint32 GetAoSysBankerWinPro() { return m_tagAutoKillScore.aosbw; }
	//uint32 GetAoRbwRobotBankerWinPro() { return m_tagAutoKillScore.aorbw; }
	uint32 GetMinPayRmb()
	{
		uint32 pay = 0;
		if (m_tagNewPlayerNoviceWelfare.newpayrange.size() == 2)
		{
			pay = m_tagNewPlayerNoviceWelfare.newpayrange[0];
		}
		return pay;
	}
	uint32 GetMaxPayRmb()
	{
		uint32 pay = 0;
		if (m_tagNewPlayerNoviceWelfare.newpayrange.size() == 2)
		{
			pay = m_tagNewPlayerNoviceWelfare.newpayrange[1];
		}
		return pay;
	}
	
    int GetAWRate() { return m_aw_rate; }
	bool GetNrwRoom() { return m_nrw_room; }

	int64 GetShowMasterMinScore() { return m_MasterShowUserInfo.lMinScore; }
	int64 GetShowMasterMaxScore() { return m_MasterShowUserInfo.lMaxScore; }

	// 是否触发库存变牌  add by har
	// return 0 不触发  -1 玩家必输  -2 玩家必赢  >0 触发奖池，玩家最多的赢数
	int64 IsStockChangeCard(CGameTable *pTable);
	// 每局结束,更新库存信息 add by har
	// playerWinScore : 玩家总赢分
	void UpdateStock(CGameTable *pTable, int64 playerWinScore);

protected:
    typedef queue<CGameTable*> QUEUE_TABLE;
    typedef stl_hash_map<uint32,CGameTable*> MAP_TABLE;
    typedef map<CGamePlayer*,uint32> QUEUE_PLAYER;

    MAP_TABLE       m_mpTables;         // 在玩桌子
    QUEUE_TABLE     m_freeTable;        // 空闲桌子
    QUEUE_PLAYER    m_marryPlayers;     // 匹配队列玩家
    uint32          m_roomIndex;        // 分配房间索引
    uint32          m_playerNum;        // 玩家人数
    uint32          m_showonline;       // 显示在线
    stRoomCfg       m_roomCfg;          // 房间配置
	stStockCfg      m_roomStockCfg;     // 房间库存配置  add by har
    uint16          m_gameType;         // 游戏类型
    CCooling        m_coolMarry;        // 搓桌CD
    CCooling        m_coolRecover;      // 回收桌子CD
    CCooling        m_coolNewTable;     // 检测生成新桌子
    CTimer*         m_pFlushTimer;
	tagJackpotScore	m_tagJackpotScore;	// 奖池控制变量
	int64			m_lPlayerMaxWinScore;//玩家赢最大的分数
	tagNewPlayerNoviceWelfare m_tagNewPlayerNoviceWelfare;

    uint16          m_aw_rate;          //活跃福利功能 用于判断当前房间亏损人数达到总人数的百分比，才触发活跃福利功能
	//tagAutoKillScore m_tagAutoKillScore;
	bool						m_bCanGameStart;	// 游戏是否能够开始
	uint64						m_bGameRunTime;
	uint64						m_bGameMoreTime;

	bool            m_nrw_room;         // 新账户福利功能 用于判断当前房间的玩家是否享受该功能
	
	struct tagMasterShowUserInfo m_MasterShowUserInfo;//显示信息

	uint64			m_curr_sys_time;		//定时器使用---精确到秒
};

#endif //__GAME_ROOM_H__

