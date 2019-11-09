
#include "twoeight_logic.h"


namespace game_twoeight
{
	//扑克数据
	static const uint8 s_szCardListData[CARD_COUNT] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,	// 1筒-9筒，白板
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
	};

	vector<int> g_vecMultiple = {0, 1, 2, 3, 4, 5}; // 牌类型->牌倍数

	//////////////////////////////////////////////////////////////////////////

	// 值大小比较
	// nextCardValue>firstCardValue  返回1
	// nextCardValue<firstCardValue  返回-1
	// nextCardValue==firstCardValue 返回0
	int CompareCardValue(uint8 firstCardValue, uint8 nextCardValue) {
		if (firstCardValue < nextCardValue)
			return 1;
		if (firstCardValue > nextCardValue)
			return -1;
		return 0;
	}

	// 解析牌类型对应的倍数
	int TwoeightLogic::ReAnalysisParam(Json::Value &jvalue) {
		if (jvalue.isMember("mp1"))
			g_vecMultiple[1] = jvalue["mp1"].asInt();
		if (jvalue.isMember("mp2"))
			g_vecMultiple[2] = jvalue["mp2"].asInt();
		if (jvalue.isMember("mp3"))
			g_vecMultiple[3] = jvalue["mp3"].asInt();
		if (jvalue.isMember("mp4"))
			g_vecMultiple[4] = jvalue["mp4"].asInt();
		if (jvalue.isMember("mp5"))
			g_vecMultiple[5] = jvalue["mp5"].asInt();

		int maxMultiple = g_vecMultiple[5];
		for (int i = 1; i < 5; ++i)
			if (g_vecMultiple[i] > maxMultiple)
				maxMultiple = g_vecMultiple[i];
		LOG_DEBUG("maxMultiple:%d, g_vecMultiple:%d-%d-%d-%d-%d", maxMultiple, g_vecMultiple[1],
			g_vecMultiple[2], g_vecMultiple[3], g_vecMultiple[4], g_vecMultiple[5]);
		return maxMultiple;
	}

	// 混乱牌
	void TwoeightLogic::RandCardList(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]) {
		//混乱准备
		uint8 cbCardData[CARD_COUNT];
		memcpy(cbCardData, s_szCardListData, sizeof(s_szCardListData));

		uint8 cbCardBuffer[CARD_COUNT] = { 0 };

		//混乱扑克
		uint8 cbRandCount = 0, cbPosition = 0, cbBufferCount = MAX_SEAT_INDEX * SINGLE_CARD_NUM;
		do {
			cbPosition = g_RandGen.RandUInt() % (CARD_COUNT - cbRandCount);
			cbCardBuffer[cbRandCount++] = cbCardData[cbPosition];
			cbCardData[cbPosition] = cbCardData[CARD_COUNT - cbRandCount];
		} while (cbRandCount < cbBufferCount);

		memcpy(cbTableCardArray, cbCardBuffer, cbBufferCount);
		return;
	}

	//获取牌型
	int TwoeightLogic::GetCardType(const uint8 cbCardData[SINGLE_CARD_NUM]) {
		if (cbCardData[0] == cbCardData[1]) {
			if (cbCardData[0] % 10 == 0)
				return emCardTypeWhiteLeopard;
			return emCardTypeLeopard;
		}
		if ((cbCardData[0] == 2 && cbCardData[1] == 8) || (cbCardData[0] == 8 && cbCardData[1] == 2))
			return emCardTypeTwoeight;
		if (GetCardPoint(cbCardData) >= 8)
			return emCardTypePoint8;
		return emCardTypePoint;
	}

	// 大小比较
	int TwoeightLogic::CompareCard(const uint8 cbFirstCardData[SINGLE_CARD_NUM], const uint8 cbNextCardData[SINGLE_CARD_NUM], int &multiple) {
		// 获取牌型
		int cbFirstCardType = GetCardType(cbFirstCardData);
		int cbNextCardType = GetCardType(cbNextCardData);

		if (cbFirstCardType > cbNextCardType) {
			multiple = g_vecMultiple[cbFirstCardType];
			return -1;
		}
		if (cbFirstCardType < cbNextCardType) {
			multiple = g_vecMultiple[cbNextCardType];
			return 1;
		}
		if (cbFirstCardType == emCardTypeWhiteLeopard || cbFirstCardType == emCardTypeTwoeight)
			return 0;

		multiple = g_vecMultiple[cbFirstCardType];
		if (cbFirstCardType == emCardTypeLeopard)
			return CompareCardValue(cbFirstCardData[0], cbNextCardData[0]);
		// 获取牌组点数,再比较
		int pointFirst = GetCardPoint(cbFirstCardData);
		int pointNext = GetCardPoint(cbNextCardData);
		if (pointFirst == pointNext) {
			uint8 maxFirst = cbFirstCardData[0] % 10; // 白板的点数是0点，要对10取模
			uint8 iTemp = cbFirstCardData[1] % 10;
			if (maxFirst < iTemp)
				maxFirst = iTemp;
			uint8 maxNext = cbNextCardData[0] % 10;
			iTemp = cbNextCardData[1] % 10;
			if (maxNext < iTemp)
				maxNext = iTemp;
			return CompareCardValue(maxFirst, maxNext);
		}
		return CompareCardValue(pointFirst, pointNext);
	}
}