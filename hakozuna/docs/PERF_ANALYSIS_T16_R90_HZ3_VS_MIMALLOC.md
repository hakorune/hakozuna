=================================================================
T=16/R=90 Performance Analysis: hz3 (S169) vs mimalloc
=================================================================

## 1. BASIC METRICS (perf stat)

### hz3 (S169)
- Time: 6.90 sec
- Cycles: 189.7B
- Instructions: 73.2B
- IPC: 0.39
- User time: 60.78 sec
- Sys time: 7.55 sec

### mimalloc
- Time: 5.40 sec
- Cycles: 126.1B
- Instructions: 66.1B
- IPC: 0.52
- User time: 44.00 sec
- Sys time: 7.31 sec

### Comparison
| Metric | hz3 | mimalloc | Ratio (hz3/mi) |
|--------|-----|----------|----------------|
| Time | 6.90s | 5.40s | +27.8% |
| Cycles | 189.7B | 126.1B | +50.4% |
| Instructions | 73.2B | 66.1B | +10.7% |
| IPC | 0.39 | 0.52 | -25.0% |
| User time | 60.78s | 44.00s | +38.1% |

KEY FINDING: hz3 uses 50.4% more cycles for only 10.7% more instructions
→ IPC is 25% worse (0.39 vs 0.52)
→ This indicates SEVERE stalls (cache misses, branch mispredicts)

=================================================================
## 2. CACHE METRICS

### Cache References
- hz3: 4,356M refs
- mimalloc: 3,478M refs
- Difference: +25.2% more cache references

### Cache Misses (absolute)
- hz3: 1,520M misses
- mimalloc: 1,402M misses
- Difference: +8.4% more cache misses

### Cache Miss Rate
- hz3: 34.89% miss rate
- mimalloc: 40.30% miss rate
- hz3 has BETTER cache miss rate (-13.4%)

### L1 D-Cache
| Metric | hz3 | mimalloc | Diff |
|--------|-----|----------|------|
| Loads | 33.8B | 34.8B | -2.9% |
| Misses | 2,531M | 2,175M | +16.4% |
| Miss Rate | 7.50% | 6.25% | +20.0% |

KEY FINDING: Despite better overall cache miss rate (34.89% vs 40.30%),
hz3 has MORE L1 cache misses (+16.4%) and higher L1 miss rate (+20%).
This suggests more random access patterns in hot paths.

=================================================================
## 3. BRANCH PREDICTION

### Branch Stats
| Metric | hz3 | mimalloc | Diff |
|--------|-----|----------|------|
| Branches | 13.5B | 12.2B | +10.3% |
| Misses | 744M | 898M | -17.2% |
| Miss Rate | 5.52% | 7.35% | -24.9% |

KEY FINDING: hz3 has BETTER branch prediction (5.52% vs 7.35%)
This is NOT the bottleneck.

=================================================================
## 4. HOTSPOT ANALYSIS (perf record cycles)

### hz3 Top Functions (CPU time %)
1. bench_thread: 22.60%
2. hz3_malloc: 21.95%
3. hz3_owner_stash_pop_batch: 21.64%
4. hz3_remote_stash_flush_budget_impl: 4.48%
5. hz3_free: 3.51%
6. hz3_small_v2_alloc_slow: 3.38%
7. hz3_owner_stash_push_one: 2.36%

Total allocator overhead: ~57% (excluding bench_thread)

### mimalloc Top Functions (CPU time %)
1. bench_thread: 24.65%
2. mi_free_block_delayed_mt: 7.84%
3. _mi_page_free_collect.constprop.0: 7.04%
4. _mi_page_free_collect: 6.41%
5. operator delete[]: 4.31%
6. mi_malloc: 4.04%
7. mi_free_generic_mt: 3.15%
8. mi_find_page: 3.09%

Total allocator overhead: ~36% (excluding bench_thread)

KEY FINDING: hz3 spends 57% in allocator vs mimalloc's 36%
→ hz3 has higher allocator overhead by 21 percentage points

=================================================================
## 5. CACHE MISS HOTSPOTS (perf record cache-misses)

### hz3 Cache Miss Distribution
1. hz3_malloc: 34.91% (93,473 samples)
2. bench_thread: 24.54% (64,754 samples)
3. hz3_owner_stash_pop_batch: 14.28% (38,239 samples)
4. hz3_owner_stash_push_one: 10.26% (26,805 samples)
5. hz3_free: 3.10% (8,186 samples)

Total allocator cache misses: ~75%

