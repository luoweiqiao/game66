//
// Created by toney on 16/4/18.
//

#include <center_log.h>
#include <server_mgr.h>
#include "msg_php_handle.h"
#include "stdafx.h"
#include "lobby_server_config.h"
#include "player.h"
#include "pb/msg_define.pb.h"
#include "json/json.h"
#include "gobal_robot_mgr.h"
#include "db_struct_define.h"
#include "data_cfg_mgr.h"

using namespace Network;
using namespace svrlib;

int CHandlePHPMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
#ifndef HANDLE_PHP_FUNC
#define HANDLE_PHP_FUNC(cmd,handle) \
	case cmd:\
	{ \
		handle(pNetObj,stream);\
	}break;
#endif
    uint16 lenght = 0;
    uint16 cmd    = 0;
    CBufferStream stream((char*)pkt_buf,buf_len);
    stream.read_(lenght);
    stream.read_(cmd);
    switch(cmd)
    {
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_SAFEPWD,handle_php_change_safepwd);
    HANDLE_PHP_FUNC(net::PHP_MSG_BROADCAST,handle_php_broadcast);
    HANDLE_PHP_FUNC(net::PHP_MSG_SYS_NOTICE,handle_php_sys_notice);
    HANDLE_PHP_FUNC(net::PHP_MSG_KILL_PLAYER,handle_php_kill_player);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ACCVALUE,handle_php_change_accvalue);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_NAME,handle_php_change_name);
    HANDLE_PHP_FUNC(net::PHP_MSG_STOP_SERVICE,handle_php_stop_service);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROBOT,handle_php_change_robot);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_VIP,handle_php_change_vip);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROOM_PARAM, handle_php_change_room_param);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_PLAYER, handle_php_control_player);
	HANDLE_PHP_FUNC(net::PHP_MSG_UPDATE_ACCVALUE_INGAME, handle_php_update_accvalue_ingame);
	HANDLE_PHP_FUNC(net::PHP_MSG_STOP_SNATCH_COIN, handle_php_stop_snatch_coin);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_VIP_BROADCAST, handle_php_change_vip_broadcast);
	HANDLE_PHP_FUNC(net::PHP_MSG_ROBOT_SNATCH_COIN, handle_php_robot_snatch_coin);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE, handle_php_notify_vip_proxy_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_MULTI_PLAYER, handle_php_control_multi_player);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_DICE_GAME_CARD, handle_php_control_dice_game_card);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE, handle_php_notify_union_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE, handle_php_notify_wechat_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE, handle_php_notify_ali_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE, handle_php_notify_other_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE, handle_php_notify_qq_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE, handle_php_notify_wechat_scan_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE, handle_php_notify_jd_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE, handle_php_notify_apple_pay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE, handle_php_notify_large_ali_pay_recharge);

	HANDLE_PHP_FUNC(net::PHP_MSG_ONLINE_CONFIG_ROBOT, handle_php_online_config_robot);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_MAJIANG_CARD, handle_php_config_mahiang_card);
	HANDLE_PHP_FUNC(net::PHP_MSG_REFRESH_AUTO_KILL_CFG, handle_php_refresh_auto_kill_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_REFRESH_AUTO_KILL_USERS, handle_php_refresh_auto_kill_users);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT, handle_php_update_new_player_novice_welfare_right);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE, handle_php_update_new_player_novice_welfare_value);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_SIGN_IN_UPDATE, handle_php_sign_in_update);
    HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_ACTIVE_WELFARE_CFG_UPDATE, handle_php_active_welfare_cfg_update);
    HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_ACTIVE_WELFARE_INFO_CLEAR, handle_php_active_welfare_cfg_clear);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_NEW_REGISTER_WELFARE_CFG_UPDATE, handle_php_new_register_welfare_cfg_update);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_CONTROL_USER, handle_php_change_user_ctrl_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROOM_STOCK_CFG, handle_php_change_room_stock_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_EXCLUSIVE_ALIPAY_RECHARGE, handle_php_notify_exclusive_alipay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_LUCKY_INFO, handle_php_change_lucky_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_FISH_INFO, handle_php_change_fish_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_RESET_LUCK_CONFIG_INFO, handle_php_reset_lucky_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_FIXED_ALIPAY_RECHARGE, handle_php_notify_fixed_alipay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_FIXED_WECHAT_RECHARGE, handle_php_notify_fixed_wechat_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_FIXED_UNIONPAY_RECHARGE, handle_php_notify_fixed_unionpay_recharge);
	HANDLE_PHP_FUNC(net::PHP_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE, handle_php_notify_exclusive_flash_recharge);

    default:
        break;
    }
    return 0;
}
/*------------------PHP消息---------------------------------*/
// PHP修改保险箱密码
int  CHandlePHPMsg::handle_php_change_safepwd(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  rep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("pwd"))
    {
        LOG_ERROR("解析连续登陆json错误");
        rep["ret"] = 0;
        SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["pwd"].isString())
    {
        LOG_ERROR("json参数类型错误");
        rep["ret"] = 0;
        SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    string passwd = jvalue["pwd"].asString();
    LOG_DEBUG("php修改保险箱密码:%d-%s",uid,passwd.c_str());
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL){
        pPlayer->SetSafePasswd(passwd);
    }
    rep["ret"] = 1;
    SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
    return 0;
}
// PHP广播消息
int  CHandlePHPMsg::handle_php_broadcast(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
	LOG_DEBUG("json analysis begin - msg:%s", msg.c_str());
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("msg") )
    {
        LOG_ERROR("json data error");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["msg"].isString())
    {
        LOG_ERROR("json param error");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
	// add type 0 发送所有玩家 1 批量发送玩家
    uint32 uid = jvalue["uid"].asUInt();
	//uint32 type = jvalue["type"].asUInt();
	//Json::Value  juids = jvalue["uid"];
    string broadCast = jvalue["msg"].asString();

    LOG_DEBUG("phpbroadcast - uid:%d,msg:%s", uid,broadCast.c_str());
    net::msg_php_broadcast_rep rep;
    rep.set_msg(broadCast);
    if(uid == 0)
	{
        CPlayerMgr::Instance().SendMsgToAll(&rep,net::S2C_MSG_PHP_BROADCAST);
    }
	else
	{
		CPlayer *pPlayer = (CPlayer *)CPlayerMgr::Instance().GetPlayer(uid);
		LOG_DEBUG("uid:%d,pPlayer:%p", uid, pPlayer);
		if (pPlayer != NULL)
		{
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PHP_BROADCAST);
		}

		//for (uint32 i = 0; i < juids.size(); i++)
		//{
		//	uint32 uid = juids[i].asUInt();
		//	CPlayer *pPlayer = (CPlayer *)CPlayerMgr::Instance().GetPlayer(uid);

		//	LOG_DEBUG("size:%d,i:%d,uid:%d,play:%p", juids.size(), i, uid, pPlayer);

		//	if (pPlayer != NULL)
		//	{
		//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_PHP_BROADCAST);
		//	}
		//}

		//bool bAddRed = false;
		//uint32 uBroadcastCount = 0;
		//else
		//{
		//	//保存起来 等客户端登陆完成再发送 保证发给具体用户的时候 用户能够收到
		//	bAddRed = CPlayerMgr::Instance().AddBroadcast(uid, broadCast, uBroadcastCount);
		//}
		//LOG_DEBUG("phpbroadcast - uid:%d,pPlayer:%p,bAddRed:%d,uBroadcastCount:%d,msg:%s", uid, pPlayer, bAddRed, uBroadcastCount, broadCast.c_str());
    }
	//else
	//{
	//	LOG_DEBUG("json analysis error - msg:%s", msg.c_str());
	//}
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
    return 0;
}
// PHP系统公告
int  CHandlePHPMsg::handle_php_sys_notice(NetworkObject* pNetObj, CBufferStream& stream)
{
    LOG_DEBUG("php系统公告");


    return 0;
}
// PHP踢出玩家
int  CHandlePHPMsg::handle_php_kill_player(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid"))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
        return 0;
    }
    if(!jvalue["uid"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    LOG_DEBUG("php踢出玩家:%d",uid);
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        NetworkObject* pSession = pPlayer->GetSession();
        if(pSession){
            pSession->DestroyObj();
        }
        pPlayer->SetNeedRecover(true);
    }
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
    return 0;
}
// PHP修改玩家数值
int  CHandlePHPMsg::handle_php_change_accvalue(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
	LOG_DEBUG("json analysis begin - msg:%s", msg.c_str());

	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ACCVALUE);

	return 0;

    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("diamond") || !jvalue.isMember("coin")
    || !jvalue.isMember("score") || !jvalue.isMember("ingot") || !jvalue.isMember("cvalue") || !jvalue.isMember("safecoin")
    || !jvalue.isMember("ptype") || !jvalue.isMember("sptype"))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["diamond"].isIntegral() || !jvalue["coin"].isIntegral()
    || !jvalue["score"].isIntegral() || !jvalue["ingot"].isIntegral() || !jvalue["cvalue"].isIntegral()
    || !jvalue["safecoin"].isIntegral() || !jvalue["ptype"].isIntegral() || !jvalue["sptype"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
        return 0;
    }
    uint32 uid       = jvalue["uid"].asUInt();
    uint32 oper_type = jvalue["ptype"].asUInt();
    uint32 sub_type  = jvalue["sptype"].asUInt();
    int64  diamond   = jvalue["diamond"].asInt64();
    int64  coin      = jvalue["coin"].asInt64();
    int64  score     = jvalue["score"].asInt64();
    int64  ingot     = jvalue["ingot"].asInt64();
    int64  cvalue    = jvalue["cvalue"].asInt64();
    int64  safecoin  = jvalue["safecoin"].asInt64();
    LOG_DEBUG("php_change_value uid:%d,oper_type:%d,sub_type:%d,diamond %lld- coin %lld- score %lld- ingot %lld-cvalue %lld,safecoin:%lld", uid, oper_type, sub_type,diamond,coin,score,ingot,cvalue,safecoin);
    bool bRet = false;
    int  code = 0;
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        bool needInLobby = true;
        if(diamond >=0 && coin >=0 && score >= 0 && ingot >= 0 && cvalue >=0 && safecoin >=0){
            needInLobby = false;
        }
        if(pPlayer->IsInLobby() || !needInLobby)
        {
            bRet = pPlayer->AtomChangeAccountValue(oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
            if(bRet){
                jrep["diamond"]     = pPlayer->GetAccountValue(emACC_VALUE_DIAMOND);
                jrep["coin"]        = pPlayer->GetAccountValue(emACC_VALUE_COIN);
                jrep["score"]       = pPlayer->GetAccountValue(emACC_VALUE_SCORE);
                jrep["ingot"]       = pPlayer->GetAccountValue(emACC_VALUE_INGOT);
                jrep["cvalue"]      = pPlayer->GetAccountValue(emACC_VALUE_CVALUE);
                jrep["safecoin"]    = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);

                pPlayer->UpdateAccValue2Client();
                pPlayer->FlushChangeAccData2GameSvr(diamond,coin,ingot,score,cvalue,safecoin);
				code = 0;
            }else{
                code=1;
            }
        }else{
            code=2;
        }
    }else{
        bRet = CCommonLogic::AtomChangeOfflineAccData(uid,oper_type,sub_type,diamond,coin,ingot,score,cvalue,safecoin);
        if(!bRet)code=3;
    }
    jrep["ret"]     = bRet ? 1 : 0;
    jrep["code"]    = code;

    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
    return 0;
}
// 修改玩家名字
int   CHandlePHPMsg::handle_php_change_name(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue["uid"].isIntegral())
    {
        LOG_ERROR("解析连续登陆json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer == NULL){
       jrep["ret"] = 1;
       SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
       return 0;  
    }
    if(jvalue.isMember("name") && jvalue["name"].isString()){
        string name = jvalue["name"].asString();
        pPlayer->SetPlayerName(name);
    }    
    if(jvalue.isMember("sex") && jvalue["sex"].isIntegral()){
        uint32 sex = jvalue["sex"].asUInt();
        pPlayer->SetSex(sex);        
    }          
    pPlayer->NotifyChangePlayerInfo2GameSvr();
    
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
    return 0;
}
// 停服
int  CHandlePHPMsg::handle_php_stop_service(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;

	LOG_ERROR("json analysis start - msg:%s", msg.c_str());

    if(!reader.parse(msg,jvalue) || !jvalue.isMember("svrid") || !jvalue.isMember("content") || !jvalue.isMember("btime") || !jvalue.isMember("etime"))
    {
		LOG_ERROR("json analysis error 1 - msg:%s", msg.c_str());
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
        return 0;
    }
    if(!jvalue["svrid"].isArray() || !jvalue["content"].isString() || !jvalue["btime"].isIntegral() || !jvalue["etime"].isIntegral())
    {
		LOG_ERROR("json analysis error 2 - msg:%s", msg.c_str());
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    string content = jvalue["content"].asString();
    uint32 bTime   = jvalue["btime"].asUInt();
    uint32 eTime   = jvalue["etime"].asUInt();
    vector<uint16> svrids;
    for(uint32 i=0;i<jvalue["svrid"].size();++i)
	{
        if(!jvalue["svrid"][i].isIntegral())
		{
            LOG_DEBUG("svrid 不是数字");
            continue;
        }
        uint16 svrid = jvalue["svrid"][i].asUInt();
        svrids.push_back(svrid);
        LOG_DEBUG("停服服务器ID:%d",svrid);
    }
    CServerMgr::Instance().NotifyStopService(bTime,eTime,svrids,content);

    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
    return 0;
}
// 更改机器人配置
int   CHandlePHPMsg::handle_php_change_robot(NetworkObject* pNetObj,CBufferStream& stream)
{
    CGobalRobotMgr::Instance().ChangeAllRobotCfg();

    Json::Value  jrep;
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ROBOT);
    return 0;
}
// 更改VIP
int   CHandlePHPMsg::handle_php_change_vip(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("vip"))
    {
        LOG_ERROR("解析连续登陆json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["vip"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();    
    uint32 vip = jvalue["vip"].asUInt();
    LOG_DEBUG("php change vip:%d-%d",uid,vip);
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL){        
        pPlayer->SetVip(vip);        
    }
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);

	return 0;
}
// 更改房间param
int   CHandlePHPMsg::handle_php_change_room_param(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("param"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["param"].isString())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	string param = jvalue["param"].asString();
	LOG_DEBUG("php room  param - gametype:%d,roomid:%d,param:%s", gametype, roomid, param.c_str());
	CServerMgr::Instance().ChangeRoomParam(gametype, roomid, param.c_str());

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);

	return 0;
}

