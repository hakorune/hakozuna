# MediumRunSizePolicy/ChunkArenaEvidence-L1

Date: 2026-06-24

Behavior SHA: uncommitted measurement build after `MediumRunResidualCostReaudit-L1`

Purpose:

```text
Add medium class distribution counters to decide whether more queue/lease tuning
is still the right next step, or whether medium size policy / run geometry is the
larger remaining limiter.
```

## Commands

```bash
make bench bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
./h8_bench --runs 3 --threads 2 --iters 30000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench_release --runs 5 --threads 2 --iters 30000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
./h8_bench --runs 2 --threads 2 --iters 5000 --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0
```

`debug_medium_phase_r90.txt` was interrupted because the full debug phase row was
too long for this pass. `debug_medium_phase_r90_short.txt` is the valid phase
sample.

## Results

Smoke:

```text
arena=68719476736 committed=2228224 owners=68 local=68 remote=32
```

Safety stress:

```text
safety_stress owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7
```

Debug medium r50 interleaved:

```text
throughput median=3.188M ops/s
steady median=3.230M ops/s
remote_pub=89870
remote_qpush=73363
remote_collect_call=10512
remote_collect_run=73363
remote_collect_slot=89870
lease_ns_per_pub=156.4
claim_ns_per_pub=31.6
qpush_ns_per_push=64.0
collect_ns_per_run=91.9
collect_runs_per_call=6.979
collect_slots_per_run=1.225
```

Release medium r50 interleaved:

```text
throughput median=7.345M ops/s
steady median=7.589M ops/s
```

Medium class distribution, debug r50:

```text
alloc=[11965,24347,47946,95742]
remote_live=[6066,12066,23971,47767]
one_slot_alloc_ratio=0.531900
one_slot_remote_ratio=0.531512
current_runs=63529
two_slot_64k_runs=39646
two_slot_64k_ratio=0.624061
slots_per_run=[1.444,1.815,1.625,1.000]
```

Short debug phase r90:

```text
throughput median=6768.778 ops/s
peak_rss median=80011264
minor_faults median=19132
alloc_ms median=1491.025
remote_ms median=37.797
```

Medium class distribution, short phase r90:

```text
alloc=[1286,2640,5322,10752]
remote_live=[1140,2350,4772,9701]
one_slot_alloc_ratio=0.537600
one_slot_remote_ratio=0.540055
current_runs=12818
two_slot_64k_runs=7968
two_slot_64k_ratio=0.621626
slots_per_run=[7.917,3.990,1.999,1.000]
```

Short phase residual:

```text
create=6513
global_skip_foreign=40880108
free_steps=8275232
madvise=12821
budget_reject=12305
remote_collect_call=4
remote_collect_run=12821
remote_collect_slot=17963
collect_ns_per_run=4118.9
collect_runs_per_call=3205.250
collect_slots_per_run=1.401
```

## Interpretation

```text
About 53-54% of medium objects are in the 64K one-slot class.
That makes one queue episode per remote free structurally common.

A hypothetical two-slot 64K geometry would reduce the simple run-mix estimate to
about 62% of current runs for both r50 and phase probes.

Queue retry pressure and lease-word replacement are not the next obvious wins.
Medium size policy / run geometry and eventually chunk arena identity are now the
more direct evidence lane.
```

## Decision

```text
MediumRunSizePolicy/ChunkArenaEvidence-L1:
  recorded

MediumRunPendingQueueMPSC follow-up:
  HOLD unless new evidence shows queue push fixed cost dominates after geometry

MediumRunOwnerLeaseWord:
  remains HOLD

Next likely lane:
  MediumRun64KGeometry/SizePolicy-v1 evidence
  or MediumChunkArena-v1 if identity/global scan remains dominant in phase rows
```
