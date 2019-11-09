#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <iterator>
#include <random>
#include <time.h>
#include <fstream>

#include "DDZ_BP_MeldGroup.h"
#include "PokerUtils.h"
#include "DDZRobot.h"

using namespace std;

#define LOOP_BEG do{
#define LOOP_END }while(1);
#define LOOP_CONTINUE continue;
#define LOOP_BREAK break;

const char pokerChar[15] = {'3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A', '2', 'X', 'D'};

char TransferCardId(int cardId)
{
    if (cardId == 53)
    {
        return 'D';
    }
    else if (cardId == 52)
    {
        return 'X';
    }
    else
    {
        return pokerChar[cardId % 13];
    }
}

bool cmp(char ch1, char ch2)
{
    static map<char, int> chMap;
    if (chMap.empty())
    {
        chMap.insert(make_pair('3', 0));
        chMap.insert(make_pair('4', 1));
        chMap.insert(make_pair('5', 2));
        chMap.insert(make_pair('6', 3));
        chMap.insert(make_pair('7', 4));
        chMap.insert(make_pair('8', 5));
        chMap.insert(make_pair('9', 6));
        chMap.insert(make_pair('T', 7));
        chMap.insert(make_pair('J', 8));
        chMap.insert(make_pair('Q', 9));
        chMap.insert(make_pair('K', 10));
        chMap.insert(make_pair('A', 11));
        chMap.insert(make_pair('2', 12));
        chMap.insert(make_pair('X', 13));
        chMap.insert(make_pair('D', 14));
    }
    return chMap[ch1] < chMap[ch2];
}

void PrintCards(const std::vector<int> &hand)
{
    string s;
    for (vector<int>::const_iterator beg = hand.begin(), end = hand.end(); beg != end; ++beg)
    {
        s.push_back(TransferCardId(*beg));
    }
    sort(s.begin(), s.end(), cmp);
    cout << s << endl;
}

int trans(char ch)
{
    switch (ch)
    {
    case 'T': return 7;
    case 'J': return 8;
    case 'Q': return 9;
    case 'K': return 10;
    case 'A': return 11;
    case '2': return 12;
    case 'X': return 52;
    case 'D': return 53;
    default: return ch - '0' - 3;
    }
}

