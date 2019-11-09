#include "CardsEvaluating.h"
#include "ddz_interface.h"
#include "OGLordRbtAIClv.h"
#include "OGLordRobot.h"
#include <algorithm>
#include <functional>
#include <iterator>

#define STATISTICS_EVALUATING

#ifndef _WIN32
#undef STATISTICS_EVALUATING
#endif

#if defined(STATISTICS_EVALUATING) && defined(_WIN32)
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

static std::ofstream fout;
static const char * statistics_file = "./statistics.log";
#endif

#define NOT_BOMB(pcombo)  ((pcombo)->type != BOMB_old && (pcombo)->type != ROCKET)

using namespace std;

static const int PLAYER_NUM = 3;

extern int search_combos_in_suit(GAME_DDZ_AI* game, POKERS *h, POKERS *opp, PLAYER* player);
extern int find_a_bigger_combo_in_hands(POKERS* h, COMBO_OF_POKERS * bigger, COMBO_OF_POKERS* combo);
extern int Check_win_quick(GAME_DDZ_AI * gm, int firstplayer_num, bool cur, bool up, bool down);

#define  STATIC_UTIL_FUNC static

STATIC_UTIL_FUNC int simulated_play(std::vector<std::vector<int>> cards, int lord_seat);
STATIC_UTIL_FUNC double evaluate_on_same_type(const GAME_DDZ_AI* game, COMBO_OF_POKERS *combos[], int combos_num);
STATIC_UTIL_FUNC double evaluate_on_same_type(const GAME_DDZ_AI* game, COMBO_OF_POKERS *combos[], int combos_num, const int min_len, const int max_len);
STATIC_UTIL_FUNC double score_cards_core(const GAME_DDZ_AI* game);

#if defined(STATISTICS_EVALUATING) && defined(_WIN32)
static int trans(int id)
{
    if (id == 53)
    {
        return BIG_JOKER;
    }
    else if (id == 52)
    {
        return LIT_JOKER;
    }
    else
    {
        return id % (MAX_POKER_KIND - 2);
    }
}

int get_bombs_num(std::vector<int> cards)
{
    transform(cards.begin(), cards.end(), cards.begin(), trans);
    sort(cards.begin(), cards.end());
    int bombs_num = 0;
    for (int i = 0, end = cards.size(); i < end - 4; )
    {
        bool found_bomb = true;
        for (int j = i + 1; j < i + 4; ++j)
        {
            if (cards[j] != cards[i])
            {
                found_bomb = false;
                i = j;
                break;
            }
        }

        if (found_bomb)
        {
            ++bombs_num;
            i += 4;
        }
    }

    // 大小王
    if (cards.size() >= 2 && cards.back() == BIG_JOKER && *(cards.end() - 2) == LIT_JOKER)
    {
        ++bombs_num;
    }
    return bombs_num;
}

void print_cards(std::vector<int> cards, std::ostream &os)
{
    transform(cards.begin(), cards.end(), cards.begin(), trans);
    sort(cards.begin(), cards.end());
    
    static const string poker_str = "3456789TJQKA2XD";

    string s;
    for (int i = 0, end = cards.size(); i < end; ++i)
    {
        s.push_back(poker_str.at(cards[i]));
    }
    os << s;
}
#endif // STATIC_UTIL_FUNC

