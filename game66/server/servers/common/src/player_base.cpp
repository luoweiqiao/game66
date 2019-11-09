

#include "player_base.h"
#include "center_log.h"
#include "data_cfg_mgr.h"
#include "json/json.h"
#include "utility/timeFunction.h"
#include "db_struct_define.h"
#include "common_logic.h"

using namespace std;
using namespace svrlib;
using namespace Network;

namespace
{

};
CPlayerBase::CPlayerBase(uint8 type)
:m_bPlayerType(type)
,m_pSession(NULL)
,m_bPlayerState(PLAYER_STATE_NULL)
{
	m_uid = 0;	
    memset(m_loadState,0,sizeof(m_loadState));
    memset(m_loadGameState,0,sizeof(m_loadGameState));
    
    memset(m_gameInfo,0,sizeof(m_gameInfo));

	m_batchID = 0;
	m_strCity = "";
	m_strLoginIP = "";

	m_defeat = 0;
	m_interval = 0;
}
CPlayerBase::~CPlayerBase()
{
	//LOG_DEBUG("desCPlayerBase_1 - m_pSession:%p,uid:%d,", m_pSession, GetUID());

    if(m_pSession != NULL)
	{
		if (!IsRobot())
		{
			LOG_DEBUG("desCPlayerBase_2 - m_pSession:%p,uid:%d,", m_pSession, m_pSession->GetUID());
		}
        m_pSession->DestroyObj();
        m_pSession = NULL;
    }
}
bool  CPlayerBase::SetBaseInfo(stPlayerBaseInfo& info)
{
    m_baseInfo = info;
    SetLoadState(emACCDATA_TYPE_BASE,1);
    return true;
}
bool  CPlayerBase::SetAccountInfo(stAccountInfo& info)
{
    m_accInfo = info;
    SetLoadState(emACCDATA_TYPE_ACC,1);
    return true;
}
bool  CPlayerBase::SetGameInfo(uint16 gameType,const stGameBaseInfo& info)
{
	//LOG_DEBUG("SetGameInfo - uid:%d gameType:%d", GetUID(), gameType);
    m_gameInfo[gameType] = info;
    m_loadGameState[gameType] = 1;
    bool isAllLoad = true;
    for(uint16 i=1;i<net::GAME_CATE_MAX_TYPE;++i){
        if(m_loadGameState[i] == 0){
            isAllLoad = false;
            break;
        }
    }
    if(isAllLoad){
        SetLoadState(emACCDATA_TYPE_GAME,1);
    }    
    return true;
}
bool  CPlayerBase::IsLoadData(uint8 dataType)
{    
    if(dataType < emACCDATA_TYPE_MAX){
        return m_loadState[dataType];
    }
    return false;
}
void  CPlayerBase::SetLoadState(uint8 dataType,uint8 state)
{
    if(dataType < emACCDATA_TYPE_MAX){
        m_loadState[dataType] = state;
    }
}
bool CPlayerBase::IsLoadOver()
{
    for(uint8 i=emACCDATA_TYPE_BASE;i<emACCDATA_TYPE_MAX;++i)
    {
        if(m_loadState[i] == 0)
            return false;
    }
    return true;
}  
void  CPlayerBase::SetPlayerState(uint8 state)
{
	m_bPlayerState = state;
}
uint8 CPlayerBase::GetPlayerState()
{
	return m_bPlayerState;
}
bool  CPlayerBase::IsPlaying()
{
	return  m_bPlayerState == PLAYER_STATE_PLAYING;
}
uint32	CPlayerBase::GetUID()
{
	return m_uid;
}
void	CPlayerBase::SetUID(uint32 uid)
{
	m_uid = uid;
}
bool    CPlayerBase::IsRobot()
{
    return (m_bPlayerType == PLAYER_TYPE_ROBOT);
}
string 	CPlayerBase::GetPlayerName()
{
	return m_baseInfo.name;
}
void 	CPlayerBase::SetPlayerName(string name)
{
    m_baseInfo.name = name;
}
uint8	CPlayerBase::GetSex()
{
    return m_baseInfo.sex;
}
void 	CPlayerBase::SetSex(uint8 sex)
{
    m_baseInfo.sex = sex;
}
uint16	CPlayerBase::GetHeadIcon()
{
    return m_baseInfo.headIcon;
}
void  	CPlayerBase::SetSession(NetworkObject* pSession)
{
    m_pSession = pSession;
}
NetworkObject* CPlayerBase::GetSession()
{
    return m_pSession;
}
void CPlayerBase::SetIP(string strIP)
{
	if (strIP.empty())
	{
		return;
	}
	uint32 uloginip = inet_addr(strIP.c_str());
	if (uloginip == INADDR_NONE)
	{
		return;
	}
	m_strLoginIP = strIP;
	m_baseInfo.loginIP = uloginip;
}
uint32	CPlayerBase::GetIP()
{
 //   if(m_pSession){
	//	return m_pSession->GetIP();
	//}
    return m_baseInfo.loginIP;
}

int32	CPlayerBase::GetVipLevel()
{
	return m_baseInfo.level;
}

string  CPlayerBase::GetIPStr()
{
    if(m_pSession){
		return m_strLoginIP;// m_pSession->GetSIP();
	}
	return " ";
}
bool  	CPlayerBase::SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type)
{
    if(m_pSession)
    {
        return SendProtobufMsg(m_pSession,msg,msg_type);
    }
    return false;
}
bool  	CPlayerBase::SendMsgToClient(const void *msg, uint16 msg_len, uint16 msg_type)
{
    if(m_pSession){
        return SendProtobufMsg(m_pSession, msg, msg_len, msg_type);
    }
    return false;
}
bool 	CPlayerBase::OnLoginOut(uint32 leaveparam)
{
	return true;
}
void  	CPlayerBase::OnLogin()
{

}
void	CPlayerBase::OnGetAllData()
{

}
void	CPlayerBase::ReLogin()
{

}
void 	CPlayerBase::OnTimeTick(uint64 uTime,bool bNewDay)
{

}
// 是否需要回收
bool 	CPlayerBase::NeedRecover()
{
    return false;
}

bool    CPlayerBase::IsCanLook()
{
	return false;
}

