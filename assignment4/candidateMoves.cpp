#include "../include/candidateMoves.h"
#include <algorithm>
#include <climits>
#include <numeric>
#include <unordered_set>

// Build nearest neighbors for each node based on distance + cost
std::vector<std::vector<int>> buildNearestNeighbors(
    int n,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int k
) {
    std::vector<std::vector<int>> nearestNeighbors(n);
    
    for (int i = 0; i < n; i++) {
        // Create pairs of (cost, node_index) for all other nodes
        std::vector<std::pair<int, int>> neighbors;
        neighbors.reserve(n - 1);
        
        for (int j = 0; j < n; j++) {
            if (i != j) {
                int combinedCost = distance[i][j] + costs[j];
                neighbors.push_back({combinedCost, j});
            }
        }
        
        // Partial sort to get k nearest
        int actualK = std::min(k, (int)neighbors.size());
        std::partial_sort(neighbors.begin(), neighbors.begin() + actualK, neighbors.end());
        
        // Store the indices of k nearest neighbors
        nearestNeighbors[i].reserve(actualK);
        for (int idx = 0; idx < actualK; idx++) {
            nearestNeighbors[i].push_back(neighbors[idx].second);
        }
    }
    
    return nearestNeighbors;
}

// Calculate delta for reversing segment (two-edges exchange, intra-route)
static int deltaReverseSegment(const std::vector<int>& sol, int pos1, int pos2,
                        const std::vector<std::vector<int>>& distance) {
    int n = sol.size();
    if (pos1 == pos2 || (pos1 + 1) % n == pos2) return 0;
    
    // Remove edges at boundaries and add new edges
    int oldCost = distance[sol[pos1]][sol[(pos1 + 1) % n]] + 
                  distance[sol[pos2]][sol[(pos2 + 1) % n]];
    int newCost = distance[sol[pos1]][sol[pos2]] + 
                  distance[sol[(pos1 + 1) % n]][sol[(pos2 + 1) % n]];
    return newCost - oldCost;
}

// Calculate delta for exchanging selected node with non-selected node (inter-route)
static int deltaExchangeNodes(const std::vector<int>& sol, int pos, int newNode,
                       const std::vector<std::vector<int>>& distance,
                       const std::vector<int>& costs) {
    int n = sol.size();
    int prev = (pos - 1 + n) % n;
    int next = (pos + 1) % n;
    
    int oldCost = distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]];
    int newCost = distance[sol[prev]][newNode] + distance[newNode][sol[next]] + costs[newNode];
    return newCost - oldCost;
}

// Check if an edge is a candidate edge (one of the endpoints has the other as a nearest neighbor)
static bool isCandidateEdge(int node1, int node2, 
                     const std::vector<std::vector<int>>& nearestNeighbors) {
    // Check if node2 is in node1's nearest neighbors
    const auto& nn1 = nearestNeighbors[node1];
    if (std::find(nn1.begin(), nn1.end(), node2) != nn1.end()) {
        return true;
    }
    
    // Check if node1 is in node2's nearest neighbors
    const auto& nn2 = nearestNeighbors[node2];
    if (std::find(nn2.begin(), nn2.end(), node1) != nn2.end()) {
        return true;
    }
    
    return false;
}

