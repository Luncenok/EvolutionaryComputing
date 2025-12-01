#ifndef LARGE_NEIGHBORHOOD_SEARCH_H
#define LARGE_NEIGHBORHOOD_SEARCH_H

#include <vector>
#include <random>

struct LNSResult {
    std::vector<int> bestSolution;
    int bestObjective;
    double totalTime;
    int iterations;  // Number of destroy-repair iterations
};

// Large Neighborhood Search - with local search after destroy-repair
LNSResult largeNeighborhoodSearchWithLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng,
    double destroyFraction = 0.30  // Default: remove 30% of nodes
);

// Large Neighborhood Search - without local search after destroy-repair
LNSResult largeNeighborhoodSearchNoLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng,
    double destroyFraction = 0.30  // Default: remove 30% of nodes
);

#endif