void	CPlayerBase::GetPlayerBaseData(net::base_info* pInfo)
{
    pInfo->set_uid(GetUID());
    pInfo->set_name(m_baseInfo.name);
    pInfo->set_sex(m_baseInfo.sex);
    pInfo->set_safeboxstate(m_baseInfo.safeboxState);
    pInfo->set_clogin(m_baseInfo.clogin);
    pInfo->set_weeklogin(m_baseInfo.weekLogin);
    pInfo->set_reward(m_baseInfo.reward);
    pInfo->set_bankrupt(m_baseInfo.bankrupt);

    pInfo->set_diamond(m_accInfo.diamond);
    pInfo->set_coin(m_accInfo.coin);
    pInfo->set_score(m_accInfo.score);
    pInfo->set_ingot(m_accInfo.ingot);
    pInfo->set_cvalue(m_accInfo.cvalue);
    pInfo->set_safe_coin(m_accInfo.safecoin);
    pInfo->set_vip(m_accInfo.vip);
    pInfo->set_head_icon(m_baseInfo.headIcon);
    pInfo->set_day_game_count(m_baseInfo.dayGameCount);
    pInfo->set_login_ip(m_baseInfo.loginIP);
	pInfo->set_rtime(m_baseInfo.rtime);
	pInfo->set_ispay(m_baseInfo.ispay);


	pInfo->set_city(GetCity());
	pInfo->set_recharge(m_accInfo.recharge);
	pInfo->set_converts(m_accInfo.converts);
	pInfo->set_batchid(m_batchID);
	pInfo->set_lvscore(m_lvScore);
	pInfo->set_lvcoin(m_lvCoin);
	//pInfo->set_rtime(m_baseInfo.rtime);
	pInfo->set_postime(m_accInfo.postime);
	pInfo->set_userright(m_accInfo.userright);
	pInfo->set_win(m_accInfo.win);
	pInfo->set_posrmb(m_accInfo.posrmb);
	pInfo->set_welcount(m_accInfo.welcount);
	pInfo->set_weltime(m_accInfo.weltime);

    pInfo->set_recharge_actwle(m_accInfo.recharge_actwle);
    pInfo->set_converts_actwle(m_accInfo.converts_actwle);
}

void	CPlayerBase::SetPlayerBaseData(const net::base_info& info)
{
    SetUID(info.uid());
    m_baseInfo.name         = info.name();
    m_baseInfo.sex          = info.sex();
    m_baseInfo.safeboxState = info.safeboxstate();
    m_baseInfo.clogin       = info.clogin();
    m_baseInfo.weekLogin    = info.weeklogin();
    m_baseInfo.reward       = info.reward();
    m_baseInfo.bankrupt     = info.bankrupt();
    m_baseInfo.headIcon     = info.head_icon();
    m_baseInfo.dayGameCount = info.day_game_count();
    m_baseInfo.loginIP      = info.login_ip();
	m_baseInfo.rtime        = info.rtime();
	m_baseInfo.ispay        = info.ispay();

    m_accInfo.diamond       = info.diamond();
    m_accInfo.coin          = info.coin();
    m_accInfo.score         = info.score();
    m_accInfo.ingot         = info.ingot();
    m_accInfo.cvalue        = info.cvalue();
    m_accInfo.safecoin      = info.safe_coin();
    m_accInfo.vip           = info.vip();

	SetCity(info.city());
	m_accInfo.recharge = info.recharge();
	m_accInfo.converts = info.converts();
	SetBatchID(info.batchid());
	SetLvScore(info.lvscore());
	SetLvCoin(info.lvcoin());
	m_accInfo.postime = info.postime();
	m_accInfo.userright = info.userright();
	m_accInfo.win = info.win();
	m_accInfo.posrmb = info.posrmb();
	m_accInfo.welcount = info.welcount();
	m_accInfo.weltime = info.weltime();
    m_accInfo.recharge_actwle = info.recharge_actwle();
    m_accInfo.converts_actwle = info.converts_actwle();

}
void    CPlayerBase::GetGameData(uint16 gameType,net::game_data_info* pInfo)
{
    stGameBaseInfo& info = m_gameInfo[gameType];
    pInfo->set_game_type(gameType);
    pInfo->set_win(info.win);
    pInfo->set_lose(info.lose);
    pInfo->set_maxwin(info.maxwin);
    pInfo->set_winc(info.winc);
    pInfo->set_losec(info.losec);
    pInfo->set_maxwinc(info.maxwinc);
    pInfo->set_daywin(info.daywin);
    pInfo->set_daywinc(info.daywinc);

    pInfo->set_land(info.land);
    pInfo->set_spring(info.spring);
    pInfo->set_landc(info.landc);
    pInfo->set_springc(info.springc);
	pInfo->set_weekwinc(info.weekwinc);
	pInfo->set_totalwinc(info.totalwinc);
	pInfo->set_stockscore(info.stockscore);
	pInfo->set_gamecount(info.gamecount);
    for(uint8 i=0;i<5;++i){
        pInfo->add_maxcard(info.bestCard[i]);
        pInfo->add_maxcardc(info.bestCardc[i]);
    }    
}
void    CPlayerBase::SetGameData(uint16 gameType,const net::game_data_info& refInfo)
{
    stGameBaseInfo& info = m_gameInfo[gameType]; 
    info.win      = refInfo.win();
    info.lose     = refInfo.lose();
    info.maxwin   = refInfo.maxwin();
    info.winc     = refInfo.winc();
    info.losec    = refInfo.losec();
    info.maxwinc  = refInfo.maxwinc();
    info.daywin   = refInfo.daywin();
    info.daywinc  = refInfo.daywinc();

    info.land     = refInfo.land();
    info.spring   = refInfo.spring();
    info.landc    = refInfo.landc();
    info.springc   = refInfo.springc();
	info.weekwinc = refInfo.weekwinc();
	info.totalwinc = refInfo.totalwinc();
	info.stockscore = refInfo.stockscore();
	info.gamecount = refInfo.gamecount();
    for(uint8 i=0;i<refInfo.maxcard_size() && i<5;++i){
        info.bestCard[i] = refInfo.maxcard(i);
    }
    for(uint8 i=0;i<refInfo.maxcardc_size() && i<5;++i){
        info.bestCardc[i] = refInfo.maxcardc(i);
    }
}
void 	CPlayerBase::SetPlayerGameData(const net::msg_enter_into_game_svr& info)
{
    SetPlayerBaseData(info.base_data());
    SetGameData(info.game_data().game_type(),info.game_data());
    
}
void	CPlayerBase::GetPlayerGameData(uint16 gameType,net::msg_enter_into_game_svr* pInfo)
{
    GetPlayerBaseData(pInfo->mutable_base_data());
    GetGameData(gameType,pInfo->mutable_game_data());    
}

