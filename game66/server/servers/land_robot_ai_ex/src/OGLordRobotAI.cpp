#include "OGLordRobotAI.h"
#include "AIUtils.h"
//#include "ddz_interface.h"

#include <algorithm>

using namespace std;
//using namespace stdext;
using namespace AIUtils;

enum _ROBOT_LEVEL
{
    BASE_LEVEL = 0,
    SIMPLE_LEVEL = 1,
    MIDDLE_LEVEL = 2,
    HARD_LEVEL = 3,
    BEST_LEVEL = 4
};

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

OGLordRobotAI::OGLordRobotAI(void)
{
    robot = new LordRobot();
    init_robot_setting(robot);
}

OGLordRobotAI::~OGLordRobotAI(void)
{
    delete robot;
}

bool OGLordRobotAI::RbtInInitCard(int argSeat, std::vector<int> argHandCard)
{
	aiSeat = argSeat;
	lordSeat = -1;

	//set up new robot
 	for (int k=0; k<17; k++)
	{
       	robot->card[0][k]= argHandCard[k];
	}
	//set robot to non-laizi
	robot->game.hun =-1;
	lord_score = -2;

	aiCardsVec = argHandCard;
	for (int i=0; i<PLAYER_NUM; ++i)
	{
		CardsInfo &cards = playerInfo[i];
		if (i == aiSeat)
		{
			cardVecToPointArr(argHandCard, cards.points);
		}
		else
		{
			fill(cards.points, cards.points + CARD_POINT_NUM, -1);
		}
		cards.total = 17;
	}
	fill(remainPoints, remainPoints + (CARD_POINT_NUM - 2), 4);
	remainPoints[BLACK_JOKER] = 1;
	remainPoints[RED_JOKER] = 1;
	pointsSub(remainPoints, playerInfo[aiSeat].points, otherPoints);
	lowestControl = CARD_2;

	curCaller = -1;
	curScore = 0;
    maxScore = 0;
	curHand.type = NOTHING;
	curHandSeat = -1;
	callHistory.clear();
	history.clear();
	sortHandMap();
	controlNum = calControl(playerInfo[aiSeat].points, otherPoints, lowestControl);

	mustHighLevel = false;

	//printPoints(playerInfo[aiSeat].points, '\n');
	//printPoints(otherPoints);
	//printPoints(remainPoints);
	//printHandsMap(handsMap);
	return true;
}

bool OGLordRobotAI::RbtInSetLevel( int argLevel )
{
	this->level = argLevel;
	robot->level = argLevel;
	robot->game.known_others_poker = argLevel >= HARD_LEVEL ;
	robot->game.use_best_for_undefined_case = argLevel>=HARD_LEVEL;
	robot->game.search_level = argLevel>=BEST_LEVEL ?1:0;	
	return true;
}

bool OGLordRobotAI::RbtInNtfCardInfo( std::vector<std::vector<int>> argHandCard)
{

    int upPlayer=(aiSeat+2)%3;
	for (int k=0; k<17; k++)
	{
       	robot->card[1][k]= argHandCard[upPlayer][k];
	}
	int downPlayer=(aiSeat+1)%3;
	for (int k=0; k<17; k++)
	{
       	robot->card[2][k]= argHandCard[downPlayer][k];
	}
	//init cards..		
    //注意玩家位置
    initCard(robot,robot->card[0],robot->card[1],robot->card[2]);


	for (int i=0; i<3; ++i)
	{
		CardsInfo &cards = playerInfo[i];
		cardVecToPointArr(argHandCard[i], cards.points);
	}
	sortHandMap();
	controlNum = calControl(playerInfo[aiSeat].points, otherPoints, lowestControl);
	return true;
}

bool OGLordRobotAI::RbtInCallScore(int argSeat, int argCallScore)
{
	callHistory.push_back(make_pair(argSeat, argCallScore));
	if (argCallScore > 0)
	{
		curCaller = argSeat;
		curScore = argCallScore;
        maxScore = max_ddz(maxScore, argCallScore);
	}
	return true;
}

bool OGLordRobotAI::RbtOutGetCallScore(int &callScore)
{
    if(level>=MIDDLE_LEVEL)
    {
	  lord_score=RobotshowCard(robot);
      if (lord_score)
      {
          //callScore=3;
          //copy from xxxxClv.cpp
          if ((lord_score & 0xf) >= 2)
              callScore = lord_score;
          else if ((lord_score & 0xf)<1)
              callScore = 0;
          else
              //argCallScore = rand()%2?0:( ((rand()%100)>10)?3:(rand()%3)); //模拟人不理智的情况？
              callScore = rand() % 2 ? 0 : (((rand() % 100)>60) ? 1 : (rand() % 3));
      }
      else
		callScore = 0;

      if (callScore <= maxScore)
      {
          callScore = 0;
      }
	  return true;
    }

	CardsInfo &aiCards = playerInfo[aiSeat];
	int *aiPoints = aiCards.points;
	
	bool isCallLord = false;
	//cout << "controlNum: " << controlNum << endl;
    bool isCallScore1 = false;
    bool isCallScore2 = false;
	if (controlNum + summary.extraBombCount <= 1)
	{
		if ((aiPoints[BLACK_JOKER] == 1 || aiPoints[RED_JOKER] == 1) && aiPoints[CARD_2] > 0)
		{
			if (summary.effectiveHandsCount > 3)
			{
				isCallLord = true;
			}
            else if (summary.effectiveHandsCount >= 2)
            {
                isCallScore2 = true;
            }
            else if (summary.effectiveHandsCount >= 1)
            {
                isCallScore1 = true;
            }
		}
		else if (aiPoints[CARD_2] == 3)
		{
			if (callHistory.size() == 2 && curScore == 2)
			{
				isCallLord = true;
			}
            else if (callHistory.size() == 2 && curScore == 1)
            {
                isCallScore2 = true;
            }
            else if (callHistory.size() == 2 && curScore == 0)
            {
                isCallScore1 = true;
            }
		}
	}
	else
	{
		if (aiPoints[BLACK_JOKER] + aiPoints[RED_JOKER] == 1
			&& aiPoints[CARD_2] == 2
			&& summary.extraBombCount == 0)
		{
			if (summary.effectiveHandsCount > 3)
			{
				isCallLord = true;
			}
            else if (summary.effectiveHandsCount >= 2)
            {
                isCallScore2 = true;
            }
            else if (summary.effectiveHandsCount >= 1)
            {
                isCallScore1 = true;
            }
		}
		else
		{
			//isCallLord = true;
            if (callHistory.size() == 2 && curScore == 2)
            {
                isCallLord = true;
            }
            else if (callHistory.size() == 2 && curScore == 1)
            {
                isCallScore2 = true;
            }
            else if (callHistory.size() == 2 && curScore == 0)
            {
                isCallScore1 = true;
            }
		}
	}
	//callScore = isCallLord ? 3 : 0;

    callScore = isCallLord ? 3 : (isCallScore2 ? 2 : (isCallScore1 ? 1 : 0));

    if (callScore <= maxScore)
    {
        callScore = 0;
    }
	return true;
}

bool OGLordRobotAI::RbtInSetGrabLord( int argSeat)
{
	//////
	return true;
}

bool OGLordRobotAI::RbtOutGetGrabLord( bool &grabLord)
{
    if(level >= MIDDLE_LEVEL)
    {
       if(lord_score == -2)
	     lord_score = RobotshowCard(robot);
	   grabLord = lord_score > 0;
	   return true;	   
    }
	int score;
	RbtOutGetCallScore(score);
	grabLord = score > 0;
	return true;
}

bool OGLordRobotAI::RbtInSetLord(int argLordSeat, std::vector<int> argReceiceCard)
{
	int pot[3];
	 for (int k=0; k<3; k++)
		 pot[k]=argReceiceCard[k];
	 
	 if(argLordSeat == aiSeat)
	   beLord(robot,pot,2);
	 if(argLordSeat==((aiSeat+1)%3))
	   beLord(robot,pot,0);
	 if(argLordSeat==((aiSeat+2)%3))
	   beLord(robot,pot,1);   

	lordSeat = argLordSeat;
	downSeat = (lordSeat + 1) % 3;
	upSeat = (lordSeat + 2) % 3;
	updateAiPosition();
	leftoverCards = argReceiceCard;
	CardsInfo &cards = playerInfo[lordSeat];
	cards.total = 20;
	for (int i=0; i<3; ++i)
	{
		if (lordSeat == aiSeat)
		{
			aiCardsVec.push_back(leftoverCards[i]);
		}
		if (lordSeat == aiSeat || cards.points[0] >= 0)
		{		
			cards.points[cardToPoint(leftoverCards[i])]++;
		}
		
	}
	
	//printPoints(playerInfo[aiSeat].points, '\n');
	return true;
}

bool OGLordRobotAI::RbtInTakeOutCard( int argSeat, std::vector<int> argCards)
{

	if(argSeat!=aiSeat){
		int out1[21];
		int i;
		for(i=0;i<argCards.size();++i)
		  out1[i]=argCards[i];
		
		out1[argCards.size()]=-1;
		//double set to non-laizi
	    robot->game.hun = -1;
		if(argSeat==((aiSeat+1)%3))
		  getPlayerTakeOutCards(robot,out1,NULL,0);  // 0:down player
		if(argSeat==((aiSeat+2)%3))
		  getPlayerTakeOutCards(robot,out1,NULL,1);  // 1:up player
	}
	else if( level >= HARD_LEVEL )
		return 1;

	Hand hand;
	if (argCards.size() == 0)
	{
		hand.type = NOTHING;
	}
	else
	{
		int points[CARD_POINT_NUM];
		cardVecToPointArr(argCards, points);

		CardsInfo &cards = playerInfo[argSeat];
		cards.total -= argCards.size();
		for (int i=0; i<CARD_POINT_NUM; ++i)
		{
			remainPoints[i] -= points[i];
			if (argSeat != aiSeat)
			{
				otherPoints[i] -= points[i];
			}
			if (argSeat == aiSeat || cards.points[0] >= 0)
			{
				cards.points[i] -= points[i];
			}
		}		
		getHand(points, &hand);
	}
	history.push_back(make_pair(argSeat, hand));
	if (hand.type != NOTHING)
	{
		curHand = hand;
		curHandSeat = argSeat;
	}

	//for (int i=0; i<3; ++i)
	//{
	//	printPoints(playerInfo[i].points, '\n');
	//}
	return true;
}

bool OGLordRobotAI::RbtOutGetTakeOutCard(std::vector<int> &vecCards)
{
    if(level >=MIDDLE_LEVEL)
    {
       int out1[26],out2[5];
       int pass1=takeOut(robot,out1,out2);
       int i,pos=0;
	   
	   PRINTF(0,"\nRbtOutGetTakeOutCard robot%d vecCards: ",aiSeat );
	   for ( i=0; i<26; ++i)
	   {    
	     if(out1[i]!=-1)		
	     {
		 	 print_one_poker(int_2_poker(out1[i]));
		         vecCards.push_back(out1[i]);
	     }
	     else
	     {
       		 //vecCards.push_back(out1[i]);
       		 PRINTF(0,"-1");
       		 break;
	     }
       }
	   return true;
    }
	lowestControl = getLowestControl(remainPoints);
	controlNum = calControl(playerInfo[aiSeat].points, otherPoints, lowestControl);
	Hand hand;
	if (level == 0)
	{
		takeOutLvl0(hand);
	}
	else if (level == 1)
	{
		takeOutLvl1(hand);
	}
	else
	{
		if (mustHighLevel)
		{
			takeOutHighLvl(hand);
		}
		else
		{
			double ratio = level * level / 100.0;
			if (getRandomDouble() < ratio)
			{
				takeOutHighLvl(hand);
			}
			else
			{
				takeOutLvl1(hand);
			}
		}
	}

	takeOutHand(hand, vecCards);

	//cout << "take out hand: ";
	//printHand(hand);
	//cout << endl;
	//for (unsigned i=0; i<vecCards.size(); ++i)
	//{
	//	cout << vecCards[i] << " ";
	//}
	//cout << endl;

	Hand checkHand;
	int checkPoints[CARD_POINT_NUM];
	cardVecToPointArr(vecCards, checkPoints);
	getHand(checkPoints, &checkHand);
	if (isFree())
	{
		if (checkHand.type == NOTHING)
		{
			findLowestHand(hand);
			takeOutHand(hand, vecCards);
		}
	}
	else
	{
		if (checkHand.type != NOTHING && !isHandHigherThan(checkHand, curHand))
		{
			hand.type = NOTHING;
			//takeOutHand(hand, vecCards);
            aiCardsVec.resize(aiCardsVec.size() + vecCards.size());
            copy_backward(vecCards.begin(), vecCards.end(), aiCardsVec.end());
            sort(aiCardsVec.begin(), aiCardsVec.end());
            vecCards.clear();
		}
	}

	//cout << "remain cards after takeout: ";
	//for (unsigned i=0; i<aiCardsVec.size(); ++i)
	//{
	//	cout << aiCardsVec[i] << " ";
	//}
	//cout << endl;
	return true;
}

bool OGLordRobotAI::RbtResetData()
{
    delete robot;
    robot = new LordRobot();
    init_robot_setting(robot);
    return robot != NULL;
}

bool OGLordRobotAI::RbtInSetSeat( int argMySeat, int argLordSeat)
{
	aiSeat = argMySeat;
	lordSeat = argLordSeat;
	downSeat = (lordSeat + 1) % 3;
	upSeat = (lordSeat + 2) % 3;
	updateAiPosition();
	return true;
}

bool OGLordRobotAI::RbtInSetCard(std::vector<int> argInitCard, std::vector<int> argReceiveCard )
{
	for (int i=0; i<PLAYER_NUM; ++i)
	{
		CardsInfo &cards = playerInfo[i];
		if (i == aiSeat)
		{
			cardVecToPointArr(argInitCard, cards.points);
		}
		else
		{
			fill(cards.points, cards.points + CARD_POINT_NUM, -1);
		}
		if (i == lordSeat)
		{
			cards.total = 20;
		}
		else
		{
			cards.total = 17;
		}
	}
	leftoverCards = argReceiveCard;
	return true;
}

bool OGLordRobotAI::RbtInTakeOutRecord( std::vector<int> argTakeOutSeat, std::vector<std::vector<int>> argTakeOutRecord)
{
	fill(remainPoints, remainPoints + (CARD_POINT_NUM - 2), 4);
	remainPoints[BLACK_JOKER] = 1;
	remainPoints[RED_JOKER] = 1;
	for (unsigned i=0; i<argTakeOutSeat.size(); ++i)
	{
		int points[CARD_POINT_NUM];
		cardVecToPointArr(argTakeOutRecord[i], points);
		
		for (int j=0; j<CARD_POINT_NUM; ++j)
		{
			remainPoints[j] -= points[j];
		}
		Hand hand;
		getHand(points, &hand);
		history.push_back(make_pair(argTakeOutSeat[i], hand));
	}
	pointsSub(remainPoints, playerInfo[aiSeat].points, otherPoints);
	return true;
}

bool OGLordRobotAI::RbtOutGetLastError( int &errorCode )
{
	errorCode = errCode;
	return true;
}

void OGLordRobotAI::updateAiPosition()
{
	if (lordSeat == aiSeat)
	{
		aiPosition = LORD;
	}
	else
	{
		int diff = aiSeat - lordSeat;
		if (diff == 1 || diff == -2)
		{
			aiPosition = DOWNFARMER;
		}
		else
		{
			aiPosition = UPFARMER;
		}
	}
}

