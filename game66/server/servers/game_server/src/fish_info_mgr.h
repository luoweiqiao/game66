//
// Created by Joe on 2019-01-26 捕鱼管理类
//

#ifndef FISH_CFG_MGR_H
#define FISH_CFG_MGR_H

#include "svrlib.h"
#include "db_struct_define.h"
#include "game_player.h"

#define		COORDINATE_PRECISION		10000				//路径坐标保留小数点后4位
#define		MAX_FISH_DIRECTORY			10000				//每一条路径上鱼的最大数量,用于鱼的序号,达到最大值,又从0开始
//每一条鱼的编号格式 用7位数表示  高3位表示路径编号  低4位表示该路径上鱼的编号   例如: 鱼编号 1019876 表示在路径编号101的线路上鱼的序列号为9876

static const int FRUSH_TIME				= 30;			//刷新频率 单位毫秒
static const int SCORE_TIME				= 10 * 1000;	//定时写入玩家战绩 10秒
static const int CLEAR_FISH_TIME		= 5 * 1000;		//定时清除超地图的鱼 5秒
static const int MAX_MAP_NUM			= 100;			//最大地图数

//同一路径下多鱼的位置信息---读取配置使用
struct fishSetInfo
{
	uint32	id;
	uint32	startTime;
	vector<uint32>			walkLines;	//只包含walktime属性
};

//同一路径下多鱼的位置信息
struct createFishSetInfo
{
	uint32	id;			//编号ID
	uint32	cost_time;	//该鱼在地图上的游走时间
};

//鱼路径信息
struct lineDataInfo
{
	uint32	id;
	string	name;
	uint32	count;
	uint32	fishWaitTime;
	uint32	color;
	vector<uint32>			walkLines;	//只包含walktime属性
	vector<fishSetInfo>		fishSets;
};

//鱼信息
struct FishDataInfo
{
	uint32			id;
	uint32			fx;
	string			name;
	string			res;
	uint32			sFrame;
	uint32			eFrame;
	uint32			R;
	uint32			life;
	uint32			gold;
	uint32			minM;
	uint32			maxM;
	int32			shx;
	int32			shy;
	int32			seabed;
	vector<uint32>	vecboomID;
};

//子弹效果信息
struct hidEffectInfo
{
	uint32			efid;
	uint32			bdong;
	uint32			scale;
};

//子弹信息
struct ZidanDataInfo
{
	uint32			id;
	uint32			fireCD;
	uint32			fireType;
	string			name;
	uint32			type;
	uint32			lv;
	uint32			R;
	uint32			speed;
	uint32			destroy;
	uint32			money;
	uint32			killCD;
	uint32			rebound;
};

//生成鱼的定时器信息
struct CreateFishTimeInfo
{
	uint32		fish_curr;			//已经生成鱼的数量
	uint32		fish_serial;		//鱼的序列号
	uint64      next_create_time;	//下一条鱼的产生时间
	bool		is_create;			//是否还需要产生
	//CCooling	timer;				//定时器信息
	CreateFishTimeInfo()
	{
		fish_curr = 0;
		fish_serial = 0;
		next_create_time = 0;
		is_create = false;
	}
};

//地图配置信息---对应level.json
struct MapCfgDataInfo
{
	uint32			id;				//线路ID
	uint32			waitTime;		//线路开始等待时间	
	uint32			lineID;			//路径ID---对应于baseLineData.json中的路径ID
	float32			walkTime;		//百分比---将baseLineData.json中walkTime乘以该百分比	
	uint32			count;			//鱼的总数---替换baseLineData.json中的count
	uint32			fishID;			//鱼的类型---对应于fishRes.json中的ID
	uint32			fishWaitTime;	//产生鱼的间隔时间---替换baseLineData.json中的fishWaitTime	

	bool			is_single;		//是否为单鱼的配置---区分walkLines与fishSet的配置
	uint32          wl_cost_time;	//鱼在地图上的总共游走时间---walkLines
		
	vector<createFishSetInfo> fs_list;		//鱼在地图上的总共游走时间---fishSet