// 修改玩家账号数值（增量修改）
bool 	CPlayerBase::CanChangeAccountValue(uint8 valueType,int64 value)
{
    if(value >= 0)
        return true;
    int64 curValue = GetAccountValue(valueType);
    if(curValue < abs(value))
        return false;
    return true;
}
int64   CPlayerBase::ChangeAccountValue(uint8 valueType,int64 value)
{
    if(value == 0)
        return 0;
    int64 curValue      = GetAccountValue(valueType);
    int64 changeValue   = value;
    curValue += value;
    if(curValue < 0){
        curValue = 0;
        changeValue = -GetAccountValue(valueType);
    }
    SetAccountValue(valueType,curValue);
    return changeValue;
}
int64 	CPlayerBase::GetAccountValue(uint8 valueType) {
    switch (valueType)
    {
    case emACC_VALUE_DIAMOND:    // 钻石
        {
            return m_accInfo.diamond;
        }break;
    case emACC_VALUE_COIN:       // 财富币
        {
            return m_accInfo.coin;
        }break;
    case emACC_VALUE_SCORE:      // 积分
        {
            return m_accInfo.score;
        }break;
    case emACC_VALUE_INGOT:      // 元宝
        {
            return m_accInfo.ingot;
        }break;
    case emACC_VALUE_CVALUE:     // 魅力值
        {
            return m_accInfo.cvalue;
        }break;
    case emACC_VALUE_SAFECOIN:   // 保险箱值
        {
            return m_accInfo.safecoin;
        }break;
	//case emACC_VALUE_STOCKSCORE:
	//	{
	//		return m_StockScore;  //库存分数
	//	}break;
    default:
        assert(false);
    }
    return 0;
}
void 	CPlayerBase::SetAccountValue(uint8 valueType,int64 value)
{
    switch (valueType)
    {
    case emACC_VALUE_DIAMOND:    // 钻石
        {
            m_accInfo.diamond = value;
        }break;
    case emACC_VALUE_COIN:       // 财富币
        {
            m_accInfo.coin = value;
        }break;
    case emACC_VALUE_SCORE:      // 积分
        {
            m_accInfo.score = value;
        }break;
    case emACC_VALUE_INGOT:      // 元宝
        {
            m_accInfo.ingot = value;
        }break;
    case emACC_VALUE_CVALUE:     // 魅力值
        {
            m_accInfo.cvalue = value;
        }break;
    case emACC_VALUE_SAFECOIN:
        {
            m_accInfo.safecoin = value;
        }break;
	//case emACC_VALUE_STOCKSCORE:
	//	{
	//		m_StockScore = value;
	//	}break;
    default:
        assert(false);
    }
}
// 原子操作账号数值
bool    CPlayerBase::AtomChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin)
{
    if(!CanChangeAccountValue(emACC_VALUE_DIAMOND,diamond) || !CanChangeAccountValue(emACC_VALUE_COIN,coin) || !CanChangeAccountValue(emACC_VALUE_SCORE,score)
       || !CanChangeAccountValue(emACC_VALUE_INGOT,ingot) || !CanChangeAccountValue(emACC_VALUE_CVALUE,cvalue) || !CanChangeAccountValue(emACC_VALUE_SAFECOIN,safecoin))
    {
        return false;
    }
    bool ret = SyncChangeAccountValue(operType,subType,diamond,coin,ingot,score,cvalue,safecoin);
    return ret;
}

// 存取保险箱
bool 	CPlayerBase::TakeSafeBox(int64 takeCoin)
{
    if(takeCoin == 0)
        return false;
    int64 curSafeValue = GetAccountValue(emACC_VALUE_SAFECOIN);
    if(takeCoin > 0)//取金币
    {
        if(curSafeValue < takeCoin){
            return false;
        }
    }else{//存金币
        if(GetAccountValue(emACC_VALUE_COIN) < abs(takeCoin)){
            return false;
        }
    }
    SyncChangeAccountValue(emACCTRAN_OPER_TYPE_SAFEBOX,0,0,takeCoin,0,0,0,-takeCoin);
    return true;
}
void 	CPlayerBase::SetSafePasswd(const string& passwd)
{
    m_baseInfo.safepwd = passwd;
}
bool 	CPlayerBase::CheckSafePasswd(const string& passwd)
{
    if(m_baseInfo.safepwd == passwd){
        return true;
    }
	if (!IsRobot())
	{
		LOG_DEBUG("old:%s-len:%d,new:%s-len:%d", m_baseInfo.safepwd.c_str(), m_baseInfo.safepwd.length(), passwd.c_str(), passwd.length());
	}
    return false;
}
void 	CPlayerBase::SetSafeBoxState(uint8 state,bool checkpwd)
{
    if(checkpwd && m_baseInfo.safepwd.length() < 1){
        m_baseInfo.safeboxState = 2;
    }else {
        m_baseInfo.safeboxState = state;
    }
}
uint8   CPlayerBase::GetSafeBoxState()
{
    return m_baseInfo.safeboxState;
}
// 钻石兑换积分财富币
uint8 	CPlayerBase::ExchangeScore(uint32 exid,uint8 extype)
{
    int64 fromValue = 0;
    int64 toValue   = 0;
    bool bRet = CDataCfgMgr::Instance().GetExchangeValue(extype,exid,fromValue,toValue);
    if(bRet != true){
        return net::RESULT_CODE_ERROR_PARAM;
    }
    if(GetAccountValue(emACC_VALUE_DIAMOND) < fromValue)
	{
        LOG_DEBUG("兑换钻石不足");
        return net::RESULT_CODE_NOT_DIAMOND;
    }
    if(extype == net::EXCHANGE_TYPE_SCORE){
        SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,extype,-fromValue,0,0,toValue,0,0);
    }else if(extype == net::EXCHANGE_TYPE_COIN){
        SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,extype,-fromValue,toValue,0,0,0,0);
    }
    LOG_DEBUG("%d 兑换积分财富币:id:%d--type:%d,from:%lld,to:%lld",GetUID(), exid,extype,fromValue,toValue);
    return net::RESULT_CODE_SUCCESS;
}
//--- 操作记录
void	CPlayerBase::SetRewardBitFlag(uint32 flag)
{
    SetBitFlag(&m_baseInfo.reward,1,flag);
}
void	CPlayerBase::UnsetRewardBitFlag(uint32 flag)
{
    UnsetBitFlag(&m_baseInfo.reward,1,flag);
}
bool	CPlayerBase::IsSetRewardBitFlag(uint32 flag)
{
    return IsSetBitFlag(&m_baseInfo.reward,1,flag);
}

void 	CPlayerBase::SendBaseValue2Client()
{
	//LOG_DEBUG("更新基础数值到前端");
	net::msg_send_base_value msg;
	msg.set_clogin(m_baseInfo.clogin);
	auto & vecCloginRewards = CDataCfgMgr::Instance().GetCloginRewards();
	for (auto & score : vecCloginRewards)
	{
		msg.add_clogin_reward(score);
	}
	SendMsgToClient(&msg, net::S2C_MSG_SEND_BASE_VALUE);
}

void    CPlayerBase::CloginCleanup()
{
	m_baseInfo.clogin = 0;
	UnsetRewardBitFlag(net::REWARD_CLOGIN);
}

