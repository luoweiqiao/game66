
#ifndef HIREDIS_OP_H__
#define HIREDIS_OP_H__

#include "hiredis.h"
#include "svrlib.h"
#include <string>

using namespace svrlib;
using namespace std;

class CHiredisOp
{
public:
	typedef bool (*GetMultiHashBinDataCB)(const redisReply *reply, void *param);
	CHiredisOp();
	~CHiredisOp();
	
	bool	SetRedisConf(const char * ip,uint32 port);
	bool	IsConnect();
	void	CheckReconnectRedis();
	
	bool	ConnectRedis();
	void	FreeReply();
	void 	FreeRedis();

	// 设置一个key的过期的时间:秒
	void    SetKeepAlive( const char * key, int iTimeOut=30*60 );
	int64 	IncrRecord(const char* key, int nAdd, int expire = -1);
	int		IsExist(const char* key);
	int		Delete(const char* key);
	int		SelectDB(int index);

	int		SetString(const char* key,const char * val,int expire = -1);
	string	GetString(const char* key);
	

	//@brief	获得Hash二进制数据
	uint32  GetHashBinData(const char* key, uint32 pid, char *pdata, uint32 len);
	//@brief	保存Hash二进制数据
	void    SetHashBinData(const char* key, uint32 pid, const char *pdata, uint32 len);
	//@brief	删除Hash二进制数据
	void    DelHashBinData(const char* key, uint32 pid);

	// 操作key 二进制数据
	uint32  GetBinData(const char* key,uint64 pid,char* pdata,uint32 len);
	void	SetBinData(const char* key,uint64 pid,const char* pdata,uint32 len,int expire = -1);
	void	DelBinData(const char* key,uint64 pid);
	
	//@brief	通过Key与field批量获得Hash二进制数据,需要提供回调函数，在回调函数中处理数据
	//input		(键值，字段列表，字段数量，回调处理函数，转给回调函数的参数)
	//notice	不能用用于分布区的情况
	bool GetMultiHashBinData(const char* key, uint32 pid[], uint32 pid_num, GetMultiHashBinDataCB func, void *func_param = NULL);

protected:
	redisContext *          m_redis;				    //连接hiredis
    redisReply *            m_reply;					//hiredis回复
	string					m_redisIp;
	uint32					m_redisPort;	

};




#endif // HIREDIS_OP_H__





