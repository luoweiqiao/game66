//
// Created by Joe on 2019-01-26 捕鱼配置管理类
//

#include "fish_info_mgr.h"
#include "stdafx.h"
#include "game_define.h"
#include "game_room.h"
#include "data_cfg_mgr.h"
#include "active_welfare_mgr.h"

using namespace std;

CFishCfgMgr::CFishCfgMgr()
{
	m_mpFishCfg.clear();
	m_mapCfgDataInfo.clear();
	m_vec_pro_elev.clear();
}
CFishCfgMgr::~CFishCfgMgr()
{
	m_mpFishCfg.clear();
	m_mapCfgDataInfo.clear();
}

bool CFishCfgMgr::Init()
{
    //启动时加载捕鱼配置表
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadFishInfoCfg(m_mpFishCfg);
    if (!bRet)
    {
		LOG_ERROR("get fish cfg table data is fail.");
        return false;
    }  
	//读取所有配置文件
	bRet = OnReadAllCfgFile();
	if (!bRet)
	{
		LOG_ERROR("read all config file is fail.");
		return false;
	}
    return true;
}

//读取所有配置文件
bool CFishCfgMgr::OnReadAllCfgFile()
{
	//读取配置文件---baseLineData.json
	std::string strShFileName = CHelper::GetExeDir() + "config/baseLineData.json";
	LOG_DEBUG("AAA file path :%s", strShFileName.c_str());

	CFileHelper oFile(strShFileName.c_str(), CFileHelper::MOD_RDONLY);
	if (!oFile.IsOpen())
	{
		LOG_ERROR("AAA open file fail filename:%s.", strShFileName.c_str());
		return false;
	}
	size_t fileLength = CFileHelper::GetFileSize(strShFileName);
	char * buffer = new char[fileLength + 1];
	if (buffer == NULL)
	{
		return false;
	}
	bool bReadSuccess = oFile.Read(buffer, fileLength);
	if (bReadSuccess)
	{
		OnReadBaseLineData(buffer);
	}

	//读取配置文件---fishRes.json
	strShFileName = CHelper::GetExeDir() + "config/fishRes.json";
	LOG_DEBUG("AAA file path :%s", strShFileName.c_str());

	CFileHelper oFile_2(strShFileName.c_str(), CFileHelper::MOD_RDONLY);
	if (!oFile_2.IsOpen())
	{
		LOG_ERROR("AAA open file fail filename:%s.", strShFileName.c_str());
		return false;
	}
	fileLength = CFileHelper::GetFileSize(strShFileName);
	char * buffer_fish = new char[fileLength + 1];
	if (buffer_fish == NULL)
	{
		return false;
	}
	bReadSuccess = oFile_2.Read(buffer_fish, fileLength);
	if (bReadSuccess)
	{
		OnReadFishRes(buffer_fish);
	}

	//读取配置文件---zidanSet.json
	strShFileName = CHelper::GetExeDir() + "config/zidanSet.json";
	LOG_DEBUG("AAA file path :%s", strShFileName.c_str());

	CFileHelper oFile_1(strShFileName.c_str(), CFileHelper::MOD_RDONLY);
	if (!oFile_1.IsOpen())
	{
		LOG_ERROR("AAA open file fail filename:%s.", strShFileName.c_str());
		return false;
	}
	fileLength = CFileHelper::GetFileSize(strShFileName);
	char * buffer_zidan = new char[fileLength + 1];
	if (buffer_zidan == NULL)
	{
		return false;
	}
	bReadSuccess = oFile_1.Read(buffer_zidan, fileLength);
	if (bReadSuccess)
	{
		OnReadZidanSet(buffer_zidan);
	}

	//读取地图配置文件---level1.json
	for (int i = 1; i <= MAX_MAP_NUM; i++)
	{
		char config_name[128];
		sprintf(config_name, "config/level%d.json", i);
		strShFileName = CHelper::GetExeDir() + config_name;
		LOG_DEBUG("AAA file path :%s", strShFileName.c_str());

		CFileHelper oFile_level(strShFileName.c_str(), CFileHelper::MOD_RDONLY);
		if (!oFile_level.IsOpen())
		{
			LOG_DEBUG("AAA open file fail filename:%s.", strShFileName.c_str());
			return true;
		}
		fileLength = CFileHelper::GetFileSize(strShFileName);
		char * buffer_level = new char[fileLength + 1];
		if (buffer_zidan == NULL)
		{
			return false;
		}
		bReadSuccess = oFile_level.Read(buffer_level, fileLength);
		if (bReadSuccess)
		{
			OnReadLevelSet(buffer_level, i);
		}

		//生成当前地图所有线路的坐标列表
		OnCreateFishDirect(i);
	}
	return true;
}

