

#include "msg_client_handle.h"
#include "data_cfg_mgr.h"
#include "stdafx.h"
#include "lobby_server_config.h"
#include "player.h"
#include "server_mgr.h"
#include "center_log.h"

using namespace Network;
using namespace svrlib;
using namespace std;

namespace
{

};
int CHandleClientMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)	
{
	LOG_DEBUG("lobby server recv client msg  - pNetObj:%p,uid:%d,uin:%d,cmd:%d,begin:%d,end:%d", pNetObj, pNetObj->GetUID(),head->uin, head->cmd, net::CMD_MSG_EVERY_COLOR_GAME_BEGIN, net::CMD_MSG_EVERY_COLOR_GAME_END);

#ifndef HANDLE_CLIENT_FUNC
#define HANDLE_CLIENT_FUNC(cmd,handle) \
	case cmd:\
	{\
		handle(pNetObj,pkt_buf,buf_len);\
	}break;
#endif
    //if(head->cmd > ROUTE_MSG_ID){
    //    return route_to_game_svr(pNetObj,pkt_buf,buf_len,head);
    //}
	switch (head->cmd)
	{
	case net::C2S_MSG_HEART:
	{
		net::msg_heart_test msg;
		msg.set_svr_time(getSysTime());
		SendProtobufMsg(pNetObj, &msg, net::C2S_MSG_HEART, 0);
		return 0;
	}break;
	HANDLE_CLIENT_FUNC(net::C2S_MSG_LOGIN, handle_msg_login);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_UPDATE_INFO_REQ, handle_msg_update_info);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_REQ_SVRS_INFO, handle_msg_req_svrs_info);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_ENTER_SVR, handle_msg_enter_gamesvr);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_LOGIN_SAFEBOX, handle_msg_login_safebox);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_CHANGE_SAFEBOX_PWD, handle_msg_change_safebox_pwd);// 修改保险箱密码
	HANDLE_CLIENT_FUNC(net::C2S_MSG_TAKE_SAFEBOX, handle_msg_take_safebox);// 保险箱存取操作
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GIVE_SAFEBOX, handle_msg_give_safebox);// 保险箱赠送
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GET_MISSION_PRIZE_REQ, handle_msg_get_mission_prize);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GET_LOGIN_REWARD_REQ, handle_msg_get_login_reward);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GET_BANKRUPT_HELP, handle_msg_get_bankrupt_help);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_EXCHANGE_SCORE_REQ, handle_msg_exchange_score);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_SPEAK_BROADCAST_REQ, handle_msg_speak_broadcast);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GET_HISTORY_SPEAK, handle_msg_get_history_speak);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_GET_SERVER_INFO, handle_msg_get_server_info);

	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_VIP_RECHARGE_REQ, handle_msg_notify_vip_recharge_req);

	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_UNION_PAY_RECHARGE_REQ, handle_msg_notify_union_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_WECHAT_PAY_RECHARGE_REQ, handle_msg_notify_wechat_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_ALI_PAY_RECHARGE_REQ, handle_msg_notify_ali_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_OTHER_PAY_RECHARGE_REQ, handle_msg_notify_other_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_QQ_PAY_RECHARGE_REQ, handle_msg_notify_qq_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE_REQ, handle_msg_notify_wechat_scan_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_JD_PAY_RECHARGE_REQ, handle_msg_notify_jd_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_APPLE_PAY_RECHARGE_REQ, handle_msg_notify_apple_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE_REQ, handle_msg_notify_large_ali_pay_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_VIP_ALIACC_RECHARGE_REQ, handle_msg_notify_large_ali_acc_recharge_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_EXCLUSIVE_ALIPAY_RECHARGE_SHOW_REQ, handle_msg_notify_exclusive_alipay_recharge_show_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_FIXED_ALIPAY_RECHARGE_SHOW_REQ, handle_msg_notify_fixed_alipay_recharge_show_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_FIXED_WECHAT_RECHARGE_SHOW_REQ, handle_msg_notify_fixed_wechat_recharge_show_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_FIXED_UNIONPAY_RECHARGE_SHOW_REQ, handle_msg_notify_fixed_unionpay_recharge_show_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE_SHOW_REQ, handle_msg_notify_exclusive_flash_recharge_show_req);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_CHAT_INFO_FORWARD_REQ, handle_msg_chat_info_forward_req);

	HANDLE_CLIENT_FUNC(net::C2S_MSG_EVERY_COLOR_GAME_ENTER, handle_msg_enter_everycolor_gamesvr);
	HANDLE_CLIENT_FUNC(net::C2S_MSG_EVERY_COLOR_GAME_LEAVE, handle_msg_leave_everycolor_gamesvr);

	
	default:
	{
		if (head->cmd > net::CMD_MSG_EVERY_COLOR_GAME_BEGIN && head->cmd < net::CMD_MSG_EVERY_COLOR_GAME_END)
		{//时时彩消息
			LOG_DEBUG(" everycolor server recv client msg  - uid:%d,uin:%d,cmd:%d", pNetObj->GetUID(), head->uin, head->cmd);

			return route_to_everycolor_game_svr(pNetObj, pkt_buf, buf_len, head);
		}
		else
		{
			return route_to_game_svr(pNetObj, pkt_buf, buf_len, head);
		}
		break;
	}
	}
	return 0;
}
// 转发给游戏服
int  CHandleClientMsg::route_to_game_svr(NetworkObject* pNetObj,const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    CPlayer* pPlayer = GetPlayer(pNetObj);
    if(pPlayer != NULL){
        CServerMgr::Instance().SendMsg2Server(pPlayer->GetCurSvrID(),pkt_buf,buf_len,head->cmd,pPlayer->GetUID());      
    }else{
		LOG_DEBUG("玩家不存在");
	}
    return 0;
}
int  CHandleClientMsg::route_to_everycolor_game_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	uint16 svrID = CServerMgr::Instance().GetGameTypeSvrID(net::GAME_CATE_EVERYCOLOR);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (svrID != 0 && pPlayer != NULL) {
		LOG_DEBUG(" everycolor server send to gameserver msg  - uid:%d,uin:%d,cmd:%d,svrID:%d", pPlayer->GetUID(), head->uin, head->cmd, svrID);
		CServerMgr::Instance().SendMsg2Server(svrID, pkt_buf, buf_len, head->cmd, pPlayer->GetUID());
	}
	else {
		LOG_DEBUG("everycolor server do not find - svrID:%d,uid:%d", svrID, pNetObj->GetUID());
	}
	return 0;
}