void OGLordRobotAI::sortHandMap()
{
	maxOppNumber = 0;
	minOppNumber = 20;
	if (lordSeat == -1 || lordSeat == aiSeat)
	{
		for (int i=0; i<PLAYER_NUM; ++i)
		{
			if (i != aiSeat)
			{
				if (playerInfo[i].total > maxOppNumber)
				{
					maxOppNumber = playerInfo[i].total;
				}
				if (playerInfo[i].total < minOppNumber)
				{
					minOppNumber = playerInfo[i].total;
				}
			}
		}
	}
	else
	{
		maxOppNumber = playerInfo[lordSeat].total;
		minOppNumber = maxOppNumber;
	}

	sortHandsWithBestKind(playerInfo[aiSeat].points,
						lowestControl,
						maxOppNumber,
						otherPoints,
						true,
						handsMap,
						summary);
}

void OGLordRobotAI::sortHandMapLvl()
{
	int cardsCopy[CARD_POINT_NUM];
	map<HandType, vector<Hand>> tmpMap;
	map<HandType, int> tmpUniHighMap;
	map<HandType, int> tmpHighMap;
	map<HandType, int> tmpGoodLowMap;
	int tmpTrioCount;
	HandsMapSummary tmpSummary;

	if (playerInfo[lordSeat].total == 1 && aiPosition != LORD)
	{
		copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, cardsCopy);
		splitCardsToHandsKind4(cardsCopy, false, handsMap);
		countHighHand(handsMap, uniHighHandCount, highHandCount, goodLowHandCount, handsTrioCount);
		summary = getHandsMapSummaryLvl(handsMap, uniHighHandCount, highHandCount, goodLowHandCount, handsTrioCount);

		copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, cardsCopy);
		splitCardsToHandsKind5(cardsCopy, false, tmpMap);
		countHighHand(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		tmpSummary = getHandsMapSummaryLvl(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		if (tmpSummary.soloCount < summary.soloCount
			|| (tmpSummary.soloCount == summary.soloCount && tmpSummary.lowestSolo > summary.lowestSolo))
		{
			handsMap = tmpMap;
			summary = tmpSummary;
			uniHighHandCount = tmpUniHighMap;
			highHandCount = tmpHighMap;
			goodLowHandCount = tmpGoodLowMap;
			handsTrioCount = tmpTrioCount;
		}
	}
	else
	{
		copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, cardsCopy);
		splitCardsToHandsKind1(cardsCopy, false, handsMap);
		countHighHand(handsMap, uniHighHandCount, highHandCount, goodLowHandCount, handsTrioCount);
		summary = getHandsMapSummaryLvl(handsMap, uniHighHandCount, highHandCount, goodLowHandCount, handsTrioCount);

		copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, cardsCopy);
		splitCardsToHandsKind2(cardsCopy, false, tmpMap);
		countHighHand(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		tmpSummary = getHandsMapSummaryLvl(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		if (!compareHandMapSummary(summary, tmpSummary))
		{
			handsMap = tmpMap;
			summary = tmpSummary;
			uniHighHandCount = tmpUniHighMap;
			highHandCount = tmpHighMap;
			goodLowHandCount = tmpGoodLowMap;
			handsTrioCount = tmpTrioCount;
		}

		copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, cardsCopy);
		splitCardsToHandsKind3(cardsCopy, false, tmpMap);
		countHighHand(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		tmpSummary = getHandsMapSummaryLvl(handsMap, tmpUniHighMap, tmpHighMap, tmpGoodLowMap, tmpTrioCount);
		if (!compareHandMapSummary(summary, tmpSummary))
		{
			handsMap = tmpMap;
			summary = tmpSummary;
			uniHighHandCount = tmpUniHighMap;
			highHandCount = tmpHighMap;
			goodLowHandCount = tmpGoodLowMap;
			handsTrioCount = tmpTrioCount;
		}
	}
}

void OGLordRobotAI::findLowestHand( Hand &hand )
{
	hand.type = NOTHING;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		if (!hands.empty())
		{
			if (hand.type == NOTHING)
			{
				hand = hands[0];
			}
			else
			{
				if (hand.type == BOMB
					|| hands[0].keyPoint < hand.keyPoint
					|| (hands[0].keyPoint == hand.keyPoint 
						&& getHandCount(hands[0]) > getHandCount(hand)))
				{
					hand = hands[0];
				}
			}
		}
	}
}

void OGLordRobotAI::findHigherHand(Hand &hand)
{
	findHigherHandNotBomb(hand);
	if (hand.type == NOTHING && (curHand.type != BOMB || curHand.type != NUKE))
	{
		vector<Hand> &bombs = handsMap[BOMB];
		if (!bombs.empty())
		{
			hand = bombs.front();
		}
	}
}

void OGLordRobotAI::findHigherHandNotBomb( Hand &hand )
{
	hand.type = NOTHING;
	vector<Hand> &hands = handsMap[curHand.type];
	if (!hands.empty())
	{
		for (unsigned i=0; i<hands.size(); ++i)
		{
			if (isChain(curHand.type))
			{
				if (hands[i].keyPoint > curHand.keyPoint
					&& hands[i].len == curHand.len)
				{
					hand = hands[i];
					break;
				}
			}
			else
			{
				if (hands[i].keyPoint > curHand.keyPoint)
				{
					hand = hands[i];
					break;
				}
			}
		}
	}
}

void OGLordRobotAI::findMostCardsHandNotBomb( Hand &hand )
{
	hand.type = NOTHING;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		if (!hands.empty())
		{
			if (hand.type == NOTHING)
			{
				hand = hands[0];
			}
			else
			{
				if (hand.type == BOMB
					|| getHandCount(hands[0]) > getHandCount(hand)
					|| (getHandCount(hands[0]) == getHandCount(hand)
						&& hands[0].keyPoint < hand.keyPoint))
				{
					hand = hands[0];
				}
			}
		}
	}
}

void OGLordRobotAI::takeOutHand(Hand &hand, std::vector<int> &takeOutCards)
{
	int points[CARD_POINT_NUM] = {0};
	handToPointsArray(hand, points);
	takeOutCards.clear();
	for (vector<int>::iterator it=aiCardsVec.begin(); it!=aiCardsVec.end();)
	{
		int point = cardToPoint(*it);
		if (points[point] > 0)
		{
			--points[point];
			takeOutCards.push_back(*it);
			it = aiCardsVec.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void OGLordRobotAI::takeOutLvl0( Hand &hand )
{
	sortHandMap();
	if (curHandSeat == aiSeat
		|| (aiPosition == LORD && curHandSeat < 0))
	{
		findLowestHand(hand);
	}
	else
	{
		findHigherHand(hand);
	}
}

void OGLordRobotAI::takeOutLvl1( Hand &hand )
{
	sortHandMap();
	if (aiPosition == LORD)
	{
		if (curHandSeat == -1 || curHandSeat == lordSeat)
		{
			lordTakeOutFree(hand);
		}
		else
		{
			lordTakeOutHigher(hand);
		}
	}
	else
	{
		if (isGoodFarmer())
		{
			if (aiPosition == DOWNFARMER)
			{
				downGoodFarmerTakeOut(hand);
			}
			else
			{
				upGoodFarmerTakeOut(hand);
			}
		}
		else
		{
			if (aiPosition == DOWNFARMER)
			{
				downBadFarmerTakeOut(hand);
			}
			else
			{
				upBadFarmerTakeOut(hand);
			}
		}
	}

    if (hand.type == TRIO || hand.type == TRIO_SOLO || hand.type == TRIO_PAIR
        || hand.type == TRIO_CHAIN_SOLO || hand.type == TRIO_CHAIN_PAIR)
    {
        refindForTrio(hand);
    }
}

void OGLordRobotAI::takeOutHighLvl( Hand &hand )
{
	sortHandMapLvl();
	if (aiPosition == LORD)
	{
		if (curHandSeat == -1 || curHandSeat == lordSeat)
		{
			lordTakeOutFreeLvl(hand);
		}
		else
		{
			lordTakeOutHigher(hand);
		}
	}
	else
	{
		if (isGoodFarmer())
		{
			if (aiPosition == DOWNFARMER)
			{
				downGoodFarmerTakeOut(hand);
			}
			else
			{
				upGoodFarmerTakeOut(hand);
			}
		}
		else
		{
			if (aiPosition == DOWNFARMER)
			{
				downBadFarmerTakeOut(hand);
			}
			else
			{
				upBadFarmerTakeOut(hand);
			}
		}
	}
}

bool OGLordRobotAI::isFree()
{
	return (aiPosition == LORD && curHandSeat == -1) || curHandSeat == aiSeat;
}

bool OGLordRobotAI::isDanger()
{
	bool ret = false;
	if (otherPoints[BLACK_JOKER] == 1 && otherPoints[RED_JOKER] == 1)
	{
		ret = true;
	}
	else
	{
		for (int i=CARD_2; i>=CARD_3; --i)
		{
			if (otherPoints[i] == 4)
			{
				vector<Hand> &aiBombs = handsMap[BOMB];
				if (aiBombs.size() == 0
					|| aiBombs.back().keyPoint < i)
				{
					ret = true;
				}
				break;
			}
		}
	}
	return ret;
}

bool OGLordRobotAI::isGood(HandsMapSummary &hmSummary, int controlCount, int minOppNum)
{
	bool ret = false;
	if (minOppNum == 1)
	{
		int oppHighestPoint = getHighestPoint(otherPoints);
		vector<Hand> &soloHands = handsMap[SOLO];
		int lowSoloCount = 0;
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint < oppHighestPoint)
			{
				++lowSoloCount;
			}
		}
		if (aiPosition == LORD)
		{
			ret = (hmSummary.effectiveHandsCount - controlCount ) < 2
				&& lowSoloCount <= 1;
		}
		else
		{
			ret = lowSoloCount <= 1;
		}
	}
	else
	{
		ret = (hmSummary.effectiveHandsCount - controlCount ) < 2;
	}
	return ret;
}

bool OGLordRobotAI::isFirstHalf()
{
	bool ret = true;
	for (int i=0; i<3; ++i)
	{
		if ((i == lordSeat && playerInfo[i].total <= 10)
			|| playerInfo[i].total <= 8)
		{
			ret = false;
			break;
		}
	}
	return ret;
}

bool OGLordRobotAI::isGoodFarmer()
{
	bool ret;
	if (playerInfo[lordSeat].total == 1)
	{
		ret = isGood(summary, controlNum, minOppNumber);
	}
	else
	{
		if (isFirstHalf())
		{
			ret = summary.effectiveHandsCount - controlNum < 5;
		}
		else
		{
			ret = summary.effectiveHandsCount - controlNum < 3;
		}
	}
	return ret;
}

void OGLordRobotAI::findChargeHandFirst( Hand &hand, bool typeCountFirst )
{
	hand.type = NOTHING;
	bool isCharge = false;
	for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		for (unsigned i=0; i<hands.size(); ++i)
		{
			if (hand.type == NOTHING
				|| (hand.type == NUKE || hand.type == BOMB))
			{
				hand = hands[i];
				if (!isCharge)
				{
					isCharge = isChargeHand(hands[i], maxOppNumber, otherPoints, &handsMap);
				}
			}
			else
			{
				if (it->first != BOMB)
				{
					if (isCharge)
					{
						if (isChargeHand(hands[i], maxOppNumber, otherPoints, &handsMap))
						{
							if (hands[i].type != hand.type)
							{
								if (typeCountFirst)
								{
									if (hands.size() > handsMap[hand.type].size()
										|| (hands.size() == handsMap[hand.type].size()
										&& getHandCount(hands[i]) > getHandCount(hand)))
									{
										hand = hands[i];
									}
								} 
								else
								{
									if (getHandCount(hands[i]) > getHandCount(hand))
									{
										hand = hands[i];
									}
								}
							}
						}
					}
					else
					{
						isCharge = isChargeHand(hands[i], maxOppNumber, otherPoints, &handsMap);
						if (isCharge)
						{
							hand = hands[i];
						}
						else
						{
							if (hands[i].keyPoint < hand.keyPoint
								|| (hands[i].keyPoint == hand.keyPoint 
								&& getHandCount(hands[i]) > getHandCount(hand)))
							{
								hand = hands[i];
							}
						}
					}
				}
			}
		}
	}
}

void OGLordRobotAI::lordTakeOutFree( Hand &hand )
{
	CardsInfo &aiCard = playerInfo[aiSeat];
	if (playerInfo[(aiSeat + 1) % 3].total == 1
		|| playerInfo[(aiSeat + 2) % 3].total == 1)
	{
		lordTakeOutFreeFarmerOnly1Card(hand);
	}
	else if ((playerInfo[(aiSeat + 1) % 3].total == 2 
				|| playerInfo[(aiSeat + 2) % 3].total == 2)
			&& (handsMap[SOLO].size() > 0 || handsMap[PAIR].size() > 0))
	{
		lordTakeOutFreeFarmerOnly2Cards(hand);
	}
	else
	{
		lordTakeOutFreeNormal(hand);
	}
}

void OGLordRobotAI::lordTakeOutFreeFarmerOnly1Card( Hand &hand )
{
	findLowestHandNotSoloAndNoControl(handsMap, lowestControl, hand);
	if (hand.type != NOTHING)
	{
		return;
	}
	if ((handsMap[SOLO].size() == 2 || handsMap[SOLO].size() == 3)
		&& isDanger()
		&& handsMap[BOMB].size() > 0)
	{
        for (int i = 0; i < handsMap[BOMB].size(); ++i)
        {
            if (handsMap[BOMB][i].type == NUKE)
            {
                continue;
            }
            hand.type = FOUR_DUAL_SOLO;
            hand.keyPoint = handsMap[BOMB][i].keyPoint;
            hand.kicker[0] = handsMap[SOLO][0].keyPoint;
            hand.kicker[1] = handsMap[SOLO][1].keyPoint;
            return;
        }
	}
	if (handsMap[SOLO].size() >= 2)
	{
		hand = handsMap[SOLO][1];
		return;
	}
	findLowestHandNotSolo(handsMap, hand);
	if (hand.type != NOTHING)
	{
		return;
	}
	if (handsMap[BOMB].size() == 0
		|| (handsMap[SOLO].size() == 1 && isDanger()))
	{
		hand = handsMap[SOLO][0];
	}
	else
	{
		hand = handsMap[BOMB][0];
	}
	return;
}

void OGLordRobotAI::lordTakeOutFreeFarmerOnly2Cards( Hand &hand )
{
	int highestPoint = CARD_3;
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		if (remainPoints[i] > 0)
		{
			highestPoint = i;
			break;
		}
	}

	CardsInfo &aiCard = playerInfo[aiSeat];
	if ((remainPoints[highestPoint] == 1 && aiCard.points[highestPoint] == 1)
		|| handsMap[BOMB].size() > 0)
	{
		if (handsMap[SOLO].size() > 0)
		{
			for (vector<Hand>::reverse_iterator it=handsMap[SOLO].rbegin(); 
				it!=handsMap[SOLO].rend(); ++it)
			{
				if (it->keyPoint < lowestControl)
				{
					hand = *it;
					return;
				}
			}
		}
		if (handsMap[PAIR].size() > 0)
		{
			hand.type = SOLO;
			hand.keyPoint = handsMap[PAIR][0].keyPoint;
			return;
		}
		lordTakeOutFreeNormal(hand);
	}
	else
	{
		lordTakeOutFreeFarmerOnly1Card(hand);
	}
}

