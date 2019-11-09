
#ifndef ASYNC_DBCALLBACK_H_
#define ASYNC_DBCALLBACK_H_

#include "dbmysql_mgr.h"

class CGameAsyncDBCallBack : public AsyncDBCallBack
{
public:
    virtual bool OnProcessDBEvent(CDBEventRep* pRep);

private:

    
    
};


#endif // ASYNC_DBCALLBACK_H_

