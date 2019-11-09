#include <data_cfg_mgr.h>

#include "game_player.h"
#include "helper/bufferStream.h"
#include "stdafx.h"
#include <time.h>
#include "game_table.h"
#include "game_room.h"
#include "active_welfare_mgr.h"

using namespace svrlib;
using namespace std;
using namespace Network;

namespace 
{	
	
}
CGamePlayer::CGamePlayer(uint8 type)
:CPlayerBase(type)
{
	m_pGameTable 	 = NULL;
	m_pGameRoom  	 = NULL;
	m_msgHeartTime   = getSysTime();
	m_autoReady		 = false;
	m_playDisconnect = false;
	m_bBankruptRecord = false;
    m_curr_aw_id = 0;
	m_lucky_read = false;
}
CGamePlayer::~CGamePlayer()
{

}
bool  	CGamePlayer::SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type,bool bSendRobot)
{
	if (IsRobot() && !bSendRobot)
	{
		return false;
	}
	//2027   4030 4053
	if (msg_type<= net::CMD_MSG_EVERY_COLOR_GAME_BEGIN || msg_type>= net::CMD_MSG_EVERY_COLOR_GAME_END)
	{
		if (IsPlayDisconnect() && msg_type > 3000)
		{
			LOG_DEBUG("断线重连未完成不发游戏消息 - uid:%d,msg_type:%d", GetUID(), msg_type);
			return false;
		}
	}


    CLobbyMgr::Instance().SendMsg2Client(msg,msg_type,GetUID());
    return true;
}    
bool	CGamePlayer::OnLoginOut(uint32 leaveparam)
{
	bool bLeaveGame = false;
	LOG_DEBUG("GamePlayer OnLoginOut:%d",GetUID());
	if (IsRobot())
	{
		if (m_pGameTable != NULL)
		{
			m_pGameTable->RobotLeavaReadJetton(GetUID());
		}
	}
	SetPlayerState(PLAYER_STATE_LOGINOUT);
	if(m_pGameTable != NULL)
	{
		LOG_DEBUG("play_leave - uid:%d", GetUID());
		bLeaveGame = m_pGameTable->LeaveTable(this);
	}
	else
	{
		bLeaveGame = true;
	}
	if(m_pGameRoom != NULL)
	{
		bLeaveGame = m_pGameRoom->LeaveRoom(this);
	}
	else
	{
		bLeaveGame = true;
	}
	FlushOnlineSvrIDToRedis(0);
	NotifyLeaveGame(leaveparam);

	if(!IsRobot())
	{
		CRedisMgr::Instance().SavePlayerBlockers(GetUID(), m_blocks);
		CRedisMgr::Instance().SavePlayerBlockerIPs(GetUID(), m_blockIPs);
	}
	return bLeaveGame;
}
void	CGamePlayer::OnLogin()
{	
	SetNetState(1);
	FlushOnlineSvrIDToRedis(CApplication::Instance().GetServerID());

	if(!IsRobot())
	{
		CRedisMgr::Instance().LoadPlayerBlockers(GetUID(), m_blocks);
		CRedisMgr::Instance().LoadPlayerBlockerIPs(GetUID(), m_blockIPs);
        CRedisMgr::Instance().GetPlayerActiveWelfareInfo(GetUID(), m_active_welfare_record);
		CRedisMgr::Instance().GetPlayerNewRegisterWelfareInfo(GetUID(), m_nrw_info);
	}

	//读取幸运值配置
	if (!m_lucky_read)
	{
		GetLuckyCfg();
	}
}
void	CGamePlayer::ReLogin()
{
	SetNetState(1);
	FlushOnlineSvrIDToRedis(CApplication::Instance().GetServerID());
}
void 	CGamePlayer::OnGameEnd()
{

}
// 更新游戏服务器ID到redis
void 	CGamePlayer::FlushOnlineSvrIDToRedis(uint16 svrID)
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
	CRedisMgr::Instance().SetPlayerOnlineSvrID(GetUID(),svrID);
}
// 通知返回大厅(退出游戏)
void	CGamePlayer::NotifyLeaveGame(uint16 gotoSvrid)
{
	LOG_DEBUG("notify leave game:%d->%d",GetUID(),gotoSvrid);

	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}

	net::msg_leave_svr msg;
	msg.set_uid(GetUID());
	msg.set_goto_svr(gotoSvrid);

	SendMsgToClient(&msg,S2L_MSG_LEAVE_SVR,true);

	if(gotoSvrid == 0){
		net::msg_back_lobby_rep rep;
		rep.set_result(1);
		SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP,true);
	}
}
// 请求大厅修改数值
void	CGamePlayer::NotifyLobbyChangeAccValue(int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid)
{
	CLobbyMgr::Instance().NotifyLobbyChangeAccValue(GetUID(),operType,subType,diamond,coin,ingot,score,cvalue,safeCoin,chessid);
}
// 修改玩家账号数值（增量修改）
bool    CGamePlayer::SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid)
{
	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND,diamond);
	coin    = ChangeAccountValue(emACC_VALUE_COIN,coin);
	score   = ChangeAccountValue(emACC_VALUE_SCORE,score);
	ingot   = ChangeAccountValue(emACC_VALUE_INGOT,ingot);
	cvalue  = ChangeAccountValue(emACC_VALUE_CVALUE,cvalue);
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN,safecoin);
	NotifyLobbyChangeAccValue(operType,subType,diamond,coin,ingot,score,cvalue,safecoin,chessid);
	return true;
}
// 修改玩家账号数值（减少修改）
void    CGamePlayer::SubChangeAccountValue(uint16 operType, uint16 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, const string& chessid)
{
	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND, diamond);
	coin = ChangeAccountValue(emACC_VALUE_COIN, coin);
	score = ChangeAccountValue(emACC_VALUE_SCORE, score);
	ingot = ChangeAccountValue(emACC_VALUE_INGOT, ingot);
	cvalue = ChangeAccountValue(emACC_VALUE_CVALUE, cvalue);
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin);

	CLobbyMgr::Instance().UpDateLobbyChangeAccValue(GetUID(), operType, subType, diamond, coin, ingot, score, cvalue, safecoin, chessid);

}
// 能否退出
bool 	CGamePlayer::CanBackLobby()
{
	if(m_pGameTable != NULL	&& !m_pGameTable->CanLeaveTable(this))
	{
		return false;
	}

	return true;
}
int 	CGamePlayer::OnGameMessage(uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if(m_pGameTable != NULL)
	{
		m_pGameTable->OnGameMessage(this,cmdID,pkt_buf,buf_len);
	}
	ResetHeart();
	return 0;
}
// 重置心跳时间
void	CGamePlayer::ResetHeart()
{
	m_msgHeartTime = getSysTime();
}
void  	CGamePlayer::OnTimeTick(uint64 uTime,bool bNewDay)
{
	if((getSysTime() - m_msgHeartTime) > SECONDS_IN_MIN*60)
	{// 10分钟没有收到消息算掉线
		if(m_pGameTable == NULL || m_pGameTable->CanLeaveTable(this)) {
			SetNetState(0);
		}
	}
	if (bNewDay)
	{
		UpdateWelCountByTime();
	}
}
// 是否需要回收
bool 	CGamePlayer::NeedRecover()
{
	if(GetNetState() == 0 || CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		if (CanBackLobby())
		{
			LOG_DEBUG("level_table_in_lobby - uid:%d", GetUID());
			return true;
		}
			
	}
	return false;
}

