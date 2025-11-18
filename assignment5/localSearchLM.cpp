#include "../include/localSearch.h"
#include <iostream>
#include <vector>
#include <algorithm>

// --- DATA STRUCTURES ---

struct LMMove {
    int type; // 0 = 2-opt, 1 = exchange
    int a1, b1; // Edge 1 (or u, v for exchange source)
    int a2, b2; // Edge 2 (or x, y for exchange target)
    int newNode; // For exchange
    int delta;

    // Sort by Delta: Most negative (best improvement) first
    bool operator<(const LMMove& other) const {
        if (delta != other.delta) return delta < other.delta;
        if (type != other.type) return type < other.type;
        return a1 < other.a1;
    }
};

// --- HELPER FUNCTIONS ---

static void buildNodePositions(const std::vector<int>& sol, std::vector<int>& nodePos) {
    std::fill(nodePos.begin(), nodePos.end(), -1);
    for (int i = 0; i < (int)sol.size(); ++i) {
        nodePos[sol[i]] = i;
    }
}

// Check edge existence AND orientation
// a_to_b = true if a->b, false if b->a
static bool edgeExistsUndirected(const std::vector<int>& sol, 
                                 const std::vector<int>& nodePos, 
                                 int a, int b, 
                                 bool &a_to_b) {
    int sz = (int)sol.size();
    int pa = nodePos[a];
    if (pa >= 0) {
        int na = (pa + 1) % sz;
        if (sol[na] == b) { a_to_b = true; return true; }
    }
    int pb = nodePos[b];
    if (pb >= 0) {
        int nb = (pb + 1) % sz;
        if (sol[nb] == a) { a_to_b = false; return true; }
    }
    return false;
}

// --- DELTA FUNCTIONS ---

static int calculate2OptDelta(int u, int v, int x, int y, const std::vector<std::vector<int>>& dist) {
    // Remove (u,v) and (x,y). Add (u,x) and (v,y)
    return (dist[u][x] + dist[v][y]) - (dist[u][v] + dist[x][y]);
}

// --- GENERATOR HELPER ---

static void generateMovesForNodes(
    const std::vector<int>& nodesToScan, 
    const std::vector<int>& sol, 
    const std::vector<bool>& inSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    std::vector<LMMove>& moves,
    bool useSymmetryCheck
) {
    int sz = (int)sol.size();
    int n = (int)inSolution.size();
    
    for (int iPos : nodesToScan) {
        int u = sol[iPos];
        int v = sol[(iPos + 1) % sz]; // Edge u-v
        
        // 1. Intra-route 2-opt
        for (int j = 0; j < sz; ++j) {
            // Skip adjacent edges (cannot 2-opt with yourself or neighbor)
            if (iPos == j || (iPos + 1) % sz == j || (j + 1) % sz == iPos) continue;
            
            int x = sol[j];
            int y = sol[(j + 1) % sz]; // Edge x-y
            
            // Only apply symmetry optimization if we are scanning EVERY node (Initialization).
            // If we are updating (partial scan), we MUST check u > x because x might not be in 'nodesToScan'.
            if (useSymmetryCheck && u > x) continue;

            // VARIANT A: Parallel (Standard)
            // Assumes directions: u->v AND x->y
            int deltaA = calculate2OptDelta(u, v, x, y, distance);
            if (deltaA < 0) {
                moves.push_back({0, u, v, x, y, -1, deltaA});
            }

            // VARIANT B: Inverted/Twisted 
            // Assumes directions: u->v AND y->x 
            // We store it as edge (y, x) so the checker looks for y->x
            int deltaB = calculate2OptDelta(u, v, y, x, distance);
            if (deltaB < 0) {
                moves.push_back({0, u, v, y, x, -1, deltaB});
            }
        }

        // 2. Inter-route Exchange
        // Replace sol[iPos] (curr) with 'node'
        for (int node = 0; node < n; ++node) {
            if (inSolution[node]) continue;
            
            int prev = sol[(iPos - 1 + sz) % sz];
            int curr = sol[iPos];
            int next = sol[(iPos + 1) % sz];
            
            int oldCost = distance[prev][curr] + distance[curr][next] + costs[curr];
            int newCost = distance[prev][node] + distance[node][next] + costs[node];
            int delta = newCost - oldCost;
            
            if (delta < 0) {
                moves.push_back({1, prev, curr, curr, next, node, delta});
            }
        }
    }
}

