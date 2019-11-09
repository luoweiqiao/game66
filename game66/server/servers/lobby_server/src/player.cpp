
#include "player.h"
#include "helper/bufferStream.h"
#include "stdafx.h"
#include <time.h>
#include <gobal_robot_mgr.h>
#include "center_log.h"

using namespace svrlib;
using namespace std;
using namespace Network;

namespace 
{	
	static const uint32 s_OfflineTime = 1;//SECONDS_IN_MIN*10;// 离线保留多久数据(回收误差3s)
};
CPlayer::CPlayer(uint8 type)
:CPlayerBase(type)
{
	m_disconnectTime = 0;
	m_missionMgr.AttachPlayer(this);
    m_lastSpeakTime = 0;
	m_needRecover = false;
	m_loginTime = getSysTime();
	m_secondSvrID = 0;
}
CPlayer::~CPlayer()
{

}
bool	CPlayer::OnLoginOut(uint32 leaveparam)
{
	LOG_DEBUG("OnLoginOut:%d,IsPlaying:%d",GetUID(), IsPlaying());
	if(IsPlaying() && !IsRobot())
	{
		// 保存数据
		m_missionMgr.SaveMiss();
		// 刷新数据
		SaveLoginInfo();

		CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(GetUID(),0,0,GetPlayerType());
		CDBMysqlMgr::Instance().AddPlayerLoginTimeInfo(GetUID(),0,(getSysTime()-m_loginTime));
	}
	SetPlayerState(PLAYER_STATE_LOGINOUT);
	NotifyNetState2GameSvr(0);
	NotifyNetState2SecondGameSvr(0);
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR){
        NotifyLeaveGame(net::RESULT_CODE_SVR_REPAIR);
    }
	return true;
}
void	CPlayer::OnLogin()
{
	LOG_DEBUG("OnLogin - uid:%d",GetUID());
    net::msg_login_rep repmsg;
	repmsg.set_result(net::RESULT_CODE_SUCCESS);
	repmsg.set_server_time(getSysTime());
    SendMsgToClient(&repmsg,net::S2C_MSG_LOGIN_REP);
        
	// 拉取数据  
    CDBMysqlMgr::Instance().AsyncLoadPlayerData(GetUID());
    CDBMysqlMgr::Instance().AsyncLoadAccountData(GetUID());
	if(IsRobot()){//机器人不加载任务
		SetLoadState(emACCDATA_TYPE_MISS,1);
	}else{
		CDBMysqlMgr::Instance().AsyncLoadMissionData(GetUID());
	}
    for(uint32 i=1;i<net::GAME_CATE_MAX_TYPE;++i)
    {
        if(!CCommonLogic::IsOpenGame(i))
        {
            m_loadGameState[i] = 1;
            continue;
        }        
        CDBMysqlMgr::Instance().AsyncLoadGameData(GetUID(),i);
    }    
    
	SetPlayerState(PLAYER_STATE_LOAD_DATA);

	CDBMysqlMgr::Instance().UpdatePlayerLoginTime(GetUID(),getSysTime(),GetIPStr());
    CCenterLogMgr::Instance().UserActionLog(GetUID(),emUSER_ACTION_TYPE_LOGIN);

	m_loadTime = getSysTime();
}
void	CPlayer::OnGetAllData()
{	
    LOG_DEBUG("OnGetData player data - uid:%d,ip:%d,sip:%s",GetUID(),GetIP(), GetIPStr().c_str());
	SetSafeBoxState(0);
	SetPlayerState(PLAYER_STATE_PLAYING);
	BuildInit();		
	// 发送数据到客户端
	SendAllPlayerData2Client();
	NotifyEnterGame();

	SetCurSvrID(CRedisMgr::Instance().GetPlayerOnlineSvrID(GetUID()),false);

	NotifyNetState2GameSvr(1);
	CServerMgr::Instance().SendSvrRepairContent(GetSession());
    CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(GetUID(),CApplication::Instance().GetServerID(),0,GetPlayerType(),GetAccountValue(emACC_VALUE_COIN),GetAccountValue(emACC_VALUE_SAFECOIN),GetAccountValue(emACC_VALUE_SCORE),GetCity());

    m_loginTime = getSysTime();
	bool bIsNoviceWelfare = IsNoviceWelfare();
	//if (bIsNoviceWelfare)
	//{
	//	CDBMysqlMgr::Instance().AsyncLoadPayData(GetUID());
	//}
	LOG_DEBUG("player_login_getdata - uid:%d,bIsNoviceWelfare:%d,city:%s", GetUID(), bIsNoviceWelfare, GetCity().c_str());
}
void	CPlayer::ReLogin()
{
	LOG_DEBUG("ReLogin - uid:%d,svrid:%d", GetUID(), GetCurSvrID());

	net::msg_login_rep repmsg;
	repmsg.set_result(net::RESULT_CODE_SUCCESS);
	repmsg.set_server_time(getSysTime());
	SendMsgToClient(&repmsg,net::S2C_MSG_LOGIN_REP);

	SetSafeBoxState(0);
    SendAllPlayerData2Client();
    NotifyEnterGame();
    if(GetCurSvrID() != 0)
    {
        // 处理断线重连
        NotifyNetState2GameSvr(1);
    }
	CDBMysqlMgr::Instance().UpdatePlayerLoginTime(GetUID(),getSysTime(),GetIPStr());
	CServerMgr::Instance().SendSvrRepairContent(GetSession());
    CCenterLogMgr::Instance().UserActionLog(GetUID(),emUSER_ACTION_TYPE_LOGIN);
}
void 	CPlayer::OnTimeTick(uint64 uTime,bool bNewDay)
{
	if(!IsPlaying())
	{
		return;
	}
	if(CCommonLogic::IsNeedReset(m_baseInfo.offlinetime,uTime))
	{
		DailyCleanup(1);
		SaveLoginInfo();
	}
	// 更新离线时间
	m_baseInfo.offlinetime = uTime;
	// 新的一天
	if( bNewDay )
	{
		tm local_time;
		getLocalTime( &local_time, uTime );

		// 跨周0星期天，1星期1
		if (local_time.tm_wday == 0)
		{
			WeeklyCleanup();
		}
		// 跨月
		if (local_time.tm_mday == 1)
		{
			MonthlyCleanup();
		}
		SaveLoginInfo();
		SendAllPlayerData2Client();
	}
	if(m_pSession == NULL)
	{
        if(m_disconnectTime == 0)
		{
		    m_disconnectTime = getSysTime();
        }
	}
	else
	{
		m_disconnectTime = 0;
	}
}
// 是否需要回收
bool 	CPlayer::NeedRecover()
{
	if(GetPlayerState() == PLAYER_STATE_LOAD_DATA && (getSysTime()-m_loadTime) > SECONDS_IN_MIN) {
		LOG_ERROR("加载数据超时下线 - uid:%d",GetUID());
		return true;
	}
	if (!IsInLobby() || GetPlayerState() == PLAYER_STATE_LOAD_DATA)
	{
		return false;
	}
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return true;
	}
	if(m_pSession == NULL)
	{
		if(IsInLobby() && (getSysTime() - m_disconnectTime) > s_OfflineTime)// 不在游戏中，或者超时下线
		{
			return true;
		}
	}
	if(m_needRecover){
		return true;
	}
	return false;
}
// 返回大厅回调
void    CPlayer::BackLobby()
{

}
// 游戏战报
void 	CPlayer::OnGameEnd(uint16 gameType)
{
    m_baseInfo.dayGameCount++;
    
}
bool 	CPlayer::CanModifyData()
{
	if (m_bPlayerState >= PLAYER_STATE_PLAYING)
	{
		return true;
	}
	return false;
}

