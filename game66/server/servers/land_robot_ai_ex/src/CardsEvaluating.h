#ifndef EVALUATOR_H_
#define EVALUATOR_H_

#include <vector>

// 输入： 3家手牌 + 底牌
// 返回： 评价出的牌最好的座位号
int evaluate_cards(const std::vector<std::vector<int>> &cards);

#endif // EVALUATOR_H_