void OGLordRobotAI::lordTakeOutFreeNormal( Hand &hand )
{
	if (!isDanger()
		&& (unsigned)summary.unChargeHandsCount <= handsMap[BOMB].size() + 1)
	{
		if (handsMap[BOMB].size() > 0 && summary.realHandsCount == 1)
		{
			hand = handsMap[BOMB].front();
		}
		else
		{
			findChargeHandFirst(hand, false);
		}
	}
	else
	{
		findLowestHand(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigher( Hand &hand )
{
	if (playerInfo[curHandSeat].total == 1 || playerInfo[(aiSeat + 2) % 3].total == 1)
	{
		lordTakeOutHigherFarmerOnly1Card(hand);
	}
	else if (playerInfo[curHandSeat].total == 2 || playerInfo[(aiSeat + 2) % 3].total == 2)
	{
		lordTakeOutHigherFarmerOnly2Cards(hand);
	}
	else
	{
		lordTakeOutHigherNormal(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherFarmerOnly1Card( Hand &hand )
{
	hand.type = NOTHING;
	if (curHand.type == SOLO)
	{
		if (handsMap[SOLO].size() >= 2)
		{
			for (unsigned i=1; i<handsMap[SOLO].size(); ++i)
			{
				if (handsMap[SOLO][i].keyPoint > curHand.keyPoint)
				{
					hand = handsMap[SOLO][i];
					return;
				}
			}
		}
		if (handsMap[SOLO].size() == 1)
		{
			bool isHighest = true;
			for (int i = handsMap[SOLO][0].keyPoint + 1; i <= RED_JOKER; ++i)
			{
				if (otherPoints[i] > 0)
				{
					isHighest = false;
					break;
				}
			}
			if (isHighest)
			{
				hand = handsMap[SOLO][0];
				return;
			}
		}
		if (handsMap[BOMB].size() > 0)
		{
			if (!isDanger())
			{
				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyBomb = copyHandsMap[BOMB];
				int handPoint[CARD_POINT_NUM];
				int copyAiPoint[CARD_POINT_NUM];
				int copyRemainPoint[CARD_POINT_NUM];
				handToPointsArray(copyBomb.front(), handPoint);
				pointsSub(playerInfo[aiSeat].points, handPoint, copyAiPoint);
				pointsSub(remainPoints, handPoint, copyRemainPoint);
				CardPoint copyLowestControl = getLowestControl(copyRemainPoint);
				int copyControlNum = calControl(copyAiPoint, otherPoints, copyLowestControl);

				copyBomb.erase(copyBomb.begin());
				HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestControl, maxOppNumber, otherPoints);
				if (isGood(copySummary, copyControlNum, minOppNumber))
				{
					hand = handsMap[BOMB].front();
					return;
				}
			}
		}
		int highest = CARD_3;
		for (int i=RED_JOKER; i>=CARD_3; --i)
		{
			if (playerInfo[aiSeat].points[i] > 0)
			{
				highest = i;
				break;
			}
		}
		if (highest > curHand.keyPoint)
		{
			hand.type = SOLO;
			hand.keyPoint = CardPoint(highest);
			return;
		}
	}
	else
	{
		lordTakeOutHigherNormal(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherFarmerOnly2Cards( Hand &hand )
{
	hand.type = NOTHING;
	if (curHand.type == PAIR)
	{
		int highestPair = -1;
		for (int i=CARD_2; i>=CARD_3; --i)
		{
			if (otherPoints[i] >= 2)
			{
				highestPair = i;
				break;;
			}
		}
		if (highestPair >= 0)
		{
			vector<Hand> &aiPairs = handsMap[PAIR];
			for (vector<Hand>::reverse_iterator it=aiPairs.rbegin(); it!=aiPairs.rend(); ++it)
			{
				if (it->keyPoint >= highestPair)
				{
					hand = *it;
					return;
				}
			}
			lordTakeOutHigherRebuild(hand);
		}
		else
		{
			lordTakeOutHigherNormal(hand);
		}
	}
}

void OGLordRobotAI::lordTakeOutHigherNormal( Hand &hand )
{
	hand.type = NOTHING;
	findBestHigherHandFromMap(hand);
	if (hand.type == NOTHING)
	{
		if (handsMap[BOMB].size() > 0)
		{
			if (!isDanger())
			{
				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyBomb = copyHandsMap[BOMB];
				int handPoint[CARD_POINT_NUM];
				int copyAiPoint[CARD_POINT_NUM];
				int copyRemainPoint[CARD_POINT_NUM];
				handToPointsArray(copyBomb.front(), handPoint);
				pointsSub(playerInfo[aiSeat].points, handPoint, copyAiPoint);
				pointsSub(remainPoints, handPoint, copyRemainPoint);
				CardPoint copyLowestControl = getLowestControl(copyRemainPoint);
				int copyControlNum = calControl(copyAiPoint, otherPoints, copyLowestControl);

				copyBomb.erase(copyBomb.begin());
				HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestControl, maxOppNumber, otherPoints);
				if (isGood(copySummary, copyControlNum, minOppNumber))
				{
					hand = handsMap[BOMB].front();
					return;
				}
			}
		}
		lordTakeOutHigherRebuild(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherRebuild( Hand &hand )
{
	hand.type = NOTHING;
	findBestHigherHandFromPoints(hand, false, false);
}

void OGLordRobotAI::farmerTakeOutLordOnly1Card( Hand &hand )
{
	sortHandsLordOnly1Card(playerInfo[aiSeat].points,
		lowestControl,
		maxOppNumber,
		otherPoints,
		true,
		handsMap,
		summary);
	int *aiPoints = playerInfo[aiSeat].points;

	hand.type = NOTHING;
	if (isFree())
	{
		if (aiPosition == UPFARMER)
		{
			if (isGood(summary, controlNum, minOppNumber))
			{
				findMostCardsHand(handsMap, hand);
			}
			else
			{
				for (int i=CARD_3; i<=CARD_2; ++i)
				{
					if (aiPoints[i] == 2 || aiPoints[i] == 3)
					{
						hand.type = PAIR;
						hand.keyPoint = CardPoint(i);
						return;
					}
				}
				vector<Hand> &soloHands = handsMap[SOLO];
				if (soloHands.size() == 2 || soloHands.size() == 3)
				{
					if (!isHighestSolo(soloHands.back().keyPoint, otherPoints))
					{
						vector<Hand> &bombHands = handsMap[BOMB];
						if (bombHands.size() > 0 && bombHands.front().keyPoint < BLACK_JOKER)
						{
							hand.type = FOUR_DUAL_SOLO;
							hand.keyPoint = bombHands.front().keyPoint;
							hand.kicker[0] = soloHands[0].keyPoint;
							hand.kicker[1] = soloHands[1].keyPoint;
							return;
						}
					}
				}
				hand.type = SOLO;
				hand.keyPoint = soloHands.back().keyPoint;
				return;
			}
		}
		else
		{
			if (isGood(summary, controlNum, minOppNumber))
			{
				findMostCardsHand(handsMap, hand);
			}
			else
			{
				if (playerInfo[(aiSeat + 2) % 3].total == 1
					&& hasUniLowestSolo(aiPoints, otherPoints))
				{
					hand.type = SOLO;
					hand.keyPoint = getLowestPoint(aiPoints);
					return;
				}
				vector<Hand> &soloHands = handsMap[SOLO];
				if (soloHands.size() == 2)
				{
					hand = soloHands.back();
					return;
				}
				vector<Hand> &pairHands = handsMap[PAIR];
				if (pairHands.size() > 0)
				{
					if (!isHighestPair(pairHands.front().keyPoint, otherPoints))
					{
						hand = pairHands.front();
						return;
					}

                    //非对子只有一套
                    int nonPairComboNum = 0;
                    for (std::map<HandType, std::vector<Hand>>::iterator beg = handsMap.begin(), end = handsMap.end(); beg != end; ++beg)
                    {
                        if (beg->first != PAIR)
                        {
                            nonPairComboNum += beg->second.size();
                        }
                    }

                    //手里只有对子
                    if (nonPairComboNum <= 1)
                    {
                        hand = pairHands.front();
                        return;
                    }                    
				}
				int notHighestSoloCount = 0;
				for (unsigned i=0; i<soloHands.size(); ++i)
				{
					if (isHighestSolo(soloHands[i].keyPoint, otherPoints))
					{
						break;
					}
					else
					{
						++notHighestSoloCount;
					}
				}
				if (notHighestSoloCount == 2 || notHighestSoloCount == 3)
				{
					vector<Hand> &bombHands = handsMap[BOMB];
					if (bombHands.size() > 0 && bombHands.front().keyPoint < BLACK_JOKER)
					{
						hand.type = FOUR_DUAL_SOLO;
						hand.keyPoint = bombHands.front().keyPoint;
						hand.kicker[0] = soloHands[0].keyPoint;
						hand.kicker[1] = soloHands[1].keyPoint;
						return;
					}
				}

                //搜索非单套
                for (std::map<HandType, std::vector<Hand>>::iterator beg = handsMap.begin(), end = handsMap.end(); beg != end; ++beg)
                {
                    if (beg->first != SOLO && !beg->second.empty())
                    {
                        hand = beg->second.front();
                        return;
                    }
                }

                if (!soloHands.empty())
                {
                    hand = soloHands.back();
                }
                return;
			}
		}
	}
	else
	{
		if (curHandSeat == lordSeat)
		{
			if (curHand.type == SOLO)
			{
				int oppHighest = (int)getHighestPoint(otherPoints);
				vector<int> highestSolo;
				for (int i=max_ddz(curHand.keyPoint + 1, oppHighest); i<=RED_JOKER; ++i)
				{
					if (aiPoints[i] > 0)
					{
						highestSolo.push_back(i);
					}
				}
				if (highestSolo.size() > 0)
				{
					int cur = 0;
					int copyPoints[CARD_POINT_NUM];
					copy(aiPoints, aiPoints + CARD_POINT_NUM, copyPoints);
					--copyPoints[highestSolo[cur]];
					map<HandType, vector<Hand>> copyHandsMap;
					HandsMapSummary copySummary;
					sortHandsLordOnly1Card(copyPoints,
						lowestControl,
						maxOppNumber,
						otherPoints,
						true,
						copyHandsMap,
						copySummary);
					++copyPoints[highestSolo[cur]];
					for (unsigned i=1; i<highestSolo.size(); ++i)
					{
						--copyPoints[highestSolo[i]];
						map<HandType, vector<Hand>> tmpHandsMap;
						HandsMapSummary tmpSummary;
						sortHandsLordOnly1Card(copyPoints,
							lowestControl,
							maxOppNumber,
							otherPoints,
							true,
							tmpHandsMap,
							tmpSummary);
						if (tmpSummary.soloCount < copySummary.soloCount
							|| (tmpSummary.soloCount == copySummary.soloCount
								&& tmpSummary.lowestSolo > copySummary.lowestSolo))
						{
							cur = i;
							copySummary = tmpSummary;
						}
						++copyPoints[highestSolo[i]];
					}

					hand.type = SOLO;
					hand.keyPoint = CardPoint(highestSolo[cur]);
				}
				vector<Hand> &bombHands = handsMap[BOMB];
				vector<Hand> &soloHands = handsMap[SOLO];
				if (bombHands.size() > 0 && soloHands.size() == 1)
				{
					hand = bombHands.front();
					return;
				}

				if (aiSeat == DOWNFARMER)
				{
					findHigherHandNotBomb(hand);
				}
				else
				{
					if (bombHands.size() > 0)
					{
						hand = bombHands.front();
						return;
					}
				}
			}
			else
			{
				farmerMustTakeOutLordOnly1Card(hand);
			}
		}
		else
		{
			if (curHand.type == SOLO)
			{
				if (aiSeat == DOWNFARMER)
				{
					if (isGood(summary, controlNum, minOppNumber))
					{
						findHigherHandNotBomb(hand);
						return;
					}
					else
					{
						hand.type = NOTHING;
						return;
					}
				}
				else
				{
					int oppHighest = (int)getHighestPoint(otherPoints);
					vector<int> highestSolo;
					for (int i=max_ddz(curHand.keyPoint + 1, oppHighest); i<=RED_JOKER; ++i)
					{
						if (aiPoints[i] > 0)
						{
							highestSolo.push_back(i);
						}
					}
					if (highestSolo.size() > 0)
					{
						int cur = 0;
						int copyPoints[CARD_POINT_NUM];
						copy(aiPoints, aiPoints + CARD_POINT_NUM, copyPoints);
						--copyPoints[highestSolo[cur]];
						map<HandType, vector<Hand>> copyHandsMap;
						HandsMapSummary copySummary;
						sortHandsLordOnly1Card(copyPoints,
							lowestControl,
							maxOppNumber,
							otherPoints,
							true,
							copyHandsMap,
							copySummary);
						++copyPoints[highestSolo[cur]];
						for (unsigned i=1; i<highestSolo.size(); ++i)
						{
							--copyPoints[highestSolo[i]];
							map<HandType, vector<Hand>> tmpHandsMap;
							HandsMapSummary tmpSummary;
							sortHandsLordOnly1Card(copyPoints,
								lowestControl,
								maxOppNumber,
								otherPoints,
								true,
								tmpHandsMap,
								tmpSummary);
							if (tmpSummary.soloCount < copySummary.soloCount
								|| (tmpSummary.soloCount == copySummary.soloCount
								&& tmpSummary.lowestSolo > copySummary.lowestSolo))
							{
								cur = i;
								copySummary = tmpSummary;
							}
							++copyPoints[highestSolo[i]];
						}

						hand.type = SOLO;
						hand.keyPoint = CardPoint(highestSolo[cur]);
					}
					vector<Hand> &bombHands = handsMap[BOMB];
					vector<Hand> &soloHands = handsMap[SOLO];
					if (bombHands.size() > 0 && soloHands.size() == 1)
					{
						hand = bombHands.front();
						return;
					}
				}
			}
			else
			{
				findHigherHandNotBomb(hand);
				return;
			}
		}
	}
}

void OGLordRobotAI::farmerTakeOutLordOnly2Cards( Hand &hand )
{
	hand.type = NOTHING;
	vector<Hand> &soloHands = handsMap[SOLO];	
	vector<Hand> &pairHands = handsMap[PAIR];
	if (isFree())
	{		
		if (soloHands.size() == 0 && pairHands.size() == 0)
		{
			return;
		}
		else
		{
			if (handsMap[BOMB].size() > 0 || hasUniHighestSolo(playerInfo[aiSeat].points, otherPoints))
			{
				if (soloHands.size() == 0)
				{
					hand.type = SOLO;
					hand.keyPoint = pairHands.front().keyPoint;
					return;
				}
				else
				{
					CardPoint oppHighest = getHighestPoint(otherPoints);
					for (vector<Hand>::reverse_iterator it=soloHands.rbegin(); it!=soloHands.rend(); ++it)
					{
						if (it->keyPoint <= oppHighest)
						{
							hand = *it;
							return;
						}
					}
				}
			}
			else
			{
				farmerTakeOutLordOnly1Card(hand);
				return;
			}
		}
	}
	else
	{
		if (curHand.type == PAIR && curHandSeat == lordSeat)
		{
			CardPoint highestPair = getHighestPairPoint(otherPoints);
			for (unsigned i=0; i<pairHands.size(); ++i)
			{
				if (pairHands[i].keyPoint >= max_ddz(curHand.keyPoint + 1, (int)highestPair))
				{
					hand = pairHands[i];
					return;
				}
			}
		}
		farmerMustTakeOutLordCharge(hand);
	}
}

void OGLordRobotAI::farmerMustTakeOutLordOnly1Card( Hand &hand )
{
	hand.type = NOTHING;
	findHigherHandNotBomb(hand);
	if (hand.type == NOTHING)
	{
		findBestHigherHandFromPoints(hand, true, true);
	}
}

void OGLordRobotAI::farmerMustTakeOutLordCharge( Hand &hand )
{
	hand.type = NOTHING;
	Hand copyHand;
	findHigherHandNotBomb(copyHand);
	if (copyHand.type != NOTHING)
	{
		hand = copyHand;
		return;
	}
	
	int handPoints[CARD_POINT_NUM];
	handToPointsArray(copyHand, handPoints);
	int copyAiPoints[CARD_POINT_NUM];
	int copyRemainPoints[CARD_POINT_NUM];
	copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, copyAiPoints);
	copy(remainPoints, remainPoints + CARD_POINT_NUM, copyRemainPoints);

	pointsSubEqual(copyAiPoints, handPoints);
	pointsSubEqual(copyRemainPoints, handPoints);
	CardPoint copyLowestCon = getLowestControl(copyRemainPoints);
	int copyCon = calControl(copyAiPoints, otherPoints, copyLowestCon);

	map<HandType, vector<Hand>> copyHandsMap = handsMap;
	vector<Hand> &copyHands = copyHandsMap[copyHand.type];
	for (vector<Hand>::iterator it=copyHands.begin(); it!=copyHands.end(); ++it)
	{
		if (it->keyPoint == copyHand.keyPoint
			&& (!isChain(copyHand.type) || it->len == copyHand.len))
		{
			copyHands.erase(it);
			break;
		}
	}
	HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestCon, maxOppNumber, otherPoints);

	if (copySummary.effectiveHandsCount < copyCon + 2)
	{
		hand = copyHand;
		return;
	}

	findBestHigherHandFromPoints(hand, true, false);
}

void OGLordRobotAI::goodFarmerOverOtherFarmer( Hand &hand )
{
	hand.type = NOTHING;
	Hand copyHand;
	findHigherHandNotBomb(copyHand);

	if (copyHand.type != NOTHING)
	{
		if (summary.realHandsCount - 1 <= (int)handsMap[BOMB].size())
		{
			hand = copyHand;
			return;
		}
	}

	int otherFarmerSeat = aiPosition == DOWNFARMER ? (aiSeat + 1) % 3 : (aiSeat + 2) % 3;
	if (playerInfo[otherFarmerSeat].total <= 2
		|| curHand.keyPoint >= CARD_K
		|| isChain(curHand.type)
		|| curHand.type == FOUR_DUAL_SOLO
		|| curHand.type == FOUR_DUAL_PAIR)
	{
		return;
	}

	if (copyHand.type != NOTHING
		&& calHandControl(copyHand, lowestControl) == 0)
	{
		hand = copyHand;
		return;
	}
	findBestHigherHandFromPoints(hand, true, false);
}

void OGLordRobotAI::downGoodFarmerTakeOut( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[(aiSeat + 1) % 3].total == 1)
		{
			downFarmerTakeOutUpFarmerOnly1Card(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (summary.realHandsCount == 2 && summary.unChargeHandsCount == 2)
		{
			for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
			{
				HandType type = it->first;
				vector<Hand> &hands = it->second;
				if (hands.size() > 0)
				{
					if (type != SOLO && type != PAIR)
					{
						hand = hands.front();
						return;
					}
					else
					{
						if (hand.type == NOTHING
							|| hands.front().keyPoint < hand.keyPoint
							|| (hands.front().keyPoint == hand.keyPoint && type == PAIR))
						{
							hand = hands.front();
							return;
						}
					}
				}
			}
		}
		else if (isGood(summary, controlNum, minOppNumber))
		{
			int oppHighestBomb = getHighestBomb(otherPoints);
			bool charge = false;
			if (oppHighestBomb < 0 || 
				(handsMap[BOMB].size() > 0 && handsMap[BOMB].back().keyPoint > oppHighestBomb))
			{
				charge = (unsigned)summary.realHandsCount <= (handsMap[BOMB].size() + 1);
			}
			else
			{
				charge = (unsigned)summary.realHandsCount <= handsMap[BOMB].size();
			}
			if (charge)
			{
				findChargeHandFirst(hand, true);
			}
			else
			{
				findChargeHandFirst(hand, true);

				int handPoints[CARD_POINT_NUM];
				handToPointsArray(hand, handPoints);
				int copyAiPoints[CARD_POINT_NUM];
				int copyRemainPoints[CARD_POINT_NUM];
				copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, copyAiPoints);
				copy(remainPoints, remainPoints + CARD_POINT_NUM, copyRemainPoints);

				pointsSubEqual(copyAiPoints, handPoints);
				pointsSubEqual(copyRemainPoints, handPoints);
				CardPoint copyLowestCon = getLowestControl(copyRemainPoints);
				int copyCon = calControl(copyAiPoints, otherPoints, copyLowestCon);

				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyHands = copyHandsMap[hand.type];
				for (vector<Hand>::iterator it=copyHands.begin(); it!=copyHands.end(); ++it)
				{
					if (it->keyPoint == hand.keyPoint
						&& (!isChain(hand.type) || it->len == hand.len))
					{
						copyHands.erase(it);
						break;
					}
				}
				HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestCon, maxOppNumber, otherPoints);
				if (copySummary.effectiveHandsCount < copyCon + 1)
				{
					return;
				}
				else
				{
					findLowestHand(hand);
				}
			}
		}
		else
		{
			findLowestHand(hand);
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			goodFarmerOverOtherFarmer(hand);
		}
		else
		{
			if (curHand.type == PAIR_CHAIN
				|| curHand.type == TRIO_CHAIN
				|| curHand.type == TRIO_CHAIN_SOLO
				|| curHand.type == TRIO_CHAIN_PAIR
				|| (curHand.type == SOLO_CHAIN && curHand.len >= 8)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
			}
			else
			{
				if (curHand.type == SOLO 
					&& (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER))
				{
					farmerTakeOutWhenLordTakeOutHighSolo(hand);
					if (hand.type != NOTHING)
					{
						return;
					}
				}				
				findBestHigherHandFromMap(hand);
				if (hand.type == NOTHING)
				{
					if (handsMap[BOMB].size() > 0)
					{
						if (!isDanger())
						{
							map<HandType, vector<Hand>> copyHandsMap = handsMap;
							vector<Hand> &copyBomb = copyHandsMap[BOMB];
							int handPoint[CARD_POINT_NUM];
							int copyAiPoint[CARD_POINT_NUM];
							int copyRemainPoint[CARD_POINT_NUM];
							handToPointsArray(copyBomb.front(), handPoint);
							pointsSub(playerInfo[aiSeat].points, handPoint, copyAiPoint);
							pointsSub(remainPoints, handPoint, copyRemainPoint);
							CardPoint copyLowestControl = getLowestControl(copyRemainPoint);
							int copyControlNum = calControl(copyAiPoint, otherPoints, copyLowestControl);

							copyBomb.erase(copyBomb.begin());
							HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestControl, maxOppNumber, otherPoints);
							if (isGood(copySummary, copyControlNum, minOppNumber))
							{
								hand = handsMap[BOMB].front();
								return;
							}
						}
					}
					findBestHigherHandFromPoints(hand, false, false);
				}
			}
		}
	}
}

void OGLordRobotAI::downBadFarmerTakeOut( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[(aiSeat + 1) % 3].total == 1)
		{
			downFarmerTakeOutUpFarmerOnly1Card(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		findLowestHand(hand);
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			hand.type = NOTHING;
			return;
		}
		else
		{
			if ((curHand.type != SOLO && curHand.type != PAIR)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
				return;
			}
			else
			{
				if (curHand.type == SOLO 
					&& (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER))
				{
					farmerTakeOutWhenLordTakeOutHighSolo(hand);
					if (hand.type != NOTHING)
					{
						return;
					}
				}
				if (!isFirstHalf())
				{
					farmerMustTakeOutLordCharge(hand);
					return;
				}
				findBestHigherHandFromMap(hand);
			}
		}
	}
}

void OGLordRobotAI::upGoodFarmerTakeOut( Hand &hand )
{
	if (isFree())
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (isGood(summary, controlNum, minOppNumber))
		{
			findLowestHand(hand);
		}
		else
		{
			if (playerInfo[(aiSeat + 2) % 3].total == 1)
			{
				if (handsMap[SOLO].size() > 0 && handsMap[SOLO].front().keyPoint < lowestControl)
				{
					hand = handsMap[SOLO].front();
					return;
				}
			}
			else if (playerInfo[(aiSeat + 2) % 3].total == 2)
			{
				if (handsMap[PAIR].size() > 0 && handsMap[PAIR].front().keyPoint < CARD_T)
				{
					hand = handsMap[PAIR].front();
					return;
				}
			}

			if (summary.unChargeHandsCount == 2)
			{
				findChargeHandFirst(hand, true);
				if (hand.type != NOTHING)
				{
					return;
				}
				else
				{
					findLowestHand(hand);
					return;
				}
			}
			else
			{
				CardPoint oppLowest = getLowestPoint(otherPoints);
				findLowestHand(hand);
				if (hand.keyPoint <= oppLowest)
				{
					return;
				}
				else
				{
					findMostCardsHandNotBomb(hand);
					return;
				}
			}
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			goodFarmerOverOtherFarmer(hand);
		}
		else
		{
			if ((curHand.type != SOLO && curHand.type != PAIR)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
				return;
			}
			else
			{
				farmerMustTakeOutLordCharge(hand);
			}
		}
	}
}

void OGLordRobotAI::upBadFarmerTakeOut( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[(aiSeat + 2) % 3].total == 1)
		{
			hand.type = SOLO;
			hand.keyPoint = getLowestPoint(playerInfo[aiSeat].points);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		else if (playerInfo[(aiSeat + 2) % 3].total == 2)
		{
			CardPoint lowestPair = getLowestPairPoint(playerInfo[aiSeat].points);
			if (!isHighestPair(lowestPair, otherPoints))
			{
				hand.type = PAIR;
				hand.keyPoint = lowestPair;
				return;
			}
		}
		vector<Hand> &pairHands = handsMap[PAIR];
		for (unsigned i=0; i<pairHands.size(); ++i)
		{
			if (pairHands[i].keyPoint <= CARD_8)
			{
				hand = pairHands[i];
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}

		vector<Hand> &soloHands = handsMap[SOLO];
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint < lowestControl)
			{
				hand = soloHands[i];
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}

		for (unsigned i=0; i<pairHands.size(); ++i)
		{
			if (pairHands[i].keyPoint < lowestControl)
			{
				hand.type = SOLO;
				hand.keyPoint = pairHands[i].keyPoint;
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}
		findLowestHand(hand);
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}

		if (curHand.type == SOLO
			&& curHand.keyPoint < min_ddz(CARD_J, lowestControl))
		{
			int higherP = max_ddz((int)CARD_J, curHand.keyPoint + 1);
			vector<Hand> &soloHands = handsMap[SOLO];
			for (unsigned i=0; i<soloHands.size(); ++i)
			{
				if (soloHands[i].keyPoint > CARD_A)
				{
					break;
				}
				if (soloHands[i].keyPoint >= higherP)
				{
					hand = soloHands[i];
				}				
			}
			if (hand.type != NOTHING)
			{
				return;
			}

			vector<Hand> &pairHands = handsMap[PAIR];
			for (unsigned i=0; i<pairHands.size(); ++i)
			{
				if (pairHands[i].keyPoint >= higherP)
				{
					hand.type = SOLO;
					hand.keyPoint = pairHands[i].keyPoint;
				}
				else
				{
					break;;
				}
			}
			if (hand.type != NOTHING)
			{
				return;
			}

			for (unsigned i=0; i<soloHands.size(); ++i)
			{
				if (soloHands[i].keyPoint > curHand.keyPoint
					&& soloHands[i].keyPoint <= CARD_2)
				{
					hand = soloHands[i];
					return;
				}
			}

			for (int i=curHand.keyPoint+1; i<= CARD_2; ++i)
			{
				if (playerInfo[aiSeat].points[i] > 0 && playerInfo[aiSeat].points[i] < 4)
				{
					hand.type = SOLO;
					hand.keyPoint = CardPoint(i);
					return;
				}
			}
		}
		else
		{
			if (curHandSeat == lordSeat)
			{
				if (curHand.type == SOLO)
				{
					if (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER)
					{
						farmerTakeOutWhenLordTakeOutHighSolo(hand);
						return;
					}
					else if (curHand.keyPoint >= CARD_J && curHand.keyPoint <= CARD_A)
					{
						for (int i=curHand.keyPoint+1; i<= CARD_2; ++i)
						{
							if (playerInfo[aiSeat].points[i] > 0 && playerInfo[aiSeat].points[i] < 4)
							{
								hand.type = SOLO;
								hand.keyPoint = CardPoint(i);
								return;
							}
						}
						return;
					}
				}
				farmerMustTakeOutLordCharge(hand);
			}
		}
	}
}

void OGLordRobotAI::downFarmerTakeOutUpFarmerOnly1Card( Hand &hand )
{
	hand.type = NOTHING;
	CardPoint lowestPoint = getLowestPoint(playerInfo[aiSeat].points);
	if (hasUniLowestSolo(playerInfo[aiSeat].points, otherPoints))
	{
		map<HandType, vector<Hand>> bombMap;
		getBomb(playerInfo[aiSeat].points, bombMap);
		if (bombMap[BOMB].size() > 0)
		{
			int oppHighestBomb = getHighestBomb(otherPoints);
			if (bombMap[BOMB].back().keyPoint > oppHighestBomb)
			{
				if (oppHighestBomb < 0 || playerInfo[lordSeat].total != 4)
				{
					hand = bombMap[BOMB].front();
				}
				else
				{
					hand = bombMap[BOMB].back();
				}
			}
			else
			{
				hand.type = SOLO;
				hand.keyPoint = lowestPoint;
			}
		}
		else
		{
			hand.type = SOLO;
			hand.keyPoint = lowestPoint;
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
		}
		else if (!isGood(summary, controlNum, minOppNumber))
		{
			hand.type = SOLO;
			hand.keyPoint = lowestPoint;
		}
	}
}

void OGLordRobotAI::findBestHigherHandFromMap( Hand &hand )
{
	hand.type = NOTHING;
	vector<Hand> &aiHands = handsMap[curHand.type];
	if (aiHands.size() > 0)
	{
		Hand *higherHand = NULL;
		for (unsigned i=0; i<aiHands.size(); ++i)
		{
			if (isHandHigherThan(aiHands[i], curHand))
			{
				higherHand = &aiHands[i];
				break;
			}
		}
		if (higherHand != NULL)
		{
			if (isChain(curHand.type)
				|| !containsControl(lowestControl, *higherHand))
			{
				hand = *higherHand;
				return;
			}
			else
			{
				int handPoints[CARD_POINT_NUM];
				handToPointsArray(*higherHand, handPoints);
				int copyAiPoints[CARD_POINT_NUM];
				int copyRemainPoints[CARD_POINT_NUM];
				copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, copyAiPoints);
				copy(remainPoints, remainPoints + CARD_POINT_NUM, copyRemainPoints);

				pointsSubEqual(copyAiPoints, handPoints);
				pointsSubEqual(copyRemainPoints, handPoints);
				CardPoint copyLowestCon = getLowestControl(copyRemainPoints);
				int copyCon = calControl(copyAiPoints, otherPoints, copyLowestCon);

				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyHands = copyHandsMap[higherHand->type];
				for (vector<Hand>::iterator it=copyHands.begin(); it!=copyHands.end(); ++it)
				{
					if (it->keyPoint == higherHand->keyPoint
						&& (!isChain(higherHand->type) || it->len == higherHand->len))
					{
						copyHands.erase(it);
						break;
					}
				}
				HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestCon, maxOppNumber, otherPoints);
				if (copySummary.effectiveHandsCount < copyCon + 2
					|| copyCon + copySummary.extraBombCount >= controlNum + summary.extraBombCount
					|| copyCon + copySummary.extraBombCount >= 3)
				{
					hand = *higherHand;
					return;
				}
			}
		}
	}
}

