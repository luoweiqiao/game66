
#ifndef SVR__CONFIG_H__
#define SVR__CONFIG_H__

#include <string>
#include "luatinker/luaTinker.h"
#include "utility/basicTypes.h"

using namespace std;

namespace svrlib
{
struct  stDBConf
{
	uint32		uPort ;
	std::string sHost ;
	std::string sDBName;
	std::string sUser ;
	std::string sPwd  ;
	stDBConf(){
		uPort	= 0;
		sHost	= "";
		sDBName = "";
		sUser	= "";
		sPwd	= "";
	}
	void operator=(const stDBConf& conf)
	{
		uPort 	= conf.uPort;
		sHost 	= conf.sHost;
		sDBName = conf.sDBName;
		sUser	= conf.sUser;
		sPwd	= conf.sPwd;		
	}
	void  SetDBInfo(const char* host,uint32 port,const char* dbName,const char* userName,const char* pwd){
		sHost	= host;
		uPort	= port;
		sDBName = dbName;
		sUser	= userName;
		sPwd	= pwd;
	}
};
struct	stListenConf
{
	uint32		listenPort;
	uint32		timeOut;
	uint32		recvSize;
	uint32		sendSize;
	uint32		maxPktSize;
	uint32		maxSession;
	stListenConf(){
		memset(this,0,sizeof(stListenConf));
	}
	void operator=(const stListenConf& conf)
	{
		listenPort = conf.listenPort;
		timeOut    = conf.timeOut;
		recvSize   = conf.recvSize;
		sendSize   = conf.sendSize;
		maxPktSize = conf.maxPktSize;
		maxSession = conf.maxSession;		
	}
	void	SetListen(uint32 port,uint32 Sessions,uint32 time)
	{
		listenPort = port;
		maxSession = Sessions;
		timeOut    = time;
	}
	void	SetBuffSize(uint32 recv,uint32 send,uint32 pktSize)
	{
		recvSize = recv;
		sendSize = send;
		maxPktSize = pktSize;
	}		
};
struct  stRedisConf
{
	std::string redisHost;
	uint32		redisPort;
	uint8		redisIndex;
	void operator=(const stRedisConf& ref)
	{
		redisHost = ref.redisHost;
		redisPort = ref.redisPort;
		redisIndex = ref.redisIndex;
	}
	void   SetRedisHost(const char* host,uint32 port,uint32 index)
	{
		redisHost  = host;
		redisPort  = port;
		redisIndex = index;
	}	
};
};
// µ¼³öLuaº¯Êý
extern  void	defLuaBaseConfig(lua_State* pL);



#endif //SVR__CONFIG_H__