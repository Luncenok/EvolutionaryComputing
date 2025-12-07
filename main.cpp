#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <climits>
#include <cfloat>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
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
#include "include/candidateMoves.h"
#include "include/multipleStartLS.h"
#include "include/iteratedLS.h"
#include "include/largeNeighborhoodSearch.h"
#include "include/globalConvexity.h"

std::vector<int> process(const std::string& filename, bool returnBestSolution = false) {
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
    
    // Store best ILS solution for return
    std::vector<int> bestILSSolution;

    // Pre-generate random initial solutions shared by all random-start methods
    std::vector<std::vector<int>> randomInitials(n);
    for (int start = 0; start < n; ++start) {
        randomInitials[start] = randomSolution(start, n, selectCount, rng);
    }

    // Random solutions
    auto resultRandom = evaluateAlgorithm("Random", n, selectCount, distance, costs,
        [&](int start) { return randomInitials[start]; });
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
    auto resultLSRandomSteepestNodes = evaluateAlgorithm("LS Random + Steepest + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestNodes(initial, distance, costs, n);
        });
    printAlgorithmResult("LS Random + Steepest + Nodes", resultLSRandomSteepestNodes);
    
    // Local Search: Random start + Greedy + Nodes
    auto resultLSRandomGreedyNodes = evaluateAlgorithm("LS Random + Greedy + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchGreedyNodes(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS Random + Greedy + Nodes", resultLSRandomGreedyNodes);
    
    // Local Search: Random start + Greedy + Edges
    auto resultLSRandomGreedyEdges = evaluateAlgorithm("LS Random + Greedy + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchGreedyEdges(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS Random + Greedy + Edges", resultLSRandomGreedyEdges);
    
    // Local Search: Greedy start + Steepest + Nodes
    auto resultLSGreedySteepestNodes = evaluateAlgorithm("LS Greedy + Steepest + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchSteepestNodes(initial, distance, costs, n);
        });
    printAlgorithmResult("LS Greedy + Steepest + Nodes", resultLSGreedySteepestNodes);
    
    // Local Search: Greedy start + Steepest + Edges
    auto resultLSGreedySteepestEdges = evaluateAlgorithm("LS Greedy + Steepest + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchSteepestEdges(initial, distance, costs, n);
        });
    printAlgorithmResult("LS Greedy + Steepest + Edges", resultLSGreedySteepestEdges);
    
    // Local Search: Greedy start + Greedy + Nodes
    auto resultLSGreedyGreedyNodes = evaluateAlgorithm("LS Greedy + Greedy + Nodes", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchGreedyNodes(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS Greedy + Greedy + Nodes", resultLSGreedyGreedyNodes);
    
    // Local Search: Greedy start + Greedy + Edges
    auto resultLSGreedyGreedyEdges = evaluateAlgorithm("LS Greedy + Greedy + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = bestGreedyFunc(start);
            return localSearchGreedyEdges(initial, distance, costs, n, rng);
        });
    printAlgorithmResult("LS Greedy + Greedy + Edges", resultLSGreedyGreedyEdges);
    
    // Local Search: Random start + Steepest + Edges
    auto resultLSRandomSteepestEdges = evaluateAlgorithm("LS Random + Steepest + Edges", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestEdges(initial, distance, costs, n);
        });
    printAlgorithmResult("LS Random + Steepest + Edges", resultLSRandomSteepestEdges);
    
    // Local Search: Random start + Steepest + Edges with LM (list of improving moves)
    auto resultLSRandomSteepestEdgesLM = evaluateAlgorithm("LM Random + Steepest + Edges", n, selectCount, distance, costs,
        [&](int start) {
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesLM(initial, distance, costs, n);
        });
    printAlgorithmResult("LM Random + Steepest + Edges", resultLSRandomSteepestEdgesLM);
        
    // Candidate Moves with different k values
    auto resultCandidatesK5 = evaluateAlgorithm("Candidates + Random + Steepest + Edges (k=5)", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesCandidates(initial, distance, costs, n, 5);
        });
    printAlgorithmResult("Candidates + Random + Steepest + Edges (k=5)", resultCandidatesK5);
    
    auto resultCandidatesK10 = evaluateAlgorithm("Candidates + Random + Steepest + Edges (k=10)", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesCandidates(initial, distance, costs, n, 10);
        });
    printAlgorithmResult("Candidates + Random + Steepest + Edges (k=10)", resultCandidatesK10);
    
    auto resultCandidatesK15 = evaluateAlgorithm("Candidates + Random + Steepest + Edges (k=15)", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesCandidates(initial, distance, costs, n, 15);
        });
    printAlgorithmResult("Candidates + Random + Steepest + Edges (k=15)", resultCandidatesK15);
    
    auto resultCandidatesK20 = evaluateAlgorithm("Candidates + Random + Steepest + Edges (k=20)", n, selectCount, distance, costs,
        [&](int start) { 
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesCandidates(initial, distance, costs, n, 20);
        });
    printAlgorithmResult("Candidates + Random + Steepest + Edges (k=20)", resultCandidatesK20);

    // LM + Candidate Moves (k=10)
    auto resultLMCandidatesK10 = evaluateAlgorithm("LM Candidates + Random + Steepest + Edges (k=10)", n, selectCount, distance, costs,
        [&](int start) {
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesLMCandidates(initial, distance, costs, n, 10);
        });
    printAlgorithmResult("LM Candidates + Random + Steepest + Edges (k=10)", resultLMCandidatesK10);

    // LM + Candidate Moves (k=20)
    auto resultLMCandidatesK20 = evaluateAlgorithm("LM Candidates + Random + Steepest + Edges (k=20)", n, selectCount, distance, costs,
        [&](int start) {
            auto initial = randomInitials[start];
            return localSearchSteepestEdgesLMCandidates(initial, distance, costs, n, 100);
        });
    printAlgorithmResult("LM Candidates + Random + Steepest + Edges (k=20)", resultLMCandidatesK20);
    
    // Multiple Start Local Search - run 20 times
    // We use a custom evaluator for this specifically because it has to run exactly 20 times.
    int mslsIterations = 200;
    AlgorithmResult mslsResult = evaluateIterativeAlgorithm<MSLSResult>(
        "MSLS",
        20,
        [&]() { 
            std::vector<std::vector<int>> randomInitials(n);
            for (int start = 0; start < n; ++start) {
                randomInitials[start] = randomSolution(start, n, selectCount, rng);
            }
            return multipleStartLS(n, selectCount, distance, costs, randomInitials, mslsIterations, rng); }
    );
    printAlgorithmResult("Multiple Start Local Search (200 iterations)", mslsResult);
    
    // Iterated Local Search - run 20 times with time limit = average MSLS time
    double ilsTimeLimit = mslsResult.avgTime;
    std::vector<ILSResult> ilsResults;
    AlgorithmResult ilsResult = evaluateIterativeAlgorithm<ILSResult>(
        "ILS",
        20,
        [&]() { 
            std::vector<std::vector<int>> randomInitials(n);
            for (int start = 0; start < n; ++start) {
                randomInitials[start] = randomSolution(start, n, selectCount, rng);
            }
            auto res = iteratedLS(n, selectCount, distance, costs, randomInitials, ilsTimeLimit, rng); 
            ilsResults.push_back(res);
            return res;
        }
    );
    
    // Store best ILS solution for assignment 8
    bestILSSolution = ilsResult.bestSolution;
    
    // Calculate average LS runs
    long long sumLSRuns = 0;
    for (const auto& res : ilsResults) {
        sumLSRuns += res.lsRuns;
    }
    double avgLSRuns = sumLSRuns / 20.0;
    
    printAlgorithmResult("Iterated Local Search (time limit = " + std::to_string((int)ilsTimeLimit) + " ms)", ilsResult);
    std::cout << "  LS Runs: Avg=" << avgLSRuns << "\n\n" << std::flush;
    
    // Large Neighborhood Search with Local Search - run 20 times with time limit = average MSLS time
    double lnsTimeLimit = mslsResult.avgTime;
    std::vector<LNSResult> lnsWithLSResults;
    AlgorithmResult lnsWithLSResult = evaluateIterativeAlgorithm<LNSResult>(
        "LNS with LS",
        20,
        [&]() { 
            std::vector<std::vector<int>> randomInitials(n);
            for (int start = 0; start < n; ++start) {
                randomInitials[start] = randomSolution(start, n, selectCount, rng);
            }
            auto res = largeNeighborhoodSearchWithLS(n, selectCount, distance, costs, randomInitials, lnsTimeLimit, rng);
            lnsWithLSResults.push_back(res);
            return res;
        }
    );
    
    // Calculate average iterations for LNS with LS
    long long sumLNSWithLSIter = 0;
    for (const auto& res : lnsWithLSResults) {
        sumLNSWithLSIter += res.iterations;
    }
    double avgLNSWithLSIter = sumLNSWithLSIter / 20.0;
    
    printAlgorithmResult("LNS with LS (time limit = " + std::to_string((int)lnsTimeLimit) + " ms)", lnsWithLSResult);
    std::cout << "  Iterations: Avg=" << avgLNSWithLSIter << "\n\n" << std::flush;
    
    // Large Neighborhood Search without Local Search - run 20 times with time limit = average MSLS time
    std::vector<LNSResult> lnsNoLSResults;
    AlgorithmResult lnsNoLSResult = evaluateIterativeAlgorithm<LNSResult>(
        "LNS without LS",
        20,
        [&]() { 
            std::vector<std::vector<int>> randomInitials(n);
            for (int start = 0; start < n; ++start) {
                randomInitials[start] = randomSolution(start, n, selectCount, rng);
            }
            auto res = largeNeighborhoodSearchNoLS(n, selectCount, distance, costs, randomInitials, lnsTimeLimit, rng);
            lnsNoLSResults.push_back(res);
            return res;
        }
    );
    
    // Calculate average iterations for LNS without LS
    long long sumLNSNoLSIter = 0;
    for (const auto& res : lnsNoLSResults) {
        sumLNSNoLSIter += res.iterations;
    }
    double avgLNSNoLSIter = sumLNSNoLSIter / 20.0;
    
    printAlgorithmResult("LNS without LS (time limit = " + std::to_string((int)lnsTimeLimit) + " ms)", lnsNoLSResult);
    std::cout << "  Iterations: Avg=" << avgLNSNoLSIter << "\n\n" << std::flush;
    
    return bestILSSolution;
}

