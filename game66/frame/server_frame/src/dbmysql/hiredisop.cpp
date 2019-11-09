
#include "dbmysql/hiredisop.h"
#include "framework/logger.h"
#include <stdio.h>
#include "utility/comm_macro.h"
#include <sstream>


CHiredisOp::CHiredisOp()
{
	m_redis = NULL;
	m_reply = NULL;
}
CHiredisOp::~CHiredisOp()
{
	FreeReply();
	FreeRedis();
}
bool	CHiredisOp::SetRedisConf(const char * ip,uint32 port)
{
	m_redisIp   = ip;
	m_redisPort = port;

	
	return false;
}
bool	CHiredisOp::IsConnect()
{
	if(m_redis == NULL)
		return false;

	bool bRet = true;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"PING"));
	if(m_reply == NULL || m_reply->type != REDIS_REPLY_STATUS || strcasecmp(m_reply->str,"PONG") != 0)
	{
		bRet = false;
	}
	FreeReply();

	return bRet;
}
void	CHiredisOp::CheckReconnectRedis()
{
	if(!IsConnect())
	{
		ConnectRedis();	
		return;
	}
}
void	CHiredisOp::FreeReply()
{
	if(m_reply != NULL)
	{
		freeReplyObject(m_reply);
		m_reply = NULL;
	}	
}
void 	CHiredisOp::FreeRedis()
{
	if(m_redis != NULL){
		redisFree(m_redis);
		m_redis = NULL;
	}

}
void    CHiredisOp::SetKeepAlive( const char * key, int iTimeOut)
{
	if(m_redis==NULL)return;

	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"expire %s %d",key,iTimeOut));	
	FreeReply();
}
bool	CHiredisOp::ConnectRedis()
{
	FreeRedis();
	//struct timeval timeout = {1, 500000};
	//m_redis = redisConnectWithTimeout(m_redisIp.c_str(), m_redisPort, timeout);

	m_redis = redisConnect(m_redisIp.c_str(), m_redisPort);
	if(m_redis == NULL || m_redis->err)	//连接redis
	{
		LOG_ERROR( "Connect Redis Server failed, ip:%s, port:%d ",m_redisIp.c_str(), m_redisPort);
        FreeRedis();
		return false;
	}
	else
	{
		LOG_ERROR("Initredis-connect OK...");
		return true;
	}
    FreeRedis();
	return false;
}
int64     CHiredisOp::IncrRecord(const char* key, int nAdd, int expire)
{
	if(m_redis==NULL)return 0;

	char value[32] = {0};
	int64 rec = -3;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "EXISTS %s", key));
	if(m_reply != NULL)
	{
		rec = m_reply->integer;
		FreeReply();
	}
	if(rec == -3)
	{
		LOG_ERROR("error in incrRecord %s ",key);
		return -1;
	}
	if(rec == 1)		//存在,更新
	{
		m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "INCRBY %s %d", key,nAdd));
		if(m_reply != NULL)
		{
			LOG_DEBUG("get key incr returned: %s || %d", key, m_reply->integer);
			rec = m_reply->integer;
			FreeReply();
		}
		else
		{
			rec = -1;
		}
		return rec;
	}
	else										//不存在,创建该key并初始化为nAdd
	{
		reinterpret_cast<redisReply*>(redisCommand(m_redis, "SET %s %d", key, nAdd));
		if(expire != -1)
		{
			sprintf(value, "%d", expire);
			m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "EXPIRE %s %s", key, value));
			if(m_reply != NULL)
			{
				LOG_DEBUG("set key:%s, time = %d, reexpire = %d\n", key, expire, m_reply->integer);
				rec = m_reply->integer;
				FreeReply();
			}
			else
			{
				rec = -1;
			}
			return rec;
		}
	}
	return 0;
}
int 	CHiredisOp::IsExist(const char* key)
{
	if(m_redis==NULL)return 0;

	int rec = -3;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "EXISTS %s", key));
	if(m_reply != NULL)
	{
		rec = m_reply->integer;
		FreeReply();
	}

	return rec;	
}
int 	CHiredisOp::Delete(const char* key)
{
	if(m_redis==NULL)return 0;

	int rec = -3;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "DEL %s", key));
	if(m_reply != NULL)
	{
		rec = m_reply->integer;
		FreeReply();
	}

	return rec;	
}
int 	CHiredisOp::SelectDB(int index)
{
	if(m_redis==NULL)return 0;

	int rec = -3;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "SELECT %d", index));
	if(m_reply != NULL)
	{
		rec = m_reply->integer;
		FreeReply();
	}

	return rec;
}
int	 CHiredisOp::SetString(const char* key,const char * val,int expire)
{
	if(m_redis==NULL)return 0;

	int rec = -3;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "SET %s %s EX %d",key,val,expire));
	if(m_reply != NULL)
	{
		rec = m_reply->integer;
		FreeReply();
	}
	return rec;	
}
string	CHiredisOp::GetString(const char* key)
{
	string res;
	if(m_redis==NULL)return res;

	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"GET %s",key));
	if(NULL != m_reply && m_reply->str != NULL && m_reply->len > 0)
	{
		res = m_reply->str;
	}
	FreeReply();	
	return res;
}

