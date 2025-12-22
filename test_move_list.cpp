// List of Improving Moves Test
// From lecture LOCALSEARCH L1050-1088:
// "List of moves that bring improvement ordered from the best to the worst"
//
// Instead of checking the whole neighborhood each iteration,
// maintain a sorted list of improving moves.

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
#include <queue>
#include <climits>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"
#include "include/greedyRegret2Weighted.h"
#include "include/nearestNeighborAny.h"

// Move structure
struct Move {
    int type;       // 0 = 2-opt, 1 = node exchange
    int pos1, pos2; // For 2-opt: positions; For exchange: pos, newNode
    int delta;
    
    bool operator>(const Move& other) const {
        return delta > other.delta;  // Min-heap by delta
    }
};

// Core delta functions
static inline int deltaReverse(const std::vector<int> &sol, int pos1, int pos2,
                               const std::vector<std::vector<int>> &distance) {
    int n = sol.size();
    int a = sol[pos1], b = sol[(pos1 + 1) % n];
    int c = sol[pos2], d = sol[(pos2 + 1) % n];
    return distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
}

static inline int deltaExchange(const std::vector<int> &sol, int pos, int newNode,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs) {
    int n = sol.size();
    int prev = (pos - 1 + n) % n, next = (pos + 1) % n;
    return (distance[sol[prev]][newNode] + distance[newNode][sol[next]] + costs[newNode])
         - (distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]]);
}

// LS with List of Improving Moves
static std::vector<int> localSearchWithMoveList(
    const std::vector<int> &initialSolution,
    const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, int n) {
    
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    // Priority queue: min-heap by delta (most improving first)
    std::priority_queue<Move, std::vector<Move>, std::greater<Move>> moveList;
    
    // Build initial move list with all improving moves
    auto buildMoveList = [&]() {
        while (!moveList.empty()) moveList.pop();
        
        // 2-opt moves
        for (int i = 0; i < sol.size(); i++) {
            for (int j = i + 2; j < sol.size(); j++) {
                if (i == 0 && j == sol.size() - 1) continue;
                int delta = deltaReverse(sol, i, j, distance);
                if (delta < 0) {
                    moveList.push({0, i, j, delta});
                }
            }
        }
        
        // Exchange moves
        for (int pos = 0; pos < sol.size(); pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int delta = deltaExchange(sol, pos, node, distance, costs);
                if (delta < 0) {
                    moveList.push({1, pos, node, delta});
                }
            }
        }
    };
    
    buildMoveList();
    
    while (!moveList.empty()) {
        Move best = moveList.top();
        moveList.pop();
        
        // Check if move is still valid
        bool valid = false;
        if (best.type == 0) {
            // 2-opt: check if positions are still valid
            if (best.pos1 < sol.size() && best.pos2 < sol.size()) {
                int newDelta = deltaReverse(sol, best.pos1, best.pos2, distance);
                valid = (newDelta < 0);
            }
        } else {
            // Exchange: check if node is still available
            if (!inSolution[best.pos2] && best.pos1 < sol.size()) {
                int newDelta = deltaExchange(sol, best.pos1, best.pos2, distance, costs);
                valid = (newDelta < 0);
            }
        }
        
        if (valid) {
            if (best.type == 0) {
                std::reverse(sol.begin() + best.pos1 + 1, sol.begin() + best.pos2 + 1);
            } else {
                inSolution[sol[best.pos1]] = false;
                inSolution[best.pos2] = true;
                sol[best.pos1] = best.pos2;
            }
            
            // Rebuild move list after change (simpler than incremental update)
            buildMoveList();
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
    std::cout << "List of Improving Moves Test\n";
    std::cout << "============================\n\n";
    
    std::vector<std::vector<int>> distance;
    std::vector<int> costs;
    int n, selectCount;
    loadInstance("input/TSPA.csv", distance, costs, n, selectCount);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    const int numRuns = 50;
    
    std::cout << "n = " << n << ", selectCount = " << selectCount << "\n";
    std::cout << "Running " << numRuns << " LS calls...\n\n";
    
    // Generate starting solutions
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
        std::cout << "Standard LS (steepest):\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n\n";
    }
    
    // Test LS with move list
    {
        auto start = std::chrono::high_resolution_clock::now();
        int totalObj = 0;
        for (int i = 0; i < numRuns; i++) {
            auto result = localSearchWithMoveList(startSolutions[i], distance, costs, n);
            totalObj += calculateObjective(result, distance, costs);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "LS with Move List:\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
        std::cout << "  Avg objective: " << (totalObj / numRuns) << "\n\n";
    }
    
    std::cout << "Done!\n";
    return 0;
}
