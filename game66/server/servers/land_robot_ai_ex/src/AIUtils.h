#ifndef AIUtils_h__
#define AIUtils_h__

#pragma once

#include <vector>
#include <map>
#include "Robot.h"

namespace AIUtils
{

double getRandomDouble();

int cardToPoint(int card);

void cardsAdd(int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM], int ret[CARD_POINT_NUM]);

void pointsSub(int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM], int ret[CARD_POINT_NUM]);

void pointsSubEqual(int c0[CARD_POINT_NUM], int c1[CARD_POINT_NUM]);

void cardVecToPointArr(std::vector<int> &cardVec, int poinArr[CARD_POINT_NUM]);

int countNoLowerThanCard(CardPoint point, int cards[CARD_POINT_NUM]);

int getCardNumber(Hand &hand);

bool isChargeHand(Hand &hand, int maxOppCardNum, int oppCards[CARD_POINT_NUM],
				  std::map<HandType, std::vector<Hand>> *handMap);

void getBomb(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void getTrio(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void getTrioChain(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void getPairChain(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void optimizePairChain(int cards[CARD_POINT_NUM], Hand &hand);

void getSoloChain(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void expandSoloChain(int cards[CARD_POINT_NUM], std::vector<Hand> &soloChains);

void optimizeSoloChain(int cards[CARD_POINT_NUM], std::vector<Hand> &soloChains );

void getNormalHand(int cards[CARD_POINT_NUM], std::map<HandType, std::vector<Hand>> &handMap);

void splitCardsToHandsKind1(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap);

void splitCardsToHandsKind2(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap);

void splitCardsToHandsKind3(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap);

HandsMapSummary getHandsMapSummary(std::map<HandType, std::vector<Hand>> &handMap, CardPoint lowestControl, int maxOppCardNum, int oppCards[CARD_POINT_NUM]);

void sortHandsWithBestKind(int points[CARD_POINT_NUM],  //in
						  CardPoint lowestControl, 
						  int maxOppCardNum, 
						  int oppCards[CARD_POINT_NUM],
						  bool searchKicker,
						  std::map<HandType, std::vector<Hand>> &handMap, //out
						  HandsMapSummary &summary);

void splitCardsToHandsKind4(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap);

void splitCardsToHandsKind5(int cards[CARD_POINT_NUM], bool searchKicker, std::map<HandType, std::vector<Hand>> &handMap);

void sortHandsLordOnly1Card(int points[CARD_POINT_NUM],  //in
						   CardPoint lowestControl, 
						   int maxOppCardNum, 
						   int oppCards[CARD_POINT_NUM],
						   bool searchKicker,
						   std::map<HandType, std::vector<Hand>> &handMap, //out
						   HandsMapSummary &summary);

void findKickerForTrioOrTrioChain(std::map<HandType, std::vector<Hand>> &handMap);

CardPoint getLowestControl(int remainPoints[CARD_POINT_NUM]);

int calHandControl(Hand &hand, CardPoint lowestControl);

bool compareHandMapSummary(HandsMapSummary &summary0, HandsMapSummary &summary1);

int calControl(int myPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM], CardPoint lowestControl);

void getHand(int points[CARD_POINT_NUM], Hand *hand);

bool isChain(HandType type);

bool isHandHigherThan(Hand &h0, Hand &h1);

bool isHighestSolo(CardPoint point, int oppPoints[CARD_POINT_NUM]);

bool isUniHighestSolo(CardPoint point, int oppPoints[CARD_POINT_NUM]);

bool hasUniLowestSolo(int aiPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM]);

bool hasUniHighestSolo(int aiPoints[CARD_POINT_NUM], int oppPoints[CARD_POINT_NUM]);

bool isHighestPair(CardPoint point, int oppPoints[CARD_POINT_NUM]);

int getHighestBomb(int points[CARD_POINT_NUM]);

int getHighestSoloChain(int points[CARD_POINT_NUM], int len);

int getHighestTrio(int points[CARD_POINT_NUM]);

CardPoint getLowestPoint(int points[CARD_POINT_NUM]);

CardPoint getHighestPoint(int points[CARD_POINT_NUM]);

CardPoint getHighestPairPoint(int points[CARD_POINT_NUM]);

CardPoint getLowestPairPoint(int points[CARD_POINT_NUM]);

void findLowestHandNotSoloAndNoControl(std::map<HandType, std::vector<Hand>> &handsMap, CardPoint lowestControl, Hand &hand);

void findLowestHandNotSolo(std::map<HandType, std::vector<Hand>> &handsMap, Hand &hand);

void findMostCardsHand(std::map<HandType, std::vector<Hand>> &handsMap, Hand &hand);

bool containsControl(CardPoint lowestControl, Hand &hand);

void handToPointsArray(Hand &hand, int points[CARD_POINT_NUM]);

int getHandCount(Hand &hand);

void findHigherHandFromPoints(Hand &hand, 
							  int points[CARD_POINT_NUM], 
							  CardPoint lowestControl, 
							  int maxOppNumber, 
							  int otherPoints[CARD_POINT_NUM],
							  Hand *higher);

bool findKickerFromPoints(int points[CARD_POINT_NUM],
						  CardPoint lowestControl, 
						  int maxOppNumber, 
						  int otherPoints[CARD_POINT_NUM], 
						  Hand *hand);

void printPoints(int points[CARD_POINT_NUM], char endChar);

void printHand(Hand &hand);

void printHandsMap(std::map<HandType, std::vector<Hand>> &handsMap);
}
#endif // AIUtils_h__