//--- 每日清理
void	CPlayer::DailyCleanup(int32 iOfflineDay)
{
	m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_DAY);

    //连续签到是否断了
	if(!IsSetRewardBitFlag(net::REWARD_CLOGIN)){
		m_baseInfo.clogin = 0;
	}
    UnsetRewardBitFlag(net::REWARD_CLOGIN);
    
	m_baseInfo.weekLogin++;
	m_baseInfo.bankrupt = 0;
    m_baseInfo.dayGameCount = 0;
    
	LOG_DEBUG("每日清理");

    for(uint16 i=1;i<net::GAME_CATE_MAX_TYPE;++i)
    {
		if (!CCommonLogic::IsOpenGame(i))
		{
			continue;
		}
	    m_gameInfo[i].daywin  = 0;
	    m_gameInfo[i].daywinc = 0;
        CDBMysqlMgr::Instance().ResetGameDaywin(i,GetUID());
    }
    CDBMysqlMgr::Instance().AddPlayerLoginTimeInfo(GetUID(),1,0);
}
void    CPlayer::SignIn()
{
	m_baseInfo.clogin++;
	//m_baseInfo.clogin = MIN(m_baseInfo.clogin,6);
}
//--- 每周清理
void	CPlayer::WeeklyCleanup()
{
	LOG_DEBUG("每周清理");
	m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_WEEK);
	UnsetRewardBitFlag(net::REWARD_WLOGIN3);
	UnsetRewardBitFlag(net::REWARD_WLOGIN5);
	UnsetRewardBitFlag(net::REWARD_WLOGIN6);
	m_baseInfo.weekLogin = 1;

	//for (uint16 i = 1; i<net::GAME_CATE_MAX_TYPE; ++i)
	//{
	//	if (!CCommonLogic::IsOpenGame(i))
	//	{
	//		continue;
	//	}
	//	if (i != net::GAME_CATE_DICE)
	//	{
	//		continue;
	//	}
	//	m_gameInfo[i].weekwinc = 0;
	//	CDBMysqlMgr::Instance().ResetGameWeekWin(i, GetUID());
	//}
}
//--- 每月清理
void	CPlayer::MonthlyCleanup()
{
	LOG_DEBUG("每月清理");
	m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_MONTH);

}
void	CPlayer::NotifyEnterGame()
{
	net::msg_enter_game_rep msg;
    msg.set_result(0);

    SendMsgToClient(&msg,net::S2C_MSG_ENTER_GAME);
	LOG_DEBUG("notify enter game - uid:%d",GetUID());
}
void    CPlayer::NotifyLeaveGame(uint32 code)
{
    LOG_DEBUG("通知离开游戏:%d--%d",GetUID(),code);
    net::msg_notify_leave_rep leavemsg;
	leavemsg.set_result(code);
	SendMsgToClient(&leavemsg,net::S2C_MSG_NOTIFY_LEAVE);        
}
bool	CPlayer::SendAllPlayerData2Client()
{
	SendAccData2Client();

    for(uint16 i=1;i<net::GAME_CATE_MAX_TYPE;++i)
    {
		if (!CCommonLogic::IsOpenGame(i))
		{
			continue;
		}
        UpdateGameInfo2Client(i);
    }
    //test 兼容斗地主数据
    /*net::msg_update_land_info msg;    
    net::land_info* pInfo = msg.mutable_land_data();
    stGameBaseInfo& info = m_gameInfo[net::GAME_CATE_LAND];
    pInfo->set_win(info.win);
    pInfo->set_lose(info.lose);
    pInfo->set_maxwin(info.maxwin);
    pInfo->set_winc(info.winc);
    pInfo->set_losec(info.losec);
    pInfo->set_maxwinc(info.maxwinc);    
    pInfo->set_land(info.land);
    pInfo->set_spring(info.spring);
    pInfo->set_landc(info.landc);
    pInfo->set_springc(info.springc);      
    SendMsgToClient(&msg,net::S2C_MSG_UPDATA_LAND_INFO);
    */
	m_missionMgr.SendMissionData2Client();


	//发送广播
	SendVipBroadCast();
	//发送vip代理充值
	SendVipProxyRecharge();

	SendUnionPayRecharge();
	SendWeChatPayRecharge();
	SendAliPayRecharge();
	SendOtherPayRecharge();
	SendQQPayRecharge();
	SendWeChatScanPayRecharge();
	SendJDPayRecharge();

	return true;
}
bool	CPlayer::SendAccData2Client()
{
	LOG_DEBUG("发送账号数据:%d",GetUID());

    net::msg_player_data_rep msg;
 	GetPlayerBaseData(msg.mutable_base_data());
    SendMsgToClient(&msg,net::S2C_MSG_PLAYER_INFO);

	return true;
}
bool 	CPlayer::UpdateAccValue2Client()
{
	//LOG_DEBUG("更新数值到前端:score:%lld,coin:%lld",GetAccountValue(emACC_VALUE_SCORE),GetAccountValue(emACC_VALUE_COIN));
	net::msg_update_acc_value msg;
	msg.set_coin(GetAccountValue(emACC_VALUE_COIN));
	msg.set_cvalue(GetAccountValue(emACC_VALUE_CVALUE));
	msg.set_diamond(GetAccountValue(emACC_VALUE_DIAMOND));
	msg.set_ingot(GetAccountValue(emACC_VALUE_INGOT));
	msg.set_safe_coin(GetAccountValue(emACC_VALUE_SAFECOIN));
	msg.set_score(GetAccountValue(emACC_VALUE_SCORE));

	SendMsgToClient(&msg,net::S2C_MSG_UPDATE_ACC_VALUE);
	return true;
}
bool 	CPlayer::UpdateBaseValue2Client()
{
	//LOG_DEBUG("更新基础数值到前端");
	net::msg_update_base_value msg;
	msg.set_bankrupt(m_baseInfo.bankrupt);
	msg.set_clogin(m_baseInfo.clogin);
	msg.set_reward(m_baseInfo.reward);
	msg.set_safeboxstate(m_baseInfo.safeboxState);
	msg.set_weeklogin(m_baseInfo.weekLogin);
    msg.set_day_game_count(m_baseInfo.dayGameCount);
    
	SendMsgToClient(&msg,net::S2C_MSG_UPDATA_BASE_VALUE);
	return true;
}


