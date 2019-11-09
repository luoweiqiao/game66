#pragma once
#ifndef __OGLordRbtAIClvDepd__	
#define __OGLordRbtAIClvDepd__
//#include <windows.h>
/***********************************************\
斗地主聪明版机器人依赖文件 : OGLordRbtClvDepd.h
提供OGLordRobotClever.h中所依赖的数据结构和基本方法
依赖<windows.h>
\***********************************************/

//===============================================
//! part Robot.h : 此部分copy自斗地主机器人MFC程序的Robot.h文件
#define ALLOW_SAME_KICKER 1

#define MAX_POKER_KIND 15
#define max_POKER_NUM 20
#define CONTROL_POKER_NUM 6
#define LEVEL 0

#define CONTROL_NUM(suit) ((suit)->max_control.single+(suit)->summary->extra_bomb_ctrl*10)
#define CONTROL_SUB_COMBO_NUM(suit)  ( (CONTROL_NUM(suit))-(10*(suit)->summary->combo_total_num))
#define CONTROL_SUB_COMBO_NUM_IS_GOOD(suit) ( CONTROL_SUB_COMBO_NUM(suit) > -20 )
#define CONTROL_NUM_EX(suit) ((suit)->max_control.single+(suit)->summary->extra_bomb_ctrl*10+(suit)->summary->nOffsetBigger)
#define CONTROL_SUB_COMBO_NUM_EX(suit)  ( (CONTROL_NUM_EX(suit))-(10*(suit)->summary->combo_total_num)- (suit)->summary->nOffsetComboNum)

enum poker_type {
    P3, //0
    P4,
    P5,
    P6,
    P7,
    P8,
    P9,
    P10,
    Pj,
    Pq,
    Pk,
    Pa,
    P2,
    LIT_JOKER,
    BIG_JOKER,     //14
};

typedef struct POKER
{
    char hands[MAX_POKER_KIND];
    char total;
    char begin;
    char end;
    char control;
    int hun;
} POKER;

typedef enum HANDS_TYPE {
    NOTHING_old,
    ROCKET=5,
	BOMB_old = 4,
    SINGLE_SERIES=111,
    PAIRS_SERIES=222,
    THREE_SERIES=333,
    THREE =3,
    PAIRS =2,
    SINGLE =1,
    THREE_ONE = 31,
    THREE_TWO = 32,
    //warning , plane is from 3311..

} HANDS_TYPE;

typedef struct COMBOHAND
{
    unsigned int type;
    char low; // the lowest poker
    char len; // length for the series
    char control; // is it a control
    char three_desc[5]; //for three
} COMBOHAND;

typedef struct
{
    char single;
    char pair;
    char series;
    char three;
    char bomb;
    char ctrl;
} CONTROLS;

typedef struct COMBOS_SUMMARY
{
    char extra_bomb_ctrl;
    char combo_total_num; //exclude biggest combos
    char real_total_num; // include  biggest combos
    char combo_typenum;
    char combo_with_2_controls;
    char control_num;
    char nOffsetComboNum;
    char nOffsetBigger;
    char singles_num;
    COMBOHAND * singles[MAX_POKER_KIND];
    char pairs_num;
    COMBOHAND * pairs[MAX_POKER_KIND];
    char three_num;
    COMBOHAND * three[MAX_POKER_KIND];
    char series_num;
    COMBOHAND * series[6];
    char series_detail[10];
    char bomb_num;
    COMBOHAND * bomb[6];
    char three_one_num;
    COMBOHAND * three_one[6];
    char three_two_num;
    COMBOHAND * three_two[6];
    char threeseries_one_num;
    COMBOHAND * threeseries_one[3];
    char threeseries_two_num;
    COMBOHAND * threeseries_two[3];
    char threeseries_num;       //3同张数量
    COMBOHAND * threeseries[3];
    char pairs_series_num;
    COMBOHAND * pairs_series[3];
    char biggest_num;
    COMBOHAND * biggest[20];                //冲锋套
    char not_biggest_num;
    COMBOHAND * not_biggest[20];
    char four_one_num;
    COMBOHAND * four_one[4];
    char four_two_num;
    COMBOHAND * four_two[3];
    char combo_smallest_single;
} COMBOS_SUMMARY;

enum {
	LORD_old,
	UPFARMER_old,
	DOWNFARMER_old,
};

typedef struct {
    bool has_series;
    bool has_double_series_begin;
    bool has_three_series_begin;
    bool has_bomb; //donot contact rocket.
    bool has_three;
} HAS_COMBOS_SUMMARY;

typedef struct
{
    POKER *h; // 当前玩家的手牌.
    POKER *opps;
    char oppUp_num,oppDown_num; //number of opps hands
    char lower_control_poker;
    char id; // 0 lord  1: up farmer, 3 down farmer
    char good_framer; //1:good
    bool need_research;
    COMBOHAND *combos;
    COMBOS_SUMMARY* summary;
    CONTROLS max_control; //max control for all perhaps combos
    POKER * Up;      //the poker of Up player has outputted
    POKER * Down; //the poker of Down player has outputted
    POKER * me;    //the poker of  cur player has outputted
    HAS_COMBOS_SUMMARY all; // to speed up..
    COMBOHAND Perhaps[20];
    bool oppSingleKing;//敌人有单王
    bool kingOut; //王出现过
    bool farmer_only_2; //农民报双
    int likeType[3]; //喜好牌
} SUITS;

typedef struct
{
    COMBOHAND h;
    char player;
} HISTORY_HAND;

typedef struct
{
    POKER p[4];
    HISTORY_HAND history[54];
    int lord;
    int winner;
    int bombed;
} RECORD;

typedef struct {
    char computer[3];
    int dapaikongzhi_is_biggest;
    int cus;
    char filename[100];
} GAME_SETTING;

typedef struct
{
    POKER *pot;
    POKER p[7];
    POKER opp[6];
    POKER all;
    int  lowest_bigPoker; // the first 6 biggest poker.
    char computer[3];
    int lord;
    COMBOHAND prepre,pre,cur;
    int prepre_playernum,pre_playernum,cur_playernum;
    SUITS * player_suit[3]; // for three playes....
    SUITS suits[6];  //for swapper
    COMBOHAND combos[6][100];
    COMBOS_SUMMARY combos_summary[6];
    RECORD rec;
    
    bool              bLord;
    SUITS              * self_suit;
} GAME;

typedef struct
{
    POKER p;
    POKER opp;
    POKER all;
    int  lowest_bigPoker; // the first 6 biggest poker.
    int lord;
    COMBOHAND prepre,pre,cur;
    int prepre_playernum,pre_playernum,cur_playernum;

    SUITS suits;  //for swapper
    COMBOHAND combos[100];
    COMBOS_SUMMARY combos_summary;

    RECORD rec;    //can be del for optimized

    bool              bLord;
    SUITS              * self_suit;

    char playerCounter; //记出牌过的轮数
    char firstPlayer; //主动出牌的玩家
} GAMEEx;

#define CEIL_A(a) ( (a)>(Pa)?(Pa):(a))

// part Robot.h end
//===============================================
#endif