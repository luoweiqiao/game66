

#include "db_operator.h"
#include "db_struct_define.h"
#include "common_logic.h"
#include "svrlib.h"
#include "json/json.h"
#include "pb/msg_define.pb.h"

using namespace std;
using namespace svrlib;

namespace
{

    
};

// 加载房间配置信息
bool	CDBOperator::LoadRoomCfg(uint16 gameType,uint8 gameSubType,vector<stRoomCfg>& vecRooms)
{
	memset(m_szCommand,0,sizeof(m_szCommand));
	sprintf(m_szCommand,"select * from roomcfg where gametype=%d and gamesubtype=%d and isopen=1;",gameType,gameSubType);
	vector<map<string,MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	for(uint32 i=0;i<vecData.size();++i) {
		map<string, MYSQLValue> &refRows = vecData[i];
		stRoomCfg roomCfg;
		roomCfg.sortID	  = i;
		roomCfg.roomID    = refRows["roomid"].as<uint16>();
		roomCfg.deal      = refRows["deal"].as<uint8>();
		roomCfg.consume   = refRows["consume"].as<uint8>();
		roomCfg.enter_min = refRows["entermin"].as<int64>();
		roomCfg.enter_max = refRows["entermax"].as<int64>();
		roomCfg.baseScore = refRows["basescore"].as<int64>();
		roomCfg.roomType  = refRows["roomtype"].as<uint32>();
        roomCfg.showhandNum = refRows["showhandnum"].as<uint32>();
        roomCfg.tableNum  = refRows["tablenum"].as<uint32>();
        roomCfg.marry     = refRows["marry"].as<uint32>();
        roomCfg.limitEnter = refRows["limitenter"].as<uint32>();
        roomCfg.robot      = refRows["robot"].as<uint32>();
        roomCfg.showonline = refRows["showonline"].as<uint32>();
        roomCfg.sitdown    = refRows["sitdown"].as<int64>();
        roomCfg.feeType    = refRows["feetype"].as<uint8>();
        roomCfg.feeValue   = refRows["fee"].as<int32>();
        roomCfg.seatNum    = refRows["seatnum"].as<uint16>();
        roomCfg.showType   = refRows["showtype"].as<uint16>();
        roomCfg.showPic    = refRows["showpic"].as<uint16>();
        roomCfg.robotMaxScore = refRows["robotmaxscore"].as<int64>();
		roomCfg.robotMinScore = refRows["robotminscore"].as<int64>();
		roomCfg.jettonMinScore = refRows["jettonminscore"].as<int64>();
		roomCfg.uproom 		= refRows["uproom"].as<uint16>();
		roomCfg.exitchip 	= refRows["exitchip"].as<uint32>();
		roomCfg.name = refRows["name"].as<string>();

        roomCfg.param      = refRows["param"].as<string>();
        
		vecRooms.push_back(roomCfg);
	}

	return true;
}

// 加载库存奖池房间配置信息  add by har
bool CDBOperator::LoadRoomStockCfg(uint16 gameType, unordered_map<uint16, stStockCfg>& vecRooms) {
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from room_stock_cfg where gametype=%d;", gameType);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
		return false;
	for (uint32 i = 0; i < vecData.size(); ++i) {
		map<string, MYSQLValue> &refRows = vecData[i];
		stStockCfg roomCfg;
		roomCfg.roomID = refRows["roomid"].as<uint16>();
		roomCfg.stockMax = refRows["stock_max"].as<int64>();
		roomCfg.stockConversionRate = refRows["stock_conversion_rate"].as<int>();
		roomCfg.stock = refRows["stock"].as<int64>();
		roomCfg.jackpotMin = refRows["jackpot_min"].as<int64>();
		roomCfg.jackpotMaxRate = refRows["jackpot_max_rate"].as<int>();
		roomCfg.jackpotRate = refRows["jackpot_rate"].as<int>();
		roomCfg.jackpotCoefficient = refRows["jackpot_coefficient"].as<int>();
		roomCfg.jackpotExtractRate = refRows["jackpot_extract_rate"].as<int>();
		roomCfg.jackpot = refRows["jackpot"].as<int64>();
		roomCfg.killPointsLine = refRows["kill_points_line"].as<int64>();
		roomCfg.playerWinRate = refRows["player_win_rate"].as<int>();

		vecRooms.insert(make_pair(roomCfg.roomID, roomCfg));
	}
	return true;
}

