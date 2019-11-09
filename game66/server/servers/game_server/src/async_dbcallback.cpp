
#include "async_dbcallback.h"
#include "svrlib.h"
#include "stdafx.h"
#include "robot_mgr.h"

using namespace svrlib;

bool CGameAsyncDBCallBack::OnProcessDBEvent(CDBEventRep* pRep)
{
    switch(pRep->eventID)
    {
    case emDBEVENT_LOAD_PLAYER_DATA:
        {

        }break;
    case emDBEVENT_LOAD_ACCOUNT_DATA:
        {

        }break;
    default:
        break;
    }
    return true;
}




