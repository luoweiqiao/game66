
#include "common_logic.h"
#include "crypt/md5.h"
#include "json/json.h"
#include <sstream>
#include "game_define.h"
#include "center_log.h"
#include "dbmysql_mgr.h"
#include <algorithm>
#include "pb/msg_define.pb.h"

using namespace std;
using namespace svrlib;

namespace
{
	const static  char	s_PassWDKey[] = "@!$#@&#%$#@$#&*()";
};

// 检测时间是否在区间
bool     CCommonLogic::IsJoinTime(string startTime,string endTime)
{
	time_t time;
	if(CTimeUtility::GetTimeFromStr(startTime,time) == false)
	{
		LOG_ERROR("开始时间错误:%s",startTime.c_str());
		return false;
	}
	uint32 startTick = time;
	if(CTimeUtility::GetTimeFromStr(endTime,time) == false)
	{
		LOG_ERROR("结束时间错误:%s",endTime.c_str());
		return false;
	}
	uint32 endTick = time;    

    return (startTick < getSysTime() && getSysTime() < endTick);
}
//获得出牌字符串
void   CCommonLogic::LogCardString(uint8 cardData[],uint8 cardCount)
{
	string str;
	for(int i = 0; i < cardCount; ++i)
	{
		str += CStringUtility::FormatToString("%x ",cardData[i]);
	}
	LOG_DEBUG("%s",str.c_str());
}
void    CCommonLogic::LogIntString(int64 value[],int32 count)
{
	string str;
	for(int i = 0; i < count; ++i)
	{
		str += CStringUtility::FormatToString("%lld ",value[i]);
	}
	LOG_DEBUG("%s",str.c_str());
}
int64   CCommonLogic::SumIntArray(int64 value[],int32 count)
{
	int64 sum = 0;
	for(int i = 0; i < count; ++i){
		sum += value[i];
	}
	return sum;
}
// 获得校验码
string  CCommonLogic::VerifyPasswd(uint32 uid,string device)
{
	char szKey[64];
	memset(szKey,0,sizeof(szKey));
	sprintf(szKey,"%s%d%s",s_PassWDKey,uid,device.c_str());
	string strDecy = szKey;
	strDecy = getMD5Str(strDecy.c_str(),strDecy.length());
	return strDecy;
}
uint32  CCommonLogic::GetDataTableNum(uint32 uid)
{
	return 0;//不分表了

	if(uid >= ROBOT_MGR_ID)
		return 0;
	return uid/5000000;// 数据库表取模数值500w
}

//有新手福利功能的非百人游戏
bool    CCommonLogic::NotIsBaiRenNewPlayerNoviceWelfare(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_NIUNIU:
	case net::GAME_CATE_FRUIT_MACHINE:
		return true;
	default:
		return false;
	}
	return false;
}

//有新手福利功能的百人游戏
bool    CCommonLogic::IsBaiRenNewPlayerNoviceWelfare(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_BACCARAT:
		return true;
	default:
		return false;
	}
	return false;
}

//有新手福利功能的游戏
bool    CCommonLogic::IsNewPlayerNoviceWelfare(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_NIUNIU:
	case net::GAME_CATE_FRUIT_MACHINE:
		return true;
	default:
		return false;
	}
	return false;
}


//有自动杀分功能的非百人游戏
bool    CCommonLogic::NotIsBaiRenAutoKillScore(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_NIUNIU:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_SHOWHAND:
	case net::GAME_CATE_TEXAS:
	case net::GAME_CATE_ZAJINHUA:
		return true;
	default:
		return false;
	}
	return false;
}

//有自动杀分功能的百人游戏
bool    CCommonLogic::IsBaiRenAutoKillScore(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_DICE:
		return true;
	default:
		return false;
	}
	return false;
}


// 根据房间配置机器人在线数量的百人游戏
bool    CCommonLogic::IsBaiRenRobotGame(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
		return true;
	default:
		return false;
	}
	return false;
}

// 百人游戏
bool    CCommonLogic::IsBaiRenGame(uint16 gameType)
{
	switch(gameType)
	{
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
		return true;
	default:
		return false;
	}
	return false;
}
// 非百人游戏 （过滤站起坐下功能）
bool    CCommonLogic::NotIsBaiRenGame(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_LAND:
	case net::GAME_CATE_ZAJINHUA:
	case net::GAME_CATE_NIUNIU:
	//case net::GAME_CATE_TEXAS:
	case net::GAME_CATE_SHOWHAND:
	case net::GAME_CATE_TWO_PEOPLE_MAJIANG:
	case net::GAME_CATE_FRUIT_MACHINE:
		return true;
	default:
		return false;
	}
	return false;
}

