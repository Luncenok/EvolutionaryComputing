#include "../include/randomSolution.h"
#include <algorithm>

std::vector<int> randomSolution(int startNode, int n, int selectCount, std::mt19937& rng) {
    std::vector<int> solution;
    solution.push_back(startNode);
    
    // Add all other nodes except startNode
    std::vector<int> otherNodes;
    for (int i = 0; i < n; i++) {
        if (i != startNode) {
            otherNodes.push_back(i);
        }
    }
    
    // Shuffle the other nodes
    std::shuffle(otherNodes.begin(), otherNodes.end(), rng);
    
    // Add nodes until we reach selectCount
    for (int i = 0; i < selectCount - 1 && i < otherNodes.size(); i++) {
        solution.push_back(otherNodes[i]);
    }
    
    return solution;
}
