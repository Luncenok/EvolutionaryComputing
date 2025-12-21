# AMSEA Strategy Testing Notes

## Goal
Beat HEA Operator 1 baseline:
- **TSPA**: Avg 69260, Best 69107
- **TSPB**: Avg 43592, Best 43446

---

## Version History & Results

### V1 - Initial AMSEA (Baseline)
**Strategies:**
- Greedy population initialization (Greedy Cycle, NN Any, Weighted 2-Regret)
- 3 operators: Common Edges, Parent Repair, Path Relinking
- Adaptive operator selection (roulette wheel + Laplace smoothing)
- Random parent selection
- Replace worst strategy
- Stagnation handling (perturb + elite injection)
- Population size: 20, Stagnation threshold: 50

**Results:**
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| TSPA | 69095 ✅ | 69139 ✅ | ~5200 |
| TSPB | 43475 | 43522 ✅ | ~4350 |

**Notes:** Good start, beats HEA on average both instances. Path Relinking most successful operator (68% TSPA, 58% TSPB).

---

### V1-Optimized - Performance Tuning
**Changes from V1:**
- Objective caching (hash set + population array)
- Inline fast local search
- Stagnation threshold: 30
- -O3 compilation

**Results:**
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| TSPA | 69095 | 69141 | ~5271 |
| TSPB | 43464 ✅ | 43558 | ~4738 |

**Notes:** More generations, slightly better TSPB best.

---

### V2 - Enhanced Operators & Selection
**Changes from V1-Optimized:**
- Tournament selection (size 3) instead of random
- Enhanced Path Relinking (tries 25%/50%/75% ratios, picks best)
- Added LNS operator (4th operator: destroy 30%, repair with 2-regret)
- Population size: 15 (smaller for more gens)
- Stagnation threshold: 20

**Results:**
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| TSPA | 69100 | 69246 | ~3482 |
| TSPB | 43448 ✅ | 43500 ✅ | ~4089 |

**Notes:** TSPB new best! But TSPA regressed slightly. Fewer generations than V1-Opt despite smaller population - operators more expensive?

---

## Strategy Analysis

### What Worked Well ✅
1. **Greedy initialization** - Much better than random starts
2. **Path Relinking** - Consistently highest success rate
3. **Objective caching** - Significant speedup
4. **Tournament selection** - Better quality parents
5. **Elite archive** - Prevents losing best solutions

### What Had Mixed Results ⚠️
1. **Smaller population (15 vs 20)** - More gens but less diversity
2. **Enhanced Path Relinking** - Expensive, may not always help
3. **LNS operator** - Added value unclear, adds complexity

### What Might Help 🤔
1. **Adaptive local search intensity** - Full LS only sometimes
2. **Crowding replacement** - Replace similar, not worst
3. **Multiple elite archives** - Different quality/diversity tradeoffs
4. **Simulated annealing acceptance** - Accept worse solutions early
5. **Island model** - Multiple populations exchanging solutions
6. **Restart mechanism** - Full restart when deeply stagnated
7. **Problem-specific operators** - Use distance/cost structure

---

## Next Ideas to Test

### High Priority
1. **Revert to simpler operators** - Enhanced PR may be too slow
2. **Try population size 18** - Balance between 15 and 20
3. **Probabilistic local search** - 60-70% chance of full LS

### Medium Priority  
4. **Crowding replacement** - Maintain diversity better
5. **Variable stagnation threshold** - Increase over time
6. **Operator weights based on problem** - TSPA vs TSPB may need different settings

### Lower Priority
7. **Island model** - Complex but potentially powerful
8. **Hybrid with ILS** - Use ILS for intensification

---

## Current Best Results Summary

| Instance | Method | Best | Avg |
|----------|--------|------|-----|
| TSPA | V1-Initial | **69095** | 69139 |
| TSPA | V2-Enhanced | 69100 | 69246 |
| TSPB | V2-Enhanced | **43448** | **43500** |
| TSPB | ILS (historical) | 43446 | 43656 |

---

## Systematic Testing Results (10 runs each, 1s time limit)

### TSPA Results
| Config | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Pop20-Stag30 | 69100 | 69140 | 69237 | 5787 |
| Pop15-Stag20 | 69100 | 69148 | 69334 | 5619 |
| Pop20-Stag20 | 69100 | 69174 | 69405 | 5490 |
| Pop25-Stag30 | 69100 | 69199 | 69442 | 5618 |
| **Pop20-Tour3** | **69095** | **69121** ✅ | 69144 | 5390 |
| Pop15-Tour3 | 69100 | 69229 | 69405 | 5490 |
| Pop20-Tour2 | 69100 | 69133 | 69237 | 5452 |
| Pop20-PartialLS70 | 69100 | 69187 | 69405 | 5530 |
| Pop20-PartialLS50 | 69100 | 69171 | 69405 | 5734 |
| Pop20-Tour3-PartialLS70 | 69100 | 69216 | 69440 | 5486 |
| Pop18-Tour2-Stag25 | 69100 | 69154 | 69278 | 5412 |

