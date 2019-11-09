//
// Created by toney on 16/4/12.
//

#ifndef SERVER_TEXAS_LOGIC_H
#define SERVER_TEXAS_LOGIC_H

#include "poker/poker_logic.h"

namespace game_texas
{
//宏定义

static const int GAME_PLAYER	=				9;									//游戏人数
//结束原因
static const int GER_NO_PLAYER	=				0x10;								//没有玩家

//数目定义
static const int FULL_COUNT		=			    52;									//全牌数目
static const int MAX_COUNT		=				2;									//最大数目
static const int MAX_CENTERCOUNT=				5;									//最大数目
static const int MAX_CARD_COUNT =				5;									//扑克数目

static const int MAX_CARD_VALUE = 14;									//最大牌值
static const int MAX_CARD_COLOR = 4;									//最大颜色


//扑克类型
enum emLAND_CARD_TYPE
{
    CT_SINGLE =					1,				//单牌类型
    CT_ONE_LONG,								//对子类型
    CT_TWO_LONG,								//两对类型
    CT_THREE_TIAO,								//三条类型
    CT_SHUN_ZI,									//顺子类型
    CT_TONG_HUA,								//同花类型
    CT_HU_LU,									//葫芦类型
    CT_TIE_ZHI,									//铁支类型
    CT_TONG_HUA_SHUN,							//同花顺型
    CT_KING_TONG_HUA_SHUN,						//皇家同花顺 (9 10 j q k)
};
//////////////////////////////////////////////////////////////////////////


//发牌概率索引
enum emTEXAS_CARD_TYPE_PRO_INDEX
{
	Texas_Pro_Index_KingTongHuaShun = 0,
	Texas_Pro_Index_TongHuaShun,
	Texas_Pro_Index_TieZhi,
	Texas_Pro_Index_HuLu,
	Texas_Pro_Index_TongHua,
	Texas_Pro_Index_ShunZi,
	Texas_Pro_Index_ThreeTiao,
	Texas_Pro_Index_TwoDouble,
	Texas_Pro_Index_OneDouble,
	Texas_Pro_Index_Single,
	Texas_Pro_Index_MAX,
};

//胜利信息结构
struct UserWinList
{
	BYTE bSameCount;
	WORD wWinerList[GAME_PLAYER];
};

//分析结构
struct tagAnalyseResult
{
	BYTE 							cbFourCount;						//四张数目
	BYTE 							cbThreeCount;						//三张数目
	BYTE 							cbLONGCount;						//两张数目
	BYTE							cbSignedCount;						//单张数目
	BYTE 							cbFourLogicVolue[1];				//四张列表
	BYTE 							cbThreeLogicVolue[1];				//三张列表
	BYTE 							cbLONGLogicVolue[2];				//两张列表
	BYTE 							cbSignedLogicVolue[5];				//单张列表
	BYTE							cbFourCardData[MAX_CENTERCOUNT];			//四张列表
	BYTE							cbThreeCardData[MAX_CENTERCOUNT];			//三张列表
	BYTE							cbLONGCardData[MAX_CENTERCOUNT];		//两张列表
	BYTE							cbSignedCardData[MAX_CENTERCOUNT];		//单张数目
};
//////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CTexasLogic : public CPokerLogic
{
	//函数定义
public:
	//构造函数
	CTexasLogic();
	//析构函数
	virtual ~CTexasLogic();

	//变量定义
public:
	static BYTE						m_cbCardData[FULL_COUNT];				//扑克定义

	//控制函数
public:
	//排列扑克（牌面逻辑值从大到小排列）
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount);
	//混乱扑克
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);

	void RandCardListEx(BYTE cbCardBuffer[], BYTE cbBufferCount)
	{
		BYTE datalen = cbBufferCount;
		BYTE cbRandCount = 0, cbPosition = 0;
		do
		{
			cbPosition = g_RandGen.RandRange(0, cbBufferCount - 1);
			BYTE tempCard = cbCardBuffer[cbRandCount];
			cbCardBuffer[cbRandCount] = cbCardBuffer[cbPosition];
			cbCardBuffer[cbPosition] = tempCard;
			cbRandCount++;
		} while (cbRandCount<cbBufferCount);
		return;
	}

	//类型函数
public:
	//获取类型
	BYTE GetCardType(BYTE ocbCardData[], BYTE cbCardCount);


	//功能函数
public:
	//逻辑数值
	BYTE GetCardLogicValue(BYTE cbCardData);
	//对比扑克
	BYTE CompareCard(BYTE ocbFirstData[], BYTE ocbNextData[], BYTE cbCardCount);
	//分析扑克
	void AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult & AnalyseResult);
	//7返5
	BYTE FiveFromSeven(BYTE cbHandCardData[],BYTE cbHandCardCount,BYTE cbCenterCardData[],BYTE cbCenterCardCount,BYTE cbLastCardData[],BYTE cbLastCardCount);
	//获得当前牌型
	BYTE GetCurCardType(BYTE cbHandCardData[],BYTE cbHandCardCount,BYTE cbCenterCardData[],BYTE cbCenterCardCount);

	//查找最大
	bool SelectMaxUser(BYTE bCardData[GAME_PLAYER][MAX_CENTERCOUNT],UserWinList &EndResult,const int64 lAddScore[]);

    //获得起手牌价值
    uint32 GetFirstCardValue(BYTE cbHandCardData[],BYTE cbCardCount);
    bool   CanPlayFirstCardValue(BYTE cbHandCardData[],BYTE cbCardCount);
    
	//获取牌型扑克
	bool GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][MAX_COUNT],BYTE cbCenterCardData[]);
	//有效判断
	bool IsValidCard(BYTE cbCardData);
	//获取同花顺
	bool GetTypeTongHuaShun(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeTieZhi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeHuLu(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeTongHua(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeShunZi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeThreeTiao(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeTwoDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	bool GetTypeOneDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);
	//获取散牌
	bool GetTypeSingle(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount, BYTE cbCenterCardData[]);

};


};


#endif //SERVER_TEXAS_LOGIC_H