//
// Created by toney on 16/4/12.
//
#include "zajinhua_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_zajinhua
{
//扑克数据
BYTE CZajinhuaLogic::m_cbCardListData[52]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D	//黑桃 A - K
};

BYTE CZajinhuaLogic::m_cbArCardColor[4]  = { 0x00,0x10,0x20,0x30 };
BYTE CZajinhuaLogic::m_cbArCardValue[13] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D };


//////////////////////////////////////////////////////////////////////////
//构造函数
CZajinhuaLogic::CZajinhuaLogic()
{
}

//析构函数
CZajinhuaLogic::~CZajinhuaLogic()
{
}

//获取对子数值
BYTE CZajinhuaLogic::GetDoubleValue(BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbDoubleValue = 0;
	BYTE cbFirstColor = GetCardColor(cbCardData[0]);
	BYTE cbFirstValue = GetCardLogicValue(cbCardData[0]);

	//牌形分析
	bool bDouble = false, bPanther = true;

	//对牌分析
	for (BYTE i = 0; i<cbCardCount - 1; i++)
	{
		for (BYTE j = i + 1; j<cbCardCount; j++)
		{
			if (GetCardLogicValue(cbCardData[i]) == GetCardLogicValue(cbCardData[j]))
			{
				cbDoubleValue = GetCardLogicValue(cbCardData[i]);
				bDouble = true;
				break;
			}
		}
		if (bDouble)
		{
			break;
		}
	}

	//三条(豹子)分析
	for (BYTE i = 1; i<cbCardCount; i++)
	{
		if (bPanther && cbFirstValue != GetCardLogicValue(cbCardData[i]))
		{
			bPanther = false;
		}
	}
	if (bPanther)
	{
		cbDoubleValue = 0;
	}
	return cbDoubleValue;
}

