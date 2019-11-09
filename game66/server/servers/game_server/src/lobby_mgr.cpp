#include <data_cfg_mgr.h>
#include "lobby_mgr.h"
#include "game_server_config.h"
#include "game_define.h"
#include "stdafx.h"

#include "msg_lobby_handle.h"

using namespace svrlib;
using namespace std;
using namespace Network;
using namespace net;

void  CLobbyMgr::OnTimer(uint8 eventID)
{
	ReConnect();
}
bool	CLobbyMgr::Init(IProtobufClientMsgRecvSink* pMsgRecvSink)
{
	const stServerCfg& refCfg = CDataCfgMgr::Instance().GetCurSvrCfg();

	vector<stServerCfg> vecLobbyCfg;
	CDataCfgMgr::Instance().GetLobbySvrsCfg(refCfg.group,vecLobbyCfg);
	if(vecLobbyCfg.empty()){
		LOG_ERROR("大厅服务器配置未找到:group:%d",refCfg.group);
		return false;
	}
	stServerCfg& lobbycfg = vecLobbyCfg[0];
	CLobbyNetObj* pNetObj = new CLobbyNetObj();
	pNetObj->SetUID(lobbycfg.svrid);
	pNetObj->SetSIP(lobbycfg.svrlanip);
	pNetObj->SetPort(lobbycfg.svrlanport);
	
	m_lobbySvr.svrID = lobbycfg.svrid;
	m_lobbySvr.isReconnecting = false;
	m_lobbySvr.isRun 	= false;
	m_lobbySvr.lobbyCfg  = lobbycfg;
	m_lobbySvr.pNetObj   = pNetObj;               	

	SetMsgSinker(pMsgRecvSink);

    m_pTimer = CApplication::Instance().MallocTimer(this,1);
    m_pTimer->StartTimer(3000,3000);
	ReConnect();
	return true;
}
void	CLobbyMgr::Register(uint16 svrid)
{
    net::msg_register_svr msg;
    msg.set_svrid(CApplication::Instance().GetServerID());
    msg.set_game_type(CDataCfgMgr::Instance().GetCurSvrCfg().gameType);
    msg.set_game_subtype(CDataCfgMgr::Instance().GetCurSvrCfg().gameSubType);
    msg.set_robot(CDataCfgMgr::Instance().GetCurSvrCfg().openRobot);

    SendMsg2Client(&msg,net::S2L_MSG_REGISTER,0);
	LOG_DEBUG("注册游戏服务器:svrid:%d,game_type:%d,robot:%d",msg.svrid(),msg.game_type(), msg.robot());
}
bool 	CLobbyMgr::SendMsg2Client(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	return SendProtobufMsg(m_lobbySvr.pNetObj,msg,msg_type,uin);
}

void	CLobbyMgr::OnCloseClient(CLobbyNetObj* pNetObj)
{	
	LOG_ERROR("lobby OnClose:%d",pNetObj->GetUID());
	if(pNetObj != m_lobbySvr.pNetObj){
		LOG_ERROR("错误的链接断开:%d--%d",pNetObj->GetUID(),m_lobbySvr.pNetObj->GetUID());
		return;
	}
	m_lobbySvr.isRun = false;
}
void	CLobbyMgr::SetRunFlag(bool bFlag)
{
	m_lobbySvr.isRun = bFlag;
}
void	CLobbyMgr::ReConnect()
{
	if (m_lobbySvr.isRun || m_lobbySvr.isReconnecting)
	{
		return;
	}
	
	if (m_lobbySvr.pNetObj->Connect() == 0)
	{
		Register(m_lobbySvr.svrID);
		m_lobbySvr.isRun = true;

		LOG_DEBUG("connect lobby success ip:%s port:%d fd:%d", m_lobbySvr.pNetObj->GetSIP().c_str(), m_lobbySvr.lobbyCfg.svrlanport, m_lobbySvr.pNetObj->GetNetfd());
	}
	else
	{
		LOG_ERROR("connect lobby failed ip:%s port:%d", m_lobbySvr.pNetObj->GetSIP().c_str(), m_lobbySvr.lobbyCfg.svrlanport);
	}
}
// 请求大厅修改数值
void	CLobbyMgr::NotifyLobbyChangeAccValue(uint32 uid,int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid)
{
	net::msg_notify_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg,S2L_MSG_NOTIFY_CHANGE_ACCOUNT_DATA,uid);
}

void	CLobbyMgr::UpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid)
{
	net::msg_update_lobby_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg, S2L_MSG_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, uid);
}

void	CLobbyMgr::NotifyUpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid)
{
	net::msg_update_lobby_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg, S2L_MSG_NOTIFY_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, uid);
}


