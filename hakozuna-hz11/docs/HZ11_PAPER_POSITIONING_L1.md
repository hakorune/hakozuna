# HZ11PaperPositioning-L1

Superseded for current public positioning by
`HZ11_FINE128_REAL_APP_POSITIONING_L1.md`. This file remains the historical paper
positioning snapshot before the later capacity, real-app, and rocksdb correctness boxes.

Status: GO for positioning. This is the current HZ11 line statement. No
allocator policy change. No default promotion. Per-CPU/rseq is **not started**.

## Box

Convert the HZ11 / fine128 evidence chain into a single publishable research
narrative, and decide whether per-CPU/rseq is warranted as the next technical
box. This is a documentation/positioning box.

Boundary:

```text
documentation and positioning only
no allocator policy changes
no default path change
no new performance claim beyond the documented lane scope
no new benchmark numbers (every number cites an existing source)
per-CPU/rseq not started
```

## Contribution Statement (thesis)

```text
HZ11/fine128 is a boxed speed-first allocator line with near-parity or better
wall time on 5/6 macro rows, major RSS wins on several rows, and one honest
open sh6bench pathology.
```

HZ11 is not a faster HZ8/HZ10 variant and not a generic tcmalloc replacement.
It deliberately explores a tcmalloc-shaped allocator (front-end caches, transfer
cache, central spans, pageheap) as a speed-first research line, with the
discipline of Box Theory: every optimization is a scoped, reversible,
compile-time-flagged experiment, and failed boxes are recorded in
`HZ11_NO_GO_LEDGER.md` so they are not retried without new evidence.

The differentiator is not a novel architecture. It is a defensible tradeoff
profile, reached evidence-first:

- **Macro gate**: near-parity or better wall time vs tcmalloc on 5 of 6 rows,
  with major RSS wins on several, and one open pathology (sh6bench).
- **Remote/mixed microbench**: HZ11 dominates tcmalloc on both throughput and
  RSS (see evidence), which is why a separate remote/mixed lane is retained.
- **Methodology**: boxed lanes, a failed-box ledger, and an evidence-gated
  positioning rule that redefines a gate transparently rather than hiding a
  miss.

## Evidence

Every number below is cited verbatim from an existing source. This box
introduces no new measurements.

### Macro gate — fine128 vs tcmalloc (medians)

Source: `HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md`, itself from
`bench_results/hz11_macro_speed_lane_20260708T083138Z/summary.md`.

| workload | tcmalloc wall | fine128 wall | wall ratio | tcmalloc max RSS KiB | fine128 max RSS KiB | max RSS ratio |
|---|---:|---:|---:|---:|---:|---:|
| python_alloc | 0.032 | 0.032 | 1.000 | 7936 | 6528 | 0.823 |
| mstress | 0.192 | 0.221 | 1.151 | 217088 | 235996 | 1.087 |
| larson | 4.152 | 4.140 | 0.997 | 278912 | 273024 | 0.979 |
| sh6bench | 0.349 | 3.532 | 10.120 | 259584 | 323712 | 1.247 |
| xmalloc_test | 2.047 | 2.027 | 0.990 | 195456 | 18432 | 0.094 |
| cache_scratch | 1.163 | 1.177 | 1.012 | 7296 | 3456 | 0.474 |

Read:

```text
5/6 macro rows are near-parity or better on wall (0.990x - 1.151x).
mstress wall is 1.151x: a small loss, so the claim is "near-parity or better",
  not unconditional parity.
RSS wins on 4/6 rows, two of them large:
  xmalloc_test   0.094x  (~10.6x less RSS than tcmalloc)
  cache_scratch  0.474x  (~2.1x  less RSS than tcmalloc)
  python_alloc   0.823x
  larson         0.979x
The single open pathology is sh6bench: 10.120x wall, 1.247x max RSS.
```

### Remote/mixed microbench — RUNS=10 transfer promotion matrix

Source: ratios computed from the raw
`bench_results/hz11_transfer_promotion_20260708T091747Z/summary.md`
(Summary and Extra Allocator Tradeoffs tables), interpreted in
`HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md`.
Conditions: RUNS=10, THREADS=16, ITERS=100000.

fine128 vs tcmalloc (this is the scoped remote/mixed microbench win):

| row | ops ratio | post RSS ratio |
|---|---:|---:|
| main_local0 | 1.001 | 0.246 |
| main_r50 | 1.853 | 0.577 |
| main_r90 | 2.346 | 0.457 |
| small_remote90 | 1.070 | 0.242 |
| medium_r50 | 4.590 | 0.446 |
| medium_r90 | 6.097 | 0.398 |

