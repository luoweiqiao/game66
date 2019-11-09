
#include "config/config.h"
#include "framework/logger.h"

using namespace svrlib;
using namespace std;

// µ¼³öLuaº¯Êý

void	defLuaBaseConfig(lua_State* pL)
{
	if(pL == NULL)
	{
		LOG_CRITIC("defLuaBaseConfig lua_state point is NULL ");
		return;
	}
	lua_tinker::class_add<stDBConf>(pL,"stDBConf");
	lua_tinker::class_def<stDBConf>(pL,"SetDBInfo",			&stDBConf::SetDBInfo);

	lua_tinker::class_add<stListenConf>(pL,"stListenConf");
	lua_tinker::class_def<stListenConf>(pL,"SetListen",		&stListenConf::SetListen);
	lua_tinker::class_def<stListenConf>(pL,"SetBuffSize",	&stListenConf::SetBuffSize);

	lua_tinker::class_add<stRedisConf>(pL,"stRedisConf");
	lua_tinker::class_def<stRedisConf>(pL,"SetRedisHost",	&stRedisConf::SetRedisHost);


}