//登录 
int	 CHandleClientMsg::handle_msg_login(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_login_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	char chTmpIP[32] = { 0 };
	uint32 tempUID = 0;
	if (pNetObj != NULL) {
		strcpy(chTmpIP, pNetObj->GetSIP().c_str());
		tempUID = pNetObj->GetUID();
	}
	string strCity = msg.city();
	LOG_DEBUG("player login game server recv msg - pNetObj:%p,uid:%d,tempUID:%d,deviceid:%s,key:%s,IP:%s,strCity:%s", pNetObj,msg.uid(), tempUID,msg.deviceid().c_str(),msg.key().c_str(), chTmpIP, strCity.c_str());
	uint32 uid = msg.uid();
	if (uid == 0)
	{
		return 0;
	}
	net::msg_login_rep repmsg;
	repmsg.set_server_time(getSysTime());
	if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		repmsg.set_result(net::RESULT_CODE_SVR_REPAIR);
		SendProtobufMsg(pNetObj,&repmsg,net::S2C_MSG_LOGIN_REP,0);
		CServerMgr::Instance().SendSvrRepairContent(pNetObj);
		LOG_ERROR("the server is being repair,can not login,uid:%d,IP:%s.", uid, chTmpIP);
		return -1;
	}

	string strDecyPHP = msg.key();
	string strCheckCode = msg.check_code();
	CPlayer* pPlayerObj = NULL;
	if (pNetObj->GetUID() != 0)
	{
		pPlayerObj = (CPlayer*)CPlayerMgr::Instance().GetPlayer(pNetObj->GetUID());
	}
	
	CPlayer* pPlayerUid = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);

	NetworkObject *pPlayerOldSock = NULL;
	if (pPlayerUid != NULL)
	{
		pPlayerOldSock = pPlayerUid->GetSession();
	}
	LOG_DEBUG("player_session - uid:%d,pPlayerUid:%p,pPlayerOldSock:%p,newUID:%d,newpPlayerObj:%p,newpNetObj:%p",
		uid, pPlayerUid,pPlayerOldSock, tempUID, pPlayerObj,  pNetObj);

	if(LobbyServerConfig::Instance().bNeedPassWD == 1)// 校验密码
	{
		if(pPlayerUid == NULL || pPlayerUid->GetLoginKey() != strDecyPHP)
		{
			string strDecy = CRedisMgr::Instance().GetPlayerLoginKey(uid);
			if (strDecy != strDecyPHP) {
				//LOG_ERROR("密码验证不通过%d-PHP:%s已过期%s", uid, strDecyPHP.c_str(), strDecy.c_str());
				LOG_ERROR("loginkey error - uid:%d,strDecyPHP:%s,strDecy:%s,IP:%s", uid, strDecyPHP.c_str(), strDecy.c_str(), chTmpIP);
				//LOG_ERROR("the ip is:%s", pNetObj->GetSIP().c_str());
				repmsg.set_result(net::RESULT_CODE_PASSWD_ERROR);
				SendProtobufMsg(pNetObj, &repmsg, net::S2C_MSG_LOGIN_REP, 0);
				return -1;
			}
			CRedisMgr::Instance().RenewalLoginKey(uid);
		}
	}
    if(CRedisMgr::Instance().IsBlackList(uid))
	{
		//LOG_ERROR("黑名单不能进入游戏:%d",uid);
		LOG_ERROR("black list not enter game,uid:%d", uid);

		repmsg.set_result(net::RESULT_CODE_BLACKLIST);
		SendProtobufMsg(pNetObj,&repmsg,net::S2C_MSG_LOGIN_REP);
		return -1;
	}
	if(pPlayerObj != NULL)
	{
		LOG_ERROR("多次发送登录包");
		if(pPlayerObj->GetPlayerState() == PLAYER_STATE_LOAD_DATA){
			LOG_DEBUG("1 the user is logging in,uid:%d,IP:%s.", uid, chTmpIP);
			pPlayerObj->OnLogin();
		}else{
			LOG_DEBUG("2 the user is logging in,uid:%d,IP:%s.", uid, chTmpIP);
			pPlayerObj->NotifyEnterGame();
			pPlayerObj->SendAllPlayerData2Client();
		}
		pPlayerObj->SetLoginKey(strDecyPHP);
		pPlayerObj->SetCheckCode(strCheckCode);
		return 0;
	}
	else
	{
		if(pPlayerUid != NULL)// 在玩
		{
			NetworkObject *pOldSock = pPlayerUid->GetSession();
			pNetObj->SetUID(uid);
			pPlayerUid->SetSession(pNetObj);
			
			//LOG_ERROR("挤号处理:%d:old:%p--new:%p ", uid, pOldSock, pNetObj);
			int oldfd = 0;
			int newfd = 0;
			if (pOldSock != NULL)
			{
				oldfd = pOldSock->GetNetfd();
			}
			if (pNetObj != NULL)
			{
				newfd = pNetObj->GetNetfd();
			}
			LOG_DEBUG("player_push_against - uid:%d,oldfd:%d,newfd:%d,oldSock:%p,newSock:%p,strDecyPHP:%s,GetLoginKey:%s", uid, oldfd, newfd, pOldSock, pNetObj, strDecyPHP.c_str(), pPlayerUid->GetLoginKey().c_str());
			if(pPlayerUid->GetPlayerState() == PLAYER_STATE_PLAYING)
			{
				pPlayerUid->ReLogin();
				LOG_DEBUG("3 the user is logging in,uid:%d,IP:%s.", uid, chTmpIP);
			}
			else
			{
				pPlayerUid->OnLogin();
				LOG_DEBUG("4 the user is logging in,uid:%d,IP:%s.", uid, chTmpIP);
			}
			if(pOldSock != NULL)
			{
				if (pPlayerUid->GetLoginKey() != strDecyPHP)
				{
					net::msg_notify_leave_rep leavemsg;
					leavemsg.set_result(net::RESULT_CODE_LOGIN_OTHER);
					SendProtobufMsg(pOldSock, &leavemsg, net::S2C_MSG_NOTIFY_LEAVE);
				}
				pOldSock->SetUID(0);
				pOldSock->DestroyObj();
			}
			pPlayerUid->SetLoginKey(strDecyPHP);
			pPlayerUid->SetCheckCode(strCheckCode);
			return 0;
		}
	}
	CPlayer* pPlayer = new CPlayer(PLAYER_TYPE_ONLINE);
    pNetObj->SetUID(uid);
	pPlayer->SetSession(pNetObj);
	pPlayer->SetUID(uid);
	pPlayer->SetCity(strCity);
	CPlayerMgr::Instance().AddPlayer(pPlayer);
	pPlayer->OnLogin();
	pPlayer->SetLoginKey(strDecyPHP);
	pPlayer->SetCheckCode(strCheckCode);
	CServerMgr::Instance().SetPlayerLoginTime(uid);

	LOG_DEBUG("5 the user is logging in - uid:%d,IP:%s,strCity:%s strCheckCode:%s", uid, chTmpIP, strCity.c_str(), strCheckCode.c_str());
	//COUNT_OCCUPY_MEM_SZIE(this);
	return 0;
}
// 请求更新玩家信息
int  CHandleClientMsg::handle_msg_update_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("请求更新玩家信息：%s-%d",pNetObj->GetSIP().c_str() ,pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	pPlayer->SendAccData2Client();
	return 0;
}
// 请求服务器信息 
int  CHandleClientMsg::handle_msg_req_svrs_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	CServerMgr::Instance().SendSvrsInfo2Client(pNetObj->GetUID());
	return 0;
}
// 请求进入游戏服务器 
int  CHandleClientMsg::handle_msg_enter_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_enter_gamesvr_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	uint16 svrID = msg.svrid();
	LOG_DEBUG("MessageEnterGameServer - serverid:%d",svrID);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	bool bRet = pPlayer->EnterGameSvr(svrID);
	LOG_DEBUG("MessageEnterGameServer - serverid:%d,uid:%d,bRet:%d", svrID, pPlayer->GetUID(), bRet);
	if(!bRet)
	{
		//LOG_DEBUG("无法进入游戏服务器:%d--%d",pPlayer->GetUID(),svrID);
		LOG_DEBUG("DoNotEnterGameServer - uid:%d,serverid:%d", pPlayer->GetUID(), svrID);
		net:msg_enter_gamesvr_rep msgrep;
		msgrep.set_result(0);
		pPlayer->SendMsgToClient(&msgrep,net::S2C_MSG_ENTER_SVR_REP);
	}

	return 0;
}
// 请求登录保险箱
int  CHandleClientMsg::handle_msg_login_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_login_safebox_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	LOG_DEBUG("登录保险箱");
	net::msg_login_safebox_rep rep;
	if(!pPlayer->CheckSafePasswd(msg.passwd())){
		rep.set_result(net::RESULT_CODE_PASSWD_ERROR);
	}else{
		pPlayer->SetSafeBoxState(1);
		rep.set_result(1);
		if(!pPlayer->IsInLobby()){
			pPlayer->NotifyChangePlayerInfo2GameSvr();
		}
	}
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_LOGIN_SAFEBOX_REP);

	return 0;
}
// 修改保险箱密码
int  CHandleClientMsg::handle_msg_change_safebox_pwd(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("保险箱密码操作");
	net::msg_change_safebox_pwd_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	net::msg_change_safebox_pwd_rep rep;


	if(!pPlayer->IsInLobby()){
		rep.set_result(net::RESULT_CODE_NEED_INLOBBY);
	}else if(!pPlayer->CheckSafePasswd(msg.old_pwd())){
		rep.set_result(net::RESULT_CODE_PASSWD_ERROR);
	}else{
		pPlayer->SetSafePasswd(msg.new_pwd());
        pPlayer->SetSafeBoxState(1);
		CDBMysqlMgr::Instance().UpdatePlayerSafePasswd(pPlayer->GetUID(),msg.new_pwd());
		rep.set_result(1);
	}
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_CHANGE_SAFEBOX_PWD_REP);

	return 0;
}
// 保险箱存取操作
int  CHandleClientMsg::handle_msg_take_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_take_safebox_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	int64 takeCoin = msg.take_coin();
	net::msg_take_safebox_rep rep;
	rep.set_take_coin(takeCoin);
	rep.set_result(0);
	LOG_DEBUG("保险箱存取操作:%d--%lld",pPlayer->GetUID(),takeCoin);

	if(pPlayer->GetSafeBoxState() == 1){
		if(!pPlayer->IsInLobby()){
			pPlayer->SendMsgToGameSvr(&msg,net::C2S_MSG_TAKE_SAFEBOX);
			return 0;
		}
		if(pPlayer->TakeSafeBox(takeCoin)){
			rep.set_result(1);
		}else{
			rep.set_result(net::RESULT_CODE_CION_ERROR);
		}
	}else{
		rep.set_result(net::RESULT_CODE_ERROR_STATE);
	}
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_TAKE_SAFEBOX_REP);
	pPlayer->UpdateAccValue2Client();
	return 0;
}
// 保险箱赠送操作
int  CHandleClientMsg::handle_msg_give_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_give_safebox_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	int64 gcoin = msg.give_coin();
	uint32 guid  = msg.give_uid();

	if(gcoin < 100 || guid == 0)// 最少送100
		return 0;
	LOG_DEBUG("give safebox sendid:%d,guid:%d,gcoin:%lld",pPlayer->GetUID(),guid,gcoin);

	CPlayer* pTarPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(guid);
	net::msg_give_safebox_rep rep;
	
	rep.set_give_coin(gcoin);
	rep.set_give_uid(guid);
	if(!pPlayer->IsInLobby())
	{
		rep.set_result(net::RESULT_CODE_NEED_INLOBBY);
		pPlayer->SendMsgToClient(&rep,net::S2C_MSG_TAKE_SAFEBOX_REP);
		return 0;
	}
	if(pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN) < gcoin)
	{
		LOG_DEBUG("账号余额不足赠送金额:%lld--%lld",pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN),gcoin);
		rep.set_result(net::RESULT_CODE_CION_ERROR);
		pPlayer->SendMsgToClient(&rep,net::S2C_MSG_GIVE_SAFEBOX);
		return 0;
	}
	bool sendVip = pPlayer->IsVip();
	bool recvVip = CRedisMgr::Instance().IsVipPlayer(guid);
	if(!sendVip && !recvVip)
	{
		rep.set_result(net::RESULT_CODE_NOVIP);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_GIVE_SAFEBOX);
		return 0;
	}
	if(!recvVip){
		if(CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).IsExistPlayerID(guid) == false){
			rep.set_result(net::RESULT_CODE_ERROR_PLAYERID);
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_GIVE_SAFEBOX);
			return 0;
		}
	}
	// 扣金币
	pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_GIVE,guid,0,0,0,0,0,-gcoin);
	// 扣税插入订单
	int64 tax = (gcoin*CDataCfgMgr::Instance().GetGiveTax(sendVip,recvVip)/PRO_DENO_100);
	gcoin -= tax;    
	if(pTarPlayer != NULL)
	{
		pTarPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_GIVE,pPlayer->GetUID(),0,0,0,0,0,gcoin);
		pTarPlayer->UpdateAccValue2Client();
	}else{
		CCommonLogic::AtomChangeOfflineAccData(guid,emACCTRAN_OPER_TYPE_GIVE,pPlayer->GetUID(),0,0,0,0,0,gcoin);
	}
	CDBMysqlMgr::Instance().GiveSafeBox(pPlayer->GetUID(),guid,gcoin,tax,pPlayer->GetIPStr());
    
	rep.set_result(1);
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_GIVE_SAFEBOX);
	pPlayer->UpdateAccValue2Client();

	return 0;
}

