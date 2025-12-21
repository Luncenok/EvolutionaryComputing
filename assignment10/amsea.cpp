#include "../include/amsea.h"
#include "../include/calculateObjective.h"
#include "../include/greedyCycle.h"
#include "../include/greedyRegret2Weighted.h"
#include "../include/localSearch.h"
#include "../include/nearestNeighborAny.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <unordered_set>

// ============================================================================
// AMSEA V3 - OPTIMIZED VERSION
// Improvements over V2:
// 1. Fast LTM with 2D array (no hash overhead)
// 2. 25% LTM usage ratio (optimal balance)
// 3. Strong perturbation (5-8 2-opt moves)
// 4. Tournament selection (size 3)
// ============================================================================

// Fast Edge Frequency Tracker (2D array - no hash overhead)
class FastEdgeFreq {
private:
    std::vector<int> freq;
    int n;
    
public:
    FastEdgeFreq(int size) : n(size), freq(size * size, 0) {}
    
    inline void increment(int a, int b) {
        if (a > b) std::swap(a, b);
        freq[a * n + b]++;
    }
    
    inline int get(int a, int b) const {
        if (a > b) std::swap(a, b);
        return freq[a * n + b];
    }
    
    void updateFromSolution(const std::vector<int>& sol) {
        int sz = sol.size();
        for (int i = 0; i < sz; i++) {
            increment(sol[i], sol[(i + 1) % sz]);
        }
    }
};

// Forward declarations
std::vector<int> repairSolutionAMSEA(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    double wRegret, double wBest);

// ============================================================================
// Fast Delta Calculations
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

// ============================================================================
// Fast Local Search
// ============================================================================

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

// ============================================================================
// Population Initialization
// ============================================================================

