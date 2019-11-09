#include <gobal_robot_mgr.h>
#include "stdafx.h"
#include "msg_server_handle.h"
#include "pb/msg_define.pb.h"
#include "lobby_server_config.h"
#include "player.h"
#include "helper/bufferStream.h"
#include "packet/streampacket.h"
#include "server_mgr.h"
#include "center_log.h"

using namespace Network;
using namespace svrlib;

int CHandleServerMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)	
{
#ifndef HANDLE_SERVER_FUNC
#define HANDLE_SERVER_FUNC(cmd,handle) \
	case cmd:\
	{ \
		handle(pNetObj,pkt_buf,buf_len);\
	}break;
#endif
	//LOG_DEBUG("收到游戏服务器消息:uin:%d--cmd:%d",head->uin,head->cmd);

    //if(head->cmd > ROUTE_MSG_ID)
    //{
    //    return route_to_client(pNetObj,pkt_buf,buf_len,head);
    //} 
	switch(head->cmd)
	{
    HANDLE_SERVER_FUNC(net::S2L_MSG_REGISTER,handle_msg_register_svr);
    HANDLE_SERVER_FUNC(net::S2L_MSG_REPORT,handle_msg_report);
    HANDLE_SERVER_FUNC(net::S2L_MSG_LEAVE_SVR,handle_msg_leave_svr);
    HANDLE_SERVER_FUNC(net::S2L_MSG_NOTIFY_CHANGE_ACCOUNT_DATA,handle_msg_notify_change_account_data);
	HANDLE_SERVER_FUNC(net::S2L_MSG_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, handle_msg_update_lobby_change_account_data);
	HANDLE_SERVER_FUNC(net::S2L_MSG_NOTIFY_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, handle_msg_notify_update_lobby_change_account_data);

    HANDLE_SERVER_FUNC(net::S2L_MSG_REPORT_GAME_RESULT,handle_msg_report_game_result);
	HANDLE_SERVER_FUNC(net::S2L_MSG_REPORT_FEE_LOG, handle_msg_report_fee_log);
    
    HANDLE_SERVER_FUNC(net::S2C_MSG_SPEAK_BROADCAST_REP,handle_msg_speak_broadcast);

	HANDLE_SERVER_FUNC(net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_STATE, handle_msg_every_color_snatch_coin_state);

	default:
        return route_to_client(pNetObj,pkt_buf,buf_len,head);
		break;		
	}
	return 0;
}
// 转发给客户端
int  CHandleServerMsg::route_to_client(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    //LOG_DEBUG("转发给客户端消息:uid:%d--cmd:%d",head->uin,head->cmd);
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(head->uin));
    if(pPlayer != NULL){
        pPlayer->SendMsgToClient(pkt_buf,buf_len,head->cmd);        
    }else{
        NotifyLeaveState(pNetObj,head->uin);
    }
    return 0;
}
//服务器注册 
int	 CHandleServerMsg::handle_msg_register_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_register_svr msg;
	PARSE_MSG_FROM_ARRAY(msg);

	LOG_DEBUG("game server regi - svrid:%d,gametype:%d,game_subtype:%d,robot:%d",msg.svrid(),msg.game_type(),msg.game_subtype(), msg.robot());
	net::msg_register_svr_rep repmsg;

    bool bRet = CServerMgr::Instance().AddServer(pNetObj,msg.svrid(),msg.game_type(),msg.game_subtype(),msg.robot());    
    if(!bRet){
        LOG_ERROR("faild :%d",msg.svrid());
    }
	repmsg.set_result(bRet);		

	SendProtobufMsg(pNetObj,&repmsg,net::L2S_MSG_REGISTER_REP,0);
    
    return 0;
}
// 服务器上报信息
int  CHandleServerMsg::handle_msg_report(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_report_svr_info msg;
    PARSE_MSG_FROM_ARRAY(msg);

    uint32 players = msg.onlines();
    uint32 robots  = msg.robots();

    stGServer* pServer = CServerMgr::Instance().GetServerBySocket(pNetObj);
    if(pServer != NULL){
        pServer->playerNum = players;
        pServer->robotNum  = robots;
    }
    
    return 0;            
}
// 返回大厅
int  CHandleServerMsg::handle_msg_leave_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_leave_svr msg;
    PARSE_MSG_FROM_ARRAY(msg);

    uint32 uid   = msg.uid();
    uint16 svrid = msg.goto_svr();
    LOG_DEBUG("通知返回大厅:%d-->goto:%d",uid,svrid);
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        pPlayer->SetCurSvrID(0,false);
        pPlayer->BackLobby();
        if(svrid != 0){
            LOG_DEBUG("跳转游戏服务器:%d",svrid);
            bool bRet = pPlayer->EnterGameSvr(svrid);
            if(!bRet)
            {
                LOG_DEBUG("无法进入游戏服务器:%d--%d",pPlayer->GetUID(),svrid);
                net:msg_enter_gamesvr_rep msgrep;
                msgrep.set_result(0);
                pPlayer->SendMsgToClient(&msgrep,net::S2C_MSG_ENTER_SVR_REP);

                pPlayer->NotifyClientBackLobby();
            }
        }else{
            CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pPlayer->GetUID(),CApplication::Instance().GetServerID(),0,pPlayer->GetPlayerType());
        }
    }else{
        CRedisMgr::Instance().SetPlayerOnlineSvrID(uid,0);
    }
    return 0;
}
// 修改玩家数值
int  CHandleServerMsg::handle_msg_notify_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_notify_change_account_data msg;
    PARSE_MSG_FROM_ARRAY(msg);

    uint32 uid      = msg.uid();
    int64  diamond  = msg.diamond();
    int64  coin     = msg.coin();
    int64  score    = msg.score();
    int64  ingot    = msg.ingot();
    int64  cvalue   = msg.cvalue();
    int64  safeCoin = msg.safe_coin();
    int32  operType = msg.oper_type();
    int32  subType  = msg.sub_type();
    string chessid  = msg.chessid();

    //LOG_DEBUG("游戏服通知修改玩家 %d 数值:diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld",uid,diamond,coin,score,ingot,cvalue,safeCoin);

    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL && pPlayer->CanModifyData())
    {
        pPlayer->SyncChangeAccountValue(operType,subType,diamond,coin,ingot,score,cvalue,safeCoin,chessid);
        pPlayer->UpdateAccValue2Client();
    }else{
        CCommonLogic::AtomChangeOfflineAccData(uid,operType,subType,diamond,coin,ingot,score,cvalue,safeCoin,chessid);
        NotifyLeaveState(pNetObj,uid);
    }
    return 0;
}

