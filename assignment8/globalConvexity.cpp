#include "../include/globalConvexity.h"
#include "../include/randomSolution.h"
#include "../include/localSearch.h"
#include "../include/calculateObjective.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>

// Calculate number of common edges between two solutions
int calculateCommonEdges(const std::vector<int>& sol1, const std::vector<int>& sol2) {
    // Build edge set for sol1
    std::set<std::pair<int, int>> edges1;
    int n1 = sol1.size();
    for (int i = 0; i < n1; i++) {
        int u = sol1[i];
        int v = sol1[(i + 1) % n1];
        if (u > v) std::swap(u, v);
        edges1.insert({u, v});
    }
    
    // Count common edges with sol2
    int commonCount = 0;
    int n2 = sol2.size();
    for (int i = 0; i < n2; i++) {
        int u = sol2[i];
        int v = sol2[(i + 1) % n2];
        if (u > v) std::swap(u, v);
        if (edges1.count({u, v})) {
            commonCount++;
        }
    }
    
    return commonCount;
}

// Calculate number of common nodes between two solutions
int calculateCommonNodes(const std::vector<int>& sol1, const std::vector<int>& sol2) {
    std::set<int> nodes1(sol1.begin(), sol1.end());
    int commonCount = 0;
    for (int node : sol2) {
        if (nodes1.count(node)) {
            commonCount++;
        }
    }
    return commonCount;
}

// Calculate Pearson correlation coefficient
double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0;
    
    int n = x.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
    
    for (int i = 0; i < n; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    
    double numerator = n * sumXY - sumX * sumY;
    double denominator = sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
    
    if (denominator < 1e-10) return 0.0;
    return numerator / denominator;
}

