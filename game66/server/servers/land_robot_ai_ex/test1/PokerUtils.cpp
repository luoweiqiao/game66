#include <unordered_map>
#include "PokerUtils.h"
#include <string.h>
#include <math.h>
#include <algorithm>

using namespace std;

CardSuit getSuit(int cardid)
{
	if (cardid == BLACK_J_ID)
	{
		return BLACK_J;
	}
	else if (cardid == RED_J_ID)
	{
		return RED_J;
	}

	switch (cardid / RANK_COUNT)
	{
	case 0: return DIAMOND;
	case 1: return CLUB;
	case 2: return HEART;
	case 3: return SPADE;
	}
	return SPADE;
}

CardRank getRank(int cardid)
{
	switch (cardid % RANK_COUNT)
	{
    case 0: return	CardRank::CARD_3;
    case 1: return	CardRank::CARD_4;
    case 2: return	CardRank::CARD_5;
    case 3: return	CardRank::CARD_6;
    case 4: return	CardRank::CARD_7;
    case 5: return	CardRank::CARD_8;
    case 6: return	CardRank::CARD_9;
    case 7: return	CardRank::CARD_T;
    case 8: return	CardRank::CARD_J;
    case 9: return	CardRank::CARD_Q;
    case 10: return	CardRank::CARD_K;
    case 11: return	CardRank::CARD_A;
    case 12: return CardRank::CARD_2;
	}
    return CardRank::CARD_3;
}

int getID(CardSuit suit, CardRank rank)
{
	switch (suit)
	{
	case DIAMOND:
	case CLUB:
	case HEART:
	case SPADE:		return (static_cast<int>(suit)* RANK_COUNT + static_cast<int>(rank));
	case BLACK_J:	return 52;
	case RED_J:		return 53;
	}
	return -1;
}

int card2id(int card, int suit)
{
	if (card == 13)
	{
		return 52;
	}
	else if (card == 14)
	{
		return 53;
	}
	return (13 * suit + card);
}

int id2card(int id)
{
	if (id == 52)
	{
		return 13;
	}
	else if (id == 53)
	{
		return 14;
	}
	else
	{
		return id % 13;
	}
}

char card2char(int card)
{
// 	static const std::unordered_map<int, char> cardsTrans = {
// 		{0, '3'},
// 		{1, '4'},
// 	};
// 	return cardsTrans[card];
	switch (card)
	{
	case 0: return '3';
	case 1: return '4';
	case 2: return '5';
	case 3: return '6';
	case 4: return '7';
	case 5: return '8';
	case 6: return '9';
	case 7: return 'T';
	case 8: return 'J';
	case 9: return 'Q';
	case 10: return 'K';
	case 11: return 'A';
	case 12: return '2';
	case 13: return 'X';
	case 14: return 'D';
	}
	return '?';
}

int char2card(char c)
{
	static const unordered_map<char, int> cardTrans = {
		{'3', 0}, {'4', 1}, {'5', 2}, {'6', 3}, {'7', 4},
		{'8', 5}, {'9', 6}, 
		{'T', 7}, {'J', 8}, {'Q', 9}, {'K', 10}, {'A', 11}, 
		{'t', 7}, {'j', 8}, {'q', 9}, {'k', 10}, {'a', 11},
		{'2', 12}, {'X', 13}, {'D', 14}, {'x', 13}, {'d', 14}
	};
	return cardTrans.at(c);
// 	switch (c)
// 	{
// 	case '3': return 0;
// 	case '4': return 1;
// 	case '5': return 2;
// 	case '6': return 3;
// 	case '7': return 4;
// 	case '8': return 5;
// 	case '9': return 6;
// 	case 'T': case 't': return 7;
// 	case 'J': case 'j': return 8;
// 	case 'Q': case 'q': return 9;
// 	case 'K': case 'k': return 10;
// 	case 'A': case 'a': return 11;
// 	case '2': return 12;
// 	case 'X': case 'x': return 13;
// 	case 'D': case 'd': return 14;
// 	}
// 	return 0;
}


template<typename T1, typename T2>
void sortBySecV(std::vector<std::pair<T1, T2>> &vec, bool up) //up=true 升序(小在前)
{
	int size = vec.size();
	for (int gap = size / 2; 0 < gap; gap /= 2)
	{
		for (int i = gap; i < size; ++i)
		{
			for (int j = i - gap; 0 <= j; j -= gap)
			{
				if ((up && vec[j + gap].second < vec[j].second) ||
					(!up && vec[j + gap].second > vec[j].second))
				{
					swap(vec[j], vec[j + gap]);
				}
			}
		}
	}
}

template<typename T>
void sortT(std::vector<T> &vec, bool up)
{
	int size = vec.size();
	if (size == 0)
	{
		return;
	}
	for (int gap = size / 2; 0 < gap; gap /= 2)
	{
		for (int i = gap; i < size; ++i)
		{
			for (int j = i - gap; 0 <= j; j -= gap)
			{
				if ((up && vec[j + gap] < vec[j]) ||
					(!up && vec[j + gap] > vec[j]))
				{
					std::swap(vec[j], vec[j + gap]);
				}
			}
		}
	}
}

int getMinSub(int *src, int count)
{
	int minv = 0xFFFFFFF;
	int minSub = -1;
	for (int i = 0; i < count; ++i)
	{
		if (src[i] < minv)
		{
			minv = src[i];
			minSub = i;
		}
		else if (src[i] == minv)
		{
			minSub = -1;
		}
	}
	return minSub;
}

