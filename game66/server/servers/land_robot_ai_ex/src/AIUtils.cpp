#ifdef ROBOT_DEBUG
#include <iostream>
#endif
#include <algorithm>
#include <numeric>
#include <set>
#include "AIUtils.h"

using namespace std;

//enum HandType
//{
//    NOTHING,
//    TRIO_CHAIN_PAIR, // 飞机带对子翅膀
//    TRIO_CHAIN_SOLO, // 飞机带单牌翅膀
//    TRIO_CHAIN, // 三顺(飞机不带翅膀)
//    PAIR_CHAIN, // 双顺
//    SOLO_CHAIN, // 单顺
//    TRIO_PAIR, // 三带对
//    TRIO_SOLO, // 三带单
//    TRIO, // 三张
//    PAIR, // 对子
//    SOLO, // 单牌
//    FOUR_DUAL_PAIR,  // 四带对	
//    FOUR_DUAL_SOLO,  // 四带单	
//    LZBOMB,	//癞子炸弹
//    //RUANBOMB，//软炸,之后有时间填上
//    BOMB,  // 炸弹 
//    NUKE,  // 火箭
//    LZ,//癞子
//};
//
//enum Position
//{
//    LORD,
//    UPFARMER,
//    DOWNFARMER,
//};

double AIUtils::getRandomDouble()
{
	return (double)rand() / (RAND_MAX + 1);
}


int AIUtils::cardToPoint( int card )
{
	int point = -1;
	if (card >= 0 && card <= 51)
	{
		point = card % 13; 
	}
	else if (card == 52 || card == 53)
	{
		point = card - 52 + 13;
	}
	return point;
}

void AIUtils::cardsAdd( int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM], int ret[CARD_POINT_NUM] )
{
	for (int i=0; i<CARD_POINT_NUM; ++i)
	{
		ret[i] = c0[i] + c1[i];
	}
}

void AIUtils::pointsSub( int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM], int ret[CARD_POINT_NUM] )
{
	for (int i=0; i<CARD_POINT_NUM; ++i)
	{
		ret[i] = c0[i] - c1[i];
	}
}

void AIUtils::pointsSubEqual( int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM] )
{
	for (int i=0; i<CARD_POINT_NUM; ++i)
	{
		c0[i] -= c1[i];
	}
}

void AIUtils::cardVecToPointArr(std::vector<int> &cardVec, int poinArr[CARD_POINT_NUM])
{
	fill(poinArr, poinArr + CARD_POINT_NUM, 0);
	for (unsigned i=0; i<cardVec.size(); ++i)
	{
		++poinArr[cardToPoint(cardVec[i])];
	}
}

int AIUtils::countNoLowerThanCard( CardPoint point, int cards[CARD_POINT_NUM] )
{
	return accumulate(cards + point, cards + CARD_POINT_NUM, 0);
}

int AIUtils::getCardNumber( Hand &hand )
{
	int ret = 0;
	switch (hand.type)
	{
	case SOLO:
		ret = 1;
		break;
	case PAIR:
	case NUKE:
		ret = 2;
		break;
	case TRIO:
		ret = 3;
		break;
	case BOMB:
	case TRIO_SOLO:
		ret = 4;
		break;
	case TRIO_PAIR:
		ret = 5;
		break;
	case FOUR_DUAL_SOLO:
		ret = 6;
		break;
	case FOUR_DUAL_PAIR:
		ret = 8;
		break;
	case SOLO_CHAIN:
		ret = hand.len;
		break;
	case PAIR_CHAIN:
		ret = 2 * hand.len;
		break;
	case TRIO_CHAIN:
		ret = 3 * hand.len;
		break;
	case TRIO_CHAIN_SOLO:
		ret = 4 * hand.len;
		break;
	case TRIO_CHAIN_PAIR:
		ret = 5 * hand.len;
	}
	return ret;
}

bool AIUtils::isChargeHand( Hand &hand, int maxOppCardNum, int oppCards[CARD_POINT_NUM], map<HandType, vector<Hand>> *handMap )
{
	bool ret = false;
	int num = getCardNumber(hand);
	if (num > maxOppCardNum)
	{
		ret = true;
	}
	else
	{
		if (hand.type == SOLO)
		{
			ret = true;
			for (int i=RED_JOKER; i>hand.keyPoint; --i)
			{
				if (oppCards[i] >= 1)
				{
					ret = false;
					break;
				}
			}
		}
		else if (hand.type == PAIR)
		{
			ret = true;
			for (int i=CARD_2; i>hand.keyPoint; --i)
			{
				if (oppCards[i] >= 2)
				{
					ret = false;
					break;
				}
			}
		}
		else if (hand.type == TRIO || hand.type == TRIO_SOLO || hand.type == TRIO_PAIR)
		{
			ret = true;
			for (int i=CARD_2; i>hand.keyPoint; --i)
			{
				if (oppCards[i] >= 3)
				{
					ret = false;
					break;
				}
			}
		}
		else if (hand.type == SOLO_CHAIN)
		{
			if (maxOppCardNum < SOLO_CHAIN_CHARGE_LEN[hand.len - 5])
			{
				ret = true;
			}
			else
			{
				int start = hand.keyPoint + 1;
				int maxKey = CARD_A - hand.len + 1;
				int maxLen = 0;
				while (start <= maxKey)
				{
					int end = start;
					while (end <= CARD_A && oppCards[end] > 0)
					{
						++end;
					}
					int len = end - start;
					if (len > maxLen)
					{
						maxLen = len;
					}
					++start;
				}
				ret = hand.len > maxLen;
			}
		} 
		else
		{
			ret = true;
		}
	}

	if (handMap != NULL && !ret 
		&& (hand.type == TRIO || hand.type == TRIO_SOLO || hand.type == TRIO_PAIR || hand.type == SOLO_CHAIN))
	{
		vector<CardPoint> typeKeyPoints;
		int oppHighestKey;
		if (hand.type == SOLO_CHAIN)
		{
			vector<Hand> &soloChainHands = (*handMap)[SOLO_CHAIN];
			for (unsigned i=0; i<soloChainHands.size(); ++i)
			{
				if (soloChainHands[i].len == hand.len)
				{
					typeKeyPoints.push_back(soloChainHands[i].keyPoint);
				}
			}
			if (typeKeyPoints.size() > 0)
			{
				oppHighestKey = getHighestSoloChain(oppCards, hand.len);
			}
		}
		else
		{
			vector<Hand> &trioHands = (*handMap)[TRIO];
			for (unsigned i=0; i<trioHands.size(); ++i)
			{
				typeKeyPoints.push_back(trioHands[i].keyPoint);
			}
			vector<Hand> &trioSoloHands = (*handMap)[TRIO_SOLO];
			for (unsigned i=0; i<trioSoloHands.size(); ++i)
			{
				typeKeyPoints.push_back(trioSoloHands[i].keyPoint);
			}
			vector<Hand> &trioPairHands = (*handMap)[TRIO_PAIR];
			for (unsigned i=0; i<trioPairHands.size(); ++i)
			{
				typeKeyPoints.push_back(trioPairHands[i].keyPoint);
			}
			if (typeKeyPoints.size() > 0)
			{
				oppHighestKey = getHighestTrio(oppCards);
			}
		}
		if (typeKeyPoints.size() > 0)
		{
			unsigned highetCount = 0;
			for (unsigned i=0; i<typeKeyPoints.size(); ++i)
			{
				if (typeKeyPoints[i] > oppHighestKey)
				{
					++highetCount;
				}
			}
			for (unsigned i=0; i<highetCount; ++i)
			{
				if (hand.keyPoint <= typeKeyPoints[i])
				{
					ret = true;
					break;
				}
			}
		}
	}
	return ret;
}

