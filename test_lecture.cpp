// Test lecture-based improvements: ERX, Island Model, Long-term Memory
// Focus on techniques from slides that haven't been tested yet

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
#include <unordered_set>
#include <unordered_map>
#include <climits>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"

// ============================================================================
// Core Functions (copied for standalone testing)
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
// NEW OPERATOR 1: Edge Recombination Crossover (ERX) from lecture slides
// "Select a random element. The next element is subsequent element from 
//  one (randomly selected) parent. If both subsequent elements are already 
//  selected, the next element is selected randomly"
// ============================================================================

std::vector<int> edgeRecombinationCrossover(
    const std::vector<int> &p1, const std::vector<int> &p2,
    int n, int selectCount, std::mt19937 &rng) {
    
    // Build adjacency lists from both parents (edges in both directions)
    std::unordered_map<int, std::vector<int>> adj;
    
    auto addEdge = [&](int a, int b) {
        adj[a].push_back(b);
        adj[b].push_back(a);
    };
    
    // Add edges from parent 1
    for (int i = 0; i < p1.size(); i++) {
        addEdge(p1[i], p1[(i + 1) % p1.size()]);
    }
    // Add edges from parent 2 (may duplicate, that's OK)
    for (int i = 0; i < p2.size(); i++) {
        addEdge(p2[i], p2[(i + 1) % p2.size()]);
    }
    
    // Get common nodes
    std::unordered_set<int> n1(p1.begin(), p1.end());
    std::unordered_set<int> n2(p2.begin(), p2.end());
    std::vector<int> commonNodes;
    for (int x : n1) if (n2.count(x)) commonNodes.push_back(x);
    
    if (commonNodes.empty()) {
        // No common nodes, return one parent
        return p1;
    }
    
    // Start from a random common node
    std::vector<int> result;
    std::unordered_set<int> used;
    
    int current = commonNodes[std::uniform_int_distribution<>(0, commonNodes.size()-1)(rng)];
    result.push_back(current);
    used.insert(current);
    
    while (result.size() < selectCount) {
        // Find next node: prefer neighbors from adjacency list
        int next = -1;
        
        if (adj.count(current)) {
            // Prefer neighbors with fewest connections (greedy ERX heuristic)
            int bestNeighbor = -1;
            int bestDegree = INT_MAX;
            
            for (int neighbor : adj[current]) {
                if (!used.count(neighbor)) {
                    // Count remaining connections of this neighbor
                    int degree = 0;
                    if (adj.count(neighbor)) {
                        for (int nn : adj[neighbor]) {
                            if (!used.count(nn)) degree++;
                        }
                    }
                    if (degree < bestDegree) {
                        bestDegree = degree;
                        bestNeighbor = neighbor;
                    }
                }
            }
            next = bestNeighbor;
        }
        
        // If no valid neighbor, pick random unused node
        if (next == -1) {
            std::vector<int> available;
            for (int node : commonNodes) {
                if (!used.count(node)) available.push_back(node);
            }
            if (available.empty()) break;
            next = available[std::uniform_int_distribution<>(0, available.size()-1)(rng)];
        }
        
        result.push_back(next);
        used.insert(next);
        current = next;
    }
    
    return result;
}

// ============================================================================
// NEW: Island Model Implementation
// Multiple populations evolving independently with periodic migration
// ============================================================================

struct IslandResult {
    int bestObjective;
    std::vector<int> bestSolution;
    long long generations;
    int migrations;
};