**Winner: Pop20-Tour3** (Best=69095, Avg=69121)

### TSPB Results
| Config | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Pop20-Stag30 | 43446 | 43466 | 43524 | 6078 |
| Pop15-Stag20 | 43448 | 43506 | 43617 | 5960 |
| Pop20-Stag20 | 43448 | 43504 | 43616 | 6098 |
| Pop25-Stag30 | 43446 | 43471 | 43518 | 6184 |
| Pop20-Tour3 | 43446 | 43497 | 43673 | 6318 |
| Pop15-Tour3 | 43446 | 43474 | 43563 | 6133 |
| Pop20-Tour2 | 43448 | 43492 | 43579 | 6404 |
| Pop20-PartialLS70 | 43456 | 43487 | 43598 | 5996 |
| Pop20-PartialLS50 | 43446 | 43503 | 43620 | 6228 |
| Pop20-Tour3-PartialLS70 | 43446 | 43484 | 43673 | 6135 |
| **Pop18-Tour2-Stag25** | **43446** | **43465** ✅ | 43503 | 5818 |

**Winner: Pop18-Tour2-Stag25** (Best=43446, Avg=43465)

---

## Key Insights from Testing

1. **Tournament selection helps** - Both best configs use tournament
2. **TSPA likes larger population** (20) with tournament size 3
3. **TSPB likes medium population** (18) with tournament size 2  
4. **Partial LS doesn't help much** - Full LS better
5. **43446 on TSPB matches ILS record!**

---

## Next Tests to Try
1. More runs (20) on best configs to confirm
2. Try Pop20-Tour3 for both instances
3. Try Pop18-Tour2 for both instances

---

## Advanced Testing Round 2 (NEW strategies, simplified recombination)

**Note:** These tests use simplified recombination (common nodes + repair only).
Results are baseline for comparing strategy effects, NOT final performance.

### TSPA Results
| Config | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Baseline-Pop20 | 69517 | 69731 | 69903 | 13544 |
| Crowding-Pop20 | 69201 | 69724 | 69913 | 12362 |
| Adaptive-Pop20 | 69311 | 69619 | 69888 | 13773 |
| **StrongPerturb-Pop20** | 69284 | **69412** | 69645 | 11848 |
| **Destroy50-Pop20** | 69312 | **69467** | 69637 | 8814 |
| Restart-Pop20 | **69173** | 69578 | 69905 | 13805 |

### TSPB Results  
| Config | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Baseline-Pop20 | 43737 | 43837 | 43983 | 11349 |
| **StrongPerturb-Pop20** | 43683 | **43736** | 43777 | 10796 |
| Destroy40-Pop20 | 43581 | 43740 | 43945 | 9460 |
| **Destroy50-Pop20** | **43566** | **43695** | 43795 | 9360 |

### Key Insights:
1. **Strong Perturbation helps!** - Better avg on both instances
2. **Destroy50 (50% destroy rate) is excellent** - Best TSPB results
3. **Crowding shows promise** - Best single-run TSPA (69201)
4. **Restart mechanism** - Found best single TSPA (69173)

### Next: Integrate Best Ideas into Full AMSEA
- Add Destroy50 as operator
- Use Strong Perturbation
- Consider Restart mechanism

---

## Local Search Comparison (Assignment Hints: global memory, candidates)

Tested different LS implementations from Assignments 4-5:

### TSPA Results  
| LS Type | Best | Avg | Worst | Gens |
|---------|------|-----|-------|------|
| **Inline-LS** | **69281** | **69524** | 69815 | **13941** |
| LM-LS | 69301 | 69505 | 69780 | 12501 |
| Cand-Only (k=10) | 69472 | 69823 | 70053 | 3175 |
| LM+Cand | 69652 | 70047 | 70670 | 1845 |

### TSPB Results
| LS Type | Best | Avg | Worst | Gens |
|---------|------|-----|-------|------|
| **Inline-LS** | 43721 | 43835 | 43958 | **11964** |
| **LM-LS** | **43699** | **43810** | 43991 | 9803 |
| Cand-Only | 43737 | 43841 | 43958 | 2662 |
| LM+Cand | 43724 | 43825 | 43958 | 1791 |