//获取类型
BYTE CZajinhuaLogic::GetCardType(BYTE cbCardData[], BYTE cbCardCount)
{
	ASSERT(cbCardCount==MAX_COUNT);
	//大小排序
	SortCardList(cbCardData,cbCardCount);

	if (cbCardCount==MAX_COUNT)
	{
		//变量定义
		bool cbSameColor=true,bLineCard=true;
		BYTE cbFirstColor=GetCardColor(cbCardData[0]);
		BYTE cbFirstValue=GetCardLogicValue(cbCardData[0]);

		//牌形分析
		for (BYTE i=1;i<cbCardCount;i++)
		{
			//数据分析
			if (GetCardColor(cbCardData[i])!=cbFirstColor) cbSameColor=false;
			if (cbFirstValue!=(GetCardLogicValue(cbCardData[i])+i)) bLineCard=false;

			//结束判断
			if ((cbSameColor==false)&&(bLineCard==false)) break;
		}

		//特殊A32
		if(!bLineCard)
		{
			bool bOne=false,bTwo=false,bThree=false;
			for(BYTE i=0;i<MAX_COUNT;i++)
			{
				if(GetCardValue(cbCardData[i])==1)		bOne=true;
				else if(GetCardValue(cbCardData[i])==2)	bTwo=true;
				else if(GetCardValue(cbCardData[i])==3)	bThree=true;
			}
			if(bOne && bTwo && bThree)bLineCard=true;
		}

		//顺金类型
		if ((cbSameColor)&&(bLineCard)) return CT_SHUN_JIN;

		//顺子类型
		if ((!cbSameColor)&&(bLineCard)) return CT_SHUN_ZI;

		//金花类型
		if((cbSameColor)&&(!bLineCard)) return CT_JIN_HUA;

		//牌形分析
		bool bDouble=false,bPanther=true;

		//对牌分析
		for (BYTE i=0;i<cbCardCount-1;i++)
		{
			for (BYTE j=i+1;j<cbCardCount;j++)
			{
				if (GetCardLogicValue(cbCardData[i])==GetCardLogicValue(cbCardData[j])) 
				{
					bDouble=true;
					break;
				}
			}
			if(bDouble)break;
		}

		//三条(豹子)分析
		for (BYTE i=1;i<cbCardCount;i++)
		{
			if (bPanther && cbFirstValue!=GetCardLogicValue(cbCardData[i])) bPanther=false;
		}

		//对子和豹子判断
		if (bDouble==true) return (bPanther)?CT_BAO_ZI:CT_DOUBLE;

		//特殊235
		bool bTwo=false,bThree=false,bFive=false;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			if(GetCardValue(cbCardData[i])==2)	bTwo=true;
			else if(GetCardValue(cbCardData[i])==3)bThree=true;
			else if(GetCardValue(cbCardData[i])==5)bFive=true;			
		}	
		if (bTwo && bThree && bFive) return CT_SPECIAL;
	}

	return CT_SINGLE;
}
//获得散牌最大值
BYTE CZajinhuaLogic::GetSingCardValue(BYTE cbCardData[], BYTE cbCardCount)
{
	//设置变量
	BYTE FirstData[MAX_COUNT];
	memcpy(FirstData,cbCardData,sizeof(FirstData));
	//大小排序
	SortCardList(FirstData,cbCardCount);

	return GetCardLogicValue(FirstData[0]);
}
//排列扑克
void CZajinhuaLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount,bool bAsc)
{
	//转换数值
	BYTE cbLogicValue[MAX_COUNT];
	for (BYTE i=0;i<cbCardCount;i++) cbLogicValue[i]=GetCardLogicValue(cbCardData[i]);	

	//排序操作
	bool bSorted=true;
	BYTE cbTempData,bLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (BYTE i=0;i<bLast;i++)
		{
            if(bAsc){
    			if((cbLogicValue[i]<cbLogicValue[i+1]) || ((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
    			{
    				//交换位置
    				cbTempData=cbCardData[i];
    				cbCardData[i]=cbCardData[i+1];
    				cbCardData[i+1]=cbTempData;
    				cbTempData=cbLogicValue[i];
    				cbLogicValue[i]=cbLogicValue[i+1];
    				cbLogicValue[i+1]=cbTempData;
    				bSorted=false;
    			}	
            }else{
                if((cbLogicValue[i]>cbLogicValue[i+1]) || ((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]>cbCardData[i+1])))
    			{
    				//交换位置
    				cbTempData=cbCardData[i];
    				cbCardData[i]=cbCardData[i+1];
    				cbCardData[i+1]=cbTempData;
    				cbTempData=cbLogicValue[i];
    				cbLogicValue[i]=cbLogicValue[i+1];
    				cbLogicValue[i+1]=cbTempData;
    				bSorted=false;
    			}                
            }	
		}
		bLast--;
	} while(bSorted==false);

	return;
}

//混乱扑克
void CZajinhuaLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	//混乱准备
	BYTE cbCardData[getArrayLen(m_cbCardListData)];
	memcpy(cbCardData,m_cbCardListData,sizeof(m_cbCardListData));

	//混乱扑克
	BYTE bRandCount=0,bPosition=0;
	do
	{
		bPosition=rand()%(getArrayLen(m_cbCardListData)-bRandCount);
		cbCardBuffer[bRandCount++]=cbCardData[bPosition];
		cbCardData[bPosition]=cbCardData[getArrayLen(m_cbCardListData)-bRandCount];
	} while (bRandCount<cbBufferCount);
    
	return;
}
//逻辑数值
BYTE CZajinhuaLogic::GetCardLogicValue(BYTE cbCardData)
{
	//扑克属性
	BYTE bCardColor=GetCardColor(cbCardData);
	BYTE bCardValue=GetCardValue(cbCardData);
	// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D
	//	 14	   2	3	 4	  5	   6    7    8	  9   10   11   12   13
	//转换数值
	return (bCardValue==1)?(bCardValue+13):bCardValue;
}