void test_level()
{
    std::random_device rd;
    std::seed_seq seed({12345678});
    std::mt19937 g(rd());
    //g.seed(seed);

    int delay = 0;
    int game = 0;

    DDZRobot robots[3];
    LOOP_BEG
    ++game;
    if (game >= 1e8)
    {
        break;
    }
    //随机发牌
    vector<int> deck(54, 0);
    int n = 0;
    generate(deck.begin(), deck.end(), [&n](){return n++; });

    std::shuffle(deck.begin(), deck.end(), g);
    //random_shuffle(deck.begin(), deck.end());

    //指定牌局,叫分有问题，请同时指定地主
    string deckStr =
        "3455566679TJKA222""34578899TTTQKAAAD""33467789JJJQQQK2X""48K";
        //"34456789TJJJKAA22""3344568999TJQQKA2""355667788TTQQKKAD""72X";
    if (game == 1)
    {
        char cdeck[54];
        copy(deckStr.begin(), deckStr.end(), cdeck);
        int ideck[54];
        transform(cdeck, cdeck + 54, ideck, trans);
        deck.assign(begin(ideck), end(ideck));
    }

    cout << "=====GAME " << game << "START=====" << endl;

    vector<vector<int>> cards(3 + 1);
    cards.at(0).assign(deck.begin(), deck.begin() + 17);
    cards.at(1).assign(deck.begin() + 17, deck.begin() + 34);
    cards.at(2).assign(deck.begin() + 34, deck.begin() + 51);
    cards.at(3).assign(deck.begin() + 51, deck.end());

    for (auto &playerCards : cards)
    {
        sort(playerCards.begin(), playerCards.end());
    }

    //发牌
    for (int i = 0; i < 3; ++i)
    {
        robots[i].RbtResetData();
        robots[i].RbtInSetLevel(2); //TODO
        robots[i].RbtInInitCard(i, cards[i]);

        robots[i].RbtInNtfCardInfo(cards);

        cout << "PLAYER " << i << ": ";
        PrintCards(cards[i]);
    }
    cout << "UNDER CARDS: ";
    PrintCards(cards[3]);

    //叫分
    int callScores[3] = {0};
    for (int i = 0; i < 3; ++i)
    {
        robots[i].RbtOutGetCallScore(callScores[i]);
        robots[(i + 1) % 3].RbtInCallScore(i, callScores[i]);
        robots[(i + 2) % 3].RbtInCallScore(i, callScores[i]);

        if (callScores[i] == 1 || callScores[i] == 2)
        {
            //ofstream fout("./call.log", ios::app);
            //fout << "Player " << i << " call " << callScores[i] << ", Cards: ";
            cout << "Player " << i << " call " << callScores[i] << ", Cards: ";

            string s;
            for (vector<int>::const_iterator beg = cards[i].begin(), end = cards[i].end(); beg != end; ++beg)
            {
                s.push_back(TransferCardId(*beg));
            }
            sort(s.begin(), s.end(), cmp);
            //fout << s << endl;
            cout << s << endl;
            //fout.close();
        }
    }
    //LOOP_CONTINUE;

    //测试可选策略
    for (int i = 0; i < 3; ++i)
    {
        bool doubleGame = false;
        robots[i].RbtOutDoubleGame(doubleGame);
        bool grabLord = false;
        robots[i].RbtOutGetGrabLord(grabLord);
    }

    //确定地主
    int lordSeat = -1;
    int maxScore = MAX(MAX(callScores[0], callScores[1]), callScores[2]);
    /*
    if (maxScore == 0)
    {
        cout << "无人叫地主，流局。" << endl;
        system("pause");
        LOOP_CONTINUE
    }
    */
    //随机地主
    lordSeat = rand() % 3;
    for (int i = 0; i < 3; ++i)
    {
        if (callScores[i] == maxScore)
        {
            lordSeat = i;
        }
    }

    //指定牌局的同时指定地主
    lordSeat = 0;

    for (int i = 0; i < 3; ++i)
    {
        robots[i].RbtInSetLord(lordSeat, cards[3]);
    }
    //地主手牌+底牌
    for (auto underCard : cards[3])
    {
        cards.at(lordSeat).push_back(underCard);
    }
    sort(cards[lordSeat].begin(), cards[lordSeat].end());

    //打牌
    int handSize[3] = {17, 17, 17};
    handSize[lordSeat] = 20;

    BP_MELD_GROUP *pPreMeld, *pCurMeld;
    pPreMeld = new BP_MELD_GROUP();
    pCurMeld = new BP_MELD_GROUP();

    int prePlayer = -1;
    int passCnt = 0;
    for (int player = lordSeat, round = 0;; player = (player + 1) % 3)
    {
        if (player == lordSeat)
        {
            ++round;
            cout << "-----ROUND " << round << "-----" << endl;
        }
        /*只测试地主
        else
        {
        continue;
        }
        */
        vector<int> outCards;
        //if (!robots[player].RbtOutGetTakeOutCard(outCards, delay))
        if (!robots[player].RbtOutGetTakeOutCard(outCards))
        {
            throw "";
        }

        cout << "PLAYER " << player << ": ";
        if (outCards.empty())
        {
            ++passCnt;
            cout << "PASS" << endl;
            if (prePlayer == player || passCnt >= 3)
            {
                throw 0; //主动不出牌
            }
        }
        else
        {
            passCnt = 0;

            sort(outCards.begin(), outCards.end());

            for (auto card : outCards)
            {
                auto pos = find(cards[player].begin(), cards[player].end(), card);
                if (pos == cards[player].end())
                {
                    throw "Card undefine.";
                }
            }

            vector<int> diff;
            set_difference(cards.at(player).begin(), cards.at(player).end(), outCards.begin(), outCards.end(), inserter(diff, diff.begin()));
            cards[player] = diff;

            vector<int> outHand;
            for (auto card : outCards)
            {
                outHand.push_back(card == 53 ? 14 : card == 52 ? 13 : card % 13);
            }

            //出牌不合法
            if (!initMeldGroup(outHand, *pCurMeld)
                || (prePlayer != -1 && prePlayer != player && !(*pPreMeld < *pCurMeld)))
            {
                throw 1;
            }

            prePlayer = player;
            PrintCards(outCards);
            handSize[player] -= outCards.size();
            if (handSize[player] == 0) //是否赢了
            {
                cout << "PLAYER " << player << " WIN" << endl;
                break;
            }
            swap(pCurMeld, pPreMeld);
        }
        robots[player].RbtInTakeOutCard(player, outCards);
        robots[(player + 1) % 3].RbtInTakeOutCard(player, outCards);
        robots[(player + 2) % 3].RbtInTakeOutCard(player, outCards);
        /*//只地主出牌
        outCards.clear();
        robots[player].RbtInTakeOutCard((player + 1) % 3, outCards);
        robots[player].RbtInTakeOutCard((player + 2) % 3, outCards);
        */
        //cout << "REMAIN CARDS:\n";
        //for (int i = 0; i < 3; ++i)
        //{
        //    cout << "PLAYER " << i << ": ";
        //    //PrintCards(robots[i].m_hand[robot[i].m_mySeat]);
        //}
    }
    delete pCurMeld;
    pCurMeld = nullptr;
    delete pPreMeld;
    pPreMeld = nullptr;
    cout << "=====GAME END=====" << endl;
    LOOP_END
}

