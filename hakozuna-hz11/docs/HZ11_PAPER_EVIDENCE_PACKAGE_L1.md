# HZ11PaperEvidencePackage-L1

Status: paper-ready consolidation. No new measurements. No policy change. No
default promotion. Every number below cites its source doc and condition.

This package fixes what HZ11 has established in a collapse-proof form: the lane
taxonomy, the two evidence tables, the negative-result ladder, the claim boundary,
and the next-work warrant. It is the companion to `HZ11PaperPositioning-L1.md`
(which states the line); this one carries the evidence and the limits.

## Thesis (approved wording)

```text
HZ11/fine128 is a boxed speed-first allocator line with near-parity-or-better
wall time on 5/6 macro rows (0.990x-1.151x), major RSS wins on several rows,
strong remote/mixed microbench results, and one honest open sh6bench pathology.
```

## 1. Final Lane Taxonomy

```text
fine128        (libhz11_span_transfer_thread_exit_cap_batch32_fine128.so):
                 recommended opt-in macro speed-lane candidate. NOT default.
span-transfer  (libhz11_span_transfer.so):
                 clean remote/mixed microbench speed lane.
global fineclass: sh6bench RSS research lane only. Not a general candidate.
batch32        (the fine128 transfer base):
                 intermediate attribution/candidate step, not the final lane.
locked percpu  (libhz11_span_transfer_thread_exit_cap_batch32_fine128_rseq.so):
                 NO-GO diagnostic; in-tree for reproducibility, not a candidate.
default allocator path: unchanged.
```

HZ8/HZ10/HZ11 roles remain distinct (HZ8 = recommended low-RSS public line; HZ10 =
speed/RSS-aware research with route/ownership/reclaim; HZ11 = speed-first tcmalloc
competitor). See `HZ11PaperPositioning-L1.md`.

## 2. Macro Evidence - fine128 vs tcmalloc

Source: `HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md` (macro gate medians,
RUNS=5, fine128 vs tcmalloc).

| workload | wall ratio | max-RSS ratio |
|---|---:|---:|
| python_alloc | 1.000 | 0.823 |
| mstress | 1.151 | 1.087 |
| larson | 0.997 | 0.979 |
| sh6bench | 10.120 | 1.247 |
| xmalloc_test | 0.990 | 0.094 |
| cache_scratch | 1.012 | 0.474 |

Read:

```text
near-parity-or-better on wall on 5/6 macro rows (0.990x-1.151x).
  (mstress 1.151x is a small loss; say "near-parity or better", never "parity".)
major RSS wins on 4/6 rows, two large:
  xmalloc_test   0.094x  (~10.6x less RSS than tcmalloc)
  cache_scratch  0.474x  (~2.1x  less RSS than tcmalloc)
  python_alloc   0.823x
  larson         0.979x
the one open wall pathology is sh6bench: 10.120x tcmalloc wall, 1.247x max RSS.
```

## 3. Remote/Mixed Microbench Evidence

Source: `HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md` + raw
`bench_results/hz11_transfer_promotion_20260708T091747Z/summary.md`
(RUNS=10, THREADS=16, ITERS=100000). This is a synthetic transfer matrix, not a
macro/app claim.

fine128 vs tcmalloc:

| row | ops ratio | post-RSS ratio |
|---|---:|---:|
| main_local0 | 1.001 | 0.246 |
| main_r50 | 1.853 | 0.577 |
| main_r90 | 2.346 | 0.457 |
| small_remote90 | 1.070 | 0.242 |
| medium_r50 | 4.590 | 0.446 |
| medium_r90 | 6.097 | 0.398 |

```text
remote/mixed rows reach 1.85x-6.10x throughput at 0.40x-0.58x RSS;
main_local0 is near parity (1.001x).
```

fine128 vs `hz11-span-transfer` (why span-transfer stays the remote/mixed lane):

| row | ops ratio | post-RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 1.081 | 1.167 | 1.814 |
| main_r50 | 1.013 | 1.044 | 1.076 |
| main_r90 | 0.957 | 1.114 | 1.189 |
| small_remote90 | 0.988 | 1.171 | 1.124 |
| medium_r50 | 0.929 | 1.261 | 1.414 |
| medium_r90 | 0.918 | 1.281 | 1.264 |

```text
fine128 is ~parity on the main rows but regresses on the medium rows
(0.918x-0.929x ops, 1.26x-1.28x RSS). So fine128 is the right combined macro
candidate, while span-transfer remains the cleaner remote/mixed-only lane.
```

## 4. Negative-Result Ladder

Each row reports the source doc's own value (with its condition) and the
interpretation, kept separate. The sh6bench-tcmalloc ratios across boxes come from
**different gate boxes / candidates under their own conditions** - they are not a
controlled ranking.