bool    CFishCfgMgr::GetFishCfgInfo(CGameRoom *pHostRoom, CGamePlayer* pPlayer, uint32 fish_id, uint32 &ret, bool &is_luck_ctrl, bool &is_luck_win, bool &is_aw_ctrl, bool &isNoviceCtrl)
{    
	if (pPlayer == NULL)
	{
		LOG_DEBUG("get player is null. return false.");
		return false;
	}

	uint32 room_id = pHostRoom->GetRoomID();
    
	/*
	概率范围：1 - 100
	幸运值正数：控制玩家盈利公式 = 鱼基础命中率 * （1 + 鱼倍数 * 概率 / 2500）
	幸运值负数：控制玩家输钱公式 = 鱼基数命中率 / （1 + 鱼倍数 * 概率 / 2500)
	*/

	//判断是否幸运值控制	
	uint32 luck_rate = 0;
	is_luck_ctrl = pPlayer->GetLuckyFlagForFish(room_id, is_luck_win, luck_rate);
	LOG_DEBUG("room_id:%d uid:%d fish_id:%d is_ctrl:%d is_win:%d luck_rate:%d", room_id, pPlayer->GetUID(), fish_id, is_luck_ctrl, is_luck_win, luck_rate);
    
	map<uint8, tagFishInfoCfg>::iterator iter = m_mpFishCfg.find(fish_id);
    if(iter!= m_mpFishCfg.end())
    {	
		ret = g_RandGen.GetRandi(iter->second.prize_min, iter->second.prize_max);
		uint32 kill_rate = iter->second.kill_rate;
		LOG_DEBUG("fish cfg info. prize_min:%d prize_max:%d kill_rate:%d ret:%d", iter->second.prize_min, iter->second.prize_max, iter->second.kill_rate, ret);
		
		//幸运值概率控制
		if (is_luck_ctrl)
		{
			float32 change_rate = 1.0 + (float32)ret * (float32)luck_rate / 2500.0;
			if (is_luck_win)
			{
				kill_rate = (float32)kill_rate * change_rate;
			}
			else
			{
				kill_rate = (float32)kill_rate / change_rate;
			}
			LOG_DEBUG("set luck ctrl. change_rate:%f kill_rate:%d", change_rate, kill_rate);
		}

		//新手福利控制
		isNoviceCtrl = false;
		if (!is_luck_ctrl && pPlayer->IsNoviceWelfare())
		{
			isNoviceCtrl = true;
			
			//获取新手福利的提升概率值
			uint32 posrmb = pPlayer->GetPosRmb();
			tagNewPlayerWelfareValue NewPlayerWelfareValue;
			NewPlayerWelfareValue = CDataCfgMgr::Instance().GetNewPlayerWelfareValue(pPlayer->GetUID(), posrmb);
			uint32 welfarepro = NewPlayerWelfareValue.welfarepro;

			//计算提升概率
			float32 change_rate = 1.0 + (float32)ret * (float32)welfarepro/200.0/2500.0;
			kill_rate = (float32)kill_rate * change_rate;

			LOG_DEBUG("set Novice ctrl. change_rate:%f kill_rate:%d", change_rate, kill_rate);
		}

		//库存控制
		bool isStockCtrl = false;
		double stockChangeRate = 0.0;
		double playerWinRate = 0.0;
		double jackpotSub = 0.0;
		stStockCfg &roomStockCfg = pHostRoom->GetRoomStockCfg();
		if (!is_luck_ctrl && !isNoviceCtrl) { 
			// 判断是否库存控制
			jackpotSub = labs(roomStockCfg.jackpot - roomStockCfg.jackpotMin) * PRO_DENO_10000 / roomStockCfg.jackpotCoefficient;
			if (roomStockCfg.jackpot < roomStockCfg.killPointsLine) {
				if (roomStockCfg.jackpot < 0)
					kill_rate = 0;
				else {
					playerWinRate = roomStockCfg.playerWinRate - jackpotSub;
					stockChangeRate = (PRO_DENO_10000 - playerWinRate) / 200.0;
					kill_rate = (float32)kill_rate / (1 + (float32)ret * (float32)stockChangeRate / 2500.0);
				}
				isStockCtrl = true;
			} else if (roomStockCfg.jackpot >= roomStockCfg.jackpotMin) {
				playerWinRate = roomStockCfg.jackpotRate + jackpotSub;
				stockChangeRate = playerWinRate / 200.0;
				kill_rate = (float32)kill_rate * (1 + (float32)ret * (float32)stockChangeRate / 2500.0);
				isStockCtrl = true;
			}
		}

		//活跃福利控制
		is_aw_ctrl = false;
		if (!is_luck_ctrl && !isNoviceCtrl && !isStockCtrl && pPlayer->IsEnterActiveWelfareRoomFlag(net::GAME_CATE_FISHING))
		{
			uint32 probability = 0;
			uint32 max_win = 0;
			bool flag = CAcTiveWelfareMgr::Instance().GetActiveWelfareCfgInfo(pPlayer->GetUID(), pPlayer->GetCurrAWID(), net::GAME_CATE_FISHING, max_win, probability);
			if (flag)
			{
				is_aw_ctrl = true;

				//计算提升概率
				probability = probability * 100;
				float32 change_rate = 1.0 + (float32)ret * (float32)probability / 200.0 / 2500.0;
				kill_rate = (float32)kill_rate * change_rate;

				LOG_DEBUG("set AW ctrl. change_rate:%f kill_rate:%d probability:%d ret:%d", change_rate, kill_rate, probability, ret);
			}
		}

		//提升/降低击中鱼的概率		
		if (!is_luck_ctrl && !isNoviceCtrl && !is_aw_ctrl && !isStockCtrl)
		{				
			uint32 tmp_value = g_RandGen.GetRandi(1, m_vec_pro_elev.size());
			uint32 old_kill_rate = kill_rate;
			kill_rate = kill_rate * m_vec_pro_elev[tmp_value-1] / 100;
			LOG_DEBUG(" default tmp_value:%d old_kill_rate:%d new_kill_rate:%d .", tmp_value, old_kill_rate, kill_rate);
		}

		//判断命中概率
		uint32 rand_value = g_RandGen.GetRandi(1, PRO_DENO_10000);
		if (rand_value > kill_rate)
		{
			isNoviceCtrl = false;
			is_aw_ctrl = false;
			LOG_DEBUG("fish not kill roomid:%d,fish_id:%d,rand_value:%d,kill_rate:%d,ret:%d,is_luck_ctrl:%d,isStockCtrl:%d,stockChangeRate:%f,jackpot:%lld,killPointsLine:%lld,jackpotMin:%lld,playerWinRate:%d,jackpotCoefficient:%d,jackpotRate:%d,realplayerWinRate:%f,jackpotSub:%f", 
				room_id, fish_id, rand_value, kill_rate, ret, is_luck_ctrl, isStockCtrl, stockChangeRate,
				roomStockCfg.jackpot, roomStockCfg.killPointsLine, roomStockCfg.jackpotMin, roomStockCfg.playerWinRate, roomStockCfg.jackpotCoefficient, roomStockCfg.jackpotRate, playerWinRate, jackpotSub);
			return false;
		}
		
		LOG_DEBUG("fish is kill uid:%lld,roomid:%d,fish_id:%d,rand_value:%d,kill_rate:%d,ret:%d,is_luck_ctrl:%d,isNoviceCtrl:%d,is_aw_ctrl:%d,isStockCtrl:%d,stockChangeRate:%f,jackpot:%lld,killPointsLine:%lld,jackpotMin:%lld,playerWinRate:%d,jackpotCoefficient:%d,jackpotRate:%d,realplayerWinRate:%f,jackpotSub:%f",
			pPlayer->GetUID(), room_id, fish_id, rand_value, kill_rate, ret, is_luck_ctrl, isNoviceCtrl, is_aw_ctrl, isStockCtrl, stockChangeRate,
			roomStockCfg.jackpot, roomStockCfg.killPointsLine, roomStockCfg.jackpotMin, roomStockCfg.playerWinRate, roomStockCfg.jackpotCoefficient, roomStockCfg.jackpotRate, playerWinRate, jackpotSub);
		return true;
    } 
	else
	{
		LOG_DEBUG("fish_id:%d is not exist.", fish_id);
		return false;
	}	
}