void AIUtils::getBomb( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &hands = handMap[BOMB];
	if (cards[BLACK_JOKER] == 1 && cards[RED_JOKER] == 1)
	{
		Hand hand;
		hand.type = NUKE;
		hand.keyPoint = BLACK_JOKER;
		hands.push_back(hand);
		cards[BLACK_JOKER] = 0;
		cards[RED_JOKER] = 0;
	}
	for (int i=CARD_3; i<=CARD_2; ++i)
	{
		if (cards[i] == 4)
		{
			Hand hand;
			hand.type = BOMB;
			hand.keyPoint = CardPoint(i);
			hands.push_back(hand);
			cards[i] = 0;
		}
	}
}

void AIUtils::getTrio( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &hands = handMap[TRIO];
	for (int i=CARD_3; i<=CARD_2; ++i)
	{
		if (cards[i] == 3)
		{
			Hand hand;
			hand.type = TRIO;
			hand.keyPoint = CardPoint(i);
			hands.push_back(hand);
			cards[i] = 0;
		}
	}
}

void AIUtils::getTrioChain( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &hands = handMap[TRIO_CHAIN];
	int start;
	int end;
	int i = CARD_3;
	while (i <= CARD_K)
	{
		if (cards[i] >= 3)
		{
			start = i;
			do 
			{
				end = i;
				++i;
			} while (i <= CARD_A && cards[i] >= 3);
			if (end - start >= 1)
			{
				Hand hand;
				hand.type = TRIO_CHAIN;
				hand.keyPoint = CardPoint(start);
				hand.len = end - start + 1;
				hands.push_back(hand);

				for (int j=start; j<=end; ++j)
				{
					cards[j] -= 3;
				}
			}
		}
		++i;
	}
}

void AIUtils::getPairChain( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &hands = handMap[PAIR_CHAIN];
	int start;
	int end;
	int i = CARD_3;
	while (i <= CARD_Q)
	{
		if (cards[i] >= 2)
		{
			start = i;
			do 
			{
				end = i;
				++i;
			} while (i <= CARD_A && cards[i] >= 2);
			if (end - start >= 2)
			{
				Hand hand;
				hand.type = PAIR_CHAIN;
				hand.keyPoint = CardPoint(start);
				hand.len = end - start + 1;

				optimizePairChain(cards, hand);
				hands.push_back(hand);

				for (int j=0; j<=hand.len; ++j)
				{
					cards[hand.keyPoint + j] -= 2;
				}
			}
		}
		++i;
	}
}

void AIUtils::optimizePairChain(int cards[CARD_POINT_NUM], Hand &hand)
{
	while (hand.len > 3)
	{
		int start = hand.keyPoint;
		int end = hand.keyPoint + hand.len - 1;
		if (cards[end] > cards[start])
		{
			--hand.len;
		}
		else if (cards[start] > 2)
		{
			hand.keyPoint = CardPoint(hand.keyPoint + 1);
			--hand.len;
		}
		else
		{
			break;
		}
	}
}


void AIUtils::getSoloChain( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &hands = handMap[SOLO_CHAIN];
	int start;
	int end;
	int i = CARD_3;
	while (i <= CARD_T)
	{
		if (cards[i] >= 1)
		{
			start = i;
			end = start;
			while (end + 1 <= CARD_A && cards[end + 1] >= 1 && end - start < 4)
			{
				++end;
			} 
			if (end - start >= 4)
			{
				Hand hand;
				hand.type = SOLO_CHAIN;
				hand.keyPoint = CardPoint(start);
				hand.len = end - start + 1;
				hands.push_back(hand);
				for (int j=start; j<=end; ++j)
				{
					--cards[j];
				}
			}
			else
			{
				i = end + 1;
			}
		}
		else
		{
			++i;
		}
	}
	expandSoloChain(cards, hands);
	optimizeSoloChain(cards, hands);
}

void AIUtils::expandSoloChain( int cards[CARD_POINT_NUM], std::vector<Hand> &soloChains )
{
	for (vector<Hand>::iterator it=soloChains.begin(); it!=soloChains.end(); ++it)
	{
		int newEnd = it->keyPoint + it->len;
		while (newEnd <= CARD_A && cards[newEnd] > 0)
		{
			++(it->len);
			--cards[newEnd];
			++newEnd;
		}
	}
	for (vector<Hand>::iterator it=soloChains.begin(); it!=soloChains.end(); ++it)
	{
		vector<Hand>::iterator newIt = it + 1;
		while (newIt != soloChains.end())
		{
			if (it->keyPoint + it->len == newIt->keyPoint)
			{
				it->len += newIt->len;
				soloChains.erase(newIt);
				break;
			}
			else
			{
				++newIt;
			}
		}
	}
}

void AIUtils::optimizeSoloChain( int cards[CARD_POINT_NUM], std::vector<Hand> &soloChains )
{
	for (vector<Hand>::iterator it=soloChains.begin(); it!=soloChains.end(); ++it)
	{
		int start = it->keyPoint;
		int end = it->keyPoint + it->len - 1;
		int maxPairLen = it->len - 5;
		if (maxPairLen >= 3)
		{
			int ll = 0;
			int hl = 0;
			while (ll < maxPairLen && cards[start + ll] > 0)
			{
				++ll;
			}
			while (hl < maxPairLen && cards[end - hl] > 0)
			{
				++hl;
			}
			int newStart = start;
			int newEnd = end;
			if (ll >= 3 && (ll >= hl || ll + hl <= maxPairLen))
			{
				newStart = start + ll;
				for (int i=start; i<=newStart-1; ++i)
				{
					++cards[i];
				}
			}
			if (hl >=3 && (hl > ll || ll + hl <= maxPairLen))
			{
				newEnd = end - ll;
				for (int i=newEnd+1; i<=end; ++i)
				{
					++cards[i];
				}
			}
			it->keyPoint = CardPoint(newStart);
			it->len = newEnd - newStart + 1;
		}
	}
	for (vector<Hand>::iterator it=soloChains.begin(); it!=soloChains.end(); ++it)
	{
		int start = it->keyPoint;
		int end = it->keyPoint + it->len - 1;
		int newStart = start;
		int newEnd = end;
		while (newEnd - newStart >= 5)
		{
			if (cards[newStart] > 0 && cards[newStart] >= cards[newEnd])
			{
				++newStart;
			}
			else if (cards[newEnd] > cards[newStart])
			{
				--newEnd;
			}
			else
			{
				break;
			}
		}
		for (int i=start; i<=newStart-1; ++i)
		{
			++cards[i];
		}
		for (int i=newEnd+1; i<=end; ++i)
		{
			++cards[i];
		}
		it->keyPoint = CardPoint(newStart);
		it->len = newEnd - newStart + 1;
	}
}