//对比扑克
BYTE CZajinhuaLogic::CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount)
{
	//设置变量
	BYTE FirstData[MAX_COUNT],NextData[MAX_COUNT];
	memcpy(FirstData,cbFirstData,sizeof(FirstData));
	memcpy(NextData,cbNextData,sizeof(NextData));

	//大小排序
	SortCardList(FirstData,cbCardCount);
	SortCardList(NextData,cbCardCount);

	//获取类型
	BYTE cbNextType=GetCardType(NextData,cbCardCount);
	BYTE cbFirstType=GetCardType(FirstData,cbCardCount);

	//特殊情况分析
	if((cbNextType+cbFirstType)==(CT_SPECIAL+CT_BAO_ZI))return (BYTE)(cbFirstType>cbNextType);

	//还原单牌类型
	if(cbNextType==CT_SPECIAL)cbNextType=CT_SINGLE;
	if(cbFirstType==CT_SPECIAL)cbFirstType=CT_SINGLE;

	//类型判断
	if (cbFirstType!=cbNextType) return (cbFirstType>cbNextType)?1:0;

	//简单类型
	switch(cbFirstType)
	{
	case CT_BAO_ZI:			//豹子
	case CT_SINGLE:			//单牌
	case CT_JIN_HUA:		//金花
		{
			//对比数值
			for (BYTE i=0;i<cbCardCount;i++)
			{
				BYTE cbNextValue=GetCardLogicValue(NextData[i]);
				BYTE cbFirstValue=GetCardLogicValue(FirstData[i]);
				if (cbFirstValue!=cbNextValue) return (cbFirstValue>cbNextValue)?1:0;
			}
			return DRAW;
		}
	case CT_SHUN_ZI:		//顺子
	case CT_SHUN_JIN:		//顺金 432>A32
		{		
			BYTE cbNextValue=GetCardLogicValue(NextData[0]);
			BYTE cbFirstValue=GetCardLogicValue(FirstData[0]);

			//特殊A32
			if(cbNextValue==14 && GetCardLogicValue(NextData[cbCardCount-1])==2)
			{
				cbNextValue=3;
			}
			if(cbFirstValue==14 && GetCardLogicValue(FirstData[cbCardCount-1])==2)
			{
				cbFirstValue=3;
			}

			//对比数值
			if (cbFirstValue!=cbNextValue) return (cbFirstValue>cbNextValue)?1:0;;
			return DRAW;
		}
	case CT_DOUBLE:			//对子
		{
			BYTE cbNextValue=GetCardLogicValue(NextData[0]);
			BYTE cbFirstValue=GetCardLogicValue(FirstData[0]);

			//查找对子/单牌
			BYTE bNextDouble=0,bNextSingle=0;
			BYTE bFirstDouble=0,bFirstSingle=0;
			if(cbNextValue==GetCardLogicValue(NextData[1]))
			{
				bNextDouble=cbNextValue;
				bNextSingle=GetCardLogicValue(NextData[cbCardCount-1]);
			}
			else
			{
				bNextDouble=GetCardLogicValue(NextData[cbCardCount-1]);
				bNextSingle=cbNextValue;
			}
			if(cbFirstValue==GetCardLogicValue(FirstData[1]))
			{
				bFirstDouble=cbFirstValue;
				bFirstSingle=GetCardLogicValue(FirstData[cbCardCount-1]);
			}
			else 
			{
				bFirstDouble=GetCardLogicValue(FirstData[cbCardCount-1]);
				bFirstSingle=cbFirstValue;
			}

			if (bNextDouble!=bFirstDouble)return (bFirstDouble>bNextDouble)?1:0;
			if (bNextSingle!=bFirstSingle)return (bFirstSingle>bNextSingle)?1:0;
			return DRAW;
		}
	}

	return DRAW;
}