bool CGamePlayer::IsCanLook()
{
	uint16 gameType = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (CCommonLogic::IsBaiRenNewPlayerNoviceWelfare(gameType) == false)
	{
		// 没有加新房间的都能看到
		return true;
	}
	if (m_pGameRoom == NULL || m_pGameTable == NULL)
	{
		return false;
	}
	if (m_pGameRoom->GetNoviceWelfare() == 0)
	{
		// 不是新手房的都能看到
		return true;
	}
	return false;
}


// 是否正在游戏中
bool 	CGamePlayer::IsInGamePlaying()
{
	if(m_pGameTable == NULL)
		return false;
	if(m_pGameTable->GetGameState() != TABLE_STATE_FREE || m_pGameTable->IsReady(this))
		return true;

	return false;
}
// 更新数值到桌子
void 	CGamePlayer::FlushAccValue2Table()
{
	if(m_pGameTable != NULL){
		m_pGameTable->SendSeatInfoToClient(NULL);
	}
}
// 修改游戏数值并更新到数据库
void 	CGamePlayer::AsyncChangeGameValue(uint16 gameType,bool isCoin,int64 winScore, int64 lExWinScore,int rwelfare)
{
    int32 lose = (winScore > 0) ? 0 : 1;
    int32 win = (winScore > 0) ? 1 : 0;
    ChangeGameValue(gameType,isCoin, win, lose, winScore);
	if (rwelfare == 1)
	{
		UpdateWelCount(win);
	}
	CDBMysqlMgr::Instance().ChangeGameValue(gameType, GetUID(),IsRobot(), isCoin, win, lose, winScore, lExWinScore, GetGameMaxScore(gameType, isCoin), rwelfare, GetWelCount());
}