// 同步游戏数据
bool    CPlayer::UpdateGameInfo2Client(uint16 gameType)
{
    net::msg_update_game_info msg;
    GetGameData(gameType,msg.mutable_data());

    SendMsgToClient(&msg,net::S2C_MSG_UPDATA_GAME_INFO);
    return true;
}
// 通知返回大厅
void    CPlayer::NotifyClientBackLobby()
{    
    net::msg_back_lobby_rep rep;
    rep.set_result(1);
    SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP);    
}    
// 构建初始化
void	CPlayer::BuildInit()
{
	LOG_DEBUG("构建初始化");

	// 检测日常
	uint32 uBuildTime			= getSysTime();
	uint32 uOfflineSecond		= ((m_baseInfo.offlinetime && uBuildTime > m_baseInfo.offlinetime) ? (uBuildTime - m_baseInfo.offlinetime) : 0);
 	//跨天检查并清理
	int32 iOfflineDay = diffTimeDay(m_baseInfo.offlinetime,uBuildTime); 
	if(m_baseInfo.offlinetime == 0 && iOfflineDay <= 0)//新上线绝对是第一天
	{
		DailyCleanup(1);	
		iOfflineDay = 0;
	}
	//跨周检查并清理   
	int32 iOfflineWeek = diffTimeWeek(m_baseInfo.offlinetime,uBuildTime);    
	if(m_baseInfo.offlinetime == 0 && iOfflineWeek <= 0){
		WeeklyCleanup();
		iOfflineWeek = 0;
	}
	//跨月检查并清理   
	int32 iOfflineMonth = diffTimeMonth(m_baseInfo.offlinetime,uBuildTime);
	if(m_baseInfo.offlinetime == 0 && iOfflineMonth <= 0){
		MonthlyCleanup();
		iOfflineMonth = 0;
	}
	if(m_baseInfo.offlinetime != 0 && CCommonLogic::IsNeedReset(m_baseInfo.offlinetime,uBuildTime)){
		DailyCleanup(iOfflineDay);
	}
	if(iOfflineWeek > 0){
		WeeklyCleanup();
	}
	if(iOfflineMonth > 0){
		MonthlyCleanup();
	}
	m_baseInfo.offlinetime = uBuildTime;
	m_baseInfo.loginIP	   = GetIP();
	
    SaveLoginInfo();
}
uint16  CPlayer::GetCurSvrID()
{
    return m_curSvrID;
}
void    CPlayer::SetCurSvrID(uint16 svrID,bool bSetRedis)
{
	stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(svrID);
	if (pServer != NULL && pServer->gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
	if(svrID != 0){
		if(CServerMgr::Instance().GetServerBySvrID(svrID) == NULL){
			svrID = 0;
			bSetRedis = true;
		}
	}
    m_curSvrID = svrID;
    if(bSetRedis){
    	CRedisMgr::Instance().SetPlayerOnlineSvrID(GetUID(),svrID);
    }
}
void 	CPlayer::SyncCurSvrIDFromRedis(uint16 svrID)
{
	stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(svrID);
	if (pServer != NULL && pServer->gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
	if (m_curSvrID != svrID)
	{
		return;
	}
	SetCurSvrID(CRedisMgr::Instance().GetPlayerOnlineSvrID(GetUID()),false);
    if(m_curSvrID == 0){
        NotifyClientBackLobby();
    }
}
// 是否在大厅中
bool 	CPlayer::IsInLobby()
{
	if(m_curSvrID != 0){
		return false;
	}
	return true;
}
bool  	CPlayer::SendMsgToGameSvr(const google::protobuf::Message* msg,uint16 msg_type)
{
	if (m_curSvrID == 0)
	{
		return false;
	}		
	CServerMgr::Instance().SendMsg2Server(m_curSvrID,msg,msg_type,GetUID());
	return true;
}
bool  	CPlayer::SendMsgToGameSvr(const void *msg, uint16 msg_len, uint16 msg_type)
{
	if (m_curSvrID == 0)
	{
		return false;
	}		
	CServerMgr::Instance().SendMsg2Server(m_curSvrID,(uint8*)msg,msg_len,msg_type,GetUID());
	return true;
}
// 通知网络状态
void	CPlayer::NotifyNetState2GameSvr(uint8 state)
{
	net::msg_notify_net_state msg;
	msg.set_uid(GetUID());
	msg.set_state(state);
	LOG_DEBUG("uid:%d,state:%d", GetUID(), state);
	SendMsgToGameSvr(&msg,net::L2S_MSG_NOTIFY_NET_STATE);
}
void	CPlayer::NotifyNetState2SecondGameSvr(uint8 state)
{
	if (GetSecondSvrID() == 0)
	{
		return;
	}
	net::msg_notify_net_state msg;
	msg.set_uid(GetUID());
	msg.set_state(state);
	CServerMgr::Instance().SendMsg2Server(GetSecondSvrID(), &msg, net::L2S_MSG_NOTIFY_NET_STATE, GetUID());
	//CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(GetUID(), 0, 0, GetPlayerType());
}
// 通知修改玩家信息
void 	CPlayer::NotifyChangePlayerInfo2GameSvr()
{
	net::msg_notify_change_playerinfo msg;
	msg.set_name(GetPlayerName());
	msg.set_safebox(GetSafeBoxState());
	SendMsgToGameSvr(&msg,net::L2S_MSG_NOTIFY_CHANGE_PLAYERINFO);
}
// 进入游戏服务器
bool	CPlayer::EnterGameSvr(uint16 svrID)
{
	if(GetCurSvrID() != 0 && GetCurSvrID() != svrID){
		//LOG_DEBUG("进入服务器失败:%d-->%d",GetCurSvrID(),svrID);
		LOG_DEBUG("enter_game_server_failed serverid error - uid:%d,isrobot:%d,batchid:%d,curCerverID:%d,enterServerID:%d",
			GetUID(), IsRobot(), GetBatchID(), GetCurSvrID(), svrID);
		return false;	
	}
    stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(svrID);
	if(pServer == NULL || pServer->status == emSERVER_STATE_REPAIR)
	{
		SetCurSvrID(0,true);
		//LOG_DEBUG("服务器不存在:%d",svrID);
		uint8 status = 33;
		if (pServer != NULL) {
			status = pServer->status;
		}

		LOG_DEBUG("enter_game_server_failed server not exist - uid:%d,isrobot:%d,batchid:%d,curCerverID:%d,enterServerID:%d,pServer:%p,status:%d",
			GetUID(), IsRobot(), GetBatchID(), GetCurSvrID(), svrID, pServer, status);
		return false;
	}
	SetCurSvrID(svrID,false);
    
	//判断当前玩家是否有精准控制权限
	uint8 ctrl_flag = 0;
	bool flag = CDataCfgMgr::Instance().GetUserControlFlag(GetUID(), pServer->gameType, GetLoginDeviceID(), GetCheckCode());
	if (flag)
	{
		ctrl_flag = 1;
	}

	// 发送游戏数据到游戏服
	net::msg_enter_into_game_svr msg;
	msg.set_player_type(GetPlayerType());
	GetPlayerGameData(pServer->gameType,&msg);
	msg.set_ctrl_flag(ctrl_flag);
	CServerMgr::Instance().SendMsg2Server(svrID,&msg,net::L2S_MSG_ENTER_INTO_SVR,GetUID());
	LOG_DEBUG("enter_game_server - svrid:%d,uid:%d,isrobot:%d,batchid:%d,score:%lld,coin:%lld,win:%d,lose:%d,maxwinc:%lld,daywinc:%lld,ctrl_flag:%d",
		svrID,GetUID(),IsRobot(),GetBatchID(),GetAccountValue(emACC_VALUE_SCORE),GetAccountValue(emACC_VALUE_COIN), msg.game_data().winc(), msg.game_data().losec(), msg.game_data().maxwinc(), msg.game_data().daywinc(), ctrl_flag);

	return true;
}
// 刷新修改数值到游戏服
void    CPlayer::FlushChangeAccData2GameSvr(int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin)
{
    if(GetCurSvrID() == 0)
        return;
    
    net::msg_flush_change_account_data msg;
    msg.set_uid(GetUID());
    msg.set_diamond(diamond);
    msg.set_coin(coin);
    msg.set_ingot(ingot);
    msg.set_score(score);
    msg.set_cvalue(cvalue);
    msg.set_safe_coin(safecoin);
    
    SendMsgToGameSvr(&msg,net::L2S_MSG_FLUSH_CHANGE_ACC_DATA);
}

void    CPlayer::UpDateChangeAccData2GameSvr(int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, uint32 oper_type, uint32 sub_type)
{
	if (GetCurSvrID() == 0)
		return;

	net::msg_update_change_account_data msg;
	msg.set_uid(GetUID());
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safecoin);
	msg.set_oper_type(oper_type);
	msg.set_sub_type(sub_type);

	SendMsgToGameSvr(&msg, net::L2S_MSG_UPDATE_CHANGE_ACC_DATA);
}

void    CPlayer::UpDateChangeAccData2GameSvrEveryColor(int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, uint32 oper_type, uint32 sub_type)
{
	net::msg_update_change_account_data msg;
	msg.set_uid(GetUID());
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safecoin);
	msg.set_oper_type(oper_type);
	msg.set_sub_type(sub_type);

	uint16 svrID = CServerMgr::Instance().GetGameTypeSvrID(net::GAME_CATE_EVERYCOLOR);
	if (svrID != 0)
	{
		CServerMgr::Instance().SendMsg2Server(svrID, &msg, net::L2S_MSG_UPDATE_CHANGE_ACC_DATA, GetUID());
	}
}


