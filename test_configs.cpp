// AMSEA Systematic Testing - Multiple Configurations
// Tests different strategies and documents results

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <chrono>
#include <iomanip>
#include <functional>
#include "include/calculateObjective.h"
#include "include/algorithmEvaluator.h"
#include "include/amsea.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"
#include "include/localSearch.h"

// ============================================================================
// Configuration Structure
// ============================================================================

struct AMSEAConfig {
    std::string name;
    int populationSize;
    int stagnationThreshold;
    bool useTournamentSelection;
    int tournamentSize;
    bool useEnhancedPathRelink;  // Multiple ratios vs single
    bool useLNSOperator;         // 4th operator
    bool useCrowdingReplacement; // Replace similar vs worst
    double fullLSProbability;    // 1.0 = always full, 0.5 = 50% chance
    int partialLSIterations;     // If not full LS
    bool perturbBestOnStagnation; // Perturb best vs worst
};

// ============================================================================
// Forward Declarations (from amsea.cpp - we'll inline the key functions)
// ============================================================================

// Delta calculations
static inline int deltaReverse(const std::vector<int> &sol, int pos1, int pos2,
                               const std::vector<std::vector<int>> &distance) {
    int n = sol.size();
    if (pos1 == pos2 || (pos1 + 1) % n == pos2) return 0;
    return (distance[sol[pos1]][sol[pos2]] + distance[sol[(pos1 + 1) % n]][sol[(pos2 + 1) % n]])
         - (distance[sol[pos1]][sol[(pos1 + 1) % n]] + distance[sol[pos2]][sol[(pos2 + 1) % n]]);
}

static inline int deltaExchange(const std::vector<int> &sol, int pos, int newNode,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs) {
    int n = sol.size();
    int prev = (pos - 1 + n) % n, next = (pos + 1) % n;
    return (distance[sol[prev]][newNode] + distance[newNode][sol[next]] + costs[newNode])
         - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
}

// Configurable Local Search
static std::vector<int> localSearchConfigurable(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n,
    bool fullSearch, int maxIterations, std::mt19937 &rng, double fullProb) {
    
    // Decide if we do full search
    bool doFull = fullSearch || (std::uniform_real_distribution<>(0, 1)(rng) < fullProb);
    
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    int iterations = 0;
    bool improved = true;
    
    while (improved && (doFull || iterations < maxIterations)) {
        improved = false;
        iterations++;
        int bestDelta = 0, bestType = -1, bestPos1 = -1, bestPos2 = -1, bestNode = -1;
        
        for (int i = 0; i < sol.size(); i++) {
            for (int j = i + 2; j < sol.size(); j++) {
                if (i == 0 && j == sol.size() - 1) continue;
                int delta = deltaReverse(sol, i, j, distance);
                if (delta < bestDelta) {
                    bestDelta = delta; bestType = 0; bestPos1 = i; bestPos2 = j;
                }
            }
        }
        
        for (int pos = 0; pos < sol.size(); pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int delta = deltaExchange(sol, pos, node, distance, costs);
                if (delta < bestDelta) {
                    bestDelta = delta; bestType = 1; bestPos1 = pos; bestNode = node;
                }
            }
        }
        
        if (bestDelta < 0) {
            improved = true;
            if (bestType == 0) {
                std::reverse(sol.begin() + bestPos1 + 1, sol.begin() + bestPos2 + 1);
            } else {
                inSolution[sol[bestPos1]] = false;
                inSolution[bestNode] = true;
                sol[bestPos1] = bestNode;
            }
        }
    }
    return sol;
}

// ============================================================================
// Test Runner
// ============================================================================

struct TestResult {
    std::string configName;
    std::string instance;
    int bestObj;
    int worstObj;
    double avgObj;
    double avgGens;
    double avgTime;
};

