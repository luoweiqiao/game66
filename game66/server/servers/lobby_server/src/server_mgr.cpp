

#include "server_mgr.h"
#include "pb/msg_define.pb.h"
#include "msg_server_handle.h"
#include "player_base.h"
#include "player_mgr.h"
#include "player.h"

using namespace svrlib;
using namespace Network;

namespace
{

}
CServerMgr::CServerMgr()
{
	m_mpServers.clear();
	m_bTimeStopSvr   = 0;
	m_eTimeStopSvr   = 0;
	m_stopSvrContent = "";
	m_stopSvrs.clear();
	m_pTimer 	     = NULL;

}
CServerMgr::~CServerMgr()
{
}
void  CServerMgr::OnTimer(uint8 eventID)
{
	CheckRepairServer();
}
bool  CServerMgr::Init()
{
	SetMsgSinker(new CHandleServerMsg());
	m_pTimer = CApplication::Instance().MallocTimer(this,1);
	m_pTimer->StartTimer(5000,5000);
    return true;
}
void  CServerMgr::ShutDown()
{
	CApplication::Instance().FreeTimer(m_pTimer);
}
bool  CServerMgr::AddServer(NetworkObject* pNetObj,uint16 svrID,uint16 gameType,uint8 gameSubType,uint8 openRobot)
{	
	stGServer server;
	server.svrID	    = svrID;
	server.gameType     = gameType;
    server.gameSubType  = gameSubType;
    server.openRobot    = openRobot;
	server.pNetObj      = pNetObj;
	pNetObj->SetUID(svrID);

	SyncPlayerCurSvrID(svrID);
	
	//if (GetServerBySvrID(svrID) != NULL)
	//{
	//	return true;
	//}

	pair< stl_hash_map<uint32, stGServer>::iterator, bool > ret;

	ret = m_mpServers.insert(make_pair(svrID, server));
	bool bretvalue =  ret.second;
	LOG_DEBUG("addserver - ip:%s,port:%d,svrID:%d,gameType:%d,gameSubType:%d,openRobot:%d,ret:%d", pNetObj->GetSIP().c_str(), pNetObj->GetPort(), svrID, gameType, gameSubType, openRobot, bretvalue);
	SendSvrsInfoToAll();
	return bretvalue;
}

void   CServerMgr::UpdateOpenRobot(uint16 svrID, uint8 openRobot)
{
	LOG_DEBUG("1 open_robot - svrID:%d,openRobot:%d,map_openRobot:%d", svrID, openRobot,m_mpServers.size());

	MAP_SERVERS::iterator it = m_mpServers.find(svrID);
	if (it != m_mpServers.end())
	{
		//stGServer & server = it->second;
		//server.openRobot = openRobot;
		it->second.openRobot = openRobot;
		LOG_DEBUG("2 open_robot - svrID:%d,openRobot:%d,map_openRobot:%d", svrID, openRobot, it->second.openRobot);
	}
	
}