IslandResult runIslandModel(
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng,
    int numIslands = 4,
    int migrationInterval = 100) {

    IslandResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;
    result.migrations = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    const int POP_PER_ISLAND = 5;  // Smaller populations per island
    const int STAGNATION = 20;
    
    // Initialize multiple islands
    std::vector<std::vector<std::vector<int>>> islands(numIslands);
    std::vector<std::vector<int>> islandObjs(numIslands);
    std::vector<std::unordered_set<int>> islandSeen(numIslands);
    
    auto initIsland = [&](int island) {
        std::vector<int> starts(n);
        std::iota(starts.begin(), starts.end(), 0);
        std::shuffle(starts.begin(), starts.end(), rng);
        
        for (int i = 0; i < POP_PER_ISLAND * 3 && islands[island].size() < POP_PER_ISLAND; i++) {
            std::vector<int> sol;
            if (i % 3 == 0) sol = greedyCycle(starts[i % n], selectCount, distance, costs);
            else if (i % 3 == 1) sol = nearestNeighborAny(starts[i % n], selectCount, distance, costs);
            else sol = greedyRegret2Weighted(starts[i % n], selectCount, distance, costs, 1.0, 1.0);
            
            sol = localSearchFast(sol, distance, costs, n);
            int obj = calculateObjective(sol, distance, costs);
            
            if (!islandSeen[island].count(obj)) {
                islandSeen[island].insert(obj);
                islands[island].push_back(sol);
                islandObjs[island].push_back(obj);
                
                if (obj < result.bestObjective) {
                    result.bestObjective = obj;
                    result.bestSolution = sol;
                }
            }
        }
    };
    
    for (int i = 0; i < numIslands; i++) {
        initIsland(i);
    }
    
    std::vector<int> stagnation(numIslands, 0);
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        // Evolve each island
        for (int island = 0; island < numIslands; island++) {
            if (islands[island].size() < 2) continue;
            
            // Tournament selection within island
            std::uniform_int_distribution<> dist(0, islands[island].size() - 1);
            auto tournament = [&]() {
                int a = dist(rng), b = dist(rng);
                return (islandObjs[island][a] <= islandObjs[island][b]) ? a : b;
            };
            int p1 = tournament(), p2 = tournament();
            while (p2 == p1) p2 = tournament();
            
            // Recombination: common nodes + repair
            std::unordered_set<int> n2(islands[island][p2].begin(), islands[island][p2].end());
            std::vector<int> partial;
            for (int x : islands[island][p1]) if (n2.count(x)) partial.push_back(x);
            
            std::vector<int> offspring = repairSolution(partial, distance, costs, n, selectCount);
            offspring = localSearchFast(offspring, distance, costs, n);
            int offObj = calculateObjective(offspring, distance, costs);
            
            // Replace worst if better
            if (!islandSeen[island].count(offObj)) {
                int worstIdx = std::max_element(islandObjs[island].begin(), islandObjs[island].end()) 
                               - islandObjs[island].begin();
                if (offObj < islandObjs[island][worstIdx]) {
                    islandSeen[island].erase(islandObjs[island][worstIdx]);
                    islandSeen[island].insert(offObj);
                    islands[island][worstIdx] = offspring;
                    islandObjs[island][worstIdx] = offObj;
                }
            }
            
            if (offObj < result.bestObjective) {
                result.bestObjective = offObj;
                result.bestSolution = offspring;
                stagnation[island] = 0;
            } else {
                stagnation[island]++;
            }
        }
        
        // Migration: best from each island moves to next island
        if (result.generations % migrationInterval == 0) {
            result.migrations++;
            for (int island = 0; island < numIslands; island++) {
                int nextIsland = (island + 1) % numIslands;
                
                // Find best in current island
                int bestIdx = std::min_element(islandObjs[island].begin(), islandObjs[island].end())
                              - islandObjs[island].begin();
                auto migrant = islands[island][bestIdx];
                int migrantObj = islandObjs[island][bestIdx];
                
                // Add to next island if unique
                if (!islandSeen[nextIsland].count(migrantObj)) {
                    int worstIdx = std::max_element(islandObjs[nextIsland].begin(), islandObjs[nextIsland].end())
                                   - islandObjs[nextIsland].begin();
                    if (migrantObj < islandObjs[nextIsland][worstIdx]) {
                        islandSeen[nextIsland].erase(islandObjs[nextIsland][worstIdx]);
                        islandSeen[nextIsland].insert(migrantObj);
                        islands[nextIsland][worstIdx] = migrant;
                        islandObjs[nextIsland][worstIdx] = migrantObj;
                    }
                }
            }
        }
    }
    
    return result;
}

// ============================================================================
// NEW: Long-term Memory - Track edge frequencies
// ============================================================================

struct LTMResult {
    int bestObjective;
    std::vector<int> bestSolution;
    long long generations;
};