bool CZajinhuaLogic::GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][MAX_COUNT])
{
	int iArTempProCardType[GAME_PLAYER] = { 0 };
	for (int i = 0; i < GAME_PLAYER; i++)
	{
		iArTempProCardType[i] = iArProCardType[i];
	}

	BYTE cbArTempCardData[GAME_PLAYER][MAX_COUNT] = { 0 };
	memset(cbArTempCardData, 0, sizeof(cbArTempCardData));

	BYTE cbCardListData[FULL_COUNT] = { 0 };
	memcpy(cbCardListData, m_cbCardListData, sizeof(m_cbCardListData));

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		bool bIsFlag = false;
		if (iArTempProCardType[i] == Pro_Index_BaoZi)
		{
			bIsFlag = GetTypeBaoZi(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}
		else if (iArTempProCardType[i] == Pro_Index_ShunJin)
		{
			bIsFlag = GetTypeShunJin(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}
		else if (iArTempProCardType[i] == Pro_Index_JinHua)
		{
			bIsFlag = GetTypeJinHua(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}
		else if (iArTempProCardType[i] == Pro_Index_ShunZi)
		{
			bIsFlag = GetTypeShunZi(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}
		else if (iArTempProCardType[i] == Pro_Index_Double)
		{
			bIsFlag = GetTypeDouble(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}
		else if (iArTempProCardType[i] == Pro_Index_Single)
		{
			bIsFlag = GetTypeSingle(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
		}

		//LOG_DEBUG("随机获取牌 1 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i,bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

		// 删除已经获取的牌值
		if (bIsFlag)
		{
			for (int c1 = 0; c1 < MAX_COUNT; c1++)
			{
				BYTE cbCard = cbArTempCardData[i][c1];
				for (int c2 = 0; c2 < FULL_COUNT; c2++)
				{
					if (cbCard == cbCardListData[c2])
					{
						cbCardListData[c2] = 0;
					}
				}
			}
		}
		else
		{
			iArTempProCardType[i] = Pro_Index_MAX;
			// error
		}

	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		if (iArTempProCardType[i] == Pro_Index_MAX)
		{
			bool bIsFlag = GetTypeSingle(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT);
			//LOG_DEBUG("随机获取牌 2 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i, bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

			// 删除已经获取的牌值
			for (int c1 = 0; c1 < MAX_COUNT; c1++)
			{
				BYTE cbCard = cbArTempCardData[i][c1];
				for (int c2 = 0; c2 < FULL_COUNT; c2++)
				{
					if (cbCard == cbCardListData[c2])
					{
						cbCardListData[c2] = 0;
					}
				}
			}
		}
	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		for (int j = 0; j < MAX_COUNT; j++)
		{
			cbArCardData[i][j] = cbArTempCardData[i][j];
		}
	}
	return false;
}

bool CZajinhuaLogic::IsValidCard(BYTE cbCardData)
{
	for (int i = 0; i < FULL_COUNT; i++)
	{
		if (m_cbCardListData[i] == cbCardData)
		{
			return true;
		}
	}
	return false;
}

//获取颜色
BYTE CZajinhuaLogic::GetRandCardColor()
{
	BYTE cbPosition = g_RandGen.RandRange(0, 3);
	BYTE cbCardColor = m_cbArCardColor[cbPosition];
	return cbCardColor;
}

//获取数值
BYTE CZajinhuaLogic::GetRandCardValue()
{
	BYTE cbPosition = g_RandGen.RandRange(0, 12);
	BYTE cbCardValue = m_cbArCardValue[cbPosition];
	return cbCardValue;
}

//获取豹子
bool CZajinhuaLogic::GetTypeBaoZi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 统计数值数目
	BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue < MAX_CARD_VALUE)
			{
				cbArCardValueCount[cbCardValue]++;
			}
		}
	}
	// 找出豹子
	BYTE cbArAllBaoZi[MAX_CARD_VALUE] = { 0 };
	int iAllBaoZiCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 3)
		{
			cbArAllBaoZi[iAllBaoZiCount] = i;
			iAllBaoZiCount++;
		}
	}
	if (iAllBaoZiCount <= 0)
	{
		return false;
	}
	// 随机豹子
	BYTE cbBaoZiPosition = g_RandGen.RandRange(0, iAllBaoZiCount - 1);
	BYTE cbBaoZiCardValue = cbArAllBaoZi[cbBaoZiPosition];
	// 豹子颜色
	BYTE cbBaoZiCard[MAX_CARD_COLOR] = { 0 };
	BYTE cbBaoZiCardCount = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbBaoZiCardValue)
			{
				cbBaoZiCard[cbBaoZiCardCount] = cbCardListData[i];
				cbBaoZiCardCount++;
			}
		}
	}
	if (cbBaoZiCardCount < cbCardCount)
	{
		return false;
	}
	// 选其中三个
	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		BYTE cbPosition = g_RandGen.RandRange(0, cbBaoZiCardCount - cbRandCount - 1);
		cbCardData[cbRandCount++] = cbBaoZiCard[cbPosition];
		cbBaoZiCard[cbPosition] = cbBaoZiCard[cbBaoZiCardCount - cbRandCount];
	} while (cbRandCount < cbCardCount);

	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	return true;
}