// PHP更新配置信息
void   CFishCfgMgr::UpdateFishCfg(uint32 id, uint32 prize_min, uint32 prize_max, uint32 kill_rate)
{
    LOG_DEBUG("UpdateFishCfg - id:%d prize_min:%d prize_max:%d kill_rate:%d", id, prize_min, prize_max, kill_rate);
    
	tagFishInfoCfg  info;
	info.id = id;
	info.prize_min = prize_min;
	info.prize_max = prize_max;
	info.kill_rate = kill_rate;
	m_mpFishCfg[id] = info;
}

//二阶贝塞尔曲线
//bool CFishCfgMgr::GetBezierCurve(vector<CoordinateInfo> &inCInfo, vector<CoordinateInfo> &outCInfo, uint32 allTime, uint32 flushTime)
//{
//	LOG_DEBUG("inCInfo size:%d allTime:%d flushTime:%d", inCInfo.size(), allTime, flushTime);
//
//	if (allTime == 0 || flushTime == 0)
//	{
//		LOG_ERROR("allTime or flushTime is zero.");
//		return false;
//	}
//
//	uint16 tcunts = allTime / flushTime; //份数
//
//	//输入参数
//	for (std::size_t i = 0; i < inCInfo.size(); i++)
//	{
//		LOG_DEBUG("inCInfo x:%d y:%d", inCInfo[i].x, inCInfo[i].y);
//	}
//
//	for (int i = 1; i <= tcunts; i++)
//	{
//		vector<CoordinateInfo> tmp_outCInfo;
//		fjfunction(inCInfo, tcunts, i, tmp_outCInfo);
//
//		for (std::size_t j = 0; j < tmp_outCInfo.size(); j++)
//		{
//			//LOG_DEBUG("tmp_outCInfo x:%d y:%d", tmp_outCInfo[j].x, tmp_outCInfo[j].y);
//			outCInfo.push_back(tmp_outCInfo[j]);
//		}
//	}
//
//	//输出参数
//	for (std::size_t i = 0; i < outCInfo.size(); i++)
//	{
//		//LOG_DEBUG("outCInfo x:%d y:%d", outCInfo[i].x, outCInfo[i].y);
//	}
//
//	return true;
//}

