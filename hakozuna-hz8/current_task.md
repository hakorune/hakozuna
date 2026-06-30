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

Latest L2 diagnostic:
  bench_results/hz8_remote_pressure_l2_20260630T095411/

L2 read:
  small_interleaved_remote90 is ACTIVE_HIT_FULL dominated
  main_interleaved_r90 is mixed but still ACTIVE_HIT_FULL leaning
  medium_interleaved_r50 is not using the small remote-pressure collector

Current behavior box:
  docs/HZ8_ACTIVE_FULL_REMOTE_COLLECT_BUDGET_L1.md
  build target: make bench-remoteactivefullbudget
  default build unchanged
  purpose: test whether active-full remote collect budget is too thin

ActiveFullRemoteCollectBudget-L1 result:
  KEEP as control evidence
  not promotion
  larger active-full budget drains pending_after sharply
  small throughput is effectively unchanged and peak RSS worsens
  main throughput is neutral/slightly down
  medium row movement is unrelated to small collect counters

Next likely target:
  medium remote collect cost / collect source policy
  medium_interleaved_r50 and main_interleaved_r90 show large medium_collect_source
  and medium_residual_budget collect/slot attribution

Current medium behavior box:
  docs/HZ8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1.md
  build target: make bench-mediumcapacitybudget
  default build unchanged
  purpose: test whether unbounded capacity-miss collect is too expensive

MediumCapacityCollectBudget-L1 R3:
  bench_results/hz8_medium_capacity_budget_l1_20260630T100420/
  main_interleaved_r90 improved clearly in debug R3
  medium_interleaved_r50 stayed roughly neutral
  small_interleaved_remote90 movement is likely noise for this knob
  KEEP as promising candidate

MediumCapacityCollectBudget-L1 R5:
  bench_results/hz8_medium_capacity_budget_l1_r5_20260630T100542/
  fresh default compare: bench_results/hz8_default_r5_compare_20260630T100915/

R5 compare:
  main_interleaved_r90:
    default 207432 ops/s, peak RSS 39.28 MiB
    candidate 246696 ops/s, peak RSS 33.79 MiB

  medium_interleaved_r50:
    default 364069 ops/s, peak RSS 22.34 MiB
    candidate 373551 ops/s, peak RSS 24.26 MiB

  guard_local0:
    default 7.12M ops/s, peak RSS 7.22 MiB
    candidate 7.16M ops/s, peak RSS 9.35 MiB

  medium_local0:
    default 0.756M ops/s, peak RSS 9.02 MiB
    candidate 0.753M ops/s, peak RSS 7.12 MiB

MediumCapacityCollectBudget-L1 decision:
  KEEP as v2 candidate-watch
  not default yet
  main remote-heavy signal is real enough to preserve
  medium-only gain is small
  guard/local safety is clean in this pass

Next check:
  pair with the small-side active-full defer fix instead of tuning this budget ladder

Current small behavior box:
  docs/HZ8_ACTIVE_FULL_DEFER8_L1.md
  build target: make bench-activefulldefer8
  combined build target: make bench-defer8mediumcapacity
  default build unchanged

Bucket probe:
  bench_results/hz8_small_active_full_bucket_20260630T114050/
  small active-full pending was mostly <= 8
  b33p was only 4 in the R3 probe

ActiveFullDefer8-L1 R5:
  bench_results/hz8_active_full_defer8_r5_20260630T114414/
  small_interleaved_remote90:
    4.63M ops/s, peak RSS 69.02 MiB
  main_interleaved_r90:
    176543 ops/s, peak RSS 44.70 MiB
  read:
    very strong small-row fix
    not sufficient alone for mixed main rows

Defer8 + MediumCapacity R3:
  bench_results/hz8_defer8_mediumcapacity_20260630T114531/
  small_interleaved_remote90:
    4.35M ops/s, peak RSS 60.29 MiB
  main_interleaved_r90:
    294364 ops/s, peak RSS 37.69 MiB
  medium_interleaved_r50:
    368147 ops/s, peak RSS 22.54 MiB

Current v2 candidate:
  ActiveFullDefer8 + MediumCapacityCollectBudget
  not default yet
  combined R5 record: bench_results/hz8_defer8_mediumcapacity_r5_20260630T114702/

Combined R5:
  small_interleaved_remote90:
    4.39M ops/s, peak RSS 62.32 MiB
  main_interleaved_r90:
    251703 ops/s, peak RSS 41.00 MiB
  medium_interleaved_r50:
    362853 ops/s, peak RSS 22.38 MiB
  guard_local0:
    7.40M ops/s, peak RSS 9.30 MiB

Current read:
  current best HZ8 v2 candidate
  small remote-heavy cliff is substantially fixed
  main remote-heavy improves versus fresh default R5
  medium row is neutral
  next check: full weak-row matrix before default promotion

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