//获取顺金（同花顺）
bool CZajinhuaLogic::GetTypeShunJin(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{

	BYTE cbTempListCount = 0;
	BYTE cbTempCardListData[FULL_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbTempCardListData[cbTempListCount++] = cbCardListData[i];
		}
	}
	//for (int i = 0; i < cbTempListCount; i++)
	//{
	//	LOG_DEBUG("获取顺金 a - i:%d,cbTempListCount:%d,cbTempCardListData:0x%02X", i, cbTempListCount, cbTempCardListData[i]);
	//}

	// 统计数值数目
	BYTE cbArCardColorCount[MAX_CARD_VALUE][MAX_CARD_COLOR] = { 0 };
	memset(cbArCardColorCount, 0, sizeof(cbArCardColorCount));
	// 每个值的颜色最多有一张牌
	// 0	1	2	3
	// 1
	// .
	// .
	// 12
	// 13
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			cbArCardColorCount[cbCardValue][cbCardColor]++;
		}
	}

	//for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	//{
	//	for (BYTE j = 0; j < MAX_CARD_COLOR; j++)
	//	{
	//		LOG_DEBUG("获取顺金 0 - i:%d,j:%d,cbArCardColorCount:%d",i,j, cbArCardColorCount[i][j]);
	//	}
	//}

	// 找出顺金
	BYTE cbArAllShunJinValue[FULL_COUNT] = { 0 };
	BYTE cbArAllShunJinColor[FULL_COUNT] = { 0 };
	int iAllShunJinCount = 0;
	for (BYTE i = 1; i < MAX_CARD_VALUE - 2; i++)
	{
		//	  0	   1	2    3	  4	   5	6	 7	  8	   9   10	11	 12	  13
		// 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D
		for (BYTE j = 0; j < MAX_CARD_COLOR; j++)
		{
			if (cbArCardColorCount[i][j] == 1 && cbArCardColorCount[i + 1][j] == 1 && cbArCardColorCount[i + 2][j] == 1)
			{
				cbArAllShunJinValue[iAllShunJinCount] = i;
				cbArAllShunJinColor[iAllShunJinCount] = j;
				iAllShunJinCount++;
			}
		}
	}

	//for (int i = 0; i < FULL_COUNT; i++)
	//{
	//	LOG_DEBUG("获取顺金 1 - i:%d,iAllShunJinCount:%d,cbArAllShunJinColor[i]:%d,cbArAllShunJinValue[i]:%d",i, iAllShunJinCount,cbArAllShunJinColor[i], cbArAllShunJinValue[i]);
	//}

	if (iAllShunJinCount <= 0)
	{
		return false;
	}

	// 随机顺金
	BYTE cbShunJinPosition = g_RandGen.RandRange(0, iAllShunJinCount - 1);
	BYTE cbShunJinCardColor = cbArAllShunJinColor[cbShunJinPosition];
	BYTE cbShunJinCardValue = cbArAllShunJinValue[cbShunJinPosition];

	
	//LOG_DEBUG("获取顺金 2 - cbShunJinPosition:%d,cbShunJinCardColor:%d, cbShunJinCardValue:%d",	cbShunJinPosition, cbShunJinCardColor, cbShunJinCardValue );


	// 顺金颜色
	BYTE cbShunJinCardCount = 0;
	BYTE cbShunJinCard[MAX_COUNT] = { 0 };

	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);

			bool bIsAddShunJin = ((cbCardValue == cbShunJinCardValue) || (cbCardValue == cbShunJinCardValue + 1) || (cbCardValue == cbShunJinCardValue + 2));
			if (cbShunJinCardCount < MAX_COUNT && cbCardColor == cbShunJinCardColor && bIsAddShunJin)
			{
				//LOG_DEBUG("获取顺金 3 - i:%d,cbShunJinCardCount:%d,cbCardColor:%d,cbCardValue:%d,cbCardListData:0x%02X", i, cbShunJinCardCount, cbCardColor, cbCardValue, cbCardListData[i]);

				cbShunJinCard[cbShunJinCardCount++] = cbCardListData[i];
			}
		}
	}
	if (cbShunJinCardCount != cbCardCount)
	{
		return false;
	}
	for (BYTE i = 0; i < cbCardCount; i++)
	{
		cbCardData[i] = cbShunJinCard[i];
	}
	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}
	return true;
}
//获取金花（同花）
bool CZajinhuaLogic::GetTypeJinHua(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 统计颜色数目
	BYTE cbArCardColorCount[MAX_CARD_COLOR] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			if (cbCardColor < MAX_CARD_COLOR)
			{
				cbArCardColorCount[cbCardColor]++;
			}
		}
	}

	// 找出金花
	BYTE cbArAllJinHua[MAX_CARD_COLOR] = { 0 };
	int iAllJinHuaCount = 0;
	for (BYTE i = 0; i < MAX_CARD_COLOR; i++)
	{
		if (cbArCardColorCount[i] >= 3)
		{
			cbArAllJinHua[iAllJinHuaCount++] = i;
		}
	}
	if (iAllJinHuaCount <= 0)
	{
		return false;
	}

	// 随机金花颜色
	BYTE cbJinHuaPosition = g_RandGen.RandRange(0, iAllJinHuaCount - 1);
	BYTE cbJinHuaCardColor = cbArAllJinHua[cbJinHuaPosition];


	// 金花颜色的所有牌
	BYTE cbJinHuaCard[MAX_CARD_VALUE] = { 0 };
	BYTE cbJinHuaCardCount = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			if (cbCardColor == cbJinHuaCardColor && cbJinHuaCardCount < MAX_CARD_VALUE)
			{
				cbJinHuaCard[cbJinHuaCardCount++] = cbCardListData[i];
			}
		}
	}

	// 选其中三个
	BYTE cbTargetCard[MAX_COUNT] = { 0 };
	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		BYTE cbPosition = g_RandGen.RandRange(0, cbJinHuaCardCount - cbRandCount - 1);
		cbTargetCard[cbRandCount++] = cbJinHuaCard[cbPosition];
		cbJinHuaCard[cbPosition] = cbJinHuaCard[cbJinHuaCardCount - cbRandCount];
	} while (cbRandCount < cbCardCount);

	//检查是否顺金
	SortCardList(cbTargetCard, MAX_COUNT);
	bool bLineCard = true;
	BYTE cbFirstValue = GetCardValue(cbTargetCard[0]);
	for (BYTE i = 1; i<cbCardCount; i++)
	{
		if (cbFirstValue != (GetCardValue(cbTargetCard[i]) + i)) bLineCard = false;
		if (bLineCard == false)break;
	}
	
	if (bLineCard == false)
	{
		for (BYTE i = 0; i < cbCardCount; i++)
		{
			cbCardData[i] = cbTargetCard[i];
		}
		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}
		return true;
	}

	return false;
}
//获取顺子
bool CZajinhuaLogic::GetTypeShunZi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 统计数值数目
	BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue < MAX_CARD_VALUE)
			{
				cbArCardValueCount[cbCardValue]++;
			}
		}
	}

	// 找出顺子
	BYTE cbArAllTargetData[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE - 2; i++)
	{
		//	  0	   1	2    3	  4	   5	6	 7	  8	   9   10	11	 12	  13
		// 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D };
		// 需要有两种花色才不会随到顺金
		if (i <= 0x0B && cbArCardValueCount[i] >= 2 && cbArCardValueCount[i + 1] >= 2 && cbArCardValueCount[i + 2] >= 2)
		{
			cbArAllTargetData[iAllTargetCount++] = i;
		}
	}
	if (iAllTargetCount <= 0)
	{
		return false;
	}
	// 随机顺子
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetData[cbTargetPosition];

	// 选出该数值顺子的全部扑克
	BYTE cbTargetColorCount[MAX_COUNT] = {0};
	BYTE cbTargetCard[MAX_COUNT][MAX_CARD_COLOR] = { 0 };
	// 三行四列
	// 06 16 26 36
	// 07 17
	// 08 28 38	
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);			
			for (BYTE j = 0; j < MAX_COUNT; j++)
			{
				if (cbCardValue == cbTargetCardValue + j)
				{
					if (cbTargetColorCount[j] < MAX_CARD_COLOR)
					{
						cbTargetCard[j][cbTargetColorCount[j]] = cbCardListData[i];
						cbTargetColorCount[j]++;
					}
				}
			}
		}
	}

	//BYTE cbTempListCount = 0;
	//BYTE cbTempCardListData[FULL_COUNT] = { 0 };
	//for (BYTE i = 0; i < cbListCount; i++)
	//{
	//	if (IsValidCard(cbCardListData[i]))
	//	{
	//		cbTempCardListData[cbTempListCount++] = cbCardListData[i];
	//	}
	//}

	//for (int i = 0; i < cbTempListCount; i++)
	//{
	//	LOG_DEBUG("获取顺子 - i:%d,cbTempListCount:%d,cbTempCardListData:0x%02X", i, cbTempListCount, cbTempCardListData[i]);
	//}

	//for (int i = 0; i < MAX_COUNT; i++)
	//{
	//	LOG_DEBUG("获取顺子 - i:%d,cbTargetColorCount:%d,cbTargetCard:0x%02X 0x%02X 0x%02X 0x%02X",i, cbTargetColorCount[i], cbTargetCard[i][0], cbTargetCard[i][1], cbTargetCard[i][2], cbTargetCard[i][3]);
	//}
	


	// 取出顺子
	bool bIsHaveCard = true;
	for (BYTE i = 0; i < MAX_COUNT; i++)
	{
		if (cbTargetColorCount[i] == 0)
		{
			bIsHaveCard = false;
		}
	}
	if (bIsHaveCard)
	{
		for (BYTE i = 0; i < MAX_COUNT; i++)
		{
			if (cbTargetColorCount[i] > 0)
			{
				BYTE cbPosition = g_RandGen.RandRange(0, cbTargetColorCount[i] - 1);
				cbCardData[i] = cbTargetCard[i][cbPosition];
			}
			else
			{
				return false;
			}
		}

		// 需要有两种花色才不会随到顺金
		bool bIsYiSe = true;
		BYTE cbCurColor = 0xFF;
		for (BYTE i = 0; i < MAX_COUNT; i++)
		{
			BYTE cbCardColor = GetCardColorValue(cbCardData[i]);
			if (cbCurColor == 0xFF)
			{
				cbCurColor = cbCardColor;
			}
			if (cbCurColor != cbCardColor)
			{
				bIsYiSe = false;
			}
		}
		if (bIsYiSe)
		{
			bool bIsBreak = false;
			for (BYTE i = 0; i < MAX_COUNT; i++)
			{
				BYTE cbCardColor = GetCardColorValue(cbCardData[i]);
				for (BYTE j = 0; j < cbTargetColorCount[i]; j++)
				{
					if (cbCardColor != GetCardColorValue(cbTargetCard[i][j]))
					{
						cbCardData[i] = cbTargetCard[i][j];
						bIsBreak = true;
						break;
						//return true;
					}
				}
				if (bIsBreak)
				{
					break;
				}
			}
		}

		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

//获取对子
bool CZajinhuaLogic::GetTypeDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 统计数值数目
	BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue < MAX_CARD_VALUE)
			{
				cbArCardValueCount[cbCardValue]++;
			}
		}
	}

	// 找出对子
	BYTE cbArAllTargetData[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 2)
		{
			cbArAllTargetData[iAllTargetCount] = i;
			iAllTargetCount++;
		}
	}
	if (iAllTargetCount <= 0)
	{
		return false;
	}
	// 随机对子
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetData[cbTargetPosition];

	// 选出该数值对子的全部扑克
	BYTE cbTargetColorCount = 0;
	BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };

	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbTargetColorCount < MAX_CARD_COLOR && cbTargetCardValue == cbCardValue)
			{
				cbTargetCard[cbTargetColorCount++] = cbCardListData[i];
			}
		}
	}
	// 取出对子
	if (cbTargetColorCount < 2)
	{
		return false;
	}

	// 选其中两个
	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		BYTE cbPosition = g_RandGen.RandRange(0, cbTargetColorCount - cbRandCount - 1);
		cbCardData[cbRandCount++] = cbTargetCard[cbPosition];
		cbTargetCard[cbPosition] = cbTargetCard[cbTargetColorCount - cbRandCount];
	} while (cbRandCount < cbCardCount - 1);

	//随机最后一个数字
	BYTE cbTempListCount = 0;
	BYTE cbTempCardListData[FULL_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbTempListCount < FULL_COUNT &&cbCardValue != cbTargetCardValue)
			{
				cbTempCardListData[cbTempListCount++] = cbCardListData[i];
			}
		}
	}

	BYTE cbRandPosition = g_RandGen.RandRange(0, cbTempListCount - 1);
	cbCardData[cbRandCount] = cbTempCardListData[cbRandPosition];

	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	return true;
}

