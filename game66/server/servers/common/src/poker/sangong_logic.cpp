//
// Created by toney on 16/4/12.
//
#include "sangong_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_sangong
{
//扑克数据
BYTE CSangongLogic::m_cbCardListData[52]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D	//黑桃 A - K
};

//////////////////////////////////////////////////////////////////////////
//构造函数
CSangongLogic::CSangongLogic()
{
}

//析构函数
CSangongLogic::~CSangongLogic()
{
}
//获取类型
	BYTE CSangongLogic::GetCardType(BYTE cbCardData[], BYTE cbCardCount)
	{
		ASSERT(cbCardCount==MAX_COUNT);

		//炸弹牌型
		BYTE bSameCount = 0;
		SortCardList(cbCardData,cbCardCount);
		if((GetCardValue(cbCardData[0]) == GetCardValue(cbCardData[2])))
		{
			if(GetCardValue(cbCardData[0]) > 10)
			{
				return OX_THREE_KING2;//大三公
			}else{
				return OX_THREE_KING1;//小三公
			}
		}
		BYTE bKingCount=0;
		for(BYTE i=0;i<cbCardCount;i++)
		{
			if((GetCardValue(cbCardData[i])>10))
			{
				bKingCount++;
			}
		}
		if(bKingCount==MAX_COUNT)
			return OX_THREE_KING0;//混三公


		BYTE bTemp[MAX_COUNT];
		BYTE bSum=0;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			bTemp[i]=GetCardLogicValue(cbCardData[i]);
			bSum+=bTemp[i];
		}

		return bSum%10;
	}

//获取倍数
	BYTE CSangongLogic::GetTimes(BYTE cbCardData[], BYTE cbCardCount)
	{
		if(cbCardCount!=MAX_COUNT)return 0;
		BYTE bCardType = GetCardType(cbCardData,MAX_COUNT);
		if(bCardType <= 6)return 1;
		else if(bCardType==7)return 2;
		else if(bCardType==8)return 3;
		else if(bCardType==9)return 4;
		else return 5;

		return 5;
	}

//获取牛牛
	bool CSangongLogic::GetOxCard(BYTE cbCardData[], BYTE cbCardCount)
	{
		ASSERT(cbCardCount==MAX_COUNT);

		//设置变量
		BYTE bTemp[MAX_COUNT],bTempData[MAX_COUNT];
		memcpy(bTempData,cbCardData,sizeof(bTempData));
		BYTE bSum=0;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			bTemp[i]=GetCardLogicValue(cbCardData[i]);
			bSum+=bTemp[i];
		}

		//查找牛牛
		for (BYTE i=0;i<cbCardCount-1;i++)
		{
			for (BYTE j=i+1;j<cbCardCount;j++)
			{
				if((bSum-bTemp[i]-bTemp[j])%10==0)
				{
					BYTE bCount=0;
					for (BYTE k=0;k<cbCardCount;k++)
					{
						if(k!=i && k!=j)
						{
							cbCardData[bCount++] = bTempData[k];
						}
					}ASSERT(bCount==3);

					cbCardData[bCount++] = bTempData[i];
					cbCardData[bCount++] = bTempData[j];

					return true;
				}
			}
		}

		return false;
	}

//获取整数
	bool CSangongLogic::IsIntValue(BYTE cbCardData[], BYTE cbCardCount)
	{
		BYTE sum=0;
		for(BYTE i=0;i<cbCardCount;i++)
		{
			sum+=GetCardLogicValue(cbCardData[i]);
		}
		ASSERT(sum>0);
		return (sum%10==0);
	}

//排列扑克
	void CSangongLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount)
	{
		//转换数值
		BYTE cbLogicValue[MAX_COUNT];
		for (BYTE i=0;i<cbCardCount;i++) cbLogicValue[i]=GetCardValue(cbCardData[i]);

		//排序操作
		bool bSorted=true;
		BYTE cbTempData,bLast=cbCardCount-1;
		do
		{
			bSorted=true;
			for (BYTE i=0;i<bLast;i++)
			{
				if ((cbLogicValue[i]<cbLogicValue[i+1])||
					((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
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
			bLast--;
		} while(bSorted==false);

		return;
	}

//混乱扑克
	void CSangongLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
	{
		//CopyMemory(cbCardBuffer,m_cbCardListData,cbBufferCount);
		//混乱准备
		BYTE cbCardData[sizeof(m_cbCardListData)];
		memcpy(cbCardData,m_cbCardListData,sizeof(m_cbCardListData));

		//混乱扑克
		BYTE bRandCount=0,bPosition=0;
		do
		{
			bPosition=rand()%(sizeof(m_cbCardListData)-bRandCount);
			cbCardBuffer[bRandCount++]=cbCardData[bPosition];
			cbCardData[bPosition]=cbCardData[sizeof(m_cbCardListData)-bRandCount];
		} while (bRandCount<cbBufferCount);

		return;
	}

//逻辑数值
	BYTE CSangongLogic::GetCardLogicValue(BYTE cbCardData)
	{
		//扑克属性
		BYTE bCardColor=GetCardColor(cbCardData);
		BYTE bCardValue=GetCardValue(cbCardData);

		//转换数值
		return (bCardValue>10)?(10):bCardValue;
	}

	bool CSangongLogic::CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount,BYTE& multiple)
	{
		//获取点数
		BYTE cbNextType=GetCardType(cbNextData,cbCardCount);
		BYTE cbFirstType=GetCardType(cbFirstData,cbCardCount);
		if(cbNextType != cbFirstType)
		{
			if(cbFirstType > cbNextType)
			{
				multiple = GetTimes(cbFirstData,cbCardCount);
				return true;
			}else{
				multiple = GetTimes(cbNextData,cbCardCount);
				return false;
			}
		}
		else
		{
			multiple = GetTimes(cbNextData,cbCardCount);
			if(GetCardValue(cbFirstData[2]) > GetCardValue(cbNextData[2])){
				return true;
			}else if(GetCardValue(cbFirstData[2]) < GetCardValue(cbNextData[2])){
				return false;
			}else{
				if(GetCardColor(cbFirstData[2]) > GetCardColor(cbNextData[2])){
					return true;
				}
				return false;
			}
		}
	}

};