LTMResult runWithLongTermMemory(
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng) {

    LTMResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Long-term memory: edge frequencies
    std::unordered_map<long long, int> edgeFreq;
    auto edgeKey = [](int a, int b) { 
        return (long long)std::min(a,b) * 1000000 + std::max(a,b); 
    };
    
    auto updateFreq = [&](const std::vector<int>& sol) {
        for (int i = 0; i < sol.size(); i++) {
            edgeFreq[edgeKey(sol[i], sol[(i+1) % sol.size()])]++;
        }
    };
    
    const int POP_SIZE = 20;
    const int STAGNATION = 30;
    
    // Initialize population
    std::vector<std::vector<int>> pop;
    std::vector<int> popObj;
    std::unordered_set<int> objSeen;
    
    std::vector<int> starts(n);
    std::iota(starts.begin(), starts.end(), 0);
    std::shuffle(starts.begin(), starts.end(), rng);
    
    for (int i = 0; i < POP_SIZE * 3 && pop.size() < POP_SIZE; i++) {
        std::vector<int> sol;
        if (i % 3 == 0) sol = greedyCycle(starts[i % n], selectCount, distance, costs);
        else if (i % 3 == 1) sol = nearestNeighborAny(starts[i % n], selectCount, distance, costs);
        else sol = greedyRegret2Weighted(starts[i % n], selectCount, distance, costs, 1.0, 1.0);
        
        sol = localSearchFast(sol, distance, costs, n);
        int obj = calculateObjective(sol, distance, costs);
        
        if (!objSeen.count(obj)) {
            objSeen.insert(obj);
            pop.push_back(sol);
            popObj.push_back(obj);
            updateFreq(sol);  // Update edge frequencies
            
            if (obj < result.bestObjective) {
                result.bestObjective = obj;
                result.bestSolution = sol;
            }
        }
    }
    
    int stagnation = 0;
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        // Tournament selection
        std::uniform_int_distribution<> dist(0, pop.size() - 1);
        auto tournament = [&]() {
            int a = dist(rng), b = dist(rng), c = dist(rng);
            if (popObj[a] <= popObj[b] && popObj[a] <= popObj[c]) return a;
            if (popObj[b] <= popObj[c]) return b;
            return c;
        };
        int p1 = tournament(), p2 = tournament();
        while (p2 == p1) p2 = tournament();
        
        // Recombination with LTM-guided repair
        std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
        std::vector<int> partial;
        for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
        
        // Enhanced repair using edge frequencies (intensification)
        std::vector<int> offspring = partial;
        std::vector<bool> sel(n, false);
        for (int x : offspring) sel[x] = true;
        
        while (offspring.size() < selectCount) {
            int chooseN = -1, chooseP = -1;
            double bestScore = -1e18;
            
            for (int i = 0; i < n; i++) {
                if (sel[i]) continue;
                int b1 = INT_MAX, bp = -1;
                for (int p = 0; p < offspring.size(); p++) {
                    int nxt = (p + 1) % offspring.size();
                    int d = distance[offspring[p]][i] + distance[i][offspring[nxt]] 
                            - distance[offspring[p]][offspring[nxt]] + costs[i];
                    if (d < b1) { b1 = d; bp = p + 1; }
                }
                
                // Bonus for edges that appeared frequently in good solutions
                int freqBonus = 0;
                if (bp > 0 && bp <= offspring.size()) {
                    freqBonus += edgeFreq[edgeKey(offspring[bp-1], i)];
                    freqBonus += edgeFreq[edgeKey(i, offspring[bp % offspring.size()])];
                }
                
                double score = -b1 + 0.1 * freqBonus;  // Combine cost with frequency bonus
                if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
            }
            if (chooseN == -1) break;
            offspring.insert(offspring.begin() + chooseP, chooseN);
            sel[chooseN] = true;
        }
        
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        updateFreq(offspring);  // Update frequencies
        
        // Replace worst if better
        if (!objSeen.count(offObj)) {
            int worstIdx = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            if (offObj < popObj[worstIdx]) {
                objSeen.erase(popObj[worstIdx]);
                objSeen.insert(offObj);
                pop[worstIdx] = offspring;
                popObj[worstIdx] = offObj;
            }
        }
        
        if (offObj < result.bestObjective) {
            result.bestObjective = offObj;
            result.bestSolution = offspring;
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        // Perturbation on stagnation
        if (stagnation >= STAGNATION) {
            int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            auto perturbed = pop[worst];
            int sz = perturbed.size();
            for (int k = 0; k < 5; k++) {
                std::uniform_int_distribution<> d(0, sz - 1);
                int a = d(rng), b = d(rng);
                if (a > b) std::swap(a, b);
                if (a != b) std::reverse(perturbed.begin() + a + 1, perturbed.begin() + b + 1);
            }
            perturbed = localSearchFast(perturbed, distance, costs, n);
            int pObj = calculateObjective(perturbed, distance, costs);
            
            if (!objSeen.count(pObj)) {
                objSeen.erase(popObj[worst]);
                objSeen.insert(pObj);
                pop[worst] = perturbed;
                popObj[worst] = pObj;
                updateFreq(perturbed);
                if (pObj < result.bestObjective) {
                    result.bestObjective = pObj;
                    result.bestSolution = perturbed;
                }
            }
            stagnation = 0;
        }
    }
    
    return result;
}

// ============================================================================
// Baseline for comparison
// ============================================================================

struct BaselineResult {
    int bestObjective;
    long long generations;
};

