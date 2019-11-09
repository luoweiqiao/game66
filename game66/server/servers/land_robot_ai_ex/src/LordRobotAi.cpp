// LordRobotAi.cpp : 定义 DLL 应用程序的入口点.

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include "OGLordRbtAIClvDepd.h"
#include "ddz_interface.h"

#define ALL_CARDS_NUM 54
using namespace std;
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************************************
//#define DEBUG_ROBOT
#define LEVEL 0
#define KEY_INFO 0
#define VERBOSE 10

FILE* logfile = NULL, *logfile2 = NULL;

void check_card(POKERS *h, int * card);
int is_game_first_half(GAME_DDZ_AI* game, int player);
int find_a_bigger_combo_in_hands(POKERS* h, COMBO_OF_POKERS * c, COMBO_OF_POKERS* a);
int Check_win_quick(GAME_DDZ_AI * gm, int firstplayer_num, bool cur, bool up, bool down); //need optimization
int get_the_best_hand(GAME_DDZ_AI *game, COMBO_OF_POKERS * cur, bool first);
void getPlayerTakeOutCards_internal(LordRobot* robot, int* poker, int * laizi, int player_number);
int takeOut_internal(LordRobot* robot, int * p, int* laizi);
void GAME_copy(GAME_DDZ_AI *dst, GAME_DDZ_AI *src);

//FILE* logfile,*logfile2;
int level = 0, log_brief = 0; /* 0 for ouput basic */
int for_test = 1;
int robot_id = 0; //robot's point

#define FUNC_NAME PRINTF(LEVEL,"ENTER %s\n",__FUNCTION__);
#define IS_HUN(h,i)   (i == h->hun)
#define HUN_NUMBER(h)   (h->hun>=0?h->hands[h->hun]:0)
#define SET_HUN_TO_i(h,i)  do{\
    h->hands[h->hun]--;\
    h->hands[i]++;\
    sort_poker(h);\
}while(0)

#define SET_i_to_HUN(h,i)  do{\
    h->hands[i]--;\
    h->hands[h->hun]++;\
    sort_poker(h);\
}while(0)



void PRINTF(int a, const char* const b, ...) {
#ifndef DEBUG_ROBOT
	return;
#else
	char   buffer[1024] = { 0 };

	va_list args;
	/* start or argument list */
	va_start(args, b);
	/* add in only format portion */
	vsprintf(buffer, (char *)b, args);

	/* tidy up */
	va_end(args);
	if (a <= level) {
		printf("%s", buffer);
	}
	if (a <= level)
	{
		if (logfile != NULL)
			fprintf(logfile, "%s", buffer);
		fflush(logfile);
	}
	if (a <= level) {
		if (logfile2 != NULL && log_brief)
			fprintf(logfile2, "%s", buffer);
	}
#endif
}

void PRINTF_ALWAYS(const char* const b, ...)
{
#ifndef DEBUG_ROBOT
	return;
#else
	char   buffer[1024] = { 0 };
	va_list args;
	/* start or argument list */
	va_start(args, b);
	/* add in only format portion */
	vsprintf(buffer, (char *)b, args);

	/* tidy up */
	va_end(args);
	printf("%s", buffer);
	//  if (logfile!=NULL)
	//  fprintf(logfile,    "%s",buffer);

	//if (logfile2!=NULL && log_brief)
	//  fprintf(logfile2,   "%s",buffer);
	//just for test
	if (for_test)
		while (1);
#endif
}

#undef CONTROL_NUM
#undef CONTROL_SUB_COMBO_NUM
#define CONTROL_NUM(player) ((player)->summary->ctrl.single+(player)->summary->extra_bomb_ctrl*10)
#define CONTROL_SUB_COMBO_NUM(player)  ( (CONTROL_NUM(player))-(10*(player)->summary->combo_total_num)-  ((player)->summary->combo_with_2_controls*10))
#define CONTROL_SUB_COMBO_NUM_IS_GOOD(player) ( CONTROL_SUB_COMBO_NUM(player) > -20 )
#define HAS_ROCKET(h)  (h->hands[LIT_JOKER] && h->hands[BIG_JOKER] )
#define PRE_PLAYER(game)  ( game->players[game->pre_playernum]->id )
#define NOT_BOMB(c)  (c->type!=BOMB_old && c->type!=ROCKET)
#define SET_PLAYER_POINT(game) \
    PLAYER * pl= game->players[CUR_PLAYER];\
    PLAYER * downPl= game->players[DOWN_PLAYER];\
    PLAYER * upPl= game->players[UP_PLAYER];


COMBO_OF_POKERS* check_for_win_now(GAME_DDZ_AI* game, COMBO_OF_POKERS* pre);

//poker operations

void full_poker(POKERS *h)
{
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		h->hands[i] = i >= LIT_JOKER ? 1 : 4;
	}
	h->end = BIG_JOKER;
	h->begin = P2;
	h->total = 54;
}


int sort_poker(POKERS * h)
{
	int res = true;
	h->begin = -1;
	h->end = h->total = 0;
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		if (h->hands[i]>0)
		{
			if (h->begin == -1) h->begin = i;
			h->end = i;
		}
		else if (h->hands[i]<0)
		{
			res = false;
			PRINTF_ALWAYS("ERR in sort_poker: fatal, poker %d is negative,reset it to 0\n", i);
			h->hands[i] = 0;
		}
		h->total += h->hands[i];
	}
	if (h->begin == -1) h->begin = 0;
	return res;
	//if (h->end>12) h->end = 12;
}


void add_poker(POKERS* a, POKERS * b, POKERS * c) // c=a+b
{
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		c->hands[i] = a->hands[i] + b->hands[i];
	}
	c->total = a->total + b->total;
	c->end = max_ddz(a->end, b->end);
	c->begin = min_ddz(a->begin, b->begin);
	sort_poker(c);
}

void sub_poker(POKERS* a, POKERS * b, POKERS * c) // c=a-b
{
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		c->hands[i] = a->hands[i] - b->hands[i];
	}
	sort_poker(c);
}

int cmp_poker(POKERS* a, POKERS * b) // return a!=b
{
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		if (a->hands[i] != b->hands[i])
			return 1;
	}
	return 0;
}

//return hun numbers
int sub_poker_with_hun(POKERS* a, POKERS * b, POKERS * c) // c=a-b
{
	int hun_num = 0;
	c->hun = a->hun;
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		c->hands[i] = a->hands[i] - b->hands[i];
		if (c->hands[i] <0) //carry from hun
		{
			hun_num = -c->hands[i];
			c->hands[c->hun] += c->hands[i];
			c->hands[i] = 0;

		}
	}
	sort_poker(c);
	return hun_num;
}

int convert_poker_to_hun(POKERS* src, POKERS * dst, POKERS* realhand)
{
	int hun_num = 0;
	int hun = realhand->hun;
	dst->hands[hun] = 0;
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{

		if (src->hands[i] > realhand->hands[i])
		{
			dst->hands[i] = realhand->hands[i];
			dst->hands[hun] += src->hands[i] - realhand->hands[i];
			hun_num += src->hands[i] - realhand->hands[i];
		}
		else if (i == hun)
			dst->hands[i] += src->hands[i];
		else
			dst->hands[i] = src->hands[i];
	}
	if (hun_num > realhand->hands[hun])
	{
		PRINTF_ALWAYS("line %d, No enough hun in real hand\n", __LINE__);
	}
	sort_poker(dst);
	return hun_num;
}

int is_sub_poker(POKERS* a, POKERS * b) // a in b ? 1 : 0
{
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		if (a->hands[i] > b->hands[i]) return false;
	}
	return true;
}

//check Poker a is the subset of poker B
int is_subset_poker_hun(POKERS* a, POKERS * b) // a in b ? 1 : 0
{
	int missed = 0;
	for (int i = 0; i<LIT_JOKER; i++)
	{
		if (a->hands[i] <= 0)
			continue;
		if (IS_HUN(b, i))
		{
			missed += a->hands[i];
			continue;
		}
		if (a->hands[i] > b->hands[i])
			missed += a->hands[i] - b->hands[i];
	}
	if (missed>HUN_NUMBER(b)) return false;
	for (int i = LIT_JOKER; i<MAX_POKER_KIND; i++)
	{
		if (a->hands[i] > b->hands[i]) return false;
	}
	return true;
}

int combo_2_poker(POKERS* hand, COMBO_OF_POKERS * h) // h must be contained by hand.
{
	for (int i = P3; i <= BIG_JOKER; i++)
		hand->hands[i] = 0;
	switch (h->type)
	{
	case ROCKET:
		hand->hands[LIT_JOKER] = 1;
		hand->hands[BIG_JOKER] = 1;
		break;
	case SINGLE_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) hand->hands[i] = 1;
		break;
	case PAIRS_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) {
			hand->hands[i] = 2;
		}
		break;
	case THREE_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) {
			hand->hands[i] = 3;
		}
		break;
	case BOMB_old:
		hand->hands[h->low] = 4;
		break;
	case THREE:
		hand->hands[h->low] = 3;
		break;
	case PAIRS:
		hand->hands[h->low] = 2;
		break;
	case SINGLE:
		hand->hands[h->low] = 1;
		break;
	case 31: //31
		for (int i = h->low; i<h->low + 1; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] = 1;
		break;
	case 32: //32
		for (int i = h->low; i<h->low + 1; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] = 2;
		//hand->hands[h->three_desc[0]];
		break;
	case 3311: //3311
		for (int i = h->low; i<h->low + 2; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		break;
	case 3322: //3322
		for (int i = h->low; i<h->low + 2; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		break;
	case 333111: //333111
		for (int i = h->low; i<h->low + 3; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		hand->hands[h->three_desc[2]] += 1;
		break;
	case 333222: //333222
		for (int i = h->low; i<h->low + 3; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 2;
		hand->hands[h->three_desc[1]] += 2;
		hand->hands[h->three_desc[2]] += 2;
		break;
	case 33332222: //33332222
		for (int i = h->low; i<h->low + 4; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 2;
		hand->hands[h->three_desc[1]] += 2;
		hand->hands[h->three_desc[2]] += 2;
		hand->hands[h->three_desc[3]] += 2;
		break;
	case 33331111: //33331111
		for (int i = h->low; i<h->low + 4; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		hand->hands[h->three_desc[2]] += 1;
		hand->hands[h->three_desc[3]] += 1;
		break;
	case 531: //531
		for (int i = h->low; i<h->low + 5; i++) hand->hands[i] = 3;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		hand->hands[h->three_desc[2]] += 1;
		hand->hands[h->three_desc[3]] += 1;
		hand->hands[(h->three_desc[4])] += 1;
		break;
	case 411: //411
		hand->hands[h->low] = 4;
		hand->hands[h->three_desc[0]] += 1;
		hand->hands[h->three_desc[1]] += 1;
		break;
	case 422: //422
		hand->hands[h->low] = 4;
		hand->hands[h->three_desc[0]] += 2;
		hand->hands[h->three_desc[1]] += 2;
		break;
	case NOTHING_old:
		//break;
	default:
		PRINTF_ALWAYS("line %d, ERR, remove failed, this combo type %d not supported\n", __LINE__, h->type);
		break;
	};

	return sort_poker(hand);

}

int remove_combo_poker(POKERS* hand, COMBO_OF_POKERS * h, COMBO_OF_POKERS *h1) // h must be contained by hand.
{
	POKERS t;
	int err = combo_2_poker(&t, h);
	if (hand->hun == -1)
	{
		sub_poker(hand, &t, hand);
		return err;
	}
	else
		return sub_poker_with_hun(hand, &t, hand);
}

//todo: add a parma number in COMBO_OF_POKERS
int get_combo_number(COMBO_OF_POKERS *h)
{
	{
		switch (h->type)
		{
		case ROCKET:
			return 2;
		case SINGLE_SERIES:
			return h->len;
		case PAIRS_SERIES:
			return h->len * 2;
		case THREE_SERIES:
			return h->len * 3;
		case 31: //31
		case BOMB_old:
			return 4;
		case THREE:
			return 3;
		case PAIRS:
			return 2;
		case SINGLE:
			return 1;

		case 32: //32
			return 5;
		case 411:
			return 6;
		case 422:
		case 3311: //3311
			return 8;
		case 3322: //3322
			return 10;
		case 333111: //333111
			return 12;
		case 333222: //333222
			return 15;
		case 33331111: //333111
			return 16;
			//  case 333222: //333222
		case 531: //333111
		case 33332222: //333222
			return 20;

		default:
			return 0;
		}
		//PRINTF(LEVEL," ");
	}
}

int check_combo_a_Big_than_b(COMBO_OF_POKERS * a, COMBO_OF_POKERS *b)
{
	if (b->type <= ROCKET || b->type == THREE_ONE || b->type == THREE_TWO)
		b->len = 1; //double check
	if (a->type <= ROCKET || a->type == THREE_ONE || a->type == THREE_TWO)
		a->len = 1; //double check

	if (a->type == ROCKET)
		return true;
	if (b->type == ROCKET)
		return false;
	if (NOT_BOMB(a) && (b->type != a->type || a->len != b->len))
	{
		return false;
	}
	if (b->type == a->type && a->len == b->len && a->type != BOMB_old)
	{
		if (b->low< a->low)
			return true;
		else
			return false;
	}
	if (a->type == BOMB_old && NOT_BOMB(b))
		return true;
	if ((a->type == BOMB_old && b->type == BOMB_old))
	{
		if (a->three_desc[0]>b->three_desc[0])
			return true;
		else if (a->three_desc[0]<b->three_desc[0])
			return false;
		else   if (b->low< a->low)
			return true;
	}
	return false;
}

int  int_2_poker(int i)
{
	if (i>51)
		i -= 39;
	else
		i = i % 13;
	return i;
}

//io functions
char poker_to_char(int i)
{
	switch (i) {
	case 0:
		return '3';
	case 1:
		return '4';
	case 2:
		return '5';
	case 3:
		return '6';
	case 4:
		return '7';
	case 5:
		return '8';
	case 6:
		return '9';
	case 7:
		return 'T';
	case 8:
		return 'J';
	case 9:
		return 'Q';
	case 10:
		return 'K';
	case 11:
		return 'A';
	case 12:
		return '2';
	case 13:
		return 'X';
	case 14:
		return 'D';
	default:
		return '?';
	}
}

int char_to_poker(char c)
{
	switch (c) {
	default:
		return -1;
	case '3':
		return 0;

	case '4':
		return 1;

	case '5':
		return 2;

	case '6':
		return 3;

	case '7':
		return 4;

	case '8':
		return 5;

	case '9':
		return 6;

	case 't':
	case 'T':
		return 7;

	case 'j':
	case 'J':
		return 8;

	case 'q':
	case 'Q':
		return 9;

	case 'k':
	case 'K':
		return 10;

	case 'a':
	case 'A':
		return 11;

	case '2':
		return 12;

	case 'x':
	case 'X':
		return 13;

	case 'D':
	case 'd':
		return 14;
	}
}

void print_one_poker(int i)
{
	PRINTF(LEVEL, "%c", poker_to_char(i));
}

void print_hand_poker(POKERS * h)
{
	PRINTF(LEVEL, "===total %d      ", h->total);//,poker_to_char(h->begin),poker_to_char(h->end));
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		if (h->hands[i]<0)
			PRINTF(LEVEL, " ERR ");
		for (int j = 0; j<h->hands[i]; j++)
			print_one_poker(i);
	}
	PRINTF(LEVEL, "       ===\n");
}

void print_hand_poker_in_line(POKERS * h)
{
	//PRINTF(LEVEL,"total %d\n===       ",h->total);//,poker_to_char(h->begin),poker_to_char(h->end));
	for (int i = 0; i<MAX_POKER_KIND; i++)
	{
		if (h->hands[i]<0)
			PRINTF(LEVEL, " ERR ");
		for (int j = 0; j<h->hands[i]; j++)
			print_one_poker(i);
	}
	//PRINTF(LEVEL,"       ===\n");
}

void print_combo_poker(COMBO_OF_POKERS * h /*, COMBO_OF_POKERS *h1 /*for three*/)
{
	if (h == NULL)
		return;
	switch (h->type)
	{
	case ROCKET:
		PRINTF(LEVEL, "Rocket!");
		break;
	case SINGLE_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) print_one_poker(i);
		break;
	case PAIRS_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) {
			print_one_poker(i);
			print_one_poker(i);
		}
		break;
	case THREE_SERIES:
		for (int i = h->low; i<h->low + h->len; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		break;
	case BOMB_old:
		print_one_poker(h->low);
	case THREE:
		print_one_poker(h->low);
	case PAIRS:
		print_one_poker(h->low);
	case SINGLE:
		print_one_poker(h->low);
		break;
	case 31: //31
		for (int i = h->low; i<h->low + 1; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		break;
	case 32: //32
		for (int i = h->low; i<h->low + 1; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[0]);
		break;
	case 3311: //3311
		for (int i = h->low; i<h->low + 2; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		break;
	case 3322: //3322
		for (int i = h->low; i<h->low + 2; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[1]);
		break;
	case 333111: //333111
		for (int i = h->low; i<h->low + 3; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[2]);
		break;
	case 333222: //333222
		for (int i = h->low; i<h->low + 3; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[2]);
		print_one_poker(h->three_desc[2]);
		break;
	case 33331111: //33331111
		for (int i = h->low; i<h->low + 4; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[2]);
		print_one_poker(h->three_desc[3] & 0xf);
		break;
	case 33332222: //33332222
		for (int i = h->low; i<h->low + 4; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[2]);
		print_one_poker(h->three_desc[2]);
		print_one_poker(h->three_desc[3]);
		print_one_poker(h->three_desc[3]);
		break;
	case 531: //531
		for (int i = h->low; i<h->low + 5; i++) {
			print_one_poker(i);
			print_one_poker(i);
			print_one_poker(i);
		}
		print_one_poker(h->three_desc[0]);
		print_one_poker(h->three_desc[1]);
		print_one_poker(h->three_desc[2]);
		print_one_poker(h->three_desc[3]);
		print_one_poker((h->three_desc[4]));
		break;
	case NOTHING_old:
		break;
	default:
		PRINTF(LEVEL, "ERR, this combo type %d not supported\n", h->type);
		break;
	}
	PRINTF(LEVEL, " ");
}

void print_all_combos(COMBO_OF_POKERS *combos, int number)
{
	DBG(
		for (int k = 0; k<number; k++)
		{
			print_combo_poker(&combos[k]);
		}
	PRINTF(LEVEL, "\n");
	);
}


void print_card_2(int * p, int num)
{
	//DBG(
	char t[4] = { 'd','c','h','s' };
	PRINTF(LEVEL, "CARD: ");
	int i = 0;
	while (i++<num)
	{
		if (*p == -1) {
			p++;
			continue;
		}
		if (*p>53 || *p<0) {
			PRINTF_ALWAYS("fatal error(card number too big)!\n");
			return;
		}
		if (*p>51)
			PRINTF(LEVEL, "%c ", *p == 52 ? 'X' : 'D');
		else
			PRINTF(LEVEL, "%c%d ", t[*p / 13], *p % 13);
		p++;
	}
	PRINTF(LEVEL, "\n");
	//)
}

void print_card(int * p)
{
	//DBG(
	char t[4] = { 'd','c','h','s' };
	PRINTF(LEVEL, "CARD: ");
	while (*p != -1)
	{
		if (*p>53 || *p<0) {
			PRINTF_ALWAYS("fatal error(card number too big)!\n");
			return;
		}
		if (*p>51)
			PRINTF(LEVEL, "%c ", *p == 52 ? 'X' : 'D');
		else
			PRINTF(LEVEL, "%c%d ", t[*p / 13], *p % 13);
		p++;
	}
	PRINTF(LEVEL, "\n");
	//)
}


void print_summary(COMBOS_SUMMARY_DDZ_AI * s)
{
	PRINTF(LEVEL, "bomb_ctrl %d,", s->extra_bomb_ctrl);
	PRINTF(LEVEL, "combos %d,big %d,type %d total %d\n", s->combo_total_num, s->biggest_num, s->combo_typenum, s->real_total_num);
	//PRINTF(LEVEL,"single %d\n",s->singles_num);

	//PRINTF(LEVEL, "\nSingles: ");
	for (int k = 0; k<s->singles_num; k++)
	{
		print_combo_poker(s->singles[k]);
	}
	//PRINTF(LEVEL, "\nPairs: ");
	for (int k = 0; k<s->pairs_num; k++)
	{
		print_combo_poker(s->pairs[k]);
	}
	//PRINTF(LEVEL, "\nThrees: ");
	for (int k = 0; k<s->three_num; k++)
	{
		print_combo_poker(s->three[k]);
	}
	for (int k = 0; k<s->three_one_num; k++)
	{
		print_combo_poker(s->three_one[k]);
	}
	for (int k = 0; k<s->three_two_num; k++)
	{
		print_combo_poker(s->three_two[k]);
	}
	//PRINTF(LEVEL, "\nSeries: ");
	for (int k = 0; k<s->series_num; k++)
	{
		print_combo_poker(s->series[k]);
	}
	//PRINTF(LEVEL, "\nBombs: ");
	for (int k = 0; k<s->bomb_num; k++)
	{
		print_combo_poker(s->bomb[k]);
	}
	PRINTF(LEVEL, "  big:");
	for (int k = 0; k<s->biggest_num; k++)
	{
		print_combo_poker(s->biggest[k]);
	}
	PRINTF(LEVEL, "\n");
}

void print_suit(PLAYER * player)
{
	COMBOS_SUMMARY_DDZ_AI * s = player->summary;
	//    PRINTF(LEVEL+1,"\nctrl %d extra_ctrl %d,",player->MAX_control.single,s->extra_bomb_ctrl);
	PRINTF(LEVEL + 1, "combos %d,big %d,type %d total %d\n", s->combo_total_num, s->biggest_num, s->combo_typenum, s->real_total_num);
	//PRINTF(LEVEL,"single %d\n",s->singles_num);

	//PRINTF(LEVEL, "\nSingles: ");
	for (int k = 0; k<s->singles_num; k++)
	{
		print_combo_poker(s->singles[k]);
	}
	//PRINTF(LEVEL, "\nPairs: ");
	for (int k = 0; k<s->pairs_num; k++)
	{
		print_combo_poker(s->pairs[k]);
	}
	//PRINTF(LEVEL, "\nThrees: ");
	for (int k = 0; k<s->three_num; k++)
	{
		print_combo_poker(s->three[k]);
	}
	for (int k = 0; k<s->three_one_num; k++)
	{
		print_combo_poker(s->three_one[k]);
	}
	for (int k = 0; k<s->three_two_num; k++)
	{
		print_combo_poker(s->three_two[k]);
	}
	//PRINTF(LEVEL, "\nSeries: ");
	for (int k = 0; k<s->series_num; k++)
	{
		print_combo_poker(s->series[k]);
	}
	//PRINTF(LEVEL, "\nBombs: ");
	for (int k = 0; k<s->bomb_num; k++)
	{
		print_combo_poker(s->bomb[k]);
	}
	PRINTF(LEVEL, "  big:");
	for (int k = 0; k<s->biggest_num; k++)
	{
		print_combo_poker(s->biggest[k]);
	}
	PRINTF(LEVEL, "\n");
}


void read_poker(char * buf, POKERS * h)
{
	do {
		switch (*buf) {
		case '3':
			h->hands[0]++;
			break;
		case '4':
			h->hands[1]++;
			break;
		case '5':
			h->hands[2]++;
			break;
		case '6':
			h->hands[3]++;
			break;
		case '7':
			h->hands[4]++;
			break;
		case '8':
			h->hands[5]++;
			break;
		case '9':
			h->hands[6]++;
			break;
		case 't':
		case 'T':
			h->hands[7]++;
			break;
		case 'j':
		case 'J':
			h->hands[8]++;
			break;
		case 'q':
		case 'Q':
			h->hands[9]++;
			break;
		case 'k':
		case 'K':
			h->hands[10]++;
			break;
		case 'a':
		case 'A':
			h->hands[11]++;
			break;
		case '2':
			h->hands[12]++;
			break;
		case 'x':
		case 'X':
			h->hands[13]++;
			break;
		case 'D':
		case 'd':
			h->hands[14]++;
			break;
		default:
			break;
		}
	} while (*buf++ != '\0');


	//check more than four
	{
		int i;
		for (i = 0; i<15; i++)
		{
			if (h->hands[i]>4) h->hands[i] = 4;
		}
	}
	sort_poker(h);
}

int read_poker_int(int * buf, POKERS * h)
{
	{
		int i;
		for (i = 0; i<15; i++)
		{
			h->hands[i] = 0;
		}
	}
	char count = 0;
	do {
		int a = *buf;
		if (a == -1) break;
		count++;
		if (a<52)
			a = a % 13;
		else
			a = (a - 39) % 15;
		h->hands[a]++;
		buf++;
	} while (1);


	//check more than four
	{
		int i;
		for (i = 0; i<15; i++)
		{
			if (h->hands[i]>4) h->hands[i] = 4;
		}
	}
	sort_poker(h);
	return count;
}

int read_poker_int_2(int * buf, int len, POKERS * h)
{
	{
		int i;
		for (i = 0; i<15; i++)
		{
			h->hands[i] = 0;
		}
	}
	char count = 0, count1 = 0;
	do {
		int a = *buf;
		count1++;
		if (count1>len)
			break;
		if (a == -1) { buf++; continue; }
		count++;
		if (a<52)
			a = a % 13;
		else
			a = (a - 39) % 15;
		h->hands[a]++;
		buf++;
	} while (1);


	//check more than four
	{
		int i;
		for (i = 0; i<15; i++)
		{
			if (h->hands[i]>4) h->hands[i] = 4;
		}
	}
	sort_poker(h);
	return count;
}

int find_and_remove_int_in_card(int* card, int value) {
	for (int i = 0; i<20; i++)
	{
		int t = -1;
		if (card[i] != -1)
		{
			if (card[i]>51)
				t = card[i] - 39;
			else
				t = card[i] % 13;
		}
		if (t == value) {
			int tmp = card[i];
			card[i] = -1;
			return tmp;
		}
	}
	return -1;
}

void remove_poker_in_int_array(POKERS * h, int * p, int *card, int hun)
{
	int hun_num = 0;
	int tmp[1 + 20 + 4 + 1];
	int aim_carder[21], *aim = aim_carder;
	int* p_bak = p;
	p = tmp;
	for (int i = P3; i <= BIG_JOKER; i++)
	{
		while (h->hands[i] != 0)
		{
			*p = find_and_remove_int_in_card(card, i);
			if (*p == -1)
			{
				if (i >= LIT_JOKER)
					PRINTF_ALWAYS("FUCK in line %d, no card for %d or hun %d\n", __LINE__, i, hun);
				//check hun again
				*p = find_and_remove_int_in_card(card, hun);
				if (*p == -1)
				{
					PRINTF_ALWAYS("FUCK in line %d, no card for %d or hun %d\n", __LINE__, i, hun);
				}
				*aim++ = i;
				hun_num++;
				p++;
				h->hands[i]--;
			}
			else
			{
				if (i == hun)
				{
					hun_num++;
					*aim++ = i;
				}
				p++;
				h->hands[i]--;
			}
		}
	}
	*p = -1;

	// re-org for hun
	if (1)// hun_num)
	{
		int huns[4];
		p = p_bak;
		*p++ = hun_num;
		int j = 0;
		for (int i = 0; i<21; i++)
		{
			if (tmp[i] != -1)
			{
				if (tmp[i] >= 52 || (tmp[i] % 13) != hun)
					*p++ = tmp[i];
				else
					huns[j++] = tmp[i];
			}
			else
				break;
		}
		for (int i = 0; i<hun_num; i++)
		{
			*p++ = aim_carder[i];//huns[i];
		}
		//just for internal testing...
		for (int i = 0; i<hun_num; i++)
		{
			*p++ = huns[i];
		}
		*p++ = -1;
	}
	else
	{
		memcpy(p_bak, tmp, 21 * sizeof(int));
	}

}
void combo_to_int_array_hun(COMBO_OF_POKERS *cur, int * p, int *card, int hun)
{
	POKERS h;
	combo_2_poker(&h, cur);
	//  convert_poker_to_hun(&h1,&h,realhand);
	int hun_num = 0;
	int tmp[1 + 20 + 4 + 1];
	int aim_carder[21], *aim = aim_carder;
	int* p_bak = p;
	p = tmp;
	for (int i = P3; i <= BIG_JOKER; i++)
	{
		while (h.hands[i] != 0)
		{
			*p = find_and_remove_int_in_card(card, i);
			if (*p == -1)
			{
				if (i >= LIT_JOKER)
					PRINTF_ALWAYS("FUCK in line %d, no card for %d or hun %d\n", __LINE__, i, hun);
				//check hun again
				*p = find_and_remove_int_in_card(card, hun);
				if (*p == -1)
				{
					PRINTF_ALWAYS("FUCK in line %d, no card for %d or hun %d\n", __LINE__, i, hun);
				}
				*aim++ = i;
				hun_num++;
				p++;
				h.hands[i]--;
			}
			else
			{
				if (i == hun)
				{
					hun_num++;
					*aim++ = i;
				}
				p++;
				h.hands[i]--;
			}
		}
	}
	*p = -1;

	// re-org for hun
	if (1)// hun_num)
	{
		int huns[4];
		p = p_bak;
		*p++ = hun_num;
		int j = 0;
		for (int i = 0; i<21; i++)
		{
			if (tmp[i] != -1)
			{
				if (tmp[i] >= 52 || (tmp[i] % 13) != hun)
					*p++ = tmp[i];
				else
					huns[j++] = tmp[i];
			}
			else
				break;
		}
		for (int i = 0; i<hun_num; i++)
		{
			*p++ = aim_carder[i];//huns[i];
		}
		//just for internal testing...
		for (int i = 0; i<hun_num; i++)
		{
			*p++ = huns[i];
		}
		*p++ = -1;
	}
	else
	{
		memcpy(p_bak, tmp, 21 * sizeof(int));
	}

}

void combo_to_int_array(COMBO_OF_POKERS *cur, int * p, int *card)
{
	POKERS h;
	combo_2_poker(&h, cur);
	for (int i = P3; i <= BIG_JOKER; i++)
	{
		while (h.hands[i] != 0)
		{
			*p = find_and_remove_int_in_card(card, i);
			if (*p == -1)
			{
				PRINTF_ALWAYS("FUCK in line %d\n", __LINE__);
			}
			p++;
			h.hands[i]--;
		}
	}
	*p = -1;
}



//check functions

int is_series(POKERS *h, int number)
{

	for (int i = h->begin; i <= CEIL_A(h->end); i++)
	{
		if (h->hands[i] != 1) return false;
	}
	return true;
}

int is_doubleseries(POKERS *h)
{

	for (int i = h->begin; i <= CEIL_A(h->end); i++)
	{
		if (h->hands[i] != 2) return false;
	}
	return true;
}

int is_threeseries(POKERS *h)
{

	for (int i = h->begin; i <= CEIL_A(h->end); i++)
	{
		if (h->hands[i] != 3) return false;
	}
	return true;
}

int is_411(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] == 4)
		{
			c->type = 411;
			c->low = i;
			c->len = 1;
			break;
		}
	}
	if (c->type != 411)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 1)
			c->three_desc[j++] = i;
		else if (h->hands[i] == 2)
		{
			c->three_desc[0] = i;
			c->three_desc[1] = i;
		}
	}
	if (c->type == 411)
		return true;
	else
		return false;
}


int is_422(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] == 4)
		{
			c->type = 422;
			c->len = 1;
			c->low = i;
			break;
		}
	}

	if (c->type != 422)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 2)
			c->three_desc[j++] = i;
	}
	if (j != 2)
		c->type = -1;
	if (c->type == 422)
		return true;
	else
		return false;
}


int is_31(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] == 3)
		{
			c->type = 31;
			c->len = 1;
			c->low = i;
			break;
		}
	}
	if (c->type != 31)
		return false;
	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 1)
		{
			c->three_desc[j++] = i;
			break;
		}
	}
	if (c->type == 31)
		return true;
	else
		return false;
}

int is_32(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] == 3)
		{
			c->type = 32;
			c->len = 1;
			c->low = i;
			break;
		}
	}
	if (c->type != 32)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 2)
		{
			c->three_desc[j++] = i;
			break;
		}
	}
	if (j != 1)
		c->type = -1;
	if (c->type == 32)
		return true;
	else
		return false;
}



int is_3322(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 1; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3)
		{
			c->type = 3322;
			c->low = i;
			c->len = 2;
			break;
		}
	}

	if (c->type != 3322)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		/*
		if( h->hands[i]==4){
		c->three_desc[0] = i;
		c->three_desc[1] = i;
		break;
		}
		else */
		if (h->hands[i] == 2)
			c->three_desc[j++] = i;
		//double check
		if (j >= 2)
			break;
	}
	if (j != 2)
		c->type = -1;
	if (c->type == 3322)
		return true;
	else
		return false;
}


int is_333222(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 2; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3 && h->hands[i + 2] == 3)
		{
			c->type = 333222;
			c->low = i;
			c->len = 3;
			break;
		}
	}
	if (c->type != 333222)
		return false;
	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 2)
			c->three_desc[j++] = i;
		//double check
		if (j >= 3)
			break;
	}
	if (j != 3)
		c->type = -1;
	if (c->type == 333222)
		return true;
	else
		return false;
}

int is_3311(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 1; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3)
			//         if ( h->hands[i] >=3 && h->hands[i+1] >=3 )
		{
			c->type = 3311;
			c->low = i;
			c->len = 2;
			break;
		}
	}
	if (c->type != 3311)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 2)
		{
			c->three_desc[j++] = i;
			c->three_desc[j++] = i;
		}
		else  if (h->hands[i] == 1)
			c->three_desc[j++] = i;
		if (j >= 2)
			break;
	}
	if (c->type == 3311)
		return true;
	else
		return false;
}

int is_333111(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 2; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3 && h->hands[i + 2] == 3)
		{
			c->type = 333111;
			c->low = i;
			c->len = 3;
			h->hands[i] -= 3;
			h->hands[i + 1] -= 3;
			h->hands[i + 2] -= 3;
			break;
		}
	}
	if (c->type != 333111)
		return false;
	for (int i = h->begin; i <= (h->end); i++)
	{

		if (h->hands[i] >= 1)
		{
			for (int k = 0; k<h->hands[i]; k++)
				c->three_desc[j++] = i;
			if (j >= 3)
				break;
		}
	}
	h->hands[c->low] += 3;
	h->hands[c->low + 1] += 3;
	h->hands[c->low + 2] += 3;

	if (c->type == 333111)
		return true;
	else
		return false;
}


int is_33331111(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 3; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3 && h->hands[i + 2] == 3 && h->hands[i + 3] == 3)
		{
			c->type = 33331111;
			c->len = 4;
			c->low = i;
			h->hands[i] -= 3;
			h->hands[i + 1] -= 3;
			h->hands[i + 2] -= 3;
			h->hands[i + 3] -= 3;
			break;
		}
	}
	if (c->type != 33331111)
		return false;
	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] >= 1)
		{
			for (int k = 0; k<h->hands[i]; k++)
				c->three_desc[j++] = i;
		}

	}
	h->hands[c->low] += 3;
	h->hands[c->low + 1] += 3;
	h->hands[c->low + 2] += 3;
	h->hands[c->low + 3] += 3;

	if (c->type == 33331111)
		return true;
	else
		return false;
}

int is_531(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 3; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3 && h->hands[i + 2] == 3 && h->hands[i + 3] == 3
			&& h->hands[i + 4] == 3)
		{
			c->type = 531;
			c->len = 5;
			c->low = i;
			h->hands[i] -= 3;
			h->hands[i + 1] -= 3;
			h->hands[i + 2] -= 3;
			h->hands[i + 3] -= 3;
			h->hands[i + 4] -= 3;
			break;
		}
	}
	if (c->type != 531)
		return false;

	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] >= 1)
		{
			for (int k = 0; k<h->hands[i]; k++)
				c->three_desc[j++] = i;
		}
	}
	h->hands[c->low] += 3;
	h->hands[c->low + 1] += 3;
	h->hands[c->low + 2] += 3;
	h->hands[c->low + 3] += 3;
	h->hands[c->low + 4] += 3;

	if (c->type == 531)
		return true;
	else
		return false;
}




int is_33332222(POKERS *h, COMBO_OF_POKERS *c)
{
	int j = 0;
	c->type = -1;
	for (int i = h->begin; i <= CEIL_A(h->end) - 3; i++)
	{
		if (h->hands[i] == 3 && h->hands[i + 1] == 3 && h->hands[i + 2] == 3 && h->hands[i + 3] == 3)
		{
			c->type = 33332222;
			c->low = i;
			c->len = 4;
			break;
		}
	}
	if (c->type != 33332222)
		return false;
	for (int i = h->begin; i <= (h->end); i++)
	{
		if (h->hands[i] == 2)
			c->three_desc[j++] = i;
		if (j >= 4)
			break;

	}
	if (j != 4)
		c->type = -1;
	if (c->type == 33332222)
		return true;
	else
		return false;
}

int is_combo(POKERS *src, COMBO_OF_POKERS * c)
{
	c->type = -1;
	c->control = 0;
	POKERS t, *h = &t;
	*h = *src;
	int number = 0;
	if (h->total> 0)
	{
		int number = h->total;
		c->len = h->total;
		c->low = h->begin;
		if (number>4 && h->end <= Pa)
		{
			if (is_series(h, number))
			{
				c->type = SINGLE_SERIES;
				return true;
			}
			else if (number % 2 == 0 && is_doubleseries(h))
			{
				c->type = PAIRS_SERIES;
				c->len /= 2;
				return true;
			}
			else if (number % 3 == 0 && is_threeseries(h))
			{
				c->type = THREE_SERIES;
				c->len /= 3;
				return true;
			}
		}
		switch (h->total)
		{
		case 1:
			c->type = SINGLE;
            c->len = 1;
			break;
		case 2:
			if (h->hands[h->begin] == 2) {
				c->type = PAIRS;
				c->len >>= 1;
			}
			else if (h->begin == LIT_JOKER) c->type = ROCKET;
			break;
		case 3:
            if (h->hands[h->begin] == 3)
            {
                c->type = THREE;
                c->len = 1;
            }
			break;
		case 4:
			if (h->hands[h->begin] == 4) {
				c->type = BOMB_old;
                c->len = 1;
				c->three_desc[0] = 0;
			}
			else if (h->hands[h->begin] == 3 || h->hands[h->end] == 3)
			{
				c->type = 31;
				c->len = 1;
				if (h->hands[h->begin] == 3)
				{
					c->three_desc[0] = h->end;
					c->low = h->begin;
				}
				else
				{
					c->three_desc[0] = h->begin;
					c->low = h->end;
				}
			}
			break;
		case 5:
			is_32(h, c);
			break;
		case 6:
			is_411(h, c);
			break;
		case 7:
			break;
		case 8:
			if (!is_3311(h, c))
				is_422(h, c);
			break; //3311
		case 9:
			break;
		case 10:
			is_3322(h, c);
			break;//3322
		case 11:
			break;
		case 12:
			is_333111(h, c);
			break; //333111
		case 13:
			break;
		case 15:
			is_333222(h, c);
			break;
		case 16:
			is_33331111(h, c);
			break; //33331111
		case 20:
			if (!is_33332222(h, c))
				is_531(h, c);
			break; //33332222
		default:
			break;

		}
		// return true;
	}
	if (c->type != -1)
		return true;
	else
		return false;
}

//return 1 if c's priority is higher 
int check_prior_first(COMBO_OF_POKERS *c, COMBO_OF_POKERS *a)
{
	if (a->type == NOTHING_old)
		return 1;

	if (c->type == a->type && c->low >a->low)
		return 1;

	switch (get_combo_number(c))
	{
	case 1:
	case 2:
	case 3:
		if (c->low >a->low)
			return 1;
		break;
	case 4:
		if (c->type == BOMB_old && (c->low >a->low || a->type != BOMB_old))
			return 1;
		else if (c->type == 31 && c->low >a->low && a->type == 31)
			return 1;
		break;
	case 5:
		if (c->type == 32 && (a->type != 32))
			return 1;
		break;
	case 6:
		if (c->type == PAIRS_SERIES && (a->type != PAIRS_SERIES))
			return 1;
		else if (c->type == 411 && (a->type != 411 && a->type != PAIRS_SERIES))
			return 1;
		else if (c->type == THREE_SERIES && (a->type != 411 && a->type != PAIRS_SERIES &&
			a->type != THREE_SERIES))
			return 1;
		break;
	case 7:
		break;
	case 8:
		if (c->type == 422 && (a->type != 422))
			return 1;
		else if (c->type == 3311 && (a->type != 422 && a->type != 3311))
			return 1;
		else if (c->type == PAIRS_SERIES && (a->type != 422 && a->type != 3311 &&
			a->type != PAIRS_SERIES))
			return 1;
		break;
	case 9:
		if (c->type == THREE_SERIES && (a->type != THREE_SERIES))
			return 1;
		break;
	case 10:
		if (c->type == 3322 && (a->type != 3322))
			return 1;
		else if (c->type == PAIRS_SERIES && (a->type != 3322 && a->type != PAIRS_SERIES))
			return 1;
		break;
	case 11:
		break;
	case 12:
		if (c->type == THREE_SERIES && (a->type != THREE_SERIES))
			return 1;
		else if (c->type == 333111 && (a->type != 333111 && a->type != THREE_SERIES))
			return 1;
		else if (c->type == PAIRS_SERIES && (a->type != PAIRS_SERIES &&a->type != 333111 && a->type != THREE_SERIES))
			return 1;
	case 13:
		break;
	case 15:
		if (c->type == THREE_SERIES && (a->type != THREE_SERIES))
			return 1;
		else if (c->type == 333222 && (a->type != 333222 && a->type != THREE_SERIES))
			return 1;
		break;
	case 16:
		break; //33331111
	case 20:
		if (c->type == 33332222 && (a->type != 33332222))
			return 1;
		break;
	default:
		break;
	}
	return 0;
}


int check_combo_with_rules(POKERS *h, COMBO_OF_POKERS * out, int first, COMBO_OF_POKERS * pre)
{
	COMBO_OF_POKERS c1, *c = &c1;
	if (first)
		out->type = NOTHING_old;
	else
		*out = *pre;


	if (HUN_NUMBER(h) == h->total || h->begin == h->end)
	{
		// if(first)
		{
			out->low = h->begin;
			switch (h->total)
			{
			case 1:
				out->type = SINGLE;
				break;
			case 2:
				out->type = PAIRS;
				break;
			case 3:
				out->type = THREE;
				break;
			case 4:
				out->type = BOMB_old;
				out->three_desc[0] = h->begin == h->hun ? 2 : 1;

				break;
			}
			if (first)
				return 0;
			else if (check_combo_a_Big_than_b(out, pre))
				return 0;
			else
				return -2;
		}
	}

	if (HUN_NUMBER(h)>0)
	{
		int ben_hun = 0;
		//set first hun values  
		for (int i1 = P3; i1 <= P2; i1++)
		{
			SET_HUN_TO_i(h, i1);
			PRINTF(VERBOSE, "==set first hun to %c\n", poker_to_char(i1));
			if (IS_HUN(h, i1)) ben_hun++;
			if (HUN_NUMBER(h)>ben_hun)
			{
				//set second hun values  
				for (int i2 = P3; i2 <= P2; i2++)
				{
					SET_HUN_TO_i(h, i2);
					PRINTF(VERBOSE, "\t==set second hun to %c\n", poker_to_char(i2));
					if (IS_HUN(h, i2)) ben_hun++;
					if (HUN_NUMBER(h)>ben_hun)
					{
						//set 3rd hun values  
						for (int i3 = P3; i3 <= P2; i3++)
						{
							SET_HUN_TO_i(h, i3);
							PRINTF(VERBOSE, "\t==set 3rd hun to %c\n", poker_to_char(i3));
							if (IS_HUN(h, i3)) ben_hun++;
							if (HUN_NUMBER(h)>ben_hun)
							{
								//set 4th hun values  
								for (int i4 = P3; i4 <= P2; i4++)
								{
									SET_HUN_TO_i(h, i4);
									if (is_combo(h, c))
									{
										if (first)
										{
											if (check_prior_first(c, out))
												*out = *c;
										}
										else
										{
											if (check_combo_a_Big_than_b(c, out))
												*out = *c;
										}
									}
									SET_i_to_HUN(h, i4);
								}
							}
							else  if (is_combo(h, c))
							{
								if (first)
								{
									if (check_prior_first(c, out))
										*out = *c;
								}
								else
								{
									if (check_combo_a_Big_than_b(c, out))
										*out = *c;
								}
							}
							if (IS_HUN(h, i3)) ben_hun--;
							SET_i_to_HUN(h, i3);
						}
					}
					else   if (is_combo(h, c))
					{
						if (first)
						{
							if (check_prior_first(c, out))
								*out = *c;
						}
						else
						{
							if (check_combo_a_Big_than_b(c, out))
								*out = *c;
						}
					}
					if (IS_HUN(h, i2)) ben_hun--;
					SET_i_to_HUN(h, i2);
				}
			}
			else  if (is_combo(h, c)) {
				if (first)
				{
					if (check_prior_first(c, out))
						*out = *c;
				}
				else
				{
					if (check_combo_a_Big_than_b(c, out))
						*out = *c;
				}
			}
			if (IS_HUN(h, i1)) ben_hun--;
			SET_i_to_HUN(h, i1);
		}
	}
	else if (is_combo(h, c))
	{
		if (first)
		{
			return 0;
		}
		else
		{
			if (check_combo_a_Big_than_b(c, pre)) {
				*out = *c;
				return 0;
			}
			else
				return -2;
		}
	}
	else
		return -1;

	//final check
	if (first)
	{
		if (out->type != NOTHING_old)
			return 0;
		else
			return -1;
	}
	else
	{
		if (check_combo_a_Big_than_b(out, pre))
			return 0;
		else
			return -2;
	}
}



int is_combo_hun(POKERS *h, COMBO_OF_POKERS * c)
{
	if (HUN_NUMBER(h)>0)
	{
		int ben_hun = 0;
		//set first hun values  
		for (int i1 = P3; i1 <= P2; i1++)
		{
			SET_HUN_TO_i(h, i1);
			PRINTF(VERBOSE, "==set first hun to %c\n", poker_to_char(i1));
			if (IS_HUN(h, i1)) ben_hun++;
			if (HUN_NUMBER(h)>ben_hun)
			{
				//set second hun values  
				for (int i2 = P3; i2 <= P2; i2++)
				{
					SET_HUN_TO_i(h, i2);
					PRINTF(VERBOSE, "\t==set second hun to %c\n", poker_to_char(i2));
					if (IS_HUN(h, i2)) ben_hun++;
					if (HUN_NUMBER(h)>ben_hun)
					{
						//set 3rd hun values  
						for (int i3 = P3; i3 <= P2; i3++)
						{
							SET_HUN_TO_i(h, i3);
							PRINTF(VERBOSE, "\t==set 3rd hun to %c\n", poker_to_char(i3));
							if (IS_HUN(h, i3)) ben_hun++;
							if (HUN_NUMBER(h)>ben_hun)
							{
								//set 4th hun values  
								for (int i4 = P3; i4 <= P2; i4++)
								{
									SET_HUN_TO_i(h, i4);
									if (is_combo(h, c)) return 1;
									SET_i_to_HUN(h, i4);
								}
							}
							else  if (is_combo(h, c)) return 1;
							if (IS_HUN(h, i3)) ben_hun--;
							SET_i_to_HUN(h, i3);
						}
					}
					else   if (is_combo(h, c)) return 1;
					if (IS_HUN(h, i2)) ben_hun--;
					SET_i_to_HUN(h, i2);
				}
			}
			else  if (is_combo(h, c)) return 1;
			if (IS_HUN(h, i1)) ben_hun--;
			SET_i_to_HUN(h, i1);
		}
	}
	else {
		return is_combo(h, c);
	}

	return 0;
}


int is_input_combo(int * p, COMBO_OF_POKERS* c)
{
	POKERS a;
	int num = read_poker_int(p, &a);

	return  is_combo(&a, c) ? num : 0;
}

bool getBomb(POKERS* h, COMBO_OF_POKERS * p) //
{
	int total = h->total;
	p->len = 1;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] >= 4) //bomb got
		{
			p->type = BOMB_old;
			p->three_desc[0] = 0;
			p->low = i;
			return true;
		}
	}

	if (h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
	{
		p->type = ROCKET;
		p->low = LIT_JOKER;
		return true;
	}
	return false;
}


bool getThree(POKERS* h, COMBO_OF_POKERS * p) //
{
	int total = h->total;
	for (int i = h->begin; i <= h->end; i++)
	{
		if (h->hands[i] == 3) //Got Three
		{
			p->type = THREE;
			p->low = i;
			return true;
		}
	}
	return false;
}



bool getThreeSeries(POKERS* h, COMBO_OF_POKERS * p)
{
	for (int i = h->begin; i <= (CEIL_A(h->end) - 1);)
	{
		int j;
		for (j = i; j <= CEIL_A(h->end); j++) {
			if (h->hands[j]<3) {
				break;
			}
		}
		if (j > i + 1)
		{
			p->type = THREE_SERIES;
			p->low = i;
			p->len = j - i;
			return true;
		}

		i = j + 1;

	}
	return false;
}

bool getDoubleSeries(POKERS* h, COMBO_OF_POKERS * p)
{
	for (int i = h->begin; i <= (CEIL_A(h->end) - 2);)
	{
		int j;
		for (j = i; j <= CEIL_A(h->end); j++) {
			if (h->hands[j]<2) {
				break;
			}
		}
		if (j >i + 2)
		{
			p->type = PAIRS_SERIES;
			p->low = i;
			p->len = j - i;
			return true;
		}
		i = j + 1;
	}
	return false;
}

//双顺调优
bool updateDoubleSeries(POKERS* h, COMBO_OF_POKERS * p)
{
	//from head to tail.
	while (p->len >3)
	{
		int end = p->len + p->low - 1;
		if (h->hands[end] > h->hands[p->low])
		{   //remove tail
			h->hands[end] += 2;
			p->len--;
			//end--;
		}
		else if (h->hands[p->low] >0)
		{
			h->hands[p->low] += 2;
			p->low++;
			p->len--;
		}
		else
			break;
	}
	return false;
}

bool updateDoubleSeries1(POKERS* h, COMBO_OF_POKERS * p)
{
	if (p->len == 3)
	{
		if (h->hands[p->low] == 1 && h->hands[p->low + 2] == 1) // cl
			return true;
	}
	return false;
}

bool getSeries(POKERS* h, COMBO_OF_POKERS * p)
{
	for (int i = h->begin; i <= (h->end - 4);)
	{
		int j;
		for (j = i; j <= CEIL_A(h->end); j++) {
			if (h->hands[j]<1) {
				break;
			}
		}
		if (j > i + 4)
		{
			p->type = SINGLE_SERIES;
			p->low = i;
			p->len = j - i;
			//          p->number = len;
			return true;
		}

		i = j + 1;
	}
	return false;
}

bool getBigBomb(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS * a) //
{
	int total = h->total;
	for (int i = a->low + 1; i <= h->end; i++)
	{
		if (h->hands[i] == 4) //bomb got
		{
			p->type = BOMB_old;
			p->three_desc[0] = 0;
			p->low = i;
			return true;
		}
	}
	if (a->low<LIT_JOKER && h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
	{
		p->type = ROCKET;
		p->low = LIT_JOKER;
		return true;
	}
	return false;
}

bool getBigBomb_hun(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS * a) //
{
	switch (a->three_desc[0])
	{
	default:
	case 0:
		if (HUN_NUMBER(h)) {
			for (int i = a->low + 1; i <= min_ddz(h->end, P2); i++)
			{
				if (IS_HUN(h, i)) continue;
				if (h->hands[i] <4 && h->hands[i]>0 &&
					h->hands[i] >= (4 - HUN_NUMBER(h))) //got hun bomb
				{
					p->type = BOMB_old;
					p->three_desc[0] = 0;
					p->low = i;
					return true;
				}
			}
		}
	case 1:
		for (int i = a->three_desc[0] == 1 ? a->low + 1 : 0; i <= h->end; i++)
		{
			if (IS_HUN(h, i)) //skip hun at first
				continue;
			if (h->hands[i] == 4) //got hun bomb
			{
				p->type = BOMB_old;
				p->three_desc[0] = 1;
				p->low = i;
				return true;
			}
		}
		if (h->hands[h->hun] == 4) //got pure hun bomb
		{
			p->type = BOMB_old;
			p->three_desc[0] = 2;
			p->low = h->hun;
			return true;
		}
	case 2:
		if (a->low<LIT_JOKER && h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
		{
			p->type = ROCKET;
			p->low = LIT_JOKER;
			return true;
		}
	}
	return false;
}

bool getBomb_hun(POKERS* h, COMBO_OF_POKERS * p) //
{
	switch (0)
	{
	default:
	case 0:
		if (HUN_NUMBER(h)) {
			for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
			{
				if (IS_HUN(h, i)) //skip hun at first
					continue;
				if (h->hands[i] <4 && h->hands[i] >0 &&
					h->hands[i] >= (4 - HUN_NUMBER(h))) //got hun bomb
				{
					p->type = BOMB_old;
					p->three_desc[0] = 0;
					p->low = i;
					return true;
				}
			}
		}
	case 1:
		for (int i = 0; i <= h->end; i++)
		{
			if (IS_HUN(h, i)) //skip hun at first
				continue;
			if (h->hands[i] == 4) //got hun bomb
			{
				p->type = BOMB_old;
				p->three_desc[0] = 1;
				p->low = i;
				return true;
			}
		}
		if (h->hands[h->hun] == 4) //got pure hun bomb
		{
			p->type = BOMB_old;
			p->three_desc[0] = 2;
			p->low = h->hun;
			return true;
		}
	case 2:
		if (h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
		{
			p->type = ROCKET;
			p->low = LIT_JOKER;
			return true;
		}
	}
	return false;
}

int get_bomb_numbers_hun(POKERS* t)
{
	POKERS tmp = *t, *h = &tmp;
	int number = 0;

	for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
	{
		if (IS_HUN(h, i)) //skip hun at first
			continue;
		if (h->hands[i] == 4) //got hun bomb
		{
			h->hands[i] = 0;
			number++;
		}
	}

	if (h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
	{
		number++;
	}

	if (HUN_NUMBER(h))
	{
		for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
		{
			if (IS_HUN(h, i)) //skip hun at first
				continue;
			if (h->hands[i] == 3 && HUN_NUMBER(h)) //got hun bomb
			{
				h->hands[i] = 0;
				h->hands[h->hun] --;
				number++;
			}
		}
	}

	if (HUN_NUMBER(h) >= 2)
	{
		for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
		{
			if (IS_HUN(h, i)) //skip hun at first
				continue;
			if (h->hands[i] == 2 && HUN_NUMBER(h) >= 2) //got hun bomb
			{
				h->hands[i] = 0;
				h->hands[h->hun] -= 2;
				number++;
			}
		}
	}

	if (HUN_NUMBER(h) >= 3)
	{
		for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
		{
			if (IS_HUN(h, i)) //skip hun at first
				continue;
			if (h->hands[i] == 1 && HUN_NUMBER(h) >= 3) //got hun bomb
			{
				number++;
				h->hands[h->hun] -= 3;
				break;
			}
		}
	}
	if (HUN_NUMBER(h)>3)
		number++;

	return number;
}

int get_bomb_numbers(POKERS* t)
{
	POKERS tmp = *t, *h = &tmp;
	int number = 0;

	for (int i = h->begin; i <= min_ddz(h->end, P2); i++)
	{
		if (h->hands[i] == 4) //got hun bomb
		{
			number++;
		}
	}

	if (h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1)
	{
		number++;
	}
	return number;
}

//fixme: ?? for single??
bool getBigSingle_hun(POKERS* h, COMBO_OF_POKERS * p, int start, int end, int number)
{
	for (int i = start; i <= end; i++)
	{
		if (IS_HUN(h, i)) //skip hun at first
			continue;
		if (h->hands[i] >= number
			|| (h->hands[i] >= number - HUN_NUMBER(h) && h->hands[i] >0)
			)
		{
			p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
			p->low = i;
			p->len = 1;
			return true;
		}
	}

	if (HUN_NUMBER(h) >= number && h->hun > start)
	{
		p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
		p->low = h->hun;
		p->len = 1;
		return true;
	}

	return false;
}

bool getBigThree(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS* a) //
{
	for (int i = a->low + 1; i <= h->end; i++)
	{
		if (h->hands[i] == 3) //Got Three,doesn't check bomb..
		{
			p->type = THREE;
			p->low = i;
			p->len = 1;
			return true;
		}
	}
	return false;
}

bool getBigSingle(POKERS* h, COMBO_OF_POKERS * p, int start, int end, int number)
{
	for (int i = start; i <= end; i++)
	{
		if (h->hands[i] >= number) //Got Three
		{
			p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
			p->low = i;
			p->len = 1;
			return true;
		}
	}
	return false;
}

bool getBigSeries_hun(POKERS* h, COMBO_OF_POKERS * p, int start, int end, int number, int len) //
{
	end = CEIL_A(end);
	POKERS t = { 0 };
	for (int j = start; j <= start + len - 1; j++) {
		t.hands[j] = number;
	}
	for (int i = start; i <= (end - len + 1);)
	{
		if (is_subset_poker_hun(&t, h))
		{
			p->type = number == 3 ? THREE_SERIES : (number == 2 ? PAIRS_SERIES : SINGLE_SERIES);
			p->low = i;
			p->len = len;
			return true;
		}
		t.hands[i] = 0;
		t.hands[i + len] = number;
		i++;
	}
	return false;
}

bool getBigSeries(POKERS* h, COMBO_OF_POKERS * p, int start, int end, int number, int len) //
{
	end = CEIL_A(end);
	for (int i = start; i <= (end - len + 1);)
	{
		int j;
		for (j = i; j <= end; j++) {
#if 1
			if (number == 3 && h->hands[j] != 3) break; //don't get bomb
#endif
			if (h->hands[j]<number) break;
		}
		if (j >= i + len)
		{
			p->type = number == 3 ? THREE_SERIES : (number == 2 ? PAIRS_SERIES : SINGLE_SERIES);
			p->low = i;
			p->len = len;
			//          p->number = p->len * number;
			return true;
		}
		i = j + 1;
	}
	return false;
}

bool getSingleSeries(POKERS* h, COMBO_OF_POKERS * p, int start, int end, int number) //
{
	end = CEIL_A(end);
	for (int i = start; i <= (end - 4);)
	{
		int j;
		for (j = i; j <= end; j++) {
			if (h->hands[j]<1) break;
			else if (j - i >= 4 && h->hands[j]>1)
			{
				j++;
				break;
			}
		}
		if (j >= i + 5)
		{
			p->type = SINGLE_SERIES;
			p->low = i;
			p->len = j - i;
			//          if( p->len > 5 )
			//          p->number = p->len * number;
			return true;
		}
		i = j + 1;
	}
	return false;
}


//only used for known_other_pokers

bool getSmallSingle(POKERS* h, COMBO_OF_POKERS * p, int end, int number)
{
	for (int i = h->begin; i< end; i++)
	{
		if (h->hands[i] >= number && h->hands[i] != 4)
		{
			p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
			p->low = i;
			p->len = 1;
			return true;
		}
	}
	return false;
}

bool getSmallSingle_hun(POKERS* h, COMBO_OF_POKERS * p, int end, int number)
{
	for (int i = h->begin; i< end; i++)
	{
		if (IS_HUN(h, i))
			continue;
		if (h->hands[i] >= number)
		{
			p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
			p->low = i;
			p->len = 1;
			return true;
		}
	}
	//check with hun
	for (int i = h->begin; i< end; i++)
	{
		if (IS_HUN(h, i))
			continue;
		if (h->hands[i] >= number - HUN_NUMBER(h))
		{
			p->type = number == 3 ? THREE : (number == 2 ? PAIRS : SINGLE);
			p->low = i;
			p->len = 1;
			return true;
		}
	}
	return false;
}


bool getSmallSeries(POKERS* h, COMBO_OF_POKERS * p, int end, int number, int len) //
{
	end = CEIL_A(end);
	//    for (int i = h->begin ; i<=  (end -len + 1);)
	for (int i = h->begin; i< end;)
	{
		int j;
		for (j = i; j< i + len; j++) {
			if (h->hands[j]<number) break;
		}
		if (j >= i + len)
		{
			p->type = number == 3 ? THREE_SERIES : (number == 2 ? PAIRS_SERIES : SINGLE_SERIES);
			p->low = i;
			p->len = len;
			//          p->number = p->len * number;
			return true;
		}
		i = j + 1;
	}
	return false;
}

bool getSmallSeries_hun(POKERS* h, COMBO_OF_POKERS * p, int end, int number, int len) //
{
	end = CEIL_A(end);
	POKERS t = { 0 };
	for (int j = 0; j <= len - 1; j++) {
		t.hands[j] = number;
	}
	for (int i = 0; i <= end - 1;)
	{
		if (is_subset_poker_hun(&t, h))
		{
			p->type = number == 3 ? THREE_SERIES : (number == 2 ? PAIRS_SERIES : SINGLE_SERIES);
			p->low = i;
			p->len = len;
			return true;
		}
		t.hands[i] = 0;
		t.hands[i + len] = number;
		i++;
	}
	return false;
}


int get_opp_bomb_number(GAME_DDZ_AI* g)
{
	if (g->known_others_poker)
	{
		int(*func)(POKERS*);
		if (g->hun == -1) {
			func = get_bomb_numbers;
		}
		else {
			func = get_bomb_numbers_hun;
		}

		COMBO_OF_POKERS *c, tmp;
		c = &tmp;
		if (g->players[CUR_PLAYER]->id == LORD_old)
		{
			return func(g->players[DOWN_PLAYER]->h) + func(g->players[UP_PLAYER]->h);
		}
		else if (g->players[CUR_PLAYER]->id == UPFARMER_old)
		{
			return func(g->players[DOWN_PLAYER]->h);
		}
		else
		{
			return func(g->players[UP_PLAYER]->h);
		}
	}
	else {
		return 0;
	}
}
int opp_hasbomb(GAME_DDZ_AI * g)
{
	if (g->known_others_poker)
	{
		bool(*func)(POKERS*, COMBO_OF_POKERS *);
		if (g->hun == -1)
			func = getBomb;
		else
			func = getBomb_hun;

		COMBO_OF_POKERS *c, tmp;
		c = &tmp;
		if (g->players[CUR_PLAYER]->id == LORD_old)
		{
			if (func(g->players[DOWN_PLAYER]->h, c)
				|| func(g->players[UP_PLAYER]->h, c))
			{
				return true;
			}
			else
				return false;
		}
		else    if (g->players[CUR_PLAYER]->id == UPFARMER_old)
		{
			if (!func(g->players[DOWN_PLAYER]->h, c))
			{
				return false;
			}
			else
				return true;
		}
		else
		{
			if (!func(g->players[UP_PLAYER]->h, c))
			{
				return false;
			}
			else
				return true;
		}
	}
	else
	{
		COMBO_OF_POKERS *c, tmp;
		c = &tmp;
		PLAYER *pl = g->players[CUR_PLAYER];
		if (getBomb(g->players[CUR_PLAYER]->opps, c))
		{
			//double check
			int check_num = 4;
			if (pl->opps->hands[BIG_JOKER] == 1 &&
				pl->opps->hands[LIT_JOKER] == 1)
			{
				check_num = 2;
			}

			if (g->players[CUR_PLAYER]->id == LORD_old &&
				pl->oppDown_num<check_num && pl->oppUp_num <check_num)
			{
				return false;
			}
			if (pl->id == UPFARMER_old && pl->oppDown_num<check_num)
			{
				return false;
			}
			if (pl->id == DOWNFARMER_old && pl->oppUp_num<check_num)
			{
				return false;
			}

			return true;
		}
		else
			return false;
	}
	return false;
}

bool  is_combo_biggest_sure(GAME_DDZ_AI * g, COMBO_OF_POKERS * a)
{
	COMBO_OF_POKERS tmp, *c = &tmp;
	PLAYER* pl = g->players[CUR_PLAYER];
	if (g->known_others_poker)
	{
		if (g->players[CUR_PLAYER]->id == LORD_old)
		{
			if (find_a_bigger_combo_in_hands(g->players[DOWN_PLAYER]->h, c, a)
				|| find_a_bigger_combo_in_hands(g->players[UP_PLAYER]->h, c, a))
			{
				return false;
			}
			else
				return true;
		}
		else    if (g->players[CUR_PLAYER]->id == UPFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(g->players[DOWN_PLAYER]->h, c, a))
			{
				return false;
			}
			else
				return true;
		}
		else
		{
			if (find_a_bigger_combo_in_hands(g->players[UP_PLAYER]->h, c, a))
			{
				return false;
			}
			else
				return true;
		}
	}
	else
	{
		int num = get_combo_number(a);
		if (a->type != BOMB_old &&a->type != ROCKET && opp_hasbomb(g))
			return false;
		if (pl->id == LORD_old)
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a))
			{
				//check again
				if (pl->oppDown_num < num && pl->oppUp_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
		else    if (g->players[CUR_PLAYER]->id == UPFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a))
			{
				//check again
				if (pl->oppDown_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
		else
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a))
			{
				//check again
				if (pl->oppUp_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
	}
}

bool  is_combo_biggest_sure_without_bomb(GAME_DDZ_AI * g, COMBO_OF_POKERS * a)
{
	COMBO_OF_POKERS tmp, *c = &tmp;
	PLAYER* pl = g->players[CUR_PLAYER];
	GAME_DDZ_AI* game = g;
	if (g->known_others_poker)
	{
		COMBO_OF_POKERS tmp;
		if (game->player_type == LORD_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &tmp, a) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else if (find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &tmp, a) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;
		}
		else if (game->player_type == UPFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &tmp, a) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;
		}
		else if (game->player_type == DOWNFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &tmp, a) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;
		}
		else
			return false;
	}
	else
	{
		int num = get_combo_number(a);
		if (pl->id == LORD_old)
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a) && NOT_BOMB(c))
			{
				if (pl->oppDown_num < num && pl->oppUp_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
		else    if (g->players[CUR_PLAYER]->id == UPFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a) && NOT_BOMB(c))
			{
				if (pl->oppDown_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
		else
		{
			if (find_a_bigger_combo_in_hands(pl->opps, c, a) && NOT_BOMB(c))
			{
				if (pl->oppUp_num < num)
					return true;
				return false;
			}
			else
				return true;
		}
	}
}
//检查本手牌是否最大
//不考虑炸弹
bool is_combo_biggest(GAME_DDZ_AI* game, POKERS *opp, COMBO_OF_POKERS * c, int opp1_num, int opp2_num, int lower)
{
	bool res = true;
	static   char BigSNum[8] = { 7,9,11,13,15,17,18,18 };
	int num = get_combo_number(c);
	//check number
	opp1_num = game->players[CUR_PLAYER]->oppDown_num;
	opp2_num = game->players[CUR_PLAYER]->oppUp_num;
	if (game->player_type == UPFARMER_old)
		opp2_num = 0;
	if (game->player_type == DOWNFARMER_old)
		opp1_num = 0;
	if (opp1_num <num && opp2_num < num)
		return res;

	if (game->known_others_poker)
	{

		COMBO_OF_POKERS tmp;
		if (game->player_type == LORD_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &tmp, c) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else if (find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &tmp, c) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;

		}
		else if (game->player_type == UPFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &tmp, c) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;
		}
		else if (game->player_type == DOWNFARMER_old)
		{
			if (find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &tmp, c) && tmp.type != ROCKET && tmp.type != BOMB_old)
				return false;
			else
				return true;
		}
		else
			return false;
	}

	switch (c->type)
	{
	case ROCKET:
	case BOMB_old:
	case PAIRS_SERIES:
	case THREE_SERIES:
	default:
		break;

	case SINGLE_SERIES:
		if (opp1_num + opp2_num > BigSNum[c->len - 5])
		{
			int start = c->low + 1;
			int len = c->len;
			int end = min_ddz(Pa, opp->end);
			for (int i = start; i <= (end - len + 1);)
			{
				int j, sum = 0;
				for (j = i; j <= end; j++) {
					if (opp->hands[j]<1) break;
					//else  if(opp->hands[j] ==1)
					//  sum++;
				}
				if (j >= i + len)
				{
					int loop = j - i - len;
					for (int k = 0; k <= loop; k++)
					{
						sum = 0;
						for (int m = i + k; m<j; m++)
							if (opp->hands[m] == 1)
								sum++;
						if (sum<4)
							res = false;
					}
				}
				i = j + 1;
			}
		}
		break;

	case THREE_TWO:
	case THREE_ONE:
		if (c->len > 1) break;
	case THREE:
		for (int k = c->low + 1; k <= min_ddz(P2, opp->end); k++)
		{
			if (opp->hands[k] >= 3) {
				res = false;
				break;
			}
		}
		break;
	case PAIRS:
		for (int k = c->low + 1; k <= min_ddz(P2, opp->end); k++)
		{
			if (opp->hands[k] >= 2) {
				res = false;
				break;
			}
		}
		break;
	case SINGLE:
		for (int k = c->low + 1; k <= opp->end; k++)
		{
			if (opp->hands[k] >= 1) {
				res = false;
				break;
			}
		}
		break;
	}
	return res;
}




int get_lowest_controls(POKERS *h, int number)
{
	int high = h->end;
	if (h->total <= number)
		return h->begin;
	int sum = 0, j = 0, j1 = 0;
	int i;
	for (i = high; i>0 && number>0; i--)
	{
		number -= h->hands[i];
		if (number <= 0)
			return i;
	}
	return 0;
}


int calc_controls(POKERS *h, POKERS *opp, int number)
{
	int high = opp->end > h->end ? opp->end : h->end;
	int sum = 0, j = 0, j1 = 0, HighPokers1[9], HighPoker[9];
	for (int i = high; i>0 && (j + j1)< number; i--)
	{
		for (int k = 0; k<h->hands[i]; k++)
		{
			HighPoker[j++] = i;
		}
		for (int k = 0; k<opp->hands[i]; k++)
		{
			HighPokers1[j1++] = i;
		}
	}
	number = j + j1;
	j--;
	j1--;

#if 0
	DBG(
		PRINTF(LEVEL, "\ncur  high:  ");
	for (int k = j; k >= 0; k--)
		print_one_poker(HighPoker[k]);
	PRINTF(LEVEL, "\n opps  high: ");
	for (int k = j1; k >= 0; k--)
		print_one_poker(HighPokers1[k]);

	//PRINTF(LEVEL,"\n round:\n ");
	)
#endif
		//j;j1;
		int M = 0, N = 0;
	//int bak_j=j,bak_j1=bak_j1;
	int last;
	for (; j >= 0 && j1 >= 0;)
	{
		// simulate round
		last = HighPoker[j];
		j--;
		//DBG(    print_one_poker(last);  PRINTF(LEVEL,"\n");      )
	search_0:
		for (int k = j1; k >= 0; k--)
			if (HighPokers1[k]> last)
			{
				last = HighPokers1[k];
				// DBG(    print_one_poker(last);  PRINTF(LEVEL,"\n");      )
				for (int p = k; p<j1; p++) //remove poker
					HighPokers1[p] = HighPokers1[p + 1];
				j1--;
				goto search_1;
			}
		N++;
		continue;
	search_1:
		for (int k = j; k >= 0; k--)
			if (last < HighPoker[k])
			{
				last = HighPoker[k];
				// DBG(    print_one_poker(last);  PRINTF(LEVEL,"\n");      )
				for (int p = k; p<j; p++) //remove poker
					HighPoker[p] = HighPoker[p + 1];
				j--;
				goto search_0;
			}
		M++;
	}
	j = j1 >= 0 ? j : j + 1;
	if (j<0) j++;
	//    PRINTF(LEVEL,"N %d M %d j %d\n", N,M,j);
	return max_ddz(((N + j) * 10 + M - number / 2), 0); // 最小为0
}


//search functions
int browse_pokers(POKERS *h, COMBO_OF_POKERS * pCombos)
{
	int num = 0;
	for (int i = h->begin; i <= min_ddz(P2, h->end); i++)
	{
		if (h->hands[i] == 0)
			continue;
		else if (h->hands[i] == 1)
			pCombos->type = SINGLE;
		else if (h->hands[i] == 2)
			pCombos->type = PAIRS;
		else if (h->hands[i] == 3)
			pCombos->type = THREE;
		else if (h->hands[i] >= 4)
			pCombos->type = BOMB_old;

		pCombos->low = i;
		pCombos->len = 1;
		pCombos->three_desc[0] = 0;
		//        pCombos->number = h->hands[i];
		pCombos++;
		num++;
		if (h->hands[i] > 4)
		{
			h->hands[i] -= 4;
			i--;
		}
	}
	//if ( h->end >= LIT_JOKER)
	//{
	if (h->hands[LIT_JOKER] == 1 && h->hands[BIG_JOKER] == 1) {
		pCombos->type = ROCKET;
		pCombos->low = LIT_JOKER;
		pCombos->len = 1;
		pCombos++;
		num++;
	}
	else if (h->hands[LIT_JOKER] == 1) {
		pCombos->type = SINGLE;
		pCombos->low = LIT_JOKER;
		pCombos->len = 1;
		pCombos++;
		num++;
	}
	else if (h->hands[BIG_JOKER] == 1) {
		pCombos->type = SINGLE;
		pCombos->len = 1;
		pCombos->low = BIG_JOKER;
		pCombos++;
		num++;
	}

	return num;
}
int search333inSingleSeries(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS * s)
{
    int len = p->len - 5;
    int res = 0;
    if (len >= 2)
    {
        // 顺子头部，从大到小找顺子
        if (h->hands[p->low + len - 1] >= 2 && h->hands[p->low + len - 2] >= 2)
        {
            s->type = THREE_SERIES;
            s->len = 2;
            s->low = p->low + len - 2;
            p->len -= len;
            h->hands[p->low + len - 1] -= 2;
            h->hands[p->low + len - 2] -= 2;
            for (int i = p->low; i < s->low; ++i)
            {
                h->hands[i]++;
            }
            p->low += len;

            //found a three series
            while (s->low - 1 >= P3 && h->hands[s->low - 1] >= 3)
            {
                h->hands[s->low - 1] -= 3;
                s->len++;
                s->low--;
            }

            // extend solo chain
            while (p->low - 1 >= P3 && h->hands[p->low - 1] >= 1)
            {
                h->hands[p->low - 1]--;
                p->len++;
                p->low--;
            }
            
            return 1;
        }
        // 顺子尾部，从小到大找飞机
        else if (h->hands[p->low + p->len - len] >= 2 && h->hands[p->low + p->len - len + 1] >= 2)
        {
            s->type = THREE_SERIES;
            s->len = 2;
            s->low = p->low + p->len - len;

            h->hands[s->low] -= 2;
            h->hands[s->low + 1] -= 2;
            for (int i = s->low + s->len; i < p->low + p->len; ++i)
            {
                h->hands[i]++;
            }
            p->len -= len;

            //found a three series
            while (s->low + s->len <= Pa && h->hands[s->low + s->len] >= 3)
            {                
                h->hands[s->low + s->len] -= 3;
                s->len++;
            }
            // extend solo chain
            while (p->low + p->len <= Pa && h->hands[p->low + p->len] >= 1)
            {
                h->hands[p->low + p->len] --;
                p->len++;
            }
            
            return 1;
        }
        return 0;
    }
    return res;
}

int search222inSingleSeries(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS * s)
{
	int len = p->len - 5;
	int res = 0;
	if (len >= 3)
	{
		if (h->hands[p->low + 1] >= 1 && h->hands[p->low + 2] >= 1 && h->hands[p->low] >= 1)
		{
			s->type = PAIRS_SERIES;
			s->len = 3;
			s->low = p->low;
			p->len -= 3;
			h->hands[p->low]--;
			h->hands[p->low + 1]--;
			h->hands[p->low + 2]--;
			p->low += 3;

			//found a double series
            
			if (len >= 4 && h->hands[p->low] >= 1)
			{
                while (h->hands[p->low] >= 1 && p->len > 5)
                {
                    s->len++;
                    p->len--;

                    h->hands[p->low]--;
                    p->low++;
                }
                
				//s->len++;
				//p->len--;

				//h->hands[p->low]--;
				//p->low++;
                while (p->low - 1 >= P3 && h->hands[p->low - 1]>0) {
					h->hands[p->low - 1]--;
					p->low--;
					p->len++;
				}
                return 1;
			}
			else if (h->hands[p->low - 1]>0) {
                while (p->low - 1 >= P3 && h->hands[p->low - 1]>0)
                {
                    h->hands[p->low - 1]--;
                    p->low--;
                    p->len++;
                }
				//h->hands[p->low - 1]--;
				//p->low--;
				//p->len++;
				return 1;
			}

			return 1;
		}
		else if (h->hands[p->low + p->len - 1] >= 1 && h->hands[p->low + p->len - 2] >= 1 && h->hands[p->low + p->len - 3] >= 1)
		{
			s->type = PAIRS_SERIES;
			s->len = 3;
			s->low = p->low + p->len - 3;

			h->hands[p->low + p->len - 1]--;
			h->hands[p->low + p->len - 1 - 1]--;
			h->hands[p->low + p->len - 1 - 2]--;
			p->len -= 3;
			//found a double series
			if (len >4 && h->hands[p->low + p->len - 1] >= 1)
			{
                while (p->len > 5 && p->low + p->len - 1 >= P3 && h->hands[p->low + p->len - 1] >= 1)
                {
                    s->len++;
                    s->low--;
                    h->hands[p->low + p->len - 1]--;
                    p->len--;
                }
				//s->len++;
				//s->low--;
				//h->hands[p->low + p->len - 1]--;
				//p->len--;
                while (p->low + p->len <= Pa && h->hands[p->low + p->len]>0) {
					h->hands[p->low + p->len]--;
					p->len++;
				}
                return 1;
			}
			else if (h->hands[p->low + p->len]>0) {
                while (p->low + p->len >= Pa && h->hands[p->low + p->len] > 0)
                {
                    h->hands[p->low + p->len]--;
                    p->len++;
                }				
				return 1;
			}

			return 1;
		}

	}
	return res;
}

int  search234inSingleSeries(POKERS* h, COMBO_OF_POKERS * p, COMBO_OF_POKERS * s) //tobe optimized
{
	//from head to tail.
	while (p->len >5)
	{
		int end = p->len + p->low - 1;
		if (h->hands[end] > h->hands[p->low])
		{   //remove tail
			h->hands[end]++;
			p->len--;
			//end--;
		}
		else if (h->hands[p->low] >0)
		{
			h->hands[p->low]++;
			p->low++;
			p->len--;
		}
		else
			break;
	}
	//check_five_series:
	//todo:
	return 0;
}


int  searchMultiSingleSeries(POKERS* h, COMBO_OF_POKERS * p) //tobe optimized
{
	//    POKERS t,*hbak;
	//    memmove(&t,h,sizefo(POKERS));
	//    h=&t;
	COMBO_OF_POKERS * first = p;
	int num = 0;

	// got all 5-series
	for (int i = h->begin; i <= (CEIL_A(h->end) - 4); i++)
	{
		//        if (getBigSeries(h, p, i ,  CEIL_A(h->end), 1, 5)) //could be optimized
		if (getSingleSeries(h, p, i, CEIL_A(h->end), 1)) //could be optimized
		{
			remove_combo_poker(h, p, NULL);
			p++;
			num++;
		}
	}
	p = first;
	for (int i = 0; i<num; i++) {
		{
			while (p->len + p->low <= Pa && h->hands[p->len + p->low])
			{
				h->hands[p->len + p->low]--;
				p->len++;
			}
			p++;
		}
	}

	p = first;
	int num_bak = num;
	for (int i = 0; i<num; i++) {

		if (i<num - 1 && p->low + p->len == (p + 1)->low) //connect
		{
			num--;
			p->len += (p + 1)->len;
			i++;
			if (num > i)
				memmove(p + 1, p + 2, sizeof(COMBO_OF_POKERS)*(num - i));
		}
		else  if (i<num - 2 && p->low + p->len == (p + 2)->low) //connect
		{

			p->len += (p + 2)->len;
			if (num == 4)
				memmove(p + 2, p + 3, sizeof(COMBO_OF_POKERS)*(1));
			num--;
		}
		else if (i<num - 3 && p->low + p->len == (p + 3)->low) //connect
		{

			p->len += (p + 3)->len;
			//              if ( num ==3 )
			//                memmove(p+2 ,p+3,sizeof(COMBO_OF_POKERS)*(num-2));
			num--;
		}
		p++;
	}

	for (int i = num; i<num_bak; i++)
	{
		first[i].type = NOTHING_old;
	}
	return num;
}

int connectSingleSeries(COMBO_OF_POKERS * first_combo, int combo_nums)
{
    int origin_combo_nums = combo_nums;
    for (int i = 0; i < combo_nums; ++i)
    {
        if (first_combo[i].type != SINGLE_SERIES)
        {
            continue;
        }
        for (int j = i + 1; j < combo_nums; ++j)
        {
            if (first_combo[j].type != SINGLE_SERIES)
            {
                continue;
            }

            if (first_combo[i].low + first_combo[i].len == first_combo[j].low) // connect single chain
            {
                first_combo[i].len += first_combo[j].len;
                memmove(first_combo + j, first_combo + j + 1, sizeof(COMBO_OF_POKERS) * (combo_nums - j));
                first_combo[combo_nums - 1].type = NOTHING_old;
                combo_nums--;
                break;
            }
        }
    }
    return origin_combo_nums - combo_nums; // 返回被合并的顺子数
}

int search_general_1(POKERS* h, COMBO_OF_POKERS * pCombos,
	bool skip_bomb, bool skip_double, bool skip_three, bool skip_series)
{
	int combo_nums = 0;
	int num = 0;
	if (!skip_series) {
		num = searchMultiSingleSeries(h, pCombos);
		combo_nums += num;
		pCombos += num;
		COMBO_OF_POKERS * tmp = pCombos - num;
		for (int k = 0; k<num; k++)
		{
            int r = search333inSingleSeries(h, tmp + k, pCombos);
            if (r == 1)
            {
                pCombos++;
                combo_nums++;
            }

			int res = search222inSingleSeries(h, tmp + k, pCombos);
			if (res == 1) {
                pCombos++;
                combo_nums++;
			}
			else
				search234inSingleSeries(h, tmp + k, pCombos);
			//     remove_combo_poker(h,pCombos,NULL);
		}

        // at last, connect single series, example: 34567 + 89TJQK
        int connect_single_series_nums = connectSingleSeries(tmp, combo_nums);
        combo_nums -= connect_single_series_nums;
        pCombos -= connect_single_series_nums;
	}
	if (!skip_bomb)
	{
		while (getBomb(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
	}
	if (!skip_three)
	{
		while (getThreeSeries(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
		//while (getThree(h, pCombos))
		//{
		//	remove_combo_poker(h, pCombos, NULL);
		//	combo_nums++;
		//	pCombos++;
		//}
	}

	if (!skip_double)
	{
		while (getDoubleSeries(h, pCombos)) //todo: check three in doubles.
		{
			remove_combo_poker(h, pCombos, NULL);
			updateDoubleSeries(h,pCombos);
			combo_nums++;
			pCombos++;
		}
	}

	/*      while( getThree(h,pCombos))
	{
	remove_combo_poker(h,pCombos,NULL);
	combo_nums++;
	pCombos++;
	}
	*/

	//todo search for three's in series head and tail.
	num = browse_pokers(h, pCombos);	

    for (int i = 0; i < num; ++i)
    {
        if (pCombos[i].type == PAIRS && pCombos[i].low < P2)
        {
            for (int j = 0; j < combo_nums; ++j)
            {
                if (pCombos[-(j + 1)].type == PAIRS_SERIES)
                {
                    //判断前后能否加上
                    if (pCombos[-(j + 1)].low == pCombos[i].low + 1)
                    {
                        pCombos[-(j + 1)].low--;
                        pCombos[-(j + 1)].len++;
                        
                        // 只需要置为nothing，会在sort_all_combos里重新规整，memmove会有问题
                        // num也不需要减1，中间可能有type为nothing的牌组，需要涵盖到最后的一个牌组
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i+1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                    else if (pCombos[-(j + 1)].low + pCombos[-(j + 1)].len == pCombos[i].low)
                    {
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                }
            }
        }
    }

    pCombos += num;
    combo_nums += num;
	//  print_all_combos(pCombos-combo_nums,combo_nums);

	return combo_nums;
}

int search_general_2(POKERS* h, COMBO_OF_POKERS * pCombos,
	bool skip_bomb, bool skip_double, bool skip_three, bool skip_series)
{
	int combo_nums = 0;
	int num;
	if (!skip_bomb)
	{
		while (getBomb(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
	}

	if (!skip_three)
	{
		while (getThreeSeries(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
		//while (getThree(h, pCombos))
		//{
		//	remove_combo_poker(h, pCombos, NULL);
		//	combo_nums++;
		//	pCombos++;
		//}
	}

	if (!skip_series)
	{
		num = searchMultiSingleSeries(h, pCombos);
		combo_nums += num;
		pCombos += num;
		COMBO_OF_POKERS * tmp = pCombos - num;
		for (int k = 0; k<num; k++)
		{
			int res = search222inSingleSeries(h, tmp + k, pCombos);
			if (res == 1) {
				pCombos++;
				combo_nums++;
			}
			else
				search234inSingleSeries(h, tmp + k, pCombos);
			//     remove_combo_poker(h,pCombos,NULL);
		}

        // at last, connect single series, example: 34567 + 89TJQK
        int connect_single_series_nums = connectSingleSeries(tmp, combo_nums);
        combo_nums -= connect_single_series_nums;
        pCombos -= connect_single_series_nums;
	}

	if (!skip_double)
	{
		//may be removed.
		while (getDoubleSeries(h, pCombos)) //todo: check three in doubles.
		{
			remove_combo_poker(h, pCombos, NULL);
			updateDoubleSeries(h,pCombos);
			combo_nums++;
			pCombos++;
		}
	}
	//todo search for three's in series head and tail.
	num = browse_pokers(h, pCombos);

    for (int i = 0; i < num; ++i)
    {
        if (pCombos[i].type == PAIRS && pCombos[i].low < P2)
        {
            for (int j = 0; j < combo_nums; ++j)
            {
                if (pCombos[-(j + 1)].type == PAIRS_SERIES)
                {
                    //判断前后能否加上
                    if (pCombos[-(j + 1)].low == pCombos[i].low + 1)
                    {
                        pCombos[-(j + 1)].low--;
                        pCombos[-(j + 1)].len++;

                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                    else if (pCombos[-(j + 1)].low + pCombos[-(j + 1)].len == pCombos[i].low)
                    {
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                }
            }
        }
    }

	pCombos += num;
	combo_nums += num;


	//   print_all_combos(pCombos-combo_nums,combo_nums);

	return combo_nums;
}

int search_general_3(POKERS* h, COMBO_OF_POKERS * pCombos,
	bool skip_bomb, bool skip_double, bool skip_three, bool skip_series)
{
	int combo_nums = 0;
	int num;
	if (!skip_bomb)
	{
		while (getBomb(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
	}

	if (!skip_three)
	{
		while (getThreeSeries(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
		//while (getThree(h, pCombos))
		//{
		//	remove_combo_poker(h, pCombos, NULL);
		//	combo_nums++;
		//	pCombos++;
		//}
	}

	if (!skip_double)
	{
		while (getDoubleSeries(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			updateDoubleSeries(h,pCombos);
			combo_nums++;
			pCombos++;
		}
	}

	if (!skip_series)
	{
		num = searchMultiSingleSeries(h, pCombos);
		combo_nums += num;
		pCombos += num;
		COMBO_OF_POKERS * tmp = pCombos - num;
		for (int k = 0; k<num; k++)
		{
			int res = search222inSingleSeries(h, tmp + k, pCombos);
			if (res == 1) {
				pCombos++;
				combo_nums++;
			}
			else
				search234inSingleSeries(h, tmp + k, pCombos);
			//     remove_combo_poker(h,pCombos,NULL);
		}

        // at last, connect single series, example: 34567 + 89TJQK
        int connect_single_series_nums = connectSingleSeries(tmp, combo_nums);
        combo_nums -= connect_single_series_nums;
        pCombos -= connect_single_series_nums;
	}
	//todo search for three's in series head and tail.
	num = browse_pokers(h, pCombos);

    for (int i = 0; i < num; ++i)
    {
        if (pCombos[i].type == PAIRS && pCombos[i].low < P2)
        {
            for (int j = 0; j < combo_nums; ++j)
            {
                if (pCombos[-(j + 1)].type == PAIRS_SERIES)
                {
                    //判断前后能否加上
                    if (pCombos[-(j + 1)].low == pCombos[i].low + 1)
                    {
                        pCombos[-(j + 1)].low--;
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                    else if (pCombos[-(j + 1)].low + pCombos[-(j + 1)].len == pCombos[i].low)
                    {
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                }
            }
        }
    }

	pCombos += num;
	combo_nums += num;

	//    print_all_combos(pCombos-combo_nums,combo_nums);

	return combo_nums;
}


//1. bomb 2. three 3. straight 4. else
int search_general_4(POKERS* h, COMBO_OF_POKERS * pCombos,
	bool skip_bomb, bool skip_double, bool skip_three, bool skip_series)
{
	int combo_nums = 0;
	int num = 0;

	if (!skip_bomb)
		while (getBomb(h, pCombos))
		{
			remove_combo_poker(h, pCombos, NULL);
			combo_nums++;
			pCombos++;
		}
	while (getThree(h, pCombos))
	{
		remove_combo_poker(h, pCombos, NULL);
		combo_nums++;
		pCombos++;
	}

	if (!skip_series) {

		num = searchMultiSingleSeries(h, pCombos);
		combo_nums += num;
		pCombos += num;
		COMBO_OF_POKERS * tmp = pCombos - num;
		for (int k = 0; k<num; k++)
		{
			int res = search222inSingleSeries(h, tmp + k, pCombos);
			if (res == 1) {
				pCombos++;
				combo_nums++;
			}
			else
				search234inSingleSeries(h, tmp + k, pCombos);
			//     remove_combo_poker(h,pCombos,NULL);
		}

        // at last, connect single series, example: 34567 + 89TJQK
        int connect_single_series_nums = connectSingleSeries(tmp, combo_nums);
        combo_nums -= connect_single_series_nums;
        pCombos -= connect_single_series_nums;
	}
	//todo search for three's in series head and tail.
	num = browse_pokers(h, pCombos);

    for (int i = 0; i < num; ++i)
    {
        if (pCombos[i].type == PAIRS && pCombos[i].low < P2)
        {
            for (int j = 0; j < combo_nums; ++j)
            {
                if (pCombos[-(j + 1)].type == PAIRS_SERIES)
                {
                    //判断前后能否加上
                    if (pCombos[-(j + 1)].low == pCombos[i].low + 1)
                    {
                        pCombos[-(j + 1)].low--;
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                    else if (pCombos[-(j + 1)].low + pCombos[-(j + 1)].len == pCombos[i].low)
                    {
                        pCombos[-(j + 1)].len++;
                        pCombos[i].type = NOTHING_old;
                        //memmove(&pCombos[i], &pCombos[i + 1], sizeof(COMBO_OF_POKERS*) * (num - i));
                        //num--;
                    }
                }
            }
        }
    }

	pCombos += num;
	combo_nums += num;


	//  print_all_combos(pCombos-combo_nums,combo_nums);

	return combo_nums;
}

COMBO_OF_POKERS*  find_MAX_len_in_combos(COMBO_OF_POKERS* combos, int total)
{
	COMBO_OF_POKERS* cur = NULL;
	int max_ddz = 0;
	for (int k = 0; k<total; k++)
	{
		if (combos[k].type != NOTHING_old)
		{
			int tmp;
			(tmp = get_combo_number(&combos[k]));
			if (tmp>max_ddz) {
				max_ddz = tmp;
				cur = combos + k;
			}
			else if (tmp == max_ddz && max_ddz>1 && combos[k].low < cur->low)
			{
				max_ddz = tmp;
				cur = combos + k;
			}
			else if (tmp == max_ddz && max_ddz == 1 && combos[k].low > cur->low)
			{
				max_ddz = tmp;
				cur = combos + k;
			}
		}
	}
	return cur;
}

COMBO_OF_POKERS*  find_biggest_in_combos(COMBO_OF_POKERS* combos, int total)
{
	COMBO_OF_POKERS* cur = NULL;
	int max_ddz = 0;
	for (int k = 0; k<total; k++)
	{
		if (combos[k].type != NOTHING_old)
		{
			if (combos[k].low >= max_ddz) {
				max_ddz = combos[k].low;
				cur = combos + k;
			}
		}
	}
	return cur;
}

/*
COMBO_OF_POKERS*  find_smallest_in_combos(COMBO_OF_POKERS* combos, int total,PLAYER* player)
{
COMBO_OF_POKERS* cur=NULL;
int max_ddz= 0;
for (int k=0; k<total; k++)
{
if (combos[k].type!=NOTHING_old &&)
{int tmp;
if(combos[k].low<max_ddz){
max_ddz=tmp;
cur= combos+k;
}
else if(combos[k].low == max_ddz && get_combo_number(combos[k])< get_combo_number(cur)){
max_ddz=tmp;
cur= combos+k;
}
}
}
return cur;
}
*/

//todo: you could sort not_biggest first...
COMBO_OF_POKERS*  find_smallest_in_combos(COMBO_OF_POKERS* com, int total, PLAYER* player, bool not_search_single)
{
	COMBO_OF_POKERS* cur = NULL;

	COMBOS_SUMMARY_DDZ_AI * s = player->summary;
	//    COMBO_OF_POKERS* combos=&s->not_biggest[0];
	if (s->not_biggest_num <= 0 && s->biggest_num <= 0)
	{
		PRINTF_ALWAYS("fatal error, no card.\n");
		cur = &player->combos_store[0];
		cur->low = P3;
		cur->type = SINGLE;
		return cur;
	}

	int max_ddz = 0;
	if (!not_search_single) {
		if (s->not_biggest_num >0) {
			cur = s->not_biggest[0];
			max_ddz = s->not_biggest[0]->low;
		}
		for (int k = 1; k<s->not_biggest_num; k++)
		{
			if (s->not_biggest[k]->type != NOTHING_old)
			{
				if (s->not_biggest[k]->low>max_ddz) {}
				else if (s->not_biggest[k]->low<max_ddz) {
					max_ddz = s->not_biggest[k]->low;
					cur = s->not_biggest[k];
				}
				else if (get_combo_number(s->not_biggest[k])> get_combo_number(cur)) {
					max_ddz = s->not_biggest[k]->low;
					cur = s->not_biggest[k];
				}
			}
		}
		if (cur == NULL) // search biggest
		{   //max_ddz len..
			cur = s->biggest[0];
			max_ddz = s->biggest[0]->low;
			if (s->biggest[0]->type == BOMB_old)
				max_ddz = LIT_JOKER;
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != NOTHING_old)
				{
					if (s->biggest[k]->low>max_ddz) {
					}
					else if (s->biggest[k]->low<max_ddz) {
						max_ddz = s->biggest[k]->low;
						cur = s->biggest[k];
					}
					else if (get_combo_number(s->biggest[k])> get_combo_number(cur)) {
						max_ddz = s->biggest[k]->low;
						cur = s->biggest[k];
					}
				}
			}
		}
	}
	else
	{
		int k;
		for (k = 0; k<s->not_biggest_num; k++)
		{
			if (s->not_biggest[k]->type != NOTHING_old &&  s->not_biggest[k]->type != SINGLE)
			{
				max_ddz = s->not_biggest[k]->low;
				cur = s->not_biggest[k];
				k++;
				break;
			}
		}
		for (; k<s->not_biggest_num; k++)
		{
			if (s->not_biggest[k]->type != NOTHING_old &&  s->not_biggest[k]->type != SINGLE)
			{
				if (s->not_biggest[k]->low>max_ddz) {}
				else if (s->not_biggest[k]->low<max_ddz) {
					max_ddz = s->not_biggest[k]->low;
					cur = s->not_biggest[k];
				}
				else if (get_combo_number(s->not_biggest[k])< get_combo_number(cur)) {
					max_ddz = s->not_biggest[k]->low;
					cur = s->not_biggest[k];
				}
			}
		}
#if 0
		if (cur == NULL) // search biggest
		{   //max_ddz len..
			cur = s->biggest[0];
			max_ddz = s->biggest[0]->low;
			if (s->biggest[0]->type == BOMB_old)
				max_ddz = LIT_JOKER;
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != NOTHING_old)
				{
					if (s->biggest[k]->low>max_ddz) {
					}
					else if (s->biggest[k]->low<max_ddz) {
						max_ddz = s->biggest[k]->low;
						cur = s->biggest[k];
					}
					else if (get_combo_number(s->biggest[k])> get_combo_number(cur)) {
						max_ddz = s->biggest[k]->low;
						cur = s->biggest[k];
					}
				}
			}
		}
#endif
	}
	return cur;
}


int  find_combo_with_3_controls_in_combos(COMBO_OF_POKERS* com, int total, PLAYER* player)
{
	COMBO_OF_POKERS* cur = NULL;

	COMBOS_SUMMARY_DDZ_AI * s = player->summary;
	//    COMBO_OF_POKERS* combos=&s->not_biggest[0];

	int max_ddz = 0;
	cur = s->not_biggest[0];
	max_ddz = s->not_biggest[0]->low;
	for (int k = 0; k<s->not_biggest_num; k++)
	{
		if (s->not_biggest[k]->type != NOTHING_old)
		{
			if (s->not_biggest[k]->low>max_ddz) {}
			else if (s->not_biggest[k]->low<max_ddz) {
				max_ddz = s->not_biggest[k]->low;
				cur = s->not_biggest[k];
			}
			else if (get_combo_number(s->not_biggest[k])< get_combo_number(cur)) {
				max_ddz = s->not_biggest[k]->low;
				cur = s->not_biggest[k];
			}
		}
	}
	return 0;
}

int get_control_poker_num_in_combo(COMBO_OF_POKERS* c, int lower)
{
	switch (c->type)
	{
	case ROCKET:
		//s->singles[s->series_num]=&c[k];
		//s->rocket_num++;
		//break;
		return 0;
	case BOMB_old:
		//if ( c->low >=lower)
		//  return 4;
		return 0;
	case SINGLE_SERIES:
		if (c->low + c->len - 1 >= lower)
			return c->low + c->len - lower;
	case PAIRS_SERIES:
		if (c->low + c->len - 1 >= lower)
			return 2 * (c->low + c->len - lower);
	case THREE_SERIES:
		if (c->low + c->len - 1 >= lower)
			return 3 * (c->low + c->len - lower);
	case 31:
		return 3 * (c->low >= lower) + c->three_desc[0] >= lower;
	case 32:
		return (c->low >= lower) * 3 + 2 * (c->three_desc[0] >= lower);
#if 0
	case THREE:
		if (c->low >= lower)
			return 3;
	case PAIRS:

		if (c->low >= lower)
			return 2;
	case SINGLE:
		if (c->low >= lower)
			return 1;
#else
	case THREE:  //donot check three,pairs,single
	case PAIRS:
	case SINGLE:
		return 0;
#endif
	case 3311:
	case 333111:
	case 33331111:
	case 531: {
		int     num = 0;
		if (c->low + c->len - 1 >= lower)  //todo fixme...
			num = 3 * (c->low + c->len - lower);
		for (int i = 0; i<c->len; i++)
		{
			num += c->three_desc[i] >= lower;
		}
		return num;
	}
	case 3322:
	case 333222:
	case 33332222:
	{
		int     num = 0;
		if (c->low + c->len - 1 >= lower)  //todo fixme...
			num = 3 * (c->low + c->len - lower);
		for (int i = 0; i<c->len; i++)
		{
			num += 2 * (c->three_desc[i] >= lower);
		}
		return num;
	}
	case 411:
	{
		int     num = 0;
		if (c->low >= lower)  //todo fixme...
			num = 4;
		for (int i = 0; i<2; i++)
		{
			num += (c->three_desc[0] >= lower);
		}
		return num;
	}
	case 422:
	{
		int     num = 0;
		if (c->low >= lower)  //todo fixme...
			num = 4;
		for (int i = 0; i<2; i++)
		{
			num += 2 * (c->three_desc[0] >= lower);
		}
		return num;
	}
	}

	return 0;
}


//update summary of combos
void update_summary(GAME_DDZ_AI* game, COMBOS_SUMMARY_DDZ_AI *s, POKERS*h, POKERS * opp, COMBO_OF_POKERS * c, int total, int opp1, int opp2, int lower, PLAYER * player)
{
	//s->ctrl.single = calc_controls(h,opp,CONTROL_POKER_NUM);
	s->real_total_num = 0;
	s->biggest_num = 0;
	s->extra_bomb_ctrl = 0;
	s->not_biggest_num = 0;
	s->combo_with_2_controls = 0;
	for (int k = 0; k<total; k++)
	{
		if (c[k].type != NOTHING_old)
		{
			s->real_total_num++;
			if (is_combo_biggest(game, opp, c + k, opp1, opp2, lower))
			{
				c[k].control = 1;
				s->biggest[s->biggest_num++] = &c[k];
				if (c[k].type == BOMB_old)
					s->extra_bomb_ctrl++;
			}
			else
			{
				c[k].control = 0;
				s->not_biggest[s->not_biggest_num++] = &c[k];

			}
			if (get_control_poker_num_in_combo(c + k, lower) >= 2 && c[k].control == 1)//&& c[k].type!=PAIRS)
				s->combo_with_2_controls++;
		}
	}


	// s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
	//  s->combo_total_num = s->real_total_num - s->biggest_num;
	//   if( player->MAX_control.single >0 )
	//     s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
	// else
	s->combo_total_num = s->real_total_num - s->biggest_num;

	int series_type = 0;
	if (s->series_num>0)
	{
		for (int k = 0; k<10; k++)
			series_type += s->series_detail[k] != 0;
	}
	s->combo_typenum = (s->pairs_num != 0) + series_type
		+ (s->singles_num != 0) + (s->three_num != 0) + (s->three_two_num != 0)
		+ (s->three_one_num != 0) + (s->pairs_series_num != 0) + (s->threeseries_num != 0)
		+ (s->threeseries_two_num != 0) + (s->threeseries_one_num != 0) + (s->four_one_num != 0)
		+ (s->four_two_num != 0)
		;
	s->combo_smallest_single = s->singles_num>0 ? s->singles[0]->low : 0;
	PRINTF(LEVEL + 1, "\nCombos summary: total %d type %d real_total %d biggest %d\n", s->combo_total_num, s->combo_typenum, s->real_total_num, s->biggest_num);
}

void update_summary_for_big_sure(GAME_DDZ_AI* game, COMBOS_SUMMARY_DDZ_AI *s,
	COMBO_OF_POKERS * c, int total, PLAYER * player)
{
	//s->ctrl.single = calc_controls(h,opp,CONTROL_POKER_NUM);
	s->real_total_num = 0;
	s->biggest_num = 0;
	s->extra_bomb_ctrl = 0;
	s->not_biggest_num = 0;
	s->combo_with_2_controls = 0;
	int lower = player->lower_control_poker;
	for (int k = 0; k<total; k++)
	{
		if (c[k].type != NOTHING_old)
		{
			s->real_total_num++;
			if (is_combo_biggest_sure_without_bomb(game, c + k))
			{
				c[k].control = 1;
				s->biggest[s->biggest_num++] = &c[k];
				if (c[k].type == BOMB_old)
					s->extra_bomb_ctrl++;
			}
			else
			{
				c[k].control = 0;
				s->not_biggest[s->not_biggest_num++] = &c[k];

			}
			if (get_control_poker_num_in_combo(c + k, lower) >= 2 && c[k].control == 1)//&& c[k].type!=PAIRS)
				s->combo_with_2_controls++;
		}
	}


	// s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
	//  s->combo_total_num = s->real_total_num - s->biggest_num;
	//   if( player->MAX_control.single >0 )
	//     s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
	// else
	s->combo_total_num = s->real_total_num - s->biggest_num;

	int series_type = 0;
	if (s->series_num>0)
	{
		for (int k = 0; k<10; k++)
			series_type += s->series_detail[k] != 0;
	}
	s->combo_typenum = (s->pairs_num != 0) + series_type
		+ (s->singles_num != 0) + (s->three_num != 0) + (s->three_two_num != 0)
		+ (s->three_one_num != 0) + (s->pairs_series_num != 0) + (s->threeseries_num != 0)
		+ (s->threeseries_two_num != 0) + (s->threeseries_one_num != 0) + (s->four_one_num != 0)
		+ (s->four_two_num != 0)
		;
	s->combo_smallest_single = s->singles_num>0 ? s->singles[0]->low : 0;
	PRINTF(LEVEL + 1, "\nCombos summary: total %d type %d real_total %d biggest %d\n", s->combo_total_num, s->combo_typenum, s->real_total_num, s->biggest_num);

}

void update_summary_for_single_and_pairs(GAME_DDZ_AI* game, COMBOS_SUMMARY_DDZ_AI *s, POKERS*h, POKERS * opp, COMBO_OF_POKERS * c, int total, int opp1, int opp2, int lower, PLAYER * player)
{
    //s->ctrl.single = calc_controls(h,opp,CONTROL_POKER_NUM);
    s->real_total_num = 0;
    s->biggest_num = 0;
    s->extra_bomb_ctrl = 0;
    s->not_biggest_num = 0;
    s->combo_with_2_controls = 0;

    int known_others_poker = game->known_others_poker; // 备份

    for (int k = 0; k < total; k++)
    {
        if (c[k].type != NOTHING_old)
        {
            // 此处使用known_others_poker函数内变量。game->known_others_poker会变化导致结果与预期不一致。
            game->known_others_poker = int(known_others_poker && (c[k].type != SINGLE && c[k].type != PAIRS)); // 只在计算单对的时候为暗牌
            s->real_total_num++;
            if (is_combo_biggest(game, opp, c + k, opp1, opp2, lower))
            {
                c[k].control = 1;
                s->biggest[s->biggest_num++] = &c[k];
                if (c[k].type == BOMB_old)
                    s->extra_bomb_ctrl++;
            }
            else
            {
                c[k].control = 0;
                s->not_biggest[s->not_biggest_num++] = &c[k];

            }
            if (get_control_poker_num_in_combo(c + k, lower) >= 2 && c[k].control == 1)//&& c[k].type!=PAIRS)
                s->combo_with_2_controls++;
        }
    }

    // 还原
    game->known_others_poker = known_others_poker;

    // s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
    //  s->combo_total_num = s->real_total_num - s->biggest_num;
    //   if( player->max_control.single >0 )
    //     s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
    // else
    s->combo_total_num = s->real_total_num - s->biggest_num;

    int series_type = 0;
    if (s->series_num > 0)
    {
        for (int k = 0; k < 10; k++)
            series_type += s->series_detail[k] != 0;
    }
    s->combo_typenum = (s->pairs_num != 0) + series_type
        + (s->singles_num != 0) + (s->three_num != 0) + (s->three_two_num != 0)
        + (s->three_one_num != 0) + (s->pairs_series_num != 0) + (s->threeseries_num != 0)
        + (s->threeseries_two_num != 0) + (s->threeseries_one_num != 0) + (s->four_one_num != 0)
        + (s->four_two_num != 0)
        ;
    s->combo_smallest_single = s->singles_num > 0 ? s->singles[0]->low : 0;
    PRINTF(LEVEL + 1, "\nCombos summary: total %d type %d real_total %d biggest %d\n", s->combo_total_num, s->combo_typenum, s->real_total_num, s->biggest_num);
}

//to be optimized
//current for
void sort_all_combos(GAME_DDZ_AI* game, COMBO_OF_POKERS * c, int total, COMBOS_SUMMARY_DDZ_AI * s,
	POKERS *opp, int opp1, int opp2, /* for check biggest*/
	int lower, PLAYER * player
)
{
	//   s->real_total_num = total;
	//    int total = s->combo_total_num;
	memset(s, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));//todo: optimized
	for (int k = 0; k<total; k++)
	{
		switch (c[k].type)
		{
		case ROCKET:
			//     s->bomb[s->bomb_num]=&c[k];
			//            s->bomb_num++;
			//     break;
		case BOMB_old:
			//  {//check if it is a laizi bomb or pure bomb
			c[k].three_desc[0] = 0;
			if (player->h->hands[c[k].low] == 4)
			{
				c[k].three_desc[0] = c[k].low == game->hun ? 2 : 1;

			}
			if (game->hun == -1)
				c[k].three_desc[0] = 0;

			{
				int pos = 0;
				for (int i = s->bomb_num - 1; i >= 0; i--)
				{
					if (check_combo_a_Big_than_b(&c[k], s->bomb[i]))
					{  //insert bomb here.
						pos = i + 1;
						break;
					}
				}
				for (int j = s->bomb_num - 1; j >= pos; j--)
				{
					s->bomb[j + 1] = s->bomb[j];
				}
				s->bomb[pos] = &c[k];
				s->bomb_num++;
			}
			break;
		case SINGLE_SERIES:
			s->series[s->series_num] = &c[k];
			s->series_num++;
			s->series_detail[c[k].len - 5]++;
			break;
		case PAIRS_SERIES:
			s->pairs_series[s->pairs_series_num] = &c[k];
			s->pairs_series_num++;
			break;
		case THREE_SERIES:
			s->threeseries[s->threeseries_num] = &c[k];
			s->threeseries_num++;
			break;
		case THREE:
			s->three[s->three_num] = &c[k];
			s->three_num++;
			break;
		case THREE_ONE:
			s->three_one[s->three_one_num] = &c[k];
			s->three_one_num++;
			break;
		case THREE_TWO:
			s->three_two[s->three_two_num] = &c[k];
			s->three_two_num++;
			break;
		case PAIRS:
			s->pairs[s->pairs_num] = &c[k];
			s->pairs_num++;
			break;
		case SINGLE:
			s->singles[s->singles_num] = &c[k];
			s->singles_num++;
			break;
		case 3311:
		case 333111:
		case 33331111:
		case 531:
			s->threeseries_one[s->threeseries_one_num++] = &c[k];
			break;
		case 3322:
		case 333222:
		case 33332222:
			s->threeseries_two[s->threeseries_two_num++] = &c[k];
			break;
		case 411:
			s->four_one[s->four_one_num++] = &c[k];
			break;
		case 422:
			s->four_two[s->four_two_num++] = &c[k];
			break;
		default:
			//NOTHING_old++;
			PRINTF_ALWAYS("!!ERR, line %d this type of combo not supported : %d\n", c[k].type, __LINE__);
			break;
		case NOTHING_old:
			break;
		}
	}

	// refine for three;
	int j, num = s->threeseries_num;
	//    int not_pad[j]
	for (j = 0; j<num; j++) //todo: spilt a pair..
	{
		unsigned int type1[6] = { 0,31,3311,333111,33331111,531 };
		unsigned int type2[5] = { 0,32,3322,333222,33332222 };

        //int not_control_singles_num = s->singles_num;
        //for (int k = 0; k < s->singles_num; ++k)
        //{
        //    if (s->singles[k]->low >= player->lower_control_poker)
        //    {
        //        not_control_singles_num -= 2;
        //    }
        //}

        //int not_control_pairs_num = s->pairs_num;
        //for (int k = 0; k < s->pairs_num; ++k)
        //{
        //    if (s->pairs[k]->low >= player->lower_control_poker)
        //    {
        //        not_control_pairs_num -= 2;
        //    }
        //}

        //if (not_control_pairs_num > not_control_singles_num && s->threeseries[j]->len <= not_control_pairs_num)
        //{
        //    goto _use_pairs;
        //}

		if (s->threeseries[j]->len <= s->singles_num && s->threeseries[j]->len <= s->pairs_num)
		{
			//todo:refine
			if (s->singles[s->threeseries[j]->len - 1]->low > s->pairs[s->threeseries[j]->len - 1]->low + 1
				|| (s->singles[s->threeseries[j]->len - 1]->low > s->pairs[s->threeseries[j]->len - 1]->low
					&& s->singles[s->threeseries[j]->len - 1]->low >= player->lower_control_poker))
			{
				goto _use_pairs;
			}
		}
		// for
		if (s->threeseries[j]->len <= s->singles_num && s->singles[s->threeseries[j]->len - 1]->low < player->lower_control_poker)
		{
			for (int k = 0; k< s->threeseries[j]->len; k++)
			{
				s->threeseries[j]->three_desc[k] = s->singles[k]->low;

				s->singles[k]->type = NOTHING_old;
			}
			s->singles_num -= s->threeseries[j]->len;
			memmove(s->singles, s->singles + s->threeseries[j]->len, (s->singles_num) * sizeof(void*));
			s->threeseries_one[s->threeseries_one_num] = s->threeseries[j];
			s->threeseries[j]->type = type1[s->threeseries[j]->len];
			s->threeseries_one_num++;
			s->threeseries_num--;
			s->threeseries[j] = NULL;
		}
        else if (s->threeseries[j]->len <= s->pairs_num && s->pairs[s->threeseries[j]->len - 1]->low < player->lower_control_poker)
		{
		_use_pairs:
			for (int k = 0; k< s->threeseries[j]->len; k++)
			{
				s->threeseries[j]->three_desc[k] = s->pairs[k]->low;
				s->pairs[k]->type = NOTHING_old;
			}
			s->pairs_num -= s->threeseries[j]->len;
			memmove(s->pairs, s->pairs + s->threeseries[j]->len, (s->pairs_num) * sizeof(void*));
			s->threeseries_two[s->threeseries_two_num] = s->threeseries[j];
			s->threeseries[j]->type = type2[s->threeseries[j]->len];
			s->threeseries_two_num++;
			//->biggest[s->biggest_num++]=  s->threeseries_two[j];
			s->threeseries_num--;
			s->threeseries[j] = NULL;
		}
#if ALLOW_SAME_KICKER // 相同带牌
        else if (s->threeseries[j]->len <= s->pairs_num * 2 + s->singles_num
            || (s->threeseries[j]->len == 5 && s->series_num > 0 && s->series[0]->len == 5)) // 长度为5的特殊情况
        {

            if (s->singles_num == 1 && s->threeseries[j]->len == 3){
                s->threeseries[j]->three_desc[0] = s->singles[0]->low;
                s->singles[0]->type = NOTHING_old;
                s->singles_num = 0;
                s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                s->threeseries[j]->three_desc[2] = s->pairs[0]->low;
                s->pairs[0]->type = NOTHING_old;

                memmove(s->pairs, s->pairs + 1, (s->pairs_num)*sizeof(void*));
                s->pairs_num--;
            }
            else if (s->singles_num == 0 && s->threeseries[j]->len == 2
                || (s->singles_num == 1 && s->pairs_num > 0 && s->threeseries[j]->len == 2 && s->pairs[0]->low < s->singles[0]->low))
            {
                s->threeseries[j]->type = 3311;
                s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                s->pairs[0]->type = NOTHING_old;

                memmove(s->pairs, s->pairs + 1, (s->pairs_num) * sizeof(void*));
                s->pairs_num--;
            }
            else if (s->threeseries[j]->len == 3 || s->threeseries[j]->len == 4)
            {
                // 整理单/对大小顺序，暂没考虑牌值限制
                int use_single_kicker[20] = {0};
                use_single_kicker[s->singles_num + s->pairs_num] = -1;
                for (int i = 0, k = 0;;)
                {
                    if (i >= s->singles_num)
                    {
                        while (k < s->pairs_num)
                        {
                            use_single_kicker[i + k] = 0;
                            k++;
                        }
                        break;
                    }
                    else if (k >= s->pairs_num)
                    {
                        while (i < s->singles_num)
                        {
                            use_single_kicker[i + k] = 1;
                            i++;
                        }
                        break;
                    }
                    else
                    {
                        if (s->singles[i]->low <= s->pairs[k]->low)
                        {
                            use_single_kicker[i + k] = 1;
                            ++i;
                        }
                        else
                        {
                            use_single_kicker[i + k] = 0;
                            ++k;
                        }
                    }
                }

                if (s->threeseries[j]->len == 3)
                {
                    if (use_single_kicker[0] != use_single_kicker[1])  //单+对
                    {
                        s->threeseries[j]->type = 333111;
                        s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                        s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                        s->threeseries[j]->three_desc[2] = s->singles[0]->low;
                        s->pairs[0]->type = NOTHING_old;
                        s->singles[0]->type = NOTHING_old;

                        memmove(s->pairs, s->pairs + 1, (s->pairs_num) * sizeof(void*));
                        s->pairs_num--;

                        memmove(s->singles, s->singles + 1, (s->singles_num) * sizeof(void*));
                        s->singles_num--;
                    }
                    else
                    {
                        if (use_single_kicker[0]) // 单+单
                        {
                            if (use_single_kicker[2]) // 3单
                            {
                                s->threeseries[j]->type = 333111;
                                s->threeseries[j]->three_desc[0] = s->singles[0]->low;
                                s->threeseries[j]->three_desc[1] = s->singles[1]->low;
                                s->threeseries[j]->three_desc[2] = s->singles[2]->low;
                                
                                s->singles[0]->type = NOTHING_old;
                                s->singles[1]->type = NOTHING_old;
                                s->singles[2]->type = NOTHING_old;

                                memmove(s->singles, s->singles + 3, (s->singles_num) * sizeof(void*));
                                s->singles_num -= 3;
                            }
                            else// 2单+对
                            {
                                if (s->singles_num >= 3 && (s->singles[2]->low < player->opps->end && s->singles[2]->low < P2
                                    && s->singles[2]->low < s->pairs[0]->low + 12)) // 单单对单
                                {
                                    s->threeseries[j]->type = 333111;
                                    s->threeseries[j]->three_desc[0] = s->singles[0]->low;
                                    s->threeseries[j]->three_desc[1] = s->singles[1]->low;
                                    s->threeseries[j]->three_desc[2] = s->singles[2]->low;

                                    s->singles[0]->type = NOTHING_old;
                                    s->singles[1]->type = NOTHING_old;
                                    s->singles[2]->type = NOTHING_old;

                                    memmove(s->singles, s->singles + 3, (s->singles_num) * sizeof(void*));
                                    s->singles_num -= 3;
                                }
                                else// 2单+拆对
                                {
                                    s->threeseries[j]->type = 333111;
                                    s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[2] = s->singles[0]->low;
                                    s->pairs[0]->type = NOTHING_old;
                                    s->singles[0]->type = NOTHING_old;

                                    memmove(s->pairs, s->pairs + 1, (s->pairs_num) * sizeof(void*));
                                    s->pairs_num--;

                                    memmove(s->singles, s->singles + 1, (s->singles_num) * sizeof(void*));
                                    s->singles_num--;
                                }
                            }
                        }
                        else // 对+对
                        {
                            if (s->pairs_num > 2 && !use_single_kicker[2]) // 对对对
                            {
                                s->threeseries[j]->type = 333222;
                                s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                                s->threeseries[j]->three_desc[1] = s->pairs[1]->low;
                                s->threeseries[j]->three_desc[2] = s->pairs[2]->low;
                                s->pairs[0]->type = NOTHING_old;
                                s->pairs[1]->type = NOTHING_old;
                                s->pairs[2]->type = NOTHING_old;

                                memmove(s->pairs, s->pairs + 3, (s->pairs_num) * sizeof(void*));
                                s->pairs_num -= 3;
                            }
                            else if (s->singles_num > 0 && use_single_kicker[2]) //对对单
                            {
                                // 拆2对
                                if (s->singles[0]->low >= player->opps->end 
                                    || s->singles[0]->low >= player->lower_control_poker
                                    || s->singles[0]->low >= s->pairs[1]->low + 12)
                                {
                                    s->threeseries[j]->type = 333111;
                                    s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[2] = s->pairs[1]->low;
                                    s->pairs[0]->type = NOTHING_old;
                                    s->pairs[1]->type = SINGLE;

                                    s->singles[s->singles_num] = s->pairs[1];
                                    s->singles_num++;

                                    memmove(s->pairs, s->pairs + 2, (s->pairs_num) * sizeof(void*));
                                    s->pairs_num -= 2;
                                }
                                else // 对+单
                                {
                                    s->threeseries[j]->type = 333111;
                                    s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                                    s->threeseries[j]->three_desc[2] = s->singles[0]->low;
                                    s->pairs[0]->type = NOTHING_old;
                                    s->singles[0]->type = NOTHING_old;

                                    memmove(s->pairs, s->pairs + 1, (s->pairs_num) * sizeof(void*));
                                    s->pairs_num--;

                                    memmove(s->singles, s->singles + 1, (s->singles_num) * sizeof(void*));
                                    s->singles_num--;
                                }
                            }
                            else // 对对空，拆2对
                            {
                                s->threeseries[j]->type = 333111;
                                s->threeseries[j]->three_desc[0] = s->pairs[0]->low;
                                s->threeseries[j]->three_desc[1] = s->pairs[0]->low;
                                s->threeseries[j]->three_desc[2] = s->pairs[1]->low;
                                s->pairs[0]->type = NOTHING_old;
                                s->pairs[1]->type = SINGLE;

                                s->singles[s->singles_num] = s->pairs[1];
                                s->singles_num++;

                                memmove(s->pairs, s->pairs + 2, (s->pairs_num) * sizeof(void*));
                                s->pairs_num -= 2;
                            }
                        }
                    }
                }
                else //if (s->threeseries[j]->len == 4)
                {
                    // 带单：有2个小单,或者对子不够,或者第4小的对子是控制牌
                    if ((s->singles_num > 1 && s->singles[1]->low < player->lower_control_poker)
                        || s->pairs_num < s->threeseries[j]->len || s->pairs[3]->low >= player->lower_control_poker)
                    {
                        //从小往大遍历
                        int kicker_cnt = 0;
                        int i = 0, k = 0;
                        for (; kicker_cnt < s->threeseries[j]->len && (i < s->singles_num || k < s->pairs_num);)
                        {
                            if (use_single_kicker[i + k])
                            {
                                // add
                                s->threeseries[j]->three_desc[kicker_cnt++] = s->singles[i]->low;
                                s->singles[i]->type = NOTHING_old;
                                i++;
                            }
                            else
                            {
                                if (kicker_cnt == s->threeseries[j]->len - 1)
                                {
                                    // 考察之后的一手单牌
                                    if (i >= s->singles_num 
                                        || s->singles[i]->low >= player->lower_control_poker 
                                        || s->singles[i]->low - s->pairs[k]->low >= 12)
                                    {
                                        //拆对
                                        s->threeseries[j]->three_desc[kicker_cnt] = s->pairs[k]->low;
                                        s->pairs[k]->type = SINGLE;                                        

                                        s->singles[s->singles_num] = s->pairs[k];
                                        s->singles_num++;                 
                                        
                                        k++;
                                    }
                                    else
                                    {
                                        // 带单
                                        s->threeseries[j]->three_desc[kicker_cnt] = s->singles[i]->low;
                                        s->singles[i]->type = NOTHING_old;
                                        i++;
                                    }
                                    kicker_cnt++;
                                }
                                else
                                {
                                    // add
                                    s->threeseries[j]->three_desc[kicker_cnt++] = s->pairs[k]->low;
                                    s->threeseries[j]->three_desc[kicker_cnt++] = s->pairs[k]->low;
                                    s->pairs[k]->type = NOTHING_old;
                                    k++;
                                }
                            }
                        }
                        
                        s->threeseries[j]->type = 33331111;

                        memmove(s->singles, s->singles + i, (s->singles_num) * sizeof(void*));
                        s->singles_num -= i;

                        memmove(s->pairs, s->pairs + k, (s->pairs_num) * sizeof(void*));
                        s->pairs_num -= k;                        
                    }
                    else
                    {
                        s->threeseries[j]->type = 33332222;
                        for (int i = 0; i < s->threeseries[j]->len; ++i)
                        {
                            s->threeseries[j]->three_desc[i] = s->pairs[i]->low;
                            s->pairs[i]->type = NOTHING_old;
                        }
                        memmove(s->pairs, s->pairs + 4, (s->pairs_num) * sizeof(void*));
                        s->pairs_num -= 4;
                    }
                }                
            }
            else if (s->threeseries[j]->len == 5)
            {
                if (player->id == LORD_old && player->h->total == max_POKER_NUM && (player->h->hands[LIT_JOKER] == 0 || player->h->hands[BIG_JOKER] == 0))
                {
                    bool has_three = false;
                    for (int i = 0; i < P2; ++i)
                    {
                        if ((i < s->threeseries[j]->low || i >= s->threeseries[j]->low + s->threeseries[j]->len) && player->h->hands[i] >= 3)
                        {
                            has_three = true;
                            break;
                        }
                    }
                    // 不带炸弹，不带3带
                    if (!has_three)
                    {
                        s->threeseries[j]->type = 531;
                        // add kicker
                        int kicker_cnt = 0;
                        for (int i = 0; i <= BIG_JOKER && kicker_cnt < s->threeseries[j]->len; ++i)
                        {
                            for (int m = 0; m < player->h->hands[i]; ++m)
                            {
                                if (i < s->threeseries[j]->low || i >= s->threeseries[j]->low + s->threeseries[j]->len)
                                {
                                    s->threeseries[j]->three_desc[kicker_cnt++] = i;
                                }
                            }
                        }
                        // set zero
                        for (int i = 0; i < s->singles_num; ++i)
                        {
                            s->singles[i]->type = 0;
                        }
                        s->singles_num = 0;

                        for (int i = 0; i < s->pairs_num; ++i)
                        {
                            s->pairs[i]->type = 0;
                        }
                        s->pairs_num = 0;

                        if (s->series_num > 0)
                        {
                            s->series[0]->type = NOTHING_old;
                            s->series_num = 0;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }
#endif
            else if (s->threeseries[j]->len <= s->pairs_num + s->singles_num) // TODO
            {
                // copy form old version
                if (s->threeseries[j]->len <= s->singles_num)
                {
                    for (int k = 0; k < s->threeseries[j]->len; k++)
                    {
                        s->threeseries[j]->three_desc[k] = s->singles[k]->low;

                        s->singles[k]->type = NOTHING_old;
                    }
                    s->singles_num -= s->threeseries[j]->len;
                    memmove(s->singles, s->singles + s->threeseries[j]->len, (s->singles_num) * sizeof(void*));
                    s->threeseries_one[s->threeseries_one_num] = s->threeseries[j];
                    s->threeseries[j]->type = type1[s->threeseries[j]->len];
                    s->threeseries_one_num++;
                    s->threeseries_num--;
                    s->threeseries[j] = NULL;
                    continue;
                }
                else if (s->threeseries[j]->len <= s->pairs_num)
                {
                    for (int k = 0; k < s->threeseries[j]->len; k++)
                    {
                        s->threeseries[j]->three_desc[k] = s->pairs[k]->low;
                        s->pairs[k]->type = NOTHING_old;
                    }
                    s->pairs_num -= s->threeseries[j]->len;
                    memmove(s->pairs, s->pairs + s->threeseries[j]->len, (s->pairs_num) * sizeof(void*));
                    s->threeseries_two[s->threeseries_two_num] = s->threeseries[j];
                    s->threeseries[j]->type = type2[s->threeseries[j]->len];
                    s->threeseries_two_num++;
                    //->biggest[s->biggest_num++]=  s->threeseries_two[j];
                    s->threeseries_num--;
                    s->threeseries[j] = NULL;
                    continue;
                }
                else
                {
                    //int single=0,pair=0;
                    //do not support len ==5;
                    //int cur_single =0;
                    int k = 0;
                    //for (int k=0; k< s->threeseries[j]->len ;  )
                    {
                        for (int i = 0; i < s->singles_num; i++)
                        {
                            s->threeseries[j]->three_desc[k++] = s->singles[i]->low;
                            s->singles[i]->type = NOTHING_old;
                        }
                        s->singles_num = 0;

                        for (; k < s->threeseries[j]->len;)
                        {
                            s->threeseries[j]->three_desc[k++] = s->pairs[0]->low;
                            {
                                s->pairs[0]->type = SINGLE;
                                s->singles_num++;
                                s->singles[s->singles_num - 1] = s->pairs[0];
                                memmove(s->pairs, s->pairs + 1, (s->pairs_num) * sizeof(void*));
                                
                                s->pairs_num--;                                
                                //break;
                            }
                        }
                    }
                }
#if ALLOW_SAME_KICKER
            }
            else
            {
                continue;
            }
#endif
            s->threeseries_one[s->threeseries_one_num] = s->threeseries[j];
            s->threeseries[j]->type = type1[s->threeseries[j]->len];
            //s->biggest[s->biggest_num++]=  s->threeseries_two[j];
            s->threeseries_one_num++;
            s->threeseries_num--;
            s->threeseries[j] = NULL;
        }
		//todo refine
		//    break;
	}


	if (s->threeseries_num >0) //strange
	{
		int k = 0;
		for (int j = 0; j<3; j++)
		{
			if (s->threeseries[j] != NULL)
			{
				s->threeseries[k++] = s->threeseries[j];
			}
		}
		if (k != s->threeseries_num)
			PRINTF_ALWAYS("ERR: 3311 or 3322\n");
	}


	if (s->three_num>0 && s->singles_num >= s->three_num && s->pairs_num >= s->three_num)
	{
		int use_pairs_for_three = 0;
		if (s->singles[s->three_num - 1]->low > s->pairs[s->three_num - 1]->low + 1
			|| (s->singles[s->three_num - 1]->low > s->pairs[s->three_num - 1]->low
				&& s->singles[s->three_num - 1]->low >= player->lower_control_poker))
		{
            if (!is_combo_biggest_sure(game, s->pairs[s->three_num - 1]))
            {
			    use_pairs_for_three = 1;
		    }
        }

        // 刚好可以把单牌带完，全部带单
        if (s->singles_num == s->three_num
            && s->pairs_num > s->singles_num
            && s->singles[s->singles_num - 1]->low < player->lower_control_poker)
        {
            use_pairs_for_three = 0;
        }

		if ((game->player_type == UPFARMER_old && game->players[CUR_PLAYER]->oppDown_num == 1)
			|| (game->player_type == DOWNFARMER_old && game->players[CUR_PLAYER]->oppUp_num == 1)
			|| (game->player_type == LORD_old &&
			(game->players[CUR_PLAYER]->oppDown_num == 1
				|| game->players[CUR_PLAYER]->oppUp_num == 1))
			)
		{
			use_pairs_for_three = 0;
		}

		for (j = 0; j<s->three_num; j++) //todo: spilt a pair..
		{
			// for
			if (!use_pairs_for_three)
			{
				s->three_one[s->three_one_num] = s->three[j];
				s->three[j]->type = 31;
				s->three[j]->three_desc[0] = s->singles[j]->low;
				s->three_one_num++;
				s->singles[j]->type = NOTHING_old;
			}
			else
			{
				//search single first
				s->three_two[s->three_two_num] = s->three[j];
				s->three[j]->type = 32;
				s->three[j]->three_desc[0] = s->pairs[j]->low;
				// s->three_two_num = j - s->singles_num;
                s->three_two_num++;
				s->pairs[j]->type = NOTHING_old;
			}
		}

		if (!use_pairs_for_three)
		{
			memmove(s->singles, s->singles + s->three_num, (s->singles_num - s->three_num) * sizeof(void*));
			s->singles_num = (s->singles_num - s->three_num);
		}
		else
		{
			memmove(s->pairs, s->pairs + s->three_num, (s->pairs_num - s->three_num) * sizeof(void*));
			s->pairs_num -= s->three_num;
		}
		s->three_num = 0;
	}
	else if (s->three_num > 0)
	{
		//特别优化
		bool oppOnly1Card = (game->player_type == LORD_old && (player->oppUp_num == 1 || player->oppDown_num == 1))
			|| (game->player_type == UPFARMER_old && player->oppDown_num == 1)
			|| (game->player_type == DOWNFARMER_old && player->oppUp_num == 1);

        if (level == 0) // level_0 == 0
        {
            oppOnly1Card = true; // 等级0始终是用之前的带牌方法
        }

		if (oppOnly1Card) //之前的带牌方法，优先带单
		{
			for (j = 0; j < s->three_num; j++) //todo: spilt a pair..
			{
				// for
				if (j < s->singles_num)
				{
					s->three_one[s->three_one_num] = s->three[j];
					s->three[j]->type = 31;
					s->three[j]->three_desc[0] = s->singles[j]->low;
					s->three_one_num++;
					s->singles[j]->type = NOTHING_old;
				}
				else if (j - s->singles_num < s->pairs_num)
				{
					//search single first
					s->three_two[s->three_two_num] = s->three[j];
					s->three[j]->type = 32;
					s->three[j]->three_desc[0] = s->pairs[j - s->singles_num]->low;
					//s->three_two_num = j - s->singles_num + 1;
                    s->three_two_num++;
					s->pairs[j - s->singles_num]->type = NOTHING_old;
				}
				else //todo refine
					break;
			}

			if (j < s->three_num)
			{
				memmove(s->three, s->three + j, (s->three_num - j) * sizeof(void*));
				s->three_num = (s->three_num - j);
				s->singles_num = 0;
				s->pairs_num = 0;
			}
			else if (s->three_num > 0)
			{
				if (s->three_num <= s->singles_num)
				{
					memmove(s->singles, s->singles + s->three_num, (s->singles_num - s->three_num) * sizeof(void*));
					s->singles_num = (s->singles_num - s->three_num);
				}
				else
				{

					int number = s->three_num - s->singles_num;
					s->singles_num = 0;
					memmove(s->pairs, s->pairs + number, (s->pairs_num - number) * sizeof(void*));
					s->pairs_num -= number;
				}
				s->three_num = 0;
			}
		}
		else
		{
			COMBO_OF_POKERS **singles = s->singles;
			COMBO_OF_POKERS **pairs = s->pairs;

			int singlesNum = s->singles_num;
			int pairsNum = s->pairs_num;

			int singleKickerNum = 0; //记录带入到kicker中的数量
			int pairKickerNum = 0;

			int i = 0;
			//只剩一个小对,且不报单
			if (pairsNum == 1 && pairs[0]->low <= P6 && !oppOnly1Card)
			{
				s->three_two[s->three_two_num] = s->three[i];
				s->three[i]->type = 32;
				s->three[i]->three_desc[0] = pairs[pairKickerNum]->low;
				pairs[pairKickerNum]->type = NOTHING_old;
				s->three_two_num++;
				pairKickerNum++;
				i++;
			}
			//只剩一个小单牌
			if (singlesNum == 1 && singles[0]->low <= P6)
			{
				s->three_one[s->three_one_num] = s->three[i];
				s->three[i]->type = 31;
				s->three[i]->three_desc[0] = singles[singleKickerNum]->low;
				singles[singleKickerNum]->type = NOTHING_old;
				s->three_one_num++;
				singleKickerNum++;
				i++;
			}

			for (; i < s->three_num; ++i)
			{
                 if (singlesNum - singleKickerNum > 0 && (pairsNum - pairKickerNum <= 0 || singles[singleKickerNum]->low <= pairs[pairKickerNum]->low)) // 带单
				{
					s->three_one[s->three_one_num] = s->three[i];
					s->three[i]->type = 31;
					s->three[i]->three_desc[0] = singles[singleKickerNum]->low;
					singles[singleKickerNum]->type = NOTHING_old;
					s->three_one_num++;
					singleKickerNum++;
				}
                else if (pairsNum - pairKickerNum > 0/* && s->pairs[pairKickerNum]->low < player->lower_control_poker*/) //带对
				{
					s->three_two[s->three_two_num] = s->three[i];
					s->three[i]->type = 32;
					s->three[i]->three_desc[0] = pairs[pairKickerNum]->low;
					pairs[pairKickerNum]->type = NOTHING_old;
					s->three_two_num++;
					pairKickerNum++;
				}
				else
				{
                    // 带完单/对就走了的情况
                    if (singlesNum - singleKickerNum > 0 && player->h->total == 4 && s->singles_num == 1)
                    {
                        s->three_one[s->three_one_num] = s->three[i];
                        s->three[i]->type = 31;
                        s->three[i]->three_desc[0] = singles[singleKickerNum]->low;
                        singles[singleKickerNum]->type = NOTHING_old;
                        s->three_one_num++;
                        singleKickerNum++;
                    }
                    else if (pairsNum - pairKickerNum > 0 && player->h->total == 5 && s->pairs_num == 1)
                    {
                        s->three_two[s->three_two_num] = s->three[i];
                        s->three[i]->type = 32;
                        s->three[i]->three_desc[0] = pairs[pairKickerNum]->low;
                        pairs[pairKickerNum]->type = NOTHING_old;
                        s->three_two_num++;
                        pairKickerNum++;
                    }
					break;
				}
			}

			if (singleKickerNum > 0)
			{
				memmove(s->singles, s->singles + singleKickerNum, (s->singles_num - singleKickerNum) * sizeof(void*));
				s->singles_num -= singleKickerNum;
			}
			if (pairKickerNum > 0)
			{
				memmove(s->pairs, s->pairs + pairKickerNum, (s->pairs_num - pairKickerNum) * sizeof(void*));
				s->pairs_num -= pairKickerNum;
			}
			if (singleKickerNum > 0 || pairKickerNum > 0)
			{
				memmove(s->three,
					s->three + singleKickerNum + pairKickerNum,
					(s->three_num - (singleKickerNum + pairKickerNum)) * sizeof(void*));
				s->three_num -= singleKickerNum + pairKickerNum;
			}
		}
	}

	update_summary(game, s, player->h, opp, c, total, opp1, opp2, lower, player);
}


int get_2nd_MIN_singles(COMBOS_SUMMARY_DDZ_AI *s)
{
	if (s->singles_num <= 1)
		return BIG_JOKER;
	else {
		return s->singles[1]->low;
	}
}

int cmp_summary_for_MIN_single_farmer(COMBOS_SUMMARY_DDZ_AI *cur, COMBOS_SUMMARY_DDZ_AI *result) //return 1 if a is better than b
{
	return
		(get_2nd_MIN_singles(cur) > get_2nd_MIN_singles(result)
			|| (cur->singles_num < result->singles_num
				&&  get_2nd_MIN_singles(cur) == get_2nd_MIN_singles(result))
			);
}

/*
i.  控制 - 套的数目 最少。
ii. 套的种类最少
iii.    单牌数量最少
iv. 最小的单牌最大
*/
int cmp_summary(COMBOS_SUMMARY_DDZ_AI *a, COMBOS_SUMMARY_DDZ_AI *b) //return a>b
{
	if ((a->combo_total_num - a->extra_bomb_ctrl< b->combo_total_num - b->extra_bomb_ctrl)
		)
		return true;
	else    if (a->combo_total_num - a->extra_bomb_ctrl > b->combo_total_num - b->extra_bomb_ctrl
		)
		return false;
	else  if (a->combo_total_num - a->extra_bomb_ctrl == b->combo_total_num - b->extra_bomb_ctrl
		)
	{
		if (a->extra_bomb_ctrl>b->extra_bomb_ctrl)
			return true;
		if (a->extra_bomb_ctrl<b->extra_bomb_ctrl)
			return false;
		if (a->combo_typenum < b->combo_typenum)
			return true;
		else if (a->combo_typenum > b->combo_typenum)
			return false;
		else {
			if (a->singles_num > b->singles_num)
				return false;
			else if (a->singles_num < b->singles_num)
				return true;
			else
				return a->combo_smallest_single > b->combo_smallest_single;
		}
	}
	return 0;
}

int search_combos_in_hands(GAME_DDZ_AI* game, POKERS* h, COMBO_OF_POKERS * pCombos, COMBOS_SUMMARY_DDZ_AI *pSummary, PLAYER * player)
{
	int combo_nums = 0;
	/*
	//serach four
	if ( (h->end-h->begin < 2 || h->end-h->begin < 4 )&& h->total <6  )
	{
	combo_nums =  browse_pokers(h,pCombos);;
	return combo_nums;
	}
	else if ( h->end-h->begin < 4 )
	{
	POKERS t;
	memmove(&t,h,sizeof(POKERS));
	h= &t;
	while ( getBomb(h,pCombos))
	{
	remove_combo_poker(h,pCombos,NULL);
	combo_nums++;
	pCombos++;
	}

	if ( getThreeSeries(h,pCombos))
	{
	remove_combo_poker(h,pCombos,NULL);
	combo_nums++;
	pCombos++;
	}
	if ( getDoubleSeries(h,pCombos)) //todo: check three in doubles.
	{
	remove_combo_poker(h,pCombos,NULL);
	updateDoubleSeries(h,pCombos);
	combo_nums++;
	pCombos++;
	}

	int num=browse_pokers(h,pCombos);
	combo_nums+=num;
	pCombos+=num;

	sort_all_combos(game,pCombos-combo_nums,combo_nums,pSummary,
	player->opps,player->oppDown_num,player->oppUp_num, player->lower_control_poker,player);
	return combo_nums;
	}
	else
	*/
	{
		COMBO_OF_POKERS *presult, *pNow, *pTmp; //todo: use alloce
		COMBOS_SUMMARY_DDZ_AI *cur, *result, *tmp;
		presult = game->tmp_combos[0];//(COMBO_OF_POKERS*) malloc(sizeof(COMBO_OF_POKERS)*20); //comtmp[0][0];
		pNow = game->tmp_combos[1];//(COMBO_OF_POKERS*) malloc(sizeof(COMBO_OF_POKERS)*20);

		memset(presult, 0, sizeof(COMBO_OF_POKERS) * 20); //comtmp[0][0];
		memset(pNow, 0, sizeof(COMBO_OF_POKERS) * 20);


		result = &game->tmp_summary[0];//(COMBOS_SUMMARY_DDZ_AI *)malloc(sizeof(COMBOS_SUMMARY_DDZ_AI));
		cur = &game->tmp_summary[1];//(COMBOS_SUMMARY_DDZ_AI *)malloc(sizeof(COMBOS_SUMMARY_DDZ_AI));
		memset(cur, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));
		memset(result, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));

		int num = 0, numRes = 0;
		POKERS t;

		memmove(&t, h, sizeof(POKERS));
		numRes = result->combo_total_num = search_general_1(&t, presult, 0, 0, 0, 0);
		sort_all_combos(game, presult, numRes, result, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);
		//DBG(print_all_combos(presult,numRes));

		memmove(&t, h, sizeof(POKERS));
		num = cur->combo_total_num = search_general_2(&t, pNow, 0, 0, 0, 0);
		sort_all_combos(game, pNow, num, cur, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);
		//DBG(print_all_combos(pNow,num));

		if (cmp_summary(cur, result))
		{
			numRes = num;
			pTmp = presult;
			presult = pNow;
			pNow = pTmp;
			tmp = result;
			result = cur;
			cur = tmp;
		}

		memmove(&t, h, sizeof(POKERS));
		//        memset(cur,0,sizeof(COMBOS_SUMMARY_DDZ_AI));
		num = cur->combo_total_num = search_general_3(&t, pNow, 0, 0, 0, 0);
		sort_all_combos(game, pNow, num, cur, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);
		// DBG(print_all_combos(pNow,num));


		if (cmp_summary(cur, result))
		{
			numRes = num;
			pTmp = presult;
			presult = pNow;
			pNow = pTmp;

			tmp = result;
			result = cur;
			cur = tmp;
		}

		// for (int k=0; k<numRes; k++)
		memmove(pCombos, presult, numRes * sizeof(COMBO_OF_POKERS));
		sort_all_combos(game, pCombos, numRes, pSummary, player->opps, player->oppDown_num
			, player->oppUp_num, player->lower_control_poker, player);
		//  print_all_combos(pCombos, numRes);
		return numRes;
	}
}

//地主报单农民组牌方式
int search_combos_in_suits_for_MIN_single_farmer_internal
(GAME_DDZ_AI* game, POKERS* h, COMBO_OF_POKERS * pCombos, COMBOS_SUMMARY_DDZ_AI *pSummary, PLAYER * player)
{
	memset(pCombos, 0, sizeof(COMBO_OF_POKERS) * 20);

	COMBO_OF_POKERS *pNow;
	COMBOS_SUMMARY_DDZ_AI *cur, *result;
	pNow = game->tmp_combos[0];// (COMBO_OF_POKERS*) malloc(sizeof(COMBO_OF_POKERS)*20);
	memset(pNow, 0, sizeof(COMBO_OF_POKERS) * 20);

	result = player->summary;
	cur = &game->tmp_summary[0];//(COMBOS_SUMMARY_DDZ_AI *)malloc(sizeof(COMBOS_SUMMARY_DDZ_AI));
	memset(cur, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));
	//        memset(result,0,sizeof(COMBOS_SUMMARY_DDZ_AI));

	POKERS t;
	memmove(&t, h, sizeof(POKERS));
	int num = search_general_1(&t, pCombos, 0, 1, 0, 0);
	sort_all_combos(game, pCombos, num, result, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);

	if (result->bomb_num != 0 && result->singles_num >= 2)
	{
		if (result->bomb[0]->type != ROCKET)
		{
			//change it to 411
			result->bomb[0]->type = 411;
			result->bomb[0]->three_desc[0] = result->singles[0]->low;
			result->bomb[0]->three_desc[1] = result->singles[1]->low;
			result->four_one[0] = result->bomb[0];
			result->bomb_num--;
			result->four_one_num++;
			result->singles_num -= 2;
			result->singles[0]->type = NULL;
			result->singles[1]->type = NULL;

			for (int i = 0; i<result->bomb_num; i++)
			{
				result->bomb[i] = result->bomb[i + 1];
			}
			for (int i = 0; i<result->singles_num; i++)
			{
				result->singles[i] = result->singles[i + 2];
			}
		}
	}
	//todo: add four_two
	memmove(&t, h, sizeof(POKERS));
	int num1 = search_general_4(&t, pNow, 0, 1, 0, 0);
	sort_all_combos(game, pNow, num1, cur, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);

	if (cur->singles_num < result->singles_num
		|| (cur->singles_num == result->singles_num
			&&  get_2nd_MIN_singles(cur) > get_2nd_MIN_singles(result))
		)
	{
		memmove(pCombos, pNow, max_ddz(num1, num) * sizeof(COMBO_OF_POKERS));
		num = num1;
		sort_all_combos(game, pCombos, num, result, player->opps, player->oppDown_num, player->oppUp_num, player->lower_control_poker, player);
	}
	*pSummary = *player->summary;

	DBG(print_all_combos(pCombos, 20));
	return num;
}



#define CMP_AND_SWAP_SMY_SINGLE()\
    /*print_summary(result);*/\
    /*print_summary(cur);*/\
    if ( cmp_summary_for_MIN_single_farmer(cur,result))\
{\
    numRes = number;\
    pTmp = presult;\
    presult=pNow;\
    pNow=pTmp;\
    tmp=result;\
    result= cur;\
    cur=tmp;\
    /*print_all_combos(presult, number);*/\
}

/*
Get the
*/
int search_combos_in_suits_for_MIN_single_farmer
(GAME_DDZ_AI* game, POKERS *h, COMBO_OF_POKERS *combos_tmp, PLAYER* player)
{
	POKERS *opp = player->opps;
	//player->lower_control_poker =
	//    player->MAX_control.single = calc_controls( h,opp, CONTROL_POKER_NUM );

	COMBO_OF_POKERS * combos = player->combos;
	COMBOS_SUMMARY_DDZ_AI *pcombo_summary, combo_summary;
	pcombo_summary = &combo_summary;
	for (int k = 0; k<20; k++)
	{
		combos[k].type = NOTHING_old;;
		//PRINTF(LEVEL,"\n");
	}

	h->hun = game->hun;

	if (game->hun != -1 && HUN_NUMBER(h)>0)
	{
		COMBO_OF_POKERS *presult, *pNow, *pTmp;
		COMBOS_SUMMARY_DDZ_AI *cur, *result, *tmp;
		presult = game->tmp_combos[2];
		pNow = game->tmp_combos[3];

		memset(presult, 0, sizeof(COMBO_OF_POKERS) * 25);
		memset(pNow, 0, sizeof(COMBO_OF_POKERS) * 25);

		result = &game->tmp_summary[2];
		cur = &game->tmp_summary[3];
		memset(cur, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));
		memset(result, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));

		int numRes = search_combos_in_suits_for_MIN_single_farmer_internal
		(game, h, presult, result, player);
		int number;

		int ben_hun = 0;
		//set first hun values  
		for (int i1 = P3; i1 <= P2; i1++)
		{
			SET_HUN_TO_i(h, i1);
			//    PRINTF(VERBOSE, "==set first hun to %c\n", poker_to_char(i1));
			if (IS_HUN(h, i1)) ben_hun++;
			if (HUN_NUMBER(h)>ben_hun)
			{
				//set second hun values  
				for (int i2 = P3; i2 <= P2; i2++)
				{
					SET_HUN_TO_i(h, i2);
					//            PRINTF(VERBOSE, "\t==set second hun to %c\n", poker_to_char(i2));                 
					if (IS_HUN(h, i2)) ben_hun++;
					if (HUN_NUMBER(h)>ben_hun)
					{
						//set second hun values  
						for (int i3 = P3; i3 <= P2; i3++)
						{
							SET_HUN_TO_i(h, i3);
							//                        PRINTF(VERBOSE, "\t\t==set third hun to %c\n", poker_to_char(i3));                                                
							number = search_combos_in_suits_for_MIN_single_farmer_internal
							(game, h, pNow, cur, player);
							CMP_AND_SWAP_SMY_SINGLE()
								SET_i_to_HUN(h, i3);
						}
					}
					else
					{
						number = search_combos_in_suits_for_MIN_single_farmer_internal
						(game, h, pNow, cur, player);
						CMP_AND_SWAP_SMY_SINGLE()
					}
					if (IS_HUN(h, i2)) ben_hun--;
					SET_i_to_HUN(h, i2);
				}
			}
			else
			{
				number = search_combos_in_suits_for_MIN_single_farmer_internal
				(game, h, pNow, cur, player);
				CMP_AND_SWAP_SMY_SINGLE()
			}
			if (IS_HUN(h, i1)) ben_hun--;
			SET_i_to_HUN(h, i1);
		}

		memmove(combos, presult, numRes * sizeof(COMBO_OF_POKERS));
		sort_all_combos(game, player->combos, number, player->summary, player->opps, player->oppDown_num
			, player->oppUp_num, player->lower_control_poker, player);

	}
	else
	{
		int number = search_combos_in_suits_for_MIN_single_farmer_internal(game, h, combos, pcombo_summary, player);
        sort_all_combos(game, player->combos, number, player->summary, player->opps, player->oppDown_num
            , player->oppUp_num, player->lower_control_poker, player);
	}

	if (level >= VERBOSE)
		print_suit(player);

	return true;
}


/*
Get the
*/
int search_combos_in_suit(GAME_DDZ_AI* game, POKERS *h, POKERS *opp, PLAYER* player)
{
	int ctrl_num = CONTROL_POKER_NUM;
	POKERS all;
	add_poker(h, opp, &all);
	if (all.total <= 20)
		ctrl_num = 4;
	else if (all.total <= 10)
		ctrl_num = 2;
	player->lower_control_poker = get_lowest_controls(&all, ctrl_num);
	int single = calc_controls(h, opp, ctrl_num);

	COMBO_OF_POKERS * combos = player->combos;
	COMBOS_SUMMARY_DDZ_AI *pcombo_summary, combo_summary;
	pcombo_summary = &combo_summary;
	for (int k = 0; k<20; k++)
	{
		combos[k].type = NOTHING_old;
		combos[k].control = 0;
		combos[k].len = 0;
		//PRINTF(LEVEL,"\n");
	}

	h->hun = game->hun;
	int changed_hun[4];
	int total_change = 0;

	if (HUN_NUMBER(h)>2)
	{
		for (int i = P3; i <= P2; i++)
		{
			if (i != h->hun && h->hands[i] == 3)
			{
				SET_HUN_TO_i(h, i);
				changed_hun[total_change++] = i;
			}
			if (HUN_NUMBER(h) == 2)
				break;
		}
	}

	if (HUN_NUMBER(h)>3)
	{
		for (int i = P2; i >= P3; i--)
		{
			if (i != h->hun && h->hands[i] == 2)
			{
				SET_HUN_TO_i(h, i);
				SET_HUN_TO_i(h, i);
				changed_hun[total_change++] = i;
				changed_hun[total_change++] = i;
				break;
			}
		}
	}

	if (game->hun != -1 && HUN_NUMBER(h)>0)
	{
		COMBO_OF_POKERS *presult, *pNow, *pTmp;
		COMBOS_SUMMARY_DDZ_AI *cur, *result, *tmp;
		presult = game->tmp_combos[2];
		pNow = game->tmp_combos[3];

		memset(presult, 0, sizeof(COMBO_OF_POKERS) * 25);
		memset(pNow, 0, sizeof(COMBO_OF_POKERS) * 25);

		result = &game->tmp_summary[2];
		cur = &game->tmp_summary[3];
		memset(cur, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));
		memset(result, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));

		int numRes = search_combos_in_hands(game, h, presult, result, player);
		int number;

#define CMP_AND_SWAP_SMY()\
    /*print_summary(result);*/\
    /*print_summary(cur);*/\
    if ( cmp_summary(cur,result))\
        {\
        numRes = number;\
        pTmp = presult;\
        presult=pNow;\
        pNow=pTmp;\
        tmp=result;\
        result= cur;\
        cur=tmp;\
        /*print_all_combos(presult, number);*/\
        }

#define SET_HUN_TO_i_REC(h,i)\
    do{\
        SET_HUN_TO_i(h, i);tmp_i=i;\
        }while(0)

		int ben_hun = 0;
		//set first hun values  
		for (int i1 = P3; i1 <= P2; i1++)
		{
			SET_HUN_TO_i(h, i1);
			//      PRINTF(VERBOSE, "==set first hun to %c\n", poker_to_char(i1));
			if (IS_HUN(h, i1)) ben_hun++;
			if (HUN_NUMBER(h)>ben_hun)
			{
				if (game->search_level<1)
				{
					number = search_combos_in_hands(game, h, pNow, cur, player);
					int no_remove = 0;
					for (int combo_itr = 0; combo_itr<cur->series_num; combo_itr++)
					{
						COMBO_OF_POKERS * c = cur->series[combo_itr];
						int extra = c->len - 5;
						extra = min_ddz(extra, 2);
						if (c->low + extra > h->hun) //3456789t
							continue;
						else if (c->low + c->len - 1 - extra < h->hun)
							continue;
						else if (h->hun >= c->low + 5 &&
							h->hun <= (c->low + c->len - 6))
							continue;
						else
							no_remove++;
					}
					if (HUN_NUMBER(h)>(ben_hun + no_remove)) {
						int tmp_i = h->hun;
						if (cur->three_num>0)
							SET_HUN_TO_i_REC(h, cur->three[0]->low);
						else if (cur->three_one_num>0)
							SET_HUN_TO_i_REC(h, cur->three_one[0]->low);
						else if (cur->three_two_num>0)
							SET_HUN_TO_i_REC(h, cur->three_two[0]->low);
						else if (cur->threeseries_num>0)
							SET_HUN_TO_i_REC(h, cur->threeseries[0]->low);
						else if (cur->threeseries_one_num>0)
							SET_HUN_TO_i_REC(h, cur->threeseries_one[0]->low);
						else if (cur->threeseries_two_num>0)
							SET_HUN_TO_i_REC(h, cur->threeseries_two[0]->low);
						else if (cur->pairs_num>0)
							SET_HUN_TO_i_REC(h, cur->pairs[cur->pairs_num - 1]->low);
						else if (cur->singles_num>0)
							SET_HUN_TO_i_REC(h, cur->singles[cur->singles_num - 1]->low);
						if (HUN_NUMBER(h)>(ben_hun + no_remove))
						{
							for (int i2 = P3; i2 <= P2; i2++)
							{
								SET_HUN_TO_i(h, i2);
								if (IS_HUN(h, i2)) ben_hun++;
								number = search_combos_in_hands(game, h, pNow, cur, player);
								CMP_AND_SWAP_SMY()
									if (IS_HUN(h, i2)) ben_hun--;
								SET_i_to_HUN(h, i2);
							}
						}
						else
							number = search_combos_in_hands(game, h, pNow, cur, player);
						CMP_AND_SWAP_SMY()
							SET_i_to_HUN(h, tmp_i);
					}
					if (IS_HUN(h, i1)) ben_hun--;
					SET_i_to_HUN(h, i1);
					continue;
				}
				//set second hun values
				for (int i2 = P3; i2 <= P2; i2++)
				{
					SET_HUN_TO_i(h, i2);
					//                PRINTF(VERBOSE, "\t==set second hun to %c\n", poker_to_char(i2));
					if (IS_HUN(h, i2)) ben_hun++;
					if (HUN_NUMBER(h)>ben_hun)
					{
						number = search_combos_in_hands(game, h, pNow, cur, player);
						// check where is hun
						int no_remove = 0;
						for (int combo_itr = 0; combo_itr<cur->series_num; combo_itr++)
						{
							COMBO_OF_POKERS * c = cur->series[combo_itr];
							int extra = c->len - 5;
							extra = min_ddz(extra, 2);
							if (c->low + extra > h->hun) //3456789t
								continue;
							else if (c->low + c->len - 1 - extra < h->hun)
								continue;
							else if (h->hun >= c->low + 5 &&
								h->hun <= (c->low + c->len - 6))
								continue;
							else
								no_remove++;
						}

						if (HUN_NUMBER(h)>(ben_hun + no_remove)) {
							int tmp_i = h->hun;
							if (cur->three_num>0)
								SET_HUN_TO_i_REC(h, cur->three[0]->low);
							else if (cur->three_one_num>0)
								SET_HUN_TO_i_REC(h, cur->three_one[0]->low);
							else if (cur->three_two_num>0)
								SET_HUN_TO_i_REC(h, cur->three_two[0]->low);
							else if (cur->threeseries_num>0)
								SET_HUN_TO_i_REC(h, cur->threeseries[0]->low);
							else if (cur->threeseries_one_num>0)
								SET_HUN_TO_i_REC(h, cur->threeseries_one[0]->low);
							else if (cur->threeseries_two_num>0)
								SET_HUN_TO_i_REC(h, cur->threeseries_two[0]->low);
							else if (cur->pairs_num>0)
								SET_HUN_TO_i_REC(h, cur->pairs[cur->pairs_num - 1]->low);
							else if (cur->singles_num>0)
								SET_HUN_TO_i_REC(h, cur->singles[cur->singles_num - 1]->low);

							number = search_combos_in_hands(game, h, pNow, cur, player);
							SET_i_to_HUN(h, tmp_i);
						}
						CMP_AND_SWAP_SMY()
					}
					else
					{
						number = search_combos_in_hands(game, h, pNow, cur, player);
						CMP_AND_SWAP_SMY()
					}
					if (IS_HUN(h, i2)) ben_hun--;
					SET_i_to_HUN(h, i2);
				}
			}
			else
			{
				number = search_combos_in_hands(game, h, pNow, cur, player);
				CMP_AND_SWAP_SMY()
			}
			if (IS_HUN(h, i1)) ben_hun--;
			SET_i_to_HUN(h, i1);
		}
		for (int i = 0; i< total_change; i++)
			SET_i_to_HUN(h, changed_hun[i]);


		memmove(combos, presult, numRes * sizeof(COMBO_OF_POKERS));

		for (int i = 0; i< numRes; i++)
		{
			COMBO_OF_POKERS * p = combos + i;
			POKERS * h = player->h;
			if (h->hands[p->low] == 0 && (
				p->type == PAIRS || p->type == SINGLE || p->type == THREE
				|| p->type == THREE_ONE))
			{
				p->low = h->hun;
			}
			else if (p->type == THREE_SERIES)  //no such case!
			{
				//do not check here.           
			}
			else if (p->type >= 3311 || p->type == 531) //no such case!
			{
				for (int i = 0; i<p->len; i++)
				{
					int val = p->three_desc[i];
					if (h->hands[val] == 0)
						p->three_desc[i] = h->hun;
				}
			}
		}
		sort_all_combos(game, player->combos, number, player->summary, player->opps, player->oppDown_num
			, player->oppUp_num, player->lower_control_poker, player);

	}
	else
	{
		int number = search_combos_in_hands(game, h, combos, pcombo_summary, player);
		sort_all_combos(game, player->combos, number, player->summary, player->opps, player->oppDown_num
			, player->oppUp_num, player->lower_control_poker, player);
	}
	player->summary->ctrl.single = single;

	if (level >= VERBOSE)
		print_suit(player);
	//get_pairs(hands_combo);

	return true;
}


//get a single or pair for  three
//and update the summary
int search_1_2_for_a_three(POKERS*h, PLAYER* player, COMBO_OF_POKERS * c)
{
	COMBO_OF_POKERS * p = c;
	player->need_research = 0;
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	if (p->type == THREE_ONE)
	{
		if (player->summary->singles_num>0) {
			c->type = THREE_ONE;
			c->three_desc[0] = player->summary->singles[0]->low;
			remove_combo_poker(h, player->summary->singles[0], NULL);
			s->singles_num--;
			if (s->singles[0]->control != 1)
				s->combo_total_num--;
			else
				s->biggest_num--;
			s->real_total_num--;
			s->singles[0]->type = NOTHING_old;
			memmove(s->singles, s->singles + 1, (s->singles_num) * sizeof(void*));
		}
		else if (player->summary->series_num>0 && s->series[0]->len>5)
		{
			c->type = THREE_ONE;
			c->three_desc[0] = s->series[0]->low;
			s->series[0]->low++;
			s->series[0]->len--;
		}
		else if (player->summary->pairs_num>0)
		{
			c->type = THREE_ONE;
			c->three_desc[0] = s->pairs[0]->low;
			s->pairs[0]->type = SINGLE;
			h->hands[s->pairs[0]->low] -= 1;
			player->need_research = 1;
		}
		else {
			for (int i = h->begin; i <= h->end; i++)
				if (h->hands[i]>0 && i != c->low)
				{
					c->type = THREE_ONE;
					c->three_desc[0] = i;
					h->hands[i]--;
					player->need_research = 1;
					//search_combos_in_suit(game,h,player->opps,player);
					return 1;
				}
			// return 0;
		}
	}

	else if (p->type == THREE_TWO)
	{
		if (player->summary->pairs_num>0)
		{
			c->type = THREE_TWO;
			c->three_desc[0] = s->pairs[0]->low;
			s->pairs[0]->type = NOTHING_old;
			memmove(s->pairs, s->pairs + 1, (s->pairs_num - 1) * sizeof(void*));
		}
		else {
			for (int i = h->begin; i <= max_ddz(P2, h->end); i++)
				if (h->hands[i] >= 2 && i != c->low)
				{
					c->type = THREE_TWO;
					c->three_desc[0] = i;
					h->hands[i] -= 2;
					player->need_research = 1;
					//search_combos_in_suit(game,h,player->opps,player);
					return 1;
				}
			return 0;
		}
	}

	else  if (p->type == 3322)
	{
		if (player->summary->pairs_num>1)
		{
			c->type = 3322;
			c->three_desc[0] = s->pairs[0]->low;
			c->three_desc[1] = s->pairs[1]->low;
			s->pairs[0]->type = NOTHING_old;
			s->pairs[1]->type = NOTHING_old;
			memmove(s->pairs, s->pairs + 2, (s->pairs_num - 2) * sizeof(void*));
		}
		else {
			int j = 0;
			for (int i = h->begin; i <= max_ddz(P2, h->end); i++) {
				if (h->hands[i] >= 2 && i != c->low)
				{
					c->three_desc[j++] = i;
					// h->hands[i]-=2;
					if (j >= 2)
					{
						c->type = 3322;
						player->need_research = 1;
						h->hands[c->three_desc[0]] -= 2;
						h->hands[c->three_desc[1]] -= 2;
						return 1;
					}
				}
			}

			return 0;
		}
	}

	if (p->type == 3311)
	{
		if (player->summary->singles_num>1) {
			c->type = 3311;
			c->three_desc[0] = player->summary->singles[0]->low;
			c->three_desc[1] = player->summary->singles[1]->low;

			remove_combo_poker(h, player->summary->singles[0], NULL);
			remove_combo_poker(h, player->summary->singles[1], NULL);
			s->singles_num -= 2;
			if (s->singles[0]->control != 1)
				s->combo_total_num--;
			else
				s->biggest_num--;
			if (s->singles[1]->control != 1)

				s->combo_total_num--;
			else
				s->biggest_num--;

			s->real_total_num -= 2;
			s->singles[0]->type = NOTHING_old;
			s->singles[1]->type = NOTHING_old;
			memmove(s->singles, s->singles + 2, (s->singles_num) * sizeof(void*));
		}
		else if (player->summary->pairs_num>0)
		{
			c->type = 3311;
			c->three_desc[0] = s->pairs[0]->low;
			c->three_desc[1] = s->pairs[0]->low;
			s->pairs[0]->type = NOTHING_old;
			memmove(s->pairs, s->pairs + 1, (s->pairs_num - 1) * sizeof(void*));
		}
		else {
			int j = 0;
			for (int i = h->begin; i <= h->end; i++)
				if (h->hands[i]>0)
				{
					c->type = 3311;
					c->three_desc[j++] = i;
					h->hands[i]--;
					if (j == 2)
						//          {
						break;
					//          }
				}
			player->need_research = 1;
			//            search_combos_in_suit(game,h,player->opps,player);
			return 0;
		}
	}
	return 1;
}

int get_combo_num_for_player_win(PLAYER* pl)
{
	//todo:refine it
	return CONTROL_SUB_COMBO_NUM(pl);
}

int is_player_ready_win(GAME_DDZ_AI* game, PLAYER* pl)
{
	//todo:refine it
	return CONTROL_SUB_COMBO_NUM_IS_GOOD(pl);
}

int rand_a_poker(POKERS *all)
{
	int k;
	while (1)
	{
		k = rand() % MAX_POKER_KIND;
		if (all->hands[k]>0)
		{
			all->hands[k]--;
			return k;
		}
	}
}

void game_init(GAME_DDZ_AI* game)
{
	//        game->players[CUR_PLAYER]->id = game->player[CUR_PLAYER].type;
	//      game->players[UP_PLAYER]->id = game->player[UP_PLAYER].type;
	//      game->players[DOWN_PLAYER]->id = game->player[DOWN_PLAYER].type;

	game->players[CUR_PLAYER]->lower_control_poker = P2;//get_lowest_controls(&game->all, CONTROL_POKER_NUM);

														//    game->player[CUR_PLAYER].cur = &game->player[CUR_PLAYER].curcomb;
														//        game->player[UP_PLAYER].cur = &game->player[UP_PLAYER].curcomb;
														//    game->player[DOWN_PLAYER].cur = &game->player[DOWN_PLAYER].curcomb;

	game->computer[0] = 1;
	game->computer[1] = 1;
	game->computer[2] = 1;

	//search and initialize Suits
	search_combos_in_suit(game, game->players[CUR_PLAYER]->h, game->players[CUR_PLAYER]->opps, game->players[CUR_PLAYER]);
	if (game->known_others_poker)
	{
		search_combos_in_suit(game, game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->opps, game->players[DOWN_PLAYER]);
		search_combos_in_suit(game, game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->opps, game->players[UP_PLAYER]);
	}
	game->good_farmer_0 = 50;
	game->good_farmer_1 = 20;
}

//search in poker h to find a combo c bigger than a
int find_a_bigger_combo_in_hands_hun(POKERS* h, COMBO_OF_POKERS * c, COMBO_OF_POKERS* a)
{
	c->control = 0;
	if (h->total<get_combo_number(a))
	{
		goto _find_bomb;
	}

	switch (a->type)
	{
	case ROCKET:
		;
		return false;
	case BOMB_old:
		if (getBigBomb_hun(h, c, a))
		{
			return true;
		}
		return false;
	case SINGLE_SERIES:
		if (getBigSeries_hun(h, c, a->low + 1, CEIL_A(h->end), 1, a->len)) //todo: fix me
		{
			return true;
		}
		break;

	case PAIRS_SERIES:
		if (getBigSeries_hun(h, c, a->low + 1, CEIL_A(h->end), 2, a->len)) //todo: fix me
		{
			return true;
		}
		break;
	case THREE_SERIES:
		if (getBigSeries_hun(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			return true;
		}
		break;
	case 3311: //todo fixme
		c->len = a->len;
		if (a->control == 2) //search a big attach for three
							 // fix me!
		{
			for (int i = a->three_desc[1] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low && i != a->low + 1
					&& i != h->hun)
				{
					c->type = 3311;
					c->low = a->low;
					c->three_desc[0] = a->three_desc[0];
					c->three_desc[1] = i;
					return true;
				}
			}

			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low && i != a->low + 1 && i != h->hun)
				{
					c->type = 3311;
					c->low = a->low;
					c->three_desc[0] = i;
					if (h->hands[i] >1)
					{
						c->three_desc[1] = i;
						return true;
					}

					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >0 && j != a->low && j != a->low + 1 && j != h->hun)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}

		}

		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			for (int i = h->begin; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != c->low && i != c->low + 1 && i != h->hun)
				{
					c->type = 3311;
					//c->low  = a->low;
					c->three_desc[0] = i;
					/*  if (h->hands[i] >1)
					{
					c->three_desc[1] = i;
					return true;
					}
					*/
					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >0 && j != c->low && j != c->low + 1 && j != h->hun)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}
		}
		return false;
		break;
	case 333111:
	case 33331111:
	case 531:
		return false;
		if (getBigSeries_hun(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			return true;
		}
		break;
	case 3322:
		c->len = a->len;
		c->type = 3322;
		c->low = a->low;

		if (a->control == 2) //search a big attach for three
		{
			for (int i = a->three_desc[1] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != a->low && i != a->low + 1 && i != h->hun)
				{
					c->three_desc[0] = a->three_desc[0];
					c->three_desc[1] = i;
					return true;
				}
			}

			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != a->low && i != a->low + 1 && i != h->hun)
				{
					c->three_desc[0] = i;
					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >1 && j != a->low && j != a->low + 1 && j != h->hun)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}

		}

		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			for (int i = h->begin; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != c->low && i != c->low + 1 && i != h->hun)
				{
					c->type = 3322;
					//                         c->low  = a->low;
					c->three_desc[0] = i;

					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >1 && j != c->low && j != c->low + 1 && j != h->hun)
						{
							c->three_desc[1] = j;
							return true;
						}
					}
				}
			}
		}
		return false;
	case 333222:
	case 33332222:
		return false;
		if (getBigSeries_hun(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			return true;
		}
		break;
	case 31:
	{
		if (a->control == 2) //search a big attach for three
		{
			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low  && i != h->hun)
				{
					c->type = THREE_ONE;
					c->low = a->low;
					c->three_desc[0] = i;
					c->len = 1;
					return true;
				}
			}
		}

		if (getBigSingle_hun(h, c, a->low + 1, P2, 3))
		{
			if (c->low == h->hun)
				return false;
			c->type = THREE_ONE;
			c->three_desc[0] = -1;
			for (int j = h->begin; j <= h->end; j++)
			{
				if (h->hands[j] >0 && j != c->low && j != h->hun) {
					c->three_desc[0] = j;
					c->len = 1;
					return true;
				}
			}
		}
		return false;
	}
	break;
	case 32:
	{
		if (a->control == 2) //search a big attach for three
		{
			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >= 2 && i != a->low && i != h->hun)//todo fixme!
				{
					c->type = 32;
					c->low = a->low;
					c->three_desc[0] = i;
					c->len = 1;
					return true;
				}
			}
		}

		if (getBigSingle_hun(h, c, a->low + 1, P2, 3))
		{
			if (c->low == h->hun)
				return false;
			c->type = THREE_TWO;
			c->three_desc[0] = -1;
			for (int j = h->begin; j <= h->end; j++)
			{
				if (h->hands[j] >= 2 && j != c->low && j != h->hun) {
					c->three_desc[0] = j;
					c->len = 1;
					return true;
				}
			}
		}
		return false;
	}
	break;
	case THREE:
		if (getBigSingle_hun(h, c, a->low + 1, min_ddz(h->end, P2), 3)) //todo: fix me
		{
			return true;
		}
		break;
	case PAIRS:
		if (getBigSingle_hun(h, c, a->low + 1, min_ddz(h->end, P2), 2)) //todo: fix me
		{
			return true;
		}
		break;
	case SINGLE:
		if (getBigSingle_hun(h, c, a->low + 1, h->end, 1)) //todo: fix me
		{
			return true;
		}
	default:
		break;
	}

_find_bomb:
	///*
	if (a->type != BOMB_old)
	{
		return getBomb_hun(h, c);
	}
	//*/
	return false;
}
//search in poker h to find a combo c bigger than a
int find_a_bigger_combo_in_hands(POKERS* h, COMBO_OF_POKERS * c, COMBO_OF_POKERS* a)
{
	c->control = 0;
	if (h->hun != -1)
	{
		return find_a_bigger_combo_in_hands_hun(h, c, a);
	}
	//= player->h;
	if (h->total<get_combo_number(a))
	{
		goto _find_bomb;
	}

	switch (a->type)
	{
	case ROCKET:
		;
		return false;
	case BOMB_old:
		if (getBigBomb(h, c, a))
		{
			return true;
		}
		return false;
	case SINGLE_SERIES:
		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 1, a->len)) //todo: fix me
		{
			return true;
		}
		break;

	case PAIRS_SERIES:
		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 2, a->len)) //todo: fix me
		{
			return true;
		}
		break;
	case THREE_SERIES:
		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			return true;
		}
		break;
	case 3311:
		c->len = a->len;
		if (h->hands[a->low] == 3) //search a big attach for three
		{
			for (int i = a->three_desc[1] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low && i != a->low + 1)
				{
					c->type = 3311;
					c->low = a->low;
					c->three_desc[0] = a->three_desc[0];
					c->three_desc[1] = i;
					return true;
				}
			}

			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low && i != a->low + 1)
				{
					c->type = 3311;
					c->low = a->low;
					c->three_desc[0] = i;
					/*if (h->hands[i] >1)
					{
					c->three_desc[1] = i;
					return true;
					}*/

					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >0 && j != a->low && j != a->low + 1)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}

		}

		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			for (int i = h->begin; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != c->low && i != c->low + 1)
				{
					c->type = 3311;
					//c->low  = a->low;
					c->three_desc[0] = i;
					/*if (h->hands[i] >1)
					{
					c->three_desc[1] = i;
					return true;
					}*/

					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >0 && j != c->low && j != c->low + 1)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}
		}
		break;
	case 333111:
	case 33331111:
	case 531:
		break;
	case 3322:
		c->len = a->len;
		c->type = 3322;
		c->low = a->low;

		if (h->hands[a->low] == 3) //search a big attach for three
		{
			for (int i = a->three_desc[1] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != a->low && i != a->low + 1)
				{
					c->three_desc[0] = a->three_desc[0];
					c->three_desc[1] = i;
					return true;
				}
			}

			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != a->low && i != a->low + 1)
				{
					c->three_desc[0] = i;
					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >1 && j != a->low && j != a->low + 1)
						{
							c->three_desc[1] = j;
							return true;
						}
					}

				}
			}

		}

		if (getBigSeries(h, c, a->low + 1, CEIL_A(h->end), 3, a->len))//todo: fix me
		{
			for (int i = h->begin; i <= h->end; i++)
			{
				if (h->hands[i] >1 && i != c->low && i != c->low + 1)
				{
					c->type = 3322;
					//                         c->low  = a->low;
					c->three_desc[0] = i;

					for (int j = i + 1; j <= h->end; j++)
					{

						if (h->hands[j] >1 && j != c->low && j != c->low + 1)
						{
							c->three_desc[1] = j;
							return true;
						}
					}
				}
			}
		}
		break;
	case 333222:
	case 33332222:
		break;
	case 31:
	{
		if (h->hands[a->low] == 3) //search a big attach for three
		{
			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >0 && i != a->low)
				{
					c->type = THREE_ONE;
					c->low = a->low;
					c->three_desc[0] = i;
					c->len = 1;
					return true;
				}
			}
		}

		for (int i = a->low + 1; i <= h->end; i++)
		{
			if (h->hands[i] == 3) //Got Three,doesn't check bomb..
			{
				c->type = THREE_ONE;
				c->low = i;
				c->three_desc[0] = -1;
				for (int j = h->begin; j <= h->end; j++)
				{
					if (h->hands[j] >0 && j != c->low) {
						c->three_desc[0] = j;
						c->len = 1;
						return true;
					}
				}
			}
		}
	}
	break;
	case 32:
	{
		if (h->hands[a->low] == 3) //search a big attach for three
		{
			for (int i = a->three_desc[0] + 1; i <= h->end; i++)
			{
				if (h->hands[i] >= 2 && i != a->low)
				{
					c->type = 32;
					c->low = a->low;
					c->three_desc[0] = i;
					c->len = 1;
					return true;
				}
			}
		}

		for (int i = a->low + 1; i <= h->end; i++)
		{
			if (h->hands[i] == 3) //Got Three,doesn't check bomb..
			{
				c->type = 32;
				c->low = i;
				for (int j = h->begin; j <= h->end; j++)
				{
					if (h->hands[j] >= 2 && j != c->low)
					{
						c->three_desc[0] = j;
						c->len = 1;
						return true;
					}
				}
			}
		}
	}
	break;
	case THREE:
		if (getBigThree(h, c, a)) //todo: fix me
		{
			return true;
		}
		break;
	case PAIRS:
		if (getBigSingle(h, c, a->low + 1, h->end, 2)) //todo: fix me
		{
			return true;
		}
		break;
	case SINGLE:
		if (getBigSingle(h, c, a->low + 1, h->end, 1)) //todo: fix me
		{
			return true;
		}
	default:
		break;
	}

_find_bomb:
	///*
	if (a->type != BOMB_old)
	{
		for (int i = h->begin; i <= min_ddz(P2, h->end); i++)
		{
			if (h->hands[i] == 4)
			{
				c->type = BOMB_old;
				c->low = i;
				return true;
			}
		}
		if (h->hands[BIG_JOKER] && h->hands[LIT_JOKER])
		{
			c->type = ROCKET;
			c->low = LIT_JOKER;
			return true;
		}
	}
    else
    {
        return getBigBomb(h, c, a);
    }
	//*/
	return false;
}

int has_combo_bigger_than_c_in_hands(POKERS * h, COMBO_OF_POKERS * c)
{
	COMBO_OF_POKERS t;
	return find_a_bigger_combo_in_hands(h, &t, c);
}

int find_a_smaller_combo_in_hands_hun(POKERS* h, COMBO_OF_POKERS * c, COMBO_OF_POKERS* a)
{
	//= player->h;
	if (h->total<get_combo_number(a))
		return false;
	switch (a->type)
	{
	case ROCKET:
		;
		return false;
	case BOMB_old:
		return false;
	case SINGLE_SERIES:
		if (getSmallSeries_hun(h, c, a->low, 1, a->len))
		{
			return true;
		}
		break;

	case PAIRS_SERIES:

	case THREE_SERIES:

	case 3311:
	case 333111:
	case 33331111:
	case 531:

	case 3322:
	case 333222:
	case 33332222:
		return false;
	case 31:
	case 32:
	case THREE:
		if (getSmallSingle_hun(h, c, a->low, 3))
		{
			return true;
		}
		break;
	case PAIRS:
		if (getSmallSingle_hun(h, c, a->low, 2))
		{
			return true;
		}
		break;
	case SINGLE:
		if (getSmallSingle_hun(h, c, a->low, 1))
		{
			return true;
		}
	default:
		break;
	}
	return false;
}


//search in poker h to find a  combo c smaller than a
int find_a_smaller_combo_in_hands(POKERS* h, COMBO_OF_POKERS * c, COMBO_OF_POKERS* a)
{
	if (h->hun != -1)
		return find_a_smaller_combo_in_hands_hun(h, c, a);
	//= player->h;
	if (h->total<get_combo_number(a))
		return false;
	switch (a->type)
	{
	case ROCKET:
		;
		return false;
	case BOMB_old:
		return false;
	case SINGLE_SERIES:
		if (getSmallSeries(h, c, a->low, 1, a->len))
		{
			return true;
		}
		break;

	case PAIRS_SERIES:

	case THREE_SERIES:

	case 3311:
	case 333111:
	case 33331111:
	case 531:

	case 3322:
	case 333222:
	case 33332222:
		return false;
	case 31:
	case 32:
	case THREE:
		if (getSmallSingle(h, c, a->low, 3))
		{
			return true;
		}
		break;
	case PAIRS:
		if (getSmallSingle(h, c, a->low, 2))
		{
			return true;
		}
		break;
	case SINGLE:
		if (getSmallSingle(h, c, a->low, 1))
		{
			return true;
		}
	default:
		break;
	}
	return false;
}


//a stupid fucntion...
//select  a combo c from player->h, but not remove
//arrange the other poker in player->h to combos and save summary to player->summary
int rearrange_suit(GAME_DDZ_AI* game, PLAYER *player, COMBO_OF_POKERS* c)
{
	POKERS t;
	DBG(PRINTF(LEVEL, "  Chaipai  \n"));
	memmove(&t, player->h, sizeof(POKERS));
	int res = remove_combo_poker(&t, c, NULL);
	search_combos_in_suit(game, &t, player->opps, player);
	return res;
}

// Remove combo a from the player s
// return true if success
int remove_combo_in_suit(COMBOS_SUMMARY_DDZ_AI *s, COMBO_OF_POKERS* a)
{
	//   COMBOS_SUMMARY_DDZ_AI *s =player->summary;
	switch (a->type)
	{
	case ROCKET:
		//s->singles[s->series_num]=&c[k];
		//s->rocket_num++;
		//break;
	case BOMB_old:
		if (s->bomb_num>0)
		{
			for (int k = 0; k<s->bomb_num; k++)
			{
				if (s->bomb[k]->low == a->low) {
					//memmove(c,s->bomb[k],sizeof(COMBO_OF_POKERS));
					if (k != s->bomb_num - 1)
						memmove(&s->bomb[k], &s->bomb[k + 1], (s->bomb_num - k) * sizeof(void*));
					s->bomb_num--;
					return true;

				}
			}
		}
		break;
	case SINGLE_SERIES:
		if (s->series_num>0)
		{
			for (int k = 0; k<s->series_num; k++)
			{
				if (s->series[k]->low == a->low && s->series[k]->len == a->len) {
					// memmove(c,s->series[k],sizeof(COMBO_OF_POKERS));
					if (k != s->series_num - 1)
						memmove(&s->series[k], &s->series[k + 1], (s->series_num - k) * sizeof(void*));
					s->series_num--;
					s->series_detail[a->len - 5]--;
					return true;

				}
			}
		}
		break;
	case PAIRS_SERIES:
		if (s->pairs_series_num>0)
		{
			for (int k = 0; k<s->pairs_series_num; k++)
			{
				if (s->pairs_series[k]->low == a->low && s->pairs_series[k]->len == a->len) {
					// memmove(c,s->series[k],sizeof(COMBO_OF_POKERS));
					if (k != s->pairs_series_num - 1)
						memmove(&s->pairs_series[k], &s->pairs_series[k + 1], (s->pairs_series_num - k) * sizeof(void*));
					s->pairs_series_num--;
					//s->series_detail[a->len-5]--;
					return true;

				}
			}
		}
		break;

	case THREE_SERIES:
		if (s->threeseries_num>0)
		{
			for (int k = 0; k<s->threeseries_num; k++)
			{
				if (s->threeseries[k]->low == a->low && s->threeseries[k]->len == a->len) {
					// memmove(c,s->series[k],sizeof(COMBO_OF_POKERS));
					if (k != s->threeseries_num - 1)
						memmove(&s->threeseries[k], &s->threeseries[k + 1], (s->threeseries_num - k) * sizeof(void*));
					s->threeseries_num--;
					//s->series_detail[a->len-5]--;
					return true;

				}
			}
		}
		break;
	case THREE:
		for (int k = 0; k<s->three_num; k++)
		{
			if (s->three[k]->low == a->low) {
				// memmove(c,s->series[k],sizeof(COMBO_OF_POKERS));
				if (k != s->three_num - 1)
					memmove(&s->three[k], &s->three[k + 1], (s->three_num - k) * sizeof(void*));
				s->three_num--;
				return true;

			}
		}
		break;

		break;
	case 31:
		if (s->three_one_num>0)
		{
			for (int k = 0; k<s->three_one_num; k++)
			{
				if (s->three_one[k]->low == a->low) {
					// memmove(c,s->three_one[k],sizeof(COMBO_OF_POKERS));
					if (k != s->three_one_num - 1)
						memmove(&s->three_one[k], &s->three_one[k + 1], (s->three_one_num - k) * sizeof(void*));
					s->three_one_num--;
					return true;

				}
			}
		}
		break;
	case 32:
		if (s->three_two_num>0)
		{
			for (int k = 0; k<s->three_two_num; k++)
			{
				if (s->three_two[k]->low == a->low) {
					//  memmove(c,s->three_two[k],sizeof(COMBO_OF_POKERS));
					if (k != s->three_two_num - 1)
						memmove(&s->three_two[k], &s->three_two[k + 1], (s->three_two_num - k) * sizeof(void*));
					s->three_two_num--;
					return true;

				}
			}
		}
		break;
	case PAIRS:
		if (s->pairs_num>0)
		{
			for (int k = 0; k<s->pairs_num; k++)
			{
				if (s->pairs[k]->low == a->low) {
					//  memmove(c,s->pairs[k],sizeof(COMBO_OF_POKERS));
					if (k != s->pairs_num - 1)
						memmove(&s->pairs[k], &s->pairs[k + 1], (s->pairs_num - k) * sizeof(void*));
					s->pairs_num--;
					return true;
				}
			}
		}
		break;
	case SINGLE:
		if (s->singles_num>0)
		{
			for (int k = 0; k<s->singles_num; k++)
			{
				if (s->singles[k]->low == a->low) {
					//memmove(c,s->singles[k],sizeof(COMBO_OF_POKERS));
					if (k != s->singles_num - 1)
						memmove(&s->singles[k], &s->singles[k + 1], (s->singles_num - k) * sizeof(void*));
					s->singles_num--;
					return true;
				}
			}
		}
		break;
	case 3311:
	case 333111:
	case 33331111:
	case 531:
		if (s->threeseries_one_num>0)
		{
			for (int k = 0; k<s->threeseries_one_num; k++)
			{
				if (s->threeseries_one[k]->low == a->low) {
					//memmove(c,s->singles[k],sizeof(COMBO_OF_POKERS));
					if (k != s->threeseries_one_num - 1)
						memmove(&s->threeseries_one[k], &s->threeseries_one[k + 1], (s->threeseries_one_num - k) * sizeof(void*));
					s->threeseries_one_num--;
					return true;
				}
			}
		}
		break;
	case 3322:
	case 333222:
	case 33332222:
		if (s->threeseries_two_num>0)
		{
			for (int k = 0; k<s->threeseries_two_num; k++)
			{
				if (s->threeseries_two[k]->low == a->low) {
					//memmove(c,s->singles[k],sizeof(COMBO_OF_POKERS));
					if (k != s->threeseries_two_num - 1)
						memmove(&s->threeseries_two[k], &s->threeseries_two[k + 1], (s->threeseries_two_num - k) * sizeof(void*));
					s->threeseries_two_num--;
					return true;
				}
			}
		}
		break;
	case 411:
	{
		for (int k = 0; k<s->four_one_num; k++)
		{
			if (s->four_one[k]->low == a->low) {
				//memmove(c,s->singles[k],sizeof(COMBO_OF_POKERS));
				if (k != s->four_one_num - 1)
					memmove(&s->four_one[k], &s->four_one[k + 1], (s->four_one_num - k) * sizeof(void*));
				s->four_one_num--;
				return true;
			}
		}
	}
	break;

	case 422:
	{
		for (int k = 0; k<s->four_two_num; k++)
		{
			if (s->four_two[k]->low == a->low) {
				//memmove(c,s->singles[k],sizeof(COMBO_OF_POKERS));
				if (k != s->four_one_num - 1)
					memmove(&s->four_two[k], &s->four_two[k + 1], (s->four_two_num - k) * sizeof(void*));
				s->four_two_num--;
				return true;
			}
		}
	}
	break;

	default:
		PRINTF_ALWAYS("!!ERR,in line %d :  no such combos in player\n", __LINE__);
	}
	return false;
}

//save all combos to in order..
COMBO_OF_POKERS** find_all_combo_bigger_than_combo_a(COMBOS_SUMMARY_DDZ_AI *s,
	COMBO_OF_POKERS* a, int* number)//find all c > a in s
{
	//   COMBOS_SUMMARY_DDZ_AI *s =player->summary;
	*number = 0;
	switch (a->type)
	{
	case ROCKET:
		//s->singles[s->series_num]=&c[k];
		//s->rocket_num++;
		//break;
		return NULL;
	case BOMB_old:
		//s->bomb[s->bomb_num++]=;
		if (s->bomb_num>0)
		{
			for (int k = 0; k<s->bomb_num; k++)
			{
				if (s->bomb[k]->low > a->low) {
					*number = s->bomb_num - k;
					return &s->bomb[k];
				}
			}
		}
		break;
	case SINGLE_SERIES:
		if (s->series_num>0)
		{
			for (int k = 0; k<s->series_num; k++)
			{
				if (s->series[k]->low > a->low && s->series[k]->len == a->len) {
					*number = s->series_num - k;
					return &s->series[k];
				}
			}
		}
		break;
	case PAIRS_SERIES:
		if (s->pairs_series_num>0)
		{
			for (int k = 0; k<s->pairs_series_num; k++)
			{
				if (s->pairs_series[k]->low > a->low && s->pairs_series[k]->len == a->len) {
					*number = s->pairs_series_num - k;
					return &s->pairs_series[k];
				}
			}
		}
		break;
	case THREE_SERIES:
		if (s->threeseries_num>0)
		{
			for (int k = 0; k<s->threeseries_num; k++)
			{
				if (s->threeseries[k]->low > a->low && s->threeseries[k]->len == a->len) {
					*number = s->threeseries_num - k;
					return &s->threeseries[k];
				}
			}
		}
		break;
		break;
	case THREE:
		if (s->three_num>0)
		{
			for (int k = 0; k<s->three_num; k++)
			{
				if (s->three[k]->low > a->low && s->three[k]->len == a->len) {
					*number = s->three_num - k;
					return &s->three[k];
				}
			}
		}
		break;
	case 31:
		if (s->three_one_num>0)
		{
			for (int k = 0; k<s->three_one_num; k++)
			{
				if (s->three_one[k]->low > a->low) {
					*number = s->three_one_num - k;
					return &s->three_one[k];
				}
			}
		}
		break;
	case 32:
		if (s->three_two_num>0)
		{
			for (int k = 0; k<s->three_two_num; k++)
			{
				if (s->three_two[k]->low > a->low) {
					*number = s->three_two_num - k;
					return &s->three_two[k];
				}
			}
		}
		break;
	case PAIRS:
		if (s->pairs_num>0)
		{
			for (int k = 0; k<s->pairs_num; k++)
			{
				if (s->pairs[k]->low > a->low) {
					*number = s->pairs_num - k;
					return &s->pairs[k];
				}
			}
		}
		break;
	case SINGLE:
		if (s->singles_num>0)
		{
			for (int k = 0; k<s->singles_num; k++)
			{
				if (s->singles[k]->low > a->low) {
					*number = s->singles_num - k;
					return &s->singles[k];
				}
			}
		}
		break;
	case 3311:
	case 333111:
	case 33331111:
	case 531:
		if (s->threeseries_one_num>0)
		{
			for (int k = 0; k<s->threeseries_one_num; k++)
			{
				if (s->threeseries_one[k]->low > a->low && s->threeseries_one[k]->len == a->len) {
					*number = s->threeseries_one_num - k;
					return &s->threeseries_one[k];
				}
			}
		}
		break;
	case 3322:
	case 333222:
	case 33332222:
		if (s->threeseries_one_num>0)
		{
			for (int k = 0; k<s->threeseries_two_num; k++)
			{
				if (s->threeseries_two[k]->low > a->low && s->threeseries_two[k]->len == a->len) {
					*number = s->threeseries_two_num - k;
					return &s->threeseries_two[k];
				}
			}
		}
		break;

	}
	/*
	if (a->type!=BOMB_old && s->bomb_num>0)
	{
	for (int k=0; k<s->bomb_num; k++)
	{
	*number=1;
	return &s->bomb[k];
	}
	}
	*/
	return NULL;
}

COMBO_OF_POKERS* find_combo(COMBOS_SUMMARY_DDZ_AI *s, COMBO_OF_POKERS* a)//find c > a in s and return c.
{
	//   COMBOS_SUMMARY_DDZ_AI *s =player->summary;
	switch (a->type)
	{
	case ROCKET:
		//s->singles[s->series_num]=&c[k];
		//s->rocket_num++;
		//break;
		return NULL;
	case BOMB_old:
		//s->bomb[s->bomb_num++]=;
		if (s->bomb_num>0)
		{
			for (int k = 0; k<s->bomb_num; k++)
			{
				if (check_combo_a_Big_than_b(s->bomb[k], a)) {
					return s->bomb[k];
				}
			}
		}
		break;
	case SINGLE_SERIES:
		if (s->series_num>0)
		{
			for (int k = 0; k<s->series_num; k++)
			{
				if (s->series[k]->low > a->low && s->series[k]->len == a->len) {
					return s->series[k];
				}
			}
		}
		break;
	case PAIRS_SERIES:
		if (s->pairs_series_num>0)
		{
			for (int k = 0; k<s->pairs_series_num; k++)
			{
				if (s->pairs_series[k]->low > a->low && s->pairs_series[k]->len == a->len) {
					return s->pairs_series[k];
				}
			}
		}
		break;
	case THREE_SERIES:
		if (s->threeseries_num>0)
		{
			for (int k = 0; k<s->threeseries_num; k++)
			{
				if (s->threeseries[k]->low > a->low && s->threeseries[k]->len == a->len) {
					return s->threeseries[k];
				}
			}
		}
		break;
	case THREE:
		if (s->three_num>0)
		{
			for (int k = 0; k<s->three_num; k++)
			{
				if (s->three[k]->low > a->low && s->three[k]->len == a->len) {
					return s->three[k];
				}
			}
		}
		break;
	case 31:
		if (s->three_one_num>0)
		{
			for (int k = 0; k<s->three_one_num; k++)
			{
				if (s->three_one[k]->low > a->low) {
					return s->three_one[k];
				}
			}
		}
		break;
	case 32:
		if (s->three_two_num>0)
		{
			for (int k = 0; k<s->three_two_num; k++)
			{
				if (s->three_two[k]->low > a->low) {
					return s->three_two[k];
				}
			}
		}
		break;
	case PAIRS:
		if (s->pairs_num>0)
		{
			for (int k = 0; k<s->pairs_num; k++)
			{
				if (s->pairs[k]->low > a->low) {
					return s->pairs[k];
				}
			}
		}
		break;
	case SINGLE:
		if (s->singles_num>0)
		{
			for (int k = 0; k<s->singles_num; k++)
			{
				if (s->singles[k]->low > a->low) {
					return s->singles[k];
				}
			}
		}
		break;
	case 3311:
	case 333111:
	case 33331111:
	case 531:
		if (s->threeseries_one_num>0)
		{
			for (int k = 0; k<s->threeseries_one_num; k++)
			{
				if (s->threeseries_one[k]->low > a->low && s->threeseries_one[k]->len == a->len) {
					return s->threeseries_one[k];
				}
			}
		}
		break;
	case 3322:
	case 333222:
	case 33332222:
		if (s->threeseries_one_num>0)
		{
			for (int k = 0; k<s->threeseries_two_num; k++)
			{
				if (s->threeseries_two[k]->low > a->low && s->threeseries_two[k]->len == a->len) {
					return s->threeseries_two[k];
				}
			}
		}
		break;

	}
	if (a->type != BOMB_old && s->bomb_num>0)
	{
		for (int k = 0; k<s->bomb_num; k++)
		{
			return s->bomb[k];
		}
	}
	return NULL;
}

int has_control_poker(COMBO_OF_POKERS* c, int lower)
{
	if (c->type == BOMB_old || c->type == ROCKET)
		return c->low;
	if (c->type == PAIRS || c->type == SINGLE)
		c->len = 1;
	if (c->len == 0)
	{
		//  PRINTF_ALWAYS("ERR: len of type is zero..! type %d\n",c->type);
		c->len = 1;
	}
	if (c->low + c->len - 1 >= lower) //for theries , pairs, single
		return c->low + c->len - 1;
	if (c->type == THREE_ONE || c->type == THREE_TWO)
	{
		if (c->three_desc[0] >= lower)
			return c->three_desc[0];
	}
	if (c->type >= 3311) //plane , trick..
	{
		if (c->three_desc[0] >= lower || c->three_desc[1] >= lower)
			return max_ddz(c->three_desc[0], c->three_desc[1]);
	}
	return -1;
}

//remove 1 or 2 pokers from opps hand
void assume_remove_controls_in_opps(POKERS * opps, COMBO_OF_POKERS * c, int begin)
{
	//     ASSERT(begin==c->low);
	if (c->type == SINGLE) {
		for (int k = begin + 1; k <= opps->end; k++)
			if (opps->hands[k]>0)
			{
				opps->hands[k]--;
				break;
			}
	}
	else if (c->type == PAIRS)
	{
		for (int k = begin + 1; k <= opps->end; k++)
			if (opps->hands[k] >= 2)
			{
				opps->hands[k] -= 2;
				break;
			}
		//    if (  opps->hands[LIT_JOKER]==1 &&opps->hands[BIG_JOKER]==1 )
		//            opps->hands[LIT_JOKER]=opps->hands[BIG_JOKER]=0;

	}
}

int find_the_biggest_combo_great_pre(GAME_DDZ_AI* game, COMBO_OF_POKERS* pre, COMBO_OF_POKERS * res, bool searchBombFirst = true)

{
	COMBOS_SUMMARY_DDZ_AI * summary = game->players[CUR_PLAYER]->summary;

	//todo: add more check here if other has bombs
	if (searchBombFirst && summary->bomb_num>0) //
	{
		int i = 0;
		for (; i<summary->bomb_num; i++)
			if (check_combo_a_Big_than_b(summary->bomb[i], pre))
			{
				*res = *summary->bomb[i];
				return 1;
			}
	}
	else
	{

		COMBO_OF_POKERS *p, *c, tmp, tmp2;
		c = &tmp;
		p = &tmp2;
		*p = *pre;
		int find = 0;
        
        while ((c = find_combo(game->players[CUR_PLAYER]->summary, p)) != NULL) // 先找套牌里的
        {
            *res = *c;
            *p = *c;
            find = 1;
            if (is_combo_biggest_sure(game, c))
            {
                break;
            }
        }

        c = &tmp;
		while (find_a_bigger_combo_in_hands(game->players[CUR_PLAYER]->h, c, p))
		{
			*res = *c;
			*p = *c;
			find = 1;
            if (is_combo_biggest_sure(game, res))
            {
                break;
            }
		}

		return find;
	}
	return 0;
}


int check_only_2_same_combo(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	SET_PLAYER_POINT(game);
	//    PLAYER *pl=game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI * s = pl->summary;
	if (s->real_total_num == 2 && s->combo_typenum == 1)
	{
		//two single!
		if (s->singles_num == 2)
		{
			if ((pl->id == UPFARMER_old && pl->oppDown_num == 1)
				|| (pl->id == DOWNFARMER_old && pl->oppUp_num == 1))
			{
				*cur = *s->singles[1];
				return 1;
			}

			if ((s->singles[1]->low > pl->opps->end)
				|| (game->known_others_poker && pl->id == UPFARMER_old && s->singles[1]->low > downPl->opps->end)
				|| (game->known_others_poker &&  pl->id == DOWNFARMER_old && s->singles[1]->low > upPl->opps->end)
				)
			{
				*cur = *s->singles[0];
				return 1;
			}
		}
		//two single!
		if (s->pairs_num == 2)
		{
			*cur = *s->pairs[0];
			if (s->pairs[1]->low == P2)
			{
				return 1;
			}
			int  res = 1;
			for (int i = max_ddz(pl->opps->begin, s->pairs[1]->low + 1); i <= P2; i++)
			{
				if (pl->opps->hands[i] >= 2)
					res = 0;
			}
			if (game->known_others_poker)
			{
				COMBO_OF_POKERS p = *s->pairs[1];
				p.low--;
				if (is_combo_biggest(game, pl->opps,
					&p, 2, 2, 2))
				{
					res = 1;
				}
			}
			return res;
		}

		if (s->three_one_num == 2)
		{
			*cur = *s->three_one[0];
			if (game->known_others_poker)
			{
				COMBO_OF_POKERS p = *s->three_one[1];
				p.low--;
				if (is_combo_biggest(game, pl->opps,
					&p, 4, 4, 4))
				{
					return 1;
				}
			}
			else
			{
				return 1;
			}
			return 0;
		}

		if (s->three_two_num == 2)
		{
			*cur = *s->three_two[0];
			if (game->known_others_poker)
			{
				COMBO_OF_POKERS p = *s->three_two[1];
				p.low--;
				if (is_combo_biggest(game, pl->opps,
					&p, 5, 5, 5))
				{
					return 1;
				}
			}
			else
			{
				return 1;
			}
			return 0;
		}

		if (s->series_num == 2)
		{
			*cur = *s->series[0];
			if (game->known_others_poker)
			{
				COMBO_OF_POKERS p = *s->series[1];
				p.low--;
				if (is_combo_biggest(game, pl->opps,
					&p, 13, 13, 13))
				{
					return 1;
				}
			}
			return 0;
		}

	}
	return 0;
}

//最简单必胜情况，只有一手小牌，其他牌都是最大
//
COMBO_OF_POKERS* check_for_win_now(GAME_DDZ_AI* game, COMBO_OF_POKERS* pre)
{
	COMBOS_SUMMARY_DDZ_AI * summary = game->players[CUR_PLAYER]->summary;
	PLAYER *pl = game->players[CUR_PLAYER];
	if (summary->real_total_num == 1) //just one combo
	{
		COMBO_OF_POKERS * ret = find_combo(summary, pre);
		return ret;
	}
	else if (summary->combo_total_num <= 1)
	{
		if (summary->real_total_num == 2 && summary->combo_total_num <= 1) //just one combo
		{
			if (summary->bomb_num == 1 && is_combo_biggest_sure(game, summary->bomb[0])) //
			{
				int i = 0;
				for (; i<summary->bomb_num; i++)
					if (check_combo_a_Big_than_b(summary->bomb[i], pre))
						return summary->bomb[i];
			}
		}

		//farmer do not make bigger for another farmer player
		if (pl->id == DOWNFARMER_old)
		{
			if (game->players[game->pre_playernum]->id == UPFARMER_old
				&& game->players[game->pre_playernum]->summary->real_total_num == 1)
			{
				//check bomb?
				return NULL;
			}
		}
		else if (0 && pl->id == UPFARMER_old && (game->players[game->pre_playernum]->id == DOWNFARMER_old)
			&& game->players[game->pre_playernum]->summary->real_total_num == 1)
		{
			//todo check
			//if(is_combo_biggest_for_player(game->players[DOWN_PLAYER],game->pre))
			return NULL;
		}
		if (NOT_BOMB(pre))
		{
			int number = 0, num = 0;
			COMBO_OF_POKERS ** ret = find_all_combo_bigger_than_combo_a(summary, pre, &number);

			if (number >= 1)
			{
				if (pre->type == SINGLE_SERIES || pre->type == PAIRS_SERIES
					|| pre->type == THREE_SERIES || pre->type == 531
					|| pre->type >= 3311
					)//todo: optimize it
				{
					return *ret;
				}
				num = number;
				{
					while (number>1)
					{
						if (ret[number - 1]->low > ret[number - 2]->low
							&&  is_combo_biggest_sure(game, ret[number - 2]))
							number--;
						else
							break;
					}
				}
				if (is_combo_biggest_sure(game, ret[number - 1]))
					return *(ret + number - 1);
				//  else
				//   return NULL;
			}
			// else
			//   return NULL;
		}
		if (summary->bomb_num>0 && !opp_hasbomb(game)) //
		{
			int i = 0;
			for (; i<summary->bomb_num; i++)
				if (check_combo_a_Big_than_b(summary->bomb[i], pre))
				{
					return summary->bomb[i];
				}
		}
		//else   

        if (summary->bomb_num > 0)
        {
            // 外面无炸弹，且只剩1套小牌
            // 或者 除炸弹外至多只剩一套，且有最大炸弹
            // 或者 全剩炸弹
            if (!opp_hasbomb(game) 
                || (is_combo_biggest_sure(game, summary->bomb[summary->bomb_num - 1]) && summary->real_total_num <= summary->bomb_num + 1)
                || (summary->bomb_num == summary->biggest_num && summary->real_total_num == summary->bomb_num))
            {
                int i = 0;
                for (; i < summary->bomb_num; i++)
                    if (check_combo_a_Big_than_b(summary->bomb[i], pre))
                        return summary->bomb[i];
            }
        }

	}
	return NULL;
}


COMBO_OF_POKERS* get_combo_in_a_win_suit(GAME_DDZ_AI* game)
{
	PLAYER* player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI* s = player->summary;
	int num = s->combo_total_num;
    if (num == 2 && (s->singles_num == 2 || s->pairs_num == 2) 
        && s->bomb_num == s->biggest_num && s->bomb_num == 1 && s->bomb[0]->low < LIT_JOKER)
    {
        if (!is_combo_biggest_sure(game, s->bomb[0])) // 
        {
            // 4+1+1 or 4+2+2
            if (s->singles_num == 2 && !s->singles[0]->control && !s->singles[1]->control)
            {
                s->bomb[0]->type = 411;
                s->bomb[0]->three_desc[0] = s->singles[0]->low;
                s->bomb[0]->three_desc[1] = s->singles[1]->low;
                return s->bomb[0];
            }
            else if (s->pairs_num == 2 && !s->pairs[0]->control && !s->pairs[1]->control)
            {
                s->bomb[0]->type = 422;
                s->bomb[0]->three_desc[0] = s->pairs[0]->low;
                s->bomb[0]->three_desc[1] = s->pairs[1]->low;
                return s->bomb[0];
            }
        }
    }

	if (num <= 1 + s->bomb_num)//for bomb
	{
		int has_big = 0;
		if (player->id == LORD_old)
		{
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type == SINGLE_SERIES ||
					s->biggest[k]->type == PAIRS_SERIES ||
					s->biggest[k]->type >= 3311)
					has_big = 1;
			}
		}
		if (!opp_hasbomb(game) ||
			player->id != LORD_old
			|| (s->bomb_num == 0 && (s->ctrl.single <10 || has_big == 1))
			|| (s->bomb_num + 1 - num) >= get_opp_bomb_number(game)
			|| ((s->bomb_num + 2 - num) >= get_opp_bomb_number(game)
				&& s->bomb_num >0 && is_combo_biggest_sure(game,
					s->bomb[s->bomb_num - 1])
				)
			)
		{
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET)//先出冲锋套
				{
					for (int i = 0; i<s->not_biggest_num; i++)
					{
						if (s->not_biggest[i]->type == s->biggest[k]->type
							&& (s->not_biggest[i]->len == s->biggest[k]->len || (s->biggest[k]->type != SINGLE_SERIES
								&& s->biggest[k]->type != PAIRS_SERIES && s->biggest[k]->type != THREE_SERIES)))
						{
							COMBO_OF_POKERS t = *s->biggest[k];
							t.low--;
							if (is_combo_biggest_sure_without_bomb(game, &t)) //todo check this?
							{
								if (s->not_biggest_num >= 1 + s->bomb_num)
								{
									COMBO_OF_POKERS *c = &t;
									if (game->known_others_poker)
									{
										//查看农民是否刚好能跑
										if (game->players[CUR_PLAYER]->id != DOWNFARMER_old &&
											game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
											&& find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i])
											&& NOT_BOMB(c))
											continue;
										if (game->players[CUR_PLAYER]->id != UPFARMER_old &&
											game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
											&& find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i])
											&& NOT_BOMB(c))
											continue;
									}
									return s->not_biggest[i];
								}
							}
						}
					}
				}
			}

            if (!has_big && s->bomb_num > 1)
            {
                // 先出小牌用炸弹接手,避免先出冲锋套被炸
                if (s->not_biggest_num >= s->bomb_num + 1)
                {    //find the small comb
                    COMBO_OF_POKERS* c1 = NULL, t, * c = &t;
                    //check c
                    for (int i = 0; i < s->not_biggest_num; i++)
                    {
                        if (game->known_others_poker)
                        {
                            //查看农民是否刚好能跑
                            if (game->players[CUR_PLAYER]->id != DOWNFARMER_old &&
                                game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
                                && find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i])
                                && NOT_BOMB(c))
                                continue;
                            if (game->players[CUR_PLAYER]->id != UPFARMER_old &&
                                game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
                                && find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i])
                                && NOT_BOMB(c))
                                continue;
                        }
                        else if ((s->not_biggest[i]->type == SINGLE
                            && game->players[CUR_PLAYER]->id != UPFARMER_old && player->oppUp_num == 1)
                            ||
                            (s->not_biggest[i]->type == PAIRS
                                && game->players[CUR_PLAYER]->id != UPFARMER_old && player->oppUp_num == 2)
                            || (s->not_biggest[i]->type == SINGLE
                                && game->players[CUR_PLAYER]->id != DOWNFARMER_old && player->oppDown_num == 1)
                            ||
                            (s->not_biggest[i]->type == PAIRS
                                && game->players[CUR_PLAYER]->id != DOWNFARMER_old && player->oppDown_num == 2)
                            )
                        {
                            continue;
                        }
                        if (c1 == NULL)
                            c1 = s->not_biggest[i];
                        if (s->not_biggest[i]->low < c1->low)
                            c1 = s->not_biggest[i];
                    }
                    if (c1)
                    {
                        return c1;
                    }
                }
            }

			for (int k = 0; k < s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET
					&& is_combo_biggest_sure_without_bomb(game, s->biggest[k]))//先出冲锋套
					return s->biggest[k];
			}
			for (int k = 0; k < s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET)//先出冲锋套
					return s->biggest[k];
			}
			if (s->not_biggest_num>0)
			{    //find the small comb
				COMBO_OF_POKERS * c1 = NULL, t, *c = &t;
				//check c
				for (int i = 0; i<s->not_biggest_num; i++)
				{
					if (game->known_others_poker)
					{
						//查看农民是否刚好能跑
						if (game->players[CUR_PLAYER]->id != DOWNFARMER_old &&
							game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
							&& find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i])
							&& NOT_BOMB(c))
							continue;
						if (game->players[CUR_PLAYER]->id != UPFARMER_old &&
							game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
							&& find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i])
							&& NOT_BOMB(c))
							continue;
					}
					else if ((s->not_biggest[i]->type == SINGLE
						&& game->players[CUR_PLAYER]->id != UPFARMER_old && player->oppUp_num == 1)
						||
						(s->not_biggest[i]->type == PAIRS
							&& game->players[CUR_PLAYER]->id != UPFARMER_old && player->oppUp_num == 2)
						|| (s->not_biggest[i]->type == SINGLE
							&& game->players[CUR_PLAYER]->id != DOWNFARMER_old&& player->oppDown_num == 1)
						||
						(s->not_biggest[i]->type == PAIRS
							&& game->players[CUR_PLAYER]->id != DOWNFARMER_old && player->oppDown_num == 2)
						)
					{
						continue;
					}
					if (c1 == NULL)
						c1 = s->not_biggest[i];
					if (s->not_biggest[i]->low < c1->low)
						c1 = s->not_biggest[i];
				}
				if (c1 == NULL)
				{
					c1 = s->not_biggest[0];
					// 以下代码考察c1
					if (c1->type == SINGLE 
                        && ((game->players[CUR_PLAYER]->id != DOWNFARMER_old && player->oppDown_num == 1) 
                            || game->players[CUR_PLAYER]->id == LORD_old && player->oppUp_num == 1)) //上家农民或者地主在对手报单的情况下
					{
						if (s->not_biggest_num <= 1 && s->bomb_num > 0 && is_combo_biggest_sure(game, s->bomb[s->bomb_num - 1]))  //之前的逻辑有出非炸弹冲锋套的
						{
							c1 = s->bomb[0];
						}
						else // 打最大的非冲锋套
						{
							c1 = s->not_biggest[s->not_biggest_num - 1];
						}
					}
                    else if (c1->type == PAIRS 
                        && ((game->players[CUR_PLAYER]->id != DOWNFARMER_old && player->oppDown_num == 2)
                            || game->players[CUR_PLAYER]->id == LORD_old && player->oppUp_num == 2)) // 
					{
						// TODO: 是否拆对？
						if (s->not_biggest_num <= 1 && s->bomb_num > 0 && is_combo_biggest_sure(game, s->bomb[s->bomb_num - 1]))  //之前的逻辑有出非炸弹冲锋套的
						{
							c1 = s->bomb[0];
						}
						else // 打最大的非冲锋套
						{
							c1 = s->not_biggest[s->not_biggest_num - 1];
						}
					}

				}
				return c1;
			}

			if (s->bomb_num>0)
			{
				return s->bomb[0];
			}
			return NULL;
		}
		else
		{
			if (s->not_biggest_num >0)
			{    //find the small comb
				COMBO_OF_POKERS * c1 = NULL, t, *c = &t;
				//check c
				for (int i = 0; i<s->not_biggest_num; i++)
				{
					if (game->known_others_poker)
					{
						//查看农民是否刚好能跑
						if (game->players[CUR_PLAYER]->id != DOWNFARMER_old &&
							game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
							&& find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i])
							&& NOT_BOMB(c))
							continue;
						if (game->players[CUR_PLAYER]->id != UPFARMER_old &&
							game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
							&& find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i])
							&& NOT_BOMB(c))
							continue;
					}
					for (int k = 0; k<s->biggest_num; k++)
					{
						if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET
							&& s->biggest[k]->type == s->not_biggest[i]->type
							&& ((s->biggest[k]->type != SINGLE_SERIES
								&& s->biggest[k]->type != PAIRS_SERIES && s->biggest[k]->type != THREE_SERIES) ||
								s->biggest[k]->len == s->not_biggest[i]->len)
							)
						{
							if (c1 == NULL ||
								s->not_biggest[i]->len > c1->len)
								c1 = s->not_biggest[i];
						}
					}
				}
				if (c1 == NULL)
				{    //find the small comb
					for (int i = 0; i<s->not_biggest_num; i++)
					{
						if (game->known_others_poker)
						{
							//查看农民是否刚好能跑
							if (game->players[CUR_PLAYER]->id != DOWNFARMER_old &&
								game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
								&& find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i])
								&& NOT_BOMB(c))
								continue;
							if (game->players[CUR_PLAYER]->id != UPFARMER_old &&
								game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
								&& find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i])
								&& NOT_BOMB(c))
								continue;
						}
						if (c1 == NULL || s->not_biggest[i]->len > c1->len)
							c1 = s->not_biggest[i];
					}
				}
				if (c1 == NULL)
				{
					c1 = s->not_biggest[0];
				}
				return c1;
			}

			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET
					&& is_combo_biggest_sure_without_bomb(game, s->biggest[k]))//先出冲锋套
					return s->biggest[k];
			}
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET)//先出冲锋套
					return s->biggest[k];
			}

			if (s->bomb_num>0)
			{
				return s->bomb[0];
			}
			return NULL;
		}
	}
	else
		return NULL;
}
// select a combo form player
COMBO_OF_POKERS* farm_select_combo_in_suit(GAME_DDZ_AI* game)
{
	PLAYER* player = game->players[CUR_PLAYER];
	PLAYER* lord = game->player_type == UPFARMER_old ? game->players[DOWN_PLAYER] : game->players[UP_PLAYER];
	PLAYER* other = game->player_type == DOWNFARMER_old ? game->players[DOWN_PLAYER] : game->players[UP_PLAYER];
	COMBOS_SUMMARY_DDZ_AI* s = player->summary;

	if (s->combo_total_num == 2)
	{
		int number = 0;
		for (int k = 0; k<s->not_biggest_num; k++)
		{
			if (s->not_biggest[k]->type == SINGLE || s->not_biggest[k]->type == PAIRS)
			{
				number++;
			}
		}
		if (number == 2 && s->bomb_num == 0 && s->ctrl.single < 5)
		{
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type == PAIRS || s->biggest[k]->type == SINGLE)
					continue;
				return s->biggest[k];
			}
		}
	}
	if (!is_player_ready_win(game, player)) //不合手
	{
		for (int k = 0; k<s->biggest_num; k++)
		{
			if (s->biggest[k]->type == SINGLE_SERIES  && s->biggest[k]->low <= P6)
			{
				return s->biggest[k];
			}
		}

		COMBO_OF_POKERS *ret, tmp;
		ret = &tmp;
		tmp.type = NOTHING_old;
		tmp.low = 0;
		tmp.len = 0;
		for (int i = 0; i < player->summary->not_biggest_num; ++i)
		{
			ret = player->summary->not_biggest[i];
			bool findBiggerinBiggest = false;
			for (int j = 0; j < player->summary->biggest_num; ++j)
			{
				if (ret->type == player->summary->biggest[j]->type && ret->len == player->summary->biggest[j]->len)
				{
					findBiggerinBiggest = true;
					break;
				}
			}
			if (findBiggerinBiggest)
			{
				break;
			}
			else
			{
				ret = &tmp;
			}
		}

		if (ret->type != NOTHING_old)
		{
			if (lord->summary->real_total_num <= 1)
			{
				COMBO_OF_POKERS *otherCombo = find_combo(other->summary, ret);
				COMBO_OF_POKERS *lordCombo;
				if (otherCombo == NULL || otherCombo->type == BOMB_old || otherCombo->type == ROCKET)
				{
					lordCombo = find_combo(lord->summary, ret);
				}
				else
				{
					lordCombo = find_combo(lord->summary, otherCombo);
				}

				if (lordCombo != NULL && lordCombo->type != BOMB_old && lordCombo->type != ROCKET)
				{
					if (game->use_best_for_undefined_case)
					{
						get_the_best_hand(game, ret, 1); // 仅主动出牌
						return ret;
					}
				}
				else
				{
					return ret;
				}
			}
			else
			{
				return ret;
			}
		}

		//出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
		return  find_smallest_in_combos(player->combos, 20, player, 0);
	}
	///*
	// rocket or biggest bomb and other..
	// for chuntian
	if (s->combo_total_num == 1 && s->bomb_num == s->biggest_num &&s->bomb_num>0
		&& player->oppDown_num >= 17 && player->oppUp_num >= 17)
	{
		//bomb is biggest
		if (s->bomb[0]->type == ROCKET)
			return s->bomb[0];
		else
		{
			COMBO_OF_POKERS tmp;
			if (!getBigBomb(player->opps, &tmp, s->bomb[0]))
				return s->bomb[0];
		}
	}
	//*/
	COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);
	if (c != NULL)
	{
		return c;
	}
	for (int k = 0; k<s->biggest_num; k++)
	{
		COMBO_OF_POKERS *ret, tmp;
		ret = &tmp;
		tmp.type = NOTHING_old;
		tmp.low = 0;
		tmp.len = 0;
		if (s->biggest[k]->type == PAIRS_SERIES && s->biggest[k]->low < P9)
		{
			for (int i = 0; i < s->not_biggest_num; i++)
			{
				if (s->not_biggest[i]->type == PAIRS_SERIES)//检查长度？
				{
					ret = s->not_biggest[i];
					break;
				}
			}
			if (ret->type == NOTHING_old)
			{
				return s->biggest[k];
			}
		}
		else if (s->biggest[k]->type == SINGLE_SERIES)
		{
			for (int i = 0; i < s->not_biggest_num; i++)
			{
				if (s->not_biggest[i]->type == SINGLE_SERIES && s->not_biggest[i]->low <= s->biggest[k]->low)//检查长度？
				{
					ret = s->not_biggest[i];
					break;
				}
			}
			if (ret->type == NOTHING_old)
			{
				return s->biggest[k];
			}
		}

		if (ret->type != NOTHING_old) //防止地主只剩最后一套刚好可以管上,与上面的逻辑略有不同
		{
			if (lord->summary->real_total_num <= 1)
			{
				COMBO_OF_POKERS *otherCombo = find_combo(other->summary, ret);
				COMBO_OF_POKERS *lordCombo;
				if (otherCombo == NULL || otherCombo->type == BOMB_old || otherCombo->type == ROCKET)
				{
					lordCombo = find_combo(lord->summary, ret);
				}
				else
				{
					lordCombo = find_combo(lord->summary, otherCombo);
				}

				if (lordCombo == NULL || lordCombo->type == BOMB_old || lordCombo->type == ROCKET)
				{
					return ret;
				}
			}
			else
			{
				return ret;
			}
		}

		if (s->biggest[k]->type >= 3311)
		{
			return s->biggest[k];
		}
	}

	//寻找牌组中，套数最多的一类牌
	int max_ddz = 0, idx = 0;
	if (s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num >max_ddz) {
		max_ddz = s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num;
		idx = 1;
	}
	if (s->pairs_series_num>max_ddz) {
		max_ddz = s->pairs_series_num;
		idx = 2;
	}
	for (int i = 0; i < 10; i++)
	{
		if (s->series_detail[i] > max_ddz) {
			max_ddz = s->series_detail[i];
			idx = 6;
		}
	}

	if (s->three_one_num + s->three_two_num + s->three_num>max_ddz) {
		max_ddz = s->three_one_num + s->three_two_num + s->three_num;
		idx = 3;
	}
	if (s->pairs_num>max_ddz) {
		max_ddz = s->pairs_num;
		idx = 4;
	}
	if (s->singles_num>max_ddz) {
		max_ddz = s->singles_num;
		idx = 5;
	}

	if (max_ddz > 1) //todo: check for controls..
	{
		COMBO_OF_POKERS *ret, tmp;
		ret = &tmp;
		tmp.type = NOTHING_old;
		tmp.low = 0;
		tmp.len = 0;
		if (idx == 3) //to be refine
		{
			if (s->three_one_num>0) {
				ret = s->three_one[0];
			}
			else if (s->three_two_num>0) {
				ret = s->three_two[0];
			}
			else
				ret = s->three[0];
		}
		else if (idx == 1) {
			if (s->threeseries_one_num>0) {
				ret = s->threeseries_one[0];
			}
			else if (s->threeseries_two_num>0) {
				ret = s->threeseries_two[0];
			}
			else
				ret = s->threeseries[0];
		}
		else if (idx == 2) {
			ret = s->pairs_series[0];
		}
		else if (idx == 4) { //单双应该在报单报双逻辑中处理
			return s->pairs[0];
		}
		else if (idx == 5) {
			return s->singles[0];
		}
		else if (idx == 6) {
			ret = s->series[0]; //todo : fixme..
		}

		if (ret->type != NOTHING_old)  //防止地主只剩最后一套刚好可以管上,与第一次的逻辑一样
		{
			if (lord->summary->real_total_num <= 1)
			{
				COMBO_OF_POKERS *otherCombo = find_combo(other->summary, ret);
				COMBO_OF_POKERS *lordCombo;
				if (otherCombo == NULL || otherCombo->type == BOMB_old || otherCombo->type == ROCKET)
				{
					lordCombo = find_combo(lord->summary, ret);
				}
				else
				{
					lordCombo = find_combo(lord->summary, otherCombo);
				}

				if (lordCombo != NULL && lordCombo->type != BOMB_old && lordCombo->type != ROCKET)
				{
					if (game->use_best_for_undefined_case)
					{
						int pass = get_the_best_hand(game, ret, 1);
                        if (!pass)
                        {
                            return ret;
                        }
					}
                   
                    for (int i = 0; i < s->not_biggest_num; ++i)
                    {
                        if (s->not_biggest[i]->type != lordCombo->type
                            || s->not_biggest[i]->low >= lordCombo->low)
                        {
                            return s->not_biggest[i];
                        }
                    }
                    for (int i = 0; i < s->biggest_num; ++i)
                    {
                        if (NOT_BOMB(s->biggest[i]) && (s->biggest[i]->type != lordCombo->type
                            || s->biggest[i]->low >= lordCombo->low))
                        {
                            return s->biggest[i];
                        }
                    }
                        
                    // 拆牌
                    if (lordCombo->type == SINGLE)
                    {
                        return s->not_biggest[s->not_biggest_num - 1];
                    }
                    else if (lordCombo->type == THREE_ONE && s->three_one_num > 0)
                    {
                        // 强行修改summary里的数据，之后应该直接返回，不再加任何逻辑
                        ret = s->three_one[0];
                        ret->low = s->three_one[0]->three_desc[0];
                        ret->type = SINGLE;
                        ret->len = 1;
                        return ret;
                    }
                    else if (lordCombo->type == THREE_TWO && s->three_one_num > 0)
                    {
                        // 强行修改summary里的数据，之后应该直接返回，不再加任何逻辑
                        ret = s->three_two[0];
                        ret->low = s->three_two[0]->three_desc[0];
                        ret->type = PAIRS;
                        ret->len = 1;
                        return ret;
                    }
                    else
                    {
                        if (s->not_biggest_num > 0)
                        {
                            // 强行修改summary里的数据，之后应该直接返回，不再加任何逻辑
                            ret = s->not_biggest[0];
                            ret->type = SINGLE;
                            ret->len = 1;
                            return ret;
                        }
                    }
                }
                return ret;
			}
			else
			{
				return ret;
			}
		}
	}
	else
	{
		//合手了，但是小牌大于一套，出标签较小那套
		return  find_smallest_in_combos(player->combos, 20, player, 0);
	}
	return NULL;
}


// select a combo form player
COMBO_OF_POKERS* lord_select_combo_in_suit(GAME_DDZ_AI* game)
{
	PLAYER* player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI* s = player->summary;

	if (s->combo_total_num == 2)
	{
		int number = 0;
        int small_pair_num = 0;
		for (int k = 0; k<s->not_biggest_num; k++)
		{
			if (s->not_biggest[k]->type == SINGLE || s->not_biggest[k]->type == PAIRS)
			{
				number++;
			}
            if (s->not_biggest[k]->type == PAIRS)
            {
                small_pair_num++;
            }
		}

		if (number == 2 && s->bomb_num == 0 && s->ctrl.single < 5)
		{
            if (small_pair_num > 0)
            {
                for (int k = 0; k < s->biggest_num; k++)
                {
                    if (s->biggest[k]->type == PAIRS)
                    {
                        return s->pairs[0];
                    }
                }
            }

			for (int k = 0; k<s->biggest_num; k++)
			{
				return s->biggest[k];
			}
		}
	}
	if (!is_player_ready_win(game, player)) //不合手
	{
		for (int k = 0; k<s->biggest_num; k++)
		{
			if (s->biggest[k]->type == SINGLE_SERIES  && s->biggest[k]->low <= P6)
			{
				return s->biggest[k];
			}
			if (s->biggest[k]->type == PAIRS_SERIES && s->biggest[k]->low <= P8)
			{
				return s->biggest[k];
			}
			if (s->biggest[k]->type >= 3311 && s->biggest[k]->low <= P7)
			{
				return s->biggest[k];
			}
		}
		//出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
		return  find_smallest_in_combos(player->combos, 20, player, 0);
	}
	///*
	// rocket or biggest bomb and other..
	// for chuntian
	if (s->combo_total_num == 1 && s->bomb_num == s->biggest_num &&s->bomb_num>0
		&& player->oppDown_num >= 17 && player->oppUp_num >= 17)
	{
		//bomb is biggest
		if (s->bomb[0]->type == ROCKET)
			return s->bomb[0];
		else
		{
			COMBO_OF_POKERS tmp;
			if (!getBigBomb(player->opps, &tmp, s->bomb[0]))
				return s->bomb[0];
		}
	}
	//*/
	COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);
	if (c != NULL)
	{
		return c;
	}
	for (int k = 0; k<s->biggest_num; k++)
	{
		if (s->biggest[k]->type == PAIRS_SERIES && s->biggest[k]->low<P9)
		{
			for (int i = 0; i<s->not_biggest_num; i++)
			{
				if (s->not_biggest[i]->type == PAIRS_SERIES)
				{
					return s->not_biggest[i];
				}
			}
			return s->biggest[k];
		}
		if (s->biggest[k]->type == SINGLE_SERIES)
		{
			for (int i = 0; i<s->not_biggest_num; i++)
			{
				if (s->not_biggest[i]->type == SINGLE_SERIES && s->not_biggest[i]->low <= s->biggest[k]->low)
				{
					return s->not_biggest[i];
				}
			}
			return s->biggest[k];
		}
		if (s->biggest[k]->type >= 3311)
		{
			return s->biggest[k];
		}
	}

	//寻找牌组中，套数最多的一类牌
	int max_ddz = 0, idx = 0;
	if (s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num >max_ddz) {
		max_ddz = s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num;
		idx = 1;
	}
	if (s->pairs_series_num>max_ddz) {
		max_ddz = s->pairs_series_num;
		idx = 2;
	}
	for (int i = 0; i<10; i++)
		if (s->series_detail[i]>max_ddz) {
			max_ddz = s->series_detail[i];
			idx = 6;
		}

	if (s->three_one_num + s->three_two_num + s->three_num>max_ddz) {
		max_ddz = s->three_one_num + s->three_two_num + s->three_num;
		idx = 3;
	}
	if (s->pairs_num>max_ddz) {
		max_ddz = s->pairs_num;
		idx = 4;
	}
	if (s->singles_num>max_ddz) {
		max_ddz = s->singles_num;
		idx = 5;
	}

	if (max_ddz > 1) //todo: check for controls..
	{
		if (idx == 3) //to be refine
		{
			if (s->three_one_num>0) {
				return s->three_one[0];
			}
			else            if (s->three_two_num>0) {
				return s->three_two[0];
			}
			else
				return s->three[0];
		}
		else if (idx == 1) {
			if (s->threeseries_one_num>0) {
				return s->threeseries_one[0];
			}
			else            if (s->threeseries_two_num>0) {
				return s->threeseries_two[0];
			}
			else
				return s->threeseries[0];
		}
		else if (idx == 2) {
			return s->pairs_series[0];
		}
		else if (idx == 4) {
			return    s->pairs[0];
		}
		else if (idx == 5) {
			return    s->singles[0];
		}
		else if (idx == 6) {
			return s->series[0]; //todo : fixme..
		}
	}
	else
	{
		//合手了，但是小牌大于一套，出标签较小那套
		return  find_smallest_in_combos(player->combos, 20, player, 0);
	}
	return NULL;
}

//check if lord win for current status
//地主强合手
//return 1: lord win
//return 0: no simple win for lord.
COMBO_OF_POKERS* is_lord_play_first_win(GAME_DDZ_AI * game)
{
	FUNC_NAME;
	PLAYER * player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS* c = &game->c[0];
	if (game->known_others_poker)
	{

		//春天可能性检查
		//如果春天则春天
		if (s->combo_total_num == 1 && s->bomb_num == s->biggest_num &&s->bomb_num>0
			&& player->oppDown_num >= 17 && player->oppUp_num >= 17)
		{
			//bomb is biggest
			if (s->bomb[0]->type == ROCKET)
				return s->bomb[0];
			else
			{
				if (is_combo_biggest_sure(game, s->bomb[0]))
					return s->bomb[0];
			}
		}

		int num = s->combo_total_num;

		COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);
		if (c != NULL)
		{
			return c;
		}

		if (num <= 1 + s->bomb_num)//for bomb
		{
			for (int k = 0; k<s->biggest_num; k++)
			{
				if (s->biggest[k]->type != BOMB_old && s->biggest[k]->type != ROCKET)//先出冲锋套
				{
					return s->biggest[k];
				}
			}

			if (s->not_biggest_num>0)
			{
				//play the one
				for (int i = 0; i<s->not_biggest_num; i++)
				{
					//查看农民是否刚好能跑
					if (game->players[DOWN_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
						&& find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, c, s->not_biggest[i]))
						continue;
					if (game->players[UP_PLAYER]->h->total == get_combo_number(s->not_biggest[i])
						&& find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, c, s->not_biggest[i]))
						continue;
					return s->not_biggest[i];
				}
				if (s->not_biggest_num >= 2)//lord would lose
				{
					return s->not_biggest[0];
				}
			}

			if (s->bomb_num>0)
			{
				return s->bomb[0];
			}
			return NULL;
		}
		return 0;
		//todo: 如果有小牌我们肯定能收回的
		for (int i = 0; i<s->not_biggest_num; i++)
		{
			//nd_biggest_in_combos(COMBO_OF_POKERS * combos, int total)
		}
	}
	else
	{
		if (s->combo_total_num == 1 && s->bomb_num == s->biggest_num &&s->bomb_num>0
			&& player->oppDown_num >= 17 && player->oppUp_num >= 17)
		{
			if (s->bomb[0]->type == ROCKET)
				return s->bomb[0];
			else
			{
				if (is_combo_biggest_sure(game, s->bomb[0]))
					return s->bomb[0];
			}
		}
		COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);
		if (c != NULL)
		{
			int good = 1;
			if ((player->oppDown_num == 1 || player->oppUp_num == 1) && c->type == SINGLE
				&& c->low < player->opps->end)
				good = 0;
			if ((player->oppDown_num == 2 || player->oppUp_num == 2) && c->type == PAIRS
				&& !is_combo_biggest_sure(game, c))
			{
				good = 0;
			}
			if (good) {
				return c;
			}
		}
	}
	return 0;
}

void lord_play_first(GAME_DDZ_AI * game, COMBO_OF_POKERS* c)//select c from player
{
	PLAYER * player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS* t = NULL;
	FUNC_NAME;

	if (NULL != (t = is_lord_play_first_win(game)))
	{
		PRINTF(LEVEL, "lord quick win found!\n");
		*c = *t;
		return;
	}

	if (player->oppDown_num == 1 // game->players[(CUR_PLAYER+1)%3]->h->total == 1  //last play farmer has 1 only
		|| player->oppUp_num == 1) // the down player of last play farmer has 1 only
								   // ||  game->players[(CUR_PLAYER+2)%3]->h->total == 1)  // the Up player of last play farmer has 1 only  and current is a single.
								   // )

	{   //农民报单
		for (int k = 0; k<s->biggest_num; k++)
		{
			if (s->biggest[k]->type == PAIRS_SERIES && s->biggest[k]->low<P9)
			{
				for (int i = 0; i<s->not_biggest_num; i++)
				{
					if (s->not_biggest[i]->type == PAIRS_SERIES)
					{
						*c = *s->not_biggest[i];
						return;
					}
				}
				*c = *s->biggest[k];
				return;
			}
			if (s->biggest[k]->type == SINGLE_SERIES)
			{
				for (int i = 0; i<s->not_biggest_num; i++)
				{
					if (s->not_biggest[i]->type == SINGLE_SERIES && s->not_biggest[i]->low <= s->biggest[k]->low)
					{
						*c = *s->not_biggest[i];   return;
					}
				}
				*c = *s->biggest[k]; return;
			}
			if (s->biggest[k]->type >= 3311)
			{
				*c = *s->biggest[k]; return;
			}
		}

		//如果手中有不含控制牌且不为单牌的套牌，则打出其中标签最小的一套
		t = find_smallest_in_combos(player->combos, 20, player, 1);

		if (t == NULL)
		{
			//如果手中有两套以上不为绝对控制的单牌，打出第二小的
			if (s->singles_num >= 2 && s->singles[1]->low<player->opps->end)  //   || s->singles_num == s->real_total_num )
				t = s->singles[1];
			else
			{
				/*if(s->not_biggest_num>0)//打出含有控制牌的非冲锋套
				t= s->not_biggest[0];
				else*/
				if (s->not_biggest_num >0)
				{
					for (int k = 0; k<s->not_biggest_num; k++)
					{
						if (s->not_biggest[k]->type != SINGLE)
						{
							t = s->not_biggest[k];
							break;
						}
					}
					if (t == NULL) {
						goto __CHONGFENG;
					}
				}
				else {
				__CHONGFENG:
					if (s->biggest_num>0 && s->biggest_num>s->bomb_num)//打出冲锋套
					{

						for (int k = 0; k<s->biggest_num; k++)
						{
							if (s->biggest[k]->type != BOMB_old&&s->biggest[k]->type != ROCKET)
							{
								t = s->biggest[k];
								break;
							}
						}
					}
					else if (s->bomb_num >0)//打出炸弹
						t = s->bomb[0];
					else if (s->singles_num>0) //打出剩余的一张单牌
						t = s->singles[0];
					else { //what happens...
						PRINTF_ALWAYS("strange error\n");
					}
				}
			}
		}
	}
	else
		t = lord_select_combo_in_suit(game);


	if (t == NULL)
	{
		PRINTF_ALWAYS("ERR: line %d lord play first: stupid error\n", __LINE__);
		return;
	}
	memmove(c, t, sizeof(COMBO_OF_POKERS));
	remove_combo_in_suit(s, t);
	t->type = NOTHING_old;
}

int Lord_Chaipai(GAME_DDZ_AI * game, COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre, int good_bomb, int mustPlay = 0)
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	COMBO_OF_POKERS tmp1;
	COMBO_OF_POKERS* c = &tmp1;
	COMBO_OF_POKERS tmp, *p = &tmp;
	*p = *pre;
	int first_time = 1;
	// int mustPlay = 0;
	// COMBOS_SUMMARY_DDZ_AI *s =player->summary;

	//to be optimized by using new search methods..
	if (pre->type == SINGLE && player->summary->ctrl.single >= 30)
	{
		good_bomb = 1;
	}
    
    good_bomb = 1; // 首先不找炸弹以及拆炸弹
    while (find_a_bigger_combo_in_hands(player->h, c, p)) //could not handle corner case for  BOMB_old in two combos.
    {
        if (!NOT_BOMB(c))
            break;
        if (c->type >= THREE_ONE)
        {
            c->control = 2;//tmporary use
        }
        else
            c->control = 0;//tmporary use 

        PLAYER tmp_player;
        POKERS h, opps;
        COMBO_OF_POKERS combo[20] = {20 * 0};
        COMBOS_SUMMARY_DDZ_AI sum = {0}, *s;
        tmp_player.combos = &combo[0];
        tmp_player.h = &h;
        tmp_player.opps = &opps;
        tmp_player.summary = s = &sum;
        tmp_player.lower_control_poker = player->lower_control_poker;
        tmp_player.oppDown_num = player->oppDown_num;
        tmp_player.oppUp_num = player->oppUp_num;
        memmove(&h, player->h, sizeof(POKERS));

        memmove(&opps, player->opps, sizeof(POKERS));
        remove_combo_poker(tmp_player.h, c, NULL);

        search_combos_in_suit(game, tmp_player.h, tmp_player.opps, &tmp_player);
        if (good_bomb)//不拆炸弹
        {
            if (tmp_player.summary->bomb_num < player->summary->bomb_num)
            {
                *p = *c;
                continue;
            }
        }
        int MAX_poker_in_c = has_control_poker(c, player->lower_control_poker);
        {
            if (c->type == SINGLE || c->type == PAIRS)
            {
                if (c->low >= player->lower_control_poker)
                    assume_remove_controls_in_opps(&opps, c, MAX_poker_in_c);
                int ctrl_num = CONTROL_POKER_NUM;

                //update the lowest control poker and update the summary
                POKERS all;
                add_poker(tmp_player.h, &opps, &all);
                if (all.total <= 20)
                    ctrl_num = 4;
                else if (all.total <= 10)
                    ctrl_num = 2;
                tmp_player.lower_control_poker = get_lowest_controls(&all, ctrl_num);
                tmp_player.summary->ctrl.single = calc_controls(tmp_player.h, tmp_player.opps, ctrl_num);
            }
            if (tmp_player.lower_control_poker<player->lower_control_poker
                && tmp_player.summary->ctrl.single > player->summary->ctrl.single)
            {//shit
                PRINTF(LEVEL, "pull back single control\n");
                tmp_player.summary->ctrl.single = max_ddz(player->summary->ctrl.single - 1, 0);
            }

            if (CONTROL_SUB_COMBO_NUM_IS_GOOD(&tmp_player)
                || CONTROL_SUB_COMBO_NUM(&tmp_player) >= CONTROL_SUB_COMBO_NUM(player)
                || CONTROL_NUM(&tmp_player) >= 30
                || (CONTROL_SUB_COMBO_NUM(&tmp_player) + 10 >= CONTROL_SUB_COMBO_NUM(player)
                && (is_combo_biggest(game, player->opps, c, player->oppDown_num, player->oppUp_num, player->lower_control_poker)
                || (c->type != SINGLE && c->type != PAIRS)
                )
                )
                || mustPlay)
            {
                goto SAVE_CUR_RESULT;
            }
            else
            {
                *p = *c;
                continue;
            }

        SAVE_CUR_RESULT:
            //update player in hands;
            PRINTF(LEVEL, "!DIZHU chaipai, control-combo %d, combo_num %d old( %d,%d)\n"
                , CONTROL_SUB_COMBO_NUM(&tmp_player), tmp_player.summary->combo_total_num,
                CONTROL_SUB_COMBO_NUM(player), player->summary->combo_total_num
                );
            if (first_time
                || (CONTROL_SUB_COMBO_NUM(&tmp_player) > CONTROL_SUB_COMBO_NUM(player))
                || (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player)
                && tmp_player.summary->extra_bomb_ctrl > player->summary->extra_bomb_ctrl)
                || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                && tmp_player.summary->combo_total_num < player->summary->combo_total_num)
                || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                && tmp_player.summary->combo_typenum < player->summary->combo_typenum)
                || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                && tmp_player.summary->combo_typenum == player->summary->combo_typenum
                && tmp_player.summary->singles_num < player->summary->singles_num)
                /* ||   ( (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player) )
                && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                && tmp_player.summary->combo_typenum == player->summary->combo_typenum
                && tmp_player.summary->singles_num == player->summary->singles_num )*/
                )
            {
                *player->summary = *tmp_player.summary;
                first_time = 0;
                *cur = *c;
            }
            *p = *c;
        }            
    }

    if (cur->type != NOTHING_old)
    {
        cur->control = 0;
        return first_time;
    }

	//cur->control = 0;
	//return first_time;

    // 炸弹,以及拆炸弹
    for (int m = 0; m < 2; ++m)
    {
        *p = *pre;

        if (m == 1 && game->use_best_for_undefined_case && Check_win_quick(game, DOWN_PLAYER, 0, 0, 0)) // 模拟PASS
        {
            return 1;
        }

        while (find_a_bigger_combo_in_hands(player->h, c, p)) //could not handle corner case for  BOMB_old in two combos.
        {
            if (m == 0 && NOT_BOMB(c)) // 炸弹
            {
                *p = *c;
                continue;
            }
            else if (m == 1 && !NOT_BOMB(c)) // 拆炸弹
            {
                *p = *c;
                continue;
            }

            PLAYER tmp_player;
            POKERS h, opps;
            COMBO_OF_POKERS combo[20] = {20 * 0};
            COMBOS_SUMMARY_DDZ_AI sum = {0}, *s;
            tmp_player.combos = &combo[0];
            tmp_player.h = &h;
            tmp_player.opps = &opps;
            tmp_player.summary = s = &sum;
            tmp_player.lower_control_poker = player->lower_control_poker;
            tmp_player.oppDown_num = player->oppDown_num;
            tmp_player.oppUp_num = player->oppUp_num;
            memmove(&h, player->h, sizeof(POKERS));
            memmove(&opps, player->opps, sizeof(POKERS));
            remove_combo_poker(&h, c, NULL);
            //remove one/two poker bigger than max_ddz poker in c in opps.
            search_combos_in_suit(game, tmp_player.h, tmp_player.opps, &tmp_player);

            if (m == 1 && player->summary->bomb_num <= tmp_player.summary->bomb_num) // 非拆炸弹
            {
                *p = *c;
                continue;
            }

            if (CONTROL_SUB_COMBO_NUM_IS_GOOD(&tmp_player)
                || CONTROL_SUB_COMBO_NUM(&tmp_player) >= CONTROL_SUB_COMBO_NUM(player)
                || CONTROL_NUM(&tmp_player) >= 30
                || (CONTROL_SUB_COMBO_NUM(&tmp_player) + 10 >= CONTROL_SUB_COMBO_NUM(player)
                && (is_combo_biggest(game, player->opps, c, player->oppDown_num, player->oppUp_num, player->lower_control_poker)
                || (c->type != SINGLE && c->type != PAIRS))
                && NOT_BOMB(c)
                //|| mustPlay
                )
                || mustPlay)
            {                
                POKERS t, t1;
                int tmp_card[26], card[21];
                if (game->hun == -1)
                {
                    remove_combo_in_suit(s, c);
                    remove_combo_poker(player->h, c, NULL);
                }
                else
                {
                    combo_2_poker(&t, c);
                    convert_poker_to_hun(&t, &t1, player->h);
                    sub_poker(player->h, &t1, player->h);
                    memcpy(card, player->card, max_POKER_NUM * sizeof(int));
                    // remove card 
                    combo_to_int_array_hun(c,
                        tmp_card, player->card, game->hun);

                }
                COMBO_OF_POKERS pre = game->pre;
                int prepre = game->prepre_playernum;
                int prep = game->pre_playernum;
                game->prepre_playernum = game->pre_playernum;
                game->pre_playernum = CUR_PLAYER;
                game->pre = *c;
                //game->players[CUR_PLAYER]->cur = c;
                memcpy(&game->players[CUR_PLAYER]->curcomb, c, sizeof(COMBO_OF_POKERS));
                if (game->use_best_for_undefined_case)
                {
                    if (Check_win_quick(game, DOWN_PLAYER, 0, 0, 0))
                    {
                        *cur = *c;
                    }
                    if (game->hun == -1)
                    {
                        POKERS p;
                        combo_2_poker(&p, c);
                        add_poker(player->h, &p, player->h);
                        search_combos_in_suit(game, player->h, player->opps, player);
                    }
                    else
                    {
                        add_poker(player->h, &t1, player->h);
                        //                        search_combos_in_suit(game, player->h, player->opps, player);
                        memcpy(player->card, card, max_POKER_NUM * sizeof(int));

                    }
                    game->pre = pre;
                    game->prepre_playernum = prepre;
                    game->pre_playernum = prep;

                    //if (cur->type != NOTHING_old)
                    //{
                    //    return 0;
                    //}
                    //else
                    //{
                    //    *p = *c;
                    //    continue;
                    //}
                }
                else {
                    if ((mustPlay && cur->type == NOTHING_old)
                        || (CONTROL_SUB_COMBO_NUM(&tmp_player) > CONTROL_SUB_COMBO_NUM(player))
                        || (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player)
                        && tmp_player.summary->extra_bomb_ctrl > player->summary->extra_bomb_ctrl)
                        || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                        && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                        && tmp_player.summary->combo_total_num < player->summary->combo_total_num)
                        || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                        && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                        && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                        && tmp_player.summary->combo_typenum < player->summary->combo_typenum)
                        || ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                        && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                        && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                        && tmp_player.summary->combo_typenum == player->summary->combo_typenum
                        && tmp_player.summary->singles_num < player->summary->singles_num)
                        /* ||   ( (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player) )
                        && tmp_player.summary->combo_total_num == player->summary->combo_total_num
                        && tmp_player.summary->combo_typenum == player->summary->combo_typenum
                        && tmp_player.summary->singles_num == player->summary->singles_num )*/
                        )
                    {
                        *player->summary = *tmp_player.summary;
                        //first_time = 0;
                        *cur = *c;
                    }

                    if (game->hun == -1)
                    {
                        POKERS p;
                        combo_2_poker(&p, c);
                        add_poker(player->h, &p, player->h);
                        //   search_combos_in_suit(game, player->h, player->opps, player);
                    }
                    else
                    {
                        add_poker(player->h, &t1, player->h);
                        //                        search_combos_in_suit(game, player->h, player->opps, player);
                        memcpy(player->card, card, max_POKER_NUM * sizeof(int));

                    }
                }
                game->pre = pre;
                game->prepre_playernum = prepre;
                game->pre_playernum = prep;

                PRINTF(LEVEL, "GuanPai \n");
                //*cur = *c;
                if (cur->type != NOTHING_old)
                {
                    return 0;
                }
                else
                {
                    *p = *c;
                    continue;
                }
            }
            else
            {
                *p = *c;
                continue;
            }
        }
    }
    return 1;
}

// 检查打完后是否能赢
// 根据lord_play_2整合出来
static bool _CheckWinWrapper(GAME_DDZ_AI * game, COMBO_OF_POKERS* cur)
{
	if (!game || !cur || !game->use_best_for_undefined_case)
	{
		return false;
	}

	PLAYER *player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS *c = cur;

	POKERS t, t1;
	int tmp_card[26], card[21];
	if (game->hun == -1)
	{
		remove_combo_in_suit(s, c);
		remove_combo_poker(player->h, c, NULL);
	}
	else
	{
		combo_2_poker(&t, c);
		convert_poker_to_hun(&t, &t1, player->h);
		sub_poker(player->h, &t1, player->h);
		memcpy(card, player->card, max_POKER_NUM * sizeof(int));
		// remove card 
		combo_to_int_array_hun(c, tmp_card, player->card, game->hun);
	}
	COMBO_OF_POKERS pre = game->pre;
	int prepre = game->prepre_playernum;
	int prep = game->pre_playernum;
	game->prepre_playernum = game->pre_playernum;
	game->pre_playernum = CUR_PLAYER;
	game->pre = *c;
    memcpy(&game->players[CUR_PLAYER]->curcomb, c, sizeof(COMBO_OF_POKERS));
    game->players[CUR_PLAYER]->cur = &game->players[CUR_PLAYER]->curcomb;

	bool win = game->use_best_for_undefined_case && Check_win_quick(game, DOWN_PLAYER, 0, 0, 0);

	// 还原
	if (game->hun == -1)
	{
		POKERS p;
		combo_2_poker(&p, c);
		add_poker(player->h, &p, player->h);
		search_combos_in_suit(game, player->h, player->opps, player);
	}
	else
	{
		add_poker(player->h, &t1, player->h);
		// search_combos_in_suit(game, player->h, player->opps, player);
		memcpy(player->card, card, max_POKER_NUM * sizeof(int));

	}
	game->pre = pre;
	game->prepre_playernum = prepre;
	game->pre_playernum = prep;

	return win;
}

int lord_play_2(GAME_DDZ_AI * game, COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre)//select a combo from player and save to cur.
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	int mustPlay = 0;
	COMBO_OF_POKERS* res;
	//if ((res = check_for_win_now(game, pre)) != NULL) //win immediately, play the biggest or bomb
	//{
	//	remove_combo_in_suit(player->summary, res);
	//	memmove(cur, res, sizeof(COMBO_OF_POKERS));
	//	res->type = NOTHING;
	//	return 0;
	//}

	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS ctmp;
	memset(&ctmp, 0, sizeof(COMBO_OF_POKERS));
	ctmp.type = NOTHING_old;
	COMBO_OF_POKERS *c = &ctmp;

	bool skipShunpai = false;

	{
		if ((player->oppDown_num == 1 && game->pre_playernum == DOWN_PLAYER) // game->players[game->pre_playernum]->h->total == 1  //last play farmer has 1 only
			|| (player->oppUp_num == 1 && game->pre_playernum == UP_PLAYER)
			|| (player->oppDown_num == 1 && game->pre_playernum == UP_PLAYER && pre->type == SINGLE)
			|| (player->oppUp_num == 1 && game->pre_playernum == DOWN_PLAYER)
			//|| (game->players[(game->pre_playernum+1)%3]->h->total == 1 ) // the down player of last play farmer has 1 only
			//   || ( pre->type == SINGLE && game->players[(game->pre_playernum+2)%3]->h->total == 1)
			// the Up player of last play farmer has 1 only  and current is a single.
			)
		{   // lord must play this turn if he can
			mustPlay = 1;
			if (pre->type == SINGLE)
			{
				if (s->singles_num >= 2)
				{
					/*todo: optimize if all farmer has single in hand only*/
					/*todo: research hands?*/

					//在当前组合中，如果有两套或以上单牌，则从第二小的开始，找出比pre大的单牌cur，
					for (int k = 1; k<s->singles_num; k++)
					{
						if (s->singles[k]->low>pre->low) {
							memmove(cur, s->singles[k], sizeof(COMBO_OF_POKERS));
							s->singles[k]->type = NOTHING_old;
							remove_combo_in_suit(s, cur);
							return 0;
						}
					}
					goto __SEARCH_BOMB_FOR_SINGLE;
					//return 1;
				}
				else //play the biggest
				{
					//if (game->use_best_for_undefined_case)
					//	return get_the_best_hand(game, cur, 0);
					//如果仅剩的一张单牌绝对大，则打出
					if (s->singles_num == 1
						&& s->singles[0]->low>pre->low
						&& s->singles[0]->low >= player->opps->end)
					{
						memmove(cur, s->singles[0], sizeof(COMBO_OF_POKERS));
						s->singles[0]->type = NOTHING_old;
						remove_combo_in_suit(s, cur);
						return 0;
					}
				__SEARCH_BOMB_FOR_SINGLE:
					//如果有炸弹，且出完炸弹合手，则出炸弹
					if (s->bomb_num >0 &&
						CONTROL_SUB_COMBO_NUM_IS_GOOD(player)
						&& get_2nd_MIN_singles(player->summary) >= player->opps->end) //有炸弹，且合手
					{
						memmove(cur, s->bomb[0], sizeof(COMBO_OF_POKERS));
						s->bomb[0]->type = NOTHING_old;
						remove_combo_in_suit(s, cur);
						return 0;
					}
					//出最大的一张牌（可以从套里拆出）
					for (int k = max_ddz(player->opps->end, pre->low + 1); k <= player->h->end; k++)
					{
						if (player->h->hands[k]>0)
						{
							cur->type = SINGLE;
							cur->low = k;
							cur->len = 1;
							return 0;
						}
					}

					for (int k = player->h->end; k>pre->low; k--)
					{
						if (player->h->hands[k]>0)
						{
							cur->type = SINGLE;
							cur->low = k;
							cur->len = 1;
							//research needed.
							player->need_research = 1;
							return 0;
						}
					}

                    // 首先模拟逻辑打出的牌是否能赢
                    if (cur->type != NOTHING_old && _CheckWinWrapper(game, cur))
                    {
                        return 0;
                    }

                    cur->type = NOTHING_old;
                    if (game->use_best_for_undefined_case)
                    {
                        return get_the_best_hand(game, cur, 0);
                    }
				}
				return 1;
			}
		}
	}

	//新增报双逻辑
	if ((player->oppDown_num == 2 && game->pre_playernum == DOWN_PLAYER) // game->players[game->pre_playernum]->h->total == 1  //last play farmer has 1 only
		|| (player->oppUp_num == 2 && game->pre_playernum == UP_PLAYER)
		|| (player->oppDown_num == 2 && game->pre_playernum == UP_PLAYER && pre->type == PAIRS)
		|| (player->oppUp_num == 2 && game->pre_playernum == DOWN_PLAYER))
	{
		skipShunpai = true; // 报双逻辑不走顺牌逻辑
		mustPlay = 1; // 拆牌时用到
		do
		{
			// 出冲锋套能管的牌
			for (int i = 0; i < s->biggest_num; ++i)
			{
				// 令类型相同，不考虑炸弹
				if (s->biggest[i]->type == pre->type && s->biggest[i]->low > pre->low)
				{
					c = s->biggest[i];
					if (!NOT_BOMB(pre) && !_CheckWinWrapper(game, c))
					{
						c = &ctmp;
						continue;
					}
					memmove(cur, s->biggest[i], sizeof(COMBO_OF_POKERS));
					s->biggest[i]->type = NOTHING_old;
					return 0;
				}
			}

			// 考虑炸弹
			for (int i = 0; i < s->bomb_num; ++i)
			{
				c = s->bomb[i];
				if (!_CheckWinWrapper(game, c))
				{
					c = &ctmp;
					continue;
				}
				memmove(cur, s->bomb[i], sizeof(COMBO_OF_POKERS));
				s->bomb[i]->type = NOTHING_old;
				return 0;
			}

			// 出套里能管住的牌
			if (pre->type == SINGLE && s->singles_num > 0)
			{
				// 多余2个则从第二大的牌出，出对时同理
				for (int i = (s->singles_num > 1); i < s->singles_num; ++i)
				{
					if (s->singles[i]->low > pre->low)
					{
						c = s->singles[i];
						if (!_CheckWinWrapper(game, c))
						{
							c = &ctmp;
							continue;
						}
						break;
					}
				}
			}
			else if (pre->type == PAIRS && s->pairs_num > 0)
			{
				for (int i = (s->pairs_num > 1); i < s->pairs_num; ++i)
				{
					if (s->pairs[i]->low > pre->low)
					{
						c = s->pairs[i];
						if (!_CheckWinWrapper(game, c))
						{
							c = &ctmp;
							continue;
						}
						break;
					}
				}
			}
			else
			{
				for (int i = 0; i < s->not_biggest_num; ++i)
				{
					if (s->not_biggest[i]->type == pre->type && s->not_biggest[i]->low > pre->low)
					{
						c = s->not_biggest[i];
						if (!_CheckWinWrapper(game, c))
						{
							c = &ctmp;
							continue;
						}
						break;
					}
				}
			}
		} while (0);

		if (c->type != NOTHING_old)
		{
			// 检测出的是炸弹，出完之后能不能赢
			remove_combo_in_suit(s, c);
			if ((c->type == BOMB_old || c->type == ROCKET) && !_CheckWinWrapper(game, c))
			{
				c = &ctmp;
			}
			else
			{
				memmove(cur, c, sizeof(COMBO_OF_POKERS));
				c->type = NOTHING_old;
				return 0;
			}
		}
	}

	//顺牌
	COMBO_OF_POKERS tmp, *p = &tmp;
	*p = *pre;
	int good_bomb = 0;
	bool findBigger = false; //没有找到大的牌才进入拆牌(非炸弹)
	while (!skipShunpai && (c = find_combo(s, p)) != NULL)
	{
		findBigger = findBigger || (c->type != BOMB_old && c->type != ROCKET);
        if (c->type == BOMB_old || c->type == ROCKET) // 顺牌不出炸弹
        {
            //if (CONTROL_SUB_COMBO_NUM_IS_GOOD(player)  || CONTROL_NUM(player) >= 30)
            //{
            //    good_bomb = 1;
            //}
            break;
        }

		int MAX_poker_in_c;
		if (c->type == SINGLE_SERIES || c->type == PAIRS_SERIES || c->type >= 3311 ||
			((MAX_poker_in_c = has_control_poker(c, player->lower_control_poker))<0))
		{   //如果该牌型不包含当前大牌控制或者当前牌组是顺子,飞机，双顺，则出牌
			DBG(PRINTF(LEVEL, "ShunPai\n"));
			memmove(cur, c, sizeof(COMBO_OF_POKERS));
			return 0;

		}
		else {
			PLAYER tmp_player;
			POKERS h, opps;
			COMBO_OF_POKERS combo[20] = { 20 * 0 };
			COMBOS_SUMMARY_DDZ_AI sum = { 0 }, *s;
			tmp_player.combos = &combo[0];
			tmp_player.h = &h;
			tmp_player.opps = &opps;
			tmp_player.summary = s = &sum;
			tmp_player.lower_control_poker = player->lower_control_poker;
			tmp_player.oppDown_num = player->oppDown_num;
			tmp_player.oppUp_num = player->oppUp_num;
			memmove(&h, player->h, sizeof(POKERS));
			memmove(&opps, player->opps, sizeof(POKERS));
			remove_combo_poker(&h, c, NULL);
			//remove one/two poker bigger than max_ddz poker in c in opps.
			search_combos_in_suit(game, tmp_player.h, tmp_player.opps, &tmp_player);

			if (CONTROL_SUB_COMBO_NUM_IS_GOOD(&tmp_player)
				|| CONTROL_SUB_COMBO_NUM(&tmp_player) >= CONTROL_SUB_COMBO_NUM(player)
				|| CONTROL_NUM(&tmp_player) >= 30
				|| (CONTROL_SUB_COMBO_NUM(&tmp_player) + 10 >= CONTROL_SUB_COMBO_NUM(player)
					&& (is_combo_biggest(game, player->opps, c, player->oppDown_num, player->oppUp_num, player->lower_control_poker)
						|| (c->type != SINGLE && c->type != PAIRS))
					&& NOT_BOMB(c)
					//|| mustPlay
					)
				|| mustPlay)
			{
				if (c->type == BOMB_old || c->type == ROCKET)
				{
					good_bomb = 1;
					//only for win sure!
					/*
					if(player->oppDown_num >8 && player->oppUp_num>8 &&
					player->summary->combo_total_num>=3) //出炸弹晚一些
					{
					break;
					}
					/*
					if(pre->type==SINGLE && pre->low!=P2 &&
					((PRE_PLAYER(game)==UPFARMER_old && player->oppUp_num>3)//单排
					||(PRE_PLAYER(game)==DOWNFARMER_old && player->oppDown_num>6))
					&& player->summary->combo_total_num>=3
					)
					{
					break;
					}
					*/
					POKERS t, t1;
					int tmp_card[26], card[21];
					if (game->hun == -1)
					{
						remove_combo_in_suit(s, c);
						remove_combo_poker(player->h, c, NULL);
					}
					else
					{
						combo_2_poker(&t, c);
						convert_poker_to_hun(&t, &t1, player->h);
						sub_poker(player->h, &t1, player->h);
						memcpy(card, player->card, max_POKER_NUM * sizeof(int));
						// remove card 
						combo_to_int_array_hun(c,
							tmp_card, player->card, game->hun);

					}
					COMBO_OF_POKERS pre = game->pre;
					int prepre = game->prepre_playernum;
					int prep = game->pre_playernum;
					game->prepre_playernum = game->pre_playernum;
					game->pre_playernum = CUR_PLAYER;
					game->pre = *c;
					//game->players[CUR_PLAYER]->cur = c;
                    memcpy(&game->players[CUR_PLAYER]->curcomb, c, sizeof(COMBO_OF_POKERS));
					if ((game->use_best_for_undefined_case) && !Check_win_quick(game, DOWN_PLAYER, 0, 0, 0))
					{
						if (game->hun == -1)
						{
							POKERS p;
							combo_2_poker(&p, c);
							add_poker(player->h, &p, player->h);
							search_combos_in_suit(game, player->h, player->opps, player);
						}
						else
						{
							add_poker(player->h, &t1, player->h);
							//                        search_combos_in_suit(game, player->h, player->opps, player);
							memcpy(player->card, card, max_POKER_NUM * sizeof(int));

						}
						game->pre = pre;
						game->prepre_playernum = prepre;
						game->pre_playernum = prep;
						break;
					}
					else {
						if (game->hun == -1)
						{
							POKERS p;
							combo_2_poker(&p, c);
							add_poker(player->h, &p, player->h);
							//   search_combos_in_suit(game, player->h, player->opps, player);
						}
						else
						{
							add_poker(player->h, &t1, player->h);
							//                        search_combos_in_suit(game, player->h, player->opps, player);
							memcpy(player->card, card, max_POKER_NUM * sizeof(int));

						}
					}
					game->pre = pre;
					game->prepre_playernum = prepre;
					game->pre_playernum = prep;

				}
				PRINTF(LEVEL, "GuanPai \n");
				*cur = *c;
				return 0;
			}
			else
			{
				*p = *c;
				continue;
			}
		}
	}

    // 拆3带1
    int pass = 1;
    if (pre->type == SINGLE && s->three_one_num > 0)
    {
        COMBO_OF_POKERS three_one_tmp;
        three_one_tmp.type = NOTHING_old;
        for (int k = 0; k < s->three_one_num; k++)
        {
            // 先找可以的带牌
            if (s->three_one[k]->three_desc[0] > pre->low
                && s->three_one[k]->three_desc[0] <= player->lower_control_poker)
            {
                memcpy(&three_one_tmp, &s->three_one[k], sizeof(COMBO_OF_POKERS));

                cur->type = SINGLE;
                cur->low = s->three_one[k]->three_desc[0];
                cur->len = 1;

                pass = 0;

                memcpy(&s->three[s->three_num], &s->three_one[k], sizeof(COMBO_OF_POKERS));
                s->three_num++;
                s->three[s->three_num - 1]->type = THREE;
                s->three[s->three_num - 1]->three_desc[0] = 0;

                s->three_one_num--;
                memmove(&s->three_one[k], &s->three_one[k + 1], sizeof(COMBO_OF_POKERS) * (s->three_one_num - k));

                break;
            }
        }

        if (pass == 1)
        {
            for (int k = 0; k < s->three_one_num; k++)
            {
                // 没有可以各的带牌，找能管上的带牌
                if (s->three_one[k]->three_desc[0] > pre->low)
                {
                    memcpy(&three_one_tmp, &s->three_one[k], sizeof(COMBO_OF_POKERS));

                    cur->type = SINGLE;
                    cur->low = s->three_one[k]->three_desc[0];
                    cur->len = 1;

                    pass = 0;

                    memcpy(&s->three[s->three_num], &s->three_one[k], sizeof(COMBO_OF_POKERS));
                    s->three_num++;
                    s->three[s->three_num - 1]->type = THREE;
                    s->three[s->three_num - 1]->three_desc[0] = 0;

                    s->three_one_num--;
                    memmove(&s->three_one[k], &s->three_one[k + 1], sizeof(COMBO_OF_POKERS) * (s->three_one_num - k));

                    break;
                }
            }
        }

        if (three_one_tmp.type == THREE_ONE)
        {
            bool fixed = false;
            // 修改not_biggest 和 biggest中对应的套
            for (int k = 0; fixed && k < s->not_biggest_num; ++k)
            {
                if (s->not_biggest[k]->type == three_one_tmp.type
                    && s->not_biggest[k]->low == three_one_tmp.low)
                {
                    s->not_biggest[k]->type = THREE;
                    s->not_biggest[k]->three_desc[0] = 0;
                    fixed = true;
                }
            }
            for (int k = 0; fixed && k < s->biggest_num; ++k)
            {
                if (s->biggest[k]->type == three_one_tmp.type
                    && s->biggest[k]->low == three_one_tmp.low)
                {
                    s->biggest[k]->type = THREE;
                    s->biggest[k]->three_desc[0] = 0;
                    fixed = true;
                }
            }
        }

        if (pass == 0)
        {
            return pass;
        }
    }

	//Chai Pai
	//没有找到大的牌才进入拆牌
    GAME_DDZ_AI game_tmp;
    GAME_copy(&game_tmp, game);
    search_combos_in_suit(&game_tmp, game_tmp.players[CUR_PLAYER]->h, game_tmp.players[CUR_PLAYER]->opps, game_tmp.players[CUR_PLAYER]);
    if (game->known_others_poker)
    {
        search_combos_in_suit(&game_tmp, game_tmp.players[DOWN_PLAYER]->h, game_tmp.players[DOWN_PLAYER]->opps, game_tmp.players[DOWN_PLAYER]);
        search_combos_in_suit(&game_tmp, game_tmp.players[UP_PLAYER]->h, game_tmp.players[UP_PLAYER]->opps, game_tmp.players[UP_PLAYER]);
    }
	if (!Lord_Chaipai(&game_tmp, cur, pre, good_bomb, mustPlay))
	{
		if (mustPlay)
		{
            if (game->use_best_for_undefined_case && _CheckWinWrapper(game, cur))
			{
				return 0;
			}
			else if (game->use_best_for_undefined_case)
			{
				get_the_best_hand(game, cur, 0); // 必出条件，返回值可能为PASS
			}
		}

        if (game->use_best_for_undefined_case && (cur->type == BOMB_old || cur->type == ROCKET)
            && game_tmp.players[CUR_PLAYER]->summary->bomb_num + 1 < game_tmp.players[CUR_PLAYER]->summary->real_total_num)
        {
            if (!_CheckWinWrapper(game, cur))
            {
                return 1;
            }
        }

		//如果之前找到非炸弹的combo能管，但在拆牌中拆了XD，则选择PASS
		//TODO：需要仔细检查为什么出2不出选择拆王炸，拆牌逻辑里应该会有点问题
		if (findBigger && cur->type == SINGLE && cur->low >= LIT_JOKER
			&& player->h->hands[LIT_JOKER] > 0 && player->h->hands[BIG_JOKER] > 0)
		{
			return 1;
		}
		return 0;
	}

	return 1;
}

//test if c is smaller than a
int is_smaller(COMBO_OF_POKERS* c, COMBO_OF_POKERS* a)
{
	if (c == NULL || a == NULL)
		return 0;
	if (c->type != NOTHING_old && c->type == a->type && c->low <a->low)
	{
		if (c->type != SINGLE_SERIES || c->len == a->len)
			return 1;
	}
	return 0;
}

//get number of combos in tobetested which is smaller than cur.
int get_num_of_smaller(COMBO_OF_POKERS* tobetested, int num, COMBO_OF_POKERS* cur)
{
	int t = 0;
	for (int i = 0; i<num; i++)
	{
		if (is_smaller(&tobetested[i], cur))
		{
			t++;
		}
	}
	return t;
}

//get number of combos in tobetested which is bigger than cur.
int get_num_of_bigger(COMBO_OF_POKERS* tobetested, int num, COMBO_OF_POKERS* cur)
{
	int t = 0;
	for (int i = 0; i<num; i++)
	{
		if (is_smaller(cur, &tobetested[i]))
		{
			t++;
		}
	}
	return t;
}

//get number of combos in s2 which there is a combo bigger in s1  than s2.
int get_num_of_bigger_in_combos(COMBO_OF_POKERS** s1, int n1, COMBO_OF_POKERS* s2)
{
	int t = 0;
	// for(int i=0;i<n2;i++)
	{
		for (int j = 0; j<n1; j++)
			if (is_smaller(s2, s1[j]))
			{
				t++;
				break;
			}
	}
	return t;
}

//get number of combos in s2 which there is a combo bigger in s1  than s2.
int get_num_of_smaller_in_combos(COMBO_OF_POKERS** s1, int n1, COMBO_OF_POKERS** s2, int n2)
{
	int t = 0;
	for (int i = 0; i<n2; i++)
	{
		for (int j = 0; j<n1; j++)
			if (is_smaller(s1[j], s2[i]))
			{
				t++;
				break;
			}
	}
	return t;
}


void farmer_normal_play_first(GAME_DDZ_AI * game, COMBO_OF_POKERS* c)//select c from player
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS* t = NULL;
	t = farm_select_combo_in_suit(game);

	if (t == NULL)
	{
		PRINTF_ALWAYS("ERR: line %d lord play first: stupid error\n", __LINE__);
		return;
	}
	*c = *t;
}

void downfarmer_good_play_first(GAME_DDZ_AI * game, COMBO_OF_POKERS* cur)//select cur from player
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	COMBOS_SUMMARY_DDZ_AI *s = player->summary;
	COMBO_OF_POKERS* t = NULL;
	//   {
	if (s->not_biggest_num == 2 && s->real_total_num == 2)
	{
		/*如果2种牌各剩1套，且没冲锋套时。如果有3张以上套，则优先出；如果只有单/对，则出号码小的那套，留号码大的套
		*/
		if (player->summary->singles_num == 1 && player->summary->pairs_num == 1)
		{
			/*
			for(int i=player->h->begin;i<player->h->end;i++)
			{
			cur->type=player->h->hands[i]==1?SINGLE:PAIRS;
			cur->low = i ;
			player->h->hands[i]=0;
			}t
			*/
			if (player->summary->singles[0]->low > player->summary->pairs[0]->low)
			{
				memmove(cur, player->summary->pairs[0], sizeof(COMBO_OF_POKERS));
				player->summary->pairs[0]->type = NOTHING_old;
			}
			else {
				memmove(cur, player->summary->singles[0], sizeof(COMBO_OF_POKERS));
				player->summary->singles[0]->type = NOTHING_old;
			}
			remove_combo_in_suit(player->summary, cur);//to be optimized
		}
		else if (player->summary->singles_num == 2)
		{
			memmove(cur, player->summary->singles[0], sizeof(COMBO_OF_POKERS));
			player->summary->singles[0]->type = NOTHING_old;
			remove_combo_in_suit(player->summary, cur);//to be optimized
		}
		else if (player->summary->pairs_num == 2)
		{
			memmove(cur, player->summary->pairs[0], sizeof(COMBO_OF_POKERS));
			player->summary->pairs[0]->type = NOTHING_old;
			remove_combo_in_suit(player->summary, cur);//to be optimized
		}
		else
		{
			for (int k = 20; k >= 0; k--)
			{
				if (get_combo_number(&player->combos[k]) >= 3)
				{
					memmove(cur, &player->combos[k], sizeof(COMBO_OF_POKERS));
					player->combos[k].type = NOTHING_old;
					remove_combo_in_suit(player->summary, cur);//to be optimized
					return;
				}
			}
		}
		return;
	}

	t = farm_select_combo_in_suit(game);

	if (t == NULL)
	{
		PRINTF_ALWAYS("ERR: line %d up farmaer play first: stupid error\n", __LINE__);
		return;
	}
	*cur = *t;
}

void farmer_select_one_combo_for_other_farmer(GAME_DDZ_AI* game, PLAYER* pl, COMBO_OF_POKERS* cur, int isup)
{
	FUNC_NAME
		PLAYER* lord = game->player_type == UPFARMER_old ?
		game->players[DOWN_PLAYER] :
		game->players[UP_PLAYER];
	PLAYER* other = game->player_type == DOWNFARMER_old ?
		game->players[DOWN_PLAYER] :
		game->players[UP_PLAYER];

	search_combos_in_suit(game,
		game->players[DOWN_PLAYER]->h,
		game->players[DOWN_PLAYER]->opps,
		game->players[DOWN_PLAYER]);
	search_combos_in_suit(game,
		game->players[UP_PLAYER]->h,
		game->players[UP_PLAYER]->opps,
		game->players[UP_PLAYER]);

	PRINTF(LEVEL, "ENTER Fun farmer_select_one_combo_for_other_farmer: up %d\n", isup);

	if (isup)
	{
		/*
		if (is_player_ready_win(game,other))
		{
		//select a combo which other farmer needed.
		}
		*/
		//select a three one
		if (other->summary->three_one_num >= 1 && lord->summary->three_one_num == 0 && lord->summary->three_two_num == 0)
		{
			if (find_a_smaller_combo_in_hands(pl->h, cur,
				other->summary->three_one[other->summary->three_one_num - 1]))
			{
				POKERS t;
				cur->type = THREE;
				memmove(&t, pl->h, sizeof(POKERS));
				remove_combo_poker(&t, cur, NULL);
				search_combos_in_suit(game, &t, pl->opps, pl);
				cur->type = 31;
				search_1_2_for_a_three(&t, pl, cur);
				pl->need_research = 1;
				return;
			}
		}
		//select a pairs
		if (other->summary->pairs_num >= 1 &&
			(lord->summary->pairs_num == 0 ||
				lord->summary->pairs[0]->low >= P2 ||
				lord->summary->pairs[(lord->summary->pairs_num - 1) / 2]->low < other->summary->pairs[(other->summary->pairs_num) / 2]->low)
			)
		{
			do {
				if (lord->summary->pairs_num == 0)
				{
					int finded = find_a_smaller_combo_in_hands(pl->h, cur, other->summary->pairs[other->summary->pairs_num - 1]);
					if (!finded) break;
					else
					{
						pl->need_research = 1;
						return;
					}
				}
				else
				{
					int finded = find_a_bigger_combo_in_hands(pl->h, cur, lord->summary->pairs[(lord->summary->pairs_num) / 2]);
					if (!finded) break;
					else if (cur->low <  other->summary->pairs[(other->summary->pairs_num) / 2]->low
						&& cur->low<lord->lower_control_poker)
					{
						pl->need_research = 1;
						return;
					}
					else
						break;
				}
			} while (1);
		}
		//select a big single
		{
			//get the singles of lord..
			int single = char_to_poker('J');

			COMBOS_SUMMARY_DDZ_AI * s = game->players[DOWN_PLAYER]->summary;

			for (int i = s->singles_num - 1; i >= 0; i--)
			{
				if (s->singles[i]->low <= char_to_poker('K'))
				{
					single = s->singles[i]->low;
					break;
				}
			}
			if (s->singles_num >= 3)
			{
				if (single > s->singles[2]->low)
					single = s->singles[2]->low;
			}



			//   int pass = 1;
			for (int i = single; i <= char_to_poker('K'); i++)
			{
				if (pl->h->hands[i] == 1 || pl->h->hands[i] == 2)
				{
					cur->type = SINGLE;
					cur->low = i;
					cur->len = 1;
					pl->h->hands[i] = 0;
					pl->need_research = 1;
					search_combos_in_suit(game, pl->h, pl->opps, pl);
					pl->h->hands[i]++;
					DBG(PRINTF(LEVEL, "!Bad Up Farmer Gepai\n"));
					//   pass = 0;
					return;
				}
			}
			//    if (pass == 1)
		}

		farmer_normal_play_first(game, cur);

	}
	else
	{
		COMBOS_SUMMARY_DDZ_AI * s = other->summary;
		//寻找牌组中，套数最多的一类牌
		int max_ddz = 0, idx = 0;
		if (s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num >max_ddz) {
			max_ddz = s->threeseries_num + s->threeseries_one_num + s->threeseries_two_num;
			idx = 1;
		}
		if (s->pairs_series_num>max_ddz) {
			max_ddz = s->pairs_series_num;
			idx = 2;
		}
		for (int i = 0; i < 10; i++)
		{
			if (s->series_detail[i] > max_ddz) {
				max_ddz = s->series_detail[i];
				idx = 6;
			}
		}

		if (s->three_one_num + s->three_two_num + s->three_num>max_ddz) {
			max_ddz = s->three_one_num + s->three_two_num + s->three_num;
			idx = 3;
		}
		if (s->pairs_num>max_ddz) {
			max_ddz = s->pairs_num;
			idx = 4;
		}
		if (s->singles_num>max_ddz) {
			max_ddz = s->singles_num;
			idx = 5;
		}

		if (max_ddz >= 1) //todo: check for controls..
		{
			COMBO_OF_POKERS * t = NULL;
			if (idx == 4 && pl->summary->pairs_num >= 1) {
				t = pl->summary->pairs[0];
			}
			else if (idx == 5 && pl->summary->singles_num >= 1) {
				t = pl->summary->singles[0];
			}

			//check for controls..
			if (t != NULL)
			{
				bool isBiggest = false;
				for (int i = 0; i < pl->summary->biggest_num; ++i)
				{
					if (pl->summary->biggest[i]->type == t->type
						&& pl->summary->biggest[i]->low == t->low)
					{
						isBiggest = true;
						break;
					}
				}
				//todo
				COMBO_OF_POKERS *tmp;
				if (!isBiggest || ((tmp = find_combo(other->summary, t)) && tmp != NULL && tmp->type != BOMB_old && tmp->type != ROCKET))
				{
					memmove(cur, t, sizeof(COMBO_OF_POKERS));
					remove_combo_in_suit(pl->summary, t);
					t->type = NOTHING_old;
					return;
				}
			}
		}
		farmer_normal_play_first(game, cur);
	}

}
/*
void upfarmer_play_first_known_others(GAME_DDZ_AI* game,PLAYER* pl,COMBO_OF_POKERS* cur,int is_good)
{
//up_farmer is better than down_farmer
//int is_good = check_farmer_is_good(game);
if ( pl->oppDown_num == 1)
{
farmer_play_first_lord_has_1_poker_only(game,cur,is_good,cur,1);
return;
}

if(is_good)
{
upfarmer_good_play_first(game,cur);
}
else
{
farmer_select_one_combo_for_other_farmer(game,pl,cur,1);
}
}

void downfarmer_play_first_known_others(GAME_DDZ_AI* game,PLAYER* pl,COMBO_OF_POKERS* cur,int is_good)
{
if(is_good)
{
downfarmer_good_play_first(game, cur);
}
else
{
farmer_select_one_combo_for_other_farmer(game,pl,cur,0);
}
}

*/

int must_play_bigger_to_stop_lord(COMBO_OF_POKERS * cur, PLAYER* player, COMBO_OF_POKERS *pre, GAME_DDZ_AI * game, int isup, int isgood, int isfarmer)
{
	FUNC_NAME
		//printf("called\n");
		COMBO_OF_POKERS * c;
	int pass = 1;
	if (pass == 1) {
		COMBO_OF_POKERS tmp1, tmp, *p = &tmp;
		c = &tmp1;
		memmove(p, pre, sizeof(COMBO_OF_POKERS));
		int first_time = 1;
		COMBOS_SUMMARY_DDZ_AI s;
		while (find_a_bigger_combo_in_hands(player->h, c, p)) //could not handle corner case for  BOMB_old in two combos.
		{
			//检查春天
			if (game->known_others_poker && game->players[CUR_PLAYER]->h->total >= 17 &&
				((game->players[CUR_PLAYER]->id == DOWNFARMER_old && game->players[CUR_PLAYER]->oppDown_num >= 17 && game->players[UP_PLAYER]->summary->real_total_num <= 1)
					|| (game->players[CUR_PLAYER]->id == UPFARMER_old && game->players[CUR_PLAYER]->oppUp_num >= 17 && game->players[DOWN_PLAYER]->summary->real_total_num <= 1)))
			{
				*cur = *c;
				return 0;
			}

			if (c->type == ROCKET || c->type == BOMB_old)
			{
				if (player->summary->combo_total_num == 1
					&& !opp_hasbomb(game)) //to be refine
				{
					*cur = *c;
					return 0;
				}
				else  if (game->known_others_poker)
				{
					PLAYER *lord = player->id == UPFARMER_old ?
						game->players[DOWN_PLAYER] :
						game->players[UP_PLAYER];
					/*if(lord->summary->control_num ==0
					&& lord->summary->bomb_num ==0)
					{
					*cur=*c;
					return 0;
					} */
					search_combos_in_suit(game, player->h, player->opps, player);
					if (game->use_best_for_undefined_case)
					{ //todo: use check win
						int pass = get_the_best_hand(game, cur, 0);
                        if (!pass)
                        {
                            return pass;
                        }
					}
				}

				if (first_time) {
					*p = *c;
					continue;
				}
			}

			if (c->type == ROCKET)
			{
				if (player->summary->real_total_num == 1) //to be refine
				{
					*cur = *c;
					return 0;
				}
				if (first_time)
					return 1;
			}

            if (game->hun == -1) //？？？？
            {
                POKERS t;
                remove_combo_poker(player->h, c, NULL);
                search_combos_in_suit(game, player->h, player->opps, player);
                combo_2_poker(&t, c);
                add_poker(player->h, &t, player->h);
            }
            else
            {
                POKERS t, t1;
                combo_2_poker(&t, c);
                convert_poker_to_hun(&t, &t1, player->h);
                sub_poker(player->h, &t1, player->h);
                search_combos_in_suit(game, player->h, player->opps, player);
                add_poker(player->h, &t1, player->h);
                if (c->type >= THREE_ONE)
                {

                    c->control = 2;//tmporary use
                }
                else
                    c->control = 0;//tmporary use        


            }

			if (first_time)
			{
				s = *player->summary;
				*cur = *c;
				first_time = 0;
			}
			else if (cmp_summary(player->summary, &s) || !cmp_summary(&s, player->summary))
			{
				s = *player->summary;
				*cur = *c;
			}

			*p = *c;
		}
		//}
		pass = first_time;
	}
	cur->control = 0;
	return pass;
}

// search in player to get a bigger one than pre
// 1. search in summary first
// 2. search in hands, if find one, re-arrange hands and save the combo summary to player->summary
//save result to cur
int must_play_bigger(COMBO_OF_POKERS * cur, PLAYER* player, COMBO_OF_POKERS *pre, GAME_DDZ_AI * game, int isup, int isgood, int isfarmer)
{
	FUNC_NAME
		COMBO_OF_POKERS * c = find_combo(player->summary, pre);
	int pass = 1;
	if (c != NULL && c->type != BOMB_old && c->type != ROCKET)// !(isup && !isgood && isfarmer))  //up bad farmer do not play bomb here.
	{
		remove_combo_in_suit(player->summary, c);
		memmove(cur, c, sizeof(COMBO_OF_POKERS));
		c->type = NOTHING_old;
		pass = 0;
		DBG(PRINTF(LEVEL, "!NongMINg Biguan: shunpai\n"));
		return pass;
	}
	//if(c==NULL) //有炸弹不瞎拆
	{
		COMBO_OF_POKERS tmp1, tmp, *p = &tmp;
		c = &tmp1;
		memmove(p, pre, sizeof(COMBO_OF_POKERS));
		int first_time = 1;
		PLAYER* suit1 = (PLAYER*)malloc(sizeof(PLAYER));
		suit1->combos = suit1->combos_store;
		suit1->h = &suit1->p;
		suit1->opps = &suit1->opp;
		suit1->summary = &suit1->combos_summary;

		//int prob1 = get_prob(game);
		//int prob2 = get_prob_2(game);
		while (find_a_bigger_combo_in_hands(player->h, &tmp1, p)) //could not handle corner case for  BOMB_old in two combos.
		{
			if (tmp1.type == BOMB_old || tmp1.type == ROCKET)
				break;
			if (c->type >= THREE_ONE)
			{

				c->control = 2;//tmporary use
			}
			else
				c->control = 0;//tmporary use

			suit1->lower_control_poker = player->lower_control_poker;
			suit1->oppDown_num = player->oppDown_num;
			suit1->oppUp_num = player->oppUp_num;
			*suit1->h = *player->h;
			*suit1->opps = *player->opps;
			remove_combo_poker(suit1->h, &tmp1, NULL);//todo: fixme

			search_combos_in_suit(game, suit1->h, suit1->opps, suit1);
			if (suit1->lower_control_poker<player->lower_control_poker
				&& suit1->summary->ctrl.single > player->summary->ctrl.single)
			{//shit
				PRINTF(LEVEL, "pull back single control\n");
				suit1->summary->ctrl.single = player->summary->ctrl.single - 1;
			}

			//update player in hands;
			PRINTF(LEVEL, "\n!farmer biguan, new_combo_num %d, new_ctrl %d pre_ctrl %d, pre_combo_num %d\n"
				, suit1->summary->combo_total_num, CONTROL_NUM(suit1), CONTROL_NUM(player), player->summary->combo_total_num
			);

			if ((first_time && suit1->summary->bomb_num >= player->summary->bomb_num)
				|| (CONTROL_SUB_COMBO_NUM(suit1) > CONTROL_SUB_COMBO_NUM(player))
				|| ((CONTROL_SUB_COMBO_NUM(suit1) == CONTROL_SUB_COMBO_NUM(player))
					&& suit1->summary->combo_total_num < player->summary->combo_total_num)
				|| ((CONTROL_SUB_COMBO_NUM(suit1) == CONTROL_SUB_COMBO_NUM(player))
					&& suit1->summary->combo_total_num == player->summary->combo_total_num
					&& suit1->summary->combo_typenum < player->summary->combo_typenum)
				|| ((CONTROL_SUB_COMBO_NUM(suit1) == CONTROL_SUB_COMBO_NUM(player))
					&& suit1->summary->combo_total_num == player->summary->combo_total_num
					&& (suit1->summary->singles_num< player->summary->singles_num ||
						suit1->summary->real_total_num< player->summary->real_total_num)
					)

				/*  ||  ( (CONTROL_SUB_COMBO_NUM(&suit1) >= (CONTROL_SUB_COMBO_NUM(player)+10) )
				&& get_prob1(game) )
				||   ( (CONTROL_SUB_COMBO_NUM(&suit1) >= (CONTROL_SUB_COMBO_NUM(player)+10) )
				&& get_prob1(game) )*/
				)
			{
				PRINTF(LEVEL, "got better\n");
				*player->summary = *suit1->summary;
				first_time = 0;
				*cur = tmp1;
			}
			*p = tmp1;
		}
		//}
		free(suit1);
		if (!first_time)
			search_combos_in_suit(game, player->h, player->opps, player);
		cur->control = 0;
		return first_time;
	}
	return pass;
}


//地主报单农民必管逻辑
int must_play_bigger_for_single(COMBO_OF_POKERS * cur, PLAYER* player, COMBO_OF_POKERS *pre, GAME_DDZ_AI * game, int isup, int isgood, int isfarmer)
{
	FUNC_NAME
		COMBO_OF_POKERS * c = find_combo(player->summary, pre);
	int pass = 1;
	if (c == NULL) {
		//拆牌
		COMBO_OF_POKERS tmp1, tmp, *p = &tmp;
		c = &tmp1;
		memmove(p, pre, sizeof(COMBO_OF_POKERS));
		int first_time = 1;
		PLAYER suit1;
		POKERS h, opps;
		COMBO_OF_POKERS combo[20] = { 20 * 0 };
		COMBOS_SUMMARY_DDZ_AI sum = { 0 }, *s;
		suit1.combos = &combo[0];
		suit1.h = &h;
		suit1.opps = &opps;
		suit1.summary = s = &sum;
		while (find_a_bigger_combo_in_hands(player->h, c, p))
		{
			suit1.lower_control_poker = player->lower_control_poker;
			suit1.oppDown_num = player->oppDown_num;
			suit1.oppUp_num = player->oppUp_num;
			memmove(&h, player->h, sizeof(POKERS));

			memmove(&opps, player->opps, sizeof(POKERS));
			remove_combo_poker(suit1.h, c, NULL);
			if (c->type >= THREE_ONE)
			{

				c->control = 2;//tmporary use
			}
			else
				c->control = 0;//tmporary use
			search_combos_in_suits_for_MIN_single_farmer(game, suit1.h, suit1.combos, &suit1);

			//update player in hands;
			DBG(PRINTF(LEVEL, "\n!farmer biguan, new_combo_num %d, new_ctrl %d pre_ctrl %d, pre_combo_num %d\n"
				, suit1.summary->combo_total_num, CONTROL_NUM(&suit1), CONTROL_NUM(player), player->summary->combo_total_num
			));
			if (first_time
				|| (isup && get_2nd_MIN_singles(suit1.summary) > get_2nd_MIN_singles(player->summary))//寻找第二小的单牌最大的组合
				|| (!isup && suit1.summary->singles_num < player->summary->singles_num)//寻找单牌最少的组合
				)
			{
				*player->summary = *suit1.summary;
				first_time = 0;
				*cur = *c;
			}
			*p = *c;
		}
		cur->control = 0;
		//}
		return first_time;
	}
	/*//地主报单农民必管
	else if ( c->type!=BOMB_old || 1)// !(isup && !isgood && isfarmer)) //检查炸弹？
	{
	remove_combo_in_suit(player->summary,c);
	memmove(cur,c,sizeof(COMBO_OF_POKERS));
	c->type=NOTHING_old;
	pass=0;
	DBG( PRINTF(LEVEL,"!NongMINg Biguan: shunpai\n"));
	}
	*/
	else if (c->type != BOMB_old && c->type != ROCKET)
	{
		*cur = *c;
		pass = 0;
	}

    if (pass == 1 && game->use_best_for_undefined_case)
    {
        return get_the_best_hand(game, cur, 0);
    }

	return pass;
}
/*
int farmer_shunpai(PLAYER * player ,COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre,GAME_DDZ_AI * game)
{

}
*/
int farmer_Chaipai(GAME_DDZ_AI*game, COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre)
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	COMBO_OF_POKERS tmp1;
	COMBO_OF_POKERS* c = &tmp1;
	COMBO_OF_POKERS tmp, *p = &tmp;
	memmove(p, pre, sizeof(COMBO_OF_POKERS));
	int first_time = 1;
	int mustPlay = 0;
	// COMBOS_SUMMARY_DDZ_AI *s =player->summary;

	//to be optimized by using new search methods..
	while (find_a_bigger_combo_in_hands(player->h, c, p)) //could not handle corner case for  BOMB_old in two combos.
	{
		if (c->type == BOMB_old || c->type == ROCKET)
			break;
		if (c->type >= THREE_ONE)
		{
			c->control = 2;//tmporary use
		}
		else
			c->control = 0;//tmporary use 
		PLAYER tmp_player;
		POKERS h, opps;
		COMBO_OF_POKERS combo[20] = { 20 * 0 };
		COMBOS_SUMMARY_DDZ_AI sum = { 0 }, *s;
		tmp_player.combos = &combo[0];
		tmp_player.h = &h;
		tmp_player.opps = &opps;
		tmp_player.summary = s = &sum;
		tmp_player.lower_control_poker = player->lower_control_poker;
		tmp_player.oppDown_num = player->oppDown_num;
		tmp_player.oppUp_num = player->oppUp_num;
		memmove(&h, player->h, sizeof(POKERS));

		memmove(&opps, player->opps, sizeof(POKERS));
		remove_combo_poker(tmp_player.h, c, NULL);

		search_combos_in_suit(game, tmp_player.h, tmp_player.opps, &tmp_player);

		int MAX_poker_in_c = has_control_poker(c, player->lower_control_poker);

		if (c->type == SINGLE || c->type == PAIRS)
		{
			if (c->low >= player->lower_control_poker)
				assume_remove_controls_in_opps(&opps, c, MAX_poker_in_c);
			int ctrl_num = CONTROL_POKER_NUM;
			POKERS all;
			add_poker(tmp_player.h, &opps, &all);
			if (all.total <= 20)
				ctrl_num = 4;
			else if (all.total <= 10)
				ctrl_num = 2;
			tmp_player.lower_control_poker = get_lowest_controls(&all, ctrl_num);
			tmp_player.summary->ctrl.single = calc_controls(tmp_player.h, tmp_player.opps, ctrl_num);
		}
		if (tmp_player.lower_control_poker<player->lower_control_poker
			&& tmp_player.summary->ctrl.single > player->summary->ctrl.single)
		{
			PRINTF(LEVEL, "pull back single control\n");
			tmp_player.summary->ctrl.single = player->summary->ctrl.single - 1;
		}


		//update the lowest control poker and update the summary

		if (CONTROL_SUB_COMBO_NUM_IS_GOOD(&tmp_player)
			|| CONTROL_SUB_COMBO_NUM(&tmp_player) >= CONTROL_SUB_COMBO_NUM(player)
			|| CONTROL_NUM(&tmp_player) >= 30
			|| tmp_player.summary->combo_total_num <= 1
			|| (PRE_PLAYER(game) == LORD_old &&
				CONTROL_SUB_COMBO_NUM(&tmp_player) + 10 >= CONTROL_SUB_COMBO_NUM(player)
				&& NOT_BOMB(c))
			)
		{
			goto SAVE_CUR_RESULT;
		}
		else
		{
			*p = *c;
			continue;
		}

	SAVE_CUR_RESULT:
		//update player in hands;
		PRINTF(LEVEL, "!farmer chaipai, ctrl %d control-combo %d combo_num %d, old(%d,%d,%d)\n",
			tmp_player.summary->ctrl.single, CONTROL_SUB_COMBO_NUM(&tmp_player), tmp_player.summary->combo_total_num, player->summary->ctrl.single,
			CONTROL_SUB_COMBO_NUM(player), player->summary->combo_total_num
		);
		print_combo_poker(c);
		if (first_time
			|| (CONTROL_SUB_COMBO_NUM(&tmp_player) > CONTROL_SUB_COMBO_NUM(player))
            || (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player) 
                && tmp_player.summary->extra_bomb_ctrl > player->summary->extra_bomb_ctrl)
			|| ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
				&& tmp_player.summary->combo_total_num < player->summary->combo_total_num)
			|| ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
				&& tmp_player.summary->combo_total_num == player->summary->combo_total_num
				&& tmp_player.summary->combo_typenum < player->summary->combo_typenum)
			|| ((CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player))
                && tmp_player.summary->extra_bomb_ctrl == player->summary->extra_bomb_ctrl
                && tmp_player.summary->combo_total_num == player->summary->combo_total_num
				&& tmp_player.summary->combo_typenum == player->summary->combo_typenum
				&& tmp_player.summary->singles_num < player->summary->singles_num)
			/* ||   ( (CONTROL_SUB_COMBO_NUM(&tmp_player) == CONTROL_SUB_COMBO_NUM(player) )
			&& tmp_player.summary->combo_total_num == player->summary->combo_total_num
			&& tmp_player.summary->combo_typenum == player->summary->combo_typenum
			&& tmp_player.summary->singles_num == player->summary->singles_num )*/
			)
		{
			*player->summary = *tmp_player.summary;
			first_time = 0;
			*cur = *c;
		}
		*p = *c;
	}
	cur->control = 0;//reset..
	return first_time;
}

int farmer_play_normal(GAME_DDZ_AI*game, COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre)
{
	FUNC_NAME
		PLAYER * player = game->players[CUR_PLAYER];
	int mustPlay = 0;
	//    if(!farmer_shunpai(player, cur, pre, game))
	//      return 0;

    if (PRE_PLAYER(game) != LORD_old)
    {
        if (pre->type >= 3311 || pre->type == PAIRS_SERIES || pre->low >= Pk)
        {
            return 1;
        }
    }

	//Chai Pai
	if (!farmer_Chaipai(game, cur, pre))
		return 0;

	/* //use bomb..
	if( PRE_PLAYER(game)== LORD_old ) //double check!!
	{
	return get_the_best_hand(game, cur, 0);
	}
	*/
	return 1;
}

int farmer_play_when_lord_has_1_poker_only(COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre, PLAYER*player, GAME_DDZ_AI*game, int isup, int isgood)
{
	FUNC_NAME
		SET_PLAYER_POINT(game);
	if (game->players[game->pre_playernum]->id == LORD_old && pre->type != SINGLE)
	{
		//地主报单农民必管逻辑
		search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);
		return must_play_bigger_for_single(cur, player, pre, game, isup, isgood, 1);
	}
	else
	{
		if (pre->type == SINGLE)
		{
			if (game->players[game->pre_playernum]->id == UPFARMER_old)
			{   //当前为地主下家农民
				goto _CHECK_GOOD_ENOUGH;
			}

			int lord_single = player->opps->end;
			if (game->known_others_poker)
			{
				lord_single = downPl->h->end;
			}
			search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);
			if (get_2nd_MIN_singles(player->summary)> lord_single)
			{
				COMBO_OF_POKERS *c;
				if (player->summary->singles_num>1
					&& player->summary->singles[player->summary->singles_num - 1]->low
				> max_ddz(pre->low, lord_single)) {
					*cur = *player->summary->singles[player->summary->singles_num - 1];
					return 0;
				}
				c = find_combo((player->summary), pre);
				if (c != NULL)
				{
					*cur = *c;
					return 0;
				}
			}
			for (int k = max_ddz(pre->low + 1, lord_single); k <= player->h->end; k++)
			{
				if (player->h->hands[k]>0)
				{
					cur->type = SINGLE;
					cur->low = k;
					cur->len = 1;
					player->h->hands[k]--;
					search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);
					player->h->hands[k]++;
					if (get_2nd_MIN_singles(player->summary)> lord_single)
					{
						return 0;
					}
				}
			}
			//如果该牌为外面最大.. 则过牌？
			if (game->players[game->pre_playernum]->id == DOWNFARMER_old
				&& (pre->low >= lord_single)
				)
			{
				return 1;
			}
			else
			{
                if (game->players[CUR_PLAYER]->id == DOWNFARMER_old)
                {
                    //如果手中有绝对大的单牌，则出
                    for (int k = max_ddz(pre->low + 1, lord_single); k <= player->h->end; k++)
                    {
                        if (player->h->hands[k] == 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                return 0;
                            }
                        }
                    }
                    for (int k = max_ddz(pre->low + 1, lord_single); k <= player->h->end; k++)
                    {
                        if (player->h->hands[k] > 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                //player->h->hands[i]--;
                                return 0;
                            }
                        }
                    }
                }
                else //if (game->players[CUR_PLAYER]->id == UPFARMER_old)
                {
                    for (int k = player->opps->end; k <= player->h->end && k > pre->low; ++k)
                    {
                        if (player->h->hands[k] == 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                return 0;
                            }
                        }
                    }
                    for (int k = player->opps->end; k <= player->h->end && k > pre->low; ++k)
                    {
                        if (player->h->hands[k] > 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                //player->h->hands[i]--;
                                return 0;
                            }
                        }
                    }
                    for (int k = player->h->end; k >= player->h->begin && k > pre->low; --k)
                    {
                        if (player->h->hands[k] == 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                return 0;
                            }
                        }
                    }
                    for (int k = player->h->end; k >= player->h->begin && k > pre->low; --k)
                    {
                        if (player->h->hands[k] > 1)
                        {
                            cur->type = SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            {
                                return 0;
                            }
                        }
                    }
                }

				if (player->summary->bomb_num != 0)//
				{
					if (isup)
					{
						memmove(cur, player->summary->bomb[0], sizeof(COMBO_OF_POKERS));
						player->summary->bomb[0]->type = NOTHING_old;
						remove_combo_in_suit(player->summary, cur);

						return 0;
					}
					else
					{
						POKERS t;
						memmove(&t, player->h, sizeof(POKERS));
						remove_combo_poker(&t, player->summary->bomb[0], NULL); //todo: for mulit bomb
						memmove(cur, player->summary->bomb[0], sizeof(COMBO_OF_POKERS));
						player->summary->bomb[0]->type = NOTHING_old;
						remove_combo_in_suit(player->summary, cur);
						search_combos_in_suits_for_MIN_single_farmer(game, &t, player->combos, player);
						if (get_2nd_MIN_singles(player->summary) >= player->opps->end)
						{   //heshou
							return 0;
						}
						else
						{
							search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);
						}
						return 1;
					}
				}

				if (isup)
				{
					if (player->h->end>pre->low)
					{
						cur->type = SINGLE;
						cur->low = player->h->end;
						cur->len = 1;
						player->need_research = 1;
						POKERS t;
						memmove(&t, player->h, sizeof(POKERS));
						remove_combo_poker(&t, cur, NULL);
						search_combos_in_suit(game, &t, player->opps, player);
						//player->h->hands[i]--;
						return 0;
					}
				}
				return 1;
			}
		}
		else //另外一个农民出的非单牌
		{
		_CHECK_GOOD_ENOUGH:
			search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);
			if (get_2nd_MIN_singles(player->summary) >= player->opps->end) //合手
			{
				COMBO_OF_POKERS* c = find_combo(player->summary, pre);//
				if (c == NULL)
				{
					COMBO_OF_POKERS tmp1, tmp, *p = &tmp;
					c = &tmp1;
					memmove(p, pre, sizeof(COMBO_OF_POKERS));
					int first_time = 1;
					PLAYER suit1;
					POKERS h, opps;
					COMBO_OF_POKERS combo[20] = { 20 * 0 };
					COMBOS_SUMMARY_DDZ_AI sum = { 0 }, *s;
					suit1.combos = &combo[0];
					suit1.h = &h;
					suit1.opps = &opps;
					suit1.summary = s = &sum;
					while (find_a_bigger_combo_in_hands(player->h, c, p)) //could not handle corner case for  BOMB_old in two combos.
					{
						if (c->type >= THREE_ONE)
						{

							c->control = 2;//tmporary use
						}
						else
							c->control = 0;//tmporary use

						suit1.lower_control_poker = player->lower_control_poker;
						suit1.oppDown_num = player->oppDown_num;
						suit1.oppUp_num = player->oppUp_num;
						memmove(&h, player->h, sizeof(POKERS));

						memmove(&opps, player->opps, sizeof(POKERS));
						remove_combo_poker(suit1.h, c, NULL);

						search_combos_in_suits_for_MIN_single_farmer(game, suit1.h, suit1.combos, &suit1);


						if (get_2nd_MIN_singles(suit1.summary) >= player->opps->end)
						{   //update player in hands;
							DBG(PRINTF(LEVEL, "\n!farmer biguan, new_combo_num %d, new_ctrl %d pre_ctrl %d, pre_combo_num %d\n"
								, suit1.summary->combo_total_num, CONTROL_NUM(&suit1), CONTROL_NUM(player), player->summary->combo_total_num
							));
							memmove(player->combos, suit1.combos, 20 * sizeof(COMBO_OF_POKERS)); //todo refine;
							memset(player->summary, 0, sizeof(COMBOS_SUMMARY_DDZ_AI));
							sort_all_combos(game, player->combos, 20, player->summary, player->opps, player->oppDown_num
								, player->oppUp_num, player->lower_control_poker, player); //to remove this..
							first_time = 0;
							memmove(cur, c, sizeof(COMBO_OF_POKERS));
							break;
						}
						*p = *c;
					}
					//}
					cur->control = 0;
					return first_time;
				}
				else if (c->type != BOMB_old || 1)// !(isup && !isgood && isfarmer))  //up bad farmer do not play bomb here.
				{
					remove_combo_in_suit(player->summary, c);
					memmove(cur, c, sizeof(COMBO_OF_POKERS));
					c->type = NOTHING_old;

					DBG(PRINTF(LEVEL, "!NongMINg Biguan: shunpai\n"));
					return 0;
				}
			}
			else if (pre->type == SINGLE)//goto...
			{
                int i = 0;
                for (; i < player->summary->singles_num; ++i)
                {
                    if (player->summary->singles[i]->low > pre->low)
                    {
                        break;
                    }
                }
                
                if (i < player->summary->singles_num)
                {
                    *cur = *player->summary->singles[i];
                    if (game->known_others_poker && _CheckWinWrapper(game, cur))
                    {
                        return 0;
                    }

                    if (PRE_PLAYER(game) == UPFARMER_old && game->known_others_poker)
                    {
                        search_combos_in_suits_for_MIN_single_farmer(game, game->players[DOWN_PLAYER]->h, 
                            game->players[DOWN_PLAYER]->combos, game->players[DOWN_PLAYER]);
                        if (get_2nd_MIN_singles(game->players[DOWN_PLAYER]->summary) < game->players[UP_PLAYER]->h->end)
                        {
                            return 0;
                        }
                    }
                }                

				return 1;
			}
		}
	}
	return 1;
}

void farmer_play_first_lord_has_1_poker_only(GAME_DDZ_AI*game, COMBO_OF_POKERS* cur, int is_good_farmer, PLAYER*player, int isup)
{
	FUNC_NAME
		search_combos_in_suits_for_MIN_single_farmer(game, player->h, player->combos, player);

	int second_single = get_2nd_MIN_singles(player->summary);
	int lord_single = player->opps->end;

	if (game->known_others_poker)
	{
		PLAYER* lord = game->player_type == UPFARMER_old ?
			game->players[DOWN_PLAYER] :
			game->players[UP_PLAYER];
		lord_single = lord->h->end;
	}

	if (game->known_others_poker)
	{
		if (!isup && game->players[DOWN_PLAYER]->h->total == 1) //down player is farmer ,and has only one poker..
		{
			if (game->players[DOWN_PLAYER]->h->end > player->h->begin)
			{
				//todo: check bombs
				cur->len = 1;
				cur->type = SINGLE;
				cur->low = player->h->begin;
				player->need_research = 1;
				return;
			}
		}

		if (player->summary->singles_num <= 1
			|| second_single >= lord_single) //self will win!
		{
			COMBO_OF_POKERS * res;
			if ((res = find_MAX_len_in_combos(player->combos, 20)) != NULL)
			{
				memmove(cur, res, sizeof(COMBO_OF_POKERS));
				res->type = NOTHING_old;
				remove_combo_in_suit(player->summary, cur);
				return;
			}
			else
			{
				PRINTF_ALWAYS("line %d stupid error happens...\n", __LINE__);
			}
		}

		//search the other farmer's poker
		PLAYER* other_farmer = game->player_type == DOWNFARMER_old ?
			game->players[DOWN_PLAYER] :
			game->players[UP_PLAYER];
		search_combos_in_suits_for_MIN_single_farmer(game, other_farmer->h, other_farmer->combos, other_farmer);

		//the other farmer can win
		int the2ndsingle = get_2nd_MIN_singles(other_farmer->summary);
		if (the2ndsingle > lord_single)
		{
			//let the other farmer play..
			//check pairs,three_one,series..
			if (other_farmer->summary->pairs_num >= 1)
			{
				if (find_a_smaller_combo_in_hands(player->h, cur,
					other_farmer->summary->pairs[other_farmer->summary->pairs_num - 1]))
				{
					player->need_research = 1;
					return;
				}
			}

			if (other_farmer->summary->series_num >= 1)
			{
				if (find_a_smaller_combo_in_hands(player->h, cur,
					other_farmer->summary->series[other_farmer->summary->series_num - 1]))
				{
					player->need_research = 1;
					return;
				}
			}

			if (other_farmer->summary->three_one_num >= 1)
			{
				if (find_a_smaller_combo_in_hands(player->h, cur,
					other_farmer->summary->three_one[other_farmer->summary->three_one_num - 1]))
				{
					POKERS t;
					cur->type = THREE;
					memmove(&t, player->h, sizeof(POKERS));
					remove_combo_poker(&t, cur, NULL);
					search_combos_in_suit(game, &t, player->opps, player);
					cur->type = 31;
					search_1_2_for_a_three(&t, player, cur);
					player->need_research = 1;
					return;
				}
			}

		}

	}

	if (player->summary->singles_num <= 1
		|| second_single >= lord_single) //win!
	{
		COMBO_OF_POKERS * res;
		if ((res = find_MAX_len_in_combos(player->combos, 20)) != NULL)
		{
			memmove(cur, res, sizeof(COMBO_OF_POKERS));
			res->type = NOTHING_old;
			remove_combo_in_suit(player->summary, cur);
			return;
		}
		else
		{
			PRINTF_ALWAYS("line %d stupid error happens...\n", __LINE__);
		}
	}
	else   if (!isup && game->players[CUR_PLAYER]->oppDown_num == 1) //down player is farmer ,and has only one poker..
	{
		for (int i = player->h->begin; i<player->h->end; i++)
		{
			if (player->h->hands[i] >= 1)
			{
				//player->h->hands[i]-=2;
				cur->len = 1;
				cur->type = SINGLE;
				cur->low = i;
				player->need_research = 1;
				return;
				//goto combo_selected;
			}
		}
	}
	else
	{
		if (!isup && player->summary->singles_num == 2)
		{
			if (player->summary->pairs_num >= 2)//todo:fixed me
			{
				*cur = *player->summary->pairs[0];
				return;
			}
			if (player->h->total == 2)
			{
				// 如果只有2套单牌，则打出最大的一套单牌，否则            
				*cur = *player->summary->singles[1];
				return;
			}
		}

		//down farmer 出牌逻辑
		if (!isup)
		{
			COMBO_OF_POKERS* t = find_smallest_in_combos(player->combos, 20, player, 1);
			if (t != NULL)
			{
				*cur = *t;
				return;
			}
		}

		for (int i = player->h->begin; i <= player->h->end; i++)
		{
			if (player->h->hands[i] >= 2)
			{
				//player->h->hands[i]-=2;
				cur->len = 1;
				cur->type = PAIRS;
				cur->low = i;
				//check if others has a big than this
				COMBO_OF_POKERS c;
                if (find_a_bigger_combo_in_hands(player->opps, &c, cur))
                {
                    return;
                }
                else
                {
                    break;
                }
                //goto combo_selected;
			}
		}

		COMBO_OF_POKERS* t = find_smallest_in_combos(player->combos, 20, player, 1);
		if (t != NULL)
		{
			*cur = *t;
			return;
		}

        // 走到这的逻辑保证农民手牌最少单牌拆分至少有2套单牌
        if (isup)
        {
            for (int i = player->opps->end - 1; i >= player->summary->singles[1]->low; --i)
            {
                if (player->h->hands[i] >= 1)
                {
                    cur->len = 1;
                    cur->type = SINGLE;
                    cur->low = i;
                    return;
                }
            }
        }
        else
		//f(isup)
		{
			//player->h->hands[i]-=1;
			//cur->len = 1;
			//cur->type = SINGLE;
			//cur->low = isup ? player->h->end : player->h->begin;
            *cur = *player->summary->singles[1];
			player->need_research = 1;
			return;
			//goto combo_selected;
		}
	}
}

//todo: update this
COMBO_OF_POKERS* upfarmer_good_play_first(GAME_DDZ_AI * game, COMBO_OF_POKERS* cur)//select cur from play
{
	FUNC_NAME
		PLAYER* player = game->players[CUR_PLAYER];
	PLAYER * lord = game->players[DOWN_PLAYER];
	PLAYER * dnFm = game->players[UP_PLAYER];

	COMBOS_SUMMARY_DDZ_AI* s = player->summary;

	if (!is_player_ready_win(game, player) && game->known_others_poker) //不合手
	{
		COMBO_OF_POKERS * list[MAX_COMBO_NUM];
		int num = 0;

		//先出 地主组合里没有的牌....
		for (int i = 0; i<s->not_biggest_num; i++)
		{
			if (!get_num_of_bigger(lord->combos, MAX_COMBO_NUM, s->not_biggest[i]))
			{
				list[num++] = s->not_biggest[i];
			}
		}

		if (num>0)
		{
			//先出自己有回手的
			for (int i = 0; i<num; i++)
			{
				if (get_num_of_bigger_in_combos(s->biggest, s->biggest_num, list[i]))
				{
					return list[i];
				}
			}
			return list[0];
		}


		if (s->biggest_num >0)
		{
			//出地主没有的牌, 且不含大牌控制的牌
			for (int i = 0; i<s->biggest_num; i++)
			{

				if (!get_num_of_smaller(lord->combos, MAX_COMBO_NUM, s->biggest[i]))
				{
					if (s->biggest[i]->type == BOMB_old)
						continue;
					if (s->biggest[i]->type == SINGLE_SERIES)
						return s->biggest[i];
					if (has_control_poker(s->biggest[i], player->lower_control_poker) == -1)
						return s->biggest[i];
				}
			}
		}

		//出自己或同伴有回手的

		num = 0;

		//出地主只有用大牌管的
		for (int i = 0; i<s->not_biggest_num; i++)
		{
			if (!get_num_of_bigger_in_combos(lord->summary->not_biggest,
				lord->summary->not_biggest_num
				, s->not_biggest[i]))
			{
				//refine?
				if (has_control_poker(s->not_biggest[i], player->lower_control_poker) == -1)
					return s->not_biggest[i];
			}
		}

        if (game->use_best_for_undefined_case && lord->h->total <= 2)
        {
            int pass = get_the_best_hand(game, &game->tmp_for_best, 1);
            if (!pass)
            {
                return &game->tmp_for_best;
            }
        }

		//出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
	}

	// rocket or biggest bomb and other..
	// for chuntian
	//bomb is biggest


	// win for sure....
	COMBO_OF_POKERS * t = farm_select_combo_in_suit(game);
	return  t;


	//寻找牌组中，套数最多的一类牌


}


void upfarmer_bad_play_first(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	FUNC_NAME
		PLAYER* curPlayer = game->players[CUR_PLAYER];
	PLAYER *downfarmer = game->players[UP_PLAYER];
	PLAYER * lord = game->players[DOWN_PLAYER];

	if (!game->known_others_poker &&
		game->players[(CUR_PLAYER)]->oppUp_num == 1)
	{
		cur->len = 1;
		cur->type = SINGLE;
		cur->low = curPlayer->h->begin;
		return;
	}

	if (!game->known_others_poker &&
		game->players[CUR_PLAYER]->oppUp_num == 2)
	{
		for (int i = curPlayer->h->begin; i <= curPlayer->h->end; i++)
		{
			if (curPlayer->h->hands[i] >= 2)
			{
				//curPlayer->h->hands[i]-=2;
				cur->len = 1;
				cur->type = PAIRS;
				cur->low = i;
				if (is_combo_biggest(game, curPlayer->opps, cur, curPlayer->oppUp_num, curPlayer->oppDown_num, curPlayer->lower_control_poker))
					goto __play_single_upfarmer;
				return;
			}
		}
		goto __play_single_upfarmer;
	}

	if (game->known_others_poker)
	{
        if (game->use_best_for_undefined_case && (lord->h->total <= 2 || game->hun == -1))
        {
            int pass = get_the_best_hand(game, cur, 1);
            //double check
            if (cur->type == PAIRS && lord->summary->real_total_num == 1
                && lord->h->total == 2 && lord->h->begin > cur->low)
                goto __play_single_upfarmer;

            if (!pass)
            {
                return;
            }
        }
	}


	if (curPlayer->oppDown_num != 2)
	{
		/*优先出对牌，从88开始往小找*/
		for (int i = char_to_poker('8'); i>0; i--)
		{
			if (curPlayer->h->hands[i] == 2)
			{
				//curPlayer->h->hands[i]-=2;
				cur->len = 1;
				cur->type = PAIRS;
				cur->low = i;
				return;
				//goto combo_selected;
			}
		}
	}
__play_single_upfarmer:
	/*没有对牌则出单牌，除去控制，从大往小出*/

	for (int i = curPlayer->lower_control_poker - 1; i >= curPlayer->h->begin; i--)
	{
		if (curPlayer->h->hands[i] == 1)
		{
			//curPlayer->h->hands[i]-=1;
			cur->len = 1;
			cur->type = SINGLE;
			cur->low = i;
			return;
			//goto combo_selected;
		}
	}

	COMBO_OF_POKERS * t = farm_select_combo_in_suit(game);
	*cur = *t;
	return;
}

int upfarmer_play_when_down_farmer_has_only_1_poker(GAME_DDZ_AI *game, COMBO_OF_POKERS * cur)
{
	FUNC_NAME
		PLAYER* curPlayer = game->players[CUR_PLAYER];
	PLAYER *dnfm = game->players[UP_PLAYER];
	PLAYER * lord = game->players[DOWN_PLAYER];

	//todo..
	// 不拆炸弹，先诈炸弹。。。
	if (game->known_others_poker && dnfm->h->total == 1 && lord->h->total>1 && (dnfm->h->begin > lord->h->end))
	{
		for (int i = 0; i<curPlayer->summary->singles_num; i++)
		{
			if (curPlayer->summary->singles[i]->low < dnfm->h->begin)
			{
				*cur = *curPlayer->summary->singles[i];
				return 1;
			}
		}

		if (dnfm->h->begin >    curPlayer->h->begin)
		{
			cur->len = 1;
			cur->type = SINGLE;
			cur->low = curPlayer->h->begin;
			return 1;
		}
	}

	if (game->known_others_poker && dnfm->summary->biggest_num > 0)
	{
		if (find_a_smaller_combo_in_hands(curPlayer->h, cur, dnfm->summary->biggest[0]))
		{
			//todo: double check if lord has the same one...
			return 1;
		}
	}

	if (game->known_others_poker && dnfm->summary->not_biggest_num >0)
	{
		if (!get_num_of_bigger(lord->combos, MAX_COMBO_NUM, dnfm->summary->not_biggest[0]))
		{
			//if(get_num_of_smaller(curPlayer->combos, MAX_COMBO_NUM,dnfm->summary->not_biggest[0]))
			for (int i = 0; i< MAX_COMBO_NUM; i++)
			{
				if (is_smaller(&curPlayer->combos[i], dnfm->summary->not_biggest[0]))
				{
					*cur = curPlayer->combos[i];
					return 1;
				}
			}
		}
	}
	return 0;
}

//select a combo from hands, save to cur
void upfarmer_play_first(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	FUNC_NAME
		PLAYER* curPlayer = game->players[CUR_PLAYER];
	PLAYER *dnfm = game->players[UP_PLAYER];
	PLAYER * lord = game->players[DOWN_PLAYER];

	int is_good_farmer = curPlayer->good_framer;

	COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);

	if (c != NULL)
	{
		int good = 1;
		if (curPlayer->oppDown_num == 1 && c->type == SINGLE
			&& c->low < curPlayer->opps->end)
			good = 0;
		if (curPlayer->oppDown_num == 2 && c->type == PAIRS
			&& !is_combo_biggest_sure(game, c))
		{
			good = 0;
		}
		if (good || curPlayer->summary->real_total_num <= 1) { // 2017-9-19 yz fixed
			*cur = *c;
			return;
		}
	}
	if (curPlayer->oppDown_num == 1)
	{
		return farmer_play_first_lord_has_1_poker_only(game, cur, is_good_farmer, curPlayer, 1);
	}

	//check for down farmer win..
	if (game->known_others_poker)
	{
		if (dnfm->summary->real_total_num == 1) {
			if (upfarmer_play_when_down_farmer_has_only_1_poker(game, cur))
				return;
		}
	}

	if (game->known_others_poker && lord->h->total == 2)
	{
		if (game->use_best_for_undefined_case)
		{
			int pass = get_the_best_hand(game, cur, 1);
			if (cur->type == PAIRS && lord->summary->real_total_num == 1
				&& lord->h->begin > cur->low)
			{
			}
            else if (!pass)
				return;
		}
	}

	if (curPlayer->oppDown_num == 2)
	{
		if (curPlayer->h->end > curPlayer->opps->end) //use known_other_poker
		{
			if (curPlayer->summary->singles_num>0)
			{
				int found = 0;
				for (int i = 0; i<curPlayer->summary->singles_num; i++)
					if (curPlayer->summary->singles[i]->low<
						curPlayer->opps->end)
					{
						*cur = *curPlayer->summary->singles[i];
						found = 1;
					}
				if (found) return;
			}
			if (curPlayer->summary->pairs_num>0 &&
				(!is_combo_biggest_sure(game, curPlayer->summary->pairs[0])))
			{ //拆对!
				cur->low = curPlayer->summary->pairs[0]->low;
				cur->type = SINGLE;
				return;
			}
		}
		//check opps has pairs?
		int haspairs = 0;
		for (int i = 0; i <= P2; i++)
		{
			if (curPlayer->opps->hands[i] >= 2)
			{
				haspairs = 1;
				break;
			}
		}
		if (haspairs == 1)
		{
			if (curPlayer->summary->pairs_num>0 &&
				(!is_combo_biggest_sure(game, curPlayer->summary->pairs[0])))
			{
				if (curPlayer->summary->singles_num>0)
				{
					int found = 0;
					for (int i = 0; i<curPlayer->summary->singles_num; i++)
						if (curPlayer->summary->singles[i]->low<
							curPlayer->opps->end)
						{
							*cur = *curPlayer->summary->singles[i];
							found = 1;
						}
					if (found) return;
				}
				else if (curPlayer->summary->pairs_num>1)
				{
					//check the second pairs is the biggest?
					if (!is_combo_biggest_sure(game, curPlayer->summary->pairs[1]))
					{ //2e??!
						cur->low = curPlayer->summary->pairs[0]->low;
						cur->type = SINGLE;
						return;
					}
				}
			}
		}
	}



	if (check_only_2_same_combo(game, cur))
	{
		return;
	}

	if (is_good_farmer) {
		COMBO_OF_POKERS * res = (upfarmer_good_play_first(game, cur));
		if (res == NULL)
		{
			PRINTF(LEVEL, "ERR stupid error @ line %d\n", __LINE__);
		}
		else
		{
			*cur = *res;
		}
	}
	else
		upfarmer_bad_play_first(game, cur);

	COMBO_OF_POKERS tmp;
	//TODO(在手牌中找还是在套牌中找)
	if (game->use_best_for_undefined_case
		&& find_a_bigger_combo_in_hands(lord->h, &tmp, cur) && tmp.type != BOMB_old && tmp.type != ROCKET //非炸弹
		&& (tmp.low < LIT_JOKER || lord->h->hands[LIT_JOKER] <= 0 || lord->h->hands[BIG_JOKER] <= 0)) //拆王炸
	{
		get_the_best_hand(game, cur, 1);
	}

	return;
}


void downfarmer_bad_play_first(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	FUNC_NAME
		PLAYER* curPlayer = game->players[CUR_PLAYER];
	PLAYER *lord = game->players[UP_PLAYER];
	PLAYER * upfm = game->players[DOWN_PLAYER];
	if (!game->known_others_poker)
	{
		if (curPlayer->oppUp_num == 2)
		{
			for (int i = curPlayer->h->begin; i <= curPlayer->h->end; i++)
			{
				if (curPlayer->h->hands[i] >= 2)
				{
					//curPlayer->h->hands[i]-=2;
					cur->len = 1;
					cur->type = PAIRS;
					cur->low = i;
					if (is_combo_biggest(game, curPlayer->opps, cur, curPlayer->oppUp_num, curPlayer->oppDown_num, curPlayer->lower_control_poker))
						return;
				}
			}
		}
		else
		{
			COMBO_OF_POKERS* t = find_smallest_in_combos(curPlayer->combos, 20, curPlayer, 0);
			*cur = *t;
		}
	}

	//todo: when up farmer is ready to win...

	//  PLAYER *lord= game->players[UP_PLAYER];
	//    PLAYER * upfm= game->players[DOWN_PLAYER];

	// 这段逻辑有问题，出了炸弹后送不走队友
	if (game->known_others_poker && upfm->summary->combo_total_num <= 2)
	{
		//get the best hand
		if (game->use_best_for_undefined_case) {
			int pass = get_the_best_hand(game, cur, 1);
			COMBO_OF_POKERS tmp;
			if (cur->type != NOTHING_old && (cur->type == BOMB_old || cur->type == ROCKET
				|| find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &tmp, cur)))
				return;
            if (!pass)
            {
                return;
            }
		}
	}

	if (game->known_others_poker)
		return   farmer_select_one_combo_for_other_farmer(game, curPlayer, cur, 0);

	COMBO_OF_POKERS* t = farm_select_combo_in_suit(game);
	*cur = *t;
}

void  downfarmer_play_first_when_upfarmer_has_only_1_pokers(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	FUNC_NAME
		PLAYER*player = game->players[CUR_PLAYER];
	PLAYER* upfm = game->players[DOWN_PLAYER];
	PLAYER* lord = game->players[UP_PLAYER];

	if (!game->known_others_poker)
	{
		// todo: add logic for bomb...
		for (int i = player->h->begin; i <= player->h->end; i++)
		{
			if (player->h->hands[i]>0)
			{
				cur->type = SINGLE;
				cur->low = i;
				return;
			}
		}
	}
	else
	{
		//search bomb in player
		// play the smallest Bomb which
		if (getBomb(player->h, cur)) {
			do
			{
				if (!has_combo_bigger_than_c_in_hands(lord->h, cur))
				{
					//double check if there's still another poker less than down's
					for (int i = player->h->begin; i <= player->h->end; i++)
					{
						if (i != cur->low && player->h->hands[i]>0 && i<upfm->h->begin)
							return;
					}
				}
			} while (getBigBomb(player->h, cur, cur));

		}
		for (int i = player->h->begin; i <= player->h->end; i++)
		{
			if (player->h->hands[i]>0)
			{
				cur->type = SINGLE;
				cur->low = i;
				return;
			}
		}
	}
}

void  downfarmer_play_first_when_upfarmer_has_only_2_pokers(GAME_DDZ_AI* game, COMBO_OF_POKERS* cur)
{
	FUNC_NAME
		PLAYER*player = game->players[CUR_PLAYER];
	PLAYER* upfm = game->players[DOWN_PLAYER];
	PLAYER* lord = game->players[UP_PLAYER];

	//search bomb in player
	if (getBomb(player->h, cur)) {
		do {

			{
				if (!has_combo_bigger_than_c_in_hands(lord->h, cur)) // bomb is the biggest
				{
					if (upfm->h->begin == upfm->h->end) //it's a pair
														//double check if there's still another poker less than down's
						for (int i = player->h->begin; i <= player->h->end; i++)
						{
							if (i != cur->low && player->h->hands[i]>1 && i<upfm->h->begin)
								return;
						}
				}
				else
				{
					int bigger = 0, first_MIN = 0;
					for (int i = player->h->begin; i <= player->h->end; i++)
					{
						if (i != cur->low && player->h->hands[i]>0) {
							if (i<upfm->h->begin && bigger == 0) {
								bigger += player->h->hands[i];
								first_MIN = i;
							}
							if (i<upfm->h->end && bigger >0) {
								bigger += player->h->hands[i];
							}
							if (bigger >= 2)
							{
								cur->type = SINGLE;
								cur->low = first_MIN;
								return;
							}
						}
					}
				}
			}
		} while (getBigBomb(player->h, cur, cur));

	}
	if (upfm->h->begin == upfm->h->end) //it's a pair
	{
		//double check if there's still another poker less than down's
		for (int i = player->h->begin; i <= player->h->end; i++)
		{
			if (player->h->hands[i]>1 && i<upfm->h->begin) {
				cur->type = PAIRS;
				cur->low = i;
				return;
			}
		}
	}

	if (player->good_framer)
	{
		downfarmer_good_play_first(game, cur);
	}
	else //bad farmer
	{
		downfarmer_bad_play_first(game, cur);
	}

}

void downfarmer_play_first(GAME_DDZ_AI*game, COMBO_OF_POKERS* cur)
{
	PLAYER*player = game->players[CUR_PLAYER];
	PLAYER* upfm = game->players[DOWN_PLAYER];
	PLAYER* lord = game->players[UP_PLAYER];
	int is_good_farmer = player->good_framer;

	/*如果下家报单，则优先出最小的单牌*/
	if (player->oppDown_num == 1
		&& (game->known_others_poker == 0
			|| player->h->begin <upfm->h->begin))
		return downfarmer_play_first_when_upfarmer_has_only_1_pokers(game, cur);

	// up farmer only have two cards
	if (player->oppDown_num == 2
		&& (game->known_others_poker == 1))
		return  downfarmer_play_first_when_upfarmer_has_only_2_pokers(game, cur);

	COMBO_OF_POKERS * c = get_combo_in_a_win_suit(game);
	if (c != NULL)
	{
		int good = 1;
		if (player->oppUp_num == 1 && c->type == SINGLE
			&& c->low < player->opps->end)
			good = 0;
		if (player->oppUp_num == 2 && c->type == PAIRS
			&& !is_combo_biggest_sure(game, c))
		{
			good = 0;
		}
		if (good || player->summary->real_total_num <= 1) { // 2017-9-19 yz fixed
			*cur = *c;
			return;
		}
	}

	if (player->oppUp_num == 1)
	{
		farmer_play_first_lord_has_1_poker_only(game, cur, is_good_farmer, player, 0);
	}
	else
	{
		if (game->known_others_poker && lord->h->total == 2)
		{
			if (game->use_best_for_undefined_case)
			{
				int pass = get_the_best_hand(game, cur, 1);
				if (cur->type == PAIRS && lord->summary->real_total_num == 1
					&& lord->h->begin > cur->low)
				{
				}
                else if (!pass)
					return;
			}
		}
		/**/
		if (player->oppUp_num == 2 && player->oppDown_num != 2)
		{
			//check opps has pairs?
			int haspairs = 0;
			for (int i = 0; i <= P2; i++)
			{
				if (player->opps->hands[i] >= 2)
				{
					haspairs = 1;
					break;
				}
			}
			if (haspairs == 1)
			{
				if (player->summary->pairs_num>0 &&
					(!is_combo_biggest_sure(game, player->summary->pairs[0])))
				{
					if (player->summary->singles_num>0)
					{
						int found = 0;
						for (int i = 0; i<player->summary->singles_num; i++)
							if (player->summary->singles[i]->low<
								player->opps->end)
							{
								*cur = *player->summary->singles[i];
								found = 1;
							}
						if (found &&player->h->end>player->opps->end) return;
					}
					else if (player->summary->pairs_num>1
						&& player->summary->pairs_num == player->summary->real_total_num)
					{
						//check the second pairs is the biggest?
						if (!is_combo_biggest_sure(game, player->summary->pairs[1]))
						{ //拆对!
							cur->low = player->summary->pairs[0]->low;
							cur->type = SINGLE;
							return;
						}
						else
						{
							*cur = *player->summary->pairs[1];
							return;
						}
					}
				}
			}
		}

		if (check_only_2_same_combo(game, cur))
		{
			return;
		}

		if (is_good_farmer)
		{
			downfarmer_good_play_first(game, cur);
		}
		else //bad farmer
		{
			downfarmer_bad_play_first(game, cur);
		}

	}
	//double check again
	if (!game->known_others_poker
		&& cur->type == PAIRS && player->oppUp_num == 2 && player->oppDown_num != 2 && player->h->total != 2)
	{
		int haspairs = 0;
		for (int i = cur->low + 1; i <= P2; i++)
		{
			if (player->opps->hands[i] >= 2)
			{
				haspairs++;
			}
		}
		if (haspairs>1 || cur->low<player->opps->begin)
		{
			cur->type = SINGLE;
		}
	}
}

//find and remove a joker in combos, 31 3311... 411
int find_joker_in_combos(PLAYER* player, int joker)
{
	if (player->summary->singles_num>0 && player->summary->singles[player->summary->singles_num - 1]->type == joker)
	{
		player->summary->singles_num--;
		player->summary->singles[player->summary->singles_num]->type = NOTHING_old;
	}
	else if (player->summary->three_one_num>1 && player->summary->three_one[player->summary->three_one_num - 1]->three_desc[0] == joker)
	{
		player->summary->three_one_num--;
		player->summary->three[player->summary->three_num++] = player->summary->three_one[player->summary->three_one_num];
		player->summary->three_one[player->summary->three_one_num]->type = THREE;
	}
	else {
		return 0;
	}

	return 1;
}

int must_play_for_poker2(GAME_DDZ_AI * game, COMBO_OF_POKERS * cur, PLAYER* player, COMBO_OF_POKERS *pre, int check_opps_lit_joker)
{
	int pass = 1;
	if (player->h->hands[LIT_JOKER] == 1)
	{
		cur->type = SINGLE;
		cur->low = LIT_JOKER;
		cur->len = 1;
		pass = 0;
		if (player->summary->bomb_num>0 && player->summary->bomb[player->summary->bomb_num - 1]->low == LIT_JOKER)
		{
			if (player->good_framer || player->id == DOWNFARMER_old) //不拆大小王
				return 1;
			player->summary->bomb[player->summary->bomb_num - 1]->type = SINGLE;
			player->summary->bomb[player->summary->bomb_num - 1]->low = BIG_JOKER;
			player->summary->bomb[player->summary->bomb_num - 1]->len = 1;
			player->summary->singles_num++;
			player->summary->singles[player->summary->singles_num - 1] = player->summary->bomb[player->summary->bomb_num - 1];
			player->summary->bomb_num--;
		}
		else {
			//find joker in 31 3311.. 411..
			if (!find_joker_in_combos(player, LIT_JOKER))
			{
				rearrange_suit(game, player, cur);
			}
		}
		//pass = 0;
	}
	else if ((player->h->hands[BIG_JOKER] == 1)
		&& (
			player->opps->hands[LIT_JOKER] == 0
			|| !check_opps_lit_joker
			)// Lit joker not seen, do not play for big_joker
		)
	{

		cur->type = SINGLE;
		cur->low = BIG_JOKER;
		cur->len = 1;
		pass = 0;
		if (!find_joker_in_combos(player, BIG_JOKER))
		{
			rearrange_suit(game, player, cur);
		}
	}
	else
		pass = 1;
	return pass;
}

void update_players(GAME_DDZ_AI * game, COMBO_OF_POKERS * cur)
{
	if (cur != NULL)
		remove_combo_poker(&game->all, cur, NULL);
	int ctrl_num = CONTROL_POKER_NUM;
	if (game->all.total <= 20)
		ctrl_num = 4;
	else if (game->all.total <= 10)
		ctrl_num = 2;

	// player->lower_control_poker=
	game->players[UP_PLAYER]->lower_control_poker =
		game->players[DOWN_PLAYER]->lower_control_poker =
		game->players[CUR_PLAYER]->lower_control_poker
		= get_lowest_controls(&game->all, ctrl_num);

	for (int i = 2; i >= 0; i--) { //TODO
		game->players[i]->summary->ctrl.single =
			calc_controls(game->players[i]->h, game->players[i]->opps, ctrl_num);
		update_summary(game, game->players[i]->summary, game->players[i]->h,
			game->players[i]->opps, game->players[i]->combos, 20,
			game->players[i]->oppDown_num, game->players[i]->oppUp_num,
			game->players[i]->lower_control_poker, game->players[i]);
	}
}

int is_upfarmer_must_play_bigger(GAME_DDZ_AI* game, int is_good_farmer, COMBO_OF_POKERS* pre)
{
	if (game->player_type == UPFARMER_old)
	{
		if (is_good_farmer)
		{
			if (PRE_PLAYER(game) == LORD_old //pre player is lord.
				&&
				((pre->type >= 3311) //飞机
					|| (pre->type == PAIRS_SERIES)
					|| (pre->type == SINGLE_SERIES && pre->len>8)
					//||( pre->type == SINGLE )
					|| (pre->type == SINGLE &&
						pre->low<game->players[CUR_PLAYER]->lower_control_poker)

					)
				)
			{   //必管..
				return true;
			}
		}
		else  //bad
		{
			if (PRE_PLAYER(game) == LORD_old
				&&
				((pre->type != SINGLE  && pre->type != PAIRS)
					|| (pre->type == SINGLE &&
						pre->low<game->players[CUR_PLAYER]->lower_control_poker)
					//|| (pre->type == PAIRS && pre->low<game->lowest_bigPoker-2)
					)
				)
			{
				return true;
			}

		}
	}
	return false;
}

int upfarmer_play_normal(GAME_DDZ_AI *game, COMBO_OF_POKERS* cur, COMBO_OF_POKERS* pre)
{
	return farmer_play_normal(game, cur, pre);
}

int upfarmer_make_a_bigger_for_single_and_pair(GAME_DDZ_AI *game, COMBO_OF_POKERS* pre, COMBO_OF_POKERS* cur, int is_good)
{
	FUNC_NAME
		int pass = 1;
	PLAYER *player = game->players[CUR_PLAYER];

	int geying = is_game_first_half(game, CUR_PLAYER) ? char_to_poker('Q') : char_to_poker('Q');

	if (game->known_others_poker && pre->type == SINGLE)
	{
		//get the singles of lord..
		int single = geying;
		//play a bigger than more of the singles
		search_combos_in_suit(game,
			game->players[DOWN_PLAYER]->h,
			game->players[DOWN_PLAYER]->opps,
			game->players[DOWN_PLAYER]);
		COMBOS_SUMMARY_DDZ_AI * s = game->players[DOWN_PLAYER]->summary;

		for (int i = s->singles_num - 1; i >= 0; i--)
		{
			if (s->singles[i]->low <= char_to_poker('K'))
			{
				single = s->singles[i]->low;
				break;
			}
		}
		if (s->singles_num >= 3)
		{
			if (single > s->singles[2]->low)
				single = s->singles[2]->low;
		}
		geying = single;
	}

	if ((pre->type == SINGLE))
	{

		if (!is_good && pre->low < geying)
		{

			if (game->players[game->pre_playernum]->id != LORD_old
				&& (pre->low >= player->lower_control_poker || pre->low >= geying))
				return 1;

			pass = 1;
			for (int i = char_to_poker('A'); i >= geying; i--)
			{
				if (player->h->hands[i] == 1)
				{
					cur->type = SINGLE;
					cur->low = i;
					cur->len = 1;
					DBG(PRINTF(LEVEL, "!Bad Up Farmer Gepai\n"));
					pass = 0;
					break;
				}
			}
			if (pass == 1)
			{
				for (int i = char_to_poker('A'); i >= geying; i--) //寻找>X的格应区的对子，拆最大的一个，否则
				{
					if (player->h->hands[i] == 2)
					{
						cur->type = SINGLE;
						cur->low = i;
						cur->len = 1;
						player->h->hands[i] = 1;
						player->need_research = 1;
						search_combos_in_suit(game, player->h, player->opps, player);
						player->h->hands[i]++;
						DBG(PRINTF(LEVEL, "!Bad Up Farmer Geying\n"));
						pass = 0;
						break;
					}
				}
			}
			if (pass == 1)// && (game->pre_playernum == game->lord-1 )) //only for preplayer is lord..
			{
				//for (int i=max_ddz(game->lowest_bigPoker,pre->low+1); i<=BIG_JOKER; i++ )
				if (PRE_PLAYER(game) == LORD_old
					|| pre->low <= P10)
				{
					if (player->h->hands[P2] >= 1)
					{
						cur->type = SINGLE;
						cur->low = P2;
						cur->len = 1;
						DBG(PRINTF(LEVEL, "!Bad Up Farmer select a control poker\n"));
						pass = 0;
						//   break;
					}
				}
			}

			if (pass == 1)
			{   //rarely here..
				for (int i = geying; i >= pre->low + 1; i--)
				{
					if (player->h->hands[i] >= 1 && player->h->hands[i] != 4)
					{
						cur->type = SINGLE;
						cur->low = i;
						player->h->hands[i]--;
						cur->len = 1;
						search_combos_in_suit(game, player->h, player->opps, player);
						player->need_research = 1;
						player->h->hands[i]++;
						DBG(PRINTF(LEVEL, "!Bad Up Farmer select other single poker\n"));
						pass = 0;
						break;
					}
				}
			}
			return pass;
		}
		else if (!is_good && PRE_PLAYER(game) == LORD_old && pre->low >= geying && pre->low <= Pa)
		{
			//寻找最小的能管住X的牌。如果不存在或为大小王，则过牌，否则打出
			pass = 1;
			for (int i = pre->low + 1; i <= P2; i++)
			{
				if (player->h->hands[i] >= 1)
				{
					cur->type = SINGLE;
					cur->low = i;
					cur->len = 1;
					player->h->hands[i]--;
					search_combos_in_suit(game, player->h, player->opps, player);
					player->need_research = 1;
					player->h->hands[i]++;
					DBG(PRINTF(LEVEL, "!Bad Up Farmer select other single poker\n"));
					pass = 0;
					break;
				}
			}
		}
		else
		{
			pass = 1;
			if (pre->low < geying)
			{
				//find in pairs..
				//()
				COMBOS_SUMMARY_DDZ_AI *s = player->summary;
				if (s->singles_num>0)
				{
					for (int k = 0; k<s->singles_num; k++)
					{
						if (s->singles[k]->low > pre->low  && s->singles[k]->low >= geying
							&&s->singles[k]->low <=
							player->lower_control_poker) {
							memmove(cur, s->singles[k], sizeof(COMBO_OF_POKERS));
							return 0;
						}
					}

                    // 从大到小，出可以各的牌
                    for (int k = s->singles_num - 1; k >= 0; --k)
                    {
                        if (s->singles[k]->low > pre->low && s->singles[k]->low < geying && s->singles[k]->low <= player->lower_control_poker) 
                        {
                            memmove(cur, s->singles[k], sizeof(COMBO_OF_POKERS));
                            return 0;
                        }
                    }
				}

				// 拆3带1
				if (s->three_one_num > 0)
				{
					COMBO_OF_POKERS three_one_tmp;
					three_one_tmp.type = NOTHING_old;
					for (int k = 0; k < s->three_one_num; k++)
					{
                        // 先找可以各的带牌
						if (s->three_one[k]->three_desc[0] > pre->low && s->three_one[k]->three_desc[0] >= geying
							&& s->three_one[k]->three_desc[0] <= player->lower_control_poker)
						{
							memcpy(&three_one_tmp, &s->three_one[k], sizeof(COMBO_OF_POKERS));

							cur->type = SINGLE;
							cur->low = s->three_one[k]->three_desc[0];
							cur->len = 1;

							pass = 0;

							memcpy(&s->three[s->three_num], &s->three_one[k], sizeof(COMBO_OF_POKERS));
							s->three_num++;
							s->three[s->three_num - 1]->type = THREE;
							s->three[s->three_num - 1]->three_desc[0] = 0;

							s->three_one_num--;
							memmove(&s->three_one[k], &s->three_one[k + 1], sizeof(COMBO_OF_POKERS) * (s->three_one_num - k));

							break;
						}
					}

                    if (pass == 1)
                    {
                        for (int k = 0; k < s->three_one_num; k++)
                        {
                            // 没有可以各的带牌，找能管上的带牌
                            if (s->three_one[k]->three_desc[0] > pre->low 
                                && s->three_one[k]->three_desc[0] <= player->lower_control_poker)
                            {
                                memcpy(&three_one_tmp, &s->three_one[k], sizeof(COMBO_OF_POKERS));

                                cur->type = SINGLE;
                                cur->low = s->three_one[k]->three_desc[0];
                                cur->len = 1;

                                pass = 0;

                                memcpy(&s->three[s->three_num], &s->three_one[k], sizeof(COMBO_OF_POKERS));
                                s->three_num++;
                                s->three[s->three_num - 1]->type = THREE;
                                s->three[s->three_num - 1]->three_desc[0] = 0;

                                s->three_one_num--;
                                memmove(&s->three_one[k], &s->three_one[k + 1], sizeof(COMBO_OF_POKERS) * (s->three_one_num - k));

                                break;
                            }
                        }
                    }

					if (three_one_tmp.type == THREE_ONE)
					{
						bool fixed = false;
						// 修改not_biggest 和 biggest中对应的套
						for (int k = 0; fixed && k < s->not_biggest_num; ++k)
						{
							if (s->not_biggest[k]->type == three_one_tmp.type
								&& s->not_biggest[k]->low == three_one_tmp.low)
							{
								s->not_biggest[k]->type = THREE;
								s->not_biggest[k]->three_desc[0] = 0;
								fixed = true;
							}
						}
						for (int k = 0; fixed && k < s->biggest_num; ++k)
						{
							if (s->biggest[k]->type == three_one_tmp.type
								&& s->biggest[k]->low == three_one_tmp.low)
							{
								s->biggest[k]->type = THREE;
								s->biggest[k]->three_desc[0] = 0;
								fixed = true;
							}
						}
					}

					if (pass == 0)
					{
						return pass;
					}
				}
				//todo: 拆牌

				/*
				if (s->pairs_num>0)
				{
				for (int k=0; k<s->pairs_num; k++)
				{
				if ( s->pairs[k]->low > pre->low  && s->singles[k]->low >=geying ) {

				{
				cur->type=SINGLE;
				cur->low =k;
				cur->len = 1;
				player->h->hands[k]-=1;
				player->need_research = 1;
				search_combos_in_suit(game,player->h,player->opps,player);
				player->h->hands[i]++;
				DBG( PRINTF(LEVEL,"!Good Up Farmer GePai\n"));
				pass = 0;
				break;
				}
				return s->pairs[k];
				}
				}
				}
				//     */
			}
            else
            {
                // 顺牌
                COMBO_OF_POKERS *tmp = find_combo(player->summary, pre);
                if (tmp && NOT_BOMB(tmp) && tmp->low < player->lower_control_poker)
                {
                    memmove(cur, tmp, sizeof(COMBO_OF_POKERS));
                    return 0;
                }
            }
			if (pass == 1 && PRE_PLAYER(game) == LORD_old)
				pass = upfarmer_play_normal(game, cur, pre);
		}
	}
	else
	{
		if (!is_good && PRE_PLAYER(game) == LORD_old)
			pass = must_play_bigger(cur, player, pre, game, 1, 0, 1);
		else if (!is_good) {
			//todo
			if (PRE_PLAYER(game) != LORD_old)
			{
				if (pre->low >= P10)
					pass = 1;
				else
					pass = upfarmer_play_normal(game, cur, pre);
			}
			if (pass == 0)
			{//double check
				if (cur->low >= P10)
					pass = 1;
			}
		}
		else
		{
			if (PRE_PLAYER(game) != LORD_old)
			{
				if (pre->low >= P10)
					pass = 1;
				else
					pass = upfarmer_play_normal(game, cur, pre);
			}
			else
				pass = upfarmer_play_normal(game, cur, pre);
		}
	}
	return pass;
}

int downfarmer_play_when_upfarmer_has_only_1_poker
(GAME_DDZ_AI* game, COMBO_OF_POKERS * cur, COMBO_OF_POKERS * pre)
{
	FUNC_NAME
		PLAYER *player = game->players[CUR_PLAYER];
	PLAYER * lord = game->players[UP_PLAYER];
	for (int i = player->h->begin; i <= player->h->end; i++)
	{
		if (player->h->hands[i] == 4) //bomb got
		{
			cur->type = BOMB_old;
			cur->low = i;
			if (is_combo_biggest_sure(game, cur)
				&& check_combo_a_Big_than_b(cur, pre))
				return 0;
		}
	}

	if (HAS_ROCKET(player->h))
	{
		cur->type = ROCKET;
		cur->low = LIT_JOKER;
		return 0;
	}

	if (PRE_PLAYER(game) != LORD_old)
		return 1;
	else if (pre->type == SINGLE && (pre->low < player->opps->begin ||
		(game->known_others_poker && pre->low < game->players[DOWN_PLAYER]->h->begin)))
	{
		return 1;
	}
	else
	{
		if (find_the_biggest_combo_great_pre(game, pre, cur))
		{
			COMBO_OF_POKERS t;
			if (!find_a_bigger_combo_in_hands(lord->h, &t, cur))
				return 0;
		}

		if (game->use_best_for_undefined_case)
			return  get_the_best_hand(game, cur, 0);
		else //play the biggest combo
		{
			if (find_the_biggest_combo_great_pre(game, pre, cur))
				return 0;
			else
				return 1;
		}
	}
}

int downfarmer_play_when_upfarmer_has_only_1_combo
(GAME_DDZ_AI* game, COMBO_OF_POKERS * cur, COMBO_OF_POKERS * pre)
{
	FUNC_NAME
		PLAYER *player = game->players[CUR_PLAYER];
	PLAYER *upfm = game->players[DOWN_PLAYER];
	PLAYER * lord = game->players[UP_PLAYER];
	COMBO_OF_POKERS * upfm_only_combo;
	//get the only combo of upfarmer
	if (upfm->summary->not_biggest_num>0)
		upfm_only_combo = upfm->summary->not_biggest[0];
	else
		upfm_only_combo = upfm->summary->biggest[0];

	if (!find_a_smaller_combo_in_hands(player->h, cur, upfm_only_combo))
		return 1;

	if (upfm->h->total >2) //break for now
		return 1;

	// for a pair...
	for (int i = player->h->begin; i <= player->h->end; i++)
	{
		if (player->h->hands[i] == 4) //bomb got
		{
			cur->type = BOMB_old;
			cur->low = i;
			if (is_combo_biggest_sure(game, cur)
				&& check_combo_a_Big_than_b(cur, pre))
			{
				POKERS t;
				t = *player->h;
				t.hands[i] = 0;
				COMBO_OF_POKERS c;
				if (find_a_smaller_combo_in_hands(&t, &c, upfm_only_combo))
					return 0;
			}
		}
	}

	if (HAS_ROCKET(player->h))
	{
		cur->type = ROCKET;
		cur->low = LIT_JOKER;
		COMBO_OF_POKERS c;
		if (find_a_smaller_combo_in_hands(player->h, &c, upfm_only_combo))
			return 0;
	}

	if (PRE_PLAYER(game) != LORD_old)
		return 1;
	else if (pre->type == upfm_only_combo->type && pre->low < game->players[DOWN_PLAYER]->h->begin)
	{
		return 1;
	}
	else
	{
		//todo: for sure
		/*
		if (find_the_biggest_combo_great_pre(game,pre,cur))
		{
		COMBO_OF_POKERS t;
		if(!find_a_bigger_combo_in_hands(lord->h, &t,cur))
		return 0;
		}
		*/
		if (game->use_best_for_undefined_case)
			return  get_the_best_hand(game, cur, 0);
		else //play the biggest combo
		{
			if (find_the_biggest_combo_great_pre(game, pre, cur))
				return 0;
			else
				return 1;
		}
	}
}



int downframer_play(GAME_DDZ_AI* game, COMBO_OF_POKERS * cur, COMBO_OF_POKERS *pre)
{
	FUNC_NAME
		PLAYER*player = game->players[CUR_PLAYER];
	PLAYER* upfm = game->players[DOWN_PLAYER];
	PLAYER* lord = game->players[UP_PLAYER];
	int is_good_farmer = player->good_framer;
	int pass;
	COMBO_OF_POKERS* res;

	//if ((res = check_for_win_now(game, pre)) != NULL) //win immediately, play the biggest or bomb
	//{
	//	remove_combo_in_suit(player->summary, res);
	//	memmove(cur, res, sizeof(COMBO_OF_POKERS));
	//	res->type = NOTHING;
	//	return 0;
	//}

    if (PRE_PLAYER(game) == LORD_old || (PRE_PLAYER(game) == UPFARMER_old && game->players[CUR_PLAYER]->oppDown_num <= 6))
    {
        if (player->summary->bomb_num > 0 && is_combo_biggest_sure(game, player->summary->bomb[player->summary->bomb_num - 1])
            && ((player->summary->bomb_num == player->summary->biggest_num && player->summary->not_biggest_num <= 1)
            || (player->summary->bomb_num + 1 == player->summary->biggest_num && player->summary->not_biggest_num == 0)))
        {
            if (opp_hasbomb(game) && game->players[UP_PLAYER]->summary->bomb_num > 0
                && game->players[UP_PLAYER]->summary->bomb[game->players[UP_PLAYER]->summary->bomb_num - 1]->low > player->summary->bomb[player->summary->bomb_num - 1]->low)
            {
                // 除炸弹外只剩一套，或者可以4带2
                if (game->players[UP_PLAYER]->summary->real_total_num <= game->players[UP_PLAYER]->summary->bomb_num + 1
                    || (game->players[UP_PLAYER]->summary->bomb_num > 1
                    && game->players[UP_PLAYER]->summary->real_total_num == game->players[UP_PLAYER]->summary->bomb_num + 2
                    && (game->players[UP_PLAYER]->summary->singles_num == 2 || game->players[UP_PLAYER]->summary->pairs_num == 2)))
                {
                    return 1;
                }
            }

            for (int i = 0; i < player->summary->bomb_num; ++i)
            {
                if (check_combo_a_Big_than_b(player->summary->bomb[i], pre))
                {
                    memcpy(cur, player->summary->bomb[i], sizeof(COMBO_OF_POKERS));
                    return 0;
                }
            }
            return 1;
        }
    }

	// up farmer has only 1 poker
	if (game->players[CUR_PLAYER]->oppDown_num == 1)
	{
		if (game->known_others_poker && (player->h->begin< game->players[DOWN_PLAYER]->h->begin))
			return downfarmer_play_when_upfarmer_has_only_1_poker(game, cur, pre);
		else
		{
			if (PRE_PLAYER(game) == UPFARMER_old)
			{
				if (player->h->begin< game->players[DOWN_PLAYER]->h->begin)
				{
					for (int i = player->h->begin; i <= player->h->end; i++)
					{
						if (player->h->hands[i] == 4) //bomb got
						{
							cur->type = BOMB_old;
							cur->low = i;
							if (is_combo_biggest_sure(game, cur)
								&& check_combo_a_Big_than_b(cur, pre))
								return 0;
						}
					}
					if (HAS_ROCKET(player->h))
					{
						cur->type = ROCKET;
						cur->low = LIT_JOKER;
						return 0;
					}
				}
				else
					return 1;
			}
			else
			{
                bool upfarmerBigSingle = false;
                if (game->known_others_poker)
                {
                    upfarmerBigSingle = player->h->begin < game->players[DOWN_PLAYER]->h->begin;
                }
                else
                {
                    upfarmerBigSingle = player->h->begin < player->opps->begin;
                }

				if (find_the_biggest_combo_great_pre(game, pre, cur, upfarmerBigSingle))
				{
					POKERS t = *player->h;
					remove_combo_poker(&t, cur, NULL);
                    if (player->h->begin < player->opps->end)
                    {
                        if (pre->type != SINGLE)
                            return 0;
                        else if (pre->type == SINGLE)
                        {
                            if (is_combo_biggest_sure(game, cur))
                            {
                                return 0;
                            }
                            else if (pre->low >= player->opps->end)
                            {
                                return 0;
                            }
                            else if (pre->low < player->opps->begin && NOT_BOMB(cur))
                            {
                                pass = 1;
                            }
                            pass = 1;
                        }
                        if (pass == 0)
                        {
                            return 0;
                        }
                        else
                        {
                            if (game->players[DOWN_PLAYER]->lastPassSingle > pre->low)
                            {
                                return 1;
                            }
                        }
                    }
				}
				else
					return 1;
			}
		}
	}

	// up farmer has only 2 poker
	if (game->known_others_poker &&  game->players[CUR_PLAYER]->oppDown_num == 2
		&& game->players[DOWN_PLAYER]->summary->real_total_num == 1
		&& (player->h->begin<game->players[DOWN_PLAYER]->h->begin))
	{
		if (game->known_others_poker) {
			return downfarmer_play_when_upfarmer_has_only_1_combo(game, cur, pre);
		}
	}


	//pre player is lord.)
	if ((game->players[CUR_PLAYER]->oppUp_num == 1))
	{
		pass = farmer_play_when_lord_has_1_poker_only(cur, pre, player, game, 0, is_good_farmer);
		return pass;
	}//pre player is lord.)
	if (player->summary->real_total_num <= 2 ||
		player->summary->combo_total_num <= 1)
	{
		if (game->use_best_for_undefined_case)
		{
			int pass = get_the_best_hand(game, cur, 0);
			if (PRE_PLAYER(game) == LORD_old)
			{
				if ((cur->type == BOMB_old || cur->type == ROCKET) && opp_hasbomb(game))
				{
				}
				else if (cur->type == NOTHING_old) // TODO:之前修改的问题
				{
				}
			}
			else if (PRE_PLAYER(game) != LORD_old && (cur->type == BOMB_old || cur->type == ROCKET))
			{
			}

            if (!pass)
            {
                return pass;
            }
		}
	}//pre player is lord.)

	 //biguan
	if (game->known_others_poker && PRE_PLAYER(game) == LORD_old
		&& ((lord->summary->combo_total_num <= 1 && player->oppUp_num <= 10)
			/*||player->oppDown_num<=2*/
			|| ((lord->summary->combo_total_num <= 2 && player->oppUp_num <= 10)
				&& !find_a_bigger_combo_in_hands(upfm->h, cur, pre))
			)
		)
	{
        if ((pre->type == SINGLE ||pre->type == PAIRS) && is_player_ready_win(game, lord))  // 合手
        {
            COMBO_OF_POKERS tmp;
            tmp.type = pre->type;
            tmp.low = max_ddz(lord->h->end - 1, P3);
            tmp.len = 1;

            COMBO_OF_POKERS combo;
            COMBO_OF_POKERS *bigger_combo;
            bigger_combo = &combo;
            if ((bigger_combo = find_combo(player->summary, &tmp)) != NULL && bigger_combo->type != BOMB_old && bigger_combo->type != ROCKET)
            {
                *cur = *bigger_combo;
                return 0;
            }

            if (pre->type == SINGLE)
            {
#pragma region SINGLE
                for (int i = 0; i < player->summary->three_one_num; ++i)
                {
                    if (player->summary->three_one[i]->three_desc[0] > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = SINGLE;
                        cur->low = player->summary->three_one[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                for (int i = 0; i < player->summary->three_two_num; ++i)
                {
                    if (player->summary->three_two[i]->three_desc[0] > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = SINGLE;
                        cur->low = player->summary->three_two[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                for (int i = 0; i < player->summary->series_num; ++i) // 长度超过5的顺子
                {
                    if (player->summary->series[i]->len > 5)
                    {
                        if (player->summary->series[i]->low > max_ddz(tmp.low, pre->low))
                        {
                            cur->type = SINGLE;
                            cur->low = player->summary->series[i]->low;
                            cur->len = 1;
                            return 0;
                        }
                        else if (player->summary->series[i]->low + player->summary->series[i]->len - 1 > max_ddz(tmp.low, pre->low))
                        {
                            cur->type = SINGLE;
                            cur->low = player->summary->series[i]->low + player->summary->series[i]->len - 1;
                            cur->len = 1;
                            return 0;
                        }
                    }
                }

                for (int i = 0; i < player->summary->pairs_num; ++i)
                {
                    if (player->summary->pairs[i]->low > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = SINGLE;
                        cur->low = player->summary->pairs[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                for (int i = 0; i < player->summary->three_num; ++i)
                {
                    if (player->summary->three[i]->low > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = SINGLE;
                        cur->low = player->summary->three[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                // 拆牌找
                for (int i = lord->h->end; i <= P2; ++i)
                {
                    if (i > pre->low && 0 < player->h->hands[i] && player->h->hands[i] < 4)
                    {
                        cur->type = SINGLE;
                        cur->low = i;
                        cur->len = 1;
                        return 0;
                    }                    
                }
#pragma endregion 
            }
            else if (pre->type == PAIRS)
            {
#pragma region PAIRS
                for (int i = 0; i < player->summary->three_two_num; ++i)
                {
                    if (player->summary->three_two[i]->three_desc[0] > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = PAIRS;
                        cur->low = player->summary->three_two[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                for (int i = 0; i < player->summary->pairs_series_num; ++i) // 长度超过5的顺子
                {
                    if (player->summary->pairs_series[i]->len > 5)
                    {
                        if (player->summary->pairs_series[i]->low > max_ddz(tmp.low, pre->low))
                        {
                            cur->type = PAIRS;
                            cur->low = player->summary->pairs_series[i]->low;
                            cur->len = 1;
                            return 0;
                        }
                        else if (player->summary->pairs_series[i]->low + player->summary->pairs_series[i]->len - 1 > max_ddz(tmp.low, pre->low))
                        {
                            cur->type = PAIRS;
                            cur->low = player->summary->pairs_series[i]->low + player->summary->pairs_series[i]->len - 1;
                            cur->len = 1;
                            return 0;
                        }
                    }
                }

                for (int i = 0; i < player->summary->three_num; ++i)
                {
                    if (player->summary->three[i]->low > max_ddz(tmp.low, pre->low))
                    {
                        cur->type = PAIRS;
                        cur->low = player->summary->three[i]->low;
                        cur->len = 1;
                        return 0;
                    }
                }

                // 拆牌找
                for (int i = lord->h->end; i <= P2; ++i)
                {
                    if (i > pre->low && 2 <= player->h->hands[i] && player->h->hands[i] < 4)
                    {
                        cur->type = PAIRS;
                        cur->low = i;
                        cur->len = 1;
                        return 0;
                    }
                }
#pragma endregion 
            }
        }

		COMBO_OF_POKERS *c;

		if ((c = find_combo(player->summary, pre)) != NULL &&c->type != BOMB_old && c->type != ROCKET)
		{
			*cur = *c;
			return 0;
		}

		if ((c = find_combo(upfm->summary, pre)) != NULL &&c->type != BOMB_old && c->type != ROCKET)
		{
			return 1;
		}
		pass = must_play_bigger_to_stop_lord(cur, player, pre, game, 1, is_good_farmer, 1);
		return pass;
	}


	{
		if (is_good_farmer)
		{
			if (PRE_PLAYER(game) == LORD_old//pre player is lord.
				&&
				((pre->type >= 3311) //飞机
					|| (pre->type == PAIRS_SERIES)
					|| (pre->type == SINGLE_SERIES && pre->len>8)
					) //lord has only one poker
				)
			{
				if (game->known_others_poker)
				{
					if (!get_num_of_bigger(player->combos, MAX_COMBO_NUM, pre))
					{
						if (get_num_of_bigger(upfm->combos, MAX_COMBO_NUM, pre))
						{
							return 1;
						}
					}
				}
				//必管..
				pass = must_play_bigger(cur, player, pre, game, 0, is_good_farmer, 1);
			}
			else if (PRE_PLAYER(game) == LORD_old //pre player is lord.
				&& (pre->type == SINGLE && pre->low >= P2 && pre->low<BIG_JOKER))
			{
				pass = must_play_for_poker2(game, cur, player, pre, 1);
				if (pass == 1)
					pass = farmer_play_normal(game, cur, pre);
			}
			else
			{
				if (game->players[game->pre_playernum]->id != LORD_old && (
					has_control_poker(pre, player->lower_control_poker) >0
					|| game->players[CUR_PLAYER]->oppDown_num <= 4
					|| get_combo_number(pre) >= 4
					|| (pre->low >= P9 && get_combo_number(pre) >= 2)
					|| pre->low >= Pj)
					)
				{
					if (game->known_others_poker)
					{

					}
					pass = 1;
				}
				else {
					pass = farmer_play_normal(game, cur, pre);
					if (PRE_PLAYER(game) != LORD_old &&
						((pre->low >= P9 && get_combo_number(pre) >= 2)
							|| pre->low >= Pk)
						)
						pass = 1;
				}
			}
		}
		else //bad down farmer
		{
			if ((game->players[game->pre_playernum]->id == LORD_old) //pre player is lord.
				&& (get_combo_number(pre) >= 3))
			{   //必管..
				pass = must_play_bigger(cur, player, pre, game, 0, is_good_farmer, 1);
			}
			else if ((game->players[game->pre_playernum]->id == LORD_old) //pre player is lord.
				&& (pre->type == SINGLE && pre->low >= P2 && pre->low<BIG_JOKER))
			{
				pass = must_play_for_poker2(game, cur, player, pre, 1);
			}
			else if ((game->players[game->pre_playernum]->id == LORD_old) //pre player is lord. //pre's type is single or pair
				)
			{
				if (!is_game_first_half(game, CUR_PLAYER) && pre->type != SINGLE && pre->type != PAIRS)
					pass = must_play_bigger(cur, player, pre, game, 0, is_good_farmer, 1);
				else
					pass = farmer_play_normal(game, cur, pre);
			}
			else //from the other farmer.
				pass = 1;
		}
	}
	return pass;
}

//play the biggest for win


int upframer_play(GAME_DDZ_AI* game, COMBO_OF_POKERS * cur, COMBO_OF_POKERS *pre)
{
	PLAYER*player = game->players[CUR_PLAYER];
	PLAYER* lord = game->players[DOWN_PLAYER];

	int is_good_farmer = player->good_framer;
	int pass;
	COMBO_OF_POKERS *res;
	//if ((res = check_for_win_now(game, pre)) != NULL) //win immediately, play the biggest or bomb
	//{
	//	remove_combo_in_suit(player->summary, res);
	//	memmove(cur, res, sizeof(COMBO_OF_POKERS));
	//	res->type = NOTHING;
	//	return 0;
	//}

    if (PRE_PLAYER(game) == LORD_old || (PRE_PLAYER(game) == DOWNFARMER_old && game->players[CUR_PLAYER]->oppUp_num <= 6))
    {
        if (player->summary->bomb_num > 0 && is_combo_biggest_sure(game, player->summary->bomb[player->summary->bomb_num - 1])
            && ((player->summary->bomb_num == player->summary->biggest_num && player->summary->not_biggest_num <= 1)
            || (player->summary->bomb_num + 1 == player->summary->biggest_num && player->summary->not_biggest_num == 0)))
        {
            if (opp_hasbomb(game) && game->players[DOWN_PLAYER]->summary->bomb_num > 0
                && game->players[DOWN_PLAYER]->summary->bomb[game->players[DOWN_PLAYER]->summary->bomb_num - 1]->low > player->summary->bomb[player->summary->bomb_num - 1]->low)
            {
                // 除炸弹外只剩一套，或者可以4带2
                if (game->players[DOWN_PLAYER]->summary->real_total_num <= game->players[DOWN_PLAYER]->summary->bomb_num + 1
                    || (game->players[DOWN_PLAYER]->summary->bomb_num > 1
                    && game->players[DOWN_PLAYER]->summary->real_total_num == game->players[DOWN_PLAYER]->summary->bomb_num + 2
                    && (game->players[DOWN_PLAYER]->summary->singles_num == 2 || game->players[DOWN_PLAYER]->summary->pairs_num == 2)))
                {
                    return 1;
                }
            }

            for (int i = 0; i < player->summary->bomb_num; ++i)
            {
                if (check_combo_a_Big_than_b(player->summary->bomb[i], pre))
                {
                    memcpy(cur, player->summary->bomb[i], sizeof(COMBO_OF_POKERS));
                    return 0;
                }
            }
            return 1;
        }
    }

	if (game->players[CUR_PLAYER]->oppDown_num == 1)
	{
		pass = farmer_play_when_lord_has_1_poker_only(cur, pre, player, game, 1, is_good_farmer);
		return pass;
	}

	//是否需要上手
	if (game->known_others_poker && PRE_PLAYER(game) == LORD_old
		&& ((lord->summary->combo_total_num <= 1 && player->oppDown_num <= 10))
		)
		/*||player->oppDown_num<=2*/
	{
		pass = must_play_bigger_to_stop_lord(cur, player, pre, game, 1, is_good_farmer, 1);
		return pass;
	}

    if (player->summary->real_total_num <= 2 ||
        player->summary->combo_total_num <= 1)
    {
        if (game->use_best_for_undefined_case)
        {
            pass = get_the_best_hand(game, cur, 0);
            if (!pass)
            {
                return pass;
            }
        }
    }
	// biguan, pre player is LORD_old...
	if (is_upfarmer_must_play_bigger(game, is_good_farmer, pre))
	{
		pass = must_play_bigger(cur, player, pre, game, 1, is_good_farmer, 1);
		if (player->oppDown_num >= 10 && !pass)
		{
			if (get_control_poker_num_in_combo(cur, player->lower_control_poker) >= 2)//&& c[k].type!=PAIRS)
			{
				if (pre->low + 2 >= cur->low) {}
				else
					pass = 1;
			}
		}
		return pass;
	}

	if (PRE_PLAYER(game) == LORD_old //pre player is lord.
		&& (pre->type == SINGLE && pre->low == P2))
	{
		pass = must_play_for_poker2(game, cur, player, pre, 1);
        if (pass == 1)
        {
            pass = upfarmer_play_normal(game, cur, pre);
            // 如果拆王炸
            if (pass == 0 && cur->type == SINGLE
                && ((cur->low == LIT_JOKER && player->h->hands[BIG_JOKER] == 1) || (cur->low == BIG_JOKER && player->h->hands[LIT_JOKER] == 1)))
            {
                COMBO_OF_POKERS tmp;
                tmp.type = ROCKET;
                tmp.low = LIT_JOKER;
                if (_CheckWinWrapper(game, &tmp))
                {
                    cur->type = tmp.type;
                    cur->low = LIT_JOKER;
                }
            }
        }
		return pass;
	}

	if (PRE_PLAYER(game) == LORD_old //pre player is lord.
		&& (pre->type == SINGLE && pre->low == LIT_JOKER) && player->h->hands[BIG_JOKER] == 1)
	{
		cur->type = SINGLE;
		cur->low = BIG_JOKER;
		return 0;
	}
	// pre is down_farmer
	if (PRE_PLAYER(game) != LORD_old)
	{
		// check downfarmer is good or not...
		if (game->known_others_poker)
		{
			//todo :
            if (pre->type != BOMB_old && lord->summary->real_total_num == 1 && find_combo(lord->summary, pre)) // 检查刚好能够管上
            {
                COMBO_OF_POKERS* tmp = find_combo(player->summary, pre);
                if (tmp && tmp->type != BOMB_old && tmp->type != ROCKET)
                {
                    memcpy(cur, tmp, sizeof(COMBO_OF_POKERS));
                    return 0;
                }
            }
		}
		if (has_control_poker(pre, player->lower_control_poker) >0
			//|| game->players[CUR_PLAYER]->oppUp_num<=4
			|| get_combo_number(pre) >= 4)
			return 1;
	}

	if (pre->type == SINGLE || pre->type == PAIRS)
	{
		pass = upfarmer_make_a_bigger_for_single_and_pair(game, pre, cur, is_good_farmer);
		return pass;
	}

	if (!player->good_framer && PRE_PLAYER(game) != LORD_old)
	{
		return  1;
	}
	///*
	if (PRE_PLAYER(game) == LORD_old) //must play again..
	{
		int prob = rand() % 100;
		int play_prob = player->good_framer ? 40 : 80;
		if (player->oppDown_num <= 4 ||
			(player->oppDown_num <= 6 && prob<play_prob + 20) ||
			prob<play_prob)
		{
			pass = must_play_bigger(cur, player, pre, game, 1, is_good_farmer, 1);
			return pass;
		}
	}
	// */
	return upfarmer_play_normal(game, cur, pre);
}

int check_farmer_is_good(GAME_DDZ_AI* game)
{

	PLAYER * curPlayer = game->players[CUR_PLAYER];
	PLAYER * upPl = game->players[UP_PLAYER];
	PLAYER * dnPl = game->players[DOWN_PLAYER];

	COMBOS_SUMMARY_DDZ_AI * s = curPlayer->summary;
	int isup = game->player_type == UPFARMER_old;
	int is_first_half_game = is_game_first_half(game, CUR_PLAYER);
	int is_good_farmer = CONTROL_SUB_COMBO_NUM(curPlayer) >=
		(is_first_half_game ? -50 : isup ? -20 : -30);

	if ((game->players[CUR_PLAYER]->oppUp_num == 1 && game->players[UP_PLAYER]->id == LORD_old)
		|| (game->players[CUR_PLAYER]->oppDown_num == 1 && game->players[DOWN_PLAYER]->id == LORD_old))
	{   //refine for lords only has one poker..
		if (s->singles_num <= 1 || s->singles[1]->low >= curPlayer->opps->end)
			is_good_farmer = 1;
		else {
			search_combos_in_suits_for_MIN_single_farmer(game, curPlayer->h, curPlayer->combos, curPlayer); //should done in other place, since you should know it..
			if (s->singles_num <= 1 || s->singles[1]->low >= curPlayer->opps->end)
				is_good_farmer = 1;
			else
				is_good_farmer = 0;
		}
	}

	//double check
	if (isup && !is_good_farmer)
	{
		if (curPlayer->summary->combo_total_num < upPl->summary->combo_total_num
			|| curPlayer->summary->combo_total_num <= 3
			|| curPlayer->summary->combo_total_num<dnPl->summary->combo_total_num + 1)
		{
			is_good_farmer = 1;
		}
	}
	else if (!isup && !is_good_farmer)
	{
		if (curPlayer->summary->combo_total_num < dnPl->summary->combo_total_num
			|| curPlayer->summary->combo_total_num <= 3
			|| curPlayer->summary->combo_total_num<upPl->summary->combo_total_num + 1)
		{
			is_good_farmer = 1;
		}
	}

	curPlayer->good_framer = is_good_farmer;
	return is_good_farmer;
}



//void DUMP_GAME_FOR_PLAYER(LordRobot * r);

void update_copied_game(GAME_DDZ_AI* game, GAME_DDZ_AI * ref_g)
{
	game->pot = &game->POT;
	game->computer[0] = 1;
	int j;
	{

        for (j = 0; j < 3; j++)
        {
            game->suits[j].h = &game->suits[j].p;
            game->suits[j].opps = &game->suits[j].opp;
            game->suits[j].combos = &game->suits[j].combos_store[0];
            game->suits[j].summary = &game->suits[j].combos_summary;
            PLAYER * pl = &game->suits[j];
            sort_all_combos(game, pl->combos_store, max_POKER_NUM,
                pl->summary, pl->opps, pl->oppUp_num, pl->oppDown_num,
                pl->lower_control_poker, pl);

            if (game->pre.type == pl->curcomb.type
                && game->pre.low == pl->curcomb.low
                && game->pre.len == pl->curcomb.len)
            {
                pl->cur = pl->curcomb.type != NOTHING_old ? &pl->curcomb : NULL;
            }
            else if (game->pre.type == pl->curcomb.type && game->pre.type == ROCKET)
            {
                pl->cur = pl->curcomb.type != NOTHING_old ? &pl->curcomb : NULL;
            }
            else
            {
                pl->cur = NULL;
            }
        }
	}
	for (j = 0; j<3; j++) {
		int i;
		for (i = 0; i<3; i++) {
			if (ref_g->players[j] == &ref_g->suits[i]) {
				game->players[j] = &game->suits[i];
			}
		}
	}
	game->rec.now = 0; //clear record

}

void Robot_copy(LordRobot *dst, LordRobot *src)
{
	memcpy(dst, src, sizeof(LordRobot));
	// update game
	GAME_DDZ_AI *game = &dst->game;
	update_copied_game(game, &src->game);
}

void GAME_copy(GAME_DDZ_AI *dst, GAME_DDZ_AI *src)
{
	memcpy(dst, src, sizeof(GAME_DDZ_AI));
	// update game
	update_copied_game(dst, src);
}

//for internal use
void convert(int * outs, int * out1, int *out2)
{
	if (*outs == -1) //no output
	{
		*out1 = -1;
		*out2 = -1;
		return;
	}
	int hun_num = *outs++;
	if (hun_num >4 || hun_num<0)
	{
		PRINTF_ALWAYS("sucks in line %d\n", __LINE__);
	}
	do
	{
		*out1++ = *outs++;
	} while (*outs != -1);
	out1[-hun_num * 2] = -1;
	outs -= hun_num * 2;
	// outs--;
	for (int i = 0; i<hun_num; i++)
	{
		*out2++ = *outs++;
	}
	*out2 = -1;
}


//for internal use
void convert_to_new_out_format(int * outs, int * out1, int *out2)
{
	if (*outs == -1) //no output
	{
		*out1 = -1;
		*out2 = -1;
		return;
	}
	int hun_num = *outs++;
	if (hun_num >4 || hun_num<0)
	{
		PRINTF_ALWAYS("sucks in line %d\n", __LINE__);
	}
	do
	{
		*out1++ = *outs++;
	} while (*outs != -1);
	//   out1[-hun_num]=-1;
	outs -= hun_num * 2;
	out1 -= hun_num * 2;

	// outs--;
	for (int i = 0; i<hun_num; i++)
	{
		*out2++ = *outs++;
	}
	for (int i = 0; i<hun_num; i++)
	{
		*out1++ = *outs++;
	}
	*out2 = -1;
	*out1 = -1;
}

//todo: fix for 1,1,1
int Check_win_quick(GAME_DDZ_AI * gm, int firstplayer_num, bool cur, bool up, bool down)  //need optimization
{

	PRINTF(LEVEL, "\n===========BEGIN CHECK WIN===============\n");
	FILE * bak_logfile2 = logfile2;
	logfile2 = NULL;
#if 1
	LordRobot* robot[3];
	robot[0] = createRobot(1, 0);       
	robot[1] = createRobot(1, 0);
	robot[2] = createRobot(1, 0);
	int out1[26], out2[5];

	GAME_copy(&robot[0]->game, gm);
	GAME_copy(&robot[1]->game, gm);
	GAME_copy(&robot[2]->game, gm);


	PLAYER* tmp;

	/*
	player1's view
	CUR:

	*/
	/*
	search_combos_in_suit(&robot[0]->game, robot[0]->game.players[CUR_PLAYER] ->h,
	robot[0]->game.players[CUR_PLAYER] ->opps, robot[0]->game.players[CUR_PLAYER] );
	search_combos_in_suit(&robot[0]->game, robot[0]->game.players[DOWN_PLAYER] ->h,
	robot[0]->game.players[DOWN_PLAYER] ->opps, robot[0]->game.players[DOWN_PLAYER] );
	search_combos_in_suit(&robot[0]->game, robot[0]->game.players[UP_PLAYER] ->h,
	robot[0]->game.players[UP_PLAYER] ->opps, robot[0]->game.players[UP_PLAYER] );
	*/

	//update for robot[1]
	tmp = robot[1]->game.players[CUR_PLAYER];
	robot[1]->game.players[CUR_PLAYER] = robot[1]->game.players[DOWN_PLAYER];
	robot[1]->game.players[DOWN_PLAYER] = robot[1]->game.players[UP_PLAYER];
	robot[1]->game.players[UP_PLAYER] = tmp;
	robot[1]->game.player_type = robot[1]->game.players[CUR_PLAYER]->id;
	if (robot[0]->game.pre_playernum == CUR_PLAYER) //now always it is
	{
		robot[1]->game.pre_playernum = UP_PLAYER;
		robot[2]->game.pre_playernum = DOWN_PLAYER;
	}
	else if (robot[0]->game.pre_playernum == DOWN_PLAYER) //now always it is
	{
		robot[1]->game.pre_playernum = CUR_PLAYER;
		robot[2]->game.pre_playernum = UP_PLAYER;
	}
	else if (robot[0]->game.pre_playernum == UP_PLAYER) //now always it is
	{
		robot[1]->game.pre_playernum = DOWN_PLAYER;
		robot[2]->game.pre_playernum = CUR_PLAYER;
	}
	/*
	search_combos_in_suit(&robot[1]->game, robot[1]->game.players[CUR_PLAYER] ->h,
	robot[1]->game.players[CUR_PLAYER] ->opps, robot[1]->game.players[CUR_PLAYER] );
	search_combos_in_suit(&robot[1]->game, robot[1]->game.players[DOWN_PLAYER] ->h,
	robot[1]->game.players[DOWN_PLAYER] ->opps, robot[1]->game.players[DOWN_PLAYER] );
	search_combos_in_suit(&robot[1]->game, robot[1]->game.players[UP_PLAYER] ->h,
	robot[1]->game.players[UP_PLAYER] ->opps, robot[1]->game.players[UP_PLAYER] );
	*/

	//update for robot[2]
	tmp = robot[2]->game.players[CUR_PLAYER];
	robot[2]->game.players[CUR_PLAYER] = robot[2]->game.players[UP_PLAYER];
	robot[2]->game.players[UP_PLAYER] = robot[2]->game.players[DOWN_PLAYER];
	robot[2]->game.players[DOWN_PLAYER] = tmp;

	robot[2]->game.player_type = robot[2]->game.players[CUR_PLAYER]->id;
	/*
	search_combos_in_suit(&robot[2]->game, robot[2]->game.players[CUR_PLAYER] ->h,
	robot[2]->game.players[CUR_PLAYER] ->opps, robot[2]->game.players[CUR_PLAYER] );
	search_combos_in_suit(&robot[2]->game, robot[2]->game.players[DOWN_PLAYER] ->h,
	robot[2]->game.players[DOWN_PLAYER] ->opps, robot[2]->game.players[DOWN_PLAYER] );
	search_combos_in_suit(&robot[2]->game, robot[2]->game.players[UP_PLAYER] ->h,
	robot[2]->game.players[UP_PLAYER] ->opps, robot[2]->game.players[UP_PLAYER] );

	*/
	robot[0]->game.players[CUR_PLAYER]->use_quick_win_check = cur;
	robot[1]->game.players[CUR_PLAYER]->use_quick_win_check = down;
	robot[2]->game.players[CUR_PLAYER]->use_quick_win_check = up;
	robot[0]->game.use_best_for_undefined_case = cur;
	robot[1]->game.use_best_for_undefined_case = down;
	robot[2]->game.use_best_for_undefined_case = up;

	switch (firstplayer_num)
	{
	default:
	case CUR_PLAYER:
		goto game_turn_p1;
	case DOWN_PLAYER:
		goto game_turn_p2;
	case UP_PLAYER:
		goto game_turn_p3;

	}

	int pass1, winner;

	while (1)
	{
	game_turn_p1:
		//DUMP_GAME_FOR_PLAYER(robot[0]);
		//      DUMP_GAME_FOR_PLAYER(robot[1]);
		//      DUMP_GAME_FOR_PLAYER(robot[2]);
		if (robot[0]->game.players[2]->h->total <= 0) //game over
			break;
		pass1 = takeOut_internal(robot[0], out1, out2);
		getPlayerTakeOutCards_internal(robot[1], out1, out2, 1);
		getPlayerTakeOutCards_internal(robot[2], out1, out2, 0);
		winner = CUR_PLAYER;
		if (robot[0]->game.players[2]->h->total <= 0) //game over
			break;

	game_turn_p2:

		//DUMP_GAME_FOR_PLAYER(robot[1]);
		pass1 = takeOut_internal(robot[1], out1, out2);
		getPlayerTakeOutCards_internal(robot[2], out1, out2, 1);
		getPlayerTakeOutCards_internal(robot[0], out1, out2, 0);
		winner = DOWN_PLAYER;
		if (robot[1]->game.players[2]->h->total <= 0) //game over
			break;
	game_turn_p3:
		//DUMP_GAME_FOR_PLAYER(robot[2]);
		if (robot[2]->game.players[2]->h->total <= 0) //game over
			break;
		pass1 = takeOut_internal(robot[2], out1, out2);
		getPlayerTakeOutCards_internal(robot[0], out1, out2, 1);
		getPlayerTakeOutCards_internal(robot[1], out1, out2, 0);
		winner = UP_PLAYER;
		if (robot[2]->game.players[2]->h->total == 0) //game over
			break;
		if (robot[2]->game.players[2]->h->total<0) //game over
		{
			PRINTF_ALWAYS("line %d sucks\n", __LINE__);
			break;
		}
		//goto turn_p1;
	}

	PRINTF(LEVEL, "winner is player %d\n", winner);
	PRINTF(LEVEL, "===========END   CHECK WIN===============\n\n");
	int res = 0;
	if (winner == CUR_PLAYER ||
		(robot[0]->game.players[CUR_PLAYER]->id != LORD_old
			&& robot[0]->game.players[winner]->id != LORD_old)
		)
		res = 1;
	destoryRobot(robot[0]);
	destoryRobot(robot[1]);
	destoryRobot(robot[2]);
	logfile2 = bak_logfile2;
#endif
	return res;

}

typedef struct {
	int id;//current player
	POKERS p;
} POKER_AND_TYPE;
typedef struct CNODE CNODE;
typedef struct CNODE {
	COMBO_OF_POKERS c;
	POKER_AND_TYPE *h;
	POKER_AND_TYPE *h1;
	POKER_AND_TYPE *h2;
	int value;
	int MAX_value;
	int prop;
	int child_nums;
	CNODE * farther;
	CNODE* childs;
} CNODE;


void generate_perhaps_nodes(CNODE * t)
{
	//if it is the first time for run..
	int first = 0;
	if (t->c.type == NOTHING_old && (t->farther == NULL || t->farther->c.type == NOTHING_old))
		first = 1;
	if (first)
	{
		COMBO_OF_POKERS combos[100];
		int num = 0;
		POKERS tmp;
		tmp = t->h1->p;
		num += search_general_1(&tmp, combos, 0, 0, 0, 0);
		//    tmp = t->h1->p;
		//   num+=search_general_2(&tmp, combos+num, 0,0,0,0);
		//    tmp = t->h1->p;
		//   num+=search_general_3(&tmp, combos+num, 0,0,0,0);
		int MAX_value = 1;
		//todo: add others and remove duplicated combos.
		PRINTF(LEVEL, "test combos %d: \n", num);
		print_all_combos(combos, num);
		t->childs = (CNODE*)malloc(sizeof(CNODE) * num);
		for (int i = 0; i<num; i++)
		{
			CNODE * n = &t->childs[t->child_nums++];
			n->c = combos[i];
			n->MAX_value = MAX_value;
			n->value = 0;
			n->farther = t;
			n->child_nums = 0;
			n->h = t->h1;
			n->h1 = t->h2;
			n->h2 = t->h;
			n->prop = 1;
		}
	}
	else
	{
		COMBO_OF_POKERS c;
		COMBO_OF_POKERS *p;
		if (t->c.type != NOTHING_old) p = &t->c;
		else p = &t->farther->c;
		int MAX_value = 1;
		t->childs = (CNODE*)malloc(sizeof(CNODE) * 100);
		while (find_a_bigger_combo_in_hands(&t->h1->p, &c, p))
		{
			CNODE *n = &t->childs[t->child_nums++];
			n->c = c;
			n->MAX_value = MAX_value;
			n->value = 0;
			n->farther = t;
			n->child_nums = 0;
			n->h = t->h1;
			n->h1 = t->h2;
			n->h2 = t->h;
			n->prop = 1;

			p = &n->c;
		}

		CNODE *n = &t->childs[t->child_nums++];
		n->c.type = NOTHING_old;
		n->MAX_value = MAX_value;
		n->value = 0;
		n->farther = t;
		n->child_nums = 0;
		n->h = t->h1;
		n->h1 = t->h2;
		n->h2 = t->h;
		n->prop = 1;
		p = &n->c;
	}

}

int get_node_value(CNODE * t)
{
	register POKERS tmp;

	// /*
	PRINTF(LEVEL, "TESTing NODE VALUE: %d, type %d Combo ", t->value, t->h->id);
	if (t->c.type != NOTHING_old)
		print_combo_poker(&t->c);
	PRINTF(LEVEL, "\n");
	print_hand_poker(&t->h->p);
	print_hand_poker(&t->h1->p);
	print_hand_poker(&t->h2->p);
	//*/
	if (t->c.type != NOTHING_old) {
		combo_2_poker(&tmp, &t->c);
		sub_poker(&t->h->p, &tmp, &t->h->p);
	}
	int val;
	if (0 == t->h->p.total) //done
	{
		PRINTF(LEVEL, "DONE, %d win\n", t->h->id);
		t->value = 1;
		if (t->c.type == BOMB_old || t->c.type == ROCKET)
		{
			t->value = 2;
		}
		val = t->value;
	}
	else
	{
		generate_perhaps_nodes(t);
		//for( int i=0;i<t->child_nums;i++)
		int loop = t->child_nums;//>2?2:t->child_nums;
		for (int i = 0; i<loop; i++)
		{
			int value = get_node_value(&t->childs[i]);
			if (t->h->id != DOWNFARMER_old) //child is UPFARMER_old
				value = -value;
			if (value >= t->MAX_value) //reach the best, exit
			{
				t->value = value;
				val = value;
				break;
			}
			t->value = value;
		}
	}
	//free child nodes
	// for( int i=0;i<t->child_nums;i++)
	{
		free(t->childs);
	}
	// PRINTF(LEVEL,"NODE VALUE: %d, type %d Combo ",t->value,t->h->id);
	//   if(t->c.type!=NOTHING_old)
	//  print_combo_poker(&t->c);
	//   PRINTF(LEVEL,"\n");
	if (t->c.type != NOTHING_old)
		add_poker(&t->h->p, &tmp, &t->h->p);
	return t->value;

}

int get_the_best_hand_2(GAME_DDZ_AI *game, COMBO_OF_POKERS * cur, bool first)
{
	//generate the top NODE;
	CNODE top;
	register POKER_AND_TYPE p[3];
	PLAYER *pl = game->players[CUR_PLAYER];
	p[0].p = *pl->h;
	p[0].id = pl->id;
	p[1].p = *game->players[DOWN_PLAYER]->h;
	p[1].id = game->players[DOWN_PLAYER]->id;
	p[2].p = *game->players[UP_PLAYER]->h;
	p[2].id = game->players[UP_PLAYER]->id;

	top.farther = NULL;
	top.h = &p[2];
	top.h1 = &p[0];
	top.h2 = &p[1];
	if (first)
	{

		top.c.type = NOTHING_old;
	}
	else
	{
		top.c = game->pre;
	}

	top.MAX_value = 1; //todo: check if there's bomb in our side;

	generate_perhaps_nodes(&top);
	int value;
	int MAX_value, MAX_value_item = 0;
	if (top.child_nums>1)
	{
		for (int i = 0; i<top.child_nums; i++)
		{
			value = get_node_value(&top.childs[i]);
			if (value >= top.MAX_value)
				break;
		}
		//get the best one
		MAX_value = top.childs[0].value;
		for (int i = 1; i<top.child_nums; i++)
		{
			if (top.childs[i].value > MAX_value)
			{
				MAX_value_item = i;
			}
		}
	}

	*cur = top.childs[MAX_value_item].c;

	//for( int i=0;i<top.child_nums;i++)
	{
		free(top.childs);
	}
	if (cur->type != NOTHING_old)
	{
		return 0;
	}
	else
	{
		if (first == 1) PRINTF_ALWAYS("line %d, fatal error, must play some cards..\n", __LINE__);
		return 1;
	}
}


void check_card(POKERS *h, int * card)
{
	POKERS t;
	read_poker_int_2(card, max_POKER_NUM, &t);
	if (0 != cmp_poker(&t, h))
	{
		PRINTF_ALWAYS("card not match poker\n");
	}
}


typedef struct {
	COMBO_OF_POKERS c;
	int val;
} COMBO_VALUE;

//get_the_best_hand出的牌是否合理极大程度的依赖于此函数cv中牌的排序
int generate_perhaps_combos(GAME_DDZ_AI *game, COMBO_VALUE* cv, int first)
{
	PLAYER *player = game->players[CUR_PLAYER];
	COMBO_VALUE empty_cv;
	empty_cv.c.type = NOTHING_old;
	empty_cv.c.low = 0;
	empty_cv.c.len = 0;
	empty_cv.val = 0;

	int i = 0;
	if (first)
	{
		//just use combos in summary..
		for (int j = 0; j<(player->summary->not_biggest_num); j++)
		{
			if (player->summary->not_biggest[j]->type != SINGLE &&
				player->summary->not_biggest[j]->type != PAIRS)
			{
				cv[i].c = *player->summary->not_biggest[j];
				cv[i++].val = 0;
			}
			/*
			if(player->combos_store[j].type!=NOTHING_old)
			{
			cv[i].c=player->combos_store[j];
			cv[i++].val=0;
			}
			*/
			//if(player->summary->not_biggest_num)
		}

		for (int j = 0; j<(player->summary->biggest_num); j++)
		{
			if (player->summary->biggest[j]->type != SINGLE
				&& player->summary->biggest[j]->type != PAIRS
				&& NOT_BOMB(player->summary->biggest[j]))
			{
				cv[i].c = *player->summary->biggest[j];
				cv[i++].val = 0;
			}
		}

		memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));

		//todo: test other combossss??
		//不拆对
		for (int j = player->h->begin; j <= player->h->end; j++)
		{
			if (player->h->hands[j] == 1)
			{
				cv[i].c.type = SINGLE;
				cv[i].c.low = j;
				cv[i].val = 0;
			}
			else if (player->h->hands[j] == 2)
			{
				cv[i].c.type = PAIRS;
				cv[i].c.low = j;
				cv[i].val = 0;
			}

			//检查是否已被找到
			if (cv[i].c.type != NOTHING_old)
			{
				bool isExist = false;
				for (int k = 0; k < i; ++k)
				{
					if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
					{
						isExist = true;
						break;
					}
				}
				if (!isExist)
				{
					++i;
				}
				memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
			}
		}

		//拆对, 拆三条
		for (int j = player->h->begin; j <= player->h->end; j++)
		{
			//单
			if (player->h->hands[j] > 0)
			{
				cv[i].c.type = SINGLE;
				cv[i].c.low = j;
				cv[i].val = 0;
			}

			//检查是否已被找到
			if (cv[i].c.type != NOTHING_old)
			{
				bool isExist = false;
				for (int k = 0; k < i; ++k)
				{
					if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
					{
						isExist = true;
						break;
					}
				}
				if (!isExist)
				{
					++i;
				}
				memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
			}

			//对
			if (player->h->hands[j] > 1)
			{
				cv[i].c.type = PAIRS;
				cv[i].c.low = j;
				cv[i].val = 0;
			}

			//检查是否已被找到
			if (cv[i].c.type != NOTHING_old)
			{
				bool isExist = false;
				for (int k = 0; k < i; ++k)
				{
					if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
					{
						isExist = true;
						break;
					}
				}
				if (!isExist)
				{
					++i;
				}
				memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
			}
		}

		// put bombs at the end
		for (int j = 0; j < (player->summary->biggest_num); j++)
		{
			if (!NOT_BOMB(player->summary->biggest[j]))
			{
				cv[i].c = *player->summary->biggest[j];
				cv[i++].val = 0;
			}
		}
	}
	else
	{
        bool isPartner = (game->players[game->pre_playernum]->id != LORD_old && game->players[CUR_PLAYER]->id != LORD_old);
        if (isPartner)
        {
            memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE)); // PASS放在管牌之前
            ++i;
        }

		COMBO_OF_POKERS *p;
		p = &game->pre;
		while ((p = find_combo(player->summary, p)) != NULL)
		{
			if (NOT_BOMB(p))  // 炸弹放在最后
			{
				cv[i].c = *p;
				cv[i].val = 0;
				i++;
			}
		}

        if (game->pre.type == SINGLE)
        {
            if (game->players[CUR_PLAYER]->id != LORD_old 
                && game->players[DOWN_PLAYER]->id != LORD_old) // 下一个出牌的是队友
            {
                // 从三带里找带的单牌
                for (int j = 0; j < player->summary->three_one_num; ++j)
                {
                    if (player->summary->three_one[j]->three_desc[0] > game->pre.low)
                    {
                        // add
                        cv[i].c.type = SINGLE;
                        cv[i].c.low = player->summary->three_one[j]->three_desc[0];
                        cv[i].c.len = 1;
                        //检查是否已被找到
                        if (cv[i].c.type != NOTHING_old)
                        {
                            bool isExist = false;
                            for (int k = 0; k < i; ++k)
                            {
                                if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                                {
                                    isExist = true;
                                    break;
                                }
                            }
                            if (!isExist)
                            {
                                ++i;
                            }
                            memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                        }
                    }
                }
                for (int j = 0; j < player->summary->threeseries_one_num; ++j)
                {
                    for (int k = 0; k < player->summary->threeseries_one[j]->len; ++k)
                    {
                        // add
                        if (player->summary->threeseries_one[j]->three_desc[k] > game->pre.low)
                        {
                            // TODO
                            cv[i].c.type = SINGLE;
                            cv[i].c.low = player->summary->threeseries_one[j]->three_desc[k];
                            cv[i].c.len = 1;
                            //检查是否已被找到
                            if (cv[i].c.type != NOTHING_old)
                            {
                                bool isExist = false;
                                for (int k = 0; k < i; ++k)
                                {
                                    if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                                    {
                                        isExist = true;
                                        break;
                                    }
                                }
                                if (!isExist)
                                {
                                    ++i;
                                }
                                memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                            }
                        }
                    }
                }
            }
            // 在hands中找只有1张的
            for (int card = P3; card <= BIG_JOKER; ++card)
            {
                if (player->h->hands[card] == 1 && card > game->pre.low)
                {
                    // add SINGLE
                    if (card <= P2 
                        || (card == LIT_JOKER && player->h->hands[BIG_JOKER] == 0) 
                        || (card == BIG_JOKER && player->h->hands[LIT_JOKER] == 0))
                    {
                        cv[i].c.type = SINGLE;
                        cv[i].c.low = card;
                        cv[i].c.len = 1;
                    }
                    //检查是否已被找到
                    if (cv[i].c.type != NOTHING_old)
                    {
                        bool isExist = false;
                        for (int k = 0; k < i; ++k)
                        {
                            if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                            {
                                isExist = true;
                                break;
                            }
                        }
                        if (!isExist)
                        {
                            ++i;
                        }
                        memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                    }
                }
            }
        }
        else if (game->pre.type == PAIRS)
        {
            if (game->players[CUR_PLAYER]->id != LORD_old 
                && game->players[DOWN_PLAYER]->id != LORD_old) // 下一个出牌的是队友
            {
                // 从三带里找带的单牌
                for (int j = 0; j < player->summary->three_two_num; ++j)
                {
                    if (player->summary->three_two[j]->three_desc[0] > game->pre.low)
                    {
                        // add
                        cv[i].c.type = PAIRS;
                        cv[i].c.low = player->summary->three_two[j]->three_desc[0];
                        cv[i].c.len = 1;
                        //检查是否已被找到
                        if (cv[i].c.type != NOTHING_old)
                        {
                            bool isExist = false;
                            for (int k = 0; k < i; ++k)
                            {
                                if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                                {
                                    isExist = true;
                                    break;
                                }
                            }
                            if (!isExist)
                            {
                                ++i;
                            }
                            memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                        }
                    }
                }
                for (int j = 0; j < player->summary->threeseries_two_num; ++j)
                {
                    for (int k = 0; k < player->summary->threeseries_two[j]->len; ++k)
                    {
                        // add
                        if (player->summary->threeseries_two[j]->three_desc[k] > game->pre.low)
                        {
                            // TODO
                            cv[i].c.type = PAIRS;
                            cv[i].c.low = player->summary->threeseries_two[j]->three_desc[k];
                            cv[i].c.len = 1;
                            //检查是否已被找到
                            if (cv[i].c.type != NOTHING_old)
                            {
                                bool isExist = false;
                                for (int k = 0; k < i; ++k)
                                {
                                    if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                                    {
                                        isExist = true;
                                        break;
                                    }
                                }
                                if (!isExist)
                                {
                                    ++i;
                                }
                                memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                            }
                        }
                    }
                }
            }
            // 在hands中找只有2张的
            for (int card = P3; card <= P2; ++card)
            {
                if (player->h->hands[card] == 2 && card > game->pre.low)
                {
                    cv[i].c.type = PAIRS;
                    cv[i].c.low = card;
                    cv[i].c.len = 1;
                    //检查是否已被找到
                    if (cv[i].c.type != NOTHING_old)
                    {
                        bool isExist = false;
                        for (int k = 0; k < i; ++k)
                        {
                            if (cv[k].c.type == cv[i].c.type && cv[k].c.low == cv[i].c.low)
                            {
                                isExist = true;
                                break;
                            }
                        }
                        if (!isExist)
                        {
                            ++i;
                        }
                        memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE));
                    }
                }
            }
        }

		p = &game->pre;
		COMBO_OF_POKERS tmp;
        COMBO_VALUE split_bomb_combos[1000] = {0};
        int split_bomb_combos_num = 0;
		while (find_a_bigger_combo_in_hands(player->h, &cv[i].c, p))
		{
            if (!NOT_BOMB((&cv[i].c))) // 炸弹放在最后
			{
                break;
				//p = &cv[i].c;
				//continue;
			}
            
            // 拆炸弹
            POKERS poker;
            combo_2_poker(&poker, &cv[i].c);
            bool split_bomb = false;
            for (int k = poker.begin; k <= poker.end; ++k)
            {
                if (player->h->hands[k] == 4 
                    || (k == LIT_JOKER && player->h->hands[BIG_JOKER] == 1)
                    || (k == BIG_JOKER && player->h->hands[LIT_JOKER] == 1))
                {
                    split_bomb = true;
                    p = &cv[i].c;
                    memcpy(&split_bomb_combos[split_bomb_combos_num++], &cv[i].c, sizeof(COMBO_VALUE));
                    break;
                }
            }
            if (split_bomb)
            {
                continue;
            }

			memcpy(&tmp, &cv[i].c, sizeof(COMBO_OF_POKERS)); //储存pre
															 //检查是否已找到
			bool isExist = false;
            // 带牌
            if (cv[i].c.type != 31 && cv[i].c.type != 32 
                && cv[i].c.type != 411 && cv[i].c.type != 422
                && cv[i].c.type != 3311 && cv[i].c.type != 3322
                && cv[i].c.type != 333111 && cv[i].c.type != 333222
                && cv[i].c.type != 33331111 && cv[i].c.type != 33332222
                && cv[i].c.type != 531)
            {
                for (int k = 0; k < i; ++k)
                {
                    if (cv[k].c.type == cv[i].c.type
                        && cv[k].c.low == cv[i].c.low
                        && cv[k].c.len == cv[i].c.len) //在检查顺子等需要检查len
                    {
                        isExist = true;
                        break;
                    }
                }
            }
			if (isExist)
			{
				memset(&(cv[i].c), 0, sizeof(COMBO_OF_POKERS));
				p = &tmp;
			}
			else
			{
				p = &cv[i].c;
				cv[i].val = 0;
				i++;
			}
		}

        if (!isPartner)
        {
            memcpy(&cv[i], &empty_cv, sizeof(COMBO_VALUE)); // PASS放再炸弹前
            ++i;
        }

        p = &tmp;
        if (i > 1) // 有其他可以管上的牌
        {
            memcpy(p, &cv[i - 2].c, sizeof(COMBO_OF_POKERS)); // 第i-1个是PASS
        }
        else // 前面只有PASS
        {
            memcpy(p, &game->pre, sizeof(COMBO_OF_POKERS));
        }
		while (find_a_bigger_combo_in_hands(player->h, &cv[i].c, p))
		{
			memcpy(p, &cv[i].c, sizeof(COMBO_OF_POKERS)); //储存pre
			if (!NOT_BOMB((&cv[i].c))) // 只找炸弹
			{
				cv[i].val = 0;
				i++;
			}
		}
        // 拆炸弹的牌组
        memcpy(&cv[i], &split_bomb_combos, sizeof(COMBO_VALUE) * split_bomb_combos_num);
        i += split_bomb_combos_num;
	}
	for (int j = 0; j<i; j++)
	{
		cv[j].c.control = 0;
	}
	return i;
}


int get_the_best_hand(GAME_DDZ_AI *game, COMBO_OF_POKERS * cur, bool first)
{
	PLAYER *pl = game->players[CUR_PLAYER];
	FUNC_NAME
		//get best hands...
		COMBO_VALUE * cv = (COMBO_VALUE *)malloc(1000 * sizeof(COMBO_VALUE));
	int cv_num = generate_perhaps_combos(game, cv, first);
	COMBO_OF_POKERS pre = game->pre;
	POKERS tmp, downTmp, upTmp;
	int card_bak[21], out[26];
	int upOppCard[21] = { 0 };
	int downOppCard[21] = { 0 };
  int t = -1, pass = 1;
	for (int i = 0; i<cv_num; i++)
	{
		if (cv[i].c.type != NOTHING_old)
		{
			game->pre = cv[i].c;

			// 在减牌之前保存副本
			tmp = *pl->h;
			downTmp = *game->players[DOWN_PLAYER]->opps;
			upTmp = *game->players[UP_PLAYER]->opps;

			memcpy(card_bak, pl->card, max_POKER_NUM * sizeof(int));
			remove_combo_poker(pl->h, &game->pre, NULL);

			memcpy(downOppCard, game->players[DOWN_PLAYER]->card, max_POKER_NUM * sizeof(int));
			remove_combo_poker(game->players[DOWN_PLAYER]->opps, &game->pre, NULL);
			game->players[DOWN_PLAYER]->oppUp_num -= get_combo_number(&game->pre);

			memcpy(upOppCard, game->players[UP_PLAYER]->card, max_POKER_NUM * sizeof(int));
			remove_combo_poker(game->players[UP_PLAYER]->opps, &game->pre, NULL);
			game->players[UP_PLAYER]->oppDown_num -= get_combo_number(&game->pre);

			if (game->hun != -1)
				combo_to_int_array_hun(&game->pre, &out[0], pl->card, pl->h->hun);
			//pl->cur = &game->pre;
            memcpy(&pl->curcomb, &game->pre, sizeof(COMBO_OF_POKERS));
			PRINTF(LEVEL, "try output: ");
			print_combo_poker(&cv[i].c);
			PRINTF(LEVEL, "\n");
		}
		else if (first)
		{
			break;
		}

		int winner = 0;
		if (pl->h->total != 0)
			winner = Check_win_quick(game, DOWN_PLAYER, 0, 0, 0);
		else
			winner = 1;

		if (winner)
		{
			cv[i].val = 100;
		}
		/*  else if(pl->id != LORD_old && game->players[winner]->id!=LORD_old)
		{
		cv[i].val=100;
		}
		*/
		else
			cv[i].val = -100;

		if (cv[i].c.type == BOMB_old || cv[i].c.type == ROCKET)
		{
			cv[i].val *= 2;
		}

		if (cv[i].c.type != NOTHING_old)
		{
			// 还原
			*game->players[DOWN_PLAYER]->opps = downTmp;
			memcpy(game->players[DOWN_PLAYER]->card, downOppCard, max_POKER_NUM * sizeof(int));
			game->players[DOWN_PLAYER]->oppUp_num += get_combo_number(&game->pre);

			*game->players[UP_PLAYER]->opps = upTmp;
			memcpy(game->players[UP_PLAYER]->card, upOppCard, max_POKER_NUM * sizeof(int));
			game->players[UP_PLAYER]->oppDown_num += get_combo_number(&game->pre);

			game->pre = pre;

			*pl->h = tmp;
			memcpy(pl->card, card_bak, max_POKER_NUM * sizeof(int));
			pl->cur = NULL; //循环开始之前，game->players3个的cur都是NULL
		}
        if (cv[i].val > 0)
        {
            t = i;
            break;
        }
	}
	
	if (t >= 0 && cv[t].c.type != NOTHING_old)
	{
		pass = (cv[t].val <= 0);
		*cur = cv[t].c;
	}

	free(cv);
	return pass;
}

//return 1 : passed
//return 0: output to cur.
//don't remove any thing...
int robot_play(LordRobot* rb, COMBO_OF_POKERS * cur, bool first)
{
	GAME_DDZ_AI *game = &rb->game;
	PLAYER *pl = game->players[CUR_PLAYER];
	int pass = 1;
	FUNC_NAME;

	//check for robot win now
	if (!first) {
		COMBO_OF_POKERS* res;
        // 仅考察单、对，满足不看牌时的大牌条件，最后还原
        update_summary_for_single_and_pairs(game, pl->summary, pl->h, pl->opps, pl->combos,
            max_POKER_NUM, pl->oppDown_num, pl->oppUp_num, pl->lower_control_poker, pl);
		if ((res = check_for_win_now(game, &game->pre)) != NULL) //win immediately, play the biggest or bomb
		{
            bool farmer_beat_farmer_with_big_cards = game->players[game->pre_playernum]->id != LORD_old 
                && ((res->type == SINGLE && res->low >= P2) 
                    || (res->type == PAIRS && res->low >= Pa) 
                    || ((res->type == THREE || res->type == THREE_ONE || res->type == THREE_TWO) && res->low >= Pa));
            if (!farmer_beat_farmer_with_big_cards || game->players[CUR_PLAYER]->id == LORD_old)
            {
			remove_combo_in_suit(pl->summary, res);//tobe removed
			*cur = *res;
			res->type = NOTHING_old;
			return  0;
		}
	}
        // 还原:仅考察单、对，满足不看牌时的大牌条件
        update_summary(game, pl->summary, pl->h, pl->opps, pl->combos, 
            max_POKER_NUM, pl->oppDown_num, pl->oppUp_num, pl->lower_control_poker, pl);
	}

	//check for other player win...
	//todo..

	//int pass;
	/*
	if(0&&pl->use_quick_win_check
	&& (game->players[CUR_PLAYER]->h->total<=2
	||game->players[DOWN_PLAYER]->h->total<=2
	|| game->players[UP_PLAYER]->h->total<=2)
	&&
	(!first || pl->id!= LORD_old)
	)
	{
	get_the_best_hand(game, cur,first);
	}
	else //normal process
	*/
	{
		//   int pass;
		COMBO_OF_POKERS* pre = &game->pre;
		memset(cur, 0, sizeof(COMBO_OF_POKERS));
		if (1) //current player is computer
		{
			if (game->player_type == LORD_old)//is lord
			{
				if (first) //zhudong chupai
				{
					lord_play_first(game, cur);
					pass = 0;
				}
				else  //beidong chupai
				{
					pass = lord_play_2(game, cur, pre);
				}

			}
			else// farmer's trun
			{
				check_farmer_is_good(game);
				int isup = game->player_type == UPFARMER_old;
				// DBG( PRINTF(LEVEL,"game firsthalf %d ： %s %s farmer\n",game->is_first_half_game,is_good_farmer?"good":"bad",isup?"up":"down"));
				if (first) //zhudong chupai
				{
					if (isup)
					{
						upfarmer_play_first(game, cur);
					}
					else  // down farmer
					{
						downfarmer_play_first(game, cur);
					}
					pass = 0;
					//game->players[CUR_PLAYER]->summary->combo_total_num -- ;
				}
				else  //beidong chupai
				{
					if (isup)
						pass = upframer_play(game, cur, pre);
					else
						pass = downframer_play(game, cur, pre);
				}
			}
		}

		//PRINTF(VERBOSE,"\n====       Current Hands      ====\n         ");//, UP_PLAYER );
		//log_brief =1;
		//log_brief=0;
		return pass;
	}

}

//just for debug
int check_poker_suit(POKERS*hand, PLAYER * player)
{
	//   return 1; //don't check
	POKERS t, *h = &t;
	memmove(h, hand, sizeof(POKERS));
	for (int k = 0; k<20; k++)
	{
		if (player->combos[k].type != NOTHING_old) {
			if (remove_combo_poker(h, &player->combos[k], NULL) == false)
				return 0;
		}
	}
	if (h->total == 0)
		return 1;
	else
		return 0;
}


int is_game_first_half(GAME_DDZ_AI* game, int player)
{
	if ((game->players[CUR_PLAYER]->oppUp_num <= 10 && game->players[UP_PLAYER]->id == LORD_old)
		|| (game->players[CUR_PLAYER]->oppDown_num <= 10 && game->players[DOWN_PLAYER]->id == LORD_old))
		return 0;
	else if (game->players[CUR_PLAYER]->oppDown_num <8 ||
		game->players[CUR_PLAYER]->oppUp_num <8)
		return 0;
	return 1;
}

//int test{}
void init_robot_pointer(LordRobot* robot)
{
	GAME_DDZ_AI *game = &robot->game;
	game->pot = &game->POT;
	{
		int j;
		for (j = 0; j<3; j++)
		{
			game->suits[j].h = &game->suits[j].p;
			game->suits[j].opps = &game->suits[j].opp;
			game->suits[j].combos = &game->suits[j].combos_store[0];
			game->suits[j].summary = &game->suits[j].combos_summary;
		}
	}
	game->players[0] = &game->suits[0];
	game->players[1] = &game->suits[1];
	game->players[2] = &game->suits[2];
	game->players[CUR_PLAYER]->use_quick_win_check = game->known_others_poker;
	game->pointer_init = 1;
}

void init_robot_setting(LordRobot* robot)
{
	GAME_DDZ_AI *game = &robot->game;
	memset(game, 0, sizeof(GAME_DDZ_AI));
	game->pot = &game->POT;
	{
		int j;
		for (j = 0; j<3; j++)
		{
			game->suits[j].h = &game->suits[j].p;
			game->suits[j].opps = &game->suits[j].opp;
			game->suits[j].combos = &game->suits[j].combos_store[0];
			game->suits[j].summary = &game->suits[j].combos_summary;
			game->suits[j].oppDown_num = 17;
			game->suits[j].oppUp_num = 17;
			game->suits[j].h->hun = -1;
			game->suits[j].opps->hun = -1;

		}
	}
	game->players[0] = &game->suits[0];
	game->players[1] = &game->suits[1];
	game->players[2] = &game->suits[2];
	game->players[CUR_PLAYER]->use_quick_win_check = game->known_others_poker;
}




//初始化机器人，返回机器人指针
LordRobot* createRobot(int type, int userid)
{
	LordRobot* tmp = (LordRobot*)malloc(sizeof(LordRobot));
	tmp->game.known_others_poker = type & 1;
	GAME_DDZ_AI* game = &tmp->game;
	game->use_best_for_undefined_case = type & 2 ? 1 : 0;
	game->delayPerGame = 100;
	//    logfile2 = NULL;
	//   logfile = NULL;
	/*
	if(userid!=0) {
	char filename[100];
	sprintf(filename,"/tmp/lordai/%d.log",userid);
	printf("log to file: %s\n",filename);

	logfile= fopen(filename,"wb");
	}*/
	//init something....
	memset(tmp->card, 0xFF, 3 * 21 * sizeof(int));
	memset(tmp->outs, 0xFF, 21 * sizeof(int));
	memset(tmp->pot, 0xFF, 4 * sizeof(int));
	memset(tmp->laizi, 0xFF, 5 * sizeof(int));
	init_robot_setting(tmp);
	return tmp;
}


void initCard(LordRobot* robot, int* robot_card, int* up_player_card, int* down_player_card)
{
#if 1
	//todo: save some code...
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;

	POKERS all;
	full_poker(&game->all);
	full_poker(&all);
	game->all.hun = -1;

	//get pot_card;
	int card_used[54] = { 0 };

	//fa pai
	//  if (!set->cus) //rand
	//TODO(检查指针合法性)
	{
		int i, k;
		for (k = 0, i = 0; k<17; k++, i++)
		{
			game->players[CUR_PLAYER]->card[k] = robot_card[i];
			card_used[robot_card[i]] = 1;
			if (robot_card[i]<52)
				game->players[CUR_PLAYER]->h->hands[robot_card[i] % 13]++;
			else if (robot_card[i]<54)
				game->players[CUR_PLAYER]->h->hands[robot_card[i] - 39]++;
			else
				PRINTF(LEVEL, "SUCks\n");

            if (up_player_card)
            {
			game->players[UP_PLAYER]->card[k] = up_player_card[i];
			card_used[up_player_card[i]] = 1;
			if (up_player_card[i]<52)
				game->players[UP_PLAYER]->h->hands[up_player_card[i] % 13]++;
			else if (up_player_card[i]<54)
				game->players[UP_PLAYER]->h->hands[up_player_card[i] - 39]++;
			else
				PRINTF(LEVEL, "SUCks\n");
            }

            if (down_player_card)
            {
			game->players[DOWN_PLAYER]->card[k] = down_player_card[i];
			card_used[down_player_card[i]] = 1;

			if (down_player_card[i]<52)
				game->players[DOWN_PLAYER]->h->hands[down_player_card[i] % 13]++;
			else  if (down_player_card[i]<54)
				game->players[DOWN_PLAYER]->h->hands[down_player_card[i] - 39]++;
			else
				PRINTF(LEVEL, "SUCks\n");
		}
		}
		for (k = 17; k<21; k++) {
			game->players[DOWN_PLAYER]->card[k] = -1;
			game->players[CUR_PLAYER]->card[k] = -1;
			game->players[UP_PLAYER]->card[k] = -1;
		}
	}


	sort_poker(game->players[2]->h);
	sort_poker(game->players[1]->h);
	sort_poker(game->players[0]->h);
	PRINTF(LEVEL, "\ncurrent PLAYER Hands: ");
	print_hand_poker(game->players[2]->h);
	print_card(game->players[2]->card);

	full_poker(&all);
	sub_poker(&all, game->players[CUR_PLAYER]->h, game->players[CUR_PLAYER]->opps);
	sort_poker(game->players[CUR_PLAYER]->opps);
	sub_poker(&all, game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->opps);
	sort_poker(game->players[UP_PLAYER]->opps);
	sub_poker(&all, game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->opps);
	sort_poker(game->players[DOWN_PLAYER]->opps);

    if (robot_card && up_player_card && down_player_card)
    {
        add_poker(game->players[DOWN_PLAYER]->h, game->players[CUR_PLAYER]->h, &game->all);
        add_poker(&game->all, game->players[UP_PLAYER]->h, &game->all);
        sub_poker(&all, &game->all, game->pot);
        sort_poker(game->pot);

        if (1)
        {
            int j = 0;
            for (int i = 0; i < 54; i++)
            {
                if (card_used[i] == 0)
                    game->pot_card[j++] = i;
                if (j > 3)
                {
                    if (game->pot->total != 3)
                        PRINTF_ALWAYS("line %d: FATAL ERROR in initCard\n", __LINE__);
                    //  fill_card_from_poker(game->pot_card, game->pot);
                    break;
                }
            }
            if (j != 3)
            {
                PRINTF(LEVEL, "line %d: FATAL ERROR in initCard\n", __LINE__);
                j = 3;
            }
            game->pot_card[j] = -1;
        }
    }
	/*
	int pot_itr=0;
	for(int i=P3; i<=BIG_JOKER; i++)
	{
	for(int j=0; j<game->pot->hands[i];j++)
	{
	game->pot_card[pot_itr++] = int_2_poker(i);
	if(pot_itr>=3)
	break;
	}

	}

	*/

	full_poker(&game->all);
	game->players[0]->cur = NULL;
	game->players[1]->cur = NULL;
	//    log_brief =0;

	game->rec.first_caller = -1;
	game->rec.p[CUR_PLAYER] = *game->players[CUR_PLAYER]->h;
	game->rec.pot = *game->pot;
	game->rec.p[DOWN_PLAYER] = *game->players[DOWN_PLAYER]->h;
	game->rec.p[UP_PLAYER] = *game->players[UP_PLAYER]->h;

    for (int i = 0; i < 3; ++i)
    {
        game->players[i]->lastPassSingle = BIG_JOKER;
    }

	game->hun = -1;
	return;
#endif
}

void setLaizi(LordRobot* robot, int laizi)
{

	GAME_DDZ_AI *game = &robot->game;
	{
		int j;
		for (j = 0; j<3; j++)
		{
			game->suits[j].h->hun = laizi;
			game->suits[j].opps->hun = laizi;
			game->rec.p[j].hun = laizi;
		}
	}
	game->hun = laizi;
	game->all.hun = laizi;

}

//传出参数为本机器人叫分
//-1 表示不叫
int RobotcallScore(LordRobot* robot)
{
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;
	PLAYER* pl = game->players[CUR_PLAYER];
	if (game->rec.first_caller == -1)
		game->rec.first_caller = CUR_PLAYER;
	//check ourselves
	if (game->known_others_poker)
	{
		int score = 0;


		LordRobot tR;
		Robot_copy(&tR, robot);
		beLord(&tR, tR.game.pot_card, CUR_PLAYER);
		int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
		if (winner)
		{
			PRINTF(LEVEL, "robot %x could win!!\n", robot);
			score = 3;
		}
		else
		{
			PRINTF(LEVEL, "robot %x would lose!!\n", robot);

			if (game->players[DOWN_PLAYER]->computer == 0
				&& game->players[UP_PLAYER]->computer == 1
				&& game->players[DOWN_PLAYER]->score == 0 /*Un called*/)
			{
				int score;
				//try be lord...
				int control = calc_controls(game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->opps, 6);
				if (control > 20)  //qiang dizhu!
					score = 3;
				if (control>10)
				{
					search_combos_in_suit(game, game->players[DOWN_PLAYER]->h,
						game->players[DOWN_PLAYER]->opps, game->players[DOWN_PLAYER]);
					if (CONTROL_SUB_COMBO_NUM(game->players[DOWN_PLAYER])>-30)
					{
						score = 3;
					}
				}
				PRINTF(LEVEL, "robot %x would lose, but should be lord since player will win!!\n", robot);
			}
			else  if (game->players[UP_PLAYER]->computer == 0
				&& game->players[DOWN_PLAYER]->computer == 1
				&& game->players[UP_PLAYER]->score == 0 /*Un called*/)
			{
				//try be lord...
				int control = calc_controls(game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->opps, 6);
				if (control > 20)  //qiang dizhu!
				{
					add_poker(game->players[DOWN_PLAYER]->h, game->pot, game->players[DOWN_PLAYER]->h);
					search_combos_in_suit(game, game->players[DOWN_PLAYER]->h,
						game->players[DOWN_PLAYER]->opps, game->players[DOWN_PLAYER]);
					if (CONTROL_SUB_COMBO_NUM(game->players[DOWN_PLAYER]) + 20 <=
						CONTROL_SUB_COMBO_NUM(tR.game.players[CUR_PLAYER]))
					{
						score = 3;
					}
				}
				PRINTF(LEVEL, "robot %x would lose, but should be lord since player will win!!\n", robot);
				//return 3;
			}
		}
		game->rec.score[(int)CUR_PLAYER] = score;

		game->call.player[game->call.cur_num] = CUR_PLAYER;
		game->call.score[game->call.cur_num] = score;
		game->call.cur_num++;

		if (score == 3)
		{
			game->call.cur_lord = CUR_PLAYER;
			return score;
		}
		else
			return -1;

	}

	//check human players...
	//

	//    GAME_DDZ_AI *game=&robot->game;
	int control = calc_controls(game->players[2]->h, game->players[2]->opps, 6);
	int score = (control>10) ? 3 : -1;
	game->rec.score[(int)CUR_PLAYER] = score;
	game->call.player[game->call.cur_num] = CUR_PLAYER;
	game->call.score[game->call.cur_num] = score;
	game->call.cur_num++;
	return score;
}

//be lord
/* 5、其他玩家叫分信息：
Player_number, 0: down player,
1: up player.
2: cur player
*/
void beLord(LordRobot* robot, int* robot_card, int player_number)
{
	init_robot_pointer(robot);
	robot->game.rec.lord = player_number;

	if (player_number == 2)
	{
		//printf("robot is lord!\n");
		GAME_DDZ_AI *game = &robot->game;

		for (int k = 17, i = 0; k<20; k++, i++)
		{
			game->players[CUR_PLAYER]->card[k] = robot_card[i];
			if (robot_card[i]<52)
				game->players[CUR_PLAYER]->h->hands[robot_card[i] % 13]++;
			else
				game->players[CUR_PLAYER]->h->hands[robot_card[i] - 39]++;
		}
		game->players[CUR_PLAYER]->card[20] = -1;
		sort_poker(game->players[CUR_PLAYER]->h);
		print_hand_poker(game->players[CUR_PLAYER]->h);
		POKERS all;
		full_poker(&all);
		sub_poker(&all, game->players[CUR_PLAYER]->h, game->players[CUR_PLAYER]->opps);
		sort_poker(game->players[CUR_PLAYER]->opps);

		//  game->lord=3;
		game->players[CUR_PLAYER]->id = LORD_old;
		game->players[DOWN_PLAYER]->id = DOWNFARMER_old;
		game->players[UP_PLAYER]->id = UPFARMER_old;
		game->player_type = game->players[CUR_PLAYER]->id;
		if (game->init == 0)
		{
			game_init(game);
			game->init = 1;
		}
		game->rec.type[CUR_PLAYER] = game->players[CUR_PLAYER]->id;
		game->rec.type[DOWN_PLAYER] = game->players[DOWN_PLAYER]->id;
		game->rec.type[UP_PLAYER] = game->players[UP_PLAYER]->id;
		return;
	}
	GAME_DDZ_AI *game = &robot->game;
	//game->player[player_number].type = LORD_old;
	if (player_number == UP_PLAYER)
	{
		game->players[CUR_PLAYER]->id = DOWNFARMER_old;//robot
		game->players[DOWN_PLAYER]->id = UPFARMER_old;
		game->players[UP_PLAYER]->id = LORD_old;
		game->players[CUR_PLAYER]->oppUp_num = 20;
        game->players[DOWN_PLAYER]->oppDown_num = 20;
		add_poker(game->players[UP_PLAYER]->h, game->pot, game->players[UP_PLAYER]->h);
		sort_poker(game->players[UP_PLAYER]->h);
		sub_poker(game->players[UP_PLAYER]->opps, game->pot, game->players[UP_PLAYER]->opps);
		sort_poker(game->players[UP_PLAYER]->opps);

		for (int k = 17, i = 0; k<20; k++, i++)
		{
			game->players[UP_PLAYER]->card[k] = robot_card[i];
		}

	}
	else if (player_number == DOWN_PLAYER)
	{
		game->players[DOWN_PLAYER]->id = LORD_old;
		game->players[UP_PLAYER]->id = DOWNFARMER_old;
		game->players[CUR_PLAYER]->id = UPFARMER_old;
		game->players[CUR_PLAYER]->oppDown_num = 20;
        game->players[UP_PLAYER]->oppUp_num = 20;
		add_poker(game->players[DOWN_PLAYER]->h, game->pot, game->players[DOWN_PLAYER]->h);
		sort_poker(game->players[DOWN_PLAYER]->h);
		sub_poker(game->players[DOWN_PLAYER]->opps, game->pot, game->players[DOWN_PLAYER]->opps);
		sort_poker(game->players[DOWN_PLAYER]->opps);
		for (int k = 17, i = 0; k<20; k++, i++)
		{
			game->players[DOWN_PLAYER]->card[k] = robot_card[i];
		}
	}
	game->player_type = game->players[CUR_PLAYER]->id;
	//  game->lord=player_number+1;
	if (game->init == 0)
	{
		game_init(game);
		game->init = 1;
	}
	game->rec.type[CUR_PLAYER] = game->players[CUR_PLAYER]->id;
	game->rec.type[DOWN_PLAYER] = game->players[DOWN_PLAYER]->id;
	game->rec.type[UP_PLAYER] = game->players[UP_PLAYER]->id;

}


void getPlayerTakeOutCards_hun(LordRobot* robot, int* poker, int * aim_poker, int player_number)
{
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;
	game->players[player_number]->cur = &game->players[player_number]->curcomb;
	if (poker[0] != -1)// || aim_poker[0]!= -1)
	{

		POKERS a, b, c;
		int i, tmp_card[26];
		int num = read_poker_int(poker, &a);
		int hun_num = read_poker_int(aim_poker, &b);

		if (hun_num != a.hands[game->hun])
		{
			PRINTF_ALWAYS("[%s][%d]: godie! \n", __FUNCTION__, __LINE__);
		}
		a.hands[game->hun] = 0;

		if (hun_num)
		{
			add_poker(&a, &b, &c);
			if (!(i = is_combo(&c, game->players[player_number]->cur)))
			{
				PRINTF_ALWAYS("godie! error pokers from other poker\n");
			}
			memset(&b, 0, sizeof(POKERS));
			b.begin = b.end = game->hun;
			b.hands[b.begin] = hun_num;

			add_poker(&a, &b, &c);
			if (game->players[player_number]->cur->type == BOMB_old)
			{
				game->players[player_number]->cur->three_desc[0] = hun_num == 4 ? 2 : 0;
			}
		}
		else
		{
			c = a;
			if (!(i = is_combo(&a, game->players[player_number]->cur)))
			{
				PRINTF_ALWAYS("godie! error pokers from other poker\n");
			}
			if (game->players[player_number]->cur->type == BOMB_old)
			{
				game->players[player_number]->cur->three_desc[0] = 1;
			}
		}

		sub_poker(game->players[CUR_PLAYER]->opps, &c,
			game->players[CUR_PLAYER]->opps);
		sub_poker(&game->all, &c, &game->all);

		memmove(&game->prepre, &game->pre, sizeof(COMBO_OF_POKERS));
		memmove(&game->pre, game->players[player_number]->cur, sizeof(COMBO_OF_POKERS));
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = player_number;

		PRINTF(VERBOSE, "itr@game[%x] %d\n", game, game->rec.now);
		PRINTF(LEVEL, "from %d:  ", player_number);
		print_combo_poker(&game->pre);
		game->rec.history[game->rec.now].player = (PLAYER_POS)player_number;
		game->rec.history[game->rec.now++].h = game->pre;
		if (game->pre.type == BOMB_old || game->pre.type == ROCKET)
		{
			game->rec.bombed++;
		}

		if (player_number == UP_PLAYER)
		{
			sub_poker(game->players[UP_PLAYER]->h, &c, game->players[UP_PLAYER]->h);
			// remove card 
			remove_poker_in_int_array(&c,
				tmp_card, game->players[UP_PLAYER]->card, game->hun);
			check_card(game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->card);
			game->players[CUR_PLAYER]->oppUp_num -= c.total;
			sub_poker(game->players[DOWN_PLAYER]->opps, &c, game->players[DOWN_PLAYER]->opps);
			game->players[DOWN_PLAYER]->oppDown_num -= c.total;
		}
		else if (player_number == DOWN_PLAYER)
		{
			sub_poker(game->players[DOWN_PLAYER]->h, &c, game->players[DOWN_PLAYER]->h);
			// remove card 
			remove_poker_in_int_array(&c,
				tmp_card, game->players[DOWN_PLAYER]->card, game->hun);
			check_card(game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->card);
			game->players[CUR_PLAYER]->oppDown_num -= c.total;
			sub_poker(game->players[UP_PLAYER]->opps, &c, game->players[UP_PLAYER]->opps);
			game->players[UP_PLAYER]->oppUp_num -= c.total;
		}

		if (!i)
			PRINTF_ALWAYS("fatal erro line %d!\n", __LINE__);

	}
	else
	{
		PRINTF(VERBOSE, "itr@game[%x] %d PASS\n", game, game->rec.now);
		game->players[player_number]->cur = NULL;
		game->rec.history[game->rec.now].player = (PLAYER_POS)player_number;
		game->rec.history[game->rec.now++].h.type = NOTHING_old;
	}

}




void getPlayerTakeOutCards_hun_internal(LordRobot* robot, int* poker, int * aim_poker, int player_number)
{
	GAME_DDZ_AI *game = &robot->game;
	game->players[player_number]->cur = &game->players[player_number]->curcomb;
	if (poker[0] != -1)// || aim_poker[0]!= -1)
	{

		POKERS a, b, c;
		int i, tmp_card[26];
		int num = read_poker_int(poker, &a);
		int hun_num = read_poker_int(aim_poker, &b);

		if (hun_num != a.hands[game->hun])
		{
			PRINTF_ALWAYS("[%s][%d]: godie! \n", __FUNCTION__, __LINE__);
		}
		a.hands[game->hun] = 0;

		if (hun_num)
		{
			add_poker(&a, &b, &c);
			if (!(i = is_combo(&c, game->players[player_number]->cur)))
			{
				PRINTF_ALWAYS("godie! error pokers from other poker\n");
			}
			memset(&b, 0, sizeof(POKERS));
			b.begin = b.end = game->hun;
			b.hands[b.begin] = hun_num;

			add_poker(&a, &b, &c);
			if (game->players[player_number]->cur->type == BOMB_old)
			{
				game->players[player_number]->cur->three_desc[0] = hun_num == 4 ? 2 : 0;
			}
		}
		else
		{
			c = a;
			if (!(i = is_combo(&a, game->players[player_number]->cur)))
			{
				PRINTF_ALWAYS("godie! error pokers from other poker\n");
			}
			if (game->players[player_number]->cur->type == BOMB_old)
			{
				game->players[player_number]->cur->three_desc[0] = 1;
			}
		}

		sub_poker(game->players[CUR_PLAYER]->opps, &c,
			game->players[CUR_PLAYER]->opps);
		sub_poker(&game->all, &c, &game->all);

		memmove(&game->prepre, &game->pre, sizeof(COMBO_OF_POKERS));
		memmove(&game->pre, game->players[player_number]->cur, sizeof(COMBO_OF_POKERS));
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = player_number;

		PRINTF(VERBOSE, "itr@game[%x] %d\n", game, game->rec.now);
		//      game->rec.history[game->rec.now].player= (PLAYER_POS)player_number;
		//      game->rec.history[game->rec.now++].h= game->pre;
		//      if(game->pre.type == BOMB_old ||game->pre.type == ROCKET)
		//      {
		//          game->rec.bombed++;
		//      }

		if (player_number == UP_PLAYER)
		{
			sub_poker(game->players[UP_PLAYER]->h, &c, game->players[UP_PLAYER]->h);
			// remove card 
			remove_poker_in_int_array(&c,
				tmp_card, game->players[UP_PLAYER]->card, game->hun);
			check_card(game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->card);
			game->players[CUR_PLAYER]->oppUp_num -= c.total;
			sub_poker(game->players[DOWN_PLAYER]->opps, &c, game->players[DOWN_PLAYER]->opps);
			game->players[DOWN_PLAYER]->oppDown_num -= c.total;
		}
		else if (player_number == DOWN_PLAYER)
		{
			sub_poker(game->players[DOWN_PLAYER]->h, &c, game->players[DOWN_PLAYER]->h);
			// remove card 
			remove_poker_in_int_array(&c,
				tmp_card, game->players[DOWN_PLAYER]->card, game->hun);
			check_card(game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->card);
			game->players[CUR_PLAYER]->oppDown_num -= c.total;
			sub_poker(game->players[UP_PLAYER]->opps, &c, game->players[UP_PLAYER]->opps);
			game->players[UP_PLAYER]->oppUp_num -= c.total;
		}

		if (!i)
			PRINTF_ALWAYS("fatal erro line %d!\n", __LINE__);

	}
	else
	{
		PRINTF(VERBOSE, "itr@game[%x] %d PASS\n", game, game->rec.now);
		game->players[player_number]->cur = NULL;
		//      game->rec.history[game->rec.now].player= (PLAYER_POS)player_number;
		//  game->rec.history[game->rec.now++].h.type= NOTHING_old;
	}

}



/*其他玩家出牌信息：
Player_number, 0: down player,
1: up  player.
参数同1、发牌，如果其他玩家未出牌，则传入null
*/
void getPlayerTakeOutCards(LordRobot* robot, int* poker, int * laizi, int player_number)
{
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;

	if (game->hun != -1)
	{
		return getPlayerTakeOutCards_hun(robot, poker, laizi, player_number);
	}

	game->players[player_number]->cur = &game->players[player_number]->curcomb;
	if (poker[0] != -1)
	{
        if (player_number == CUR_PLAYER)
        {
            PLAYER* pl = game->players[CUR_PLAYER];
            PLAYER* downPl = game->players[DOWN_PLAYER];
            PLAYER* upPl = game->players[UP_PLAYER];

            //update pokers..
            //int number = get_combo_number(cur);
            int number = is_input_combo(poker, pl->cur);
            remove_combo_poker(pl->h, pl->cur, NULL);
            remove_combo_poker(upPl->opps, pl->cur, NULL);
            upPl->oppDown_num -= number;
            remove_combo_poker(downPl->opps, pl->cur, NULL);
            downPl->oppUp_num -= number;

            //combo_to_int_array(pl->cur, poker, game->players[CUR_PLAYER]->card);
            for (int i = 0; i < max_POKER_NUM; ++i)
            {
                if (poker[i] == -1)
                {
                    break;
                }

                for (int j = 0; j < max_POKER_NUM; ++j)
                {
                    if (game->players[CUR_PLAYER]->card[j] == poker[i])
                    {
                        game->players[CUR_PLAYER]->card[j] = -1;
                    }
                }
            }

            remove_combo_poker(&game->all, pl->cur, NULL);
            *laizi = -1;


            PRINTF(VERBOSE, "game[%x] ", game);
            print_card_2(game->players[CUR_PLAYER]->card, 21);
            memmove(&game->prepre, &game->pre, sizeof(COMBO_OF_POKERS));
            memmove(&game->pre, game->players[player_number]->cur, sizeof(COMBO_OF_POKERS));
            game->prepre_playernum = game->pre_playernum;
            game->pre_playernum = CUR_PLAYER;


            PRINTF(VERBOSE, "itr@game[%x] own %d\n", game, game->rec.now);
            game->rec.history[game->rec.now].player = CUR_PLAYER;
            game->rec.history[game->rec.now++].h = *game->players[CUR_PLAYER]->cur;
            if (game->pre.type == BOMB_old || game->pre.type == ROCKET)
            {
                game->rec.bombed++;
            }
            if (*poker == -1)
                PRINTF_ALWAYS("ERR:  you play what??\n");

            return;
        }

		int i = is_input_combo(poker, game->players[player_number]->cur);
		memmove(&game->prepre, &game->pre, sizeof(COMBO_OF_POKERS));
		memmove(&game->pre, game->players[player_number]->cur, sizeof(COMBO_OF_POKERS));
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = player_number;
		remove_combo_poker(game->players[CUR_PLAYER]->opps, &game->pre, NULL);

		remove_combo_poker(&game->all, &game->pre, NULL);
		//check this message
		//    if(game->rec.now>0)
		//{
		//    if( player_number!=((game->rec.history[game->rec.now-1].player +1)%3))
		//        {
		//             printf("strange!\n");
		//        //     while(1);
		//        }
		//}

		PRINTF(VERBOSE, "itr@game[%x] %d\n", game, game->rec.now);
		game->rec.history[game->rec.now].player = (PLAYER_POS)player_number;
		game->rec.history[game->rec.now].h = game->pre;

		game->rec.now++;
		if (game->pre.type == BOMB_old || game->pre.type == ROCKET)
		{
			game->rec.bombed++;
		}        

		if (player_number == UP_PLAYER)
		{
			remove_combo_poker(game->players[UP_PLAYER]->h, &game->pre, NULL);
			game->players[CUR_PLAYER]->oppUp_num -= i;
			remove_combo_poker(game->players[DOWN_PLAYER]->opps, &game->pre, NULL);
			game->players[DOWN_PLAYER]->oppDown_num -= i;

			//remove card...
			/*
			for(int i=0;i<21;i++)
			{
			if(poker[i]==-1) break;
			for(int j=0;j<21;j++)
			if( game->players[UP_PLAYER]->card[j]==poker[i])
			game->players[UP_PLAYER]->card[j] = -1;
			}
			*/
			//if( game->known_others_poker) //check others
			{
				//            search_combos_in_suit(game,game->players[UP_PLAYER]->h,game->players[UP_PLAYER]->opps,game->players[UP_PLAYER]);
			}
		}
		else if (player_number == DOWN_PLAYER)
		{
			remove_combo_poker(game->players[DOWN_PLAYER]->h, &game->pre, NULL);
			game->players[CUR_PLAYER]->oppDown_num -= i;
			remove_combo_poker(game->players[UP_PLAYER]->opps, &game->pre, NULL);
			game->players[UP_PLAYER]->oppUp_num -= i;
		}

		game->players[CUR_PLAYER]->first_hint = 1;

		if (!i)
			PRINTF_ALWAYS("fatal erro line %d!\n", __LINE__);



	}
	else
	{
		PRINTF(VERBOSE, "itr@game[%x] %d PASS\n", game, game->rec.now);
		game->players[player_number]->cur = NULL;

        if (game->pre.type == SINGLE)
        {
            if (game->players[player_number]->id == UPFARMER_old)
            {
                game->players[(player_number + 2) % 3]->lastPassSingle = game->pre.low;
            }
            if (game->players[player_number]->id == DOWNFARMER_old)
            {
                game->players[(player_number + 1) % 3]->lastPassSingle = game->pre.low;
            }
        }

		//check this message
		//    if(game->rec.now>0)
		//{
		//    if( player_number!=((game->rec.history[game->rec.now-1].player +1)%3))
		//        {
		//             printf("strange!\n");
		//        //     while(1);
		//        }
		//}

		game->rec.history[game->rec.now].player = (PLAYER_POS)player_number;
		game->rec.history[game->rec.now++].h.type = NOTHING_old;
	}

}





/*其他玩家出牌信息：
Player_number, 0: down player,
1: up  player.
参数同1、发牌，如果其他玩家未出牌，则传入null
*/
void getPlayerTakeOutCards_internal(LordRobot* robot, int* poker, int * laizi, int player_number)
{
	GAME_DDZ_AI *game = &robot->game;

	if (game->hun != -1)
	{
		return getPlayerTakeOutCards_hun_internal(robot, poker, laizi, player_number);
	}

	game->players[player_number]->cur = &game->players[player_number]->curcomb;
	if (poker[0] != -1)
	{
		int i = is_input_combo(poker, game->players[player_number]->cur);
		memmove(&game->prepre, &game->pre, sizeof(COMBO_OF_POKERS));
		memmove(&game->pre, game->players[player_number]->cur, sizeof(COMBO_OF_POKERS));
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = player_number;
		remove_combo_poker(game->players[CUR_PLAYER]->opps, &game->pre, NULL);

		remove_combo_poker(&game->all, &game->pre, NULL);

		PRINTF(VERBOSE, "itr@game[%x] %d\n", game, game->rec.now);
		/*      game->rec.history[game->rec.now].player= (PLAYER_POS)player_number;
		game->rec.history[game->rec.now].h= game->pre;

		game->rec.now++;
		if(game->pre.type == BOMB_old ||game->pre.type == ROCKET)
		{
		game->rec.bombed++;
		}
		*/
		if (player_number == UP_PLAYER)
		{
			remove_combo_poker(game->players[UP_PLAYER]->h, &game->pre, NULL);
			game->players[CUR_PLAYER]->oppUp_num -= i;
			remove_combo_poker(game->players[DOWN_PLAYER]->opps, &game->pre, NULL);
			game->players[DOWN_PLAYER]->oppDown_num -= i;

		}
		else if (player_number == DOWN_PLAYER)
		{
			remove_combo_poker(game->players[DOWN_PLAYER]->h, &game->pre, NULL);
			game->players[CUR_PLAYER]->oppDown_num -= i;
			remove_combo_poker(game->players[UP_PLAYER]->opps, &game->pre, NULL);
			game->players[UP_PLAYER]->oppUp_num -= i;
		}

		game->players[CUR_PLAYER]->first_hint = 1;

		if (!i)
			PRINTF_ALWAYS("fatal erro line %d!\n", __LINE__);



	}
	else
	{
		PRINTF(VERBOSE, "itr@game[%x] %d PASS\n", game, game->rec.now);
		game->players[player_number]->cur = NULL;
		//      game->rec.history[game->rec.now].player= (PLAYER_POS)player_number;
		//      game->rec.history[game->rec.now++].h.type= NOTHING_old;
	}

}



/* 5、其他玩家叫分信息：
Player_number, 0: up player,
1: down player.
score,  分值, -1表示不叫
*/
void getPlayerCallScore(LordRobot* robot, int score, int player_number)
{
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;
	game->players[player_number]->score = score;
	game->rec.score[player_number] = score;
	if (game->rec.first_caller == -1)
		game->rec.first_caller = player_number;
	if (game->call.cur_num>2)
		game->call.cur_num = 2;
	game->call.player[game->call.cur_num] = player_number;
	game->call.score[game->call.cur_num] = score;
	game->call.cur_num++;
	if (score == 4) //doubled game
	{
		game->call.double_times++;
	}
	if (score != -1)
		game->call.cur_lord = player_number;
}

int  doubleGame(LordRobot* robot)
{
	GAME_DDZ_AI *game = &robot->game;
	PLAYER * pl = game->players[CUR_PLAYER];
	int lord = game->call.cur_lord;
	if (1)//new algorthim 
	{
		search_combos_in_suit(game, pl->h, pl->opps, pl);
		if ((pl->summary->bomb_num >= 2 && (pl->h->hands[LIT_JOKER] + pl->h->hands[BIG_JOKER] >= 1))
			|| (pl->summary->bomb_num >= 1 && (pl->summary->combo_total_num <= 2))
			|| (pl->summary->bomb_num >= 3)
			)
		{
			return 1;
		}
		return 0;
	}
	if (0)//(game->players[lord]->computer)
	{
		if (lord == CUR_PLAYER) //double check
		{
			return 1;
		}
		else
		{  // 50% 概率 加倍
			return rand() % 2;
		}
	}
	else
	{
		int player_pos = lord;
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			PRINTF(LEVEL, "=============START DOUBLEGAME %d=============\n", player_pos);
			if (player_pos == DOWN_PLAYER)
			{
				return 0;
				POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
				POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
				*tR.game.players[CUR_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
				*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
				*tR.game.players[DOWN_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
				*tR.game.players[DOWN_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
				*tR.game.players[UP_PLAYER]->h = tmph;
				*tR.game.players[UP_PLAYER]->opps = tmpopps;
			}
			else
				if (player_pos == UP_PLAYER)
				{
					return 0;
					POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
					POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
					*tR.game.players[CUR_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
					*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
					*tR.game.players[UP_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
					*tR.game.players[UP_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
					*tR.game.players[DOWN_PLAYER]->h = tmph;
					*tR.game.players[DOWN_PLAYER]->opps = tmpopps;
				}
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (winner)
			{
				return 0;
			}
			else
			{
				return (rand() % 100)>75;
			}
		}
		else
			return 0;
	}
	return 0;
}

int RobotshowCard(LordRobot* robot)
{
	init_robot_pointer(robot);
	int ret = 0;
	GAME_DDZ_AI *game = &robot->game;
	//check if i'm lord
	{
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			PLAYER * pl = tR.game.players[CUR_PLAYER];
			search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
			int winner;
            if ((CONTROL_SUB_COMBO_NUM(pl) > -20 && (pl->summary->bomb_num > 0 || !opp_hasbomb(&tR.game)))
                || (CONTROL_SUB_COMBO_NUM(pl) > -10)
                || (HUN_NUMBER(tR.game.players[CUR_PLAYER]->h) >= 3)
                )
            {
                if (get_opp_bomb_number(&tR.game) - get_bomb_numbers(tR.game.players[CUR_PLAYER]->h) >= 2)
                {
                    winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
                }
                else
                {
                    winner = 1;
                }
            }
			else
				winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (winner)
			{
				PRINTF(LEVEL, "robot %x could win!!\n", robot);
				if (CONTROL_SUB_COMBO_NUM(pl) > 5 * (LORD_LEVEL_3 - 4))
					ret |= 0x3;
				else if (CONTROL_SUB_COMBO_NUM(pl) > 5 * (LORD_LEVEL_2 - 8))
					ret |= 0x2;
				else
				{
					ret |= 0x1;
				}
			}

		}
	}
	//check up_farmer
	if (0)
	{
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			PLAYER * pl = tR.game.players[CUR_PLAYER];
			PLAYER * dnpl = tR.game.players[DOWN_PLAYER];
			PLAYER * uppl = tR.game.players[UP_PLAYER];
			POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
			POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
			*tR.game.players[CUR_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
			*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
			*tR.game.players[DOWN_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
			*tR.game.players[DOWN_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
			*tR.game.players[UP_PLAYER]->h = tmph;
			*tR.game.players[UP_PLAYER]->opps = tmpopps;
			int tmpcard[21];
			memcpy(tmpcard, pl->card, sizeof(tmpcard));
			memcpy(pl->card, dnpl->card, sizeof(tmpcard));
			memcpy(dnpl->card, uppl->card, sizeof(tmpcard));
			memcpy(uppl->card, tmpcard, sizeof(tmpcard));
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (!winner)
			{
				PRINTF(LEVEL, "robot %x could win!!\n", robot);
				PLAYER * pl = tR.game.players[CUR_PLAYER];
				search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
				if (CONTROL_SUB_COMBO_NUM(pl) > 5 * (UPFARMER_LEVEL_3 - 6))
					ret |= (0x3 << 8);
				else if (CONTROL_SUB_COMBO_NUM(pl) >5 * (UPFARMER_LEVEL_2 - 10))
					ret |= (0x2 << 8);
				else
				{
					ret |= (1 << 8);
				}
			}
		}
	}
	//check down_farmer
	if (0)
	{
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			PLAYER * pl = tR.game.players[CUR_PLAYER];
			PLAYER * dnpl = tR.game.players[DOWN_PLAYER];
			PLAYER * uppl = tR.game.players[UP_PLAYER];
			POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
			POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
			*tR.game.players[CUR_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
			*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
			*tR.game.players[UP_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
			*tR.game.players[UP_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
			*tR.game.players[DOWN_PLAYER]->h = tmph;
			*tR.game.players[DOWN_PLAYER]->opps = tmpopps;
			int tmpcard[21];
			memcpy(tmpcard, pl->card, sizeof(tmpcard));
			memcpy(pl->card, uppl->card, sizeof(tmpcard));
			memcpy(uppl->card, dnpl->card, sizeof(tmpcard));
			memcpy(dnpl->card, tmpcard, sizeof(tmpcard));
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (!winner)
			{
				PRINTF(LEVEL, "robot %x could win!!\n", robot);
				PLAYER * pl = tR.game.players[CUR_PLAYER];
				search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
				if (CONTROL_SUB_COMBO_NUM(pl) > 5 * (DOWNFARMER_LEVEL_3 - 4))
					ret |= (0x3 << 16);
				else if (CONTROL_SUB_COMBO_NUM(pl) >5 * (DOWNFARMER_LEVEL_2 - 8))
					ret |= (0x2 << 16);
				else
				{
					ret |= (1 << 16);
				}
			}
		}
	}
	return ret;
}

int forceLord(LordRobot* robot)
{
	GAME_DDZ_AI *game = &robot->game;
	int pos = 1; //pos=1, first
	int up_done = 0;
	int down_done = 0;
	//int cur_lord =0;
	int lord = game->call.cur_lord;
	return 0;
	for (int i = 0; i<game->call.cur_num; i++)
	{
		if (game->call.score[i] == -1
			|| game->call.score[i] == 4) //不叫或者不抢
		{
			if (game->call.player[i] == UP_PLAYER)
			{
				up_done = 1;
			}
			if (game->call.player[i] == DOWN_PLAYER)
			{
				down_done = 1;
			}
		}
	}
	pos += up_done + down_done;
	//  if(pos==3) //最后一次抢地主
	{
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (winner)
			{
				PRINTF(LEVEL, "robot %x could win!!\n", robot);
				PLAYER * pl = tR.game.players[CUR_PLAYER];
				search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
				if (CONTROL_SUB_COMBO_NUM(pl) >0)
					return 1;
				else if (CONTROL_SUB_COMBO_NUM(pl) >-10)
					return 1;
				else
				{
					return rand() % 2;
				}
			}
			else
			{
				return 0;
			}
		}
		else
			return rand() % 2;
	}

	return 0;

	if (pos == 2 || pos == 1) //后面还有1 -2人可以抢地主
	{
		int other_player_type = 0; //computer or human
		int player_pos = UP_PLAYER;
		if (up_done)
		{
			player_pos = DOWN_PLAYER;
		}
		other_player_type = game->players[player_pos]->computer;
		if (game->known_others_poker)
		{
			int score = 0;
			LordRobot tR;
			Robot_copy(&tR, robot);
			PRINTF(LEVEL, "=============START LORD_old %d=============\n", player_pos);
			if (player_pos == DOWN_PLAYER)
			{
				POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
				POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
				*tR.game.players[CUR_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
				*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
				*tR.game.players[DOWN_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
				*tR.game.players[DOWN_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
				*tR.game.players[UP_PLAYER]->h = tmph;
				*tR.game.players[UP_PLAYER]->opps = tmpopps;
			}
			else if (player_pos == UP_PLAYER)
			{
				POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
				POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
				*tR.game.players[CUR_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
				*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
				*tR.game.players[UP_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
				*tR.game.players[UP_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
				*tR.game.players[DOWN_PLAYER]->h = tmph;
				*tR.game.players[DOWN_PLAYER]->opps = tmpopps;
			}
			beLord(&tR, tR.game.pot_card, CUR_PLAYER);
			int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
			if (winner)
			{
				if (other_player_type)
				{
					PLAYER * pl = tR.game.players[CUR_PLAYER];
					search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
					if (CONTROL_SUB_COMBO_NUM(pl) >0)
						return 1;
				}
				else
					return 0;
			}

			if (pos == 1) //check down player as well
			{
				LordRobot tR;
				Robot_copy(&tR, robot);
				POKERS tmph = *tR.game.players[CUR_PLAYER]->h;
				POKERS tmpopps = *tR.game.players[CUR_PLAYER]->opps;
				*tR.game.players[CUR_PLAYER]->h = *tR.game.players[DOWN_PLAYER]->h;
				*tR.game.players[CUR_PLAYER]->opps = *tR.game.players[DOWN_PLAYER]->opps;
				*tR.game.players[DOWN_PLAYER]->h = *tR.game.players[UP_PLAYER]->h;
				*tR.game.players[DOWN_PLAYER]->opps = *tR.game.players[UP_PLAYER]->opps;
				*tR.game.players[UP_PLAYER]->h = tmph;
				*tR.game.players[UP_PLAYER]->opps = tmpopps;
				beLord(&tR, tR.game.pot_card, CUR_PLAYER);
				int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
				if (winner)
				{
					return 0;
				}
			}

			//check self
			{
				LordRobot tR;
				Robot_copy(&tR, robot);
				beLord(&tR, tR.game.pot_card, CUR_PLAYER);
				int winner = Check_win_quick(&tR.game, CUR_PLAYER, 0, 0, 0);
				if (winner)
				{
					PRINTF(LEVEL, "robot %x could win!!\n", robot);
					PLAYER * pl = tR.game.players[CUR_PLAYER];
					search_combos_in_suit(&tR.game, pl->h, pl->opps, pl);
					if (CONTROL_SUB_COMBO_NUM(pl) >0)
						return 1;
					else if (CONTROL_SUB_COMBO_NUM(pl) >-10)
						return rand() % 2;
					else
					{
						if (game->players[lord]->computer)
							return 0;
						return (rand() % 100)>50;
					}
				}
				else
				{
					return 0;
				}
			}

		}
		else
			return 0;
	}

	return 0;
}


static int check_feiji_rules(COMBO_OF_POKERS * p)
{
	if (p->type >= 3311 || p->type == 531) //no such case!
	{
		POKERS tmp;
		combo_2_poker(&tmp, p);
		int att = 2;
		if (p->type == 3322 || p->type == 333222 || p->type == 33332222)
			att = 1;
		for (int i = tmp.begin; i <= tmp.end; i++)
		{
			if (tmp.hands[i] == att || tmp.hands[i] == 4)
				return 0;
		}
	}
	return 1;

}


static int check_laizi_rules(COMBO_OF_POKERS * cur,
	POKERS *h,
	int first,
	COMBO_OF_POKERS * pre)
{
	{
		COMBO_OF_POKERS * p = cur;
		if (p->type == THREE_ONE && p->three_desc[0] == p->low)
		{
			if (!first)
				return 0;
			p->type = SINGLE; //rare case                   
		}


		if (h->hands[p->low] == 0 && (
			p->type == PAIRS || p->type == SINGLE || p->type == THREE
			|| p->type == THREE_ONE))
		{
			p->low = h->hun;
			if (first || p->low > pre->low)
				return 1;
			else
				return 0;
		}
		else if (p->type == THREE_SERIES)  //no such case!
		{
			for (int i = p->low; i<p->low + p->len; i++)
			{
				if (h->hands[i] == 0) {
					if (!first)
						return 0;
					if (i == p->low)
						p->low++;
					p->type = THREE;
					return 1;
				}
			}
		}
		else if (p->type >= 3311 || p->type == 531) //no such case!
		{

			for (int i = p->low; i<p->low + p->len; i++)
			{
				if (h->hands[i] == 0) {
					if (!first)
						return 0;
					if (i == p->low)
						p->low++;
					p->type = THREE;
					return 1;
				}
			}

			for (int i = 0; i<p->len; i++)
			{
				int val = p->three_desc[i];
				if (h->hands[val] == 0)
					p->three_desc[i] = h->hun;
			}

			for (int i = 0; i<p->len; i++)
			{
				int val = p->three_desc[i];
				if (val >= p->low && val<p->low + p->len)
				{
					if (!first)
						return 0;
					p->low = val;
					p->type = THREE;
					return 1;
				}
			}

		}
	}
	return 1;

}

/*6、出牌：传出参数同[发牌]的传入参数，如果机器人不出牌，则传出null*/
int takeOut(LordRobot* robot, int * p, int* laizi, int * delay/*=NULL*/)
{
	init_robot_pointer(robot);
	GAME_DDZ_AI *game = &robot->game;
	COMBO_OF_POKERS *cur, combo;
	PLAYER * pl = game->players[CUR_PLAYER];
	PLAYER * downPl = game->players[DOWN_PLAYER];
	PLAYER * upPl = game->players[UP_PLAYER];

	//yz 兼容不带delay参数的策略
	int localDelay = 0;
	if (!delay)
	{
		delay = &localDelay;
	}

	*delay = 0;
	cur = &combo;
	//  game->cur_playernum = 2;

	static int turn = 0;
	DBG(
		PRINTF(LEVEL, "==START_OF_ROBOT_PLAY TURN %d==\n", turn);
	)
		int pass = 1, first = (game->players[UP_PLAYER]->cur == NULL && game->players[DOWN_PLAYER]->cur == NULL);

	//reset to NULL
	game->players[UP_PLAYER]->cur = NULL;
	game->players[DOWN_PLAYER]->cur = NULL;
	search_combos_in_suit(game, game->players[CUR_PLAYER]->h, game->players[CUR_PLAYER]->opps, game->players[CUR_PLAYER]);
	if (game->known_others_poker)
	{
		search_combos_in_suit(game, game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->opps, game->players[DOWN_PLAYER]);
		search_combos_in_suit(game, game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->opps, game->players[UP_PLAYER]);
	}

	update_players(game, NULL);

	if (pl->h->total >0) //error check
	{
		if (first)
		{   //主动出牌
			//printf("zhudong chupai!\n");

			if (pl->id == LORD_old && pl->h->total == 20) //first
			{
				*delay = 3000 + rand() % 5000;
			}


			pass = robot_play(robot, cur, 1);
			pass = 0;
		}
		else //被动出牌
		{
			//printf("beidong chupai!\n");
			pass = robot_play(robot, cur, 0);

            // 地主剩王炸就走了时，农民还炸的情况
            if (pl->id != LORD_old && cur->type == BOMB_old && pl->h->total != 4)
            {
                PLAYER *lord = NULL;
                if (pl->id == UPFARMER_old)
                {
                    lord = game->players[DOWN_PLAYER];
                }
                else if (pl->id == DOWNFARMER_old)
                {
                    lord = game->players[UP_PLAYER];
                }

                // TODO： 考虑，可能在选择出牌时有能管上的牌却出了炸弹，最后在这里被置空的情况，需要在必管或其他局面下另作处理
                if (lord) // 只剩大小王（+1单/对）
                {
                    if (lord->summary->bomb_num == 1 
                        && lord->summary->bomb[0]->type == ROCKET
                        && (lord->h->total == 2 || lord->h->total == 3 || (lord->h->total == 4 && lord->summary->pairs_num == 1)))
                    {
                        cur->type = NOTHING_old; // 不炸
                    }
                }
            }
		}
	}
	else
	{
		PRINTF_ALWAYS("fatal error in line %d:no cards.\n", __LINE__);
	}

    if (!pass && (cur->type == THREE || cur->type == THREE_ONE || cur->type == THREE_TWO))
    {
        search_combos_in_suit(game, pl->h, pl->opps, pl);
    }

    if (!pass)
    {
        if (cur->type == THREE)
        {
            bool find_kicker_in_31 = false;
            bool find_kicker_in_32 = false;
            if (pl->summary->three_one_num > 0 && pl->summary->three_two_num > 0)
            {
                if (pl->summary->three_one[0]->three_desc[0] <= pl->summary->three_two[0]->three_desc[0])
                {
                    // single
                    find_kicker_in_31 = true;
                }
                else
                {
                    //pair;
                    find_kicker_in_32 = true;
                }
            }
            else if (pl->summary->three_one_num > 0)
            {
                find_kicker_in_31 = true;
            }
            else if (pl->summary->three_two_num > 0)
            {
                find_kicker_in_32 = true;
            }

            if (find_kicker_in_31 
                && (cur->control == 1 || (pl->id == LORD_old && pl->oppDown_num != 4 && pl->oppUp_num != 4) 
                                      || (pl->id == DOWNFARMER_old && pl->oppUp_num != 4) 
                                      || (pl->id == UPFARMER_old && pl->oppDown_num != 4))) // 冲锋套或者对手剩牌不为4（3+1）张
            {
                cur->type = THREE_ONE;
                cur->three_desc[0] = pl->summary->three_one[0]->three_desc[0];

                pl->summary->three_one[0]->type = THREE;
                pl->summary->three_one[0]->three_desc[0] = 0;
                memmove(&pl->summary->three_one[0], &pl->summary->three_one[1], sizeof(void*) * (pl->summary->three_one_num));
                pl->summary->three_one_num--;
            }
            else if (find_kicker_in_32 
                && (cur->control == 1 || (pl->id == LORD_old && pl->oppDown_num != 5 && pl->oppUp_num != 5)
                                      || (pl->id == DOWNFARMER_old && pl->oppUp_num != 5)
                                      || (pl->id == UPFARMER_old && pl->oppDown_num != 5))) // 冲锋套或者对手剩牌不为5（3+2）张
            {
                cur->type = THREE_TWO;
                cur->three_desc[0] = pl->summary->three_two[0]->three_desc[0];

                pl->summary->three_two[0]->type = THREE;
                pl->summary->three_two[0]->three_desc[0] = 0;
                memmove(&pl->summary->three_two[0], &pl->summary->three_two[1], sizeof(void*) * (pl->summary->three_two_num));
                pl->summary->three_two_num--;
            }
        }
    }

    // 三带一/三带二 打出后检查其他带牌里是否有更小的，如有则交换，注意修改not_biggest和biggest里的内容
    if (!pass)
    {
        //if (cur->type == THREE_ONE || cur->type == THREE_TWO)
        //{
        //    search_combos_in_suit(game, pl->h, pl->opps, pl);
        //}

        if (cur->type == THREE_ONE)
        {
            //if (first && pl->summary->bomb_num + 1 < pl->summary->real_total_num && pl->summary->real_total_num > 1)
            //{
            //    if (pl->opps->end >= Pq && cur->three_desc[0] >= pl->opps->end 
            //        && pl->h->end - cur->three_desc[0] <= 1 && pl->summary->singles_num >= 3) //带牌是最大牌
            //    {
            //        cur->type = THREE;
            //        cur->three_desc[0] = 0;
            //    }
            //}

            if (cur->type == THREE_ONE) // 上面逻辑可能会修改为THREE
            {
                for (int k = 0; k < pl->summary->three_one_num; ++k)
                {
                    if (pl->summary->three_one[k]->three_desc[0] < cur->three_desc[0]
                        && pl->summary->three_one[k]->three_desc[0] != cur->low)
                    {
                        char kicker = cur->three_desc[0];
                        cur->three_desc[0] = pl->summary->three_one[k]->three_desc[0];
                        pl->summary->three_one[k]->three_desc[0] = kicker;

                        //// not_biggest
                        //for (int m = 0; m < pl->summary->not_biggest_num; ++m)
                        //{
                        //    if (pl->summary->not_biggest[m]->type == THREE_ONE 
                        //        && pl->summary->not_biggest[m]->low == pl->summary->three_one[k]->low)
                        //    {
                        //        pl->summary->not_biggest[m]->three_desc[0] = pl->summary->three_one[k]->three_desc[0];
                        //    }
                        //}

                        //// biggest
                        //for (int n = 0; n < pl->summary->biggest_num; ++n)
                        //{
                        //    if (pl->summary->biggest[n]->type == THREE_ONE
                        //        && pl->summary->biggest[n]->low == pl->summary->three_one[k]->low)
                        //    {
                        //        pl->summary->biggest[n]->three_desc[0] = pl->summary->three_one[k]->three_desc[0];
                        //    }
                        //}

                        break; // 找到
                    }
                }
                for (int k = 0; k < pl->summary->singles_num; ++k)
                {
                    if (pl->summary->singles[k]->low < cur->three_desc[0]
                        && pl->summary->singles[k]->low != cur->low)
                    {
                        char kicker = cur->three_desc[0];
                        cur->three_desc[0] = pl->summary->singles[k]->low;
                        pl->summary->singles[k]->low = kicker;
                    }
                }
            }
        }
        else if (cur->type == THREE_TWO)
        {
            //if (first && pl->summary->bomb_num + 1 < pl->summary->real_total_num && pl->summary->real_total_num > 1)
            //{
            //    int opp_max_pair = -1;
            //    for (int i = pl->opps->end; i >= pl->opps->begin; --i)
            //    {
            //        if (pl->opps->hands[i] >= 2)
            //        {
            //            opp_max_pair = i;
            //            break;
            //        }
            //    }

            //    if (opp_max_pair >= Pq && cur->three_desc[0] >= opp_max_pair) //带牌是最大牌
            //    {
            //        cur->type = THREE;
            //        cur->three_desc[0] = 0;
            //    }
            //}

            if (cur->type == THREE_TWO)
            {
                for (int k = 0; k < pl->summary->three_two_num; ++k)
                {
                    if (pl->summary->three_two[k]->three_desc[0] < cur->three_desc[0]
                        && pl->summary->three_two[k]->three_desc[0] != cur->low)
                    {
                        char kickers[2] = {cur->three_desc[0], cur->three_desc[0]};
                        cur->three_desc[0] = pl->summary->three_two[k]->three_desc[0];
                        //cur->three_desc[1] = pl->summary->three_two[k]->three_desc[1];
                        pl->summary->three_two[k]->three_desc[0] = kickers[0];
                        //pl->summary->three_two[k]->three_desc[1] = kickers[1];

                        //// not_biggest
                        //for (int m = 0; m < pl->summary->not_biggest_num; ++m)
                        //{
                        //    if (pl->summary->not_biggest[m]->type == THREE_TWO
                        //        && pl->summary->not_biggest[m]->low == pl->summary->three_two[k]->low)
                        //    {
                        //        pl->summary->not_biggest[m]->three_desc[0] = pl->summary->three_two[k]->three_desc[0];
                        //        pl->summary->not_biggest[m]->three_desc[1] = pl->summary->three_two[k]->three_desc[1];
                        //    }
                        //}

                        //// biggest
                        //for (int n = 0; n < pl->summary->not_biggest_num; ++n)
                        //{
                        //    if (pl->summary->biggest[n]->type == THREE_TWO
                        //        && pl->summary->biggest[n]->low == pl->summary->three_two[k]->low)
                        //    {
                        //        pl->summary->biggest[n]->three_desc[0] = pl->summary->three_two[k]->three_desc[0];
                        //        pl->summary->biggest[n]->three_desc[1] = pl->summary->three_two[k]->three_desc[1];
                        //    }
                        //}

                        break; // 找到
                    }
                }
            }            
        }
    }

	if (!pass)
		pass = !(check_laizi_rules(cur, pl->h, first, &game->pre));

#if !ALLOW_SAME_KICKER
	if (!pass) //check special rules for LZ feiji.
		pass = !check_feiji_rules(cur);
#endif

	//for test
	if (!pass && !first && !check_combo_a_Big_than_b(cur, &game->pre))
	{

		PRINTF(LEVEL, "wrong out\n");
		pass = 1;
	}
	int rand_1000 = rand() % 1000;

	if (!pass) {
		if (game->hun == -1)
		{
            int card[max_POKER_NUM + 1];
            memcpy(card, game->players[CUR_PLAYER]->card, (max_POKER_NUM + 1) * sizeof(card[0]));
            combo_to_int_array(cur, p, card);
			////update pokers..
			int number = get_combo_number(cur);
			//remove_combo_poker(pl->h, cur, NULL);
			//remove_combo_poker(upPl->opps, cur, NULL);
			//upPl->oppDown_num -= number;
			//remove_combo_poker(downPl->opps, cur, NULL);
			//downPl->oppUp_num -= number;

			//combo_to_int_array(cur, p, game->players[CUR_PLAYER]->card);
			//remove_combo_poker(&game->all, cur, NULL);
			//*laizi = -1;

			if (first) {
				if (*delay == 0 && pl->h->total == 0) //last 
				{
					if (number <= 2) {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 2000;
						else if (rand_1000 <= 999)
							* delay = 3000 + rand() % 4000;
						else
							* delay = 7000 + rand() % 8000;
					}
					else {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 4000;
						else if (rand_1000 <= 999)
							* delay = 5000 + rand() % 5000;
						else
							* delay = 10000 + rand() % 5000;
					}

				}
				else if (*delay == 0)
				{
					if (number <= 2) {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 4000;
						else if (rand_1000 <= 990)
							* delay = 5000 + rand() % 5000;
						else
							* delay = 10000 + rand() % 5000;
					}
					else if (number <= 5) {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 5000;
						else if (rand_1000 <= 990)
							* delay = 6000 + rand() % 5000;
						else
							* delay = 11000 + rand() % 4000;
					}
					else {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 6000;
						else if (rand_1000 <= 990)
							* delay = 7000 + rand() % 5000;
						else
							* delay = 12000 + rand() % 4000;
					}
				}
				else {
					if (rand_1000 <= 200)
						* delay = 3000 + rand() % 3000;
					else if (rand_1000 <= 700)
						* delay = 6000 + rand() % 5000;
					else if (rand_1000 <= 900)
						* delay = 11000 + rand() % 5000;
					else
						* delay = 16000 + rand() % 8000;
				}

			}
			else {
				if (*delay == 0 && pl->h->total == 0) //last 
				{
					{
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 2000;
						else if (rand_1000 <= 999)
							* delay = 3000 + rand() % 4000;
						else
							* delay = 7000 + rand() % 8000;
					}
				}
				else if (*delay == 0)
				{
					if (number <= 2) {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 5000;
						else if (rand_1000 <= 990)
							* delay = 6000 + rand() % 6000;
						else
							* delay = 12000 + rand() % 4000;
					}
					else if (number <= 5) {
						if (rand_1000 <= 900)
							* delay = 1000 + rand() % 5000;
						else if (rand_1000 <= 990)
							* delay = 6000 + rand() % 5000;
						else
							* delay = 11000 + rand() % 4000;
					}
					else {
						if (rand_1000 <= 800)
							* delay = 1000 + rand() % 4000;
						else if (rand_1000 <= 980)
							* delay = 5000 + rand() % 5000;
						else
							* delay = 10000 + rand() % 5000;
					}
				}
			}


		}
		else
		{
			POKERS a;
			int tmp[26];
			//fixme: for the bomb case, KEEP BOMB_old OR KEEP hun?
			combo_to_int_array_hun(cur, tmp, pl->card, game->hun);
			convert_to_new_out_format(tmp, p, laizi);


			int num = read_poker_int(p, &a);

			//update pokers..
			sub_poker(pl->h, &a, pl->h);//cur, NULL);
										//check cards and h
			check_card(pl->h, pl->card);
			sub_poker(upPl->opps, &a, upPl->opps);
			sub_poker(downPl->opps, &a, downPl->opps);
			sub_poker(&game->all, &a, &game->all);

			upPl->oppDown_num -= get_combo_number(cur);
			downPl->oppUp_num -= get_combo_number(cur);

		}

		//PRINTF(VERBOSE, "game[%x] ", game);
		//print_card(p);
		//print_card_2(game->players[CUR_PLAYER]->card, 21);
		//game->prepre_playernum = game->pre_playernum;
		//game->pre_playernum = CUR_PLAYER;


		//PRINTF(VERBOSE, "itr@game[%x] own %d\n", game, game->rec.now);
		//game->rec.history[game->rec.now].player = CUR_PLAYER;
		//game->rec.history[game->rec.now++].h = *cur;
		//if (game->pre.type == BOMB_old || game->pre.type == ROCKET)
		//{
		//	game->rec.bombed++;
		//}
		//if (*p == -1)
		//	PRINTF_ALWAYS("ERR:  you play what??\n");

	}
	else
	{
		COMBO_OF_POKERS c;
		if (find_a_bigger_combo_in_hands(pl->h, &c, &game->pre)) {
			if (rand_1000 <= 900)
				* delay = 1000 + rand() % 4000;
			else if (rand_1000 <= 990)
				* delay = 5000 + rand() % 5000;
			else
				* delay = 10000 + rand() % 5000;
		}
		else {
			if (rand_1000 <= 800)
				* delay = 1000 + rand() % 2000;
			else if (rand_1000 <= 980)
				* delay = 3000 + rand() % 4000;
			else
				* delay = 7000 + rand() % 8000;
		}


		p[0] = -1;
		laizi[0] = -1;
		game->rec.history[game->rec.now].player = CUR_PLAYER;
		game->rec.history[game->rec.now++].h.type = NOTHING_old;
		if (first)
			PRINTF_ALWAYS("ERR:  you are the first player..please output something? OK?\n");
	}
	/*

	if( !check_poker_suit(game->players[CUR_PLAYER]->h,game->players[CUR_PLAYER]))
	{
	PRINTF_ALWAYS("own poker and combos doesn't match!\n");
	}
	/*
	if( game->known_others_poker) //check others
	{
	if( !check_poker_suit(game->players[UP_PLAYER]->h,game->players[UP_PLAYER]))
	{
	PRINTF_ALWAYS("up's poker and combos doesn't match!\n");
	}
	if( !check_poker_suit(game->players[DOWN_PLAYER]->h,game->players[DOWN_PLAYER]))
	{
	PRINTF_ALWAYS("down's poker and combos doesn't match!\n");
	}
	}
	//*/
	DBG(
		PRINTF(LEVEL, "==END_OF_ROBOT_PLAY TURN %d==\n\n", turn++);
	)

		*delay = (*delay) *game->delayPerGame / 100;

	return !pass;
}


int takeOut_internal(LordRobot* robot, int * p, int* laizi)
{
	GAME_DDZ_AI *game = &robot->game;
	COMBO_OF_POKERS *cur, combo;
	PLAYER * pl = game->players[CUR_PLAYER];
	PLAYER * downPl = game->players[DOWN_PLAYER];
	PLAYER * upPl = game->players[UP_PLAYER];

	cur = &combo;
	//  game->cur_playernum = 2;

	static int turn = 0;
	DBG(
		//PRINTF(LEVEL,"==START_OF_ROBOT_PLAY TURN %d==\n",turn);
	)
		int pass = 1, first = (game->players[UP_PLAYER]->cur == NULL && game->players[DOWN_PLAYER]->cur == NULL);

	search_combos_in_suit(game, game->players[CUR_PLAYER]->h, game->players[CUR_PLAYER]->opps, game->players[CUR_PLAYER]);
	if (game->known_others_poker)
	{
		search_combos_in_suit(game, game->players[DOWN_PLAYER]->h, game->players[DOWN_PLAYER]->opps, game->players[DOWN_PLAYER]);
		search_combos_in_suit(game, game->players[UP_PLAYER]->h, game->players[UP_PLAYER]->opps, game->players[UP_PLAYER]);
	}

	update_players(game, NULL);

	if (pl->h->total != 0) //error check
	{
		if (first)
		{   //主动出牌
			//printf("zhudong chupai!\n");
			pass = robot_play(robot, cur, 1);
			pass = 0;
		}
		else //被动出牌
		{
			//printf("beidong chupai!\n");
			pass = robot_play(robot, cur, 0);
		}
	}

	if (!pass) {
		if (game->hun == -1)
		{

			//update pokers..
			remove_combo_poker(pl->h, cur, NULL);
			remove_combo_poker(upPl->opps, cur, NULL);
			upPl->oppDown_num -= get_combo_number(cur);
			remove_combo_poker(downPl->opps, cur, NULL);
			downPl->oppUp_num -= get_combo_number(cur);

			combo_to_int_array(cur, p, game->players[CUR_PLAYER]->card);
			remove_combo_poker(&game->all, cur, NULL);
			*laizi = -1;
		}
		else
		{
			POKERS a;
			int tmp[26];
			//fixme: for the bomb case, KEEP BOMB_old OR KEEP hun?
			combo_to_int_array_hun(cur, tmp, pl->card, game->hun);
			convert_to_new_out_format(tmp, p, laizi);
			int num = read_poker_int(p, &a);

			//update pokers..
			sub_poker(pl->h, &a, pl->h);//cur, NULL);
										//check cards and h
			check_card(pl->h, pl->card);
			sub_poker(upPl->opps, &a, upPl->opps);
			sub_poker(downPl->opps, &a, downPl->opps);
			sub_poker(&game->all, &a, &game->all);

			upPl->oppDown_num -= get_combo_number(cur);
			downPl->oppUp_num -= get_combo_number(cur);

		}

		//  PRINTF(VERBOSE,"game[%x] ",game);
		//  print_card(p);
		//  print_card_2(game->players[CUR_PLAYER]->card,21);
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = CUR_PLAYER;


		//  PRINTF(VERBOSE,"itr@game[%x] own %d\n",game,game->rec.now);
		/*  game->rec.history[game->rec.now].player= CUR_PLAYER;
		game->rec.history[game->rec.now++].h= *cur;
		if(game->pre.type == BOMB_old ||game->pre.type == ROCKET)
		{
		game->rec.bombed++;
		}
		*/
		if (*p == -1)
			PRINTF_ALWAYS("ERR:  you play what??\n");

	}
	else
	{
		p[0] = -1;
		laizi[0] = -1;
		//      game->rec.history[game->rec.now].player= CUR_PLAYER;
		//      game->rec.history[game->rec.now++].h.type= NOTHING_old;
		if (first)
			PRINTF_ALWAYS("ERR:  you are the first player..please output something? OK?\n");
	}
	/*

	if( !check_poker_suit(game->players[CUR_PLAYER]->h,game->players[CUR_PLAYER]))
	{
	PRINTF_ALWAYS("own poker and combos doesn't match!\n");
	}
	/*
	if( game->known_others_poker) //check others
	{
	if( !check_poker_suit(game->players[UP_PLAYER]->h,game->players[UP_PLAYER]))
	{
	PRINTF_ALWAYS("up's poker and combos doesn't match!\n");
	}
	if( !check_poker_suit(game->players[DOWN_PLAYER]->h,game->players[DOWN_PLAYER]))
	{
	PRINTF_ALWAYS("down's poker and combos doesn't match!\n");
	}
	}
	//*/
	DBG(
		//PRINTF(LEVEL,"==END_OF_ROBOT_PLAY TURN %d==\n\n",turn++);
	)

		return !pass;
}



/*提示 可以出的牌;*/
int Hint(LordRobot* robot, int * p)
{
	GAME_DDZ_AI *game = &robot->game;
	int first = (game->players[UP_PLAYER]->cur == NULL && game->players[DOWN_PLAYER]->cur == NULL);

	if (first)
	{
		*p = -1;
		return 0;
	}
	else
	{
		int tmp_card[21];
		COMBO_OF_POKERS *cur, combo;
		PLAYER * pl = game->players[CUR_PLAYER];
		cur = &combo;
		memcpy(&tmp_card, game->players[CUR_PLAYER]->card, 21 * sizeof(int));

		if (!pl->first_hint && find_a_bigger_combo_in_hands(pl->h, cur, &pl->hint_combo))
		{
			combo_to_int_array(cur, p, tmp_card);
			pl->hint_combo = *cur;
			return 1;
		}
		else if (find_a_bigger_combo_in_hands(pl->h, cur, &game->pre))
		{
			combo_to_int_array(cur, p, tmp_card);
			pl->hint_combo = *cur;
			pl->first_hint = 0;
			return 1;
		}
		else {
			*p = -1;
			return 0;
		}
	}
}

#if 0
void DebugString(const char* lpszFormat, ...)
{
#ifdef WIN32
	va_list args;
	char szText[1024];

	va_start(args, lpszFormat);
	vsprintf(szText, lpszFormat, args);
	OutputDebugStringA(szText);
	va_end(args);
#endif //WIN32
}
#endif


//7、销毁机器人
void destoryRobot(LordRobot* robot)
{
	free(robot);
	if (logfile != NULL)
		fclose(logfile);
	logfile = NULL;
}
char TYPE[3][11] = { "LORD_old  ", "UPFARM","DNFARM" };
char POS[3][11] = { "CUR  ", "DOWN","UP   " };


void dump_game_record(RECORD_DDZ_AI *rec)
{
	//todo: rebuild p[3]..

	PRINTF(LEVEL, "\n\n\n=========GAME_DDZ_AI RECORD_DDZ_AI=======\n");

	PRINTF(LEVEL, "===CUR    PLAYER===\n");
	print_hand_poker(&rec->p[CUR_PLAYER]);
	PRINTF(LEVEL, "===DOWN  PLAYER===\n");
	print_hand_poker(&rec->p[DOWN_PLAYER]);
	PRINTF(LEVEL, "===UP       PLAYER===\n");
	print_hand_poker(&rec->p[UP_PLAYER]);
	PRINTF(LEVEL, "===   POT            ===\n");
	print_hand_poker(&rec->pot);

	PRINTF(LEVEL, "===   LORD_old is  %s    ===\n", rec->lord == CUR_PLAYER ? "CUR_PLAYER" :
		(rec->lord == DOWN_PLAYER ? "DOWN_PLAYER" : "UP_PLAYER"));

	PRINTF(LEVEL, "===   CUR_PLAYER is  %s    ===\n", TYPE[rec->type[CUR_PLAYER]]);


	add_poker(&rec->p[rec->lord], &rec->pot, &rec->p[rec->lord]);
	for (int i = 0; i<rec->now; i++)
	{
		int pos = rec->history[i].player;
		//          PRINTF(LEVEL,"[%s][%s] " , TYPE[rec->type[pos]], POS[pos]);
		PRINTF(LEVEL, "[%s] ", TYPE[rec->type[pos]]);

		print_combo_poker(&rec->history[i].h);
		PRINTF(LEVEL, "\t\tHANDS:");
		print_hand_poker_in_line(&rec->p[pos]);
		if (rec->history[i].h.type != NOTHING_old)
			remove_combo_poker(&rec->p[pos],
				&rec->history[i].h, NULL);
		PRINTF(LEVEL, "\n");
	}


}

#if 0//ef BUILD_TEST

char VER[30] = "0.99";
GAME_SETTING_DDZ_AI test_set;

void DUMP_GAME_FOR_PLAYER(LordRobot * r);

void change_game_setting(GAME_DDZ_AI * game)
{
	printf("player0 is [%s], 1: computer 0:human, ", game->computer[0] ? "Computer" : "Human   ");
	scanf("%d", &test_set.computer[0]);
	printf("player1 is [%s], 1: computer 0:human, ", game->computer[1] ? "Computer" : "Human   ");
	scanf("%d", &test_set.computer[1]);
	printf("player2 is [%s], 1: computer 0:human, ", game->computer[2] ? "Computer" : "Human   ");
	scanf("%d", &test_set.computer[2]);
	//    test_set.computer[0]=game->computer[0];
	//   test_set.computer[1]=game->computer[1];
	//  test_set.computer[2]=game->computer[2];
}

void test_suit()
{
	char buf[50];
	POKERS dz = { 0 };
	POKERS all = { 0 };
	POKERS opp = { 0 };
	COMBO_OF_POKERS testhands[20];
	COMBOS_SUMMARY_DDZ_AI s = { 0 };
	PLAYER hands_dz = { 0 };
	hands_dz.combos = testhands;
	hands_dz.h = &dz;
	hands_dz.opps = &opp;
	hands_dz.summary = &s;

	GAME_DDZ_AI *game, g;
	game = &g;


	int t = game->known_others_poker;
	int t2 = game->use_best_for_undefined_case;
	memset(game, 0, sizeof(GAME_DDZ_AI));
	game->known_others_poker = t;
	game->use_best_for_undefined_case = t2;

	game->pot = &game->POT;

	game->computer[0] = 1;
	{
		int j;
		for (j = 0; j<3; j++)
		{
			game->suits[j].h = &game->suits[j].p;
			game->suits[j].opps = &game->suits[j].opp;
			game->suits[j].combos = &game->suits[j].combos_store[0];
			game->suits[j].summary = &game->suits[j].combos_summary;
			game->suits[j].oppDown_num = 17;
			game->suits[j].oppUp_num = 17;

		}
	}
	game->players[0] = &game->suits[0];
	game->players[1] = &game->suits[1];
	game->players[2] = &game->suits[2];

	hands_dz.oppDown_num
		= hands_dz.oppUp_num = 17;
	PRINTF(LEVEL, "\n\nput hun:\n");
	scanf("%d", &game->hun);

	PRINTF(LEVEL, "\n\nput pokers:\n");
	scanf("%s", buf);

	read_poker(buf, &dz);
	print_hand_poker(&dz);
	full_poker(&all);
	sub_poker(&all, &dz, &opp);
	sort_poker(&opp);
	PRINTF(LEVEL, "opps:\n");
	print_hand_poker(&opp);

	search_combos_in_suit(game, &dz, &opp, &hands_dz);
	print_suit(&hands_dz);
	printf("\nfor min_ddz single..:\n");
	search_combos_in_suits_for_MIN_single_farmer(game, &dz, hands_dz.combos, &hands_dz);
	print_suit(&hands_dz);
}

int rand_a_card(int *p)
{
	int t;
	do {
		t = rand() % 54;
		if (p[t]>0) {
			p[t]--;
			return t;
		}
	} while (1);



}



//dump the game in player's view
void DUMP_GAME_FOR_PLAYER(LordRobot * r)
{
	GAME_DDZ_AI * game = &r->game;
	if (level >= VERBOSE)
	{
		//clrscr();
		PRINTF(LEVEL, "\n+-----------Current game------------+");
		//PRINTF(LEVEL,"control poker since %c",poker_to_char(game->lowest_bigPoker));
		PRINTF(LEVEL, "\n|CurPLAYER [%10s] [%s] \nHands: ", TYPE[game->player_type], game->computer[0] ? "Computer" : "Human   ");
		//PRINTF(LEVEL,"\33\33[42;7m test");
		print_hand_poker(game->players[CUR_PLAYER]->h);
		//print real card... for debug
		for (int i = 0; i<21; i++)
		{
			if (game->players[CUR_PLAYER]->card[i] != -1)
				PRINTF(LEVEL, "%d ", game->players[CUR_PLAYER]->card[i]);
		}
		PRINTF(LEVEL, "\n");

		if (1)//(game->computer[0])
			print_suit(game->players[CUR_PLAYER]);
		PRINTF(LEVEL, "\n OPPs: ");
		print_hand_poker(game->players[CUR_PLAYER]->opps);
		///*
		if (game->prepre.type != NOTHING_old)
		{
			PRINTF(LEVEL, "======Pre Pre hands from Player%d []:   ", game->prepre_playernum);
			print_combo_poker(&game->prepre);
			PRINTF(LEVEL, "      ======\n");
		}

		if (game->pre.type != NOTHING_old)
		{
			PRINTF(LEVEL, "======    Pre hands from Player%d []:   ", game->pre_playernum);
			print_combo_poker(&game->pre);
			PRINTF(LEVEL, "      ======\n");
		}
		//     PRINTF(LEVEL,"\nTurn of Player%d [%s]\n", CUR_PLAYER,game->lord==CUR_PLAYER+1?"lord  ":"farmer");

		// */
	}
}

int cards[3][20];
int all_cards[54]; //

void fill_card_from_poker(int * card, POKERS*p)
{
	for (int i = P3; i <= BIG_JOKER; i++)
	{
		for (int j = 0; j<p->hands[i]; j++)
		{
			if (i >= LIT_JOKER) {
				*card = 52 + i - LIT_JOKER;
			}
			else
				*card = i;
			card++;
		}
	}

}

void init_cus_game(GAME_DDZ_AI * game, GAME_SETTING_DDZ_AI * set)
{
	for (int k = 0; k<54; k++)
	{
		all_cards[k] = 1;
	}

	int t = game->known_others_poker;
	int t2 = game->use_best_for_undefined_case;
	memset(game, 0, sizeof(GAME_DDZ_AI));
	game->known_others_poker = t;
	game->use_best_for_undefined_case = t2;

	game->pot = &game->POT;

	game->computer[0] = 1;
	{
		int j;
		for (j = 0; j<3; j++)
		{
			game->suits[j].h = &game->suits[j].p;
			game->suits[j].opps = &game->suits[j].opp;
			game->suits[j].combos = &game->suits[j].combos_store[0];
			game->suits[j].summary = &game->suits[j].combos_summary;
			game->suits[j].oppDown_num = 17;
			game->suits[j].oppUp_num = 17;

		}
	}
	game->players[0] = &game->suits[0];
	game->players[1] = &game->suits[1];
	game->players[2] = &game->suits[2];
	game->players[CUR_PLAYER]->use_quick_win_check = game->known_others_poker;

	POKERS all;
	full_poker(&game->all);
	full_poker(&all);

	if (set->cus)
	{
		char buf[50];
	RE_INPUT:
		memset(game->players[0]->h, 0, sizeof(POKERS));
		memset(game->players[1]->h, 0, sizeof(POKERS));
		memset(game->players[2]->h, 0, sizeof(POKERS));
		if (set->cus == 2)
		{
			fscanf(set->input, "%s", buf);
			read_poker(buf, game->players[CUR_PLAYER]->h);
			fscanf(set->input, "%s", buf);
			read_poker(buf, game->players[DOWN_PLAYER]->h);
			fscanf(set->input, "%s", buf);
			read_poker(buf, game->players[UP_PLAYER]->h);
		}
		else {
			printf("player0:");
			scanf("%s", buf);
			//memset(game->players[0]->h,0,sizeof(POKER));
			read_poker(buf, game->players[0]->h);
			printf("player1:");
			scanf("%s", buf);
			//memset(game->players[1]->h,0,sizeof(POKER));
			read_poker(buf, game->players[1]->h);
			printf("player2:");
			scanf("%s", buf);
			//memset(game->players[2]->h,0,sizeof(POKER));
			read_poker(buf, game->players[2]->h);
			memset(game->pot, 0, sizeof(POKERS));
		}
		POKERS t = { 0 };

		add_poker(game->players[0]->h, game->players[1]->h, &t);
		add_poker(game->players[2]->h, &t, &all);


		for (int i = 0; i <= P2; i++)
			if (all.hands[i]>4) {
				printf("error input\n");
				goto RE_INPUT;
			}
		for (int i = LIT_JOKER; i <= BIG_JOKER; i++)
			if (all.hands[i]>1) {
				printf("error input\n");
				goto RE_INPUT;
			}
		sort_poker(&all);
	}

	sort_poker(game->players[0]->h);
	sort_poker(game->players[1]->h);
	sort_poker(game->players[2]->h);

	memset(game->players[CUR_PLAYER]->card, -1, sizeof(game->players[CUR_PLAYER]->card));
	memset(game->players[DOWN_PLAYER]->card, -1, sizeof(game->players[DOWN_PLAYER]->card));
	memset(game->players[UP_PLAYER]->card, -1, sizeof(game->players[UP_PLAYER]->card));
	fill_card_from_poker(game->players[CUR_PLAYER]->card, game->players[CUR_PLAYER]->h);
	fill_card_from_poker(game->players[DOWN_PLAYER]->card, game->players[DOWN_PLAYER]->h);
	fill_card_from_poker(game->players[UP_PLAYER]->card, game->players[UP_PLAYER]->h);
	for (int k = 0; k<game->players[CUR_PLAYER]->h->total; k++)
	{
		//game->players[CUR_PLAYER]->card[k]= robot_card[i];
	}


	log_brief = 1;

	if (set->cus)
	{
		memcpy(&game->all, &all, sizeof(POKERS));
	}
	else
		full_poker(&all);

	sub_poker(&all, game->players[0]->h, game->players[0]->opps);
	sub_poker(&all, game->players[1]->h, game->players[1]->opps);
	sub_poker(&all, game->players[2]->h, game->players[2]->opps);
	int lord = 0;
	if (set->cus)
	{
		if (set->cus == 2)
		{
			fscanf(set->input, "%d", &lord);
			lord += 2;
			lord %= 3;
			fscanf(set->input, "%d", &game->cur_playernum);
			if (game->cur_playernum >2) game->cur_playernum = 2;

			fscanf(set->input, "%d", &game->computer[DOWN_PLAYER]);
			fscanf(set->input, "%d", &game->computer[UP_PLAYER]);
			fscanf(set->input, "%d", &game->computer[CUR_PLAYER]);
			set->computer[CUR_PLAYER] = game->computer[CUR_PLAYER];
			set->computer[DOWN_PLAYER] = game->computer[DOWN_PLAYER];
			set->computer[UP_PLAYER] = game->computer[UP_PLAYER];
			int laizi;
			fscanf(set->input, "%d", &laizi);
			game->hun = laizi;
		}
		else {
			printf("select lord:");
			scanf("%d", &lord);
			lord++;
			if (lord >3) lord = 3;
			printf("first player:");
			scanf("%d", &game->cur_playernum);
			if (game->cur_playernum >2) game->cur_playernum = 2;

			change_game_setting(game);
		}
		game->players[0]->oppDown_num = game->players[1]->h->total;
		game->players[1]->oppDown_num = game->players[2]->h->total;
		game->players[2]->oppDown_num = game->players[0]->h->total;
		game->players[2]->oppUp_num = game->players[1]->h->total;
		game->players[0]->oppUp_num = game->players[2]->h->total;
		game->players[1]->oppUp_num = game->players[0]->h->total;


	}

	if (lord == DOWN_PLAYER) {
		game->players[0]->id = LORD_old;
		game->players[1]->id = DOWNFARMER_old;
		game->players[2]->id = UPFARMER_old;
		//    game->players[1]->oppDown_num = 0;
		//   game->players[2]->oppUp_num = 0;
	}
	else if (lord == UP_PLAYER) {
		game->players[1]->id = LORD_old;
		game->players[2]->id = DOWNFARMER_old;
		game->players[0]->id = UPFARMER_old;
		//    game->players[2]->oppDown_num = 0;
		//    game->players[0]->oppUp_num = 0;

	}
	else if (lord == CUR_PLAYER) {
		game->players[2]->id = LORD_old;
		game->players[0]->id = DOWNFARMER_old;
		game->players[1]->id = UPFARMER_old;
		//     game->players[0]->oppDown_num = 0;
		//   game->players[1]->oppUp_num = 0;
	}

	game->players[1]->lower_control_poker =
		game->players[0]->lower_control_poker =
		game->players[2]->lower_control_poker =
		game->lowest_bigPoker = get_lowest_controls(&game->all, CONTROL_POKER_NUM);

	game->player_type = game->players[CUR_PLAYER]->id;

	//    return 1;
}


void read_poker2(char * buf, int * out)
{
	do {
		switch (*buf++) {
		case '3':
			*out++ = 0 + 13 * (*buf - 48);
			break;
		case '4':
			*out++ = 1 + 13 * (*buf - 48);
			break;
		case '5':
			*out++ = 2 + 13 * (*buf - 48);
			break;
		case '6':
			*out++ = 3 + 13 * (*buf - 48);
			break;
		case '7':
			*out++ = 4 + 13 * (*buf - 48);
			break;
		case '8':
			*out++ = 5 + 13 * (*buf - 48);
			break;
		case '9':
			*out++ = 6 + 13 * (*buf - 48);
			break;
		case 't':
		case 'T':
			*out++ = 7 + 13 * (*buf - 48);
			break;
		case 'j':
		case 'J':
			*out++ = 8 + 13 * (*buf - 48);
			break;
		case 'q':
		case 'Q':
			*out++ = 9 + 13 * (*buf - 48);
			break;
		case 'k':
		case 'K':
			*out++ = 10 + 13 * (*buf - 48);
			break;
		case 'a':
		case 'A':
			*out++ = 11 + 13 * (*buf - 48);
			break;
		case '2':
			*out++ = 12 + 13 * (*buf - 48);
			break;
		case 'x':
		case 'X':
			*out++ = 52 + (*buf - 48);
			break;
		default:
			break;
		}
		buf++;
	} while (*buf++ != '\0');
}

int arrange_poker_from_file2(LordRobot * robot[3], int *pot, FILE* fp, int islaizi)
{
	int laizi = -1;
	char buf[100];
	for (int k = 0; k<3; k++)
	{
		fscanf(fp, "%s", buf);
		read_poker2(buf, robot[0]->card[k]);
	}
	fscanf(fp, "%s", buf);
	read_poker2(buf, pot);
	fscanf(fp, "%s", buf);
	//处理地主编号和先出编号
	fscanf(fp, "%s", buf);
	//处理人工出牌还是机器出牌 
	fscanf(fp, "%s", buf);
	read_poker2(buf, &laizi);
	if (-1 == islaizi)
	{
		laizi = -1;
	}

	initCard(robot[0], robot[0]->card[0], robot[0]->card[2], robot[0]->card[1]);//robot 0
	initCard(robot[1], robot[0]->card[1], robot[0]->card[0], robot[0]->card[2]);//robot 1
	initCard(robot[2], robot[0]->card[2], robot[0]->card[1], robot[0]->card[0]);//robot 2

	setLaizi(robot[0], laizi);
	setLaizi(robot[1], laizi);
	setLaizi(robot[2], laizi);

	//fill_card_from_poker(robot[0]->game.pot_card, robot[0]->game.pot);
	//fill_card_from_poker(robot[1]->game.pot_card, robot[1]->game.pot);
	//fill_card_from_poker(robot[2]->game.pot_card, robot[2]->game.pot);     

	//memcpy(robot[0]->card[0], robot[0]->game.players[2]->card, (max_POKER_NUM+1)*sizeof(int));
	//memcpy(robot[0]->card[1], robot[1]->game.players[2]->card, (max_POKER_NUM+1)*sizeof(int));
	//memcpy(robot[0]->card[2], robot[2]->game.players[2]->card, (max_POKER_NUM+1)*sizeof(int));
	//memcpy(pot, robot[0]->game.pot_card, 4*sizeof(int));

	return laizi;
}

int  arrange_poker(LordRobot * robot[3], int *pot, int laizi)
{
#ifdef WIN32
	FILE* fp = fopen("robot_cards.csv", "rt");
#else
	FILE* fp = fopen("/mnt/sdcard/robot_cards.csv", "rb");
#endif
	if (fp == NULL)
	{
		for (int k = 0; k<54; k++)
		{
			all_cards[k] = 1;
		}
		// if (!test_set.cus) //rand
		{
			for (int k = 0; k<17; k++)
			{
				robot[0]->card[0][k] = rand_a_card(all_cards);
				robot[0]->card[1][k] = rand_a_card(all_cards);
				robot[0]->card[2][k] = rand_a_card(all_cards);
			}
			for (int k = 17; k<21; k++)
			{
				robot[0]->card[0][k] = -1;
				robot[0]->card[1][k] = -1;
				robot[0]->card[2][k] = -1;
			}
			for (int k = 0; k<3; k++)
			{
				pot[k] = rand_a_card(all_cards);
			}
			pot[3] = -1;
		}
		initCard(robot[0], robot[0]->card[0], robot[0]->card[2], robot[0]->card[1]);//robot 0
		initCard(robot[1], robot[0]->card[1], robot[0]->card[0], robot[0]->card[2]);//robot 1
		initCard(robot[2], robot[0]->card[2], robot[0]->card[1], robot[0]->card[0]);//robot 2
		setLaizi(robot[0], laizi);
		setLaizi(robot[1], laizi);
		setLaizi(robot[2], laizi);
	}
	else {

		laizi = arrange_poker_from_file2(robot, pot, fp, laizi);
		fclose(fp);
	}

	return laizi;
}



int arrange_poker_from_file(LordRobot * robot[3], int *pot)
{
	for (int k = 0; k<54; k++)
	{
		all_cards[k] = 1;
	}
	POKERS a, cur = { 0 };
	int laizi = 0;
	char buf[100];
	//if ( set->cus==2)
	full_poker(&a);
	for (int k = 0; k<3; k++)
	{
		memset(&cur, 0, sizeof(POKERS));
		fscanf(test_set.input, "%s", buf);
		read_poker(buf, &cur);
		sub_poker(&a, &cur, &a);
		fill_card_from_poker(cards[k], &cur);
	}
	printf("laizi: \n");
	scanf("%d", &laizi);

	fill_card_from_poker(pot, &a);

	initCard(robot[0], cards[0], cards[2], cards[1]);//robot 0
	initCard(robot[1], cards[1], cards[0], cards[2]);//robot 1
	initCard(robot[2], cards[2], cards[1], cards[0]);//robot 2

	setLaizi(robot[0], laizi);
	setLaizi(robot[1], laizi);
	setLaizi(robot[2], laizi);

	fill_card_from_poker(robot[0]->game.pot_card, robot[0]->game.pot);
	fill_card_from_poker(robot[1]->game.pot_card, robot[1]->game.pot);
	fill_card_from_poker(robot[2]->game.pot_card, robot[2]->game.pot);
	return laizi;
}
int deterMINe_lord(LordRobot * robot[3])
{
	//call score
	int lord = -1;
	int score = 0;
	//for(int k=0;k<3;k++)
	int m1 = 0;//(robot[0]);
	int m2 = forceLord(robot[0]);
	int m3 = doubleGame(robot[0]);

	//    int m2=showCard(robot[1]);
	//   int m3=showCard(robot[2]);
	printf("%d %d %d\n", m1, m1, m1);
	{
		score = RobotcallScore(robot[0]);
		getPlayerCallScore(robot[1], score, 0);
		getPlayerCallScore(robot[2], score, 1);
		if (score>0)
			lord = 0;
		if (score == 3)
		{
			goto BE_LORD;
		}
	}

	{
		score = RobotcallScore(robot[1]);
		getPlayerCallScore(robot[2], score, 0);
		getPlayerCallScore(robot[0], score, 1);
		if (score>0)
			lord = 1;
		if (score == 3)
		{
			goto BE_LORD;
		}
	}

	{
		score = RobotcallScore(robot[2]);
		getPlayerCallScore(robot[0], score, 0);
		getPlayerCallScore(robot[1], score, 1);
		if (score>0)
			lord = 2;
		if (score == 3)
		{
			goto BE_LORD;
		}
	}
	if (lord == -1)
	{
		printf("no one call lord,player0 be lord...\n");
		lord = 0;
	}
BE_LORD:
	return lord;
}


int user_play(LordRobot* robot, int * p, int * laizi)
{
#if 1
	GAME_DDZ_AI * game = &robot->game;
	char buf[100];
	POKERS tmp;
	PLAYER * pl = game->players[CUR_PLAYER];
	PLAYER * downPl = game->players[UP_PLAYER];
	PLAYER * upPl = game->players[DOWN_PLAYER];
	POKERS *h = pl->h;

	COMBO_OF_POKERS *combo, hand;
	combo = &hand;
	int pass = 1, first;
	first = (game->players[UP_PLAYER]->cur == NULL && game->players[DOWN_PLAYER]->cur == NULL);
	while (1) {

		//pass =1;
		//break;
	HUMAN_PLAY:
		printf("please select pokers (enter 0 for pass)(enter m for change setting):\n");
		scanf("%s", buf);
		if (*buf == 'm')
		{
			change_game_setting(game);
			if (game->computer[CUR_PLAYER])
				return takeOut(robot, p, laizi);
			else
				goto HUMAN_PLAY;

		}
		memset(&tmp, 0, sizeof(POKERS));
		read_poker(buf, &tmp);
		sort_poker(&tmp);
		tmp.hun = h->hun;
		if (tmp.total == 0)
		{
			if (first) {
				PRINTF(LEVEL, "!!ERR: you cannot pass\n");
				continue;
			}
			pass = 1;
			break;
		}
		if (is_sub_poker(&tmp, h))
		{
			if (is_combo_hun(&tmp, combo))
			{
				if (first || check_combo_a_Big_than_b(combo, &game->pre))
				{
					pass = 0;
					break;
				}
				else
				{
					PRINTF(LEVEL, "!!ERR:wrong input, too litte! \n");
					continue;
				}
			}
			else
			{
				PRINTF(LEVEL, "!!ERR: wrong input, not a hands\n");
				continue;
			}
		}
		else
		{
			PRINTF(LEVEL, "!!ERR: wrong input, poker in hands is not enough\n");
			continue;
		}
	}
#endif

	COMBO_OF_POKERS * cur = combo;
	if (!pass) {
		if (game->hun == -1)
		{
			//update pokers..
			remove_combo_poker(pl->h, cur, NULL);
			remove_combo_poker(upPl->opps, cur, NULL);
			upPl->oppDown_num -= get_combo_number(cur);
			remove_combo_poker(downPl->opps, cur, NULL);
			downPl->oppUp_num -= get_combo_number(cur);

			combo_to_int_array(cur, p, game->players[CUR_PLAYER]->card);
			remove_combo_poker(&game->all, cur, NULL);
			laizi[0] = -1;
		}
		else
		{
			POKERS a, b, c;
			int tmp[26];
			//fixme: for the bomb case, KEEP BOMB_old OR KEEP hun?
			combo_to_int_array_hun(cur, tmp, pl->card, game->hun);
			convert_to_new_out_format(tmp, p, laizi);

			int i;
			int num = read_poker_int(p, &a);
			//update pokers..
			sub_poker(pl->h, &a, pl->h);//cur, NULL);
										//check cards and h
			check_card(pl->h, pl->card);
			sub_poker(upPl->opps, &a, upPl->opps);
			sub_poker(downPl->opps, &a, downPl->opps);
			sub_poker(&game->all, &a, &game->all);

			upPl->oppDown_num -= get_combo_number(cur);
			downPl->oppUp_num -= get_combo_number(cur);



		}

		PRINTF(VERBOSE, "game[%x] ", game);
		print_card(p);
		game->prepre_playernum = game->pre_playernum;
		game->pre_playernum = CUR_PLAYER;
		//research here to get more robust
		/*
		search_combos_in_suit(game, game->players[CUR_PLAYER]->h,  game->players[CUR_PLAYER]->opps,game->players[CUR_PLAYER]);
		search_combos_in_suit(game, game->players[DOWN_PLAYER]->h,  game->players[DOWN_PLAYER]->opps,game->players[DOWN_PLAYER]);
		search_combos_in_suit(game, game->players[UP_PLAYER]->h,  game->players[UP_PLAYER]->opps,game->players[UP_PLAYER]);

		update_players(game,cur); */


		PRINTF(VERBOSE, "itr@game[%x] own %d\n", game, game->rec.now);
		game->rec.history[game->rec.now].player = CUR_PLAYER;
		game->rec.history[game->rec.now++].h = *cur;
		if (game->pre.type == BOMB_old || game->pre.type == ROCKET)
		{
			game->rec.bombed++;
		}
		if (*p == -1)
			PRINTF_ALWAYS("ERR:  you play what??\n");

	}
	else
	{
		p[0] = -1;
		laizi[0] = -1;
		game->rec.history[game->rec.now].player = CUR_PLAYER;
		game->rec.history[game->rec.now++].h.type = NOTHING_old;
	}
	return pass;

}

//#define Lydebug
#ifdef Lydebug
int main(int argc, char** argv)
{
	int out1[26], out2[5];
	//GAME_DDZ_AI game;
	int end = 10000, game_number = 0;
	int seed = 300000;
	char filename[50];
	test_set.computer[0] = 1;
	test_set.computer[1] = 1;
	test_set.computer[2] = 1;


	int lord_win = 0, farmer1_win = 0, farmer2_win = 0, game_played = 0;
	int laizi = -1;
NEW_GAME:
	level = 0;
	logfile = NULL;
	logfile2 = NULL;
	for_test = 1;
	test_set.cus = 0;
	printf("\n == DouDiZhu Algorithm Test (VER: %s) Powered By ZZF ==\n", VER);
	printf(" == press 0 for exit: \n");
	printf(" == press 1 for a rand new game \n");
	printf(" == press 2 for selecting a new game \n");

	fflush(stdin);
	if (argc>1)
		end = 8;
	else {
		end = 0;
		scanf("%d", &end);
	}

	if (end == 0)
	{
		exit(0);
	}
	else if (end == 2) {
		printf("input game id:\n");
		scanf("%d", &seed);
		printf("input laizi :\n");
		scanf("%d", &laizi);

		printf("player0 is [%s], 1: computer 0:human, ", test_set.computer[0] ? "Computer" : "Human   ");
		scanf("%d", &test_set.computer[0]);
		printf("player1 is [%s], 1: computer 0:human, ", test_set.computer[1] ? "Computer" : "Human   ");
		scanf("%d", &test_set.computer[1]);
		printf("player2 is [%s], 1: computer 0:human, ", test_set.computer[2] ? "Computer" : "Human   ");
		scanf("%d", &test_set.computer[2]);
	}
	else if (end == 1) {
		seed = rand();
		//test_set.computer[0]=1;
	}
	else goto NEW_GAME;

LOOP:
	//test_set.computer[0]=1;

	if (end != 6)
	{
		printf("Begin game %d ,log to %d.txt and brief_%d.txt \n", seed, seed, seed);
		sprintf(filename, "%d.txt", seed);
		if ((logfile = fopen(filename, "wb")) == NULL)
		{
			printf("could not open log %s", filename);
			// exit(0);
		}
		sprintf(filename, "brief_%d.txt", seed);
		if ((logfile2 = fopen(filename, "wb")) == NULL)
		{
			printf("could not open log %s", filename);
			// exit(0);
		}
	}
	srand(seed);

INIT_GAME:

	log_brief = 1;
	PRINTF(LEVEL, "MAIN: === GAME_DDZ_AI %d start (VER: %s)===\n", seed, VER);
	log_brief = 0;


	if (logfile != NULL)
		fclose(logfile);
	logfile = NULL;
	//set up robot
	LordRobot* robot[3];
	robot[0] = createRobot(3, 0);
	robot[1] = createRobot(3, 0);
	robot[2] = createRobot(3, 0);
	int lord;
	if (test_set.cus == 0)
	{
		//fapai
		int pot[10];

		if (laizi == -1)
			laizi = rand() % 13;
		printf("laizi is %d\n", laizi);
		arrange_poker(robot, pot, laizi);


		//deterMINe_lord
		//lord = deterMINe_lord(robot);
		lord = -1;
		for (int i = 0; i<3; ++i)
		{
			PRINTF(0, "Main Before RbtOutGetCallScore robot%d", i);
			int score = 0;
			score = RobotcallScore(robot[i]);
			PRINTF(0, "Main RbtOutGetCallScore robot%d=%d", i, score);
			if (score > 0)
			{
				lord = i;
				break;
			}
		}

		if (lord == -1)  //lyadd
			lord = 0;  //lyadd

		PRINTF(LEVEL, "MAIN: === LORD_old is %d===\n", lord);

		//reservedCard(robot[lord],pot);
		beLord(robot[(lord + 0) % 3], pot, 2);
		beLord(robot[(lord + 1) % 3], pot, 1);
		beLord(robot[(lord + 2) % 3], pot, 0);
	}

PLAY_GAME:
	int winner;
	{
		COMBO_OF_POKERS *pre = NULL, *cur = NULL, Hands[3];
		//int last = game->lord - 1;
		int first = 1;
		int pass1 = 1, pass2 = 1, pass3 = 1;
		pre = &Hands[1];
		cur = &Hands[2];
		if (lord == 0) {
			goto turn_p1;
		}
		if (lord == 1) goto turn_p2;
		if (lord == 2) goto turn_p3;

		while (1)
		{
		turn_p1:
			//  PRINTF(LEVEL,"MAIN: === PLAYER0's TURN ===\n");

			//         DUMP_GAME_FOR_PLAYER(robot[0]);
			//      DUMP_GAME_FOR_PLAYER(robot[1]);
			//      DUMP_GAME_FOR_PLAYER(robot[2]);
			if (test_set.computer[0])
				pass1 = takeOut(robot[0], out1, out2);
			else
				pass1 = user_play(robot[0], out1, out2);

			getPlayerTakeOutCards(robot[1], out1, out2, 1);
			getPlayerTakeOutCards(robot[2], out1, out2, 0);

			if (!pass1)
			{
				PRINTF(LEVEL, "MAIN: === PLAYER0 passed===\n");
			}
			else
			{
				PRINTF(LEVEL, "MAIN: === PLAYER0: ");
				print_combo_poker(&robot[1]->game.pre);
				PRINTF(LEVEL, "===\n");

			}


			winner = 0;
			if (robot[0]->game.players[2]->h->total == 0) //game over
				break;
		turn_p2:
			//   PRINTF(LEVEL,"MAIN: === PLAYER1's TURN ===\n");

			//         DUMP_GAME_FOR_PLAYER(robot[1]);
			if (test_set.computer[1])
				pass1 = takeOut(robot[1], out1, out2);
			else
				pass1 = user_play(robot[1], out1, out2);

			getPlayerTakeOutCards(robot[2], out1, out2, 1);
			getPlayerTakeOutCards(robot[0], out1, out2, 0);
			if (!pass1)
			{
				PRINTF(LEVEL, "MAIN: === PLAYER1 passed===\n");
			}
			else
			{
				PRINTF(LEVEL, "MAIN: === PLAYER1: ");
				print_combo_poker(&robot[2]->game.pre);
				PRINTF(LEVEL, "===\n");

			}
			winner = 1;
			if (robot[1]->game.players[2]->h->total == 0) //game over
				break;
		turn_p3:
			//  PRINTF(LEVEL,"MAIN: === PLAYER2's TURN ===\n");

			//            DUMP_GAME_FOR_PLAYER(robot[2]);
			if (test_set.computer[2])
				pass1 = takeOut(robot[2], out1, out2);
			else
				pass1 = user_play(robot[2], out1, out2);
			getPlayerTakeOutCards(robot[0], out1, out2, 1);
			getPlayerTakeOutCards(robot[1], out1, out2, 0);
			if (!pass1)
			{
				PRINTF(LEVEL, "MAIN: === PLAYER2 passed===\n");
			}
			else
			{
				PRINTF(LEVEL, "MAIN: === PLAYER2: ");
				print_combo_poker(&robot[1]->game.pre);
				PRINTF(LEVEL, "===\n");

			}
			winner = 2;
			if (robot[2]->game.players[2]->h->total == 0) //game over
				break;
			//goto turn_p1;
		}
		log_brief = 1;
	}

	dump_game_record(&robot[0]->game.rec);
	//destory robot
	destoryRobot(robot[0]);
	destoryRobot(robot[1]);
	destoryRobot(robot[2]);
	//   int res=play_game(&game);
	// if (res==1) goto NEW_GAME;
	lord_win += robot[winner]->game.player_type == LORD_old;

	game_played++;
	printf("lord wins %d in %d games\n", lord_win, game_played);
	log_brief = 1;
	PRINTF(LEVEL, "=== GAME_DDZ_AI %d Finished (Ver: %s)===\n", seed, VER);
	log_brief = 0;

	fflush(logfile);
	fflush(logfile2);
	if ((logfile) != NULL)
		fclose(logfile);
	if (logfile2 != NULL)
		fclose(logfile2);
	if (game_number >0)
	{
		seed++;
		game_number--;
		goto LOOP;
	}

	goto NEW_GAME;

}
#else
void testLaiziAI(vector<int> allCards)
{
	int LaiZicard = 0;
	int LZshow = 0;
	COGLZLordRobotAI ai0;
	COGLZLordRobotAI ai1;
	COGLZLordRobotAI ai2;
	ai0.RbtInSetLevel(10);
	ai1.RbtInSetLevel(10);
	ai2.RbtInSetLevel(10);
	vector<COGLZLordRobotAI *> allAi;
	allAi.push_back(&ai0);
	allAi.push_back(&ai1);
	allAi.push_back(&ai2);


	/*  vector<int> allCards;
	vector<vector<int>> aiCards;
	vector<int> leftCards;
	for (int i=0; i<ALL_CARDS_NUM; ++i)
	{
	allCards.push_back(i);
	}

	random_shuffle(allCards.begin(), allCards.end());*/

	vector<vector<int>> aiCards;
	vector<int> leftCards;
	for (unsigned i = 0; i<3; ++i)
	{
		vector<int> cards;
		for (unsigned j = 0; j<17; ++j)
		{
			cards.push_back(allCards[i * 17 + j]);
		}
		aiCards.push_back(cards);
	}
	for (unsigned i = 51; i<54; ++i)
	{
		leftCards.push_back(allCards[i]);
	}

	LaiZicard = rand() % (P2 + 1);
	LZshow = LaiZicard + 3;

	for (int i = 0; i<3; ++i)
	{
		allAi[i]->RbtInInitCard(i, aiCards[i]);
		allAi[i]->RbtInNtfCardInfo(aiCards);
		allAi[i]->RbtInSetLaizi(LaiZicard);
	}
	int lord = -1;
	for (int i = 0; i<3; ++i)
	{
		int score = 0;
		allAi[i]->RbtOutGetCallScore(score);
		if (score > 0)
		{
			lord = i;
			break;
		}
	}

	if (lord == -1)  //lyadd
		lord = 0;  //lyadd
	cout << "癞子:" << LZshow << endl;
	cout << "lord seat: " << lord << endl;

	if (lord >= 0)
	{
		for (int i = 0; i<3; ++i)
		{
			allAi[i]->RbtInSetLord(lord, leftCards, LaiZicard);
		}
		int remain[3] = { 17, 17, 17 };
		remain[lord] = 20;
		int curPlayer = lord;
		int round = 0;
		PRINTF(0, "main Game start lord=%d,laizi=%d", lord, LaiZicard + 3);
		while (round < 200)
		{
			vector<int> outCards;
			vector<int> replaceCard;
			//  cout << "player " << curPlayer << endl;
			allAi[curPlayer]->RbtOutGetTakeOutCard(outCards, replaceCard);
			for (int i = 0; i<3; ++i)
			{
				//if(i!=curPlayer)    //lyadd
				allAi[i]->RbtInTakeOutCard(curPlayer, outCards, replaceCard);
			}
			remain[curPlayer] -= outCards.size();
			if (remain[curPlayer] <= 0)
			{
				break;
			}
			else        if (outCards.size() >= 2 && outCards[0] == 52 && outCards[1] == 53)
			{
				cout << "rocket!" << endl;
			}
			else
			{
				curPlayer = (curPlayer + 1) % 3;
			}
			++round;
			//cout << "-------------------------\n";
		}
	}
	//cout << "seed: " << seed << endl;
	vector<int> argTakeOutSeat;
	vector<vector<int>> argTakeOutRecord;
	vector<vector<int>> argTakeOutReplaceRecord;
	//allAi[lord]->RbtInTakeOutRecord(argTakeOutSeat,argTakeOutRecord,argTakeOutReplaceRecord);
	int errcode;
	allAi[lord]->RbtOutGetLastError(errcode);   //for debug

}

void testLaiziAIwithTimes(int times)
{
	unsigned seed = 1017095;//(unsigned)time( NULL );   
	cout << "seed: " << seed << endl;
	for (int i = 0; i<times; ++i)
	{
		srand(seed + i);

		cout << "-------------  " << seed + i << endl;
		vector<int> allCards;
		for (int i = 0; i<ALL_CARDS_NUM; ++i)
		{
			allCards.push_back(i);
		}

		random_shuffle(allCards.begin(), allCards.end());

		testLaiziAI(allCards);
		cout << "-------------   " << i << endl;
	}


	cout << "seed: " << seed << endl;

}

int ChangeCharToInt(char c)
{
	int retnum;
	if ('t' == c || 'T' == c)
	{
		retnum = 10;
	}
	else if ('j' == c || 'J' == c)
	{
		retnum = 11;
	}
	else if ('q' == c || 'Q' == c)
	{
		retnum = 12;
	}
	else if ('k' == c || 'K' == c)
	{
		retnum = 13;
	}
	else if ('a' == c || 'A' == c)
	{
		retnum = 14;
	}
	else if ('2' == c)
	{
		retnum = 15;
	}
	else if ('x' == c || 'X' == c)
	{
		retnum = 16;
	}
	else if ('d' == c || 'D' == c)
	{
		retnum = 17;
	}
	else
	{
		retnum = c - '0';
	}
	return retnum;
}

void ReadDDZFile()
{
	fstream fin;
	fin.open("laiziddz.txt", ios::in);
	if (!fin)
	{
		cout << "File open failed!" << endl;
		return;
	}
	char c;
	vector<int> allCards;
	int total = 0;
	while ((c = fin.get()) != EOF)
	{
		int p = ChangeCharToInt(c) - 3;
		if ((p >= 0) && (p <= 14))
		{
			int p1 = p <= 12 ? p : 52 + (p - 13);
			allCards.push_back(p1);
			total++;
		}
	}

	if (total == ALL_CARDS_NUM)
	{
		//  for(int i=0;i<total;++i)
		//     cout <<allCards[i];
		testLaiziAI(allCards);
	}
	else
		cout << "Error: the number of poker in the file is wrong" << endl;
	fin.close();
}


int _tmain(int argc, char* argv[])
{
	int num, times;

	logfile = NULL;
	logfile2 = NULL;

	logfile = fopen("log.txt", "wb");
	logfile2 = fopen("log1.txt", "wb");

	cout << "\nLaiziDDZ AI Test " << endl;
	cout << "== press 0 for exit" << endl;
	cout << "== press 1 for test 100 times" << endl;
	cout << "== press 2 for stress test" << endl;
	cout << "== press 3 for read a game from file" << endl;
	cin >> num;

	switch (num)
	{
	case 0:
		return 0;
		break;
	case 1:
		cout << "Please wait..." << endl;
		testLaiziAIwithTimes(100);
		break;
	case 2:
		cout << "Please input the times to be tested:" << endl;
		cin >> times;
		cout << "Please wait..." << endl;
		testLaiziAIwithTimes(times);
		break;
	case 3:
		ReadDDZFile();
		break;
	}

	fflush(logfile);
	fflush(logfile2);
	if ((logfile) != NULL)
		fclose(logfile);
	if (logfile2 != NULL)
		fclose(logfile2);

	system("pause");
	return 0;
}

#endif

#endif
