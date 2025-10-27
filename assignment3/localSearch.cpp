#include "../include/localSearch.h"
#include <algorithm>
#include <climits>
#include <numeric>

// Calculate delta for swapping two nodes in the cycle (intra-route)
int deltaSwapNodes(const std::vector<int>& sol, int pos1, int pos2, 
                   const std::vector<std::vector<int>>& distance) {
    int n = sol.size();
    if (pos1 == pos2) return 0;
    if (pos1 > pos2) std::swap(pos1, pos2);
    
    int prev1 = (pos1 - 1 + n) % n;
    int next1 = (pos1 + 1) % n;
    int prev2 = (pos2 - 1 + n) % n;
    int next2 = (pos2 + 1) % n;
    
    // Handle adjacent nodes specially
    if (next1 == pos2) {
        // Nodes are adjacent: prev1-pos1-pos2-next2
        int oldCost = distance[sol[prev1]][sol[pos1]] + 
                      distance[sol[pos1]][sol[pos2]] + 
                      distance[sol[pos2]][sol[next2]];
        int newCost = distance[sol[prev1]][sol[pos2]] + 
                      distance[sol[pos2]][sol[pos1]] + 
                      distance[sol[pos1]][sol[next2]];
        return newCost - oldCost;
    }
    
    // Handle wraparound case: pos2-pos1 are adjacent (pos2 followed by pos1)
    if (next2 == pos1) {
        // Nodes are adjacent with wraparound: prev2-pos2-pos1-next1
        int oldCost = distance[sol[prev2]][sol[pos2]] + 
                      distance[sol[pos2]][sol[pos1]] + 
                      distance[sol[pos1]][sol[next1]];
        int newCost = distance[sol[prev2]][sol[pos1]] + 
                      distance[sol[pos1]][sol[pos2]] + 
                      distance[sol[pos2]][sol[next1]];
        return newCost - oldCost;
    }
    
    // Non-adjacent nodes
    int oldCost = distance[sol[prev1]][sol[pos1]] + distance[sol[pos1]][sol[next1]] +
                  distance[sol[prev2]][sol[pos2]] + distance[sol[pos2]][sol[next2]];
    int newCost = distance[sol[prev1]][sol[pos2]] + distance[sol[pos2]][sol[next1]] +
                  distance[sol[prev2]][sol[pos1]] + distance[sol[pos1]][sol[next2]];
    return newCost - oldCost;
}

// Calculate delta for reversing segment (two-edges exchange, intra-route)
int deltaReverseSegment(const std::vector<int>& sol, int pos1, int pos2,
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
int deltaExchangeNodes(const std::vector<int>& sol, int pos, int newNode,
                       const std::vector<std::vector<int>>& distance,
                       const std::vector<int>& costs) {
    int n = sol.size();
    int prev = (pos - 1 + n) % n;
    int next = (pos + 1) % n;
    
    int oldCost = distance[sol[prev]][sol[pos]] + distance[sol[pos]][sol[next]] + costs[sol[pos]];
    int newCost = distance[sol[prev]][newNode] + distance[newNode][sol[next]] + costs[newNode];
    return newCost - oldCost;
}

std::vector<int> localSearchSteepestNodes(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n
) {
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    bool improved = true;
    while (improved) {
        improved = false;
        int bestDelta = 0;
        int bestType = -1; // 0: intra-swap, 1: inter-exchange
        int bestPos1 = -1, bestPos2 = -1, bestNode = -1;
        
        // Evaluate all intra-route moves (nodes exchange)
        for (int i = 0; i < sol.size(); i++) {
            for (int j = i + 1; j < sol.size(); j++) {
                int delta = deltaSwapNodes(sol, i, j, distance);
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestType = 0;
                    bestPos1 = i;
                    bestPos2 = j;
                }
            }
        }
        
        // Evaluate all inter-route moves (exchange with non-selected)
        for (int pos = 0; pos < sol.size(); pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int delta = deltaExchangeNodes(sol, pos, node, distance, costs);
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestType = 1;
                    bestPos1 = pos;
                    bestNode = node;
                }
            }
        }
        
        // Apply best move
        if (bestDelta < 0) {
            improved = true;
            if (bestType == 0) {
                // Intra-route: swap nodes
                std::swap(sol[bestPos1], sol[bestPos2]);
            } else {
                // Inter-route: exchange nodes
                inSolution[sol[bestPos1]] = false;
                inSolution[bestNode] = true;
                sol[bestPos1] = bestNode;
            }
        }
    }
    
    return sol;
}