// 加载机器人在线配置
bool    CDBOperator::LoadRobotOnlineCfg(map<uint32,stRobotOnlineCfg>& mpRobotCfg)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from robotonline where logintype = %d;", LOGIN_TYPE_BY_ALL_DAY);
    vector<map<string,MYSQLValue> > vecData;

    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
    mpRobotCfg.clear();
	LOG_DEBUG("load_robot_cfg - size:%d", vecData.size());
    for(uint32 i=0;i<vecData.size();++i)
	{
        map<string, MYSQLValue> &refRows = vecData[i];
        stRobotOnlineCfg cfg;
		cfg.batchID     = refRows["id"].as<uint32>();
        cfg.gameType    = refRows["gametype"].as<uint16>();
		cfg.roomID      = refRows["roomid"].as<uint16>();
		cfg.leveltype	= refRows["leveltype"].as<uint32>();
        cfg.onlines[0]  = refRows["online1"].as<uint32>();
        cfg.onlines[1]  = refRows["online2"].as<uint32>();
        cfg.onlines[2]  = refRows["online3"].as<uint32>();
        cfg.onlines[3]  = refRows["online4"].as<uint32>();
        cfg.onlines[4]  = refRows["online5"].as<uint32>();
        cfg.onlines[5]  = refRows["online6"].as<uint32>();
        cfg.onlines[6]  = refRows["online7"].as<uint32>();
        cfg.onlines[7]  = refRows["online8"].as<uint32>();
        cfg.onlines[8]  = refRows["online9"].as<uint32>();
        cfg.onlines[9]  = refRows["online10"].as<uint32>();
		cfg.onlines[10] = refRows["online11"].as<uint32>();
		cfg.onlines[11] = refRows["online12"].as<uint32>();
		cfg.onlines[12] = refRows["online13"].as<uint32>();
		cfg.onlines[13] = refRows["online14"].as<uint32>();
		cfg.onlines[14] = refRows["online15"].as<uint32>();
		cfg.onlines[15] = refRows["online16"].as<uint32>();
		cfg.onlines[16] = refRows["online17"].as<uint32>();
		cfg.loginType   = refRows["logintype"].as<uint16>();
		cfg.enterTime   = refRows["entertime"].as<uint16>();
		cfg.leaveTime   = refRows["leavetime"].as<uint16>();
		
		//for (uint32 j = 0; j < ROBOT_MAX_LEVEL; j++)
		//{
		//	LOG_DEBUG("dwBatchID:%d,gameType:%d,roomID:%d,loginType:%d,enterTime:%d,leaveTime:%d,j:%d,onlines:%d",
		//		cfg.batchID, cfg.gameType, cfg.roomID, cfg.loginType, cfg.enterTime, cfg.leaveTime, j, cfg.onlines[j]);
		//}
		if (cfg.enterTime > DAY_MAX_SECONDS || cfg.leaveTime > DAY_MAX_SECONDS)
		{
			LOG_ERROR("robot_time_error - dwBatchID:%d,gameType:%d,roomID:%d,loginType:%d,enterTime:%d,leaveTime:%d",
				cfg.batchID, cfg.gameType, cfg.roomID, cfg.loginType, cfg.enterTime, cfg.leaveTime);
		}
		auto iter_exist = mpRobotCfg.find(cfg.batchID);
		if (iter_exist!= mpRobotCfg.end())
		{
			LOG_DEBUG("error - batchID:%d", cfg.batchID);
			continue;
		}
        mpRobotCfg.insert(make_pair(cfg.batchID,cfg));
    }

	auto it_loop = mpRobotCfg.begin();
	for (; it_loop != mpRobotCfg.end(); it_loop++)
	{
		stRobotOnlineCfg& refCfg = it_loop->second;
		string strOnlines;
		uint32 allRobotCount = 0;
		for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
		{
			uint32 robotNum = refCfg.onlines[lv];
			strOnlines += CStringUtility::FormatToString("%d-%d ", lv, robotNum);
			allRobotCount += robotNum;
		}
		LOG_DEBUG("robot_cfg_al - batchID:%03d,gametype:%02d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d,allRobotCount:%03d,strOnlines:%s",
			refCfg.batchID, refCfg.gameType, refCfg.roomID, refCfg.leveltype, refCfg.loginType, refCfg.enterTime, refCfg.leaveTime, allRobotCount, strOnlines.c_str());
	}

    return true;
}

// 加载机器人在线配置
bool    CDBOperator::TimeLoadRobotOnlineEx(map<uint32, stRobotOnlineCfg>& mpRobotCfg)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from robotonline where leveltype = %d and logintype = %d;",net::ROOM_CONSUME_TYPE_COIN, LOGIN_TYPE_BY_ACCORD_TIME);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	mpRobotCfg.clear();
	LOG_DEBUG("load_robot_cfg - size:%d", vecData.size());
	for (uint32 i = 0; i<vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		stRobotOnlineCfg cfg;
		cfg.batchID     = refRows["id"].as<uint32>();
		cfg.gameType    = refRows["gametype"].as<uint16>();
		cfg.roomID      = refRows["roomid"].as<uint16>();
		cfg.leveltype	= refRows["leveltype"].as<uint32>();
		cfg.onlines[0]  = refRows["online1"].as<uint32>();
		cfg.onlines[1]  = refRows["online2"].as<uint32>();
		cfg.onlines[2]  = refRows["online3"].as<uint32>();
		cfg.onlines[3]  = refRows["online4"].as<uint32>();
		cfg.onlines[4]  = refRows["online5"].as<uint32>();
		cfg.onlines[5]  = refRows["online6"].as<uint32>();
		cfg.onlines[6]  = refRows["online7"].as<uint32>();
		cfg.onlines[7]  = refRows["online8"].as<uint32>();
		cfg.onlines[8]  = refRows["online9"].as<uint32>();
		cfg.onlines[9]  = refRows["online10"].as<uint32>();
		cfg.onlines[10] = refRows["online11"].as<uint32>();
		cfg.onlines[11] = refRows["online12"].as<uint32>();
		cfg.onlines[12] = refRows["online13"].as<uint32>();
		cfg.onlines[13] = refRows["online14"].as<uint32>();
		cfg.onlines[14] = refRows["online15"].as<uint32>();
		cfg.onlines[15] = refRows["online16"].as<uint32>();
		cfg.onlines[16] = refRows["online17"].as<uint32>();
		cfg.loginType   = refRows["logintype"].as<uint32>();
		cfg.enterTime   = refRows["entertime"].as<uint32>();
		cfg.leaveTime   = refRows["leavetime"].as<uint32>();

		//for (uint32 j = 0; j < ROBOT_MAX_LEVEL; j++)
		//{
		//	LOG_DEBUG("dwBatchID:%d,gameType:%d,roomID:%d,loginType:%d,enterTime:%d,leaveTime:%d,j:%d,onlines:%d",
		//		cfg.batchID,cfg.gameType, cfg.roomID, cfg.loginType, cfg.enterTime, cfg.leaveTime, j, cfg.onlines[j]);
		//}
		if (cfg.enterTime > DAY_MAX_SECONDS || cfg.leaveTime > DAY_MAX_SECONDS)
		{
			LOG_ERROR("robot_time_error - dwBatchID:%d,gameType:%d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d",
				cfg.batchID, cfg.gameType, cfg.roomID, cfg.leveltype,cfg.loginType, cfg.enterTime, cfg.leaveTime);
		}
		auto iter_exist = mpRobotCfg.find(cfg.batchID);
		if (iter_exist != mpRobotCfg.end())
		{
			LOG_DEBUG("error - dwBatchID:%d", cfg.batchID);
			continue;
		}
		mpRobotCfg.insert(make_pair(cfg.batchID, cfg));
	}
	auto it_loop = mpRobotCfg.begin();
	for (; it_loop != mpRobotCfg.end(); it_loop++)
	{
		stRobotOnlineCfg& refCfg = it_loop->second;
		string strOnlines;
		uint32 allRobotCount = 0;
		for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
		{
			uint32 robotNum = refCfg.onlines[lv];
			strOnlines += CStringUtility::FormatToString("%d-%d ", lv, robotNum);
			allRobotCount += robotNum;
		}
		LOG_DEBUG("robot_cfg_ex - batchID:%d,gametype:%02d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d,allRobotCount:%d,strOnlines:%s",
			refCfg.batchID, refCfg.gameType, refCfg.roomID, refCfg.leveltype, refCfg.loginType, refCfg.enterTime, refCfg.leaveTime, allRobotCount, strOnlines.c_str());
	}
	return true;
}