int simulated_play(std::vector<std::vector<int>> cards, int lord_seat)
{
    // init robot
    OGLordRobot robots[PLAYER_NUM];
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        sort(cards[i].begin(), cards[i].end());
        robots[i].RbtResetData();
        robots[i].RbtInSetLevel(BEST_LEVEL_2);
        robots[i].RbtInInitCard(i, cards[i]);
        robots[i].RbtInNtfCardInfo(cards);
        
    }

    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        robots[i].RbtInSetLord(lord_seat, cards.back());
    }

    vector<int> out_cards;
    int winner = -1;
    for (int player = lord_seat, round = 0; round < 100; player = (player + 1) % PLAYER_NUM) //TODO
    {
        out_cards.clear();
        robots[player].RbtOutGetTakeOutCard(out_cards);
        if (!out_cards.empty())
        {
            sort(out_cards.begin(), out_cards.end());
            vector<int> diff;
            set_difference(cards[player].begin(), cards[player].end(), out_cards.begin(), out_cards.end(), inserter(diff, diff.begin()));
            cards[player].swap(diff);

            //cout << "PLAYER " << player << ": ";
            //print_cards(out_cards, cout);
            //cout << endl;

            if (cards[player].empty()) // TODO: check win
            {
                winner = player;
                break;
            }
        }
        robots[player].RbtInTakeOutCard(player, out_cards);
        robots[(player + 1) % PLAYER_NUM].RbtInTakeOutCard(player, out_cards);
        robots[(player + 2) % PLAYER_NUM].RbtInTakeOutCard(player, out_cards);
    }
    //cout << "PLAYER " << winner << " WIN." << endl << endl;
    return winner;
}

double evaluate_on_same_type(const GAME_DDZ_AI* game, COMBO_OF_POKERS *combos[], int combos_num)
{
    if (combos_num <= 0)
    {
        return 0;
    }

    int max_combos_num = 0;

    // 考察比最大套刚好小一个点的牌套，用来检查是否绝大套牌
    COMBO_OF_POKERS combo_tmp = *combos[combos_num - 1];
    
    bool is_biggest = false;
    bool is_unique_biggest = false;
    if (game->players[CUR_PLAYER]->id == LORD_old)
    {
        COMBO_OF_POKERS down_bigger_combo;
        COMBO_OF_POKERS up_bigger_combo;

        is_biggest = (!find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &down_bigger_combo, &combo_tmp) || !NOT_BOMB(&down_bigger_combo))
            && (!find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &up_bigger_combo, &combo_tmp) || !NOT_BOMB(&up_bigger_combo));

        combo_tmp.low -= 1;
        down_bigger_combo.type = NOTHING_old;
        up_bigger_combo.type = NOTHING_old;
        is_unique_biggest = (!find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &down_bigger_combo, &combo_tmp) || !NOT_BOMB(&down_bigger_combo))
            && (!find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &up_bigger_combo, &combo_tmp) || !NOT_BOMB(&up_bigger_combo));

        if (is_unique_biggest) // 有绝大套牌, is_unique_biggest为true->is_biggest为true
        {
            ++max_combos_num; // 最大一套已经考察过
            // 从第二大的开始
            for (int i = combos_num - 2; i > 0; --i)
            {
                if ((!find_a_bigger_combo_in_hands(game->players[DOWN_PLAYER]->h, &down_bigger_combo, combos[i]) || !NOT_BOMB(&down_bigger_combo))
                    && (!find_a_bigger_combo_in_hands(game->players[UP_PLAYER]->h, &up_bigger_combo, combos[i]) || !NOT_BOMB(&up_bigger_combo)))
                {
                    ++max_combos_num;
                    down_bigger_combo.type = NOTHING_old;
                    up_bigger_combo.type = NOTHING_old;
                }
                else
                {
                    break;
                }
            }
        }
        else if (is_biggest)
        {
            max_combos_num = 0.5;
        }
    }
    else
    {
        COMBO_OF_POKERS opp_bigger_combo;
        is_biggest = !find_a_bigger_combo_in_hands(game->players[CUR_PLAYER]->opps, &opp_bigger_combo, &combo_tmp) || !NOT_BOMB(&opp_bigger_combo);
        
        combo_tmp.low -= 1;
        opp_bigger_combo.type = NOTHING_old;
        is_unique_biggest = !find_a_bigger_combo_in_hands(game->players[CUR_PLAYER]->opps, &opp_bigger_combo, &combo_tmp) || !NOT_BOMB(&opp_bigger_combo);

        if (is_unique_biggest) // 有绝大套牌, is_unique_biggest为true->is_biggest为true
        {
            ++max_combos_num; // 最大一套已经考察过
            // 从第二大的开始
            for (int i = combos_num - 2; i > 0; --i)
            {
                if (!find_a_bigger_combo_in_hands(game->players[CUR_PLAYER]->opps, &opp_bigger_combo, &combo_tmp) || !NOT_BOMB(&opp_bigger_combo))
                {
                    ++max_combos_num;
                    opp_bigger_combo.type = NOTHING_old;
                }
                else
                {
                    break;
                }
            }
        }
        else if (is_biggest)
        {
            max_combos_num = 0.5;
        }
    }

    
    return max(combos_num - max_combos_num * 2, 0);
}

