// AMSEA Islands - 2 Islands (10+10), Migration every 200 generations
// Based on best tested configuration: TSPA 69152 avg, TSPB 43650 avg, 6263 gens
// Uses: 3 operators (CommonNodes, Parent, PathRelink) + Greedy LS

#include "../include/amseaIslands.h"
#include "../include/calculateObjective.h"
#include "../include/greedyCycle.h"
#include "../include/nearestNeighborAny.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <numeric>
#include <unordered_set>

// ============================================================================
// Greedy Local Search (4x faster than Steepest!)
// ============================================================================

static std::vector<int> localSearchGreedy(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n, std::mt19937 &rng) {
    
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    bool improved = true;
    while (improved) {
        improved = false;
        int sz = sol.size();
        int startI = std::uniform_int_distribution<>(0, sz - 1)(rng);
        
        for (int iter = 0; iter < sz && !improved; iter++) {
            int i = (startI + iter) % sz;
            for (int j = i + 2; j < sz; j++) {
                if (i == 0 && j == sz - 1) continue;
                int a = sol[i], b = sol[(i + 1) % sz], c = sol[j], d = sol[(j + 1) % sz];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
                if (delta < 0) {
                    std::reverse(sol.begin() + i + 1, sol.begin() + j + 1);
                    improved = true; break;
                }
            }
        }
        if (improved) continue;
        
        int startPos = std::uniform_int_distribution<>(0, sz - 1)(rng);
        for (int iter = 0; iter < sz && !improved; iter++) {
            int pos = (startPos + iter) % sz;
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int prev = (pos - 1 + sz) % sz, next = (pos + 1) % sz;
                int delta = (distance[sol[prev]][node] + distance[node][sol[next]] + costs[node])
                          - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
                if (delta < 0) {
                    inSolution[sol[pos]] = false;
                    inSolution[node] = true;
                    sol[pos] = node;
                    improved = true; break;
                }
            }
        }
    }
    return sol;
}

// ============================================================================
// Repair Function (Weighted 2-Regret)
// ============================================================================