void    CGamePlayer::AsyncSetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount)
{
    SetGameMaxCard(gameType,isCoin,cardData,cardCount);
    CDBMysqlMgr::Instance().UpdateGameMaxCard(gameType,GetUID(),isCoin,cardData,cardCount);        
}

void 	CGamePlayer::AsyncChangeGameValueForFish(bool isCoin, int64 winScore)
{
	int32 lose = (winScore > 0) ? 0 : 1;
	int32 win = (winScore > 0) ? 1 : 0;
	ChangeGameValue(net::GAME_CATE_FISHING, isCoin, win, lose, winScore);
	//LOG_DEBUG("uid:%d win:%lld ", GetUID(), m_accInfo.win);
	CDBMysqlMgr::Instance().ChangeGameValueForFish(GetUID(), isCoin, winScore, GetWelCount());
}

uint8	CGamePlayer::GetNetState()
{
	return m_netState;
}
void	CGamePlayer::SetNetState(uint8 state)
{
	m_netState = state;
	LOG_DEBUG("设置网络状态 uid:%d,state:%d,m_pGameTable:%p", GetUID(), state, m_pGameTable);

	if(m_pGameTable != NULL)
	{
		m_pGameTable->OnActionUserNetState(this,m_netState,false);
		if(state == 0)
		{
			if(m_pGameTable->CanLeaveTable(this))
			{
				LOG_DEBUG("play_leave - uid:%d", GetUID());
				if(m_pGameTable->LeaveTable(this))
				{
					m_pGameRoom->LeaveRoom(this);
				}
			}
		}
	}else{
		SetPlayDisconnect(false);
	}
}
uint16  CGamePlayer::GetRoomID()
{
	if(m_pGameRoom != NULL){
		return m_pGameRoom->GetRoomID();
	}
	return 0;
}
void	CGamePlayer::SetTable(CGameTable* pTable)
{
	m_pGameTable = pTable;
	if(m_pGameTable == NULL){
		SetPlayDisconnect(false);
	}
}
uint32  CGamePlayer::GetTableID()
{
	if(m_pGameTable != NULL){
		return m_pGameTable->GetTableID();
	}
	return 0;
}
void 	CGamePlayer::AddBlocker(uint32 uid) {
	if(uid != 0 && uid != GetUID())
	{
		m_blocks.push_back(uid);
		//LOG_DEBUG("添加黑名单:%d-->%d",GetUID(),uid);
	}
}
bool 	CGamePlayer::IsExistBlocker(uint32 uid)
{
	if(uid == 0)
		return false;
	for(uint32 i=0;i<m_blocks.size();++i){
		if(m_blocks[i] == uid)
			return true;
	}
	return false;
}
void 	CGamePlayer::ClearBlocker()
{
	m_blocks.clear();
	//LOG_DEBUG("清理黑名单:%d",GetUID());
}
void 	CGamePlayer::AddBlockerIP(uint32 ip)
{
	//return;
	if(ip != 0 && ip != GetIP()){
		m_blockIPs.push_back(ip);
		//LOG_DEBUG("添加黑名单IP:%d-->%d",GetUID(),ip);
	}
}
bool 	CGamePlayer::IsExistBlockerIP(uint32 ip)
{
	if(ip == 0)
		return false;
	for(uint32 i=0;i<m_blockIPs.size();++i){
		if(m_blockIPs[i] == ip)
			return true;
	}
	return false;
}
void 	CGamePlayer::ClearBlockerIP()
{
	m_blockIPs.clear();
	//LOG_DEBUG("清理黑名单IP:%d",GetUID());
}

// 修改游戏玩家库存并更新到数据库
void 	CGamePlayer::AsyncChangeStockScore(uint16 gametype,int64 winScore)
{
	ChangeStockScore(gametype,winScore);
	CDBMysqlMgr::Instance().ChangeStockScore(gametype,GetUID(), winScore);
}

