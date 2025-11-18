#include "include/algorithmEvaluator.h"
#include "include/calculateObjective.h"
#include <iostream>
#include <climits>
#include <chrono>

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
    result.minTime = 1e9;
    result.maxTime = 0;
    long long sumObj = 0;
    double sumTime = 0;
    
    for (int start = 0; start < n; start++) {
        auto startTime = std::chrono::high_resolution_clock::now();
        auto sol = algorithmFunc(start);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        int obj = calculateObjective(sol, distance, costs);
        double timeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        sumObj += obj;
        sumTime += timeMs;
        
        if (obj < result.minObj) {
            result.minObj = obj;
            result.bestSolution = sol;
        }
        if (obj > result.maxObj) {
            result.maxObj = obj;
        }
        if (timeMs < result.minTime) {
            result.minTime = timeMs;
        }
        if (timeMs > result.maxTime) {
            result.maxTime = timeMs;
        }
    }
    
    result.avgObj = sumObj / n;
    result.avgTime = sumTime / n;
    return result;
}

void printAlgorithmResult(const std::string& name, const AlgorithmResult& result) {
    std::cout << name << ":\n";
    std::cout << "  Objective: Min=" << result.minObj << ", Max=" << result.maxObj << ", Avg=" << result.avgObj << "\n";
    std::cout << "  Time (ms): Min=" << result.minTime << ", Max=" << result.maxTime << ", Avg=" << result.avgTime << "\n";
    std::cout << "  Best: ";
    for (int node : result.bestSolution) std::cout << node << " ";
    std::cout << "\n\n" << std::flush;
}
