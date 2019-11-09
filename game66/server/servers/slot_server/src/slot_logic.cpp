#include "slot_logic.h"


using namespace game_slot;
using namespace svrlib;
//静态变量
namespace game_slot
{

	bool compareBySysKey(const WheelInfo & info, const compareDef & defRate)
	{
		if (info.TotalSysRatio[defRate.linenum][defRate.index] < defRate.compRate)
		{
			//LOG_DEBUG("CSlotLogic::compareBySysKey,info[=%d].TotalSysRatio[%d][%d]=%d < defRate.compRate=%d", info.id,defRate.linenum, defRate.index, info.TotalSysRatio[defRate.linenum][defRate.index], defRate.compRate);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool compareByUseKey(const WheelInfo & info, const compareDef & defRate)
	{
		if (info.TotalUserRatio[defRate.linenum][defRate.index] < defRate.compRate)
		{
			//LOG_DEBUG("CSlotLogic::compareBySysKey,info[%d].TotalUserRatio[%d][%d]=%d < defRate.compRate=%d", info.id,defRate.linenum, defRate.index, info.TotalUserRatio[defRate.linenum][defRate.index],defRate.compRate) ;
			return true;
		}
		else
		{
			return false;
		}
	}

    //构造函数
	CSlotLogic::CSlotLogic()
	{
		Init();
	}

	//析构函数
	CSlotLogic::~CSlotLogic()
	{

	}

	//初始化
	void CSlotLogic::Init()
	{
		memset(&m_WeightPicCfg, 0, sizeof(m_WeightPicCfg));
		memset(m_ReturnUserRate, 0, sizeof(m_ReturnUserRate));
		memset(m_ReturnSysRate, 0, sizeof(m_ReturnSysRate));
		memset(m_BonusFreeTimes, 0, sizeof(m_BonusFreeTimes));
		m_IsNewWheel = false;
		m_FindWheelInfo.clear();
		m_NoRateWheel.clear();
		//MakeWheels();
	}

	void CSlotLogic::MakeWheels()
	{

		//for (int f = 0; f < MAXCOLUMN; f++)
		//{
		//	LOG_DEBUG("CSlotLogic::MakeWheels->WeightPicCfg:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", m_WeightPicCfg[f][0], m_WeightPicCfg[f][1], m_WeightPicCfg[f][2], m_WeightPicCfg[f][3],
		//		m_WeightPicCfg[f][4], m_WeightPicCfg[f][5], m_WeightPicCfg[f][6], m_WeightPicCfg[f][7], m_WeightPicCfg[f][8], m_WeightPicCfg[f][9], m_WeightPicCfg[f][10]);
		//}
		m_WheelInfo.clear();
		LOG_DEBUG("CSlotLogic::MakeWheels");
		uint64  WheelMultipleCount[MAX_LINE];//统计所有有赔率的周图库数量
		memset(&WheelMultipleCount, 0, sizeof(WheelMultipleCount));
		uint32  GamePic[MAXROW][MAXCOLUMN];//矩阵3*5
		memset(&GamePic, 0, sizeof(GamePic));
		//uint32  nWeightPicCfg[MAXWEIGHT];
		//memset(&nWeightPicCfg, 0, sizeof(nWeightPicCfg));MAX_WHEELS
		uint64 nTotal9Multiple = 0;
		//生成图集库
		for(uint32 i = 0; i < MAX_WHEELS;i++)
		{

			//////最后100个为不中奖
			if(i >= (MAX_WHEELS - 100))
			{	
				memset(&GamePic, 0, sizeof(GamePic));
				for (uint32 j = 0; j < MAXCOLUMN; j++)
				{
					for (uint32 k = 0; k < MAXROW; k++)
					{
						if (j == 1)
						{
							uint32 nIndex = GetNoRateWeightIndex(GamePic[0][0], GamePic[1][0], GamePic[2][0]);
							GamePic[k][j] = nIndex + 1;//矩阵3*5	
						}
						else
						{
							uint32 nIndex = GetNoRateWeightIndex(0, 0, 0);
							GamePic[k][j] = nIndex + 1;//矩阵3*5	
						}
					}
				}
			}
			else if (i >= (MAX_WHEELS - 200) && i < (MAX_WHEELS - 100))
			{
				for (uint32 j = 0; j < MAXCOLUMN; j++)
				{
					for (uint32 k = 0; k < MAXROW; k++)
					{
						uint32 nIndex = GetBananaWeightIndex(j);
						GamePic[k][j] = nIndex + 1;//矩阵3*5	
					}
				}
			}
			else
			{
				for (uint32 j = 0; j < MAXCOLUMN; j++)
				{
					for (uint32 k = 0; k < MAXROW; k++)
					{
						//memcpy(nWeightPicCfg, &m_WeightPicCfg[j][0], 10*4);
						uint32 nIndex = GetWeightIndex(j);
						GamePic[k][j] = nIndex + 1;//矩阵3*5					
					}
				}
			}

			//计算所有的线的倍率
			//计算赔率
			uint64 nTotalMultiple = 0;//总倍率	
			WheelInfo  info;			
			memset(&info, 0,sizeof(info));
			info.id = i;//序号
			memcpy(info.pic[0], GamePic[0], MAXROW *MAXCOLUMN*4);	//保存单个图集		
			for (uint32 j = 0; j < MAX_LINE; j++)
			{
				uint32 linePic[MAXCOLUMN] = { 0 };//顺序的线
				//计算每条线的赔率
				for (int k = 0; k < MAXCOLUMN; k++)
				{
					linePic[k] = GamePic[(m_LogicLinePos[j].pos[k] - 1) % MAXROW][(m_LogicLinePos[j].pos[k] - 1) / MAXROW];
				}
				//统计线的结果
				picLinkinfo pLink;
				pLink.lineNo = j + 1;//线号
				//计算赔率
				uint64 nMultipleTmp = GetLineMultiple(linePic, pLink);
				if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
				{
					//picLinkVec.push_back(pLink);
					nTotalMultiple += nMultipleTmp;
				}
				else if (pLink.picID == PIC_BONUS)//计算bouns的赔率
				{
					info.LineBonus[pLink.linkNum] += m_BonusFreeTimes[pLink.linkNum];//保存bonus获得的免费次数
				}
				info.LineMultiple[j] = nTotalMultiple;//每条线对应的总赔率	
				if (info.LineMultiple[j] > 0 || info.LineBonus[pLink.linkNum] >0) //统计当前赔率不为0个数
				{
					WheelMultipleCount[j]++;
					if (j == 8)
					{
						nTotal9Multiple += info.LineMultiple[j];
					}
				}
			}
			//加入图集库		
			m_WheelInfo.push_back(info);
		}
		LOG_DEBUG("CSlotLogic::MakeWheels size=%d,WheelMultipleCount[0-9]=%d,%d,nTotal9Multiple=%lld", m_WheelInfo.size(), WheelMultipleCount[0], WheelMultipleCount[8], nTotal9Multiple);
		//判断生存的图集数是否少于需要的个数
		if (m_WheelInfo.size() < MAX_WHEELS || WheelMultipleCount[8] == 0 || WheelMultipleCount[0] == 0)
		{
			LOG_DEBUG("CSlotLogic::MakeWheels size < MAX_WHEELS=%d,WheelMultipleCount[0]=%d", m_WheelInfo.size(), WheelMultipleCount[0]);
			return;
		}
		//计算图集概率
		uint32    nTotalSysRatio[MAX_LINE][5];      //9线系统返奖1-5调节和		
		uint32    nTotalUserRatio[MAX_LINE][2];     //9线个人返奖1-2调节和
		memset(&nTotalSysRatio, 0, sizeof(nTotalSysRatio));
		memset(&nTotalUserRatio, 0, sizeof(nTotalUserRatio));
		vector<WheelInfo>::iterator iter = m_WheelInfo.begin();
		for (;iter != m_WheelInfo.end(); iter++)
		{
			//计算每个线的返奖率+
			for (uint32 i = 0; i < MAX_LINE; i++)
			{
				//系统返奖
				uint32 k = 0;
				for (k = 0; k < 5; k++)
				{
					//倍数为0的时候概率为0
					if (0 == iter->LineMultiple[i])
					{
						iter->SysRatio[i][k] = 0;
					}
					else
					{
						//LOG_DEBUG("CSlotLogic::MakeWheels m_FeeRate=%d,m_JackpotRate=%d,m_ReturnSysRate[%d]=%d,WheelMultipleCount=%d, iter->LineMultiple[i]=%d,iter->LineBonus[i]=%d",
						//	m_FeeRate, m_JackpotRate, k, m_ReturnSysRate[k], WheelMultipleCount, iter->LineMultiple[i], iter->LineBonus[i]);
						uint64 tmp = (MAX_RATIO - m_FeeRate* 10000 - m_JackpotRate * 10000 + m_ReturnSysRate[k] * 10000) * 100  / WheelMultipleCount[i];
						iter->SysRatio[i][k] = (tmp / ((iter->LineMultiple[i] * 100) / (i+1) + iter->LineBonus[i] * 100));
					}
					nTotalSysRatio[i][k]      +=  iter->SysRatio[i][k];
					iter->TotalSysRatio[i][k]  =  nTotalSysRatio[i][k];

					//LOG_DEBUG("CSlotLogic::MakeWheels id=%d nTotalSysRatio[i][k]=%d,SysRatio[i][k]=%d,m_FeeRate=%d,m_JackpotRate=%d,m_ReturnSysRate[k]=%d,WheelMultipleCount=%d, iter->LineMultiple[i]=%d,iter->LineBonus[i]=%d",
					//	iter->id,nTotalSysRatio[i][k],iter->SysRatio[i][k],m_FeeRate, m_JackpotRate, m_ReturnSysRate[k], WheelMultipleCount[i], iter->LineMultiple[i], iter->LineBonus[i]);
					////中奖概率和不能大于MAX_WHEELS
					if (iter->TotalSysRatio[i][k] > MAX_RATIO)
					{
						LOG_DEBUG("CSlotLogic::MakeWheels is erro id =%d,TotalSysRatio[%d][%d]=%d", iter->id,i,k,iter->TotalSysRatio[i][k]);
						LOG_DEBUG("CSlotLogic::MakeWheels m_FeeRate=%d,m_JackpotRate=%d,m_ReturnSysRate[k]=%d,WheelMultipleCount=%d, iter->LineMultiple[i]=%d,iter->LineBonus[i]=%d",
							m_FeeRate, m_JackpotRate, m_ReturnSysRate[k], WheelMultipleCount[i], iter->LineMultiple[i], iter->LineBonus[i]);
						return;
					}
				}			
				//个人返奖
				for (k = 0; k < 2; k++)
				{
					//倍数为0的时候概率为0
					if (0 == iter->LineMultiple[i])
					{
						iter->UserRatio[i][k] = 0;						
					}
					else
					{
						uint64 tmp = (MAX_RATIO - m_FeeRate* 10000 - m_JackpotRate* 10000 + m_ReturnSysRate[k]* 10000) * 100;
						iter->UserRatio[i][k] = (tmp / WheelMultipleCount[i] / ( (iter->LineMultiple[i] * 100)/(i+1)  + iter->LineBonus[i] * 100));
					}
					nTotalUserRatio[i][k]       += iter->UserRatio[i][k];
					iter->TotalUserRatio[i][k]   = nTotalUserRatio[i][k];
					//中奖概率和不能大于MAX_WHEELS
					if (iter->TotalUserRatio[i][k] > MAX_RATIO)
					{
						LOG_DEBUG("CSlotLogic::MakeWheels is erro,id=%d,nTotalUserRatio[%d][%d]=%d", iter->id,i,k, iter->TotalUserRatio[i][k]);
						return;
					}
				}
			}
			if(iter->id == 100 || iter->id == 10000 || iter->id == 99999)
			    LOG_DEBUG("CSlotLogic::MakeWheels id =%d, TotalSysRatio[0][0]=%d,TotalSysRatio[8][4]=%d,TotalSysRatio[0][2]=%d,TotalSysRatio[8][2]=%d,", 
					iter->id, iter->TotalSysRatio[0][0], iter->TotalSysRatio[8][4], iter->TotalSysRatio[0][2], iter->TotalSysRatio[8][2]);

		}
		//for (int k = 0; k < 10000; k++)
		//{
		//	LOG_DEBUG("CSlotLogic::MakeWheels id =%d,TotalSysRatio[0][0]=%d,SysRatio[8][4]=%d,LineMultiple[8]=%d,m_FeeRate=%d,m_JackpotRate=%d,,m_ReturnSysRate[4]=%d",
		//		m_WheelInfo[k].id, m_WheelInfo[k].TotalSysRatio[8][4], m_WheelInfo[k].SysRatio[8][4], m_WheelInfo[k].LineMultiple[8],
		//		m_FeeRate ,m_JackpotRate ,m_ReturnSysRate[4]);
		//}
		//for (int k = 0; k < 100; k++)
		//{
		//	LOG_DEBUG("CSlotLogic::MakeWheels id =%d,TotalSysRatio[0][0]=%d,SysRatio[8][4]=%d,LineMultiple[8]=%d,m_FeeRate=%d,m_JackpotRate=%d,,m_ReturnSysRate[4]=%d",
		//		m_WheelInfo[k].id, m_WheelInfo[k].TotalSysRatio[8][4], m_WheelInfo[k].SysRatio[8][4], m_WheelInfo[k].LineMultiple[8],
		//		m_FeeRate ,m_JackpotRate ,m_ReturnSysRate[4]);
		//}		
		LOG_DEBUG("CSlotLogic::MakeWheels is done size =%d", m_WheelInfo.size());
		m_IsNewWheel = true;
	//	MakeNoRateWheels();
		return;
	}

	void CSlotLogic::MakeNoRateWheels()
	{
		m_NoRateWheel.clear();
		uint32  GamePic[MAXROW][MAXCOLUMN];//矩阵3*5
		//没有赔率的图库
		for (int n = 0; n < 100; n++)
		{
			memset(&GamePic, 0, sizeof(GamePic));
			for (uint32 j = 0; j < MAXCOLUMN; j++)
			{				
				for (uint32 k = 0; k < MAXROW; k++)
				{
					if (j == 1)
					{
						uint32 nIndex = GetNoRateWeightIndex(GamePic[0][0], GamePic[1][0], GamePic[2][0]);
						GamePic[k][j] = nIndex + 1;//矩阵3*5	
					}
					else
					{
						uint32 nIndex = GetNoRateWeightIndex(0, 0, 0);
						GamePic[k][j] = nIndex + 1;//矩阵3*5	
					}				
				}				
			}
			WheelInfo  info;
			memset(&info, 0, sizeof(info));
			info.id = n;//序号
			memcpy(info.pic[0], GamePic[0], MAXROW *MAXCOLUMN * 4);	//保存单个图集	
			LOG_DEBUG("CSlotLogic::MakeWheels line1:%d,%d,%d,%d,%d", GamePic[0][0], GamePic[0][1], GamePic[0][2], GamePic[0][3], GamePic[0][4]);
			LOG_DEBUG("CSlotLogic::MakeWheels line2:%d,%d,%d,%d,%d", GamePic[1][0], GamePic[1][1], GamePic[1][2], GamePic[1][3], GamePic[1][4]);
			LOG_DEBUG("CSlotLogic::MakeWheels line3:%d,%d,%d,%d,%d", GamePic[2][0], GamePic[1][1], GamePic[2][2], GamePic[2][3], GamePic[2][4]);
			uint64 nTotalMultiple = 0;//总倍率	
			for (uint32 j = 0; j < MAX_LINE; j++)
			{
				uint32 linePic[MAXCOLUMN] = { 0 };//顺序的线
												  //计算每条线的赔率
				for (int k = 0; k < MAXCOLUMN; k++)
				{
					linePic[k] = GamePic[(m_LogicLinePos[j].pos[k] - 1) % MAXROW][(m_LogicLinePos[j].pos[k] - 1) / MAXROW];
				}
				LOG_DEBUG("CSlotLogic::MakeWheels linePic:%d,%d,%d,%d,%d", linePic[0], linePic[1], linePic[2], linePic[3], linePic[4]);
				//统计线的结果
				picLinkinfo pLink;
				pLink.lineNo = j + 1;//线号
				//计算赔率
				uint64 nMultipleTmp = GetLineMultiple(linePic, pLink);
				if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
				{
					//picLinkVec.push_back(pLink);
					nTotalMultiple += nMultipleTmp;
				}
				else if (pLink.picID == PIC_BONUS)//计算bouns的赔率
				{
					info.LineBonus[pLink.linkNum] += m_BonusFreeTimes[pLink.linkNum];//保存bonus获得的免费次数
				}
				info.LineMultiple[j] = nTotalMultiple;//每条线对应的总赔率	
				if (info.LineMultiple[j] > 0 || info.LineBonus[pLink.linkNum] >0) //统计当前赔率不为0个数
				{
					LOG_DEBUG("CSlotLogic::MakeNoRateWheels LineMultiple>0");
				}
			}			
			m_NoRateWheel.push_back(info);
		}
	}

	//获取个人返奖调节图片矩阵
	bool CSlotLogic::GetUserPicArray(uint32 randRate, uint32 linenum,uint32 ReturnIndex, uint32 GamePic[])
	{
		//参数检查
		ServerWheelInfoType *  p_FindWheelInfo = NULL;
		if (ReturnIndex > 1 || linenum > MAX_LINE)
		{
			return false;
		}
		if (m_IsNewWheel)
		{		
			//m_FindWheelInfo.clear();
			//m_FindWheelInfo = m_WheelInfo;
			p_FindWheelInfo = &m_FindWheelInfo;
			m_IsNewWheel = false;
		}
		else
		{
			p_FindWheelInfo = &m_WheelInfo;
		}
	
		//查找
		compareDef val;
		val.linenum  = linenum-1;
		val.index    = ReturnIndex;
		val.compRate = randRate;
		vector<WheelInfo>::iterator iter = std::lower_bound(p_FindWheelInfo->begin(), p_FindWheelInfo->end(), val, compareByUseKey);
		if (iter != p_FindWheelInfo->end())
		{
			//LOG_DEBUG("CSlotLogic::GetUserPicArray->id=%d,compRate=%d,randRate=%d", iter->id,iter->TotalUserRatio[val.linenum][val.index],randRate);
			memcpy(GamePic, &iter->pic, MAXROW * MAXCOLUMN *4);	//保存单个图集
			return true;
		}
		else
		{
			//如果图库没有生成成功 则固定生成一个不中奖的
			if (m_FindWheelInfo.size() == 0)
			{
				uint32 tmpGamePic[MAXROW][MAXCOLUMN] = { 	{ 1,2,6,7,6 },
															{ 3,7,3,4,5 },
															{ 4,5,3,5,5 } }; //对应点 矩阵
				memcpy(GamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);	//保存单个图集	
				return false;
			}
			int32 nRand = g_RandGen.RandRange(1, 99);  //第五组图案
		//	LOG_DEBUG("CSlotLogic::GetUserPicArray->[%d].id=%ld,randRate=%d", MAX_WHEELS - nRand, p_FindWheelInfo[MAX_WHEELS - nRand].id, randRate);
			memcpy(GamePic, (*p_FindWheelInfo)[MAX_WHEELS - nRand].pic, MAXROW * MAXCOLUMN*4);	//保存单个图集		

		}
		LOG_DEBUG("CSlotLogic::GetUserPicArray no found");
		return false;
	}
	//获取系统返奖调节图片矩阵
	bool CSlotLogic::GetSysPicArray(uint32 randRate, uint32 linenum, uint32 ReturnIndex, uint32 GamePic[])
	{
		//参数检查
		ServerWheelInfoType *  p_FindWheelInfo = NULL;
		if (ReturnIndex >= 5 || linenum > MAX_LINE)
		{
			return false;
		}
		if (m_IsNewWheel)
		{
			m_FindWheelInfo.clear();
			//m_FindWheelInfo = m_WheelInfo;
			p_FindWheelInfo = &m_FindWheelInfo;
			m_IsNewWheel = false;
			//MakeNoRateWheels();
		}
		else
		{
			p_FindWheelInfo = &m_WheelInfo;
		}
		////=====================================新增测试代码============================================================================= 
		////线1
		//uint32 tmpGamePic11[MAXROW][MAXCOLUMN] = { 
		//{ 1,2,6,7,6 },
		//{ 3,3,3,4,5 },
		//{ 4,5,2,5,5 } }; //对应点 矩阵
		//uint32 tmpGamePic12[MAXROW][MAXCOLUMN] = { 
		//{ 1,2,6,7,6 },
		//{ 3,7,3,4,5 },
		//{ 4,5,3,5,5 } }; //对应点 矩阵

		////线2
		//uint32 tmpGamePic21[MAXROW][MAXCOLUMN] = {
		//	{ 3,3,3,4,5 },
		//	{ 1,2,6,7,6 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵
		//uint32 tmpGamePic22[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 3,7,3,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵
		// //线3
		//uint32 tmpGamePic31[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 4,5,2,5,5 },
		//	{ 3,3,3,4,5 } }; //对应点 矩阵
		//uint32 tmpGamePic32[MAXROW][MAXCOLUMN] = { 
		//{ 1,2,6,7,6 },
		//{ 3,7,3,4,5 },
		//{ 4,5,3,5,5 } }; //对应点 矩阵

		////线4
		//uint32 tmpGamePic41[MAXROW][MAXCOLUMN] = {
		//	{ 3,2,6,7,6 },
		//	{ 1,3,2,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵

		//uint32 tmpGamePic42[MAXROW][MAXCOLUMN] = {
		//{ 1,2,6,7,6 },
		//{ 3,7,3,4,5 },
		//{ 4,5,3,5,5 } }; //对应点 矩阵

		////线5															
		//uint32 tmpGamePic51[MAXROW][MAXCOLUMN] = {
		//	{ 4,5,3,5,5 },
		//	{ 1,3,2,4,5 },
		//	{ 3,2,6,7,6 } }; //对应点 矩阵
		//uint32 tmpGamePic52[MAXROW][MAXCOLUMN] = {
		//{ 1,2,6,7,6 },
		//{ 3,7,3,4,5 },
		//{ 4,5,3,5,5 } }; //对应点 矩阵

		// //线6															
		//uint32 tmpGamePic61[MAXROW][MAXCOLUMN] = {
		//	{ 3,3,2,4,5 },
		//	{ 4,5,3,5,5 },
		//	{ 1,2,6,7,6 } }; //对应点 矩阵
		//uint32 tmpGamePic62[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 3,7,3,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵

		// //线7														
		//uint32 tmpGamePic71[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 4,5,3,5,5 },
		//	{ 3,3,2,4,5 } }; //对应点 矩阵
		//uint32 tmpGamePic72[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 3,7,3,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵

		// //线8													
		//uint32 tmpGamePic81[MAXROW][MAXCOLUMN] = {
		//	{ 4,5,2,5,5 },
		//	{ 3,7,3,4,5 },
		//	{ 1,3,6,7,6 } }; //对应点 矩阵
		//uint32 tmpGamePic82[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 3,7,3,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵

		////线9														
		//uint32 tmpGamePic91[MAXROW][MAXCOLUMN] = {
		//	{ 4,3,2,5,5 },
		//	{ 3,5,3,4,5 },
		//	{ 1,2,6,7,6 }, }; //对应点 矩阵

		//uint32 tmpGamePic92[MAXROW][MAXCOLUMN] = {
		//	{ 1,2,6,7,6 },
		//	{ 3,7,3,4,5 },
		//	{ 4,5,3,5,5 } }; //对应点 矩阵

		//概率获取
		//if (randRate >= 50000)
		//{
		//	memcpy(GamePic, tmpGamePic91, MAXROW * MAXCOLUMN * 4);	//保存单个图集	
		//}
		//else
		//{
		//	memcpy(GamePic, tmpGamePic92, MAXROW * MAXCOLUMN * 4);	//保存单个图集	
		//}
		//return true;
		//=====================================测试代码=============================================================================
		//查找
		compareDef val;
		val.linenum  = linenum-1;
		val.index    = ReturnIndex;
		val.compRate = randRate;
		vector<WheelInfo>::iterator iter = std::lower_bound(p_FindWheelInfo->begin(), p_FindWheelInfo->end(), val, compareBySysKey);
		if (iter != p_FindWheelInfo->end())
		{
			//LOG_DEBUG("CSlotLogic::GetUserPicArray[%d]->compRate=%d,randRate=%d", iter->id, iter->TotalSysRatio[val.linenum][val.index], randRate);
			memcpy(GamePic, iter->pic, MAXROW * MAXCOLUMN * 4);	//保存单个图集		
			return true;
		}
		else
		{
			//如果图库没有生成成功 则固定生成一个不中奖的
			if (p_FindWheelInfo->size() == 0)
			{
				uint32 tmpGamePic[MAXROW][MAXCOLUMN] =  {{ 1,2,6,7,6 },
				                                         { 3,7,3,4,5 },
														 { 4,5,3,5,5 } }; //对应点 矩阵
				memcpy(GamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);	//保存单个图集	
				return false;
			}
			int32 nRand = g_RandGen.RandRange(1, 99);  //第五组图案
			//LOG_DEBUG("CSlotLogic::GetUserPicArray->id=%d,randRate=%d,nRand=%d", m_FindWheelInfo[MAX_WHEELS - nRand].id, randRate, nRand);
			 memcpy(GamePic, (*p_FindWheelInfo)[MAX_WHEELS - nRand].pic, MAXROW * MAXCOLUMN *4);	//保存单个图集	
		}
		return false;
	}

	bool CSlotLogic::GetRandPicArray(uint32 GamePic[][MAXCOLUMN])
	{
		//uint32  GamePic[MAXROW][MAXCOLUMN];//矩阵3*5
		//memset(&GamePic, 0, sizeof(GamePic));

		for (uint32 j = 0; j < MAXCOLUMN; j++)
		{
			for (uint32 k = 0; k < MAXROW; k++)
			{
				uint32 nIndex = GetWeightIndex(j);
				GamePic[k][j] = nIndex + 1;
			}
		}

		return true;
	}

	//根据权重算法,获取拿到第几大的牌型
	uint32 CSlotLogic::GetWeightIndex(uint32 num)
	{
		//根据权重表计算拿牌概率
		uint32   nWeightSum = 0; //权重总和
		for (uint8 i = 0; i < MAXWEIGHT; i++)
		{
			nWeightSum += m_WeightPicCfg[num][i];
		}
		//uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum * PRO_DENO_100);
		uint32  nSectionNum = g_RandGen.GetRandi(0, nWeightSum * PRO_DENO_100);
		uint32  tmpSectionNum = nSectionNum;
		uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
		for (uint32 i = 0; i < MAXWEIGHT; i++)
		{
			if (m_WeightPicCfg[num][i] == 0)
			{
				continue;
			}
			if (nSectionNum <= (m_WeightPicCfg[num][i] * PRO_DENO_100))
			{
				nCardIndex = i;
				break;
			}
			else
			{
				nSectionNum -= (m_WeightPicCfg[num][i] * PRO_DENO_100);
			}
		}		
		return nCardIndex;
	}

	uint32 CSlotLogic::GetMultipleRandIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple)
	{
		//根据权重表计算拿牌概率
		uint32 tempArrWeightPicCfg[MAXCOLUMN][MAXWEIGHT] =
		{
			//00 01 02 03 04 05 06 07 08 09 10
			{ 64,64,64,64,64,64,64,64,64,00,00 },
			{ 64,64,64,64,64,64,64,64,64,00,00 },
			{ 64,64,64,64,64,64,64,64,64,00,00 },
			{ 64,64,64,64,64,64,64,64,64,00,00 },
			{ 64,64,64,64,64,64,64,64,64,00,00 }
		};

		uint32   nWeightSum = 0; //权重总和
		for (uint8 i = 0; i < MAXWEIGHT; i++)
		{
			nWeightSum += tempArrWeightPicCfg[num][i];
		}
		//uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum);
		uint32  nSectionNum = g_RandGen.GetRandi(0, nWeightSum);
		uint32  tmpSectionNum = nSectionNum;
		uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
		for (uint32 i = 0; i < MAXWEIGHT; i++)
		{
			if (tempArrWeightPicCfg[num][i] == 0)
			{
				continue;
			}
			if (nSectionNum <= (tempArrWeightPicCfg[num][i]))
			{
				nCardIndex = i;
				break;
			}
			else
			{
				nSectionNum -= (tempArrWeightPicCfg[num][i]);
			}
		}
		return nCardIndex;
	}

	bool CSlotLogic::GetMultiplePicArray(uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple, uint32 GamePic[][MAXCOLUMN])
	{
		for (uint32 j = 0; j < MAXCOLUMN; j++)
		{
			for (uint32 k = 0; k < MAXROW; k++)
			{
				uint32 nIndex = GetMultipleRandIndex(j, nline, lMinWinMultiple, lMaxWinMultiple);
				GamePic[k][j] = nIndex + 1;
			}
		}

		struct tagLineMulConfig
		{
			uint32 j;
			uint32 k;
			int64 m;
		};

		//int64 tempArrMulCfg[MAXWEIGHT][MAXCOLUMN] =
		//{
		//	//0 1 2   3   4
		//	{ 0,1,3,  10, 75   },// 0
		//	{ 0,0,3,  10, 85   },// 1
		//	{ 0,0,15, 40, 250  },// 2
		//	{ 0,0,25, 50, 400  },// 3
		//	{ 0,0,30, 70, 550  },// 4
		//	{ 0,0,35, 80, 650  },// 5
		//	{ 0,0,45, 100,800  },// 6
		//	{ 0,0,75, 175,1250 },// 7
		//	{ 0,0,0,  0,  0    },// 8
		//	{ 0,0,0,  0,  0    },// 9
		//	{ 0,0,100,200,1750 } // 10
		//};
		
		int64 tempArrMulCfg[MAXWEIGHT][MAXCOLUMN] =
		{
			//0 1 2   3   4
			{ 0,1,3,  10, 75   },// 0
			{ 0,0,3,  10, 85   },// 1
			{ 0,0,15, 40, 250  },// 2
			{ 0,0,25, 50, 400  },// 3
			{ 0,0,30, 70, 550  },// 4
			{ 0,0,35, 80, 650  },// 5
			{ 0,0,45, 100,800  },// 6
			{ 0,0,75, 175,1250 },// 7
			{ 0,0,0,  0,  0    },// 8
			{ 0,0,0,  0,  0    },// 9
			{ 0,0,0,  0,  0    } // 10
		};
		std::vector<tagLineMulConfig> vecTempArrMul;

		//i : 0, j : 0, k : 1, m : 1 
		//i : 1, j : 0, k : 2, m : 3 
		//i : 2, j : 1, k : 2, m : 3 
		//i : 3, j : 0, k : 3, m : 10 
		//i : 4, j : 1, k : 3, m : 10 
		//i : 5, j : 2, k : 2, m : 15 
		//i : 6, j : 3, k : 2, m : 25 
		//i : 7, j : 4, k : 2, m : 30 
		//i : 8, j : 5, k : 2, m : 35 
		//i : 9, j : 2, k : 3, m : 40 
		//i : 10, j : 6, k : 2, m : 45 
		//i : 11, j : 3, k : 3, m : 50 
		//i : 12, j : 4, k : 3, m : 70 
		//i : 13, j : 0, k : 4, m : 75 
		//i : 14, j : 7, k : 2, m : 75 
		//i : 15, j : 5, k : 3, m : 80 
		//i : 16, j : 1, k : 4, m : 85 
		//i : 17, j : 6, k : 3, m : 100 
		//i : 18, j : 7, k : 3, m : 175 
		//i : 19, j : 2, k : 4, m : 250 
		//i : 20, j : 3, k : 4, m : 400 
		//i : 21, j : 4, k : 4, m : 550 
		//i : 22, j : 5, k : 4, m : 650 
		//i : 23, j : 6, k : 4, m : 800 
		//i : 24, j : 7, k : 4, m : 1250		
		//tagLineMulConfig tagMul;
		//tagMul.j = 0;	tagMul.k = 1;	tagMul.m = 1;   vecTempArrMul.push_back(tagMul);   //	j:	0, k : 1, m : 1
		//tagMul.j = 0;	tagMul.k = 2;	tagMul.m = 3;   vecTempArrMul.push_back(tagMul);   //	j : 0, k : 2, m : 3
		//tagMul.j = 1;	tagMul.k = 2;	tagMul.m = 3;   vecTempArrMul.push_back(tagMul);   //	j : 1, k : 2, m : 3
		//tagMul.j = 0;	tagMul.k = 3;	tagMul.m = 10;   vecTempArrMul.push_back(tagMul);  //	j : 0, k : 3, m : 10
		//tagMul.j = 1;	tagMul.k = 3;	tagMul.m = 10;   vecTempArrMul.push_back(tagMul);  //	j : 1, k : 3, m : 10
		//tagMul.j = 2;	tagMul.k = 2;	tagMul.m = 15;   vecTempArrMul.push_back(tagMul);  //	j : 2, k : 2, m : 15
		//tagMul.j = 3;	tagMul.k = 2;	tagMul.m = 25;   vecTempArrMul.push_back(tagMul);  //	j : 3, k : 2, m : 25
		//tagMul.j = 4;	tagMul.k = 2;	tagMul.m = 30;   vecTempArrMul.push_back(tagMul);  //	j : 4, k : 2, m : 30
		//tagMul.j = 5;	tagMul.k = 2;	tagMul.m = 35;   vecTempArrMul.push_back(tagMul);  //	j : 5, k : 2, m : 35
		//tagMul.j = 2;	tagMul.k = 3;	tagMul.m = 40;   vecTempArrMul.push_back(tagMul);  //	j : 2, k : 3, m : 40
		//tagMul.j = 6;	tagMul.k = 2;	tagMul.m = 45;   vecTempArrMul.push_back(tagMul);  //	j : 6, k : 2, m : 45
		//tagMul.j = 3;	tagMul.k = 3;	tagMul.m = 50;   vecTempArrMul.push_back(tagMul);  //	j : 3, k : 3, m : 50
		//tagMul.j = 4;	tagMul.k = 3;	tagMul.m = 70;   vecTempArrMul.push_back(tagMul);  //	j : 4, k : 3, m : 70
		//tagMul.j = 0;	tagMul.k = 4;	tagMul.m = 75;   vecTempArrMul.push_back(tagMul);  //	j : 0, k : 4, m : 75
		//tagMul.j = 7;	tagMul.k = 2;	tagMul.m = 75;   vecTempArrMul.push_back(tagMul);  //	j : 7, k : 2, m : 75
		//tagMul.j = 5;	tagMul.k = 3;	tagMul.m = 80;   vecTempArrMul.push_back(tagMul);  //	j : 5, k : 3, m : 80
		//tagMul.j = 1;	tagMul.k = 4;	tagMul.m = 85;   vecTempArrMul.push_back(tagMul);  //	j : 1, k : 4, m : 85
		//tagMul.j = 6;	tagMul.k = 3;	tagMul.m = 100;  vecTempArrMul.push_back(tagMul);  //	j : 6, k : 3, m : 100
		//tagMul.j = 7;	tagMul.k = 3;	tagMul.m = 175;  vecTempArrMul.push_back(tagMul);  //	j : 7, k : 3, m : 175
		//tagMul.j = 2;	tagMul.k = 4;	tagMul.m = 250;  vecTempArrMul.push_back(tagMul);  //	j : 2, k : 4, m : 250
		//tagMul.j = 3;	tagMul.k = 4;	tagMul.m = 400;  vecTempArrMul.push_back(tagMul);  //	j : 3, k : 4, m : 400
		//tagMul.j = 4;	tagMul.k = 4;	tagMul.m = 500;  vecTempArrMul.push_back(tagMul);  //	j : 4, k : 4, m : 550
		//tagMul.j = 5;	tagMul.k = 4;	tagMul.m = 650;  vecTempArrMul.push_back(tagMul);  //	j : 5, k : 4, m : 650
		//tagMul.j = 6;	tagMul.k = 6;	tagMul.m = 800;  vecTempArrMul.push_back(tagMul);  //	j : 6, k : 4, m : 800
		//tagMul.j = 7;	tagMul.k = 4;	tagMul.m = 1250; vecTempArrMul.push_back(tagMul);  //	j : 7, k : 4, m : 1250

		for (uint32 j = 0; j < MAXWEIGHT; j++)
		{
			for (uint32 k = 0; k < MAXCOLUMN; k++)
			{
				if (tempArrMulCfg[j][k]>0)
				{
					tagLineMulConfig tagTempMul;
					tagTempMul.j = j;
					tagTempMul.k = k;
					tagTempMul.m = tempArrMulCfg[j][k];
					vecTempArrMul.push_back(tagTempMul);
				}
			}
		}
		std::sort(std::begin(vecTempArrMul), std::end(vecTempArrMul), [](const auto &a, const auto &b) { return a.m < b.m; });

		uint32 uMaxCount = 0;
		uint32 uFirstMinIndex = 1024;
		uint32 uFirstMaxIndex = 1024;
		for (uint32 i = 0; i < vecTempArrMul.size(); i++)
		{
			if (uFirstMinIndex == 1024 && vecTempArrMul[i].m >=lMinWinMultiple && vecTempArrMul[i].m <= lMaxWinMultiple)
			{
				uFirstMinIndex = i;
			}
			if (uFirstMaxIndex == 1024 && vecTempArrMul[i].m >lMaxWinMultiple)
			{
				uFirstMaxIndex = i;
			}
		}
		int uStartIndex = 0;
		int uEndIndex = vecTempArrMul.size();
		if (uFirstMinIndex != 1024 && uFirstMinIndex< vecTempArrMul.size())
		{
			//uStartIndex = uFirstMinIndex;
		}
		if (uFirstMaxIndex != 1024 && uFirstMaxIndex< vecTempArrMul.size())
		{
			uEndIndex = uFirstMaxIndex;
		}
		//int iHitWinCount = g_RandGen.GetRandi(1, nline);
		std::vector<uint32> vecTempIndex;
		int64 nTotalMultiple = 0;
		bool bIsSuccessMult = false;

		std::string strMultLog;
		//for (uint32 i = 0; i < vecTempArrMul.size(); i++)
		//{
		//	tagLineMulConfig & tagTempMul = vecTempArrMul[i];
		//	strMultLog += CStringUtility::FormatToString("i:%d,j:%d,k:%d,m:%lld ", i, tagTempMul.j, tagTempMul.k,tagTempMul.m);
		//}
		//strMultLog += CStringUtility::FormatToString("uStartIndex:%d,uEndIndex:%d,", uStartIndex, uEndIndex);

		//if (uEndIndex > 0 && uEndIndex < vecTempArrMul.size())
		//{
		//	uEndIndex--;
		//	vecTempIndex.push_back(uEndIndex);
		//	nTotalMultiple += vecTempArrMul[uEndIndex].m;
		//	if (lMinWinMultiple > 0 && lMaxWinMultiple > 0 && nTotalMultiple >= lMinWinMultiple && nTotalMultiple <= lMaxWinMultiple && vecTempIndex.size() <= nline)
		//	{
		//		bIsSuccessMult = true;
		//	}
		//	if (bIsSuccessMult == false && uEndIndex > 0)
		//	{
		//		bool bIsBreakLoop = false;
		//		iHitWinCount--;
		//		for (int o = 0; o < 512; o++)
		//		{
		//			nTotalMultiple += vecTempArrMul[iIndex].m;
		//			for (int i = 0; i < iHitWinCount; i++)
		//			{
		//				int iIndex = g_RandGen.GetRandi(uStartIndex, uEndIndex-1);
		//				vecTempArrMul[iIndex].m;
		//				vecTempIndex.push_back(iIndex);
		//				nTotalMultiple += vecTempArrMul[iIndex].m;
		//				
		//				if (lMinWinMultiple > 0 && lMaxWinMultiple > 0 && nTotalMultiple >= lMinWinMultiple && nTotalMultiple <= lMaxWinMultiple && vecTempIndex.size() <= nline)
		//				{
		//					bIsSuccessMult = true;
		//					bIsBreakLoop = true;
		//					break;
		//				}
		//				if (vecTempIndex.size() == nline)
		//				{
		//					bIsBreakLoop = true;
		//					break;
		//				}
		//			}
		//			if (bIsBreakLoop)
		//			{
		//				break;
		//			}
		//		}
		//	}

		//}

		//if (lMinWinMultiple == 5 && lMaxWinMultiple == 10)
		//{
		//	uEndIndex = 15;
		//	if (uEndIndex > 0 && uEndIndex < (int)vecTempArrMul.size() && lMaxWinMultiple > 0 && vecTempArrMul[uEndIndex].m <= lMaxWinMultiple && ((nTotalMultiple + vecTempArrMul[uEndIndex].m) <= lMaxWinMultiple))
		//	{
		//		vecTempIndex.push_back(uEndIndex);
		//		nTotalMultiple += vecTempArrMul[uEndIndex].m;
		//		//strMultLog += CStringUtility::FormatToString("i:%d,m:%lld,t:%lld,", uEndIndex, vecTempArrMul[uEndIndex].m, nTotalMultiple);
		//	}
		//}

		if (lMinWinMultiple == 90 && lMaxWinMultiple == 99)
		{
			uEndIndex = 15;
			if (uEndIndex > 0 && uEndIndex < (int)vecTempArrMul.size() && lMaxWinMultiple > 0 && vecTempArrMul[uEndIndex].m <= lMaxWinMultiple && ((nTotalMultiple + vecTempArrMul[uEndIndex].m) <= lMaxWinMultiple))
			{
				vecTempIndex.push_back(uEndIndex);
				nTotalMultiple += vecTempArrMul[uEndIndex].m;
				//strMultLog += CStringUtility::FormatToString("i:%d,m:%lld,t:%lld,", uEndIndex, vecTempArrMul[uEndIndex].m, nTotalMultiple);
			}
		}

		if (uEndIndex > uStartIndex)
		{
			uEndIndex--;
			while (uEndIndex >= uStartIndex)
			{
				if (uEndIndex > 0 && uEndIndex < (int)vecTempArrMul.size() && lMaxWinMultiple > 0 && vecTempArrMul[uEndIndex].m <= lMaxWinMultiple && ((nTotalMultiple + vecTempArrMul[uEndIndex].m) <= lMaxWinMultiple))
				{
					vecTempIndex.push_back(uEndIndex);
					nTotalMultiple += vecTempArrMul[uEndIndex].m;
					//strMultLog += CStringUtility::FormatToString("i:%d,m:%lld,t:%lld,", uEndIndex, vecTempArrMul[uEndIndex].m, nTotalMultiple);
				}
				if (lMinWinMultiple > 0 && lMaxWinMultiple > 0 && nTotalMultiple >= lMinWinMultiple && nTotalMultiple <= lMaxWinMultiple && vecTempIndex.size() <= nline)
				{
					bIsSuccessMult = true;
					break;
				}
				if (nTotalMultiple > lMaxWinMultiple)
				{
					break;
				}
				if (vecTempIndex.size() >= MAXWEIGHT)
				{
					break;
				}
				uEndIndex--;
				if (uEndIndex < uStartIndex)
				{
					break;
				}
				if (uEndIndex < 0 || uEndIndex >= (int)vecTempArrMul.size())
				{
					break;
				}
			}
		}

		//strMultLog += CStringUtility::FormatToString("bIsSuccessMult:%d,uEndIndex:%d,", bIsSuccessMult, uEndIndex);

		if (bIsSuccessMult)
		{
			// 01 04 07 10 13
			// 02 05 08 11 14
			// 03 06 09 12 15
			int64 tempArrLineCfg[MAXWEIGHT][MAXCOLUMN] =
			{
				{ 2,5,8,11,14 },
				{ 1,4,7,10,13 },
				{ 3,6,9,12,15 },
				{ 1,5,9,11,13 },
				{ 3,5,7,11,15 },
				{ 1,4,8,12,15 },
				{ 3,6,8,10,13 },
				{ 2,6,8,10,14 },
				{ 2,4,8,12,14 }
			};
			for (uint32 line = 0; line < vecTempIndex.size(); line++)
			{
				uint32 uIndex = vecTempIndex[line];
				//strMultLog += CStringUtility::FormatToString("i:%d,uvIndex:%d,", line,uIndex);
				tagLineMulConfig & tagTempMul = vecTempArrMul[uIndex];
				for (uint32 k = 0; k <= tagTempMul.k; k++)
				{
					//mGamePic[(m_LogicLinePos[line].pos[k] - 1) % MAXROW][(m_LogicLinePos[line].pos[k] - 1) / MAXROW] = tagTempMul.j + 1;
					GamePic[(tempArrLineCfg[line][k] - 1) % MAXROW][(tempArrLineCfg[line][k] - 1) / MAXROW] = tagTempMul.j + 1;
				}
			}
			//修正图片倍数


		}

		//LOG_DEBUG("nline:%d,nTotalMultiple:%lld,lMinWinMultiple:%lld,lMaxWinMultiple:%lld,bIsSuccessMult:%d,vecTempIndex:%d,strMultLog:%s",	nline, nTotalMultiple, lMinWinMultiple, lMaxWinMultiple, bIsSuccessMult, vecTempIndex.size(), strMultLog.c_str());
		
		// 选出 n 个数字相加在 x 和 y 之间
		// 比如从下面选择 3 个数字相加在 600 和 699 之间,可以选择 550+70+50=670 在 600-699之间
		//uint32 cfg[11][5] =
		//{
		//	//0 1 2   3   4
		//	{ 5,1,3,  10, 75   },// 0
		//	{ 0,0,3,  10, 85   },// 1
		//	{ 0,0,15, 40, 230  },// 2
		//	{ 3,0,25, 50, 400  },// 3
		//	{ 0,0,30, 70, 550  },// 4
		//	{ 2,8,35, 80, 600  },// 5
		//	{ 0,0,45, 100,800  },// 6
		//	{ 0,0,75, 175,125  },// 7
		//	{ 6,0,0,  0,  0    },// 8
		//	{ 0,7,0,  0,  0    },// 9
		//	{ 7,0,100,200,175  } // 10
		//};

		return bIsSuccessMult;
	}

	uint32 CSlotLogic::GetFreeSpinWeightIndex(uint32 num)
	{
		//根据权重表计算拿牌概率
		uint32 tempArrWeightPicCfg[MAXCOLUMN][MAXWEIGHT] = 
		{
			//00 01 02 03 04 05 06 07 08 09 10
			{ 64,64,64,64,64,64,64,64,64,256,0 },
			{ 64,64,64,64,64,64,64,64,64,256,0 },
			{ 64,64,64,64,64,64,64,64,64,256,0 },
			{ 64,64,64,64,64,64,64,64,64,256,0 },
			{ 64,64,64,64,64,64,64,64,64,256,0 }
		};
		uint32   nWeightSum = 0; //权重总和
		for (uint8 i = 0; i < MAXWEIGHT; i++)
		{
			nWeightSum += tempArrWeightPicCfg[num][i];
		}
		//uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum * PRO_DENO_100);
		uint32  nSectionNum = g_RandGen.GetRandi(0, nWeightSum);
		uint32  tmpSectionNum = nSectionNum;
		uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
		for (uint32 i = 0; i < MAXWEIGHT; i++)
		{
			if (tempArrWeightPicCfg[num][i] == 0)
			{
				continue;
			}
			if (nSectionNum <= (tempArrWeightPicCfg[num][i]))
			{
				nCardIndex = i;
				break;
			}
			else
			{
				nSectionNum -= (tempArrWeightPicCfg[num][i]);
			}
		}
		return nCardIndex;
	}

	uint32 CSlotLogic::GetMultipleSpinWeightIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple)
	{
		//根据权重表计算拿牌概率
		uint32 tempArrWeightPicCfg[MAXCOLUMN][MAXWEIGHT] =
		{
			//00 01 02 03 04 05 06 07 08 09 10
			{ 64,64,64,64,64,64,64,64,64,0,0 },
			{ 64,64,64,64,64,64,64,64,64,0,0 },
			{ 64,64,64,64,64,64,64,64,64,0,0 },
			{ 64,64,64,64,64,64,64,64,64,0,0 },
			{ 64,64,64,64,64,64,64,64,64,0,0 }
		};

		if (nline == 1)
		{
			if (lMinWinMultiple == 100 && lMaxWinMultiple == 149)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,64,264,64,64,0,0 },
					{ 64,64,64,64,64,64,264,64,64,0,0 },
					{ 64,64,64,64,64,64,264,64,64,0,0 },
					{ 64,64,64,64,64,64,264,64,64,0,0 },
					{ 64,64,64,64,64,64,264,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 150 && lMaxWinMultiple == 199)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 250 && lMaxWinMultiple == 299)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,264,64,64,64,64,64,64,0,0 },
					{ 64,64,264,64,64,64,64,64,64,0,0 },
					{ 64,64,264,64,64,64,64,64,64,0,0 },
					{ 64,64,264,64,64,64,64,64,64,0,0 },
					{ 64,64,264,64,64,64,64,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 400 && lMaxWinMultiple == 499)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,264,64,64,64,64,64,0,0 },
					{ 64,64,64,264,64,64,64,64,64,0,0 },
					{ 64,64,64,264,64,64,64,64,64,0,0 },
					{ 64,64,64,264,64,64,64,64,64,0,0 },
					{ 64,64,64,264,64,64,64,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 500 && lMaxWinMultiple == 599)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,264,64,64,64,64,0,0 },
					{ 64,64,64,64,264,64,64,64,64,0,0 },
					{ 64,64,64,64,264,64,64,64,64,0,0 },
					{ 64,64,64,64,264,64,64,64,64,0,0 },
					{ 64,64,64,64,264,64,64,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 600 && lMaxWinMultiple == 699)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,264,64,64,64,0,0 },
					{ 64,64,64,64,64,264,64,64,64,0,0 },
					{ 64,64,64,64,64,264,64,64,64,0,0 },
					{ 64,64,64,64,64,264,64,64,64,0,0 },
					{ 64,64,64,64,64,264,64,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 800 && lMaxWinMultiple == 899)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,64,364,64,64,0,0 },
					{ 64,64,64,64,64,64,364,64,64,0,0 },
					{ 64,64,64,64,64,64,364,64,64,0,0 },
					{ 64,64,64,64,64,64,364,64,64,0,0 },
					{ 64,64,64,64,64,64,364,64,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 1250 && lMaxWinMultiple == 1999)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 },
					{ 64,64,64,64,64,64,64,264,64,0,0 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
		}
		else if (nline > 1)
		{
			if (lMinWinMultiple == 5 && lMaxWinMultiple == 9)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					// 00 01 02 03 04 05 06 07 08 09 10
					{ 264,64,00,00,00,00,00,00,00,00,00 },
					{ 264,64,00,00,00,00,00,00,00,00,00 },
					{ 264,64,00,00,00,00,00,00,00,00,00 },
					{ 264,64,00,00,00,00,00,00,00,00,00 },
					{ 264,64,00,00,00,00,00,00,00,00,00 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
			else if (lMinWinMultiple == 60 && lMaxWinMultiple == 69)
			{
				uint32 tempArrWeightStep[MAXCOLUMN][MAXWEIGHT] =
				{
					//00 01 02 03 04 05 06 07 08 09 10
					{ 64,64,64,64,64,64,64,00,00,00,00 },
					{ 64,64,64,64,64,64,64,00,00,00,00 },
					{ 64,64,64,64,64,64,64,00,00,00,00 },
					{ 64,64,64,64,64,64,64,00,00,00,00 },
					{ 64,64,64,64,64,64,64,00,00,00,00 }
				};
				memcpy(tempArrWeightPicCfg, tempArrWeightStep, sizeof(tempArrWeightPicCfg));
			}
		}


		uint32   nWeightSum = 0; //权重总和
		for (uint8 i = 0; i < MAXWEIGHT; i++)
		{
			nWeightSum += tempArrWeightPicCfg[num][i];
		}
		//uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum);
		uint32  nSectionNum = g_RandGen.GetRandi(0, nWeightSum);
		uint32  tmpSectionNum = nSectionNum;
		uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
		for (uint32 i = 0; i < MAXWEIGHT; i++)
		{
			if (tempArrWeightPicCfg[num][i] == 0)
			{
				continue;
			}
			if (nSectionNum <= (tempArrWeightPicCfg[num][i]))
			{
				nCardIndex = i;
				break;
			}
			else
			{
				nSectionNum -= (tempArrWeightPicCfg[num][i]);
			}
		}
		return nCardIndex;
	}

	//计算相连的个数
	uint64 CSlotLogic::GetLineMultiple(uint32 GamePic[MAXCOLUMN], picLinkinfo &picLink)
	{
		picLink.linkNum = 0;
		uint64 nMultiple = 0;
		//判断首位是否是万能图片
		if (PIC_WILD == GamePic[0])
		{
			for (uint32 i = 1; i < MAXCOLUMN; i++)//计算相同的连线
			{
				//特殊符号
				if (GamePic[i] == PIC_BONUS)
				{
					break;
				}
				if (GamePic[i] != PIC_WILD && GamePic[0] == PIC_WILD)
				{
					GamePic[0] = GamePic[i];
				}
				//统计个数
				if (GamePic[i] == GamePic[0] || GamePic[i] == PIC_WILD)
				{
					picLink.linkNum += 1;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			//计算相同的连线
			for (uint32 i = 1; i < MAXCOLUMN; i++)//计算相同的连线
			{
				if (0 == GamePic[i])
				{
					break;
				}
				//统计相连的个数
				if (GamePic[0] == GamePic[i] || (GamePic[i] == PIC_WILD && PIC_BONUS != GamePic[0] && PIC_SEVEN != GamePic[0]))
				{
					picLink.linkNum += 1;
				}
				else
				{
					break;
				}
			}
		}
		picLink.picID = GamePic[0];
		if (picLink.linkNum >= MAXCOLUMN || GamePic[0] == 0)
		{
			return nMultiple;
		}
		//获取赔率
		nMultiple = m_LogicPicRate[GamePic[0]-1].multiple[picLink.linkNum];//获取赔率
		return	nMultiple;
	}

}


//根据权重算法,获取拿到第几大的牌型
uint32 CSlotLogic::GetNoRateWeightIndex(uint32 posone, uint32 postwo, uint32 posthree)
{
	//根据权重表计算拿牌概率
	uint32   nWeightSum = 0; //权重总和
	uint32   WeightCfg[MAXWEIGHT] = { 10,43,88,58,72,38,28,25,0,0,38 };
	//更改权重
	if (posone > 0 && posone <= MAXWEIGHT && postwo > 0  && postwo <= MAXWEIGHT && posthree > 0 && posthree <= MAXWEIGHT)
	{
		WeightCfg[posone - 1] = 0;
		WeightCfg[postwo - 1] = 0;
		WeightCfg[posthree - 1] = 0;
	}
	//计算总权重
	for (uint8 i = 0; i < MAXWEIGHT; i++)
	{
		nWeightSum += WeightCfg[i];
	}
	uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum * PRO_DENO_100);
	uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
	for (uint32 i = 0; i < MAXWEIGHT; i++)
	{
		if (WeightCfg[i] == 0)
		{
			continue;
		}
		if (nSectionNum <= (WeightCfg[i] * PRO_DENO_100))
		{
			nCardIndex = i;
			break;
		}
		else
		{
			nSectionNum -= (WeightCfg[i] * PRO_DENO_100);
		}
	}
	return nCardIndex;
}


