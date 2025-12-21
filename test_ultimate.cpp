// Ultimate AMSEA: Combining ALL best strategies from research
// Based on patterns found in strategy_notes.md

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
// Core Functions
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

// Edge key for LTM
static inline long long edgeKey(int a, int b) { 
    return (long long)std::min(a,b) * 1000000 + std::max(a,b); 
}

// LTM-enhanced repair: uses edge frequencies to prefer good edges
static std::vector<int> repairWithLTM(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    const std::unordered_map<long long, int>& edgeFreq,
                    double freqWeight = 0.1) {
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
            
            // LTM bonus: prefer edges that appear in good solutions
            int freqBonus = 0;
            if (bp > 0 && bp <= sol.size()) {
                auto it1 = edgeFreq.find(edgeKey(sol[bp-1], i));
                auto it2 = edgeFreq.find(edgeKey(i, sol[bp % sol.size()]));
                if (it1 != edgeFreq.end()) freqBonus += it1->second;
                if (it2 != edgeFreq.end()) freqBonus += it2->second;
            }
            
            double score = 1.0 * regret - 1.0 * b1 + freqWeight * freqBonus;
            if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

// Strong perturbation (from testing: helps both instances)
static std::vector<int> strongPerturb(const std::vector<int> &sol, int n, std::mt19937 &rng) {
    std::vector<int> p = sol;
    int sz = p.size();
    
    // 5-8 2-opt moves
    int k = 5 + std::uniform_int_distribution<>(0, 3)(rng);
    for (int i = 0; i < k; i++) {
        std::uniform_int_distribution<> d(0, sz - 1);
        int a = d(rng), b = d(rng);
        while (a == b || (a+1)%sz == b || (b+1)%sz == a) b = d(rng);
        if (a > b) std::swap(a, b);
        std::reverse(p.begin() + a + 1, p.begin() + b + 1);
    }
    
    // 50% chance of swapping 2 nodes
    if (std::uniform_real_distribution<>(0, 1)(rng) < 0.5) {
        std::vector<bool> inS(n, false);
        for (int x : p) inS[x] = true;
        std::vector<int> notS;
        for (int i = 0; i < n; i++) if (!inS[i]) notS.push_back(i);
        if (!notS.empty()) {
            for (int s = 0; s < 2 && !notS.empty(); s++) {
                int pos = std::uniform_int_distribution<>(0, sz-1)(rng);
                int idx = std::uniform_int_distribution<>(0, (int)notS.size()-1)(rng);
                int newNode = notS[idx];
                notS[idx] = p[pos];
                p[pos] = newNode;
            }
        }
    }
    return p;
}

// Destroy & Repair with 50% destroy (from testing: excellent on TSPB)
static std::vector<int> destroyRepair50(const std::vector<int> &sol, int n, int selectCount,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs, std::mt19937 &rng,
                                const std::unordered_map<long long, int>& edgeFreq) {
    int destroyCount = (int)(selectCount * 0.5);
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
    return repairWithLTM(partial, distance, costs, n, selectCount, edgeFreq, 0.1);
}

// ============================================================================
// ULTIMATE AMSEA: Combining all best strategies
// ============================================================================

struct UltimateResult {
    int bestObjective;
    std::vector<int> bestSolution;
    long long generations;
    int opSuccess[5];
    int opAttempts[5];
};

UltimateResult runUltimateAMSEA(
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng) {

    UltimateResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;
    for (int i = 0; i < 5; i++) { result.opSuccess[i] = 0; result.opAttempts[i] = 1; }

    auto startTime = std::chrono::high_resolution_clock::now();
    
    // BEST PARAMETERS FROM TESTING:
    const int POP_SIZE = 20;      // From testing: works best for TSPA
    const int STAGNATION = 30;    // From testing: good balance
    const int TOURNAMENT_K = 3;   // From testing: best for TSPA
    
    // Long-Term Memory: track edge frequencies
    std::unordered_map<long long, int> edgeFreq;
    
    auto updateFreq = [&](const std::vector<int>& sol) {
        for (int i = 0; i < sol.size(); i++) {
            edgeFreq[edgeKey(sol[i], sol[(i+1) % sol.size()])]++;
        }
    };
    
    // Initialize population with greedy heuristics
    std::vector<std::vector<int>> pop;
    std::vector<int> popObj;
    std::unordered_set<int> objSeen;
    
    std::vector<int> starts(n);
    std::iota(starts.begin(), starts.end(), 0);
    std::shuffle(starts.begin(), starts.end(), rng);
    
    auto addUnique = [&](const std::vector<int> &s) {
        if (pop.size() >= POP_SIZE) return;
        auto improved = localSearchFast(s, distance, costs, n);
        int obj = calculateObjective(improved, distance, costs);
        if (!objSeen.count(obj)) {
            objSeen.insert(obj);
            pop.push_back(improved);
            popObj.push_back(obj);
            updateFreq(improved);  // LTM: track edges
            if (obj < result.bestObjective) {
                result.bestObjective = obj;
                result.bestSolution = improved;
            }
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
    
    // Adaptive operator selection (5 operators)
    // 0: Common nodes + repair
    // 1: Parent-based repair
    // 2: Path relinking
    // 3: LNS (destroy 30%)
    // 4: LNS (destroy 50%) - NEW from testing
    int opSuccess[5] = {0,0,0,0,0};
    int opAttempts[5] = {1,1,1,1,1};
    
    auto selectOp = [&]() {
        double rates[5], total = 0;
        for (int i = 0; i < 5; i++) {
            rates[i] = (opSuccess[i] + 1.0) / (opAttempts[i] + 2.0);
            total += rates[i];
        }
        double r = std::uniform_real_distribution<>(0, total)(rng), cum = 0;
        for (int i = 0; i < 5; i++) {
            cum += rates[i];
            if (cum >= r) return i;
        }
        return 4;
    };
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        // Tournament selection (size TOURNAMENT_K)
        std::uniform_int_distribution<> dist(0, pop.size() - 1);
        auto tournament = [&]() {
            int best = dist(rng);
            for (int i = 1; i < TOURNAMENT_K; i++) {
                int cand = dist(rng);
                if (popObj[cand] < popObj[best]) best = cand;
            }
            return best;
        };
        int p1 = tournament(), p2 = tournament();
        while (p2 == p1) p2 = tournament();
        
        // Select operator adaptively
        int op = selectOp();
        opAttempts[op]++;
        
        std::vector<int> offspring;
        
        if (op == 0) {
            // Common nodes + LTM-enhanced repair
            std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
            std::vector<int> partial;
            for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
            offspring = repairWithLTM(partial, distance, costs, n, selectCount, edgeFreq, 0.1);
        } else if (op == 1) {
            // Parent-based repair
            offspring = pop[std::uniform_int_distribution<>(0, 1)(rng) ? p1 : p2];
            // Perturb slightly
            int sz = offspring.size();
            for (int k = 0; k < 2; k++) {
                std::uniform_int_distribution<> d(0, sz - 1);
                int a = d(rng), b = d(rng);
                if (a > b) std::swap(a, b);
                if (a != b) std::reverse(offspring.begin() + a + 1, offspring.begin() + b + 1);
            }
        } else if (op == 2) {
            // Path relinking
            std::unordered_set<int> n1(pop[p1].begin(), pop[p1].end());
            std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
            std::vector<int> unique1, unique2, common;
            for (int x : pop[p1]) {
                if (n2.count(x)) common.push_back(x);
                else unique1.push_back(x);
            }
            for (int x : pop[p2]) if (!n1.count(x)) unique2.push_back(x);
            
            // Mix unique nodes
            int halfU = unique1.size() / 2;
            std::shuffle(unique1.begin(), unique1.end(), rng);
            std::shuffle(unique2.begin(), unique2.end(), rng);
            
            for (int i = 0; i < halfU && i < unique2.size(); i++) {
                unique1[i] = unique2[i];
            }
            
            std::vector<int> partial = common;
            for (int x : unique1) partial.push_back(x);
            offspring = repairWithLTM(partial, distance, costs, n, selectCount, edgeFreq, 0.1);
        } else if (op == 3) {
            // LNS 30% destroy
            int destroyCount = (int)(selectCount * 0.3);
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
            for (int i = 0; i < selectCount; i++) if (keep[i]) partial.push_back(pop[p1][i]);
            offspring = repairWithLTM(partial, distance, costs, n, selectCount, edgeFreq, 0.1);
        } else {
            // LNS 50% destroy (NEW - showed excellent results in testing)
            offspring = destroyRepair50(pop[p1], n, selectCount, distance, costs, rng, edgeFreq);
        }
        
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        updateFreq(offspring);  // LTM: track edges
        
        // Replace worst if better
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
            result.bestSolution = offspring;
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        // Strong perturbation on stagnation (from testing: helps)
        if (stagnation >= STAGNATION) {
            int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
            auto perturbed = strongPerturb(pop[worst], n, rng);
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
    
    for (int i = 0; i < 5; i++) {
        result.opSuccess[i] = opSuccess[i];
        result.opAttempts[i] = opAttempts[i];
    }
    
    return result;
}

// ============================================================================
// Load Instance and Main
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
    std::cout << "Ultimate AMSEA: Combining ALL Best Strategies\n";
    std::cout << "=============================================\n\n";
    std::cout << "Strategies combined:\n";
    std::cout << "  - Tournament selection (K=3)\n";
    std::cout << "  - Population 20, Stagnation 30\n";
    std::cout << "  - 5 operators: CommonNodes, Parent, PathRelink, LNS30, LNS50\n";
    std::cout << "  - Long-Term Memory (edge frequencies)\n";
    std::cout << "  - Strong perturbation (5-8 2-opt, 50% node swap)\n";
    std::cout << "  - LTM-enhanced repair\n\n";
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;
    const int numRuns = 20;
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "=== " << instance << " (n=" << n << ") ===\n\n";
        
        std::vector<int> objectives;
        long long totalGens = 0;
        int totalOpSuccess[5] = {0,0,0,0,0};
        int totalOpAttempts[5] = {0,0,0,0,0};
        
        for (int run = 0; run < numRuns; run++) {
            std::random_device rd;
            std::mt19937 rng(rd());
            auto result = runUltimateAMSEA(n, selectCount, distance, costs, timeLimit, rng);
            objectives.push_back(result.bestObjective);
            totalGens += result.generations;
            for (int i = 0; i < 5; i++) {
                totalOpSuccess[i] += result.opSuccess[i];
                totalOpAttempts[i] += result.opAttempts[i];
            }
        }
        
        int best = *std::min_element(objectives.begin(), objectives.end());
        int worst = *std::max_element(objectives.begin(), objectives.end());
        double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
        
        std::cout << "Results (20 runs, 1s each):\n";
        std::cout << "  Best:  " << best << "\n";
        std::cout << "  Avg:   " << std::fixed << std::setprecision(1) << avg << "\n";
        std::cout << "  Worst: " << worst << "\n";
        std::cout << "  Gens:  " << (totalGens / numRuns) << "\n\n";
        
        std::cout << "Operator Stats:\n";
        const char* opNames[] = {"CommonNodes", "Parent", "PathRelink", "LNS30", "LNS50"};
        for (int i = 0; i < 5; i++) {
            std::cout << "  " << std::setw(12) << opNames[i] << ": " 
                      << totalOpSuccess[i] << "/" << totalOpAttempts[i]
                      << " (" << std::fixed << std::setprecision(1) 
                      << (100.0 * totalOpSuccess[i] / totalOpAttempts[i]) << "%)\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "Done!\n";
    return 0;
}
