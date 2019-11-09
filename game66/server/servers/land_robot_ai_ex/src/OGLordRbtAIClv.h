#pragma once
#ifndef __OGLordRbtAIClv__	
#define __OGLordRbtAIClv__
#include <vector>
#include "OGLordRbtAIClvDepd.h"
#include "ddz_interface.h"

/***********************************************\
斗地主聪明版机器人 : COGLordRbtClv
COGLordRbtClv 实现了COGLordRobotInterface, 提供了游戏机器人AI逻辑,是比较聪明的版本.
\***********************************************/

class COGLordRbtAIClv
{
    enum RBT_ERR // deprecated, invalid
    {
        RBT_ERR_NOERR = -1,            // 无错误
        RBT_ERR_ERRSEAT,               // 座位号错误
        RBT_ERR_RECORED_UNMATCH,       // 出牌记录不匹配
        RBT_ERR_RECORED_SEAT,          // 出牌记录-座位错误
        RBT_ERR_RECORED_CARD,          // 出牌记录-牌错误
        RBT_ERR_SETCARD_HAND,          // 设置牌-手牌错误
        RBT_ERR_SETCARD_RECEIVE,       // 设置牌-底牌错误
        RBT_ERR_TAKEOUT_CARD,          // 收到出牌-牌错误
    };
    
public:
    COGLordRbtAIClv();
    ~COGLordRbtAIClv();
 
// 【对外接口】
public:
    // 收到发牌消息
    //************************************
    // Method:    RbtInInitCard
    // FullName:  COGLordRbtAIClv::RbtInInitCard
    // Access:    public 
    // Returns:   bool
    // Parameter: int argSeat, AI座位（0/1/2）
    // Parameter: std::vector<int> argHandCard, AI手牌(size is 17)
    //************************************
    bool RbtInInitCard(int argSeat, std::vector<int> argHandCard);
    
    // 收到玩家叫分信息
    //************************************
    // Method:    RbtInCallScore
    // FullName:  COGLordRbtAIClv::RbtInCallScore
    // Access:    public 
    // Returns:   bool
    // Parameter: int argSeat, 其他玩家座位（0/1/2）
    // Parameter: int argCallScore, 玩家叫分(0/1/2/3)
    //************************************
    bool RbtInCallScore(int argSeat, int argCallScore);

    // 请求给出叫分策略
    //************************************
    // Method:    RbtOutGetCallScore
    // FullName:  COGLordRbtAIClv::RbtOutGetCallScore
    // Access:    public 
    // Returns:   bool
    // Parameter: int & argCallScore, AI叫分(0/1/2/3)
    // Parameter: int & delay
    //************************************
    bool RbtOutGetCallScore(int &argCallScore, int &delay);

    // 收到确定地主信息
    //************************************
    // Method:    RbtInSetLord
    // FullName:  COGLordRbtAIClv::RbtInSetLord
    // Access:    public 
    // Returns:   bool
    // Parameter: int argLordSeat, 地主座位（0/1/2）
    // Parameter: std::vector<int> argReceiceCard, 底牌(size is 3)
    //************************************
    bool RbtInSetLord(int argLordSeat, std::vector<int> argReceiceCard);

    // 收到玩家牌信息
    //************************************
    // Method:    RbtInTakeOutCard
    // FullName:  COGLordRbtAIClv::RbtInTakeOutCard
    // Access:    public 
    // Returns:   bool
    // Parameter: int argSeat, 玩家座位（0/1/2）
    // Parameter: std::vector<int> argCards, 玩家打出的牌（PASS为空）
    //************************************
    bool RbtInTakeOutCard(int argSeat, std::vector<int> argCards);

    // 请求给出出牌策略
    //************************************
    // Method:    RbtOutGetTakeOutCard
    // FullName:  COGLordRbtAIClv::RbtOutGetTakeOutCard
    // Access:    public 
    // Returns:   bool
    // Parameter: std::vector<int> & argCards, AI打出的牌（PASS为空）
    // Parameter: int & delay
    //************************************
    bool RbtOutGetTakeOutCard(std::vector<int> &argCards, int &delay);
    
    // 重置机器人信息
    bool RbtResetData();

    // 设置地主和玩家座位
    bool RbtInSetSeat(int argMySeat, int argLordSeat);

    // 设置手牌和底牌
    bool RbtInSetCard(std::vector<int> argInitCard, std::vector<int> argReceiveCard);

    // 设置出牌记录
    bool RbtInTakeOutRecord(std::vector<int> argTakeOutSeat ,std::vector<std::vector<int>> argTakeOutRecord);