void AIUtils::getNormalHand( int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap )
{
	if (cards[BLACK_JOKER] == 1 && cards[RED_JOKER] == 1)
	{
		Hand hand;
		hand.type = NUKE;
		hand.keyPoint = BLACK_JOKER;
		handMap[BOMB].push_back(hand);
		cards[BLACK_JOKER] = 0;
		cards[RED_JOKER] = 0;
	}
	for (int i=CARD_3; i<=RED_JOKER; ++i)
	{
		if (cards[i] == 4)
		{
			Hand hand;
			hand.type = BOMB;
			hand.keyPoint = CardPoint(i);
			handMap[BOMB].push_back(hand);
			cards[i] = 0;
		}
		else if (cards[i] == 3)
		{
			Hand hand;
			hand.type = TRIO;
			hand.keyPoint = CardPoint(i);
			handMap[TRIO].push_back(hand);
			cards[i] = 0;
		}
		if (cards[i] == 2)
		{
			Hand hand;
			hand.type = PAIR;
			hand.keyPoint = CardPoint(i);
			handMap[PAIR].push_back(hand);
			cards[i] = 0;
		}
		if (cards[i] == 1)
		{
			Hand hand;
			hand.type = SOLO;
			hand.keyPoint = CardPoint(i);
			handMap[SOLO].push_back(hand);
			cards[i] = 0;
		}
	}
}

void AIUtils::splitCardsToHandsKind1(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap )
{
	handMap.clear();
	getSoloChain(cards, handMap);
	getBomb(cards, handMap);
	getTrioChain(cards, handMap);	
	getPairChain(cards, handMap);
	getNormalHand(cards, handMap);
	if (searchKicker)
	{
		findKickerForTrioOrTrioChain(handMap);
	}
}

void AIUtils::splitCardsToHandsKind2( int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap )
{
	handMap.clear();
	getBomb(cards, handMap);
	getTrioChain(cards, handMap);
	getSoloChain(cards, handMap);
	getPairChain(cards, handMap);
	getNormalHand(cards, handMap);
	if (searchKicker)
	{
		findKickerForTrioOrTrioChain(handMap);
	}
}

void AIUtils::splitCardsToHandsKind3( int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap )
{
	handMap.clear();
	getSoloChain(cards, handMap);
	getBomb(cards, handMap);
	getPairChain(cards, handMap);
	getTrioChain(cards, handMap);	
	getNormalHand(cards, handMap);
	if (searchKicker)
	{
		findKickerForTrioOrTrioChain(handMap);
	}
}

void AIUtils::sortHandsWithBestKind(int aiPoints[CARD_POINT_NUM], 
								   CardPoint lowestControl, 
								   int maxOppNumber, 
								   int otherPoints[CARD_POINT_NUM],
								   bool searchKicker,
								   std::map<HandType, std::vector<Hand>> &handsMap,
								   HandsMapSummary &summary)
{
	int cardsCopy[CARD_POINT_NUM];
	map<HandType, vector<Hand>> tmpMap;
	HandsMapSummary tmpSummary;

	copy(aiPoints, aiPoints + CARD_POINT_NUM, cardsCopy);
	splitCardsToHandsKind1(cardsCopy, searchKicker, handsMap);
	summary = getHandsMapSummary(handsMap, lowestControl, maxOppNumber, otherPoints);

	copy(aiPoints, aiPoints + CARD_POINT_NUM, cardsCopy);
	splitCardsToHandsKind2(cardsCopy, searchKicker, tmpMap);
	tmpSummary = getHandsMapSummary(tmpMap, lowestControl, maxOppNumber, otherPoints);
	if (!compareHandMapSummary(summary, tmpSummary))
	{
		handsMap = tmpMap;
		summary = tmpSummary;
	}

	copy(aiPoints, aiPoints + CARD_POINT_NUM, cardsCopy);
	splitCardsToHandsKind3(cardsCopy, searchKicker, tmpMap);
	tmpSummary = getHandsMapSummary(tmpMap, lowestControl, maxOppNumber, otherPoints);
	if (!compareHandMapSummary(summary, tmpSummary))
	{
		handsMap = tmpMap;
		summary = tmpSummary;
	}
}

void AIUtils::splitCardsToHandsKind4( int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap )
{
	handMap.clear();
	getSoloChain(cards, handMap);
	getBomb(cards, handMap);
	getNormalHand(cards, handMap);
	if (searchKicker)
	{
		findKickerForTrioOrTrioChain(handMap);
	}
}

void AIUtils::splitCardsToHandsKind5( int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap )
{
	handMap.clear();
	getBomb(cards, handMap);
	getTrio(cards, handMap);
	getSoloChain(cards, handMap);
	getNormalHand(cards, handMap);
	if (searchKicker)
	{
		findKickerForTrioOrTrioChain(handMap);
	}
}

void AIUtils::sortHandsLordOnly1Card( int points[CARD_POINT_NUM], 
									 CardPoint lowestControl, 
									 int maxOppNumber, 
									 int otherPoints[CARD_POINT_NUM],
									 bool searchKicker, 
									 std::map<HandType, std::vector<Hand>> &handsMap, 
									 HandsMapSummary &summary )
{
	int cardsCopy[CARD_POINT_NUM];
	map<HandType, vector<Hand>> tmpMap;
	HandsMapSummary tmpSummary;

	copy(points, points + CARD_POINT_NUM, cardsCopy);
	splitCardsToHandsKind4(cardsCopy, searchKicker, handsMap);
	summary = getHandsMapSummary(handsMap, lowestControl, maxOppNumber, otherPoints);

	copy(points, points + CARD_POINT_NUM, cardsCopy);
	splitCardsToHandsKind5(cardsCopy, searchKicker, tmpMap);
	tmpSummary = getHandsMapSummary(tmpMap, lowestControl, maxOppNumber, otherPoints);
	if (tmpSummary.soloCount < summary.soloCount
		|| (tmpSummary.soloCount == summary.soloCount && tmpSummary.lowestSolo > summary.lowestSolo))
	{
		handsMap = tmpMap;
		summary = tmpSummary;
	}
}