fine128 vs `hz11-span-transfer` (why span-transfer stays the remote/mixed lane):

| row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 1.081 | 1.167 | 1.814 |
| main_r50 | 1.013 | 1.044 | 1.076 |
| main_r90 | 0.957 | 1.114 | 1.189 |
| small_remote90 | 0.988 | 1.171 | 1.124 |
| medium_r50 | 0.929 | 1.261 | 1.414 |
| medium_r90 | 0.918 | 1.281 | 1.264 |

Read:

```text
HZ11 dominates tcmalloc on remote/mixed microbench: 1.85x - 6.1x throughput at
0.40x - 0.58x RSS. This is microbench-scoped (synthetic transfer matrix), not a
macro/app claim.
fine128 is acceptable as the combined macro candidate, but it is NOT the cleanest
remote/mixed lane: vs span-transfer it regresses 0.918x - 0.929x ops and
1.26x - 1.28x RSS on medium rows. So span-transfer stays the remote/mixed lane.
```

## Honest sh6bench gap

sh6bench is the one open pathology and is **not solved**.

```text
fine128 sh6bench wall: 3.532s vs tcmalloc 0.349s  -> 10.120x tcmalloc wall
fine128 sh6bench max RSS: 1.247x tcmalloc (inside the 1.25x guard)
```

Path-cost attribution localizes the wall gap, not to a single knob:

```text
HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1:
  active blocker is transfer/central path volume plus created span/page footprint
  (central count drains to zero; cap pressure is not the active blocker)

HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1:
  a wider transfer batch/cap (xferwide) eliminates central spill (~25.5M -> 0)
  and improves wall ~20% (4.542s -> 3.627s), but still leaves 10.019x tcmalloc.
  Wider transfer batches alone do not close the gap.

HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1:
  batch32 is selected as the smallest useful wall candidate; wider batches regress.

HZ11_SH6BENCH_REQUEST_SIZE_CLASS_PACKING_OBSERVATION_L1:
  sh6bench RSS gap is attributed primarily to power-of-two class-packing waste;
  selective fine128 size classes reduce sh6bench RSS while keeping batch32 wall
  roughly flat.
```

sh6bench is the multi-threaded allocator-stress row. It is exactly the workload
a per-CPU cache serves locally, which is why per-CPU/rseq is the leading next
technical hypothesis (see Next-Work Decision). It is not a public performance
claim, and the HZ11 macro gate is deliberately permissive on absolute sh6bench
wall (it gates on RSS guard + internal non-regression + correctness, not on
parity with tcmalloc wall).

## Lane Taxonomy

```text
fine128   (libhz11_span_transfer_thread_exit_cap_batch32_fine128.so):
            recommended opt-in macro speed-lane candidate. NOT default.

span-transfer (libhz11_span_transfer.so):
            clean remote/mixed microbench speed lane.

global fineclass:
            sh6bench RSS research only. Not a general candidate.

batch32   (libhz11_span_transfer_thread_exit_cap_batch32_fine128's batch base):
            intermediate attribution/candidate step, not the final lane.

default allocator path: unchanged.
```

Why each lane exists (evidence chain):

```text
span-transfer   -> HZ11_TRANSFER_PROMOTION_MATRIX_L1: GO remote/mixed microbench lane
batch32         -> HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1: sh6bench wall lever
fine128         -> selective fine size classes; HZ11_SELECTIVE_FINECLASS_RANGE_L1
                   selected fine128 over fine256/global fineclass
global fineclass-> HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1: material remote/mixed
                   tradeoffs keep it research-only
```

## Claim Boundary

Allowed:

```text
fine128 is the recommended opt-in HZ11 macro speed-lane candidate, backed by
macro-gate, current-RSS, and remote/mixed evidence.

span-transfer remains the clean HZ11 remote/mixed microbench speed lane.

HZ11 reaches near-parity or better wall time vs tcmalloc on 5/6 macro rows,
with major RSS wins on several rows, and one open sh6bench pathology.

HZ11 dominates tcmalloc on remote/mixed microbench throughput and RSS
(scoped to the transfer promotion matrix, not a macro/app claim).
```

Not allowed (do-not-claim guardrails):

```text
HZ11 generally beats tcmalloc.
fine128 is the default allocator.
fine128 supersedes span-transfer on remote/mixed microbench rows.
global fineclass is a general candidate.
batch32 is the final candidate.
sh6bench is solved.
per-CPU/rseq is guaranteed to fix the sh6bench gap.
```

