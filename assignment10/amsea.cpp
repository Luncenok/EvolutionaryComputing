#include "../include/amsea.h"
#include "../include/calculateObjective.h"
#include "../include/greedyCycle.h"
#include "../include/greedyRegret2Weighted.h"
#include "../include/localSearch.h"
#include "../include/nearestNeighborAny.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <unordered_set>

// ============================================================================
// Forward Declarations
// ============================================================================

std::vector<int>
repairSolutionAMSEA(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    double wRegret, double wBest);

// ============================================================================
// Population Initialization - Greedy with Diversity
// ============================================================================

std::vector<std::vector<int>> initializePopulationGreedy(
    int n, int selectCount, const std::vector<std::vector<int>> &distance,
    const std::vector<int> &costs, std::mt19937 &rng, int populationSize) {
  std::vector<std::vector<int>> population;
  std::set<int> objectivesSeen; // For duplicate detection

  // Function to add a solution if it's unique
  auto addIfUnique = [&](const std::vector<int> &solution) {
    if (population.size() >= populationSize)
      return;

    // Apply local search
    std::vector<int> improved =
        localSearchSteepestEdges(solution, distance, costs, n);
    int obj = calculateObjective(improved, distance, costs);

    if (objectivesSeen.find(obj) == objectivesSeen.end()) {
      objectivesSeen.insert(obj);
      population.push_back(improved);
    }
  };

  // Strategy 1: Use different greedy heuristics from different starting nodes
  // Spread starting nodes evenly across the instance
  std::vector<int> startNodes(n);
  std::iota(startNodes.begin(), startNodes.end(), 0);
  std::shuffle(startNodes.begin(), startNodes.end(), rng);

  // Use first few starting nodes with each heuristic
  int nodesPerHeuristic = (populationSize / 3) + 1;

  // Greedy Cycle heuristic
  for (int i = 0; i < nodesPerHeuristic && population.size() < populationSize;
       i++) {
    int start = startNodes[i % n];
    std::vector<int> solution =
        greedyCycle(start, selectCount, distance, costs);
    addIfUnique(solution);
  }

  // Nearest Neighbor Any heuristic
  for (int i = 0; i < nodesPerHeuristic && population.size() < populationSize;
       i++) {
    int start = startNodes[(i + nodesPerHeuristic) % n];
    std::vector<int> solution =
        nearestNeighborAny(start, selectCount, distance, costs);
    addIfUnique(solution);
  }

  // Weighted 2-Regret heuristic
  double wRegret = 1.0, wBest = 1.0;
  for (int i = 0; i < nodesPerHeuristic && population.size() < populationSize;
       i++) {
    int start = startNodes[(i + 2 * nodesPerHeuristic) % n];
    std::vector<int> solution = greedyRegret2Weighted(
        start, selectCount, distance, costs, wRegret, wBest);
    addIfUnique(solution);
  }

  // Fill remaining slots with random solutions + LS
  int attempts = 0;
  int maxAttempts = populationSize * 10;

  while (population.size() < populationSize && attempts < maxAttempts) {
    attempts++;

    std::vector<int> allNodes(n);
    std::iota(allNodes.begin(), allNodes.end(), 0);
    std::shuffle(allNodes.begin(), allNodes.end(), rng);
    std::vector<int> solution(allNodes.begin(), allNodes.begin() + selectCount);

    addIfUnique(solution);
  }

  return population;
}

// ============================================================================
// Duplicate Detection
// ============================================================================