//获得任务奖励
int  CHandleClientMsg::handle_msg_get_mission_prize(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("获得任务奖励");
	net::msg_get_mission_prize_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL || !pPlayer->IsInLobby())
		return 0;
	uint32 msid = msg.msid();
	pPlayer->GetMissionMgr().GetMissionPrize(msid);
	return 0;
}
//获得登陆奖励
int  CHandleClientMsg::handle_msg_get_login_reward(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("获得登陆奖励");
	net::msg_get_login_reward_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL || !pPlayer->IsInLobby())
		return 0;
	uint8 flag = msg.reward_flag();
	LOG_DEBUG("获得登陆奖励2  uid:%d,flag:%d", pPlayer->GetUID(), flag);
	uint8 bRet = 0;
	if(pPlayer->IsSetRewardBitFlag(flag)){
		LOG_DEBUG("奖励已经领取过了:uid:%d,flag:%d", pPlayer->GetUID(), flag);
		bRet = net::RESULT_CODE_REPEAT_GET;
	}else{
		switch(flag)
		{
		case net::REWARD_CLOGIN:
			{
                if(pPlayer->GetDayGameCount() < CDataCfgMgr::Instance().GetSignGameCount())
                {
                    LOG_DEBUG("游戏局数不够领取");
                    bRet = net::RESULT_CODE_FAIL;
                }
                else if(CRedisMgr::Instance().IsHaveSignInDev(pPlayer->GetLoginDeviceID())){
                    LOG_DEBUG("登录奖励IP限制");
                    bRet = net::RESULT_CODE_IP_LIMIT; 
                }
                else{
					pPlayer->SignIn();
    				bRet = net::RESULT_CODE_SUCCESS;
    				int64 coin = CDataCfgMgr::Instance().GetCLoginScore(pPlayer->GetCLogin());
    				pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_LOGIN,flag,0,coin,0,0,0,0);
                    CRedisMgr::Instance().AddSignInDev(pPlayer->GetLoginDeviceID(),pPlayer->GetUID());
					
                    string title    = "奖励提示"; 
                    string content  = CStringUtility::FormatToString("您已获得 %d 天签到奖励，奖金是 %.2F 筹码 (备注：7天及以后签到是 %.2F 筹码)",pPlayer->GetCLogin(),(coin*0.01),(CDataCfgMgr::Instance().GetCLoginScore(7)*0.01)); 
              
                    string nickname = "系统提示";
                    CDBMysqlMgr::Instance().SendMail(0,pPlayer->GetUID(),title,content,nickname);
                }
			}break;
		case net::REWARD_WLOGIN3:
			{
				bRet = (pPlayer->GetWLogin() < 3) ? net::RESULT_CODE_NOT_COND : net::RESULT_CODE_SUCCESS;
				if(bRet == net::RESULT_CODE_SUCCESS){
					int64 ingot = CDataCfgMgr::Instance().GetWLoginIngot(2);
					pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_LOGIN,flag,0,0,ingot,0,0,0);
				}
			}break;
		case net::REWARD_WLOGIN5:
			{
				bRet = (pPlayer->GetWLogin() < 5) ? net::RESULT_CODE_NOT_COND : net::RESULT_CODE_SUCCESS;
				if(bRet == net::RESULT_CODE_SUCCESS ){
					int64 ingot = CDataCfgMgr::Instance().GetWLoginIngot(4);
					pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_LOGIN,flag,0,0,ingot,0,0,0);
				}
			}break;
		case net::REWARD_WLOGIN6:
			{
				bRet = (pPlayer->GetWLogin() < 6) ? net::RESULT_CODE_NOT_COND : net::RESULT_CODE_SUCCESS;
				if(bRet == net::RESULT_CODE_SUCCESS){
					int64 ingot = CDataCfgMgr::Instance().GetWLoginIngot(5);
					pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_LOGIN,flag,0,0,ingot,0,0,0);
				}
			}break;
		default:
			break;
		}
	}
	if(bRet == net::RESULT_CODE_SUCCESS){
		pPlayer->SetRewardBitFlag(flag);
		pPlayer->UpdateBaseValue2Client();
		pPlayer->UpdateAccValue2Client();
		pPlayer->SaveLoginInfo();
	}
	net::msg_get_login_reward_rep rep;
	rep.set_reward_flag(flag);
	rep.set_result(bRet);
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_GET_LOGIN_REWARD_REP);

	return 0;
}
// 破产补助
int  CHandleClientMsg::handle_msg_get_bankrupt_help(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("破产补助");
	net::msg_get_bankrupt_help_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL || !pPlayer->IsInLobby())
		return 0;
	net::msg_get_bankrupt_help_rep rep;
	uint8 bRet = pPlayer->GetBankruptHelp() ? 1 : 0;

	rep.set_result(bRet);
	rep.set_bankrupt_count(pPlayer->GetBankrupt());
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_GET_BANKRUPT_HELP_REP);

	return 0;
}
// 兑换积分
int  CHandleClientMsg::handle_msg_exchange_score(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("兑换积分");
	net::msg_exchange_score_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if(pPlayer == NULL)
		return 0;
	if(!pPlayer->IsInLobby()){
		pPlayer->SendMsgToGameSvr(&msg,net::C2S_MSG_EXCHANGE_SCORE_REQ);
		return 0;
	}

	net::msg_exchange_score_rep rep;
	rep.set_exchange_id(msg.exchange_id());
	rep.set_exchange_type(msg.exchange_type());
	uint8 bRet = pPlayer->ExchangeScore(msg.exchange_id(),msg.exchange_type());
	rep.set_result(bRet);
	pPlayer->SendMsgToClient(&rep,net::S2C_MSG_EXCHANGE_SCORE_REP);
	pPlayer->UpdateAccValue2Client();
	return 0;
}
// 发送喇叭
int  CHandleClientMsg::handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_speak_broadcast_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CPlayer* pPlayer = GetPlayer(pNetObj);
    if(pPlayer == NULL)
        return 0;
    net::msg_speak_oper_rep operrep;
    if(pPlayer->GetSpeakCDTime() > 0){
        operrep.set_result(net::RESULT_CODE_CDING);
        operrep.set_cdtime(pPlayer->GetSpeakCDTime());
        pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP);
        return 0;
    }    
    LOG_DEBUG("发送喇叭:%d--%s",pPlayer->GetUID(),msg.msg().c_str());
    if(!pPlayer->IsInLobby()){
        LOG_DEBUG("不在大厅服务器");
        pPlayer->SendMsgToGameSvr(&msg,net::C2S_MSG_SPEAK_BROADCAST_REQ);        
        return 0;
    }
    
    int64 cost = CDataCfgMgr::Instance().GetSpeakCost();
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("财富币不够:%lld",cost);
        operrep.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP);
        return 0;
    }
    pPlayer->Speak();
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_SPEAK,0,0,-cost,0,0,0,0);
    pPlayer->UpdateAccValue2Client();

    operrep.set_result(net::RESULT_CODE_SUCCESS);
    pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP);

    net::msg_speak_broadcast_rep repmsg;
    repmsg.set_send_id(pPlayer->GetUID());
    repmsg.set_send_name(pPlayer->GetPlayerName());
    repmsg.set_msg(msg.msg());

    CPlayerMgr::Instance().SendMsgToAll(&repmsg,net::S2C_MSG_SPEAK_BROADCAST_REP);
    CGobalEventMgr::Instance().AddSpeak(repmsg);
    
    return 0;
}
// 获取历史喇叭记录
int  CHandleClientMsg::handle_msg_get_history_speak(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_get_history_speak msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CPlayer* pPlayer = GetPlayer(pNetObj);
    if(pPlayer == NULL)
        return 0;
    LOG_DEBUG("获取喇叭历史记录:%d",pPlayer->GetUID());

    CGobalEventMgr::Instance().SendSpeakListToPlayer(pPlayer);
    
    return 0;
}
int  CHandleClientMsg::handle_msg_get_server_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
    net::msg_get_server_info msg;
    PARSE_MSG_FROM_ARRAY(msg);
        
    CPlayer* pPlayer = GetPlayer(pNetObj);
    if(pPlayer == NULL)
        return 0;

    CServerMgr::Instance().SendSvrsPlayInfo2Client(pPlayer->GetUID());
        
    return 0;
}

