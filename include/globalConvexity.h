#ifndef GLOBAL_CONVEXITY_H
#define GLOBAL_CONVEXITY_H

#include <vector>
#include <string>
#include <random>

struct ConvexityData {
    std::vector<double> objectives;
    std::vector<double> similarities;
    double correlation;
    std::string measureName;
    std::string versionName;
    
    // Similarity statistics
    double minSimilarity;
    double maxSimilarity;
    double avgSimilarity;
};

struct GlobalConvexityResult {
    std::string instanceName;
    std::vector<ConvexityData> edgesData;  // 3 versions: avg, best_1000, best_method
    std::vector<ConvexityData> nodesData;  // 3 versions: avg, best_1000, best_method
    
    // Statistics
    int minObjective;
    int maxObjective;
    double avgObjective;
    std::vector<int> bestSolution;
    double totalTime;
};

// Analyze global convexity by generating 1000 random local optima
// and calculating fitness-distance correlations
GlobalConvexityResult analyzeGlobalConvexity(
    const std::string& instanceName,
    int n,
    int selectCount,
    const std::vector<std::vector<int>>& distance,
    const std::vector<int>& costs,
    const std::vector<int>& bestSolutionFromBestMethod,
    std::mt19937& rng
);

// Export convexity data to CSV files
void exportConvexityData(const GlobalConvexityResult& result, const std::string& outputDir);

#endif