//bool CFishCfgMgr::fjfunction(vector<CoordinateInfo> pList, int32 tc, int32 index, vector<CoordinateInfo> &outCInfo)
//{
//	CoordinateInfo p;
//	vector<CoordinateInfo> leftLine;
//
//	uint8 len = pList.size() - 1;
//	for (uint8 i = 0; i < len; i++)
//	{
//		int32 sp_x = pList[i].x;
//		int32 sp_y = pList[i].y;
//
//		int32 ep_x = pList[i + 1].x;
//		int32 ep_y = pList[i + 1].y;
//
//		//LOG_DEBUG("A sp_x:%d sp_y:%d ep_x:%d ep_y:%d", sp_x, sp_y, ep_x, ep_y);
//
//		int32 addx = (ep_x - sp_x) / tc;
//		int32 addy = (ep_y - sp_y) / tc;
//
//		int32 nx = sp_x + addx * index;
//		int32 ny = sp_y + addy * index;
//
//		CoordinateInfo info;
//		info.x = nx;
//		info.y = ny;
//		leftLine.push_back(info);
//		//LOG_DEBUG("AAA addx:%d addy:%d x:%d y:%d index:%d", addx, addy, info.x, info.y, index);
//	}
//
//	if (leftLine.size() == 1)
//	{
//		outCInfo.push_back(leftLine[0]);
//		return true;
//	}
//	else
//	{
//		fjfunction(leftLine, tc, index, outCInfo);
//	}
//	return true;
//}

