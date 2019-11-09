// It's used for Horn Games
// The interfaces for DDZ ai.
#pragma once
#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <stdarg.h>

#define MAX_POKER_KIND 15
#define max_POKER_NUM 20
#define CONTROL_POKER_NUM 6
#define MAX_COMBO_NUM 20
//typedef char BOOL;
//#define true 1
//#define false 0

#define CONTROL_POKER_NUM_FUNC(game) \

typedef struct POKERS
{
    char hands[MAX_POKER_KIND];
    char total;
    char begin;
    char end;
    char hun;
    char hun_number;	
    char control;
} POKERS;

typedef struct COMBO_OF_POKERS
{
    unsigned int type;
    //char number;
    char low; // the lowest poker
    char len; // length for the series
    char control; // is it a control
    char three_desc[5]; //for three and 411,422
                                 // if it's bomb,three_desc[0] : 0, 带混诈，1 不带混诈，2 纯混诈
} COMBO_OF_POKERS;

//don't CHANGE value in the following!!
typedef enum PLAYER_POS {
    DOWN_PLAYER =0,
	UP_PLAYER =1,
	CUR_PLAYER=2,
}PLAYER_POS;


typedef struct
{
    char single;
    char pair;
    char series;
    char three;
    char bomb;
    char ctrl;
} CONTROLS_DDZ_AI;

typedef struct COMBOS_SUMMARY_DDZ_AI
{
    CONTROLS_DDZ_AI ctrl;
    char extra_bomb_ctrl;
    char combo_total_num; //exclude biggest combos
    char real_total_num; // include  biggest combos

    char combo_typenum;
    char combo_with_2_controls;

    char control_num;

    char singles_num;
    COMBO_OF_POKERS * singles[MAX_POKER_KIND];
    char pairs_num;
    COMBO_OF_POKERS * pairs[MAX_POKER_KIND];

    char three_num;
    COMBO_OF_POKERS * three[MAX_POKER_KIND];

    char series_num;
    COMBO_OF_POKERS * series[6];
    char series_detail[10];

    char bomb_num;
    COMBO_OF_POKERS * bomb[6];

    char three_one_num;
    COMBO_OF_POKERS * three_one[6];

    char three_two_num;
    COMBO_OF_POKERS * three_two[6];

    char threeseries_one_num;
    COMBO_OF_POKERS * threeseries_one[3];

    char threeseries_two_num;
    COMBO_OF_POKERS * threeseries_two[3];

    char threeseries_num;
    COMBO_OF_POKERS * threeseries[3];

    char pairs_series_num;
    COMBO_OF_POKERS * pairs_series[3];
    char biggest_num;
    COMBO_OF_POKERS * biggest[20];
    char not_biggest_num;
    COMBO_OF_POKERS * not_biggest[20];
    char four_one_num;
    COMBO_OF_POKERS * four_one[4];
    char four_two_num;
    COMBO_OF_POKERS * four_two[3];

    char combo_smallest_single;
} COMBOS_SUMMARY_DDZ_AI;
/*
typedef struct {
    bool has_series;
    bool has_double_series_begin;
    bool has_three_series_begin;
    bool has_bomb; //donot contact rocket.
    bool has_three;
} HAS_COMBOS_SUMMARY;
*/
typedef struct
{
    POKERS *h; // current poker in hands.
    POKERS *opps;
    char oppUp_num,oppDown_num; //number of opps hands
    char lower_control_poker;
    int id; //
    char good_framer; //1:good
    bool need_research;
     int chuntian; //是否可能被对方春天
    COMBO_OF_POKERS *combos;
    COMBOS_SUMMARY_DDZ_AI* summary;
//    CONTROLS max_control; //max control for all perhaps combos
    int  computer; //1  1:com 0:player
    bool use_quick_win_check;
    char score;
    //POKERS* poker;
    //COMBO_OF_POKERS* history[20];
    //char currComb;
    COMBO_OF_POKERS *cur;
    COMBO_OF_POKERS curcomb;//to be deleted

    POKERS * Up;      //the poker of Up player has outputted
    POKERS * Down; //the poker of Down player has outputted
    POKERS * me;    //the poker of  cur player has outputted

    //HAS_COMBOS_SUMMARY all; // to speed up..
    COMBO_OF_POKERS Perhaps[20];
    COMBO_OF_POKERS combos_store[MAX_COMBO_NUM];	
    COMBOS_SUMMARY_DDZ_AI combos_summary;	
    int card[max_POKER_NUM+1]; //card
    POKERS p;
    POKERS opp;   	
    int first_hint;
    COMBO_OF_POKERS hint_combo; 
    int lastPassSingle; // 上一个管不上的单牌
} PLAYER;
///*
typedef struct
{
    COMBO_OF_POKERS h;
    int player;
} HISTORY_HAND_DDZ_AI;
//*/
#define CALL_MAX_TIME 10
typedef struct
{
	int player[CALL_MAX_TIME];
	int score[CALL_MAX_TIME];
	int cur_num;
	int double_times;
	int cur_lord;
}CALL_HISTORY;