// 修改玩家数值
int  CHandleServerMsg::handle_msg_update_lobby_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_update_lobby_change_account_data msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 uid = msg.uid();
	int64  diamond = msg.diamond();
	int64  coin = msg.coin();
	int64  score = msg.score();
	int64  ingot = msg.ingot();
	int64  cvalue = msg.cvalue();
	int64  safeCoin = msg.safe_coin();
	int32  operType = msg.oper_type();
	int32  subType = msg.sub_type();
	string chessid = msg.chessid();

	LOG_DEBUG("游戏服通知修改玩家 uid:%d 数值 - diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld", uid, diamond, coin, score, ingot, cvalue, safeCoin);
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	//新增游戏中更新数据
	if (pPlayer != NULL)
	{
		pPlayer->SyncChangeAccountValue(operType, subType, diamond, coin, ingot, score, cvalue, safeCoin, chessid);
		pPlayer->UpdateAccValue2Client();
		stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
		if (pServer != NULL)
		{
			pPlayer->UpDateChangeAccData2GameSvr(diamond, coin, ingot, score, cvalue, safeCoin, operType, subType);
		}
	}
	else {
		CCommonLogic::AtomChangeOfflineAccData(uid, operType, subType, diamond, coin, ingot, score, cvalue, safeCoin, chessid);
		NotifyLeaveState(pNetObj, uid);
	}
	return 0;
}

// 修改玩家数值
int  CHandleServerMsg::handle_msg_notify_update_lobby_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_update_lobby_change_account_data msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 uid = msg.uid();
	int64  diamond = msg.diamond();
	int64  coin = msg.coin();
	int64  score = msg.score();
	int64  ingot = msg.ingot();
	int64  cvalue = msg.cvalue();
	int64  safeCoin = msg.safe_coin();
	int32  operType = msg.oper_type();
	int32  subType = msg.sub_type();
	string chessid = msg.chessid();

	LOG_DEBUG("游戏服通知修改玩家 uid:%d 数值 - diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld", uid, diamond, coin, score, ingot, cvalue, safeCoin);
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	//新增游戏中更新数据
	if (pPlayer != NULL)
	{
		stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
		if (pServer != NULL)
		{
			pPlayer->UpDateChangeAccData2GameSvrEveryColor(diamond, coin, ingot, score, cvalue, safeCoin, operType, subType);
		}
	}
	return 0;
}

