# HZ8 ActiveFullDefer4 L1

Status:

```text
experimental behavior lane
default unchanged
KEEP as balanced HZ8 v2 RC candidate
```

## Why This Box Exists

`ActiveFullDefer8 + MediumCapacityCollectBudget` fixed the remote90 cliff, but
the broader gate showed that the limit was too permissive for `main_remote50`.

```text
Defer8 + MediumCapacity:
  main_remote50:
    default   401673 ops/s, peak RSS 22.16 MiB
    candidate 367322 ops/s, peak RSS 27.46 MiB
    result    -8.6% throughput, higher RSS
```

The follow-up check keeps the same mechanism but lowers the active-full defer
limit from 8 to 4. The goal is to keep the remote90 fix while avoiding the
remote50 regression.

## Behavior

The lane enables:

```text
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT = 4
H8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1
```

Build target:

```text
make bench-defer4mediumcapacity
./h8_bench_defer4mediumcapacity ...
```

Preserved:

```text
remote publish protocol
pending bitmap authority
slot_state authority
owner lifecycle
default HZ8 build
```

## Gate Record

Record:

```text
bench_results/hz8_defer4_mediumcapacity_gate_20260630T120049/
```

Candidate R3 medians:

```text
guard_remote90:
  4.715M ops/s
  peak RSS 61.52 MiB

main_remote50:
  393920 ops/s
  peak RSS 22.85 MiB

main_remote90:
  270743 ops/s
  peak RSS 33.51 MiB

medium_remote90:
  237225 ops/s
  peak RSS 34.96 MiB
```

Fresh default comparison:

```text
guard_remote90:
  default   4.051M ops/s, peak RSS 121.13 MiB
  candidate 4.715M ops/s, peak RSS 61.52 MiB
  result    +16.4% throughput, much lower RSS

main_remote50:
  default   401673 ops/s, peak RSS 22.16 MiB
  candidate 393920 ops/s, peak RSS 22.85 MiB
  result    -1.9% throughput, near-neutral RSS

main_remote90:
  default   200548 ops/s, peak RSS 38.88 MiB
  candidate 270743 ops/s, peak RSS 33.51 MiB
  result    +35.0% throughput, lower RSS

medium_remote90:
  default   174881 ops/s, peak RSS 50.29 MiB
  candidate 237225 ops/s, peak RSS 34.96 MiB
  result    +35.6% throughput, lower RSS
```

## Read

```text
Defer8:
  strongest remote90-oriented signal
  no-go risk for default because main_remote50 regresses

Defer4:
  better balanced RC
  keeps most remote90 gains
  keeps main_remote50 close to default
  preserves the medium capacity budget win
```

## Local Gate

Record:

```text
bench_results/hz8_defer4_local_gate_20260630T120825/
```

Candidate R3 medians:

```text
main_local0:
  959777 ops/s
  peak RSS 7.76 MiB
  pending_enqueue = 0
  pending_dequeue = 0
  remote = 0

medium_local0:
  827891 ops/s
  peak RSS 8.75 MiB
  pending_enqueue = 0
  pending_dequeue = 0
  remote = 0
```

Read:

```text
local rows do not exercise the active-full defer path
main_local0 stays neutral/positive in this short gate
medium_local0 stays throughput-positive with a modest RSS movement
```

## Decision

```text
KEEP Defer4 + MediumCapacityCollectBudget as the balanced HZ8 v2 RC candidate
KEEP Defer8 as high-remote-pressure evidence/control
do not replace the frozen v1.1 default until the broader release matrix passes
largeish remote rows remain separate because they currently exercise sys/route-miss behavior
```

## Acceptance Before Default Promotion

```text
required:
  main_remote50 >= default - 3%
  main_remote90 clearly above default
  medium_remote90 clearly above default
  guard_remote90 clearly above default with lower RSS
  safety counters clean
  source files remain below 800 lines

still needed:
  repeat the balanced RC on the full release-oriented matrix
  keep largeish rows separate until the large/sys route path is cleaned up
```
