
#include "stdafx.h"
#include "game_net_mgr.h"
#include "pb/msg_define.pb.h"
#include "lobby_server_config.h"
#include "msg_client_handle.h"
#include "msg_php_handle.h"
#include "helper/helper.h"

using namespace svrlib;
using namespace std;
using namespace Network;

bool CGameNetMgr::Init()
{
    SetMsgSinker(new CHandleClientMsg());
    CCReactor::Instance().Init();
    //! 初始化各个端口
    const stServerCfg& refCfg = CDataCfgMgr::Instance().GetCurSvrCfg();
    
    ClientSocketServer * clientServer = new ClientSocketServer("0.0.0.0", refCfg.svrport);
    int ret = CCReactor::Instance().RegistServer(clientServer);
    if (ret != 0)
    {
        LOG_ERROR("Regist client Server Failed.");
        return false;
    }

    SocketServer * serverServer = new SocketServer("0.0.0.0", refCfg.svrlanport);
    ret = CCReactor::Instance().RegistServer(serverServer);
    if (ret != 0)
    {
        //LOG_ERROR("the seqnum is error:%d--%d", m_seqNum, head->uin);
        return true;// 跳过校验
    }

    PhpServer * phpServer = new PhpServer("0.0.0.0", refCfg.phpport);
    ret = CCReactor::Instance().RegistServer(phpServer);
    if (ret != 0)
    {
        LOG_ERROR("Regist Php Server Failed.");
        return false;
    }

    ClientHandle::m_pConnMng = new CObjSeg(sizeof(ClientHandle), CLINET_MAX_CONN_NUM, ClientHandle::CreateObject);
    ClientHandle::m_stConnHash.Initialize(ClientHandle::m_pConnMng, 0);

    ServerHandle::m_pConnMng = new CObjSeg(sizeof(ServerHandle), SERVER_MAX_CONN_NUM, ServerHandle::CreateObject);
    ServerHandle::m_stConnHash.Initialize(ServerHandle::m_pConnMng, 0);

    PhpHandle::m_pConnMng = new CObjSeg(sizeof(PhpHandle), PHP_MAX_CONN_NUM, PhpHandle::CreateObject);
    PhpHandle::m_stConnHash.Initialize(PhpHandle::m_pConnMng, 0);

    return true;
}

//----------------------------------------------------

int ClientHandle::s_flow = 0;

IMPLEMENT_DYN(ClientHandle)

CHashTab<CLINET_MAX_CONN_NUM> ClientHandle::m_stConnHash;
CObjSeg*	ClientHandle::m_pConnMng;


int ClientHandle::Init()
{
    CCSocketHandler::Init();
    CCReactor::Instance().StartTimer(this, 20 * 1000);
    //m_flow = 0;
    m_cnt = 0;
    m_tm = getTickCount64();

    return 0;
}

int ClientHandle::OnPacketComplete(char * data, int len)
{
    //! 校验
    ++m_cnt;
    uint64 now_tm = getTickCount64();
    if (now_tm - m_tm > 1000) //! 1秒检查一次
    {
        if (m_cnt >= 100)
        {
            string ip = GetSIP();
            LOG_ERROR("uid:%d msg cnt:%d ip:%s error", GetUID(), m_cnt, ip.c_str());
            return -1;
        }  
        m_cnt = 0;
        m_tm = now_tm;
    }    

    CGameNetMgr::Instance().OnHandleClientMsg(this, (uint8_t *)data, len);

	//LOG_DEBUG("client handle recv pack len:%d", len);
	//int fd = 0;
	//if (this != NULL)
	//{
	//	fd = GetNetfd();
	//}
	//LOG_ERROR("client handle recv pack - this:%p,uid:%d,fd:%d,len:%d", this, GetUID(), fd, len);


    CCReactor::Instance().StopTimer(this);
    CCReactor::Instance().StartTimer(this, 20 * 1000);
    return 0;
}

void ClientHandle::OnTimer()
{
    LOG_ERROR("client handle heart timeout this:%p uid:%d,fd:%d", this, GetUID(), GetNetfd());
    //! 心跳超时
    DestroyObj();
}

