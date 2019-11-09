
#include "robniu_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

/*

   1    2    3    4    5    6    7    8    9   10   11   12   13
0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
  17   18   19   20   21   22   23   24   25   26   27   28   29
0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
  33   34   35   36   37   38   39   40   41   42   43   44   45
0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
  49   50   51   52   53   54   55   56   57   58   59   60   61
0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K


*/

namespace game_robniu
{
//扑克数据
const BYTE CRobNiuLogic::m_cbCardListData[CARD_COUNT]=
{
	    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
		0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
		0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K		
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CRobNiuLogic::CRobNiuLogic()
{
}

//析构函数
CRobNiuLogic::~CRobNiuLogic()
{
}

//混乱扑克
void CRobNiuLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	//混乱准备
	BYTE cbCardData[CARD_COUNT];
	memcpy(cbCardData,m_cbCardListData,sizeof(m_cbCardListData));

	//混乱扑克
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=g_RandGen.RandUInt()%(CARD_COUNT-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[CARD_COUNT-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	return;
}

void CRobNiuLogic::RandCardListEx(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	BYTE *cbCardData=new BYTE[cbBufferCount];
	memcpy(cbCardData,cbCardBuffer,sizeof(BYTE)*cbBufferCount);

	//混乱扑克
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=g_RandGen.RandUInt()%(cbBufferCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);
	delete[] cbCardData;
}

bool CRobNiuLogic::RandUpsetCardData(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	if (cbCardBuffer == NULL || cbBufferCount <= 1)
	{
		return true;
	}

	BYTE cbRandCardData[CARD_COUNT];
	memcpy(cbRandCardData, cbCardBuffer, cbBufferCount);

	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		cbPosition = g_RandGen.RandRange(0, cbBufferCount - cbRandCount - 1);

		BYTE cbTempSwapCardData = cbRandCardData[cbPosition];
		cbRandCardData[cbPosition] = cbRandCardData[cbBufferCount - cbRandCount - 1];
		cbRandCardData[cbBufferCount - cbRandCount - 1] = cbTempSwapCardData;

		cbRandCount++;
	} while (cbRandCount < cbBufferCount);

	for (BYTE i = 0; i < cbBufferCount; i++)
	{
		cbCardBuffer[i] = cbRandCardData[i];
	}

	for (int i = 0; i < cbBufferCount; i++)
	{
		if (IsValidCard(cbCardBuffer[i]) == false)
		{
			return false;
		}
	}

	return true;
}

int CRobNiuLogic::RetType(int itype)
{
	itype = itype%10;
	switch(itype)
	{
	case 0:
		return CT_SPECIAL_NIUNIU;
	case 1:
		return CT_SPECIAL_NIU1;
		break;
	case 2:
		return CT_SPECIAL_NIU2;
		break;
	case 3:
		return CT_SPECIAL_NIU3;
		break;
	case 4:
		return CT_SPECIAL_NIU4;
		break;
	case 5:
		return CT_SPECIAL_NIU5;
		break;
	case 6:
		return CT_SPECIAL_NIU6;
		break;
	case 7:
		return CT_SPECIAL_NIU7;
		break;
	case 8:
		return CT_SPECIAL_NIU8;
		break;
	case 9:
		return CT_SPECIAL_NIU9;
		break;
	default :
		return CT_POINT;
		break;
	}
}
//获取牌型
BYTE CRobNiuLogic::GetCardType(const BYTE cbCardData[], BYTE cbCardCount,BYTE *bcOutCadData )
{
	//合法判断
	ASSERT(5==cbCardCount);
	if (5!=cbCardCount) return CT_ERROR;

	//排序扑克
	BYTE cbCardDataSort[CARD_COUNT];
	memcpy(cbCardDataSort,cbCardData,sizeof(BYTE)*cbCardCount);
	SortCardList(cbCardDataSort,cbCardCount,ST_NEW);

	if(bcOutCadData != NULL)
	{
		memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
	}
	int igetW= 0;
	//            cbCardCount-2 = 3
	// cbCardDataSort[0] [1] [2] [3] [4]
	// 炸弹：四张相同牌+一张任意散牌
	if(GetCardNewValue(cbCardDataSort[0])==GetCardNewValue(cbCardDataSort[cbCardCount-2]))
	{
		if(bcOutCadData != NULL)
		{
			memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
		}
		return CT_SPECIAL_BOMEBOME;
	}
	else
	{
		// 炸弹：五张相同牌
		if(GetCardNewValue(cbCardDataSort[1])==GetCardNewValue(cbCardDataSort[cbCardCount-1]))
		{
			if(bcOutCadData != NULL)
			{
				memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
			}
			return CT_SPECIAL_BOMEBOME;
		}
	}
    //判断五小牛
    bool    bFlag = true;
    uint32  tenCount = 0;
    for(uint8 i=0;i<cbCardCount;++i)
    {
        uint32 v = GetCardLogicValue(cbCardData[i]);
        tenCount += v;
        if(v > 4 || tenCount > 10)
		{
            bFlag = false;
            break;
        }
    }
    if(bFlag)
	{
		// 五小牛：五张牌点数加起来小于10，且每张牌都小于5
        return CT_SPECIAL_NIUNIUXW;
    }
    //判断五花牛
    bFlag = true;
    for(uint8 i=0;i<cbCardCount;++i)
    {
		//五花牛：五张均为花牌（J~K）所组成的牌型
        if(GetCardLogicValue(cbCardData[i]) != 10 || GetCardValue(cbCardData[i]) == 10)
        {
            bFlag = false;
            break;
        }        
    }
    if(bFlag)
	{
        return CT_SPECIAL_NIUNIUDW;
    }

	if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
	{
		if(GetCardNewValue(cbCardDataSort[2])==GetCardNewValue(cbCardDataSort[3]))
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[2];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[0];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[4];
			}
			return CT_SPECIAL_BOMEBOME;
		}
		else
		{
			if(GetCardNewValue(cbCardDataSort[3])==GetCardNewValue(cbCardDataSort[4]))
			{
				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[0];
					bcOutCadData[1] = cbCardDataSort[1];
					bcOutCadData[2] = cbCardDataSort[3];
					bcOutCadData[3] = cbCardDataSort[4];
					bcOutCadData[4] = cbCardDataSort[2];
				}
				return CT_SPECIAL_BOMEBOME;
			}
		}
	}
	if(GetCardColor(cbCardDataSort[0])==0x04)
	{
		if(GetCardNewValue(cbCardDataSort[1])==GetCardNewValue(cbCardDataSort[3]))
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[1];
				bcOutCadData[1] = cbCardDataSort[2];
				bcOutCadData[2] = cbCardDataSort[3];
				bcOutCadData[3] = cbCardDataSort[0];
				bcOutCadData[4] = cbCardDataSort[4];
			}
			return CT_SPECIAL_BOMEBOME;
		}
		else
		{
			if(GetCardNewValue(cbCardDataSort[2])==GetCardNewValue(cbCardDataSort[4]))
			{
				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[2];
					bcOutCadData[1] = cbCardDataSort[3];
					bcOutCadData[2] = cbCardDataSort[4];
					bcOutCadData[3] = cbCardDataSort[0];
					bcOutCadData[4] = cbCardDataSort[1];
				}
				return CT_SPECIAL_BOMEBOME;
			}
		}
	}

	//bool blBig = true;
	//int iCount = 0;
	//int iLogicValue = 0;
	//int iValueTen = 0;
	//for (int i = 0;i<cbCardCount;i++)
	//{
	//	BYTE bcGetValue = GetCardLogicValue(cbCardDataSort[i]);
	//	if(bcGetValue!=10&&bcGetValue!=11)
	//	{
	//		blBig = false;
	//		break;
	//	}
	//	else
	//	{
	//		if(GetCardNewValue(cbCardDataSort[i])==10)
	//		{
	//			iValueTen++;
	//		}
	//	}
	//	iCount++;
	//}
	/*
	if(blBig)
	{
		if(bcOutCadData != NULL)
		{
			CopyMemory(bcOutCadData,cbCardDataSort,cbCardCount);
		}
		if(iValueTen==0)
		{
			return CT_SPECIAL_NIUKING;
		}
		else
		{
			if(iValueTen==1)
			{
				return CT_SPECIAL_NIUYING;
			}
		}
	}
	*/

	int n = 0;

	BYTE bcMakeMax[5];
	memset(bcMakeMax,0,5);
	int iBigValue = 0;
	BYTE iSingleA[2];
	int iIndex = 0;
	bcMakeMax[0]= cbCardDataSort[n];

	int iGetTenCount = 0;

	for (int iten = 0;iten<cbCardCount;iten++)
	{
		if (GetCardLogicValue(cbCardDataSort[iten]) == 10 || GetCardLogicValue(cbCardDataSort[iten]) == 11)
		{
			iGetTenCount++;
		}
	}
	if( iGetTenCount>=3)
	{
		if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];

			}
			return CT_SPECIAL_NIUNIUDW;

		}
		if(GetCardColor(cbCardDataSort[0])==0x04)
		{
			//大小王与最小的组合成牛 
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];
			}
			if(cbCardDataSort[0]==0x42)
				return CT_SPECIAL_NIUNIUDW;
			else
				return CT_SPECIAL_NIUNIUXW;
		}
		else
		{
			return RetType(GetCardLogicValue(cbCardDataSort[3])+GetCardLogicValue(cbCardDataSort[4]));
		}
	}
	if(iGetTenCount==2||(iGetTenCount==1&&GetCardColor(cbCardDataSort[0])==0x04))
	{
		if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];
			}
			return CT_SPECIAL_NIUNIUDW;
		}
		else
		{
			//如果有一张王 其他任意三张组合为10则是牛牛
			if(GetCardColor(cbCardDataSort[0])==0x04)
			{
				for ( n=1;n<cbCardCount;n++)
				{
					for (int j = 1;j<cbCardCount;j++)
					{
						if(j != n)
						{
							for (int w = 1;w<cbCardCount;w++)
							{
								if(w != n&&w!=j)
								{
									//如果剩余的四张中任意三张能组合位10的整数倍

									if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j])+GetCardLogicValue(cbCardDataSort[w]))%10==0)
									{

										int add = 0;
										for (int y = 1;y<cbCardCount;y++)
										{
											if(y != n&&y!=j&&y!=w)
											{
												iSingleA[add] =cbCardDataSort[y]; 
												add++;

											}

										}
										if(bcOutCadData != NULL)
										{
											bcOutCadData[0] = cbCardDataSort[n];
											bcOutCadData[1] = cbCardDataSort[j];
											bcOutCadData[2] = cbCardDataSort[w];
											bcOutCadData[3] = cbCardDataSort[0];
											bcOutCadData[4] = iSingleA[0];
										}
										if (cbCardDataSort[0] == 0x42)
										{
											return CT_SPECIAL_NIUNIUDW;
										}
										else
										{
											return CT_SPECIAL_NIUNIUXW;
										}
									}
								}
							}
						}
					}
				}
				//如果有一张王 其他任意三张组合不为10则 取两张点数最大的组合
				BYTE bcTmp[4];
				int iBig = 0;
				int in = 0;
				for ( in = 1;in<cbCardCount;in++)
				{
					for (int j = 1;j<cbCardCount;j++)
					{
						if(in != j)
						{
							BYTE bclogic = (GetCardLogicValue(cbCardDataSort[in])+GetCardLogicValue(cbCardDataSort[j]))%10;
							if(bclogic>iBig)
							{
								iBig = bclogic;
								int add = 0;
								bcTmp[0]=cbCardDataSort[in];
								bcTmp[1]=cbCardDataSort[j];
								for (int y = 1;y<cbCardCount;y++)
								{
									if(y != in&&y!=j)
									{
										iSingleA[add] =cbCardDataSort[y]; 
										add++;
									}
								}
								bcTmp[2]=iSingleA[0];
								bcTmp[3]=iSingleA[1];
							}
						}
					}   
				}

				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[0];
					bcOutCadData[1] = bcTmp[2];
					bcOutCadData[2] = bcTmp[3];
					bcOutCadData[3] = bcTmp[0];
					bcOutCadData[4] = bcTmp[1];
				}
				if(iGetTenCount==1&&GetCardColor(cbCardDataSort[0])==0x04)
				{
					//下面还能组合 有两张为 10 也可以组合成牛牛

				}
				else
				{
					//如果没有则比较 完与最小组合最大点数和组合
					return RetType(GetCardLogicValue(bcTmp[0])+GetCardLogicValue(bcTmp[1]));
				}
			}
			else
			{
				if((GetCardLogicValue(cbCardDataSort[2])+GetCardLogicValue(cbCardDataSort[3])+GetCardLogicValue(cbCardDataSort[4]))%10==0)
				{
					if(bcOutCadData != NULL)
					{
						bcOutCadData[0] = cbCardDataSort[2];
						bcOutCadData[1] = cbCardDataSort[3];
						bcOutCadData[2] = cbCardDataSort[4];
						bcOutCadData[3] = cbCardDataSort[0];
						bcOutCadData[4] = cbCardDataSort[1];
					}
					return CT_SPECIAL_NIUNIU;                    
				}
				else
				{
					for ( n= 2;n<cbCardCount;n++)
					{
						for (int j = 2;j<cbCardCount;j++)
						{
							if(j != n)
							{
								if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
								{
									int add = 0;
									for (int y = 2;y<cbCardCount;y++)
									{
										if(y != n&&y!=j)
										{
											iSingleA[add] =cbCardDataSort[y]; 
											add++;

										}
									}
									if(iBigValue<=iSingleA[0]%10)
									{
										iBigValue = GetCardLogicValue(iSingleA[0])%10;
										if(bcOutCadData != NULL)
										{
											bcOutCadData[0]= cbCardDataSort[0];
											bcOutCadData[1]= cbCardDataSort[n]; 
											bcOutCadData[2]= cbCardDataSort[j]; 
											bcOutCadData[3]= cbCardDataSort[1];
											bcOutCadData[4]= iSingleA[0]; 

										}

										if(iBigValue==0)
										{
											return CT_SPECIAL_NIUNIU;                                            
										}
									}

								}
							}
						}
					}
					if(iBigValue != 0)
					{
						return RetType(iBigValue);
					}
				}
			}

		}

		iGetTenCount = 1;

	}
	//4个组合
	if(iGetTenCount==1)
	{
		if(GetCardColor(cbCardDataSort[0])==0x04)
		{
			for ( n= 1;n<cbCardCount;n++)
			{
				for (int j = 1;j<cbCardCount;j++)
				{
					if(j != n)
					{
						//任意两张组合成牛
						if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
						{
							int add = 0;
							for (int y = 1;y<cbCardCount;y++)
							{
								if(y != n&&y!=j)
								{
									iSingleA[add] =cbCardDataSort[y]; 
									add++;

								}

							}

							if(bcOutCadData != NULL)
							{
								bcOutCadData[0] = cbCardDataSort[0];
								bcOutCadData[1] = iSingleA[0];
								bcOutCadData[2] = iSingleA[1];
								bcOutCadData[3] = cbCardDataSort[n];
								bcOutCadData[4] = cbCardDataSort[j];
							}
							if(cbCardDataSort[0]==0x42)
								return CT_SPECIAL_NIUNIUDW;
							else
								return CT_SPECIAL_NIUNIUXW;

						}
					}

				}
			}

			//取4张中组合最大的点数

			BYTE bcTmp[4];
			int iBig = 0;
			int in = 0;
			for ( in = 1;in<cbCardCount;in++)
			{
				for (int j = 1;j<cbCardCount;j++)
				{
					if(in != j)
					{
						BYTE bclogic = (GetCardLogicValue(cbCardDataSort[in])+GetCardLogicValue(cbCardDataSort[j]))%10;
						if(bclogic>iBig)
						{
							iBig = bclogic;
							int add = 0;
							bcTmp[0]=cbCardDataSort[in];
							bcTmp[1]=cbCardDataSort[j];
							for (int y = 1;y<cbCardCount;y++)
							{
								if(y != in&&y!=j)
								{
									iSingleA[add] =cbCardDataSort[y]; 
									add++;
								}

							}
							bcTmp[2]=iSingleA[0];
							bcTmp[3]=iSingleA[1];

						}

					}
				}   
			}

			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = bcTmp[2];
				bcOutCadData[2] = bcTmp[3];
				bcOutCadData[3] = bcTmp[0];
				bcOutCadData[4] = bcTmp[1];
			}
			return RetType(GetCardLogicValue(bcTmp[0])+GetCardLogicValue(bcTmp[1]));

		}
		//取4张中任两张组合为10 然后求另外两张的组合看是否是组合中最大
		for ( n= 1;n<cbCardCount;n++)
		{
			for (int j = 1;j<cbCardCount;j++)
			{
				if(j != n)
				{
					if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
					{
						int add = 0;
						for (int y = 1;y<cbCardCount;y++)
						{
							if(y != n&&y!=j)
							{
								iSingleA[add] =cbCardDataSort[y]; 
								add++;

							}

						}
						if(iBigValue<=(GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1]))%10)
						{
							iBigValue = GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1])%10;
							bcMakeMax[0]= cbCardDataSort[0];
							bcMakeMax[1]= cbCardDataSort[j];
							bcMakeMax[2]= cbCardDataSort[n]; 
							bcMakeMax[3]= iSingleA[0]; 
							bcMakeMax[4]= iSingleA[1]; 

							if(bcOutCadData != NULL)
							{
								memcpy(bcOutCadData,bcMakeMax,cbCardCount);
							}
							if(iBigValue==0)
							{
								return CT_SPECIAL_NIUNIU;								
							}
						}

					}
				}
			}
		}
		if(iBigValue!= 0)
		{
			return RetType(iBigValue);
		}else
		{
			//如果组合不成功
			iGetTenCount = 0;
		}

	}
	if(iGetTenCount==0)
	{
		//5个组合
		for ( n= 0;n<cbCardCount;n++)
		{
			for (int j = 0;j<cbCardCount;j++)
			{
				if(j != n)
				{
					for (int w = 0;w<cbCardCount;w++)
					{
						if(w != n&&w!=j)
						{
							int valueAdd = GetCardLogicValue(cbCardDataSort[n]);
							valueAdd += GetCardLogicValue(cbCardDataSort[j]);
							valueAdd += GetCardLogicValue(cbCardDataSort[w]);

							if(valueAdd%10==0)
							{
								int add = 0;
								for (int y = 0;y<cbCardCount;y++)
								{
									if(y != n&&y!=j&&y!=w)
									{
										iSingleA[add] =cbCardDataSort[y]; 
										add++;

									}

								}
								if(iBigValue<=(GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1]))%10)
								{
									iBigValue = GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1])%10;
									bcMakeMax[0]= cbCardDataSort[n];
									bcMakeMax[1]= cbCardDataSort[j];
									bcMakeMax[2]= cbCardDataSort[w]; 
									bcMakeMax[3]= iSingleA[0]; 
									bcMakeMax[4]= iSingleA[1]; 

									if(bcOutCadData != NULL)
									{
										memcpy(bcOutCadData,bcMakeMax,cbCardCount);
									}
									if(iBigValue==0)
									{
										return CT_SPECIAL_NIUNIU;                                        
									}
								}

							}

						}
					}
				}
			}		
		}
		if(iBigValue!=0)
		{
			return RetType(iBigValue);
		}	
		else
		{
			return CT_POINT;
		}
	}
	return CT_POINT;
}

