

#ifndef _GAME_SERVER_CONFIG_H__
#define _GAME_SERVER_CONFIG_H__

#include <string>
#include "fundamental/noncopyable.h"
#include "luatinker/luaTinker.h"
#include "svrlib.h"
#include <string.h>
#include "config/config.h"
#include "game_define.h"

using namespace std;
using namespace svrlib;

/**
 * 单例，用于存放配置
 */

struct GameServerConfig : public AutoDeleteSingleton<GameServerConfig>
{
public:
	stRedisConf	  redisConf[REDIS_INDEX_MAX];
	stDBConf	  DBConf[DB_INDEX_TYPE_MAX];
	stRedisConf*  GetRedisConf(uint8 index)
	{
		if(index < REDIS_INDEX_MAX)
		{
			return &redisConf[index];
		}
		return NULL;
	}
	stDBConf*  GetDBConf(uint8 index)
	{
		if(index < DB_INDEX_TYPE_MAX)
		{
			return &DBConf[index];
		}
		return NULL;		
	}
};



// 导出Lua函数
extern void	defLuaConfig(lua_State* pL);



#endif  // _GAME_SERVER_CONFIG_H__