// 控制玩家
int   CHandlePHPMsg::handle_php_control_player(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("uid") || !jvalue.isMember("operatetype") || !jvalue.isMember("gamecount") || !jvalue.isMember("control_log_id"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["uid"].isIntegral() || !jvalue["operatetype"].isIntegral() || !jvalue["gamecount"].isIntegral() || !jvalue["control_log_id"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);
		return 0;
	}
	int64 id = jvalue["control_log_id"].asInt64();
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	uint32 uid = jvalue["uid"].asUInt();
	uint32 operatetype = jvalue["operatetype"].asUInt();
	uint32 gamecount = jvalue["gamecount"].asUInt();
	
	LOG_DEBUG("control player data - id:%lld,gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d", id, gametype, roomid, uid, operatetype, gamecount);
	CServerMgr::Instance().ChangeContorlPlayer(id,gametype, roomid, uid, operatetype, gamecount);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);

	return 0;
}

// 更新游戏内金币
int   CHandlePHPMsg::handle_php_update_accvalue_ingame(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("diamond") || !jvalue.isMember("coin")
		|| !jvalue.isMember("score") || !jvalue.isMember("ingot") || !jvalue.isMember("cvalue") || !jvalue.isMember("safecoin")
		|| !jvalue.isMember("ptype") || !jvalue.isMember("sptype"))
	{
		LOG_ERROR("analysis json error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
		return 0;
	}
	if (!jvalue["uid"].isIntegral() || !jvalue["diamond"].isIntegral() || !jvalue["coin"].isIntegral()
		|| !jvalue["score"].isIntegral() || !jvalue["ingot"].isIntegral() || !jvalue["cvalue"].isIntegral()
		|| !jvalue["safecoin"].isIntegral() || !jvalue["ptype"].isIntegral() || !jvalue["sptype"].isIntegral())
	{
		LOG_ERROR("json param error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
		return 0;
	}
	uint32 uid = jvalue["uid"].asUInt();
	uint32 oper_type = jvalue["ptype"].asUInt();
	uint32 sub_type = jvalue["sptype"].asUInt();
	int64  diamond = jvalue["diamond"].asInt64();
	int64  coin = jvalue["coin"].asInt64();
	int64  score = jvalue["score"].asInt64();
	int64  ingot = jvalue["ingot"].asInt64();
	int64  cvalue = jvalue["cvalue"].asInt64();
	int64  safecoin = jvalue["safecoin"].asInt64();
	LOG_DEBUG("1 update_accvalue_ingame data - uid:%d,oper_type:%d,sub_type:%d,diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld", uid, oper_type, sub_type, diamond, coin, score, ingot, cvalue, safecoin);
	bool bRet = false;
	int  code = 0;
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);

	LOG_DEBUG("2 update_accvalue_ingame status - uid:%d,oper_type:%d,pPlayer:%p", uid, oper_type, pPlayer);

	if (pPlayer != NULL)
	{
		if (oper_type == emACCTRAN_OPER_TYPE_BUY)
		{
			pPlayer->AddRecharge(coin);
			pPlayer->SendVipBroadCast();
			pPlayer->SendVipProxyRecharge();
			pPlayer->SetIsPay(1);
		}
		//bool needInLobby = true;
		//if (diamond >= 0 && coin >= 0 && score >= 0 && ingot >= 0 && cvalue >= 0 && safecoin >= 0) {
		//	needInLobby = false;
		//}
		bool needInLobby = false;
		if (coin < 0 || score < 0) {
			needInLobby = true;
		}
		LOG_DEBUG("3 update_accvalue_ingame status - uid:%d,oper_type:%d,IsInLobby:%d,needInLobby:%d,curSvrID:%d", uid, oper_type, pPlayer->IsInLobby(), needInLobby, pPlayer->GetCurSvrID());
		if (pPlayer->IsInLobby() || !needInLobby)
		{
			bRet = pPlayer->PhpAtomChangeAccountValue(oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
			LOG_DEBUG("4 update_accvalue_ingame status - uid:%d,oper_type:%d,bRet:%d", uid, oper_type, bRet);

			if (bRet) {
				jrep["diamond"] = pPlayer->GetAccountValue(emACC_VALUE_DIAMOND);
				jrep["coin"] = pPlayer->GetAccountValue(emACC_VALUE_COIN);
				jrep["score"] = pPlayer->GetAccountValue(emACC_VALUE_SCORE);
				jrep["ingot"] = pPlayer->GetAccountValue(emACC_VALUE_INGOT);
				jrep["cvalue"] = pPlayer->GetAccountValue(emACC_VALUE_CVALUE);
				jrep["safecoin"] = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);

				pPlayer->UpdateAccValue2Client();
				uint32 temp_sub_type = 0;
				stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
				if (pServer != NULL)
				{
					temp_sub_type = pServer->gameType;
				}
				pPlayer->UpDateChangeAccData2GameSvr(diamond, coin, ingot, score, cvalue, safecoin, oper_type, temp_sub_type);

				code = 0;
			}
			else {
				code = 1;
			}
		}
		else {
			code = 2;
		}
	}
	else
	{
		
		bRet = CCommonLogic::AtomChangeOfflineAccData(uid, oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
		LOG_DEBUG("5 update_accvalue_ingame status player is null - uid:%d,oper_type:%d,bRet:%d", uid, oper_type, bRet);
		if (!bRet)
		{
			code = 3;
		}
		//else
		//{
		//	code = 4;
		//}
	}
	jrep["ret"] = bRet ? 1 : 0;
	jrep["code"] = code;

	LOG_DEBUG("6 update_accvalue_ingame status - uid:%d,oper_type:%d,bRet:%d,code:%d", uid, oper_type, bRet, code);

	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
	return 0;
}

int CHandlePHPMsg::handle_php_stop_snatch_coin(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("stop"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["stop"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	uint32 stop = jvalue["stop"].asUInt();
	LOG_DEBUG("php room  param - gametype:%d,roomid:%d,stop:%d", gametype, roomid, stop);
	CServerMgr::Instance().StopSnatchCoin(gametype, roomid, stop);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);

	return 0;
}


int CHandlePHPMsg::handle_php_change_vip_broadcast(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("viphorn"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);
		return 0;
	}
	if (!jvalue["viphorn"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);
		return 0;
	}
	Json::Value  jvalueBroadCast = jvalue["viphorn"];
	string strBroadCast = jvalueBroadCast.toFastString();

	LOG_DEBUG("php strBroadCast  param - strBroadCast:%s", strBroadCast.c_str());

	CDataCfgMgr::Instance().VipBroadCastAnalysis(true,strBroadCast);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);

	return 0;
}