// --- MAIN ALGORITHM ---

std::vector<int> localSearchSteepestEdgesLM(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n
) {
    std::vector<int> sol = initialSolution;
    int sz = (int)sol.size();

    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;

    std::vector<int> nodePos(n);
    buildNodePositions(sol, nodePos);

    std::vector<LMMove> LM;
    LM.reserve(sz * sz); 

    // --- PHASE 1: INITIALIZATION ---
    // We scan ALL nodes here, so we CAN use the symmetry check to save time.
    std::vector<int> allIndices(sz);
    for(int i=0; i<sz; ++i) allIndices[i] = i;
    
    generateMovesForNodes(allIndices, sol, inSolution, distance, costs, LM, true);
    
    // Initial Sort
    std::sort(LM.begin(), LM.end());

    // --- PHASE 2: MAIN LOOP ---
    bool improved = true;
    while (improved) {
        improved = false;

        // 1. CLEANUP: Remove dead moves efficiently
        auto newEnd = std::remove_if(LM.begin(), LM.end(), 
                                     [](const LMMove& m) { return m.delta >= 0; });
        LM.erase(newEnd, LM.end());

        LMMove appliedMove;
        bool moveFound = false;

        auto it = LM.begin();
        while (it != LM.end()) {
            LMMove& m = *it;

            // Exchange Check
            if (m.type == 1 && inSolution[m.newNode]) {
                m.delta = 0; // garbage
                ++it; continue;
            }

            bool dir1, dir2;
            bool exists1 = edgeExistsUndirected(sol, nodePos, m.a1, m.b1, dir1);
            bool exists2 = edgeExistsUndirected(sol, nodePos, m.a2, m.b2, dir2);

            // REQ 1: Edges gone -> Remove
            if (!exists1 || !exists2) {
                m.delta = 0; 
                ++it; continue;
            }

            // REQ 2: Different relative direction -> Leave but don't apply
            if (dir1 != dir2) {
                ++it; continue;
            }

            // REQ 3: Same direction -> Apply
            moveFound = true;
            appliedMove = m;

            if (appliedMove.type == 0) { // 2-opt
                int u = dir1 ? nodePos[appliedMove.a1] : nodePos[appliedMove.b1];
                int v = dir2 ? nodePos[appliedMove.a2] : nodePos[appliedMove.b2];
                if (u > v) std::swap(u, v);
                std::reverse(sol.begin() + u + 1, sol.begin() + v + 1);
            } else { // Exchange
                int pos = nodePos[appliedMove.b1];
                inSolution[sol[pos]] = false;
                inSolution[appliedMove.newNode] = true;
                sol[pos] = appliedMove.newNode;
            }
            break; 
        }

        if (moveFound) {
            improved = true;
            buildNodePositions(sol, nodePos); 

            // --- UPDATE ---
            std::vector<int> touched;
            if (appliedMove.type == 0) {
                // Robustly find the new positions of the nodes involved
                // Note: We can't rely on 'appliedMove.a1' index because orientation might have flipped.
                // We rely on the node IDs.
                int p1 = nodePos[appliedMove.a1]; if (p1<0) p1=nodePos[appliedMove.b1];
                int p2 = nodePos[appliedMove.a2]; if (p2<0) p2=nodePos[appliedMove.b2];
                // Add neighbors
                touched = {p1, (p1+1)%sz, (p1-1+sz)%sz, p2, (p2+1)%sz, (p2-1+sz)%sz};
            } else {
                int pos = nodePos[appliedMove.newNode];
                touched = {pos, (pos - 1 + sz) % sz, (pos + 1) % sz};
            }

            // Generate NEW moves
            std::vector<LMMove> newMoves;
            newMoves.reserve(touched.size() * sz);
            
            // IMPORTANT: Pass 'false' for useSymmetryCheck here!
            // We must check (touched vs ALL), even if touched > other.
            generateMovesForNodes(touched, sol, inSolution, distance, costs, newMoves, false);

            std::sort(newMoves.begin(), newMoves.end());

            // Merge
            auto garbageIt = std::remove_if(LM.begin(), LM.end(), 
                                           [](const LMMove& m) { return m.delta >= 0; });
            LM.erase(garbageIt, LM.end());
            
            size_t oldSize = LM.size();
            LM.insert(LM.end(), newMoves.begin(), newMoves.end());
            std::inplace_merge(LM.begin(), LM.begin() + oldSize, LM.end());
        }
    }

    return sol;
}