std::vector<int> localSearchSteepestEdges(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n
) {
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    bool improved = true;
    while (improved) {
        improved = false;
        int bestDelta = 0;
        int bestType = -1; // 0: intra-reverse, 1: inter-exchange
        int bestPos1 = -1, bestPos2 = -1, bestNode = -1;
        
        // Evaluate all intra-route moves (edges exchange / segment reversal)
        for (int i = 0; i < sol.size(); i++) {
            for (int j = i + 1; j < sol.size(); j++) {
                int delta = deltaReverseSegment(sol, i, j, distance);
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestType = 0;
                    bestPos1 = i;
                    bestPos2 = j;
                }
            }
        }
        
        // Evaluate all inter-route moves (exchange with non-selected)
        for (int pos = 0; pos < sol.size(); pos++) {
            for (int node = 0; node < n; node++) {
                if (inSolution[node]) continue;
                int delta = deltaExchangeNodes(sol, pos, node, distance, costs);
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestType = 1;
                    bestPos1 = pos;
                    bestNode = node;
                }
            }
        }
        
        // Apply best move
        if (bestDelta < 0) {
            improved = true;
            if (bestType == 0) {
                // Intra-route: reverse segment
                std::reverse(sol.begin() + bestPos1 + 1, sol.begin() + bestPos2 + 1);
            } else {
                // Inter-route: exchange nodes
                inSolution[sol[bestPos1]] = false;
                inSolution[bestNode] = true;
                sol[bestPos1] = bestNode;
            }
        }
    }
    
    return sol;
}

std::vector<int> localSearchGreedyNodes(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    std::mt19937& rng
) {
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    int solSize = sol.size();
    int numIntra = solSize * (solSize - 1) / 2;
    int numInter = solSize * (n - solSize);
    int totalMoves = numIntra + numInter;
    
    bool improved = true;
    while (improved) {
        improved = false;
        
        // Random sampling without replacement - no vector creation!
        std::vector<bool> tried(totalMoves, false);
        int triedCount = 0;
        std::uniform_int_distribution<> moveDist(0, totalMoves - 1);
        
        while (!improved && triedCount < totalMoves) {
            // Pick random untried move
            int moveIdx = moveDist(rng);
            if (tried[moveIdx]) continue;
            
            tried[moveIdx] = true;
            triedCount++;
            
            bool isInter = (moveIdx < numInter);
            int delta;
            
            if (isInter) {
                // Inter-route move
                int pos = moveIdx / (n - solSize);
                int nodeIdx = moveIdx % (n - solSize);
                
                // Find the nodeIdx-th non-selected node
                int node = -1;
                int count = 0;
                for (int i = 0; i < n; i++) {
                    if (!inSolution[i]) {
                        if (count == nodeIdx) {
                            node = i;
                            break;
                        }
                        count++;
                    }
                }
                
                delta = deltaExchangeNodes(sol, pos, node, distance, costs);
                if (delta < 0) {
                    inSolution[sol[pos]] = false;
                    inSolution[node] = true;
                    sol[pos] = node;
                    improved = true;
                }
            } else {
                // Intra-route move
                int pairIdx = moveIdx - numInter;
                int i = 0, j = 0;
                
                // Convert linear index to (i, j) pair
                int count = 0;
                bool found = false;
                for (i = 0; i < solSize && !found; i++) {
                    for (j = i + 1; j < solSize; j++) {
                        if (count == pairIdx) {
                            found = true;
                            break;
                        }
                        count++;
                    }
                }
                
                delta = deltaSwapNodes(sol, i, j, distance);
                if (delta < 0) {
                    std::swap(sol[i], sol[j]);
                    improved = true;
                }
            }
        }
    }
    
    return sol;
}

std::vector<int> localSearchGreedyEdges(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    std::mt19937& rng
) {
    std::vector<int> sol = initialSolution;
    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;
    
    int solSize = sol.size();
    int numIntra = solSize * (solSize - 1) / 2;
    int numInter = solSize * (n - solSize);
    int totalMoves = numIntra + numInter;
    
    bool improved = true;
    while (improved) {
        improved = false;
        
        // Random sampling without replacement - no vector creation!
        std::vector<bool> tried(totalMoves, false);
        int triedCount = 0;
        std::uniform_int_distribution<> moveDist(0, totalMoves - 1);
        
        while (!improved && triedCount < totalMoves) {
            // Pick random untried move
            int moveIdx = moveDist(rng);
            if (tried[moveIdx]) continue;
            
            tried[moveIdx] = true;
            triedCount++;
            
            bool isInter = (moveIdx < numInter);
            int delta;
            
            if (isInter) {
                // Inter-route move
                int pos = moveIdx / (n - solSize);
                int nodeIdx = moveIdx % (n - solSize);
                
                // Find the nodeIdx-th non-selected node
                int node = -1;
                int count = 0;
                for (int i = 0; i < n; i++) {
                    if (!inSolution[i]) {
                        if (count == nodeIdx) {
                            node = i;
                            break;
                        }
                        count++;
                    }
                }
                
                delta = deltaExchangeNodes(sol, pos, node, distance, costs);
                if (delta < 0) {
                    inSolution[sol[pos]] = false;
                    inSolution[node] = true;
                    sol[pos] = node;
                    improved = true;
                }
            } else {
                // Intra-route move: edges exchange (segment reversal)
                int pairIdx = moveIdx - numInter;
                int i = 0, j = 0;
                
                // Convert linear index to (i, j) pair
                int count = 0;
                bool found = false;
                for (i = 0; i < solSize && !found; i++) {
                    for (j = i + 1; j < solSize; j++) {
                        if (count == pairIdx) {
                            found = true;
                            break;
                        }
                        count++;
                    }
                }
                
                delta = deltaReverseSegment(sol, i, j, distance);
                if (delta < 0) {
                    std::reverse(sol.begin() + i + 1, sol.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
    
    return sol;
}
