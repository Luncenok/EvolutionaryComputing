#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include <vector>
#include <random>

// Local search with steepest descent and nodes exchange (intra-route)
std::vector<int> localSearchSteepestNodes(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n
);

// Local search with steepest descent and edges exchange (intra-route)
std::vector<int> localSearchSteepestEdges(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n
);

// Local search with greedy (random order) and nodes exchange (intra-route)
std::vector<int> localSearchGreedyNodes(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    std::mt19937& rng
);

// Local search with greedy (random order) and edges exchange (intra-route)
std::vector<int> localSearchGreedyEdges(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    std::mt19937& rng
);

#endif
