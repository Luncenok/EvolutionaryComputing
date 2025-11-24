#ifndef ITERATED_LS_H
#define ITERATED_LS_H

#include <vector>
#include <random>

struct ILSResult {
    std::vector<int> bestSolution;
    int bestObjective;
    double totalTime;
    int lsRuns;
};

// Iterated Local Search - applies perturbation and local search iteratively
ILSResult iteratedLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng
);

#endif