bool	CFishCfgMgr::OnReadBaseLineData(string msg)
{
	//解析配置文件json格式
	Json::Reader reader;// 解析json用Json::Reader   
	Json::Value root; // Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array    

	if (!reader.parse(msg, root))
	{
		LOG_ERROR("AAA json analysis error");
		return false;
	}

	if (!root.isMember("lineData"))
	{
		LOG_ERROR("AAA json analysis lineData error");
		return false;
	}

	int lineData_size = root["lineData"].size();  // 得到"lineData"的数组个数  
	LOG_DEBUG("AAA lineData_size:%d", lineData_size);
	for (int i = 0; i < lineData_size; ++i)  // 遍历数组  
	{
		lineDataInfo ld_info;
		ld_info.id = root["lineData"][i]["id"].asUInt();
		ld_info.name = root["lineData"][i]["name"].asString();

		if (root["lineData"][i].isMember("count"))
			ld_info.count = root["lineData"][i]["count"].asUInt();

		if (root["lineData"][i].isMember("fishWaitTime"))
			ld_info.fishWaitTime = root["lineData"][i]["fishWaitTime"].asUInt();

		if (root["lineData"][i].isMember("color"))
			ld_info.color = root["lineData"][i]["color"].asUInt();

		//判断walkLines是否存在
		if (root["lineData"][i].isMember("walkLines"))
		{
			Json::Value walkLines = root["lineData"][i]["walkLines"];			
			int walkLines_size = walkLines.size();
			for (int j = 0; j < walkLines_size; ++j)  // 遍历数组  
			{				
				if (walkLines[j].isMember("walktime"))
				{
					int32 walktime = walkLines[j]["walktime"].asInt();
					ld_info.walkLines.push_back(walktime);
				}
				else
				{
					LOG_DEBUG("AAA get walktime is fail");
				}						
			}
		}

		//判断fishSet是否存在
		if (root["lineData"][i].isMember("fishSet"))
		{
			int fishSet_size = root["lineData"][i]["fishSet"].size();
			for (int k = 0; k < fishSet_size; ++k)  // 遍历数组  
			{
				fishSetInfo fs_info;
				Json::Value fishSetInfo = root["lineData"][i]["fishSet"][k];
				fs_info.id = fishSetInfo["id"].asUInt();				
				fs_info.startTime = fishSetInfo["startTime"].asUInt();

				//判断walkLines是否存在
				if (fishSetInfo.isMember("walkLines"))
				{
					Json::Value walkLines = fishSetInfo["walkLines"];
					int walkLines_size = walkLines.size();
					for (int j = 0; j < walkLines_size; ++j)  // 遍历数组  
					{
						
						uint32  walktime = walkLines[j]["walktime"].asUInt();					
						fs_info.walkLines.push_back(walktime);
					}
				}
				ld_info.fishSets.push_back(fs_info);
			}
		}
		m_mapLineCfgData[ld_info.id] = ld_info;
	}	
	return true;
}