int getMaxSub(int *src, int count)
{
	int maxv = -1;
	int maxSub = -1;
	for (int i = 0; i < count; ++i)
	{
		if (src[i] > maxv)
		{
			maxv = src[i];
			maxSub = i;
		}
		else if (src[i] == maxv)
		{
			maxSub = -1;
		}
	}
	return maxSub;
}


int getmax(int *x, int len)
{
	int r = x[0];
	for (int i = 1; i < len; ++i)
	{
		r = MAX(r, x[i]);
	}
	return r;
}

double getmax(double *x, int len)
{
	double r = x[0];
	for (int i = 1; i < len; ++i)
	{
		r = MAX(r, x[i]);
	}
	return r;
}

template<typename T>
T getmax(vector<T> x)
{
	T r = x[0];
	for (int i = 1, sizeI = x.size(); i < sizeI; ++i)
	{
		r = MAX(r, x[i]);
	}
	return r;
}

bool isbomb_id(std::vector<int> ids)
{
	if (ids.size() == 4)
	{
		int cards[4] = { id2card(ids[0]), id2card(ids[1]), id2card(ids[2]), id2card(ids[3]) };
		if (cards[1] == cards[0] && cards[2] == cards[0] && cards[3] == cards[0])
		{
			return true;
		}
	}
	else if (ids.size() == 2)
	{
		if (ids[0] + ids[1] == BLACK_J_ID + RED_J_ID)
		{
			return true;
		}
	}
	return false;
}


bool isbomb_card(std::vector<int> cards)
{
	if (cards.size() == 4)
	{
		if (cards[1] == cards[0] && cards[2] == cards[0] && cards[3] == cards[0])
		{
			return true;
		}
	}
	else if (cards.size() == 2)
	{
		if (cards[0] + cards[1] == 13 + 14)
		{
			return true;
		}
	}
	return false;
}
bool card2id_handid(std::vector<int> &cards, const std::vector<int> &IDs)
{
	std::vector<int> retIDs;
	std::vector<int> tempIDs = IDs;

	for (int i = 0, sizeI = cards.size(); i < sizeI; ++i)
	{
		switch (cards[i])
		{
		case 13:
		case 14:
		{
			int cardID = cards[i] + 39;
			std::vector<int>::iterator it = find(tempIDs.begin(), tempIDs.end(), cardID);
			if (it != tempIDs.end())
			{
				retIDs.push_back(cardID);
				tempIDs.erase(it);
			}
			break;
		}
		default:
		{
			for (int j = 0; j < 4; ++j)
			{
				int cardID = cards[i] + j * 13;
				std::vector<int>::iterator it = find(tempIDs.begin(), tempIDs.end(), cardID);
				if (it != tempIDs.end())
				{
					retIDs.push_back(cardID);
					tempIDs.erase(it);
					break;
				}
			}
			break;
		}
		}
	}

	if (retIDs.size() != cards.size())
	{
		return false;
	}

	cards.swap(retIDs);
	return true;
}

bool id2card_vec2array(int *cards, const std::vector<int> &IDs)
{
	memset(cards, 0, 15 * sizeof(int));
	for (int i = 0, size = IDs.size(); i < size; ++i)
	{
		cards[id2card(IDs[i])]++;
	}
	return true;
}

bool id2card_vec2vec(std::vector<int> &cards, const std::vector<int> &IDs)
{
	for (int i = 0, size = IDs.size(); i < size; ++i)
	{
		cards.push_back(id2card(IDs[i]));
	}
	sortT(cards, true);
	return true;
}

vector<int> str2card_deck(string str)
{
	vector<int> deck;
	if (str == "" || str == "p" || str == "pass" || str == "P" || str == "PASS")
	{
		return deck;
	}

	for (int i = 0, sizeI = str.size(); i < sizeI; ++i)
	{
		deck.push_back(char2card(str[i]));
	}

	sortT(deck, true);
	return deck;
}

vector<int> char2id_deck(string str)
{
	vector<int> deck;

	int subs[15] = { 0 };
	for (int i = 0, sizeI = str.size(); i < sizeI; ++i)
	{
		int card = char2card(str[i]);
		if (card == 13 || card == 14)
		{
			deck.push_back(card + 39);
		}
		else
		{
			deck.push_back(subs[card] * 13 + card);
		}
		++subs[card];
	}

	return deck;
}

bool iscard_char(char c)
{
	switch (c)
	{
	case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	case 't': case 'T': case 'j': case 'J': case 'q': case 'Q':
	case 'k': case 'K': case 'a': case 'A': case '2':
	case 'x': case 'X': case 'd': case 'D':
	case 'p': case 'P':
		return true;
	}
	return false;
}

int getPos(int seat, int lord)
{
	int lordPre = (lord + 2) % 3;
	int lordNext = (lord + 1) % 3;
	if (seat == lord)
	{
		return 1;
	}
	else if (seat == lordPre)
	{
		return 3;
	}
	return 2;
}

double calSqrtPow(const std::vector<int> &pa)
{
	double ret = 0;
	for (int i = 0, sizeI = pa.size(); i < sizeI; ++i)
	{
		//ret += pow(pa[i], 2);
		ret += pa[i] * pa[i];
	}
	return sqrt(ret);
}

double sigmoid(double x) //S函数
{
	return 1.0 / (1.0 + exp(-x));
}
