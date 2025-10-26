#include "include/algorithmEvaluator.h"
#include "include/calculateObjective.h"
#include <iostream>
#include <climits>

AlgorithmResult evaluateAlgorithm(
    const std::string& name,
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    std::function<std::vector<int>(int)> algorithmFunc
) {
    AlgorithmResult result;
    result.minObj = INT_MAX;
    result.maxObj = 0;
    int sumObj = 0;
    
    for (int start = 0; start < n; start++) {
        auto sol = algorithmFunc(start);
        int obj = calculateObjective(sol, distance, costs);
        sumObj += obj;
        if (obj < result.minObj) {
            result.minObj = obj;
            result.bestSolution = sol;
        }
        if (obj > result.maxObj) {
            result.maxObj = obj;
        }
    }
    
    result.avgObj = sumObj / n;
    return result;
}

void printAlgorithmResult(const std::string& name, const AlgorithmResult& result) {
    std::cout << name << ":\n";
    std::cout << "  Min: " << result.minObj << ", Max: " << result.maxObj << ", Avg: " << result.avgObj << "\n";
    std::cout << "  Best: ";
    for (int node : result.bestSolution) std::cout << node << " ";
    std::cout << "\n\n";
}