// 修改斗地主数值(增量修改)
void    CPlayerBase::ChangeLandValue(bool isCoin,int32 win,int32 lose,int32 land,int32 spring,int64 winscore)
{    
    if(isCoin){
        m_gameInfo[net::GAME_CATE_LAND].winc      += win;
        m_gameInfo[net::GAME_CATE_LAND].landc     += land;
        m_gameInfo[net::GAME_CATE_LAND].losec     += lose;
        m_gameInfo[net::GAME_CATE_LAND].springc   += spring;
        m_gameInfo[net::GAME_CATE_LAND].maxwinc   = MAX(m_gameInfo[net::GAME_CATE_LAND].maxwinc,winscore);
    }else {
        m_gameInfo[net::GAME_CATE_LAND].win      += win;
        m_gameInfo[net::GAME_CATE_LAND].land     += land;
        m_gameInfo[net::GAME_CATE_LAND].lose     += lose;
        m_gameInfo[net::GAME_CATE_LAND].spring   += spring;
        m_gameInfo[net::GAME_CATE_LAND].maxwin   = MAX(m_gameInfo[net::GAME_CATE_LAND].maxwin,winscore);
    }
}
// 修改游戏数值
void 	CPlayerBase::ChangeGameValue(uint16 gameType,bool isCoin,int32 win,int32 lose,int64 winscore)
{
	m_accInfo.win += winscore;
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin)
	{
        m_gameInfo[gameType].winc     += win;
        m_gameInfo[gameType].losec    += lose;
        m_gameInfo[gameType].maxwinc  = MAX(m_gameInfo[gameType].maxwinc,winscore);
        m_gameInfo[gameType].daywinc  += winscore;
		m_gameInfo[gameType].weekwinc += winscore;
		m_gameInfo[gameType].totalwinc += winscore;
		//m_gameInfo[gameType].stockscore += winscore;
		m_gameInfo[gameType].gamecount += 1;
		SetVecWin(gameType, win);
    }
	else
	{
        m_gameInfo[gameType].win      += win;
        m_gameInfo[gameType].lose     += lose;
        m_gameInfo[gameType].maxwin   = MAX(m_gameInfo[gameType].maxwin,winscore);
        m_gameInfo[gameType].daywin   += winscore;
    }
}
void CPlayerBase::ClearVecWin(uint16 gameType)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecwin.clear();
	}
}
void CPlayerBase::ClearVecBet(uint16 gameType)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecbet.clear();
	}
}
void CPlayerBase::SetVecWin(uint16 gameType,int win)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecwin.push_back(win);
		if (m_gameInfo[gameType].vecwin.size() > MAX_STATISTICS_GAME_COUNT)
		{
			m_gameInfo[gameType].vecwin.erase(m_gameInfo[gameType].vecwin.begin());
		}
	}
}
int CPlayerBase::GetVecWin(uint16 gameType)
{
	int count = 0;
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		for (uint32 i = 0; i < m_gameInfo[gameType].vecwin.size(); i++)
		{
			if (m_gameInfo[gameType].vecwin[i] == 1)
			{
				count++;
			}
		}
	}
	return count;
}
int CPlayerBase::GetBetCount(uint16 gameType)
{
	int count = 0;
	if (gameType < net::GAME_CATE_MAX_TYPE)
	{
		count = m_gameInfo[gameType].vecwin.size();
	}
	return count;
}

void CPlayerBase::SetVecBet(uint16 gameType, int64 score)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecbet.push_back(score);
		if (m_gameInfo[gameType].vecbet.size() > MAX_STATISTICS_GAME_COUNT)
		{
			m_gameInfo[gameType].vecbet.erase(m_gameInfo[gameType].vecbet.begin());
		}
	}
}

int64 CPlayerBase::GetVecBet(uint16 gameType)
{
	int64 score = 0;
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		for (uint32 i = 0; i < m_gameInfo[gameType].vecbet.size(); i++)
		{
			score += m_gameInfo[gameType].vecbet[i];
		}
	}
	return score;
}


//void CPlayerBase::SetBetScore(uint16 gameType, int64 score)
//{
//	if (gameType < net::GAME_CATE_MAX_TYPE && score>0)
//	{
//		m_gameInfo[gameType].bet = score;
//	}
//}
//
//int64 CPlayerBase::GetBetScore(uint16 gameType)
//{
//	if (gameType < net::GAME_CATE_MAX_TYPE)
//	{
//		return m_gameInfo[gameType].bet;
//	}
//	return 0;
//}


int64	CPlayerBase::GetGameMaxScore(uint16 gameType,bool isCoin)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        return m_gameInfo[gameType].maxwinc;
    }else{
        return m_gameInfo[gameType].maxwin;
    }
}
// 设置最大牌型
void 	CPlayerBase::SetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        memcpy(m_gameInfo[gameType].bestCard,cardData,cardCount);
    }else{
        memcpy(m_gameInfo[gameType].bestCardc,cardData,cardCount);
    }    
}
void 	CPlayerBase::GetGameMaxCard(uint16 gameType,bool isCoin,uint8* cardData,uint8 cardCount)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        memcpy(cardData,m_gameInfo[gameType].bestCard,cardCount);
    }else{
        memcpy(cardData,m_gameInfo[gameType].bestCard,cardCount);
    }    
}

int64	CPlayerBase::GetPlayerWeekWinScore(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].weekwinc;
	}
	return 0;
}

int64	CPlayerBase::GetPlayerDayWin(uint16 gameType)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].daywinc;
	}
	return 0;
}


int64	CPlayerBase::GetPlayerTotalWinScore(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].totalwinc;
	}
	return 0;
}

int64	CPlayerBase::GetPlayerGameCount(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].gamecount;
	}
	return 0;
}

int64	CPlayerBase::GetPlayerStockScore(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].stockscore;
	}
	return 0;
}

void	CPlayerBase::ReSetGameCount(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].gamecount = 0;
	}
}

bool	CPlayerBase::SendVipBroadCast()
{
	string broadCast = CDataCfgMgr::Instance().GetVipBroadcastMsg();
	int64  lVipRecharge = CDataCfgMgr::Instance().GetVipBroadcastRecharge();
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipBroadcastStatus();
	//LOG_DEBUG("SendBroadcast  - uid:%d,iVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,broadCast:%s", GetUID(), iVipStatus, GetRecharge(), lVipRecharge, broadCast.c_str());

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge || broadCast.empty())
	{
		return false;
	}
	net::msg_php_broadcast_rep rep;
	rep.set_msg(broadCast);
	SendMsgToClient(&rep, net::S2C_MSG_PHP_BROADCAST);

	return true;
}