// 加载任务配置信息
bool	CDBOperator::LoadMissionCfg(map<uint32,stMissionCfg>& mapMissions)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from mission;");
    vector<map<string,MYSQLValue> > vecData;
    
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	mapMissions.clear();
    for(uint32 i=0;i<vecData.size();++i)
    {
        map<string, MYSQLValue>& refRows = vecData[i];
        stMissionCfg missioncfg;
        missioncfg.msid = refRows["msid"].as<uint32>();
        missioncfg.type = refRows["type"].as<uint16>();
		if (missioncfg.type>= net::MISSION_TYPE_MAXVALUE)
		{
			continue;
		}
		missioncfg.status = refRows["status"].as<uint32>();
        missioncfg.autoprize = refRows["auto"].as<uint8>();
        missioncfg.cate1 = refRows["cate1"].as<uint32>();
        missioncfg.cate2 = refRows["cate2"].as<uint32>();
        missioncfg.cate3 = refRows["cate3"].as<uint32>();
        missioncfg.cate4 = refRows["cate4"].as<uint32>();
        //missioncfg.mtimes = refRows["mtimes"].as<uint32>();
		string strMtimes = refRows["mtimes"].as<string>();

		Json::Reader reader;
		Json::Value  jvalve;
		if (!strMtimes.empty() && reader.parse(strMtimes, jvalve) && jvalve.isArray() && jvalve.size() >0)
		{
			for (uint32 i = 0; i < jvalve.size(); i++)
			{
				if (jvalve[i].isIntegral())
				{
					missioncfg.mtimes.push_back(jvalve[i].asInt());
				}
			}
		}
		//LOG_DEBUG("--------------------------------------------------------------------");
		//for (uint32 i = 0; i < missioncfg.mtimes.size(); i++)
		//{
		//	LOG_DEBUG("msid:%d,autoprize:%d,i:%d,mtimes:%d", missioncfg.msid, missioncfg.autoprize, i, missioncfg.mtimes[i]);
		//}


        missioncfg.straight = refRows["straight"].as<uint32>();
        missioncfg.cycle = refRows["cycle"].as<uint32>();
        missioncfg.cycletimes = refRows["cycletimes"].as<uint32>();

		string strmtimes;
		for (unsigned int i = 0; i < missioncfg.mtimes.size(); ++i)
		{
			strmtimes += CStringUtility::FormatToString("%d ", missioncfg.mtimes[i]);
		}
		LOG_DEBUG("msid:%d,type:%d,autoprize:%d,mtimes:%s", missioncfg.msid, missioncfg.type, missioncfg.autoprize, strmtimes.c_str());

        //memset(m_szCommand,0,sizeof(m_szCommand));
        //sprintf(m_szCommand,"select * from missionprize where msid = %d;",missioncfg.msid);
        //vector<map<string,MYSQLValue> > vecDataPri;
        //this->Query(m_szCommand,vecDataPri);
        //for(uint32 j=0;j<vecDataPri.size();++j)
        //{
        //    stMissionPrizeCfg prizeCfg;
        //    prizeCfg.poid = vecDataPri[j]["poid"].as<uint32>();
        //    prizeCfg.qty = vecDataPri[j]["qty"].as<uint32>();
        //    missioncfg.missionprize.push_back(prizeCfg);
        //}
        mapMissions.insert(make_pair(missioncfg.msid,missioncfg));

    }
    
    return true;
}
// 加载系统配置
bool 	CDBOperator::LoadSysCfg(map<string,string>& mapSysCfg)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from sysconfig;");
    vector<map<string,MYSQLValue> > vecData;

    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	mapSysCfg.clear();
    for(uint32 i=0;i<vecData.size();++i)
    {
        map<string, MYSQLValue> &refRows = vecData[i];
        string key  = refRows["sckey"].as<string>();
        string value = refRows["value"].as<string>();

		LOG_DEBUG("key:%s,valve:%s",key.c_str(),value.c_str());
        mapSysCfg.insert(make_pair(key,value));
    }

    return true;
}
//加载vip充值微信信息
bool CDBOperator::LoadVipProxyRecharge(map<string, tagVipRechargeWechatInfo> &mapInfo, int type) {
	Json::Reader reader;
	Json::Value  jshowtime;
	Json::Value  jvip;
	Json::Value  jvPayType;

	mapInfo.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from vip_proxy_cfg where type=%d;", type);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	for (uint32 i = 0; i<vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		int is_enabled = refRows["is_enabled"].as<int>();
		if (is_enabled == 1)
		{
			tagVipRechargeWechatInfo tagInfo;
			tagInfo.sortid = refRows["sort_id"].as<uint32>();
			tagInfo.account = refRows["wx_account"].as<string>();
			tagInfo.title = refRows["wx_title"].as<string>();
			tagInfo.showtime = refRows["showtime"].as<string>();
			tagInfo.strVip = refRows["vip"].as<string>();
			tagInfo.low_amount = refRows["low_amount"].as<uint32>();
			string strPayType = refRows["ptype"].as<string>();
			bool bIsWeChatInfo = true;
			if (!tagInfo.showtime.empty() && reader.parse(tagInfo.showtime, jshowtime))
			{
				if (jshowtime.isMember("sh") && jshowtime["sh"].isIntegral())
				{
					tagInfo.shour = jshowtime["sh"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("sm") && jshowtime["sm"].isIntegral())
				{
					tagInfo.sminute = jshowtime["sm"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("eh") && jshowtime["eh"].isIntegral())
				{
					tagInfo.ehour = jshowtime["eh"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("em") && jshowtime["em"].isIntegral())
				{
					tagInfo.eminute = jshowtime["em"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
			}
			else
			{
				bIsWeChatInfo = false;

				LOG_ERROR("解析微信充值显示时间错误 - type:%d,showtime:%s", type, tagInfo.showtime.c_str());
			}
			
			tagInfo.timecontrol = bIsWeChatInfo;

			LOG_ERROR("解析微信充值显示 - account:%s,bIsWeChatInfo:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,type:%d,showtime:%s",
				tagInfo.account.c_str(), bIsWeChatInfo, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, type, tagInfo.showtime.c_str());

			if (!tagInfo.strVip.empty() && reader.parse(tagInfo.strVip, jvip)) {
				if (jvip.isArray())
					for (uint32 i = 0; i < jvip.size(); ++i) {
						LOG_DEBUG("解析微信充值 - type:%d,strVip:%s,isArray:%d,isIntegral:%d",
							type, tagInfo.strVip.c_str(), jvip.isArray(), jvip[i].isIntegral());
						if (jvip[i].isIntegral()) {
							LOG_DEBUG("解析微信充值 - type:%d,strVip:%s,jvip:%d",
								type, tagInfo.strVip.c_str(), jvip[i].asInt());
							tagInfo.vecVip.push_back(jvip[i].asInt());
						}
					}
				else
					LOG_ERROR("解析微信充值错误 - type:%d,strVip:%s,jvip.isArray():%d",
						type, tagInfo.strVip.c_str(), jvip.isArray());
			} else
				LOG_ERROR("解析微信充值错误 - type:%d,strVip:%s", type, tagInfo.strVip.c_str());

			if (!strPayType.empty() && reader.parse(strPayType, jvPayType)) {
				if (jvPayType.isArray())
					for (uint32 i = 0; i < jvPayType.size(); ++i) {
						LOG_DEBUG("解析微信充值支付方式 - type:%d,strPayType:%s,isArray:%d,isIntegral:%d",
							type, strPayType.c_str(), jvPayType.isArray(), jvPayType[i].isIntegral());
						if (jvPayType[i].isIntegral()) {
							LOG_DEBUG("解析微信充值支付方式 - type:%d,strPayType:%s,jvPayType:%d",
								type, strPayType.c_str(), jvPayType[i].asInt());
							tagInfo.vecPayType.push_back(jvPayType[i].asInt());
						}
					}
				else
					LOG_ERROR("解析微信充值支付方式错误 - type:%d,strPayType:%s,jvPayType.isArray():%d",
						type, strPayType.c_str(), jvPayType.isArray());
			}
			else
				LOG_ERROR("解析微信充值支付方式错误 - type:%d,strPayType:%s", type, strPayType.c_str());

			mapInfo.insert(make_pair(tagInfo.account, tagInfo));
		}
	}

	return true;
}
/*bool 	CDBOperator::LoadVipAliAccRecharge(map<string, tagVipRechargeWechatInfo> & mapInfo)
{
	Json::Reader reader;
	Json::Value  jshowtime;
	Json::Value  jvip;


	mapInfo.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from vip_proxy_cfg where type = 1;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	for (uint32 i = 0; i<vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		int is_enabled = refRows["is_enabled"].as<int>();
		if (is_enabled == 1)
		{
			tagVipRechargeWechatInfo tagInfo;
			tagInfo.sortid = refRows["sort_id"].as<uint32>();
			tagInfo.account = refRows["wx_account"].as<string>();
			tagInfo.title = refRows["wx_title"].as<string>();
			tagInfo.showtime = refRows["showtime"].as<string>();
			tagInfo.strVip = refRows["vip"].as<string>();
			tagInfo.low_amount = refRows["low_amount"].as<uint32>();
			bool bIsWeChatInfo = true;
			if (!tagInfo.showtime.empty() && reader.parse(tagInfo.showtime, jshowtime))
			{
				if (jshowtime.isMember("sh") && jshowtime["sh"].isIntegral())
				{
					tagInfo.shour = jshowtime["sh"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("sm") && jshowtime["sm"].isIntegral())
				{
					tagInfo.sminute = jshowtime["sm"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("eh") && jshowtime["eh"].isIntegral())
				{
					tagInfo.ehour = jshowtime["eh"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
				if (jshowtime.isMember("em") && jshowtime["em"].isIntegral())
				{
					tagInfo.eminute = jshowtime["em"].asInt();
				}
				else
				{
					bIsWeChatInfo = false;
				}
			}
			else
			{
				bIsWeChatInfo = false;

				LOG_ERROR("解析微信充值显示时间错误 - showtime:%s", tagInfo.showtime.c_str());
			}

			tagInfo.timecontrol = bIsWeChatInfo;

			LOG_ERROR("解析微信充值显示 - bIsWeChatInfo:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s", bIsWeChatInfo, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str());

			if (!tagInfo.strVip.empty() && reader.parse(tagInfo.strVip, jvip))
			{
				if (jvip.isArray())
				{
					for (uint32 i = 0; i < jvip.size(); i++)
					{
						LOG_DEBUG("解析微信充值 - strVip:%s,isArray:%d,isIntegral:%d", tagInfo.strVip.c_str(), jvip.isArray(), jvip[i].isIntegral());
						if (jvip[i].isIntegral())
						{
							LOG_DEBUG("解析微信充值 - strVip:%s,jvip:%d", tagInfo.strVip.c_str(), jvip[i].asInt());
							tagInfo.vecVip.push_back(jvip[i].asInt());
						}
					}
				}
				else
				{
					LOG_ERROR("解析微信充值错误 - strVip:%s,jvip.isArray():%d", tagInfo.strVip.c_str(), jvip.isArray());
				}
			}
			else
			{
				LOG_ERROR("解析微信充值错误 - strVip:%s", tagInfo.strVip.c_str());
			}


			mapInfo.insert(make_pair(tagInfo.account, tagInfo));
		}
	}

	return true;
}*/

bool 	CDBOperator::LoadNewPlayerWelfareValue(vector<tagNewPlayerWelfareValue> & vecNewPlayerWelfareValue)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from newuser_welfare;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}

	vecNewPlayerWelfareValue.clear();

	for (uint32 i = 0; i<vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		tagNewPlayerWelfareValue data;

		data.id = refRows["id"].as<uint32>();
		data.minpayrmb = refRows["minpayrmb"].as<uint32>();
		data.maxpayrmb = refRows["maxpayrmb"].as<uint32>();
		data.maxwinscore = refRows["maxwinscore"].as<uint32>();
		data.welfarecount = refRows["welfarecount"].as<uint32>();
		data.welfarepro = refRows["welfarepro"].as<uint32>();
		data.postime = refRows["postime"].as<uint32>();
		data.lift_odds = refRows["lift_odds"].as<uint32>();
		//LOG_DEBUG("key:%s,valve:%s", key.c_str(), value.c_str());
		vecNewPlayerWelfareValue.push_back(data);
	}

	for (uint32 i = 0; i < vecNewPlayerWelfareValue.size(); i++)
	{
		tagNewPlayerWelfareValue & tempData = vecNewPlayerWelfareValue[i];
		LOG_DEBUG("in_welfare_value - id:%d,minpayrmb:%d,maxpayrmb:%d,maxwinscore:%d,welfarecount:%d,welfarepro:%d,postime:%d",
			tempData.id, tempData.minpayrmb, tempData.maxpayrmb, tempData.maxwinscore, tempData.welfarecount, tempData.welfarepro,tempData.postime);
	}

	return true;
}

// 加载服务器配置信息
bool    CDBOperator::LoadSvrCfg(map<uint32,stServerCfg>& mapSvrCfg)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from serverinfo;");
    vector<map<string,MYSQLValue> > vecData;
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	//string str_svrip = CHelper::GetNetIP();
	//string str_svrlanip = CHelper::GetLanIP();

	//bool bHaveNetIp = CHelper::IsHaveNetIP();
	//bool bHaveLanIp = (strcmp("1.1.0.1", str_svrlanip.c_str()) != 0);

	mapSvrCfg.clear();
    for(uint32 i=0;i<vecData.size();++i)
    {
        stServerCfg cfg;
        map<string, MYSQLValue> &refRows = vecData[i];
        cfg.svrid           = refRows["svrid"].as<uint32>();
        cfg.group           = refRows["group"].as<uint32>();
        cfg.gameType        = refRows["game_type"].as<uint32>();
        cfg.gameSubType     = refRows["game_subtype"].as<uint32>();
        cfg.svrType         = refRows["svr_type"].as<uint32>();
        cfg.svrip           = refRows["svrip"].as<string>();
        cfg.svrport         = refRows["svrport"].as<uint32>();
        cfg.svrlanip        = refRows["svrlanip"].as<string>();
        cfg.svrlanport      = refRows["svrlanport"].as<uint32>();
        cfg.phpport         = refRows["phpport"].as<uint32>();
        cfg.openRobot       = refRows["openrobot"].as<uint8>();

		//if (bHaveNetIp)
		//{
		//	cfg.svrip = str_svrip;
		//}
		//else
		//{
		//	cfg.svrip = refRows["svrip"].as<string>();
		//}
		//if (bHaveLanIp)
		//{
		//	cfg.svrlanip = str_svrlanip;
		//}
		//else
		//{
		//	cfg.svrlanip = refRows["svrlanip"].as<string>();
		//}

		//LOG_DEBUG("加载服务器配置信息 - vecData.size:%d,svrid:%d,openRobot:%d,bHaveNetIp:%d,bHaveLanIp:%d,svrip:%s,svrlanip:%s",	vecData.size(),cfg.svrid, cfg.openRobot, bHaveNetIp, bHaveLanIp, cfg.svrip.c_str(), cfg.svrlanip.c_str());
		LOG_DEBUG("加载服务器配置信息 - vecData.size:%d,svrid:%d,openRobot:%d,svrip:%s,svrlanip:%s",	vecData.size(),cfg.svrid, cfg.openRobot, cfg.svrip.c_str(), cfg.svrlanip.c_str());

        mapSvrCfg.insert(make_pair(cfg.svrid,cfg));
    }

    return true;
}
// 加载兑换数据
bool CDBOperator::LoadExchangeDiamondCfg(map<uint32,stExchangeCfg>& mapExcfg)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from diamondprice;");
    vector<map<string,MYSQLValue> > vecData;
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	mapExcfg.clear();
    for(uint32 i=0;i<vecData.size();++i)
    {
        stExchangeCfg cfg;
        map<string, MYSQLValue> &refRows = vecData[i];
        cfg.id              = refRows["id"].as<uint32>();
        cfg.toValue         = refRows["diamond"].as<uint64>();
        cfg.fromValue       = refRows["rmb"].as<uint64>();

        mapExcfg.insert(make_pair(cfg.id,cfg));
    }

    return true;
}
bool CDBOperator::LoadExchangeCoinCfg(map<uint32,stExchangeCfg>& mapExcfg)
{
	
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from coinprice;");
    vector<map<string,MYSQLValue> > vecData;
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
	mapExcfg.clear();
    for(uint32 i=0;i<vecData.size();++i)
    {
        stExchangeCfg cfg;
        map<string, MYSQLValue> &refRows = vecData[i];
        cfg.id              = refRows["id"].as<uint32>();
        cfg.fromValue       = refRows["diamond"].as<uint64>();
        cfg.toValue         = refRows["coin"].as<uint64>();

        mapExcfg.insert(make_pair(cfg.id,cfg));
    }

    return true;
}
bool CDBOperator::LoadExchangeScoreCfg(map<uint32,stExchangeCfg>& mapExcfg)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from scoreprice;");
    vector<map<string,MYSQLValue> > vecData;
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
    for(uint32 i=0;i<vecData.size();++i)
    {
        stExchangeCfg cfg;
        map<string, MYSQLValue> &refRows = vecData[i];
        cfg.id              = refRows["id"].as<uint32>();
        cfg.fromValue       = refRows["diamond"].as<uint64>();
        cfg.toValue         = refRows["score"].as<uint64>();

        mapExcfg.insert(make_pair(cfg.id,cfg));
    }

    return true;
}
// 加载私人房
bool    CDBOperator::LoadPrivateTable(uint16 gameType,vector<stPrivateTable>& tables)
{
    memset(m_szCommand,0,sizeof(m_szCommand));
    sprintf(m_szCommand,"select * from proom where duetime > %lld and gametype = %d;",getSysTime(),gameType);
    vector<map<string,MYSQLValue> > vecData;
    int iRet = this->Query(m_szCommand,vecData);
	if (iRet == -1)
	{
		return false;
	}
    for(uint32 i=0;i<vecData.size();++i)
    {
        stPrivateTable table;
        map<string, MYSQLValue> &refRows = vecData[i];
        table.tableID       = refRows["tableid"].as<uint32>();
        table.tableName     = refRows["tablename"].as<string>();
        table.hostName      = refRows["hostname"].as<string>();
        table.hostID        = refRows["hostid"].as<uint32>();
        table.passwd        = refRows["passwd"].as<string>();
        table.dueTime       = refRows["duetime"].as<uint32>();
        table.baseScore     = refRows["basescore"].as<int64>();
        table.deal          = refRows["deal"].as<uint32>();
        table.enterMin      = refRows["entermin"].as<int64>();
        table.consume       = refRows["consume"].as<uint32>();
        table.feeType       = refRows["feetype"].as<uint32>();
        table.feeValue      = refRows["feevalue"].as<int64>();
        table.isShow        = refRows["display"].as<uint32>();
        table.hostIncome    = refRows["hostincome"].as<int64>();
        table.sysIncome     = refRows["sysincome"].as<int64>();
        table.gameType      = refRows["gametype"].as<uint32>();
        table.createTime    = refRows["createtime"].as<uint32>();

        tables.push_back(table);
    }


    return true;
}
// 创建私人房信息
uint32  CDBOperator::CreatePrivateTable(stPrivateTable& table)
{
    try
    {
        CMySQLMultiFree	clMultiFree(&m_clDatabase);
        CMySQLStatement clStatement = m_clDatabase.createStatement();

        if(!clStatement.cmd("INSERT INTO proom (tablename,hostname,hostid,passwd,duetime,basescore,deal,entermin,consume,\
                                    feetype,feevalue,display,hostincome,sysincome,gametype,createtime)\
                                     VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"))
        {
            return 0;
        }
        int nBind = 0;
        clStatement.bindParam_String(nBind++,const_cast<char*>(table.tableName.c_str()),table.tableName.length());
        clStatement.bindParam_String(nBind++,const_cast<char*>(table.hostName.c_str()),table.hostName.length());
        clStatement.bindParam_UInt32(nBind++,&table.hostID);
        clStatement.bindParam_String(nBind++,const_cast<char*>(table.passwd.c_str()),table.passwd.length());
        clStatement.bindParam_UInt32(nBind++,&table.dueTime);
        clStatement.bindParam_Int64(nBind++,&table.baseScore);
        clStatement.bindParam_UInt8(nBind++,&table.deal);
        clStatement.bindParam_Int64(nBind++,&table.enterMin);
        clStatement.bindParam_UInt8(nBind++,&table.consume);
        clStatement.bindParam_UInt8(nBind++,&table.feeType);
        clStatement.bindParam_Int64(nBind++,&table.feeValue);
        clStatement.bindParam_UInt8(nBind++,&table.isShow);
        clStatement.bindParam_Int64(nBind++,&table.hostIncome);
        clStatement.bindParam_Int64(nBind++,&table.sysIncome);
        clStatement.bindParam_UInt16(nBind++,&table.gameType);
        clStatement.bindParam_UInt32(nBind++,&table.createTime);

        clStatement.bindParams();
        if(!clStatement.execute())
        {
            LOG_ERROR("Create Private Table ()execute fail!");
            return 0;
        }

        uint32 tableID = (uint32)clStatement.getInsertIncrement();
        return tableID;
    }
    catch(CMySQLException & e)
    {
        LOG_ERROR("Error:(create private table )[]:%u(%s)->%s->%s",e.uCode,e.szError,e.szMsg,e.szSqlState);
    }

    return 0;
}
// 判断用户是否存在
bool  CDBOperator::IsExistPlayerID(uint32 id)
{
    return GetResNumExeSqlCmd("select * from account%d where uid = %d;",CCommonLogic::GetDataTableNum(id),id) > 0;
}

