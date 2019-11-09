
#ifndef _RAND_TABLE_H__
#define _RAND_TABLE_H__

#include <vector>
#include <time.h>
#include <random>
#include <functional>

#include "utility/basicTypes.h"

using namespace std;

class RandTable
{
public:
	RandTable();
	~RandTable();

	//	添加某一个类型及其概率进入圆桌
	void AddIntoRoundTable(int32 iType, uint32 iProbability);

	//	通过随机值获取某一个类型
	int32  GetTypeFromRoundTable(uint32 iRand);

	void Reset();
private:
	struct TROUNT_TABLE_ELEMENT
	{
		uint32 iStartPos;
		uint32 iEndPos;
		int32  Type;
	};
	vector<TROUNT_TABLE_ELEMENT> m_vtTableElement;
	uint32 	m_iNowStartPos;
	uint32 	m_iTypeNum;
};

//随机数生成类

class RandGen
{	
private:
	uint32 m_Seed[3];
	static const uint32 Max32BitLong = 0xFFFFFFFFLU;
public:
	static const uint32 RandomMax = Max32BitLong;

	RandGen(const uint32 p_Seed = 0)
	{
		Reset(p_Seed);
	}
	//ReSeed the random number generator
	//种子处理
	void Reset(const uint32 p_Seed = 0)
	{		
		m_Seed[0] = (p_Seed ^ 0xFEA09B9DLU) & 0xFFFFFFFELU;
		m_Seed[0] ^= (((m_Seed[0] << 7) & Max32BitLong) ^ m_Seed[0]) >> 31;

		m_Seed[1] = (p_Seed ^ 0x9C129511LU) & 0xFFFFFFF8LU;
		m_Seed[1] ^= (((m_Seed[1] << 2) & Max32BitLong) ^ m_Seed[1]) >> 29;

		m_Seed[2] = (p_Seed ^ 0x2512CFB8LU) & 0xFFFFFFF0LU;
		m_Seed[2] ^= (((m_Seed[2] << 9) & Max32BitLong) ^ m_Seed[2]) >> 28;

		RandUInt();
	}

	//Returns an unsigned integer from 0..RandomMax
	//0~RandMax uint 随机数
	uint32 RandUInt()
	{
		m_Seed[0] = (((m_Seed[0] & 0xFFFFFFFELU) << 24) & Max32BitLong)
			^ ((m_Seed[0] ^ ((m_Seed[0] << 7) & Max32BitLong)) >> 7);

		m_Seed[1] = (((m_Seed[1] & 0xFFFFFFF8LU) << 7) & Max32BitLong)
			^ ((m_Seed[1] ^ ((m_Seed[1] << 2) & Max32BitLong)) >> 22);

		m_Seed[2] = (((m_Seed[2] & 0xFFFFFFF0LU) << 11) & Max32BitLong)
			^ ((m_Seed[2] ^ ((m_Seed[2] << 9) & Max32BitLong)) >> 17);

		return (m_Seed[0] ^ m_Seed[1] ^ m_Seed[2]);
	}
	//Returns a double in [0.0, 1.0]
	//返回0.0~1.0之间的双精度浮点
	double RandDouble()
	{
		return static_cast<double>(RandUInt())
			/ (static_cast<double>(RandomMax) );
	}	
	int	   GetRand(int nStart, int nEnd);
	
	int32  RandRange(int32 nMax,int32 nMin);
	bool   RandRatio(uint32 uRatio,uint32 uRatioMax = 10000);
	
	float GetRandf(float min, float max)
	{
		std::random_device r;
		std::default_random_engine generator(r());
		std::uniform_real_distribution<float> dis(min, max);
		auto dice = std::bind(dis, generator);
		return dice();
	}

	int GetRandi(int min, int max)
	{
		std::random_device r;
		std::default_random_engine generator(r());
		std::uniform_int_distribution<int> dis(min, max);
		auto dice = std::bind(dis, generator);
		return dice();
	}

	bool GetRandp(uint32 uRatio, uint32 uRatioMax)
	{
		if (uRatio == 0)
		{
			return false;
		}
		return (uRatio >= (uint32)GetRandi(uRatioMax, 0));
	}
};

extern	RandGen	g_RandGen;




#endif //_RAND_TABLE_H__

