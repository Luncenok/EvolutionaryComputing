#include "../include/largeNeighborhoodSearch.h"
#include "../include/localSearch.h"
#include "../include/calculateObjective.h"
#include <chrono>
#include <climits>
#include <algorithm>
#include <numeric>
#include <cmath>

// Destroy operator: removes a fraction of nodes from the solution
// Uses weighted random removal - nodes connected by longer edges have higher probability of removal
std::vector<int> destroySolution(
    const std::vector<int>& solution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    double destroyFraction,
    std::mt19937& rng
) {
    int solSize = solution.size();
    int numToRemove = std::max(1, static_cast<int>(solSize * destroyFraction));
    
    // Calculate weights based on edge costs (longer edges = higher weight for adjacent nodes)
    std::vector<double> weights(solSize);
    for (int i = 0; i < solSize; i++) {
        int prev = (i - 1 + solSize) % solSize;
        int next = (i + 1) % solSize;
        // Weight = sum of adjacent edge lengths + node cost (normalized)
        double edgeCost = distance[solution[prev]][solution[i]] + distance[solution[i]][solution[next]];
        weights[i] = edgeCost + costs[solution[i]];  // Include node cost with equal weight
    }
    
    // Select nodes to remove using weighted random selection
    std::vector<bool> toRemove(solSize, false);
    std::vector<int> indices(solSize);
    std::iota(indices.begin(), indices.end(), 0);
    
    for (int removed = 0; removed < numToRemove; removed++) {
        // Calculate sum of weights for non-removed nodes
        double totalWeight = 0.0;
        for (int i = 0; i < solSize; i++) {
            if (!toRemove[i]) totalWeight += weights[i];
        }
        
        // Weighted random selection
        std::uniform_real_distribution<> dist(0.0, totalWeight);
        double r = dist(rng);
        double cumulative = 0.0;
        
        for (int i = 0; i < solSize; i++) {
            if (toRemove[i]) continue;
            cumulative += weights[i];
            if (cumulative >= r) {
                toRemove[i] = true;
                break;
            }
        }
    }
    
    // Build remaining solution (preserving order)
    std::vector<int> remaining;
    for (int i = 0; i < solSize; i++) {
        if (!toRemove[i]) {
            remaining.push_back(solution[i]);
        }
    }
    
    return remaining;
}

// Repair operator: rebuilds solution to full size using greedy insertion
// Uses weighted 2-regret heuristic (best performing greedy heuristic)
std::vector<int> repairSolution(
    const std::vector<int>& partial,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    int selectCount,
    double wRegret,
    double wBest
) {
    std::vector<int> solution = partial;
    
    // Mark nodes already in solution
    std::vector<bool> selected(n, false);
    for (int node : solution) {
        selected[node] = true;
    }
    
    // If solution is empty or has only one node, start fresh
    if (solution.size() < 2) {
        // Find best starting node among unselected
        int bestStart = -1;
        int bestCost = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!selected[i] && costs[i] < bestCost) {
                bestCost = costs[i];
                bestStart = i;
            }
        }
        if (solution.empty()) {
            solution.push_back(bestStart);
            selected[bestStart] = true;
        }
        
        // Add second node
        if (solution.size() == 1 && selectCount > 1) {
            int startNode = solution[0];
            int bestNode = -1;
            int bestDelta = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (selected[i]) continue;
                int delta = distance[startNode][i] + costs[i];
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestNode = i;
                }
            }
            if (bestNode != -1) {
                solution.push_back(bestNode);
                selected[bestNode] = true;
            }
        }
    }
    
    // Use weighted 2-regret to insert remaining nodes
    const double EPSILON = 1e-9;
    const double INIT_SCORE = -1e18;
    
    while (solution.size() < selectCount) {
        int chooseNode = -1;
        int choosePos = -1;
        double bestScore = INIT_SCORE;
        int tieBestDelta = INT_MAX;
        int tieBestRegret = -1;
        
        for (int i = 0; i < n; i++) {
            if (selected[i]) continue;
            
            int best1 = INT_MAX, best2 = INT_MAX;
            int bestPos = -1;
            
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] 
                           - distance[solution[pos]][solution[next]] + costs[i];
                
                if (delta < best1) {
                    best2 = best1;
                    best1 = delta;
                    bestPos = pos + 1;
                } else if (delta < best2) {
                    best2 = delta;
                }
            }
            
            int regret = (best2 == INT_MAX ? 0 : (best2 - best1));
            double score = wRegret * regret - wBest * best1;
            
            if (score > bestScore || 
                (std::abs(score - bestScore) < EPSILON && 
                 (best1 < tieBestDelta || (best1 == tieBestDelta && regret > tieBestRegret)))) {
                bestScore = score;
                tieBestDelta = best1;
                tieBestRegret = regret;
                chooseNode = i;
                choosePos = bestPos;
            }
        }
        
        if (chooseNode == -1) break;  // No valid node found
        
        solution.insert(solution.begin() + choosePos, chooseNode);
        selected[chooseNode] = true;
    }
    
    return solution;
}