    // 设置机器人类别
    //************************************
    // Method:    RbtInSetLevel
    // FullName:  COGLordRbtAIClv::RbtInSetLevel
    // Access:    public 
    // Returns:   bool
    // Parameter: int argLevel， 1为暗牌
    //************************************
    bool RbtInSetLevel(int argLevel);

    // 通知机器人所有玩家手牌
    bool RbtInNtfCardInfo(std::vector<std::vector<int>> argHandCard);

    // 输入抢地主信息
    bool RbtInSetGrabLord(int argSeat);

    // 输出抢地主策略
    bool RbtOutGetGrabLord(bool &grabLord, int &delay);

    // 新版AI不支持的接口，返回true
    bool RbtOutGetLastError(int &errorCode);

    // 输入玩家明牌信息 
    bool RbtInShowCard(int argShowSeat, std::vector<int> argHandCard);

    // 输出明牌策略 
    bool RbtOutShowCard(bool &showCard, int &delay);

    // 输出加倍策略 
    bool RbtOutDoubleGame(bool &Double_game, int &delay);

    // 输入机器人延时时间
    bool RbtInSetTimeDelay(int timeDelay);

#pragma region NON-INTERFACE
public: //for old C version robot    
    LordRobot* robot;
    LordRobot m_robot;
    int lord_score;
    int LaiZicard;
    int level;
    int errCode;
    int aiSeat;
    int lordSeat;
    int downSeat;
    int upSeat;
    int debug_level;

    int maxScore;

private:
    int m_nSeat;
    int m_nLastPlayerNum;
    int m_nRemainNum[3];
    int m_nLord;
    std::vector<int> m_handCard;    
    GAMEEx *game;
    GAMEEx m_game;
    bool m_bRobot;
    int good_farmer_0;
    int good_farmer_1;

    int m_nLastErrorCode;

private:
    void send_poker(std::vector<int> &curCard);
    void SetSeat(int nSeat);
    bool IsCallLord();
    void SetLord(int nNum);
    void send_pot(std::vector<int> &curPot);
    int takeout_poker(std::vector<int> &pOut);
    void OnMesTakeOutCard(std::vector<int> &curCard, int nPlayerNum);
    void SetCurPlayer(int nNum);

    // 以后可以用于盘中更换机器人 
    // 1. 收到过去的出牌记录
    void send_takeout_card(std::vector<int> &takeCard);
    // 2. 收到玩家剩余的牌数量
    void init_playercard_num(std::vector<int> &cardNum);
private:
    void ResetData();
    void SetSuit(SUITS *suit);

private:
    void OnTakeOut(int nPlayerNum, COMBOHAND *cur); //响应出牌消息
    int select_a_combo(COMBOHAND *cur); //select a combo and save it to cur
    int is_game_first_half(int player);
    int lord_select_a_combo_2(SUITS *suit, COMBOHAND *cur, COMBOHAND *pre); //select a combo from suit and save to cur.
    void lord_select_a_combo(SUITS *suit, COMBOHAND *c); //select c from suit
    void farmer_play_first_lord_has_1_poker_only( COMBOHAND* cur,int is_good_farmer, SUITS*suit,  int isup);    
    void upfarmer_play_first( COMBOHAND* cur,int is_good_farmer, SUITS*suit);//select a combo from hands, save to cur
    void downfarmer_play_first( COMBOHAND* cur,int is_good_farmer, SUITS*suit);
    int must_play_for_poker2( COMBOHAND * cur, SUITS* suit,COMBOHAND *pre, int check_opps_lit_joker);
    void up_good_farmer_play_first( SUITS * suit ,COMBOHAND* cur);//select cur from suit
    void down_good_farmer_play_first( SUITS * suit ,COMBOHAND* cur);//select cur from suit

    // search in suit to get a bigger one than pre
    // 1. search in summary first
    // 2. search in hands, if find one, re-arrange hands and save the combo summary to suit->summary
    //save result to cur
    int must_play_bigger( COMBOHAND * cur, SUITS* suit,COMBOHAND *pre,int isup, int isgood,int isfarmer);