//大小比较
/*
cbNextCardData>cbFirstCardData  返回1
cbNextCardData<cbFirstCardData  返回-1
cbNextCardData==cbFirstCardData 返回0
*/
//Multiple 比较出来的倍数
int CRobNiuLogic::CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount,const BYTE cbNextCardData[], BYTE cbNextCardCount,BYTE &Multiple)
{
	//合法判断
	ASSERT(ROBNIU_CARD_NUM ==cbFirstCardCount && ROBNIU_CARD_NUM ==cbNextCardCount);
	if (!(ROBNIU_CARD_NUM == cbFirstCardCount && ROBNIU_CARD_NUM == cbNextCardCount))
	{
		return 0;
	}
	//													 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14
	static int32 s_Multiple[CT_SPECIAL_BOMEBOME + 1] = { 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 4, 4, 4 };

	//获取牌型
	BYTE cbFirstCardType=GetCardType(cbFirstCardData, cbFirstCardCount);
	BYTE cbNextCardType=GetCardType(cbNextCardData, cbNextCardCount);

    Multiple = s_Multiple[cbFirstCardType];
	//牌型比较
	if(cbFirstCardType != cbNextCardType) 
	{
		if(cbNextCardType > cbFirstCardType)
		{
            Multiple = s_Multiple[cbNextCardType];
			return 1;
		}
		else
		{
            Multiple = s_Multiple[cbFirstCardType];
			return -1;
		}
	}

	//特殊牌型判断
	if(CT_POINT != cbFirstCardType && cbFirstCardType == cbNextCardType)
	{
		//排序扑克
		BYTE cbFirstCardDataTmp[CARD_COUNT], cbNextCardDataTmp[CARD_COUNT];

		memcpy(cbFirstCardDataTmp,cbFirstCardData,sizeof(BYTE)*cbFirstCardCount);
		memcpy(cbNextCardDataTmp,cbNextCardData,sizeof(BYTE)*cbNextCardCount);
		SortCardList(cbFirstCardDataTmp,cbFirstCardCount,ST_NEW);
		SortCardList(cbNextCardDataTmp,cbNextCardCount,ST_NEW);

        Multiple = s_Multiple[cbFirstCardType];
            
		BYTE firstValue = GetCardNewValue(cbFirstCardDataTmp[0]);
		BYTE NextValue = GetCardNewValue(cbNextCardDataTmp[0]);

		if (cbFirstCardType == CT_SPECIAL_BOMEBOME)
		{
			firstValue = GetCardNewValue(cbFirstCardDataTmp[2]);
			NextValue = GetCardNewValue(cbNextCardDataTmp[2]);

			if (firstValue < NextValue)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}

		BYTE firstColor = GetCardColor(cbFirstCardDataTmp[0]);
		BYTE NextColor = GetCardColor(cbNextCardDataTmp[0]);
		if(firstValue<NextValue)
		{
			return 1;
		}else
		{
			if(firstValue==NextValue)
			{
				if(firstColor<NextColor)
				{
					return 1;

				}else
				{
					return -1;
				}
			}
			return -1;
		}
	}

	//排序扑克
	BYTE cbFirstCardDataTmp[CARD_COUNT], cbNextCardDataTmp[CARD_COUNT];
	memcpy(cbFirstCardDataTmp,cbFirstCardData,sizeof(BYTE)*cbFirstCardCount);
	memcpy(cbNextCardDataTmp,cbNextCardData,sizeof(BYTE)*cbNextCardCount);
	SortCardList(cbFirstCardDataTmp,cbFirstCardCount,ST_NEW);
	SortCardList(cbNextCardDataTmp,cbNextCardCount,ST_NEW);

	BYTE firstValue = GetCardNewValue(cbFirstCardDataTmp[0]);
	BYTE NextValue = GetCardNewValue(cbNextCardDataTmp[0]);
	BYTE firstColor = GetCardColor(cbFirstCardDataTmp[0]);
	BYTE NextColor = GetCardColor(cbNextCardDataTmp[0]);

	if(firstValue<NextValue)
	{
		return 1;
	}else
	{
		if(firstValue==NextValue)
		{
			if(firstColor<NextColor)
			{
				return 1;

			}else
			{
				return -1;
			}
		}
		return -1;
	}
}