// 是否开放这个游戏
bool    CCommonLogic::IsOpenGame(uint16 gameType)
{
    switch(gameType)
    {
    case net::GAME_CATE_LAND:
    case net::GAME_CATE_ZAJINHUA:
	case net::GAME_CATE_NIUNIU:
	case net::GAME_CATE_BULLFIGHT:
	case net::GAME_CATE_DICE:
	case net::GAME_CATE_BACCARAT:
	case net::GAME_CATE_TEXAS:
	case net::GAME_CATE_PAIJIU:
	case net::GAME_CATE_SHOWHAND:
	//case net::GAME_CATE_EVERYCOLOR: //这个不开
	case net::GAME_CATE_TWO_PEOPLE_MAJIANG:
	case net::GAME_CATE_FRUIT_MACHINE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_ROBNIU:
	case net::GAME_CATE_FISHING:
	case net::GAME_CATE_TWOEIGHT:
	case net::GAME_CATE_CARCITY:
        {
            return true;    
        }break;
    default:
        return false;
    }
    
    return false;
}

// 统计前20局的百人游戏 (客户端显示神算子 大富豪)
// 玩家不能主动点击坐到座位上，系统自动设置座位上的玩家
bool    CCommonLogic::IsBaiRenCount(uint16 gameType)
{
	switch (gameType)
	{
	//case net::GAME_CATE_BULLFIGHT:
	//case net::GAME_CATE_BACCARAT:
	//case net::GAME_CATE_PAIJIU:
	//case net::GAME_CATE_DICE:
	case net::GAME_CATE_WAR:
	case net::GAME_CATE_FIGHT:
	case net::GAME_CATE_TWOEIGHT:
	{
		return true;
	}break;
	default:
		return false;
	}
	return false;
}

// 判断是否重置信息
bool 	CCommonLogic::IsNeedReset(uint32 lastTime,uint32 curTime)
{
	if(lastTime == curTime)
		return false;

	static tm	tm_early;
	static tm	tm_late;
	memset(&tm_early,0,sizeof(tm_early));
	memset(&tm_late,0,sizeof(tm_late));
	getLocalTime(&tm_early,lastTime);
	getLocalTime(&tm_late,curTime);

	uint32 diffDay = 0;
	//同年同日
	if(tm_early.tm_year != tm_late.tm_year)
	{
		diffDay = 365;
	}
	else if(tm_early.tm_yday != tm_late.tm_yday)
	{
		diffDay =  abs(tm_late.tm_yday - tm_early.tm_yday);
	}
	static uint32 reset_time = 0;// 0  点重置
	uint32 lastDayTime  = tm_early.tm_hour * SECONDS_IN_ONE_HOUR + tm_early.tm_min * 60 + tm_early.tm_sec;
	uint32 curDayTime   = tm_late.tm_hour * SECONDS_IN_ONE_HOUR + tm_late.tm_min * 60   + tm_late.tm_sec;

	if(diffDay >= 2)
	{
		return true;
	}
	else if(diffDay == 1)
	{
		if(curDayTime > reset_time || lastDayTime <= reset_time)
		{
			return true;
		}
	}
	else
	{
		if(curDayTime > reset_time && lastDayTime <= reset_time)
		{
			return true;
		}
	}

	return false;
}
// 原子修改离线玩家数据
bool    CCommonLogic::AtomChangeOfflineAccData(uint32 uid,uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid)
{
	stAccountInfo data;
	data.clear();
	bool haveold = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).GetAccountInfoByUid(uid, data);
	bool bRet = CDBMysqlMgr::Instance().AtomChangeAccountValue(uid,diamond,coin,ingot,score,cvalue,safecoin);
	LOG_DEBUG("uid:%d,operType:%d,subType:%d,diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld,haveold:%d,bRet:%d,coin:%lld,safecoin:%lld",
		uid, operType, subType, diamond, coin, score, ingot, cvalue, safecoin, haveold, bRet, data.coin, data.safecoin);
	if(bRet)
	{
		CCenterLogMgr::Instance().OfflineAccountTransction(uid, operType, subType, diamond, coin, ingot, score, cvalue, safecoin, chessid, haveold, data);
	}
	return bRet;
}

//判断是否当前游戏是否有幸运值功能
bool    CCommonLogic::IsLuckyFuncitonGame(uint16 gameType)
{
	switch (gameType)
	{
	case net::GAME_CATE_LAND:
	case net::GAME_CATE_TWO_PEOPLE_MAJIANG:
	case net::GAME_CATE_SHOWHAND:
	case net::GAME_CATE_ZAJINHUA:
	case net::GAME_CATE_TEXAS:
	case net::GAME_CATE_NIUNIU:
	case net::GAME_CATE_ROBNIU:
	case net::GAME_CATE_FISHING:
		return true;
	default:
		return false;
	}
	return false;
}



