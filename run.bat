@echo off
g++ -std=c++17 -O2 -I. ^
    main.cpp ^
    calculateObjective.cpp ^
    algorithmEvaluator.cpp ^
    assignment1/randomSolution.cpp ^
    assignment1/nearestNeighborEnd.cpp ^
    assignment1/nearestNeighborAny.cpp ^
    assignment1/greedyCycle.cpp ^
    assignment2/greedyRegret2.cpp ^
    assignment2/greedyRegret2Weighted.cpp ^
    assignment2/nearestNeighborAnyRegret2.cpp ^
    assignment2/nearestNeighborAnyRegret2Weighted.cpp ^
    assignment3/localSearch.cpp ^
    assignment4/candidateMoves.cpp ^
    assignment5/localSearchLM.cpp ^
    assignment5/localSearchLMCandidates.cpp ^
    assignment6/multipleStartLS.cpp ^
    assignment6/iteratedLS.cpp ^
    assignment7/largeNeighborhoodSearch.cpp ^
    assignment8/globalConvexity.cpp ^
    -o main && main.exe > output.txt