void  CServerMgr::RemoveServer(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.pNetObj == pNetObj)
		{
			LOG_DEBUG("removeserver - svrID:%d", server.svrID);

			m_mpServers.erase(it);
			pNetObj->SetUID(0);
			SendSvrsInfoToAll();
			break;
		}
	}
}
stGServer* CServerMgr::GetServerBySocket(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.pNetObj == pNetObj)
		{
			return &it->second;
		}
	}
	return NULL;
}
stGServer* CServerMgr::GetServerBySvrID(uint16 svrID)
{
	MAP_SERVERS::iterator it = m_mpServers.find(svrID);
	if(it != m_mpServers.end())
	{
		return &it->second;
	}
	return NULL;
}
void   CServerMgr::SendMsg2Server(uint16 svrID,const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	stGServer* pServer = GetServerBySvrID(svrID);
	if(pServer == NULL) {
		LOG_DEBUG("send msg server is not exists - serverid:%d",svrID);
		return;
	}
	SendProtobufMsg(pServer->pNetObj,msg,msg_type,uin);
}
void   CServerMgr::SendMsg2AllServer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		SendProtobufMsg(server.pNetObj,msg,msg_type,uin);
	}
}
void   CServerMgr::SendMsg2Server(uint16 svrID,const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin)
{
	stGServer* pServer = GetServerBySvrID(svrID);
	if(pServer == NULL) {
		LOG_DEBUG("send serverid is not exist - serverid:%d",svrID);
		return;
	}
	SendProtobufMsg(pServer->pNetObj,pkt_buf,buf_len,msg_type,uin);
}
void   CServerMgr::SendMsg2AllServer(const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		SendProtobufMsg(server.pNetObj,pkt_buf,buf_len,msg_type,uin);
	}
}
// 获取服务器列表
void   CServerMgr::SendSvrsInfo2Client(uint32 uid)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if(pPlayer == NULL)
		return;
	net::msg_svrs_info_rep info;
	info.set_cur_svrid(pPlayer->GetCurSvrID());

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		net::svr_info *pSvr = info.add_svrs();
		pSvr->set_svrid(server.svrID);
		pSvr->set_game_type(server.gameType);
        pSvr->set_game_subtype(server.gameSubType);
		pSvr->set_state(server.status);
		if (pPlayer->IsRobot() == false)
		{
			LOG_DEBUG("发送服务器列表:uid:%d,size:%02d,svrID:%03d,gametype:%02d", uid, info.svrs_size(), server.svrID, server.gameType);
		}
	}
	pPlayer->SendMsgToClient(&info,net::S2C_MSG_SVRS_INFO);
	if (pPlayer->IsRobot() == false)
	{
		LOG_DEBUG("发送服务器列表:%d--%d", uid, info.svrs_size());
		map<uint32, uint64>::iterator iter = m_mpPlayerLoginTime.find(uid);
		if (iter != m_mpPlayerLoginTime.end()) {
			int64 expendTime = getTickCount64() - iter->second;
			if (m_minLoginTime == 0)
				m_minLoginTime = expendTime;
			if (expendTime > m_maxLoginTime)
				m_maxLoginTime = expendTime;
			else if (expendTime < m_minLoginTime)
				m_minLoginTime = expendTime;
			m_loginAllTime += expendTime;
			++m_loginAllCount;
			m_mpPlayerLoginTime.erase(uid);
			LOG_DEBUG("uid:%d,登陆耗时:%lldms,总次数:%lld,平均延时:%lldms,最大延时:%lldms,最小延时:%lldms,m_mpPlayerLoginTime.size:%lld",
				uid, expendTime, m_loginAllCount, m_loginAllTime/m_loginAllCount, m_maxLoginTime, m_minLoginTime, m_mpPlayerLoginTime.size());
		}
	}
	
}
void   CServerMgr::SendSvrsPlayInfo2Client(uint32 uid)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if(pPlayer == NULL)
		return;
	net::msg_send_server_info info;	
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		net::server_info *pSvr = info.add_servers();
		pSvr->set_svrid(server.svrID);
		pSvr->set_player_num(server.playerNum);
        pSvr->set_robot_num(server.robotNum);
        pSvr->set_game_type(server.gameType);
		uint32 allCount = server.robotNum + server.playerNum;
		LOG_DEBUG("send_server_info - uid:%d,size:%02d,svrID:%03d,gametype:%02d,all:%d,robotNum:%d,playerNum:%d", uid, info.servers_size(), server.svrID, server.gameType, allCount, server.robotNum, server.playerNum);

	}
	pPlayer->SendMsgToClient(&info,net::S2C_MSG_SEND_SERVER_INFO);     
}
void   CServerMgr::SendSvrsInfoToAll()
{
	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
		SendSvrsInfo2Client(pPlayer->GetUID());
	}
	vecPlayers.clear();
}
// 发送维护公告
void   CServerMgr::SendSvrRepairContent(NetworkObject* pNetObj)
{
	if(pNetObj == NULL)
		return;
	if(IsNeedRepair(CApplication::Instance().GetServerID()))
	{	
    	net::msg_system_broadcast_rep broad;
    	broad.set_msg(m_stopSvrContent);
    	SendProtobufMsg(pNetObj,&broad,net::S2C_MSG_SYSTEM_BROADCAST,0);
	}
}
// 发送停服广播
void   CServerMgr::SendSvrRepairAll()
{
    net::msg_system_broadcast_rep broad;
	broad.set_msg(m_stopSvrContent);    
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR || IsNeedRepair(CApplication::Instance().GetServerID())){
        CPlayerMgr::Instance().SendMsgToAll(&broad,net::S2C_MSG_SYSTEM_BROADCAST); 
        return;
    }    
    vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
        uint16 curSvrID = pPlayer->GetCurSvrID();
        if(IsNeedRepair(curSvrID))
        {
            pPlayer->SendMsgToClient(&broad,net::S2C_MSG_SYSTEM_BROADCAST);
        }                       
	}
	vecPlayers.clear();        
}    
bool   CServerMgr::IsNeedRepair(uint16 svrID)
{
    for(uint32 i=0;i<m_stopSvrs.size();++i){
        if(m_stopSvrs[i] == svrID){
            return true;
        }        
    }      
    return false;
}