uint32 CSlotLogic::GetBananaWeightIndex(uint32 line)
{
	//根据权重表计算拿牌概率
	uint32   nWeightSum = 0; //权重总和
	uint32   WeightCfg[5][MAXWEIGHT] = {
				{ 100,10,10,10,10,10,10,10,50,10,0 },
				{ 100,10,10,10,10,10,10,10,50,10,0 }, 
				{ 100,10,10,10,10,10,10,10,50,10,0 }, 
				{ 100,10,10,10,10,10,10,10,50,10,0 },
				{ 100,10,10,10,10,10,10,10,50,10,0 } };
	//计算总权重
	for (uint8 i = 0; i < MAXWEIGHT; i++)
	{
		nWeightSum += WeightCfg[line][i];
	}
	uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum * PRO_DENO_100);
	uint8   nCardIndex = 0;//根据权重获得能拿第几大的牌
	for (uint32 i = 0; i < MAXWEIGHT; i++)
	{
		if (WeightCfg[line][i] == 0)
		{
			continue;
		}
		if (nSectionNum <= (WeightCfg[line][i] * PRO_DENO_100))
		{
			nCardIndex = i;
			break;
		}
		else
		{
			nSectionNum -= (WeightCfg[line][i] * PRO_DENO_100);
		}
	}
	return nCardIndex;
}

