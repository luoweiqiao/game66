
#ifndef GAME_NIUNIU_LOGIC_HEAD_FILE
#define GAME_NIUNIU_LOGIC_HEAD_FILE


#include "poker_logic.h"

namespace game_niuniu
{
//////////////////////////////////////////////////////////////////////////
//牌型
	struct tagNiuMultiple {
		int32	niu1;
		int32	niu2;
		int32	niu3;
		int32	niu4;
		int32	niu5;
		int32	niu6;
		int32	niu7;
		int32	niu8;
		int32	niu9;
		int32	niuniu;
		int32	big5niu;
		int32	small5niu;
		int32   bomebome;
		tagNiuMultiple() {
			Init();
		}
		void Init() {
			niu1 = 0;
			niu2 = 0;
			niu3 = 0;
			niu4 = 0;
			niu5 = 0;
			niu6 = 0;
			niu7 = 0;
			niu8 = 0;
			niu9 = 0;
			niuniu = 0;
			big5niu = 0;
			small5niu = 0;
			bomebome = 0;
		}
	};
enum emCardType
{
	CT_ERROR			=		0,								//错误类型
	CT_POINT			=		1,								//点数类型
	CT_SPECIAL_NIU1		=		2,								//牛一
	CT_SPECIAL_NIU2		=		3,								//牛二
	CT_SPECIAL_NIU3		=		4,								//牛三
	CT_SPECIAL_NIU4		=		5,								//牛四
	CT_SPECIAL_NIU5		=		6,								//牛五
	CT_SPECIAL_NIU6		=		7,								//牛六
	CT_SPECIAL_NIU7		=		8,								//牛七
	CT_SPECIAL_NIU8		=		9,								//牛八
	CT_SPECIAL_NIU9		=	    10,								//牛九
	CT_SPECIAL_NIUNIU	=		11,								//牛牛
	CT_SPECIAL_NIUNIUDW	=		12,								//大五牛(五张全部JQK)
    CT_SPECIAL_NIUNIUXW	=		13,								//小五牛(五张A234)
	CT_SPECIAL_BOMEBOME	=		14								//炸弹
};

//排序类型
#define	ST_VALUE					1									//数值排序
#define	ST_NEW					    2									//数值排序
#define	ST_LOGIC					3									//逻辑排序

//扑克数目
//#define CARD_COUNT					52									//扑克数目
static const int CARD_COUNT 	 = 52;
static const int NIUNIU_CARD_NUM = 5;

//游戏逻辑
class CNiuNiuLogic : public CPokerLogic
{
	//变量定义
public:
	static const BYTE				m_cbCardListData[CARD_COUNT];	//扑克定义

	//函数定义
public:
	//构造函数
	CNiuNiuLogic();
	//析构函数
	virtual ~CNiuNiuLogic();

	//类型函数
public:
	//获取数值
	BYTE GetCardValue(BYTE cbCardData) 
	{ 
		return cbCardData&MASK_VALUE; 
	}
	//获取花色
	BYTE GetCardColor(BYTE cbCardData)
	{
		return (cbCardData&MASK_COLOR)>>4;
	}

	//控制函数
public:
	//混乱扑克
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
	//混乱扑克
	void RandCardListEx(BYTE cbCardBuffer[], BYTE cbBufferCount);
        
	//排列扑克
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

	int  RetType(int itype);

	//逻辑函数
public:
	//获取牌点
	BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
	//获取牌型
	BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount,BYTE *bcOutCadData = NULL);
    
	//大小比较
	int  CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount, const BYTE cbNextCardData[], BYTE cbNextCardCount, BYTE &Multiple, BYTE multipleType = 0, tagNiuMultiple * ptagNiuMultiple = NULL);
	//逻辑大小
	BYTE GetCardLogicValue(BYTE cbCardData);

	BYTE GetCardNewValue(BYTE cbCardData);
};

//////////////////////////////////////////////////////////////////////////
};
#endif
