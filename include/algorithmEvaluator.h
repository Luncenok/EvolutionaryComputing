#ifndef ALGORITHM_EVALUATOR_H
#define ALGORITHM_EVALUATOR_H

#include <vector>
#include <string>
#include <functional>
#include <climits>
#include <cfloat>

struct AlgorithmResult {
    int minObj;
    int maxObj;
    int avgObj;
    double minTime;
    double maxTime;
    double avgTime;
    std::vector<int> bestSolution;
};

AlgorithmResult evaluateAlgorithm(
    const std::string& name,
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    std::function<std::vector<int>(int)> algorithmFunc
);

void printAlgorithmResult(const std::string& name, const AlgorithmResult& result);

// Evaluator for iterative algorithms (MSLS, ILS) that run a fixed number of times
// Function should return a struct with: bestSolution, bestObjective, totalTime
template<typename ResultType>
AlgorithmResult evaluateIterativeAlgorithm(
    const std::string& name,
    int runs,
    std::function<ResultType()> algorithmFunc
) {
    AlgorithmResult result;
    result.minObj = INT_MAX;
    result.maxObj = 0;
    result.minTime = DBL_MAX;
    result.maxTime = 0;
    long long sumObj = 0;
    double sumTime = 0;
    
    for (int run = 0; run < runs; run++) {
        auto res = algorithmFunc();
        
        sumObj += res.bestObjective;
        sumTime += res.totalTime;
        
        if (res.bestObjective < result.minObj) {
            result.minObj = res.bestObjective;
            result.bestSolution = res.bestSolution;
        }
        if (res.bestObjective > result.maxObj) {
            result.maxObj = res.bestObjective;
        }
        if (res.totalTime < result.minTime) {
            result.minTime = res.totalTime;
        }
        if (res.totalTime > result.maxTime) {
            result.maxTime = res.totalTime;
        }
    }
    
    result.avgObj = sumObj / runs;
    result.avgTime = sumTime / runs;
    return result;
}

#endif