// 修改玩家账号数值（增量修改）
bool    CPlayer::SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid)
{
	LOG_DEBUG("sta - uid:%d,operType:%d,coin:%lld,cur_coin:%lld", GetUID(), operType, coin, GetAccountValue(emACC_VALUE_COIN));

	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND,diamond);
	if(diamond != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_DIAMOND,operType,subType,diamond,GetAccountValue(emACC_VALUE_DIAMOND)-diamond,GetAccountValue(emACC_VALUE_DIAMOND),chessid);
	}
	coin    = ChangeAccountValue(emACC_VALUE_COIN,coin);
	if(coin != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_COIN,operType,subType,coin,GetAccountValue(emACC_VALUE_COIN)-coin,GetAccountValue(emACC_VALUE_COIN),chessid);
	}
	score   = ChangeAccountValue(emACC_VALUE_SCORE,score);
	if(score != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_SCORE,operType,subType,score,GetAccountValue(emACC_VALUE_SCORE)-score,GetAccountValue(emACC_VALUE_SCORE),chessid);
	}
	ingot   = ChangeAccountValue(emACC_VALUE_INGOT,ingot);
	if(ingot != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_INGOT,operType,subType,ingot,GetAccountValue(emACC_VALUE_INGOT)-ingot,GetAccountValue(emACC_VALUE_INGOT),chessid);
	}
	cvalue  = ChangeAccountValue(emACC_VALUE_CVALUE,cvalue);
	if(cvalue != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_CVALUE,operType,subType,cvalue,GetAccountValue(emACC_VALUE_CVALUE)-cvalue,GetAccountValue(emACC_VALUE_CVALUE),chessid);
	}
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN,safecoin);
	if(safecoin != 0){
		CCenterLogMgr::Instance().AccountTransction(GetUID(),emACC_VALUE_SAFECOIN,operType,subType,safecoin,GetAccountValue(emACC_VALUE_SAFECOIN)-safecoin,GetAccountValue(emACC_VALUE_SAFECOIN),chessid);
	}

	bool ret = CDBMysqlMgr::Instance().ChangeAccountValue(GetUID(), diamond, coin, ingot, score, cvalue, safecoin);
	if (ret == false)
	{
		return false;
	}

	LOG_DEBUG("end - uid:%d,operType:%d,coin:%lld,cur_coin:%lld", GetUID(), operType, coin, GetAccountValue(emACC_VALUE_COIN));

	return ret;
}
// 保存登陆奖励状态
void 	CPlayer::SaveLoginInfo()
{
	LOG_DEBUG("保存登陆login信息:%d",GetUID());
	CDBMysqlMgr::Instance().UpdatePlayerLoginInfo(GetUID(),m_baseInfo.offlinetime,m_baseInfo.clogin,m_baseInfo.weekLogin,m_baseInfo.reward,m_baseInfo.bankrupt,m_baseInfo.dayGameCount);
}
// 领取破产补助
bool 	CPlayer::GetBankruptHelp()
{
	LOG_DEBUG("领取破产保护:%d",GetUID());
    uint8 coinType = CDataCfgMgr::Instance().GetBankruptType();
    
	if(GetAccountValue(coinType) + GetAccountValue(emACC_VALUE_SAFECOIN) > CDataCfgMgr::Instance().GetBankruptBase() ||  GetBankrupt() >= CDataCfgMgr::Instance().GetBankruptCount())
	{
		LOG_DEBUG("没到破产补助的条件:%d-%d",GetAccountValue(coinType),GetBankrupt());
		return false;
	}
	else
	{
        int64 score = 0;
        int64 coin  = 0;
        if(coinType == emACC_VALUE_COIN){
            coin  = CDataCfgMgr::Instance().GetBankruptValue();
        }else if(coinType == emACC_VALUE_SCORE){
            score = CDataCfgMgr::Instance().GetBankruptValue();
        }
        
		SyncChangeAccountValue(emACCTRAN_OPER_TYPE_BANKRUPT,GetBankrupt(),0,coin,0,score,0,0);
		AddBankrupt();
		UpdateAccValue2Client();
		CDBMysqlMgr::Instance().AddBankruptValue(GetUID());
		SaveLoginInfo();
		return true;
	}
}
uint32  CPlayer::GetSpeakCDTime()
{
	if (m_lastSpeakTime < getSysTime())
	{
		return 0;
	}
    return m_lastSpeakTime - getSysTime();
}
void    CPlayer::Speak()
{
    m_lastSpeakTime = getSysTime() + SECONDS_IN_MIN*2;
}