double evaluate_on_same_type(const GAME_DDZ_AI* game, COMBO_OF_POKERS *combos[], int combos_num, const int min_len, const int max_len)
{
    // 按照长度分类
    vector<vector<COMBO_OF_POKERS*>> combos_classified_by_len(max_len - min_len + 1);
    for (int i = 0; i < combos_num; ++i)
    {
        combos_classified_by_len[combos[i]->len - min_len].push_back(combos[i]);
    }

    double score = 0;
    for (int i = 0; i < max_len - min_len + 1; ++i)
    {
        if (!combos_classified_by_len[i].empty())
        {
            score += evaluate_on_same_type(game, combos_classified_by_len[i].data(), combos_classified_by_len[i].size());
        }
    }
    return score;
}

// 返回评价得分,自己是地主
double score_cards_core(const GAME_DDZ_AI* game)
{
    PLAYER *self = game->players[CUR_PLAYER];
    COMBOS_SUMMARY_DDZ_AI* summary = self->summary;

    // 单
    double single = evaluate_on_same_type(game, summary->singles, self->summary->singles_num);
    // 对
    double pair_ = evaluate_on_same_type(game, summary->pairs, self->summary->pairs_num);
    // 三带
    double three = evaluate_on_same_type(game, summary->three, summary->three_num);
    // 三带一
    double three_one = evaluate_on_same_type(game, summary->three_one, summary->three_one_num);
    // 三带二
    double three_two = evaluate_on_same_type(game, summary->three_two, summary->three_two_num);
    // 单顺
    double single_series = evaluate_on_same_type(game, summary->series, summary->series_num, 5, 12);
    // 连对
    double pair_series = evaluate_on_same_type(game, summary->pairs_series, summary->pairs_series_num, 3, 10);
    // 飞机
    double airplane = evaluate_on_same_type(game, summary->threeseries, summary->threeseries_num, 2, 6);
    double airplane_one = evaluate_on_same_type(game, summary->threeseries_one, summary->threeseries_one_num, 2, 5);
    double airplane_two = evaluate_on_same_type(game, summary->threeseries_two, summary->threeseries_two_num, 2, 4);
    // 四带单
    double four_one = evaluate_on_same_type(game, summary->four_one, summary->four_one_num);
    // 四带对
    double four_two = evaluate_on_same_type(game, summary->four_two, summary->four_two_num);

    // 需要顺走的牌
    double combo_score = -1.0 * (single + pair_ + three + three_one + three_two
        + single_series + pair_series + airplane + airplane_one + airplane_two + four_one + four_two);

    // 炸弹(特殊处理)
    PLAYER *down_player = game->players[DOWN_PLAYER];
    PLAYER *up_player = game->players[UP_PLAYER];

    vector<int> my_bombs;
    for (int i = 0; i < summary->bomb_num; ++i)
    {
        my_bombs.push_back(summary->bomb[i]->low);
    }

    vector<int> opp_bombs;
    if (self->id == DOWNFARMER_old)
    {
        down_player = NULL;
    }
    else if (self->id == UPFARMER_old)
    {
        up_player = NULL;
    }
          
    for (int k = 0; k < MAX_POKER_KIND - 2; ++k)
    {
        if ((down_player && down_player->h->hands[k] == 4) 
            || (up_player && up_player->h->hands[k] == 4))
        {
            opp_bombs.push_back(k);
        }
    }
    if ((down_player && down_player->h->hands[LIT_JOKER] == 1 && down_player->h->hands[BIG_JOKER] == 1)
        || (up_player && up_player->h->hands[LIT_JOKER] == 1 && up_player->h->hands[BIG_JOKER] == 1))
    {
        opp_bombs.push_back(LIT_JOKER);
    }
    

    int my_biggest_bombs_num = 0;
    if (!opp_bombs.empty())
    {
        vector<int>::iterator pos = find_if(my_bombs.begin(), my_bombs.end(), bind1st(std::greater<int>(), opp_bombs.back()));
        my_biggest_bombs_num = my_bombs.end() - pos;
    }
    else
    {
        my_biggest_bombs_num = my_bombs.size();
    }

    // 注意计算时.size()返回的是size_t类型，一般为unsigned int, 负数注意溢出
    double bombs_score = 2.0 * (my_bombs.size() + 0.5 * my_biggest_bombs_num - opp_bombs.size());

    return bombs_score + combo_score + (-0.5 * (summary->real_total_num - my_bombs.size()));
}