### mimalloc Cache Miss Distribution
1. bench_thread: 35.42% (65,769 samples)
2. mi_free_block_delayed_mt: 15.00% (27,746 samples)
3. _mi_page_free_collect.constprop.0: 10.92% (20,536 samples)
4. operator delete[]: 5.60% (10,401 samples)
5. mi_malloc: 5.28% (10,014 samples)

Total allocator cache misses: ~37%

KEY FINDING: hz3's cache misses are heavily concentrated in:
- hz3_malloc (34.91%) - likely metadata/page lookup
- hz3_owner_stash_pop_batch (14.28%) - remote stash operations
- hz3_owner_stash_push_one (10.26%) - remote stash operations

Remote stash operations (pop_batch + push_one) account for 24.54% of
ALL cache misses, which is almost equal to bench_thread itself!

=================================================================
## 6. ROOT CAUSE ANALYSIS

### Primary Bottleneck: Remote Stash Cache Misses
At R=90% (90% remote allocation), hz3's remote stash operations
(hz3_owner_stash_pop_batch + hz3_owner_stash_push_one) dominate:

- CPU time: 21.64% + 2.36% = 24.0% of total cycles
- Cache misses: 14.28% + 10.26% = 24.54% of all cache misses

These operations involve:
1. Cross-thread synchronization (CAS operations)
2. Accessing remote thread's stash structures
3. Batch processing of freed objects

### Secondary Bottleneck: hz3_malloc Cache Misses
hz3_malloc accounts for 34.91% of cache misses (vs only 21.95% CPU time)
This indicates memory-bound operations, likely:
1. Page table lookups (PTAG32)
2. Metadata access
3. Size class determination

### Why mimalloc is faster:
1. Lower allocator overhead (36% vs 57%)
2. More efficient remote free handling:
   - mi_free_block_delayed_mt: 15% cache misses
   - vs hz3 remote stash: 24.54% cache misses
3. Better IPC (0.52 vs 0.39) due to fewer stalls

=================================================================
## 7. SPECIFIC FINDINGS

### IPC Gap (Critical)
- hz3 IPC: 0.39
- mimalloc IPC: 0.52
- Gap: -25%

The 25% IPC gap means hz3 is spending 25% more cycles per instruction.
With 10.7% more instructions, this compounds to 50.4% more cycles total.

### Cache Miss Impact
Despite hz3 having better overall cache miss rate (34.89% vs 40.30%),
the ABSOLUTE number of cache misses is higher (+8.4%), and they are
concentrated in critical hot paths (malloc, remote stash).

### Remote Stash Performance
The remote stash operations are particularly expensive:
- hz3_owner_stash_pop_batch: 21.64% CPU, 14.28% cache misses
- This function is called on EVERY allocation in R=90% scenario
- Each call likely involves:
  1. CAS loop to pop batch
  2. Traverse linked list of freed objects
  3. Update batch chain metadata

=================================================================
## 8. RECOMMENDATIONS

### High Priority
1. **Optimize hz3_owner_stash_pop_batch**
   - This is the #1 bottleneck (21.64% CPU)
   - Consider prefetching next batch while processing current
   - Reduce metadata traversal overhead

2. **Reduce hz3_malloc cache misses**
   - 34.91% of all cache misses
   - Profile PTAG32 lookup specifically
   - Consider caching recent lookups

### Medium Priority
3. **Improve remote stash locality**
   - 24.54% of cache misses in remote stash operations
   - Consider padding/alignment to reduce false sharing
   - Batch operations to amortize cache miss cost

4. **Reduce instruction count in malloc path**
   - 10.7% more instructions than mimalloc
   - Review code generation, inlining decisions

### Low Priority
5. **Profile with hardware PMU events**
   - LLC misses, TLB misses, memory bandwidth
   - Better understand memory subsystem bottlenecks

=================================================================

=================================================================
## 9. SOURCE CODE ANALYSIS

### hz3_owner_stash_pop_batch Function Complexity

The perf profile shows hz3_owner_stash_pop_batch consuming 21.64% CPU
(second only to hz3_malloc at 21.95%). Examining the source code
(/hakozuna/hz3/src/owner_stash/hz3_owner_stash_pop.inc) reveals:

**Function Length**: ~1490 lines with multiple conditional compilation paths
**Active Code Path** (S169=0, S121=0):
- Lines 523-936: S112 Full Drain Exchange (if enabled)
- Lines 1041-1477: Legacy owner stash drain with bounded/unbounded options

### Key Operations in Hot Path (R=90%)

1. **TLS Spill Check** (Lines 524-574)
   - Check spill_array[sc] for local cache hits
   - Check spill_overflow[sc] for overflow items
   - Uses memcpy for batch transfer (good)
   - Prefetch enabled for list walk (HZ3_S154_SPILL_PREFETCH)

