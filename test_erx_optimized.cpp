// ERX Analysis: Understanding the true overhead source
// The 2D array approach was slower - let's understand why

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
#include <climits>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"

// ============================================================================
// Analysis: Why was 2D array slower?
// ============================================================================
// - n*n = 40000 bools = 40KB for adjacency alone
// - Cache line is 64 bytes, so random access causes cache misses
// - removeNode() touches n elements = n cache misses
// - Original uses small vectors (~4 neighbors each) = cache friendly
//
// Conclusion: For sparse graphs (TSP has ~4 edges per node), 
// vectors are actually optimal!
//
// Real optimization: Minimize allocations, use fixed-size arrays
// ============================================================================

// Version 1: Original (baseline)
static std::vector<int> erxOriginal(
    const std::vector<int> &p1, const std::vector<int> &p2,
    int n, int selectCount, std::mt19937 &rng) {
    
    std::vector<std::vector<int>> adj(n);
    auto addEdge = [&](int a, int b) {
        if (std::find(adj[a].begin(), adj[a].end(), b) == adj[a].end()) adj[a].push_back(b);
        if (std::find(adj[b].begin(), adj[b].end(), a) == adj[b].end()) adj[b].push_back(a);
    };
    
    for (int i = 0; i < p1.size(); i++) addEdge(p1[i], p1[(i + 1) % p1.size()]);
    for (int i = 0; i < p2.size(); i++) addEdge(p2[i], p2[(i + 1) % p2.size()]);
    
    std::vector<bool> used(n, false);
    std::vector<int> result;
    result.reserve(selectCount);
    
    // Collect available nodes
    std::vector<int> availList;
    std::vector<bool> inParent(n, false);
    for (int x : p1) if (!inParent[x]) { inParent[x] = true; availList.push_back(x); }
    for (int x : p2) if (!inParent[x]) { inParent[x] = true; availList.push_back(x); }
    
    int startIdx = std::uniform_int_distribution<>(0, availList.size()-1)(rng);
    int current = availList[startIdx];
    result.push_back(current);
    used[current] = true;
    
    while (result.size() < selectCount) {
        // Remove current from neighbors' adjacency lists
        for (int neighbor : adj[current]) {
            auto& nlist = adj[neighbor];
            nlist.erase(std::remove(nlist.begin(), nlist.end(), current), nlist.end());
        }
        
        // Find neighbor with minimum degree
        int next = -1;
        int minDegree = INT_MAX;
        for (int neighbor : adj[current]) {
            if (!used[neighbor] && adj[neighbor].size() < minDegree) {
                minDegree = adj[neighbor].size();
                next = neighbor;
            }
        }
        
        // If no neighbor, pick random unused
        if (next == -1) {
            for (int node : availList) {
                if (!used[node]) { next = node; break; }
            }
        }
        
        if (next == -1) break;
        result.push_back(next);
        used[next] = true;
        current = next;
    }
    
    return result;
}

// Version 2: Optimized with pre-allocated fixed-size adjacency
// Each node has at most 4 neighbors (2 from each parent)
static std::vector<int> erxOptimized(
    const std::vector<int> &p1, const std::vector<int> &p2,
    int n, int selectCount, std::mt19937 &rng) {
    
    // Fixed-size adjacency: each node can have at most 4 neighbors
    // Layout: adj[node*5] = count, adj[node*5 + 1..4] = neighbors
    std::vector<int> adj(n * 5, 0);  // Pre-allocate everything
    
    auto addEdge = [&](int a, int b) {
        // Check if edge already exists
        int countA = adj[a * 5];
        for (int i = 0; i < countA; i++) {
            if (adj[a * 5 + 1 + i] == b) return;  // Already exists
        }
        // Add edge
        if (countA < 4) {
            adj[a * 5 + 1 + countA] = b;
            adj[a * 5]++;
        }
        int countB = adj[b * 5];
        if (countB < 4) {
            adj[b * 5 + 1 + countB] = a;
            adj[b * 5]++;
        }
    };
    
    auto removeFromAdj = [&](int node, int toRemove) {
        int count = adj[node * 5];
        for (int i = 0; i < count; i++) {
            if (adj[node * 5 + 1 + i] == toRemove) {
                // Swap with last and decrease count
                adj[node * 5 + 1 + i] = adj[node * 5 + count];
                adj[node * 5]--;
                return;
            }
        }
    };
    
    // Build adjacency
    for (int i = 0; i < p1.size(); i++) addEdge(p1[i], p1[(i + 1) % p1.size()]);
    for (int i = 0; i < p2.size(); i++) addEdge(p2[i], p2[(i + 1) % p2.size()]);
    
    std::vector<bool> used(n, false);
    std::vector<int> result;
    result.reserve(selectCount);
    
    // Collect available nodes
    std::vector<int> availList;
    std::vector<bool> inParent(n, false);
    for (int x : p1) if (!inParent[x]) { inParent[x] = true; availList.push_back(x); }
    for (int x : p2) if (!inParent[x]) { inParent[x] = true; availList.push_back(x); }
    
    int startIdx = std::uniform_int_distribution<>(0, availList.size()-1)(rng);
    int current = availList[startIdx];
    result.push_back(current);
    used[current] = true;
    
    while (result.size() < selectCount) {
        // Remove current from all neighbors
        int count = adj[current * 5];
        for (int i = 0; i < count; i++) {
            int neighbor = adj[current * 5 + 1 + i];
            removeFromAdj(neighbor, current);
        }
        
        // Find neighbor with minimum degree
        int next = -1;
        int minDegree = INT_MAX;
        count = adj[current * 5];
        for (int i = 0; i < count; i++) {
            int neighbor = adj[current * 5 + 1 + i];
            if (!used[neighbor]) {
                int degree = adj[neighbor * 5];
                if (degree < minDegree) {
                    minDegree = degree;
                    next = neighbor;
                }
            }
        }
        
        // If no neighbor, pick random unused
        if (next == -1) {
            for (int node : availList) {
                if (!used[node]) { next = node; break; }
            }
        }
        
        if (next == -1) break;
        result.push_back(next);
        used[next] = true;
        current = next;
    }
    
    return result;
}