void OGLordRobotAI::findBestHigherHandFromPoints( Hand &hand, bool force, bool lordOnly1Card)
{
	hand.type = NOTHING;
	Hand nowHand = curHand;
	vector<Hand> higherHands;
	vector<int> higherHandsControl;
	vector<HandsMapSummary> higherHandsSummary;
	Hand higher;
	do 
	{
		findHigherHandFromPoints(nowHand, playerInfo[aiSeat].points, lowestControl, maxOppNumber, otherPoints, &higher);
		if (higher.type != NOTHING)
		{
			int copyPoints[CARD_POINT_NUM];
			int handPoints[CARD_POINT_NUM];
			handToPointsArray(higher, handPoints);
			pointsSub(playerInfo[aiSeat].points, handPoints, copyPoints);
			int copyControl = calControl(copyPoints, otherPoints, lowestControl);

			map<HandType, vector<Hand>> copyHandsMap;
			HandsMapSummary copySummary;
			sortHandsWithBestKind(copyPoints, lowestControl, maxOppNumber, otherPoints,
				true, copyHandsMap, copySummary);
			if (calHandControl(higher, lowestControl) == 0
				&& (copySummary.effectiveHandsCount < summary.effectiveHandsCount
					|| (copySummary.effectiveHandsCount == summary.effectiveHandsCount
						&& isChargeHand(higher, maxOppNumber, otherPoints, &handsMap))))
			{
				hand = higher;
				return;
			}
			if (force)
			{
				higherHands.push_back(higher);
				higherHandsControl.push_back(copyControl);
				higherHandsSummary.push_back(copySummary);
			}
			else if (copySummary.effectiveHandsCount - 2 < copyControl
				|| copySummary.effectiveHandsCount - copyControl <= summary.effectiveHandsCount - controlNum
				|| copyControl + copySummary.extraBombCount >= 3)
			{
				higherHands.push_back(higher);
				higherHandsControl.push_back(copyControl);
				higherHandsSummary.push_back(copySummary);
			}
			nowHand = higher;
		}
		else
		{
			break;
		}
	} while (true);

	if (higherHands.size() > 0)
	{
		int cur = 0;
		if (lordOnly1Card)
		{
			for (unsigned i=1; i<higherHands.size(); ++i)
			{
				if (aiPosition == DOWNFARMER)
				{
					if (higherHandsSummary[i].sencondLowestSolo > higherHandsSummary[cur].sencondLowestSolo)
					{
						cur = i;
					}
				}
				else
				{
					if (higherHandsSummary[i].lowestSolo > higherHandsSummary[cur].lowestSolo)
					{
						cur = i;
					}
				}
			}
		}
		else
		{
			for (unsigned i=1; i<higherHands.size(); ++i)
			{
				if (higherHandsSummary[i].effectiveHandsCount - higherHandsControl[i]
				< higherHandsSummary[cur].effectiveHandsCount - higherHandsControl[cur])
				{
					cur = i;
				}
				else if (higherHandsSummary[i].effectiveHandsCount - higherHandsControl[i] 
				== higherHandsSummary[cur].effectiveHandsCount - higherHandsControl[cur])
				{
					if (higherHandsSummary[i].effectiveHandsCount < higherHandsSummary[cur].effectiveHandsCount)
					{
						cur = i;
					}
					else if (higherHandsSummary[i].effectiveHandsCount == higherHandsSummary[cur].realHandsCount)
					{
						if (higherHandsSummary[i].handsTypeCount < higherHandsSummary[cur].handsTypeCount)
						{
							cur = i;
						}
						else if (higherHandsSummary[i].handsTypeCount == higherHandsSummary[cur].handsTypeCount)
						{
							if (higherHandsSummary[i].soloCount < higherHandsSummary[cur].soloCount)
							{
								cur = i;
							}
						}
					}
				}
			}
		}		
		hand = higherHands[cur];
	}
}

