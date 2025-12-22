// Quality-Distance Correlation Analysis
// From lecture THEORY L545-595: "Perform quality-distance correlation tests"
// 
// This tests which solution features correlate with quality

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <iomanip>

#include "include/calculateObjective.h"
#include "include/greedyCycle.h"

// Different similarity measures to test
// 1. Common nodes
int commonNodes(const std::vector<int>& a, const std::vector<int>& b) {
    std::unordered_set<int> setA(a.begin(), a.end());
    int common = 0;
    for (int x : b) if (setA.count(x)) common++;
    return common;
}

// 2. Common edges (undirected)
int commonEdges(const std::vector<int>& a, const std::vector<int>& b) {
    auto makeEdgeSet = [](const std::vector<int>& s) {
        std::unordered_set<long long> edges;
        for (int i = 0; i < s.size(); i++) {
            int x = s[i], y = s[(i + 1) % s.size()];
            if (x > y) std::swap(x, y);
            edges.insert((long long)x * 1000000 + y);
        }
        return edges;
    };
    auto edgesA = makeEdgeSet(a);
    auto edgesB = makeEdgeSet(b);
    int common = 0;
    for (auto& e : edgesA) if (edgesB.count(e)) common++;
    return common;
}

// 3. Common node pairs (not necessarily adjacent)
int commonPairs(const std::vector<int>& a, const std::vector<int>& b) {
    auto makePairSet = [](const std::vector<int>& s) {
        std::unordered_set<long long> pairs;
        for (int i = 0; i < s.size(); i++) {
            for (int j = i + 1; j < s.size(); j++) {
                int x = s[i], y = s[j];
                if (x > y) std::swap(x, y);
                pairs.insert((long long)x * 1000000 + y);
            }
        }
        return pairs;
    };
    auto pairsA = makePairSet(a);
    auto pairsB = makePairSet(b);
    int common = 0;
    for (auto& p : pairsA) if (pairsB.count(p)) common++;
    return common;
}

// Local search
static std::vector<int> localSearch(
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
                int sz = sol.size();
                int a = sol[i], b = sol[(i + 1) % sz], c = sol[j], d = sol[(j + 1) % sz];
                int delta = distance[a][c] + distance[b][d] - distance[a][b] - distance[c][d];
                if (delta < bestDelta) {
                    bestDelta = delta; bestType = 0; bestPos1 = i; bestPos2 = j;
                }
            }
        }
        
        for (int pos = 0; pos < sol.size(); pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int sz = sol.size();
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

// Pearson correlation coefficient
double correlation(const std::vector<double>& x, const std::vector<double>& y) {
    int n = x.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
    for (int i = 0; i < n; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    double num = n * sumXY - sumX * sumY;
    double den = std::sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
    return den > 0 ? num / den : 0;
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
    std::cout << "Quality-Distance Correlation Analysis\n";
    std::cout << "======================================\n";
    std::cout << "From lecture: Boese, Kahng, Muddu (1994)\n\n";
    
    std::vector<std::vector<int>> distance;
    std::vector<int> costs;
    int n, selectCount;
    loadInstance("input/TSPA.csv", distance, costs, n, selectCount);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    
    const int numSolutions = 100;
    
    std::cout << "Generating " << numSolutions << " local optima...\n";
    
    // Generate local optima from random starting points
    std::vector<std::vector<int>> solutions;
    std::vector<int> objectives;
    
    for (int i = 0; i < numSolutions; i++) {
        auto start = greedyCycle(i % n, selectCount, distance, costs);
        auto optimum = localSearch(start, distance, costs, n);
        solutions.push_back(optimum);
        objectives.push_back(calculateObjective(optimum, distance, costs));
    }
    
    // Find best solution
    int bestIdx = std::min_element(objectives.begin(), objectives.end()) - objectives.begin();
    int bestObj = objectives[bestIdx];
    
    std::cout << "Best objective: " << bestObj << "\n\n";
    
    // Calculate similarity to best for each measure
    std::vector<double> simNodes, simEdges, simPairs;
    std::vector<double> objs;
    
    for (int i = 0; i < numSolutions; i++) {
        simNodes.push_back(commonNodes(solutions[i], solutions[bestIdx]));
        simEdges.push_back(commonEdges(solutions[i], solutions[bestIdx]));
        simPairs.push_back(commonPairs(solutions[i], solutions[bestIdx]));
        objs.push_back(objectives[i]);
    }
    
    // Calculate average similarity to all better solutions
    std::vector<double> avgSimNodes(numSolutions), avgSimEdges(numSolutions), avgSimPairs(numSolutions);
    
    for (int i = 0; i < numSolutions; i++) {
        double sumN = 0, sumE = 0, sumP = 0;
        int count = 0;
        for (int j = 0; j < numSolutions; j++) {
            if (objectives[j] < objectives[i]) {  // j is better
                sumN += commonNodes(solutions[i], solutions[j]);
                sumE += commonEdges(solutions[i], solutions[j]);
                sumP += commonPairs(solutions[i], solutions[j]);
                count++;
            }
        }
        if (count > 0) {
            avgSimNodes[i] = sumN / count;
            avgSimEdges[i] = sumE / count;
            avgSimPairs[i] = sumP / count;
        }
    }
    
    std::cout << "Correlation with objective (distance to best):\n";
    std::cout << "  Common Nodes: " << std::fixed << std::setprecision(3) 
              << correlation(simNodes, objs) << "\n";
    std::cout << "  Common Edges: " << std::fixed << std::setprecision(3) 
              << correlation(simEdges, objs) << "\n";
    std::cout << "  Common Pairs: " << std::fixed << std::setprecision(3) 
              << correlation(simPairs, objs) << "\n";
    
    std::cout << "\nCorrelation with objective (avg sim to better):\n";
    std::cout << "  Common Nodes: " << std::fixed << std::setprecision(3) 
              << correlation(avgSimNodes, objs) << "\n";
    std::cout << "  Common Edges: " << std::fixed << std::setprecision(3) 
              << correlation(avgSimEdges, objs) << "\n";
    std::cout << "  Common Pairs: " << std::fixed << std::setprecision(3) 
              << correlation(avgSimPairs, objs) << "\n";
    
    std::cout << "\nInterpretation:\n";
    std::cout << "(Negative = more similar to best -> lower objective = GOOD)\n";
    std::cout << "(Positive = more similar to best -> higher objective = BAD)\n";
    
    std::cout << "\nDone!\n";
    return 0;
}