//进入时时彩服务器
int  CHandleClientMsg::handle_msg_enter_everycolor_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{

	uint16 svrID = CServerMgr::Instance().GetGameTypeSvrID(net::GAME_CATE_EVERYCOLOR);
	LOG_DEBUG("entering everycolor game server - uid:%d,serverid:%d", pNetObj->GetUID(), svrID);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL || svrID == 0)
	{
		return 0;
	}

	LOG_DEBUG("player enter everycolor game server - uid:%d,serverid:%d", pPlayer->GetUID(), svrID);

	bool bRet = pPlayer->EnterEveryColorGameSvr(svrID);
	if (!bRet)
	{
		LOG_DEBUG("do not enter game server uid:%d,serverid:%d", pPlayer->GetUID(), svrID);
	}

	return 0;
}

//离开时时彩服务器
int  CHandleClientMsg::handle_msg_leave_everycolor_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{

	uint16 svrID = CServerMgr::Instance().GetGameTypeSvrID(net::GAME_CATE_EVERYCOLOR);
	LOG_DEBUG("Leaveing everycolor game server - uid:%d,serverid:%d", pNetObj->GetUID(), svrID);
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL || svrID == 0)
		return 0;

	LOG_DEBUG("player Leave everycolor game server - uid:%d,serverid:%d", pPlayer->GetUID(), svrID);

	bool bRet = pPlayer->LeaveEveryColorGameSvr(svrID);
	if (!bRet)
	{
		LOG_DEBUG("do not Leave game server uid:%d,serverid:%d", pPlayer->GetUID(), svrID);
	}

	return 0;
}

