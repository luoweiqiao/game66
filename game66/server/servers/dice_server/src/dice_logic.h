
#ifndef GAME_DICE_LOGIC_HEAD_FILE
#define GAME_DICE_LOGIC_HEAD_FILE


#include "poker/poker_logic.h"

namespace game_dice
{
//////////////////////////////////////////////////////////////////////////

	enum enDiceType
	{
		CT_ERROR            = 0,	   //错误类型

		CT_SUM_SMALL        = 1,       //小
		CT_SUM_BIG          = 2,       //大

		CT_POINT_THREE      = 3,       //3点(实际无此压注项)
		CT_POINT_FOUR       = 4,
		CT_POINT_FIVE       = 5,
		CT_POINT_SIX        = 6,
		CT_POINT_SEVEN      = 7,
		CT_POINT_EIGHT      = 8,
		CT_POINT_NINE       = 9,
		CT_POINT_TEN        = 10,
		CT_POINT_ELEVEN     = 11,
		CT_POINT_TWELVE     = 12,
		CT_POINT_THIR_TEEN  = 13,
		CT_POINT_FOUR_TEEN  = 14,
		CT_POINT_FIF_TEEN   = 15,
		CT_POINT_SIX_TEEN   = 16,
		CT_POINT_SEVEN_TEEN = 17,
		CT_POINT_EIGHT_TEEN = 18,       //18点(实际无此压注项)

		CT_ANY_CICLE_DICE   = 19,       //任意围骰
		CT_LIMIT_CICLE_DICE = 20,       //指定围骰

		CT_ONE            = 21,			//一个1点
		CT_TWO            = 22,			//一个2点
		CT_THREE          = 23,			//一个3点
		CT_FOUR           = 24,			//一个4点
		CT_FIVE           = 25,			//一个5点
		CT_SIX            = 26,			//一个6点

		CT_TWICE_ONE      = 27,			//两个1点
		CT_TWICE_TWO      = 28,			//两个2点
		CT_TWICE_THREE    = 29,			//两个3点
		CT_TWICE_FOUR     = 30,			//两个4点
		CT_TWICE_FIVE     = 31,			//两个5点
		CT_TWICE_SIX      = 32,			//两个6点

		CT_TRIPLE_ONE     = 34,			//三个1点
		CT_TRIPLE_TWO     = 35,			//三个2点
		CT_TRIPLE_THREE   = 36,			//三个3点
		CT_TRIPLE_FOUR    = 37,			//三个4点
		CT_TRIPLE_FIVE    = 38,			//三个5点
		CT_TRIPLE_SIX     = 39,			//三个6点
	};

//////////////////////////////////////////////////////////////////////////
#define GAME_PLAYER					4									//座位人数
#define MAX_SCORE_HISTORY           20									//历史个数
#define DICE_POINT_COUNT            6                                   //骰子点数
#define DICE_COUNT                  3                                   //骰子个数


#define CONTROL_TRY_TIMES           30

#define AREA_COUNT              	40                                  //压注项数
#define BET_MONEY_NUMBER			4									//压注金额项数
//////////////////////////////////////////////////////////////////////////
//赔率
	#define  Multiple_CT_ERROR             	0			//错误类型

	#define  Multiple_CT_SUM_SMALL         	1			//小
	#define  Multiple_CT_SUM_BIG           	1			//大

	#define  Multiple_CT_POINT_THREE       	0			//3点(实际无此压注项)
	#define  Multiple_CT_POINT_FOUR        	60
	#define  Multiple_CT_POINT_FIVE        	30
	#define  Multiple_CT_POINT_SIX         	17
	#define  Multiple_CT_POINT_SEVEN       	12
	#define  Multiple_CT_POINT_EIGHT       	8
	#define  Multiple_CT_POINT_NINE        	6
	#define  Multiple_CT_POINT_TEN         	6
	#define  Multiple_CT_POINT_ELEVEN      	6
	#define  Multiple_CT_POINT_TWELVE      	6
	#define  Multiple_CT_POINT_THIR_TEEN   	8
	#define  Multiple_CT_POINT_FOUR_TEEN   	12
	#define  Multiple_CT_POINT_FIF_TEEN    	17
	#define  Multiple_CT_POINT_SIX_TEEN    	30
	#define  Multiple_CT_POINT_SEVEN_TEEN  	60
	#define  Multiple_CT_POINT_EIGHT_TEEN  	0			//18点(实际无此压注项)

