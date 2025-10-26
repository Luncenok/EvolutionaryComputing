#include "../include/nearestNeighborAny.h"
#include <climits>

std::vector<int> nearestNeighborAny(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
    std::vector<int> solution;
    std::vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    while (solution.size() < selectCount) {
        int bestNode = -1;
        int bestPos = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            
            for (int pos = 0; pos <= solution.size(); pos++) {
                int delta = costs[i];
                if (pos == 0) {
                    delta += distance[i][solution[0]];
                } else if (pos == solution.size()) {
                    delta += distance[solution.back()][i];
                } else {
                    delta += distance[solution[pos-1]][i] + distance[i][solution[pos]] - distance[solution[pos-1]][solution[pos]];
                }
                
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestNode = i;
                    bestPos = pos;
                }
            }
        }
        
        solution.insert(solution.begin() + bestPos, bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}