uint16 CServerMgr::GetGameTypeSvrID(uint32 gameType)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.gameType == gameType)
			return server.svrID;
	}
	return 0;
}
bool   CServerMgr::IsOpenRobot(uint32 gameType)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.gameType == gameType){
			return server.openRobot == 1;
		}
	}
	return false;
}
// 服务器重连检测玩家所在服务器状态
void   CServerMgr::SyncPlayerCurSvrID(uint16 svrID)
{
	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
		pPlayer->SyncCurSvrIDFromRedis(svrID);
	}
	vecPlayers.clear();
}
// 停服通知
void   CServerMgr::NotifyStopService(uint32 btime,uint32 etime,vector<uint16>& svrids,string content)
{
	m_bTimeStopSvr 	 = btime;
	m_eTimeStopSvr 	 = etime;
	m_stopSvrContent = content;
	m_stopSvrs       = svrids;
}
// 获得机器人服务器ID
uint16 CServerMgr::GetRobotSvrID(CLobbyRobot* pRobot)
{
    vector<stGServer*> vecSvrs;
    MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.openRobot == 1 && pRobot->CanEnterGameType(server.gameType))
		{
            vecSvrs.push_back(&server);
		}
	}
    if(vecSvrs.size() > 0){
        sort(vecSvrs.begin(),vecSvrs.end(),CompareServerRobotNum);
        return vecSvrs[0]->svrID;
    }    
	return 0;       
}    
// 服务器机器人数量排序
bool   CServerMgr::CompareServerRobotNum(stGServer* pSvr1,stGServer* pSvr2)
{
    return (pSvr1->playerNum+pSvr1->robotNum) < (pSvr2->playerNum+pSvr2->robotNum);//总人数少的优先分配机器人
}
void   CServerMgr::CheckRepairServer()
{
	if(m_bTimeStopSvr == 0)
		return;
	uint32 curTime = getSysTime(); 
	if(!m_stopSvrs.empty())
	{
		if(m_bTimeStopSvr < curTime)
		{
            bool isRepairLobby = false;
            for(uint32 i=0;i<m_stopSvrs.size();++i)
			{
                if(m_stopSvrs[i] == CApplication::Instance().GetServerID())
				{
                    isRepairLobby = true;
                }
            }

			LOG_DEBUG("维护时间到了，通知服务器开启维护状态 - isRepairLobby:%d", isRepairLobby);

            net::msg_notify_stop_service msg;
		    msg.set_btime(m_bTimeStopSvr);
		    msg.set_etime(m_eTimeStopSvr); 
            if(isRepairLobby)
            {
        		CApplication::Instance().SetStatus(emSERVER_STATE_REPAIR);
                MAP_SERVERS::iterator it = m_mpServers.begin();
            	for(;it != m_mpServers.end();++it)
            	{
            		stGServer& server = it->second;
            		server.status = emSERVER_STATE_REPAIR;
            	}
                SendMsg2AllServer(&msg,net::L2S_MSG_NOTIFY_STOP_SERVICE,0);
            }
			else
			{
    			for(uint32 i=0;i<m_stopSvrs.size();++i)
    			{
    				uint16 svrid = m_stopSvrs[i];
    				stGServer* pServer = GetServerBySvrID(svrid);
    				if(pServer != NULL)
					{
    					pServer->status = emSERVER_STATE_REPAIR;    					
    					SendMsg2Server(svrid,&msg,net::L2S_MSG_NOTIFY_STOP_SERVICE,0);
    				}
    			}
            }
			m_stopSvrs.clear();
            SendSvrsInfoToAll();
		}
		else
		{
			if(m_coolBroad.isTimeOut())
			{
                LOG_DEBUG("发送维护广播:%d",curTime);
                SendSvrRepairAll();
                
				uint32 diffTime = m_bTimeStopSvr - curTime;
				if(diffTime < SECONDS_IN_MIN*5)
				{
					m_coolBroad.beginCooling(SECONDS_IN_MIN*1000);
				}
				else
				{
					m_coolBroad.beginCooling((m_eTimeStopSvr - curTime - SECONDS_IN_MIN) * 1000);
				}
			}
		}
	}
	// 检测是否维护完毕
    if(curTime > m_bTimeStopSvr && curTime < m_eTimeStopSvr)
    {
    	bool isAllOver = true;
    	MAP_SERVERS::iterator it = m_mpServers.begin();
    	for(;it != m_mpServers.end();++it)
    	{
    		stGServer& server = it->second;
    		if(server.status == emSERVER_STATE_REPAIR)
    		{
    			isAllOver = false;
    			break;
    		}
    	}
    	if(isAllOver)
		{
            LOG_DEBUG("维护结束");
    		m_stopSvrContent = "";
    		m_bTimeStopSvr   = 0;
    		m_eTimeStopSvr   = 0;
    	}
    }
}