//进入时时彩游戏服
bool	CPlayer::EnterEveryColorGameSvr(uint16 svrID)
{
	stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(svrID);
	if (pServer == NULL || pServer->status == emSERVER_STATE_REPAIR)
	{
		int  status = -1;
		if (pServer != NULL)
		{
			status = pServer->status;
		}
		SetSecondSvrID(0);
		LOG_DEBUG("enter EveryColor server failed,server not exists - uid:%d,status:%d,en_serverid%d", GetUID(), status, svrID);
		return false;
	}
	SetSecondSvrID(svrID);
	// 发送游戏数据到游戏服
	net::msg_enter_into_game_svr msg;
	msg.set_player_type(GetPlayerType());
	GetPlayerGameData(pServer->gameType, &msg);
	//LOG_DEBUG("enter EveryColor server start - svrid:%d,uid:%d,score:%lld,coin:%lld", svrID, GetUID(), GetAccountValue(emACC_VALUE_SCORE), GetAccountValue(emACC_VALUE_COIN));
	CServerMgr::Instance().SendMsg2Server(svrID, &msg, net::L2S_MSG_ENTER_INTO_EVERY_COLOR_SVR, GetUID());
	LOG_DEBUG("enter EveryColor server success - svrid:%d,uid:%d,score:%lld,coin:%lld", svrID, GetUID(), GetAccountValue(emACC_VALUE_SCORE), GetAccountValue(emACC_VALUE_COIN));
	if (GetCurSvrID() == 0)
	{
		CDBMysqlMgr::Instance().UpdatePlayerOnlineInfoEx(GetUID(), GetSecondSvrID(), 0, GetPlayerType(), GetAccountValue(emACC_VALUE_COIN), GetAccountValue(emACC_VALUE_SAFECOIN), GetAccountValue(emACC_VALUE_SCORE), GetCity());
	}
	return true;
}