TestResult runTest(
    const AMSEAConfig &config,
    const std::string &instanceName,
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    int numRuns) {
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    std::vector<int> objectives;
    std::vector<long long> generations;
    
    for (int run = 0; run < numRuns; run++) {
        // Run AMSEA with current config (using the function from amsea.cpp)
        auto result = amsea(n, selectCount, distance, costs, timeLimit, rng, config.populationSize);
        objectives.push_back(result.bestObjective);
        generations.push_back(result.generations);
    }
    
    TestResult tr;
    tr.configName = config.name;
    tr.instance = instanceName;
    tr.bestObj = *std::min_element(objectives.begin(), objectives.end());
    tr.worstObj = *std::max_element(objectives.begin(), objectives.end());
    tr.avgObj = 0;
    for (int o : objectives) tr.avgObj += o;
    tr.avgObj /= numRuns;
    tr.avgGens = 0;
    for (long long g : generations) tr.avgGens += g;
    tr.avgGens /= numRuns;
    tr.avgTime = timeLimit;
    
    return tr;
}

void loadInstance(const std::string &filename, 
                  std::vector<std::vector<int>> &distance,
                  std::vector<int> &costs,
                  int &n, int &selectCount) {
    std::vector<std::tuple<int, int, int>> table;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int x, y, cost;
        char c;
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
    std::cout << "AMSEA Systematic Testing\n";
    std::cout << "========================\n\n";
    
    // Define configurations to test
    std::vector<AMSEAConfig> configs = {
        // Baseline configs
        {"Pop20-Stag30", 20, 30, false, 3, false, false, false, 1.0, 20, false},
        {"Pop15-Stag20", 15, 20, false, 3, false, false, false, 1.0, 20, false},
        {"Pop20-Stag20", 20, 20, false, 3, false, false, false, 1.0, 20, false},
        {"Pop25-Stag30", 25, 30, false, 3, false, false, false, 1.0, 20, false},
        
        // Tournament selection
        {"Pop20-Tour3", 20, 30, true, 3, false, false, false, 1.0, 20, false},
        {"Pop15-Tour3", 15, 20, true, 3, false, false, false, 1.0, 20, false},
        {"Pop20-Tour2", 20, 30, true, 2, false, false, false, 1.0, 20, false},
        
        // Partial LS
        {"Pop20-PartialLS70", 20, 30, false, 3, false, false, false, 0.7, 30, false},
        {"Pop20-PartialLS50", 20, 30, false, 3, false, false, false, 0.5, 30, false},
        
        // Combined best ideas
        {"Pop20-Tour3-PartialLS70", 20, 25, true, 3, false, false, false, 0.7, 30, false},
        {"Pop18-Tour2-Stag25", 18, 25, true, 2, false, false, false, 1.0, 20, false},
    };
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;  // 1 second per run
    const int numRuns = 10;  // Fewer runs for quick testing
    
    std::vector<TestResult> allResults;
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "\n=== " << instance << " (n=" << n << ") ===\n\n";
        std::cout << std::setw(25) << "Config" 
                  << std::setw(10) << "Best" 
                  << std::setw(10) << "Avg" 
                  << std::setw(10) << "Worst"
                  << std::setw(10) << "Gens" << "\n";
        std::cout << std::string(65, '-') << "\n";
        
        for (const auto &config : configs) {
            auto result = runTest(config, instance, n, selectCount, distance, costs, timeLimit, numRuns);
            allResults.push_back(result);
            
            std::cout << std::setw(25) << config.name 
                      << std::setw(10) << result.bestObj
                      << std::setw(10) << std::fixed << std::setprecision(0) << result.avgObj
                      << std::setw(10) << result.worstObj
                      << std::setw(10) << std::setprecision(0) << result.avgGens << "\n";
        }
    }
    
    // Summary
    std::cout << "\n\n=== BEST RESULTS BY INSTANCE ===\n";
    for (const auto &instance : instances) {
        TestResult* best = nullptr;
        for (auto &r : allResults) {
            if (r.instance == instance) {
                if (!best || r.avgObj < best->avgObj) {
                    best = &r;
                }
            }
        }
        if (best) {
            std::cout << instance << ": " << best->configName 
                      << " (Best=" << best->bestObj << ", Avg=" << best->avgObj << ")\n";
        }
    }
    
    return 0;
}
