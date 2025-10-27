g++ -std=c++17 -O2 -I. \
    main.cpp \
    calculateObjective.cpp \
    algorithmEvaluator.cpp \
    assignment1/randomSolution.cpp \
    assignment1/nearestNeighborEnd.cpp \
    assignment1/nearestNeighborAny.cpp \
    assignment1/greedyCycle.cpp \
    assignment2/greedyRegret2.cpp \
    assignment2/greedyRegret2Weighted.cpp \
    assignment2/nearestNeighborAnyRegret2.cpp \
    assignment2/nearestNeighborAnyRegret2Weighted.cpp \
    assignment3/localSearch.cpp \
    -o main && ./main | tee output.txt