// 上报游戏结果
int  CHandleServerMsg::handle_msg_report_game_result(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_report_game_result msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 gameType = msg.game_type();
    uint32 uid      = msg.uid();
    uint8  consume  = msg.consume();
	int64 lBetScore = msg.bet_score();
	int32 branker = msg.branker();
	int64 lWinScore = msg.win_score();
	int welcount = msg.welcount();
	uint64 weltime = msg.weltime();

    bool   isCoin = (consume == net::ROOM_CONSUME_TYPE_COIN) ? true : false;
    int32  win  = msg.win_score() > 0 ? 1 : 0;
    int32  lose = (win==1) ? 0 : 1;
    uint32 cate1 = msg.game_type(); //游戏id
    uint32 cate2 = consume;         //货币种类
    uint32 cate3 = msg.roomid();    //房间类别
    uint32 cate4 = 0;               //房间级别
    
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer == NULL)
    {
        NotifyLeaveState(pNetObj,uid);
        return 0;
    }
	if (!pPlayer->CanModifyData())
	{
		return 0;
	}
	pPlayer->SetWelCount(welcount);
	pPlayer->SetWelTime(weltime);
    pPlayer->OnGameEnd(gameType);

	LOG_DEBUG("uid:%d,cate1:%d, cate2:%d, cate3:%d, cate4:%d,branker:%d, lBetScore:%lld, lWinScore:%lld,welcount:%d,weltime:%lld",
		uid, cate1, cate2, cate3, cate4, branker, lBetScore, lWinScore, welcount, weltime);
    // 激活任务
    //pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY, 1, cate1 , cate2 , cate3 , cate4);
    //pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_WIN, win, cate1 , cate2 , cate3 , cate4);
	pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_FINISH, 1, cate1, cate2, cate3, cate4);
	pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_BRANKER, branker, cate1, cate2, cate3, cate4);
	pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_JETTON, lBetScore, cate1, cate2, cate3, cate4);
	pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_WIN, win, cate1, cate2, cate3, cate4);
	if (lWinScore > 0)
	{
		pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_SING_WIN, lWinScore, cate1, cate2, cate3, cate4);
		pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_STAT_WIN, lWinScore, cate1, cate2, cate3, cate4);
	}
	if (lWinScore >0 && branker == 1)
	{
		pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_BRANKER_WIN, 1, cate1, cate2, cate3, cate4);
	}
	if (lWinScore <=0 && branker == 1)
	{
		pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY_BRANKER_WIN, 0, cate1, cate2, cate3, cate4);
	}

    if(gameType == net::GAME_CATE_LAND)
	{
        int32  land   = msg.land().is_land();
        int32  spring = msg.land().is_spring();
        uint32 cate3    = msg.land().deal();      //房间类别
        uint32 cate4    = msg.land().basescore(); //房间级别
        pPlayer->ChangeLandValue(isCoin,win,lose,land,spring,msg.win_score());
        // 激活任务
        //pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PRESS, msg.land().press_count(), cate1 , cate2 , cate3 , cate4);
        //pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_KILL, msg.land().bankrupt_count(), cate1 , cate2 , cate3 , cate4);        
    }else{
        pPlayer->ChangeGameValue(gameType,isCoin,win,lose,msg.win_score());             
    }
	if (msg.ex_win_score()>0)
	{
		pPlayer->ReSetGameCount(gameType);
	}
    pPlayer->UpdateGameInfo2Client(gameType); 
    pPlayer->UpdateBaseValue2Client();
    pPlayer->GetMissionMgr().SaveMiss();
    return 0;
}
// 上报抽水日志
int  CHandleServerMsg::handle_msg_report_fee_log(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len) 
{
    net::msg_report_fee_log msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid      = msg.uid();
    int64  feeWin   = msg.fee_win();
    int64  feeLose  = msg.fee_lose();

    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer == NULL)
    {
        NotifyLeaveState(pNetObj,uid);
        return 0;
    }
    if(!pPlayer->CanModifyData())
        return 0;
        
    // 激活任务
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_FEEWIN, feeWin, 0,0,0,0);
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_FEELOSE,feeLose,0,0,0,0);  
    pPlayer->GetMissionMgr().SaveMiss();
    
    return 0;
}
// 喇叭返回结果
int  CHandleServerMsg::handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_speak_broadcast_rep msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("喇叭返回");

    uint32 uid = msg.send_id();
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        pPlayer->Speak();
    }else{
        NotifyLeaveState(pNetObj,uid);
    }
    
    CPlayerMgr::Instance().SendMsgToAll(&msg,net::S2C_MSG_SPEAK_BROADCAST_REP);
    CGobalEventMgr::Instance().AddSpeak(msg);    
    
    return 0;
}
//夺宝状态发送
int  CHandleServerMsg::handle_msg_every_color_snatch_coin_state(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_everycolor_snatch_coin_state msg;
	PARSE_MSG_FROM_ARRAY(msg);

	LOG_ERROR("success - serverid:%d,stop:%d", pNetObj->GetUID(), msg.stop_state());


	CPlayerMgr::Instance().SendMsgToAll(&msg, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_STATE);

	return 0;
}
void  CHandleServerMsg::NotifyLeaveState(NetworkObject* pNetObj,uint32 uid)
{
	if (uid == 0)
	{
		return;
	}
	LOG_ERROR("uid:%d", pNetObj->GetUID());

    net::msg_notify_net_state msg;
	msg.set_uid(uid);
	msg.set_state(0);
    SendProtobufMsg(pNetObj,&msg,net::L2S_MSG_NOTIFY_NET_STATE,uid);    
}
    
