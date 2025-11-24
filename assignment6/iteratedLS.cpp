#include "../include/iteratedLS.h"
#include "../include/localSearch.h"
#include "../include/calculateObjective.h"
#include <chrono>
#include <climits>
#include <algorithm>

// Perturbation function: performs multiple random edge exchanges to escape local optimum
// This destroys enough structure to escape but preserves quality better than random restart
std::vector<int> perturbSolution(
    const std::vector<int>& solution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    std::mt19937& rng
) {
    std::vector<int> perturbed = solution;
    int solSize = perturbed.size();
    
    // Perturbation strength: perform k random 2-opt moves
    // k is proportional to solution size but bounded
    int k = std::min(5, std::max(2, solSize / 20));
    
    for (int i = 0; i < k; i++) {
        // Random edge exchange (2-opt)
        std::uniform_int_distribution<> dist(0, solSize - 1);
        int pos1 = dist(rng);
        int pos2 = dist(rng);
        
        // Ensure pos1 and pos2 are different and not adjacent
        while (pos1 == pos2 || (pos1 + 1) % solSize == pos2 || (pos2 + 1) % solSize == pos1) {
            pos2 = dist(rng);
        }
        
        if (pos1 > pos2) std::swap(pos1, pos2);
        
        // Apply 2-opt: reverse segment between pos1+1 and pos2
        std::reverse(perturbed.begin() + pos1 + 1, perturbed.begin() + pos2 + 1);
    }
    
    // Additional perturbation: random node exchange with probability 0.3
    std::uniform_real_distribution<> probDist(0.0, 1.0);
    if (probDist(rng) < 0.3) {
        // Build set of nodes not in solution
        std::vector<bool> inSolution(n, false);
        for (int node : perturbed) inSolution[node] = true;
        
        std::vector<int> notSelected;
        for (int i = 0; i < n; i++) {
            if (!inSolution[i]) notSelected.push_back(i);
        }
        
        if (!notSelected.empty()) {
            // Replace random node in solution with random external node
            std::uniform_int_distribution<> posDist(0, solSize - 1);
            std::uniform_int_distribution<> nodeDist(0, notSelected.size() - 1);
            
            int replacePos = posDist(rng);
            int newNode = notSelected[nodeDist(rng)];
            perturbed[replacePos] = newNode;
        }
    }
    
    return perturbed;
}

ILSResult iteratedLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    double timeLimit,
    std::mt19937& rng
) {
    ILSResult result;
    result.bestObjective = INT_MAX;
    result.lsRuns = 0;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Use first pre-generated random solution as initial solution
    std::vector<int> current = randomInitials[0];
    
    // Apply local search to initial solution
    current = localSearchSteepestEdges(current, distance, costs, n);
    result.lsRuns++;
    
    int currentObj = calculateObjective(current, distance, costs);
    result.bestObjective = currentObj;
    result.bestSolution = current;
    
    // Main ILS loop - continue until time limit
    while (true) {
        // Check time
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(currentTime - startTime).count();
        if (elapsed >= timeLimit) break;
        
        // Perturbation
        std::vector<int> perturbed = perturbSolution(current, distance, costs, n, rng);
        
        // Local search on perturbed solution
        std::vector<int> improved = localSearchSteepestEdges(perturbed, distance, costs, n);
        result.lsRuns++;
        
        int improvedObj = calculateObjective(improved, distance, costs);
        
        // Acceptance criterion: accept if better
        if (improvedObj < currentObj) {
            current = improved;
            currentObj = improvedObj;
            
            // Update global best
            if (improvedObj < result.bestObjective) {
                result.bestObjective = improvedObj;
                result.bestSolution = improved;
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return result;
}
