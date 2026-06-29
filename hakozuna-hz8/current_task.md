# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

## Frozen Baselines

```text
small-v0:
  hz8-small-v0-rc1
  docs/HZ8_SMALL_V0_RC1.md

medium-v1 protocol/geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

medium-v1.1 current default:
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md
```

Small-v0 behavior is frozen unless a hard safety issue appears. MediumRun-v1.1
is the current balanced default. Do not reopen its owner queue, pending/qstate
protocol, residency policy, or class geometry for incremental tuning.

User-facing policy:

```text
public allocator:
  HZ8

internal lineage / reference:
  HZ3 / HZ4 / HZ5 / HZ6 / HZ7

experimental lab:
  HZ9
```

Do not ask users to choose among HZ3..HZ9. HZ8 is the balanced release line;
HZ9 remains an opt-in throughput research lane until it earns a separate,
simple public promise.

Platform policy:

```text
Linux:
  current release/default evidence line
  v1.1 stays frozen except hard safety fixes

Windows:
  bring-up lane only
  no public default claim yet
  no parity claim vs Linux v1.1 yet
```

```text
frozen small:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  startup-loaded Linux x86_64 ELF TLS contract
```

## Current Default Shape

```text
medium range:
  4097..65536

medium classes:
  8K / 16K / 24K / 32K / 48K / 64K

medium geometry:
  q64-v12-48k2
  24K class uses 64KiB run / 2 slots
  48K class uses 128KiB run / 2 slots
  64K class uses 128KiB run / 2 slots

medium identity:
  direct registry
  64KiB quantum directory
  power-of-two slot decode for p2 classes
  24K local-free uses exact two-offset decode
  remote/route/usable_size and 48K still use the generic exact non-p2 decode
  interior pointers INVALID

medium residency:
  budgeted empty-resident retention
  TLS active empty-live retention
  ctx-aware collector active-empty-live retention
  owner exit is the hard drain point

legacy comparison:
  q64-run64k2 remains available through medium64k2 build targets

medium remote:
  owner-attached remote free publishes to owner queue
  pending bit is remote claim authority
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

## Latest Benchmark Snapshot

Same-run matrix:

```text
record:
  bench_results/hz8_v11_same_run_matrix_20260629T004316Z/

row family:
  hz8 / hz8_legacy64k2 / hz3 / hz4 / mimalloc / tcmalloc / system
  RUNS=10, THREADS=16, ITERS=100000
```

Representative medians:

```text
guard_local0:
  hz8 264.15M ops/s, RSS 6.14 MiB

fixed24_local0:
  hz8 323.23M ops/s, RSS 6.05 MiB

fixed48_local0:
  hz8 340.68M ops/s, RSS 6.10 MiB

medium_local0:
  hz8 169.49M ops/s, RSS 6.21 MiB

main_local0:
  hz8 136.48M ops/s, RSS 6.53 MiB

main_interleaved_r90:
  hz8 2.68M ops/s, RSS 144.06 MiB

small_interleaved_remote90:
  hz8 0.91M ops/s, RSS 868.28 MiB

Post-L1 targeted R3:

```text
record:
  bench_results/small_interleaved_remote90_hz8_r3.log
  bench_results/main_interleaved_r90_hz8_r3.log
  bench_results/medium_interleaved_r50_hz8_r3.log

hz8 medians:
  small_interleaved_remote90  1.06M ops/s, peak RSS 455.47 MiB
  main_interleaved_r90        4.54M ops/s, peak RSS 113.75 MiB
  medium_interleaved_r50      8.19M ops/s, peak RSS 122.10 MiB
```
```

Interpretation:

```text
local rows:
  low RSS remains the best HZ8 property
  fixed-class local rows are the clearest strength

interleaved / remote-heavy rows:
  remain the weakest area
  small_interleaved_remote90 is the clearest pain point
  main_interleaved_r90 and medium_interleaved_r50 are the next two

L1 observation:
  owner-side remote-pressure collect helped the weakest rows
  small_interleaved_remote90 improved modestly
  main_interleaved_r90 improved clearly
  medium_interleaved_r50 improved the most
  RSS stayed bounded on HZ8 while mimalloc/tcmalloc still trade RSS for speed
```

## Current Direction

```text
HZ8-v1.1:
  frozen balanced default

HZ8-v2:
  throughput lane only
  next work should attack the weakest rows first
  preserve v1.1 safety and RSS

Current candidate box:
  MediumLocalFastTierActiveRun-Shadow-L1
  HOLD because the release path got larger, not smaller

Interpretation:
  shadow reuse was near-perfect, so miss rate is not the limiter
  v2 throughput gains need a larger architectural move than local micro-tuning

Weakness-first order:
  1. small_interleaved_remote90
  2. main_interleaved_r90
  3. medium_interleaved_r50

Next design box:
  docs/HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md

Implementation status:
  HZ8 SmallRemotePressureCollect L1 is in the working tree
  smoke pass: ./h8_smoke
  next check: benchmark the three weakness-first rows again

Current attribution box:
  docs/HZ8_REMOTE_PRESSURE_COLLECT_ATTRIBUTION_L2.md
  source-tagged collect attribution is now in the working tree
  next decision depends on active_full vs active_miss dominance

Local-only tuning is not the next ROI.

Current policy:
  keep the v1.1 default stable
  keep Windows as bring-up / evidence only
  do not promote profile-only lanes without focused guards
```

## Gates To Preserve

```text
small frozen:
  INVALID platform fallback = 0
  duplicate claim accepted twice = 0
  quiescent pending bitmap = 0
  timeout / abort = 0

medium:
  invalid owned platform fallback = 0
  remote publish lost = 0
  claim accepted twice = 0
  collect duplicate = 0
  idle with pending = 0
  empty with pending = 0
  decommit while pending = 0
  owner/list mismatch = 0
  route authority mismatch = 0
```

## Read First

```text
lane guide:
  docs/HZ8_V1_LANES.md

medium protocol/geometry RC:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

same-run matrix:
  docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

main/medium attribution:
  docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md

benchmark gates:
  docs/HZ8_BENCH_GATE.md

small remote pressure collect:
  docs/HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md

v2 design:
  docs/HZ8_V2_HZ9_DESIGN.md
```

## Archive Map

```text
Archived latest-direction ledger:
  docs/archive/current_task_20260629_hz8_latest_direction.md

General archive index:
  docs/archive/README.md
```
