#include "../include/hybridEvolutionaryAlgorithm.h"
#include "../include/calculateObjective.h"
#include "../include/localSearch.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <map>
#include <numeric>
#include <set>
#include <unordered_set>

// Forward declaration for repair function (reused from LNS concept)
std::vector<int>
repairSolutionHEA(const std::vector<int> &partial,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, int n, int selectCount,
                  double wRegret, double wBest);

// ============================================================================
// Population Management Functions
// ============================================================================

// Initialize population with random solutions + local search
std::vector<std::vector<int>> initializePopulation(
    int n, int selectCount, const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, std::mt19937 &rng, int populationSize) {
  std::vector<std::vector<int>> population;
  std::set<int> objectivesSeen; // For duplicate detection

  int attempts = 0;
  int maxAttempts = populationSize * 10; // Prevent infinite loop

  while (population.size() < populationSize && attempts < maxAttempts) {
    attempts++;

    // Generate random solution
    std::vector<int> allNodes(n);
    std::iota(allNodes.begin(), allNodes.end(), 0);
    std::shuffle(allNodes.begin(), allNodes.end(), rng);
    std::vector<int> solution(allNodes.begin(), allNodes.begin() + selectCount);

    // Apply local search
    solution = localSearchSteepestEdges(solution, distance, costs, n);

    // Check for duplicate (using objective value)
    int obj = calculateObjective(solution, distance, costs);
    if (objectivesSeen.find(obj) == objectivesSeen.end()) {
      objectivesSeen.insert(obj);
      population.push_back(solution);
    }
  }

  return population;
}

// Check if solution is duplicate in population
bool isDuplicate(const std::vector<int> &solution,
                 const std::vector<std::vector<int>> &population,
                 const std::vector<std::vector<int>> &distance,
                 const std::vector<int> &costs) {
  int obj = calculateObjective(solution, distance, costs);
  for (const auto &member : population) {
    if (calculateObjective(member, distance, costs) == obj) {
      return true;
    }
  }
  return false;
}

// Select two distinct parents uniformly at random
std::pair<int, int> selectParents(int populationSize, std::mt19937 &rng) {
  std::uniform_int_distribution<> dist(0, populationSize - 1);
  int p1 = dist(rng);
  int p2 = dist(rng);
  while (p2 == p1) {
    p2 = dist(rng);
  }
  return {p1, p2};
}

// Find worst solution in population, return its index
int findWorstIndex(const std::vector<std::vector<int>> &population,
                   const std::vector<std::vector<int>> &distance,
                   const std::vector<int> &costs) {
  int worstIdx = 0;
  int worstObj = calculateObjective(population[0], distance, costs);
  for (int i = 1; i < population.size(); i++) {
    int obj = calculateObjective(population[i], distance, costs);
    if (obj > worstObj) {
      worstObj = obj;
      worstIdx = i;
    }
  }
  return worstIdx;
}

// ============================================================================
// Recombination Operator 1: Common Nodes and Edges
// ============================================================================