void	CServerMgr::ChangeRoomParam(uint32 gametype, uint32 roomid, string param) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_change_room_param msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_param(param.c_str());
	SendMsg2Server(svrid,&msg,net::L2S_MSG_CHANGE_ROOM_PARAM,0);
}

void	CServerMgr::ChangeContorlPlayer(int64 id,uint32 gametype,uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_contorl_player msg;
	msg.set_id(id);
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_uid(uid);
	msg.set_operatetype(operatetype);
	msg.set_gamecount(gamecount);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_CONTORL_PLAYER, 0);
}

void	CServerMgr::ChangeContorlMultiPlayer(int64 id, uint32 gametype, uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount, uint64 gametime,int64 totalscore) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_contorl_multi_player msg;
	msg.set_id(id);
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_uid(uid);
	msg.set_operatetype(operatetype);
	msg.set_gamecount(gamecount);
	msg.set_gametime(gametime);
	msg.set_totalscore(totalscore);
	
	SendMsg2Server(svrid, &msg, net::L2S_MSG_CONTORL_MULTI_PLAYER, 0);
}

void	CServerMgr::StopSnatchCoin(uint32 gametype, uint32 roomid, uint32 stop) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_stop_snatch_coin msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_stop(stop);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_STOP_SNATCH_COIN, 0);
}

void	CServerMgr::RobotSnatchCoin(uint32 gametype, uint32 roomid, uint32 snatchtype, uint32 robotcount, uint32 cardcount) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_robot_snatch_coin msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_snatchtype(snatchtype);
	msg.set_robotcount(robotcount);
	msg.set_cardcount(cardcount);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_ROBOT_SNATCH_COIN, 0);
}


void	CServerMgr::ContorlDiceGameCard(uint32 gametype, uint32 roomid, uint32 uDice[])
{
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_dice_control_req msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	net::dice_control_req* pdice_control_req = msg.mutable_dice();

	for (int i = 0; i < 3; i++)
	{
		pdice_control_req->add_table_cards(uDice[i]);
	}

	SendMsg2Server(svrid, &msg, net::L2S_MSG_DICE_CONTROL_REQ, 0);
}

