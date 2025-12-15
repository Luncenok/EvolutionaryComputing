#ifndef AMSEA_H
#define AMSEA_H

#include <random>
#include <vector>

// Result structure for Adaptive Multi-Strategy Evolutionary Algorithm
struct AMSEAResult {
  std::vector<int> bestSolution;
  int bestObjective;
  double totalTime;
  int generations;
  int operatorSuccesses[3]; // Track which operators were most successful
  int operatorAttempts[3];
};

// Main AMSEA function
AMSEAResult amsea(int n, int selectCount,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, double timeLimit,
                  std::mt19937 &rng, int populationSize = 20);

#endif