std::vector<int> recombineOperator1(const std::vector<int> &parent1,
                                    const std::vector<int> &parent2, int n,
                                    int selectCount, std::mt19937 &rng) {
  int solSize = parent1.size();

  // Find common nodes
  std::unordered_set<int> nodes1(parent1.begin(), parent1.end());
  std::unordered_set<int> nodes2(parent2.begin(), parent2.end());
  std::unordered_set<int> commonNodes;
  for (int node : nodes1) {
    if (nodes2.count(node)) {
      commonNodes.insert(node);
    }
  }

  // Find common edges (bidirectional)
  // Edge represented as pair with smaller node first
  auto makeEdge = [](int a, int b) {
    return std::make_pair(std::min(a, b), std::max(a, b));
  };

  std::set<std::pair<int, int>> edges1, edges2;
  for (int i = 0; i < solSize; i++) {
    int next = (i + 1) % solSize;
    edges1.insert(makeEdge(parent1[i], parent1[next]));
    edges2.insert(makeEdge(parent2[i], parent2[next]));
  }

  std::set<std::pair<int, int>> commonEdges;
  for (const auto &edge : edges1) {
    if (edges2.count(edge)) {
      commonEdges.insert(edge);
    }
  }

  // Build adjacency list from common edges
  std::map<int, std::vector<int>> adj;
  for (const auto &edge : commonEdges) {
    adj[edge.first].push_back(edge.second);
    adj[edge.second].push_back(edge.first);
  }

  // Build subpaths from common edges
  std::vector<std::vector<int>> subpaths;
  std::unordered_set<int> visited;

  for (int node : commonNodes) {
    if (visited.count(node))
      continue;

    // Start a new subpath from this node
    std::vector<int> path;
    path.push_back(node);
    visited.insert(node);

    // Extend in one direction
    int current = node;
    while (true) {
      auto it = adj.find(current);
      if (it == adj.end())
        break;

      int next = -1;
      for (int neighbor : it->second) {
        if (!visited.count(neighbor) && commonNodes.count(neighbor)) {
          next = neighbor;
          break;
        }
      }
      if (next == -1)
        break;

      path.push_back(next);
      visited.insert(next);
      current = next;
    }

    // Extend in the other direction (prepend to path)
    current = node;
    while (true) {
      auto it = adj.find(current);
      if (it == adj.end())
        break;

      int prev = -1;
      for (int neighbor : it->second) {
        if (!visited.count(neighbor) && commonNodes.count(neighbor)) {
          prev = neighbor;
          break;
        }
      }
      if (prev == -1)
        break;

      path.insert(path.begin(), prev);
      visited.insert(prev);
      current = prev;
    }

    subpaths.push_back(path);
  }

  // Count total nodes so far
  int totalNodes = 0;
  for (const auto &path : subpaths) {
    totalNodes += path.size();
  }

  // Add random nodes to reach selectCount
  std::vector<int> unselected;
  for (int i = 0; i < n; i++) {
    if (!visited.count(i)) {
      unselected.push_back(i);
    }
  }
  std::shuffle(unselected.begin(), unselected.end(), rng);

  int toAdd = selectCount - totalNodes;
  for (int i = 0; i < toAdd && i < unselected.size(); i++) {
    // Each random node becomes its own single-node subpath
    subpaths.push_back({unselected[i]});
  }

  // Shuffle subpaths order
  std::shuffle(subpaths.begin(), subpaths.end(), rng);

  // Connect subpaths with random direction
  std::vector<int> offspring;
  std::uniform_int_distribution<> coinFlip(0, 1);
  for (auto &path : subpaths) {
    if (coinFlip(rng)) {
      std::reverse(path.begin(), path.end());
    }
    for (int node : path) {
      offspring.push_back(node);
    }
  }

  return offspring;
}

// ============================================================================
// Recombination Operator 2: Parent-based with LNS Repair
// ============================================================================

std::vector<int>
recombineOperator2(const std::vector<int> &parent1,
                   const std::vector<int> &parent2, int n, int selectCount,
                   const std::vector<std::vector<int>> &distance,
                   const std::vector<int> &costs) {
  // Find nodes present in both parents
  std::unordered_set<int> nodes2(parent2.begin(), parent2.end());

  // Start with parent1, keep only nodes also in parent2 (preserve order)
  std::vector<int> partial;
  for (int node : parent1) {
    if (nodes2.count(node)) {
      partial.push_back(node);
    }
  }

  // Repair using weighted 2-regret (same as LNS repair)
  double wRegret = 1.0, wBest = 1.0;
  return repairSolutionHEA(partial, distance, costs, n, selectCount, wRegret,
                           wBest);
}