typedef struct
{
    POKERS pot;
    POKERS p[3];
    HISTORY_HAND_DDZ_AI history[180];
    int score[3];	
    int type[3];	
    int first_caller;	
	CALL_HISTORY call;
    int lord;
    int winner;
    int bombed;
    int now;	
} RECORD_DDZ_AI;

typedef struct {
    int computer[3];
    int dapaikongzhi_is_biggest;
    int cus;
    char filename[1000];
    FILE* input;
} GAME_SETTING_DDZ_AI;

typedef struct
{
    POKERS *pot;
    POKERS POT;	
    POKERS all;
    int pot_card[4];
    int  lowest_bigPoker; // the first 6 biggest poker.
    
    int computer[3];
    COMBO_OF_POKERS prepre,pre,cur;
    int prepre_playernum,pre_playernum;
    int cur_playernum;
    PLAYER * players[3]; // for three playes....
    PLAYER suits[3];  //

    COMBO_OF_POKERS tmp_combos[4][25];	
    COMBO_OF_POKERS c[5]; //use for internal...		
    COMBOS_SUMMARY_DDZ_AI tmp_summary[4];
    CALL_HISTORY call;
    RECORD_DDZ_AI rec;
    //global
    int player_type; //lord or farmer;
    int init;
    int first_half;
    int known_others_poker;
    int refer_others_poker; //for calculate the biggest combos.
    int use_best_for_undefined_case;
    COMBO_OF_POKERS tmp_for_best;//to be removed
    int good_farmer_0;
    int good_farmer_1 ;
    int hun;
	int search_level;	
	int pointer_init;
    int delayPerGame; // defalut is 100, 0 - 100.
} GAME_DDZ_AI;

void full_poker(POKERS*);
int sort_poker(POKERS*);
void sub_poker(POKERS* A,POKERS*b,POKERS*c);//c=A-b
int calc_controls(POKERS*,POKERS*, int);
int is_input_combo(int * p , COMBO_OF_POKERS* c);

int robot_play(GAME_DDZ_AI * game, POKERS * h, POKERS * opps,COMBO_OF_POKERS* pre, COMBO_OF_POKERS * cur,bool first);
int remove_combo_poker(POKERS* hand, COMBO_OF_POKERS * h, COMBO_OF_POKERS *h1  ); // h must be contained by hand.
void game_init(GAME_DDZ_AI* game);
void combo_to_int_array(COMBO_OF_POKERS *cur,int * p,int *card);
void combo_to_int_array_hun(COMBO_OF_POKERS *cur,int * p,int *card, int hun);

#ifndef max_ddz
#define max_ddz(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min_ddz
#define min_ddz(a,b) ((a)<(b)?(a):(b))
#endif
#define CEIL_A(a) ( (a)>(Pa)?(Pa):(a))
#define DBG(A) A



void add_poker(POKERS* a, POKERS * b, POKERS * c); // c=a+b
int is_combo(POKERS *src, COMBO_OF_POKERS * c);
int is_series(POKERS *h,int number);
int is_doubleseries(POKERS *h);


