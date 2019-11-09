
#ifndef DB_TASK_H__
#define DB_TASK_H__

#include "utility/basicTypes.h"
#include "utility/basicFunctions.h"
#include "utility/stringFunctions.h"
#include "utility/thread.h"
#include <string>
#include <queue>
#include "dbmysql/dbmysql.h"
#include "dbmysql/db_wrap.h"
#include "config/config.h"

using namespace std;
using namespace svrlib;

#ifndef MAX_DB_ADD_PARAM
#define MAX_DB_ADD_PARAM 5
#endif

enum emDBCALLBACK
{
    emDBCALLBACK_NULL   = 0, //不回调
    emDBCALLBACK_QUERY  = 1, //查询结果集
    emDBCALLBACK_AFFECT = 2, //影响行数
    
};

class CDBEventReq
{  
public:
    CDBEventReq()
    {
        Reset();
    }
    ~CDBEventReq()
    {
        Reset();
    }
    CDBEventReq(const CDBEventReq& ev)  
    {  
        eventID = ev.eventID;
        needCb  = ev.needCb;
        sqlStr  = ev.sqlStr;
        memcpy(params,ev.params,sizeof(params));
    }  
    void operator=(const CDBEventReq& ev)
	{
        eventID = ev.eventID;
        needCb  = ev.needCb;
        sqlStr  = ev.sqlStr;
        memcpy(params,ev.params,sizeof(params));
	}
    void Reset()
    {
        eventID = 0;
        needCb  = 0;
        sqlStr  = "";
        memset(params,0,sizeof(params));
    }
	uint32 	eventID;	                // 事件ID
	uint8   needCb;                     // 是否需要回调
    int64   params[MAX_DB_ADD_PARAM];   // 附加参数
	string  sqlStr;                     // sql语句
};
class CDBEventRep
{   
public:
    CDBEventRep()
    {
        Reset();
    }
    ~CDBEventRep()
    {
        Reset();
    }
    CDBEventRep(const CDBEventRep& ev)  
    {  
        eventID = ev.eventID;
        vecData = ev.vecData;
        memcpy(params,ev.params,sizeof(params));
    }  
    void operator=(const CDBEventRep& ev)
	{
        eventID = ev.eventID;
        vecData = ev.vecData;
        memcpy(params,ev.params,sizeof(params));
	}    
    void Reset()
    {
        eventID = 0;
        vecData.clear();
        memset(params,0,sizeof(params));
    }
    uint32  eventID;    // 事件ID
    int64   params[MAX_DB_ADD_PARAM];   // 附加参数
    vector<map<string, MYSQLValue> > vecData;// 数据      
};

class  CDBTask : public Thread,public CDBWrap
{
public:
	CDBTask();
	virtual ~CDBTask();

	bool	init(const stDBConf& conf);
	virtual void stop();
	virtual void run();
    virtual void writeLog(string logStr);
    
	void	push(string sql); 
	void	pushCmd(const char*pCmd,...);
    // 异步插入数据
    void    AsyncInsert(const char* tblName,SQLJoin& data);
    // 异步更新数据
    void    AsyncUpdate(const char* tblName,SQLJoin& data,SQLJoin& where);
    // 异步插入或者更新
    void    AsyncUpdateOrInsert(const char* tblName,SQLJoin& data);
    // 异步删除数据
    void    AsyncDelete(const char* tblName,SQLJoin& where);

	void 	setDBIndex(int32 index);

    // 异步处理,外部申请内存
    void    AsyncQuery(CDBEventReq *pReq);
    // 外部释放内存
    CDBEventRep* GetAsyncQueryResult();

    
    CDBEventReq* MallocDBEventReq(uint16 eventID,uint8 needCb);
    void    FreeDBEventReq(CDBEventReq * pReq);

    CDBEventRep* MallocDBEventRep();
    void    FreeDBEventRep(CDBEventRep* pRep);
    
        
private:
    // 添加请求数据库操作
    void    AddDBReqEvent(CDBEventReq* pReqEvent);    
    // 添加数据库返回事件
    void    AddDBRepEvent(CDBEventRep* pRepEvent);   
    // 处理请求事件
    bool    OnProcessEvent(CDBEventReq* pReqEvent);
        
private:
	bool	ConnectDB();
    
private:
	bool			m_bRun;
	stDBConf		m_conf;

	TLock			        m_lockReq;// 请求队列
    TLock                   m_lockRep;// 返回队列                  
    
	queue<CDBEventReq*>	    m_QueueReq;//请求队列
	queue<CDBEventRep*>     m_QueueRep;//返回队列
    
	int32           m_lastCountTime;
	int32           m_Counts;
	int32           m_dbIndex;	

};

#endif //DB_TASK_H__


