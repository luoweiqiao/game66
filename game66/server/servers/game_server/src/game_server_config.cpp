

#include "game_server_config.h"

using namespace svrlib;
using namespace std;



// 导出Lua函数

void	defLuaConfig(lua_State* pL)
{
	if(pL == NULL)
	{
		LOG_CRITIC("defLuaConfig lua_state point is NULL ");
		return;
	}
	defLuaBaseConfig(pL);	
	
	lua_tinker::class_add<GameServerConfig>(pL,"GameServerConfig");
	lua_tinker::class_def<GameServerConfig>(pL,"GetRedisConf" 	,			&GameServerConfig::GetRedisConf		);
	lua_tinker::class_def<GameServerConfig>(pL,"GetDBConf" 		,			&GameServerConfig::GetDBConf		);

	LOG_DEBUG("导出Lua函数");

}




