HandsMapSummary AIUtils::getHandsMapSummary( std::map<HandType, std::vector<Hand>> &handMap, CardPoint lowestControl, int maxOppCardNum, int oppCards[CARD_POINT_NUM])
{
	HandsMapSummary summary = {0, 0, 0, 0, 0, 0, 0, RED_JOKER, RED_JOKER};
	for (map<HandType, vector<Hand>>::iterator it=handMap.begin(); it!=handMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		if (it->first == BOMB)
		{
			for (unsigned i=0; i<hands.size(); ++i)
			{
				if (hands[i].keyPoint < lowestControl)
				{
					++summary.extraBombCount;
				}
			}
		}
		else
		{
			if (it->first == SOLO_CHAIN)
			{
				set<int> chainLength;
				for (unsigned i=0; i<hands.size(); ++i)
				{
					chainLength.insert(hands[i].len);
				}
				summary.handsTypeCount += chainLength.size();
			}
			else
			{
				++summary.handsTypeCount;
				if (it->first == SOLO)
				{
					if (hands.size() >= 1)
					{
						summary.lowestSolo = hands[0].keyPoint;
					}
					if (hands.size() >= 2)
					{
						summary.sencondLowestSolo = hands[1].keyPoint;
					}
				}
			}

			summary.realHandsCount += hands.size();
			for (unsigned i=0; i<hands.size(); ++i)
			{
				if (!isChargeHand(hands[i], maxOppCardNum, oppCards, &handMap))
				{
					++summary.unChargeHandsCount;
				}
				int controlNum = calHandControl(hands[i], lowestControl);
				if (controlNum >= 2)
				{
					++summary.twoControlsHandsCount;
				}			
			}
		}
	}
	summary.effectiveHandsCount = summary.unChargeHandsCount + summary.twoControlsHandsCount - summary.extraBombCount;
	return summary;
}

void AIUtils::findKickerForTrioOrTrioChain( std::map<HandType, std::vector<Hand>> &handMap )
{
	vector<Hand> &trioChains = handMap[TRIO_CHAIN];
	while (!trioChains.empty())
	{
		Hand tChain = trioChains.front();
		int usefulSoloCount = 0;
		int jokerCount = 0;
		for (unsigned i=0; i<handMap[SOLO].size(); ++i)
		{
			if (handMap[SOLO][i].keyPoint < tChain.keyPoint
				|| handMap[SOLO][i].keyPoint > tChain.keyPoint + tChain.len - 1)
			{
				++usefulSoloCount;
			}
			if (handMap[SOLO][i].keyPoint == BLACK_JOKER
				|| handMap[SOLO][i].keyPoint == RED_JOKER)
			{
				++jokerCount;
			}
		}
		if (jokerCount == 2)
		{
			usefulSoloCount -= 2;
		}

		if (usefulSoloCount >= tChain.len)
		{
			vector<CardPoint> k;
			vector<Hand>::iterator it = handMap[SOLO].begin();
			while ((int)k.size() < tChain.len && it != handMap[SOLO].end())
			{
				if (it->keyPoint < tChain.keyPoint
					|| it->keyPoint > tChain.keyPoint + tChain.len - 1)
				{
					k.push_back(it->keyPoint);
					it = handMap[SOLO].erase(it);
				}
				else
				{
					++it;
				}
			}
			for (unsigned i=0; i<k.size(); ++i)
			{
				tChain.kicker[i] = k[i];
			}
			tChain.type = TRIO_CHAIN_SOLO;
			handMap[TRIO_CHAIN_SOLO].push_back(tChain);
			trioChains.erase(trioChains.begin());
		}
		else if ((int)handMap[PAIR].size() >= tChain.len)
		{
			vector<CardPoint> k;
			vector<Hand>::iterator it = handMap[PAIR].begin();
			while ((int)k.size() < tChain.len && it != handMap[PAIR].end())
			{
				k.push_back(it->keyPoint);
				it = handMap[PAIR].erase(it);
			}
			for (unsigned i=0; i<k.size(); ++i)
			{
				tChain.kicker[i] = k[i];
			}
			tChain.type = TRIO_CHAIN_PAIR;
			handMap[TRIO_CHAIN_PAIR].push_back(tChain);
			trioChains.erase(trioChains.begin());
		}
		else
		{
			break;
		}
	}

	vector<Hand> &trios = handMap[TRIO];
	while (!trios.empty())
	{
		if (handMap[SOLO].size() == 0 && handMap[PAIR].size() == 0)
		{
			break;
		}
		HandType kickerType = SOLO;
		if (handMap[SOLO].size() == 0
			|| (handMap[PAIR].size() > 0 && handMap[PAIR][0].keyPoint < handMap[SOLO][0].keyPoint))
		{
			kickerType = PAIR;
		}
		Hand trio = trios.front();
		if (kickerType == SOLO)
		{
			trio.type = TRIO_SOLO;
			trio.kicker[0] = handMap[SOLO][0].keyPoint;
			handMap[SOLO].erase(handMap[SOLO].begin());
			handMap[TRIO_SOLO].push_back(trio);
		}
		else
		{
			trio.type = TRIO_PAIR;
			trio.kicker[0] = handMap[PAIR][0].keyPoint;
			handMap[PAIR].erase(handMap[PAIR].begin());
			handMap[TRIO_PAIR].push_back(trio);
		}
		handMap[TRIO].erase(handMap[TRIO].begin());
	}
}

void AIUtils::findHigherHandFromPoints(Hand &hand, 
									   int points[CARD_POINT_NUM], 
									   CardPoint lowestControl, 
									   int maxOppNumber, 
									   int otherPoints[CARD_POINT_NUM],
									   Hand *higher)
{
	higher->type = NOTHING;
	int copyPoints[CARD_POINT_NUM];
	copy(points, points + CARD_POINT_NUM, copyPoints);
	if (isChain(hand.type))
	{
		int number;
		switch (hand.type)
		{
		case SOLO_CHAIN:
			number = 1;
			break;
		case PAIR_CHAIN:
			number = 2;
			break;
		default:
			number = 3;
		}
		CardPoint higherKey = CARD_3;
		int i = hand.keyPoint + 1;
		int maxKey = CARD_A - hand.len + 1;
		int start;
		int end;
		while (i <= maxKey)
		{
			if (points[i] >= number)
			{
				start = i;
				do 
				{
					end = i;
					if (end - start == hand.len - 1)
					{
						break;
					}
					++i;
				} while (i <= CARD_A && points[i] >= number);

				if (end - start == hand.len - 1)
				{
					for (int j=start; j<=end; ++j)
					{
						copyPoints[j] -= number;
					}
					higherKey = CardPoint(start);
					break;
				}
			}
			++i;
		}
		if (higherKey > CARD_3) //find chain
		{
			higher->type = hand.type;
			higher->keyPoint = higherKey;
			higher->len = hand.len;
		}
	}
	else
	{
		int number;
		switch (hand.type)
		{
		case SOLO:
			number = 1;
			break;
		case PAIR:
			number = 2;
			break;
		case TRIO:
		case TRIO_SOLO:
		case TRIO_PAIR:
			number = 3;
			break;
		default:
			number = 4;
			break;
		}
		for (int i=hand.keyPoint + 1; i<=RED_JOKER; ++i)
		{
			if (copyPoints[i] >= number)
			{
				copyPoints[i] -= number;

				higher->type = hand.type;
				higher->keyPoint = CardPoint(i);
				break;
			}
		}
	}
	if (higher->type == TRIO_SOLO || higher->type == TRIO_PAIR
		|| higher->type == TRIO_CHAIN_SOLO || higher->type == TRIO_CHAIN_PAIR
        || higher->type == FOUR_DUAL_SOLO || higher->type == FOUR_DUAL_PAIR)
	{
		bool hasKicker = findKickerFromPoints(copyPoints, lowestControl, maxOppNumber, otherPoints, higher);
		if (!hasKicker)
		{
			higher->type = NOTHING;
		}
	}
}