//离开时时彩游戏服
bool	CPlayer::LeaveEveryColorGameSvr(uint16 svrID)
{
	stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(svrID);
	if (pServer == NULL || pServer->status == emSERVER_STATE_REPAIR)
	{
		int  status = -1;
		if (pServer != NULL) {
			status = pServer->status;
		}
		LOG_DEBUG("leave EveryColor server failed,server not exists - uid:%d,status:%d,en_serverid%d", GetUID(), status, svrID);
		return false;
	}
	SetSecondSvrID(0);
	// 发送游戏数据到游戏服
	net::msg_leave_every_color_gamesvr_req msg;
	msg.set_svrid(pServer->svrID);
	CServerMgr::Instance().SendMsg2Server(svrID, &msg, net::L2S_MSG_ENTER_INTO_LEAVE_COLOR_SVR, GetUID());
	LOG_DEBUG("leave EveryColor server success - svrid:%d,uid:%d,GetSecondSvrID:%d,score:%lld,coin:%lld", svrID, GetUID(), GetSecondSvrID(),GetAccountValue(emACC_VALUE_SCORE), GetAccountValue(emACC_VALUE_COIN));
	if (GetCurSvrID()==0)
	{
		CDBMysqlMgr::Instance().UpdatePlayerOnlineInfoEx(GetUID(), CApplication::Instance().GetServerID(), 0, GetPlayerType(), GetAccountValue(emACC_VALUE_COIN), GetAccountValue(emACC_VALUE_SAFECOIN), GetAccountValue(emACC_VALUE_SCORE), GetCity());
	}
	
	return true;
}


