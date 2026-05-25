# HZ5 Bench Results Index

This file keeps only the current benchmark index. Full historical notes are
archived at:

```text
hakozuna-hz5/docs/archive/HZ5_BENCH_RESULTS_INDEX_HISTORY_2026-05.md
```

Raw logs remain under:

```text
private/raw-results/linux/
```

## Current Focus Roots

| Root | Purpose | Decision |
| --- | --- | --- |
| `hz5_large128_t4r50_perf_current_20260526_053300` | perf stat/report for current large128 t4/r50 gap | gap is instruction/branch/refill/drain pressure, not cache misses alone |
| `hz5_large128_current_focus_r3_20260526_052851` | source16/r50-drain/directmap focused comparison | r50-drain is diagnostic; source16 remains comparison lane |
| `hz5_large128_rb_current_r3_20260526_053400` | rb32/rb64 recheck | no broad promotion |
| `hz5_large128_ownerfast_r3_20260526_053858` | ownerfast diagnostic | no-go |
| `hz5_large128_direct_header_recheck_r3_20260526_055156` | unsafe adjacent-header lookup recheck | free lookup alone is not t4/r50 fix |
| `hz5_large128_source_populate_r3_20260526_055548` | MAP_POPULATE/source pre-fault diagnostic | hard no-go |
| `hz5_large128_l0_compare_20260526_060606` | source16 versus r50-drain L0 observation | r50-drain republish churn is huge |
| `hz5_large128_drainbulk_tail_r3_20260526_061107` | bulk local-list owner-drain diagnostic | t4 signal, t8 regression; no promote |

## Current Large128 Baselines

```text
large128-rss:
  source batch4 saved low-RSS profile

large128-source16:
  source batch16 throughput comparison lane

tcmalloc:
  current target for large128/t4/r50
```

Key perf read from `hz5_large128_t4r50_perf_current_20260526_053300`:

| Allocator | ops/s | instructions | branches | cache misses | cycles |
| --- | ---: | ---: | ---: | ---: | ---: |
| HZ5 source16 | 13.98M | 250.4M | 54.2M | 1.92M | 288.0M |
| tcmalloc | 27.06M | 155.7M | 30.1M | 2.22M | 219.8M |

Interpretation:

```text
The active large128/t4/r50 gap is not primarily cache misses.
The next useful diagnostics should reduce LargeFront drain/refill path length
or prove that page-fault/source pressure is the remaining bound.
```

## Recently Closed Diagnostics

| Diagnostic | Root | Decision |
| --- | --- | --- |
| `large128-direct-header` | `hz5_large128_direct_header_recheck_r3_20260526_055156` | unsafe no-promote; does not solve t4/r50 |
| `source-populate` | `hz5_large128_source_populate_r3_20260526_055548` | hard no-go; RSS explosion |
| `ownerfast` | `hz5_large128_ownerfast_r3_20260526_053858` | no-go versus source16 |
| `base-directmap` | `hz5_large128_base_directmap_r3_20260526_051731` | helps some rows, loses broad t8 |
| `r50-drain-directmap` | `hz5_large128_drain_directmap_r3_20260526_053047` | no-go composition |
| `large128-drainbulk` | `hz5_large128_drainbulk_tail_r3_20260526_061107` | diagnostic only; local-push batching is not broad fix |

## Update Discipline

```text
1. Put detailed result narratives in archive docs or current_task.md.
2. Keep this file as a short index of active evidence.
3. Do not add speed-lane counter runs as performance claims.
4. Always include run root, allocator lane, compared baseline, and decision.
```