// 加载夺宝游戏数据
bool	CDBOperator::SynLoadSnatchCoinGameData(vector<tagSnatchCoinGameData>& vecGameData)
{
	vecGameData.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from game_everycolor_snatchcoin;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	if (vecData.size() == 0)
	{
		return false;
	}
	for (uint32 i = 0; i<vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		tagSnatchCoinGameData tagData;
		tagData.uid = refRows["uid"].as<uint32>();
		tagData.type = refRows["type"].as<uint32>();
		tagData.card = refRows["card"].as<string>();
		LOG_DEBUG("uid:%d,type:%d,card:%s", tagData.uid, tagData.type, tagData.card.c_str());
		vecGameData.push_back(tagData);
	}
	return true;
}


bool CDBOperator::GetAccountInfoByUid(uint32 uid, stAccountInfo & data)
{
	data.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from account%d where uid=%d;", CCommonLogic::GetDataTableNum(uid), uid);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		LOG_DEBUG("uid:%d,m_szCommand:%s", uid, m_szCommand);
		return false;
	}
	if (vecData.size() != 1)
	{
		LOG_DEBUG("uid:%d,vecData_size:%d,m_szCommand:%s", uid, vecData.size(), m_szCommand);
		return false;
	}
	for (uint32 i = 0; i < vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];

		data.diamond = refRows["diamond"].as<int64>();
		data.coin = refRows["coin"].as<int64>();
		data.ingot = refRows["ingot"].as<int64>();
		data.score = refRows["score"].as<int64>();
		data.cvalue = refRows["cvalue"].as<int64>();
		data.vip = refRows["vip"].as<uint32>();
		data.safecoin = refRows["safecoin"].as<int64>();
		data.recharge = refRows["Recharge"].as<int64>();
		data.converts = refRows["converts"].as<int64>();
		data.userright = refRows["userright"].as<uint32>();
		data.win = refRows["win"].as<int64>();
		data.posrmb = refRows["posrmb"].as<uint32>();
		data.postime = refRows["postime"].as<uint32>();
		data.welcount = refRows["welcount"].as<int32>();
		data.weltime = refRows["weltime"].as<uint64>();
	}
	return true;
}

