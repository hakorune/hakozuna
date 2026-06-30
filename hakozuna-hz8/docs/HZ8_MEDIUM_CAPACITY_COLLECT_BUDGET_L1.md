# HZ8 MediumCapacityCollectBudget L1

Status:

```text
experimental behavior lane
default unchanged
```

## Why This Box Exists

`medium_interleaved_r50` and `main_interleaved_r90` show large medium collect
work. In the current allocator, the capacity-miss fallback drains owner pending
with an unbounded budget:

```text
capacity miss -> collect pending with SIZE_MAX
```

That is simple and safe, but the diagnostic rows show high collect work:

```text
medium_collect_source capacity is large
medium_residual_budget collect_ms is large
```

## Behavior

The lane is guarded by:

```text
H8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1
```

When enabled, the capacity-miss collection budget is finite:

```text
H8_MEDIUM_CAPACITY_COLLECT_BUDGET_VALUE = 16
```

Periodic collection, owner-exit collection, and small collection behavior are
unchanged.

## Bench Target

```text
make bench-mediumcapacitybudget
./h8_bench_mediumcapacitybudget ...
```

## Acceptance

```text
medium_interleaved_r50:
  throughput improves
  collect_ms drops
  RSS does not rise materially

main_interleaved_r90:
  no material regression

small_interleaved_remote90:
  not expected to move
```

No-go:

```text
capacity backlog causes RSS growth
active miss create/global reuse rises materially
medium throughput falls
safety counters become non-zero
```

## R3 Result

Record:

```text
bench_results/hz8_medium_capacity_budget_l1_20260630T100420/
```

Compared with the preceding L2 diagnostic:

```text
small_interleaved_remote90:
  throughput 478718 -> 517407 ops/s
  peak RSS 851.72 -> 853.91 MiB
  small collector unchanged in kind

main_interleaved_r90:
  throughput 202472 -> 295587 ops/s
  peak RSS 39.79 -> 32.91 MiB
  medium collect_ms 66098 -> 48165
  medium attributed_ms 198133 -> 133393

medium_interleaved_r50:
  throughput 344074 -> 346596 ops/s
  peak RSS 20.79 -> 22.52 MiB
  medium collect_ms 23939 -> 23440
```

Read:

```text
capacity-miss SIZE_MAX collect was too expensive on mixed main rows
finite capacity budget improves main_interleaved_r90 without obvious safety noise
medium_interleaved_r50 is roughly neutral in this short R3
small movement should be treated cautiously because the small path does not use this knob
```

Decision:

```text
KEEP as promising candidate
next check should be repeat-5 on main_interleaved_r90 and medium_interleaved_r50
do not promote before repeat-5 / guard rows
```

## R5 / Guard Check

Record:

```text
bench_results/hz8_medium_capacity_budget_l1_r5_20260630T100542/
```

Candidate medians:

```text
main_interleaved_r90:
  246696 ops/s
  peak RSS 33.79 MiB

medium_interleaved_r50:
  373551 ops/s
  peak RSS 24.26 MiB

guard_local0:
  7.16M ops/s
  peak RSS 9.35 MiB

medium_local0:
  0.753M ops/s
  peak RSS 7.12 MiB
```

Read:

```text
main remains above the preceding L2 debug median, though less strongly than R3
medium improves versus the preceding L2 debug median
local/guard rows do not show an obvious safety issue
all checked safety counters remain clean
```

Decision:

```text
KEEP as candidate-watch
not default yet
next compare against a fresh default R5 before any promotion
```

## Fresh Default R5 Compare

Default record:

```text
bench_results/hz8_default_r5_compare_20260630T100915/
```

Candidate record:

```text
bench_results/hz8_medium_capacity_budget_l1_r5_20260630T100542/
```

Comparison:

```text
main_interleaved_r90:
  default   207432 ops/s, peak RSS 39.28 MiB
  candidate 246696 ops/s, peak RSS 33.79 MiB

medium_interleaved_r50:
  default   364069 ops/s, peak RSS 22.34 MiB
  candidate 373551 ops/s, peak RSS 24.26 MiB

guard_local0:
  default   7.12M ops/s, peak RSS 7.22 MiB
  candidate 7.16M ops/s, peak RSS 9.35 MiB

medium_local0:
  default   0.756M ops/s, peak RSS 9.02 MiB
  candidate 0.753M ops/s, peak RSS 7.12 MiB
```

Read:

```text
finite capacity-miss collect remains useful on main_interleaved_r90
medium_interleaved_r50 shows only a small throughput gain
local rows are essentially neutral
RSS moves are mixed but bounded in the checked rows
```

Decision:

```text
KEEP as v2 candidate-watch
not default yet
do not build a budget ladder
promote only if the full weak-row matrix keeps the main gain without RSS drift
```
