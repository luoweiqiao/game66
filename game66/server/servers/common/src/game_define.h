

#ifndef __GAME_DEFINE_H__
#define __GAME_DEFINE_H__

#include "svrlib.h"
#include <vector>

using namespace std;

static  const uint32 SECONDS_IN_MIN 		= 60;
static  const uint32 SECONDS_IN_30_MIN		= 1800;		// 半小时
static  const uint32 SECONDS_IN_ONE_HOUR	= 3600;		// 一小时
static	const uint32 SECONDS_IN_ONE_DAY 	= 24*3600;	// 一天的秒数	 
static	const uint32 SECONDS_IN_ONE_WEEK	= 7*86400;	// 一周的秒数

#define PRO_DENO_100w    1000000
#define PRO_DENO_10w  	 100000
#define PRO_DENO_10000   10000
#define PRO_DENO_100     100	

#define ROUTE_MSG_ID     2000
#define PHP_HEAD_LEN     4
#define ROBOT_MGR_ID     1000000000
// 最大游戏类型
//#define MAX_GAME_TYPE    10

#define MAX_LOOP_COUNT	(1000000)

// 数据库标示
enum DB_INDEX_TYPE
{
    DB_INDEX_TYPE_ACC       = 0,
    DB_INDEX_TYPE_CFG       = 1,
    DB_INDEX_TYPE_MISSION   = 2,
    DB_INDEX_TYPE_LOG       = 3,
    DB_INDEX_TYPE_MAX       = 4,
};
#define REDIS_INDEX_MAX  2
#define LOBBY_INDEX_MAX  1

// 服务器配置信息
struct stServerCfg
{
    uint32  svrid;
    uint32  group;
    uint32  svrType;
    uint32  gameType;
    uint8   gameSubType;
    string  svrip;
    uint32  svrport;
    string  svrlanip;
    uint32  svrlanport;
    uint32  phpport;
    uint8   openRobot;
    stServerCfg(){
      svrid     = 0;
      group     = 0;
      svrType   = 0;
      gameType  = 0;
      gameSubType = 0;
      svrip     = "";
      svrport   = 0;
      svrlanip  = "" ;
      svrlanport = 0;
      phpport   = 0;
      openRobot = 0;
    }
    void operator=(const stServerCfg& info)
    {
        svrid       = info.svrid;
        group       = info.group;
        svrType     = info.svrType;
        gameType    = info.gameType;
        gameSubType = info.gameSubType;
        svrip       = info.svrip;
        svrport     = info.svrport;
        svrlanip    = info.svrlanip;
        svrlanport  = info.svrlanport;
        phpport     = info.phpport;
        openRobot   = info.openRobot;
    }
};
// 服务器状态
enum emSERVER_STATE
{
    emSERVER_STATE_NORMAL   = 0, // 正常状态
    emSERVER_STATE_REPAIR,       // 维护状态

};
// 机器人AI类型
enum emROBOT_AI_TYPE
{
    emROBOT_AI_TYPE_NORMAL  = 0, // 正常的
    emROBOT_AI_TYPE_CARE,        // 谨慎型
    emROBOT_AI_TYPE_CRAZY,       // 激进型
};




#endif // __GAME_DEFINE_H__







