int ClientHandle::DestroyObj()
{
	int fd = 0;
	if (this != NULL)
	{
		fd = GetNetfd();
	}
    CCReactor::Instance().StopTimer(this);
    DetachPoller();
    DeleteNode(m_flow);

    uint32 uid = GetUID();
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	LOG_DEBUG("client handle destory - this:%p,uid:%d,pPlayer:%p,fd:%d", this, GetUID(), pPlayer, fd);

    if (pPlayer != NULL)
    {
		int player_fd = 0;
		if (pPlayer->GetSession() != NULL)
		{
			player_fd = pPlayer->GetSession()->GetNetfd();
		}
		if (pPlayer->GetSession() != this)
		{
			LOG_DEBUG("client_handle_destory_error - uid:%d,this:%p,Session:%p,fd:%d,player_fd:%d", GetUID(),this, pPlayer->GetSession(), fd, player_fd);
			SetUID(0);
			return 0;
		}
        // 不直接断线,保留一定时间
        pPlayer->SetSession(NULL);
        SetUID(0);
        pPlayer->NotifyNetState2GameSvr(0);
		pPlayer->NotifyNetState2SecondGameSvr(0);
    }
	SetUID(0);
    return 0;
}

ClientHandle* ClientHandle::GetDRNodeByKey(unsigned int Key, int isCreate /*= True*/)
{
    ClientHandle *pConn = (ClientHandle *)ClientHandle::m_stConnHash.GetObjectByKey((const void *)&Key, sizeof(Key));
    if (pConn == NULL)
    {
        if (isCreate)
        {
            //如果找不到,则创建一个新节点
            pConn = CreateDRNode(Key);
            if (pConn)
            {
                //LOG_DEBUG("create node--key[%u] ObjId[%d]", Key, pConn->GetObjectID());
            }
            else
            {
                LOG_ERROR("create node fail--key[%u]", Key);
                return NULL;
            }
        }
        else
        {
            //LOG_ERROR("The Key %u is not exists, and not need to create", Key);
            return NULL;
        }        
    }
    return pConn;
}

ClientHandle* ClientHandle::CreateDRNode(unsigned int Key)
{    
    if (ClientHandle::m_pConnMng->GetFreeCount() == 0)
    {
        LOG_ERROR("create fail,not free node--used_cnt[%d],free_cnt[%d]",
            ClientHandle::m_pConnMng->GetUsedCount(), ClientHandle::m_pConnMng->GetFreeCount());
        return NULL;
    }

    LOG_DEBUG("create node used_cnt[%d],free_cnt[%d]", ClientHandle::m_pConnMng->GetUsedCount(), ClientHandle::m_pConnMng->GetFreeCount());

    ClientHandle* pConn = NULL;
    int iErrNo = 0;
    pConn = (ClientHandle *)ClientHandle::m_stConnHash.CreateObjectByKey((const void *)&Key, sizeof(Key), iErrNo);

    //初始化链接信息
    if (pConn)
    {
        pConn->Init();
        pConn->m_flow = Key;
    }

    return pConn;
}

int ClientHandle::DeleteNode(unsigned int Key)
{
    ClientHandle* pConnNode = NULL;
    int iDelIdx = -1;
    int iTempKeyLength = 0;
    BYTE abyTempKey[8];
    memset(abyTempKey, 0, sizeof(abyTempKey));

    pConnNode = GetDRNodeByKey(Key, False);
    //删除hash和引用信息
	uint32 uid = 0;
	int fd = 0;
    if (pConnNode)
    {
		uid = pConnNode->GetUID();
		fd = pConnNode->GetNetfd();
        pConnNode->InitSock();
        //回收用户缓存
        iDelIdx = pConnNode->GetHashKey((void *)&abyTempKey[0], iTempKeyLength);
        if (iDelIdx >= 0)
        {
            iDelIdx = ClientHandle::m_stConnHash.DeleteObjectByKey((const void *)&abyTempKey[0], iTempKeyLength);

            if (iDelIdx < 0)
            {
                return -1;
            }
            ClientHandle::m_pConnMng->DestroyObject(iDelIdx);
        }
    }

	LOG_DEBUG("delete node - pConnNode:%p,uid:%d,fd:%d,used_cnt[%d] free_cnt[%d]", pConnNode, uid, fd, ClientHandle::m_pConnMng->GetUsedCount(), ClientHandle::m_pConnMng->GetFreeCount());

    return 0;
}

int ClientHandle::GetFlow()
{
    ++s_flow;
    s_flow = s_flow % 0xFFFFFF;
    return s_flow;
}

int ClientHandle::GetHashKey(void *pvKey, int& iKeyLength)
{
    if (!pvKey)
    {
        iKeyLength = 0;
        return -1;
    }

    iKeyLength = sizeof(m_flow);
    memcpy(pvKey, (const void *)&m_flow, sizeof(m_flow));

    return 0;
}