	#define  Multiple_CT_ANY_CICLE_DICE    	30			//任意围骰
	#define  Multiple_CT_LIMIT_CICLE_DICE  	180			//指定围骰	

	#define  Multiple_CT_ONE             	3			//一个1点
	#define  Multiple_CT_TWO             	3			//一个2点
	#define  Multiple_CT_THREE           	3			//一个3点
	#define  Multiple_CT_FOUR            	3			//一个4点
	#define  Multiple_CT_FIVE            	3			//一个5点
	#define  Multiple_CT_SIX             	3			//一个6点

	#define  Multiple_CT_TWICE_ONE       	8			//两个1点
	#define  Multiple_CT_TWICE_TWO       	8			//两个2点
	#define  Multiple_CT_TWICE_THREE     	8			//两个3点
	#define  Multiple_CT_TWICE_FOUR      	8			//两个4点
	#define  Multiple_CT_TWICE_FIVE      	8			//两个5点
	#define  Multiple_CT_TWICE_SIX       	8			//两个6点

	#define  Multiple_CT_TRIPLE_ONE      	180			//三个1点
	#define  Multiple_CT_TRIPLE_TWO      	180			//三个2点
	#define  Multiple_CT_TRIPLE_THREE    	180			//三个3点
	#define  Multiple_CT_TRIPLE_FOUR     	180			//三个4点
	#define  Multiple_CT_TRIPLE_FIVE     	180			//三个5点
	#define  Multiple_CT_TRIPLE_SIX      	180			//三个6点

//////////////////////////////////////////////////////////////////////////


//游戏逻辑
class CDiceLogic : public CPokerLogic
{
	//函数定义
public:
	//构造函数
	CDiceLogic();
	//析构函数
	virtual ~CDiceLogic();

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

	//变量定义
public:
	static const BYTE m_cbDiceData[DICE_POINT_COUNT];
	BYTE m_nDice[DICE_COUNT];					//三个骰子
	int m_nCountNum[DICE_POINT_COUNT];
	int m_nSumPoint;						   //点数和
	enDiceType m_enBigSmall;                   //大、小


public:
	enDiceType m_enNumber_1;                   //1点骰子个数
	enDiceType m_enNumber_2;                   //2点骰子个数
	enDiceType m_enNumber_3;                   //3点骰子个数
	enDiceType m_enNumber_4;                   //4点骰子个数
	enDiceType m_enNumber_5;                   //5点骰子个数
	enDiceType m_enNumber_6;                   //6点骰子个数

	int m_nCompensateRatio[AREA_COUNT];		   //赔率

public:
	bool							m_bIsControlDice;
	BYTE							m_cbControlDice[DICE_COUNT];				//控制骰子
	void ResetGameData();

	//逻辑函数
public:
	//打乱骰子
	void RandDice(BYTE cbDiceBuffer[], BYTE cbPointNumber);
	//摇骰子
	void ShakeRandDice(BYTE cbDiceBuffer[], BYTE cbDiceCount);
	//计算骰子结果
	void ComputeDiceResult();

	//获取点数型
	enDiceType GetDicePoint(BYTE nDiceArray[]);

	//获取大小
	enDiceType GetBigSmall(BYTE nDiceArray[]);

	//围骰
	bool IsWaidice();

	//双骰子
	bool IsCoupleDice(BYTE cbDiceData[]);

	//获取三个相同骰子类型
	enDiceType GetThreeSameDice(BYTE nDiceArray[]);

	//获取相同点数的骰子个数
	void CountSameDice(BYTE nDiceArray[]);

	bool CompareHitPrize(int nBetIndex,int &nMultiple);

};

//////////////////////////////////////////////////////////////////////////
};
#endif
