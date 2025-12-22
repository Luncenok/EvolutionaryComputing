// Quick test: Greedy LS vs Steepest LS
// Greedy takes first improving move, steepest scans whole neighborhood

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

// Greedy LS - takes first improving move
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
        
        // Randomize starting point to avoid bias
        int sz = sol.size();
        int startI = std::uniform_int_distribution<>(0, sz - 1)(rng);
        
        // 2-opt moves
        for (int iter = 0; iter < sz && !improved; iter++) {
            int i = (startI + iter) % sz;
            for (int j = i + 2; j < sz; j++) {
                if (i == 0 && j == sz - 1) continue;
                int a = sol[i], b = sol[(i + 1) % sz], c = sol[j], d = sol[(j + 1) % sz];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
                if (delta < 0) {
                    std::reverse(sol.begin() + i + 1, sol.begin() + j + 1);
                    improved = true;
                    break;
                }
            }
        }
        
        if (improved) continue;
        
        // Node exchange moves
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
                    improved = true;
                    break;
                }
            }
        }
    }
    return sol;
}

// Steepest LS - scans whole neighborhood
static std::vector<int> localSearchSteepest(
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
        int sz = sol.size();
        
        for (int i = 0; i < sz; i++) {
            for (int j = i + 2; j < sz; j++) {
                if (i == 0 && j == sz - 1) continue;
                int a = sol[i], b = sol[(i + 1) % sz], c = sol[j], d = sol[(j + 1) % sz];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
                if (delta < bestDelta) {
                    bestDelta = delta; bestType = 0; bestPos1 = i; bestPos2 = j;
                }
            }
        }
        
        for (int pos = 0; pos < sz; pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int prev = (pos - 1 + sz) % sz, next = (pos + 1) % sz;
                int delta = (distance[sol[prev]][node] + distance[node][sol[next]] + costs[node])
                          - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
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
    std::cout << "Greedy vs Steepest LS Comparison\n";
    std::cout << "=================================\n\n";
    
    std::vector<std::vector<int>> distance;
    std::vector<int> costs;
    int n, selectCount;
    loadInstance("input/TSPA.csv", distance, costs, n, selectCount);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    const int numRuns = 100;
    
    // Generate same starting solutions
    std::vector<std::vector<int>> starts;
    for (int i = 0; i < numRuns; i++) {
        starts.push_back(greedyCycle(i % n, selectCount, distance, costs));
    }
    
    // Test Steepest
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalObj = 0;
        for (int i = 0; i < numRuns; i++) {
            auto result = localSearchSteepest(starts[i], distance, costs, n);
            totalObj += calculateObjective(result, distance, costs);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Steepest LS:\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n\n";
    }
    
    // Test Greedy
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalObj = 0;
        for (int i = 0; i < numRuns; i++) {
            auto result = localSearchGreedy(starts[i], distance, costs, n, rng);
            totalObj += calculateObjective(result, distance, costs);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Greedy LS:\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n\n";
    }
    
    std::cout << "Done!\n";
    return 0;
}