    int find_a_bigger_combo_in_suit( SUITS* suit,COMBOHAND * cur, COMBOHAND* a);
    //地主报单农民必管逻辑
    int must_play_bigger_for_single( COMBOHAND * cur, SUITS* suit,COMBOHAND *pre,int isup, int isgood,int isfarmer);
    int farmer_shunpai( SUITS * suit ,COMBOHAND* cur, COMBOHAND* pre);
    int farmer_play_when_lord_has_1_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit,  int isup,int isgood);
    //地主报双
    int farmer_play_first_lord_has_2_poker_only(COMBOHAND* cur,int is_good_farmer, SUITS*suit, int isup);
    int farmer_play_when_lord_has_2_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit,  int isup,int isgood);
    //报双主动出牌逻辑
    int play_first_opp_has_2_poker_only(COMBOHAND* cur,int is_good_farmer, SUITS*suit, int isup);
    int lord_play_when_famer_has_2_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit);

    //不顺牌的逻辑
    COMBOHAND* unComfortPoker(SUITS * suit,int is_good_famer,int isUp);
    // select a combo form suit
    COMBOHAND* lord_select_combo_in_suit(COMBOS_SUMMARY* s,SUITS* suit);
    //返回1表示pass , 农民管队友
    int farmer_select_after_farmer(SUITS * suit ,COMBOHAND* cur, COMBOHAND* pre);

    GAMEEx* GetGame();
    SUITS* GetSuit();

    //去掉本手自己出的牌，不需要转换牌值
    void RemoveCard(std::vector<int> &curCard);
    void VectorToPoker(std::vector<int> &curCard, POKER *h);
    void prase_poker(std::vector<int> &ParaVec);
    void search_poker(std::vector<int> &ParaVec, COMBOHAND *h, std::vector<int> &pOut);
