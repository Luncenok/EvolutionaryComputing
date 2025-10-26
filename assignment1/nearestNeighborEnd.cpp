#include "../include/nearestNeighborEnd.h"
#include <climits>

std::vector<int> nearestNeighborEnd(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
    std::vector<int> solution;
    std::vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    while (solution.size() < selectCount) {
        int lastNode = solution.back();
        int bestNode = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[lastNode][i] + costs[i];
            if (delta < bestDelta) {
                bestDelta = delta;
                bestNode = i;
            }
        }
        
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}