int CHandlePHPMsg::handle_php_robot_snatch_coin(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("snatchtype") || !jvalue.isMember("robotcount") || !jvalue.isMember("cardcount"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ROBOT_SNATCH_COIN);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["snatchtype"].isIntegral() || !jvalue["robotcount"].isIntegral() || !jvalue["cardcount"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ROBOT_SNATCH_COIN);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	uint32 snatchtype = jvalue["snatchtype"].asUInt();
	uint32 robotcount = jvalue["robotcount"].asUInt();
	uint32 cardcount = jvalue["cardcount"].asUInt();
	LOG_DEBUG("php room  param - gametype:%d,roomid:%d,snatchtype:%d,robotcount:%d,cardcount:%d", gametype, roomid, snatchtype, robotcount, cardcount);
	CServerMgr::Instance().RobotSnatchCoin(gametype, roomid, snatchtype, robotcount, cardcount);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ROBOT_SNATCH_COIN);

	return 0;
}

/*
{
	"vipproxyrecharge": {
		"data": {
			"action": "vip_wechat",
			"wcinfo": [{
				"wx_account": "weixin001",
				"wx_title": "\u6d4b\u8bd5\u6807\u9898001001001001001001",
				"sort_id": 1,
				"showtime": "",
				"vip": "[1,3,8,9,12,0]",
				"low_amount": 80
			}, {
				"wx_account": "weixin004",
				"wx_title": "\u5fae\u4fe1\u6807\u9898004",
				"sort_id": 10,
				"showtime": "",
				"vip": "[10,12,13,14,4,2,3]",
				"low_amount": 50
			}]
		}
	}
}

{"vipalipayrecharge":{"data":{"action":"vip_proxy","recharge":0,"status":0}}}

	"vipalipayrecharge": {
		"data": {
			"action": "vip_aliacc",
			"wcinfo": [{
				"wx_account": "weixin001",
				"wx_title": "\u6d4b\u8bd5\u6807\u9898001001001001001001",
				"sort_id": 1,
				"showtime": "",
				"vip": "[1,3,8,9,12,0]",
				"low_amount": 80
			}, {
				"wx_account": "weixin004",
				"wx_title": "\u5fae\u4fe1\u6807\u9898004",
				"sort_id": 10,
				"showtime": "",
				"vip": "[10,12,13,14,4,2,3]",
				"low_amount": 50
			}]
		}
	}
}

{"vipalipayrecharge":{"data":{"action":"vipalipay","recharge":0,"status":0}}}


*/


