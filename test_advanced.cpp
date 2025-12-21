// AMSEA Advanced Testing - Testing NEW strategies not yet tried
// Focus on ideas that could significantly improve results

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
#include <set>
#include <map>
#include <unordered_set>
#include <climits>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"

// ============================================================================
// Core Functions (copied from amsea.cpp for standalone testing)
// ============================================================================

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

static std::vector<int> localSearchFast(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n) {
    
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    bool improved = true;
    while (improved) {
        improved = false;
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

static std::vector<int> repairSolution(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount) {
    std::vector<int> sol = partial;
    std::vector<bool> sel(n, false);
    for (int x : sol) sel[x] = true;
    
    if (sol.size() < 2) {
        int best = -1, bestC = INT_MAX;
        for (int i = 0; i < n; i++) if (!sel[i] && costs[i] < bestC) { bestC = costs[i]; best = i; }
        if (sol.empty() && best != -1) { sol.push_back(best); sel[best] = true; }
        if (sol.size() == 1 && selectCount > 1) {
            int s = sol[0], bn = -1, bd = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (sel[i]) continue;
                int d = distance[s][i] + costs[i];
                if (d < bd) { bd = d; bn = i; }
            }
            if (bn != -1) { sol.push_back(bn); sel[bn] = true; }
        }
    }
    
    while (sol.size() < selectCount) {
        int chooseN = -1, chooseP = -1;
        double bestScore = -1e18;
        
        for (int i = 0; i < n; i++) {
            if (sel[i]) continue;
            int b1 = INT_MAX, b2 = INT_MAX, bp = -1;
            for (int p = 0; p < sol.size(); p++) {
                int nxt = (p + 1) % sol.size();
                int d = distance[sol[p]][i] + distance[i][sol[nxt]] - distance[sol[p]][sol[nxt]] + costs[i];
                if (d < b1) { b2 = b1; b1 = d; bp = p + 1; }
                else if (d < b2) b2 = d;
            }
            int regret = (b2 == INT_MAX) ? 0 : (b2 - b1);
            double score = 1.0 * regret - 1.0 * b1;
            if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

// ============================================================================
// NEW STRATEGY 1: Crowding Replacement
// Replace the most SIMILAR solution instead of worst
// ============================================================================

int countCommonNodes(const std::vector<int> &a, const std::vector<int> &b) {
    std::unordered_set<int> setA(a.begin(), a.end());
    int count = 0;
    for (int x : b) if (setA.count(x)) count++;
    return count;
}

int findMostSimilar(const std::vector<int> &offspring, 
                    const std::vector<std::vector<int>> &pop,
                    const std::vector<int> &popObj, int offspringObj) {
    int bestIdx = -1;
    int bestSim = -1;
    
    for (int i = 0; i < pop.size(); i++) {
        // Only consider solutions worse than offspring
        if (popObj[i] > offspringObj) {
            int sim = countCommonNodes(offspring, pop[i]);
            if (sim > bestSim) {
                bestSim = sim;
                bestIdx = i;
            }
        }
    }
    
    // If no worse solution found, find most similar overall
    if (bestIdx == -1) {
        for (int i = 0; i < pop.size(); i++) {
            int sim = countCommonNodes(offspring, pop[i]);
            if (sim > bestSim) {
                bestSim = sim;
                bestIdx = i;
            }
        }
    }
    return bestIdx;
}

// ============================================================================
// NEW STRATEGY 2: Adaptive Stagnation (increases over time)
// ============================================================================

int adaptiveStagnationThreshold(int generation, int maxGens, int baseThreshold) {
    // Starts at baseThreshold, increases to 2x near end
    double progress = std::min(1.0, (double)generation / maxGens);
    return baseThreshold + (int)(baseThreshold * progress);
}

// ============================================================================
// NEW STRATEGY 3: Stronger Perturbation (more moves)
// ============================================================================

std::vector<int> strongPerturb(const std::vector<int> &sol, int n, std::mt19937 &rng) {
    std::vector<int> p = sol;
    int sz = p.size();
    
    // More 2-opt moves (5-8 instead of 2-4)
    int k = 5 + std::uniform_int_distribution<>(0, 3)(rng);
    for (int i = 0; i < k; i++) {
        std::uniform_int_distribution<> d(0, sz - 1);
        int a = d(rng), b = d(rng);
        while (a == b || (a+1)%sz == b || (b+1)%sz == a) b = d(rng);
        if (a > b) std::swap(a, b);
        std::reverse(p.begin() + a + 1, p.begin() + b + 1);
    }
    
    // Higher chance of node swap (50%)
    if (std::uniform_real_distribution<>(0, 1)(rng) < 0.5) {
        std::vector<bool> inS(n, false);
        for (int x : p) inS[x] = true;
        std::vector<int> notS;
        for (int i = 0; i < n; i++) if (!inS[i]) notS.push_back(i);
        if (!notS.empty()) {
            // Swap 2 nodes
            for (int swaps = 0; swaps < 2 && !notS.empty(); swaps++) {
                int pos = std::uniform_int_distribution<>(0, sz-1)(rng);
                int newNode = notS[std::uniform_int_distribution<>(0, notS.size()-1)(rng)];
                notS.erase(std::find(notS.begin(), notS.end(), newNode));
                notS.push_back(p[pos]);
                p[pos] = newNode;
            }
        }
    }
    return p;
}

// ============================================================================
// NEW STRATEGY 4: Destroy & Repair (LNS-style but with variable destroy rate)
// ============================================================================

std::vector<int> destroyRepair(const std::vector<int> &sol, int n, int selectCount,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs, std::mt19937 &rng,
                                double destroyRate) {
    int destroyCount = (int)(selectCount * destroyRate);
    std::vector<bool> keep(selectCount, true);
    std::uniform_int_distribution<> posDist(0, selectCount - 1);
    std::unordered_set<int> destroyed;
    
    for (int i = 0; i < destroyCount; i++) {
        int pos = posDist(rng);
        while (destroyed.count(pos)) pos = posDist(rng);
        destroyed.insert(pos);
        keep[pos] = false;
    }
    
    std::vector<int> partial;
    for (int i = 0; i < selectCount; i++) if (keep[i]) partial.push_back(sol[i]);
    return repairSolution(partial, distance, costs, n, selectCount);
}

// ============================================================================
// NEW STRATEGY 5: Restart when deeply stagnated
// ============================================================================

bool shouldRestart(int stagnationCounter, int threshold) {
    return stagnationCounter > threshold * 3; // Triple the threshold = restart
}

// ============================================================================
// CONFIGURABLE AMSEA with all new strategies
// ============================================================================

struct AdvancedConfig {
    std::string name;
    int populationSize;
    int stagnationThreshold;
    bool useCrowding;
    bool useAdaptiveStagnation;
    bool useStrongPerturb;
    bool useVariableDestroy;
    bool useRestart;
    double destroyRate;
};

struct AdvancedResult {
    int bestObjective;
    std::vector<int> bestSolution;
    long long generations;
    int restarts;
};

AdvancedResult runAdvancedAMSEA(
    const AdvancedConfig &config,
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng) {

    AdvancedResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;
    result.restarts = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize population
    auto initPop = [&]() {
        std::vector<std::vector<int>> pop;
        std::vector<int> popObj;
        std::unordered_set<int> objSeen;
        
        auto addUnique = [&](const std::vector<int> &s) {
            if (pop.size() >= config.populationSize) return;
            auto improved = localSearchFast(s, distance, costs, n);
            int obj = calculateObjective(improved, distance, costs);
            if (!objSeen.count(obj)) {
                objSeen.insert(obj);
                pop.push_back(improved);
                popObj.push_back(obj);
            }
        };
        
        std::vector<int> starts(n);
        std::iota(starts.begin(), starts.end(), 0);
        std::shuffle(starts.begin(), starts.end(), rng);
        
        int perH = config.populationSize / 3 + 1;
        for (int i = 0; i < perH && pop.size() < config.populationSize; i++)
            addUnique(greedyCycle(starts[i % n], selectCount, distance, costs));
        for (int i = 0; i < perH && pop.size() < config.populationSize; i++)
            addUnique(nearestNeighborAny(starts[(i + perH) % n], selectCount, distance, costs));
        for (int i = 0; i < perH && pop.size() < config.populationSize; i++)
            addUnique(greedyRegret2Weighted(starts[(i + 2*perH) % n], selectCount, distance, costs, 1.0, 1.0));
        
        for (int att = 0; att < config.populationSize * 10 && pop.size() < config.populationSize; att++) {
            std::vector<int> all(n); std::iota(all.begin(), all.end(), 0);
            std::shuffle(all.begin(), all.end(), rng);
            addUnique(std::vector<int>(all.begin(), all.begin() + selectCount));
        }
        
        return std::make_tuple(pop, popObj, objSeen);
    };
    
    auto [pop, popObj, objSeen] = initPop();
    
    for (int i = 0; i < pop.size(); i++) {
        if (popObj[i] < result.bestObjective) {
            result.bestObjective = popObj[i];
            result.bestSolution = pop[i];
        }
    }
    
    int stagnation = 0;
    int estimatedMaxGens = 10000; // For adaptive stagnation
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        // Tournament selection (size 3)
        std::uniform_int_distribution<> dist(0, pop.size() - 1);
        auto tournament = [&]() {
            int a = dist(rng), b = dist(rng), c = dist(rng);
            if (popObj[a] <= popObj[b] && popObj[a] <= popObj[c]) return a;
            if (popObj[b] <= popObj[c]) return b;
            return c;
        };
        int p1 = tournament(), p2 = tournament();
        while (p2 == p1) p2 = tournament();
        
        // Recombination: common nodes + repair
        std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
        std::vector<int> partial;
        for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
        
        std::vector<int> offspring = repairSolution(partial, distance, costs, n, selectCount);
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        
        // Replacement strategy
        if (!objSeen.count(offObj)) {
            int replaceIdx;
            if (config.useCrowding) {
                replaceIdx = findMostSimilar(offspring, pop, popObj, offObj);
            } else {
                replaceIdx = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            }
            
            if (offObj < popObj[replaceIdx] || config.useCrowding) {
                objSeen.erase(popObj[replaceIdx]);
                objSeen.insert(offObj);
                pop[replaceIdx] = offspring;
                popObj[replaceIdx] = offObj;
            }
        }
        
        // Update best
        if (offObj < result.bestObjective) {
            result.bestObjective = offObj;
            result.bestSolution = offspring;
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        // Get current stagnation threshold
        int currentThreshold = config.stagnationThreshold;
        if (config.useAdaptiveStagnation) {
            currentThreshold = adaptiveStagnationThreshold(result.generations, estimatedMaxGens, config.stagnationThreshold);
        }
        
        // Stagnation handling
        if (stagnation >= currentThreshold) {
            // Check for restart
            if (config.useRestart && shouldRestart(stagnation, config.stagnationThreshold)) {
                auto [newPop, newPopObj, newObjSeen] = initPop();
                pop = newPop;
                popObj = newPopObj;
                objSeen = newObjSeen;
                result.restarts++;
            } else {
                // Perturb worst 2 solutions
                for (int i = 0; i < 2 && i < pop.size(); i++) {
                    int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
                    
                    std::vector<int> perturbed;
                    if (config.useVariableDestroy) {
                        perturbed = destroyRepair(pop[worst], n, selectCount, distance, costs, rng, config.destroyRate);
                    } else if (config.useStrongPerturb) {
                        perturbed = strongPerturb(pop[worst], n, rng);
                    } else {
                        // Default perturbation
                        perturbed = pop[worst];
                        int sz = perturbed.size();
                        for (int k = 0; k < 3; k++) {
                            std::uniform_int_distribution<> d(0, sz - 1);
                            int a = d(rng), b = d(rng);
                            if (a > b) std::swap(a, b);
                            if (a != b) std::reverse(perturbed.begin() + a + 1, perturbed.begin() + b + 1);
                        }
                    }
                    
                    perturbed = localSearchFast(perturbed, distance, costs, n);
                    int pObj = calculateObjective(perturbed, distance, costs);
                    
                    if (!objSeen.count(pObj)) {
                        objSeen.erase(popObj[worst]);
                        objSeen.insert(pObj);
                        pop[worst] = perturbed;
                        popObj[worst] = pObj;
                        
                        if (pObj < result.bestObjective) {
                            result.bestObjective = pObj;
                            result.bestSolution = perturbed;
                        }
                    }
                }
            }
            stagnation = 0;
        }
    }
    
    return result;
}

// ============================================================================
// Test Runner
// ============================================================================

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
    std::cout << "AMSEA Advanced Strategy Testing\n";
    std::cout << "================================\n\n";
    
    // Define NEW configurations to test
    std::vector<AdvancedConfig> configs = {
        // Baseline for comparison
        {"Baseline-Pop20", 20, 30, false, false, false, false, false, 0.3},
        
        // Test Crowding
        {"Crowding-Pop20", 20, 30, true, false, false, false, false, 0.3},
        {"Crowding-Pop15", 15, 25, true, false, false, false, false, 0.3},
        
        // Test Adaptive Stagnation
        {"Adaptive-Pop20", 20, 20, false, true, false, false, false, 0.3},
        
        // Test Strong Perturbation
        {"StrongPerturb-Pop20", 20, 30, false, false, true, false, false, 0.3},
        
        // Test Variable Destroy Rate
        {"Destroy20-Pop20", 20, 30, false, false, false, true, false, 0.2},
        {"Destroy40-Pop20", 20, 30, false, false, false, true, false, 0.4},
        {"Destroy50-Pop20", 20, 30, false, false, false, true, false, 0.5},
        
        // Test Restart
        {"Restart-Pop20", 20, 30, false, false, false, false, true, 0.3},
        
        // Combinations
        {"Crowding+StrongP", 20, 25, true, false, true, false, false, 0.3},
        {"Crowding+Destroy40", 20, 25, true, false, false, true, false, 0.4},
        {"Crowding+Adaptive", 20, 20, true, true, false, false, false, 0.3},
        {"All-Combined", 20, 20, true, true, true, false, false, 0.3},
    };
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;
    const int numRuns = 10;
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "\n=== " << instance << " (n=" << n << ") ===\n\n";
        std::cout << std::setw(22) << "Config" 
                  << std::setw(8) << "Best" 
                  << std::setw(10) << "Avg" 
                  << std::setw(8) << "Worst"
                  << std::setw(8) << "Gens"
                  << std::setw(6) << "Rest" << "\n";
        std::cout << std::string(62, '-') << "\n";
        
        for (const auto &config : configs) {
            std::vector<int> objectives;
            long long totalGens = 0;
            int totalRestarts = 0;
            
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runAdvancedAMSEA(config, n, selectCount, distance, costs, timeLimit, rng);
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
                totalRestarts += result.restarts;
            }
            
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = 0;
            for (int o : objectives) avg += o;
            avg /= numRuns;
            
            std::cout << std::setw(22) << config.name 
                      << std::setw(8) << best
                      << std::setw(10) << std::fixed << std::setprecision(0) << avg
                      << std::setw(8) << worst
                      << std::setw(8) << (totalGens / numRuns)
                      << std::setw(6) << (totalRestarts / numRuns) << "\n";
        }
    }
    
    std::cout << "\n\nDone!\n";
    return 0;
}
