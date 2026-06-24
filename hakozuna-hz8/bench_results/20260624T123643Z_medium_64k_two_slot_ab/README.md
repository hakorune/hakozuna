# MediumRun64KTwoSlotAB-L1

Date: 2026-06-24

Behavior:

```text
Default remains q64-run64k.
Candidate build uses H8_MEDIUM_64K_TWO_SLOT:
  64K class = 128KiB run payload / 2 slots per run.
Class map, slot size, slot decode, pending bitmap, and slot_state authority stay unchanged.
```

## Commands

```bash
make bench bench-medium64k2 bench-release bench-release-medium64k2 smoke safety-stress
./h8_smoke
./h8_safety_stress

./h8_bench --runs 2 --threads 2 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench_medium64k2 --runs 2 --threads 2 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

./h8_bench_release --runs 3 --threads 2 --iters 30000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench_release_medium64k2 --runs 3 --threads 2 --iters 30000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

./h8_bench --runs 1 --threads 2 --iters 5000 --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0
./h8_bench_medium64k2 --runs 1 --threads 2 --iters 5000 --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0

./h8_bench_release_medium64k2 --runs 2 --threads 4 --iters 30000 --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1
```

## Results

Smoke:

```text
arena=68719476736 committed=2228224 owners=68 local=68 remote=32
```

Safety stress:

```text
safety_stress owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7
```

Debug medium r50:

```text
default:
  throughput median=3.273M
  remote_qpush=33259
  remote_collect_run=33259
  remote_collect_slot=39999
  collect_slots_per_run=1.203
  64K class slots/run=1.000

64k2:
  throughput median=3.844M
  remote_qpush=23000
  remote_collect_run=23000
  remote_collect_slot=39999
  collect_slots_per_run=1.739
  64K class slots/run=1.770
```

Release medium r50:

```text
default median=7.356M ops/s
64k2 median=8.765M ops/s
ratio=1.192
```

Short debug phase r90:

```text
default:
  throughput=6.521K ops/s
  alloc_ms=1481.612
  remote_ms=37.278
  create=6368
  global_skip_foreign=20269338
  free_steps=3694803
  remote_qpush=6368
  collect_slots_per_run=1.413

64k2:
  throughput=16.239K ops/s
  alloc_ms=580.971
  remote_ms=24.093
  create=3979
  global_skip_foreign=7912248
  free_steps=2319054
  remote_qpush=3979
  collect_slots_per_run=2.261
```

Small quick candidate check:

```text
small interleaved r90 T4 median=20.194M ops/s
```

This is a short smoke-style row, not the frozen R10 x2 gate.

## Zero Gates

```text
invalid_owned=0
active_owner_mismatch=0
owner_list_mismatch=0
route_authority_mismatch=0
empty_with_pending=0
writer_overlap=0
writer_foreign=0
writer_token_change=0
collect_wrong_owner=0
detached_while_attached=0
lease_shadow_mismatch=0
```

## Decision

```text
MediumRun64KTwoSlotAB-L1:
  candidate implemented
  short A/B is strongly positive

Default:
  unchanged until paired RUNS=10 x2

Next:
  run paired R10 x2 for medium r50 and small frozen rows
  if clean, promote q64-run64k2 and proceed to MediumDetachedRunClassIndex-L1
```