int CHandlePHPMsg::handle_php_notify_vip_proxy_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("vipproxyrecharge") && jvalue["vipproxyrecharge"].isObject()) {
		Json::Value  jvalueVipProxyRecharge = jvalue["vipproxyrecharge"];
		string strjvalueVipProxyRecharge = jvalueVipProxyRecharge.toFastString();

		LOG_DEBUG("php strjvalueVipProxyRecharge  param - strjvalueVipProxyRecharge:%s", strjvalueVipProxyRecharge.c_str());

		CDataCfgMgr::Instance().VipProxyRechargeAnalysis(true, strjvalueVipProxyRecharge);
		jrep["ret"] = 1;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
		return 0;
	}

	if (reader.parse(msg, jvalue) && jvalue.isMember("vipalipayrecharge") && jvalue["vipalipayrecharge"].isObject()) {
		Json::Value  jvalueVipRecharge = jvalue["vipalipayrecharge"];
		string strjvalueVipRecharge = jvalueVipRecharge.toFastString();

		LOG_DEBUG("php strjvalueVipRecharge param - str:%s", strjvalueVipRecharge.c_str());

		CDataCfgMgr::Instance().VipAliAccRechargeAnalysis(true, strjvalueVipRecharge);
		jrep["ret"] = 1;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
		return 0;
	}

	if (reader.parse(msg, jvalue) && jvalue.isMember("vipshanfurecharge") && jvalue["vipshanfurecharge"].isObject()) {
		Json::Value  jvalueVipRecharge = jvalue["vipshanfurecharge"];
		string strjvalueVipRecharge = jvalueVipRecharge.toFastString();

		LOG_DEBUG("php strjvalueVipRecharge param - str:%s", strjvalueVipRecharge.c_str());

		CDataCfgMgr::Instance().ExclusiveFlashRechargeAnalysis(true, strjvalueVipRecharge);
		jrep["ret"] = 1;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
		return 0;
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
	return 0;
}

