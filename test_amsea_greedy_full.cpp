// AMSEA with Greedy Local Search - Test Script
// Tests if faster Greedy LS can improve AMSEA performance
// Does NOT modify the main amsea.cpp

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
#include <climits>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"

// ============================================================================
// Greedy Local Search (takes first improving move)
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
        
        // 2-opt - take first improving
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
        
        // Node exchange - take first improving
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
// Steepest Local Search (for comparison)
// ============================================================================
static std::vector<int> localSearchSteepest(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n, std::mt19937 &) {
    
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    bool improved = true;
    while (improved) {
        improved = false;
        int bestDelta = 0, bestType = -1, bestPos1 = -1, bestPos2 = -1, bestNode = -1;
        int sz = sol.size();
        
        for (int i = 0; i < sz; i++) {
            for (int j = i + 2; j < sz; j++) {
                if (i == 0 && j == sz - 1) continue;
                int a = sol[i], b = sol[(i + 1) % sz], c = sol[j], d = sol[(j + 1) % sz];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
                if (delta < bestDelta) { bestDelta = delta; bestType = 0; bestPos1 = i; bestPos2 = j; }
            }
        }
        
        for (int pos = 0; pos < sz; pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int prev = (pos - 1 + sz) % sz, next = (pos + 1) % sz;
                int delta = (distance[sol[prev]][node] + distance[node][sol[next]] + costs[node])
                          - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
                if (delta < bestDelta) { bestDelta = delta; bestType = 1; bestPos1 = pos; bestNode = node; }
            }
        }
        
        if (bestDelta < 0) {
            improved = true;
            if (bestType == 0) std::reverse(sol.begin() + bestPos1 + 1, sol.begin() + bestPos2 + 1);
            else { inSolution[sol[bestPos1]] = false; inSolution[bestNode] = true; sol[bestPos1] = bestNode; }
        }
    }
    return sol;
}

// ============================================================================
// Repair (regret-weighted insertion)
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
// AMSEA with configurable LS
// ============================================================================
struct Result { int bestObjective; long long generations; };

template<typename LSFunc>
Result runAMSEA(int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, double timeLimit, std::mt19937 &rng,
    LSFunc localSearch, const char* lsName) {

    Result result;
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
    
    auto addUnique = [&](const std::vector<int> &s) {
        if (pop.size() >= POP_SIZE) return;
        auto improved = localSearch(s, distance, costs, n, rng);
        int obj = calculateObjective(improved, distance, costs);
        if (!objSeen.count(obj)) {
            objSeen.insert(obj);
            pop.push_back(improved);
            popObj.push_back(obj);
            if (obj < result.bestObjective) result.bestObjective = obj;
        }
    };
    
    int perH = POP_SIZE / 3 + 1;
    for (int i = 0; i < perH && pop.size() < POP_SIZE; i++)
        addUnique(greedyCycle(starts[i % n], selectCount, distance, costs));
    for (int i = 0; i < perH && pop.size() < POP_SIZE; i++)
        addUnique(nearestNeighborAny(starts[(i + perH) % n], selectCount, distance, costs));
    for (int i = 0; i < perH && pop.size() < POP_SIZE; i++)
        addUnique(greedyRegret2Weighted(starts[(i + 2*perH) % n], selectCount, distance, costs, 1.0, 1.0));
    
    for (int att = 0; att < POP_SIZE * 10 && pop.size() < POP_SIZE; att++) {
        std::vector<int> all(n); std::iota(all.begin(), all.end(), 0);
        std::shuffle(all.begin(), all.end(), rng);
        addUnique(std::vector<int>(all.begin(), all.begin() + selectCount));
    }
    
    int stagnation = 0;
    double opWeights[3] = {1.0, 1.0, 1.0};
    int opSuccess[3] = {0, 0, 0};
    int opTotal[3] = {0, 0, 0};
    
    auto selectOp = [&]() {
        double sum = opWeights[0] + opWeights[1] + opWeights[2];
        double r = std::uniform_real_distribution<>(0, sum)(rng);
        if (r < opWeights[0]) return 0;
        if (r < opWeights[0] + opWeights[1]) return 1;
        return 2;
    };
    
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
        
        int op = selectOp();
        std::vector<int> offspring;
        
        if (op == 0) {
            // CommonNodes
            std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
            std::vector<int> partial;
            for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
            offspring = repairStandard(partial, distance, costs, n, selectCount);
        } else if (op == 1) {
            // Parent (copy with perturbation)
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
            std::unordered_set<int> inP2(pop[p2].begin(), pop[p2].end());
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
        
        opTotal[op]++;
        offspring = localSearch(offspring, distance, costs, n, rng);
        int offObj = calculateObjective(offspring, distance, costs);
        
        if (!objSeen.count(offObj)) {
            int worstIdx = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            if (offObj < popObj[worstIdx]) {
                objSeen.erase(popObj[worstIdx]);
                objSeen.insert(offObj);
                pop[worstIdx] = offspring;
                popObj[worstIdx] = offObj;
                opSuccess[op]++;
            }
        }
        
        if (offObj < result.bestObjective) {
            result.bestObjective = offObj;
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        // Update operator weights
        if (result.generations % 100 == 0) {
            for (int i = 0; i < 3; i++) {
                if (opTotal[i] > 0) {
                    double rate = (double)opSuccess[i] / opTotal[i];
                    opWeights[i] = 0.9 * opWeights[i] + 0.1 * (0.5 + rate);
                }
            }
        }
        
        if (stagnation >= STAGNATION) {
            int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            auto perturbed = pop[worst];
            int sz = perturbed.size();
            int k = 5 + std::uniform_int_distribution<>(0, 3)(rng);
            for (int i = 0; i < k; i++) {
                std::uniform_int_distribution<> d(0, sz - 1);
                int a = d(rng), b = d(rng);
                if (a > b) std::swap(a, b);
                if (a != b) std::reverse(perturbed.begin() + a + 1, perturbed.begin() + b + 1);
            }
            perturbed = localSearch(perturbed, distance, costs, n, rng);
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
    std::cout << "AMSEA: Greedy LS vs Steepest LS Comparison\n";
    std::cout << "==========================================\n";
    std::cout << "20 runs per config, 1 second time limit\n\n";
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;
    const int numRuns = 20;
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "=== " << instance << " ===\n\n";
        
        // Test Steepest LS
        {
            std::vector<int> objectives;
            long long totalGens = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runAMSEA(n, selectCount, distance, costs, timeLimit, rng, localSearchSteepest, "Steepest");
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            std::cout << "AMSEA + Steepest LS:\n";
            std::cout << "  Best: " << best << ", Worst: " << worst << ", Avg: " << std::fixed << std::setprecision(1) << avg << "\n";
            std::cout << "  Generations: " << (totalGens / numRuns) << "\n\n";
        }
        
        // Test Greedy LS
        {
            std::vector<int> objectives;
            long long totalGens = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runAMSEA(n, selectCount, distance, costs, timeLimit, rng, localSearchGreedy, "Greedy");
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            std::cout << "AMSEA + Greedy LS:\n";
            std::cout << "  Best: " << best << ", Worst: " << worst << ", Avg: " << std::fixed << std::setprecision(1) << avg << "\n";
            std::cout << "  Generations: " << (totalGens / numRuns) << "\n\n";
        }
    }
    
    std::cout << "Done!\n";
    return 0;
}
