# HZ8 Medium Active Slot Mutation Attribution

Status: recorded / behavior unchanged.

```text
allocator_behavior_base:
  0388eff0 Attribute HZ8 medium collect cadence

working tree:
  debug-only active slot mutation counters
```

## Rows

```text
medium_i0:
  threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=0

medium_i1:
  threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1

medium_r50:
  threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1

main_i0:
  threads=16 iters=100000 size=16..32768 remote_pct=0 interleaved=0
```

All rows are R1 debug/audit attribution rows, not promotion performance rows.

## Active Allocation Shape

```text
medium_i0:
  alloc_live_nonempty      0
  alloc_live_active_empty  1599936
  alloc_live_resident      0
  alloc_live_decommitted   64
  alloc_state_fail         0
  alloc_free_zero          0

medium_i1:
  alloc_live_nonempty      0
  alloc_live_active_empty  1599936
  alloc_live_resident      0
  alloc_live_decommitted   64
  alloc_state_fail         0
  alloc_free_zero          0

main_i0:
  alloc_live_nonempty      0
  alloc_live_active_empty  1401091
  alloc_live_resident      0
  alloc_live_decommitted   48
  alloc_state_fail         0
  alloc_free_zero          0
```

Interpretation:

```text
local active-hit rows:
  each allocation reactivates the current active empty-live run

state/free-mask failures:
  absent in local active-hit rows

candidate:
  active-empty allocation path can bypass generic residency mutation work
```

## Remote-Mixed Shape

```text
medium_r50:
  alloc_live_nonempty      793504
  alloc_live_active_empty  523070
  alloc_live_resident      282999
  alloc_live_decommitted   427
  alloc_state_fail         0
  alloc_free_zero          293424

  active_reuse             1315428
  owner_reuse              284145
  remote_collect_slot      801492
  remote_collect_run       422118
  empty                    283409
  retain                   283409
  reactivate               282999
```

Interpretation:

```text
r50:
  active-empty live reuse is still material
  resident reactivation is also material
  alloc_free_zero mostly reflects active miss before owner-list reuse

do not:
  remove active free-mask/state checks only from this evidence

do:
  consider a narrow fast path for current active empty-live allocation
  preserve owner-list and remote cadence behavior
```

## Gates

```text
invalid_owned             0
route_authority_mismatch  0
writer_overlap            0
writer_foreign            0
writer_token_change       0
collect_wrong_owner       0
empty_with_pending        0
```

## Next Candidate

```text
MediumActiveEmptyAllocFastPath-L1

scope:
  current TLS active run only
  owner token already matched
  allocated_mask == 0
  payload_state == LIVE
  active_live_empty_charge != 0
  pending_bits == 0

expected effect:
  remove generic h8_medium_mark_live_on_alloc work from dominant local path

must preserve:
  owner exit active-live drain
  active replacement demotion
  r50 owner-list/reactivation behavior
```