//vip充值界面信息
int  CHandleClientMsg::handle_msg_notify_vip_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("vip recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}

	LOG_DEBUG("vip recharge show req 2 - uid:%d", pPlayer->GetUID());

	pPlayer->SendVipProxyRecharge();

	return 0;
}

int  CHandleClientMsg::handle_msg_notify_union_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendUnionPayRecharge();
	return 0;
}

int  CHandleClientMsg::handle_msg_notify_wechat_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendWeChatPayRecharge();
	return 0;
}
int  CHandleClientMsg::handle_msg_notify_ali_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendAliPayRecharge();
	return 0;
}
int  CHandleClientMsg::handle_msg_notify_other_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendOtherPayRecharge();
	return 0;
}
int  CHandleClientMsg::handle_msg_notify_qq_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendQQPayRecharge();
	return 0;
}
int  CHandleClientMsg::handle_msg_notify_wechat_scan_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendWeChatScanPayRecharge();
	return 0;
}
int  CHandleClientMsg::handle_msg_notify_jd_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendJDPayRecharge();
	return 0;
}

int  CHandleClientMsg::handle_msg_notify_apple_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendApplePayRecharge();
	return 0;
}

int  CHandleClientMsg::handle_msg_notify_large_ali_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("pay recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("pay recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendLargeAliPayRecharge();
	return 0;
}

