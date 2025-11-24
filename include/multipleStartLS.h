#ifndef MULTIPLE_START_LS_H
#define MULTIPLE_START_LS_H

#include <vector>
#include <random>

struct MSLSResult {
    std::vector<int> bestSolution;
    int bestObjective;
    double totalTime;
    int iterations;
};

// Multiple Start Local Search - runs local search multiple times from random starting solutions
MSLSResult multipleStartLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    int iterations,
    std::mt19937& rng
);

#endif
