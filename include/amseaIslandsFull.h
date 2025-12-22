#ifndef AMSEA_ISLANDS_FULL_H
#define AMSEA_ISLANDS_FULL_H

#include <vector>
#include <random>

struct AMSEAIslandsFullResult {
    int bestObjective;
    std::vector<int> bestSolution;
    int generations;
    double totalTime;
    int operatorSuccesses[4];
    int operatorAttempts[4];
};

AMSEAIslandsFullResult amseaIslandsFull(int n, int selectCount,
                          const std::vector<std::vector<int>> &distance,
                          const std::vector<int> &costs, double timeLimit,
                          std::mt19937 &rng, int populationSize);

#endif // AMSEA_ISLANDS_FULL_H