bool CDBOperator::GetUserInfoByUid(uint32 uid,string & nickname, string & city)
{
	nickname.clear();
	city.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from user%d where uid=%d;", CCommonLogic::GetDataTableNum(uid), uid);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	if (vecData.size() != 1)
	{
		return false;
	}

	for (uint32 i = 0; i < vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];

		nickname = refRows["nickname"].as<string>();
	}
	return true;
}

// 骰宝本周前十收入
bool    CDBOperator::LoadGameDataDiceWeekRank(vector<tagDiceGameDataRank> & vecGameData)
{
	vecGameData.clear();
	memset(m_szCommand, 0, sizeof(m_szCommand));
	//sprintf(m_szCommand, "select * from game_dice order by weekwinc desc limit 0,%d;", GAME_DATA_DICE_WEEK_RANK_COUNT);
	vector<map<string, MYSQLValue> > vecData;

	//int iRet = this->Query(m_szCommand, vecData);
	//if (iRet == -1)
	//{
	//	return false;
	//}
	//if (vecData.size() == 0)
	//{
	//	return false;
	//}
	//for (uint32 i = 0; i<vecData.size(); ++i)
	//{
	//	map<string, MYSQLValue> &refRows = vecData[i];
	//	tagDiceGameDataRank tagData;
	//	tagData.uid = refRows["uid"].as<uint32>();
	//	tagData.weekwinc = refRows["weekwinc"].as<int64>();

	//	GetUserInfoByUid(tagData.uid, tagData.nick_name, tagData.city);

	//	LOG_DEBUG("uid:%d,weekwinc:%lld,nick_name:%s", tagData.uid, tagData.weekwinc, tagData.nick_name.c_str());

	//	vecGameData.push_back(tagData);
	//}

	return true;
}