bool isDuplicateAMSEA(const std::vector<int> &solution,
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

// ============================================================================
// Parent Selection
// ============================================================================

std::pair<int, int> selectParentsRandom(int populationSize, std::mt19937 &rng) {
  std::uniform_int_distribution<> dist(0, populationSize - 1);
  int p1 = dist(rng);
  int p2 = dist(rng);
  while (p2 == p1) {
    p2 = dist(rng);
  }
  return {p1, p2};
}

// ============================================================================
// Population Management
// ============================================================================

int findWorstIndexAMSEA(const std::vector<std::vector<int>> &population,
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

int getWorstObjective(const std::vector<std::vector<int>> &population,
                      const std::vector<std::vector<int>> &distance,
                      const std::vector<int> &costs) {
  int worstIdx = findWorstIndexAMSEA(population, distance, costs);
  return calculateObjective(population[worstIdx], distance, costs);
}

// ============================================================================
// Operator 1: Common Nodes and Edges (from HEA)
// ============================================================================

std::vector<int> recombineOp1(const std::vector<int> &parent1,
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

    // Extend in the other direction
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

  // Count total nodes
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
    subpaths.push_back({unselected[i]});
  }

  // Shuffle subpaths order and direction
  std::shuffle(subpaths.begin(), subpaths.end(), rng);

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
// Operator 2: Parent-based with Repair (from HEA)
// ============================================================================

std::vector<int> recombineOp2(const std::vector<int> &parent1,
                              const std::vector<int> &parent2, int n,
                              int selectCount,
                              const std::vector<std::vector<int>> &distance,
                              const std::vector<int> &costs) {
  // Find nodes present in both parents
  std::unordered_set<int> nodes2(parent2.begin(), parent2.end());

  // Start with parent1, keep only nodes also in parent2
  std::vector<int> partial;
  for (int node : parent1) {
    if (nodes2.count(node)) {
      partial.push_back(node);
    }
  }

  // Repair using weighted 2-regret
  double wRegret = 1.0, wBest = 1.0;
  return repairSolutionAMSEA(partial, distance, costs, n, selectCount, wRegret,
                             wBest);
}

// ============================================================================
// Operator 3: Path Relinking (NEW)
// ============================================================================

std::vector<int> pathRelink(const std::vector<int> &parent1,
                            const std::vector<int> &parent2, int n,
                            int selectCount,
                            const std::vector<std::vector<int>> &distance,
                            const std::vector<int> &costs, std::mt19937 &rng) {
  // Find common and unique nodes
  std::unordered_set<int> nodes2(parent2.begin(), parent2.end());

  std::vector<int> common;
  std::vector<int> unique1;

  for (int node : parent1) {
    if (nodes2.count(node)) {
      common.push_back(node);
    } else {
      unique1.push_back(node);
    }
  }

  // Randomly keep some of the unique nodes from parent1
  std::shuffle(unique1.begin(), unique1.end(), rng);
  int keepCount = unique1.size() / 2;

  // Build partial solution: common nodes + random subset of unique from parent1
  std::unordered_set<int> partialSet(common.begin(), common.end());
  for (int i = 0; i < keepCount; i++) {
    partialSet.insert(unique1[i]);
  }

  // Preserve parent1's order for the partial solution
  std::vector<int> partial;
  for (int node : parent1) {
    if (partialSet.count(node)) {
      partial.push_back(node);
    }
  }

  // Repair to full size
  double wRegret = 1.0, wBest = 1.0;
  return repairSolutionAMSEA(partial, distance, costs, n, selectCount, wRegret,
                             wBest);
}

// ============================================================================
// Repair Function (Weighted 2-Regret)
// ============================================================================

std::vector<int>
repairSolutionAMSEA(const std::vector<int> &partial,
                    const std::vector<std::vector<int>> &distance,
                    const std::vector<int> &costs, int n, int selectCount,
                    double wRegret, double wBest) {
  std::vector<int> solution = partial;

  std::vector<bool> selected(n, false);
  for (int node : solution) {
    selected[node] = true;
  }

  // Handle empty or single-node solutions
  if (solution.size() < 2) {
    int bestStart = -1;
    int bestCost = INT_MAX;
    for (int i = 0; i < n; i++) {
      if (!selected[i] && costs[i] < bestCost) {
        bestCost = costs[i];
        bestStart = i;
      }
    }
    if (solution.empty() && bestStart != -1) {
      solution.push_back(bestStart);
      selected[bestStart] = true;
    }

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
      break;

    solution.insert(solution.begin() + choosePos, chooseNode);
    selected[chooseNode] = true;
  }

  return solution;
}

// ============================================================================
// Perturbation Function (from ILS)
// ============================================================================

std::vector<int> perturbSolutionAMSEA(const std::vector<int> &solution, int n,
                                      std::mt19937 &rng) {
  std::vector<int> perturbed = solution;
  int solSize = perturbed.size();

  // Perturbation strength
  int k = std::min(5, std::max(2, solSize / 20));

  for (int iter = 0; iter < k; iter++) {
    std::uniform_int_distribution<> dist(0, solSize - 1);
    int pos1 = dist(rng);
    int pos2 = dist(rng);

    while (pos1 == pos2 || (pos1 + 1) % solSize == pos2 ||
           (pos2 + 1) % solSize == pos1) {
      pos2 = dist(rng);
    }

    if (pos1 > pos2)
      std::swap(pos1, pos2);

    std::reverse(perturbed.begin() + pos1 + 1, perturbed.begin() + pos2 + 1);
  }

  // Random node exchange with probability 0.3
  std::uniform_real_distribution<> probDist(0.0, 1.0);
  if (probDist(rng) < 0.3) {
    std::vector<bool> inSolution(n, false);
    for (int node : perturbed)
      inSolution[node] = true;

    std::vector<int> notSelected;
    for (int i = 0; i < n; i++) {
      if (!inSolution[i])
        notSelected.push_back(i);
    }

    if (!notSelected.empty()) {
      std::uniform_int_distribution<> posDist(0, solSize - 1);
      std::uniform_int_distribution<> nodeDist(0, notSelected.size() - 1);

      int replacePos = posDist(rng);
      int newNode = notSelected[nodeDist(rng)];
      perturbed[replacePos] = newNode;
    }
  }

  return perturbed;
}

// ============================================================================
// Adaptive Operator Selection
// ============================================================================

int selectOperatorAdaptive(const int *success, const int *attempts,
                           std::mt19937 &rng) {
  // Calculate success rates with Laplace smoothing
  double rates[3];
  double totalRate = 0.0;

  for (int i = 0; i < 3; i++) {
    rates[i] = (double)(success[i] + 1) / (double)(attempts[i] + 2);
    totalRate += rates[i];
  }

  // Roulette wheel selection
  std::uniform_real_distribution<> dist(0.0, totalRate);
  double r = dist(rng);
  double cumulative = 0.0;

  for (int i = 0; i < 3; i++) {
    cumulative += rates[i];
    if (cumulative >= r) {
      return i;
    }
  }

  return 2; // Default to last operator
}

// ============================================================================
// Elite Archive Management
// ============================================================================

void updateEliteArchive(std::vector<std::pair<int, std::vector<int>>> &archive,
                        const std::vector<int> &solution, int objective,
                        int maxSize) {
  // Check if this solution is already in archive (by objective)
  for (const auto &entry : archive) {
    if (entry.first == objective) {
      return; // Already exists
    }
  }

  if (archive.size() < maxSize) {
    archive.push_back({objective, solution});
  } else {
    // Find worst in archive
    int worstIdx = 0;
    for (int i = 1; i < archive.size(); i++) {
      if (archive[i].first > archive[worstIdx].first) {
        worstIdx = i;
      }
    }

    if (objective < archive[worstIdx].first) {
      archive[worstIdx] = {objective, solution};
    }
  }

  // Sort archive by objective (best first)
  std::sort(archive.begin(), archive.end());
}

// ============================================================================
// Main AMSEA Function
// ============================================================================

AMSEAResult amsea(int n, int selectCount,
                  const std::vector<std::vector<int>> &distance,
                  const std::vector<int> &costs, double timeLimit,
                  std::mt19937 &rng, int populationSize) {
  AMSEAResult result;
  result.bestObjective = INT_MAX;
  result.generations = 0;
  for (int i = 0; i < 3; i++) {
    result.operatorSuccesses[i] = 0;
    result.operatorAttempts[i] = 1; // Avoid division by zero
  }

  auto startTime = std::chrono::high_resolution_clock::now();

  // ========== INITIALIZATION ==========
  // Initialize population with diverse greedy solutions
  auto population = initializePopulationGreedy(n, selectCount, distance, costs,
                                               rng, populationSize);

  // Find initial best
  for (const auto &sol : population) {
    int obj = calculateObjective(sol, distance, costs);
    if (obj < result.bestObjective) {
      result.bestObjective = obj;
      result.bestSolution = sol;
    }
  }

  // Initialize elite archive
  const int ARCHIVE_SIZE = 5;
  std::vector<std::pair<int, std::vector<int>>> eliteArchive;
  updateEliteArchive(eliteArchive, result.bestSolution, result.bestObjective,
                     ARCHIVE_SIZE);

  // Operator tracking
  int operatorSuccess[3] = {0, 0, 0};
  int operatorAttempts[3] = {1, 1, 1};

  // Stagnation tracking
  const int STAGNATION_THRESHOLD = 50;
  int stagnationCounter = 0;

  // ========== MAIN LOOP ==========
  while (true) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(currentTime - startTime)
            .count();
    if (elapsed >= timeLimit)
      break;

    result.generations++;
    int previousBest = result.bestObjective;

    // Select operator adaptively
    int operatorIdx =
        selectOperatorAdaptive(operatorSuccess, operatorAttempts, rng);
    operatorAttempts[operatorIdx]++;
    result.operatorAttempts[operatorIdx]++;

    // Select parents
    auto [p1, p2] = selectParentsRandom(population.size(), rng);

    // Apply selected operator
    std::vector<int> offspring;
    switch (operatorIdx) {
    case 0:
      offspring =
          recombineOp1(population[p1], population[p2], n, selectCount, rng);
      break;
    case 1:
      offspring = recombineOp2(population[p1], population[p2], n, selectCount,
                               distance, costs);
      break;
    case 2:
      offspring = pathRelink(population[p1], population[p2], n, selectCount,
                             distance, costs, rng);
      break;
    }

    // Apply local search
    offspring = localSearchSteepestEdges(offspring, distance, costs, n);
    int offspringObj = calculateObjective(offspring, distance, costs);

    // Check if operator was successful
    int worstObj = getWorstObjective(population, distance, costs);
    if (offspringObj < worstObj) {
      operatorSuccess[operatorIdx]++;
      result.operatorSuccesses[operatorIdx]++;
    }

    // Population update
    if (!isDuplicateAMSEA(offspring, population, distance, costs)) {
      int worstIdx = findWorstIndexAMSEA(population, distance, costs);
      if (offspringObj <
          calculateObjective(population[worstIdx], distance, costs)) {
        population[worstIdx] = offspring;
      }
    }

    // Update best
    if (offspringObj < result.bestObjective) {
      result.bestObjective = offspringObj;
      result.bestSolution = offspring;
      updateEliteArchive(eliteArchive, offspring, offspringObj, ARCHIVE_SIZE);
      stagnationCounter = 0;
    } else {
      stagnationCounter++;
    }

    // ========== STAGNATION HANDLING ==========
    if (stagnationCounter >= STAGNATION_THRESHOLD) {
      // Perturb worst solutions
      for (int i = 0; i < 3 && i < population.size(); i++) {
        int worstIdx = findWorstIndexAMSEA(population, distance, costs);

        std::vector<int> perturbed =
            perturbSolutionAMSEA(population[worstIdx], n, rng);
        perturbed = localSearchSteepestEdges(perturbed, distance, costs, n);

        if (!isDuplicateAMSEA(perturbed, population, distance, costs)) {
          population[worstIdx] = perturbed;

          int perturbedObj = calculateObjective(perturbed, distance, costs);
          if (perturbedObj < result.bestObjective) {
            result.bestObjective = perturbedObj;
            result.bestSolution = perturbed;
            updateEliteArchive(eliteArchive, perturbed, perturbedObj,
                               ARCHIVE_SIZE);
          }
        }
      }

      // Inject elite solution
      if (!eliteArchive.empty()) {
        std::uniform_int_distribution<> eliteDist(0, eliteArchive.size() - 1);
        auto &elite = eliteArchive[eliteDist(rng)].second;

        if (!isDuplicateAMSEA(elite, population, distance, costs)) {
          int worstIdx = findWorstIndexAMSEA(population, distance, costs);
          population[worstIdx] = elite;
        }
      }

      stagnationCounter = 0;
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  result.totalTime =
      std::chrono::duration<double, std::milli>(endTime - startTime).count();

  return result;
}
