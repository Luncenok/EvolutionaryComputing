#include "../include/greedyRegret2.h"
#include <climits>

std::vector<int> greedyRegret2(int startNode, int selectCount, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
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
            if (delta < bestDist) { bestDist = delta; bestNode = i; }
        }
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    while (solution.size() < selectCount) {
        int chooseNode = -1;
        int choosePos = -1;
        int bestRegret = -1;
        int tieBestDelta = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int best1 = INT_MAX, best2 = INT_MAX;
            int bestPos = -1;
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] - distance[solution[pos]][solution[next]] + costs[i];
                if (delta < best1) {
                    best2 = best1;
                    best1 = delta;
                    bestPos = pos + 1;
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