// Repair function (adapted from LNS repairSolution)
std::vector<int>
repairSolutionHEA(const std::vector<int> &partial,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, int n, int selectCount,
                  double wRegret, double wBest) {
  std::vector<int> solution = partial;

  // Mark nodes already in solution
  std::vector<bool> selected(n, false);
  for (int node : solution) {
    selected[node] = true;
  }

  // If solution is empty or has only one node, start fresh
  if (solution.size() < 2) {
    // Find best starting node among unselected
    int bestStart = -1;
    int bestCost = INT_MAX;
    for (int i = 0; i < n; i++) {
      if (!selected[i] && costs[i] < bestCost) {
        bestCost = costs[i];
        bestStart = i;
      }
    }
    if (solution.empty()) {
      solution.push_back(bestStart);
      selected[bestStart] = true;
    }

    // Add second node
    if (solution.size() == 1 && selectCount > 1) {
      int startNode = solution[0];
      int bestNode = -1;
      int bestDelta = INT_MAX;
      for (int i = 0; i < n; i++) {
        if (selected[i])
          continue;
        int delta = distance[startNode][i] + costs[i];
        if (delta < bestDelta) {
          bestDelta = delta;
          bestNode = i;
        }
      }
      if (bestNode != -1) {
        solution.push_back(bestNode);
        selected[bestNode] = true;
      }
    }
  }

  // Use weighted 2-regret to insert remaining nodes
  const double EPSILON = 1e-9;
  const double INIT_SCORE = -1e18;

  while (solution.size() < selectCount) {
    int chooseNode = -1;
    int choosePos = -1;
    double bestScore = INIT_SCORE;
    int tieBestDelta = INT_MAX;
    int tieBestRegret = -1;

    for (int i = 0; i < n; i++) {
      if (selected[i])
        continue;

      int best1 = INT_MAX, best2 = INT_MAX;
      int bestPos = -1;

      for (int pos = 0; pos < solution.size(); pos++) {
        int next = (pos + 1) % solution.size();
        int delta = distance[solution[pos]][i] + distance[i][solution[next]] -
                    distance[solution[pos]][solution[next]] + costs[i];

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

      if (score > bestScore ||
          (std::abs(score - bestScore) < EPSILON &&
           (best1 < tieBestDelta ||
            (best1 == tieBestDelta && regret > tieBestRegret)))) {
        bestScore = score;
        tieBestDelta = best1;
        tieBestRegret = regret;
        chooseNode = i;
        choosePos = bestPos;
      }
    }

    if (chooseNode == -1)
      break; // No valid node found

    solution.insert(solution.begin() + choosePos, chooseNode);
    selected[chooseNode] = true;
  }

  return solution;
}

// ============================================================================
// Main HEA Functions
// ============================================================================

HEAResult hybridEAOperator1(int n, int selectCount,
                            const std::vector<std::vector<int>> &distance,
                            const std::vector<int> &costs, double timeLimit,
                            std::mt19937 &rng, int populationSize) {
  HEAResult result;
  result.bestObjective = INT_MAX;
  result.generations = 0;

  auto startTime = std::chrono::high_resolution_clock::now();

  // Initialize population
  auto population = initializePopulation(n, selectCount, distance, costs, rng,
                                         populationSize);

  // Find initial best
  for (const auto &sol : population) {
    int obj = calculateObjective(sol, distance, costs);
    if (obj < result.bestObjective) {
      result.bestObjective = obj;
      result.bestSolution = sol;
    }
  }

  // Main loop
  while (true) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(currentTime - startTime)
            .count();
    if (elapsed >= timeLimit)
      break;

    result.generations++;

    // Select parents
    auto [p1, p2] = selectParents(population.size(), rng);

    // Recombine using Operator 1
    std::vector<int> offspring =
        recombineOperator1(population[p1], population[p2], n, selectCount, rng);

    // Apply local search
    offspring = localSearchSteepestEdges(offspring, distance, costs, n);
    int offspringObj = calculateObjective(offspring, distance, costs);

    // Check for duplicate
    if (!isDuplicate(offspring, population, distance, costs)) {
      // Find worst in population
      int worstIdx = findWorstIndex(population, distance, costs);
      int worstObj = calculateObjective(population[worstIdx], distance, costs);

      // Replace if offspring is better than worst
      if (offspringObj < worstObj) {
        population[worstIdx] = offspring;
      }
    }

    // Update best
    if (offspringObj < result.bestObjective) {
      result.bestObjective = offspringObj;
      result.bestSolution = offspring;
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  result.totalTime =
      std::chrono::duration<double, std::milli>(endTime - startTime).count();

  return result;
}

HEAResult hybridEAOperator2WithLS(int n, int selectCount,
                                  const std::vector<std::vector<int>> &distance,
                                  const std::vector<int> &costs,
                                  double timeLimit, std::mt19937 &rng,
                                  int populationSize) {
  HEAResult result;
  result.bestObjective = INT_MAX;
  result.generations = 0;

  auto startTime = std::chrono::high_resolution_clock::now();

  // Initialize population
  auto population = initializePopulation(n, selectCount, distance, costs, rng,
                                         populationSize);

  // Find initial best
  for (const auto &sol : population) {
    int obj = calculateObjective(sol, distance, costs);
    if (obj < result.bestObjective) {
      result.bestObjective = obj;
      result.bestSolution = sol;
    }
  }

  // Main loop
  while (true) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(currentTime - startTime)
            .count();
    if (elapsed >= timeLimit)
      break;

    result.generations++;

    // Select parents
    auto [p1, p2] = selectParents(population.size(), rng);

    // Recombine using Operator 2
    std::vector<int> offspring = recombineOperator2(
        population[p1], population[p2], n, selectCount, distance, costs);

    // Apply local search
    offspring = localSearchSteepestEdges(offspring, distance, costs, n);
    int offspringObj = calculateObjective(offspring, distance, costs);

    // Check for duplicate
    if (!isDuplicate(offspring, population, distance, costs)) {
      // Find worst in population
      int worstIdx = findWorstIndex(population, distance, costs);
      int worstObj = calculateObjective(population[worstIdx], distance, costs);

      // Replace if offspring is better than worst
      if (offspringObj < worstObj) {
        population[worstIdx] = offspring;
      }
    }

    // Update best
    if (offspringObj < result.bestObjective) {
      result.bestObjective = offspringObj;
      result.bestSolution = offspring;
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  result.totalTime =
      std::chrono::duration<double, std::milli>(endTime - startTime).count();

  return result;
}

HEAResult hybridEAOperator2NoLS(int n, int selectCount,
                                const std::vector<std::vector<int>> &distance,
                                const std::vector<int> &costs, double timeLimit,
                                std::mt19937 &rng, int populationSize) {
  HEAResult result;
  result.bestObjective = INT_MAX;
  result.generations = 0;

  auto startTime = std::chrono::high_resolution_clock::now();

  // Initialize population (still uses LS for initial population)
  auto population = initializePopulation(n, selectCount, distance, costs, rng,
                                         populationSize);

  // Find initial best
  for (const auto &sol : population) {
    int obj = calculateObjective(sol, distance, costs);
    if (obj < result.bestObjective) {
      result.bestObjective = obj;
      result.bestSolution = sol;
    }
  }

  // Main loop
  while (true) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(currentTime - startTime)
            .count();
    if (elapsed >= timeLimit)
      break;

    result.generations++;

    // Select parents
    auto [p1, p2] = selectParents(population.size(), rng);

    // Recombine using Operator 2 (NO local search after)
    std::vector<int> offspring = recombineOperator2(
        population[p1], population[p2], n, selectCount, distance, costs);
    int offspringObj = calculateObjective(offspring, distance, costs);

    // Check for duplicate
    if (!isDuplicate(offspring, population, distance, costs)) {
      // Find worst in population
      int worstIdx = findWorstIndex(population, distance, costs);
      int worstObj = calculateObjective(population[worstIdx], distance, costs);

      // Replace if offspring is better than worst
      if (offspringObj < worstObj) {
        population[worstIdx] = offspring;
      }
    }

    // Update best
    if (offspringObj < result.bestObjective) {
      result.bestObjective = offspringObj;
      result.bestSolution = offspring;
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  result.totalTime =
      std::chrono::duration<double, std::milli>(endTime - startTime).count();

  return result;
}
