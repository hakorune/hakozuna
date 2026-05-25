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
| `hz5_large128_draintrust_r3_20260526_061614` | trusted owner-drain state transition diagnostic | t8/r50 signal, r90 regression; no promote |
| `hz5_large128_draintrust_budget1_manual_r3_20260526_062121` | draintrust + drain budget 1 manual check | t4/r50-only spike; no named lane |
| `hz5_large128_source16_draintrust_l0_compare_20260526_062558` | source16/draintrust L0 observation | draintrust changes refill pressure; row split remains |
| `hz5_large128_source16_draintrust_perf_20260526_062616` | t8 perf stat for source16/draintrust/tcmalloc | t8 gap is not pure instruction count |
| `hz5_large128_source16_draintrust_perf_t4_20260526_062634` | t4 perf stat for source16/draintrust/tcmalloc | t4 remains instruction/branch/refill sensitive |
| `hz5_large128_source16_draintrust_median_r3_20260526_062644` | RUNS=3 source16/draintrust/tcmalloc recheck | draintrust wins t8/r90, loses t8/r50; no broad promotion |

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

Latest source16/draintrust focused recheck:

```text
private/raw-results/linux/hz5_large128_source16_draintrust_median_r3_20260526_062644

t4/r50:
  tcmalloc              28.24M /  46MB
  hz5-large128-draintrust 15.18M /  50MB
  hz5-large128-source16   14.03M /  55MB

t4/r90:
  tcmalloc              25.55M /  56MB
  hz5-large128-draintrust 12.89M /  68MB
  hz5-large128-source16   10.06M /  90MB

t8/r50:
  tcmalloc              19.34M / 114MB
  hz5-large128-source16   13.18M / 116MB
  hz5-large128-draintrust 10.75M / 149MB

t8/r90:
  hz5-large128-draintrust 19.15M / 102MB
  tcmalloc              13.65M / 181MB
  hz5-large128-source16    8.32M / 229MB
```

Interpretation:

```text
Draintrust is not simply no-go anymore; it exposes a policy split.
Trusted owner-drain state transition can win t8/r90 with much lower RSS, but
the same choice hurts t8/r50 and still does not close t4 rows. Next work should
explain the row split before adding another default policy.
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
| `large128-draintrust` | `hz5_large128_draintrust_r3_20260526_061614` | diagnostic only; drain CAS is not broad fix |
| `draintrust+budget1` | `hz5_large128_draintrust_budget1_manual_r3_20260526_062121` | no named lane; t4/r50-only local optimum |

## Update Discipline

```text
1. Put detailed result narratives in archive docs or current_task.md.
2. Keep this file as a short index of active evidence.
3. Do not add speed-lane counter runs as performance claims.
4. Always include run root, allocator lane, compared baseline, and decision.
```
