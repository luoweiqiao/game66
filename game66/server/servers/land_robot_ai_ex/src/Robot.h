#pragma once
#ifndef __ROBOT__	
#define __ROBOT__	
#include <vector>
const int ALL_CARDS_NUM = 54;

const int CARD_POINT_NUM = 16;

const int PLAYER_NUM = 3;

const int SOLO_CHAIN_CHARGE_LEN[8] = {7, 9, 11, 13, 15, 17, 18, 18};

const int LAIZI_TOTAL_COUNT = 4;

enum CardPoint
{
	CARD_3 = 0,
	CARD_4,
	CARD_5,
	CARD_6,
	CARD_7,
	CARD_8,
	CARD_9,
	CARD_T,
	CARD_J,
	CARD_Q,
	CARD_K,
	CARD_A,
	CARD_2,
	BLACK_JOKER,
	RED_JOKER,		//14	
	CARD_LZ,		//癞子
};


const char POINT_CHAR[CARD_POINT_NUM] = {'3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A', '2', 'X', 'D','L'};

 //命名冲突 移到包含有相应定义的cpp文件
enum HandType
{
	NOTHING,   
	TRIO_CHAIN_PAIR, // 飞机带对子翅膀
	TRIO_CHAIN_SOLO, // 飞机带单牌翅膀
	TRIO_CHAIN, // 三顺(飞机不带翅膀)
	PAIR_CHAIN, // 双顺
	SOLO_CHAIN, // 单顺
	TRIO_PAIR, // 三带对
	TRIO_SOLO, // 三带单
	TRIO, // 三张
	PAIR, // 对子
	SOLO, // 单牌
	FOUR_DUAL_PAIR,  // 四带对	
	FOUR_DUAL_SOLO,  // 四带单	
	LZBOMB,	//癞子炸弹
	//RUANBOMB，//软炸,之后有时间填上
	BOMB,  // 炸弹 
	NUKE,  // 火箭
	LZ,//癞子
};

enum Position
{
	LORD,  
	UPFARMER,  
	DOWNFARMER,
};


enum HandType;
enum Position;

struct CardsInfo
{
	int points[CARD_POINT_NUM];
	int total;  //总共有多少张牌
	int control;
};

enum HandProp
{
	NORMAL,
	HIGHEST,
	UNI_HIGHEST,	
};

struct Hand
{
	HandType type;  // 牌型
	CardPoint keyPoint;  // 套牌标签
	int len;  
	CardPoint kicker[5];
	HandProp prob;
	int laizi_num;
	std::vector<int> m_vecReplaceCards;
};

struct HandsMapSummary
{
	int realHandsCount; //总套数，不包括炸弹
	int unChargeHandsCount; //非冲锋套数
	int extraBombCount; //非控制牌组成的炸弹数
	int twoControlsHandsCount; //有多张控制牌的套数
	int effectiveHandsCount; //有效套数
	int handsTypeCount; //牌型种类，不包括炸弹，单顺区分长度
	int soloCount; //单牌数
	CardPoint lowestSolo; //最小单牌
	CardPoint sencondLowestSolo; //次小单牌
};

struct HandsMapSummaryLvl
{
	int realHandsCount; //非炸弹套数和
	int bombCount; //炸弹数
	int effectiveHandsCount; //有效套数
	int handsTypeCount; //牌型种类，不包括炸弹，单顺区分长度
	int soloCount; //单牌数
	CardPoint lowestSolo; //最小单牌
	CardPoint sencondLowestSolo; //次小单牌
};
#endif