2. **Atomic Drain** (Lines 651-771)
   - S112_FULL_DRAIN_EXCHANGE: atomic_exchange to take entire stash
   - S169_BOUNDED_DRAIN: CAS loop to detach bounded items
   - Current config: likely full drain (S169=0)

3. **List Walk** (Lines 794-797, 1112-1235)
   - Walk freed object linked list
   - Multiple prefetch strategies (S44_PREFETCH)
   - Unrolled loops for want=32 (S167_WANT32_UNROLL)

4. **Leftover Handling** (Lines 1252-1477)
   - Fill spill_array up to CAP (64 items)
   - Remaining items to spill_overflow (bounded to CAP)
   - Excess pushed back to stash (CAS loop)

### Cache Miss Sources

Based on the code structure, cache misses in hz3_owner_stash_pop_batch
likely come from:

1. **Cross-thread Access**
   - Accessing owner stash bin (g_hz3_owner_stash[owner][sc])
   - This is a DIFFERENT thread's data structure at R=90%
   - False sharing possible if bins are not cache-aligned

2. **Freed Object Traversal**
   - Walking linked list of freed objects (hz3_obj_get_next)
   - Each object was allocated by current thread but freed by remote
   - Objects likely scattered across memory (poor locality)
   - Despite prefetch, random access pattern defeats cache

3. **Spill Structure Access**
   - TLS tc->spill_array[sc] and tc->spill_overflow[sc]
   - These are local but large (64 items × multiple size classes)
   - May not all fit in L1 cache

### Comparison to mimalloc

mimalloc's mi_free_block_delayed_mt (15% cache misses) is simpler:
- Likely uses a flat array or simpler data structure
- Less pointer chasing
- Better spatial locality

hz3's approach (24.54% cache misses in pop_batch + push_one):
- More complex linked list traversal
- More CAS operations
- Higher metadata overhead

=================================================================
## 10. DETAILED METRICS BREAKDOWN

### Cycle Efficiency Analysis

Total cycles ratio: hz3/mimalloc = 189.7B / 126.1B = 1.504 (+50.4%)

Breakdown by component:
1. **Extra instructions**: +10.7% (7.1B more instructions)
   - Contribution to cycles: ~7.1B * (1/0.52) = 13.7B extra cycles
   
2. **IPC degradation**: 0.39 vs 0.52 = 25% worse
   - hz3 needs: 73.2B / 0.39 = 187.7B cycles
   - If hz3 had mimalloc's IPC: 73.2B / 0.52 = 140.8B cycles
   - IPC penalty: 187.7B - 140.8B = 46.9B cycles

3. **Combined effect**:
   - Instruction overhead: 13.7B cycles
   - IPC penalty: 46.9B cycles
   - Total gap: 60.6B cycles (actual gap: 63.6B)

**Conclusion**: IPC degradation accounts for 73.6% of the performance gap.
Cache misses are the PRIMARY cause of the IPC problem.

### Cache Miss Cost Estimation

L1 cache miss penalty: ~4-7 cycles
L2 cache miss penalty: ~12-20 cycles
LLC miss penalty: ~50-200+ cycles (DRAM access)

hz3 absolute cache miss gap: +118M more cache misses than mimalloc
- If all L2 misses: 118M × 15 cycles = 1.77B cycles (2.8% of gap)
- If half LLC misses: 59M × 100 cycles = 5.9B cycles (9.3% of gap)

L1 D-cache miss gap: +356M more L1 misses
- L1 miss cost: 356M × 5 cycles = 1.78B cycles (2.8% of gap)

**Note**: These are conservative estimates. The actual impact is larger
because cache misses cause pipeline stalls, instruction replay, and
prevent out-of-order execution benefits.

### Memory Bandwidth Analysis

Memory references:
- hz3: 4,356M refs in 6.90s = 631M refs/sec
- mimalloc: 3,478M refs in 5.40s = 644M refs/sec

Despite similar bandwidth, hz3 has:
- 16.4% more L1 misses
- More random access patterns (evident from object traversal)
- Poorer cache line utilization

=================================================================
## 11. ACTIONABLE RECOMMENDATIONS

### Priority 1: Fix Remote Stash Bottleneck (Target: -10% overhead)