int CHandlePHPMsg::handle_php_notify_union_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("unionpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["unionpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueUnionpayRecharge = jvalue["unionpayrecharge"];
	string strjvalueUnionpayRecharge = jvalueUnionpayRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueUnionpayRecharge:%s", strjvalueUnionpayRecharge.c_str());

	CDataCfgMgr::Instance().UnionPayRechargeAnalysis(true, strjvalueUnionpayRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_wechat_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("wechatpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["wechatpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["wechatpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().WeChatPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_ali_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("alipayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["alipayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["alipayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().AliPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_other_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("otherpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["otherpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["otherpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().OtherPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_qq_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("qqpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["qqpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["qqpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().QQPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_wechat_scan_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("wcscanpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["wcscanpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["wcscanpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().WeChatScanPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_jd_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("jdpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["jdpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["jdpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().JDPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_apple_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("applepayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["applepayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["applepayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().ApplePayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);

	return 0;
}

int CHandlePHPMsg::handle_php_notify_large_ali_pay_recharge(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("bigalipayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["bigalipayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["bigalipayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().LargeAliPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);

	return 0;
}

// 控制玩家
int   CHandlePHPMsg::handle_php_control_multi_player(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	LOG_ERROR("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("uid") || !jvalue.isMember("operatetype") || !jvalue.isMember("gamecount") || !jvalue.isMember("gametime") || !jvalue.isMember("totalscore"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["uid"].isArray() || !jvalue["operatetype"].isIntegral() || !jvalue["gamecount"].isIntegral() || !jvalue["gametime"].isIntegral() || !jvalue["totalscore"].isIntegral())
	{
		//LOG_ERROR("json param analysis error - uid:%d, msg:%s", jvalue["uid"].isObject(),msg.c_str());
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	//uint32 uid = jvalue["uid"].asUInt();
	uint32 operatetype = jvalue["operatetype"].asUInt();
	uint32 gamecount = jvalue["gamecount"].asUInt();
	uint64 gametime = jvalue["gametime"].asUInt64();
	int64 totalscore = jvalue["totalscore"].asInt64();
	
	Json::Value  juids = jvalue["uid"];
	LOG_ERROR("json analysis - juids_size:%d", juids.size());

	if (juids.size() > 0)
	{
		for (uint32 i = 0; i < juids.size(); i++)
		{
			Json::Value jLogUid = juids[i];
			LOG_ERROR("json analysis - juids_size:%d,i:%d,isObject:%d", juids.size(),i, jLogUid.isObject());
			if (!jLogUid.isObject())
			{
				continue;
			}
			if (!jLogUid.isMember("uid") || !jLogUid.isMember("logid"))
			{
				continue;
			}
			if (!jLogUid["uid"].isIntegral() || !jLogUid["logid"].isIntegral())
			{
				continue;
			}
			uint32 uid = jLogUid["uid"].asUInt();
			int64 id = jLogUid["logid"].asInt64();
			
			LOG_DEBUG("control player data - gametype:%d,roomid:%d,id:%d,uid:%d,operatetype:%d,gamecount:%d,gametime:%lld,totalscore:%lld", gametype, roomid,id, uid, operatetype, gamecount, gametime, totalscore);

			CServerMgr::Instance().ChangeContorlMultiPlayer(id,gametype, roomid, uid, operatetype, gamecount, gametime, totalscore);
		}
	}


	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);

	return 0;
}

int   CHandlePHPMsg::handle_php_control_dice_game_card(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("dice"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["dice"].isArray())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	Json::Value  jdice = jvalue["dice"];
	LOG_DEBUG("json analysis - gametype:%d,roomid:%d,jdice:%d", gametype, roomid,jdice.size());

	uint32 uDice[3] = { 0 };
	bool bIsSendDiceServer = true;
	if (jdice.size() == 3)
	{
		for (uint32 i = 0; i < jdice.size(); i++)
		{
			LOG_DEBUG("json analysis - jdice_size:%d,i:%d,isUint:%d", jdice.size(), i, jdice[i].isIntegral());
			if (!jdice[i].isIntegral())
			{
				continue;
			}
			uDice[i] = jdice[i].asUInt();
			if (uDice[i] <= 0 || uDice[i] >= 7)
			{
				bIsSendDiceServer = false;
				break;
			}
		}
	}
	else
	{
		bIsSendDiceServer = false;
	}
	if (bIsSendDiceServer)
	{
		CServerMgr::Instance().ContorlDiceGameCard(gametype, roomid, uDice);
	}


	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);

	return 0;
}

//PHP_MSG_ONLINE_CONFIG_ROBOT = 37;
// 添加总账号
//{"action":"update_acount","gametype" : 13}
// 修改游戏机器配置
//{"action":"update_game",  "gametype" : 13, "svrid" : 131, "robot" : 1}
// 修改游戏房间机器配置
//{"action":"update_room",  "gametype" : 13, "roomid" : 1,  "robot" : 1}
// 更新机器全天在线数量配置
//{"action":"update_count", "gametype" : 13, "roomid" : 1,  "leveltype" : 1, "id" : 1, "online" : [0, 0, 0, 20, 15, 12, 4, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0]}
// 更新机器按时段上线配置
//{"action":"update_batch", "gametype" : 13, "roomid" : 1,  "leveltype" : 1, "id" : 1, "logintype" : 1,"entertimer" : 1,"leavetimer" : 3600,"online" : [0, 0, 0, 20, 15, 12, 4, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0]}
// 删除机器人批次
//{"action":"delete_bacthid",  "gametype" : 13, "id" : 1}

int CHandlePHPMsg::handle_php_online_config_robot(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("action") || !jvalue["action"].isString())
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
		return 0;
	}
	string straction = jvalue["action"].asString();
	if (straction == "update_acount")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			CGobalRobotMgr::Instance().AddRobotPayPoolData(gametype);
		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}
	else if (straction == "update_game")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("svrid") && jvalue["svrid"].isIntegral() && 
			jvalue.isMember("robot") && jvalue["robot"].isIntegral() )
		{
			int gametype = jvalue["gametype"].asInt();
			int svrid = jvalue["svrid"].asInt();
			int robot = jvalue["robot"].asInt();

			LOG_DEBUG("gametype:%d,svrid:%d, robot:%d", gametype, svrid, robot);

			CServerMgr::Instance().UpdateOpenRobot(svrid, robot);
		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}
	else if (straction == "update_room")
	{
		if (jvalue.isMember("robot") && jvalue["robot"].isIntegral() &&
			jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("roomid") && jvalue["roomid"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			//int svrid = jvalue["svrid"].asInt();
			int roomid = jvalue["roomid"].asInt();
			int robot = jvalue["robot"].asInt();

			LOG_DEBUG("gametype:%d,roomid:%d, robot:%d", gametype, roomid, robot);

			CServerMgr::Instance().UpdateServerRoomRobot(gametype, roomid, robot);
		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}
	else if (straction == "update_count")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("roomid") && jvalue["roomid"].isIntegral() &&
			jvalue.isMember("leveltype") && jvalue["leveltype"].isIntegral() &&
			jvalue.isMember("id") && jvalue["id"].isIntegral() && 
			jvalue.isMember("online") && jvalue["online"].isArray())
		{
			int gametype = jvalue["gametype"].asInt();
			int roomid = jvalue["roomid"].asInt();
			int leveltype = jvalue["leveltype"].asInt();
			int batchid = jvalue["id"].asInt();
			Json::Value jonline = jvalue["online"];

			int logintype = 0;
			int entertimer = 0;
			int leavetimer = 0;

			if (ROBOT_MAX_LEVEL != jonline.size())
			{
				LOG_ERROR("json analysis error - msg:%s", msg.c_str());
				jrep["ret"] = 0;
				SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
				return 0;
			}

			vector<int> vecOnline;
			for (uint32 i = 0; i < jonline.size(); i++)
			{
				LOG_DEBUG("json analysis - jonline:%d,i:%d,isIntegral:%d", jonline.size(), i, jonline[i].isIntegral());
				if (!jonline[i].isIntegral())
				{
					continue;
				}
				int count = jonline[i].asInt();
				if (count>=0)
				{
					vecOnline.push_back(count);
				}
			}
			CDataCfgMgr::Instance().UpdateRobotLvOnline(gametype, roomid, leveltype, batchid, vecOnline);
			int optype = ONLINE_ROBOT_BATCH_ID_ADD;
			CServerMgr::Instance().ReloadRobotOnlineCfg(optype, gametype, roomid, leveltype, batchid, logintype, entertimer, leavetimer, vecOnline);

		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}	
	else if (straction == "update_batch")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral()
			&& jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() 
			&& jvalue.isMember("roomid") && jvalue["roomid"].isIntegral()
			&& jvalue.isMember("leveltype") && jvalue["leveltype"].isIntegral()
			&& jvalue.isMember("id") && jvalue["id"].isIntegral()
			&& jvalue.isMember("logintype") && jvalue["logintype"].isIntegral()
			&& jvalue.isMember("entertimer") && jvalue["entertimer"].isIntegral()
			&& jvalue.isMember("leavetimer") && jvalue["leavetimer"].isIntegral()
			&& jvalue.isMember("online") && jvalue["online"].isArray() )
		{
			int gametype = jvalue["gametype"].asInt();
			int roomid = jvalue["roomid"].asInt();
			int leveltype = jvalue["leveltype"].asInt();
			int batchid = jvalue["id"].asInt();
			int logintype = jvalue["logintype"].asInt();
			int entertimer = jvalue["entertimer"].asInt();
			int leavetimer = jvalue["leavetimer"].asInt();
			Json::Value jonline = jvalue["online"];

			if (entertimer > DAY_MAX_SECONDS || leavetimer > DAY_MAX_SECONDS)
			{
				LOG_ERROR("json analysis error - msg:%s", msg.c_str());
				jrep["ret"] = 0;
				SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
				return 0;
			}

			if (ROBOT_MAX_LEVEL != jonline.size())
			{
				LOG_ERROR("json analysis error - msg:%s", msg.c_str());
				jrep["ret"] = 0;
				SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
				return 0;
			}

			vector<int> vecOnline;
			for (uint32 i = 0; i < jonline.size(); i++)
			{
				LOG_DEBUG("json analysis - jonline:%d,i:%d,isIntegral:%d", jonline.size(), i, jonline[i].isIntegral());
				if (!jonline[i].isIntegral())
				{
					continue;
				}
				int count = jonline[i].asInt();
				if (count>=0)
				{
					vecOnline.push_back(count);
				}
			}

			CDataCfgMgr::Instance().UpdateTimeRobotOnline(gametype,roomid, leveltype, batchid, logintype, entertimer, leavetimer, vecOnline);

			int optype = ONLINE_ROBOT_BATCH_ID_ADD;
			CServerMgr::Instance().ReloadRobotOnlineCfg(optype, gametype, roomid, leveltype, batchid, logintype, entertimer, leavetimer, vecOnline);
		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}
	else if (straction == "delete_bacthid")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral()
			&& jvalue.isMember("id") && jvalue["id"].isIntegral())
		{
			int optype = ONLINE_ROBOT_BATCH_ID_DEL;
			int gametype = jvalue["gametype"].asInt();
			int batchid = jvalue["id"].asInt();
			
			int roomid = 0;
			int leveltype = 0;
			int logintype = 0;
			int entertimer = 0;
			int leavetimer = 0;
			vector<int> vecOnline;
			CDataCfgMgr::Instance().DeleteRobotOnlineByBatchID(batchid);
			CServerMgr::Instance().ReloadRobotOnlineCfg(optype, gametype, roomid, leveltype, batchid, logintype, entertimer, leavetimer, vecOnline);

		}
		else
		{
			LOG_ERROR("json analysis error - msg:%s", msg.c_str());
			jrep["ret"] = 0;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
			return 0;
		}
	}
	else
	{
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
		return 0;
	}

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);

	return 0;
}

// {"gametype":12, "roomid" : 1,"handcard":[[0x37,0x05],[0x37,0x37]]}
int CHandlePHPMsg::handle_php_config_mahiang_card(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}

	if (!jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("handcard"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["handcard"].isArray())
	{
		LOG_ERROR("json param analysis error - msg:%s",msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}

	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	string strHandCard = jvalue["handcard"].toFastString();
	LOG_DEBUG("json analysis - gametype:%d,roomid:%d,strHandCard:%s", gametype, roomid, strHandCard.c_str());
	CServerMgr::Instance().ConfigMajiangHandCard(gametype, roomid, strHandCard);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);

	return 0;
}

// 刷新自动杀分配置
int CHandlePHPMsg::handle_php_refresh_auto_kill_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_CFG);
		return 0;
	}

	// 刷新自动杀分配置 
	//bool b_ret = CDataCfgMgr::Instance().refreshAutoKillCfg(msg);
	//if (!b_ret)
	//{
	//	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	//	jrep["ret"] = 0;
	//	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_CFG);
	//}

	// 通知其他服务器刷新配置 
	CServerMgr::Instance().UpdateServerAutoKillCfg(msg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_CFG);

	return 0;
}

// 刷新自动杀分玩家列表
int CHandlePHPMsg::handle_php_refresh_auto_kill_users(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_CFG);
		return 0;
	}

	// 刷新自动杀分玩家列表
	//bool b_ret = CDataCfgMgr::Instance().refreshAutoKillUsers(msg);
	//if (!b_ret)
	//{
	//	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	//	jrep["ret"] = 0;
	//	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_USERS);
	//}

	// 通知其他服务器刷新配置 
	CServerMgr::Instance().UpdateServerAutoKillUsers(msg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_REFRESH_AUTO_KILL_USERS);

	return 0;
}



// {"uid":12, "userright" : 3,"posrmb":50,"postime":1544689978}
int CHandlePHPMsg::handle_php_update_new_player_novice_welfare_right(NetworkObject* pNetObj, CBufferStream& stream)
{

	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT);
		return 0;
	}

	if (!jvalue.isMember("uid") || !jvalue.isMember("userright") || !jvalue.isMember("posrmb") || !jvalue.isMember("postime"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT);
		return 0;
	}
	if (!jvalue["uid"].isIntegral() || !jvalue["userright"].isIntegral() || !jvalue["posrmb"].isIntegral() || !jvalue["postime"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT);
		return 0;
	}

	uint32 uid = jvalue["uid"].asUInt();
	uint32 userright = jvalue["userright"].asUInt();
	uint32 posrmb = jvalue["posrmb"].asUInt();
	uint64 postime = jvalue["postime"].asUInt64();
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);

	LOG_DEBUG("update_welfare_right - uid:%d,userright:%d,posrmb:%d,postime:%lld,pPlayer:%p", uid, userright, posrmb, postime, pPlayer);

	if (pPlayer != NULL)
	{
		pPlayer->SetNoviceWelfare();
		pPlayer->SetPosRmb(posrmb);
		pPlayer->SetPosTime(postime);
	}
	CServerMgr::Instance().UpdateNewPlayerNoviceWelfareRight(uid, userright, posrmb, postime);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_RIGHT);
	return 0;
}

