

#ifndef SERVER_FIGHT_LOGIC_H
#define SERVER_FIGHT_LOGIC_H

#include "poker/poker_logic.h"

//区域索引
#define JETTON_INDEX_DRAGON			0 // 龙
#define JETTON_INDEX_TIGER			1 // 虎
#define JETTON_INDEX_OTHER			2 // 和

#define JETTON_INDEX_COUNT			3				//区域数目
#define SHOW_CARD_COUNT				2
#define MAX_CARD_COUNT				1


namespace game_fight
{
//宏定义

static const int GAME_PLAYER	    =			5;									//游戏人数
//结束原因
static const int  GER_NO_PLAYER		=			0x10;								//没有玩家
static const int  GER_COMPARECARD	=			0x20;								//比牌结束
static const int  GER_OPENCARD		=			0x30;								//开牌结束

//数目定义
static const int FULL_COUNT		=			    52;									//全牌数目
static const int MAX_COUNT		=				1;									//最大数目
static const int DRAW           =				2;									//和局类型
static const int MAX_CARD_VALUE =				14;									//最大牌值
static const int MAX_CARD_COLOR	=				4;									//最大颜色

//扑克类型
enum emFIGHT_CARD_TYPE
{
    //扑克类型
    CT_SINGLE      = 		    1,									//单牌类型
    CT_DOUBLE					,									//对子类型
    CT_SHUN_ZI					,									//顺子类型
    CT_JIN_HUA					,									//金花类型
    CT_SHUN_JIN					,									//顺金类型
    CT_BAO_ZI					,									//豹子类型
    CT_SPECIAL					,									//特殊类型
};

//发牌概率索引
enum emFIGHT_CARD_TYPE_PRO_INDEX
{
	Pro_Index_Dragon = 0,
	Pro_Index_Tiger,
	Pro_Index_Draw,
	Pro_Index_MAX,
};

//游戏逻辑类
class CFightLogic : public CPokerLogic
{
	//变量定义
private:
	static BYTE						m_cbCardListData[52];			//扑克定义
	static BYTE						m_cbArCardColor[4];				//扑克定义
	static BYTE						m_cbArCardValue[13];			//扑克定义
	//函数定义
public:
	//构造函数
	CFightLogic();
	//析构函数
	virtual ~CFightLogic();

	//类型函数
public:
	BYTE GetDoubleValue(BYTE cbCardData[], BYTE cbCardCount);

	//获取类型
	BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount);
	//获得散牌最大值
	BYTE GetSingCardValue(BYTE cbCardData[], BYTE cbCardCount);

	//控制函数
public:
	//排列扑克
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount,bool bAsc = true);
	//混乱扑克
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
    
	//功能函数
public:
	//逻辑数值
	BYTE GetCardLogicValue(BYTE cbCardData);
	//对比扑克
	BYTE CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount);
	//有效判断
	bool IsValidCard(BYTE cbCardData);
	//获取颜色
	BYTE GetRandCardColor();
	//获取数值
	BYTE GetRandCardValue();

	bool IsSameCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount);


};


};


#endif //SERVER_FIGHT_LOGIC_H