int ClientHandle::SetHashKey(const void *pvKey, int iKeyLength)
{
    if((unsigned int ) iKeyLength < sizeof(m_flow) || !pvKey )
	{
		return -1;
	}

	memcpy((void *)&m_flow, pvKey, sizeof(m_flow));
	return 0;
}



// ------------------------------------------------------------------
int ServerHandle::s_flow = 0;

IMPLEMENT_DYN(ServerHandle)

CHashTab<SERVER_MAX_CONN_NUM> ServerHandle::m_stConnHash;
CObjSeg*	ServerHandle::m_pConnMng;

int ServerHandle::Init()
{
    CCSocketHandler::Init();
    CCReactor::Instance().StartTimer(this, 300 * 1000);

    return 0;
}

int ServerHandle::OnPacketComplete(char * data, int len)
{
    CServerMgr::Instance().OnHandleClientMsg(this, (uint8_t *)data, len);
    CCReactor::Instance().StopTimer(this);
    CCReactor::Instance().StartTimer(this, 300 * 1000);
    //LOG_DEBUG("server msg len:%d", len);
    return 0;
}

void ServerHandle::OnTimer()
{
    LOG_ERROR("server heart timeout flow:%d", m_flow);
    //! 心跳超时
    DestroyObj();
}

int ServerHandle::DestroyObj()
{
    CCReactor::Instance().StopTimer(this);
    CServerMgr::Instance().RemoveServer(this);
    DetachPoller();
    DeleteNode(m_flow);
    return 0;
}

ServerHandle* ServerHandle::GetDRNodeByKey(unsigned int Key, int isCreate /*= True*/)
{
    ServerHandle *pConn = (ServerHandle *)ServerHandle::m_stConnHash.GetObjectByKey((const void *)&Key, sizeof(Key));
    if (pConn == NULL)
    {
        if (isCreate)
        {
            //如果找不到，则创建一个新节点
            pConn = CreateDRNode(Key);
            if (pConn)
            {
                LOG_DEBUG("create node--key[%u] ObjId[%d]", Key, pConn->GetObjectID());
            }
            else
            {
                LOG_ERROR("create node fail--key[%u]", Key);
                return NULL;
            }
        }
        else
        {
            //LOG_ERROR("The Key %u is not exists, and not need to create", Key);
            return NULL;
        }
    }

    return pConn;
}

ServerHandle* ServerHandle::CreateDRNode(unsigned int Key)
{
    if (ServerHandle::m_pConnMng->GetFreeCount() == 0)
    {
        LOG_ERROR("create fail,not free node--used_cnt[%d],free_cnt[%d]",
            ServerHandle::m_pConnMng->GetUsedCount(), ServerHandle::m_pConnMng->GetFreeCount());
        return NULL;
    }

    ServerHandle* pConn = NULL;
    int iErrNo = 0;
    pConn = (ServerHandle *)ServerHandle::m_stConnHash.CreateObjectByKey((const void *)&Key, sizeof(Key), iErrNo);

    //初始化链接信息
    if (pConn)
    {
        pConn->Init();
        pConn->m_flow = Key;
    }

    return pConn;
}

int ServerHandle::DeleteNode(unsigned int Key)
{
    ServerHandle* pConnNode = NULL;
    int iDelIdx = -1;
    int iTempKeyLength = 0;
    BYTE abyTempKey[8];
    memset(abyTempKey, 0, sizeof(abyTempKey));

    pConnNode = GetDRNodeByKey(Key, False);
    //删除hash和引用信息
    if (pConnNode)
    {
        pConnNode->InitSock();
        //回收用户缓存
        iDelIdx = pConnNode->GetHashKey((void *)&abyTempKey[0], iTempKeyLength);
        if (iDelIdx >= 0)
        {
            iDelIdx = ServerHandle::m_stConnHash.DeleteObjectByKey((const void *)&abyTempKey[0], iTempKeyLength);

            if (iDelIdx < 0)
            {
                return -1;
            }
            ServerHandle::m_pConnMng->DestroyObject(iDelIdx);
        }
    }

    return 0;
}

int ServerHandle::GetFlow()
{
    ++s_flow;
    s_flow = s_flow % 0xFFFFFF;
    return s_flow;
}

int ServerHandle::GetHashKey(void *pvKey, int& iKeyLength)
{
    if (!pvKey)
    {
        iKeyLength = 0;
        return -1;
    }

    iKeyLength = sizeof(m_flow);
    memcpy(pvKey, (const void *)&m_flow, sizeof(m_flow));

    return 0;
}

int ServerHandle::SetHashKey(const void *pvKey, int iKeyLength)
{
    if ((unsigned int)iKeyLength < sizeof(m_flow) || !pvKey)
    {
        return -1;
    }

    memcpy((void *)&m_flow, pvKey, sizeof(m_flow));
    return 0;
}



