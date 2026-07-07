# HZ11 Front-End Attribution L0

Status: COMPLETE. Attribution GO -- the largest single contributor is identified.

## Why this box

L0 (token/hash) measured fixed64 at tcmalloc*0.72. L1 (span/direct-index) at
tcmalloc*0.74. Classify method is NOT the lever (both plateau at tc*0.66-0.83).
The remaining gap must be in the overall front-end per-op cost. This box
DECOMPOSES that cost in instructions/op and cycles/op -- it does NOT optimize.

## What was measured

`scripts/run_hz11_frontend_attribution.sh` runs fixed16/64/256/4096 for glibc /
tcmalloc / mimalloc / hz10 / hz11-token / hz11-span.

```text
ops/s         : RUNS=3 median (wall-clock)
perf counters : perf stat -r 3 average (instructions, cycles, branch-misses,
                L1-dcache-load-misses)
derived       : perf average / ITERS  (instr/op, cycles/op, branch-miss/op, IPC)
objdump       : static instruction count per symbol (AUXILIARY -- LTO may merge)
```

## Measurement table (RUNS=3, ITERS=10M, fixed64 highlighted)

```text
row      allocator     ops/s        instr/op  cycles/op  brnch-ms/op  IPC
fixed64  glibc         108.4M        149.4      36.5       0.0092     4.09
         tcmalloc      185.5M         87.3      22.6       0.0255     3.86
         mimalloc      113.0M        112.1      33.7       0.0258     3.33
         hz10          117.3M        124.4      34.8       0.0102     3.57
         hz11-token    133.7M        129.4      29.5       0.0092     4.39
         hz11-span     139.1M        135.4      37.4       0.0097     3.62
```

All 4 rows (16/64/256/4096) show the same pattern: hz11-token ~129-130 instr/op,
hz11-span ~135-136 instr/op, tcmalloc ~87-90 instr/op. The gap is consistent
across sizes.

## The gap

```text
                 instr/op   cycles/op   IPC
tcmalloc           87.3       22.6      3.86   (the floor)
hz11-token        129.4       29.5      4.39   (+42 instr, +6.9 cycles)
hz11-span         135.4       37.4      3.62   (+48 instr, +14.8 cycles)
```

HZ11-token does **+42 instructions/op** vs tcmalloc. HZ11-token's IPC (4.39) is
HIGHER than tcmalloc's (3.86), so the cycles gap (+6.9) is proportionally
smaller than the instruction gap (+42). The bottleneck is instruction COUNT,
not instruction EFFICIENCY.

## objdump instruction counts (AUXILIARY)

```text
lib     malloc(wrapper)  hz11_malloc(body)  free(wrapper)  hz11_free(body)
token        3                 120                3              81
span         3                  86                3              99
```

The wrapper (malloc→hz11_malloc) is a 3-instruction tail-call (endbr64 + jmp).
This rules out H1 (wrapper/call-frame) as a major contributor.

## H1-H5 ranking (step-list analysis of the ~42 extra instr/op)

```text
H1 wrapper/call frame : ~6 instr/op  (3+3 tail-calls)        -- RULED OUT (negligible)
H2 TLS get            : ~6-8 instr/op (2x TLS load + guard)  -- moderate
H3 size-class table   : ~4-5 instr/op (1x table lookup)      -- minor
H4 stats/counters     : ~12-15 instr/op (4-5 increments/op)  -- LARGEST SINGLE CHUNK
H5 pop/push itself    : ~10-12 instr/op (2x array + count)   -- 2nd largest
```

The ~42 extra instructions are spread, but **H4 (counters) is the single largest
contributor** at ~12-15 instr/op (~30% of the gap). H5 (pop/push) is close at
~10-12. H1 is ruled out (3-instruction wrapper). H2/H3 are moderate/minor.

## Verdict: Attribution GO

The largest single contributor to the tcmalloc gap is **H4 (hot-path counter
increments)**: 4-5 `uint64_t` RMW per malloc+free pair, ~12-15 instr/op, the
biggest single chunk of the +42 instruction gap. These counters are diagnostic
(used only by `HZ11_DUMP_STATS=1` at exit), but they are ALWAYS compiled into
the hot path. A compile-time opt-out (the same pattern as HZ10's
`HZ10ShimThreadStatsCompileGate-L0`) should recover most of this chunk.

Secondary: H5 (pop/push layout, ~10-12 instr/op) is the next target if H4 alone
does not close enough.

## Next box

**HZ11StatsCompileGate-L1**: gate the hot-path counter increments
(`malloc_count`, `malloc_hit`, `free_count`, `token_hit`/`direct_hit_count`)
behind a compile-time flag (default OFF in the speed lane). Keep them ON only in
a diagnostic sibling build. Expected recovery: ~12-15 instr/op (~30% of the
+42 gap). Measure against the same fixed64 row; if it moves HZ11 toward
tcmalloc's 87 instr/op, the counters were the primary cost. If not, open
HZ11CacheShape-L1 (pop/push layout) next.

## NO_GO_LEDGER entry

Write ONLY the attribution conclusion (which hypothesis dominates + the numbers).
Do NOT mark HZ11StatsCompileGate-L1 as GO yet -- it is a SEPARATE box.
