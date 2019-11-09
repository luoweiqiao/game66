

#ifndef _LOBBY_SERVER_CONFIG_H__
#define _LOBBY_SERVER_CONFIG_H__

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

struct LobbyServerConfig : public AutoDeleteSingleton<LobbyServerConfig>
{
public:	
	uint8		  bNeedPassWD;
	stRedisConf	  redisConf[REDIS_INDEX_MAX];
	stDBConf	  DBConf[DB_INDEX_TYPE_MAX];

	void	SetNeedPassWD(uint8 bNeed)
	{
		bNeedPassWD = bNeed;
	}
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
	LobbyServerConfig()
	{
		bNeedPassWD = 0;
	}
};



// 导出Lua函数
extern void	defLuaConfig(lua_State* pL);



#endif  // _LOBBY_SERVER_CONFIG_H__

