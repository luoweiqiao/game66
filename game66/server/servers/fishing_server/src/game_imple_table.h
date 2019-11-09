//
// Created by Joe on 19/6/3.
//
// 捕鱼的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "pb/error_code.pb.h"
#include "fish_info_mgr.h"

using namespace svrlib;
using namespace std;

//宏定义
static const int GAME_PLAYER = 4;			//每桌游戏人数

struct KillFishInfo
{
	int32 add_value;
	int32 times;
	KillFishInfo()
	{
		add_value = 0;
		times = 0;
	}
};

//新手福利的统计数据
struct NoviceCtrlInfo
{
	int32 times;
	int32 win_socre;
	NoviceCtrlInfo()
	{
		times = 0;
		win_socre = 0;
	}
};

class CGamePlayer;
class CGameRoom;

// 捕鱼游戏桌子
class CGameFishingTable : public CGameTable
{
public:
	CGameFishingTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameFishingTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
	virtual bool    LeaveTable(CGamePlayer* pPlayer, bool bNotify = false); // add by har
	virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

public:
    //配置桌子
    virtual bool Init();
	virtual bool ReAnalysisParam();
    virtual void ShutDown();

    //结束游戏，复位桌子
    virtual void ResetTable();
    virtual void OnTimeTick();

    //游戏消息
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
 
public:
    // 游戏开始
    virtual bool OnGameStart();
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
	
public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer); 
	
protected:
	
	//击中目标
	void    OnUserHitFish(uint32 seat_id, uint32 bullet_id, uint32 bullet_bot, uint32 fish_no, uint64 time_stamp);

    //定时同步数据到数据库及redis
    void    OnSynDBAndRedis();

	//玩家退出同步数据到数据库及redis
	void    OnLeaveSynDBAndRedis(CGamePlayer* pPlayer, uint32 seat_id);

	//初始化金流日志
	void	InitBlingLog(uint32 seat_id);
	
	//定时写入金流log
	void	WriteAddScoreLog(uint32 uid, int64 win, uint32 seat_id);
		
	//判断是否产生新鱼---定时器调用
	void	OnCreateFishData();

	//清除地图上无效鱼信息---定时器调用
	void	OnClearInvalidFish();

	//获取地图ID
	uint32	GetMapID();	
	
private:
	
	uint64								m_start_time;						//游戏开始时间
	vector<FishInfo>					m_all_fish_info;					//所有已生成鱼的列表
	
	uint8								m_map_level;						//当前桌子的地图等级
	vector<uint8>						m_map_list;							//当前房间配置的地图列表---房间配置
	uint8                               m_map_pos;							//当前使用的地图列表下标	
	vector<uint32>						m_bot_list;							//当前房间配置的底分列表---房间配置
	vector<uint32>						m_arm_range_list;					//当前房间配置的瞄准倍数列表---房间配置

	vector<CurrMapInfo>                 m_curr_map_info;					//当前使用的地图信息---地图切换时就要重置

	uint32								m_map_cost_time;					//当前地图走完的花费时间
	CCooling							m_map_cycle_timer;					//地图循环使用定时器
	uint32								m_map_cycle_count;					//当前地图的循环使用次数

	int64								m_lTableScore[GAME_PLAYER];			//玩家的输赢值
	CCooling							m_score_timer;						//玩家战绩定时器

	CCooling							m_clear_fish_timer;					//清除无效鱼定时器

	uint32								m_lTableBulletID[GAME_PLAYER];		//玩家的射击子弹ID编号

	bool								m_create_fish_end;					//当前地图的所有鱼是否都已经生成
	
	map<uint32, map<uint32, uint32>>	m_bullet_classification;			//统计每个玩家使用的子弹消耗分类

	map<uint32, map<uint32, KillFishInfo>>	m_kill_classification;			//统计每个玩家打死鱼的奖励分类

	uint64								m_MsgTime[GAME_PLAYER];				//玩家的最新消息的统计值---用于清除长时间不操作的玩家

	map<uint32, NoviceCtrlInfo>	        m_player_novicectrlinfo;			//每个玩家新手福利的统计信息

};

#endif //SERVER_GAME_IMPLE_TABLE_H