GlobalConvexityResult analyzeGlobalConvexity(
    const std::string& instanceName,
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<int>& bestSolutionFromBestMethod,
    std::mt19937& rng
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    GlobalConvexityResult result;
    result.instanceName = instanceName;
    
    const int NUM_LOCAL_OPTIMA = 1000;
    
    // Generate 1000 random local optima (silently for clean output)
    std::vector<std::vector<int>> localOptima;
    std::vector<int> objectives;
    localOptima.reserve(NUM_LOCAL_OPTIMA);
    objectives.reserve(NUM_LOCAL_OPTIMA);
    
    for (int i = 0; i < NUM_LOCAL_OPTIMA; i++) {
        // Generate random initial solution
        std::uniform_int_distribution<> startDist(0, n - 1);
        int start = startDist(rng);
        auto initial = randomSolution(start, n, selectCount, rng);
        
        // Apply greedy local search with edges
        auto localOpt = localSearchGreedyEdges(initial, distance, costs, n, rng);
        
        localOptima.push_back(localOpt);
        objectives.push_back(calculateObjective(localOpt, distance, costs));
    }
    
    // Find best, worst, and average objective values
    int bestIdx = 0;
    int bestObj = objectives[0];
    int worstObj = objectives[0];
    long long sumObj = 0;
    
    for (int i = 0; i < NUM_LOCAL_OPTIMA; i++) {
        sumObj += objectives[i];
        if (objectives[i] < bestObj) {
            bestObj = objectives[i];
            bestIdx = i;
        }
        if (objectives[i] > worstObj) {
            worstObj = objectives[i];
        }
    }
    
    // Store statistics
    result.minObjective = bestObj;
    result.maxObjective = worstObj;
    result.avgObjective = sumObj / (double)NUM_LOCAL_OPTIMA;
    result.bestSolution = localOptima[bestIdx];
    
    // Calculate similarities for each measure
    for (int measureType = 0; measureType < 2; measureType++) {
        // measureType 0: edges, 1: nodes
        std::string measureName = (measureType == 0) ? "edges" : "nodes";
        
        // Version 1: Average similarity to all other local optima
        {
            std::vector<double> objValues;
            std::vector<double> similarities;
            
            for (int i = 0; i < NUM_LOCAL_OPTIMA; i++) {
                double avgSimilarity = 0.0;
                for (int j = 0; j < NUM_LOCAL_OPTIMA; j++) {
                    if (i != j) {
                        int commonCount = (measureType == 0) 
                            ? calculateCommonEdges(localOptima[i], localOptima[j])
                            : calculateCommonNodes(localOptima[i], localOptima[j]);
                        avgSimilarity += commonCount;
                    }
                }
                avgSimilarity /= (NUM_LOCAL_OPTIMA - 1);
                
                objValues.push_back(objectives[i]);
                similarities.push_back(avgSimilarity);
            }
            
            double corr = calculateCorrelation(objValues, similarities);
            
            // Calculate similarity statistics
            double minSim = similarities[0], maxSim = similarities[0], sumSim = 0;
            for (double s : similarities) {
                if (s < minSim) minSim = s;
                if (s > maxSim) maxSim = s;
                sumSim += s;
            }
            
            ConvexityData data;
            data.objectives = objValues;
            data.similarities = similarities;
            data.correlation = corr;
            data.measureName = measureName;
            data.versionName = "avg_all";
            data.minSimilarity = minSim;
            data.maxSimilarity = maxSim;
            data.avgSimilarity = sumSim / similarities.size();
            
            if (measureType == 0) {
                result.edgesData.push_back(data);
            } else {
                result.nodesData.push_back(data);
            }
        }
        
        // Version 2: Similarity to best out of 1000 local optima
        {
            std::vector<double> objValues;
            std::vector<double> similarities;
            
            for (int i = 0; i < NUM_LOCAL_OPTIMA; i++) {
                if (i == bestIdx) continue; // Skip the best solution itself
                
                int commonCount = (measureType == 0) 
                    ? calculateCommonEdges(localOptima[i], localOptima[bestIdx])
                    : calculateCommonNodes(localOptima[i], localOptima[bestIdx]);
                
                objValues.push_back(objectives[i]);
                similarities.push_back(commonCount);
            }
            
            double corr = calculateCorrelation(objValues, similarities);
            
            // Calculate similarity statistics
            double minSim = similarities[0], maxSim = similarities[0], sumSim = 0;
            for (double s : similarities) {
                if (s < minSim) minSim = s;
                if (s > maxSim) maxSim = s;
                sumSim += s;
            }
            
            ConvexityData data;
            data.objectives = objValues;
            data.similarities = similarities;
            data.correlation = corr;
            data.measureName = measureName;
            data.versionName = "best_1000";
            data.minSimilarity = minSim;
            data.maxSimilarity = maxSim;
            data.avgSimilarity = sumSim / similarities.size();
            
            if (measureType == 0) {
                result.edgesData.push_back(data);
            } else {
                result.nodesData.push_back(data);
            }
        }
        
        // Version 3: Similarity to very good solution from best method (ILS)
        {
            std::vector<double> objValues;
            std::vector<double> similarities;
            
            for (int i = 0; i < NUM_LOCAL_OPTIMA; i++) {
                int commonCount = (measureType == 0) 
                    ? calculateCommonEdges(localOptima[i], bestSolutionFromBestMethod)
                    : calculateCommonNodes(localOptima[i], bestSolutionFromBestMethod);
                
                objValues.push_back(objectives[i]);
                similarities.push_back(commonCount);
            }
            
            double corr = calculateCorrelation(objValues, similarities);
            
            // Calculate similarity statistics
            double minSim = similarities[0], maxSim = similarities[0], sumSim = 0;
            for (double s : similarities) {
                if (s < minSim) minSim = s;
                if (s > maxSim) maxSim = s;
                sumSim += s;
            }
            
            ConvexityData data;
            data.objectives = objValues;
            data.similarities = similarities;
            data.correlation = corr;
            data.measureName = measureName;
            data.versionName = "best_method";
            data.minSimilarity = minSim;
            data.maxSimilarity = maxSim;
            data.avgSimilarity = sumSim / similarities.size();
            
            if (measureType == 0) {
                result.edgesData.push_back(data);
            } else {
                result.nodesData.push_back(data);
            }
        }
    }
    
    // Calculate total time
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    return result;
}

void exportConvexityData(const GlobalConvexityResult& result, const std::string& outputDir) {
    // Export edges data
    for (const auto& data : result.edgesData) {
        std::string filename = outputDir + "/" + result.instanceName + "_" + 
                               data.measureName + "_" + data.versionName + ".csv";
        std::ofstream fout(filename);
        fout << "objective,similarity,correlation\n";
        for (size_t i = 0; i < data.objectives.size(); i++) {
            fout << data.objectives[i] << "," << data.similarities[i] << "," << data.correlation << "\n";
        }
        fout.close();
    }
    
    // Export nodes data
    for (const auto& data : result.nodesData) {
        std::string filename = outputDir + "/" + result.instanceName + "_" + 
                               data.measureName + "_" + data.versionName + ".csv";
        std::ofstream fout(filename);
        fout << "objective,similarity,correlation\n";
        for (size_t i = 0; i < data.objectives.size(); i++) {
            fout << data.objectives[i] << "," << data.similarities[i] << "," << data.correlation << "\n";
        }
        fout.close();
    }
}
