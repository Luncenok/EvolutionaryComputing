#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <random>
#include <cmath>
#include <climits>

using namespace std;

// calculate objective function: path length + sum of node costs
int calculateObjective(const vector<int>& solution, const vector<vector<int>>& distance, const vector<int>& costs) {
    int obj = 0;
    for (int i = 0; i < solution.size(); i++) {
        obj += distance[solution[i]][solution[(i + 1) % solution.size()]]; // % to loop back to the first node
        obj += costs[solution[i]];
    }
    return obj;
}

// random solution: select random 50% of nodes
vector<int> randomSolution(int n, int selectCount, mt19937& rng) {
    vector<int> nodes(n);
    for (int i = 0; i < n; i++) nodes[i] = i;
    shuffle(nodes.begin(), nodes.end(), rng);
    vector<int> solution(nodes.begin(), nodes.begin() + selectCount);
    return solution;
}

// nearest neighbor - add at end only
vector<int> nearestNeighborEnd(int startNode, int selectCount, const vector<vector<int>>& distance, const vector<int>& costs) {
    vector<int> solution;
    vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    while (solution.size() < selectCount) {
        int lastNode = solution.back();
        int bestNode = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[lastNode][i] + costs[i];
            if (delta < bestDelta) {
                bestDelta = delta;
                bestNode = i;
            }
        }
        
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}

// nearest neighbor - add at any position
vector<int> nearestNeighborAny(int startNode, int selectCount, const vector<vector<int>>& distance, const vector<int>& costs) {
    vector<int> solution;
    vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    while (solution.size() < selectCount) {
        int bestNode = -1;
        int bestPos = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            
            // try inserting at each position
            for (int pos = 0; pos <= solution.size(); pos++) {
                int delta = costs[i];
                if (pos == 0) {
                    // insert at beginning
                    delta += distance[i][solution[0]];
                } else if (pos == solution.size()) {
                    // insert at end
                    delta += distance[solution.back()][i];
                } else {
                    // insert in middle
                    delta += distance[solution[pos-1]][i] + distance[i][solution[pos]] - distance[solution[pos-1]][solution[pos]];
                }
                
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestNode = i;
                    bestPos = pos;
                }
            }
        }
        
        solution.insert(solution.begin() + bestPos, bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}

// greedy cycle
vector<int> greedyCycle(int startNode, int selectCount, const vector<vector<int>>& distance, const vector<int>& costs) {
    vector<int> solution;
    vector<bool> selected(distance.size(), false);
    
    solution.push_back(startNode);
    selected[startNode] = true;
    
    // find nearest node to start
    if (selectCount > 1) {
        int bestNode = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[startNode][i] + costs[i];
            if (delta < bestDist) {
                bestDist = delta;
                bestNode = i;
            }
        }
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    
    while (solution.size() < selectCount) {
        int bestNode = -1;
        int bestPos = -1;
        int bestDelta = INT_MAX;
        
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            
            // try inserting between each pair of adjacent nodes in cycle
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] 
                           - distance[solution[pos]][solution[next]] + costs[i];
                
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestNode = i;
                    bestPos = pos + 1;
                }
            }
        }
        
        solution.insert(solution.begin() + bestPos, bestNode);
        selected[bestNode] = true;
    }
    
    return solution;
}

vector<int> greedyRegret2(int startNode, int selectCount, const vector<vector<int>>& distance, const vector<int>& costs) {
    vector<int> solution;
    vector<bool> selected(distance.size(), false);
    solution.push_back(startNode);
    selected[startNode] = true;
    if (selectCount > 1) {
        int bestNode = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[startNode][i] + costs[i];
            if (delta < bestDist) { bestDist = delta; bestNode = i; }
        }
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    while (solution.size() < selectCount) {
        int chooseNode = -1;
        int choosePos = -1;
        int bestRegret = -1;
        int tieBestDelta = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int best1 = INT_MAX, best2 = INT_MAX;
            int bestPos = -1;
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] - distance[solution[pos]][solution[next]] + costs[i];
                if (delta < best1) {
                    best2 = best1;
                    best1 = delta;
                    bestPos = pos + 1;
                } else if (delta < best2) {
                    best2 = delta;
                }
            }
            int regret = (best2 == INT_MAX ? 0 : (best2 - best1));
            if (regret > bestRegret || (regret == bestRegret && best1 < tieBestDelta)) {
                bestRegret = regret;
                tieBestDelta = best1;
                chooseNode = i;
                choosePos = bestPos;
            }
        }
        solution.insert(solution.begin() + choosePos, chooseNode);
        selected[chooseNode] = true;
    }
    return solution;
}