double score_my_cards(const std::vector<std::vector<int>> &cards, int lord_seat, int my_seat)
{
    double score = -100; // 初始设置-100，避免出错得分比正常计算得分高
    COGLordRbtAIClv ai_robot;
    ai_robot.RbtInSetLevel(2);
    ai_robot.RbtInInitCard(my_seat, cards[my_seat]);
    ai_robot.RbtInNtfCardInfo(cards);
    ai_robot.RbtInSetLord(lord_seat, cards.back());
    // 拆分
    search_combos_in_suit(&ai_robot.robot->game,
        ai_robot.robot->game.players[CUR_PLAYER]->h,
        ai_robot.robot->game.players[CUR_PLAYER]->opps,
        ai_robot.robot->game.players[CUR_PLAYER]);
    score = score_cards_core(&ai_robot.robot->game);
    return score;
}

std::vector<int> filter_by_scores(vector<double> scores)
{
    vector<int> check_seats; // 按优先级push
    double max_score = -100;
    for (; check_seats.size() < scores.size();)
    {
        vector<double>::iterator pos = max_element(scores.begin(), scores.end());
        if (pos == scores.end() || *pos < max_score - 0.6)
        {
            break;
        }
        check_seats.push_back(pos - scores.begin());
        *pos = -100;
    }
    return check_seats;
}

