
#ifndef GAME_TWOEIGHT_LOGIC_HEAD_FILE
#define GAME_TWOEIGHT_LOGIC_HEAD_FILE
#include "svrlib.h"
#include "json/json.h"

namespace game_twoeight
{
//////////////////////////////////////////////////////////////////////////
#define CARD_COUNT 40 // 牌的总数量
#define MAX_SEAT_INDEX 4 // 牌位
#define SINGLE_CARD_NUM 2 // 每组牌有多少张牌

// 牌型和牌型倍数定义
enum emCardType {
	emCardTypeError        = 0,	// 错误类型
	emCardTypePoint        = 1,	// 点数类型7点以下
	emCardTypePoint8       = 2,	// 点数类型8-9点
	emCardTypeTwoeight     = 3,	// 二八杠
	emCardTypeLeopard      = 4,	// 对子
	emCardTypeWhiteLeopard = 5,	// 白板对子
};

//游戏逻辑
struct TwoeightLogic {
	//函数定义

	// 解析牌类型对应的倍数
	// return : 最大牌类型的倍数
	static int ReAnalysisParam(Json::Value &jvalue);

	/// 控制函数 ///
	// 混乱牌
	static void RandCardList(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]);

	/// 逻辑函数 ///
	// 获取牌点
	static int GetCardPoint(const uint8 cbCardData[SINGLE_CARD_NUM])
	{ return (cbCardData[0] + cbCardData[1]) % 10; }
	//获取牌型
	static int GetCardType(const uint8 cbCardData[SINGLE_CARD_NUM]);

	// 大小比较
    // cbNextCardData>cbFirstCardData  返回1
    // cbNextCardData<cbFirstCardData  返回-1
    // cbNextCardData==cbFirstCardData 返回0
	// multiple : 赢的牌型的倍数
	static int CompareCard(const uint8 cbFirstCardData[SINGLE_CARD_NUM], const uint8 cbNextCardData[SINGLE_CARD_NUM], int &multiple);
};

//////////////////////////////////////////////////////////////////////////
};
#endif // GAME_TWOEIGHT_LOGIC_HEAD_FILE