// 根据当前亏损的区间ID获取已经补助的金额----用于活跃福利功能
uint64 	CGamePlayer::GetCurrAWInfo(uint8 aw_id)
{
    uint64 ret = 0;
    map<uint8, uint64>::iterator iter = m_active_welfare_record.find(aw_id);
    if (iter != m_active_welfare_record.end())
    {
        ret = iter->second;
    }
    return ret;
}

// 设置当前亏损的区间已经补助的金额----用于活跃福利功能
void	CGamePlayer::SetCurrAWInfo(uint8 aw_id, uint64 add_count)
{
    uint64 sum = 0;
    map<uint8, uint64>::iterator iter = m_active_welfare_record.find(aw_id);
    if (iter != m_active_welfare_record.end())
    {
        iter->second += add_count;
        sum = iter->second;
    }
    else
    {
        m_active_welfare_record[aw_id] = add_count;
        sum = add_count;
    }

    // 更新对应的redis中值
    CRedisMgr::Instance().SetPlayerActiveWelfareInfo(GetUID(), aw_id, sum);

    return ;
}

//// 将当前亏损的区间的补助金额清零----用于活跃福利功能
//void	CGamePlayer::ClearCurrAWInfo(uint8 aw_id)
//{
//    m_active_welfare_record[aw_id] = 0;
//      
//    // 更新对应的redis中值
//    CRedisMgr::Instance().SetPlayerActiveWelfareInfo(GetUID(), aw_id, 0);    
//}

// 判断玩家是否可以进入活跃福利房----用于活跃福利功能
bool	CGamePlayer::IsEnterActiveWelfareRoomFlag(uint8 game_type)
{
    //判断玩家是否亏损 --- 玩家亏损=自身携带+保险箱+提现-充值
    int64 tmp_loss = GetCurrLoss();
    LOG_DEBUG("IsEnterActiveWelfareRoomFlag uid:%d tmp_loss:%d coin:%d safecoin:%d converts_actwle:%d recharge_actwle:%d", GetUID(), tmp_loss, m_accInfo.coin, m_accInfo.safecoin, m_accInfo.converts_actwle, m_accInfo.recharge_actwle);

    if (tmp_loss > 0)
    {
        return false;
    }
    else
    {       
        return CAcTiveWelfareMgr::Instance().GetPlayerActiveWelfareFlag(GetUID(), abs(tmp_loss), game_type);
    }   
}

bool CGamePlayer::IsNewRegisterPlayerWelfareFlag(uint8 game_id)
{
	bool bInGameFlag = CNewRegisterWelfareMgr::Instance().IsExistGame(game_id);
	uint64 register_time = m_baseInfo.rtime;
	uint8 ispay = m_baseInfo.ispay;
	uint8 period_day = CNewRegisterWelfareMgr::Instance().GetPeriodDay();

	LOG_DEBUG("IsNewRegisterPlayer - uid:%d,register_time:%lld,ispay:%d,isRobot:%d period_day:%d bInGameFlag:%d", GetUID(), register_time, ispay, IsRobot(), period_day, bInGameFlag);

	//判断是否为机器人以及是否为充值玩家
	if (ispay || IsRobot() || !bInGameFlag)
	{
		return false;
	}

	//判断有效期
	uint64 curr_time = getSysTime();
	if (curr_time > (SECONDS_IN_ONE_DAY * period_day + register_time))
	{
		return false;
	}
		
	uint32 cfg_must_win = CNewRegisterWelfareMgr::Instance().GetMustWin();
	uint32 cfg_total_win = CNewRegisterWelfareMgr::Instance().GetTotalWin();
	
	LOG_DEBUG("IsNewRegisterPlayer - uid:%d curr_must_win_number:%d curr_total_win_number:%d cfg_must_win:%d cfg_total_win:%d ",
		GetUID(), m_nrw_info.curr_must_win_number, m_nrw_info.curr_total_win_number, cfg_must_win, cfg_total_win);
	
	//判断必赢局数是否满足
	if (cfg_total_win <= 0)
	{
		if (m_nrw_info.curr_must_win_number >= cfg_must_win)
		{
			return false;
		}
	}	
	return true;
}

