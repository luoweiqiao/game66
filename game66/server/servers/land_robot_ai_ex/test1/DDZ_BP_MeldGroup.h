#pragma once
//#include "DDZ_BP_Function.h"
#include <vector>
#include "PokerUtils.h"
#include <string.h>
#include <algorithm>
// 全部以card[0~12, 13, 14]格式处理

#pragma warning(disable: 4244 4267)

struct BP_MELD_GROUP
{
	int seat;

	std::vector<int> cards;
	DDZ_BP_MELDTYPE type;

	int flag;		// 该牌组最小牌 （三带是三条中最小牌，四带是四条中最小牌）
	int bFlag[5];	// 带牌的所有牌flag
	int len;		// 该类型有多少组 （例如连对554433，len=3）

	bool operator<(const BP_MELD_GROUP &mg) const
	{
		//return type == mg.type && len == mg.len && flag < mg.flag && cards.size() == mg.cards.size();

		if (type == mg.type)
		{
			return (len == mg.len && flag < mg.flag && cards.size() == mg.cards.size());
		}
		else if (type == DDZ_BP_BOMB || type == DDZ_BP_ROCKET)
		{
			return mg.type == DDZ_BP_ROCKET;
		}
		return (mg.type == DDZ_BP_BOMB || mg.type == DDZ_BP_ROCKET);
	}
};