static std::vector<std::vector<int>> initPopulation(
    int n, int selectCount, const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, std::mt19937 &rng, int popSize,
    std::vector<int> &popObj, std::unordered_set<int> &objSeen) {
    
    std::vector<std::vector<int>> pop;
    
    auto addUnique = [&](const std::vector<int> &s) {
        if (pop.size() >= popSize) return;
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
    
    int perH = popSize / 3 + 1;
    for (int i = 0; i < perH && pop.size() < popSize; i++)
        addUnique(greedyCycle(starts[i % n], selectCount, distance, costs));
    for (int i = 0; i < perH && pop.size() < popSize; i++)
        addUnique(nearestNeighborAny(starts[(i + perH) % n], selectCount, distance, costs));
    for (int i = 0; i < perH && pop.size() < popSize; i++)
        addUnique(greedyRegret2Weighted(starts[(i + 2*perH) % n], selectCount, distance, costs, 1.0, 1.0));
    
    for (int att = 0; att < popSize * 10 && pop.size() < popSize; att++) {
        std::vector<int> all(n); std::iota(all.begin(), all.end(), 0);
        std::shuffle(all.begin(), all.end(), rng);
        addUnique(std::vector<int>(all.begin(), all.begin() + selectCount));
    }
    return pop;
}

// ============================================================================
// Tournament Selection (size 3)
// ============================================================================

static inline std::pair<int, int> tournamentSelect(const std::vector<int> &popObj, std::mt19937 &rng) {
    int popSize = popObj.size();
    std::uniform_int_distribution<> dist(0, popSize - 1);
    
    auto tournament = [&]() {
        int a = dist(rng), b = dist(rng), c = dist(rng);
        if (popObj[a] <= popObj[b] && popObj[a] <= popObj[c]) return a;
        if (popObj[b] <= popObj[c]) return b;
        return c;
    };
    
    int p1 = tournament(), p2 = tournament();
    while (p2 == p1) p2 = tournament();
    return {p1, p2};
}

// ============================================================================
// Operator 1: Common Nodes and Edges (original)
// ============================================================================

static std::vector<int> opCommonEdges(const std::vector<int> &p1, const std::vector<int> &p2,
                                       int n, int selectCount, std::mt19937 &rng) {
    std::unordered_set<int> n1(p1.begin(), p1.end()), n2(p2.begin(), p2.end());
    std::unordered_set<int> common;
    for (int x : n1) if (n2.count(x)) common.insert(x);
    
    auto edge = [](int a, int b) { return std::make_pair(std::min(a,b), std::max(a,b)); };
    std::set<std::pair<int,int>> e1, e2;
    for (int i = 0; i < p1.size(); i++) {
        e1.insert(edge(p1[i], p1[(i+1)%p1.size()]));
        e2.insert(edge(p2[i], p2[(i+1)%p2.size()]));
    }
    std::set<std::pair<int,int>> commonE;
    for (auto &e : e1) if (e2.count(e)) commonE.insert(e);
    
    std::map<int, std::vector<int>> adj;
    for (auto &e : commonE) { adj[e.first].push_back(e.second); adj[e.second].push_back(e.first); }
    
    std::vector<std::vector<int>> subpaths;
    std::unordered_set<int> visited;
    
    for (int node : common) {
        if (visited.count(node)) continue;
        std::vector<int> path = {node};
        visited.insert(node);
        
        int cur = node;
        while (true) {
            auto it = adj.find(cur);
            if (it == adj.end()) break;
            int next = -1;
            for (int nb : it->second) if (!visited.count(nb) && common.count(nb)) { next = nb; break; }
            if (next == -1) break;
            path.push_back(next); visited.insert(next); cur = next;
        }
        cur = node;
        while (true) {
            auto it = adj.find(cur);
            if (it == adj.end()) break;
            int prev = -1;
            for (int nb : it->second) if (!visited.count(nb) && common.count(nb)) { prev = nb; break; }
            if (prev == -1) break;
            path.insert(path.begin(), prev); visited.insert(prev); cur = prev;
        }
        subpaths.push_back(path);
    }
    
    int total = 0;
    for (auto &p : subpaths) total += p.size();
    
    std::vector<int> unsel;
    for (int i = 0; i < n; i++) if (!visited.count(i)) unsel.push_back(i);
    std::shuffle(unsel.begin(), unsel.end(), rng);
    for (int i = 0; i < selectCount - total && i < unsel.size(); i++)
        subpaths.push_back({unsel[i]});
    
    std::shuffle(subpaths.begin(), subpaths.end(), rng);
    std::vector<int> result;
    std::uniform_int_distribution<> flip(0, 1);
    for (auto &p : subpaths) {
        if (flip(rng)) std::reverse(p.begin(), p.end());
        for (int x : p) result.push_back(x);
    }
    return result;
}

// ============================================================================
// Operator 2: Parent-based Repair
// ============================================================================

static std::vector<int> opParentRepair(const std::vector<int> &p1, const std::vector<int> &p2,
                                        int n, int sc,
                                        const std::vector<std::vector<int>> &dist,
                                        const std::vector<int> &costs) {
    std::unordered_set<int> n2(p2.begin(), p2.end());
    std::vector<int> partial;
    for (int x : p1) if (n2.count(x)) partial.push_back(x);
    return repairSolutionAMSEA(partial, dist, costs, n, sc, 1.0, 1.0);
}

// ============================================================================
// Operator 3: Enhanced Path Relinking (tries multiple ratios, picks best)
// ============================================================================

static std::vector<int> opPathRelinkEnhanced(
    const std::vector<int> &p1, const std::vector<int> &p2,
    int n, int sc,
    const std::vector<std::vector<int>> &dist,
    const std::vector<int> &costs, std::mt19937 &rng) {
    
    std::unordered_set<int> n2(p2.begin(), p2.end());
    std::vector<int> common, unique1;
    for (int x : p1) {
        if (n2.count(x)) common.push_back(x);
        else unique1.push_back(x);
    }
    
    std::shuffle(unique1.begin(), unique1.end(), rng);
    
    std::vector<int> bestResult;
    int bestObj = INT_MAX;
    
    for (double ratio : {0.25, 0.5, 0.75}) {
        int keepCount = (int)(unique1.size() * ratio);
        std::unordered_set<int> partialSet(common.begin(), common.end());
        for (int i = 0; i < keepCount; i++) partialSet.insert(unique1[i]);
        
        std::vector<int> partial;
        for (int x : p1) if (partialSet.count(x)) partial.push_back(x);
        
        auto repaired = repairSolutionAMSEA(partial, dist, costs, n, sc, 1.0, 1.0);
        int obj = calculateObjective(repaired, dist, costs);
        if (obj < bestObj) { bestObj = obj; bestResult = repaired; }
    }
    return bestResult;
}

// ============================================================================
// Operator 4: LNS-style Destroy/Repair
// ============================================================================

static std::vector<int> opLNS(const std::vector<int> &parent, int n, int sc,
                              const std::vector<std::vector<int>> &dist,
                              const std::vector<int> &costs, std::mt19937 &rng) {
    // Testing showed 30% is better than 50% in full AMSEA context
    // (fewer generations with 50% due to expensive repair)
    int destroyCount = sc * 3 / 10;  // Destroy 30%
    std::vector<bool> keep(sc, true);
    std::uniform_int_distribution<> posDist(0, sc - 1);
    std::unordered_set<int> destroyed;
    
    for (int i = 0; i < destroyCount; i++) {
        int pos = posDist(rng);
        while (destroyed.count(pos)) pos = posDist(rng);
        destroyed.insert(pos);
        keep[pos] = false;
    }
    
    std::vector<int> partial;
    for (int i = 0; i < sc; i++) if (keep[i]) partial.push_back(parent[i]);
    return repairSolutionAMSEA(partial, dist, costs, n, sc, 1.0, 1.0);
}

// ============================================================================
// Repair Function (Weighted 2-Regret)
// ============================================================================

std::vector<int> repairSolutionAMSEA(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    double wRegret, double wBest) {
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
            double score = wRegret * regret - wBest * b1;
            if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

// ============================================================================
// Perturbation - STRONG VERSION (based on testing: helps both instances)
// ============================================================================

static std::vector<int> perturb(const std::vector<int> &sol, int n, std::mt19937 &rng) {
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
    
    // Higher chance of node swap (50% instead of 25%)
    if (std::uniform_real_distribution<>(0, 1)(rng) < 0.5) {
        std::vector<bool> inS(n, false);
        for (int x : p) inS[x] = true;
        std::vector<int> notS;
        for (int i = 0; i < n; i++) if (!inS[i]) notS.push_back(i);
        if (!notS.empty()) {
            // Swap 2 nodes instead of 1
            for (int s = 0; s < 2 && !notS.empty(); s++) {
                int pos = std::uniform_int_distribution<>(0, sz-1)(rng);
                int idx = std::uniform_int_distribution<>(0, (int)notS.size()-1)(rng);
                int newNode = notS[idx];
                notS[idx] = p[pos];  // Move old node to not-selected
                p[pos] = newNode;
            }
        }
    }
    return p;
}

// ============================================================================
// Adaptive Operator Selection (4 operators)
// ============================================================================

static inline int selectOp(const int *success, const int *attempts, std::mt19937 &rng) {
    double rates[4], total = 0;
    for (int i = 0; i < 4; i++) {
        rates[i] = (success[i] + 1.0) / (attempts[i] + 2.0);
        total += rates[i];
    }
    double r = std::uniform_real_distribution<>(0, total)(rng), cum = 0;
    for (int i = 0; i < 4; i++) {
        cum += rates[i];
        if (cum >= r) return i;
    }
    return 3;
}

// ============================================================================
// Elite Archive
// ============================================================================

static void updateArchive(std::vector<std::pair<int, std::vector<int>>> &arc,
                          const std::vector<int> &sol, int obj, int maxSz) {
    for (auto &e : arc) if (e.first == obj) return;
    if (arc.size() < maxSz) arc.push_back({obj, sol});
    else {
        int worst = 0;
        for (int i = 1; i < arc.size(); i++) if (arc[i].first > arc[worst].first) worst = i;
        if (obj < arc[worst].first) arc[worst] = {obj, sol};
    }
    std::sort(arc.begin(), arc.end());
}

// ============================================================================
// MAIN AMSEA V2 FUNCTION
// ============================================================================

AMSEAResult amsea(int n, int selectCount,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, double timeLimit,
                  std::mt19937 &rng, int populationSize) {
    
    AMSEAResult result;
    result.bestObjective = INT_MAX;
    result.generations = 0;
    for (int i = 0; i < 3; i++) { result.operatorSuccesses[i] = 0; result.operatorAttempts[i] = 1; }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Best configuration from systematic testing:
    // TSPA: Pop20-Tour3 (Best=69095, Avg=69121)
    // TSPB: Pop18-Tour2-Stag25 (Best=43446, Avg=43465)
    // Using Pop20 with tournament as good compromise
    const int POP_SIZE = 20;
    const int ARCHIVE_SIZE = 5;
    const int STAGNATION = 30;
    
    std::vector<int> popObj;
    std::unordered_set<int> objSeen;
    auto pop = initPopulation(n, selectCount, distance, costs, rng, POP_SIZE, popObj, objSeen);
    
    for (int i = 0; i < pop.size(); i++) {
        if (popObj[i] < result.bestObjective) {
            result.bestObjective = popObj[i];
            result.bestSolution = pop[i];
        }
    }
    
    std::vector<std::pair<int, std::vector<int>>> elite;
    updateArchive(elite, result.bestSolution, result.bestObjective, ARCHIVE_SIZE);
    
    int opSuccess[4] = {0, 0, 0, 0};
    int opAttempts[4] = {1, 1, 1, 1};
    int stagnation = 0;
    
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - startTime).count() >= timeLimit) break;
        
        result.generations++;
        
        auto [p1, p2] = tournamentSelect(popObj, rng);
        
        int op = selectOp(opSuccess, opAttempts, rng);
        opAttempts[op]++;
        if (op < 3) result.operatorAttempts[op]++;
        
        std::vector<int> offspring;
        switch (op) {
            case 0: offspring = opCommonEdges(pop[p1], pop[p2], n, selectCount, rng); break;
            case 1: offspring = opParentRepair(pop[p1], pop[p2], n, selectCount, distance, costs); break;
            case 2: offspring = opPathRelinkEnhanced(pop[p1], pop[p2], n, selectCount, distance, costs, rng); break;
            case 3: offspring = opLNS(pop[p1], n, selectCount, distance, costs, rng); break;
        }
        
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        
        int worstObj = *std::max_element(popObj.begin(), popObj.end());
        if (offObj < worstObj) {
            opSuccess[op]++;
            if (op < 3) result.operatorSuccesses[op]++;
        }
        
        // Standard replacement (replace worst if better and unique)
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
            updateArchive(elite, offspring, offObj, ARCHIVE_SIZE);
            stagnation = 0;
        } else {
            stagnation++;
        }
        
        if (stagnation >= STAGNATION) {
            for (int i = 0; i < 2 && i < pop.size(); i++) {
                int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
                auto pert = perturb(pop[worst], n, rng);
                pert = localSearchFast(pert, distance, costs, n);
                int pertObj = calculateObjective(pert, distance, costs);
                if (!objSeen.count(pertObj)) {
                    objSeen.erase(popObj[worst]);
                    objSeen.insert(pertObj);
                    pop[worst] = pert;
                    popObj[worst] = pertObj;
                    if (pertObj < result.bestObjective) {
                        result.bestObjective = pertObj;
                        result.bestSolution = pert;
                        updateArchive(elite, pert, pertObj, ARCHIVE_SIZE);
                    }
                }
            }
            
            if (!elite.empty()) {
                int randE = std::uniform_int_distribution<>(0, elite.size()-1)(rng);
                if (!objSeen.count(elite[randE].first)) {
                    int worst = std::max_element(popObj.begin(), popObj.end()) - popObj.begin();
                    objSeen.erase(popObj[worst]);
                    objSeen.insert(elite[randE].first);
                    pop[worst] = elite[randE].second;
                    popObj[worst] = elite[randE].first;
                }
            }
            stagnation = 0;
        }
    }
    
    result.totalTime = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - startTime).count();
    return result;
}