void    CGamePlayer::UpdateNewRegisterPlayerInfo(int64 win_coin)
{
	LOG_DEBUG("UpdateNewRegisterPlayerInfo uid:%d win_coin:%lld", GetUID(), win_coin);

	uint32 cfg_must_win = CNewRegisterWelfareMgr::Instance().GetMustWin();
	uint32 cfg_total_win = CNewRegisterWelfareMgr::Instance().GetTotalWin();
			
	//判断时间是否跨天,如果跨天,则需要清除累计的总赢取次数
	uint64 uTime = getTime();
	bool bNewDay = (diffTimeDay(m_nrw_info.last_update_time, uTime) != 0);
	if (bNewDay)
	{
		m_nrw_info.curr_total_win_number = 0;
	}

	if (win_coin > 0)
	{
		//更新局数	
		if (m_nrw_info.curr_must_win_number < cfg_must_win)
		{
			m_nrw_info.curr_must_win_number++;
		}
		else
		{
			m_nrw_info.curr_total_win_number++;
		}
	}

	//更新操作时间
	m_nrw_info.last_update_time = uTime;

	//更新赢取值并同步到redis中
	UpdateNRWPlayerScore(win_coin);	

	LOG_DEBUG("UpdateNewRegisterPlayerInfo - uid:%d,curr_must_win_number:%d,curr_total_win_number:%d,cfg_must_win:%d cfg_total_win:%d ",
		GetUID(), m_nrw_info.curr_must_win_number, m_nrw_info.curr_total_win_number, cfg_must_win, cfg_total_win);
}

bool  CGamePlayer::GetNewRegisterWelfareStatus(int &status)
{
	status = 0;  //0:不指定牌型赢 1:表示指定牌型赢 2:表示输

	uint32 cfg_must_win = CNewRegisterWelfareMgr::Instance().GetMustWin();
	uint32 cfg_total_win = CNewRegisterWelfareMgr::Instance().GetTotalWin();
	uint32 cfg_total_win_rate = CNewRegisterWelfareMgr::Instance().GetTotalWinRate();
	uint32 cfg_min_win_coin = CNewRegisterWelfareMgr::Instance().GetMinWinCoin();
	uint32 cfg_max_win_coin = CNewRegisterWelfareMgr::Instance().GetMaxWinCoin();

	LOG_DEBUG("uid:%d curr_must_win_number:%d curr_total_win_number:%d curr_total_win_coin:%d is_reach_max_win_coin:%d is_reach_min_win_coin:%d cfg_must_win:%d cfg_total_win:%d cfg_total_win_rate:%d cfg_min_win_coin:%d cfg_max_win_coin:%d",
		GetUID(), m_nrw_info.curr_must_win_number, m_nrw_info.curr_total_win_number, m_nrw_info.curr_total_win_coin, m_nrw_info.is_reach_max_win_coin, m_nrw_info.is_reach_min_win_coin, cfg_must_win, cfg_total_win, cfg_total_win_rate, cfg_min_win_coin, cfg_max_win_coin);
	
	//判断必赢局数	
	if (m_nrw_info.curr_must_win_number < cfg_must_win)
	{
		//玩家赢取金额大于最大赢取金额 
		if (m_nrw_info.curr_total_win_coin >= cfg_max_win_coin)
		{
			LOG_DEBUG("1 The current player current_total_win is more than cfg_max_win.- uid:%d", GetUID());
			status = 2;
			return true;
		}
		else
		{
			//玩家赢取金额处于[min_win,max_win]之间时
		    // 如果曾经赢取超过最大赢取, 则必须输到小于最小赢取才能返回真, 如果一直都没有超过最大赢取, 只要小于最大赢取, 就返回真
			if (m_nrw_info.is_reach_max_win_coin == 1) //如果从最大赢取往下走，则走房间的正常配置
			{
				LOG_DEBUG("7 The current player curr_must_win_number is less then min cfg. is_reach_max_win_coin is ture then return false. uid:%d", GetUID());
				return false;
			}
			if (m_nrw_info.is_reach_min_win_coin == 1)	//如果从最小赢取往上走，则走让玩家赢的逻辑
			{
				status = 1;
				LOG_DEBUG("8 The current player curr_must_win_number is less then min cfg. is_reach_min_win_coin is ture. then player is win. uid:%d", GetUID());
				return true;
			}			
			return true;
		}		
	}

	//判断时间是否跨天,如果跨天,则需要清除累计的总赢取次数
	uint64 uTime = getTime();
	bool bNewDay = (diffTimeDay(m_nrw_info.last_update_time, uTime) != 0);
	if (bNewDay)
	{
		m_nrw_info.curr_total_win_number = 0;
		LOG_DEBUG("7 NewDay The set current_total_win is zero.- uid:%d", GetUID());
	}
			
	//判断总赢取局数
	if (m_nrw_info.curr_total_win_number < cfg_total_win)
	{
		//先判断当前控制玩家是否在配置概率范围内
		bool tmp = g_RandGen.RandRatio(cfg_total_win_rate, PRO_DENO_10000);
		if (!tmp)
		{
			LOG_DEBUG("2 The current player is not in config rate - uid:%d tmp:%d probability:%d", GetUID(), tmp, cfg_total_win_rate);
			return false;
		}

		//玩家赢取金额大于最大赢取金额 
		if (m_nrw_info.curr_total_win_coin > cfg_max_win_coin)
		{
			LOG_DEBUG("3 The current player current_total_win is more than cfg_max_win.- uid:%d", GetUID());
			status = 2;
			return true;
		}

		//玩家赢取金额小于最小赢取金额
		if (m_nrw_info.curr_total_win_coin < cfg_min_win_coin)
		{
			status = 0;
			LOG_DEBUG("4 The current player in config rate - uid:%d tmp:%d probability:%d", GetUID(), tmp, cfg_total_win_rate);
			return true;
		}

		//玩家赢取金额处于[min_win,max_win]之间时
		//- - 如果曾经赢取超过最大赢取, 则必须输到小于最小赢取才能返回真, 如果一直都没有超过最大赢取, 只要小于最大赢取, 就返回真
		if (m_nrw_info.is_reach_max_win_coin == 1) //如果从最大赢取往下走，则走房间的正常配置
		{						
			LOG_DEBUG("5 The current player is_reach_max_win_coin is ture then return false. uid:%d", GetUID());
			return false;			
		}
		if (m_nrw_info.is_reach_min_win_coin == 1)	//如果从最小赢取往上走，则走让玩家赢的逻辑
		{			
			status = 0;
			LOG_DEBUG("6 The current player is_reach_min_win_coin is ture. then player is win. uid:%d", GetUID());
			return true;			
		}		
	}
	LOG_ERROR(" The current player total win is over cfg - uid:%d.", GetUID());
	return false;
}

