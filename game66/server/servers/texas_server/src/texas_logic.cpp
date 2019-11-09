//
// Created by toney on 16/4/12.
//
#include "texas_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_texas
{
//扑克数据
BYTE CTexasLogic::m_cbCardData[FULL_COUNT]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
};

//构造函数
CTexasLogic::CTexasLogic()
{
}

//析构函数
CTexasLogic::~CTexasLogic()
{
}

//混乱扑克
void CTexasLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	//测试代码
	//memcpy(cbCardBuffer,m_cbCardData,cbBufferCount);

	//混乱准备
	BYTE bCardCount = getArrayLen(m_cbCardData);
	BYTE cbCardData[getArrayLen(m_cbCardData)];
	memcpy(cbCardData,m_cbCardData,sizeof(m_cbCardData));

	//混乱扑克
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=g_RandGen.RandUInt()%(bCardCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[bCardCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	return;
}

//获取类型
BYTE CTexasLogic::GetCardType(BYTE ocbCardData[], BYTE cbCardCount)
{
	//数据校验
	ASSERT(cbCardCount == MAX_CENTERCOUNT);
	if(cbCardCount !=MAX_CENTERCOUNT) return 0;

	uint8 cbCardData[MAX_CENTERCOUNT]; // 不改变原始牌顺序，这里使用临时变量 add by har
	memcpy(cbCardData, ocbCardData, cbCardCount); // add by har
	SortCardList(cbCardData, cbCardCount);

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

	//最小同花顺
	if((bLineCard == false)&&(cbFirstValue == 14))
	{
		BYTE i=1;
		for (i=1;i<cbCardCount;i++)
		{
			BYTE cbLogicValue=GetCardLogicValue(cbCardData[i]);
			if ((cbFirstValue!=(cbLogicValue+i+8))) break;
		}
		if( i == cbCardCount)
			bLineCard =true;
	}

	//皇家同花顺
	if ((cbSameColor==true)&&(bLineCard==true)&&(GetCardLogicValue(cbCardData[1]) ==13 )) 
		return CT_KING_TONG_HUA_SHUN;

	//顺子类型
	if ((cbSameColor==false)&&(bLineCard==true)) 
		return CT_SHUN_ZI;

	//同花类型
	if ((cbSameColor==true)&&(bLineCard==false)) 
		return CT_TONG_HUA;

	//同花顺类型
	if ((cbSameColor==true)&&(bLineCard==true))
		return CT_TONG_HUA_SHUN;

	//扑克分析
	tagAnalyseResult AnalyseResult;
	AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

	//类型判断
	if (AnalyseResult.cbFourCount==1) 
		return CT_TIE_ZHI;
	if (AnalyseResult.cbLONGCount==2) 
		return CT_TWO_LONG;
	if ((AnalyseResult.cbLONGCount==1)&&(AnalyseResult.cbThreeCount==1))
		return CT_HU_LU;
	if ((AnalyseResult.cbThreeCount==1)&&(AnalyseResult.cbLONGCount==0))
		return CT_THREE_TIAO;
	if ((AnalyseResult.cbLONGCount==1)&&(AnalyseResult.cbSignedCount==3)) 
		return CT_ONE_LONG;

	return CT_SINGLE;
}

//最大牌型
BYTE CTexasLogic::FiveFromSeven(BYTE cbHandCardData[],BYTE cbHandCardCount,BYTE cbCenterCardData[],BYTE cbCenterCardCount,BYTE cbLastCardData[],BYTE cbLastCardCount)
{
	//临时变量
	BYTE cbTempCardData[MAX_COUNT+MAX_CENTERCOUNT],cbTempLastCardData[5];
	memset(cbTempCardData,0,sizeof(cbTempCardData));
	memset(cbTempLastCardData,0,sizeof(cbTempLastCardData));

	//拷贝数据
	memcpy(cbTempCardData,cbHandCardData,sizeof(BYTE)*MAX_COUNT);
	memcpy(&cbTempCardData[2],cbCenterCardData,sizeof(BYTE)*MAX_CENTERCOUNT);

	//排列扑克
	SortCardList(cbTempCardData,getArrayLen(cbTempCardData));

	memcpy(cbLastCardData,cbTempCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbCardKind = GetCardType(cbLastCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbTempCardKind = 0;

	//组合算法
	for (int i=0;i<3;i++)
	{
		for (int j= i+1;j<4;j++)
		{
			for (int k = j+1;k<5;k++)
			{
				for (int l =k+1;l<6;l++)
				{
					for (int m=l+1;m<7;m++)
					{
						//获取数据
						cbTempLastCardData[0]=cbTempCardData[i];
						cbTempLastCardData[1]=cbTempCardData[j];
						cbTempLastCardData[2]=cbTempCardData[k];
						cbTempLastCardData[3]=cbTempCardData[l];
						cbTempLastCardData[4]=cbTempCardData[m];

						//获取牌型
						cbTempCardKind = GetCardType(cbTempLastCardData,getArrayLen(cbTempLastCardData));

						//牌型大小
						if(CompareCard(cbTempLastCardData,cbLastCardData,getArrayLen(cbTempLastCardData))==2)
						{
							cbCardKind = cbTempCardKind;
							memcpy(cbLastCardData,cbTempLastCardData,sizeof(BYTE)*getArrayLen(cbTempLastCardData));
						}
					}
				}
			}
		}
	}

	return cbCardKind;
}
//获得当前牌型
BYTE CTexasLogic::GetCurCardType(BYTE cbHandCardData[],BYTE cbHandCardCount,BYTE cbCenterCardData[],BYTE cbCenterCardCount)
{
	//临时变量
	BYTE cbTempCardData[MAX_COUNT+MAX_CENTERCOUNT],cbTempLastCardData[5];
	memset(cbTempCardData,0,sizeof(cbTempCardData));
	memset(cbTempLastCardData,0,sizeof(cbTempLastCardData));

	//拷贝数据
	memcpy(cbTempCardData,cbHandCardData,sizeof(BYTE)*MAX_COUNT);
	memcpy(&cbTempCardData[2],cbCenterCardData,sizeof(BYTE)*cbCenterCardCount);

	//排列扑克
	SortCardList(cbTempCardData,getArrayLen(cbTempCardData));

	BYTE cbLastCardData[5];
	memcpy(cbLastCardData,cbTempCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbCardKind = GetCardType(cbLastCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbTempCardKind = 0;

	//组合算法
	for (int i=0;i<3;i++)
	{
		for (int j= i+1;j<4;j++)
		{
			for (int k = j+1;k<5;k++)
			{
				for (int l =k+1;l<6;l++)
				{
					for (int m=l+1;m<7;m++)
					{
						//获取数据
						cbTempLastCardData[0]=cbTempCardData[i];
						cbTempLastCardData[1]=cbTempCardData[j];
						cbTempLastCardData[2]=cbTempCardData[k];
						cbTempLastCardData[3]=cbTempCardData[l];
						cbTempLastCardData[4]=cbTempCardData[m];

						//获取牌型
						cbTempCardKind = GetCardType(cbTempLastCardData,getArrayLen(cbTempLastCardData));

						//牌型大小
						if(CompareCard(cbTempLastCardData,cbLastCardData,getArrayLen(cbTempLastCardData))==2)
						{
							cbCardKind = cbTempCardKind;
							memcpy(cbLastCardData,cbTempLastCardData,sizeof(BYTE)*getArrayLen(cbTempLastCardData));
						}
					}
				}
			}
		}
	}

	return cbCardKind;
}
//查找最大
bool CTexasLogic::SelectMaxUser(BYTE bCardData[GAME_PLAYER][MAX_CENTERCOUNT],UserWinList &EndResult,const int64 lAddScore[])
{
	//清理数据
	memset(&EndResult,0,sizeof(EndResult));

	//First数据
	WORD wWinnerID;
    BYTE i = 0;
	for(i=0;i<GAME_PLAYER;i++)
	{
		if(bCardData[i][0]!=0)
		{
			wWinnerID=i;
			break;
		}
	}
	//过滤全零
	if(i==GAME_PLAYER)return false;

	//查找最大用户
	WORD wTemp = wWinnerID;
	for(WORD i=1;i<GAME_PLAYER;i++)
	{
		WORD j=(i+wTemp)%GAME_PLAYER;
		if(bCardData[j][0]==0)continue;
		if(CompareCard(bCardData[j],bCardData[wWinnerID],MAX_CENTERCOUNT)>1)
		{
			wWinnerID=j;
		}
	}

	//查找相同数据
	EndResult.wWinerList[EndResult.bSameCount++] = wWinnerID;
	for(WORD i=0;i<GAME_PLAYER;i++)
	{
		if(i==wWinnerID || bCardData[i][0]==0)continue;
		if(CompareCard(bCardData[i],bCardData[wWinnerID],MAX_CENTERCOUNT)==FALSE)
		{
			EndResult.wWinerList[EndResult.bSameCount++] = i;
		}
	}

	//从小到大
	if(EndResult.bSameCount>1 && lAddScore!=NULL)
	{
		int iSameCount=(int)EndResult.bSameCount;
		while((iSameCount--)>0)
		{
			int64 lTempSocre = lAddScore[EndResult.wWinerList[iSameCount]];
			for(int i=iSameCount-1;i>=0;i--)
			{
				ASSERT(lAddScore[EndResult.wWinerList[i]]>0);
				if(lTempSocre < lAddScore[EndResult.wWinerList[i]])
				{
					lTempSocre = lAddScore[EndResult.wWinerList[i]];
					WORD wTemp = EndResult.wWinerList[iSameCount];
					EndResult.wWinerList[iSameCount] = EndResult.wWinerList[i];
					EndResult.wWinerList[i] = wTemp;
				}
			}
		}
	}

	return true;
}

//排列扑克（牌面逻辑值从大到小排列）
void CTexasLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount)
{
	//转换数值
	BYTE cbLogicValue[FULL_COUNT];
	for (BYTE i=0;i<cbCardCount;i++) 
		cbLogicValue[i]=GetCardLogicValue(cbCardData[i]);	

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

//逻辑数值
BYTE CTexasLogic::GetCardLogicValue(BYTE cbCardData)
{
	//扑克属性
	BYTE bCardColor=GetCardColor(cbCardData);
	BYTE bCardValue=GetCardValue(cbCardData);

	//转换数值
	return (bCardValue==1)?(bCardValue+13):bCardValue;
}

//对比扑克
BYTE CTexasLogic::CompareCard(BYTE ocbFirstData[], BYTE ocbNextData[], BYTE cbCardCount)
{
	//获取类型
	BYTE cbNextType=GetCardType(ocbNextData,cbCardCount);
	BYTE cbFirstType=GetCardType(ocbFirstData,cbCardCount);

	//类型判断
	//大
	if(cbFirstType>cbNextType)
		return 2;

	//小
	if(cbFirstType<cbNextType)
		return 1;

	// add by har
	uint8 cbNextData[MAX_CARD_COUNT]; // 按牌值从大到小排好序的数组，下面的比较会用到。这里使用临时变量
	memcpy(cbNextData, ocbNextData, cbCardCount);
	SortCardList(cbNextData, cbCardCount);
	uint8 cbFirstData[MAX_CARD_COUNT]; // 按牌值从大到小排好序的数组组，下面的比较会用到。这里使用临时变量
	memcpy(cbFirstData, ocbFirstData, cbCardCount);
	SortCardList(cbFirstData, cbCardCount); // add by har end

	//简单类型
	switch(cbFirstType)
	{
	case CT_SINGLE:			//单牌
		{
			//对比数值
			BYTE i=0;
			for (i=0;i<cbCardCount;i++)
			{
				BYTE cbNextValue=GetCardLogicValue(cbNextData[i]);
				BYTE cbFirstValue=GetCardLogicValue(cbFirstData[i]);

				//大
				if(cbFirstValue > cbNextValue)
					return 2;
				//小
				else if(cbFirstValue <cbNextValue)
					return 1;
				//等
				else
					continue;
			}
			//平
			if( i == cbCardCount)
				return 0;
			ASSERT(FALSE);
		}
	case CT_ONE_LONG:		//对子
	case CT_TWO_LONG:		//两对
	case CT_THREE_TIAO:		//三条
	case CT_TIE_ZHI:		//铁支
	case CT_HU_LU:			//葫芦
		{
			//分析扑克
			tagAnalyseResult AnalyseResultNext;
			tagAnalyseResult AnalyseResultFirst;
			AnalysebCardData(cbNextData,cbCardCount,AnalyseResultNext);
			AnalysebCardData(cbFirstData,cbCardCount,AnalyseResultFirst);

			//四条数值
			if (AnalyseResultFirst.cbFourCount>0)
			{
				BYTE cbNextValue=AnalyseResultNext.cbFourLogicVolue[0];
				BYTE cbFirstValue=AnalyseResultFirst.cbFourLogicVolue[0];

				//比较四条
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;

				//比较单牌
				ASSERT(AnalyseResultFirst.cbSignedCount==1 && AnalyseResultNext.cbSignedCount==1);
				cbFirstValue = AnalyseResultFirst.cbSignedLogicVolue[0];
				cbNextValue = AnalyseResultNext.cbSignedLogicVolue[0];
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;
				else return 0;
			}

			//三条数值
			if (AnalyseResultFirst.cbThreeCount>0)
			{
				BYTE cbNextValue=AnalyseResultNext.cbThreeLogicVolue[0];
				BYTE cbFirstValue=AnalyseResultFirst.cbThreeLogicVolue[0];

				//比较三条
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;

				//葫芦牌型
				if(CT_HU_LU == cbFirstType)
				{
					//比较对牌
					ASSERT(AnalyseResultFirst.cbLONGCount==1 && AnalyseResultNext.cbLONGCount==1);
					cbFirstValue = AnalyseResultFirst.cbLONGLogicVolue[0];
					cbNextValue = AnalyseResultNext.cbLONGLogicVolue[0];
					if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;
					else return 0;
				}
				else //三条带单
				{
					//比较单牌
					ASSERT(AnalyseResultFirst.cbSignedCount==2 && AnalyseResultNext.cbSignedCount==2);

					//散牌数值
					BYTE i=0;
					for (i=0;i<AnalyseResultFirst.cbSignedCount;i++)
					{
						BYTE cbNextValue=AnalyseResultNext.cbSignedLogicVolue[i];
						BYTE cbFirstValue=AnalyseResultFirst.cbSignedLogicVolue[i];
						//大
						if(cbFirstValue > cbNextValue)
							return 2;
						//小
						else if(cbFirstValue <cbNextValue)
							return 1;
						//等
						else continue;
					}
					if( i == AnalyseResultFirst.cbSignedCount)
						return 0;
					ASSERT(FALSE);
				}
			}

			//对子数值
			BYTE i=0;
			for ( i=0;i<AnalyseResultFirst.cbLONGCount;i++)
			{
				BYTE cbNextValue=AnalyseResultNext.cbLONGLogicVolue[i];
				BYTE cbFirstValue=AnalyseResultFirst.cbLONGLogicVolue[i];
				//大
				if(cbFirstValue > cbNextValue)
					return 2;
				//小
				else if(cbFirstValue <cbNextValue)
					return 1;
				//平
				else
					continue;
			}

			//比较单牌
			ASSERT( i == AnalyseResultFirst.cbLONGCount);
			{
				ASSERT(AnalyseResultFirst.cbSignedCount==AnalyseResultNext.cbSignedCount
					&& AnalyseResultNext.cbSignedCount>0);
				//散牌数值
				for (i=0;i<AnalyseResultFirst.cbSignedCount;i++)
				{
					BYTE cbNextValue=AnalyseResultNext.cbSignedLogicVolue[i];
					BYTE cbFirstValue=AnalyseResultFirst.cbSignedLogicVolue[i];
					//大
					if(cbFirstValue > cbNextValue)
						return 2;
					//小
					else if(cbFirstValue <cbNextValue)
						return 1;
					//等
					else continue;
				}
				//平
				if( i == AnalyseResultFirst.cbSignedCount)
					return 0;
			}
			break;
		}
	case CT_SHUN_ZI:		//顺子
	case CT_TONG_HUA_SHUN:	//同花顺
		{
			//数值判断
			BYTE cbNextValue=GetCardLogicValue(cbNextData[0]);
			BYTE cbFirstValue=GetCardLogicValue(cbFirstData[0]);

			bool bFirstmin= (cbFirstValue ==(GetCardLogicValue(cbFirstData[1])+9));
			bool bNextmin= (cbNextValue ==(GetCardLogicValue(cbNextData[1])+9));

			//大小顺子
			if ((bFirstmin==true)&&(bNextmin == false))
			{
				return 1;
			}
			//大小顺子
			else if ((bFirstmin==false)&&(bNextmin == true))
			{
				return 2;
			}
			//等同顺子
			else
			{
				//平
				if(cbFirstValue == cbNextValue)return 0;		
				return (cbFirstValue > cbNextValue)?2:1;		
			}
		}
	case CT_TONG_HUA:		//同花
		{	
			//散牌数值
			BYTE i=0;
			for(i=0;i<cbCardCount;i++)
			{
				BYTE cbNextValue=GetCardLogicValue(cbNextData[i]);
				BYTE cbFirstValue=GetCardLogicValue(cbFirstData[i]);

				if(cbFirstValue == cbNextValue)continue;
				return (cbFirstValue > cbNextValue)?2:1;
			}
			//平
			if(i == cbCardCount) return 0;
		}
	}

	return  0;
}

//分析扑克
void CTexasLogic::AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//设置结果
	memset(&AnalyseResult,0,sizeof(AnalyseResult));

	//扑克分析
	for (BYTE i=0;i<cbCardCount;i++)
	{
		//变量定义
		BYTE cbSameCount=1;
		BYTE cbSameCardData[4]={cbCardData[i],0,0,0};
		BYTE cbLogicValue=GetCardLogicValue(cbCardData[i]);

		//获取同牌
		for (int j=i+1;j<cbCardCount;j++)
		{
			//逻辑对比
			if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

			//设置扑克
			cbSameCardData[cbSameCount++]=cbCardData[j];
		}

		//保存结果
		switch (cbSameCount)
		{
		case 1:		//单张
			{
				AnalyseResult.cbSignedLogicVolue[AnalyseResult.cbSignedCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbSignedCardData[(AnalyseResult.cbSignedCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 2:		//两张
			{
				AnalyseResult.cbLONGLogicVolue[AnalyseResult.cbLONGCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbLONGCardData[(AnalyseResult.cbLONGCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 3:		//三张
			{
				AnalyseResult.cbThreeLogicVolue[AnalyseResult.cbThreeCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbThreeCardData[(AnalyseResult.cbThreeCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 4:		//四张
			{
				AnalyseResult.cbFourLogicVolue[AnalyseResult.cbFourCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbFourCardData[(AnalyseResult.cbFourCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		}

		//设置递增
		i+=cbSameCount-1;
	}

	return;
}

//获得起手牌价值
uint32 CTexasLogic::GetFirstCardValue(BYTE cbHandCardData[],BYTE cbCardCount)
{
    bool  sameColor = false; // 同色
    bool  duizi     = false; // 对子
    int32 maxValue  = 0;     // 最大值
    bool  lianshun  = false; // 连顺
    bool  zashun    = false; // 杂顺   
    
    if(GetCardColor(cbHandCardData[0]) == GetCardColor(cbHandCardData[1])){
        sameColor = true;
    }
    int32 values[2];
    values[0] = GetCardLogicValue(cbHandCardData[0]);
    values[1] = GetCardLogicValue(cbHandCardData[1]);
    if(values[0] == values[1]){
        duizi = true;
    }
    maxValue = max(values[0],values[1]);
    if(abs(values[0]-values[1]) == 1){
        lianshun = true;
    }else if(abs(values[0]-values[1]) > 1 && abs(values[0]-values[1]) < 5){
        zashun = true;
    }
    uint32 cardScore = 0;//得分
    if(duizi)
	{//对子
        if(maxValue > 10)//过河
		{
			cardScore = maxValue * 100;
		}else{
			cardScore = maxValue * 50;
		}
    }else{
        cardScore += 5*maxValue;
        if(sameColor)
            cardScore += 200;
        if(lianshun)
            cardScore += 20*maxValue;
        if(zashun)
            cardScore += 5*maxValue;        
    }            
    
    return cardScore;
}
bool   CTexasLogic::CanPlayFirstCardValue(BYTE cbHandCardData[],BYTE cbCardCount)
{
    bool  sameColor = false; // 同色
    bool  duizi     = false; // 对子
    bool  lianshun  = false; // 连顺
    bool  zashun    = false; // 杂顺   
    
    if(GetCardColor(cbHandCardData[0]) == GetCardColor(cbHandCardData[1])){
        sameColor = true;
    }
    int32 values[2];
    values[0] = GetCardLogicValue(cbHandCardData[0]);
    values[1] = GetCardLogicValue(cbHandCardData[1]);
    if(values[0] == values[1]){
        duizi = true;
    }
    if(abs(values[0]-values[1]) == 1){
        lianshun = true;
    }else if(abs(values[0]-values[1]) > 1 && abs(values[0]-values[1]) < 5){
        zashun = true;
    }   
    if(sameColor || duizi || lianshun || zashun)
        return true;
    
    return false;           
}




bool CTexasLogic::GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][MAX_COUNT], BYTE cbCenterCardData[])
{
	int iArTempProCardType[GAME_PLAYER] = { 0 };
	for (int i = 0; i < GAME_PLAYER; i++)
	{
		iArTempProCardType[i] = iArProCardType[i];
	}

	BYTE cbArTempCardData[GAME_PLAYER][MAX_COUNT] = { 0 };
	memset(cbArTempCardData, 0, sizeof(cbArTempCardData));

	BYTE cbCardListData[FULL_COUNT] = { 0 };
	memcpy(cbCardListData, m_cbCardData, sizeof(m_cbCardData));

	// 删除中心牌值
	for (int c1 = 0; c1 < MAX_CENTERCOUNT; c1++)
	{
		BYTE cbCard = cbCenterCardData[c1];
		for (int c2 = 0; c2 < FULL_COUNT; c2++)
		{
			if (cbCard == cbCardListData[c2])
			{
				cbCardListData[c2] = 0;
			}
		}
	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		bool bIsFlag = false;
		if (iArTempProCardType[i] == Texas_Pro_Index_TongHuaShun)
		{
			bIsFlag = GetTypeTongHuaShun(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_TieZhi)
		{
			bIsFlag = GetTypeTieZhi(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_HuLu)
		{
			bIsFlag = GetTypeHuLu(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_TongHua)
		{
			bIsFlag = GetTypeTongHua(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_ShunZi)
		{
			bIsFlag = GetTypeShunZi(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_ThreeTiao)
		{
			bIsFlag = GetTypeThreeTiao(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_TwoDouble)
		{
			bIsFlag = GetTypeTwoDouble(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_OneDouble)
		{
			bIsFlag = GetTypeOneDouble(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
		}
		else if (iArTempProCardType[i] == Texas_Pro_Index_Single)
		{
			bIsFlag = GetTypeSingle(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
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
			iArTempProCardType[i] = Texas_Pro_Index_MAX;
			// error
		}

	}

	for (int i = 0; i < GAME_PLAYER; i++)
	{
		if (iArTempProCardType[i] == Texas_Pro_Index_MAX)
		{
			bool bIsFlag = GetTypeSingle(cbCardListData, FULL_COUNT, cbArTempCardData[i], MAX_COUNT, cbCenterCardData);
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

bool CTexasLogic::IsValidCard(BYTE cbCardData)
{
	for (int i = 0; i < FULL_COUNT; i++)
	{
		if (m_cbCardData[i] == cbCardData)
		{
			return true;
		}
	}
	return false;
}

bool CTexasLogic::GetTypeTongHuaShun(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	//	LOG_DEBUG("获取同花顺 a - i:%d,cbTempListCount:%d,cbTempCardListData:0x%02X", i, cbTempListCount, cbTempCardListData[i]);
	//}

	// 统计数值数目
	BYTE cbArCardColorCount[MAX_CARD_VALUE][MAX_CARD_COLOR] = { 0 };
	memset(cbArCardColorCount, 0, sizeof(cbArCardColorCount));
	// 每个值的颜色最多有一张牌
	// 0	1	2	3
	// 
	// 8
	// 9
	// 10
	// 11
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
	//		LOG_DEBUG("获取同花顺 0 - i:%d,j:%d,cbArCardColorCount:%d",i,j, cbArCardColorCount[i][j]);
	//	}
	//}

	// 找出同花顺
	BYTE cbArAllTargetCard[FULL_COUNT] = { 0 };
	BYTE cbArAllTargetColor[FULL_COUNT] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 1; i < MAX_CARD_VALUE - 4; i++)
	{
		//	  1	   8    9   10	 11	  12   13
		// 0x01,0x08,0x09,0x0A,0x0B,0x0C,0x0D
		for (BYTE j = 0; j < MAX_CARD_COLOR; j++)
		{
			if (cbArCardColorCount[i][j] == 1 && cbArCardColorCount[i + 1][j] == 1 &&
				cbArCardColorCount[i + 2][j] == 1 && cbArCardColorCount[i + 3][j] == 1 && cbArCardColorCount[i + 4][j] == 1)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				cbArAllTargetColor[iAllTargetCount] = j;
				iAllTargetCount++;
			}
		}
	}

	//for (int i = 0; i < FULL_COUNT; i++)
	//{
	//	LOG_DEBUG("获取同花顺 1 - i:%d,iAllTargetCount:%d,cbArAllTargetColor[i]:%d,cbArAllTargetCard[i]:%d",i, iAllTargetCount,cbArAllTargetColor[i], cbArAllTargetCard[i]);
	//}

	if (iAllTargetCount <= 0)
	{
		return false;
	}

	// 随机同花顺
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardColor = cbArAllTargetColor[cbTargetPosition];
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];


	//LOG_DEBUG("获取同花顺 2 - cbTargetPosition:%d,cbTargetCardColor:%d, cbTargetCardValue:%d",	cbTargetPosition, cbTargetCardColor, cbTargetCardValue );


	// 同花顺颜色
	BYTE cbTargetCardCount = 0;
	BYTE cbTargetCard[MAX_COUNT] = { 0 };

	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);

			bool bIsAdd = ((cbCardValue == cbTargetCardValue) || (cbCardValue == cbTargetCardValue + 1) || (cbCardValue == cbTargetCardValue + 2) || (cbCardValue == cbTargetCardValue + 3) || (cbCardValue == cbTargetCardValue + 4));
			if (cbTargetCardCount < MAX_COUNT && cbCardColor == cbTargetCardColor && bIsAdd)
			{
				//LOG_DEBUG("获取同花顺 3 - i:%d,cbTargetCardCount:%d,cbCardColor:%d,cbCardValue:%d,cbCardListData:0x%02X", i, cbTargetCardCount, cbCardColor, cbCardValue, cbCardListData[i]);

				cbTargetCard[cbTargetCardCount++] = cbCardListData[i];
			}
		}
	}
	if (cbTargetCardCount != cbCardCount)
	{
		return false;
	}
	for (BYTE i = 0; i < cbCardCount; i++)
	{
		cbCardData[i] = cbTargetCard[i];
	}
	RandCardListEx(cbCardData, cbCardCount);
	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}
	return true;
}

bool CTexasLogic::GetTypeTieZhi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	// 找出四个
	BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] == 4)
		{
			cbArAllTargetCard[iAllTargetCount] = i;
			iAllTargetCount++;
		}
	}
	if (iAllTargetCount <= 0)
	{
		return false;
	}

	// 随机四个
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

	BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
	BYTE cbTargetCardCount = 0;
	bool bIsAddRand = false;
	BYTE cbCardCountIndex = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbTargetCardValue)
			{
				cbTargetCard[cbTargetCardCount] = cbCardListData[i];
				cbTargetCardCount++;
			}
			else
			{
				if (bIsAddRand == false)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					bIsAddRand = true;
				}
			}
		}
	}
	for (BYTE i = 0; i < cbTargetCardCount; i++)
	{
		cbCardData[cbCardCountIndex++] = cbTargetCard[i];
	}
	RandCardListEx(cbCardData, cbCardCount);
	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	return true;
}
bool CTexasLogic::GetTypeHuLu(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	// 找出三个
	BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	BYTE cbArAllSubTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllSubTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 3)
		{
			cbArAllTargetCard[iAllTargetCount] = i;
			iAllTargetCount++;
		}
		if (cbArCardValueCount[i] >= 2)
		{
			cbArAllSubTargetCard[iAllSubTargetCount] = i;
			iAllSubTargetCount++;
		}
	}
	if (iAllTargetCount <= 0 || iAllSubTargetCount <= 0)
	{
		return false;
	}
	// 随机三个
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

	LOG_DEBUG("2 cbTargetPosition:%d,cbTargetCardValue:%d", cbTargetPosition, cbTargetCardValue);
	for (int i = 0; i < iAllTargetCount; i++)
	{
		LOG_DEBUG("3 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
	}

	// 随机两个
	for (BYTE i = 0; i < iAllSubTargetCount; i++)
	{
		if (cbTargetCardValue == cbArAllSubTargetCard[i])
		{

			//往前移
			BYTE j = i;
			for (; j < iAllSubTargetCount - 1; j++)
			{
				cbArAllSubTargetCard[j] = cbArAllSubTargetCard[j + 1];
			}
			cbArAllSubTargetCard[iAllSubTargetCount - 1] = 0;
			iAllSubTargetCount--;
			break;
		}
	}
	BYTE cbSubTargetPosition = g_RandGen.RandRange(0, iAllSubTargetCount - 1);
	BYTE cbSubTargetCardValue = cbArAllSubTargetCard[cbSubTargetPosition];

	LOG_DEBUG("2 iAllSubTargetCount:%d,cbSubTargetCardValue:%d", cbSubTargetPosition, cbSubTargetCardValue);
	for (int i = 0; i < iAllSubTargetCount; i++)
	{
		LOG_DEBUG("3 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
	}

	BYTE cbTargetCard[MAX_COUNT] = { 0 };
	BYTE cbTargetCardCount = 0;
	BYTE cbSubTargetCardCount = 0;
	BYTE cbCardCountIndex = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbTargetCardValue && cbTargetCardCount<3)
			{
				cbTargetCard[cbCardCountIndex++] = cbCardListData[i];
				cbTargetCardCount++;
			}
			if (cbCardValue == cbSubTargetCardValue && cbSubTargetCardCount<2)
			{
				cbTargetCard[cbCardCountIndex++] = cbCardListData[i];
				cbSubTargetCardCount++;
			}
		}
	}
	if (cbTargetCardCount + cbSubTargetCardCount != cbCardCount || cbCardCountIndex != cbCardCount)
	{
		return false;
	}
	for (BYTE i = 0; i < cbTargetCardCount + cbSubTargetCardCount; i++)
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
bool CTexasLogic::GetTypeTongHua(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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

	// 找出同花
	BYTE cbArAllTarget[MAX_CARD_COLOR] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_COLOR; i++)
	{
		if (cbArCardColorCount[i] >= 5)
		{
			cbArAllTarget[iAllTargetCount++] = i;
		}
	}
	LOG_DEBUG("1 iAllTargetCount:%d", iAllTargetCount);

	if (iAllTargetCount <= 0)
	{
		return false;
	}

	// 随机颜色
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardColor = cbArAllTarget[cbTargetPosition];

	LOG_DEBUG("2 cbTargetPosition:%d,cbTargetCardColor:%d", cbTargetPosition, cbTargetCardColor);
	for (int i = 0; i < iAllTargetCount; i++)
	{
		LOG_DEBUG("3 iAllTargetCount:%d,cbArAllTarget:%d", iAllTargetCount, cbArAllTarget[i]);
	}

	// 相同颜色的所有牌
	BYTE cbTargetCard[MAX_CARD_VALUE] = { 0 };
	BYTE cbTargetCardCount = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
			if (cbCardColor == cbTargetCardColor && cbTargetCardCount < MAX_CARD_VALUE)
			{
				cbTargetCard[cbTargetCardCount++] = cbCardListData[i];
			}
		}
	}

	// 选其中五个
	BYTE cbLastTargetCard[MAX_COUNT] = { 0 };
	BYTE cbRandCount = 0, cbPosition = 0;
	do
	{
		BYTE cbPosition = g_RandGen.RandRange(0, cbTargetCardCount - cbRandCount - 1);
		cbLastTargetCard[cbRandCount++] = cbTargetCard[cbPosition];
		cbTargetCard[cbPosition] = cbTargetCard[cbTargetCardCount - cbRandCount];
	} while (cbRandCount < cbCardCount);

	LOG_DEBUG("cbRandCount:%d,cbLastTargetCard:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
		cbRandCount, cbLastTargetCard[0], cbLastTargetCard[1], cbLastTargetCard[2], cbLastTargetCard[3], cbLastTargetCard[4]);


	//检查是否同花
	SortCardList(cbLastTargetCard, MAX_COUNT);
	bool bLineCard = true;
	BYTE cbFirstValue = GetCardValue(cbLastTargetCard[0]);
	for (BYTE i = 1; i<cbCardCount; i++)
	{
		if (cbFirstValue != (GetCardValue(cbLastTargetCard[i]) + i)) bLineCard = false;
		if (bLineCard == false)break;
	}

	if (bLineCard == false)
	{
		for (BYTE i = 0; i < cbCardCount; i++)
		{
			cbCardData[i] = cbLastTargetCard[i];
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
bool CTexasLogic::GetTypeShunZi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	for (BYTE i = 0; i < MAX_CARD_VALUE - 4; i++)
	{
		//	  0	   1	2    3	  4	   5	6	 7	  8	   9   10	11	 12	  13
		// 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D };
		// 需要有两种花色才不会随到顺金
		if (cbArCardValueCount[i] >= 2 && cbArCardValueCount[i + 1] >= 2 && cbArCardValueCount[i + 2] >= 2 && cbArCardValueCount[i + 3] >= 2 && cbArCardValueCount[i + 4] >= 2)
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
	BYTE cbTargetColorCount[MAX_COUNT] = { 0 };
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
	return true;
}
bool CTexasLogic::GetTypeThreeTiao(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	// 找出三个
	BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 3)
		{
			cbArAllTargetCard[iAllTargetCount] = i;
			iAllTargetCount++;
		}
	}
	if (iAllTargetCount <= 0)
	{
		return false;
	}

	// 随机三个
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

	BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
	BYTE cbTargetCardCount = 0;
	BYTE cbFirstCardValue = 0xFF;
	BYTE cbCardCountIndex = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbTargetCardValue && cbTargetCardCount < 3)
			{
				cbTargetCard[cbTargetCardCount] = cbCardListData[i];
				cbTargetCardCount++;
			}
			else if (cbCardCountIndex < 2)
			{
				if (cbFirstCardValue == 0xFF)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					cbFirstCardValue = cbCardValue;
				}
				else if (cbFirstCardValue != cbCardValue)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
				}
			}
		}
	}

	if (cbTargetCardCount + cbCardCountIndex != cbCardCount)
	{
		return false;
	}

	for (BYTE i = 0; i < cbTargetCardCount; i++)
	{
		cbCardData[cbCardCountIndex++] = cbTargetCard[i];
	}
	RandCardListEx(cbCardData, cbCardCount);
	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	return true;
}
bool CTexasLogic::GetTypeTwoDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	// 找出三个
	BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	BYTE cbArAllSubTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllSubTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 2)
		{
			cbArAllTargetCard[iAllTargetCount] = i;
			iAllTargetCount++;
		}
		if (cbArCardValueCount[i] >= 2)
		{
			cbArAllSubTargetCard[iAllSubTargetCount] = i;
			iAllSubTargetCount++;
		}
	}
	if (iAllTargetCount <= 0 || iAllSubTargetCount <= 0)
	{
		return false;
	}
	for (int i = 0; i < iAllTargetCount; i++)
	{
		LOG_DEBUG("1 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
	}
	for (int i = 0; i < iAllSubTargetCount; i++)
	{
		LOG_DEBUG("1 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
	}
	// 随机两个
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

	// 随机两个
	for (BYTE i = 0; i < iAllSubTargetCount; i++)
	{
		if (cbTargetCardValue == cbArAllSubTargetCard[i])
		{
			//往前移
			BYTE j = i;
			for (; j < iAllSubTargetCount - 1; j++)
			{
				cbArAllSubTargetCard[j] = cbArAllSubTargetCard[j + 1];
			}
			cbArAllSubTargetCard[iAllSubTargetCount - 1] = 0;
			iAllSubTargetCount--;
			break;
		}
	}

	for (int i = 0; i < iAllTargetCount; i++)
	{
		LOG_DEBUG("2 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
	}
	for (int i = 0; i < iAllSubTargetCount; i++)
	{
		LOG_DEBUG("2 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
	}

	BYTE cbSubTargetPosition = g_RandGen.RandRange(0, iAllSubTargetCount - 1);
	BYTE cbSubTargetCardValue = cbArAllSubTargetCard[cbSubTargetPosition];

	LOG_DEBUG("cbTargetPosition:%d,cbTargetCardValue:%d", cbTargetPosition, cbTargetCardValue);
	LOG_DEBUG("cbSubTargetPosition:%d,cbSubTargetCardValue:%d", cbSubTargetPosition, cbSubTargetCardValue);

	BYTE cbTargetCard[MAX_COUNT] = { 0 };
	BYTE cbTargetCardCount = 0;
	BYTE cbSubTargetCardCount = 0;
	BYTE cbCardCountIndex = 0;
	bool bIsAddLastCard = false;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbTargetCardValue && cbTargetCardCount<2)
			{
				cbCardData[cbCardCountIndex++] = cbCardListData[i];
				cbTargetCardCount++;
			}
			else if (cbCardValue == cbSubTargetCardValue && cbSubTargetCardCount<2)
			{
				cbCardData[cbCardCountIndex++] = cbCardListData[i];
				cbSubTargetCardCount++;
			}
			else if (bIsAddLastCard == false)
			{
				cbCardData[cbCardCountIndex++] = cbCardListData[i];
				bIsAddLastCard = true;
			}
		}
	}
	if (cbCardCountIndex != cbCardCount)
	{
		return false;
	}

	for (int i = 0; i < cbCardCount; i++)
	{
		if (IsValidCard(cbCardData[i]) == false)
		{
			return false;
		}
	}

	LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

	return true;
}
bool CTexasLogic::GetTypeOneDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
	// 找出三个
	BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
	int iAllTargetCount = 0;
	for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
	{
		if (cbArCardValueCount[i] >= 2)
		{
			cbArAllTargetCard[iAllTargetCount] = i;
			iAllTargetCount++;
		}
	}
	if (iAllTargetCount <= 0)
	{
		return false;
	}

	// 随机两个
	BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
	BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

	BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
	BYTE cbTargetCardCount = 0;
	BYTE cbFirstCardValue = 0xFF;
	BYTE cbSecondCardValue = 0xFF;
	BYTE cbCardCountIndex = 0;
	for (BYTE i = 0; i < cbListCount; i++)
	{
		if (IsValidCard(cbCardListData[i]))
		{
			BYTE cbCardValue = GetCardValue(cbCardListData[i]);
			if (cbCardValue == cbTargetCardValue)
			{
				if (cbTargetCardCount < 2)
				{
					cbTargetCard[cbTargetCardCount] = cbCardListData[i];
					cbTargetCardCount++;
				}
			}
			else if (cbCardCountIndex < 3)
			{
				if (cbFirstCardValue == 0xFF)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					cbFirstCardValue = cbCardValue;
				}
				else if (cbSecondCardValue == 0xFF && cbFirstCardValue != cbCardValue)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					cbSecondCardValue = cbCardValue;
				}
				else if (cbFirstCardValue != cbCardValue && cbSecondCardValue != cbCardValue)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
				}
			}
		}
	}

	LOG_DEBUG("cbTargetCardCount:%d,cbCardCountIndex:%d,cbCardCount:%d,cbFirstCardValue:%d,cbSecondCardValue:%d",
		cbTargetCardCount, cbCardCountIndex, cbCardCount, cbFirstCardValue, cbSecondCardValue);

	if (cbTargetCardCount + cbCardCountIndex != cbCardCount)
	{
		return false;
	}

	for (BYTE i = 0; i < cbTargetCardCount; i++)
	{
		cbCardData[cbCardCountIndex++] = cbTargetCard[i];
	}

	LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

	RandCardListEx(cbCardData, cbCardCount);

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
bool CTexasLogic::GetTypeSingle(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[])
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
		LOG_DEBUG("cbTempListCount:%d,cbCardCount:%d", cbTempListCount, cbCardCount);
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
			LOG_DEBUG("i:%d,cbCardCount:%d,cbCardData[i]:0x%02X", i, cbCardCount, cbCardData[i]);
			return false;
		}
	}

	LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

	return true;
}

};