int  CHandleClientMsg::handle_msg_notify_large_ali_acc_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	LOG_DEBUG("ali recharge show req - uid:%d", pNetObj->GetUID());
	CPlayer* pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL)
	{
		return 0;
	}
	LOG_DEBUG("ali recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendVipAliAccRecharge();
	return 0;
}

int CHandleClientMsg::handle_msg_notify_exclusive_alipay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_DEBUG("exclusive recharge show req pPlayer == NULL - uid:%d", pNetObj->GetUID());
		return 0;
	}
	LOG_DEBUG("exclusive recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendExclusiveAlipayRecharge();
	return 0;
}

// 定额支付宝显示请求
int CHandleClientMsg::handle_msg_notify_fixed_alipay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_DEBUG("fixed recharge show req pPlayer == NULL - uid:%d", pNetObj->GetUID());
		return 0;
	}
	LOG_DEBUG("fixed recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendFixedAlipayRecharge();
	return 0;
}

int CHandleClientMsg::handle_msg_notify_fixed_wechat_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_DEBUG("fixed recharge show req pPlayer == NULL - uid:%d", pNetObj->GetUID());
		return 0;
	}
	LOG_DEBUG("fixed recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendFixedWechatRecharge();
	return 0;
}

int CHandleClientMsg::handle_msg_notify_fixed_unionpay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_DEBUG("fixed recharge show req pPlayer == NULL - uid:%d", pNetObj->GetUID());
		return 0;
	}
	LOG_DEBUG("fixed recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendFixedUnionpayRecharge();
	return 0;
}