bool	CPlayerBase::SendVipProxyRecharge()
{
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipProxyRechargeStatus();
	int64  lVipRecharge; // = CDataCfgMgr::Instance().GetVipProxyRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInVipProxyRechargeSpecialCnid(m_accInfo.siteid))
		lVipRecharge = CDataCfgMgr::Instance().GetVipProxySpecialRecharge();
	else
		lVipRecharge = CDataCfgMgr::Instance().GetVipProxyRecharge(); // add by har end

	auto & mpVipProxyWeChatInfo = CDataCfgMgr::Instance().GetVipProxyWeChatInfo();

	uint32 uPlayerVipStatus = 1;

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge)
	{
		uPlayerVipStatus = 1;
	}
	else if (iVipStatus == 0 && GetRecharge() >= lVipRecharge)
	{
		uPlayerVipStatus = 0;
	}
	else
	{
		uPlayerVipStatus = 1;
	}

	net::msg_notify_vip_recharge_show msg;
	msg.set_status(uPlayerVipStatus);

	tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());

	// shour:15, sminute:0, ehour:16, eminute:8
	// tmTime.tm_hour:15,, tmTime.tm_min:43

	// account:weixin003,tmTime.tm_hour:15,tmTime.tm_min:43,bIsShowWeChatInfo:1,timecontrol:1,shour:15, sminute:38, ehour:15, eminute:40,showtime:{"sh":15,"sm":38,"eh":15,"em":40}
	// account:weixin002, tmTime.tm_hour : 16, tmTime.tm_min : 3, bIsShowWeChatInfo : 0, timecontrol : 1, shour : 15, sminute : 0, ehour : 16, eminute : 8, showtime : {"sh":15, "sm" : 0, "eh" : 16, "em" : 8}, size : 3, tagInfo : 2 - 标题002 - weixin002]
	for (auto &iter : mpVipProxyWeChatInfo) {
		tagVipRechargeWechatInfo &tagInfo = iter.second;
		bool bIsShowWeChatInfo = IsShowVipRechargeWeChatInfo(tagInfo, tmTime); // false;
		/*if (tagInfo.shour > 23 || tagInfo.ehour > 23 || tagInfo.sminute > 59 || tagInfo.eminute > 59)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否正在进行中
			if (tmTime.tm_min >= tagInfo.sminute && tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			//是否开始了
			if (tmTime.tm_min >= tagInfo.sminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否结束了
			if (tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else
		{
			bIsShowWeChatInfo = false;
		}
		if (tagInfo.timecontrol == false)
		{
			bIsShowWeChatInfo = true;
		}


		bool bIsVipShow = false;
		if (tagInfo.vecVip.size() == 0)
		{
			bIsVipShow = true;
		}
		for (uint32 uIndex = 0; uIndex < tagInfo.vecVip.size(); uIndex++)
		{
			if (!IsRobot())
			{
				LOG_DEBUG("uid:%d,uIndex:%d,vecVip:%d,GetVipLevel:%d", GetUID(), uIndex, tagInfo.vecVip[uIndex], GetVipLevel());
			}

			if (tagInfo.vecVip[uIndex] == GetVipLevel())
			{
				bIsVipShow = true;
				break;
			}
		}

		if (bIsVipShow == false)
		{
			bIsShowWeChatInfo = false;
		}*/

		if (bIsShowWeChatInfo) {
			net::vip_recharge_wechatinfo *pwechatinfo = msg.add_info();
			/*pwechatinfo->set_sortid(tagInfo.sortid);
			pwechatinfo->set_title(tagInfo.title);
			pwechatinfo->set_account(tagInfo.account);
			pwechatinfo->set_low_amount(tagInfo.low_amount);*/
			SetVipRechargeWechatinfo(pwechatinfo, tagInfo);
		}
		if (!IsRobot())
		{
			LOG_DEBUG("SendVipProxyRecharge  - uid:%d,tagInfo.account:%s,tmTime.tm_hour:%d,tmTime.tm_min:%d,bIsShowWeChatInfo:%d,timecontrol:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s,size:%d,tagInfo:%d - %s - %s", GetUID(), tagInfo.account.c_str(), tmTime.tm_hour, tmTime.tm_min, bIsShowWeChatInfo, tagInfo.timecontrol, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str(), msg.info_size(), tagInfo.sortid, tagInfo.title.c_str(), tagInfo.account.c_str());
		}
	}

	//for (uint32 i = 0; i < msg.info_size(); i++)
	//{

	//}
	if (!IsRobot())
	{
		LOG_DEBUG("SendVipProxyRecharge  - uid:%d,iVipStatus:%d,uPlayerVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,size:%d %d", GetUID(), iVipStatus, uPlayerVipStatus, GetRecharge(), lVipRecharge, mpVipProxyWeChatInfo.size(), msg.info_size());
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_VIP_RECHARGE_SHOW);

	return true;
}

