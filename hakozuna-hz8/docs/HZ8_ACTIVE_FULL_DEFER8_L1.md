# HZ8 ActiveFullDefer8 L1

Status:

```text
experimental behavior lane
default unchanged
KEEP as v2 candidate-watch
```

## Why This Box Exists

`ACTIVE_HIT_FULL` attribution showed that small remote-heavy rows call the
remote-pressure collector frequently. The first behavior check made that
collector thicker, but it did not improve throughput and worsened RSS.

The next diagnostic added pending-size buckets and showed that most
`ACTIVE_HIT_FULL` calls are small:

```text
small_interleaved_remote90 R3:
  b1     = 5953
  b2_4   = 30281
  b5_8   = 35262
  b9_32  = 3892
  b33p   = 4
```

So the next question is not whether the collector is too thin, but whether
small pending counts should be deferred until the active-miss path.

## Behavior

The lane is guarded by:

```text
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1
```

When enabled, an `ACTIVE_HIT_FULL` pending count at or below:

```text
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT = 8
```

does not immediately call the remote-pressure collector. The pending work is
left for the later active-miss slow path. Larger pending counts still collect
at `ACTIVE_HIT_FULL`.

Preserved:

```text
remote publish protocol
pending bitmap authority
slot_state authority
owner lifecycle
default HZ8 build
```

## Bench Targets

```text
make bench-activefulldefer8
./h8_bench_activefulldefer8 ...

make bench-defer8mediumcapacity
./h8_bench_defer8mediumcapacity ...
```

The combined target also enables:

```text
H8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1
```

## R3 Bucket Probe

Record:

```text
bench_results/hz8_small_active_full_bucket_20260630T114050/
```

Default small row:

```text
small_interleaved_remote90:
  throughput 527124 ops/s
  peak RSS 864.04 MiB
  active-full pending buckets:
    b1    5953
    b2_4  30281
    b5_8  35262
    b9_32 3892
    b33p  4
```

Read:

```text
small pending counts dominate the active-full collect trigger
large pending backlogs are not the common case
```

## Defer8 R5 Result

Record:

```text
bench_results/hz8_active_full_defer8_r5_20260630T114414/
```

Candidate medians:

```text
small_interleaved_remote90:
  4.63M ops/s
  peak RSS 69.02 MiB
  defer = 233592

main_interleaved_r90:
  176543 ops/s
  peak RSS 44.70 MiB

guard_local0:
  7.46M ops/s
  peak RSS 7.32 MiB
```

Read:

```text
defer8 is a very strong small-remote fix
defer8 alone is not suitable for mixed main rows
main still needs the medium capacity-collect fix
```

## Combined R3 Result

Record:

```text
bench_results/hz8_defer8_mediumcapacity_20260630T114531/
```

Candidate medians:

```text
small_interleaved_remote90:
  4.35M ops/s
  peak RSS 60.29 MiB

main_interleaved_r90:
  294364 ops/s
  peak RSS 37.69 MiB

medium_interleaved_r50:
  368147 ops/s
  peak RSS 22.54 MiB
```

Read:

```text
ActiveFullDefer8 handles the small remote-heavy RSS/throughput cliff
MediumCapacityCollectBudget handles the mixed main-row medium collect cost
the combined lane is the current best v2 candidate
```

Decision:

```text
KEEP as v2 candidate-watch
not default yet
next check should be combined repeat-5 and full weak-row gate
```

## Combined R5 Result

Record:

```text
bench_results/hz8_defer8_mediumcapacity_r5_20260630T114702/
```

Candidate medians:

```text
small_interleaved_remote90:
  4.39M ops/s
  peak RSS 62.32 MiB

main_interleaved_r90:
  251703 ops/s
  peak RSS 41.00 MiB

medium_interleaved_r50:
  362853 ops/s
  peak RSS 22.38 MiB

guard_local0:
  7.40M ops/s
  peak RSS 9.30 MiB
```

Read:

```text
the combined lane keeps the small remote-heavy win in repeat-5
main_interleaved_r90 remains above fresh default R5
medium_interleaved_r50 is effectively neutral
guard_local0 is not harmed
safety counters remain clean in the checked rows
```

Decision:

```text
KEEP as current best HZ8 v2 candidate
next gate is full weak-row matrix before default promotion
```
