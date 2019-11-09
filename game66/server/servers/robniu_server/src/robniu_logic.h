
#ifndef GAME_ROBNIU_LOGIC_HEAD_FILE
#define GAME_ROBNIU_LOGIC_HEAD_FILE


#include "poker/poker_logic.h"

namespace game_robniu
{
//////////////////////////////////////////////////////////////////////////
//牌型



enum emCardType
{
	CT_ERROR			=		0,								//错误类型
	CT_POINT			=		1,								//点数类型（无牛）
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
	CT_SPECIAL_NIUNIUDW	=		12,								//大五牛(五张全部JQK)（五花牛）
    CT_SPECIAL_NIUNIUXW	=		13,								//小五牛(五张A234)（五小牛）
	CT_SPECIAL_BOMEBOME	=		14								//炸弹
};

#define CT_SPECIAL_MAX_TYPE     15								//最大类型概率

//排序类型
#define	ST_VALUE					1									//数值排序
#define	ST_NEW					    2									//数值排序
#define	ST_LOGIC					3									//逻辑排序

//扑克数目
//#define CARD_COUNT					52									//扑克数目
static const int CARD_COUNT 	 = 52;
static const int ROBNIU_CARD_NUM = 5;

static const int GAME_PLAYER = 5;	//最大游戏人数
//static const int JETTON_MULTIPLE_COUNT = 4;  // 下注倍数选择数量
#define JETTON_MULTIPLE_COUNT 4
//static const int ROB_MULTIPLE_COUNT = 5; // 抢庄倍数选择数量 0 1 2 3 4 一共五个选择
#define ROB_MULTIPLE_COUNT 5


static const int MAX_CARD_VALUE = 14;									//最大牌值
static const int MAX_CARD_COLOR = 4;									//最大颜色

static const int MAX_RAND_LOOP_COUNT = 4096;

struct tagTripleCardDataHasTenTimes
{
	BYTE cbCardDataOne;
	BYTE cbCardDataTwo;
	BYTE cbCardDataThree;
	tagTripleCardDataHasTenTimes()
	{
		cbCardDataOne = 255;
		cbCardDataTwo = 255;
		cbCardDataThree = 255;
	}
};

//游戏逻辑
class CRobNiuLogic : public CPokerLogic
{
	//变量定义
public:
	static const BYTE				m_cbCardListData[CARD_COUNT];	//扑克定义

	//函数定义
public:
	//构造函数
	CRobNiuLogic();
	//析构函数
	virtual ~CRobNiuLogic();

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
    
	bool RandUpsetCardData(BYTE cbCardBuffer[], BYTE cbBufferCount);

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
	int  CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount, const BYTE cbNextCardData[], BYTE cbNextCardCount, BYTE &Multiple);
	//逻辑大小
	BYTE GetCardLogicValue(BYTE cbCardData);

	BYTE GetCardNewValue(BYTE cbCardData);
public:
	bool GetSubDataCard(BYTE cbSubCardData[][ROBNIU_CARD_NUM], vector<BYTE> & vecRemainCardData);
	//有效判断
	bool IsValidCard(BYTE cbCardData);
	//获取牌型扑克
	bool GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][ROBNIU_CARD_NUM]);
	//获取点数类型
	bool GetTypePoint(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);

	bool GetTripleCardDataHasTenTimes(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);

	//获取牛一
	bool GetTypeNiuOne(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛二
	bool GetTypeNiuTwo(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛三
	bool GetTypeNiuThree(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛四
	bool GetTypeNiuFour(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛五
	bool GetTypeNiuFive(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛六
	bool GetTypeNiuSix(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛七
	bool GetTypeNiuSeven(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛八
	bool GetTypeNiuEight(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛九
	bool GetTypeNiuNine(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取牛牛
	bool GetTypeNiuNiu(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取大五牛
	bool GetTypeNiuBig_5(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取小五牛
	bool GetTypeNiuSmall_5(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取炸弹
	bool GetTypeBome(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);
	//获取随机牌
	bool GetTypeRand(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount);


};

//////////////////////////////////////////////////////////////////////////
};
#endif