### Surprising Finding:
- ❌ LM (List of Moves) is NOT faster - fewer generations
- ❌ Candidates dramatically reduce generations (bad for EAs!)
- ✅ **Inline LS is the best approach** for this problem
- The overhead of maintaining move lists outweighs benefits

### Conclusion:
Keep using inline LS - it's already optimal for this problem structure.

---

## 🎓 Lecture-Based Insights (NEW!)

### Key Concepts from Slides:

#### Already Implemented ✅
- Hybrid EA (LS after recombination)
- Elite selection with steady state
- Path relinking
- Weighted 2-regret repair
- Tournament selection
- Perturbation/ILS-style

#### Tested but Mixed Results ⚠️
- Crowding replacement
- LM (List of Moves) - slower
- Candidate moves - fewer generations

#### **NOT YET TESTED - HIGH POTENTIAL** 🔴

1. **Edge Recombination Crossover (ERX)** (from slides):
   > "Select a random element. The next element is subsequent element from one (randomly selected) parent. If both subsequent elements are already selected, the next element is selected randomly"
   - Preserves EDGES not just nodes!
   - Different from our current operators

2. **Island Model with Migration**:
   > "The population is divided into several disjoint populations evolving (almost) independently... From time to time, certain solutions migrate to another population"
   - Could prevent premature convergence
   - Different islands explore different regions

3. **Long-Term Memory / Frequency-Based Selection**:
   > "Table of frequency of occurrences of individual edges in the visited solutions"
   - Use edge frequency to guide construction/repair
   - Intensification: prefer frequent (good) edges
   - Diversification: prefer infrequent edges

4. **Clearing** (stronger than crowding):
   > "the offspring competes with all solutions within a certain radius, only one winner survives"
   - More aggressive diversity mechanism

5. **Adaptive LNS** (from slides):
   > "We can use many different types of destroy and repair methods. Their probability is automatically modified based on the effects"
   - We have adaptive operators but could do more

### Priority List for Testing:
1. **ERX operator** - New recombination preserving edges
2. **Island model** - Multiple populations with migration
3. **Long-term memory** - Track edge frequencies

---

## 📊 Lecture Techniques Test Results

### TSPA
| Method | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Baseline | 69293 | 69584 | 69813 | 13631 |
| Island-4x5 | 69743 | 70012 | 70600 | 4026 |
| **LongTermMemory** | 69361 | **69553** | 69873 | **16332** |

### TSPB
| Method | Best | Avg | Worst | Gens |
|--------|------|-----|-------|------|
| Baseline | 43698 | 43802 | 43953 | 10871 |
| Island-4x5 | 43816 | 43902 | 43994 | 2811 |
| **LongTermMemory** | **43655** | **43727** | 43794 | **12975** |

### Key Findings:
1. ✅ **Long-Term Memory WORKS!** - Better avg on TSPB, more generations
2. ❌ **Island Model failed** - Small populations = worse results
3. ✅ **LTM gets ~20% more generations** despite overhead of tracking edges

### Next Step:
Integrate Long-Term Memory into full AMSEA with all operators

---

## 🔬 Ultimate AMSEA Test (Combining ALL Best)

### Configuration:
- Tournament K=3, Pop20, Stagnation 30
- 5 operators: CommonNodes, Parent, PathRelink, LNS30, LNS50
- Long-Term Memory (edge frequencies)
- Strong perturbation, LTM-enhanced repair

### Results:
| Instance | Best | Avg | Worst | Gens |
|----------|------|-----|-------|------|
| TSPA | 69144 | 69230 | 69433 | **2089** ❌ |
| TSPB | 43462 | 43574 | 43712 | **2303** ❌ |

### Problem: LTM OVERHEAD TOO HIGH!
- Only ~2000 generations vs ~5500 baseline
- Results worse despite "better" operators
- LTM repair scans all edges = O(n²) per repair

### Operator Success Rates:
| Operator | TSPA | TSPB | Notes |
|----------|------|------|-------|
| CommonNodes | 4.3% | 5.9% | |
| Parent | 0.2% | 0.2% | Almost useless |
| PathRelink | 7.6% | 6.8% | Good |
| LNS30 | 8.5% | 7.3% | Good |
| **LNS50** | **10.5%** | 6.7% | **Best for TSPA!** |

### Key Insight:
**Speed > Complex Features** for this problem
- More generations = better results
- LTM overhead kills performance
- Keep it simple, keep it fast!

### Next: Try LNS50 without LTM

