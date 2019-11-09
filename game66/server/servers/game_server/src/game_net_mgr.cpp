
#include "stdafx.h"
#include "game_net_mgr.h"
#include "lobby_mgr.h"
#include "pb/msg_define.pb.h"
#include "game_server_config.h"
#include "data_cfg_mgr.h"

using namespace svrlib;
using namespace std;
using namespace Network;

bool CGameNetMgr::Init()
{
    CCReactor::Instance().Init();
	return true;
}

void CGameNetMgr::ShutDown()
{

}

void CGameNetMgr::Update()
{
    CCReactor::Instance().Update();
}

int CLobbyNetObj::Init()
{
    NetworkObject::EnableReconnect();
    CCTcpHandle::Init();

    return 0;
}

int CLobbyNetObj::OnClose()
{
    LOG_DEBUG("lobby onclose svrID:%d !!!!", GetUID());
    CCTcpHandle::OnClose();
    CPollerObject::InitObj();
    CCTcpHandle::Reset();
    NetworkObject::EnableReconnect();

    CLobbyMgr::Instance().OnCloseClient(this);

    return 0;
}

int CLobbyNetObj::OnPacketComplete(char * data, int len)
{
    //LOG_DEBUG("1 lobby recv pack len:%d", len);
    CLobbyMgr::Instance().OnHandleClientMsg(this, (uint8_t *)data, len);
	//LOG_DEBUG("2 lobby recv pack len:%d", len);

    return 0;
}
