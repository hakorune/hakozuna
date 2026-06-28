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

## Current Direction

```text
HZ8-v1.1:
  frozen balanced default

HZ8-v2:
  throughput lane only
  design / evidence work may continue if it preserves v1.1 safety and RSS
  identity

Current candidate box:
  MediumLocalFastTierActiveRun-Shadow-L1
  HOLD because the release path got larger, not smaller

Interpretation:
  shadow reuse was near-perfect, so miss rate is not the limiter
  v2 throughput gains need a larger architectural move than local micro-tuning

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