int is_threeseries(POKERS *h);


int is_411(POKERS *h, COMBO_OF_POKERS *c );



int is_422(POKERS *h, COMBO_OF_POKERS *c );



int is_31(POKERS *h, COMBO_OF_POKERS *c );


int is_32(POKERS *h, COMBO_OF_POKERS *c );


int is_3311(POKERS *h, COMBO_OF_POKERS *c );



int is_3322(POKERS *h, COMBO_OF_POKERS *c );


int is_333111(POKERS *h, COMBO_OF_POKERS *c );


int is_333222(POKERS *h, COMBO_OF_POKERS *c );


int is_33331111(POKERS *h, COMBO_OF_POKERS *c );


int is_3333311111(POKERS *h, COMBO_OF_POKERS *c );
int is_33332222(POKERS *h, COMBO_OF_POKERS *c );



//FILE* logfile,*logfile2;

#define LORD_LEVEL_3   (0)
#define LORD_LEVEL_2     (0)
#define UPFARMER_LEVEL_3   (0)
#define UPFARMER_LEVEL_2     (0)
#define DOWNFARMER_LEVEL_3   (0)
#define DOWNFARMER_LEVEL_2     (0)
typedef struct
{
    GAME_DDZ_AI game;
	int level;

	FILE * logfile;
    int  card[3][21];
    int  pot[4];
    int  outs[1+21+4+1];
	int  laizi[5];
} LordRobot;


//初始化机器人，返回机器人指针
//参数 0不看其他玩家的牌  1看其他玩家的牌
LordRobot* createRobot(int,int);

int  setRobotLevel(LordRobot* robot,int level);

/*
参数为int[] 数组 ，表示为发到机器人手中的牌。牌值如下：
	3	4	5	6	7	8	9	10	J	Q	K	A	2
方	0	1	2	3	4	5	6	7	8	9	10	11	12
梅	13	14	15	16	17	18	19	20	21	22	23	24	25
红	26	27	28	29	30	31	32	33	34	35	36	37	38
黑	39	40	41	42	43	44	45	46	47	48	49	50	51
*/
void initCard(LordRobot* robot, int* robot_card, int* up_player_card, int* down_player_card);

//传出参数为本机器人叫分
//-1 表示不叫
int RobotcallScore(LordRobot* robot);

// 1 表示加倍，0 表示不加倍
int doubleGame(LordRobot* robot);

// bit 0-7:    0 表示当地主赢不了， 3表示赢概率比较大， 2 次之， 1 更次
// bit 8-15:  0 表示当上家农民赢不了， 3表示赢概率比较大， 2 次之， 1 更次
// bit 16-23: 0 表示当下家农民赢不了， 3表示赢概率比较大， 2 次之， 1 更次

int showCard(LordRobot* robot);


//  1表示抢地主，0 表示不抢
int forceLord(LordRobot* robot);

/*其他玩家出牌信息：
Player_number, 0: down player,
               1: up player.
参数同1、发牌，如果其他玩家未出牌，则传入null
*/
void getPlayerTakeOutCards(LordRobot* robot,int* poker,int* laizi, int player_number) ;


/* 5、其他玩家叫分信息：
Player_number, 0: down player,
               1: up player.
score,  分值, -1表示没叫
*/
void getPlayerCallScore(LordRobot* robot,int score, int player_number) ;

//void beLord(LordRobot* robot,int *pot,int player_number);

/*6、出牌：1表示pass，0表示出牌，牌组信息放在P里*/
int  takeOut(LordRobot* robot, int * p, int* laizi, int * delay = NULL);

//7、销毁机器人
void destoryRobot(LordRobot* robot);

void setLaizi(LordRobot* robot, int laizi);
void beLord(LordRobot* robot,int* robot_card,int player_number);
void dump_game_record(RECORD_DDZ_AI *rec);
void print_one_poker(int i);
void PRINTF(int a,const char* const b, ...);
int  int_2_poker(int i);
char poker_to_char(int i);

int RobotshowCard(LordRobot* robot);
void init_robot_setting(LordRobot* robot);
