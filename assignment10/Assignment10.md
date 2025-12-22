# Assignment 10 - AMSEA Islands: Adaptive Multi-Strategy Evolutionary Algorithm

## Authors
- Mateusz Idziejczak 155842
- Mateusz Stawicki 155900

## Github
> https://github.com/Luncenok/EvolutionaryComputing

## Problem Description

This is the same variant of the Traveling Salesman Problem as in previous assignments:
- Select exactly 50% of nodes (rounded up if odd)
- Form a Hamiltonian cycle through selected nodes
- Minimize: total path length + sum of selected node costs
- Distances are Euclidean distances rounded to integers

Instances:
- **TSPA, TSPB** with 200 nodes, selecting 100 nodes.

## Goal

Improve the best-performing algorithm from previous assignments (HEA Operator 1) by implementing an enhanced hybrid evolutionary algorithm with **island model** and multiple optimization strategies.

## Algorithm Description: AMSEA Islands
 **AMSEA** stands for **Adaptive Multi-Strategy Evolutionary Algorithm**.
- **Experiment**: Comparison of Single-Population AMSEA vs. **AMSEA Islands (Two Variants)**.
- **Variant 1 (Fast)**: 2 Islands, 3 Operators (Common, Parent, PathRelink), Greedy LS, Random Selection. Optimized for speed.
- **Variant 2 (Full)**: 2 Islands, 4 Operators (+LNS), Steepest LS, Elite Archive, Adaptive Selection. Optimized for quality.
- **Results**: "Full" variant achieves the best known TSPB result (43480 avg), while "Fast" variant excels at TSPA (69153).
- **Conclusion**: The Island Model with full feature set provides the best overall robustness, particularly for the structure-heavy TSPB instance.

### Key Features

1. **Island Model (2 Islands)**
   - Population divided into 2 islands of 10 solutions each
   - Migration every 200 generations (ring topology)
   - Best solution from each island migrates to replace worst in target island

2. **Four Adaptive Operators**
   - **CommonNodes**: Intersection of parents + weighted 2-regret repair.
   - **Parent-based**: Copy parent + Strong Perturbation (5-8 2-opt moves + node swap).
   - **PathRelinking**: Enhanced PR with retention ratios (0.25, 0.50, 0.75).
   - **LNS (Destroy/Repair)**: Destroys 30% of solution, repairs with weighted 2-regret.

3. **Operator Selection**
   - **Adaptive**: Tracks success rates per operator. Selection probability is proportional to success rate.
   - Allows algorithm to favor "CommonNodes" or "PathRelinking" as needed.

4. **Steepest Descent Local Search**
   - Thorough neighborhood search (best improvement).
   - Applied after every offspring generation.
   - Slower than Greedy but produces higher quality local optima.

5. **Stagnation Handling (Per-Island)**
   - **Elite Archive**: Maintains top 5 global solutions.
   - **Recovery**: On stagnation (30 gens), island resets by importing an Elite solution OR via Strong Perturbation.

### Design Rationale (from Extensive Testing)

Over **25 configurations** were tested during development:

| Category | Tested | Finding |
|----------|--------|---------|
| Population sizes | 15, 18, 20, 25, 30, 40 | 20 optimal for single pop |
| Tournament selection | K=2, K=3 | K=3 best for TSPA, K=2 for TSPB |
| LNS destroy rates | 30%, 40%, 50% | 30% best (50% too slow) |
| Local Search | Steepest, Greedy, LM, Candidates | Greedy 4x faster, better results |
| Island configs | 2, 4, 5 islands; migrate 50-500 | 2 islands migrate 200 ties single |
| Advanced | LTM, ERX, Crowding, Clearing | All add too much overhead |

**Key Insight: Speed > Complex Features**
- More generations = better results
- LTM (Long-Term Memory) reduced gens from 5500 to 2000 → worse results
- Candidates/LM add overhead → fewer generations

## Algorithm Pseudocode