bool	CFishCfgMgr::OnReadFishRes(string msg)
{
	//解析配置文件json格式
	Json::Reader reader;// 解析json用Json::Reader   
	Json::Value root; // Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array    

	if (!reader.parse(msg, root))
	{
		LOG_ERROR("AAA json analysis error");
		return false;
	}

	if (!root.isMember("fdata"))
	{
		LOG_ERROR("AAA json analysis fdata error");
		return false;
	}

	int fdata_size = root["fdata"].size();  // 得到"fdata"的数组个数  
	LOG_DEBUG("AAA fdata_size:%d", fdata_size);
	for (int i = 0; i < fdata_size; ++i)  // 遍历数组  
	{
		FishDataInfo fd_info;
		fd_info.id = root["fdata"][i]["id"].asUInt();
		fd_info.fx = root["fdata"][i]["fx"].asUInt();
		fd_info.name = root["fdata"][i]["name"].asString();
		fd_info.res = root["fdata"][i]["res"].asString();
		fd_info.sFrame = root["fdata"][i]["sFrame"].asUInt();
		fd_info.eFrame = root["fdata"][i]["eFrame"].asUInt();
		fd_info.R = root["fdata"][i]["R"].asUInt();
		fd_info.life = root["fdata"][i]["life"].asUInt();
		fd_info.gold = root["fdata"][i]["gold"].asUInt();
		fd_info.minM = root["fdata"][i]["minM"].asUInt();
		fd_info.maxM = root["fdata"][i]["maxM"].asUInt();
		fd_info.shx = root["fdata"][i]["shx"].asInt();
		fd_info.shy = root["fdata"][i]["shy"].asInt();
		fd_info.seabed = root["fdata"][i]["seabed"].asInt();

		if (root["fdata"][i].isMember("boomID"))
		{
			int boomID_size = root["fdata"][i]["boomID"].size();
			for (int j = 0; j < boomID_size; j++)
			{
				uint16 tmp = root["fdata"][i]["boomID"][j].asUInt();
				fd_info.vecboomID.push_back(tmp);
			}
		}
		m_mapFishCfgData[fd_info.id] = fd_info;
	}

	//输出所有配置
	LOG_DEBUG("fdata");
	for (auto &iter : m_mapFishCfgData)
	{
		FishDataInfo tagInfo = iter.second;
		LOG_DEBUG("id:%d fx:%d name:%s res:%s sFrame:%d eFrame:%d R:%d life:%d gold:%d minM:%d maxM:%d shx:%d shy:%d seabed:%d",
			tagInfo.id, tagInfo.fx, tagInfo.name.c_str(), tagInfo.res.c_str(), tagInfo.sFrame, tagInfo.eFrame, tagInfo.R, tagInfo.life,
			tagInfo.gold, tagInfo.minM, tagInfo.maxM, tagInfo.shx, tagInfo.shy, tagInfo.seabed);

		for (size_t i = 0; i < tagInfo.vecboomID.size(); i++)
		{
			LOG_DEBUG("boomID:%d", tagInfo.vecboomID[i]);
		}
	}
	return true;
}

bool	CFishCfgMgr::OnReadZidanSet(string msg)
{
	//解析配置文件json格式
	Json::Reader reader;// 解析json用Json::Reader   
	Json::Value root; // Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array    

	if (!reader.parse(msg, root))
	{
		LOG_ERROR("AAA json analysis error");
		return false;
	}

	if (!root.isMember("zidanData"))
	{
		LOG_ERROR("AAA json analysis zidanData error");
		return false;
	}

	int zdata_size = root["zidanData"].size();  // 得到"zidanData"的数组个数  
	LOG_DEBUG("AAA zdata_size:%d", zdata_size);
	for (int i = 0; i < zdata_size; ++i)  // 遍历数组  
	{
		ZidanDataInfo zd_info;
		zd_info.id = root["zidanData"][i]["id"].asUInt();
		zd_info.fireCD = root["zidanData"][i]["fireCD"].asUInt();
		zd_info.fireType = root["zidanData"][i]["fireType"].asUInt();
		zd_info.name = root["zidanData"][i]["name"].asString();
		zd_info.type = root["zidanData"][i]["type"].asUInt();
		zd_info.lv = root["zidanData"][i]["lv"].asUInt();
		zd_info.R = root["zidanData"][i]["R"].asUInt();
		zd_info.speed = root["zidanData"][i]["speed"].asUInt();
		zd_info.destroy = root["zidanData"][i]["destroy"].asUInt();
		zd_info.money = root["zidanData"][i]["money"].asUInt();
		zd_info.killCD = root["zidanData"][i]["killCD"].asUInt();
		zd_info.rebound = root["zidanData"][i]["rebound"].asInt();
		m_mapZidanCfgData[zd_info.id] = zd_info;
	}

	//输出所有配置
	LOG_DEBUG("zidanData");
	for (auto &iter : m_mapZidanCfgData)
	{
		ZidanDataInfo tagInfo = iter.second;
		LOG_DEBUG("id:%d fireCD:%d fireType:%d name:%s type:%d lv:%d R:%d speed:%d destroy:%d money:%d killCD:%d rebound:%d",
			tagInfo.id, tagInfo.fireCD, tagInfo.fireType, tagInfo.name.c_str(), tagInfo.type, tagInfo.lv, tagInfo.R, tagInfo.speed,
			tagInfo.destroy, tagInfo.money, tagInfo.killCD, tagInfo.rebound);
	}
	return true;
}