| item | source-doc value (condition) | interpretation / rules out | doc |
|---|---|---|---|
| span-return | sh6bench wall 17.805s vs span-transfer 4.562s (3.9x regression); fixed python_alloc/mstress aborts but larson RSS 2.348x, sh6bench max RSS 1.314x | NO-GO macro. Rules out central-freelist span return with transfer-cache metadata accounting as a macro RSS fix (metadata-lock traffic). | `docs/no_go/HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md` |
| xferwide | sh6bench wall 3.627s (from 4.542s), still 10.019x tcmalloc; central spill ~25.5M -> 0; RSS 1.318x; spans ~16.7K | NO-GO macro (GO attribution). Rules out wider transfer capacity alone as sufficient; the wall lever is locality, not batch width. | `docs/HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1.md` |
| batch granularity | in its own box: batch32 3.506s (9.904x), batch64 5.501s, batch128 9.312s; xfer_insert batch16 502M -> batch128 2,097M | NO-GO macro (GO attribution). Rules out widening batch beyond 32; batch32 is the smallest useful candidate. (Different box/condition from fine128's 10.120x - not a direct ranking.) | `docs/HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1.md` |
| span reuse | span_create 16753 vs 16755; reuse rate 0.394% | NO-GO RSS fix. Rules out central-only full-span reuse (the "all objects of a span in central" condition is too rare on sh6bench). | `docs/no_go/HZ11_SH6BENCH_SPAN_REUSE_POLICY_L1.md` |
| nohuge | max RSS 351616 KiB vs 351104; wall 3.522 vs 3.508; minor faults 88168 vs 88218 | NO-GO RSS fix. Rules out MADV_NOHUGEPAGE / simple arena madvise as the RSS lever; ~1.0 GiB carved vs ~350 MiB resident is not THP/eager-commit. | `docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1.md` |
| global fineclass | vs span-transfer: main_local0 0.893x, main_r50 0.912x, medium_r50 0.903x ops; small_remote90 post RSS 2.906x | NO-GO general. Rules out global fineclass as a general/default candidate; keep sh6bench-RSS research only. | `docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1.md` |
| locked percpu | sh6bench wall fine128 3.527s -> rseq-locked 12.917s (3.66x regression); xfer_insert 868,988,121 -> 757,752,618 (12.8% fewer) | NO-GO. Lock overhead killed the locality benefit even though the slab absorbed traffic. Rules out a locked per-CPU layer. | `docs/HZ11_PERCPU_RSEQ_CACHE_PROTOTYPE_L2.md` |

### sh6bench open-gap spine

Each value is a different gate box / candidate under its own conditions (NOT a
controlled ranking). Source: `HZ11_NO_GO_LEDGER.md` + reclassify doc.

```text
12.71x (transfer) -> 13.16x (thread-exit) -> 12.85x (thread-exit-cap)
  -> 9.93x (batch32) -> 10.120x (fine128)
```

The sh6bench wall gap has moved from ~13x to ~10x across the lane maturation but
never approached tcmalloc. The two documented levers (transfer/central volume;
created span/page footprint) remain open; the cheap attempts above did not close it.

## 5. Claim Boundary

Allowed:

```text
- near-parity-or-better wall on 5/6 macro rows (0.990x-1.151x) vs tcmalloc;
  major RSS wins on several rows; one open sh6bench pathology.
- remote/mixed microbench: 1.85x-6.10x throughput at 0.40x-0.58x RSS vs tcmalloc
  (main_local0 near parity), scoped to the transfer matrix.
- fine128 is the recommended opt-in HZ11 macro candidate; span-transfer the
  clean remote/mixed lane.
```

Not allowed:

```text
- HZ11 generally beats tcmalloc.
- fine128 is the default allocator (or any lane is).
- sh6bench is solved, or per-CPU/rseq fixes it.
- locked per-CPU locality improves sh6bench (it regressed 3.66x).
- HZ11 replaces HZ8 or HZ10.
```

These are a strict superset of `HZ11PaperPositioning-L1.md` and
`HZ11_FINE128_CANDIDATE_POSITIONING_L1.md`; nothing is loosened.

## 6. Next Technical Warrant

```text
- Lock-free single-commit rseq CS: only if a NEW cheap justification appears.
  The locked per-CPU test did not justify it (sh6bench regressed); lock-free
  locality remains unproven and is not pursued without new warrant.
- Pageheap release / decommit: future RSS discipline. This is a SEPARATE lever
  from the sh6bench wall gap - it targets the created span/page footprint
  (span_reuse ~0.4%, ~1.0 GiB carved vs ~350 MiB resident on sh6bench). It may
  hurt speed on a speed-first line, so it needs its own macro evidence.
- Real-application workloads (server / compile / DB): required before any public
  claim. Every current bench is synthetic (micro/stress); there is zero real-app
  evidence today.
- Platform/COA coverage beyond this single Linux x86-64 machine, or an explicit
  scope stamp on every public claim.
```

The line's current contribution is articulable and bounded; the next investment
is gated by new evidence, not by optimism.

## Provenance

```text
Every numeric value above is quoted from an existing doc (cited per row/section).
This package introduces no new measurement and no new claim. If a source doc is
later revised, this package must be re-checked against it.
```