// {"id":1,"minpayrmb":100,"maxpayrmb":200,"maxwinscore":10000, "welfarecount":30, "welfarepro":500,"postime":7, "lift_odds":1000}
int CHandlePHPMsg::handle_php_update_new_player_novice_welfare_value(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE);
		return 0;
	}

	if (!jvalue.isMember("id") || !jvalue.isMember("minpayrmb") || !jvalue.isMember("maxpayrmb") || !jvalue.isMember("maxwinscore") || !jvalue.isMember("welfarecount") || !jvalue.isMember("welfarepro") || !jvalue.isMember("postime") || !jvalue.isMember("lift_odds"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE);
		return 0;
	}
	if (!jvalue["id"].isIntegral() || !jvalue["minpayrmb"].isIntegral() || !jvalue["maxpayrmb"].isIntegral() || !jvalue["maxwinscore"].isIntegral() || !jvalue["welfarecount"].isIntegral() || !jvalue["welfarepro"].isIntegral() || !jvalue["postime"].isIntegral() || !jvalue["lift_odds"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE);
		return 0;
	}

	tagNewPlayerWelfareValue data;

	data.id = jvalue["id"].asUInt();
	data.minpayrmb = jvalue["minpayrmb"].asUInt();
	data.maxpayrmb = jvalue["maxpayrmb"].asUInt();
	data.maxwinscore = jvalue["maxwinscore"].asUInt();
	data.welfarecount = jvalue["welfarecount"].asUInt();
	data.welfarepro = jvalue["welfarepro"].asUInt();
	data.postime = jvalue["postime"].asUInt();
	data.lift_odds = jvalue["lift_odds"].asUInt();

	CDataCfgMgr::Instance().SetNewPlayerWelfareValue(data);


	CServerMgr::Instance().UpdateNewPlayerNoviceWelfareValue(data);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UPDATE_NEW_PLAYER_NOVICE_WELFARE_VALUE);


	return 0;
}

//{"cloginreward":"[100,100,100,100,100,100,200]","reset":0}
int CHandlePHPMsg::handle_php_sign_in_update(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());
	return 0;
	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_SIGN_IN_UPDATE);
		return 0;
	}

	if (!jvalue.isMember("cloginreward") || !jvalue.isMember("reset") )
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_SIGN_IN_UPDATE);
		return 0;
	}
	if (!jvalue["reset"].isString() || !jvalue["reset"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_SIGN_IN_UPDATE);
		return 0;
	}
	std::string strJson = jvalue["cloginreward"].asString();
	int reset = jvalue["reset"].asInt();
	bool bIsSuccess = CDataCfgMgr::Instance().cloginrewardAnalysis(true, strJson);

	LOG_DEBUG("reset:%d,strJson:%s", reset, strJson.data());
	if (bIsSuccess)
	{
		if (reset == 1)
		{
			CDBMysqlMgr::Instance().UpdatePlayerClogin();
			CRedisMgr::Instance().ClearSignInDev();
			CPlayerMgr::Instance().CloginCleanup();
		}
		jrep["ret"] = 1;
	}
	else
	{
		jrep["ret"] = 0;
	}
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_SIGN_IN_UPDATE);


	return 0;
}

int CHandlePHPMsg::handle_php_active_welfare_cfg_update(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;

    LOG_DEBUG("json analysis - msg: handle_php_active_welfare_cfg_update");

    // 通知其他服务器刷新配置 
    CServerMgr::Instance().UpdateServerActiveWelfareCfg(msg);

    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ACTIVE_WELFARE_CFG_UPDATE);
    
    return 0;
}

void  CHandlePHPMsg::SendPHPMsg(NetworkObject* pNetObj,string jmsg,uint16 cmd)
{
    CBufferStream& sendStream = sendStream.buildStream();
    uint16 len = PHP_HEAD_LEN + jmsg.length();
    sendStream.write_(len);
    sendStream.write_(cmd);
    sendStream.writeString(jmsg);

    pNetObj->Send(sendStream.getBuffer(),sendStream.getPosition());
    //LOG_DEBUG("回复php消息:%d--%d",cmd,sendStream.getPosition());
}