//获取牌点
BYTE CRobNiuLogic::GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount)
{
	//变量定义
	BYTE cbPipCount=0;

	//获取牌点
	BYTE cbCardValue=0;
	for (BYTE i=0;i<cbCardCount;i++)
	{
		cbCardValue=GetCardValue(cbCardData[i]);
		if(cbCardValue>10)
		{
			cbCardValue = 10;

		}
		cbPipCount+=cbCardValue;
	}
	return (cbPipCount%10);
}
BYTE CRobNiuLogic::GetCardNewValue(BYTE cbCardData)
{
	//扑克属性
	BYTE cbCardColor=GetCardColor(cbCardData);
	BYTE cbCardValue=GetCardValue(cbCardData);

	//转换数值
	if (cbCardColor==0x04) return cbCardValue+13+2;
	return cbCardValue;

}
//逻辑大小
BYTE CRobNiuLogic::GetCardLogicValue(BYTE cbCardData)
{
	BYTE cbValue=GetCardValue(cbCardData);

	//获取花色
	BYTE cbColor=GetCardColor(cbCardData);

	if(cbValue>10)
	{
		cbValue = 10;
	}
	if(cbColor==0x4)
	{
		return 11;
	}
	return cbValue;
}

//排列扑克
void CRobNiuLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType)
{
	//数目过虑
	if (cbCardCount==0) return;

	//转换数值
	BYTE cbSortValue[CARD_COUNT];
	if (ST_VALUE==cbSortType)
	{
		for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardValue(cbCardData[i]);	
	}
	else 
	{
		if(cbSortType==ST_NEW)
		{
			for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardNewValue(cbCardData[i]);	

		}else
		{
			for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);	

		}

	}


	//排序操作
	bool bSorted=true;
	BYTE cbThreeCount,cbLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (BYTE i=0;i<cbLast;i++)
		{
			if ((cbSortValue[i]<cbSortValue[i+1])||
				((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//交换位置
				cbThreeCount=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbThreeCount;
				cbThreeCount=cbSortValue[i];
				cbSortValue[i]=cbSortValue[i+1];
				cbSortValue[i+1]=cbThreeCount;
				bSorted=false;
			}	
		}
		cbLast--;
	} while(bSorted==false);

	return;
}