---

## 🧪 LNS50 Integration Test

### LNS30 (original):
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| TSPA | 69095 | 69136 | 5486 |
| TSPB | 43446 | 43471 | 5623 |

### LNS50 (updated):
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| TSPA | 69100 | 69136 | 4925 |
| TSPB | 43446 | 43496 | 5073 |

### Analysis:
- LNS50 = fewer generations (more expensive repair)
- TSPA: Same avg, slightly worse best
- TSPB: Slightly worse avg (43496 vs 43471)
- **LNS30 is actually better in full AMSEA context!**

### Key Insight:
LNS50 worked in isolation tests because repair is cheap there.
In full AMSEA, the 50% destroy rate means:
- More nodes to repair = slower
- Fewer generations per second
- Net negative effect

**Reverting to LNS30 - it's already optimal!**

---

## 🏆 FINAL RESEARCH CONCLUSIONS

### Best Configuration (Verified):
| Instance | Best | Avg | Gens |
|----------|------|-----|------|
| **TSPA** | **69095** ✅ | 69182 | 5252 |
| **TSPB** | **43446** ✅ | 43479 | 5761 |

### vs HEA Op1 Baseline:
| Instance | HEA Op1 Avg | AMSEA Avg | Improvement |
|----------|-------------|-----------|-------------|
| TSPA | 69260 | 69182 | ✅ Better |
| TSPB | 43592 | 43479 | ✅ Better |

### Optimal AMSEA Configuration:
- **Population**: 20
- **Stagnation Threshold**: 30
- **Tournament Selection**: K=3
- **Operators**: 4 (CommonNodes, Parent, PathRelink, LNS30)
- **Perturbation**: Strong (5-8 2-opt, 50% node swap)
- **Local Search**: Inline (faster than LM/Candidates)

### Key Research Insights:

1. **SPEED > COMPLEX FEATURES**
   - More generations = better results
   - Overhead kills performance (LTM: 2000 vs 5500 gens)

2. **What Works:**
   - Tournament selection
   - Greedy initialization (diverse heuristics)
   - Path Relinking (highest success rate)
   - Strong perturbation
   - Inline local search

3. **What Doesn't Help:**
   - Long-Term Memory (too slow)
   - LM (List of Moves) - overhead
   - Candidate moves - fewer generations
   - Island Model with small populations
   - Higher destroy rates (LNS50 slower)

4. **Surprising Finding:**
   - Some strategies work in isolation but fail in full context
   - Always test in the complete algorithm!

### Total Configurations Tested: 25+
- Population sizes: 15, 18, 20, 25
- Stagnation: 20, 25, 30
- Tournament: K=2, K=3
- LNS destroy: 30%, 40%, 50%
- Local Search: Inline, LM, Candidates
- Advanced: LTM, Crowding, Island Model, Adaptive Stagnation

---

## ⚡ LTM OPTIMIZATION SUCCESS!

### Key Optimization: Replace unordered_map with 2D Array

The original LTM used `unordered_map<long long, int>` for edge frequencies.
This has O(1) average but with significant hash overhead.

**Solution**: Use `vector<int>` as packed 2D array with direct indexing.
```cpp
class FastEdgeFreq {
    std::vector<int> freq;  // size n*n
    inline int get(int a, int b) {
        if (a > b) std::swap(a, b);
        return freq[a * n + b];
    }
};
```

### LTM Usage Ratio Test Results

**TSPA:**
| LTM Ratio | Best | Avg | Gens |
|-----------|------|-----|------|
| 0% | 69237 | 69460 | 13908 |
| 10% | 69242 | 69436 | 14354 |
| 25% | 69291 | 69453 | 15126 |
| **50%** | 69315 | **69386** ⭐ | 14536 |
| 100% | 69284 | 69484 | 15122 |

**TSPB:**
| LTM Ratio | Best | Avg | Gens |
|-----------|------|-----|------|
| 0% | 43612 | 43757 | 11522 |
| **10%** | **43562** | 43718 | 11699 |
| 25% | 43629 | **43709** ⭐ | 11657 |
| 50% | 43606 | 43734 | 12661 |
| 100% | 43713 | 43792 | 11826 |

### Key Findings:
1. ✅ Fast 2D-array LTM eliminates hash overhead
2. ✅ Partial LTM usage (10-50%) better than 0% or 100%
3. ✅ TSPA prefers 50% LTM, TSPB prefers 10-25% LTM
4. ✅ More generations maintained compared to hash-based LTM

### Recommendation:
Use **25% LTM ratio** as balanced default for both instances.

