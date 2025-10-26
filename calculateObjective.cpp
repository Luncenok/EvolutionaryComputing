#include "include/calculateObjective.h"

int calculateObjective(const std::vector<int>& solution, const std::vector<std::vector<int>>& distance, const std::vector<int>& costs) {
    int obj = 0;
    for (int i = 0; i < solution.size(); i++) {
        obj += distance[solution[i]][solution[(i + 1) % solution.size()]];
        obj += costs[solution[i]];
    }
    return obj;
}