	MapCfgDataInfo()
	{
		id = 0;
		waitTime = 0;
		lineID = 0;
		walkTime = 1;		
		count = 0;
		fishID = 0;
		fishWaitTime = 0;
		is_single = false;
		wl_cost_time = 0;
		fs_list.clear();
	}
};

//每条鱼的信息
struct FishInfo
{
	uint64			createTime;		//创建时间
	uint32			fish_no;		//鱼的编号
	uint32			fish_type;		//鱼的类型---对应于fishRes.json中的ID
	bool			fish_dead;		//是否已死
	uint64			deadTime;		//死亡时间
	FishInfo()
	{
		createTime = 0;
		fish_no = 0;
		fish_type = 0;
		fish_dead = false;
		deadTime = 0;
	}
};

//当前正在使用的地图信息
struct CurrMapInfo
{
	uint32			id;						//线路ID
	uint32			waitTime;				//线路开始等待时间	
	uint32			lineID;					//路径ID---对应于baseLineData.json中的路径ID
	uint32			count;					//鱼的总数---替换baseLineData.json中的count
	uint32			fishID;					//鱼的类型---对应于fishRes.json中的ID
	uint32			fishWaitTime;			//产生鱼的间隔时间---替换baseLineData.json中的fishWaitTime

	bool			is_single;				//是否为单鱼的配置---区分walkLines与fishSet的配置
	uint32          wl_cost_time;			//鱼在地图上的总共游走时间---walkLines
	
	vector<createFishSetInfo> fs_list;		//鱼在地图上的总共游走时间---fishSet

	CreateFishTimeInfo create_info;			//生成鱼的过程信息

	CurrMapInfo()
	{
		id = 0;
		waitTime = 0;
		lineID = 0;		
		count = 0;
		fishID = 0;
		fishWaitTime = 0;
		is_single = false;
		wl_cost_time = 0;
		fs_list.clear();
	}
};

class CFishCfgMgr : public AutoDeleteSingleton<CFishCfgMgr>
{
public:
	CFishCfgMgr();
    ~CFishCfgMgr();

    // 初始化
    bool	Init();

    // 根据鱼的种类获取相应的配置信息
    bool    GetFishCfgInfo(CGameRoom *pHostRoom, CGamePlayer* pPlayer, uint32 fish_id, uint32 &ret, bool &is_luck_ctrl, bool &is_luck_win, bool &is_aw_ctrl, bool &isNoviceCtrl);

    // PHP更新配置信息
    void    UpdateFishCfg(uint32 id, uint32 prize_min, uint32 prize_max, uint32 kill_rate);

	// 读取配置文件
	bool	OnReadAllCfgFile();
	bool	OnReadBaseLineData(string msg);					//baseLineData.json
	bool	OnReadFishRes(string msg);						//fishRes.json
	bool	OnReadZidanSet(string msg);						//zidanSet.json
	bool	OnReadLevelSet(string msg, uint8 level_id);		//level.json		目前支持多种地图

	// 根据地图ID获取对应的地图信息
	bool  GetCurrMapInfo(uint8 map_level, uint32 &map_allTime, vector<CurrMapInfo> &map_info);

	// 创建当前地图的鱼群线路
	void	OnCreateFishDirect(uint32 map_id);

	// 重置提升/降低击中鱼的概率列表---房间配置
	void    ResetHitFishProEvelCfg(vector<uint32> &vec_pro_elev);
    	
private: 

    map<uint8, tagFishInfoCfg>			m_mpFishCfg;						//对应fish_cfg配置表

	map<uint32, lineDataInfo>			m_mapLineCfgData;					//所有路径配置信息
	map<uint32, FishDataInfo>			m_mapFishCfgData;					//所有鱼的配置信息
	map<uint32, ZidanDataInfo>			m_mapZidanCfgData;					//所有子弹的配置信息
	map<uint32, vector<MapCfgDataInfo>>	m_mapCfgDataInfo;					//所有的地图配置信息 key:地图等级

	map<uint32, uint32>					m_mapCostTimeInfo;					//所有的地图行走时间 key:地图等级
	
	vector<uint32>						m_vec_pro_elev;						//提升/降低击中鱼的概率列表
};







#endif //ACTIVE_PLAYER_MGR_H