bool CRobNiuLogic::GetSubDataCard(BYTE cbSubCardData[][ROBNIU_CARD_NUM], vector<BYTE> & vecRemainCardData)
{
	vecRemainCardData.clear();
	BYTE cbCardListData[CARD_COUNT] = { 0 };
	memcpy(cbCardListData, m_cbCardListData, sizeof(cbCardListData));

	//打乱手牌
	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		cbPosition = g_RandGen.RandRange(0, CARD_COUNT - cbRandCount - 1);
		BYTE cbTempSwapCardData = cbCardListData[cbPosition];
		cbCardListData[cbPosition] = cbCardListData[CARD_COUNT - cbRandCount - 1];
		cbCardListData[CARD_COUNT - cbRandCount - 1] = cbTempSwapCardData;

		cbRandCount++;
	} while (cbRandCount<CARD_COUNT);


	for (int i = 0; i < CARD_COUNT; i++)
	{
		BYTE cbTempCardData = cbCardListData[i];
		bool bIsInSubCard = false;
		for (int i = 0; i < GAME_PLAYER; i++)
		{
			for (int j = 0; j < ROBNIU_CARD_NUM; j++)
			{
				if (cbSubCardData[i][j] == cbTempCardData)
				{
					bIsInSubCard = true;
					break;
				}
			}
			if (bIsInSubCard)
			{
				break;
			}
		}
		if (bIsInSubCard == false)
		{
			vecRemainCardData.push_back(cbCardListData[i]);
		}
	}

	return true;
}
//有效判断
bool CRobNiuLogic::IsValidCard(BYTE cbCardData)
{
	for (int i = 0; i < CARD_COUNT; i++)
	{
		if (m_cbCardListData[i] == cbCardData)
		{
			return true;
		}
	}
	return false;
}