uint32 CHiredisOp::GetHashBinData(const char* key, uint32 pid, char *pdata, uint32 len)
{
	if(m_redis==NULL)return 0;

	CHECK_RET(NULL != key &&  pid > 0 && NULL != pdata && len > 0, 0);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HGET %s %d",key, pid));
	CHECK_RET(NULL != m_reply, 0);
	CHECK_RET((int)len >= m_reply->len, 0);
	memcpy(pdata, m_reply->str, m_reply->len);
	uint32 datalen = m_reply->len;
	FreeReply();	
	return datalen;
}

void CHiredisOp::SetHashBinData(const char* key, uint32 pid, const char *pdata, uint32 len)
{
	if(m_redis==NULL)return;

	CHECK_VOID(NULL != key && pid > 0 && NULL != pdata);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %d %b", key, pid, pdata, len));
	FreeReply();	
}

void CHiredisOp::DelHashBinData(const char* key, uint32 pid)
{
	if(m_redis==NULL)return;

	CHECK_VOID(NULL != key && pid > 0);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HDEL %s %d", key, pid));
	FreeReply();	
}
// 操作key 二进制数据
uint32  CHiredisOp::GetBinData(const char* key,uint64 pid,char* pdata,uint32 len)
{
	if(m_redis==NULL)return 0;

	CHECK_RET(NULL != key &&  pid > 0 && NULL != pdata && len > 0, 0);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"GET %s%llu",key, pid));
	CHECK_RET(NULL != m_reply, 0);
	CHECK_RET((int)len >= m_reply->len, 0);
	memcpy(pdata, m_reply->str, m_reply->len);

	uint32 datalen = m_reply->len;
	
	FreeReply();	
	return datalen;
}
void    CHiredisOp::SetBinData(const char* key,uint64 pid,const char* pdata,uint32 len,int expire)
{
	if(m_redis==NULL)return;

	CHECK_VOID(NULL != key && pid > 0 && NULL != pdata);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"SET %s%llu %b EX %d", key, pid, pdata, len,expire));
	FreeReply();   
}
void    CHiredisOp::DelBinData(const char* key,uint64 pid)
{
	if(m_redis==NULL)return;

	CHECK_VOID(NULL != key && pid > 0);
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"DEL %s%llu", key, pid));
	FreeReply();
}
bool CHiredisOp::GetMultiHashBinData(const char* key, uint32 pid[], uint32 pid_num, GetMultiHashBinDataCB func, void *func_param)
{
	if(m_redis==NULL)return false;

	CHECK_RET(NULL != key && pid_num > 0 && NULL != func, false);
	static std::stringstream ss;
	ss.clear();
	ss.str("");
	for (uint32 i = 0; i < pid_num; ++i){
		ss << " " << pid[i];
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HMGET %s %s",key, ss.str().c_str()));
	CHECK_RET(NULL != m_reply, false);
	bool ret = func(m_reply, func_param);
	FreeReply();	
	return ret;
}




