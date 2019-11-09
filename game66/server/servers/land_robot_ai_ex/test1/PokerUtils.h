#ifndef POKER_UTILS_H_
#define POKER_UTILS_H_

#include <cassert>
#include <vector>
#include <string>

// 代码中对牌的定义有两种格式：
// ID格式，值范围[0~53]，分别是四种花色的[3~4~...~A~2]，花色顺序是[D~C~H~S]，最后是小王大王
// Card格式，值范围[0~15]，分别是无花色的[3~4~...~A~2~X~D]

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

const int RANK_COUNT = 13; //牌点数
const int PLAYER_COUNT = 3;

const int BLACK_J_ID = 52;
const int RED_J_ID = 53;

const int BLACK_J_CARD = 13;
const int RED_J_CARD = 14;

enum CardSuit
{
	DIAMOND,
	CLUB,
	HEART,
	SPADE,
	RED_J,
	BLACK_J,
};

enum class CardRank
{
	CARD_3,
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
};

enum DDZ_BP_MELDTYPE
{
	DDZ_BP_NOTHING = 0,

	DDZ_BP_SINGLE = 1,
	DDZ_BP_PAIR = 2,
	DDZ_BP_THREE = 3,
	DDZ_BP_BOMB = 4,
	DDZ_BP_ROCKET = 5,

	DDZ_BP_FOUR_ONE = 6,
	DDZ_BP_FOUR_TWO = 7,

	DDZ_BP_CSINGLE = 10,
	DDZ_BP_CSINGLE5 = 15,
	DDZ_BP_CSINGLE6 = 16,
	DDZ_BP_CSINGLE7 = 17,
	DDZ_BP_CSINGLE8 = 18,
	DDZ_BP_CSINGLE9 = 19,
	DDZ_BP_CSINGLE10 = 20,
	DDZ_BP_CSINGLE11 = 21,
	DDZ_BP_CSINGLE12 = 22,

	DDZ_BP_PLANE = 33,
	DDZ_BP_CPAIR = 222,

	DDZ_BP_THREE_ONE = 31,
	DDZ_BP_THREE_TWO = 32,
	DDZ_BP_PLANE_ONE = 3311,
	DDZ_BP_PLANE_TWO = 3322,

	//为了方便记录牌型标签，加上长度信息（均不考虑带牌）
	DDZ_BP_PLANE2 = DDZ_BP_PLANE + 2,
	DDZ_BP_PLANE3 = DDZ_BP_PLANE + 3,
	DDZ_BP_PLANE4 = DDZ_BP_PLANE + 4,
	DDZ_BP_PLANE5 = DDZ_BP_PLANE + 5,
	DDZ_BP_PLANE6 = DDZ_BP_PLANE + 6,

	DDZ_BP_CPAIR3 = DDZ_BP_CPAIR + 3,
	DDZ_BP_CPAIR4 = DDZ_BP_CPAIR + 4,
	DDZ_BP_CPAIR5 = DDZ_BP_CPAIR + 5,
	DDZ_BP_CPAIR6 = DDZ_BP_CPAIR + 6,
	DDZ_BP_CPAIR7 = DDZ_BP_CPAIR + 7,
	DDZ_BP_CPAIR8 = DDZ_BP_CPAIR + 8,
	DDZ_BP_CPAIR9 = DDZ_BP_CPAIR + 9,

	// 	DDZ_BP_SINGLE_SERIES = 111,
	// 	DDZ_BP_PAIRS_SERIES = 222,
	// 	DDZ_BP_THREE_SERIES = 333,
};

struct Split_DDZ_Meld 
{
	DDZ_BP_MELDTYPE type;
	int low;
	int len;
	bool isCharge;//是否为冲锋套
	std::vector<int> cards;

	Split_DDZ_Meld(DDZ_BP_MELDTYPE t, int o, int e, const std::vector<int>& c) : type(t), low(o), len(e), cards(c) { isCharge = false; }
 	Split_DDZ_Meld() {}
	
	bool operator==(const Split_DDZ_Meld &meld) const
	{
		return type == meld.type && low == meld.low && len == meld.len;
	}
	bool operator!=(const Split_DDZ_Meld &meld) const
	{
		return !(*this == meld);
	}
	bool operator<(const Split_DDZ_Meld &meld) const
	{
		assert(type == meld.type && len == meld.len);
		return low < meld.low;
	}
	bool operator>(const Split_DDZ_Meld &meld) const
	{
		return !(*this < meld && *this == meld);
	}
};

CardSuit getSuit(int cardid);
CardRank getRank(int cardid);
int getID(CardSuit suit, CardRank rank);


int card2id(int card, int suit);
int id2card(int id);
char card2char(int card);
int char2card(char c);

bool card2id_handid(std::vector<int> &cards, const std::vector<int> &IDs);
bool id2card_vec2array(int *cards, const std::vector<int> &IDs);
bool id2card_vec2vec(std::vector<int> &cards, const std::vector<int> &IDs);
std::vector<int> str2card_deck(std::string str);
std::vector<int> char2id_deck(std::string str);


template<typename T1, typename T2> void sortBySecV(std::vector<std::pair<T1, T2>> &vec, bool up); // up=true 升序(小在前)
template<typename T> void sortT(std::vector<T> &vec, bool up);


// 找到最大最小值的下标，如果最大最小值不止一个，则返回-1
int getMinSub(int *src, int count);
int getMaxSub(int *src, int count);

int getmax(int *x, int len);
double getmax(double *x, int len);
template<typename T> T getmax(std::vector<T> x);


bool isbomb_id(std::vector<int> ids);
bool isbomb_card(std::vector<int> cards);

bool iscard_char(char c);

int getPos(int seat, int lord);

double calSqrtPow(const std::vector<int> &pa);
double sigmoid(double x); // S函数

#endif // !POKER_UTILS_H_
