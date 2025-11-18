#include "../include/localSearch.h"
#include "../include/candidateMoves.h"
#include <vector>
#include <algorithm>

struct LMMoveCandidates {
    int type;
    int a1, b1;
    int a2, b2;
    int newNode;
    int delta;

    bool operator<(const LMMoveCandidates& other) const {
        if (delta != other.delta) return delta < other.delta;
        if (type != other.type) return type < other.type;
        return a1 < other.a1;
    }
};

static void buildNodePositionsCandidates(const std::vector<int>& sol, std::vector<int>& nodePos) {
    std::fill(nodePos.begin(), nodePos.end(), -1);
    for (int i = 0; i < (int)sol.size(); ++i) {
        nodePos[sol[i]] = i;
    }
}

static bool edgeExistsUndirectedCandidates(const std::vector<int>& sol,
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

static int calculate2OptDeltaCandidates(int u, int v, int x, int y,
                                        const std::vector<std::vector<int>>& dist) {
    return (dist[u][x] + dist[v][y]) - (dist[u][v] + dist[x][y]);
}

static bool isCandidateEdge(int node1, int node2,
                            const std::vector<std::vector<int>>& nearestNeighbors) {
    const auto& nn1 = nearestNeighbors[node1];
    if (std::find(nn1.begin(), nn1.end(), node2) != nn1.end()) return true;
    const auto& nn2 = nearestNeighbors[node2];
    if (std::find(nn2.begin(), nn2.end(), node1) != nn2.end()) return true;
    return false;
}

static void generateMovesForNodesCandidates(
    const std::vector<int>& nodesToScan,
    const std::vector<int>& sol,
    const std::vector<bool>& inSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<std::vector<int>>& nearestNeighbors,
    const std::vector<int>& nodePos,
    std::vector<LMMoveCandidates>& moves,
    bool useSymmetryCheck
) {
    int sz = (int)sol.size();
    int n = (int)inSolution.size();

    for (int iPos : nodesToScan) {
        int u = sol[iPos];
        int v = sol[(iPos + 1) % sz];

        for (int j = 0; j < sz; ++j) {
            if (iPos == j || (iPos + 1) % sz == j || (j + 1) % sz == iPos) continue;

            int x = sol[j];
            int y = sol[(j + 1) % sz];

            if (useSymmetryCheck && u > x) continue;

            bool candidateA = isCandidateEdge(u, x, nearestNeighbors) ||
                              isCandidateEdge(v, y, nearestNeighbors);
            if (candidateA) {
                int deltaA = calculate2OptDeltaCandidates(u, v, x, y, distance);
                if (deltaA < 0) {
                    moves.push_back({0, u, v, x, y, -1, deltaA});
                }
            }

            bool candidateB = isCandidateEdge(u, y, nearestNeighbors) ||
                              isCandidateEdge(v, x, nearestNeighbors);
            if (candidateB) {
                int deltaB = calculate2OptDeltaCandidates(u, v, y, x, distance);
                if (deltaB < 0) {
                    moves.push_back({0, u, v, y, x, -1, deltaB});
                }
            }
        }

        int prev = sol[(iPos - 1 + sz) % sz];
        int curr = sol[iPos];
        int next = sol[(iPos + 1) % sz];

        for (int node = 0; node < n; ++node) {
            if (inSolution[node]) continue;

            bool candidateEdge1 = isCandidateEdge(prev, node, nearestNeighbors);
            bool candidateEdge2 = isCandidateEdge(node, next, nearestNeighbors);
            if (!candidateEdge1 && !candidateEdge2) continue;

            int oldCost = distance[prev][curr] + distance[curr][next] + costs[curr];
            int newCost = distance[prev][node] + distance[node][next] + costs[node];
            int delta = newCost - oldCost;

            if (delta < 0) {
                moves.push_back({1, prev, curr, curr, next, node, delta});
            }
        }
    }
}

std::vector<int> localSearchSteepestEdgesLMCandidates(
    const std::vector<int>& initialSolution,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    int n,
    int k
) {
    std::vector<int> sol = initialSolution;
    int sz = (int)sol.size();

    std::vector<bool> inSolution(n, false);
    for (int node : sol) inSolution[node] = true;

    auto nearestNeighbors = buildNearestNeighbors(n, distance, costs, k);

    std::vector<int> nodePos(n);
    buildNodePositionsCandidates(sol, nodePos);

    std::vector<LMMoveCandidates> LM;
    LM.reserve(sz * sz);

    std::vector<int> allIndices(sz);
    for (int i = 0; i < sz; ++i) allIndices[i] = i;

    generateMovesForNodesCandidates(allIndices, sol, inSolution, distance, costs,
                                    nearestNeighbors, nodePos, LM, true);
    std::sort(LM.begin(), LM.end());

    bool improved = true;
    while (improved) {
        improved = false;

        auto newEnd = std::remove_if(LM.begin(), LM.end(),
                                     [](const LMMoveCandidates& m) { return m.delta >= 0; });
        LM.erase(newEnd, LM.end());

        LMMoveCandidates appliedMove;
        bool moveFound = false;

        auto it = LM.begin();
        while (it != LM.end()) {
            LMMoveCandidates& m = *it;

            if (m.type == 1 && inSolution[m.newNode]) {
                m.delta = 0;
                ++it;
                continue;
            }

            bool dir1, dir2;
            bool exists1 = edgeExistsUndirectedCandidates(sol, nodePos, m.a1, m.b1, dir1);
            bool exists2 = edgeExistsUndirectedCandidates(sol, nodePos, m.a2, m.b2, dir2);

            if (!exists1 || !exists2) {
                m.delta = 0;
                ++it;
                continue;
            }

            if (dir1 != dir2) {
                ++it;
                continue;
            }

            moveFound = true;
            appliedMove = m;

            if (appliedMove.type == 0) {
                int uPos = dir1 ? nodePos[appliedMove.a1] : nodePos[appliedMove.b1];
                int vPos = dir2 ? nodePos[appliedMove.a2] : nodePos[appliedMove.b2];
                if (uPos > vPos) std::swap(uPos, vPos);
                std::reverse(sol.begin() + uPos + 1, sol.begin() + vPos + 1);
            } else {
                int pos = nodePos[appliedMove.b1];
                inSolution[sol[pos]] = false;
                inSolution[appliedMove.newNode] = true;
                sol[pos] = appliedMove.newNode;
            }
            break;
        }

        if (moveFound) {
            improved = true;
            buildNodePositionsCandidates(sol, nodePos);

            std::vector<int> touched;
            if (appliedMove.type == 0) {
                int p1 = nodePos[appliedMove.a1]; if (p1 < 0) p1 = nodePos[appliedMove.b1];
                int p2 = nodePos[appliedMove.a2]; if (p2 < 0) p2 = nodePos[appliedMove.b2];
                touched = {p1, (p1 + 1) % sz, (p1 - 1 + sz) % sz,
                           p2, (p2 + 1) % sz, (p2 - 1 + sz) % sz};
            } else {
                int pos = nodePos[appliedMove.newNode];
                touched = {pos, (pos - 1 + sz) % sz, (pos + 1) % sz};
            }

            std::vector<LMMoveCandidates> newMoves;
            newMoves.reserve(touched.size() * sz);

            generateMovesForNodesCandidates(touched, sol, inSolution, distance, costs,
                                            nearestNeighbors, nodePos, newMoves, false);
            std::sort(newMoves.begin(), newMoves.end());

            auto garbageIt = std::remove_if(LM.begin(), LM.end(),
                                            [](const LMMoveCandidates& m) { return m.delta >= 0; });
            LM.erase(garbageIt, LM.end());

            size_t oldSize = LM.size();
            LM.insert(LM.end(), newMoves.begin(), newMoves.end());
            std::inplace_merge(LM.begin(), LM.begin() + oldSize, LM.end());
        }
    }

    return sol;
}