int comp_cards(const std::vector<std::vector<int>> &cards, const vector<int> &check_seats)
{
    if (check_seats.size() > 1)
    {
        // 模拟
        vector<int> check_results(check_seats.size(), 0);
        for (int i = 0; i < check_seats.size(); ++i)
        {
            int lord_seat = check_seats[i];
            int winner = simulated_play(cards, lord_seat);
            check_results[i] += (winner == lord_seat) ? 1 : 0;

            // 交换农民手牌
            vector<vector<int>> exchanged_cards = cards;
            std::swap(exchanged_cards[(lord_seat + 1) % PLAYER_NUM],
                exchanged_cards[(lord_seat + 2) % PLAYER_NUM]);
            int winner_exchange = simulated_play(exchanged_cards, lord_seat);
            if (winner_exchange != lord_seat)
            {
                winner_exchange = (lord_seat + 1) % PLAYER_NUM + (lord_seat + 2) % PLAYER_NUM - winner_exchange;
            }
            check_results[i] += (winner_exchange == lord_seat) ? 0.8 : 0; // TODO
        }

        vector<int>::iterator pos = max_element(check_results.begin(), check_results.end());
        if (pos != check_results.end())
        {
            return check_seats[pos - check_results.begin()];
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return check_seats.empty() ? -1 : check_seats.front();
    }
}

int evaluate_cards(const std::vector<std::vector<int>> &cards)
{
    // 
    vector<double> scores(PLAYER_NUM, 0);
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        scores[i] = score_my_cards(cards, i, i);
    }
    //COGLordRbtAIClv rbt[PLAYER_NUM];
    //for (int i = 0; i < PLAYER_NUM; ++i)
    //{
    //    rbt[i].RbtInSetLevel(2);
    //    rbt[i].RbtInInitCard(i, cards[i]);
    //    rbt[i].RbtInNtfCardInfo(cards);
    //    vector<int> my_cards = cards[i];
    //    my_cards.insert(my_cards.end(), cards.back().begin(), cards.back().end());
    //    rbt[i].RbtInSetLord(i, cards.back());
    //    // 拆分
    //    search_combos_in_suit(&rbt[i].robot->game,
    //        rbt[i].robot->game.players[CUR_PLAYER]->h,
    //        rbt[i].robot->game.players[CUR_PLAYER]->opps,
    //        rbt[i].robot->game.players[CUR_PLAYER]);
    //    scores[i] = score_cards_core(&rbt[i].robot->game);
    //}

#ifdef STATISTICS_EVALUATING
    fout.open(statistics_file, ios::app);

    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        print_cards(cards[i], fout);
        fout << ", ";
    }
    if (cards.size() > PLAYER_NUM)
    {
        print_cards(cards[PLAYER_NUM], fout);
        fout << ',';
    }
    char buf[1024] = {0};

    // 玩家当地主时的手牌
    vector<vector<int>> lord_cards = cards;
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        lord_cards[i].insert(lord_cards[i].end(), cards.back().begin(), cards.back().end());
    }

    // 记录炸弹数
    ::sprintf(buf, "%d, %d, %d, %d, %d, %d,",
        get_bombs_num(cards[0]), get_bombs_num(lord_cards[0]),
        get_bombs_num(cards[1]), get_bombs_num(lord_cards[1]),
        get_bombs_num(cards[2]), get_bombs_num(lord_cards[2]));

    // 计算玩家在农民身份时的分数
    vector<double> scores_1(PLAYER_NUM, 0);
    COGLordRbtAIClv rbt_1[PLAYER_NUM];
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        rbt_1[i].RbtInSetLevel(2);
        rbt_1[i].RbtInInitCard(i, cards[i]);
        rbt_1[i].RbtInNtfCardInfo(cards);
        vector<int> my_cards = cards[i];
        my_cards.insert(my_cards.end(), cards.back().begin(), cards.back().end());
        rbt_1[i].RbtInSetLord((i + 2) % PLAYER_NUM, cards.back()); // 上家地主
        // 拆分
        search_combos_in_suit(&rbt_1[i].robot->game,
            rbt_1[i].robot->game.players[CUR_PLAYER]->h,
            rbt_1[i].robot->game.players[CUR_PLAYER]->opps,
            rbt_1[i].robot->game.players[CUR_PLAYER]);
        scores_1[i] = score_cards_core(&rbt_1[i].robot->game);
    }

    vector<double> scores_2(PLAYER_NUM, 0);
    COGLordRbtAIClv rbt_2[PLAYER_NUM];
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        rbt_2[i].RbtInSetLevel(2);
        rbt_2[i].RbtInInitCard(i, cards[i]);
        rbt_2[i].RbtInNtfCardInfo(cards);
        vector<int> my_cards = cards[i];
        my_cards.insert(my_cards.end(), cards.back().begin(), cards.back().end());
        rbt_2[i].RbtInSetLord((i + 1) % PLAYER_NUM, cards.back()); // 上家地主
        // 拆分
        search_combos_in_suit(&rbt_2[i].robot->game,
            rbt_2[i].robot->game.players[CUR_PLAYER]->h,
            rbt_2[i].robot->game.players[CUR_PLAYER]->opps,
            rbt_2[i].robot->game.players[CUR_PLAYER]);
        scores_2[i] = score_cards_core(&rbt_2[i].robot->game);
    }

    // 记录各位置分数
    ::sprintf(buf, "%s %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,", buf,
        scores[0], scores_1[0], scores_2[0],
        scores[1], scores_1[1], scores_2[2],
        scores[2], scores_1[2], scores_2[2]);

    // 全位置模拟
    // TODO
    int winner_0[2] = {-1, -1};
    winner_0[0] = simulated_play(cards, 0);
    vector<vector<int>> cards_0 = cards;
    swap(cards_0[1], cards_0[2]);
    winner_0[1] = simulated_play(cards_0, 0);
    if (winner_0[1] != 0)
    {
        winner_0[1] = (1 + 2) - winner_0[1];
    }

    int winner_1[2] = {-1, -1};
    winner_1[0] = simulated_play(cards, 1);
    vector<vector<int>> cards_1 = cards;
    swap(cards_1[2], cards_1[0]);
    winner_1[1] = simulated_play(cards_1, 1);
    if (winner_1[1] != 1)
    {
        winner_1[1] = (0 + 2) - winner_1[1];
    }

    int winner_2[2] = {-1, -1};
    winner_2[0] = simulated_play(cards, 2);
    vector<vector<int>> cards_2 = cards;
    swap(cards_2[0], cards_2[1]);
    winner_2[1] = simulated_play(cards_2, 2);
    if (winner_2[1] != 2)
    {
        winner_2[1] = (0 + 1) - winner_2[1];
    }

    ::sprintf(buf, "%s %d, %d, %d, %d, %d, %d", buf,
        winner_0[0], winner_0[1], winner_1[0], winner_1[1], winner_2[0], winner_2[1]);

    fout << buf << flush;
    fout.close();
