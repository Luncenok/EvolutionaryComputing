#include "../include/greedyCycle.h"
#include <climits>

std::vector<int> greedyCycle(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
    std::vector<int> solution;
    std::vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    if (selectCount > 1) {
        int bestNode = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[startNode][i] + costs[i];
            if (delta < bestDist) {
                bestDist = delta;
                bestNode = i;
            }
        }
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    
    while (solution.size() < selectCount) {
        int bestNode = -1;
        int bestPos = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] 
                           - distance[solution[pos]][solution[next]] + costs[i];
                
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestNode = i;
                    bestPos = pos + 1;
                }
            }
        }
        
        solution.insert(solution.begin() + bestPos, bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}