bool	CPlayerBase::SendVipAliAccRecharge()
{
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipAliAccRechargeStatus();
	int64  lVipRecharge; // = CDataCfgMgr::Instance().GetVipAliAccRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInVipAliAccRechargeSpecialCnid(m_accInfo.siteid))
		lVipRecharge = CDataCfgMgr::Instance().GetVipAliAccSpecialRecharge();
	else
		lVipRecharge = CDataCfgMgr::Instance().GetVipAliAccRecharge(); // add by har end
	auto & mpVipProxyWeChatInfo = CDataCfgMgr::Instance().GetVipAliAccInfo();

	uint32 uPlayerVipStatus = 1;

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge)
	{
		uPlayerVipStatus = 1;
	}
	else if (iVipStatus == 0 && GetRecharge() >= lVipRecharge)
	{
		uPlayerVipStatus = 0;
	}
	else
	{
		uPlayerVipStatus = 1;
	}

	net::msg_notify_vip_recharge_show msg;
	msg.set_status(uPlayerVipStatus);

	tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());

	// shour:15, sminute:0, ehour:16, eminute:8
	// tmTime.tm_hour:15,, tmTime.tm_min:43

	// account:weixin003,tmTime.tm_hour:15,tmTime.tm_min:43,bIsShowWeChatInfo:1,timecontrol:1,shour:15, sminute:38, ehour:15, eminute:40,showtime:{"sh":15,"sm":38,"eh":15,"em":40}
	// account:weixin002, tmTime.tm_hour : 16, tmTime.tm_min : 3, bIsShowWeChatInfo : 0, timecontrol : 1, shour : 15, sminute : 0, ehour : 16, eminute : 8, showtime : {"sh":15, "sm" : 0, "eh" : 16, "em" : 8}, size : 3, tagInfo : 2 - 标题002 - weixin002]
	for (auto &iter : mpVipProxyWeChatInfo) {
		tagVipRechargeWechatInfo &tagInfo = iter.second;
		bool bIsShowWeChatInfo = IsShowVipRechargeWeChatInfo(tagInfo, tmTime); // false;
		/*tagVipRechargeWechatInfo tagInfo = iter.second;
		if (tagInfo.shour > 23 || tagInfo.ehour > 23 || tagInfo.sminute > 59 || tagInfo.eminute > 59)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否正在进行中
			if (tmTime.tm_min >= tagInfo.sminute && tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			//是否开始了
			if (tmTime.tm_min >= tagInfo.sminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否结束了
			if (tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else
		{
			bIsShowWeChatInfo = false;
		}
		if (tagInfo.timecontrol == false)
		{
			bIsShowWeChatInfo = true;
		}


		bool bIsVipShow = false;
		if (tagInfo.vecVip.size() == 0)
		{
			bIsVipShow = true;
		}
		for (uint32 uIndex = 0; uIndex < tagInfo.vecVip.size(); uIndex++)
		{
			if (!IsRobot())
			{
				LOG_DEBUG("uid:%d,uIndex:%d,vecVip:%d,GetVipLevel:%d", GetUID(), uIndex, tagInfo.vecVip[uIndex], GetVipLevel());
			}

			if (tagInfo.vecVip[uIndex] == GetVipLevel())
			{
				bIsVipShow = true;
				break;
			}
		}

		if (bIsVipShow == false)
		{
			bIsShowWeChatInfo = false;
		}*/

		if (bIsShowWeChatInfo) {
			net::vip_recharge_wechatinfo *pwechatinfo = msg.add_info();
			/*pwechatinfo->set_sortid(tagInfo.sortid);
			pwechatinfo->set_title(tagInfo.title);
			pwechatinfo->set_account(tagInfo.account);
			pwechatinfo->set_low_amount(tagInfo.low_amount);*/
			SetVipRechargeWechatinfo(pwechatinfo, tagInfo);
		}
		if (!IsRobot())
		{
			LOG_DEBUG("SendVipProxyRecharge  - uid:%d,tagInfo.account:%s,tmTime.tm_hour:%d,tmTime.tm_min:%d,bIsShowWeChatInfo:%d,timecontrol:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s,size:%d,tagInfo:%d - %s - %s", GetUID(), tagInfo.account.c_str(), tmTime.tm_hour, tmTime.tm_min, bIsShowWeChatInfo, tagInfo.timecontrol, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str(), msg.info_size(), tagInfo.sortid, tagInfo.title.c_str(), tagInfo.account.c_str());
		}
	}

	//for (uint32 i = 0; i < msg.info_size(); i++)
	//{

	//}
	if (!IsRobot())
	{
		LOG_DEBUG("SendVipProxyRecharge  - uid:%d,iVipStatus:%d,uPlayerVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,size:%d %d", GetUID(), iVipStatus, uPlayerVipStatus, GetRecharge(), lVipRecharge, mpVipProxyWeChatInfo.size(), msg.info_size());
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_VIP_ALIACC_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendUnionPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetUnionPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetUnionPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInUnionPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetUnionPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetUnionPayRecharge(); // add by har end

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_unionpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);
	
	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_UNION_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendWeChatPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetWeChatPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetWeChatPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInWeChatPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetWeChatPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetWeChatPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_wechatpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_WECHAT_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendAliPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetAliPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetAliPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInAliPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetAliPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetAliPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_alipayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_ALI_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendOtherPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetOtherPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetOtherPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInOtherPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetOtherPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetOtherPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_otherpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_OTHER_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendQQPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetQQPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetQQPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInQQPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetQQPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetQQPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_qqpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_QQ_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendWeChatScanPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetWeChatScanPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetWeChatScanPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInWeChatScanPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetWeChatScanPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetWeChatScanPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_wechatscanpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendJDPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetJDPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetJDPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInJDPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetJDPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetJDPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_jdpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_JD_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendApplePayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetApplePayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetApplePayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_applepayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_APPLE_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendLargeAliPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetLargeAliPayRechargeStatus();
	int64  lPayRecharge; // = CDataCfgMgr::Instance().GetLargeAliPayRecharge(); modify by har
	// add by har
	if (CDataCfgMgr::Instance().IsInLargeAliPayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetLargeAliPaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetLargeAliPayRecharge(); // add by har end
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_large_ali_payrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE_REP);

	return true;
}

// 发送个人专属支付宝信息
void CPlayerBase::SendExclusiveAlipayRecharge() {
	if (IsRobot())
		return;

	int32 exclusiveAlipayRechargeStatus = 1;
	int64 exclusiveAlipayRecharge;
	if (CDataCfgMgr::Instance().IsInExclusiveAlipayRechargeSpecialCnid(m_accInfo.siteid))
		exclusiveAlipayRecharge = CDataCfgMgr::Instance().GetExclusiveAlipaySpecialRecharge();
	else
		exclusiveAlipayRecharge = CDataCfgMgr::Instance().GetExclusiveAlipayRecharge();

	if (GetRecharge() >= exclusiveAlipayRecharge)
		exclusiveAlipayRechargeStatus = CDataCfgMgr::Instance().GetExclusiveAlipayRechargeStatus();
	
	net::msg_notify_exclusive_alipay_recharge_show msg;
	msg.set_status(exclusiveAlipayRechargeStatus);
	if (exclusiveAlipayRechargeStatus == 0) {
		vector<tagExclusiveAlipayInfo> &vAlipayInfo = CDataCfgMgr::Instance().GetExclusiveAlipayInfo();
		for (tagExclusiveAlipayInfo &tagInfo : vAlipayInfo) {
			if (tagInfo.vips.find(GetVipLevel()) == tagInfo.vips.end())
				continue;
			net::exclusive_alipay_info *pInfo = msg.add_info();
			pInfo->set_account(tagInfo.account);
			pInfo->set_name(tagInfo.name);
			pInfo->set_title(tagInfo.title);
			pInfo->set_min_pay(tagInfo.min_pay);
			pInfo->set_max_pay(tagInfo.max_pay);
			pInfo->set_lower_float(tagInfo.lower_float);
		}
	}
	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_EXCLUSIVE_ALIPAY_RECHARGE_SHOW);
	LOG_DEBUG("SendExclusiveAliPayRecharge - uid:%d,exclusiveAlipayRechargeStatus:%d,GetRecharge:%lld,exclusiveAlipayRecharge:%lld,siteid:%d",
		GetUID(), exclusiveAlipayRechargeStatus, GetRecharge(), exclusiveAlipayRecharge, m_accInfo.siteid);
}

// 发送定额支付宝信息
void CPlayerBase::SendFixedAlipayRecharge() {
	if (IsRobot() || !IsPlaying())
		return;
	int32  iPayStatus = CDataCfgMgr::Instance().GetFixedAlipayRechargeStatus();
	int64  lPayRecharge;
	if (CDataCfgMgr::Instance().IsInFixedAlipayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetFixedAlipaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetFixedAlipayRecharge();
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
		uPlayerPayStatus = 1;
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
		uPlayerPayStatus = 0;
	else
		uPlayerPayStatus = 1;

	net::msg_notify_fixed_alipay_recharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);
	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_FIXED_ALIPAY_RECHARGE_SHOW_REP);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
}

