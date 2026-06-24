# HZ8 Medium Active Empty Fast Path A/B

Status: evidence-only / candidate reverted.

```text
baseline:
  0388eff0 Attribute HZ8 medium collect cadence

candidate:
  active-hit allocation tried to handle active-empty LIVE runs before the
  generic h8_medium_mark_live_on_alloc() path

candidate_record:
  bench_results/hz8_medium_active_empty_fast_ab_20260624T190519Z/

baseline_record:
  bench_results/hz8_medium_active_empty_fast_ab_20260624T190541Z_base/
```

Rows are R3 debug/audit rows, not final promotion runs.

## Median Throughput

```text
medium_i0:
  baseline   11.08M
  candidate  10.21M
  ratio       0.922

medium_r50:
  baseline    8.22M
  candidate   8.15M
  ratio       0.992

main_i0:
  baseline   12.82M
  candidate  11.89M
  ratio       0.927

fixed16:
  baseline   12.04M
  candidate  10.89M
  ratio       0.904
```

## Decision

```text
MediumActiveEmptyAllocFastPath-L1:
  NO-GO in this shape

reason:
  the added branch/code shape regressed the local active-hit rows
  the active-empty condition is real, but this implementation did not make it
  cheaper
```

## Follow-Up

```text
keep:
  active slot mutation attribution counters

next:
  inspect generated code / residual active alloc/free slot mutation
  avoid adding another pre-branch around h8_medium_mark_live_on_alloc()
```