void OGLordRobotAI::farmerTakeOutWhenLordTakeOutHighSolo( Hand &hand )
{
	hand.type = NOTHING;
	if (curHand.keyPoint == CARD_2)
	{
		if (playerInfo[aiSeat].points[BLACK_JOKER] == 1)
		{
			hand.type = SOLO;
			hand.keyPoint = BLACK_JOKER;
		}
		else if (otherPoints[BLACK_JOKER] == 0 && playerInfo[aiSeat].points[RED_JOKER] == 1)
		{
			hand.type = SOLO;
			hand.keyPoint = RED_JOKER;
		}
	}
	else if (curHand.keyPoint == BLACK_JOKER)
	{
		if (playerInfo[aiSeat].points[RED_JOKER] == 1)
		{
			hand.type = SOLO;
			hand.keyPoint = RED_JOKER;
		}
	}
}

void OGLordRobotAI::refindForTrio(Hand &hand)
{
	int copyPoints[CARD_POINT_NUM];
	int handPoints[CARD_POINT_NUM];
	handToPointsArray(hand, handPoints);
	pointsSub(playerInfo[aiSeat].points, handPoints, copyPoints);

	map<HandType, vector<Hand>> copyHandsMap;
	HandsMapSummary copySummary;
	sortHandsWithBestKind(copyPoints, lowestControl, maxOppNumber, otherPoints,
		false, copyHandsMap, copySummary);
	if (hand.type == TRIO_SOLO || hand.type == TRIO_PAIR || hand.type == TRIO)
	{
		vector<Hand> &trioHands = copyHandsMap[TRIO];
		for (unsigned i=0; i<trioHands.size(); ++i)
		{
			if ((isFree() || trioHands[i].keyPoint > curHand.keyPoint)
				&& trioHands[i].keyPoint < hand.keyPoint)
			{
				hand.keyPoint = trioHands[i].keyPoint;
				break;
			}
		}
	}
	else if (hand.type == TRIO_CHAIN_SOLO || hand.type == TRIO_CHAIN_PAIR)
	{
		vector<Hand> &trioChainHands = copyHandsMap[TRIO_CHAIN];
		for (unsigned i=0; i<trioChainHands.size(); ++i)
		{
			if ((isFree() || trioChainHands[i].keyPoint > curHand.keyPoint)
				&& trioChainHands[i].keyPoint < hand.keyPoint
				&& trioChainHands[i].len == hand.len)
			{
				hand.keyPoint = trioChainHands[i].keyPoint;
				break;
			}
		}
	}
	int kickerSize = 0;
	if (hand.type == TRIO_SOLO || hand.type == TRIO_PAIR)
	{
		kickerSize = 1;
	}
	else if (hand.type == TRIO_CHAIN_SOLO || hand.type == TRIO_CHAIN_PAIR)
	{
		kickerSize = hand.len;
	}
	if (kickerSize > 0)
	{
		set<CardPoint> kickerSet;
		for (int i=0; i<kickerSize; ++i)
		{
			kickerSet.insert(hand.kicker[i]);
		}
		
		vector<Hand> &kickerHands = (hand.type == TRIO_SOLO || hand.type == TRIO_CHAIN_SOLO) 
			? copyHandsMap[SOLO] : copyHandsMap[PAIR];
		for (unsigned i=0; i<kickerHands.size(); ++i)
		{
			if (kickerHands[i].keyPoint < hand.kicker[kickerSize - 1] 
                && (kickerHands[i].keyPoint < hand.keyPoint || kickerHands[i].keyPoint >= hand.keyPoint + hand.len))//不带同样的牌，如333444带35不可以
			{
				kickerSet.insert(kickerHands[i].keyPoint);
			}
			else
			{
				break;
			}
		}

		if (kickerSet.size() > (unsigned)kickerSize)
		{
			int i=0;
			for (set<CardPoint>::iterator it=kickerSet.begin(); it!=kickerSet.end(); ++it)
			{
				if (i<kickerSize)
				{
					hand.kicker[i++] = *it;
				}
				else
				{
					break;
				}
			}
		}
	}
}

bool OGLordRobotAI::isDangerLvl()
{
	bool danger = false;
	int highBomb = -1;
	if (aiSeat == lordSeat)
	{
		highBomb = max_ddz(getHighestBomb(playerInfo[upSeat].points), 
			getHighestBomb(playerInfo[downSeat].points));
	}
	else
	{
		highBomb = getHighestBomb(playerInfo[lordSeat].points);
	}
	if (highBomb > 0)
	{
		vector<Hand> &aiBombs = handsMap[BOMB];
		if (aiBombs.size() == 0
			|| aiBombs.back().keyPoint < highBomb)
		{
			danger = true;
		}
	}
	return danger;
}

bool OGLordRobotAI::isGoodLvl( HandsMapSummary &hmSummary, int minOppNum)
{
	bool ret = false;
	if (minOppNum == 1)
	{
		int oppHighestPoint = getHighestPoint(otherPoints);
		vector<Hand> &soloHands = handsMap[SOLO];
		int lowSoloCount = 0;
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint < oppHighestPoint)
			{
				++lowSoloCount;
			}
		}
		if (aiPosition == LORD)
		{
			ret = hmSummary.effectiveHandsCount <= 1 && lowSoloCount <= 1;
		}
		else
		{
			ret = lowSoloCount <= 1;
		}
	}
	else
	{
		ret = hmSummary.effectiveHandsCount <= 1;
	}
	return ret;
}