int CHandlePHPMsg::handle_php_active_welfare_cfg_clear(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;

    LOG_DEBUG("json analysis - msg: handle_php_active_welfare_cfg_clear");

    // 清除redis中所有活跃福利的数据 
    CRedisMgr::Instance().ClearAllPlayerActiveWelfareInfo();

	// 通知其他服务器刷新配置 
	CServerMgr::Instance().ResetPlayerActiveWelfareInfo(msg);

    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ACTIVE_WELFARE_INFO_CLEAR);

    return 0;
}

int CHandlePHPMsg::handle_php_new_register_welfare_cfg_update(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg: handle_php_new_register_welfare_cfg_update");

	// 通知其他服务器刷新配置 
	CServerMgr::Instance().UpdateServerNewRegisterWelfareCfg(msg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_NEW_REGISTER_WELFARE_CFG_UPDATE);

	return 0;
}

int  CHandlePHPMsg::handle_php_change_user_ctrl_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
		return 0;
	}

	if (!jvalue.isMember("opertype") || !jvalue.isMember("suid"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
		return 0;
	}
	uint32 opertype = jvalue["opertype"].asUInt();
	uint32 suid = 0;
	set<uint32> set_suid;

	//如果为删除类型，则需要按数组类型解析
	if (opertype == 3)
	{
		string suid_tmp = jvalue["suid"].toFastString();

		//解析用户ID
		if (suid_tmp.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(suid_tmp, jvalue))
			{
				LOG_ERROR("json param analysis error - msg:%s", suid_tmp.c_str());
				return 0;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				set_suid.insert(jvalue[i].asUInt());
			}
		}
	}
	else
	{
		suid = jvalue["suid"].asUInt();
	}
	tagUserControlCfg info;
	info.suid = suid;

	if (jvalue.isMember("sdeviceid"))
	{
		info.sdeviceid = jvalue["sdeviceid"].asString();
	}

	if (jvalue.isMember("skey"))
	{
		info.skey = jvalue["skey"].asString();
	}

	if (jvalue.isMember("tuid"))
	{
		info.tuid = jvalue["tuid"].asUInt();
	}
	string cgid_tmp;
	if (jvalue.isMember("cgid"))
	{
		cgid_tmp = jvalue["cgid"].toFastString();
		//解析游戏ID
		if (cgid_tmp.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(cgid_tmp, jvalue))
			{
				LOG_ERROR("json param analysis error - msg:%s", cgid_tmp.c_str());
				jrep["ret"] = 0;
				SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
				return 0;
			}
			LOG_DEBUG("cgid size:%d", jvalue.size());
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				info.cgid.insert(jvalue[i].asUInt());
				LOG_DEBUG("value:%d info.cgid size:%d", jvalue[i].asUInt(), info.cgid.size());
			}
		}
	}
	LOG_DEBUG("opertype:%d suid:%d tuid:%d cgid:%s skey:%s sdeviceid:%s", opertype, suid, info.tuid, cgid_tmp.c_str(), info.skey.c_str(), info.sdeviceid.c_str());
	
	//对于修改和删除类型，需要在线的控制玩家的行为	
	map<uint32, tagUserControlCfg> mpCfgInfo;
	CDataCfgMgr::Instance().GetUserControlCfg(mpCfgInfo);

	bool isNoticeFlag = false;  //是否通知玩家返回大厅，由于配置发生变化
	uint16 curSvrID = 0;		//当前玩家所在的SVRID

	//修改控制玩家配置
	if (opertype == 2)
	{
		//判断配置是否存在
		auto iter_player = mpCfgInfo.find(suid);
		if (iter_player != mpCfgInfo.end())
		{
			//如果当前玩家处于某个游戏中时，需要判断当前控制游戏是否发生改变
			info.skey = iter_player->second.skey;
			CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(suid);
			if (pPlayer != NULL && pPlayer->GetCurSvrID() != 0)
			{
				//设备码是否发生改变
				if (!isNoticeFlag && info.sdeviceid != iter_player->second.sdeviceid)
				{
					isNoticeFlag = true;
				}

				//tuid值是否发生改变
				if (!isNoticeFlag && info.tuid != iter_player->second.tuid)
				{
					isNoticeFlag = true;
				}
				stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
				if (!isNoticeFlag && pServer != NULL)
				{
					//判断所在游戏ID是否在新的游戏配置列表中
					auto iter_gametype = info.cgid.find(pServer->gameType);
					if (iter_gametype == info.cgid.end())
					{
						isNoticeFlag = true;
					}
				}
				curSvrID = pPlayer->GetCurSvrID();

				//如果当前控制玩家正在某个游戏中，则需要中断控制
				if (isNoticeFlag && curSvrID != 0)
				{
					CServerMgr::Instance().StopContrlPlayer(curSvrID, suid);
				}
			}
		}
	}

	//删除控制玩家配置
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{
			auto iter_player = mpCfgInfo.find(suid_tmp);
			if (iter_player != mpCfgInfo.end())
			{
				//如果当前玩家处于某个游戏中时，需要判断当前控制游戏是否发生改变
				CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(suid_tmp);
				if (pPlayer != NULL && pPlayer->GetCurSvrID() != 0)
				{
					isNoticeFlag = true;
					curSvrID = pPlayer->GetCurSvrID();
					CServerMgr::Instance().StopContrlPlayer(curSvrID, suid_tmp);
				}
			}
		}
	}

	//更新配置信息
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{
			CDataCfgMgr::Instance().UpdateUserControlInfo(suid_tmp, opertype, info);
		}
	}
	else
	{
		CDataCfgMgr::Instance().UpdateUserControlInfo(suid, opertype, info);
	}

	msg_syn_ctrl_user_cfg synMsg;
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{
			synMsg.add_vec_suid(suid_tmp);

		}
	}
	else
	{
		synMsg.add_vec_suid(suid);
	}
	synMsg.set_opertype(opertype);

	synMsg.set_tag_suid(info.suid);
	synMsg.set_tag_sdeviceid(info.sdeviceid);
	synMsg.set_tag_tuid(info.tuid);
	for (uint32 gid : info.cgid)
	{
		synMsg.add_tag_cgid(gid);
	}
	synMsg.set_tag_skey(info.skey);

	CServerMgr::Instance().SynCtrlUserCfg(&synMsg);
		
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
	return 0;	
}

