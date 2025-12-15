#ifndef HYBRID_EVOLUTIONARY_ALGORITHM_H
#define HYBRID_EVOLUTIONARY_ALGORITHM_H

#include <random>
#include <vector>

struct HEAResult {
  std::vector<int> bestSolution;
  int bestObjective;
  double totalTime;
  int generations; // Number of offspring created
};

// Hybrid EA with Operator 1: Common nodes/edges recombination with local search
HEAResult hybridEAOperator1(int n, int selectCount,
                            const std::vector<std::vector<int>> &distance,
                            const std::vector<int> &costs, double timeLimit,
                            std::mt19937 &rng, int populationSize = 20);

// Hybrid EA with Operator 2: Parent-based removal + LNS repair with local
// search
HEAResult hybridEAOperator2WithLS(int n, int selectCount,
                                  const std::vector<std::vector<int>> &distance,
                                  const std::vector<int> &costs,
                                  double timeLimit, std::mt19937 &rng,
                                  int populationSize = 20);

// Hybrid EA with Operator 2: Parent-based removal + LNS repair without local
// search
HEAResult hybridEAOperator2NoLS(int n, int selectCount,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs, double timeLimit,
                                std::mt19937 &rng, int populationSize = 20);

#endif
