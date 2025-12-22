// Global Memory of Deltas Test
// From lecture: "Moves and their deltas can be repeated during various LS runs.
// So one can remember all the already calculated deltas in a global memory"
//
// For TSP with 2-edge exchange: O(n^4) possible moves, (n-1)! solutions
// We cache deltas keyed by the 4 edges involved

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
// Global Delta Memory
// ============================================================================
// For 2-opt move removing edges (a,b) and (c,d), adding (a,c) and (b,d):
// Delta = dist[a][c] + dist[b][d] - dist[a][b] - dist[c][d]
// This depends ONLY on the 4 nodes, not on their position in solution!
// So we can cache: key = (a,b,c,d) sorted, value = delta
// ============================================================================

class GlobalDeltaMemory {
private:
    // Key: 4 nodes packed into uint64_t, Value: delta
    std::unordered_map<uint64_t, int> cache;
    long long hits = 0;
    long long misses = 0;
    
    inline uint64_t makeKey(int a, int b, int c, int d) const {
        // Pack 4 16-bit integers into 64-bit key
        // Sort the pairs to normalize
        if (a > b) std::swap(a, b);
        if (c > d) std::swap(c, d);
        // Ensure first pair is smaller
        if (a > c || (a == c && b > d)) {
            std::swap(a, c);
            std::swap(b, d);
        }
        return ((uint64_t)a << 48) | ((uint64_t)b << 32) | ((uint64_t)c << 16) | (uint64_t)d;
    }
    
public:
    int getOrCompute(int a, int b, int c, int d, 
                     const std::vector<std::vector<int>>& distance) {
        uint64_t key = makeKey(a, b, c, d);
        auto it = cache.find(key);
        if (it != cache.end()) {
            hits++;
            return it->second;
        }
        misses++;
        // Delta for 2-opt: remove (a,b) and (c,d), add (a,c) and (b,d)
        int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
        cache[key] = delta;
        return delta;
    }
    
    void clear() { cache.clear(); hits = 0; misses = 0; }
    double hitRate() const { return hits + misses > 0 ? (double)hits / (hits + misses) : 0; }
    size_t size() const { return cache.size(); }
    long long getHits() const { return hits; }
    long long getMisses() const { return misses; }
};

// Global instance
GlobalDeltaMemory g_deltaCache;

// Core functions
static inline int deltaExchange(const std::vector<int> &sol, int pos, int newNode,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs) {
    int n = sol.size();
    int prev = (pos - 1 + n) % n, next = (pos + 1) % n;
    return (distance[sol[prev]][newNode] + distance[newNode][sol[next]] + costs[newNode])
         - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
}

// LS with global delta memory
static std::vector<int> localSearchWithGlobalMemory(
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
        
        // 2-opt with global delta cache
        for (int i = 0; i < sol.size(); i++) {
            for (int j = i + 2; j < sol.size(); j++) {
                if (i == 0 && j == sol.size() - 1) continue;
                
                // Get the 4 nodes involved
                int a = sol[i], b = sol[(i + 1) % sol.size()];
                int c = sol[j], d = sol[(j + 1) % sol.size()];
                
                // Use cached delta
                int delta = g_deltaCache.getOrCompute(a, b, c, d, distance);
                
                if (delta < bestDelta) {
                    bestDelta = delta; bestType = 0; bestPos1 = i; bestPos2 = j;
                }
            }
        }
        
        // Node exchange (can't cache - depends on position)
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

// Standard LS for comparison
static std::vector<int> localSearchStandard(
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
                int a = sol[i], b = sol[(i + 1) % sol.size()];
                int c = sol[j], d = sol[(j + 1) % sol.size()];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
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
    std::cout << "Global Memory of Deltas Test\n";
    std::cout << "============================\n\n";
    
    std::vector<std::vector<int>> distance;
    std::vector<int> costs;
    int n, selectCount;
    loadInstance("input/TSPA.csv", distance, costs, n, selectCount);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    const int numRuns = 100;
    
    std::cout << "n = " << n << ", selectCount = " << selectCount << "\n";
    std::cout << "Running " << numRuns << " LS calls...\n\n";
    
    // Generate random starting solutions
    std::vector<std::vector<int>> startSolutions;
    for (int i = 0; i < numRuns; i++) {
        auto sol = greedyCycle(i % n, selectCount, distance, costs);
        startSolutions.push_back(sol);
    }
    
    // Test standard LS
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalObj = 0;
        for (int i = 0; i < numRuns; i++) {
            auto result = localSearchStandard(startSolutions[i], distance, costs, n);
            totalObj += calculateObjective(result, distance, costs);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Standard LS:\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n\n";
    }
    
    // Test LS with global delta memory
    g_deltaCache.clear();
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalObj = 0;
        for (int i = 0; i < numRuns; i++) {
            auto result = localSearchWithGlobalMemory(startSolutions[i], distance, costs, n);
            totalObj += calculateObjective(result, distance, costs);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "LS with Global Delta Memory:\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n";
        std::cout << "  Cache size: " << g_deltaCache.size() << " entries\n";
        std::cout << "  Hit rate: " << std::setprecision(1) << (g_deltaCache.hitRate() * 100) << "%\n";
        std::cout << "  Hits: " << g_deltaCache.getHits() << ", Misses: " << g_deltaCache.getMisses() << "\n\n";
    }
    
    // Memory analysis
    std::cout << "Memory Analysis:\n";
    std::cout << "  Max possible 2-opt moves: " << (n * n * n * n / 4) << "\n";
    std::cout << "  Unique moves encountered: " << g_deltaCache.size() << "\n";
    std::cout << "  Coverage: " << std::setprecision(4) << (100.0 * g_deltaCache.size() / (n * n * n * n / 4)) << "%\n";
    
    std::cout << "\nDone!\n";
    return 0;
}