bool    CPlayer::PhpAtomChangeAccountValue(uint16 operType, uint16 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin)
{
	LOG_DEBUG("sta - uid:%d,operType:%d,coin:%lld,cur_coin:%lld", GetUID(), operType, coin, GetAccountValue(emACC_VALUE_COIN));

	if (!CanChangeAccountValue(emACC_VALUE_DIAMOND, diamond) || !CanChangeAccountValue(emACC_VALUE_COIN, coin) || !CanChangeAccountValue(emACC_VALUE_SCORE, score)
		|| !CanChangeAccountValue(emACC_VALUE_INGOT, ingot) || !CanChangeAccountValue(emACC_VALUE_CVALUE, cvalue) || !CanChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin))
	{
		return false;
	}
	bool ret = CDBMysqlMgr::Instance().AtomChangeAccountValue(GetUID(), diamond, coin, ingot, score, cvalue, safecoin);
	if (ret == false)
	{
		return false;
	}
	string chessid;
	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND, diamond);
	if (diamond != 0) {
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_DIAMOND, operType, subType, diamond, GetAccountValue(emACC_VALUE_DIAMOND) - diamond, GetAccountValue(emACC_VALUE_DIAMOND), chessid);
	}
	coin = ChangeAccountValue(emACC_VALUE_COIN, coin);
	if (coin != 0) 
	{
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_COIN, operType, subType, coin, GetAccountValue(emACC_VALUE_COIN) - coin, GetAccountValue(emACC_VALUE_COIN), chessid);
		if (operType == emACCTRAN_OPER_TYPE_BUY)
		{
			m_accInfo.recharge_actwle += coin;
		}
		if (operType == emACCTRAN_OPER_TYPE_CHUJIN)
		{
			m_accInfo.converts_actwle += abs(coin);
		}
		LOG_DEBUG("add coin - uid:%d,operType:%d,coin:%lld,cur_coin:%lld recharge_actwle:%lld converts_actwle:%lld", GetUID(), operType, coin, GetAccountValue(emACC_VALUE_COIN), m_accInfo.recharge_actwle, m_accInfo.converts_actwle);
	}
	score = ChangeAccountValue(emACC_VALUE_SCORE, score);
	if (score != 0) {
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_SCORE, operType, subType, score, GetAccountValue(emACC_VALUE_SCORE) - score, GetAccountValue(emACC_VALUE_SCORE), chessid);
	}
	ingot = ChangeAccountValue(emACC_VALUE_INGOT, ingot);
	if (ingot != 0) {
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_INGOT, operType, subType, ingot, GetAccountValue(emACC_VALUE_INGOT) - ingot, GetAccountValue(emACC_VALUE_INGOT), chessid);
	}
	cvalue = ChangeAccountValue(emACC_VALUE_CVALUE, cvalue);
	if (cvalue != 0) {
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_CVALUE, operType, subType, cvalue, GetAccountValue(emACC_VALUE_CVALUE) - cvalue, GetAccountValue(emACC_VALUE_CVALUE), chessid);
	}
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin);
	if (safecoin != 0) {
		CCenterLogMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_SAFECOIN, operType, subType, safecoin, GetAccountValue(emACC_VALUE_SAFECOIN) - safecoin, GetAccountValue(emACC_VALUE_SAFECOIN), chessid);
	}


	LOG_DEBUG("end - uid:%d,operType:%d,coin:%lld,cur_coin:%lld", GetUID(), operType, coin, GetAccountValue(emACC_VALUE_COIN));

	return true;
}