

#include "lobby_server_config.h"

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
	
	lua_tinker::class_add<LobbyServerConfig>(pL,"LobbyServerConfig");
	lua_tinker::class_def<LobbyServerConfig>(pL,"SetNeedPassWD" 	,		&LobbyServerConfig::SetNeedPassWD	);
	lua_tinker::class_def<LobbyServerConfig>(pL,"GetRedisConf" 	,			&LobbyServerConfig::GetRedisConf	);
	lua_tinker::class_def<LobbyServerConfig>(pL,"GetDBConf" 		,		&LobbyServerConfig::GetDBConf		); 	

	LOG_DEBUG("导出Lua函数");

}




