private:
    char poker_to_char(int i);
    int char_to_poker(char c);
    int sort_poker(POKER *h);
    void read_poker(char *buf, POKER *h);
    int remove_combo_poker(POKER* hand, COMBOHAND * h, COMBOHAND *h1  ); // h must be contained by hand.
    void add_poker(POKER *a, POKER *b, POKER *c); // c=a+b
    void sub_poker(POKER *a, POKER *b, POKER *c); // c=a-b
    int is_sub_poker(POKER *a, POKER *b); // a in b ? 1 : 0
    bool getBomb(POKER *h, COMBOHAND *p) ;
    bool getThree(POKER* h, COMBOHAND *p) ;
    bool getThreeSeries(POKER* h, COMBOHAND *p);
    bool getDoubleSeries(POKER* h, COMBOHAND *p);
    bool updateDoubleSeries(POKER* h, COMBOHAND *p);
    bool updateDoubleSeries1(POKER* h, COMBOHAND *p);
    bool getSeries(POKER *h, COMBOHAND *p);
    bool getBigBomb(POKER *h, COMBOHAND *p, COMBOHAND *a);
    bool getBigThree(POKER *h, COMBOHAND *p, COMBOHAND *a);
    bool getBigSingle(POKER *h, COMBOHAND *p, int start, int end, int number);
    bool getBigSeries(POKER *h, COMBOHAND *p, int start, int end, int number, int len);
    bool getSingleSeries(POKER *h, COMBOHAND *p, int start, int end, int number);
    int is_series(POKER *h,int number);
    int is_doubleseries(POKER *h);
    int is_threeseries(POKER *h);
    int is_411(POKER *h, COMBOHAND *c);
    int is_422(POKER *h, COMBOHAND *c);
    int is_31(POKER *h, COMBOHAND *c);
    int is_32(POKER *h, COMBOHAND *c);
    int is_3311(POKER *h, COMBOHAND *c);
    int is_3322(POKER *h, COMBOHAND *c);
    int is_333111(POKER *h, COMBOHAND *c);
    int is_333222(POKER *h, COMBOHAND *c);
    int is_33331111(POKER *h, COMBOHAND *c);
    int is_3333311111(POKER *h, COMBOHAND *c);
    int is_33332222(POKER *h, COMBOHAND *c);

    // todo: add a parma number in COMBOHAND
    int get_combo_number(COMBOHAND *h);
    int is_combo(POKER *h, COMBOHAND * c);

    // 检查本手牌是否最大
    bool is_combo_biggest(POKER *opp, COMBOHAND *c , int opp1_num, int opp2_num, int lower);
    int check_combo_a_Big_than_b(COMBOHAND *a, COMBOHAND *b);
    int get_lowest_controls(POKER *h, int number);
    int calc_controls(POKER *h, POKER *opp ,int number);
    int browse_pokers(POKER *h, COMBOHAND *pCombos);
    int search222inSingleSeries(POKER *h, COMBOHAND *p, COMBOHAND *s);
    int  search234inSingleSeries(POKER *h, COMBOHAND *p, COMBOHAND *s);
    int  searchMultiSingleSeries(POKER *h, COMBOHAND *p); //tobe optimized
    int search_general_1(POKER *h , COMBOHAND *pCombos,
        bool skip_bomb, bool skip_double, bool skip_three, bool skip_series);
    int search_general_2(POKER *h , COMBOHAND *pCombos,
        bool skip_bomb, bool skip_double, bool skip_three, bool skip_series);
    int search_general_3(POKER* h , COMBOHAND *pCombos,
        bool skip_bomb,bool skip_double, bool skip_three, bool skip_series);
    // 1. bomb 2. three 3. straight 4. else
    int search_general_4(POKER *h , COMBOHAND *pCombos,
        bool skip_bomb, bool skip_double, bool skip_three, bool skip_series);
    COMBOHAND* find_max_len_in_combos(COMBOHAND *combos, int total);
    COMBOHAND* find_biggest_in_combos(COMBOHAND *combos, int total);
    // todo: you could sort not_biggest first...
    COMBOHAND*  find_smallest_in_combos(COMBOHAND* com, int total,SUITS* suit ,bool not_search_single);
    int  find_combo_with_3_controls_in_combos(COMBOHAND* com, int total,SUITS* suit);
    int get_control_poker_num_in_combo(COMBOHAND* c, int lower);
    // to be optimized
    // current for
    void sort_all_combos(COMBOHAND *c , int total, COMBOS_SUMMARY *s,
        POKER *opp, int opp1, int opp2, /* for check biggest*/
        int lower  /*for check control,not used now*/
        );
    // update summary of combos
    void update_summary(COMBOS_SUMMARY *s, POKER *h, POKER *opp, COMBOHAND *c,int total, int opp1, int opp2, int lower);
    int cmp_summary_for_min_single_farmer( COMBOS_SUMMARY *a, COMBOS_SUMMARY *b) ;//return 1 if a is better than b
    /*
    i.    控制 - 套的数目 最少。
    ii.    套的种类最少
    iii.    单牌数量最少
    iv.    最小的单牌最大
    */
    int cmp_summary(COMBOS_SUMMARY *a, COMBOS_SUMMARY *b); //return a>b
    int search_combos_in_hands(POKER *h ,COMBOHAND *pCombos, COMBOS_SUMMARY *pSummary, SUITS *suit);
    int get_2nd_min_singles(COMBOS_SUMMARY *s);
    // 地主报单农民组牌方式
    int search_combos_in_suits_for_min_single_farmer(POKER* h ,COMBOHAND *pCombos, SUITS *suit);
    int search_combos_in_suit( POKER *h , POKER *opp, SUITS *suit);
    // get a single or pair for  three
    // and update the summary
    int search_1_2_for_a_three(COMBOHAND *p ,POKER *h, SUITS *suit,COMBOHAND *c);
    // search in poker h to find a combo c bigger than a
    // do not search bombs for non-bomb combo hand
    int find_a_bigger_combo_in_hands(SUITS *suit, POKER *h, COMBOHAND *c, COMBOHAND *a);
    // a stupid fucntion...
    // select  a combo c from suit->h, but not remove
    // arrange the other poker in suit->h to combos and save summary to suit->summary
    int    rearrange_suit(SUITS *suit, COMBOHAND *c);
    // Remove combo a from the suit s
    // return true if success
    int remove_combo_in_suit(COMBOS_SUMMARY *s , COMBOHAND *a);
    COMBOHAND* find_combo(COMBOS_SUMMARY *s, COMBOHAND *a);   //find c > a in s and return c.
    int has_control_poker(COMBOHAND* c, int lower);
    // remove 1 or 2 pokers from opps hand
    void assume_remove_controls_in_opps(POKER *opps, COMBOHAND *c, int begin);
    int check_poker_suit(POKER *hand ,SUITS *suit);
    int find_max(COMBOS_SUMMARY *s);
    int rand_a_poker(POKER *all);
    void full_poker(POKER *h);
    //find and remove a joker in combos, 31 3311... 411
    int find_joker_in_combos(SUITS *suit, int joker);
    // 转换牌值
    int card_to_poker(int nCard);
    // 是否是最大的炸弹
    bool isBiggestBomb(int p, POKER* opp);
    // 外面存在的最多炸弹数
    int maxBombCount(POKER* opp);
    void checkKicker(COMBOHAND *hand, COMBOS_SUMMARY *summary);
    // 判断强合手
    int isStrongGood(SUITS* suit);
    // 判断危险区
    int isDangous(SUITS* suit);
    // 将冲锋套或非冲锋套整理为标签从小到大
    int orderBiggest(COMBOHAND *c, int num);
#pragma endregion
};
#endif