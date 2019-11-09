//
// Created by toney on 16/4/12.
//

#ifndef SERVER_SANGONG_LOGIC_H
#define SERVER_SANGONG_LOGIC_H

#include "poker_logic.h"

namespace game_sangong
{
//宏定义

static const int GAME_PLAYER	    =			6;									//游戏人数

//数目定义
static const int FULL_COUNT		=			    52;									//全牌数目
static const int MAX_COUNT		=				3;									//最大数目

//扑克类型
enum emSANGONG_CARD_TYPE
{
	//扑克类型
 	OX_VALUE0					 = 0,									//混合牌型
	OX_VALUE1,
	OX_VALUE2,
	OX_VALUE3,
	OX_VALUE4,
	OX_VALUE5,
	OX_VALUE6,
	OX_VALUE7,
	OX_VALUE8,
	OX_VALUE9,
 	OX_THREE_KING0,														//混三公牌型
	OX_THREE_KING1,														//小三公
	OX_THREE_KING2,														//大三公
};

//游戏逻辑类
class CSangongLogic : public CPokerLogic
{
	//变量定义
private:
	static BYTE						m_cbCardListData[52];			//扑克定义    
	//函数定义
public:
	//构造函数
	CSangongLogic();
	//析构函数
	virtual ~CSangongLogic();

	//类型函数
public:
	//获取类型
	BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount);
	//获取倍数
	BYTE GetTimes(BYTE cbCardData[], BYTE cbCardCount);
	//获取牛牛
	bool GetOxCard(BYTE cbCardData[], BYTE cbCardCount);
	//获取整数
	bool IsIntValue(BYTE cbCardData[], BYTE cbCardCount);

	//控制函数
public:
	//排列扑克
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount);
	//混乱扑克
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);

	//功能函数
public:
	//逻辑数值
	BYTE GetCardLogicValue(BYTE cbCardData);
	//对比扑克
	bool CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount,BYTE& multiple);

};


};


#endif //SERVER_SANGONG_LOGIC_H