int CHandleClientMsg::handle_msg_notify_exclusive_flash_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_DEBUG("recharge show req pPlayer == NULL - uid:%d", pNetObj->GetUID());
		return 0;
	}
	LOG_DEBUG("recharge show req 2 - uid:%d", pPlayer->GetUID());
	pPlayer->SendExclusiveFlashRecharge();
	return 0;
}

// 聊天信息转发
int CHandleClientMsg::handle_msg_chat_info_forward_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	CPlayer *pPlayer = GetPlayer(pNetObj);
	if (pPlayer == NULL) {
		LOG_ERROR("pPlayer == NULL  GetNetfd:%d,GetUID:%d", pNetObj->GetNetfd(), pNetObj->GetUID());
		return 0;
	}
	net::msg_chat_info_forward msg;
	PARSE_MSG_FROM_ARRAY(msg);

	//  校验参数
	int toid_size = msg.toid_size();
	if (toid_size < 1) {
		LOG_ERROR("toid_size < 1  GetNetfd:%d,GetUID:%d,toid_size:%d",
			pNetObj->GetNetfd(), pNetObj->GetUID(), toid_size);
		return 0;
	}

	msg.set_time(getTime());
	string str;
	for (int i = 0; i < toid_size; ++i) {
		uint32 uid = msg.toid(i);
		int online = 0;
		string name("*");
		CPlayer *pPlayer2 = static_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
		if (pPlayer2 == NULL) {
			msg.add_online(0);
			msg.add_toname(name);
		} else {
			online = 1;
			name = pPlayer2->GetNickName();
			msg.add_online(1);
			msg.add_toname(name);
			if (pPlayer != pPlayer2)
			    pPlayer2->SendMsgToClient(&msg, net::S2C_MSG_CHAT_INFO_FORWARD_REP);
		}
		str += CStringUtility::FormatToString("uid_%d,online_%d,name_%s ", uid, online, name.c_str());
	}

	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_CHAT_INFO_FORWARD_REP);
	LOG_DEBUG("uid:%d,str:%s", pPlayer->GetUID(), str.c_str());
	return 0;
}

CPlayer* CHandleClientMsg::GetPlayer(NetworkObject* pNetObj)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(pNetObj->GetUID());
	if(pPlayer == NULL || !pPlayer->IsPlaying()){
		LOG_DEBUG("玩家不存在，或者玩家不在在线状态:%d",pNetObj->GetUID());
		return NULL;
	}
	return pPlayer;
}
