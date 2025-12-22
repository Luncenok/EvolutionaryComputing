// Full test for AMSEA Islands - 20 runs aligned with main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <numeric>

#include "include/amseaIslands.h"
#include "include/calculateObjective.h"

void loadInstance(const std::string &filename, 
                  std::vector<std::vector<int>> &distance,
                  std::vector<int> &costs, int &n, int &selectCount) {
    std::vector<std::tuple<int, int, int>> table;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int x, y, cost; char c;
        ss >> x >> c >> y >> c >> cost;
        table.push_back(std::make_tuple(x, y, cost));
    }
    n = table.size();
    selectCount = (n + 1) / 2;
    distance.assign(n, std::vector<int>(n, 0));
    costs.resize(n);
    for (int i = 0; i < n; i++) {
        costs[i] = std::get<2>(table[i]);
        for (int j = 0; j < n; j++) {
            int dx = std::get<0>(table[i]) - std::get<0>(table[j]);
            int dy = std::get<1>(table[i]) - std::get<1>(table[j]);
            distance[i][j] = std::round(std::sqrt(dx*dx + dy*dy));
        }
    }
}

int main() {
    std::cout << "AMSEA Islands Full Test (20 runs, 1s time limit)\n";
    std::cout << "================================================\n\n";
    
    const double timeLimit = 1000.0;
    const int numRuns = 20;
    
    for (const char* instance : {"input/TSPA.csv", "input/TSPB.csv"}) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "=== " << instance << " ===\n";
        
        std::vector<int> objectives;
        long long totalGens = 0;
        long long totalOpSucc[3] = {0, 0, 0};
        long long totalOpAtt[3] = {0, 0, 0};
        double totalTime = 0;
        
        for (int run = 0; run < numRuns; run++) {
            std::random_device rd;
            std::mt19937 rng(rd());
            auto result = amseaIslands(n, selectCount, distance, costs, timeLimit, rng);
            objectives.push_back(result.bestObjective);
            totalGens += result.generations;
            totalTime += result.totalTime;
            for (int i = 0; i < 3; i++) {
                totalOpSucc[i] += result.operatorSuccesses[i];
                totalOpAtt[i] += result.operatorAttempts[i];
            }
        }
        
        int best = *std::min_element(objectives.begin(), objectives.end());
        int worst = *std::max_element(objectives.begin(), objectives.end());
        double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
        
        std::cout << "Best=" << best << ", Worst=" << worst 
                  << ", Avg=" << std::fixed << std::setprecision(1) << avg << "\n";
        std::cout << "Generations: " << (totalGens / numRuns) << "\n";
        std::cout << "Time: " << std::fixed << std::setprecision(2) << (totalTime / numRuns) << " ms\n";
        std::cout << "Operator Success Rates:\n";
        std::cout << "  CommonNodes: " << totalOpSucc[0] << "/" << totalOpAtt[0] 
                  << " (" << std::fixed << std::setprecision(1) << (100.0 * totalOpSucc[0] / totalOpAtt[0]) << "%)\n";
        std::cout << "  Parent: " << totalOpSucc[1] << "/" << totalOpAtt[1] 
                  << " (" << std::fixed << std::setprecision(1) << (100.0 * totalOpSucc[1] / totalOpAtt[1]) << "%)\n";
        std::cout << "  PathRelink: " << totalOpSucc[2] << "/" << totalOpAtt[2] 
                  << " (" << std::fixed << std::setprecision(1) << (100.0 * totalOpSucc[2] / totalOpAtt[2]) << "%)\n";
        std::cout << "\n";
    }
    
    std::cout << "Done!\n";
    return 0;
}
