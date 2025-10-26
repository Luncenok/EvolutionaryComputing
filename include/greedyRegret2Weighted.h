#ifndef GREEDY_REGRET2_WEIGHTED_H
#define GREEDY_REGRET2_WEIGHTED_H

#include <vector>

std::vector<int> greedyRegret2Weighted(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs, double wRegret = 1.0, double wBest = 1.0);

#endif