//获取散牌
bool CZajinhuaLogic::GetTypeSingle(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[FULL_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbArTempCardData[cbTempListCount++] = cbCardListData[i];
		}
	}
	if (cbTempListCount < cbCardCount)
	{
		return false;
	}
	// 随便选三张牌 不用管了
	BYTE cbTargetCard[MAX_COUNT] = { 0 };
	for (int i = 0; i < 1000; i++)
	{
		memset(cbTargetCard, 0, sizeof(cbTargetCard));
		BYTE cbTempCardListData[FULL_COUNT] = { 0 };
		memcpy(cbTempCardListData, cbArTempCardData, sizeof(cbTempCardListData));
		BYTE cbRandCount = 0, cbPosition = 0;
		do
		{
			BYTE cbPosition = g_RandGen.RandRange(0, cbTempListCount - cbRandCount - 1);
			cbTargetCard[cbRandCount++] = cbTempCardListData[cbPosition];
			cbTempCardListData[cbPosition] = cbTempCardListData[cbTempListCount - cbRandCount];
		} while (cbRandCount < cbCardCount);

		if (GetCardType(cbTargetCard, cbCardCount) == CT_SINGLE)
		{
			break;
		}
	}

	for (BYTE i = 0; i < cbCardCount; i++)
	{
		cbCardData[i] = cbTargetCard[i];
	}

	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	return true;
}

};



