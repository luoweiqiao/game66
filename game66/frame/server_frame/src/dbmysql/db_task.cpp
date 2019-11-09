
#include "dbmysql/db_task.h"
#include "framework/logger.h"
#include "utility/stringutility.h"

using namespace std;
using namespace svrlib;

#ifndef END_DB_FLAG
#define END_DB_FLAG 100
#endif

CDBTask::CDBTask()
{
	m_bRun = false;
}
CDBTask::~CDBTask()
{
	dbClose();
}
bool CDBTask::init(const stDBConf& conf)
{
	m_Counts = 0;
	m_lastCountTime = time(NULL);
	m_conf = conf;

	return ConnectDB();
}
void CDBTask::stop()
{
	m_bRun = false;
}
void CDBTask::run()
{  	
	m_bRun = true;
	while( m_bRun || !m_QueueReq.empty())
	{
		if(!m_QueueReq.empty())
		{
			m_lockReq.Lock();
			CDBEventReq* pReq = m_QueueReq.front();  
			m_lockReq.Unlock();
            // 关闭后查询类sql不再处理
            if(m_bRun || pReq->needCb != emDBCALLBACK_QUERY)
			{
                if(OnProcessEvent(pReq)== false)
    			{
    				if(!m_clDatabase.isOpen())
    				{                		
    					ConnectDB();					
    					continue;
    				}else{
                        //LOG_ERROR("sql事件执行失败:%s",pReq->sqlStr.c_str());
                        writeLog(pReq->sqlStr.c_str());
                    }
    			}
            }
            m_lockReq.Lock();
			m_QueueReq.pop();
			m_lockReq.Unlock();
            FreeDBEventReq(pReq);
       
			m_Counts++;
			if((time(NULL)-m_lastCountTime) > 300 )
			{
				//LOG_DEBUG("thread %d presss min:%d count:%d",m_dbIndex, m_Counts/5);
				m_Counts = 0;
				m_lastCountTime = time(NULL);
			}            
		}else{
			sleep(5);
		}
	}
	//LOG_DEBUG("CAsyncTask run over:%d",m_dbIndex);
}
void CDBTask::writeLog(string logStr)
{
    
}
void CDBTask::push(string sql)
{
    CDBEventReq* pReq = MallocDBEventReq(0,0);
    pReq->sqlStr = sql;
    AddDBReqEvent(pReq);
}
void	CDBTask::pushCmd(const char*pCmd,...)
{
	if(!pCmd) return;

	memset(m_szCommand,0,sizeof(m_szCommand));
	va_list	argptr;
	va_start(argptr,pCmd);
	dVsprintf(m_szCommand,sizeof(m_szCommand),pCmd,argptr);
	va_end(argptr);	

    push(m_szCommand);	
}
// 异步插入数据
void    CDBTask::AsyncInsert(const char* tblName,SQLJoin& data)
{
    push(GetInsertSql(tblName,data));
}
// 异步更新数据
void    CDBTask::AsyncUpdate(const char* tblName,SQLJoin& data,SQLJoin& where)
{
    push(GetUpdateSql(tblName,data,where));
}
// 异步插入或者更新
void    CDBTask::AsyncUpdateOrInsert(const char* tblName,SQLJoin& data)
{
    push(GetUpdateOrInsertSql(tblName,data));
}
// 异步删除数据
void    CDBTask::AsyncDelete(const char* tblName,SQLJoin& where)
{
    push(GetDeleteSql(tblName,where));
}

void	CDBTask::setDBIndex(int32 index)
{ 
	m_dbIndex = index;
}
bool	CDBTask::ConnectDB()
{
    //LOG_DEBUG("DBTask connect mysql:%s,%s,%s,%s,%d",m_conf.sHost.c_str(),m_conf.sUser.c_str(),m_conf.sPwd.c_str(),m_conf.sDBName.c_str(),m_conf.uPort);
	return dbOpen(m_conf.sHost.c_str(),m_conf.sUser.c_str(),m_conf.sPwd.c_str(),m_conf.sDBName.c_str(),m_conf.uPort);
}
// 异步处理
void    CDBTask::AsyncQuery(CDBEventReq *pReq)
{     
     AddDBReqEvent(pReq);
}
CDBEventRep* CDBTask::GetAsyncQueryResult()
{
    // 处理数据库操作返回事件
    while(!m_QueueRep.empty())
    {
        m_lockRep.Lock();    
        CDBEventRep* pRepEvent = m_QueueRep.front();
        m_QueueRep.pop();
        m_lockRep.Unlock();
        return pRepEvent;
    }     
    return NULL;
}
CDBEventReq* CDBTask::MallocDBEventReq(uint16 eventID,uint8 needCb)
{
    CDBEventReq* pReq = new CDBEventReq();    
    pReq->eventID = eventID;
    pReq->needCb  = needCb;        
    return pReq;
}
void    CDBTask::FreeDBEventReq(CDBEventReq * pReq)
{
    delete pReq;
    pReq = NULL;
}
CDBEventRep* CDBTask::MallocDBEventRep()
{
    CDBEventRep* pRep = new CDBEventRep();   
    return pRep;    
}
void    CDBTask::FreeDBEventRep(CDBEventRep* pRep)
{
    delete pRep;
    pRep = NULL;
}
// 添加请求数据库操作
void    CDBTask::AddDBReqEvent(CDBEventReq* pReqEvent)    
{
    if(!m_bRun)
    {
        FreeDBEventReq(pReqEvent);
        return;
    }
    m_lockReq.Lock();
    m_QueueReq.push(pReqEvent);
    m_lockReq.Unlock();        
}
// 添加数据库返回事件
void    CDBTask::AddDBRepEvent(CDBEventRep* pRepEvent) 
{
    if(!m_bRun)
    {
        FreeDBEventRep(pRepEvent);
        return;
    }    
    m_lockRep.Lock();
    m_QueueRep.push(pRepEvent);    
    m_lockRep.Unlock();            
}
// 处理请求事件
bool    CDBTask::OnProcessEvent(CDBEventReq* pReqEvent)
{
    if(pReqEvent->needCb == emDBCALLBACK_NULL)
    {
        return ExeSql(pReqEvent->sqlStr);
    }
    
    CDBEventRep* pRep = MallocDBEventRep();
    pRep->eventID = pReqEvent->eventID;
    memcpy(pRep->params,pReqEvent->params,sizeof(pRep->params));

    if(pReqEvent->needCb == emDBCALLBACK_QUERY)
    {
        if(this->Query(pReqEvent->sqlStr.c_str(),pRep->vecData) == 0)
        {
            AddDBRepEvent(pRep);
            return true;
        }
    }
    if(pReqEvent->needCb == emDBCALLBACK_AFFECT)
    {
        uint32 affectNum = this->GetAffectedNumExeSql(pReqEvent->sqlStr.c_str());
        m_tmpMapData.clear();
        MYSQLValue &value = m_tmpMapData["affectnum"];
        string affectStr = CStringUtility::FormatToString("%d",affectNum);
        value.SetData(affectStr.c_str(),affectStr.length());          
        pRep->vecData.push_back(m_tmpMapData);
        
        AddDBRepEvent(pRep);
        return true;                      
    }

    FreeDBEventRep(pRep);
    return false;
}



