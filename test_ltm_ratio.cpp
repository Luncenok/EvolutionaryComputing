// Test: LTM only for intensification (once every N iterations)
// Hypothesis: LTM helps quality but hurts speed. Use selectively.

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

// Fast Edge Frequency Tracker
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

// Core functions
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

// Two repair functions: standard and LTM-enhanced
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
            double score = regret - b1;
            if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

static std::vector<int> repairWithLTM(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    const FastEdgeFreq& ltm, double freqWeight) {
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
            int freqBonus = 0;
            if (bp > 0 && bp <= sol.size()) {
                freqBonus = ltm.get(sol[bp-1], i) + ltm.get(i, sol[bp % sol.size()]);
            }
            double score = regret - b1 + freqWeight * freqBonus;
            if (score > bestScore) { bestScore = score; chooseN = i; chooseP = bp; }
        }
        if (chooseN == -1) break;
        sol.insert(sol.begin() + chooseP, chooseN);
        sel[chooseN] = true;
    }
    return sol;
}

struct Result {
    int bestObjective;
    long long generations;
};

// Test with LTM usage ratio (0.0 = never use LTM, 1.0 = always use LTM)
Result runWithLTMRatio(
    int n, int selectCount,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs,
    double timeLimit,
    std::mt19937 &rng,
    double ltmRatio,  // 0.0-1.0
    double ltmWeight = 0.05) {

    Result result;
    result.bestObjective = INT_MAX;
    result.generations = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    const int POP_SIZE = 20;
    const int STAGNATION = 30;
    
    FastEdgeFreq ltm(n);
    
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
            ltm.updateFromSolution(improved);
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
    std::uniform_real_distribution<> ratioCheck(0, 1);
    
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
        
        // Common nodes recombination
        std::unordered_set<int> n2(pop[p2].begin(), pop[p2].end());
        std::vector<int> partial;
        for (int x : pop[p1]) if (n2.count(x)) partial.push_back(x);
        
        // Use LTM with probability = ltmRatio
        std::vector<int> offspring;
        if (ratioCheck(rng) < ltmRatio) {
            offspring = repairWithLTM(partial, distance, costs, n, selectCount, ltm, ltmWeight);
        } else {
            offspring = repairStandard(partial, distance, costs, n, selectCount);
        }
        
        offspring = localSearchFast(offspring, distance, costs, n);
        int offObj = calculateObjective(offspring, distance, costs);
        ltm.updateFromSolution(offspring);
        
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
            int k = 5 + std::uniform_int_distribution<>(0, 3)(rng);
            for (int i = 0; i < k; i++) {
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
                ltm.updateFromSolution(perturbed);
                if (pObj < result.bestObjective) result.bestObjective = pObj;
            }
            stagnation = 0;
        }
    }
    
    return result;
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
    std::cout << "LTM Usage Ratio Test\n";
    std::cout << "====================\n\n";
    
    const char* instances[] = {"input/TSPA.csv", "input/TSPB.csv"};
    const double timeLimit = 1000.0;
    const int numRuns = 10;
    
    // Test different LTM ratios: 0%, 10%, 25%, 50%, 100%
    double ratios[] = {0.0, 0.1, 0.25, 0.5, 1.0};
    
    for (const auto &instance : instances) {
        std::vector<std::vector<int>> distance;
        std::vector<int> costs;
        int n, selectCount;
        loadInstance(instance, distance, costs, n, selectCount);
        
        std::cout << "=== " << instance << " ===\n\n";
        std::cout << std::setw(12) << "LTM Ratio"
                  << std::setw(8) << "Best" 
                  << std::setw(10) << "Avg" 
                  << std::setw(8) << "Worst"
                  << std::setw(10) << "Gens" << "\n";
        std::cout << std::string(48, '-') << "\n";
        
        for (double ratio : ratios) {
            std::vector<int> objectives;
            long long totalGens = 0;
            for (int run = 0; run < numRuns; run++) {
                std::random_device rd;
                std::mt19937 rng(rd());
                auto result = runWithLTMRatio(n, selectCount, distance, costs, timeLimit, rng, ratio);
                objectives.push_back(result.bestObjective);
                totalGens += result.generations;
            }
            int best = *std::min_element(objectives.begin(), objectives.end());
            int worst = *std::max_element(objectives.begin(), objectives.end());
            double avg = std::accumulate(objectives.begin(), objectives.end(), 0.0) / numRuns;
            
            std::cout << std::setw(12) << (int)(ratio * 100) << "%" 
                      << std::setw(8) << best
                      << std::setw(10) << std::fixed << std::setprecision(0) << avg
                      << std::setw(8) << worst
                      << std::setw(10) << (totalGens / numRuns) << "\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "Done!\n";
    return 0;
}