static std::vector<int> repairStandard(const std::vector<int> &partial,
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
            if (regret - b1 > bestScore) { bestScore = regret - b1; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

// ============================================================================
// AMSEA Islands - Main Function
// Config: 2 islands (10+10), migrate every 200 gens
// ============================================================================

AMSEAIslandsResult amseaIslands(int n, int selectCount,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, double timeLimit,
                  std::mt19937 &rng, int populationSize) {
    
    AMSEAIslandsResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;
    for (int i = 0; i < 3; i++) { result.operatorSuccesses[i] = 0; result.operatorAttempts[i] = 1; }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Best tested configuration
    const int NUM_ISLANDS = 2;
    const int ISLAND_SIZE = populationSize / NUM_ISLANDS;  // 10 per island
    const int MIGRATION_INTERVAL = 200;
    const int STAGNATION = 30;
    
    // Initialize data structures
    std::vector<std::vector<std::vector<int>>> islands(NUM_ISLANDS);
    std::vector<std::vector<int>> islandObjs(NUM_ISLANDS);
    std::vector<std::unordered_set<int>> islandSeen(NUM_ISLANDS);
    std::vector<int> stagnation(NUM_ISLANDS, 0);
    
    // Initialize each island
    std::vector<int> starts(n);
    std::iota(starts.begin(), starts.end(), 0);
    std::shuffle(starts.begin(), starts.end(), rng);
    
    int startIdx = 0;
    for (int island = 0; island < NUM_ISLANDS; island++) {
        for (int i = 0; i < ISLAND_SIZE && islands[island].size() < ISLAND_SIZE; i++) {
            auto improved = localSearchGreedy(greedyCycle(starts[startIdx++ % n], selectCount, distance, costs), distance, costs, n, rng);
            int obj = calculateObjective(improved, distance, costs);
            if (!islandSeen[island].count(obj)) {
                islandSeen[island].insert(obj);
                islands[island].push_back(improved);
                islandObjs[island].push_back(obj);
                if (obj < result.bestObjective) {
                    result.bestObjective = obj;
                    result.bestSolution = improved;
                }
            }
        }
    }
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        for (int island = 0; island < NUM_ISLANDS; island++) {
            auto& pop = islands[island];
            auto& popObj = islandObjs[island];
            auto& objSeen = islandSeen[island];
            
            if (pop.size() < 2) continue;
            
            // Random parent selection (like proven implementation)
            std::uniform_int_distribution<> dist(0, pop.size() - 1);
            int p1 = dist(rng), p2 = dist(rng);
            while (p2 == p1) p2 = dist(rng);
            
            // Random operator from 3 operators
            int op = std::uniform_int_distribution<>(0, 2)(rng);
            result.operatorAttempts[op]++;
            std::vector<int> offspring;
            
            if (op == 0) {
                // CommonNodes
                std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
                std::vector<int> partial;
                for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
                offspring = repairStandard(partial, distance, costs, n, selectCount);
            } else if (op == 1) {
                // Parent
                offspring = pop[p1];
                int k = 2 + std::uniform_int_distribution<>(0, 2)(rng);
                for (int i = 0; i < k; i++) {
                    int sz = offspring.size();
                    int a = std::uniform_int_distribution<>(0, sz - 1)(rng);
                    int b = std::uniform_int_distribution<>(0, sz - 1)(rng);
                    if (a > b) std::swap(a, b);
                    if (a != b) std::reverse(offspring.begin() + a + 1, offspring.begin() + b + 1);
                }
            } else {
                // PathRelink
                std::unordered_set<int> inP1(pop[p1].begin(), pop[p1].end());
                std::vector<int> onlyP2;
                for (int x : pop[p2]) if (!inP1.count(x)) onlyP2.push_back(x);
                
                offspring = pop[p1];
                std::vector<bool> inOff(n, false);
                for (int x : offspring) inOff[x] = true;
                
                int steps = std::min((int)onlyP2.size(), 10);
                for (int s = 0; s < steps; s++) {
                    int bestNew = -1, bestPos = -1, bestDelta = INT_MAX;
                    for (int node : onlyP2) {
                        if (inOff[node]) continue;
                        for (int pos = 0; pos < offspring.size(); pos++) {
                            int prev = (pos - 1 + offspring.size()) % offspring.size();
                            int next = (pos + 1) % offspring.size();
                            int delta = distance[offspring[prev]][node] + distance[node][offspring[next]] + costs[node]
                                      - distance[offspring[prev]][offspring[pos]] - distance[offspring[pos]][offspring[next]] - costs[offspring[pos]];
                            if (delta < bestDelta) { bestDelta = delta; bestNew = node; bestPos = pos; }
                        }
                    }
                    if (bestNew != -1) {
                        inOff[offspring[bestPos]] = false;
                        inOff[bestNew] = true;
                        offspring[bestPos] = bestNew;
                    }
                }
            }
            
            offspring = localSearchGreedy(offspring, distance, costs, n, rng);
            int offObj = calculateObjective(offspring, distance, costs);
            
            int worstObj = *std::max_element(popObj.begin(), popObj.end());
            if (offObj < worstObj) {
                result.operatorSuccesses[op]++;
            }
            
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
                stagnation[island] = 0;
            } else {
                stagnation[island]++;
            }
            
            // Stagnation handling
            if (stagnation[island] >= STAGNATION) {
                int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
                auto perturbed = pop[worst];
                int k = 5 + std::uniform_int_distribution<>(0, 3)(rng);
                for (int i = 0; i < k; i++) {
                    int sz = perturbed.size();
                    int a = std::uniform_int_distribution<>(0, sz - 1)(rng);
                    int b = std::uniform_int_distribution<>(0, sz - 1)(rng);
                    if (a > b) std::swap(a, b);
                    if (a != b) std::reverse(perturbed.begin() + a + 1, perturbed.begin() + b + 1);
                }
                perturbed = localSearchGreedy(perturbed, distance, costs, n, rng);
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
                stagnation[island] = 0;
            }
        }
        
        // Migration every MIGRATION_INTERVAL generations
        if (result.generations % MIGRATION_INTERVAL == 0) {
            for (int island = 0; island < NUM_ISLANDS; island++) {
                if (islands[island].empty()) continue;
                int bestIdx = std::min_element(islandObjs[island].begin(), islandObjs[island].end()) 
                            - islandObjs[island].begin();
                int targetIsland = (island + 1) % NUM_ISLANDS;
                if (!islandSeen[targetIsland].count(islandObjs[island][bestIdx])) {
                    int worstIdx = std::max_element(islandObjs[targetIsland].begin(), 
                                                    islandObjs[targetIsland].end()) 
                                 - islandObjs[targetIsland].begin();
                    if (islandObjs[island][bestIdx] < islandObjs[targetIsland][worstIdx]) {
                        islandSeen[targetIsland].erase(islandObjs[targetIsland][worstIdx]);
                        islandSeen[targetIsland].insert(islandObjs[island][bestIdx]);
                        islands[targetIsland][worstIdx] = islands[island][bestIdx];
                        islandObjs[targetIsland][worstIdx] = islandObjs[island][bestIdx];
                    }
                }
            }
        }
    }
    
    result.totalTime = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - startTime).count();
    return result;
}
