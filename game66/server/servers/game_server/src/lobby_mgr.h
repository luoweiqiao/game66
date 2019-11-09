
#ifndef _LOBBY_MGR_H__
#define _LOBBY_MGR_H__

#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include <string.h>
#include "network/NetworkObject.h"
#include "network/protobuf_pkg.h"
#include "game_net_mgr.h"

using namespace std;
using namespace svrlib;
using namespace Network;

/**
 * 管理大厅服务器
 */
struct  stLobbyServer
{
	uint16 			svrID;
	CLobbyNetObj*   pNetObj;
	bool  			isRun;
	bool  			isReconnecting;
	stServerCfg 	lobbyCfg;
	stLobbyServer(){
		svrID 	= 0;
		pNetObj = 0;
		isRun   = false;
		isReconnecting = false;
	}
};
/*
 * 根据目前的设计，只支持开一个大厅服务器
 */

class CLobbyMgr : public ITimerSink,public CProtobufMsgHanlde,public AutoDeleteSingleton<CLobbyMgr>
{
public:
	virtual void  OnTimer(uint8 eventID);
	
	bool	Init(IProtobufClientMsgRecvSink* pMsgRecvSink);

	void	Register(uint16 svrid);
	bool 	SendMsg2Client(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin);

    void	OnCloseClient(CLobbyNetObj* pNetObj);

	void	SetRunFlag(bool bFlag);
	void	ReConnect();

	// 请求大厅修改数值
	void	NotifyLobbyChangeAccValue(uint32 uid,int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid="");
	void	UpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid = "");
	void	NotifyUpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid = "");

private:


private:
	//typedef stl_hash_map<uint32,stLobbyServer> MAP_LOBBY;
	//MAP_LOBBY	  m_lobbySvrs;// 大厅服务器
	stLobbyServer m_lobbySvr;  
    CTimer*       m_pTimer;
};







#endif // _LOBBY_MGR_H__





