#include "../include/nearestNeighborAnyRegret2.h"
#include <climits>

std::vector<int> nearestNeighborAnyRegret2(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
    std::vector<int> solution;
    std::vector<bool> selected(distance.size(), false);
    solution.push_back(startNode);
    selected[startNode] = true;
    
    while (solution.size() < selectCount) {
        int chooseNode = -1;
        int choosePos = -1;
        int bestRegret = -1;
        int tieBestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int best1 = INT_MAX, best2 = INT_MAX;
            int bestPos = -1;
            
            for (int pos = 0; pos <= solution.size(); pos++) {
                int delta = costs[i];
                if (pos == 0) {
                    delta += distance[i][solution[0]];
                } else if (pos == solution.size()) {
                    delta += distance[solution.back()][i];
                } else {
                    delta += distance[solution[pos-1]][i] + distance[i][solution[pos]] - distance[solution[pos-1]][solution[pos]];
                }
                
                if (delta < best1) {
                    best2 = best1;
                    best1 = delta;
                    bestPos = pos;
                } else if (delta < best2) {
                    best2 = delta;
                }
            }
            
            int regret = (best2 == INT_MAX ? 0 : (best2 - best1));
            if (regret > bestRegret || (regret == bestRegret && best1 < tieBestDelta)) {
                bestRegret = regret;
                tieBestDelta = best1;
                chooseNode = i;
                choosePos = bestPos;
            }
        }
        
        solution.insert(solution.begin() + choosePos, chooseNode);
        selected[chooseNode] = true;
    }
    
    return solution;
}