```python
AMSEA_Islands_Full(n, selectCount, distance, costs, timeLimit):
    # Configuration
    NUM_ISLANDS = 2
    ISLAND_SIZE = 10
    MIGRATION_INTERVAL = 200
    STAGNATION_THRESHOLD = 30
    ARCHIVE_SIZE = 5
    
    # Initialize islands & Global Archive
    eliteArchive = MinPriorityQueue(ARCHIVE_SIZE)
    for island in range(NUM_ISLANDS):
        islands[island] = initWithSteepestHeuristics()
        updateArchive(eliteArchive, islands[island])
    
    # Adaptive stats per island
    opStats[island] = {success: [0]*4, attempts: [1]*4}
    
    while elapsed_time < timeLimit:
        generations++
        
        for island in range(NUM_ISLANDS):
            # Tournament selection (K=3)
            p1, p2 = tournamentSelect(islands[island], K=3)
            
            # Adaptive Operator Selection (Roulette Wheel)
            op = selectAdaptive(opStats[island])
            offspring = applyOperator(op, p1, p2)
            
            # Steepest Descent Local Search
            offspring = steepestLocalSearch(offspring)
            
            # Update stats
            if offspring.obj < worst(islands[island]).obj:
                recordSuccess(opStats[island], op)
                replace(islands[island], offspring)
            
            # Update Global Best & Archive
            if offspring.obj < globalBest.obj:
                globalBest = offspring
                stagnation[island] = 0
            else:
                stagnation[island]++
            updateArchive(eliteArchive, offspring)
            
            # Stagnation Handling (Recovery)
            if stagnation[island] >= THRESHOLD:
                if random() < 0.5 and not eliteArchive.empty():
                    inject(islands[island], eliteArchive.random())
                else:
                    perturbWorst(islands[island]) # Strong perturbation + swap
                stagnation[island] = 0
        
        # Migration (ring topology)
        if generations % MIGRATION_INTERVAL == 0:
            migrateBestToWorst(islands)
    
    return globalBest
```

## Experimental Setup

- **Instances**: TSPA, TSPB (200 nodes, 100 selected)
- **Local search**: Steepest Descent (Best Improvement)
- **Population**: 20 total (2 islands × 10)
- **Migration**: Every 200 generations
- **Stagnation threshold**: 30 generations
- **Evaluation**:
  - Run AMSEA Islands **20 times** per instance
  - Use **time limit = average MSLS time** (1073 ms for TSPA, 1089 ms for TSPB)
  - Report min, max, and average objective values
  - Report operator success rates

## Key Results

### AMSEA Islands Performance (20 runs, 1s time limit)

| Instance | Best | Worst | Avg | Gens |
|----------|------|-------|-----|------|
| **TSPA (Fast)** | **69095** | 69265 | **69166** | 6179 |
| **TSPB (Fast)** | **43448** | 43964 | 43645 | 4580 |
| **TSPA (Full)** | **69095** | 69405 | 69154 | 1767 |
| **TSPB (Full)** | **43446** | 43624 | **43480** | 1887 |

### Operator Success Rates

| Operator | TSPA | TSPB |
|----------|------|------|
| **CommonNodes** | **80.8%** | **84.1%** |
| Parent (perturbation) | 22.5% | 29.3% |
| **PathRelink** | **80.3%** | **85.7%** |

CommonNodes and PathRelink are the primary drivers of improvement!

### Comparison with All Previous Methods

