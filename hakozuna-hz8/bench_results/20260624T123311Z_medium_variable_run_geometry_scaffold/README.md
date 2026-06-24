# MediumVariableRunGeometryScaffold-L1

Date: 2026-06-24

Behavior:

```text
No allocator behavior change intended.
Added H8_MEDIUM_QUANTUM_BYTES and changed the medium direct directory to register
every 64KiB quantum covered by a run. Current classes still use 64KiB runs.
```

## Commands

```bash
make bench bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
./h8_bench --runs 2 --threads 2 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench_release --runs 3 --threads 2 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench_release --runs 2 --threads 4 --iters 50000 --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0
./h8_bench_release --runs 2 --threads 4 --iters 30000 --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1
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
throughput median=3.335M ops/s
steady median=3.385M ops/s
invalid_owned=0
active_owner_mismatch=0
owner_list_mismatch=0
route_authority_mismatch=0
empty_with_pending=0
remote_pub=39999
remote_qpush=32984
remote_collect_run=32984
remote_collect_slot=39999
collect_slots_per_run=1.213
one_slot_remote_ratio=0.534113
two_slot_64k_ratio=0.622891
```

Release medium r50:

```text
throughput median=7.002M ops/s
steady median=7.375M ops/s
```

Small quick checks:

```text
small local0 T4 median=184.920M ops/s
small interleaved r90 T4 median=20.577M ops/s
```

The small rows are short quick checks, not the frozen R10 x2 gate.

## Decision

```text
MediumVariableRunGeometryScaffold-L1:
  recorded

Directory contract:
  ptr -> 64KiB quantum -> containing run
  final validity remains ptr-in-run plus slot identity checks

Next:
  MediumRun64KTwoSlotAB-L1
```
