
#ifndef ASYNC_DBCALLBACK_H_
#define ASYNC_DBCALLBACK_H_

#include "dbmysql_mgr.h"

class CLobbyAsyncDBCallBack : public AsyncDBCallBack
{
public:
    virtual bool OnProcessDBEvent(CDBEventRep* pRep);


private:
    void    OnLoadPlayerDataEvent(CDBEventRep* pRep);
    void    OnLoadAccountDataEvent(CDBEventRep* pRep);
    void    OnLoadMissionDataEvent(CDBEventRep* pRep);
	//void    OnLoadPayDataEvent(CDBEventRep* pRep);

    void    OnLoadGameDataEvent(CDBEventRep* pRep);    
    void    OnLoadRobotLoginDataEvent(CDBEventRep* pRep);
	void    OnLoadRobotTimeLoginDataEvent(CDBEventRep* pRep);

    void    OnSendMailEvent(CDBEventRep* pRep);
    
};


#endif // ASYNC_DBCALLBACK_H_