These are a strict superset of the guardrails in
`HZ11_FINE128_CANDIDATE_POSITIONING_L1.md`; no prior claim is loosened.

## Next-Work Decision (per-CPU/rseq)

per-CPU/rseq (prospective box `HZ11PerCpuRseqCache-L2`) is **the leading next
technical hypothesis** for the sh6bench gap. It is **not** committed, and it is
**not** asserted to fix the gap.

Rationale for the hypothesis (not proof):

```text
- sh6bench is the multi-threaded allocator-stress row, the workload a per-CPU
  cache serves locally.
- HZ11 currently has NO per-CPU/rseq cache (verified: zero rseq / sched_getcpu /
  per-cpu references in src/ or include/). It is pthread / thread-cache shaped.
- xferwide proved that widening the transfer cache alone leaves sh6bench at
  10.019x, so the lever is locality, not batch width.
```

Why it is deferred, not started:

```text
- It is a large, Linux-specific L2 design jump from scratch (only HZ3 legacy
  hz3_cpu_rrq.c exists, and it is the wrong architecture for HZ11's transfer /
  span-class design).
- The contribution is already articulable without it; the positioning box must
  define the claim, the gap, and the exact warrant before sinking L2 cost.
```

Exact gate per-CPU/rseq must earn itself with before adoption:

```text
1. sh6bench wall closed to a defensible factor vs tcmalloc (target <= ~1.5x).
2. No regression on the 5/6 near-parity macro rows (wall within the existing
   gate shape; max RSS inside the 1.25x guard).
3. No remote/mixed regression vs span-transfer on the RUNS=10 transfer matrix.
4. A documented rollback for the per-CPU path, mirroring the default-promotion
   precondition (see HZ11_FINE128_CANDIDATE_POSITIONING_L1.md "Next Step").
```

This box decides whether B is justified; it does not commit to B.

## NO_GO Cross-Reference Index

Each public claim above is bounded by these ledger entries. Do not relax a claim
without re-opening the entry that bounds it.

```text
Claim: "near-parity or better on 5/6 macro rows"
  bounded by: HZ11_NO_GO_LEDGER.md macro-gate entries (sh6bench wall never near
  parity: 13.048x -> 13.161x -> 12.848x -> ~12.7x -> 10.019x -> 9.929x as the
  lane improved; still 10.120x for fine128).

Claim: "sh6bench is the open pathology, not solved"
  bounded by: HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1,
              HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1 (xferwide -> 10.019x),
              docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1 (MADV_NOHUGEPAGE
              is not the RSS/page lever).

Claim: "fine128 is opt-in, not default"
  bounded by: HZ11_FINE128_CANDIDATE_POSITIONING_L1 (rollback + claim wording
  must be defined before any default-path change; rollback is currently absent),
              current_task.md "Do not do yet".

Claim: "global fineclass is research-only"
  bounded by: docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1,
              docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.

Claim: "fine128 macro gate GO"
  bounded by: docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1 (original MIXED under
  the single-sample current-RSS rule) superseded by
  HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1 under
  HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1 (current RSS is a hard fail only with
  focused RUNS>=10 stability evidence). The reclassification is a documented,
  transparent gate-semantics change, not hidden evidence.

Claim: "span-transfer stays the remote/mixed lane"
  bounded by: HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1 (fine128 regresses
  0.918x-0.929x ops, 1.26x-1.28x RSS vs span-transfer on medium rows).
```

## Checks Required Before Stronger Public Claims

(Carried forward from the review; none are satisfied today.)

```text
1. sh6bench wall closed to a defensible factor (target <= ~1.5x tcmalloc). [10.120x]
2. Rollback path defined and tested (currently absent; precondition per
   current_task.md and HZ11_FINE128_CANDIDATE_POSITIONING_L1).
3. Current-RSS gate rule independently replicated with a pre-registered, fixed
   threshold (the rule and its clearing diagnostic are the line's own).
4. At least one real-application macro workload (server / compile / DB), not
   synthetic micro/stress benches. [none today]
5. Platform/COA coverage beyond a single Linux config, or an explicit
   "Linux x86-64, single-socket" scope on every public claim. [single machine]
```

## Status of This Box

```text
GO for positioning. This doc is the current HZ11 line statement.
No allocator policy changes. No default promotion. Per-CPU/rseq not started.
Next technical box is decided, not committed: per-CPU/rseq must earn itself
with the gate above.
```