void    CGamePlayer::UpdateNRWPlayerScore(int64 coin)
{
	LOG_DEBUG("UpdateNRWPlayerScore uid:%d coin:%lld", GetUID(), coin);
	
	uint32 cfg_min_win_coin = CNewRegisterWelfareMgr::Instance().GetMinWinCoin();
	uint32 cfg_max_win_coin = CNewRegisterWelfareMgr::Instance().GetMaxWinCoin();
		
	//更新赢取值
	if (coin<0 && (uint64)(abs(coin))> m_nrw_info.curr_total_win_coin)
	{
		m_nrw_info.curr_total_win_coin = 0;
		m_nrw_info.is_reach_min_win_coin = 1;
		m_nrw_info.is_reach_max_win_coin = 0;
	}
	else
	{
		m_nrw_info.curr_total_win_coin += coin;
		if (m_nrw_info.curr_total_win_coin < cfg_min_win_coin)
		{
			m_nrw_info.is_reach_min_win_coin = 1;
			m_nrw_info.is_reach_max_win_coin = 0;
		}
	}
	if (m_nrw_info.curr_total_win_coin > cfg_max_win_coin)
	{
		m_nrw_info.is_reach_max_win_coin = 1;
		m_nrw_info.is_reach_min_win_coin = 0;
	}
	
	CRedisMgr::Instance().SetPlayerNewRegisterWelfareInfo(GetUID(), m_nrw_info);

	LOG_DEBUG("UpdateNRWPlayerScore - uid:%d,curr_total_win_coin:%d is_reach_max_win_coin:%d is_reach_min_win_coin:%d cfg_min_win_coin:%d cfg_max_win_coin:%d",
		GetUID(), m_nrw_info.curr_total_win_coin, m_nrw_info.is_reach_max_win_coin, m_nrw_info.is_reach_min_win_coin, cfg_min_win_coin, cfg_max_win_coin);
}

// 读取当前玩家的幸运值配置信息
void	CGamePlayer::GetLuckyCfg()
{
	uint16 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (!CCommonLogic::IsLuckyFuncitonGame(gametype))
	{
		LOG_ERROR("The current gametype is not deal lucky function. - uid:%d gametype:%d.", GetUID(), gametype);
		return;
	}
	CDataCfgMgr::Instance().LoadLuckyConfig(GetUID(), gametype, m_lucky_info);
	LOG_DEBUG("Get current player uid:%d lucky info. gametype:%d result size:%d.", GetUID(), gametype, m_lucky_info.size());
	m_lucky_read = true;
	return;	
}

