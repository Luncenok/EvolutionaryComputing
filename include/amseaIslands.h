#ifndef AMSEA_ISLANDS_H
#define AMSEA_ISLANDS_H

#include <random>
#include <vector>

// Result structure for AMSEA with Island Model
struct AMSEAIslandsResult {
  std::vector<int> bestSolution;
  int bestObjective;
  double totalTime;
  int generations;
  int operatorSuccesses[3]; // CommonNodes, Parent, PathRelink
  int operatorAttempts[3];
};

// Main AMSEA Islands function
// Uses 2 islands (10+10), migration every 200 generations
AMSEAIslandsResult amseaIslands(int n, int selectCount,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, double timeLimit,
                  std::mt19937 &rng, int populationSize = 20);

#endif