void	CServerMgr::ConfigMajiangHandCard(uint32 gametype, uint32 roomid, string strHandCard)
{
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_majiang_config_hand_card msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_hand_card(strHandCard.c_str());
	
	SendMsg2Server(svrid, &msg, net::L2S_MSG_MAJIANG_CONFIG_HAND_CARD, 0);
}


void	CServerMgr::UpdateServerRoomRobot(int gametype, int roomid, int robot)
{
	uint16 svrid = GetGameTypeSvrID(gametype);

	LOG_DEBUG("gametype:%d,svrid:%d, roomid:%d, robot:%d", gametype, svrid, roomid, robot);

	net::msg_update_server_room_robot msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_robot(robot);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_UPDATE_SERVER_ROOM_ROBOT, 0);
}


void	CServerMgr::ReloadRobotOnlineCfg(int optype,int gametype,int roomid,int leveltype,int batchid, int logintype, int entertimer, int leavetimer, vector<int> & vecOnline)
{

	LOG_DEBUG("optype:%d,gametype:%d,roomid:%d,leveltype:%d, batchid:%d, logintype:%d, entertimer:%d, leavetimer:%d,vecOnline.size():%d",
		optype,gametype, roomid,leveltype, batchid, logintype, entertimer, leavetimer, vecOnline.size());

	net::msg_reload_robot_online_cfg msg;
	msg.set_optype(optype);
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_leveltype(leveltype);
	msg.set_batchid(batchid);
	msg.set_logintype(logintype);
	msg.set_entertimer(entertimer);
	msg.set_leavetimer(leavetimer);
	for (uint32 i = 0; i < vecOnline.size(); i++)
	{
		msg.add_online(vecOnline[i]);
	}

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, &msg, net::L2S_MSG_RELOAD_ROBOT_ONLINE_CFG, 0);

		LOG_DEBUG("gametype:%d,svrid:%d", gametype, server.svrID);
	}
}


void	CServerMgr::UpdateNewPlayerNoviceWelfareRight(uint32 uid, uint32 userright, uint32 posrmb, uint64 postime)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	LOG_DEBUG("update_user_right - uid:%d,userright:%d,posrmb:%d,postime:%lld,pPlayer:%p", uid, userright, posrmb,postime, pPlayer);
	if (pPlayer != NULL)
	{
		uint16 svrid = pPlayer->GetCurSvrID();

		net::msg_update_new_player_novice_welfare_right msg;
		msg.set_uid(uid);
		msg.set_userright(userright);
		msg.set_posrmb(posrmb);
		msg.set_postime(postime);
		SendMsg2Server(svrid, &msg, net::L2S_MSG_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT, 0);
	}
}

void	CServerMgr::UpdateNewPlayerNoviceWelfareValue(tagNewPlayerWelfareValue & data)
{
	net::msg_update_new_player_novice_welfare_value msg;
	msg.set_id(data.id);
	msg.set_minpayrmb(data.minpayrmb);
	msg.set_maxpayrmb(data.maxpayrmb);
	msg.set_maxwinscore(data.maxwinscore);
	msg.set_welfarecount(data.welfarecount);
	msg.set_welfarepro(data.welfarepro);
	msg.set_postime(data.postime);
	msg.set_lift_odds(data.lift_odds);
	
	LOG_DEBUG("update_welfare_value - id:%d,minpayrmb:%d,maxpayrmb:%d,maxwinscore:%d,welfarecount:%d,welfarepro:%d,postime:%d,lift_odds:%d",
		data.id, data.minpayrmb, data.maxpayrmb, data.maxwinscore, data.welfarecount, data.welfarepro, data.postime, data.lift_odds);

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, &msg, net::L2S_MSG_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE, 0);

		LOG_DEBUG("svrid:%d", server.svrID);
	}
}

// 通知服务器刷新自动杀分配置
void	CServerMgr::UpdateServerAutoKillCfg(string & strjvalueRecharge)
{
	//net::msg_auto_kill_user_cfg msg;
	//msg.set_updatejson(strjvalueRecharge);
	//SendMsg2AllServer(&msg, net::L2S_MSG_REFRESH_AUTO_KILL_CFG, 0);
}