// Version 3: Skip ERX entirely - just use common nodes
// This is what current AMSEA operators do
static std::vector<int> skipERX(
    const std::vector<int> &p1, const std::vector<int> &p2,
    int n, int selectCount, std::mt19937 &rng) {
    
    std::vector<bool> inP2(n, false);
    for (int x : p2) inP2[x] = true;
    
    std::vector<int> result;
    for (int x : p1) {
        if (inP2[x]) result.push_back(x);
    }
    
    return result;
}

// Repair function
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

static std::vector<int> localSearchFast(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n);  // Forward declaration

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
    std::cout << "ERX Overhead Analysis\n";
    std::cout << "=====================\n\n";
    
    std::vector<std::vector<int>> distance;
    std::vector<int> costs;
    int n, selectCount;
    loadInstance("input/TSPA.csv", distance, costs, n, selectCount);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    // Create parent solutions
    auto p1 = greedyCycle(0, selectCount, distance, costs);
    auto p2 = greedyCycle(50, selectCount, distance, costs);
    
    const int numIterations = 10000;
    
    std::cout << "n = " << n << ", selectCount = " << selectCount << "\n";
    std::cout << "Testing " << numIterations << " iterations...\n\n";
    
    // Benchmark Original ERX
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalSize = 0;
        for (int i = 0; i < numIterations; i++) {
            auto result = erxOriginal(p1, p2, n, selectCount, rng);
            totalSize += result.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Original ERX (vector adj): " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg output size: " << (totalSize / numIterations) << "/" << selectCount << "\n";
        std::cout << "  Per-call: " << std::setprecision(3) << (ms / numIterations) << " ms\n\n";
    }
    
    // Benchmark Fixed-size ERX
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalSize = 0;
        for (int i = 0; i < numIterations; i++) {
            auto result = erxOptimized(p1, p2, n, selectCount, rng);
            totalSize += result.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Optimized ERX (fixed adj): " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg output size: " << (totalSize / numIterations) << "/" << selectCount << "\n";
        std::cout << "  Per-call: " << std::setprecision(3) << (ms / numIterations) << " ms\n\n";
    }
    
    // Benchmark Common Nodes (current approach)
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalSize = 0;
        for (int i = 0; i < numIterations; i++) {
            auto result = skipERX(p1, p2, n, selectCount, rng);
            totalSize += result.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Common Nodes (no ERX): " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg output size: " << (totalSize / numIterations) << "/" << selectCount << "\n";
        std::cout << "  Per-call: " << std::setprecision(3) << (ms / numIterations) << " ms\n\n";
    }
    
    // Full cycle benchmark (ERX + repair + LS)
    std::cout << "Full cycle benchmark (crossover + repair to full size):\n\n";
    
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100; i++) {
            auto partial = erxOriginal(p1, p2, n, selectCount, rng);
            auto repaired = repairStandard(partial, distance, costs, n, selectCount);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "ERX + Repair: " << std::fixed << std::setprecision(2) << ms << " ms (100 calls)\n";
        std::cout << "  Per-call: " << std::setprecision(3) << (ms / 100) << " ms\n\n";
    }
    
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100; i++) {
            auto partial = skipERX(p1, p2, n, selectCount, rng);
            auto repaired = repairStandard(partial, distance, costs, n, selectCount);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "CommonNodes + Repair: " << std::fixed << std::setprecision(2) << ms << " ms (100 calls)\n";
        std::cout << "  Per-call: " << std::setprecision(3) << (ms / 100) << " ms\n";
    }
    
    std::cout << "\nKey insight: The repair step (O(n²)) dominates total time.\n";
    std::cout << "ERX vs CommonNodes difference is small compared to repair cost.\n";
    
    return 0;
}
