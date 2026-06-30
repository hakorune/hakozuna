# HZ8 ActiveFullRemoteCollectBudget L1

Status:

```text
experimental behavior lane
default unchanged
KEEP as control evidence
not promotion
```

## Why This Box Exists

`RemotePressureCollect Attribution L2` showed that the small remote-pressure
collect calls are mostly triggered by:

```text
ACTIVE_HIT_FULL
```

Representative diagnostic run:

```text
small_interleaved_remote90:
  active_hit_full = 77038
  active_miss     =   327

main_interleaved_r90:
  active_hit_full = 8707
  active_miss     = 5254

medium_interleaved_r50:
  small collect source counters = 0
```

This makes the next small-side question:

```text
when the active span is full and remote pending exists,
is the collect budget too thin?
```

## Behavior

The lane is guarded by:

```text
H8_REMOTE_PRESSURE_ACTIVE_FULL_BUDGET_L1
```

Only `ACTIVE_HIT_FULL` collect calls use the larger budget:

```text
pending <= 2    -> pending
pending <= 8    -> 8
pending <= 32   -> 16
pending <= 128  -> 32
otherwise       -> 64
```

Other remote-pressure collect sources keep the default L1 budget.

## Scope

Allowed:

```text
source-specific collect budget
diagnostic bench target
weak-row measurement
```

Preserved:

```text
remote publish protocol
pending bitmap authority
slot_state authority
owner lifecycle
default HZ8 build
```

## Bench Target

```text
make bench-remoteactivefullbudget
./h8_bench_remoteactivefullbudget ...
```

## R3 Result

Record:

```text
bench_results/hz8_active_full_budget_l1_20260630T095923/
```

Compared with the preceding L2 diagnostic:

```text
small_interleaved_remote90:
  throughput 478718 -> 478874 ops/s
  peak RSS 851.72 -> 917.19 MiB
  pending_after 110133 -> 9039
  no_help 33194 -> 4464

main_interleaved_r90:
  throughput 202472 -> 201967 ops/s
  peak RSS 39.79 -> 40.25 MiB
  pending_after 10502 -> 80
  no_help 328 -> 330

medium_interleaved_r50:
  throughput 344074 -> 355129 ops/s
  small remote-pressure counters remain zero
```

Read:

```text
larger ACTIVE_HIT_FULL budget drains pending more completely
but it does not improve the small/main throughput rows
small peak RSS gets worse
medium movement is unrelated to the small collector and should be treated as run noise
```

Decision:

```text
do not promote
do not make default
keep as evidence that budget thickness is not the next small-side limiter
```

## Acceptance

```text
small_interleaved_remote90:
  throughput improves or RSS drops clearly
  helped/no-help ratio improves

main_interleaved_r90:
  does not regress

medium_interleaved_r50:
  not expected to move from this small-side lane
```

No-go:

```text
small row slows down
pending_after stays high despite larger budget
main row regresses materially
any safety counter becomes non-zero
```
