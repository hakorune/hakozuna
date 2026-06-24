# MediumRunResidualCostReaudit-L1

Date: 2026-06-24 UTC

Base:

```text
095184be Record medium lease word hold
```

Change:

```text
bench output only
added derived medium_residual line from existing debug counters:
  lease_ns_per_pub
  claim_ns_per_pub
  qpush_ns_per_push
  collect_ns_per_run
  collect_runs_per_call
  collect_slots_per_run
allocator behavior unchanged
```

Functional:

```text
h8_smoke:
  arena=68719476736 committed=2228224 owners=68 local=68 remote=32

h8_safety_stress:
  owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 5 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput:
  median 3.264M ops/s
  steady median 3.297M ops/s

medium_residual:
  lease_ns_per_pub=155.4
  claim_ns_per_pub=31.0
  qpush_ns_per_push=64.1
  collect_ns_per_run=91.6
  collect_runs_per_call=6.846
  collect_slots_per_run=1.219

medium remote:
  remote_pub=149954
  remote_qpush=123023
  remote_collect_call=17969
  remote_collect_run=123023
  remote_collect_slot=149954

class density:
  8K  slots/run=1.416
  16K slots/run=1.784
  32K slots/run=1.613
  64K slots/run=1.000

zero gates:
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
  empty_with_pending=0
  lease decision/ref gates=0
```

Release medium r50:

```text
command:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput:
  median 7.621M ops/s
  p25 7.525M
  p75 7.696M
  steady median 7.906M
```

Interpretation:

```text
owner lease remains the largest measured debug bucket, but the dedicated
medium lease word A/B was safety-clean and performance-flat, so do not reopen
that path without a materially different lifetime design.

queue push fixed cost is visible at about 64ns/push, but MPSC retry pressure is
low and 64K class structurally has one slot/run. Sticky armed queues would be a
new lifecycle/membership design, not a small local fix.

collect cadence is already batching calls: about 6.85 runs per collect call.
Remaining class density is limited by 64K one-slot runs and broad medium size
distribution.
```

Next:

```text
HOLD medium lease word
HOLD sticky queue / queue episode lease
next likely useful lane is MediumRunSizePolicy/ChunkArena evidence, or a
separate release-side micro comparison if a new concrete code-shape candidate
appears
```