// ------------------------------------------------------------------
int PhpHandle::s_flow = 0;

IMPLEMENT_DYN(PhpHandle)

CHashTab<PHP_MAX_CONN_NUM> PhpHandle::m_stConnHash;
CObjSeg*	PhpHandle::m_pConnMng;

int PhpHandle::Init()
{
    CCSocketHandler::Init();
    CCReactor::Instance().StartTimer(this, 300 * 1000);

    return 0;
}

int PhpHandle::OnPacketComplete(char * data, int len)
{
    CHandlePHPMsg::Instance().OnRecvClientMsg(this, (uint8_t *)data, len);
    CCReactor::Instance().StopTimer(this);
    CCReactor::Instance().StartTimer(this, 300 * 1000);
    return 0;
}

void PhpHandle::OnTimer()
{
    LOG_ERROR("php heart timeout flow:%d", m_flow);
    //! 心跳超时
    DestroyObj();
}

int PhpHandle::DestroyObj()
{
    CCReactor::Instance().StopTimer(this);
    DetachPoller();
    DeleteNode(m_flow);
    return 0;
}

PhpHandle* PhpHandle::GetDRNodeByKey(unsigned int Key, int isCreate /*= True*/)
{
    PhpHandle *pConn = (PhpHandle *)PhpHandle::m_stConnHash.GetObjectByKey((const void *)&Key, sizeof(Key));
    if (pConn == NULL)
    {
        if (isCreate)
        {
            //如果找不到，则创建一个新节点
            pConn = CreateDRNode(Key);
            if (pConn)
            {
                LOG_DEBUG("create node--key[%u] ObjId[%d]", Key, pConn->GetObjectID());
            }
            else
            {
                LOG_ERROR("create node fail--key[%u]", Key);
                return NULL;
            }
        }
        else
        {
            //LOG_ERROR("The Key %u is not exists, and not need to create", Key);
            return NULL;
        }
    }

    return pConn;
}

PhpHandle* PhpHandle::CreateDRNode(unsigned int Key)
{
    if (PhpHandle::m_pConnMng->GetFreeCount() == 0)
    {
        LOG_ERROR("create fail,not free node--used_cnt[%d],free_cnt[%d]",
            PhpHandle::m_pConnMng->GetUsedCount(), PhpHandle::m_pConnMng->GetFreeCount());
        return NULL;
    }

    PhpHandle* pConn = NULL;
    int iErrNo = 0;
    pConn = (PhpHandle *)PhpHandle::m_stConnHash.CreateObjectByKey((const void *)&Key, sizeof(Key), iErrNo);

    //初始化链接信息
    if (pConn)
    {
        pConn->Init();
        pConn->m_flow = Key;
    }

    return pConn;
}

int PhpHandle::DeleteNode(unsigned int Key)
{
    PhpHandle* pConnNode = NULL;
    int iDelIdx = -1;
    int iTempKeyLength = 0;
    BYTE abyTempKey[8];
    memset(abyTempKey, 0, sizeof(abyTempKey));

    pConnNode = GetDRNodeByKey(Key, False);
    //删除hash和引用信息
    if (pConnNode)
    {
        pConnNode->InitSock();
        //回收用户缓存
        iDelIdx = pConnNode->GetHashKey((void *)&abyTempKey[0], iTempKeyLength);
        if (iDelIdx >= 0)
        {
            iDelIdx = PhpHandle::m_stConnHash.DeleteObjectByKey((const void *)&abyTempKey[0], iTempKeyLength);

            if (iDelIdx < 0)
            {
                return -1;
            }
            PhpHandle::m_pConnMng->DestroyObject(iDelIdx);
        }
    }

    return 0;
}

int PhpHandle::GetFlow()
{
    ++s_flow;
    s_flow = s_flow % 0xFFFFFF;
    return s_flow;
}

int PhpHandle::GetHashKey(void *pvKey, int& iKeyLength)
{
    if (!pvKey)
    {
        iKeyLength = 0;
        return -1;
    }

    iKeyLength = sizeof(m_flow);
    memcpy(pvKey, (const void *)&m_flow, sizeof(m_flow));

    return 0;
}

int PhpHandle::SetHashKey(const void *pvKey, int iKeyLength)
{
    if ((unsigned int)iKeyLength < sizeof(m_flow) || !pvKey)
    {
        return -1;
    }

    memcpy((void *)&m_flow, pvKey, sizeof(m_flow));
    return 0;
}