int main() {
    // std::ios_base::sync_with_stdio(false);
    // std::cin.tie(0);
    // std::cout.tie(0);
    
    // Record start time
    auto startTime = std::chrono::high_resolution_clock::now();
    auto startTimeT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "Execution started at: " << std::put_time(std::localtime(&startTimeT), "%Y-%m-%d %H:%M:%S") << "\n\n" << std::flush;
    
    std::vector<int> bestSolutionA = process("input/TSPA.csv", true);
    std::vector<int> bestSolutionB = process("input/TSPB.csv", true);
    
    // Assignment 8: Global Convexity Tests
    std::cout << "\n" << std::flush;
    
    // We need to re-read the instances to run global convexity analysis
    std::mt19937 rngGC(DEFAULT_SEED);
    
    // TSPA
    {
        std::vector<std::tuple<int, int, int>> table;
        std::ifstream fin("input/TSPA.csv");
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
        
        int n = table.size();
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
        
        int selectCount = (n + 1) / 2;
        auto resultA = analyzeGlobalConvexity("TSPA", n, selectCount, distance, costs, bestSolutionA, rngGC);
        exportConvexityData(resultA, "output");
        
        // Print results in structured format
        std::cout << "Global Convexity Analysis (TSPA) - 1000 Random Local Optima:\n";
        std::cout << "  Objective: Min=" << resultA.minObjective << ", Max=" << resultA.maxObjective 
                  << ", Avg=" << (int)resultA.avgObjective << "\n";
        std::cout << "  Time (ms): Min=" << resultA.totalTime << ", Max=" << resultA.totalTime 
                  << ", Avg=" << resultA.totalTime << "\n";
        std::cout << "  Best:";
        for (int node : resultA.bestSolution) {
            std::cout << " " << node;
        }
        std::cout << "\n";
        std::cout << "  Common Edges (out of 100):\n";
        std::cout << "    Avg similarity to all: Min=" << resultA.edgesData[0].minSimilarity 
                  << ", Max=" << resultA.edgesData[0].maxSimilarity 
                  << ", Avg=" << resultA.edgesData[0].avgSimilarity 
                  << " (r=" << resultA.edgesData[0].correlation << ")\n";
        std::cout << "    Best of 1000 optima:   Min=" << resultA.edgesData[1].minSimilarity 
                  << ", Max=" << resultA.edgesData[1].maxSimilarity 
                  << ", Avg=" << resultA.edgesData[1].avgSimilarity 
                  << " (r=" << resultA.edgesData[1].correlation << ")\n";
        std::cout << "    Best method (ILS):     Min=" << resultA.edgesData[2].minSimilarity 
                  << ", Max=" << resultA.edgesData[2].maxSimilarity 
                  << ", Avg=" << resultA.edgesData[2].avgSimilarity 
                  << " (r=" << resultA.edgesData[2].correlation << ")\n";
        std::cout << "  Common Nodes (out of 100):\n";
        std::cout << "    Avg similarity to all: Min=" << resultA.nodesData[0].minSimilarity 
                  << ", Max=" << resultA.nodesData[0].maxSimilarity 
                  << ", Avg=" << resultA.nodesData[0].avgSimilarity 
                  << " (r=" << resultA.nodesData[0].correlation << ")\n";
        std::cout << "    Best of 1000 optima:   Min=" << resultA.nodesData[1].minSimilarity 
                  << ", Max=" << resultA.nodesData[1].maxSimilarity 
                  << ", Avg=" << resultA.nodesData[1].avgSimilarity 
                  << " (r=" << resultA.nodesData[1].correlation << ")\n";
        std::cout << "    Best method (ILS):     Min=" << resultA.nodesData[2].minSimilarity 
                  << ", Max=" << resultA.nodesData[2].maxSimilarity 
                  << ", Avg=" << resultA.nodesData[2].avgSimilarity 
                  << " (r=" << resultA.nodesData[2].correlation << ")\n\n";
    }
    
    // TSPB
    {
        std::vector<std::tuple<int, int, int>> table;
        std::ifstream fin("input/TSPB.csv");
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
        
        int n = table.size();
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
        
        int selectCount = (n + 1) / 2;
        auto resultB = analyzeGlobalConvexity("TSPB", n, selectCount, distance, costs, bestSolutionB, rngGC);
        exportConvexityData(resultB, "output");
        
        // Print results in structured format
        std::cout << "Global Convexity Analysis (TSPB) - 1000 Random Local Optima:\n";
        std::cout << "  Objective: Min=" << resultB.minObjective << ", Max=" << resultB.maxObjective 
                  << ", Avg=" << (int)resultB.avgObjective << "\n";
        std::cout << "  Time (ms): Min=" << resultB.totalTime << ", Max=" << resultB.totalTime 
                  << ", Avg=" << resultB.totalTime << "\n";
        std::cout << "  Best:";
        for (int node : resultB.bestSolution) {
            std::cout << " " << node;
        }
        std::cout << "\n";
        std::cout << "  Common Edges (out of 100):\n";
        std::cout << "    Avg similarity to all: Min=" << resultB.edgesData[0].minSimilarity 
                  << ", Max=" << resultB.edgesData[0].maxSimilarity 
                  << ", Avg=" << resultB.edgesData[0].avgSimilarity 
                  << " (r=" << resultB.edgesData[0].correlation << ")\n";
        std::cout << "    Best of 1000 optima:   Min=" << resultB.edgesData[1].minSimilarity 
                  << ", Max=" << resultB.edgesData[1].maxSimilarity 
                  << ", Avg=" << resultB.edgesData[1].avgSimilarity 
                  << " (r=" << resultB.edgesData[1].correlation << ")\n";
        std::cout << "    Best method (ILS):     Min=" << resultB.edgesData[2].minSimilarity 
                  << ", Max=" << resultB.edgesData[2].maxSimilarity 
                  << ", Avg=" << resultB.edgesData[2].avgSimilarity 
                  << " (r=" << resultB.edgesData[2].correlation << ")\n";
        std::cout << "  Common Nodes (out of 100):\n";
        std::cout << "    Avg similarity to all: Min=" << resultB.nodesData[0].minSimilarity 
                  << ", Max=" << resultB.nodesData[0].maxSimilarity 
                  << ", Avg=" << resultB.nodesData[0].avgSimilarity 
                  << " (r=" << resultB.nodesData[0].correlation << ")\n";
        std::cout << "    Best of 1000 optima:   Min=" << resultB.nodesData[1].minSimilarity 
                  << ", Max=" << resultB.nodesData[1].maxSimilarity 
                  << ", Avg=" << resultB.nodesData[1].avgSimilarity 
                  << " (r=" << resultB.nodesData[1].correlation << ")\n";
        std::cout << "    Best method (ILS):     Min=" << resultB.nodesData[2].minSimilarity 
                  << ", Max=" << resultB.nodesData[2].maxSimilarity 
                  << ", Avg=" << resultB.nodesData[2].avgSimilarity 
                  << " (r=" << resultB.nodesData[2].correlation << ")\n\n";
    }
    
    // Record end time and calculate elapsed time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto endTimeT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "\nExecution ended at: " << std::put_time(std::localtime(&endTimeT), "%Y-%m-%d %H:%M:%S") << "\n";
    std::cout << "Total time elapsed: " << (elapsedMs / 1000.0) << " seconds (" << elapsedMs << " ms)\n" << std::flush;
    
    return 0;
}