// mGamePic[3][5];

void CSlotLogic::GetJackpotScoreOne(uint32 mGamePic[][MAXCOLUMN])
{
	for (int j = 0; j < MAXCOLUMN; j++)
	{
		mGamePic[1][j] = PIC_SEVEN;
	}
}

void CSlotLogic::GetJackpotScoreTwo(uint32 mGamePic[][MAXCOLUMN])
{
	for (int j = 0; j < MAXCOLUMN; j++)
	{
		mGamePic[0][j] = PIC_SEVEN;
	}
}

void CSlotLogic::GetJackpotScoreThree(uint32 mGamePic[][MAXCOLUMN])
{
	for (int j = 0; j < MAXCOLUMN; j++)
	{
		mGamePic[2][j] = PIC_SEVEN;
	}
}

void CSlotLogic::GetJackpotScoreFour(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[0][0] = PIC_SEVEN;
	mGamePic[1][1] = PIC_SEVEN;
	mGamePic[2][2] = PIC_SEVEN;
	mGamePic[3][1] = PIC_SEVEN;
	mGamePic[4][0] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreFive(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[2][0] = PIC_SEVEN;
	mGamePic[1][1] = PIC_SEVEN;
	mGamePic[0][2] = PIC_SEVEN;
	mGamePic[1][3] = PIC_SEVEN;
	mGamePic[2][4] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreSix(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[0][0] = PIC_SEVEN;
	mGamePic[0][1] = PIC_SEVEN;
	mGamePic[0][2] = PIC_SEVEN;
	mGamePic[2][3] = PIC_SEVEN;
	mGamePic[2][4] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreSeven(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[2][0] = PIC_SEVEN;
	mGamePic[2][1] = PIC_SEVEN;
	mGamePic[1][2] = PIC_SEVEN;
	mGamePic[0][3] = PIC_SEVEN;
	mGamePic[0][4] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreEight(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[1][0] = PIC_SEVEN;
	mGamePic[2][1] = PIC_SEVEN;
	mGamePic[1][2] = PIC_SEVEN;
	mGamePic[0][3] = PIC_SEVEN;
	mGamePic[1][4] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreNine(uint32 mGamePic[][MAXCOLUMN])
{
	mGamePic[1][0] = PIC_SEVEN;
	mGamePic[0][1] = PIC_SEVEN;
	mGamePic[1][2] = PIC_SEVEN;
	mGamePic[2][3] = PIC_SEVEN;
	mGamePic[1][4] = PIC_SEVEN;
}

void CSlotLogic::GetJackpotScoreByLine(uint32 line,uint32 mGamePic[][MAXCOLUMN])
{
	if (line < 1 || line>9)
	{
		return;
	}
	uint32 uWinRandLine = g_RandGen.RandRange(1, line);

	switch (uWinRandLine)
	{
		case 1:
		{
			GetJackpotScoreOne(mGamePic);
		}break;
		case 2:
		{
			GetJackpotScoreTwo(mGamePic);
		}break;
		case 3:
		{
			GetJackpotScoreThree(mGamePic);
		}break;
		case 4:
		{
			GetJackpotScoreFour(mGamePic);
		}break;
		case 5:
		{
			GetJackpotScoreFive(mGamePic);
		}break;
		case 6:
		{
			GetJackpotScoreSix(mGamePic);
		}break;
		case 7:
		{
			GetJackpotScoreSeven(mGamePic);
		}break;
		case 8:
		{
			GetJackpotScoreEight(mGamePic);
		}break;
		case 9:
		{
			GetJackpotScoreNine(mGamePic);
		}break;
		default:
		{
			break;
		}
	}
}

void CSlotLogic::testMakeWheels()
{
	m_WheelInfo.clear();
	LOG_DEBUG("CSlotLogic::MakeWheels");
	uint64  WheelMultipleCount[MAX_LINE];//统计所有有赔率的周图库数量
	memset(&WheelMultipleCount, 0, sizeof(WheelMultipleCount));
	uint32  GamePic[MAXROW][MAXCOLUMN];//矩阵3*5
	memset(&GamePic, 0, sizeof(GamePic));
	//uint32  nWeightPicCfg[MAXWEIGHT];
	//memset(&nWeightPicCfg, 0, sizeof(nWeightPicCfg));MAX_WHEELS
	uint64 nTotal9Multiple = 0;
	////生成图集库
	for (uint32 i = 0; i < 10; i++)
	{
		for (uint32 j = 0; j < MAXCOLUMN; j++)
		{
			for (uint32 k = 0; k < MAXROW; k++)
			{
				uint32 nIndex = GetWeightIndex(j);
				GamePic[k][j] = nIndex + 1;//矩阵3*5					
			}
		}

		//计算所有的线的倍率
		//计算赔率
		uint64 nTotalMultiple = 0;//总倍率	
		WheelInfo  info;
		memset(&info, 0, sizeof(info));
		info.id = i;//序号
		memcpy(info.pic[0], GamePic[0], MAXROW *MAXCOLUMN * 4);	//保存单个图集		
		uint32    nTotalSysRatio[MAX_LINE][5];      //9线系统返奖1-5调节和		
	//	uint32    nTotalUserRatio[MAX_LINE][2];     //9线个人返奖1-2调节和
		memset(&nTotalSysRatio, 0, sizeof(nTotalSysRatio));
	//	memset(&nTotalUserRatio, 0, sizeof(nTotalUserRatio));
		for (uint32 j = 0; j < MAX_LINE; j++)
		{
			uint32 linePic[MAXCOLUMN] = { 0 };//顺序的线
											  //计算每条线的赔率
			for (int k = 0; k < MAXCOLUMN; k++)
			{
				linePic[k] = GamePic[(m_LogicLinePos[j].pos[k] - 1) % MAXROW][(m_LogicLinePos[j].pos[k] - 1) / MAXROW];
			}
			//统计线的结果
			picLinkinfo pLink;
			pLink.lineNo = j + 1;//线号
								 //计算赔率
			uint64 nMultipleTmp = GetLineMultiple(linePic, pLink);
			if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
			{
				//picLinkVec.push_back(pLink);
				nTotalMultiple += nMultipleTmp;
			}
			else if (pLink.picID == PIC_BONUS)//计算bouns的赔率
			{
				info.LineBonus[pLink.linkNum] += m_BonusFreeTimes[pLink.linkNum];//保存bonus获得的免费次数
			}
			info.LineMultiple[j] = nTotalMultiple;//每条线对应的总赔率	
			if (info.LineMultiple[j] > 0 || info.LineBonus[pLink.linkNum] >0) //统计当前赔率不为0个数
			{
				WheelMultipleCount[j]++;
				if (j == 8)
				{
					nTotal9Multiple += info.LineMultiple[j];
				}
			}

			for (uint32 k = 0; k < 5; k++)
			{
				if (0 == info.LineMultiple[j])
				{
					info.SysRatio[j][k] = 0;
					
				}
				else
				{
					info.SysRatio[j][k] = 1;
					nTotalSysRatio[j][k] += info.SysRatio[j][k];
				}
				info.TotalSysRatio[j][k] = nTotalSysRatio[j][k];
			}				
		}
		//加入图集库		
		m_WheelInfo.push_back(info);
	}
	LOG_DEBUG("CSlotLogic::MakeWheels size=%d,WheelMultipleCount[0-9]=%d,%d,nTotal9Multiple=%lld", m_WheelInfo.size(), WheelMultipleCount[0], WheelMultipleCount[8], nTotal9Multiple);

	//计算图集概率
	vector<WheelInfo>::iterator iter = m_WheelInfo.begin();
	for (; iter != m_WheelInfo.end(); iter++)
	{
		//计算每个线的返奖率+
			LOG_DEBUG("CSlotLogic::MakeWheels id =%d, TotalSysRatio[0][0]=%d,TotalSysRatio[8][4]=%d,TotalSysRatio[0][2]=%d,TotalSysRatio[8][2]=%d,",
				iter->id, iter->TotalSysRatio[0][0], iter->TotalSysRatio[8][4], iter->TotalSysRatio[0][2], iter->TotalSysRatio[8][2]);

	}
	//for (int k = 0; k < 10000; k++)
	//{
	//	LOG_DEBUG("CSlotLogic::MakeWheels id =%d,TotalSysRatio[0][0]=%d,SysRatio[8][4]=%d,LineMultiple[8]=%d,m_FeeRate=%d,m_JackpotRate=%d,,m_ReturnSysRate[4]=%d",
	//		m_WheelInfo[k].id, m_WheelInfo[k].TotalSysRatio[8][4], m_WheelInfo[k].SysRatio[8][4], m_WheelInfo[k].LineMultiple[8],
	//		m_FeeRate ,m_JackpotRate ,m_ReturnSysRate[4]);
	//}
	LOG_DEBUG("CSlotLogic::MakeWheels is done size=%d", m_WheelInfo.size());
	m_IsNewWheel = true;
	return;
}