bool AIUtils::findKickerFromPoints(int points[CARD_POINT_NUM],
								   CardPoint lowestControl, 
								   int maxOppNumber, 
								   int otherPoints[CARD_POINT_NUM], 
								   Hand *hand)
{
	map<HandType, vector<Hand>> handsMap;
	HandsMapSummary summary;
	sortHandsWithBestKind(points,
		lowestControl,
		maxOppNumber,
		otherPoints,
		false,
		handsMap,
		summary);
	if (hand->type == TRIO_SOLO)
	{
		vector<Hand> &soloHands = handsMap[SOLO];
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint != hand->keyPoint)
			{
				hand->kicker[0] = soloHands[i].keyPoint;
				return true;
			}
		}
		vector<Hand> &soloChainHands = handsMap[SOLO_CHAIN];
		if (soloChainHands.size() >= 2)
		{
			for (unsigned i=0; i<soloChainHands.size(); ++i)
			{
				for (unsigned j=i+1; j<soloChainHands.size(); ++j)
				{
					if (soloChainHands[i].keyPoint + soloChainHands[i].len - 1
						== soloChainHands[j].keyPoint)
					{
						if (soloChainHands[j].keyPoint != hand->keyPoint)
						{
							hand->kicker[0] = soloChainHands[j].keyPoint;
							return true;
						}
					}
				}
			}
		}
		for (unsigned i=0; i<soloChainHands.size(); ++i)
		{
			if (soloChainHands[i].len > 5)
			{
				if (soloChainHands[i].keyPoint != hand->keyPoint)
				{
					hand->kicker[0] = soloChainHands[i].keyPoint;
				}
				else
				{
					hand->kicker[0] = CardPoint(soloChainHands[i].keyPoint + soloChainHands[i].len - 1);
				}				
				return true;
			}
		}
		for (int i=CARD_3; i<=RED_JOKER; ++i)
		{
			if (points[i] > 0 && i != hand->keyPoint)
			{
				hand->kicker[0] = CardPoint(i);
				return true;
			}
		}
	}
	else if (hand->type == TRIO_PAIR)
	{
		vector<Hand> &pairHands = handsMap[PAIR];
		if (pairHands.size() > 0)
		{
			hand->kicker[0] = pairHands[0].keyPoint;
			return true;
		}
		vector<Hand> &pairChainHands = handsMap[PAIR_CHAIN];
		for (unsigned i=0; i<pairChainHands.size(); ++i)
		{
			if (pairChainHands[0].len > 3)
			{
				hand->kicker[0] = pairChainHands[0].keyPoint;
				return true;
			}
		}
		for (int i=CARD_3; i<=CARD_2; ++i)
		{
			if (points[i] >= 2 && i != hand->keyPoint)
			{
				hand->kicker[0] = CardPoint(i);
				return true;
			}
		}
	}
	else if (hand->type == TRIO_CHAIN_SOLO || hand->type == FOUR_DUAL_SOLO) //加入四带单
	{
        int kickerNum = hand->len;
        if (hand->type == FOUR_DUAL_SOLO)
        {
            hand->len = 1;
            kickerNum = 2;
        }        

		vector<int> k;
		vector<Hand> &soloHands = handsMap[SOLO];
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint < hand->keyPoint 
				|| soloHands[i].keyPoint > hand->keyPoint + hand->len - 1)
			{
				k.push_back(soloHands[i].keyPoint);
			}
		}
		if (k.size() < (unsigned)kickerNum)
		{
			vector<Hand> &soloChainHands = handsMap[SOLO_CHAIN];
			for (unsigned i=0; i<soloChainHands.size(); ++i)
			{
                while (k.size() < (unsigned)kickerNum && soloChainHands[i].len > 5)
				{
					if (soloChainHands[i].keyPoint < hand->keyPoint
						|| soloHands[i].keyPoint > hand->keyPoint + hand->len - 1)
					{
						k.push_back(soloChainHands[i].keyPoint);
						soloChainHands[i].keyPoint = CardPoint(soloChainHands[i].keyPoint + 1);
						--soloChainHands[i].len;
					}
					else
					{
						k.push_back(CardPoint(soloChainHands[i].keyPoint + soloChainHands[i].len - 1));
						--soloChainHands[i].len;
					}				
				}
                if (k.size() >= (unsigned)kickerNum)
				{
					break;
				}
			}
		}
		for (int i=CARD_3; i<=RED_JOKER; ++i)
		{
            if (k.size() < (unsigned)kickerNum)
			{
				if (points[i] > 0)
				{
					k.push_back(i);
				}
			}
			else
			{
				break;
			}
		}
        if (k.size() >= (unsigned)kickerNum)
		{
            for (int i = 0; i<kickerNum; ++i)
			{
				hand->kicker[i] = CardPoint(k[i]);
			}
			return true;
		}
	}
	else if (hand->type == TRIO_CHAIN_PAIR || hand->type == FOUR_DUAL_PAIR)
	{
        int kickerNum = hand->len;
        if (hand->type == FOUR_DUAL_PAIR)
        {
            hand->len = 1;
            kickerNum = 2;
        }

		vector<int> k;
		vector<Hand> &pairHands = handsMap[PAIR];
		for (unsigned i=0; i<pairHands.size(); ++i)
		{
			k.push_back(pairHands[i].keyPoint);
		}
        if (k.size() < (unsigned)kickerNum)
		{
			vector<Hand> &pairChainHands = handsMap[PAIR_CHAIN];
			for (unsigned i=0; i<pairChainHands.size(); ++i)
			{
                while (k.size() < (unsigned)kickerNum && pairChainHands[0].len > 3)
				{
					k.push_back(pairChainHands[0].keyPoint);
					pairChainHands[i].keyPoint = CardPoint(pairChainHands[i].keyPoint + 1);
					--pairChainHands[i].len;
				}
			}
		}
		for (int i=CARD_3; i<=CARD_2; ++i)
		{
            if (find(k.begin(), k.end(), i) != k.end()) //检查重复
            {
                continue;
            }
            if (k.size() < (unsigned)kickerNum)
			{
				if (points[i] >= 2)
				{
					k.push_back(i);
				}
			}
			else
			{
				break;
			}
		}
        if (k.size() >= (unsigned)kickerNum)
		{
            for (int i = 0; i<kickerNum; ++i)
			{
				hand->kicker[i] = CardPoint(k[i]);
			}
			return true;
		}
	}
	return false;
}

CardPoint AIUtils::getLowestControl( int remainPoints[CARD_POINT_NUM] )
{
	int count = 0;
	CardPoint lowest = CARD_3;
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		count += remainPoints[i];
		if (count >= 6)
		{
			lowest = CardPoint(i);
			break;
		}
	}
	return lowest;
}