//// 加载自动杀分玩家列表
//bool 	CDBOperator::LoadAutoKillUsers(vector<uint32>& vecAutoKillUsers)
//{
//    memset(m_szCommand,0,sizeof(m_szCommand));
//    sprintf(m_szCommand,"select * from automatic_killing;");
//    vector<map<string,MYSQLValue> > vecData;
//
//    int iRet = this->Query(m_szCommand,vecData);
//    if(iRet == -1)
//        return false;
//	vecAutoKillUsers.clear();
//    for(uint32 i=0;i<vecData.size();++i)
//    {
//        map<string, MYSQLValue> &refRows = vecData[i];
//        uint32 uid  = refRows["uid"].as<uint32>();
//
//		LOG_DEBUG("uid:%d",uid);
//		vecAutoKillUsers.push_back(uid);
//    }
//
//    return true;
//}

// active_welfare_cfg配置表---活跃福利
bool    CDBOperator::LoadActiveWelfareCfg(map<uint8, stActiveWelfareCfg> &mpCfg)
{
    memset(m_szCommand, 0, sizeof(m_szCommand));
    sprintf(m_szCommand, "select * from active_welfare_cfg;");
    vector<map<string, MYSQLValue> > vecData;

    int iRet = this->Query(m_szCommand, vecData);
    if (iRet == -1)
    {
        return false;
    }
    mpCfg.clear();
    LOG_DEBUG("load_active_welfare_cfg - size:%d", vecData.size());
    for (uint32 i = 0; i < vecData.size(); ++i)
    {
        map<string, MYSQLValue> &refRows = vecData[i];
        int id = 0;
        string multi_games = "";
        string bettle_games = "";
        stActiveWelfareCfg cfg;
        id = refRows["id"].as<uint32>(); 
        cfg.min_loss = refRows["min_loss"].as<uint64>();
        cfg.max_loss = refRows["max_loss"].as<uint64>();
        cfg.welfare = refRows["welfare"].as<uint64>();
        cfg.max_win = refRows["max_win"].as<uint64>();
        cfg.multi_probability = refRows["multi_probability"].as<uint32>()/100;
        cfg.doublefight_probability = refRows["doublefight_probability"].as<uint32>()/100;
        multi_games = refRows["multiplayer_games"].as<string>();
        bettle_games = refRows["doublefight_games"].as<string>();

        LOG_DEBUG("record - ID:%d min_loss:%d max_loss:%d welfare:%d max_win:%d multi_probability:%d doublefight_probability:%d multi_games:%s bettle_games:%s",
            id, cfg.min_loss, cfg.max_loss, cfg.welfare, cfg.max_win, cfg.multi_probability, cfg.doublefight_probability, multi_games.c_str(), bettle_games.c_str());
       
        //解析百人场的游戏ID
        if(multi_games.size()>0)
        {            
            Json::Reader reader;
            Json::Value  jvalue;
            if (!reader.parse(multi_games, jvalue)) {
                LOG_ERROR("解析 multi_games:%s json串错误", multi_games.c_str());
                continue;
            }
            for (uint32 i = 0; i < jvalue.size(); ++i) 
            {               
                cfg.multiplayer_games.insert(jvalue[i].asUInt());
            }
        }

        //解析对战场的游戏ID
        if (bettle_games.size() > 0)
        {
            Json::Reader reader;
            Json::Value  jvalue;
            if (!reader.parse(bettle_games, jvalue)) {
                LOG_ERROR("解析 bettle_games:%s json串错误", bettle_games.c_str());
                continue;
            }
            for (uint32 i = 0; i < jvalue.size(); ++i)
            {
                cfg.doublefight_games.insert(jvalue[i].asUInt());
            }
        }
       
        mpCfg.insert(make_pair(id, cfg));
    }
    return true;
}