bool	CFishCfgMgr::OnReadLevelSet(string msg, uint8 level_id)
{
	//解析配置文件json格式
	Json::Reader reader;// 解析json用Json::Reader   
	Json::Value root; // Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array    

	if (!reader.parse(msg, root))
	{
		LOG_ERROR("AAA json analysis error");
		return false;
	}

	if (!root.isMember("allTime"))
	{
		LOG_ERROR("AAA json analysis zidanData error");
		return false;
	}
	else
	{
		m_mapCostTimeInfo[level_id] = root["allTime"].asUInt64();
	}

	if (!root.isMember("levelSet"))
	{
		LOG_ERROR("AAA json analysis zidanData error");
		return false;
	}

	int levelSet_size = root["levelSet"].size();	// 得到"levelSet"的数组个数  
	LOG_DEBUG("AAA allTime:%d levelSet_size:%d", root["allTime"].asUInt64(), levelSet_size);

	vector<MapCfgDataInfo> vec_mapdata_info;

	for (int i = 0; i < levelSet_size; ++i)  // 遍历数组  
	{
		MapCfgDataInfo md_info;
		md_info.id			= root["levelSet"][i]["id"].asUInt();
		md_info.waitTime	= root["levelSet"][i]["waitTime"].asUInt();
		md_info.lineID		= root["levelSet"][i]["lineID"].asUInt();
		md_info.walkTime	= root["levelSet"][i]["walkTime"].asFloat();		
		md_info.count		= root["levelSet"][i]["count"].asUInt();
		md_info.fishID		= root["levelSet"][i]["fishID"].asUInt();
		md_info.fishWaitTime= root["levelSet"][i]["fishWaitTime"].asUInt();		

		//过滤掉没有配置fishID的记录
		if (md_info.fishID != 0)
		{
			vec_mapdata_info.push_back(md_info);
		}
	}

	//将当前地图数据置入地图列表
	m_mapCfgDataInfo[level_id] = vec_mapdata_info;

	//输出所有配置
	LOG_DEBUG("level %d", level_id);
	for (auto &iter : m_mapCfgDataInfo)
	{
		vector<MapCfgDataInfo>::iterator iter_line = iter.second.begin();
		for (; iter_line != iter.second.end(); iter_line++)
		{
			LOG_DEBUG("id:%d waitTime:%d lineID:%d walkTime:%f count:%d fishID:%d fishWaitTime:%d",
				iter_line->id, iter_line->waitTime, iter_line->lineID, iter_line->walkTime, iter_line->count,iter_line->fishID, iter_line->fishWaitTime);
		}
	}
	return true;
}

//创建当前地图的鱼群线路
void  CFishCfgMgr::OnCreateFishDirect(uint32 map_id)
{	
	LOG_DEBUG("OnCreateFishDirect - map_id:%d ", map_id);

	//先查找当前地图ID是否存在
	map<uint32, vector<MapCfgDataInfo>>::iterator iter_level = m_mapCfgDataInfo.find(map_id);
	if (iter_level == m_mapCfgDataInfo.end())
	{
		LOG_ERROR("The level is not exist in map list. level:%d", map_id);
	}
	else
	{
		//遍历当前地图的所有线路
		vector<MapCfgDataInfo>::iterator iter_line = iter_level->second.begin();
		for (; iter_line != iter_level->second.end(); iter_line++)
		{
			uint32 line_id = iter_line->lineID;
			
			//判断路径ID是否在baseLineData.json配置文件中存在
			map<uint32, lineDataInfo>::iterator iter_direct = m_mapLineCfgData.find(line_id);
			if (iter_direct == m_mapLineCfgData.end())
			{
				LOG_ERROR("The line id is not exist in direct list. line_id:%d", line_id);
			}
			else
			{
				uint32 walkLines_cost_time = 0;
				uint32 fishSet_cost_time = 0;

				//同一路径多鱼的情况---walkLines
				for (size_t i=0; i< iter_direct->second.walkLines.size(); i++)
				{
					walkLines_cost_time += iter_direct->second.walkLines[i];
				}
				
				//同一路径单鱼的情况---fishSet
				for (size_t i = 0; i < iter_direct->second.fishSets.size(); i++)
				{			
					createFishSetInfo info;
					info.id = iter_direct->second.fishSets[i].id;
					fishSet_cost_time = 0;
					for (size_t j = 0; j < iter_direct->second.fishSets[i].walkLines.size(); j++)
					{						
						fishSet_cost_time += iter_direct->second.fishSets[i].walkLines[j];						
					}	
					info.cost_time = fishSet_cost_time * iter_line->walkTime;
					iter_line->fs_list.push_back(info);
				}		

				if (walkLines_cost_time > 0)
				{
					iter_line->wl_cost_time = walkLines_cost_time * iter_line->walkTime;
					iter_line->is_single = false;
				}
				else
				{					
					iter_line->is_single = true;
				}

				LOG_DEBUG("id:%d line id:%d walk_percent:%f wl_cost_time:%d is_single:%d walkLines_cost_time:%d fs_list.size():%d", 
					iter_line->id, line_id, iter_line->walkTime, iter_line->wl_cost_time, iter_line->is_single, walkLines_cost_time, iter_line->fs_list.size());
			}
		}
	}
}