// 发送定额微信信息
void CPlayerBase::SendFixedWechatRecharge() {
	if (IsRobot() || !IsPlaying())
		return;
	int32  iPayStatus = CDataCfgMgr::Instance().GetFixedWechatRechargeStatus();
	int64  lPayRecharge;
	if (CDataCfgMgr::Instance().IsInFixedWechatRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetFixedWechatSpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetFixedWechatRecharge();
	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
		uPlayerPayStatus = 1;
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
		uPlayerPayStatus = 0;
	else
		uPlayerPayStatus = 1;

	net::msg_notify_fixed_wechat_recharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);
	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_FIXED_WECHAT_RECHARGE_SHOW_REP);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
}

// 发送定额银联云闪付信息
void CPlayerBase::SendFixedUnionpayRecharge() {
	if (IsRobot() || !IsPlaying())
		return;
	int32  iPayStatus = CDataCfgMgr::Instance().GetFixedUnionpayRechargeStatus();
	int64  lPayRecharge;
	if (CDataCfgMgr::Instance().IsInFixedUnionpayRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetFixedUnionpaySpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetFixedUnionpayRecharge();

	uint32 uPlayerPayStatus = 1;
	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
		uPlayerPayStatus = 1;
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
		uPlayerPayStatus = 0;
	else
		uPlayerPayStatus = 1;

	net::msg_notify_fixed_unionpay_recharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);
	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_FIXED_UNIONPAY_RECHARGE_SHOW_REP);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);
}

// 发送专享闪付信息
void CPlayerBase::SendExclusiveFlashRecharge() {
	if (IsRobot() || !IsPlaying())
		return;
	int32  iPayStatus = CDataCfgMgr::Instance().GetExclusiveFlashRechargeStatus();
	int64  lPayRecharge;
	if (CDataCfgMgr::Instance().IsInExclusiveFlashRechargeSpecialCnid(m_accInfo.siteid))
		lPayRecharge = CDataCfgMgr::Instance().GetExclusiveFlashSpecialRecharge();
	else
		lPayRecharge = CDataCfgMgr::Instance().GetExclusiveFlashRecharge();

	uint32 uPlayerPayStatus = 1;
	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
		uPlayerPayStatus = 1;
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
		uPlayerPayStatus = 0;
	else
		uPlayerPayStatus = 1;
	map<string, tagVipRechargeWechatInfo> &mpVipProxyWeChatInfo = CDataCfgMgr::Instance().GetVipProxyFlashAccInfo();

	net::msg_notify_exclusive_flash_recharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());
	uint64 weChatInfoNum = 0;
	for (map<string, tagVipRechargeWechatInfo>::iterator iter = mpVipProxyWeChatInfo.begin(); iter != mpVipProxyWeChatInfo.end(); ++iter) {
		tagVipRechargeWechatInfo &tagInfo = iter->second;
		bool bIsShowWeChatInfo = IsShowVipRechargeWeChatInfo(tagInfo, tmTime);
		if (bIsShowWeChatInfo) {
			net::vip_recharge_wechatinfo *pwechatinfo = msg.add_info();
			SetVipRechargeWechatinfo(pwechatinfo, tagInfo);
			++weChatInfoNum;
		}
		if (!IsRobot())
			LOG_DEBUG("SendVipProxyRecharge  - uid:%d,tagInfo.account:%s,tmTime.tm_hour:%d,tmTime.tm_min:%d,bIsShowWeChatInfo:%d,timecontrol:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s,size:%d,tagInfo:%d - %s - %s",
				GetUID(), tagInfo.account.c_str(), tmTime.tm_hour, tmTime.tm_min, bIsShowWeChatInfo, tagInfo.timecontrol, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str(), msg.info_size(), tagInfo.sortid, tagInfo.title.c_str(), tagInfo.account.c_str());
	}
	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE_SHOW_REP);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld,mpVipProxyWeChatInfo.size=%lld,weChatInfoNum=%d",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge, mpVipProxyWeChatInfo.size(), weChatInfoNum);
}

// 设置用户水果机库存 
//bool  CPlayerBase::SetStockScore(int64 nStockScore)
//{
//	m_StockScore = nStockScore;
//	SetLoadState(emACCDATA_TYPE_ACC, 1);
//	return true;
//}

// 更新用户水果机库存 
int64  CPlayerBase::ChangeStockScore(uint16 gametype,int64 nStockScore)
{
	int64 stockscore = 0;
	if (gametype < net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gametype].stockscore += nStockScore;
		stockscore = m_gameInfo[gametype].stockscore;
	}
	return stockscore;
}

int    CPlayerBase::GetPosTimeDayCount()
{
	int daycount = -1;
	uint64 uTime = getTime();
	if (GetNoviceWelfare() && uTime != 0 && m_accInfo.postime != 0)
	{
		daycount = diffTimeDay(m_accInfo.postime, uTime);
	}
	return daycount;
}

void    CPlayerBase::UpdateWelCountByTime()
{
	int daycount = -1;
	uint64 uTime = getTime();
	if (GetNoviceWelfare() && uTime != 0)
	{
		if (m_accInfo.weltime != 0)
		{
			daycount = diffTimeDay(m_accInfo.weltime, uTime);
		}
		else
		{
			daycount = 0;
			m_accInfo.weltime = uTime;
		}
		if (daycount>0)
		{
			m_accInfo.welcount = 0;
			m_accInfo.weltime = uTime;
		}
	}

	return;
}

void CPlayerBase::UpdateWelCount(int value)
{
	UpdateWelCountByTime();
	m_accInfo.welcount += value;	
}


bool CPlayerBase::IsCanEnterWelfareRoom()
{
	bool bflag = false;
	bool bWelfare = GetNoviceWelfare();
	int pdaycount = GetPosTimeDayCount();
	uint32 posrmb = GetPosRmb();

	tagNewPlayerWelfareValue NewPlayerWelfareValue = CDataCfgMgr::Instance().GetNewPlayerWelfareValue(GetUID(), posrmb);

	if (bWelfare && pdaycount >= 0 && (uint32)pdaycount < NewPlayerWelfareValue.postime)
	{
		bflag = true;
	}
	if (bflag)
	{
		if ((uint32)m_accInfo.welcount >= NewPlayerWelfareValue.welfarecount)
		{
			bflag = false;
		}
	}
	if (!IsRobot())
	{
		LOG_DEBUG("uid:%d,bflag:%d,bWelfare:%d,pdaycount:%d,postime:%d,posrmb:%d,allwin_score:%d,allmaxwin_score:%d,welcount:%d,welfare_count:%d,id:%d,minpayrmb:%d,maxpayrmb:%d,welfarepro:%d,w_postime:%d",
			GetUID(), bflag, bWelfare, pdaycount, m_accInfo.postime, posrmb, m_accInfo.win, NewPlayerWelfareValue.maxwinscore, m_accInfo.welcount, NewPlayerWelfareValue.welfarecount, NewPlayerWelfareValue.id, NewPlayerWelfareValue.minpayrmb, NewPlayerWelfareValue.maxpayrmb, NewPlayerWelfareValue.welfarepro, NewPlayerWelfareValue.postime);

	}

	return bflag;
}