// new_register_welfare_cfg配置表---活跃福利
bool    CDBOperator::LoadNewRegisterWelfareCfg(stNewRegisterWelfareCfg &mpCfg)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from new_register_welfare_cfg;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	
	LOG_DEBUG("LoadNewRegisterWelfareCfg - size:%d", vecData.size());
	for (uint32 i = 0; i < vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
	
		string game_list = "";
				
		mpCfg.min_win_coin = refRows["min_win_coin"].as<uint64>();
		mpCfg.max_win_coin = refRows["max_win_coin"].as<uint64>();
		mpCfg.total_win = refRows["total_win"].as<uint64>();
		mpCfg.must_win = refRows["must_win"].as<uint64>();		
		game_list = refRows["game_list"].as<string>();
		mpCfg.total_win_rate = refRows["total_win_rate"].as<uint64>();
		mpCfg.period_day = refRows["period_day"].as<uint64>();

		LOG_DEBUG("record - min_win_coin:%d max_win_coin:%d total_win:%d must_win:%d game_list:%s total_win_rate:%d period_day:%d",
			mpCfg.min_win_coin, mpCfg.max_win_coin, mpCfg.total_win, mpCfg.must_win, game_list.c_str(), mpCfg.total_win_rate, mpCfg.period_day);

		//解析游戏ID
		if (game_list.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(game_list, jvalue)) {
				LOG_ERROR("解析 game_list:%s json串错误", game_list.c_str());
				continue;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				mpCfg.game_list.insert(jvalue[i].asUInt());
			}
		}		
	}
	return true;
}

// 加载精准控制配置信息
bool    CDBOperator::LoadUserControlCfg(map<uint32, tagUserControlCfg> & mapInfo)
{
	LOG_DEBUG("LoadUserControlCfg.");
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from user_control_cfg;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
		return false;
	mapInfo.clear();
	for (uint32 i = 0; i < vecData.size(); ++i) {
		map<string, MYSQLValue> &refRows = vecData[i];
		tagUserControlCfg cfg;
		cfg.suid = refRows["suid"].as<uint32>();
		cfg.sdeviceid = refRows["sdeviceid"].as<string>();
		cfg.tuid = refRows["tuid"].as<uint32>();
		string cgid_tmp = refRows["cgid"].as<string>();

		//解析游戏ID
		if (cgid_tmp.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(cgid_tmp, jvalue)) {
				LOG_ERROR("解析 game_list:%s json串错误", cgid_tmp.c_str());
				continue;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				cfg.cgid.insert(jvalue[i].asUInt());
			}
		}
		cfg.skey = refRows["skey"].as<string>();
		mapInfo.insert(make_pair(cfg.suid, cfg));
		LOG_DEBUG("suid:%d sdeviceid:%s tuid:%d cgid:%s skey:%s", cfg.suid, cfg.sdeviceid.c_str(), cfg.tuid, cgid_tmp.c_str(), cfg.skey.c_str());
	}
	return true;
}