template<typename T> void sortT(std::vector<T> &vec, bool up)
{
	int size = vec.size();
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

inline bool dealSingle(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.flag = group.cards[sizeV - 1];
	memset(group.bFlag, 0, sizeof(group.bFlag));
	group.len = sizeV;
	group.type = group.len == 1 ? DDZ_BP_SINGLE : (DDZ_BP_MELDTYPE)(DDZ_BP_CSINGLE + group.len);

	if (sizeV == 1)
	{
		return true;
	}
	else if (sizeV < 5 || (sizeV > 0 && group.cards[0] >= 12))
	{
		return false;
	}

	for (int i = 1; i < sizeV; ++i)
	{
		if (group.cards[i] + 1 != group.cards[i - 1])
		{
			return false;
		}
	}
	return true;
}

inline bool dealPair(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.flag = group.cards[sizeV - 1];
	memset(group.bFlag, 0, sizeof(group.bFlag));
	group.len = sizeV / 2;
	group.type = group.len == 1 ? DDZ_BP_PAIR : DDZ_BP_CPAIR;

	if (sizeV == 2)
	{
		if (group.cards[0] == group.cards[1])
		{
			return true;
		}
		return false;
	}
	else if (sizeV < 6 || sizeV % 2 != 0 || (sizeV > 0 && group.cards[0] >= 12))
	{
		return false;
	}


	for (int i = 0; i < sizeV; i += 2)
	{
		if (group.cards[i] != group.cards[i + 1] ||
			(i + 2 < sizeV && group.cards[i] - 1 != group.cards[i + 2]))
		{
			return false;
		}
	}
	return true;
}

inline bool dealThree(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.flag = group.cards[sizeV - 1];
	memset(group.bFlag, 0, sizeof(group.bFlag));
	group.len = sizeV / 3;
	group.type = group.len == 1 ? DDZ_BP_THREE : DDZ_BP_PLANE;

	if (sizeV == 3)
	{
		if (group.cards[0] == group.cards[1] && group.cards[0] == group.cards[2])
		{
			return true;
		}
		return false;
	}
	else if (sizeV < 6 || sizeV % 3 != 0 || (sizeV > 0 && group.cards[0] >= 12))
	{
		return false;
	}


	for (int i = 0; i < sizeV; i += 3)
	{
		if (group.cards[i] != group.cards[i + 1] ||
			group.cards[i] != group.cards[i + 2] ||
			(i + 3 < sizeV && group.cards[i] - 1 != group.cards[i + 3]))
		{
			return false;
		}
	}
	return true;
}

inline bool dealThreeOne(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.len = sizeV / 4;
	group.type = group.len == 1 ? DDZ_BP_THREE_ONE : DDZ_BP_PLANE_ONE;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	std::vector<int> threes;
	std::vector<int> singles;
	for (int i = 0, j = 0; i < sizeV; ++i)
	{
		int thisCount = count(group.cards.begin(), group.cards.end(), group.cards[i]);
		if (thisCount == 3 && 
			(group.len == 1
			|| count(group.cards.begin(), group.cards.end(), group.cards[i] - 1) == 3 || count(group.cards.begin(), group.cards.end(), group.cards[i] + 1) == 3))
		{
			if (find(threes.begin(), threes.end(), group.cards[i]) == threes.end())
			{
				threes.push_back(group.cards[i]);
			}
			continue;
		}
		else if (thisCount != 4)
		{
			if (find(singles.begin(), singles.end(), group.cards[i]) == singles.end())
			{
				singles.insert(singles.end(), thisCount, group.cards[i]);
				for (int n = 0; n < thisCount; ++n)
				{
					group.bFlag[j] = group.cards[i];
					++j;
				}
			}
			continue;
		}
		return false;
	}
	if (threes.size() == 0 || threes.size() != singles.size())
	{
		return false;
	}
	group.flag = threes[threes.size() - 1];

	for (int i = 1, sizeT = threes.size(); i < sizeT; ++i)
	{
		if (threes[i] + 1 != threes[i - 1])
		{
			return false;
		}
	}
	return true;
}

inline bool dealThreeTwo(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.len = sizeV / 5;
	group.type = group.len == 1 ? DDZ_BP_THREE_TWO : DDZ_BP_PLANE_TWO;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	std::vector<int> threes;
	std::vector<int> pairs;
	for (int i = 0, j = 0; i < sizeV; ++i)
	{
		int thisCount = count(group.cards.begin(), group.cards.end(), group.cards[i]);
		if (thisCount == 3)
		{
			if (find(threes.begin(), threes.end(), group.cards[i]) == threes.end())
			{
				threes.push_back(group.cards[i]);
			}
			continue;
		}
		else if (thisCount == 2)
		{
			if (find(pairs.begin(), pairs.end(), group.cards[i]) == pairs.end())
			{
				pairs.push_back(group.cards[i]);
				group.bFlag[j] = group.cards[i];
				++j;
			}
			continue;
		}
		return false;
	}
	if (threes.size() == 0 || threes.size() != pairs.size())
	{
		return false;
	}
	group.flag = threes[threes.size() - 1];

	for (int i = 1, sizeT = threes.size(); i < sizeT; ++i)
	{
		if (threes[i] + 1 != threes[i - 1])
		{
			return false;
		}
	}
	return true;
}

inline bool dealFourOne(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.type = DDZ_BP_FOUR_ONE;
	group.len = 1;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	if (sizeV == 6)
	{
		if (count(group.cards.begin(), group.cards.end(), group.cards[0]) == 4)
		{
			group.flag = group.cards[0];
			group.bFlag[0] = MAX(group.cards[4], group.cards[5]);
			group.bFlag[1] = MIN(group.cards[4], group.cards[5]);
			return true;
		}
		else if (count(group.cards.begin(), group.cards.end(), group.cards[1]) == 4)
		{
			group.flag = group.cards[1];
			group.bFlag[0] = MAX(group.cards[0], group.cards[5]);
			group.bFlag[1] = MIN(group.cards[0], group.cards[5]);
			return true;
		}
		else if (count(group.cards.begin(), group.cards.end(), group.cards[2]) == 4)
		{
			group.flag = group.cards[2];
			group.bFlag[0] = MAX(group.cards[0], group.cards[1]);
			group.bFlag[1] = MIN(group.cards[0], group.cards[1]);
			return true;
		}
	}
	return false;
}

inline bool dealFourTwo(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.type = DDZ_BP_FOUR_TWO;
	group.len = 1;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	if (sizeV == 8)
	{
		if (count(group.cards.begin(), group.cards.end(), group.cards[0]) == 4 &&
			count(group.cards.begin(), group.cards.end(), group.cards[4]) == 2 &&
			count(group.cards.begin(), group.cards.end(), group.cards[6]) == 2)
		{
			group.flag = group.cards[0];
			group.bFlag[0] = MAX(group.cards[4], group.cards[6]);
			group.bFlag[1] = MIN(group.cards[4], group.cards[6]);
			return true;
		}
		else if (count(group.cards.begin(), group.cards.end(), group.cards[0]) == 2 &&
			count(group.cards.begin(), group.cards.end(), group.cards[2]) == 4 &&
			count(group.cards.begin(), group.cards.end(), group.cards[6]) == 2)
		{
			group.flag = group.cards[2];
			group.bFlag[0] = MAX(group.cards[0], group.cards[6]);
			group.bFlag[1] = MIN(group.cards[0], group.cards[6]);
			return true;
		}
		else if (count(group.cards.begin(), group.cards.end(), group.cards[0]) == 2 &&
			count(group.cards.begin(), group.cards.end(), group.cards[2]) == 2 &&
			count(group.cards.begin(), group.cards.end(), group.cards[4]) == 4)
		{
			group.flag = group.cards[4];
			group.bFlag[0] = MAX(group.cards[0], group.cards[2]);
			group.bFlag[1] = MIN(group.cards[0], group.cards[2]);
			return true;
		}
	}
	return false;
}

inline bool dealBowb(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.type = DDZ_BP_BOMB;
	group.len = 1;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	if (sizeV == 4 && count(group.cards.begin(), group.cards.end(), group.cards[0]) == 4)
	{
		group.flag = group.cards[0];
		return true;
	}
	return false;
}

inline bool dealRocket(BP_MELD_GROUP &group)
{
	sortT(group.cards, false);
	int sizeV = group.cards.size();

	group.type = DDZ_BP_ROCKET;
	group.len = 1;
	memset(group.bFlag, 0, sizeof(group.bFlag));

	if (sizeV == 2 && group.cards[0] == 14 && group.cards[1] == 13)
	{
		group.flag = BLACK_J_CARD;
		return true;
	}
	return false;
}

inline bool initMeldGroup(const std::vector<int> &cards_, BP_MELD_GROUP &group, DDZ_BP_MELDTYPE type = DDZ_BP_NOTHING)
{
	group.seat = -1;
	group.cards = cards_;

	if (group.cards.size() == 0)
	{
		return true;
	}

	switch (type)
	{
	case DDZ_BP_SINGLE:
	case DDZ_BP_CSINGLE:
	case DDZ_BP_CSINGLE5: case DDZ_BP_CSINGLE6: case DDZ_BP_CSINGLE7: case DDZ_BP_CSINGLE8:
	case DDZ_BP_CSINGLE9: case DDZ_BP_CSINGLE10: case DDZ_BP_CSINGLE11: case DDZ_BP_CSINGLE12:
													return dealSingle(group);
	case DDZ_BP_PAIR:			case DDZ_BP_CPAIR:			return dealPair(group);
	case DDZ_BP_THREE:			case DDZ_BP_PLANE:			return dealThree(group);
	case DDZ_BP_THREE_ONE:		case DDZ_BP_PLANE_ONE:		return dealThreeOne(group);
	case DDZ_BP_THREE_TWO:		case DDZ_BP_PLANE_TWO:		return dealThreeTwo(group);
	case DDZ_BP_FOUR_ONE:								return dealFourOne(group);
	case DDZ_BP_FOUR_TWO:								return dealFourTwo(group);
	case DDZ_BP_BOMB:									return dealBowb(group);
	case DDZ_BP_ROCKET:									return dealRocket(group);
	case DDZ_BP_NOTHING:
		{
			if (dealSingle(group))		return true;
			if (dealPair(group))		return true;
			if (dealThree(group))		return true;
			if (dealThreeOne(group))	return true;
			if (dealThreeTwo(group))	return true;
			if (dealFourOne(group))	    return true;
			if (dealFourTwo(group))	    return true;
			if (dealBowb(group))		return true;
			if (dealRocket(group))		return true;
		}
	}

	return false;
}