// Steepest local search with candidate moves (edges exchange)
std::vector<int> localSearchSteepestEdgesCandidates(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    int k
) {
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    // Build nearest neighbors for all nodes
    auto nearestNeighbors = buildNearestNeighbors(n, distance, costs, k);
    
    // Build position map for quick lookup
    std::vector<int> nodePosition(n, -1);
    for (int i = 0; i < sol.size(); i++) {
        nodePosition[sol[i]] = i;
    }
    
    bool improved = true;
    while (improved) {
        improved = false;
        int bestDelta = 0;
        int bestType = -1; // 0: intra-reverse, 1: inter-exchange
        int bestPos1 = -1, bestPos2 = -1, bestNode = -1;
        
        // Evaluate candidate intra-route moves (edges exchange / segment reversal)
        // A 2-opt move between positions i and j introduces two new edges:
        // Edge 1: (sol[i], sol[j])
        // Edge 2: (sol[i+1], sol[j+1])
        // We need at least ONE of these to be a candidate edge
        
        // Use a simple grid to track evaluated pairs - much faster than set
        std::vector<std::vector<bool>> evaluated(sol.size(), std::vector<bool>(sol.size(), false));
        
        for (int i = 0; i < sol.size(); i++) {
            int node_i = sol[i];
            int next_i = (i + 1) % sol.size();
            int node_next_i = sol[next_i];
            
            // Case 1: Find moves where (sol[i], sol[j]) is a candidate edge
            for (int neighbor : nearestNeighbors[node_i]) {
                if (inSolution[neighbor]) {
                    int j = nodePosition[neighbor];
                    if (i == j || (i + 1) % sol.size() == j) continue;
                    
                    int pos1 = std::min(i, j);
                    int pos2 = std::max(i, j);
                    
                    if (evaluated[pos1][pos2]) continue;
                    evaluated[pos1][pos2] = true;
                    
                    int delta = deltaReverseSegment(sol, pos1, pos2, distance);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestType = 0;
                        bestPos1 = pos1;
                        bestPos2 = pos2;
                    }
                }
            }
            
            // Case 2: Find moves where (sol[i+1], sol[j+1]) is a candidate edge
            for (int neighbor : nearestNeighbors[node_next_i]) {
                if (inSolution[neighbor]) {
                    int j_plus_1 = nodePosition[neighbor];
                    int j = (j_plus_1 - 1 + sol.size()) % sol.size();
                    if (i == j || (i + 1) % sol.size() == j) continue;
                    
                    int pos1 = std::min(i, j);
                    int pos2 = std::max(i, j);
                    
                    if (evaluated[pos1][pos2]) continue;
                    evaluated[pos1][pos2] = true;
                    
                    int delta = deltaReverseSegment(sol, pos1, pos2, distance);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestType = 0;
                        bestPos1 = pos1;
                        bestPos2 = pos2;
                    }
                }
            }
        }
        
        // Evaluate candidate inter-route moves (exchange with non-selected)
        // Exchanging node at pos introduces edges: (sol[prev], newNode) and (newNode, sol[next])
        // At least one must be a candidate edge
        
        // Use 2D boolean array for O(1) duplicate checking
        std::vector<std::vector<bool>> evaluatedInter(sol.size(), std::vector<bool>(n, false));
        
        for (int pos = 0; pos < sol.size(); pos++) {
            int prev = (pos - 1 + sol.size()) % sol.size();
            int next = (pos + 1) % sol.size();
            
            // Case 1: New edge (sol[prev], newNode) is a candidate edge
            // Try all nearest neighbors of sol[prev]
            for (int newNode : nearestNeighbors[sol[prev]]) {
                if (!inSolution[newNode]) {
                    if (evaluatedInter[pos][newNode]) continue;
                    evaluatedInter[pos][newNode] = true;
                    
                    int delta = deltaExchangeNodes(sol, pos, newNode, distance, costs);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestType = 1;
                        bestPos1 = pos;
                        bestNode = newNode;
                    }
                }
            }
            
            // Case 2: New edge (newNode, sol[next]) is a candidate edge
            // Try all nearest neighbors of sol[next]
            for (int newNode : nearestNeighbors[sol[next]]) {
                if (!inSolution[newNode]) {
                    if (evaluatedInter[pos][newNode]) continue;
                    evaluatedInter[pos][newNode] = true;
                    
                    int delta = deltaExchangeNodes(sol, pos, newNode, distance, costs);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        bestType = 1;
                        bestPos1 = pos;
                        bestNode = newNode;
                    }
                }
            }
        }
        
        // Apply best move
        if (bestDelta < 0) {
            improved = true;
            if (bestType == 0) {
                // Intra-route: reverse segment
                std::reverse(sol.begin() + bestPos1 + 1, sol.begin() + bestPos2 + 1);
                
                // Update position map for affected nodes
                for (int i = bestPos1 + 1; i <= bestPos2; i++) {
                    nodePosition[sol[i]] = i;
                }
            } else {
                // Inter-route: exchange nodes
                int oldNode = sol[bestPos1];
                inSolution[oldNode] = false;
                inSolution[bestNode] = true;
                sol[bestPos1] = bestNode;
                
                // Update position map
                nodePosition[oldNode] = -1;
                nodePosition[bestNode] = bestPos1;
            }
        }
    }
    
    return sol;
}