void CDBOperator::LoadExclusiveAlipayRecharge(vector<tagExclusiveAlipayInfo> &vExclusiveInfo) {
	Json::Reader reader;
	Json::Value  jshowtime;
	Json::Value  jvip;

	vExclusiveInfo.clear();
	ZeroMemory(m_szCommand, sizeof(m_szCommand));
	sprintf(m_szCommand, 
		"SELECT account,name,title,min_pay,max_pay,lower_float,show_vip FROM chess_sysdata.alipay_account_cfg WHERE status=1 and max_pay_day>day_pay;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
		return;
	for (uint32 i = 0; i < vecData.size(); ++i) {
		map<string, MYSQLValue> &refRows = vecData[i];
		tagExclusiveAlipayInfo tagInfo;
		tagInfo.account = refRows["account"].as<string>();
		tagInfo.name = refRows["name"].as<string>();
		tagInfo.title = refRows["title"].as<string>();
		tagInfo.min_pay = refRows["min_pay"].as<int32>();
		tagInfo.max_pay = refRows["max_pay"].as<int32>();
		tagInfo.lower_float = refRows["lower_float"].as<int32>();
		string strVips = refRows["show_vip"].as<string>();
		Json::Reader reader;
		Json::Value  jvalve;
		if (!strVips.empty() && reader.parse(strVips, jvalve) && jvalve.isArray() && jvalve.size() != 0)
			for (uint32 i = 0; i < jvalve.size(); ++i)
				if (jvalve[i].isIntegral())
					tagInfo.vips.insert(jvalve[i].asInt());

		LOG_DEBUG("专属支付宝充值显示 - account:%s,name:%s,title:%s,min_pay:%d,max_pay:%d,lower_float:%d,strVips:%s", tagInfo.account.c_str(), 
			tagInfo.name.c_str(), tagInfo.title.c_str(), tagInfo.min_pay, tagInfo.max_pay, tagInfo.lower_float, strVips.c_str());
		vExclusiveInfo.push_back(tagInfo);
	}
}

// 加载玩家的幸运值配置
bool    CDBOperator::LoadLuckyCfg(uint32 uid, uint32 gameType, map<uint8, tagUserLuckyCfg> &mpLuckyCfg)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from lucky_cfg where uid = %d and gametype = %d;", uid, gameType);
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	mpLuckyCfg.clear();
	LOG_DEBUG("load_lucky_cfg - uid:%d gametype:%d size:%d", uid, gameType, vecData.size());
	for (uint32 i = 0; i < vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		tagUserLuckyCfg cfg;
		cfg.uid = refRows["uid"].as<uint32>();
		cfg.gametype = refRows["gametype"].as<uint16>();
		cfg.roomid = refRows["roomid"].as<uint16>();
		cfg.lucky_value = refRows["luckyvalue"].as<int32>();
		cfg.rate = refRows["rate"].as<uint32>();
		cfg.accumulated = refRows["accumulated"].as<int32>();
		
		LOG_DEBUG("lucky cfg info - uid:%d,gameType:%d,roomid:%d,lucky_value:%d,rate:%d,accumulated:%d",
				cfg.uid, cfg.gametype, cfg.roomid, cfg.lucky_value, cfg.rate, cfg.accumulated);
		
		auto iter_exist = mpLuckyCfg.find(cfg.roomid);
		if (iter_exist != mpLuckyCfg.end())
		{
			LOG_DEBUG("error - roomid:%d", cfg.roomid);
			continue;
		}
		mpLuckyCfg.insert(make_pair(cfg.roomid, cfg));
	}
	return true;
}

// 更新玩家的幸运值配置表
bool    CDBOperator::UpdateLuckyCfg(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyCfg)
{
	//当幸运值完成时，需要清除累计值
	if (mpLuckyCfg.lucky_value == 0)
	{
		mpLuckyCfg.accumulated = 0;
	}
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "update lucky_cfg set luckyvalue=%d,rate=%d,accumulated=%d where uid=%d and gametype=%d and roomid=%d;", mpLuckyCfg.lucky_value, mpLuckyCfg.rate, mpLuckyCfg.accumulated, uid, gameType, roomid);
	LOG_DEBUG("update lucky info sql. - m_szCommand:%s", m_szCommand);

	uint32 affectNum = this->GetAffectedNumExeSql(m_szCommand);
	if (affectNum != 1)
	{
		LOG_DEBUG("update lucky cfg fail. - m_szCommand:%s", m_szCommand);
		return false;
	}	
	return true;
}

// 更新玩家的幸运值日志表
bool    CDBOperator::UpdateLuckyLog(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyCfg)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "update lucky_cfg_log set accumulated=%d where uid=%d and gametype=%d and roomid=%d order by id desc limit 1;", 
		mpLuckyCfg.accumulated, uid, gameType, roomid);

	LOG_DEBUG("update lucky log sql. - m_szCommand:%s", m_szCommand);

	uint32 affectNum = this->GetAffectedNumExeSql(m_szCommand);
	if (affectNum != 1)
	{
		LOG_DEBUG("update lucky log fail. - m_szCommand:%s", m_szCommand);
		return false;
	}
	return true;
}

// 加载捕鱼配置表---fish_cfg表
bool    CDBOperator::LoadFishInfoCfg(map<uint8, tagFishInfoCfg> &mpCfg)
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf(m_szCommand, "select * from fish_cfg;");
	vector<map<string, MYSQLValue> > vecData;

	int iRet = this->Query(m_szCommand, vecData);
	if (iRet == -1)
	{
		return false;
	}
	mpCfg.clear();
	LOG_DEBUG("LoadFishInfoCfg - size:%d", vecData.size());
	for (uint32 i = 0; i < vecData.size(); ++i)
	{
		map<string, MYSQLValue> &refRows = vecData[i];
		
		tagFishInfoCfg cfg;
		cfg.id = refRows["id"].as<uint32>();
		cfg.prize_min = refRows["prize_min"].as<uint32>();
		cfg.prize_max = refRows["prize_max"].as<uint32>();
		cfg.kill_rate = refRows["kill_rate"].as<uint32>();
		
		LOG_DEBUG("record - ID:%d prize_min:%d prize_max:%d kill_rate:%d", cfg.id, cfg.prize_min, cfg.prize_max, cfg.kill_rate);
		mpCfg.insert(make_pair(cfg.id, cfg));
	}
	return true;
}