#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <climits>
#include "include/constants.h"
#include "include/calculateObjective.h"
#include "include/randomSolution.h"
#include "include/nearestNeighborEnd.h"
#include "include/nearestNeighborAny.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAnyRegret2.h"
#include "include/nearestNeighborAnyRegret2Weighted.h"
#include "include/algorithmEvaluator.h"

void process(const std::string& filename) {
    std::vector<std::tuple<int, int, int>> table;
    
    // Read file
    std::ifstream fin(filename);
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        std::replace(line.begin(), line.end(), ';', ' ');
        std::istringstream iss(line);
        int x, y, cost;
        if (iss >> x >> y >> cost) {
            table.push_back(std::make_tuple(x, y, cost));
        }
    }
    fin.close();
    
    // Calculate distance matrix and costs
    int n = table.size();
    int selectCount = (n + 1) / 2;
    
    std::vector<std::vector<int>> distance(n, std::vector<int>(n));
    std::vector<int> costs(n);
    
    for (int i = 0; i < n; i++) {
        costs[i] = std::get<2>(table[i]);
        for (int j = 0; j < n; j++) {
            double dx = std::get<0>(table[i]) - std::get<0>(table[j]);
            double dy = std::get<1>(table[i]) - std::get<1>(table[j]);
            distance[i][j] = round(sqrt(dx * dx + dy * dy));
        }
    }
    
    std::cout << "\n=-=-= " << filename << " =-=-=\n";
    std::cout << "Nodes: " << n << ", Selecting: " << selectCount << "\n\n";
    
    std::mt19937 rng(DEFAULT_SEED);
    
    // Random solutions
    auto resultRandom = evaluateAlgorithm("Random", n, selectCount, distance, costs,
        [&](int start) { return randomSolution(start, n, selectCount, rng); });
    printAlgorithmResult("Random", resultRandom);
    
    // Nearest neighbor (end only)
    auto resultNNEnd = evaluateAlgorithm("Nearest Neighbor (end only)", n, selectCount, distance, costs,
        [&](int start) { return nearestNeighborEnd(start, selectCount, distance, costs); });
    printAlgorithmResult("Nearest Neighbor (end only)", resultNNEnd);
    
    // Nearest neighbor (any position)
    auto resultNNAny = evaluateAlgorithm("Nearest Neighbor (any position)", n, selectCount, distance, costs,
        [&](int start) { return nearestNeighborAny(start, selectCount, distance, costs); });
    printAlgorithmResult("Nearest Neighbor (any position)", resultNNAny);
    
    // Greedy cycle
    auto resultGC = evaluateAlgorithm("Greedy Cycle", n, selectCount, distance, costs,
        [&](int start) { return greedyCycle(start, selectCount, distance, costs); });
    printAlgorithmResult("Greedy Cycle", resultGC);

    // Greedy 2-regret
    auto resultGR2 = evaluateAlgorithm("Greedy 2-Regret", n, selectCount, distance, costs,
        [&](int start) { return greedyRegret2(start, selectCount, distance, costs); });
    printAlgorithmResult("Greedy 2-Regret", resultGR2);

    // Greedy weighted (2-regret + best delta)
    double wRegret = 1.0, wBest = 1.0;
    auto resultGW = evaluateAlgorithm("Greedy Weighted (2-Regret + BestDelta)", n, selectCount, distance, costs,
        [&](int start) { return greedyRegret2Weighted(start, selectCount, distance, costs, wRegret, wBest); });
    printAlgorithmResult("Greedy Weighted (2-Regret + BestDelta)", resultGW);

    // Nearest Neighbor Any 2-Regret
    auto resultNNAR2 = evaluateAlgorithm("Nearest Neighbor Any 2-Regret", n, selectCount, distance, costs,
        [&](int start) { return nearestNeighborAnyRegret2(start, selectCount, distance, costs); });
    printAlgorithmResult("Nearest Neighbor Any 2-Regret", resultNNAR2);

    // Nearest Neighbor Any Weighted (2-regret + best delta)
    auto resultNNAW = evaluateAlgorithm("Nearest Neighbor Any Weighted (2-Regret + BestDelta)", n, selectCount, distance, costs,
        [&](int start) { return nearestNeighborAnyRegret2Weighted(start, selectCount, distance, costs, wRegret, wBest); });
    printAlgorithmResult("Nearest Neighbor Any Weighted (2-Regret + BestDelta)", resultNNAW);
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
    
    process("input/TSPA.csv");
    process("input/TSPB.csv");
    
    return 0;
}