#endif
    // 比较最好牌
    double max_score = *max_element(scores.begin(), scores.end());
    vector<int> check_seats;
    for (int i = 0; i < PLAYER_NUM; ++i)
    {
        if (max_score - scores[i] <= 0.6 || (max_score <= -9.0)) // 分差数值
        {
            check_seats.push_back(i);
        }
    }

    if (check_seats.size() == 1)
    {
#ifdef STATISTICS_EVALUATING
        fout.open(statistics_file, ios::app);
        fout << ", " << check_seats.front() << endl;
        fout.close();
#endif 
        return check_seats.front();
    }
    else
    {
        int check_results[PLAYER_NUM] = {0};
        // 模拟
        for (int i = 0; i < PLAYER_NUM; ++i)
        {
            if (find(check_seats.begin(), check_seats.end(), i) == check_seats.end())
            {
                check_results[i] = -100; // 表示没有进行模拟
                continue;
            }

            int winner = simulated_play(cards, i);
            if (winner == i)
            {
                check_results[i] += 1;
            }

            // 交换2家农民手牌
            vector<vector<int>> exchanging_cards = cards;
            swap(exchanging_cards[(i + 1) % PLAYER_NUM], exchanging_cards[(i + 2) % PLAYER_NUM]);
            int winner_2 = simulated_play(exchanging_cards, i);
            if (winner_2 == i)
            {
                check_results[i] += 1;
            }
        }

        double final_scores[PLAYER_NUM] = {0};
        for (int i = 0; i < PLAYER_NUM; ++i)
        {
            final_scores[i] += check_results[i] * 100 + scores[i];
        }

        double *pos = max_element(final_scores, final_scores + PLAYER_NUM);
        if (pos != final_scores + PLAYER_NUM)
        {
#ifdef STATISTICS_EVALUATING
            fout.open(statistics_file, ios::app);
            fout << ", " << pos - final_scores << endl;
            fout.close();
#endif 
            return pos - final_scores;
        }
        else
        {
#ifdef STATISTICS_EVALUATING
            fout.open(statistics_file, ios::app);
            fout << ", " << check_seats.front() << endl;
            fout.close();
#endif 
            return check_seats.front();
        }
//        int * pos = max_element(check_results, check_results + PLAYER_NUM);
//        if (pos != check_results + PLAYER_NUM)
//        {
//#ifdef STATISTICS_EVALUATING
//            fout.open(statistics_file, ios::app);
//            fout << ", " << pos - check_results << endl;
//            fout.close();
//#endif 
//            return pos - check_results;
//        }
//#ifdef STATISTICS_EVALUATING
//        fout.open(statistics_file, ios::app);
//        fout << ", " << check_seats.front() << endl;
//        fout.close();
//#endif 
//        return check_seats.front();
    }
}
