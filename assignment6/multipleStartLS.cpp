#include "../include/multipleStartLS.h"
#include "../include/localSearch.h"
#include "../include/calculateObjective.h"
#include <chrono>
#include <climits>

MSLSResult multipleStartLS(
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& randomInitials,
    int iterations,
    std::mt19937& rng
) {
    MSLSResult result;
    result.bestObjective = INT_MAX;
    result.iterations = iterations;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int iter = 0; iter < iterations; iter++) {
        // Use pre-generated random starting solution (cycling through if iterations > n)
        std::vector<int> initial = randomInitials[iter % n];
        
        // Apply steepest local search with edges exchange
        std::vector<int> solution = localSearchSteepestEdges(initial, distance, costs, n);
        
        // Evaluate solution
        int objective = calculateObjective(solution, distance, costs);
        
        // Update best if improved
        if (objective < result.bestObjective) {
            result.bestObjective = objective;
            result.bestSolution = solution;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return result;
}