int AIUtils::calHandControl( Hand &hand, CardPoint lowestControl )
{
	int ret = 0;
	switch (hand.type)
	{
	case SOLO:
		ret = hand.keyPoint >= lowestControl ? 1 : 0;
		break;
	case PAIR:
		ret = hand.keyPoint >= lowestControl ? 2 : 0;
		break;
	case TRIO:
		ret = hand.keyPoint >= lowestControl ? 3 : 0;
		break;
	case TRIO_SOLO:
		ret = hand.keyPoint >= lowestControl ? 3 : 0;
		if (hand.kicker[0] >= lowestControl)
		{
			++ret;
		}
		break;
	case TRIO_PAIR:
		ret = hand.keyPoint >= lowestControl ? 3 : 0;
		if (hand.kicker[0] >= lowestControl)
		{
			ret += 2;
		}
		break;
	case FOUR_DUAL_SOLO:
		ret = hand.keyPoint >= lowestControl ? 4: 0;
		for (int i=0; i<2; ++i)
		{
			if (hand.kicker[i] >= lowestControl)
			{
				++ret;
			}
		}
		break;
	case FOUR_DUAL_PAIR:
		ret = hand.keyPoint >= lowestControl ? 4: 0;
		for (int i=0; i<2; ++i)
		{
			if (hand.kicker[i] >= lowestControl)
			{
				ret += 2;
			}
		}
		break;
	case SOLO_CHAIN:
		ret = max(hand.keyPoint + hand.len - lowestControl + 1, 0);
		break;
	case PAIR_CHAIN:
		ret = 2 * max(hand.keyPoint + hand.len - lowestControl + 1, 0);
		break;
	case TRIO_CHAIN:
		ret = 3 * max(hand.keyPoint + hand.len - lowestControl + 1, 0);
		break;
	case TRIO_CHAIN_SOLO:
		ret = 3 * max(hand.keyPoint + hand.len - lowestControl + 1, 0);
		for (int i=0; i<hand.len; ++i)
		{
			if (hand.kicker[i] >= lowestControl)
			{
				++ret;
			}
		}
		break;
	case TRIO_CHAIN_PAIR:
		ret = 3 * max(hand.keyPoint + hand.len - lowestControl + 1, 0);
		for (int i=0; i<hand.len; ++i)
		{
			if (hand.kicker[i] >= lowestControl)
			{
				ret += 2;
			}
		}
		break;	
	default:
		break;
	}
	return ret;
}

bool AIUtils::compareHandMapSummary(HandsMapSummary &summary0, HandsMapSummary &summary1)
{
	if (summary0.effectiveHandsCount < summary1.effectiveHandsCount)
	{
		return true;
	}
	else if (summary0.effectiveHandsCount > summary1.effectiveHandsCount)
	{
		return false;
	}
	else
	{
		if (summary0.handsTypeCount < summary1.handsTypeCount)
		{
			return true;
		}
		else if (summary0.handsTypeCount > summary1.handsTypeCount)
		{
			return false;
		}
		else
		{
			if (summary0.soloCount < summary1.soloCount)
			{
				return true;
			}
			else if (summary0.soloCount > summary1.soloCount)
			{
				return false;
			}
			else
			{
				return summary0.lowestSolo >= summary1.lowestSolo;
			}
		}
	}
	return false;
}

int AIUtils::calControl( int myPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM], CardPoint lowestControl)
{
	map<int, int> myCon;
	int myConNum = 0;
	map<int, int> oppCon;
	int oppConNum = 0;
	for (int i=lowestControl; i<=RED_JOKER; ++i)
	{
		myConNum += myPoints[i];
		myCon[i] = myPoints[i];
		oppConNum += oppPoints[i];
		oppCon[i] = oppPoints[i];
	}
	int ret = 0;
	while (myConNum > 0 && oppConNum > 0)
	{
		map<int, int> *curCon = &myCon;
		map<int, int> *lastCon = NULL;
		for (int i=lowestControl; i<=RED_JOKER; ++i)
		{
			if ((*curCon)[i] > 0)
			{
				lastCon = curCon;
				--(*curCon)[i];
				if (curCon == &myCon)
				{
					--myConNum;
					curCon = &oppCon;
				}
				else
				{
					--oppConNum;
					curCon = &myCon;
				}
			}
		}
		if (lastCon == &myCon)
		{
			ret += 10;
		}
		else if (lastCon == &oppCon)
		{
			ret -= 1;
		}
	}
	ret += (10 * myConNum);
	if (ret < 0)
	{
		ret = 0;
	}
	return ret/10;
}

