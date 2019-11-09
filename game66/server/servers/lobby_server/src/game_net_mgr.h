
#ifndef GAME_NET_MGR_H_
#define GAME_NET_MGR_H_

#include "Network.h"
#include "NetworkObject.h"
#include "network/protobuf_pkg.h"
#include "svrlib.h"
#include "network/CCSocketHandler.h"
#include "network/CCSocketServer.h"
#include "network/idxobj.h"
#include "network/CCReactor.h"
#include "hashtab.h"
#include "msg_client_handle.h"

class CGameNetMgr : public CProtobufMsgHanlde, public AutoDeleteSingleton<CGameNetMgr>
{
public:
	CGameNetMgr() {}
    ~CGameNetMgr() {}

    bool Init();

    void Update()
    {
        CCReactor::Instance().Update();
    }
};


#define CLINET_MAX_CONN_NUM   10000
#define SERVER_MAX_CONN_NUM   100
#define PHP_MAX_CONN_NUM   1000
//! 客户端连接
class ClientHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    ClientHandle() {}
    virtual ~ClientHandle()
	{
		//LOG_DEBUG("this:%p,uid:%d", this, GetUID());
	}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();

    void ResetHeart();
private:
    int m_flow;
    int m_cnt; //! 消息包计数器,用于检查间隔时间内的包数量是否异常
    uint64 m_tm;
public: 
    //! 数据管理器
    static ClientHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static ClientHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<CLINET_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg*	m_pConnMng;

    DECLARE_DYN
};

class ClientSocketServer : public CCSocketServer
{
public:
    ClientSocketServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~ClientSocketServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = ClientHandle::GetFlow();
        LOG_DEBUG("client netfd[%d] flow[%d]", netfd, flow);

        return ClientHandle::GetDRNodeByKey(flow);
    }
};

//! server连接
class ServerHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    ServerHandle() {}
    virtual ~ServerHandle() {}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();
private:
    int m_flow;
public:
    //! 数据管理器
    static ServerHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static ServerHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<SERVER_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg*	m_pConnMng;

    DECLARE_DYN
};

class SocketServer : public CCSocketServer
{
public:
    SocketServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~SocketServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = ServerHandle::GetFlow();
        LOG_DEBUG("server netfd[%d] flow[%d]", netfd, flow);

        return ServerHandle::GetDRNodeByKey(flow);
    }
};

//! php连接
class PhpHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    PhpHandle() {}
    virtual ~PhpHandle() {}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();

    virtual ICC_Decoder*  CreateDecoder() { _decode = new PhpDecoder();  return _decode; }
private:
    int m_flow;
public:
    //! 数据管理器
    static PhpHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static PhpHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<PHP_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg*	m_pConnMng;

    DECLARE_DYN
};

class PhpServer : public CCSocketServer
{
public:
    PhpServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~PhpServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = PhpHandle::GetFlow();
        LOG_DEBUG("php netfd[%d] flow[%d]", netfd, flow);

        return PhpHandle::GetDRNodeByKey(flow);
    }
};


#endif // GAME_NET_MGR_H_