// 更改房间库存配置 add by har
int CHandlePHPMsg::handle_php_change_room_stock_cfg(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") ||
		!jvalue.isMember("stock_max") || !jvalue.isMember("stock_conversion_rate") || !jvalue.isMember("jackpot_min") ||
		!jvalue.isMember("jackpot_max_rate") || !jvalue.isMember("jackpot_rate") || !jvalue.isMember("jackpot_coefficient") ||
		!jvalue.isMember("jackpot_extract_rate") || !jvalue.isMember("add_stock") || !jvalue.isMember("kill_points_line") ||
		!jvalue.isMember("player_win_rate") || !jvalue.isMember("add_jackpot") || !jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() ||
		!jvalue["stock_max"].isIntegral() || !jvalue["stock_conversion_rate"].isIntegral() || 
		!jvalue["jackpot_min"].isIntegral() || !jvalue["jackpot_max_rate"].isIntegral() || !jvalue["jackpot_rate"].isIntegral() ||
		!jvalue["jackpot_coefficient"].isIntegral() || !jvalue["jackpot_extract_rate"].isIntegral() || !jvalue["add_stock"].isIntegral() || 
		!jvalue["kill_points_line"].isIntegral() || !jvalue["player_win_rate"].isIntegral() || !jvalue["add_jackpot"].isIntegral())
	{
		LOG_ERROR("handle_php_change_room_stock_cfg json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	stStockCfg st;
	uint32 gametype = jvalue["gametype"].asUInt();
	st.roomID = jvalue["roomid"].asUInt();
	st.stockMax = jvalue["stock_max"].asInt64();
	st.stockConversionRate = jvalue["stock_conversion_rate"].asInt();
	st.jackpotMin = jvalue["jackpot_min"].asInt64();
	st.jackpotMaxRate = jvalue["jackpot_max_rate"].asInt();
	st.jackpotRate = jvalue["jackpot_rate"].asInt();
	st.jackpotCoefficient = jvalue["jackpot_coefficient"].asInt(); // 奖池系数过大，会宕机！
	st.jackpotExtractRate = jvalue["jackpot_extract_rate"].asInt();
	st.stock = jvalue["add_stock"].asInt64();
	st.killPointsLine = jvalue["kill_points_line"].asInt64();
	st.playerWinRate = jvalue["player_win_rate"].asInt();
	st.jackpot = jvalue["add_jackpot"].asInt64();
	LOG_DEBUG("handle_php_change_room_stock_cfg  param - gametype:%d,roomid:%d,stockMax:%d,stockonvCersionRate:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,stock:%lld,killPointsLine:%lld,playerWinRate:%d,jackpot:%lld",
		gametype, st.roomID, st.stockMax, st.stockConversionRate, st.jackpotMin, st.jackpotMaxRate, st.jackpotRate, 
		st.jackpotCoefficient, st.jackpotExtractRate, st.stock, st.killPointsLine, st.playerWinRate, st.jackpot);

	if (!CServerMgr::Instance().NotifyGameSvrsChangeRoomStockCfg(gametype, st)) {
		jrep["ret"] = 0;
		LOG_ERROR("handle_php_change_room_stock_cfg game server not exist - msg:%s", msg.c_str());
	} else
		jrep["ret"] = 1;

	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);

	return 0;
}

// php开启或关闭或更新个人专属支付宝
int CHandlePHPMsg::handle_php_notify_exclusive_alipay_recharge(NetworkObject* pNetObj, CBufferStream& stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("exclusivealipay") && jvalue["exclusivealipay"].isObject()) {
		Json::Value  jvalueRecharge = jvalue["exclusivealipay"];
		string strjvalueRecharge = jvalueRecharge.toFastString();

		LOG_DEBUG("php strjvalueRecharge  param - strjvalueRecharge:%s", strjvalueRecharge.c_str());

		if (CDataCfgMgr::Instance().ExclusiveAlipayRechargeAnalysis(true, strjvalueRecharge)) {
			jrep["ret"] = 1;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_EXCLUSIVE_ALIPAY_RECHARGE);
			return 0;
		}
	} 

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_EXCLUSIVE_ALIPAY_RECHARGE);
	return 0;
}

int  CHandlePHPMsg::handle_php_change_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
		return 0;
	}

	uint32 uid = 0;
	if (!jvalue.isMember("uid"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
		return 0;
	}
	else
	{		
		uid = jvalue["uid"].asUInt();		
	}
	
    LOG_DEBUG("update player lucky info - uid:%d", uid);

	msg_syn_lucky_cfg synMsg;
	synMsg.set_uid(uid);				
	CServerMgr::Instance().SynLuckyCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
	return 0;
}

int  CHandlePHPMsg::handle_php_change_fish_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}

	uint32 id = 0;
	if (!jvalue.isMember("id"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		id = jvalue["id"].asUInt();
	}

	uint32 prize_min = 0;
	if (!jvalue.isMember("prize_min"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		prize_min = jvalue["prize_min"].asUInt();
	}

	uint32 prize_max = 0;
	if (!jvalue.isMember("prize_max"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		prize_max = jvalue["prize_max"].asUInt();
	}

	uint32 kill_rate = 0;
	if (!jvalue.isMember("kill_rate"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		kill_rate = jvalue["kill_rate"].asUInt();
	}

	LOG_DEBUG("update fish config info - id:%d prize_min:%d prize_max:%d kill_rate:%d", id, prize_min, prize_max, kill_rate);

	msg_syn_fish_cfg synMsg;
	synMsg.set_id(id);
	synMsg.set_prize_min(prize_min);
	synMsg.set_prize_max(prize_max);
	synMsg.set_kill_rate(kill_rate);
	CServerMgr::Instance().SynFishCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
	return 0;
}

int  CHandlePHPMsg::handle_php_reset_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	msg_reset_lucky_cfg synMsg;
	synMsg.set_uid(0);
	CServerMgr::Instance().ResetLuckyCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_RESET_LUCK_CONFIG_INFO);
	return 0;
}

// php修改定额支付宝充值显示信息
int CHandlePHPMsg::handle_php_notify_fixed_alipay_recharge(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("fixedalipay") && jvalue["fixedalipay"].isObject()) {
		Json::Value  jvalueRecharge = jvalue["fixedalipay"];
		string strjvalueRecharge = jvalueRecharge.toFastString();

		LOG_DEBUG("php strjvalueRecharge  param - strjvalueRecharge:%s", strjvalueRecharge.c_str());

		if (CDataCfgMgr::Instance().FixedAlipayRechargeAnalysis(true, strjvalueRecharge)) {
			jrep["ret"] = 1;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_ALIPAY_RECHARGE);
			return 0;
		}
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_ALIPAY_RECHARGE);
	return 0;
}

// php修改定额微信充值显示信息
int CHandlePHPMsg::handle_php_notify_fixed_wechat_recharge(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("fixedwxpay") && jvalue["fixedwxpay"].isObject()) {
		Json::Value  jvalueRecharge = jvalue["fixedwxpay"];
		string strjvalueRecharge = jvalueRecharge.toFastString();

		LOG_DEBUG("php strjvalueRecharge  param - strjvalueRecharge:%s", strjvalueRecharge.c_str());

		if (CDataCfgMgr::Instance().FixedWechatRechargeAnalysis(true, strjvalueRecharge)) {
			jrep["ret"] = 1;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_WECHAT_RECHARGE);
			return 0;
		}
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_WECHAT_RECHARGE);
	return 0;
}

// php修改定额云联云闪付充值显示信息
int CHandlePHPMsg::handle_php_notify_fixed_unionpay_recharge(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("fixedysfpay") && jvalue["fixedysfpay"].isObject()) {
		Json::Value  jvalueRecharge = jvalue["fixedysfpay"];
		string strjvalueRecharge = jvalueRecharge.toFastString();

		LOG_DEBUG("php strjvalueRecharge  param - strjvalueRecharge:%s", strjvalueRecharge.c_str());

		if (CDataCfgMgr::Instance().FixedUnionpayRechargeAnalysis(true, strjvalueRecharge)) {
			jrep["ret"] = 1;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_UNIONPAY_RECHARGE);
			return 0;
		}
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_FIXED_UNIONPAY_RECHARGE);
	return 0;
}

// php修改专享闪付充值显示信息
int CHandlePHPMsg::handle_php_notify_exclusive_flash_recharge(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("exclusiveshanpay") && jvalue["exclusiveshanpay"].isObject()) {
		Json::Value  jvalueRecharge = jvalue["exclusiveshanpay"];
		string strjvalueRecharge = jvalueRecharge.toFastString();

		LOG_DEBUG("php strjvalueRecharge  param - strjvalueRecharge:%s", strjvalueRecharge.c_str());

		if (CDataCfgMgr::Instance().ExclusiveFlashRechargeAnalysis(true, strjvalueRecharge)) {
			jrep["ret"] = 1;
			SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE);
			return 0;
		}
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE);
	return 0;
}
