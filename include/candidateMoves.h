#ifndef CANDIDATE_MOVES_H
#define CANDIDATE_MOVES_H

#include <vector>

// Build nearest neighbors for each node based on distance + cost
std::vector<std::vector<int>> buildNearestNeighbors(
    int n,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int k
);

// Steepest local search with candidate moves (edges exchange)
std::vector<int> localSearchSteepestEdgesCandidates(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    int k
);

#endif
