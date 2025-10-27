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
#include "include/localSearch.h"

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
    
    // Find best greedy heuristic for starting solutions
    int bestGreedyObj = std::min({resultNNAny.avgObj, resultGC.avgObj, resultGR2.avgObj, resultGW.avgObj, resultNNAR2.avgObj, resultNNAW.avgObj});
    std::function<std::vector<int>(int)> bestGreedyFunc;
    if (bestGreedyObj == resultNNAny.avgObj) {
        bestGreedyFunc = [&](int start) { return nearestNeighborAny(start, selectCount, distance, costs); };
    } else if (bestGreedyObj == resultGW.avgObj) {
        bestGreedyFunc = [&](int start) { return greedyRegret2Weighted(start, selectCount, distance, costs, wRegret, wBest); };
    } else if (bestGreedyObj == resultNNAW.avgObj) {
        bestGreedyFunc = [&](int start) { return nearestNeighborAnyRegret2Weighted(start, selectCount, distance, costs, wRegret, wBest); };
    } else if (bestGreedyObj == resultGC.avgObj) {
        bestGreedyFunc = [&](int start) { return greedyCycle(start, selectCount, distance, costs); };
    } else if (bestGreedyObj == resultGR2.avgObj) {
        bestGreedyFunc = [&](int start) { return greedyRegret2(start, selectCount, distance, costs); };
    } else {
        bestGreedyFunc = [&](int start) { return nearestNeighborAnyRegret2(start, selectCount, distance, costs); };
    }
    
    // Local Search: Random start + Steepest + Nodes
    auto resultLSRandomSteepestNodes = evaluateAlgorithm("LS: Random + Steepest + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomSolution(start, n, selectCount, rng);
            return localSearchSteepestNodes(initial, distance, costs, n);
        });
    printAlgorithmResult("LS: Random + Steepest + Nodes", resultLSRandomSteepestNodes);
    
    // Local Search: Random start + Steepest + Edges
    auto resultLSRandomSteepestEdges = evaluateAlgorithm("LS: Random + Steepest + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomSolution(start, n, selectCount, rng);
            return localSearchSteepestEdges(initial, distance, costs, n);
        });
    printAlgorithmResult("LS: Random + Steepest + Edges", resultLSRandomSteepestEdges);
    
    // Local Search: Random start + Greedy + Nodes
    auto resultLSRandomGreedyNodes = evaluateAlgorithm("LS: Random + Greedy + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomSolution(start, n, selectCount, rng);
            return localSearchGreedyNodes(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS: Random + Greedy + Nodes", resultLSRandomGreedyNodes);
    
    // Local Search: Random start + Greedy + Edges
    auto resultLSRandomGreedyEdges = evaluateAlgorithm("LS: Random + Greedy + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomSolution(start, n, selectCount, rng);
            return localSearchGreedyEdges(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS: Random + Greedy + Edges", resultLSRandomGreedyEdges);
    
    // Local Search: Greedy start + Steepest + Nodes
    auto resultLSGreedySteepestNodes = evaluateAlgorithm("LS: Greedy + Steepest + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchSteepestNodes(initial, distance, costs, n);
        });
    printAlgorithmResult("LS: Greedy + Steepest + Nodes", resultLSGreedySteepestNodes);
    
    // Local Search: Greedy start + Steepest + Edges
    auto resultLSGreedySteepestEdges = evaluateAlgorithm("LS: Greedy + Steepest + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchSteepestEdges(initial, distance, costs, n);
        });
    printAlgorithmResult("LS: Greedy + Steepest + Edges", resultLSGreedySteepestEdges);
    
    // Local Search: Greedy start + Greedy + Nodes
    auto resultLSGreedyGreedyNodes = evaluateAlgorithm("LS: Greedy + Greedy + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchGreedyNodes(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS: Greedy + Greedy + Nodes", resultLSGreedyGreedyNodes);
    
    // Local Search: Greedy start + Greedy + Edges
    auto resultLSGreedyGreedyEdges = evaluateAlgorithm("LS: Greedy + Greedy + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchGreedyEdges(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS: Greedy + Greedy + Edges", resultLSGreedyGreedyEdges);
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
    
    process("input/TSPA.csv");
    process("input/TSPB.csv");
    
    return 0;
}