void AIUtils::getHand( int points[CARD_POINT_NUM], Hand *hand)
{	
	hand->type = NOTHING;
	if (points[BLACK_JOKER] == 1 && points[RED_JOKER] == 1)
	{
		if (accumulate(points, points + CARD_POINT_NUM, 0) == 2)
		{
			hand->type = NUKE;
			hand->keyPoint = BLACK_JOKER;
		}
	}
	else
	{
		vector<vector<CardPoint>> sortedCard(4);
		for (int i=CARD_3; i<=RED_JOKER; ++i)
		{
			if (points[i] > 0)
			{
				sortedCard[points[i] - 1].push_back(CardPoint(i));
			}
		}
		if (sortedCard[3].size() == 0) //no bomb
		{
			//check trio chain
			if (sortedCard[2].size() >= 2 
				&& sortedCard[2].back() <= CARD_A
				&& sortedCard[2].back() - sortedCard[2].front() == sortedCard[2].size() - 1)
			{
				if (sortedCard[0].size() == 0 && sortedCard[1].size() == 0)
				{
					hand->type = TRIO_CHAIN;
					hand->keyPoint = sortedCard[2].front();
					hand->len = sortedCard[2].size();
				}
				else if (sortedCard[0].size() == sortedCard[2].size() 
					&& sortedCard[1].size() == 0)
				{
					hand->type = TRIO_CHAIN_SOLO;
					hand->keyPoint = sortedCard[2].front();
					hand->len = sortedCard[2].size();
					for (unsigned i=0; i<sortedCard[2].size(); ++i)
					{
						hand->kicker[i] = sortedCard[0][i];
					}
				}
				else if (sortedCard[1].size() == sortedCard[2].size()
					&& sortedCard[0].size() == 0)
				{
					hand->type = TRIO_CHAIN_PAIR;
					hand->keyPoint = sortedCard[2].front();
					hand->len = sortedCard[2].size();
					for (unsigned i=0; i<sortedCard[2].size(); ++i)
					{
						hand->kicker[i] = sortedCard[1][i];
					}
				}
			}
			//pair chain
			else if (sortedCard[2].size() == 0 && sortedCard[0].size() == 0
				&& sortedCard[1].size() >= 3 && sortedCard[1].back() <= CARD_A)
			{
				hand->type = PAIR_CHAIN;
				hand->keyPoint = sortedCard[1].front();
				hand->len = sortedCard[1].size();
			}
			//solo chain
			else if (sortedCard[2].size() == 0 && sortedCard[1].size() == 0
				&& sortedCard[0].size() >= 5 && sortedCard[0].back() <= CARD_A)
			{
				hand->type = SOLO_CHAIN;
				hand->keyPoint = sortedCard[0].front();
				hand->len = sortedCard[0].size();
			}
			else
			{
				if (sortedCard[2].size() == 1)
				{
					if (sortedCard[1].size() == 0 && sortedCard[0].size() == 0)
					{
						hand->type = TRIO;
						hand->keyPoint = sortedCard[2][0];
					}
					else if (sortedCard[1].size() == 1 && sortedCard[0].size() == 0)
					{
						hand->type = TRIO_PAIR;
						hand->keyPoint = sortedCard[2][0];
						hand->kicker[0] = sortedCard[1][0];
					}
					else if (sortedCard[1].size() == 0 && sortedCard[0].size() == 1)
					{
						hand->type = TRIO_SOLO;
						hand->keyPoint = sortedCard[2][0];
						hand->kicker[0] = sortedCard[0][0];
					}
				}
				else if (sortedCard[2].size() == 0 
					&& sortedCard[1].size() == 1 
					&& sortedCard[0].size() == 0)
				{
					hand->type = PAIR;
					hand->keyPoint = sortedCard[1][0];
				}
				else if (sortedCard[2].size() == 0 
					&& sortedCard[1].size() == 0 
					&& sortedCard[0].size() == 1)
				{
					hand->type = SOLO;
					hand->keyPoint = sortedCard[0][0];
				}
			}
			
		}
		else if (sortedCard[3].size() == 1) //BOMB/FOUR_DUAL_SOLO/FOUR_DUAL_PAIR
		{
			if (sortedCard[2].size() == 0 && sortedCard[1].size() == 0 && sortedCard[0].size() == 0)
			{
				hand->type = BOMB;
				hand->keyPoint = sortedCard[3][0];
			}
			else if (sortedCard[2].size() == 0 && sortedCard[1].size() == 2 && sortedCard[0].size() == 0)
			{
				hand->type = FOUR_DUAL_PAIR;
				hand->keyPoint = sortedCard[3][0];
				hand->kicker[0] = sortedCard[1][0];
				hand->kicker[1] = sortedCard[1][1];
			}
			else if (sortedCard[2].size() == 0 && sortedCard[1].size() == 0 && sortedCard[0].size() == 2)
			{
				hand->type = FOUR_DUAL_SOLO;
				hand->keyPoint = sortedCard[3][0];
				hand->kicker[0] = sortedCard[0][0];
				hand->kicker[1] = sortedCard[0][1];
			}
		}
	}
}

bool AIUtils::isChain( HandType type )
{
	return type == SOLO_CHAIN || type == PAIR_CHAIN || type == TRIO_CHAIN
		|| type == TRIO_CHAIN_SOLO || type == TRIO_CHAIN_PAIR;
}

bool AIUtils::isHandHigherThan( Hand &h0, Hand &h1 )
{
	if (h1.type == NUKE)
	{
		return false;
	}
	else if (h1.type == BOMB)
	{
		return h0.type == NUKE || (h0.type == BOMB && h0.keyPoint > h1.keyPoint);
	}
	else
	{
		if (h0.type == NUKE || h0.type == BOMB)
		{
			return true;
		}
		else
		{
			if (h0.type == h1.type && h0.keyPoint > h1.keyPoint)
			{
				if (isChain(h0.type))
				{
					return h0.len == h1.len;
				}
				else
				{
					return true;
				}
			}
			else
			{
				return false;
			}
		}
	}
}

void AIUtils::findLowestHandNotSoloAndNoControl(map<HandType, vector<Hand>> &handsMap, CardPoint lowestControl,  Hand &hand)
{
	hand.type = NOTHING;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		if (it->first != BOMB && it->first != SOLO)
		{
			vector<Hand> &hands = it->second;
			for (unsigned i=0; i<hands.size(); ++i)
			{
				if (!containsControl(lowestControl, hands[i]))
				{
					if (hand.type == NOTHING
						|| hands[i].keyPoint < hand.keyPoint
						|| (hands[i].keyPoint == hand.keyPoint
							&& getHandCount(hands[i]) > getHandCount(hands[i])))
					{
						hand = hands[i];
					}
				}					
			}
		}
	}
}

void AIUtils::findLowestHandNotSolo( std::map<HandType, std::vector<Hand>> &handsMap, Hand &hand )
{
	hand.type = NOTHING;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		if (it->first != BOMB && it->first != SOLO)
		{
			vector<Hand> &hands = it->second;
			for (unsigned i=0; i<hands.size(); ++i)
			{
				if (hand.type == NOTHING
					|| hands[i].keyPoint < hand.keyPoint
					|| (hands[i].keyPoint == hand.keyPoint
					&& getHandCount(hands[i]) > getHandCount(hands[i])))
				{
					hand = hands[i];
				}					
			}
		}
	}
}

void AIUtils::findMostCardsHand( std::map<HandType, std::vector<Hand>> &handsMap, Hand &hand )
{
	hand.type = NOTHING;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		for (unsigned i=0; i<hands.size(); ++i)
		{
			if (hand.type == NOTHING
				|| hand.type == NUKE
				|| hand.type == BOMB
				|| getHandCount(hands[i]) > getHandCount(hands[i])
				|| (getHandCount(hands[i]) == getHandCount(hands[i]) && 
					hands[i].keyPoint < hand.keyPoint))
			{
				hand = hands[i];
			}					
		}
	}
}

bool AIUtils::containsControl( CardPoint lowestControl, Hand &hand )
{
	bool ret = true;
	switch (hand.type)
	{
	case SOLO:
	case PAIR:
	case TRIO:
		ret = hand.keyPoint >= lowestControl;
		break;
	case SOLO_CHAIN:
	case PAIR_CHAIN:
	case TRIO_CHAIN:
		ret = hand.keyPoint + hand.len - 1 >= lowestControl;
		break;
	case TRIO_CHAIN_SOLO:
	case TRIO_CHAIN_PAIR:
		ret = hand.keyPoint + hand.len - 1 >= lowestControl;
		if (!ret)
		{
			for (int i=0; i<hand.len; ++i)
			{
				if (hand.kicker[i] >= lowestControl)
				{
					ret = true;
					break;
				}
			}
		}
		break;
	case TRIO_SOLO:
	case TRIO_PAIR:
		ret = hand.keyPoint >= lowestControl || hand.kicker[0] >= lowestControl;
		break;
	default:
		ret = true;
		break;
	}
	return ret;
}

