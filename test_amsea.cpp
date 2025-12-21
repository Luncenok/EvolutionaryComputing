// Quick test for AMSEA only
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <chrono>
#include <iomanip>
#include "include/calculateObjective.h"
#include "include/algorithmEvaluator.h"
#include "include/amsea.h"

void testAMSEA(const std::string& filename) {
    std::vector<std::tuple<int, int, int>> table;
    std::ifstream file(filename);
    std::string line;
    // No header to skip in these files
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int x, y, cost;
        char c;
        ss >> x >> c >> y >> c >> cost;
        table.push_back(std::make_tuple(x, y, cost));
    }
    
    int n = table.size();
    int selectCount = (n + 1) / 2;
    
    std::vector<std::vector<int>> distance(n, std::vector<int>(n, 0));
    std::vector<int> costs(n);
    
    for (int i = 0; i < n; i++) {
        costs[i] = std::get<2>(table[i]);
        for (int j = 0; j < n; j++) {
            int dx = std::get<0>(table[i]) - std::get<0>(table[j]);
            int dy = std::get<1>(table[i]) - std::get<1>(table[j]);
            distance[i][j] = std::round(std::sqrt(dx*dx + dy*dy));
        }
    }
    
    std::cout << "\n=-=-= " << filename << " =-=-=\n";
    std::cout << "Nodes: " << n << ", Selecting: " << selectCount << "\n\n";
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    double timeLimit = 1000.0;  // 1 second per run
    
    std::vector<AMSEAResult> results;
    AlgorithmResult amseaResult =
        evaluateIterativeAlgorithm<AMSEAResult>("AMSEA", 20, [&]() {
            auto res = amsea(n, selectCount, distance, costs, timeLimit, rng);
            results.push_back(res);
            return res;
        });
    
    long long sumGen = 0;
    int totalOp1Success = 0, totalOp2Success = 0, totalPathRelinkSuccess = 0;
    int totalOp1Attempts = 0, totalOp2Attempts = 0, totalPathRelinkAttempts = 0;
    for (const auto& res : results) {
        sumGen += res.generations;
        totalOp1Success += res.operatorSuccesses[0];
        totalOp2Success += res.operatorSuccesses[1];
        totalPathRelinkSuccess += res.operatorSuccesses[2];
        totalOp1Attempts += res.operatorAttempts[0];
        totalOp2Attempts += res.operatorAttempts[1];
        totalPathRelinkAttempts += res.operatorAttempts[2];
    }
    
    printAlgorithmResult("AMSEA (time limit = " + std::to_string((int)timeLimit) + " ms)", amseaResult);
    std::cout << "  Generations: Avg=" << (sumGen / 20.0) << "\n";
    std::cout << "  Operator Stats: Op1=" << totalOp1Success << "/" << totalOp1Attempts
              << ", Op2=" << totalOp2Success << "/" << totalOp2Attempts
              << ", PathRelink=" << totalPathRelinkSuccess << "/" << totalPathRelinkAttempts << "\n\n";
}

int main() {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "AMSEA Quick Test\n";
    std::cout << "================\n";
    
    testAMSEA("input/TSPA.csv");
    testAMSEA("input/TSPB.csv");
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();
    std::cout << "Total time: " << std::fixed << std::setprecision(2) << elapsed << " seconds\n";
    
    return 0;
}