bool CPlayerBase::IsNoviceWelfare()
{
	bool bflag = false;
	bool bWelfare = GetNoviceWelfare();
	int pdaycount = GetPosTimeDayCount();
	uint32 posrmb = GetPosRmb();

	tagNewPlayerWelfareValue NewPlayerWelfareValue = CDataCfgMgr::Instance().GetNewPlayerWelfareValue(GetUID(),posrmb);

	if (bWelfare && pdaycount >= 0 && (uint32)pdaycount < NewPlayerWelfareValue.postime)
	{
		bflag = true;
	}
	if (bflag)
	{
		if (m_accInfo.win >= NewPlayerWelfareValue.maxwinscore)
		{
			bflag = false;
		}
	}
	if (bflag)
	{
		if ((uint32)m_accInfo.welcount >= NewPlayerWelfareValue.welfarecount)
		{
			bflag = false;
		}
	}
	
	if (!IsRobot())
	{
		LOG_DEBUG("uid:%d,bflag:%d,bWelfare:%d,pdaycount:%d,postime:%d,posrmb:%d,allwin_score:%d,allmaxwin_score:%d,welcount:%d,welfare_count:%d,id:%d,minpayrmb:%d,maxpayrmb:%d,welfarepro:%d,w_postime:%d",
			GetUID(), bflag, bWelfare, pdaycount, m_accInfo.postime, posrmb, m_accInfo.win, NewPlayerWelfareValue.maxwinscore, m_accInfo.welcount, NewPlayerWelfareValue.welfarecount, NewPlayerWelfareValue.id, NewPlayerWelfareValue.minpayrmb, NewPlayerWelfareValue.maxpayrmb, NewPlayerWelfareValue.welfarepro, NewPlayerWelfareValue.postime);

	}

	return bflag;
}

void CPlayerBase::SetNoviceWelfare()
{
	m_accInfo.userright |= UR_NOVICE_WELFARE;
}

bool CPlayerBase::GetNoviceWelfare()
{
	bool bflag = ((m_accInfo.userright & UR_NOVICE_WELFARE) == 0x2);
	return bflag;
}

//bool CPlayerBase::IsKilledScore(uint16 gametype)
//{
//	if (CCommonLogic::IsBaiRenAutoKillScore(gametype))
//	{
//		bool bIsAutoKilling = CDataCfgMgr::Instance().IsAutoKillingUsers(GetUID());
//		if (bIsAutoKilling)
//		{
//			int64 profit = GetAccountValue(emACC_VALUE_COIN) + GetAccountValue(emACC_VALUE_SAFECOIN) + GetConverts() - GetRecharge();
//
//			bool bOverRechargeProfit = CDataCfgMgr::Instance().OverRechargeProfit(GetUID(), profit, GetRecharge());
//
//			if (bOverRechargeProfit)
//			{
//				return true;
//			}
//		}
//	}
//
//	if (CCommonLogic::NotIsBaiRenAutoKillScore(gametype))
//	{
//		bool bIsAutoKilling = CDataCfgMgr::Instance().IsAutoKillingUsers(GetUID());
//		if (bIsAutoKilling)
//		{
//			int64 profit = GetAccountValue(emACC_VALUE_COIN) + GetAccountValue(emACC_VALUE_SAFECOIN) + GetConverts() - GetRecharge();
//
//			bool bOverRechargeProfit = CDataCfgMgr::Instance().OverRechargeProfit(GetUID(), profit, GetRecharge());
//
//			if (bOverRechargeProfit)
//			{
//				return true;
//			}
//		}
//	}
//
//	return false;
//}


//stAutoKillConfrontationCfg CPlayerBase::GetKilledScoreCfg()
//{
//	int64 profit = GetAccountValue(emACC_VALUE_COIN) + GetAccountValue(emACC_VALUE_SAFECOIN) + GetConverts() - GetRecharge();
//
//	stAutoKillConfrontationCfg cfg = CDataCfgMgr::Instance().GetAutoKillScoreCfg(GetUID(), profit, GetRecharge());
//
//	return cfg;
//
//}

bool CPlayerBase::IsShowVipRechargeWeChatInfo(tagVipRechargeWechatInfo &tagInfo, tm &tmTime) {
	bool bIsShowWeChatInfo = false;
	if (tagInfo.shour > 23 || tagInfo.ehour > 23 || tagInfo.sminute > 59 || tagInfo.eminute > 59)
		bIsShowWeChatInfo = true;
	else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		bIsShowWeChatInfo = true;
	else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour == tagInfo.ehour) {
		//是否正在进行中
		if (tmTime.tm_min >= tagInfo.sminute && tmTime.tm_min <= tagInfo.eminute)
			bIsShowWeChatInfo = true;
		else
			bIsShowWeChatInfo = false;
	} else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour < tagInfo.ehour) {
		//是否开始了
		if (tmTime.tm_min >= tagInfo.sminute)
			bIsShowWeChatInfo = true;
		else
			bIsShowWeChatInfo = false;
	} else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour == tagInfo.ehour) {
		//是否结束了
		if (tmTime.tm_min <= tagInfo.eminute)
			bIsShowWeChatInfo = true;
		else
			bIsShowWeChatInfo = false;
	} else
		bIsShowWeChatInfo = false;

	if (!tagInfo.timecontrol)
		bIsShowWeChatInfo = true;

	bool bIsVipShow = false;
	uint32 vecVipSize = tagInfo.vecVip.size();
	if (vecVipSize == 0)
		bIsVipShow = true;

	for (uint32 uIndex = 0; uIndex < vecVipSize; ++uIndex) {
		if (!IsRobot())
			LOG_DEBUG("uid:%d,uIndex:%d,vecVip:%d,GetVipLevel:%d", GetUID(), uIndex, tagInfo.vecVip[uIndex], GetVipLevel());
		if (tagInfo.vecVip[uIndex] == GetVipLevel()) {
			bIsVipShow = true;
			break;
		}
	}
	if (!bIsVipShow)
		bIsShowWeChatInfo = false;
	return bIsShowWeChatInfo;
}

void CPlayerBase::SetVipRechargeWechatinfo(net::vip_recharge_wechatinfo *pwechatinfo, tagVipRechargeWechatInfo &tagInfo) {
	pwechatinfo->set_sortid(tagInfo.sortid);
	pwechatinfo->set_title(tagInfo.title);
	pwechatinfo->set_account(tagInfo.account);
	pwechatinfo->set_low_amount(tagInfo.low_amount);
	for (int pay_type : tagInfo.vecPayType)
		pwechatinfo->add_pay_type(pay_type);
}
