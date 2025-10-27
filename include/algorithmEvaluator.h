#ifndef ALGORITHM_EVALUATOR_H
#define ALGORITHM_EVALUATOR_H

#include <vector>
#include <string>
#include <functional>

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

#endif