bool CRobNiuLogic::GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][ROBNIU_CARD_NUM])
{
	int iArTempProCardType[GAME_PLAYER] = { 0 };
	for (int i = 0; i < GAME_PLAYER; i++)
	{
		iArTempProCardType[i] = iArProCardType[i];
	}

	BYTE cbArTempCardData[GAME_PLAYER][ROBNIU_CARD_NUM] = { 0 };
	memset(cbArTempCardData, 0, sizeof(cbArTempCardData));

	BYTE cbCardListData[CARD_COUNT] = { 0 };
	memcpy(cbCardListData, m_cbCardListData, sizeof(m_cbCardListData));

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		// 没有人的座位先别获取扑克牌
		if (iArTempProCardType[i] == CT_ERROR)
		{
			continue;
		}

		bool bIsFlag = false;
		if (iArTempProCardType[i] == CT_POINT)
		{
			bIsFlag = GetTypePoint(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU1)
		{
			bIsFlag = GetTypeNiuOne(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU2)
		{
			bIsFlag = GetTypeNiuTwo(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU3)
		{
			bIsFlag = GetTypeNiuThree(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU4)
		{
			bIsFlag = GetTypeNiuFour(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU5)
		{
			bIsFlag = GetTypeNiuFive(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU6)
		{
			bIsFlag = GetTypeNiuSix(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU7)
		{
			bIsFlag = GetTypeNiuSeven(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU8)
		{
			bIsFlag = GetTypeNiuEight(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIU9)
		{
			bIsFlag = GetTypeNiuNine(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIUNIU)
		{
			bIsFlag = GetTypeNiuNiu(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIUNIUDW)
		{
			bIsFlag = GetTypeNiuBig_5(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_NIUNIUXW)
		{
			bIsFlag = GetTypeNiuSmall_5(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}
		else if (iArTempProCardType[i] == CT_SPECIAL_BOMEBOME)
		{
			bIsFlag = GetTypeBome(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
		}

		//LOG_DEBUG("随机获取牌 1 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i,bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

		// 扑克检测
		if (bIsFlag)
		{
			// 检查手牌是否正确
			bool bHandCardIsRepeat = false;
			for (int p0 = 0; p0 < ROBNIU_CARD_NUM; p0++)
			{
				for (int p1 = 0; p1 < ROBNIU_CARD_NUM; p1++)
				{
					if (p0!=p1)
					{
						if(cbArTempCardData[p0]== cbArTempCardData[p1])
						{
							bHandCardIsRepeat = true;
							break;
						}
					}
				}
				if (bHandCardIsRepeat)
				{
					break;
				}
			}
			// 删除已经获取的牌值
			if (bHandCardIsRepeat == false)
			{
				for (int c1 = 0; c1 < ROBNIU_CARD_NUM; c1++)
				{
					BYTE cbCard = cbArTempCardData[i][c1];
					for (int c2 = 0; c2 < CARD_COUNT; c2++)
					{
						if (cbCard == cbCardListData[c2])
						{
							cbCardListData[c2] = 0;
							break;
						}
					}
				}
			}
			else
			{
				iArTempProCardType[i] = CT_SPECIAL_MAX_TYPE;
				// error
			}
		}
		else
		{
			iArTempProCardType[i] = CT_SPECIAL_MAX_TYPE;
			// error
		}

	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		// 最后把没有人的座位也获取扑克牌
		if (iArTempProCardType[i] == CT_SPECIAL_MAX_TYPE)// || iArTempProCardType[i] == CT_ERROR)
		{
			bool bIsFlag = GetTypeRand(cbCardListData, CARD_COUNT, cbArTempCardData[i], ROBNIU_CARD_NUM);
			//LOG_DEBUG("随机获取牌 2 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i, bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

			// 删除已经获取的牌值
			for (int c1 = 0; c1 < ROBNIU_CARD_NUM; c1++)
			{
				BYTE cbCard = cbArTempCardData[i][c1];
				for (int c2 = 0; c2 < CARD_COUNT; c2++)
				{
					if (cbCard == cbCardListData[c2])
					{
						cbCardListData[c2] = 0;
						break;
					}
				}
			}
		}
	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		for (int j = 0; j < ROBNIU_CARD_NUM; j++)
		{
			cbArCardData[i][j] = cbArTempCardData[i][j];
		}
	}
	return true;
}

//获取点数类型
bool CRobNiuLogic::GetTypePoint(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	int iLoopIndex = 0;
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = cbCardCount;
	do
	{
		memset(cbTargetCard, 0, sizeof(cbTargetCard));
		for (int i = 0; i < cbTargetCount; i++)
		{
			cbTargetCard[i] = cbRemainCardData[i];
		}
		BYTE cbCardType = GetCardType(cbTargetCard, cbTargetCount);
		if (cbCardType == CT_POINT)
		{
			break;
		}
		bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
		if (bIsUpsetRemain == false)
		{
			return false;
		}
		iLoopIndex++;
	} while (iLoopIndex < MAX_RAND_LOOP_COUNT);

	//输出手牌
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

/*
// 十组合
 1 + 1  + 8
 1 + 2  + 7
 1 + 3  + 6
 1 + 4  + 5
 1 + 5  + 4
 1 + 6  + 3
 1 + 7  + 2
 1 + 8  + 1
 1 + 9  + 10
 1 + 10 + 9

 2 + 2  + 6
 2 + 3  + 5
 2 + 4  + 4	
 2 + 5  + 3
 2 + 6  + 2
 2 + 7  + 1
 2 + 8  + 10
 2 + 9  + 9
 2 + 10 + 8

 3 + 3  + 4
 3 + 4  + 3
 3 + 5  + 2
 3 + 6  + 1
 3 + 7  + 10
 3 + 8  + 9
 3 + 9  + 8
 3 + 10 + 7

 4 + 4  + 6
 4 + 5  + 1
 4 + 6  + 10
 4 + 7  + 9
 4 + 8  + 8
 4 + 9  + 7
 4 + 10 + 6

 5 + 5  + 10
 5 + 6  + 9
 5 + 7  + 8
 5 + 8  + 7
 5 + 9  + 6
 5 + 10 + 5

 6 + 6  + 8
 6 + 7  + 7
 6 + 8  + 6
 6 + 9  + 5
 6 + 10 + 4

 7 + 7  + 6
 7 + 8  + 5
 7 + 9  + 4
 7 + 10 + 3

 8 + 8  + 4
 8 + 9  + 3
 8 + 10 + 2

 9 +  9 + 2
 9 + 10 + 1

 10 + 10 + 10

 // 去重复

 10 + 10 + 10


 1 + 1  + 8
 1 + 2  + 7
 1 + 3  + 6
 1 + 4  + 5
 1 + 9  + 10

 2 + 2  + 6
 2 + 3  + 5
 2 + 4  + 4
 2 + 8  + 10
 2 + 9  + 9

 3 + 3  + 4
 3 + 7  + 10
 3 + 8  + 9

 4 + 4  + 6
 4 + 6  + 10
 4 + 7  + 9
 4 + 8  + 8

 5 + 5  + 10
 5 + 6  + 9
 5 + 7  + 8

 6 + 6  + 8
 6 + 7  + 7

//7

//8

//9

*/


bool CRobNiuLogic::GetTripleCardDataHasTenTimes(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 获取剩余扑克
	std::vector<BYTE> vecRemainCardData[MAX_CARD_VALUE];
	BYTE cbRemainCardValue[MAX_CARD_VALUE] = { 0 };
	BYTE cbRemainCardCount = 0;
	vector<BYTE> vecOverTenCardData;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbTempCardValue = GetCardValue(cbCardListData[i]);
			if (cbTempCardValue > 0 && cbTempCardValue < MAX_CARD_VALUE)
			{
				cbRemainCardCount++;
				cbRemainCardValue[cbTempCardValue]++;
				vecRemainCardData[cbTempCardValue].push_back(cbCardListData[i]);
				if (cbTempCardValue >= 10 && cbTempCardValue < MAX_CARD_VALUE)
				{
					vecOverTenCardData.push_back(cbTempCardValue);
				}
			}
		}
	}

	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}

	vector<tagTripleCardDataHasTenTimes> vecCardDataHasTenTimes;

	
	tagTripleCardDataHasTenTimes data;
	
	//  1 + 1 + 8
	//	1 + 2 + 7
	//	1 + 3 + 6
	//	1 + 4 + 5
	//	1 + 9 + 10	
	data.cbCardDataOne = 1;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		data.cbCardDataTwo = 1;
		data.cbCardDataThree = 8;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 2;
		data.cbCardDataThree = 7;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 3;
		data.cbCardDataThree = 6;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 4;
		data.cbCardDataThree = 5;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 9;
		data.cbCardDataThree = 10;
		vecTryCardData.push_back(data);

		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}
	
	//  2 + 2 + 6
	//	2 + 3 + 5
	//	2 + 4 + 4
	//	2 + 8 + 10
	//	2 + 9 + 9
	data.cbCardDataOne = 2;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		data.cbCardDataTwo = 2;
		data.cbCardDataThree = 6;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 3;
		data.cbCardDataThree = 5;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 4;
		data.cbCardDataThree = 4;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 8;
		data.cbCardDataThree = 10;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 9;
		data.cbCardDataThree = 9;
		vecTryCardData.push_back(data);

		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}

	//  3 + 3 + 4
	//	3 + 7 + 10
	//	3 + 8 + 9
	data.cbCardDataOne = 3;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		data.cbCardDataTwo = 3;
		data.cbCardDataThree = 4;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 7;
		data.cbCardDataThree = 10;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 8;
		data.cbCardDataThree = 9;
		vecTryCardData.push_back(data);

		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}

	//  4 + 4 + 6
	//	4 + 6 + 10
	//	4 + 7 + 9
	//	4 + 8 + 8

	data.cbCardDataOne = 4;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		//data.cbCardDataTwo = 4;
		//data.cbCardDataThree = 6;
		//vecTryCardData.push_back(data);
		data.cbCardDataTwo = 6;
		data.cbCardDataThree = 10;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 7;
		data.cbCardDataThree = 9;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 8;
		data.cbCardDataThree = 8;
		vecTryCardData.push_back(data);
		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}

	//	5 + 5 + 10
	//	5 + 6 + 9
	//	5 + 7 + 8
	data.cbCardDataOne = 5;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		data.cbCardDataTwo = 5;
		data.cbCardDataThree = 10;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 6;
		data.cbCardDataThree = 9;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 7;
		data.cbCardDataThree = 8;
		vecTryCardData.push_back(data);
		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}

	//	6 + 6 + 8
	//	6 + 7 + 7
	data.cbCardDataOne = 6;
	if (cbRemainCardValue[data.cbCardDataOne] > 0)
	{
		vector<tagTripleCardDataHasTenTimes> vecTryCardData;
		data.cbCardDataTwo = 6;
		data.cbCardDataThree = 8;
		vecTryCardData.push_back(data);
		data.cbCardDataTwo = 7;
		data.cbCardDataThree = 7;
		vecTryCardData.push_back(data);
		for (uint32 iTryIndex = 0; iTryIndex < vecTryCardData.size(); iTryIndex++)
		{
			tagTripleCardDataHasTenTimes tempData = vecTryCardData[iTryIndex];

			if (data.cbCardDataOne > 0 && data.cbCardDataOne < MAX_CARD_VALUE&& tempData.cbCardDataTwo > 0 && tempData.cbCardDataTwo < MAX_CARD_VALUE  && tempData.cbCardDataThree > 0 && tempData.cbCardDataThree < MAX_CARD_VALUE)
			{
				if (tempData.cbCardDataOne == tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataTwo == tempData.cbCardDataThree && tempData.cbCardDataTwo != tempData.cbCardDataOne)
				{
					if (cbRemainCardValue[tempData.cbCardDataTwo] >= 2 && cbRemainCardValue[tempData.cbCardDataOne] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne == tempData.cbCardDataThree && tempData.cbCardDataOne != tempData.cbCardDataTwo)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 2 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else if (tempData.cbCardDataOne != tempData.cbCardDataTwo && tempData.cbCardDataOne != tempData.cbCardDataThree)
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
				else
				{
					if (cbRemainCardValue[tempData.cbCardDataOne] >= 1 && cbRemainCardValue[tempData.cbCardDataTwo] >= 1 && cbRemainCardValue[tempData.cbCardDataThree] >= 1)
					{
						vecCardDataHasTenTimes.push_back(tempData);
					}
				}
			}
		}
	}

	// 10 + 10 + 10
	
	if (vecOverTenCardData.size() >= 3)
	{
		uint32 uLoopTenCount = vecOverTenCardData.size() / 3;
		for (uint32 uLoopIndex = 0; uLoopIndex < uLoopTenCount; uLoopIndex++)
		{
			if (vecOverTenCardData.size() >= 3)
			{
				data.cbCardDataOne = vecOverTenCardData[0];
				data.cbCardDataTwo = vecOverTenCardData[1];
				data.cbCardDataThree = vecOverTenCardData[2];

				vecOverTenCardData.erase(vecOverTenCardData.begin());
				vecOverTenCardData.erase(vecOverTenCardData.begin());
				vecOverTenCardData.erase(vecOverTenCardData.begin());

				vecCardDataHasTenTimes.push_back(data);
			}
		}
	}

	if (vecCardDataHasTenTimes.size() == 0)
	{
		return false;
	}

	int iRandTenIndex = g_RandGen.RandRange(0, vecCardDataHasTenTimes.size() - 1);

	tagTripleCardDataHasTenTimes & realData = vecCardDataHasTenTimes[iRandTenIndex];
	BYTE cbHasTenTimesCardData[3] = { 0 };

	if (realData.cbCardDataOne > 0 && realData.cbCardDataOne < MAX_CARD_VALUE)
	{
		if (vecRemainCardData[realData.cbCardDataOne].size() > 0)
		{
			int iRandTenIndex = g_RandGen.RandRange(0, vecRemainCardData[realData.cbCardDataOne].size() - 1);
			BYTE cbRealCardData = vecRemainCardData[realData.cbCardDataOne][iRandTenIndex];
			cbHasTenTimesCardData[0] = cbRealCardData;
			//
			auto iter_find = std::find(vecRemainCardData[realData.cbCardDataOne].begin(), vecRemainCardData[realData.cbCardDataOne].end(), cbRealCardData);
			if (iter_find != vecRemainCardData[realData.cbCardDataOne].end())
			{
				vecRemainCardData[realData.cbCardDataOne].erase(iter_find);
			}
		}
	}
	
	if (realData.cbCardDataTwo > 0 && realData.cbCardDataTwo < MAX_CARD_VALUE)
	{
		if (vecRemainCardData[realData.cbCardDataTwo].size() > 0)
		{
			int iRandTenIndex = g_RandGen.RandRange(0, vecRemainCardData[realData.cbCardDataTwo].size() - 1);
			BYTE cbRealCardData = vecRemainCardData[realData.cbCardDataTwo][iRandTenIndex];
			cbHasTenTimesCardData[1] = cbRealCardData;
			auto iter_find = std::find(vecRemainCardData[realData.cbCardDataTwo].begin(), vecRemainCardData[realData.cbCardDataTwo].end(), cbRealCardData);
			if (iter_find != vecRemainCardData[realData.cbCardDataTwo].end())
			{
				vecRemainCardData[realData.cbCardDataTwo].erase(iter_find);
			}
		}
	}

	if (realData.cbCardDataThree > 0 && realData.cbCardDataThree < MAX_CARD_VALUE)
	{
		if (vecRemainCardData[realData.cbCardDataThree].size() > 0)
		{
			int iRandTenIndex = g_RandGen.RandRange(0, vecRemainCardData[realData.cbCardDataThree].size() - 1);
			BYTE cbRealCardData = vecRemainCardData[realData.cbCardDataThree][iRandTenIndex];
			cbHasTenTimesCardData[2] = cbRealCardData;
			auto iter_find = std::find(vecRemainCardData[realData.cbCardDataThree].begin(), vecRemainCardData[realData.cbCardDataThree].end(), cbRealCardData);
			if (iter_find != vecRemainCardData[realData.cbCardDataThree].end())
			{
				vecRemainCardData[realData.cbCardDataThree].erase(iter_find);
			}
		}
	}

	for (BYTE i = 0; i < 3; i++)
	{
		if (IsValidCard(cbHasTenTimesCardData[i]) == false)
		{
			return false;
		}
	}

	for (BYTE i = 0; i < 3; i++)
	{
		cbCardData[i] = cbHasTenTimesCardData[i];
	}

	return true;
}

//获取牛一
bool CRobNiuLogic::GetTypeNiuOne(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	// 有可能有 1 的最后两个扑克牌
	// 2-9
	// 3-8
	// 4-7
	// 5-6

	// 1

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}	
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			// 2 3 4 5
			if (cbCardValue == 2)
			{
				vecAllDigitCardSmall[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 3)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 4)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 5)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 9 8 7 6
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 6)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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




//获取牛二
bool CRobNiuLogic::GetTypeNiuTwo(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//3-9
	//4-8
	//5-7

	//1-1 6-6

	// 2

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[3];
	vector<BYTE> vecAllDigitCardBig[3];
	vector<BYTE> vecAllDigitCardMid;
	vector<BYTE> vecAllDigitCardBeyond;

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			if (cbCardValue == 1) // 1
			{
				vecAllDigitCardMid.push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 6) // 6
			{
				vecAllDigitCardBeyond.push_back(cbTempCardData[i]);
			}

			// 3 4 5
			if (cbCardValue == 3)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 4)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 5)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 9 8 7
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 3; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardMid.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardMid[0];
			tempPairCard.second = vecAllDigitCardMid[1];
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardBeyond.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardBeyond[0];
			tempPairCard.second = vecAllDigitCardBeyond[1];
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛三
bool CRobNiuLogic::GetTypeNiuThree(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-2
	//4-9
	//5-8
	//6-7

	// 3


	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			// 1 4 5 6
			if (cbCardValue == 1)
			{
				vecAllDigitCardSmall[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 4)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 5)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 6)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 2 9 8 7
			if (cbCardValue == 2)
			{
				vecAllDigitCardBig[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛四
bool CRobNiuLogic::GetTypeNiuFour(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-3
	//5-9
	//6-8

	//2-2 7-7

	// 4

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[3];
	vector<BYTE> vecAllDigitCardBig[3];
	vector<BYTE> vecAllDigitCardMid;
	vector<BYTE> vecAllDigitCardBeyond;

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			if (cbCardValue == 2) // 2
			{
				vecAllDigitCardMid.push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7) // 7
			{
				vecAllDigitCardBeyond.push_back(cbTempCardData[i]);
			}

			// 1 5 6
			if (cbCardValue == 1)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 5)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 6)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 3 9 8
			if (cbCardValue == 3)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 3; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardMid.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardMid[0];
			tempPairCard.second = vecAllDigitCardMid[1];
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardBeyond.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardBeyond[0];
			tempPairCard.second = vecAllDigitCardBeyond[1];
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛五
bool CRobNiuLogic::GetTypeNiuFive(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-4
	//2-3
	//6-9
	//7-8

	// 5
	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			// 1 2 6 7
			if (cbCardValue == 1)
			{
				vecAllDigitCardSmall[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 2)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 6)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 4 3 9 8
			if (cbCardValue == 4)
			{
				vecAllDigitCardBig[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 3)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛六
bool CRobNiuLogic::GetTypeNiuSix(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-5
	//2-4
	//7-9

	//3-3 8-8

	// 6

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[3];
	vector<BYTE> vecAllDigitCardBig[3];
	vector<BYTE> vecAllDigitCardMid;
	vector<BYTE> vecAllDigitCardBeyond;

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			if (cbCardValue == 3) // 3
			{
				vecAllDigitCardMid.push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8) // 8
			{
				vecAllDigitCardBeyond.push_back(cbTempCardData[i]);
			}

			// 1 2 7
			if (cbCardValue == 1)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 2)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 7)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 5 4 9
			if (cbCardValue == 5)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 4)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 3; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardMid.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardMid[0];
			tempPairCard.second = vecAllDigitCardMid[1];
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardBeyond.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardBeyond[0];
			tempPairCard.second = vecAllDigitCardBeyond[1];
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛七
bool CRobNiuLogic::GetTypeNiuSeven(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-6
	//2-5
	//3-4
	//8-9

	// 7

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			// 1 2 3 8
			if (cbCardValue == 1)
			{
				vecAllDigitCardSmall[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 2)
			{
				vecAllDigitCardSmall[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 3)
			{
				vecAllDigitCardSmall[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 8)
			{
				vecAllDigitCardSmall[0].push_back(cbTempCardData[i]);
			}

			// 6 5 4 9
			if (cbCardValue == 6)
			{
				vecAllDigitCardBig[3].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 5)
			{
				vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 4)
			{
				vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9)
			{
				vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛八
bool CRobNiuLogic::GetTypeNiuEight(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-7
	//2-6
	//3-5

	//4-4 9-9

	//8

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[3];
	vector<BYTE> vecAllDigitCardBig[3];
	vector<BYTE> vecAllDigitCardMid;
	vector<BYTE> vecAllDigitCardBeyond;

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			if (cbCardValue == 4) // 4
			{
				vecAllDigitCardMid.push_back(cbTempCardData[i]);
			}
			if (cbCardValue == 9) // 9
			{
				vecAllDigitCardBeyond.push_back(cbTempCardData[i]);
			}
			if (cbCardValue > 0 && cbCardValue < 4)  // 1 2 3
			{
				vecAllDigitCardSmall[cbCardValue - 1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue > 4 && cbCardValue <= 7) // 7 6 5
			{
				if (cbCardValue == 5)
				{
					vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
				}
				if (cbCardValue == 6)
				{
					vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
				}
				if (cbCardValue == 7)
				{
					vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
				}
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 3; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardMid.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardMid[0];
			tempPairCard.second = vecAllDigitCardMid[1];
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardBeyond.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardBeyond[0];
			tempPairCard.second = vecAllDigitCardBeyond[1];
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardBeyond.erase(vecAllDigitCardBeyond.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}

	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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
//获取牛九
bool CRobNiuLogic::GetTypeNiuNine(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	//1-8
	//2-7
	//3-6
	//4-5

	// 9

	// 统计剩余扑克
	BYTE cbRemainCardCount = 0;
	BYTE cbRemainCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			cbRemainCardData[cbRemainCardCount++] = cbCardListData[i];
		}
	}
	if (cbRemainCardCount < cbCardCount)
	{
		return false;
	}
	// 打乱剩余扑克
	bool bIsUpsetRemain = RandUpsetCardData(cbRemainCardData, cbRemainCardCount);
	if (bIsUpsetRemain == false)
	{
		return false;
	}
	// 获取三个扑克为十的倍数
	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 3;
	bool bIsCardDataHasTenTimes = GetTripleCardDataHasTenTimes(cbRemainCardData, cbRemainCardCount, cbTargetCard, cbTargetCount);
	if (bIsCardDataHasTenTimes == false)
	{
		return false;
	}
	// 去除十的倍数扑克
	BYTE cbTempCardCount = 0;
	BYTE cbTempCardData[CARD_COUNT] = { 0 };
	for (BYTE i = 0; i < cbListCount; i++)
	{
		bool bIsExistTenTimes = false;
		for (int j = 0; j < cbTargetCount; j++)
		{
			if (cbTargetCard[j] == cbRemainCardData[i])
			{
				bIsExistTenTimes = true;
				break;
			}
		}
		if (bIsExistTenTimes == false && IsValidCard(cbRemainCardData[i]))
		{
			cbTempCardData[cbTempCardCount++] = cbRemainCardData[i];
		}
	}
	if (cbTempCardCount + cbTargetCount < cbCardCount)
	{
		return false;
	}
	//数值分布
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];

	// 获取数值
	for (BYTE i = 0; i < cbTempCardCount; i++)
	{
		if (IsValidCard(cbTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbTempCardData[i]);

			if (cbCardValue > 0 && cbCardValue < 5)  // 1 2 3 4
			{
				vecAllDigitCardSmall[cbCardValue - 1].push_back(cbTempCardData[i]);
			}
			if (cbCardValue >= 5 && cbCardValue < 9) // 8 7 6 5
			{
				if (cbCardValue == 5)
				{
					vecAllDigitCardBig[3].push_back(cbTempCardData[i]);
				}
				if (cbCardValue == 6)
				{
					vecAllDigitCardBig[2].push_back(cbTempCardData[i]);
				}
				if (cbCardValue == 7)
				{
					vecAllDigitCardBig[1].push_back(cbTempCardData[i]);
				}
				if (cbCardValue == 8)
				{
					vecAllDigitCardBig[0].push_back(cbTempCardData[i]);
				}
			}
		}
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];

			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造具体牛数手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	bool bIsUpsetTarget = RandUpsetCardData(cbTargetCard, cbTargetCount);
	if (bIsUpsetTarget == false)
	{
		return false;
	}
	//输出手牌
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

//获取牛牛
bool CRobNiuLogic::GetTypeNiuNiu(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[CARD_COUNT] = { 0 };
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
	// 打乱剩余扑克
	BYTE cbRandCount = 0, cbRandPosition = 0;
	do
	{
		cbRandPosition = g_RandGen.RandRange(0, cbTempListCount - cbRandCount - 1);
		BYTE cbTempSwapCardData = cbArTempCardData[cbRandPosition];
		cbArTempCardData[cbRandPosition] = cbArTempCardData[cbTempListCount - cbRandCount - 1];
		cbArTempCardData[cbTempListCount - cbRandCount - 1] = cbTempSwapCardData;
		cbRandCount++;
	} while (cbRandCount<cbTempListCount);

	//1-9
	//2-8
	//3-7
	//4-6

	//5-5
	
	vector<BYTE> vecAllDigitCardSmall[4];
	vector<BYTE> vecAllDigitCardBig[4];
	vector<BYTE> vecAllDigitCardMid;

	BYTE cbArAllTargetData[CARD_COUNT] = { 0 };
	int iAllTargetCount = 0;
	// 获取数值
	for (BYTE i = 0; i < cbTempListCount; i++)
	{
		if (IsValidCard(cbArTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbArTempCardData[i]);
			if (cbCardValue >= 10 && cbCardValue < MAX_CARD_VALUE)
			{
				cbArAllTargetData[iAllTargetCount++] = cbArTempCardData[i];
			}
			if (cbCardValue == 5) // 5
			{
				vecAllDigitCardMid.push_back(cbArTempCardData[i]);
			}
			if (cbCardValue > 0 && cbCardValue < 5)  // 1 2 3 4
			{
				vecAllDigitCardSmall[cbCardValue - 1].push_back(cbArTempCardData[i]);
			}
			if (cbCardValue > 5 && cbCardValue <= 9) // 9 8 7 6
			{
				if (cbCardValue == 6)
				{
					vecAllDigitCardBig[3].push_back(cbArTempCardData[i]);
				}
				if (cbCardValue == 7)
				{
					vecAllDigitCardBig[2].push_back(cbArTempCardData[i]);
				}
				if (cbCardValue == 8)
				{
					vecAllDigitCardBig[1].push_back(cbArTempCardData[i]);
				}
				if (cbCardValue == 9)
				{
					vecAllDigitCardBig[0].push_back(cbArTempCardData[i]);
				}				
			}
		}
	}

	if (cbCardCount - 2 > iAllTargetCount)
	{
		return false;
	}
	//合并数值
	vector<pair<BYTE, BYTE>> vecAllDigitCardMerge;
	for (int i = 0; i < 4; i++)
	{
		if (vecAllDigitCardSmall[i].size()>0 && vecAllDigitCardBig[i].size() > 0)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardSmall[i][0];
			tempPairCard.second = vecAllDigitCardBig[i][0];
			
			vecAllDigitCardSmall[i].erase(vecAllDigitCardSmall[i].begin());
			vecAllDigitCardBig[i].erase(vecAllDigitCardBig[i].begin());

			vecAllDigitCardMerge.push_back(tempPairCard);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (vecAllDigitCardMid.size() >= 2)
		{
			pair<BYTE, BYTE> tempPairCard;
			tempPairCard.first = vecAllDigitCardMid[0];
			tempPairCard.second = vecAllDigitCardMid[1];
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMid.erase(vecAllDigitCardMid.begin());
			vecAllDigitCardMerge.push_back(tempPairCard);
		}
		else
		{
			break;
		}
	}
	
	if (vecAllDigitCardMerge.size() == 0)
	{
		return false;
	}
	//构造手牌
	BYTE cbRandMergeIndex = g_RandGen.RandRange(0, vecAllDigitCardMerge.size() - 1);

	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 0;

	for (BYTE i = 0; i < cbCardCount - 2; i++)
	{
		cbTargetCard[cbTargetCount++] = cbArAllTargetData[i];
	}
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].first;
	cbTargetCard[cbTargetCount++] = vecAllDigitCardMerge[cbRandMergeIndex].second;

	if (cbTargetCount != cbCardCount)
	{
		return false;
	}
	//打乱手牌
	BYTE cbRandTargetCount = 0, cbRandTargetPosition = 0;
	do
	{
		cbRandTargetPosition = g_RandGen.RandRange(0, cbTargetCount - cbRandTargetCount - 1);
		BYTE cbTempSwapCardData = cbTargetCard[cbRandTargetPosition];
		cbTargetCard[cbRandTargetPosition] = cbTargetCard[cbTargetCount - cbRandTargetCount - 1];
		cbTargetCard[cbTargetCount - cbRandTargetCount - 1] = cbTempSwapCardData;

		cbRandTargetCount++;
	} while (cbRandTargetCount<cbTargetCount);

	//输出手牌
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

//获取大五牛
bool CRobNiuLogic::GetTypeNiuBig_5(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[CARD_COUNT] = { 0 };
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
	
	BYTE cbArAllTargetData[CARD_COUNT] = { 0 };
	int iAllTargetCount = 0;

	for (BYTE i = 0; i < cbTempListCount; i++)
	{
		if (IsValidCard(cbArTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbArTempCardData[i]);
			if (cbCardValue > 10 && cbCardValue < MAX_CARD_VALUE)
			{
				cbArAllTargetData[iAllTargetCount++] = cbArTempCardData[i];
			}
		}
	}

	if (cbCardCount > iAllTargetCount)
	{
		return false;
	}

	BYTE cbRandCardBuffer[CARD_COUNT] = { 0 };
	BYTE cbRandCount = 0, cbRandPosition = 0;
	do
	{
		cbRandPosition = g_RandGen.RandRange(0, iAllTargetCount - cbRandCount - 1);
		cbRandCardBuffer[cbRandCount++] = cbArAllTargetData[cbRandPosition];
		cbArAllTargetData[cbRandPosition] = cbArAllTargetData[iAllTargetCount - cbRandCount];
	} while (cbRandCount<iAllTargetCount);

	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };

	for (BYTE i = 0; i < cbCardCount; i++)
	{
		cbTargetCard[i] = cbRandCardBuffer[i];
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
//获取小五牛
bool CRobNiuLogic::GetTypeNiuSmall_5(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[CARD_COUNT] = { 0 };
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

	BYTE cbArAllTargetData[CARD_COUNT] = { 0 };
	int iAllTargetCount = 0;

	for (BYTE i = 0; i < cbTempListCount; i++)
	{
		if (IsValidCard(cbArTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbArTempCardData[i]);
			if (cbCardValue > 0 && cbCardValue < 5)
			{
				cbArAllTargetData[iAllTargetCount++] = cbArTempCardData[i];
			}
		}
	}

	if (cbCardCount > iAllTargetCount)
	{
		return false;
	}

	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };	
	BYTE cbSumCardValue = 0;

	int iLoopIndex = 0;
	for (; iLoopIndex < MAX_RAND_LOOP_COUNT; iLoopIndex++)
	{
		cbSumCardValue = 0;
		BYTE cbRandCount = 0, cbRandPosition = 0;
		memset(cbTargetCard, 0, sizeof(cbTargetCard));
		BYTE cbCardBufferData[CARD_COUNT] = { 0 };
		memcpy(cbCardBufferData, cbArAllTargetData, sizeof(cbCardBufferData));
		do
		{
			cbRandPosition = g_RandGen.RandRange(0, iAllTargetCount - cbRandCount - 1);
			cbTargetCard[cbRandCount++] = cbCardBufferData[cbRandPosition];
			cbSumCardValue += GetCardValue(cbCardBufferData[cbRandPosition]);
			cbCardBufferData[cbRandPosition] = cbCardBufferData[iAllTargetCount - cbRandCount];
		} while (cbRandCount<cbCardCount);
		if (cbSumCardValue <= 10)
		{
			break;
		}
	}
	if (cbSumCardValue > 10)
	{
		return false;
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
//获取炸弹
bool CRobNiuLogic::GetTypeBome(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[CARD_COUNT] = { 0 };
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
	// 统计数值数目
	BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
	for (BYTE i = 0; i < cbTempListCount; i++)
	{
		if (IsValidCard(cbArTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbArTempCardData[i]);
			if (cbCardValue < MAX_CARD_VALUE)
			{
				cbArCardValueCount[cbCardValue]++;
			}
		}
	}

	BYTE cbArBomeAll[MAX_CARD_VALUE] = { 0 };
	int iBomeAllCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] == 4)
		{
			cbArBomeAll[iBomeAllCount] = i;
			iBomeAllCount++;
		}
	}
	if (iBomeAllCount <= 0)
	{
		return false;
	}

	BYTE cbRandPosition = g_RandGen.RandRange(0, iBomeAllCount - 1);
	BYTE cbRandCardValue = cbArBomeAll[cbRandPosition];

	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };
	BYTE cbTargetCount = 0;

	BYTE cbArSubCardData[CARD_COUNT] = { 0 };
	BYTE cbSubCardCount = 0;

	for (BYTE i = 0; i < cbTempListCount; i++)
	{
		if (IsValidCard(cbArTempCardData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbArTempCardData[i]);
			if (cbCardValue == cbRandCardValue)
			{
				cbTargetCard[cbTargetCount++] = cbArTempCardData[i];				
			}
			else
			{
				cbArSubCardData[cbSubCardCount++] = cbArTempCardData[i];				
			}
		}
	}

	if (cbSubCardCount == 0)
	{
		return false;
	}

	BYTE cbRandLastPosition = g_RandGen.RandRange(0, cbSubCardCount - 1);
	
	cbTargetCard[cbTargetCount++] = cbArSubCardData[cbRandLastPosition];
	
	if (cbTargetCount != cbCardCount)
	{
		return false;
	}

	BYTE cbRandTargetCount = 0, cbRandTargetPosition = 0;
	do
	{
		cbRandTargetPosition = g_RandGen.RandRange(0, cbTargetCount - cbRandTargetCount - 1);
		BYTE cbTempSwapCardData = cbArTempCardData[cbRandTargetPosition];
		cbArTempCardData[cbRandTargetPosition] = cbArTempCardData[cbTargetCount - cbRandTargetCount - 1];
		cbArTempCardData[cbTargetCount - cbRandTargetCount - 1] = cbTempSwapCardData;

		cbRandTargetCount++;
	} while (cbRandTargetCount<cbTargetCount);


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

//获取随机牌
bool CRobNiuLogic::GetTypeRand(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
{
	BYTE cbTempListCount = 0;
	BYTE cbArTempCardData[CARD_COUNT] = { 0 };
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
	//混乱扑克
	BYTE cbRandCardBuffer[CARD_COUNT] = { 0 };
	BYTE cbRandCount = 0, cbRandPosition = 0;
	do
	{
		cbRandPosition = g_RandGen.RandRange(0, cbTempListCount - cbRandCount - 1);
		//cbRandPosition = g_RandGen.RandUInt() % (cbTempListCount - cbRandCount);
		cbRandCardBuffer[cbRandCount++] = cbArTempCardData[cbRandPosition];
		cbArTempCardData[cbRandPosition] = cbArTempCardData[cbTempListCount - cbRandCount];
	} while (cbRandCount<cbTempListCount);

	BYTE cbTargetCard[ROBNIU_CARD_NUM] = { 0 };

	for (BYTE i = 0; i < cbCardCount; i++)
	{
		cbTargetCard[i] = cbRandCardBuffer[i];
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