// 根据地图ID获取对应的地图信息
bool  CFishCfgMgr::GetCurrMapInfo(uint8 map_level, uint32 &map_allTime, vector<CurrMapInfo> &map_info)
{
	uint64 curr_time = getTickCount64();		//获取当前系统时间(毫秒)
	LOG_DEBUG(" get current map info. map_level:%d curr_time:%llu", map_level, curr_time);

	//查找对应地图的行走时间
	map<uint32, uint32>::iterator iter = m_mapCostTimeInfo.find(map_level);
	if (iter == m_mapCostTimeInfo.end())
	{		
		LOG_ERROR("the maplevel %d is not find allTime in level json file.", map_level);
		return false;
	}
	else
	{
		map_allTime = iter->second;		
	}	

	map_info.clear();

	//先查找当前地图ID是否存在
	map<uint32, vector<MapCfgDataInfo>>::iterator iter_level = m_mapCfgDataInfo.find(map_level);
	if (iter_level == m_mapCfgDataInfo.end())
	{
		LOG_ERROR("The level is not exist in map list. level:%d", map_level);
		return false;
	}
	else
	{
		//遍历当前地图的所有线路
		vector<MapCfgDataInfo>::iterator iter_line = iter_level->second.begin();
		for (; iter_line != iter_level->second.end(); iter_line++)
		{
			CurrMapInfo curr_info;
			curr_info.id = iter_line->id;		//当前线路ID
			curr_info.waitTime = iter_line->waitTime;
			curr_info.lineID = iter_line->lineID;
			curr_info.count = iter_line->count;
			curr_info.fishID = iter_line->fishID;
			curr_info.fishWaitTime = iter_line->fishWaitTime;
			curr_info.is_single = iter_line->is_single;
			curr_info.wl_cost_time = iter_line->wl_cost_time;
			curr_info.fs_list.clear();
			for (size_t i = 0; i < iter_line->fs_list.size(); i++)
			{
				createFishSetInfo  info;
				info.id = iter_line->fs_list[i].id;
				info.cost_time = iter_line->fs_list[i].cost_time;
				curr_info.fs_list.push_back(info);
			}
			
			//设置下次产生鱼的信息
			curr_info.create_info.fish_curr = 0;
			curr_info.create_info.next_create_time = curr_time + curr_info.waitTime;
			curr_info.create_info.fish_serial = 0;
			curr_info.create_info.is_create = true;
			LOG_DEBUG("set curr map fish create time. id:%d lineID:%d waitTime:%d next_create_time:%llu is_single:%d wl_cost_time:%d", 
				curr_info.id, curr_info.lineID, curr_info.waitTime, curr_info.create_info.next_create_time, curr_info.is_single, curr_info.wl_cost_time);
			map_info.push_back(curr_info);
		}		
	}

	return true;
}

// 重置提升/降低击中鱼的概率列表---房间配置
void   CFishCfgMgr::ResetHitFishProEvelCfg(vector<uint32> &vec_pro_elev)
{
	LOG_DEBUG("ResetHitFishProEvelCfg - vec_pro_elev size:%d", vec_pro_elev.size());
	m_vec_pro_elev.clear();
	for (uint16 i = 0; i < vec_pro_elev.size(); i++)
	{
		m_vec_pro_elev.push_back(vec_pro_elev[i]);
		LOG_DEBUG("Reset - i:%d value:%d", i, vec_pro_elev[i]);
	}
}