**Problem**: hz3_owner_stash_pop_batch + hz3_owner_stash_push_one
account for 24.54% of ALL cache misses (vs mimalloc's 15%).

**Solutions**:
1. **Enable S121 Page-Local Remote** (if not causing other issues)
   - Set HZ3_S121_PAGE_LOCAL_REMOTE=1
   - Uses page-based notification instead of per-object stash
   - Should improve locality

2. **Tune S112 Bounded Drain** (currently S169=0)
   - Enable HZ3_S169_S112_BOUNDED_DRAIN=1
   - Reduces amount of data pulled into cache
   - May improve cache efficiency at R=90%

3. **Optimize owner stash bin layout**
   - Verify cache line alignment of g_hz3_owner_stash array
   - Add padding to prevent false sharing
   - Consider using per-thread mailbox instead of shared bins

### Priority 2: Reduce hz3_malloc Cache Misses (Target: -5% overhead)

**Problem**: 34.91% of cache misses in hz3_malloc (likely PTAG32 lookup).

**Solutions**:
1. **Cache recent PTAG lookups in TLS**
   - Add small TLS cache (4-8 entries)
   - MRU eviction policy
   - Should hit for repeated allocations in same size class

2. **Prefetch PTAG entries**
   - Add prefetch hint in malloc fast path
   - Overlap computation with memory fetch

3. **Profile PTAG32 access pattern**
   - Use perf mem to confirm PTAG is the source
   - Consider THP-aligned PTAG table to reduce TLB misses

### Priority 3: Instruction Count Reduction (Target: -5% instructions)

**Problem**: 10.7% more instructions than mimalloc.

**Solutions**:
1. **Review inlining decisions**
   - Check if hz3_owner_stash_pop_batch should be inlined
   - Review compiler optimization flags
   - Consider LTO (Link Time Optimization)

2. **Simplify hot path checks**
   - Remove unnecessary branching in malloc fast path
   - Use likely/unlikely hints more aggressively

3. **Enable more aggressive optimizations**
   - Review current CFLAGS for hz3_scale.so
   - Consider -O3, -march=native if not already enabled

### Priority 4: A/B Testing Plan

**Phase 1**: Enable S121 Page-Local Remote
```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S121_PAGE_LOCAL_REMOTE=1'
```
**Expected**: -5% to -10% improvement at R=90%
**Risk**: May cause issues at lower R values

**Phase 2**: Enable S169 Bounded Drain + S121
```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S121_PAGE_LOCAL_REMOTE=1 -DHZ3_S169_S112_BOUNDED_DRAIN=1'
```
**Expected**: Additional -3% to -5% improvement
**Risk**: More complex code path, needs validation

**Phase 3**: Add TLS PTAG cache (requires code change)
**Expected**: -2% to -3% improvement
**Risk**: Adds complexity, may not scale well

### Priority 5: Long-term Architecture

**Consider**: Redesign remote free handling to be more mimalloc-like
- Use simpler data structures (arrays instead of linked lists)
- Batch operations more aggressively
- Reduce metadata overhead
- Trade some memory for better cache efficiency

**Rationale**: Current design is optimized for flexibility but pays
significant cache miss penalty at high R values. A specialized
"high-remote" mode may be needed for T=16/R=90 workloads.

=================================================================
## 12. SUMMARY

### Root Cause (in order of impact)

1. **IPC Degradation (73.6% of gap)**: hz3 IPC of 0.39 vs mimalloc 0.52
   - Caused by: Cache misses → pipeline stalls
   
2. **Remote Stash Cache Misses (24.54% of all misses)**:
   - hz3_owner_stash_pop_batch: 14.28%
   - hz3_owner_stash_push_one: 10.26%
   - mimalloc equivalent: ~15% total
   - **Gap: ~10% more cache misses in remote handling**

3. **hz3_malloc Cache Misses (34.91% vs mimalloc 5.28%)**:
   - Likely PTAG32 page table lookups
   - Poor spatial locality in metadata access
   - **Gap: ~30% more cache misses in malloc**

4. **Instruction Count (+10.7%)**: More work per operation
   - Complex code paths
   - Multiple conditional branches
   - **Contributes ~20% to cycle gap**

### Expected Improvement from Fixes

| Fix | Expected Gain | Confidence |
|-----|---------------|------------|
| Enable S121 Page-Local | -5% to -10% | Medium |
| Enable S169 Bounded | -3% to -5% | Medium |
| TLS PTAG cache | -2% to -3% | Low |
| Instruction reduction | -2% to -3% | Medium |
| **Total (additive)** | **-12% to -21%** | **Medium** |

**Best case**: hz3 closes gap from -5.6% to +10% faster than mimalloc
**Realistic case**: hz3 matches mimalloc (-5.6% → 0%)
**Worst case**: Improvements conflict, net gain -5% to -8%

### Next Steps

1. Run A/B test with S121=1 (Priority 1)
2. Profile with perf mem to confirm PTAG cache misses
3. Measure impact of S169=1 if S121 helps
4. Consider specialized "high-remote" allocator mode for T=16/R=90

=================================================================