void AIUtils::handToPointsArray( Hand &hand, int points[CARD_POINT_NUM] )
{
	fill(points, points + CARD_POINT_NUM, 0);
	switch (hand.type)
	{
	case NUKE:
		points[BLACK_JOKER] = 1;
		points[RED_JOKER] = 1;
		break;
	case SOLO:
		points[hand.keyPoint] = 1;
		break;
	case PAIR:
		points[hand.keyPoint] = 2;
		break;
	case TRIO:
		points[hand.keyPoint] = 3;
		break;
	case TRIO_SOLO:
		points[hand.keyPoint] = 3;
		points[hand.kicker[0]] = 1;
		break;
	case TRIO_PAIR:
		points[hand.keyPoint] = 3;
		points[hand.kicker[0]] = 2;
		break;
	case BOMB:
		points[hand.keyPoint] = 4;
		break;
	case FOUR_DUAL_SOLO:
		points[hand.keyPoint] = 4;
		points[hand.kicker[0]] = 1;
		points[hand.kicker[1]] = 1;
		break;
	case FOUR_DUAL_PAIR:
		points[hand.keyPoint] = 4;
		points[hand.kicker[0]] = 2;
		points[hand.kicker[1]] = 2;
		break;
	case SOLO_CHAIN:
		for (int i=0; i<hand.len; ++i)
		{
			points[hand.keyPoint + i] = 1;
		}
		break;
	case PAIR_CHAIN:
		for (int i=0; i<hand.len; ++i)
		{
			points[hand.keyPoint + i] = 2;
		}
		break;
	case TRIO_CHAIN:
		for (int i=0; i<hand.len; ++i)
		{
			points[hand.keyPoint + i] = 3;
		}
		break;
	case TRIO_CHAIN_SOLO:
		for (int i=0; i<hand.len; ++i)
		{
			points[hand.keyPoint + i] = 3;
			points[hand.kicker[i]] = 1;
		}
		break;
	case TRIO_CHAIN_PAIR:
		for (int i=0; i<hand.len; ++i)
		{
			points[hand.keyPoint + i] = 3;
			points[hand.kicker[i]] = 2;
		}
		break;
	default:
		break;
	}
}


void AIUtils::printPoints( int points[CARD_POINT_NUM], char endChar )
{
#ifndef ROBOT_DEBUG
    return;
#else
	for (int i=0; i<CARD_POINT_NUM; ++i)
	{
		for (int j=0; j<points[i]; ++j)
		{
			cout << POINT_CHAR[i];
		}
	}
	cout << endChar;
#endif
}

void AIUtils::printHand( Hand &hand )
{
	int points[CARD_POINT_NUM] = {0};
	handToPointsArray(hand, points);
	printPoints(points, ' ');
}

void AIUtils::printHandsMap( std::map<HandType, std::vector<Hand>> &handsMap )
{
#ifndef ROBOT_DEBUG
    return;
#else
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		for (unsigned i=0; i<hands.size(); ++i)
		{
			printHand(hands[i]);
		}
	}
	cout << endl;
#endif 
}

int AIUtils::getHandCount( Hand &hand )
{
	int ret = 0;
	switch (hand.type)
	{
	case SOLO:
		ret = 1;
		break;
	case PAIR:
	case NUKE:
		ret = 2;
		break;
	case TRIO:
		ret = 3;
		break;
	case BOMB:
	case TRIO_SOLO:
		ret = 4;
		break;
	case TRIO_PAIR:
		ret = 5;
		break;
	case FOUR_DUAL_SOLO:
		ret = 6;
		break;
	case FOUR_DUAL_PAIR:
		ret = 8;
		break;
	case SOLO_CHAIN:
		ret = hand.len;
		break;
	case PAIR_CHAIN:
		ret = 2 * hand.len;
		break;
	case TRIO_CHAIN:
		ret = 3 * hand.len;
		break;
	case TRIO_CHAIN_SOLO:
		ret = 4 * hand.len;
		break;
	case TRIO_CHAIN_PAIR:
		ret = 5 * hand.len;
		break;
	default:
		break;
	}
	return ret;
}

bool AIUtils::isHighestSolo( CardPoint point, int oppPoints[CARD_POINT_NUM] )
{
	for (int i=point+1; i<=RED_JOKER; ++i)
	{
		if (oppPoints[i] > 0)
		{
			return false;
		}
	}
	return true;
}

bool AIUtils::isUniHighestSolo( CardPoint point, int oppPoints[CARD_POINT_NUM] )
{
	for (int i=point; i<=RED_JOKER; ++i)
	{
		if (oppPoints[i] > 0)
		{
			return false;
		}
	}
	return true;
}

bool AIUtils::hasUniLowestSolo( int aiPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM] )
{
	for (int i=CARD_3; i<=RED_JOKER; ++i)
	{
		if (aiPoints[i] > 0 && oppPoints[i] == 0)
		{
			return true;
		}
		if (oppPoints[i] > 0)
		{
			return false;
		}
	}
	return false;
}

bool AIUtils::hasUniHighestSolo( int aiPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM] )
{
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		if (aiPoints[i] > 0 && oppPoints[i] == 0)
		{
			return true;
		}
		if (oppPoints[i] > 0)
		{
			return false;
		}
	}
	return false;
}

CardPoint AIUtils::getLowestPoint( int points[CARD_POINT_NUM] )
{
	for (int i=CARD_3; i<=RED_JOKER; ++i)
	{
		if (points[i] > 0)
		{
			return CardPoint(i);
		}
	}
	return RED_JOKER;
}

CardPoint AIUtils::getHighestPoint( int points[CARD_POINT_NUM] )
{
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		if (points[i] > 0)
		{
			return CardPoint(i);
		}
	}
	return CARD_3;
}

CardPoint AIUtils::getHighestPairPoint( int points[CARD_POINT_NUM] )
{
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		if (points[i] >= 2)
		{
			return CardPoint(i);
		}
	}
	return CARD_3;
}

CardPoint AIUtils::getLowestPairPoint( int points[CARD_POINT_NUM] )
{
	for (int i=CARD_3; i<=RED_JOKER; ++i)
	{
		if (points[i] >= 2)
		{
			return CardPoint(i);
		}
	}
	return RED_JOKER;
}

bool AIUtils::isHighestPair( CardPoint point, int oppPoints[CARD_POINT_NUM] )
{
	for (int i=point+1; i<=RED_JOKER; ++i)
	{
		if (oppPoints[i] >= 2)
		{
			return false;
		}
	}
	return true;
}

int AIUtils::getHighestBomb(int points[CARD_POINT_NUM])
{
	if (points[BLACK_JOKER] == 1 && points[RED_JOKER] == 1)
	{
		return BLACK_JOKER;
	}
	for (int i=CARD_2; i>=CARD_3; --i)
	{
		if (points[i] == 4)
		{
			return i;
		}
	}
	return -1;
}

int AIUtils::getHighestSoloChain( int points[CARD_POINT_NUM], int len )
{
	int end = -1;
	int start = -1;
	for (int i=CARD_A; i>=CARD_3; --i)
	{
		if (points[i] > 0)
		{
			if (end < 0)
			{
				end = i;
			}
			else
			{
				start = i;
				if (end - start + 1 == len)
				{
					break;
				}
			}
		}
		else
		{
			end = -1;
		}
	}
	return start;
}

int AIUtils::getHighestTrio( int points[CARD_POINT_NUM] )
{
	for (int i=CARD_3; i<=CARD_2; ++i)
	{
		if (points[i] >= 3)
		{
			return i;
		}
	}
	return -1;
}