// 通知服务器刷新自动杀分玩家列表
void	CServerMgr::UpdateServerAutoKillUsers(string & strjvalueRecharge)
{
	//net::msg_auto_kill_user_cfg msg;
	//msg.set_updatejson(strjvalueRecharge);
	//SendMsg2AllServer(&msg, net::L2S_MSG_REFRESH_AUTO_KILL_USERS, 0);
}

// 通知服务器刷新活跃福利配置
void	CServerMgr::UpdateServerActiveWelfareCfg(string & json_msg)
{
    net::msg_flush_active_welfare_cfg msg;
    msg.set_updatejson(json_msg);
    SendMsg2AllServer(&msg, net::L2S_MSG_REFRESH_ACTIVE_WELFARE_CFG, 0);
}

// 通知服务器重置所有在线玩家的活跃福利信息
void	CServerMgr::ResetPlayerActiveWelfareInfo(string & json_msg)
{
	net::msg_reset_active_welfare_info msg;
	msg.set_updatejson(json_msg);
	SendMsg2AllServer(&msg, net::L2S_MSG_RESET_ACTIVE_WELFARE_INFO, 0);
}

// 通知服务器刷新新帐号福利配置
void	CServerMgr::UpdateServerNewRegisterWelfareCfg(string & json_msg)
{
	net::msg_flush_new_register_welfare_cfg msg;
	msg.set_updatejson(json_msg);
	SendMsg2AllServer(&msg, net::L2S_MSG_RESET_NEW_REGISTER_WELFARE_INFO, 0);
}

void	CServerMgr::StopContrlPlayer(uint16 svrid, uint32 uid)
{
	LOG_DEBUG("svrid:%d, uid:%d", svrid, uid);
	net::msg_stop_conctrl_player msg;
	msg.set_uid(uid);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_STOP_CONTROL_PLAYER, uid);
}

void	CServerMgr::SynCtrlUserCfg(net::msg_syn_ctrl_user_cfg * pmsg)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_CTRL_USER_CFG, 0);
	}
}

// 通知游戏服修改房间库存配置  add by har
bool CServerMgr::NotifyGameSvrsChangeRoomStockCfg(uint32 gameType, stStockCfg &st) {
	uint16 svrid = GetGameTypeSvrID(gameType);
	if (svrid == 0)
		return false;
	net::msg_change_room_stock_cfg msg;
	msg.set_roomid(st.roomID);
	msg.set_stock_max(st.stockMax);
	msg.set_stock_conversion_rate(st.stockConversionRate);
	msg.set_jackpot_min(st.jackpotMin);
	msg.set_jackpot_max_rate(st.jackpotMaxRate);
	msg.set_jackpot_rate(st.jackpotRate);
	msg.set_jackpot_coefficient(st.jackpotCoefficient);
	msg.set_jackpot_extract_rate(st.jackpotExtractRate);
	msg.set_add_stock(st.stock);
	msg.set_kill_points_line(st.killPointsLine);
	msg.set_player_win_rate(st.playerWinRate);
	msg.set_add_jackpot(st.jackpot);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_CHANGE_ROOM_STOCK_CFG, 0);
	return true;
}

void	CServerMgr::SynLuckyCfg(net::msg_syn_lucky_cfg * pmsg)
{	
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_LUCKY_CFG, 0);
	}	
}

void	CServerMgr::SynFishCfg(net::msg_syn_fish_cfg * pmsg)
{
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		if (server.gameType == net::GAME_CATE_FISHING)
		{
			SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_FISH_CFG, 0);
		}
	}
}

void	CServerMgr::ResetLuckyCfg(net::msg_reset_lucky_cfg * pmsg)
{
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_RESET_LUCKY_CFG, 0);
	}
}

// 设置玩家登录时间
void CServerMgr::SetPlayerLoginTime(uint32 uid) {
	m_mpPlayerLoginTime[uid] = getTickCount64();
}