void test_play()
{
    int delay = 0;
    int game = 0;

    DDZRobot robots[3];
    LOOP_BEG
    ++game;
    //随机发牌
    vector<int> deck(54, 0);
    int n = 0;
    generate(deck.begin(), deck.end(), [&n](){return n++; });

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(deck.begin(), deck.end(), g);
    //random_shuffle(deck.begin(), deck.end());

    //指定牌局
    string deckStr =
        "34555668TJJJQQKAX""337788999TTTQ222D""444567789JKKKAAA2""36Q";
    if (game == 1)
    {
        char cdeck[54];
        copy(deckStr.begin(), deckStr.end(), cdeck);
        int ideck[54];
        transform(cdeck, cdeck + 54, ideck, trans);
        deck.assign(begin(ideck), end(ideck));
    }

    cout << "=====GAME START=====" << endl;

    vector<vector<int>> cards(3 + 1);
    cards.at(0).assign(deck.begin(), deck.begin() + 17);
    cards.at(1).assign(deck.begin() + 17, deck.begin() + 34);
    cards.at(2).assign(deck.begin() + 34, deck.begin() + 51);
    cards.at(3).assign(deck.begin() + 51, deck.end());

    for (auto &playerCards : cards)
    {
        sort(playerCards.begin(), playerCards.end());
    }

    //发牌
    for (int i = 0; i < 3; ++i)
    {
        robots[i].RbtResetData();
        robots[i].RbtInSetLevel(2);  //TODO
        robots[i].RbtInInitCard(i, cards[i]);

        robots[i].RbtInNtfCardInfo(cards);

        cout << "PLAYER " << i << ": ";
        PrintCards(cards[i]);
    }
    cout << "UNDER CARDS: ";
    PrintCards(cards[3]);

    //叫分
    int callScores[3] = {0};
    for (int i = 0; i < 3; ++i)
    {
        //robots[i].RbtOutGetCallScore(callScores[i], delay);
        robots[i].RbtOutGetCallScore(callScores[i]);
        robots[(i + 1) % 3].RbtInCallScore(i, callScores[i]);
        robots[(i + 2) % 3].RbtInCallScore(i, callScores[i]);
    }
    //bool grab = false;
    //robots[0].RbtOutGetGrabLord(grab, delay);

    //测试可选策略
    for (int i = 0; i < 3; ++i)
    {
        bool doubleGame = false;
        robots[i].RbtOutDoubleGame(doubleGame);
        bool grabLord = false;
        robots[i].RbtOutGetGrabLord(grabLord);
    }

    //确定地主
    int lordSeat = -1;
    int maxScore = MAX(MAX(callScores[0], callScores[1]), callScores[2]);
    /*
    if (maxScore == 0)
    {
    cout << "无人叫地主，流局。" << endl;
    system("pause");
    LOOP_CONTINUE
    }
    */
    //随机地主
    //lordSeat = rand() % 3;
    for (int i = 0; i < 3; ++i)
    {
        if (callScores[i] == maxScore)
        {
            lordSeat = i;
        }
    }

    //指定AI座位及地主
    int aiSeat = 0;
    cout << "输入AI座位号（0，1，2）： ";
    cin >> aiSeat;

    cout << "指定地主座位号（0，1，2）： ";
    cin >> lordSeat;

    for (int i = 0; i < 3; ++i)
    {
        robots[i].RbtInSetLord(lordSeat, cards[3]);
    }
    //地主手牌+底牌
    for (auto underCard : cards[3])
    {
        cards.at(lordSeat).push_back(underCard);
    }
    sort(cards[lordSeat].begin(), cards[lordSeat].end());

    //打牌
    int handSize[3] = {17, 17, 17};
    handSize[lordSeat] = 20;

    BP_MELD_GROUP *pPreMeld, *pCurMeld;
    pPreMeld = new BP_MELD_GROUP();
    pCurMeld = new BP_MELD_GROUP();

    int prePlayer = -1;
    int passCnt = 0;
    for (int player = lordSeat, round = 0;; player = (player + 1) % 3)
    {
        if (player == lordSeat)
        {
            ++round;
            cout << "-----ROUND " << round << "-----" << endl;
        }
        /*只测试地主
        else
        {
        continue;
        }
        */
        vector<int> outCards;
        //if (!robots[player].RbtOutGetTakeOutCard(outCards, delay))
        if (player == aiSeat && !robots[player].RbtOutGetTakeOutCard(outCards))
        {
            throw "";
        }
        else if (player != aiSeat)
        {
            cout << "PLAYER " << player << "（玩家） DISCARD: ";
            string s;
            cin >> s;
            if (s != "P" && s != "PASS")
            {
                for (auto card : s)
                {
                    outCards.push_back(trans(card));
                }
            }
        }

        cout << "PLAYER " << player << "（" << (player == aiSeat ? "AI" : "玩家") << "）: ";
        if (outCards.empty())
        {
            ++passCnt;
            cout << "PASS" << endl;
            if (prePlayer == player || passCnt >= 3)
            {
                throw 0; //主动不出牌
            }
        }
        else
        {
            passCnt = 0;

            sort(outCards.begin(), outCards.end());

            for (auto card : outCards)
            {
                auto pos = find(cards[player].begin(), cards[player].end(), card);
                if (pos == cards[player].end())
                {
                    throw "Card undefine.";
                }
            }

            vector<int> diff;
            set_difference(cards.at(player).begin(), cards.at(player).end(), outCards.begin(), outCards.end(), inserter(diff, diff.begin()));
            cards[player] = diff;

            vector<int> outHand;
            for (auto card : outCards)
            {
                outHand.push_back(card == 53 ? 14 : card == 52 ? 13 : card % 13);
            }

            //出牌不合法
            if (!initMeldGroup(outHand, *pCurMeld)
                || (prePlayer != -1 && prePlayer != player && !(*pPreMeld < *pCurMeld)))
            {
                throw 1;
            }

            prePlayer = player;
            PrintCards(outCards);
            handSize[player] -= outCards.size();
            if (handSize[player] == 0) //是否赢了
            {
                cout << "PLAYER " << player << " WIN" << endl;
                break;
            }
            swap(pCurMeld, pPreMeld);
        }
        robots[player].RbtInTakeOutCard(player, outCards);
        robots[(player + 1) % 3].RbtInTakeOutCard(player, outCards);
        robots[(player + 2) % 3].RbtInTakeOutCard(player, outCards);
        /*//只地主出牌
        outCards.clear();
        robots[player].RbtInTakeOutCard((player + 1) % 3, outCards);
        robots[player].RbtInTakeOutCard((player + 2) % 3, outCards);
        */
        cout << "PLAYER " << player << "（" << (player == aiSeat ? "AI" : "玩家") << "） REMAIN CARDS: ";
        PrintCards(cards[player]);
    }
    delete pCurMeld;
    pCurMeld = nullptr;
    delete pPreMeld;
    pPreMeld = nullptr;
    cout << "=====GAME END=====" << endl;
    LOOP_END
}


int main()
{
    //test();
    test_level();
    //test_play();
    system("pause");
    return 0;
}