
#ifndef GAME_NET_MGR_H_
#define GAME_NET_MGR_H_

#include "Network.h"
#include "NetworkObject.h"
#include "protobuf_pkg.h"
#include "svrlib.h"
#include "network/CCTcpHandler.h"
#include "network/idxobj.h"
#include "network/CCReactor.h"
#include "hashtab.h"

using namespace svrlib;

// lobby 连接
class  CLobbyNetObj : public CCTcpHandle
{
public:
    CLobbyNetObj() {}
    virtual ~CLobbyNetObj() {}

public:
    virtual int Init();

    virtual int OnClose();
    virtual int OnPacketComplete(char * data, int len);
};


class CGameNetMgr : public AutoDeleteSingleton<CGameNetMgr>
{
public:
    CGameNetMgr(){}
    ~CGameNetMgr() {}

	bool	Init();
	void	ShutDown();
	void	Update();
};

#endif // GAME_NET_MGR_H_
























































