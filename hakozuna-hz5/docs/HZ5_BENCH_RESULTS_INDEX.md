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
| `hz5_large128_transfer128_smoke_r3_20260526_063953` | first LargeFront transfer128 diagnostic | t4/r50 signal, t8 regression |
| `hz5_large128_transfer128_flushmiss_r3_20260526_064056` | transfer128 with alloc-miss TLS flush | t4/r50 still improves, t8 still no-go |
| `hz5_large128_transfer128_tlsfirst_smoke_r1` | transfer128 with TLS-local reuse before global cache | no-go; producer-local retention hurts t4/t8 throughput and RSS |
| `hz5_large128_transfer128_ownershard_r3` | transfer128 routed by old owner-slot shard | no-go; owner shard loses t4/r50 and r90, only partial high-thread signal |

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

Transfer128 diagnostic:

```text
private/raw-results/linux/hz5_large128_transfer128_flushmiss_r3_20260526_064056

t4/r50:
  tcmalloc                29.82M /  42MB
  hz5-large128-transfer128 22.81M /  16MB
  hz5-large128-draintrust  17.22M /  47MB
  hz5-large128-source16    17.07M /  49MB

t8/r50:
  tcmalloc                21.48M / 106MB
  hz5-large128-source16    17.82M /  92MB
  hz5-large128-transfer128 13.23M /  69MB

t8/r90:
  hz5-large128-source16    16.75M / 108MB
  tcmalloc                13.27M / 166MB
  hz5-large128-transfer128 10.14M /  85MB
```

Decision:

```text
Class-level transfer is a real t4/r50 lever and preserves very low RSS, but the
single global transfer cache is not a broad replacement. Keep transfer128 as an
L9 diagnostic. If continued, the next variant needs less global contention or a
thread/owner-aware transfer cache, not just another policy switch.
```

Transfer128 TLS-first smoke:

```text
private/raw-results/linux/hz5_large128_transfer128_tlsfirst_smoke_r1

t4/r50:
  tcmalloc                         30.60M /  45MB
  hz5-large128-transfer128         22.46M /  16MB
  hz5-large128-transfer128-tlsfirst 10.33M /  79MB

t8/r50:
  tcmalloc                         23.35M / 111MB
  hz5-large128-source16            20.17M /  87MB
  hz5-large128-transfer128-tlsfirst  7.32M / 195MB

t8/r90:
  hz5-large128-source16            15.56M / 121MB
  tcmalloc                         14.32M / 169MB
  hz5-large128-transfer128-tlsfirst  5.19M / 282MB
```

Decision:

```text
TLS-first is no-go. Holding TRANSFER_FREE spans producer-local prevents the
consumer side from seeing them and increases retention. Keep the lane as
diagnostic evidence; do not promote.
```

Transfer128 owner-shard follow-up:

```text
private/raw-results/linux/hz5_large128_transfer128_ownershard_r3

t4/r50:
  tcmalloc                          26.17M /  56MB
  hz5-large128-transfer128          22.54M /  15MB
  hz5-large128-source16             16.54M /  51MB
  hz5-large128-transfer128-ownershard 10.55M / 65MB

t8/r50:
  tcmalloc                          23.75M /  95MB
  hz5-large128-source16             20.89M /  80MB
  hz5-large128-transfer128          12.93M /  73MB
  hz5-large128-transfer128-ownershard 10.66M / 118MB

t8/r90:
  tcmalloc                          12.62M / 196MB
  hz5-large128-transfer128           8.76M / 130MB
  hz5-large128-source16              8.06M / 245MB
  hz5-large128-transfer128-ownershard  6.42M / 227MB
```

Decision:

```text
Owner-shard transfer is no-go. Routing by old owner slot preserves neither the
t4/r50 transfer128 win nor source16's r90 behavior. The remaining transfer
candidate would need a consumer-visible queue with lower fixed cost than owner
inbox and less retention than TLS/owner shards.
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
| `large128-transfer128-tlsfirst` | `hz5_large128_transfer128_tlsfirst_smoke_r1` | no-go; TLS-local transfer starves consumers and worsens RSS |
| `large128-transfer128-ownershard` | `hz5_large128_transfer128_ownershard_r3` | no-go; owner-slot shard loses transfer128/source16 strengths |

## Update Discipline

```text
1. Put detailed result narratives in archive docs or current_task.md.
2. Keep this file as a short index of active evidence.
3. Do not add speed-lane counter runs as performance claims.
4. Always include run root, allocator lane, compared baseline, and decision.
```