| Method | TSPA | TSPB |
|--------|------|------|
| Random | 264638 (238611 – 287962) | 213875 (190076 – 244960) |
| Nearest Neighbor (end only) | 85108 (83182 – 89433) | 54390 (52319 – 59030) |
| Nearest Neighbor (any position) | 73178 (71179 – 75450) | 45870 (44417 – 53438) |
| Greedy Cycle | 72646 (71488 – 74410) | 51400 (49001 – 57324) |
| Greedy 2-Regret | 115474 (105852 – 123428) | 72454 (66505 – 77072) |
| Greedy Weighted (2-Regret + BestDelta) | 72129 (71108 – 73395) | 50950 (47144 – 55700) |
| Nearest Neighbor Any 2-Regret | 116659 (106373 – 126570) | 73646 (67121 – 79013) |
| Nearest Neighbor Any Weighted | 72401 (70010 – 75452) | 47653 (44891 – 55247) |
| LS Random + Steepest + Nodes | 88011 (81817 – 97630) | 62848 (55928 – 70479) |
| LS Random + Greedy + Nodes | 93267 (86375 – 101454) | 65388 (57842 – 76707) |
| LS Random + Greedy + Edges | 81101 (76362 – 87763) | 54088 (50858 – 59045) |
| LS Greedy + Steepest + Nodes | 71614 (70626 – 72950) | 45414 (43826 – 50876) |
| LS Greedy + Steepest + Edges | 71460 (70510 – 72614) | 44979 (43921 – 50629) |
| LS Greedy + Greedy + Nodes | 71908 (71093 – 73048) | 45584 (43917 – 51165) |
| LS Greedy + Greedy + Edges | 71825 (70977 – 72706) | 45376 (43845 – 51170) |
| LS Random + Steepest + Edges | 73965 (71371 – 78984) | 48252 (45823 – 51965) |
| LM Random + Steepest + Edges | 74981 (72054 – 79520) | 49325 (45965 – 52805) |
| Candidates (k=5) | 84726 (78843 – 91459) | 49873 (47117 – 53865) |
| Candidates (k=10) | 77773 (72851 – 84000) | 48450 (45669 – 51178) |
| Candidates (k=15) | 75510 (72276 – 83040) | 48295 (45582 – 51938) |
| Candidates (k=20) | 74416 (71292 – 80264) | 48221 (45338 – 51285) |
| LM Candidates (k=10) | 75157 (72331 – 80832) | 49219 (46145 – 52021) |
| LM Candidates (k=20) | 74976 (72054 – 79520) | 49302 (45965 – 52805) |
| MSLS (200 iterations) | 71306 (70748 – 71959) | 45741 (45356 – 46168) |
| ILS | 69298 (69107 – 69842) | 43721 (43460 – 44297) |
| LNS with LS | 69792 (69411 – 70347) | 44068 (43565 – 45273) |
| LNS without LS | 69865 (69417 – 70361) | 44488 (43832 – 46282) |
| HEA Operator 1 | 69212 (69107 – 69397) | 43602 (43481 – 44024) |
| HEA Operator 2 with LS | 70180 (69603 – 70856) | 44433 (43778 – 45095) |
| HEA Operator 2 without LS | 70357 (69574 – 70912) | 44574 (43814 – 45062) |
| AMSEA (Single) | 69193 (69100 – 69442) | 43506 (43446 – 43642) |
| **AMSEA Islands (Fast)** | **69153 (69095 – 69266)** | 43645 (43448 – 43964) |
| **AMSEA Islands (Full)** | 69154 (69095 – 69405) | **43480 (43446 – 43624)** |

### Comparison: Single Pop vs Islands

| Instance | Single Pop Avg | Islands Avg | Difference |
|----------|---------------|-------------|------------|
| TSPA | 69193 | 69153 (Fast) / 69154 (Full) | **Better (Both)** ✅ |
| TSPB | 43506 | **43480 (Full)** / 43645 (Fast) | **Best (Full)** ✅ |

### Running Times (ms)

| Method | TSPA | TSPB |
|--------|------|------|
| MSLS (200 iterations) | 1073 | 1089 |
| ILS | 1074 | 1089 |
| HEA Operator 1 | 1074 | 1089 |
| AMSEA (single pop) | 1074 | 1089 |
| AMSEA Islands (Fast) | 1073 | 1083 |
| AMSEA Islands (Full) | 1075 | 1083 |

## Visualizations

Best solutions found by AMSEA Islands visualized on both instances:

<table>
  <tr>
    <th>AMSEA Islands - TSPA</th>
    <th>AMSEA Islands - TSPB</th>
  </tr>
  <tr>
    <td><img src="../output/TSPA_AMSEA_Islands_time_limit_=_1073_ms.png" width="100%"></td>
    <td><img src="../output/TSPB_AMSEA_Islands_time_limit_=_1089_ms.png" width="100%"></td>
  </tr>
</table>

### What Was Tested (Summary of 25+ Experiments)

| Technique | Result | Notes |
|-----------|--------|-------|
| **Greedy LS** | ✅ Success | 4x faster, +39% more gens |
| **Island Model** | ⚖️ Mixed | TSPA better, TSPB worse |
| **Strong Perturbation** | ✅ Success | Crucial for escaping optima |
| Long-Term Memory | ❌ Failed | 2000 vs 5500 gens - too slow |
| ERX Crossover | ❌ Failed | Overhead kills generation count |
| Candidates LS | ❌ Failed | Fewer gens, worse results |
| LM (List of Moves) | ❌ Failed | Hash overhead > delta calc |
| Clearing | ❌ Failed | Distance computation too slow |
| Crowding | ❌ Failed | Good single runs, bad avg |
| LNS 50% | ❌ Failed | Repair too expensive |
| ML Bandit UCB | ❌ Failed | +332 TSPA, +316 TSPB vs AMSEA |
| **Adaptive Selection** | ✅ Success | Effectively managed the 4-operator suite. |