// LNS with local search after destroy-repair
LNSResult largeNeighborhoodSearchWithLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng,
    double destroyFraction
) {
    LNSResult result;
    result.bestObjective = INT_MAX;
    result.iterations = 0;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize with random solution
    std::vector<int> current = randomInitials[0];
    
    // Apply local search to initial solution (always)
    current = localSearchSteepestEdges(current, distance, costs, n);
    
    int currentObj = calculateObjective(current, distance, costs);
    result.bestObjective = currentObj;
    result.bestSolution = current;
    
    // Repair parameters (weighted 2-regret)
    double wRegret = 1.0, wBest = 1.0;
    
    // Main LNS loop
    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(currentTime - startTime).count();
        if (elapsed >= timeLimit) break;
        
        result.iterations++;
        
        // Destroy
        std::vector<int> destroyed = destroySolution(current, distance, costs, n, destroyFraction, rng);
        
        // Repair
        std::vector<int> repaired = repairSolution(destroyed, distance, costs, n, selectCount, wRegret, wBest);
        
        // Local search
        std::vector<int> improved = localSearchSteepestEdges(repaired, distance, costs, n);
        
        int improvedObj = calculateObjective(improved, distance, costs);
        
        // Accept if better
        if (improvedObj < result.bestObjective) {
            result.bestObjective = improvedObj;
            result.bestSolution = improved;
        }
        
        // Update current (always move to new solution for exploration)
        if (improvedObj < currentObj) {
            current = improved;
            currentObj = improvedObj;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return result;
}

// LNS without local search after destroy-repair
LNSResult largeNeighborhoodSearchNoLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng,
    double destroyFraction
) {
    LNSResult result;
    result.bestObjective = INT_MAX;
    result.iterations = 0;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize with random solution
    std::vector<int> current = randomInitials[0];
    
    // Apply local search to initial solution (always, as per spec)
    current = localSearchSteepestEdges(current, distance, costs, n);
    
    int currentObj = calculateObjective(current, distance, costs);
    result.bestObjective = currentObj;
    result.bestSolution = current;
    
    // Repair parameters (weighted 2-regret)
    double wRegret = 1.0, wBest = 1.0;
    
    // Main LNS loop
    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(currentTime - startTime).count();
        if (elapsed >= timeLimit) break;
        
        result.iterations++;
        
        // Destroy
        std::vector<int> destroyed = destroySolution(current, distance, costs, n, destroyFraction, rng);
        
        // Repair (no local search)
        std::vector<int> repaired = repairSolution(destroyed, distance, costs, n, selectCount, wRegret, wBest);
        
        int repairedObj = calculateObjective(repaired, distance, costs);
        
        // Accept if better
        if (repairedObj < result.bestObjective) {
            result.bestObjective = repairedObj;
            result.bestSolution = repaired;
        }
        
        // Update current (always move to new solution for exploration)
        if (repairedObj < currentObj) {
            current = repaired;
            currentObj = repairedObj;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return result;
}