void OGLordRobotAI::findKickerLvl( Hand &hand )
{
	bool rebuild = !containsHand(handsMap, hand);
	int trioPoints[CARD_POINT_NUM];
	if (rebuild)
	{		
		handToPointsArray(hand, trioPoints);
		int remainPoints[CARD_POINT_NUM];
		pointsSub(playerInfo[aiSeat].points, trioPoints, remainPoints);
		sortHandMapLvl();
	}	
	unsigned kickerCount = hand.type == TRIO_CHAIN ? hand.len : 1;
	vector<Hand> soloHands = handsMap[SOLO];
	if (hand.type == TRIO_CHAIN)
	{
		for (vector<Hand>::iterator it=soloHands.begin(); it!=soloHands.end();)
		{
			if (it->keyPoint >= hand.keyPoint || it->keyPoint <= hand.keyPoint + hand.len - 1)
			{
				it = soloHands.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
	vector<Hand> pairHands = handsMap[PAIR];
	if (soloHands.size() + pairHands.size() > (unsigned)kickerCount)
	{
		if (kickerCount == 1)
		{
			if (pairHands.size() == 0)
			{
				if (soloHands.size() > 1 ||
					findOppHighestPoint(SOLO) > soloHands[0].keyPoint)
				{
					hand.type = TRIO_SOLO;
					hand.kicker[0] = soloHands[0].keyPoint;
				}
			}
			else if (soloHands.size() == 0)
			{
				if (pairHands.size() > 1 ||
					findOppHighestPoint(PAIR) > pairHands[0].keyPoint)
				{
					hand.type = TRIO_PAIR;
					hand.kicker[0] = pairHands[0].keyPoint;
				}
			}
			else
			{
				if (soloHands[0].keyPoint < CARD_T || soloHands[0].keyPoint < pairHands[0].keyPoint)
				{
					hand.type = TRIO_SOLO;
					hand.kicker[0] = soloHands[0].keyPoint;
				}
				else
				{
					hand.type = TRIO_PAIR;
					hand.kicker[0] = pairHands[0].keyPoint;
				}
			}
		}
		else
		{
			if (soloHands.size() >= kickerCount && pairHands.size() < kickerCount)
			{
				hand.type = TRIO_CHAIN_SOLO;
				for (unsigned i=0; i<kickerCount; ++i)
				{
					hand.kicker[i] = soloHands[i].keyPoint;
				}
			}
			else if (soloHands.size() < kickerCount && pairHands.size() >= kickerCount)
			{
				hand.type = TRIO_CHAIN_PAIR;
				for (unsigned i=0; i<kickerCount; ++i)
				{
					hand.kicker[i] = pairHands[i].keyPoint;
				}
			}
			else if (soloHands.size() >= kickerCount && pairHands.size() >= kickerCount)
			{
				if ((pairHands[0].keyPoint < soloHands[0].keyPoint
						&& pairHands[kickerCount - 1].keyPoint < soloHands[kickerCount - 1].keyPoint)
					|| (soloHands.size() == kickerCount 
						&& findOppHighestPoint(SOLO) < soloHands.back().keyPoint
						&& findOppHighestPoint(SOLO) >= pairHands.back().keyPoint))
				{
					hand.type = TRIO_CHAIN_PAIR;
					for (unsigned i=0; i<kickerCount; ++i)
					{
						hand.kicker[i] = pairHands[i].keyPoint;
					}
				}
				else
				{
					hand.type = TRIO_CHAIN_SOLO;
					for (unsigned i=0; i<kickerCount; ++i)
					{
						hand.kicker[i] = soloHands[i].keyPoint;
					}
				}				
			}
			else
			{
				if (findOppHighestPoint(TRIO) < hand.keyPoint)
				{
					hand.type = TRIO;
					hand.len = 1;
					findKickerLvl(hand);
				}
				else
				{
					vector<CardPoint> kickers;
					for (unsigned i=0; i<soloHands.size(); ++i)
					{
						if (i < soloHands.size() - 1
							|| findOppHighestPoint(SOLO) >= soloHands[i].keyPoint)
						{
							kickers.push_back(soloHands[i].keyPoint);
						}						
					}
					for (unsigned i=0; i<pairHands.size(); ++i)
					{
						if (kickers.size() < kickerCount)
						{
							kickers.push_back(pairHands[i].keyPoint);
						}
						else
						{
							break;
						}
					}
					if (kickers.size() == kickerCount)
					{
						sort(kickers.begin(), kickers.end());
						hand.type = TRIO_CHAIN_SOLO;
						for (unsigned i=0; i<kickerCount; ++i)
						{
							hand.kicker[i] = kickers[i];
						}
					}
				}
			}
		}
	}
}

void OGLordRobotAI::lordTakeOutFreeLvl( Hand &hand )
{
	CardsInfo &aiCard = playerInfo[aiSeat];
	if (playerInfo[(aiSeat + 1) % 3].total == 1
		|| playerInfo[(aiSeat + 2) % 3].total == 1)
	{
		lordTakeOutFreeFarmerOnly1Card(hand);
	}
	else if ((playerInfo[(aiSeat + 1) % 3].total == 2 
		|| playerInfo[(aiSeat + 2) % 3].total == 2)
		&& (handsMap[SOLO].size() > 0 || handsMap[PAIR].size() > 0))
	{
		lordTakeOutFreeFarmerOnly2CardsLvl(hand);
	}
	else
	{
		lordTakeOutFreeNormalLvl(hand);
	}
}

void OGLordRobotAI::lordTakeOutFreeFarmerOnly2CardsLvl( Hand &hand )
{
	int pairPoint = -1;
	if (playerInfo[(aiSeat + 1) % 3].total == 2)
	{
		CardPoint p = getLowestPairPoint(playerInfo[(aiSeat + 1) % 3].points);
		if (p >= CARD_3 && p <= CARD_2)
		{
			pairPoint = p;
		}
	}
	if (playerInfo[(aiSeat + 2) % 3].total == 2)
	{
		CardPoint p = getLowestPairPoint(playerInfo[(aiSeat + 1) % 3].points);
		if (p >= CARD_3 && p <= CARD_2 && p > pairPoint)
		{
			pairPoint = p;
		}
	}
	if (pairPoint == -1)
	{
		lordTakeOutFreeFarmerOnly1Card(hand);
	}
	else
	{
		lordTakeOutFreeFarmerOnly2Cards(hand);
	}
}

void OGLordRobotAI::lordTakeOutFreeNormalLvl( Hand &hand )
{
	if (!isDangerLvl()
		&& summary.effectiveHandsCount == 1)
	{
		if (handsMap[BOMB].size() > 0 && summary.realHandsCount == 1)
		{
			hand = handsMap[BOMB].front();
		}
		else
		{
			findChargeHandFirstLvl(hand, false);
		}
	}
	else
	{
		findLowestHand(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherLvl( Hand &hand )
{
	if (playerInfo[curHandSeat].total == 1 || playerInfo[(aiSeat + 2) % 3].total == 1)
	{
		lordTakeOutHigherFarmerOnly1Card(hand);
	}
	else if (playerInfo[curHandSeat].total == 2 || playerInfo[(aiSeat + 2) % 3].total == 2)
	{
		lordTakeOutHigherFarmerOnly2CardsLvl(hand);
	}
	else
	{
		lordTakeOutHigherNormalLvl(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherFarmerOnly1CardLvl( Hand &hand )
{
	hand.type = NOTHING;
	if (curHand.type == SOLO)
	{
		if (handsMap[SOLO].size() >= 2)
		{
			for (unsigned i=1; i<handsMap[SOLO].size(); ++i)
			{
				if (handsMap[SOLO][i].keyPoint > curHand.keyPoint)
				{
					hand = handsMap[SOLO][i];
					return;
				}
			}
		}
		if (handsMap[SOLO].size() == 1)
		{
			bool isHighest = true;
			for (int i = handsMap[SOLO][0].keyPoint + 1; i <= RED_JOKER; ++i)
			{
				if (otherPoints[i] > 0)
				{
					isHighest = false;
					break;
				}
			}
			if (isHighest)
			{
				hand = handsMap[SOLO][0];
				return;
			}
		}
		if (handsMap[BOMB].size() > 0)
		{
			if (!isDangerLvl())
			{
				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyBomb = copyHandsMap[BOMB];
				copyBomb.erase(copyBomb.begin());

				map<HandType, int> copyUniHighHands;
				map<HandType, int> copyHighHands;
				map<HandType, int> copyGoodLowHands;
				int copyTrioCount = 0;
				HandsMapSummary copySummary = getHandsMapSummaryLvl(copyHandsMap, 
					copyUniHighHands, 
					copyHighHands, 
					copyGoodLowHands, 
					copyTrioCount);
				if (isGoodLvl(copySummary, minOppNumber))
				{
					hand = handsMap[BOMB].front();
					return;
				}
			}
		}
		int highest = CARD_3;
		for (int i=RED_JOKER; i>=CARD_3; --i)
		{
			if (playerInfo[aiSeat].points[i] > 0)
			{
				highest = i;
				break;
			}
		}
		if (highest > curHand.keyPoint)
		{
			hand.type = SOLO;
			hand.keyPoint = CardPoint(highest);
			return;
		}
	}
	else
	{
		lordTakeOutHigherNormalLvl(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherFarmerOnly2CardsLvl( Hand &hand )
{
	hand.type = NOTHING;
	if (curHand.type == PAIR)
	{
		int highestPair = findOppHighestPoint(PAIR);
		if (highestPair >= 0)
		{
			vector<Hand> &aiPairs = handsMap[PAIR];
			for (vector<Hand>::reverse_iterator it=aiPairs.rbegin(); it!=aiPairs.rend(); ++it)
			{
				if (it->keyPoint >= highestPair)
				{
					hand = *it;
					return;
				}
			}
			lordTakeOutHigherRebuildLvl(hand);
		}
		else
		{
			lordTakeOutHigherNormalLvl(hand);
		}
	}
}

void OGLordRobotAI::lordTakeOutHigherNormalLvl( Hand &hand )
{
	hand.type = NOTHING;
	findBestHigherHandFromMap(hand);
	if (hand.type == NOTHING)
	{
		if (handsMap[BOMB].size() > 0)
		{
			if (!isDangerLvl())
			{
				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyBomb = copyHandsMap[BOMB];
				copyBomb.erase(copyBomb.begin());

				map<HandType, int> copyUniHighHands;
				map<HandType, int> copyHighHands;
				map<HandType, int> copyGoodLowHands;
				int copyTrioCount = 0;
				HandsMapSummary copySummary = getHandsMapSummaryLvl(copyHandsMap, 
					copyUniHighHands, 
					copyHighHands, 
					copyGoodLowHands, 
					copyTrioCount);

				if (isGoodLvl(copySummary, minOppNumber))
				{
					hand = handsMap[BOMB].front();
					return;
				}
			}
		}
		lordTakeOutHigherRebuildLvl(hand);
	}
}

void OGLordRobotAI::lordTakeOutHigherRebuildLvl( Hand &hand )
{
	hand.type = NOTHING;
	findBestHigherHandFromPoints(hand, false, false);
}

void OGLordRobotAI::farmerTakeOutLordOnly1CardLvl( Hand &hand )
{
	int *aiPoints = playerInfo[aiSeat].points;

	hand.type = NOTHING;
	if (isFree())
	{
		if (aiPosition == UPFARMER)
		{
			if (isGoodLvl(summary, minOppNumber))
			{
				findMostCardsHand(handsMap, hand);
			}
			else
			{
				for (int i=CARD_3; i<=CARD_2; ++i)
				{
					if (aiPoints[i] == 2 || aiPoints[i] == 3)
					{
						hand.type = PAIR;
						hand.keyPoint = CardPoint(i);
						return;
					}
				}
				vector<Hand> &soloHands = handsMap[SOLO];
				if (soloHands.size() == 2 || soloHands.size() == 3)
				{
					if (!isHighestSolo(soloHands.back().keyPoint, otherPoints))
					{
						vector<Hand> &bombHands = handsMap[BOMB];
						if (bombHands.size() > 0 && bombHands.front().keyPoint < BLACK_JOKER)
						{
							hand.type = FOUR_DUAL_SOLO;
							hand.keyPoint = bombHands.front().keyPoint;
							hand.kicker[0] = soloHands[0].keyPoint;
							hand.kicker[1] = soloHands[1].keyPoint;
							return;
						}
					}
				}
				hand.type = SOLO;
				hand.keyPoint = soloHands.back().keyPoint;
				return;
			}
		}
		else
		{
			if (isGoodLvl(summary, minOppNumber))
			{
				findMostCardsHand(handsMap, hand);
			}
			else
			{
				if (playerInfo[(aiSeat + 2) % 3].total == 1
					&& hasUniLowestSolo(aiPoints, otherPoints))
				{
					hand.type = SOLO;
					hand.keyPoint = getLowestPoint(aiPoints);
					return;
				}
				vector<Hand> &soloHands = handsMap[SOLO];
				if (soloHands.size() == 2)
				{
					hand = soloHands.back();
					return;
				}
				vector<Hand> &pairHands = handsMap[PAIR];
				if (pairHands.size() > 0)
				{
					if (!isHighestPair(pairHands.front().keyPoint, otherPoints))
					{
						hand = pairHands.front();
						return;
					}
				}
				int notHighestSoloCount = 0;
				for (unsigned i=0; i<soloHands.size(); ++i)
				{
					if (isHighestSolo(soloHands[i].keyPoint, otherPoints))
					{
						break;
					}
					else
					{
						++notHighestSoloCount;
					}
				}
				if (notHighestSoloCount == 2 || notHighestSoloCount == 3)
				{
					vector<Hand> &bombHands = handsMap[BOMB];
					if (bombHands.size() > 0 && bombHands.front().keyPoint < BLACK_JOKER)
					{
						hand.type = FOUR_DUAL_SOLO;
						hand.keyPoint = bombHands.front().keyPoint;
						hand.kicker[0] = soloHands[0].keyPoint;
						hand.kicker[1] = soloHands[1].keyPoint;
						return;
					}
				}
				hand = soloHands.back();
				return;
			}
		}
	}
	else
	{
		if (curHandSeat == lordSeat)
		{
			if (curHand.type == SOLO)
			{
				int oppHighest = (int)getHighestPoint(otherPoints);
				vector<int> highestSolo;
				for (int i=max_ddz(curHand.keyPoint + 1, oppHighest); i<=RED_JOKER; ++i)
				{
					if (aiPoints[i] > 0)
					{
						highestSolo.push_back(i);
					}
				}
				if (highestSolo.size() > 0)
				{
					int cur = 0;
					int copyPoints[CARD_POINT_NUM];
					copy(aiPoints, aiPoints + CARD_POINT_NUM, copyPoints);
					--copyPoints[highestSolo[cur]];
					map<HandType, vector<Hand>> copyHandsMap;
					HandsMapSummary copySummary;
					sortHandsLordOnly1Card(copyPoints,
						lowestControl,
						maxOppNumber,
						otherPoints,
						true,
						copyHandsMap,
						copySummary);
					++copyPoints[highestSolo[cur]];
					for (unsigned i=1; i<highestSolo.size(); ++i)
					{
						--copyPoints[highestSolo[i]];
						map<HandType, vector<Hand>> tmpHandsMap;
						HandsMapSummary tmpSummary;
						sortHandsLordOnly1Card(copyPoints,
							lowestControl,
							maxOppNumber,
							otherPoints,
							true,
							tmpHandsMap,
							tmpSummary);
						if (tmpSummary.soloCount < copySummary.soloCount
							|| (tmpSummary.soloCount == copySummary.soloCount
								&& tmpSummary.lowestSolo > copySummary.lowestSolo))
						{
							cur = i;
							copySummary = tmpSummary;
						}
						++copyPoints[highestSolo[i]];
					}

					hand.type = SOLO;
					hand.keyPoint = CardPoint(highestSolo[cur]);
				}
				vector<Hand> &bombHands = handsMap[BOMB];
				vector<Hand> &soloHands = handsMap[SOLO];
				if (bombHands.size() > 0 && soloHands.size() == 1)
				{
					hand = bombHands.front();
					return;
				}

				if (aiSeat == DOWNFARMER)
				{
					findHigherHandNotBomb(hand);
				}
				else
				{
					if (bombHands.size() > 0)
					{
						hand = bombHands.front();
						return;
					}
				}
			}
			else
			{
				farmerMustTakeOutLordOnly1CardLvl(hand);
			}
		}
		else
		{
			if (curHand.type == SOLO)
			{
				if (aiSeat == DOWNFARMER)
				{
					if (isGoodLvl(summary, minOppNumber))
					{
						findHigherHandNotBomb(hand);
						return;
					}
					else
					{
						hand.type = NOTHING;
						return;
					}
				}
				else
				{
					int oppHighest = (int)getHighestPoint(otherPoints);
					vector<int> highestSolo;
					for (int i=max_ddz(curHand.keyPoint + 1, oppHighest); i<=RED_JOKER; ++i)
					{
						if (aiPoints[i] > 0)
						{
							highestSolo.push_back(i);
						}
					}
					if (highestSolo.size() > 0)
					{
						int cur = 0;
						int copyPoints[CARD_POINT_NUM];
						copy(aiPoints, aiPoints + CARD_POINT_NUM, copyPoints);
						--copyPoints[highestSolo[cur]];
						map<HandType, vector<Hand>> copyHandsMap;
						HandsMapSummary copySummary;
						sortHandsLordOnly1Card(copyPoints,
							lowestControl,
							maxOppNumber,
							otherPoints,
							true,
							copyHandsMap,
							copySummary);
						++copyPoints[highestSolo[cur]];
						for (unsigned i=1; i<highestSolo.size(); ++i)
						{
							--copyPoints[highestSolo[i]];
							map<HandType, vector<Hand>> tmpHandsMap;
							HandsMapSummary tmpSummary;
							sortHandsLordOnly1Card(copyPoints,
								lowestControl,
								maxOppNumber,
								otherPoints,
								true,
								tmpHandsMap,
								tmpSummary);
							if (tmpSummary.soloCount < copySummary.soloCount
								|| (tmpSummary.soloCount == copySummary.soloCount
								&& tmpSummary.lowestSolo > copySummary.lowestSolo))
							{
								cur = i;
								copySummary = tmpSummary;
							}
							++copyPoints[highestSolo[i]];
						}

						hand.type = SOLO;
						hand.keyPoint = CardPoint(highestSolo[cur]);
					}
					vector<Hand> &bombHands = handsMap[BOMB];
					vector<Hand> &soloHands = handsMap[SOLO];
					if (bombHands.size() > 0 && soloHands.size() == 1)
					{
						hand = bombHands.front();
						return;
					}
				}
			}
			else
			{
				findHigherHandNotBomb(hand);
				return;
			}
		}
	}
}

void OGLordRobotAI::farmerTakeOutLordOnly2CardsLvl( Hand &hand )
{
	hand.type = NOTHING;
	vector<Hand> &soloHands = handsMap[SOLO];	
	vector<Hand> &pairHands = handsMap[PAIR];
	if (isFree())
	{		
		if (soloHands.size() == 0 && pairHands.size() == 0)
		{
			return;
		}
		else
		{
			if (handsMap[BOMB].size() > 0 || hasUniHighestSolo(playerInfo[aiSeat].points, otherPoints))
			{
				if (soloHands.size() == 0)
				{
					hand.type = SOLO;
					hand.keyPoint = pairHands.front().keyPoint;
					return;
				}
				else
				{
					int oppHighest = findOppHighestPoint(SOLO);
					for (vector<Hand>::reverse_iterator it=soloHands.rbegin(); it!=soloHands.rend(); ++it)
					{
						if (it->keyPoint <= oppHighest)
						{
							hand = *it;
							return;
						}
					}
				}
			}
			else
			{
				farmerTakeOutLordOnly1CardLvl(hand);
				return;
			}
		}
	}
	else
	{
		if (curHand.type == PAIR && curHandSeat == lordSeat)
		{
			int highestPair = findOppHighestPoint(PAIR);
			for (unsigned i=0; i<pairHands.size(); ++i)
			{
				if (pairHands[i].keyPoint >= max_ddz(curHand.keyPoint + 1, (int)highestPair))
				{
					hand = pairHands[i];
					return;
				}
			}
		}
		farmerMustTakeOutLordChargeLvl(hand);
	}
}

void OGLordRobotAI::farmerMustTakeOutLordOnly1CardLvl( Hand &hand )
{
	hand.type = NOTHING;
	findHigherHandNotBomb(hand);
	if (hand.type == NOTHING)
	{
		findBestHigherHandFromPoints(hand, true, true);
	}
}

void OGLordRobotAI::farmerMustTakeOutLordChargeLvl( Hand &hand )
{
	hand.type = NOTHING;
	Hand copyHand;
	findHigherHandNotBomb(copyHand);
	if (copyHand.type != NOTHING)
	{
		hand = copyHand;
		return;
	}
	
	int handPoints[CARD_POINT_NUM];
	handToPointsArray(copyHand, handPoints);
	int copyAiPoints[CARD_POINT_NUM];
	int copyRemainPoints[CARD_POINT_NUM];
	copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, copyAiPoints);
	copy(remainPoints, remainPoints + CARD_POINT_NUM, copyRemainPoints);

	pointsSubEqual(copyAiPoints, handPoints);
	pointsSubEqual(copyRemainPoints, handPoints);
	CardPoint copyLowestCon = getLowestControl(copyRemainPoints);
	int copyCon = calControl(copyAiPoints, otherPoints, copyLowestCon);

	map<HandType, vector<Hand>> copyHandsMap = handsMap;
	vector<Hand> &copyHands = copyHandsMap[copyHand.type];
	for (vector<Hand>::iterator it=copyHands.begin(); it!=copyHands.end(); ++it)
	{
		if (it->keyPoint == copyHand.keyPoint
			&& (!isChain(copyHand.type) || it->len == copyHand.len))
		{
			copyHands.erase(it);
			break;
		}
	}
	HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestCon, maxOppNumber, otherPoints);

	if (copySummary.effectiveHandsCount < copyCon + 2)
	{
		hand = copyHand;
		return;
	}

	findBestHigherHandFromPoints(hand, true, false);
}

void OGLordRobotAI::goodFarmerOverOtherFarmerLvl( Hand &hand )
{
	hand.type = NOTHING;
	Hand copyHand;
	findHigherHandNotBomb(copyHand);

	if (copyHand.type != NOTHING)
	{
		if (summary.realHandsCount - 1 <= (int)handsMap[BOMB].size())
		{
			hand = copyHand;
			return;
		}
	}

	int otherFarmerSeat = aiPosition == DOWNFARMER ? (aiSeat + 1) % 3 : (aiSeat + 2) % 3;
	if (playerInfo[otherFarmerSeat].total <= 2
		|| curHand.keyPoint >= CARD_K
		|| isChain(curHand.type)
		|| curHand.type == FOUR_DUAL_SOLO
		|| curHand.type == FOUR_DUAL_PAIR)
	{
		return;
	}

	if (copyHand.type != NOTHING
		&& calHandControl(copyHand, lowestControl) == 0)
	{
		hand = copyHand;
		return;
	}
	findBestHigherHandFromPoints(hand, true, false);
}

void OGLordRobotAI::downGoodFarmerTakeOutLvl( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[(aiSeat + 1) % 3].total == 1)
		{
			downFarmerTakeOutUpFarmerOnly1CardLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (summary.realHandsCount == 2 && summary.unChargeHandsCount == 2)
		{
			for (map<HandType, vector<Hand>>::iterator it=handsMap.begin(); it!=handsMap.end(); ++it)
			{
				HandType type = it->first;
				vector<Hand> &hands = it->second;
				if (hands.size() > 0)
				{
					if (type != SOLO && type != PAIR)
					{
						hand = hands.front();
						return;
					}
					else
					{
						if (hand.type == NOTHING
							|| hands.front().keyPoint < hand.keyPoint
							|| (hands.front().keyPoint == hand.keyPoint && type == PAIR))
						{
							hand = hands.front();
							return;
						}
					}
				}
			}
		}
		else if (isGoodLvl(summary, minOppNumber))
		{
			int oppHighestBomb = getHighestBomb(otherPoints);
			bool charge = false;
			if (isDangerLvl())
			{
				charge = (summary.effectiveHandsCount <= 0);
			}
			else
			{
				charge = (summary.effectiveHandsCount <= 1);
			}
			if (charge)
			{
				findChargeHandFirstLvl(hand, true);
			}
			else
			{
				findChargeHandFirstLvl(hand, true);

				int handPoints[CARD_POINT_NUM];
				handToPointsArray(hand, handPoints);
				int copyAiPoints[CARD_POINT_NUM];
				int copyRemainPoints[CARD_POINT_NUM];
				copy(playerInfo[aiSeat].points, playerInfo[aiSeat].points + CARD_POINT_NUM, copyAiPoints);
				copy(remainPoints, remainPoints + CARD_POINT_NUM, copyRemainPoints);

				pointsSubEqual(copyAiPoints, handPoints);
				pointsSubEqual(copyRemainPoints, handPoints);
				CardPoint copyLowestCon = getLowestControl(copyRemainPoints);
				int copyCon = calControl(copyAiPoints, otherPoints, copyLowestCon);

				map<HandType, vector<Hand>> copyHandsMap = handsMap;
				vector<Hand> &copyHands = copyHandsMap[hand.type];
				for (vector<Hand>::iterator it=copyHands.begin(); it!=copyHands.end(); ++it)
				{
					if (it->keyPoint == hand.keyPoint
						&& (!isChain(hand.type) || it->len == hand.len))
					{
						copyHands.erase(it);
						break;
					}
				}
				HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestCon, maxOppNumber, otherPoints);
				if (copySummary.effectiveHandsCount < copyCon + 1)
				{
					return;
				}
				else
				{
					findLowestHand(hand);
				}
			}
		}
		else
		{
			findLowestHand(hand);
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1Card(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			goodFarmerOverOtherFarmer(hand);
		}
		else
		{
			if (curHand.type == PAIR_CHAIN
				|| curHand.type == TRIO_CHAIN
				|| curHand.type == TRIO_CHAIN_SOLO
				|| curHand.type == TRIO_CHAIN_PAIR
				|| (curHand.type == SOLO_CHAIN && curHand.len >= 8)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
			}
			else
			{
				if (curHand.type == SOLO 
					&& (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER))
				{
					farmerTakeOutWhenLordTakeOutHighSolo(hand);
					if (hand.type != NOTHING)
					{
						return;
					}
				}				
				findBestHigherHandFromMap(hand);
				if (hand.type == NOTHING)
				{
					if (handsMap[BOMB].size() > 0)
					{
						if (!isDangerLvl())
						{
							map<HandType, vector<Hand>> copyHandsMap = handsMap;
							vector<Hand> &copyBomb = copyHandsMap[BOMB];
							int handPoint[CARD_POINT_NUM];
							int copyAiPoint[CARD_POINT_NUM];
							int copyRemainPoint[CARD_POINT_NUM];
							handToPointsArray(copyBomb.front(), handPoint);
							pointsSub(playerInfo[aiSeat].points, handPoint, copyAiPoint);
							pointsSub(remainPoints, handPoint, copyRemainPoint);
							CardPoint copyLowestControl = getLowestControl(copyRemainPoint);
							int copyControlNum = calControl(copyAiPoint, otherPoints, copyLowestControl);

							copyBomb.erase(copyBomb.begin());
							HandsMapSummary copySummary = getHandsMapSummary(copyHandsMap, copyLowestControl, maxOppNumber, otherPoints);
							if (isGoodLvl(copySummary, minOppNumber))
							{
								hand = handsMap[BOMB].front();
								return;
							}
						}
					}
					findBestHigherHandFromPoints(hand, false, false);
				}
			}
		}
	}
}

void OGLordRobotAI::downBadFarmerTakeOutLvl( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[(aiSeat + 1) % 3].total == 1)
		{
			downFarmerTakeOutUpFarmerOnly1CardLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		findLowestHand(hand);
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			hand.type = NOTHING;
			return;
		}
		else
		{
			if ((curHand.type != SOLO && curHand.type != PAIR)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
				return;
			}
			else
			{
				if (curHand.type == SOLO 
					&& (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER))
				{
					farmerTakeOutWhenLordTakeOutHighSolo(hand);
					if (hand.type != NOTHING)
					{
						return;
					}
				}
				if (!isFirstHalf())
				{
					farmerMustTakeOutLordCharge(hand);
					return;
				}
				findBestHigherHandFromMap(hand);
			}
		}
	}
}

void OGLordRobotAI::upGoodFarmerTakeOutLvl( Hand &hand )
{
	if (isFree())
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (isGoodLvl(summary, minOppNumber))
		{
			findLowestHand(hand);
		}
		else
		{
			if (playerInfo[(aiSeat + 2) % 3].total == 1)
			{
				if (handsMap[SOLO].size() > 0 && handsMap[SOLO].front().keyPoint < lowestControl)
				{
					hand = handsMap[SOLO].front();
					return;
				}
			}
			else if (playerInfo[(aiSeat + 2) % 3].total == 2)
			{
				if (handsMap[PAIR].size() > 0 && handsMap[PAIR].front().keyPoint < CARD_T)
				{
					hand = handsMap[PAIR].front();
					return;
				}
			}

			if (summary.effectiveHandsCount == 2)
			{
				findChargeHandFirstLvl(hand, true);
				if (hand.type != NOTHING)
				{
					return;
				}
				else
				{
					findLowestHand(hand);
					return;
				}
			}
			else
			{
				CardPoint oppLowest = getLowestPoint(otherPoints);
				findLowestHand(hand);
				if (hand.keyPoint <= oppLowest)
				{
					return;
				}
				else
				{
					findMostCardsHandNotBomb(hand);
					return;
				}
			}
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		if (curHandSeat != lordSeat)
		{
			goodFarmerOverOtherFarmer(hand);
		}
		else
		{
			if ((curHand.type != SOLO && curHand.type != PAIR)
				|| (curHand.type == PAIR && curHand.keyPoint == CARD_A))
			{
				farmerMustTakeOutLordCharge(hand);
				return;
			}
			else
			{
				farmerMustTakeOutLordCharge(hand);
			}
		}
	}
}

void OGLordRobotAI::upBadFarmerTakeOutLvl( Hand &hand )
{
	hand.type = NOTHING;
	if (isFree())
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[(aiSeat + 2) % 3].total == 1)
		{
			hand.type = SOLO;
			hand.keyPoint = getLowestPoint(playerInfo[aiSeat].points);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2Cards(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}
		else if (playerInfo[(aiSeat + 2) % 3].total == 2)
		{
			CardPoint lowestPair = getLowestPairPoint(playerInfo[aiSeat].points);
			if (!isHighestPair(lowestPair, otherPoints))
			{
				hand.type = PAIR;
				hand.keyPoint = lowestPair;
				return;
			}
		}
		vector<Hand> &pairHands = handsMap[PAIR];
		for (unsigned i=0; i<pairHands.size(); ++i)
		{
			if (pairHands[i].keyPoint <= CARD_8)
			{
				hand = pairHands[i];
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}

		vector<Hand> &soloHands = handsMap[SOLO];
		for (unsigned i=0; i<soloHands.size(); ++i)
		{
			if (soloHands[i].keyPoint < lowestControl)
			{
				hand = soloHands[i];
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}

		for (unsigned i=0; i<pairHands.size(); ++i)
		{
			if (pairHands[i].keyPoint < lowestControl)
			{
				hand.type = SOLO;
				hand.keyPoint = pairHands[i].keyPoint;
			}
			else
			{
				break;;
			}
		}
		if (hand.type != NOTHING)
		{
			return;
		}
		findLowestHand(hand);
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
			return;
		}
		else if (playerInfo[lordSeat].total == 2)
		{
			farmerTakeOutLordOnly2CardsLvl(hand);
			if (hand.type != NOTHING)
			{
				return;
			}
		}

		if (curHand.type == SOLO
			&& curHand.keyPoint < min_ddz(CARD_J, lowestControl))
		{
			int higherP = max_ddz((int)CARD_J, curHand.keyPoint + 1);
			vector<Hand> &soloHands = handsMap[SOLO];
			for (unsigned i=0; i<soloHands.size(); ++i)
			{
				if (soloHands[i].keyPoint > CARD_A)
				{
					break;
				}
				if (soloHands[i].keyPoint >= higherP)
				{
					hand = soloHands[i];
				}				
			}
			if (hand.type != NOTHING)
			{
				return;
			}

			vector<Hand> &pairHands = handsMap[PAIR];
			for (unsigned i=0; i<pairHands.size(); ++i)
			{
				if (pairHands[i].keyPoint >= higherP)
				{
					hand.type = SOLO;
					hand.keyPoint = pairHands[i].keyPoint;
				}
				else
				{
					break;;
				}
			}
			if (hand.type != NOTHING)
			{
				return;
			}

			for (unsigned i=0; i<soloHands.size(); ++i)
			{
				if (soloHands[i].keyPoint > curHand.keyPoint
					&& soloHands[i].keyPoint <= CARD_2)
				{
					hand = soloHands[i];
					return;
				}
			}

			for (int i=curHand.keyPoint+1; i<= CARD_2; ++i)
			{
				if (playerInfo[aiSeat].points[i] > 0 && playerInfo[aiSeat].points[i] < 4)
				{
					hand.type = SOLO;
					hand.keyPoint = CardPoint(i);
					return;
				}
			}
		}
		else
		{
			if (curHandSeat == lordSeat)
			{
				if (curHand.type == SOLO)
				{
					if (curHand.keyPoint == CARD_2 || curHand.keyPoint == BLACK_JOKER)
					{
						farmerTakeOutWhenLordTakeOutHighSolo(hand);
						return;
					}
					else if (curHand.keyPoint >= CARD_J && curHand.keyPoint <= CARD_A)
					{
						for (int i=curHand.keyPoint+1; i<= CARD_2; ++i)
						{
							if (playerInfo[aiSeat].points[i] > 0 && playerInfo[aiSeat].points[i] < 4)
							{
								hand.type = SOLO;
								hand.keyPoint = CardPoint(i);
								return;
							}
						}
						return;
					}
				}
				farmerMustTakeOutLordCharge(hand);
			}
		}
	}
}

void OGLordRobotAI::downFarmerTakeOutUpFarmerOnly1CardLvl( Hand &hand )
{
	hand.type = NOTHING;
	CardPoint upFarmerLowestPoint = getLowestPoint(playerInfo[upSeat].points);
	CardPoint lowestPoint = getLowestPoint(playerInfo[aiSeat].points);	
	if (lowestPoint < upFarmerLowestPoint)
	{
		mustHighLevel = true;
		
		map<HandType, vector<Hand>> bombMap;
		getBomb(playerInfo[aiSeat].points, bombMap);
		if (bombMap[BOMB].size() > 0)
		{
			int oppHighestBomb = getHighestBomb(otherPoints);
			if (bombMap[BOMB].back().keyPoint > oppHighestBomb)
			{
				if (oppHighestBomb < 0 || playerInfo[lordSeat].total != 4)
				{
					hand = bombMap[BOMB].front();
				}
				else
				{
					hand = bombMap[BOMB].back();
				}
			}
			else
			{
				hand.type = SOLO;
				hand.keyPoint = lowestPoint;
			}
		}
		else
		{
			hand.type = SOLO;
			hand.keyPoint = lowestPoint;
		}
	}
	else
	{
		if (playerInfo[lordSeat].total == 1)
		{
			farmerTakeOutLordOnly1CardLvl(hand);
		}
		else if (!isGoodLvl(summary, minOppNumber))
		{
			hand.type = SOLO;
			hand.keyPoint = lowestPoint;
		}
	}
}


void OGLordRobotAI::countHighHand(map<HandType, vector<Hand>> &allHands, 
								  map<HandType, int> &uniHighHands, 
								  map<HandType, int> &highHands,
								  map<HandType, int> &goodLowHands,
								  int &trioCount)
{
	trioCount = 0;
	for (map<HandType, vector<Hand>>::iterator it=allHands.begin(); it!=allHands.end(); ++it)
	{
		HandType type = it->first;
		vector<Hand> &hands = it->second;
		if (type != BOMB)
		{
			uniHighHands[type] = 0;
			highHands[type] = 0;
			goodLowHands[type] = 0;
			if (isChain(type))
			{
				set<int> uniHighLength;
				for (vector<Hand>::reverse_iterator rit=hands.rbegin(); rit!=hands.rend(); ++rit)
				{
					if (type == TRIO_CHAIN)
					{
						trioCount += rit->len;
					}
					int point = findOppHighestChain(type, rit->len);
					if (rit->keyPoint > point)
					{
						++uniHighHands[type];
						uniHighLength.insert(rit->len);
					}
					else if (rit->keyPoint == point)
					{
						++highHands[type];
					}
					else
					{
						if (uniHighLength.count(rit->len) > 0 && goodLowHands[type] == 0)
						{
							++goodLowHands[type];
						}
					}
				}	
			}
			else
			{
				if (type == TRIO)
				{
					trioCount += hands.size();
				}
				int point = findOppHighestPoint(type);
				for (vector<Hand>::reverse_iterator rit=hands.rbegin(); rit!=hands.rend(); ++rit)
				{
					if (rit->keyPoint > point)
					{
						++uniHighHands[type];
					}
					else if (rit->keyPoint == point)
					{
						++highHands[type];
					}
				}
				int lowCount = hands.size() - uniHighHands[type] - highHands[type];
				if (lowCount > 0)
				{
					int highCount = uniHighHands[type] < highHands[type] ? 2 * uniHighHands[type] : uniHighHands[type] + highHands[type];
					if (highCount < lowCount)
					{
						goodLowHands[type] += highCount;
					}
					else
					{
						goodLowHands[type] += lowCount;
					}
				}
			}
		}
	}
}

int OGLordRobotAI::findOppHighestPoint( HandType type )
{
	int count;
	switch (type)
	{
	case SOLO:
		count = 1;
		break;
	case PAIR:
		count = 2;
		break;
	case TRIO:
		count = 3;
		break;
	default:
		count = 4;
		break;
	}
	int *pointArray = aiPosition == LORD ? playerInfo[downSeat].points : playerInfo[lordSeat].points;
	int highPoint = -1;
	if (count == 4 && pointArray[BLACK_JOKER] == 1 && pointArray[RED_JOKER] == 1)
	{
		highPoint = BLACK_JOKER;
		return highPoint;
	}
	for (int i=RED_JOKER; i>=CARD_3; --i)
	{
		if (pointArray[i] >= count)
		{
			highPoint = i;
			break;
		}
	}
	if (aiPosition == LORD)
	{
		pointArray = playerInfo[upSeat].points;
		if (count == 4 && pointArray[BLACK_JOKER] == 1 && pointArray[RED_JOKER] == 1)
		{
			highPoint = BLACK_JOKER;
			return highPoint;
		}
		for (int i=RED_JOKER; i>highPoint; --i)
		{
			if (pointArray[i] >= count)
			{
				highPoint = i;
				break;
			}
		}
	}
	return highPoint;
}

int OGLordRobotAI::findOppHighestChain( HandType type, int len )
{
	int count;
	switch (type)
	{
	case SOLO_CHAIN:
		count = 1;
		break;
	case PAIR_CHAIN:
		count = 2;
		break;
	default:
		count = 3;
		break;
	}
	int *pointArray = aiPosition == LORD ? playerInfo[downSeat].points : playerInfo[lordSeat].points;
	int highPoint = -1;
	int start = -1;
	int end = -1;
	for (int i=CARD_A; i>=CARD_3; --i)
	{
		if (pointArray[i] >= count)
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
					highPoint = start;
					break;
				}
			}
		}
		else
		{
			end = -1;
		}
	}

	if (aiPosition == LORD)
	{
		pointArray = playerInfo[upSeat].points;
		for (int i=CARD_A; i>highPoint; --i)
		{
			if (pointArray[i] >= count)
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
						highPoint = start;
						break;
					}
				}
			}
			else
			{
				end = -1;
			}
		}
	}
	return highPoint;
}

HandsMapSummary OGLordRobotAI::getHandsMapSummaryLvl( std::map<HandType, std::vector<Hand>> &allHands, 
													 std::map<HandType, int> &uniHighHands, 
													 std::map<HandType, int> &highHands, 
													 std::map<HandType, int> &goodLowHands, 
													 int trioCount )
{

	bool changeTrio = false;

	vector<Hand> &trioChainHands = allHands[TRIO_CHAIN];
	for (vector<Hand>::iterator it=trioChainHands.begin(); it!=trioChainHands.end();)
	{
		int lowSoloCount = allHands[SOLO].size() - uniHighHands[SOLO] - highHands[SOLO] - goodLowHands[SOLO];
		int lowPairCount = allHands[PAIR].size() - uniHighHands[PAIR] - highHands[PAIR] - goodLowHands[PAIR];
		int k = 0;
		if (lowSoloCount >= lowPairCount && lowSoloCount >= it->len)
		{
			k = 1;
		}
		else if (lowSoloCount < lowPairCount && lowPairCount >= it->len)
		{
			k = 2;
		}
		else
		{
			int lowSoloCount0 = allHands[SOLO].size() - uniHighHands[SOLO];
			int lowPairCount0 = allHands[PAIR].size() - uniHighHands[PAIR];
			if (lowSoloCount0 >= lowPairCount0 && lowSoloCount0 >= it->len)
			{
				k = 1;
			}
			else if (lowSoloCount0 < lowPairCount0 && lowPairCount0 >= it->len)
			{
				k = 2;
			}
			else
			{
				if (allHands[SOLO].size() >= allHands[PAIR].size() && allHands[SOLO].size() >= (unsigned)it->len)
				{
					k = 1;
				}
				else if (allHands[SOLO].size() < allHands[PAIR].size() && allHands[PAIR].size() >= (unsigned)it->len)
				{
					k = 2;
				}
			}
		}
		if (k == 0)
		{
			++it;
		}
		else
		{
			if (k == 1)
			{
				vector<Hand> &soloHands = allHands[SOLO];
				vector<Hand>::iterator sit = soloHands.begin();
				Hand h = *it;
				h.type = TRIO_CHAIN_SOLO;
				for (int i=0; i<h.len; ++i)
				{
					h.kicker[i] = sit->keyPoint;
					sit = soloHands.erase(sit);
				}
				allHands[TRIO_CHAIN_SOLO].push_back(h);
			}
			else
			{
				vector<Hand> &pairHands = allHands[PAIR];
				vector<Hand>::iterator sit = pairHands.begin();
				Hand h = *it;
				h.type = TRIO_CHAIN_PAIR;
				for (int i=0; i<h.len; ++i)
				{
					h.kicker[i] = sit->keyPoint;
					sit = pairHands.erase(sit);
				}
				allHands[TRIO_CHAIN_PAIR].push_back(h);
			}
			it = trioChainHands.erase(it);
			changeTrio = true;
		}
	}

	vector<Hand> &trioHands = allHands[TRIO];
	for (vector<Hand>::iterator it=trioHands.begin(); it!=trioHands.end();)
	{
		int lowSoloCount = allHands[SOLO].size() - uniHighHands[SOLO] - highHands[SOLO] - goodLowHands[SOLO];
		int lowPairCount = allHands[PAIR].size() - uniHighHands[PAIR] - highHands[PAIR] - goodLowHands[PAIR];
		int k = 0;
		if (lowSoloCount >= lowPairCount && lowSoloCount >= 1)
		{
			k = 1;
		}
		else if (lowSoloCount < lowPairCount && lowPairCount >= 1)
		{
			k = 2;
		}
		else
		{
			int lowSoloCount0 = allHands[SOLO].size() - uniHighHands[SOLO];
			int lowPairCount0 = allHands[PAIR].size() - uniHighHands[PAIR];
			if (lowSoloCount0 >= lowPairCount0 && lowSoloCount0 >= 1)
			{
				k = 1;
			}
			else if (lowSoloCount0 < lowPairCount0 && lowPairCount0 >= 1)
			{
				k = 2;
			}
			else
			{
				if (allHands[SOLO].size() >= allHands[PAIR].size() && allHands[SOLO].size() >= 1)
				{
					k = 1;
				}
				else if (allHands[SOLO].size() < allHands[PAIR].size() && allHands[PAIR].size() >= 1)
				{
					k = 2;
				}
			}
		}
		if (k == 0)
		{
			++it;
		}
		else
		{
			if (k == 1)
			{
				vector<Hand> &soloHands = allHands[SOLO];
				vector<Hand>::iterator sit = soloHands.begin();
				Hand h = *it;
				h.type = TRIO_SOLO;
				h.kicker[0] = sit->keyPoint;
				soloHands.erase(sit);
				allHands[TRIO_SOLO].push_back(h);
			}
			else
			{
				vector<Hand> &pairHands = allHands[PAIR];
				vector<Hand>::iterator sit = pairHands.begin();
				Hand h = *it;
				h.type = TRIO_PAIR;
				h.kicker[0] = sit->keyPoint;
				pairHands.erase(sit);
				allHands[TRIO_PAIR].push_back(h);
			}
			it = trioHands.erase(it);
			changeTrio = true;
		}
	}
	
	if (changeTrio)
	{
		countHighHand(allHands, uniHighHands, highHands, goodLowHands, trioCount);
	}
	
	int lowSoloCount = allHands[SOLO].size() - uniHighHands[SOLO] - highHands[SOLO] - goodLowHands[SOLO];
	int lowPairCount = allHands[PAIR].size() - uniHighHands[PAIR] - highHands[PAIR] - goodLowHands[PAIR];
	if (lowSoloCount > 0)
	{
		int oppHighestPoint = findOppHighestPoint(SOLO);
		if (allHands[TRIO].size() > 0 
			&& allHands[TRIO].back().keyPoint > oppHighestPoint
			&& goodLowHands[TRIO] < uniHighHands[TRIO])
		{
			if (lowSoloCount == 1)
			{
				lowSoloCount = 0;
				lowPairCount = max_ddz(lowPairCount - 1, 0);

				CardPoint key = allHands[TRIO].back().keyPoint;
				Hand newSolo = {SOLO, key, 1};
				allHands[SOLO].push_back(newSolo);
				Hand newPair = {PAIR, key, 1};
				allHands[PAIR].push_back(newPair);

				allHands[TRIO].pop_back();
				
			}
			else
			{
				lowSoloCount = max_ddz(lowSoloCount - 3, 0);

				CardPoint key = allHands[TRIO].back().keyPoint;
				Hand newSolo = {SOLO, key, 1};
				allHands[SOLO].push_back(newSolo);
				allHands[SOLO].push_back(newSolo);
				allHands[SOLO].push_back(newSolo);

				allHands[TRIO].pop_back();
			}
		}
		else if (allHands[PAIR].size() > 0 
			&& allHands[PAIR].back().keyPoint > oppHighestPoint
			&& (lowSoloCount >= 2 || goodLowHands[PAIR] < uniHighHands[PAIR]))
		{
			lowSoloCount = max_ddz(lowSoloCount - 2, 0);
			if (goodLowHands[PAIR] == uniHighHands[PAIR])
			{
				++lowPairCount;
			}

			CardPoint key = allHands[PAIR].back().keyPoint;
			Hand newSolo = {SOLO, key, 1};
			allHands[PAIR].push_back(newSolo);
			allHands[PAIR].push_back(newSolo);

			allHands[PAIR].pop_back();
		}
	}
	
	HandsMapSummary handsSummary = {0, 0, 0, 0, 0, 0, 0, RED_JOKER, RED_JOKER};
	for (map<HandType, vector<Hand>>::iterator it=allHands.begin(); it!=allHands.end(); ++it)
	{
		vector<Hand> &hands = it->second;
		if (it->first == BOMB)
		{
			handsSummary.extraBombCount = hands.size();
		}
		else
		{
			handsSummary.realHandsCount += hands.size();
			if (it->first == SOLO_CHAIN)
			{
				set<int> chainLength;
				for (unsigned i=0; i<hands.size(); ++i)
				{
					chainLength.insert(hands[i].len);
				}
				handsSummary.handsTypeCount += chainLength.size();
			}
			else
			{
				++handsSummary.handsTypeCount;
			}
		}
	}
	handsSummary.effectiveHandsCount = 0;
	for (map<HandType, std::vector<Hand>>::iterator it=allHands.begin(); it!=allHands.end(); ++it)
	{
		HandType type = it->first;
		if (type != BOMB)
		{
			if (type == SOLO)
			{
				handsSummary.effectiveHandsCount += lowSoloCount;
			}
			else if (type == PAIR)
			{
				handsSummary.effectiveHandsCount += lowPairCount;
			}
			else
			{
				vector<Hand> &hands = it->second;
				handsSummary.effectiveHandsCount +=
					(hands.size() - uniHighHands[type] - highHands[type] - goodLowHands[type]);
			}
		}
	}
	handsSummary.effectiveHandsCount -= handsSummary.extraBombCount;
	if (allHands[SOLO].size() == 0 || allHands[SOLO].size() < (unsigned)trioCount)
	{
		handsSummary.soloCount = 0;
		handsSummary.lowestSolo = RED_JOKER;
		handsSummary.sencondLowestSolo = RED_JOKER;
	}
	else
	{
		handsSummary.soloCount = trioCount - allHands[SOLO].size();
		handsSummary.lowestSolo = (unsigned)trioCount <= allHands[SOLO].size() - 1 ? 
			allHands[SOLO][trioCount].keyPoint : RED_JOKER;
		handsSummary.sencondLowestSolo = (unsigned)trioCount + 1 <= allHands[SOLO].size() - 1 ? 
			allHands[SOLO][trioCount + 1].keyPoint : RED_JOKER;
	}
	return handsSummary;
}

int OGLordRobotAI::addAllValue( std::map<HandType, int> &handsCount )
{
	int count = 0;
	for (map<HandType, int>::iterator it=handsCount.begin(); it!=handsCount.end(); ++it)
	{
		count += it->second;
	}
	return count;
}

bool OGLordRobotAI::containsHand(map<HandType, vector<Hand>> &allHands, Hand &hand)
{
	HandType type = hand.type == NUKE ? BOMB : hand.type;
	vector<Hand> &hands = allHands[type];
	for (unsigned i=0; i<hands.size(); ++i)
	{
		if (hand.keyPoint == hands[i].keyPoint
			&& (!isChain(type) || hand.len == hands[i].len))
		{
			return true;
		}
	}
	return false;
}

void OGLordRobotAI::findChargeHandFirstLvl( Hand &hand, bool typeCountFirst )
{
	HandType hType = NOTHING;
	hand.type = NOTHING;
	int charge = -1;
	int count = 0;
	map<HandType, int> &searchMap = goodLowHandCount;
	count = addAllValue(goodLowHandCount);
	if (count > 0)
	{
		charge = 0;
	}
	else
	{
		count = addAllValue(highHandCount);
		if (count > 0)
		{
			charge = 1;
			searchMap = highHandCount;
		}
		else
		{
			count = addAllValue(uniHighHandCount);
			if (count > 0)
			{
				charge = 2;
				searchMap = uniHighHandCount;
			}
		}
	}

	if (charge >= 0)
	{
		for (map<HandType, int>::iterator it=searchMap.begin(); it!=searchMap.end(); ++it)
		{
			if (it->second > 0)
			{
				charge = 0;
				if (hType == NOTHING)
				{
					hType = it->first;
				}
				else
				{
					if (typeCountFirst)
					{
						if (handsMap[it->first].size() > handsMap[hType].size()
							|| (handsMap[it->first].size() == handsMap[hType].size()
							&& it->first < hType))
						{
							hType = it->first;
						}
					} 
					else
					{
						if (it->first < hType)
						{
							hType = it->first;
						}
					}
				}
			}
		}
	}

	if (hType != NOTHING)
	{
		vector<Hand> &handsVec = handsMap[hType];
		if (isChain(hType))
		{
			set<int> uniHighLength;
			for (unsigned i=0; i<handsVec.size(); ++i)
			{
				int point = findOppHighestChain(hType, handsVec[i].len);
				if (handsVec[i].keyPoint > point)
				{
					if (charge == 2)
					{
						hand = handsVec[i];
						break;
					}
					else
					{
						uniHighLength.insert(handsVec[i].len);
					}					
				}
				else if (handsVec[i].keyPoint == point)
				{
					if (charge == 1)
					{
						hand = handsVec[i];
						break;
					}
				}
			}
			if (charge == 0)
			{
				for (unsigned i=0; i<handsVec.size(); ++i)
				{
					int point = findOppHighestChain(hType, handsVec[i].len);
					if (handsVec[i].keyPoint < point
						&& uniHighLength.count(handsVec[i].len) > 0)
					{
						hand = handsVec[i];
						break;
					}
				}
			}
		}
		else
		{
			for (unsigned i=0; i<handsVec.size(); ++i)
			{
				int oppHigh = findOppHighestPoint(hType);
				if (charge == 0
					|| (charge == 1 && handsVec[i].keyPoint == oppHigh)
					|| (charge == 2 && handsVec[i].keyPoint > oppHigh))
				{
					hand = handsVec[i];
					break;
				}
			}
		}
	}
	if (hand.type == NOTHING)
	{
		findChargeHandFirst(hand, typeCountFirst);
	}
}

bool OGLordRobotAI::RbtInShowCard(int argShowSeat, std::vector<int> argHandCard)
{
    return true;
}

bool OGLordRobotAI::RbtOutShowCard(bool &showCard)
{
    showCard = false;
    return true;
}

bool OGLordRobotAI::RbtOutDoubleGame(bool &Double_game)
{
    if (level >= HARD_LEVEL)
    {
        int i = doubleGame(robot);
        Double_game = i;
        //delay = 500 + rand() % 500; //todo update
        return true;
    }
    Double_game = 0;
    //delay = 0;
    return true;
}