vector<int> greedyRegret2Weighted(int startNode, int selectCount, const vector<vector<int>>& distance, const vector<int>& costs, double wRegret = 1.0, double wBest = 1.0) {
    vector<int> solution;
    vector<bool> selected(distance.size(), false);
    solution.push_back(startNode);
    selected[startNode] = true;
    if (selectCount > 1) {
        int bestNode = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int delta = distance[startNode][i] + costs[i];
            if (delta < bestDist) { bestDist = delta; bestNode = i; }
        }
        solution.push_back(bestNode);
        selected[bestNode] = true;
    }
    while (solution.size() < selectCount) {
        int chooseNode = -1;
        int choosePos = -1;
        double bestScore = -1e300;
        int tieBestDelta = INT_MAX;
        int tieBestRegret = -1;
        for (int i = 0; i < distance.size(); i++) {
            if (selected[i]) continue;
            int best1 = INT_MAX, best2 = INT_MAX;
            int bestPos = -1;
            for (int pos = 0; pos < solution.size(); pos++) {
                int next = (pos + 1) % solution.size();
                int delta = distance[solution[pos]][i] + distance[i][solution[next]] - distance[solution[pos]][solution[next]] + costs[i];
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
            if (score > bestScore || (abs(score - bestScore) < 1e-12 && (best1 < tieBestDelta || (best1 == tieBestDelta && regret > tieBestRegret)))) {
                bestScore = score;
                tieBestDelta = best1;
                tieBestRegret = regret;
                chooseNode = i;
                choosePos = bestPos;
            }
        }
        solution.insert(solution.begin() + choosePos, chooseNode);
        selected[chooseNode] = true;
    }
    return solution;
}

void process(const string& filename) {
    // variables
    vector<tuple<int, int, int>> table; // x,y,cost
    
    // input - handle semicolon separator
    ifstream fin(filename);
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        replace(line.begin(), line.end(), ';', ' ');
        istringstream iss(line);
        int x, y, cost;
        if (iss >> x >> y >> cost) {
            table.push_back(make_tuple(x, y, cost));
        }
    }
    fin.close();
    
    int n = table.size();
    int selectCount = (n + 1) / 2; // round up
    
    // distance matrix (euclidean rounded to int)
    vector<vector<int>> distance(n, vector<int>(n));
    vector<int> costs(n);
    
    for (int i = 0; i < n; i++) {
        costs[i] = get<2>(table[i]);
        for (int j = 0; j < n; j++) {
            double dx = get<0>(table[i]) - get<0>(table[j]);
            double dy = get<1>(table[i]) - get<1>(table[j]);
            distance[i][j] = round(sqrt(dx * dx + dy * dy));
        }
    }
    
    cout << "\n=-=-= " << filename << " =-=-=\n";
    cout << "Nodes: " << n << ", Selecting: " << selectCount << "\n\n";
    
    mt19937 rng(12345);
    
    // Random solutions
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        
        for (int i = 0; i < 200; i++) {
            auto sol = randomSolution(n, selectCount, rng);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        
        cout << "Random:\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / 200 << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }
    
    // Nearest neighbor (end only)
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        
        for (int start = 0; start < n; start++) {
            auto sol = nearestNeighborEnd(start, selectCount, distance, costs);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        
        cout << "Nearest Neighbor (end only):\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / n << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }
    
    // Nearest neighbor (any position)
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        
        for (int start = 0; start < n; start++) {
            auto sol = nearestNeighborAny(start, selectCount, distance, costs);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        
        cout << "Nearest Neighbor (any position):\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / n << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }
    
    // Greedy cycle
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        
        for (int start = 0; start < n; start++) {
            auto sol = greedyCycle(start, selectCount, distance, costs);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        
        cout << "Greedy Cycle:\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / n << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }

    // Greedy 2-regret (cycle insertion)
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        for (int start = 0; start < n; start++) {
            auto sol = greedyRegret2(start, selectCount, distance, costs);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        cout << "Greedy 2-Regret:\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / n << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }

    // Greedy weighted (2-regret + best delta), equal weights by default
    {
        int minObj = INT_MAX, maxObj = 0, sumObj = 0;
        vector<int> bestSol;
        double wRegret = 1.0, wBest = 1.0;
        for (int start = 0; start < n; start++) {
            auto sol = greedyRegret2Weighted(start, selectCount, distance, costs, wRegret, wBest);
            int obj = calculateObjective(sol, distance, costs);
            sumObj += obj;
            if (obj < minObj) { minObj = obj; bestSol = sol; }
            if (obj > maxObj) maxObj = obj;
        }
        cout << "Greedy Weighted (2-Regret + BestDelta):\n";
        cout << "  Min: " << minObj << ", Max: " << maxObj << ", Avg: " << sumObj / n << "\n";
        cout << "  Best: ";
        for (int node : bestSol) cout << node << " ";
        cout << "\n\n";
    }
}

int main() {
    // io optimizations
    ios_base::sync_with_stdio(false);
    cin.tie(0);
    cout.tie(0);
    
    process("TSPA.csv");
    process("TSPB.csv");
    
    return 0;
}