//根据roomid获取当前玩家是否触发幸运值---针对非捕鱼
bool	CGamePlayer::GetLuckyFlag(uint16 roomid, bool &flag)
{
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - uid:%d roomid:%d", GetUID(), roomid);
		return false;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero - uid:%d", GetUID());
			return false;
		}
		else
		{
			if (g_RandGen.RandRatio(iter_exist->second.rate, PRO_DENO_100))
			{
				//根据幸运值来判断当前控制的赢或输
				if (iter_exist->second.lucky_value > 0)
				{
					flag = true;
				}
				else
				{
					flag = false;
				}
				LOG_DEBUG("get player lucky is success. uid:%d flag:%d", GetUID(), flag);
				return true;
			}
			else
			{
				LOG_DEBUG("the rand is fail. uid:%d rate:%d", GetUID(), iter_exist->second.rate);
				return false;
			}
		}
	}
	return false;
}

//设置当前玩家的幸运值统计信息
void	CGamePlayer::SetLuckyInfo(uint16 roomid, int32 add_value)
{
	LOG_DEBUG("update lucky info. uid:%d roomid:%d add_value:%d", GetUID(), roomid, add_value);
	if(add_value==0)
	{
		LOG_DEBUG("the add_value is zero.");
		return;
	}
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - roomid:%d", roomid);
		return;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero.");
			return;
		}

		//如果控制当前玩家为赢，则只记录赢的牌局信息
		if (iter_exist->second.lucky_value > 0 && add_value < 0)
		{
			LOG_DEBUG("the set lucky player is win but current result is lose.");
			return;
		}

		//如果控制当前玩家为输，则只记录输的牌局信息
		if (iter_exist->second.lucky_value < 0 && add_value > 0)
		{
			LOG_DEBUG("the set lucky player is lose but current result is win.");
			return;
		}
						
		//取更新前的幸运值
		int64 before_lucky = iter_exist->second.lucky_value;

		//更新累计值与幸运值
		iter_exist->second.accumulated += add_value;
		iter_exist->second.lucky_value -= add_value;

		//赢的情况---完成条件
		if (before_lucky > 0 && iter_exist->second.lucky_value <= 0)
		{				    
			iter_exist->second.lucky_value = 0;
			iter_exist->second.rate = 0;				
		}

		//输的情况---完成条件
		if (before_lucky < 0 && iter_exist->second.lucky_value >= 0)
		{				
			iter_exist->second.lucky_value = 0;
			iter_exist->second.rate = 0;				
		}
		
		LOG_DEBUG("update lucky info after. uid:%d roomid:%d lucky_value:%d rate:%d accumulated:%d", 
			GetUID(), roomid, iter_exist->second.lucky_value, iter_exist->second.rate, iter_exist->second.accumulated);

		//更新到数据库
		uint16 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
		CDataCfgMgr::Instance().UpdateLuckyInfo(GetUID(), gametype, roomid, iter_exist->second);
	}
	return;
}

//根据roomid获取当前玩家是否触发幸运值---针对捕鱼
bool	CGamePlayer::GetLuckyFlagForFish(uint16 roomid, bool &flag, uint32 &rate)
{
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - uid:%d roomid:%d", GetUID(), roomid);
		return false;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero - uid:%d", GetUID());
			return false;
		}
		else
		{			
			//根据幸运值来判断当前控制的赢或输
			if (iter_exist->second.lucky_value > 0)
			{
				flag = true;
			}
			else
			{
				flag = false;
			}
			rate = iter_exist->second.rate;
			LOG_DEBUG("get player lucky is success. uid:%d flag:%d rate:%d", GetUID(), flag, rate);
			return true;			
		}
	}
	return false;
}

//重置当前玩家的幸运值信息---每天凌晨
void	CGamePlayer::ResetLuckyInfo()
{
	LOG_DEBUG("reset lucky info. uid:%d", GetUID());
	
	map<uint8, tagUserLuckyCfg>::iterator iter = m_lucky_info.begin();
	for(;iter!= m_lucky_info.end();iter++)
	{
		iter->second.lucky_value = 0;
		iter->second.accumulated = 0;
		iter->second.rate = 0;
	}	
	return;
}