BaselineResult runBaseline(
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng) {

    BaselineResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    const int POP_SIZE = 20;
    const int STAGNATION = 30;
    
    std::vector<std::vector<int>> pop;
    std::vector<int> popObj;
    std::unordered_set<int> objSeen;
    
    std::vector<int> starts(n);
    std::iota(starts.begin(), starts.end(), 0);
    std::shuffle(starts.begin(), starts.end(), rng);
    
    for (int i = 0; i < POP_SIZE * 3 && pop.size() < POP_SIZE; i++) {
        std::vector<int> sol;
        if (i % 3 == 0) sol = greedyCycle(starts[i % n], selectCount, distance, costs);
        else if (i % 3 == 1) sol = nearestNeighborAny(starts[i % n], selectCount, distance, costs);
        else sol = greedyRegret2Weighted(starts[i % n], selectCount, distance, costs, 1.0, 1.0);
        
        sol = localSearchFast(sol, distance, costs, n);
        int obj = calculateObjective(sol, distance, costs);
        
        if (!objSeen.count(obj)) {
            objSeen.insert(obj);
            pop.push_back(sol);
            popObj.push_back(obj);
            if (obj < result.bestObjective) result.bestObjective = obj;
        }
    }
    
    int stagnation = 0;
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        std::uniform_int_distribution<> dist(0, pop.size() - 1);
        auto tournament = [&]() {
            int a = dist(rng), b = dist(rng), c = dist(rng);
            if (popObj[a] <= popObj[b] && popObj[a] <= popObj[c]) return a;
            if (popObj[b] <= popObj[c]) return b;
            return c;
        };
        int p1 = tournament(), p2 = tournament();
        while (p2 == p1) p2 = tournament();
        
        std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
        std::vector<int> partial;
        for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
        
        std::vector<int> offspring = repairSolution(partial, distance, costs, n, selectCount);
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        
        if (!objSeen.count(offObj)) {
            int worstIdx = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            if (offObj < popObj[worstIdx]) {
                objSeen.erase(popObj[worstIdx]);
                objSeen.insert(offObj);
                pop[worstIdx] = offspring;
                popObj[worstIdx] = offObj;
            }
        }
        
        if (offObj < result.bestObjective) {
            result.bestObjective = offObj;
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        if (stagnation >= STAGNATION) {
            int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            auto perturbed = pop[worst];
            int sz = perturbed.size();
            for (int k = 0; k < 5; k++) {
                std::uniform_int_distribution<> d(0, sz - 1);
                int a = d(rng), b = d(rng);
                if (a > b) std::swap(a, b);
                if (a != b) std::reverse(perturbed.begin() + a + 1, perturbed.begin() + b + 1);
            }
            perturbed = localSearchFast(perturbed, distance, costs, n);
            int pObj = calculateObjective(perturbed, distance, costs);
            
            if (!objSeen.count(pObj)) {
                objSeen.erase(popObj[worst]);
                objSeen.insert(pObj);
                pop[worst] = perturbed;
                popObj[worst] = pObj;
                if (pObj < result.bestObjective) result.bestObjective = pObj;
            }
            stagnation = 0;
        }
    }
    
    return result;
}

// ============================================================================
// Load Instance
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
    std::cout << "Lecture-Based Improvements Test\n";
    std::cout << "================================\n\n";
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;
    const int numRuns = 10;
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "\n=== " << instance << " (n=" << n << ") ===\n\n";
        std::cout << std::setw(18) << "Method" 
                  << std::setw(8) << "Best" 
                  << std::setw(10) << "Avg" 
                  << std::setw(8) << "Worst"
                  << std::setw(10) << "Gens" << "\n";
        std::cout << std::string(54, '-') << "\n";
        
        // Test Baseline
        {
            std::vector<int> objectives;
            long long totalGens = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runBaseline(n, selectCount, distance, costs, timeLimit, rng);
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            std::cout << std::setw(18) << "Baseline" 
                      << std::setw(8) << best
                      << std::setw(10) << std::fixed << std::setprecision(0) << avg
                      << std::setw(8) << worst
                      << std::setw(10) << (totalGens / numRuns) << "\n";
        }
        
        // Test Island Model
        {
            std::vector<int> objectives;
            long long totalGens = 0;
            int totalMigrations = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runIslandModel(n, selectCount, distance, costs, timeLimit, rng, 4, 100);
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
                totalMigrations += result.migrations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            std::cout << std::setw(18) << "Island-4x5" 
                      << std::setw(8) << best
                      << std::setw(10) << std::fixed << std::setprecision(0) << avg
                      << std::setw(8) << worst
                      << std::setw(10) << (totalGens / numRuns) << "\n";
        }
        
        // Test Long-Term Memory
        {
            std::vector<int> objectives;
            long long totalGens = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runWithLongTermMemory(n, selectCount, distance, costs, timeLimit, rng);
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            std::cout << std::setw(18) << "LongTermMemory" 
                      << std::setw(8) << best
                      << std::setw(10) << std::fixed << std::setprecision(0) << avg
                      << std::setw(8) << worst
                      << std::setw(10) << (totalGens / numRuns) << "\n";
        }
    }
    
    std::cout << "\nDone!\n";
    return 0;
}
