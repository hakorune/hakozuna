# Current Task

## Freeze Baseline

HZ8 small v0 is recorded as `hz8-small-v0-rc1`.

```text
rc1 record:
  docs/HZ8_SMALL_V0_RC1.md

matrix record:
  bench_results/hz8_same_run_matrix_20260623T174503Z/

archived detailed task log:
  docs/archive/current_task_20260624_small_v0_rc1.md
```

Small v0 behavior is frozen unless a hard safety issue is found.

```text
frozen:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  purge policy
  startup-loaded Linux x86_64 ELF TLS contract
```

## Active Lane: MediumRun-v1

Goal:

```text
add >4KiB coverage and make main_* / cross128 rows meaningful
```

Current shape:

```text
range:
  4097..65536

classes:
  8K / 16K / 32K / 64K

identity:
  direct medium registry
  power-of-two slot decode
  interior pointers rejected

residency:
  empty run resident retention is budgeted
  owner exit drains retained empty payload

ownership:
  owner-attached runs publish owner tokens
  TLS active run must match current owner
  global allocation skips foreign-attached runs

remote:
  owner-attached foreign free uses owner lifecycle lease
  pending bit is remote claim authority
  per-owner medium pending queue transfers slot mutation to owner collect
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

Key recorded evidence:

```text
owner affinity:
  bench_results/20260624T005226Z_medium_owner_affinity/README.md
  active_owner_mismatch=0
  owner_list_mismatch=0

remote owned publish:
  bench_results/20260624T010753Z_medium_remote_owned_publish/README.md
  attached remote frees publish through owner queue

collect cadence:
  bench_results/20260624T014320Z_medium_collect_cadence/README.md
  remote_collect_call 20002 -> 2511
  release r50 about 5.18M -> 5.42M

cadence reaudit:
  bench_results/20260624T022910Z_medium_remote_cadence_reaudit/README.md
  remote_qpush_ms=1.521
  class slots/run=[1.162,1.326,1.441,1.000]

route authority:
  bench_results/20260624T024303Z_medium_route_slot_authority/README.md
  route_authority_mismatch=0

lockless publish shadow:
  bench_results/20260624T024545Z_medium_remote_lockless_shadow/README.md
  shadow_attempt=29962
  shadow_match=29962
  shadow_mismatch=0
```

## Current Box

### MediumRunOwnerLeaseCeiling-L1

Status:

```text
RECORDED
```

Goal:

```text
measure the medium remote speed ceiling if the regular-owner lifecycle lease is
removed from the producer path
```

Scope:

```text
normal release behavior unchanged
unsafe evidence build only:
  H8_ENABLE_UNSAFE_EVIDENCE_KNOBS
  H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION=1

medium remote publish now honors the existing unsafe lease-elision flag
```

Non-goal:

```text
do not promote lease elision
do not weaken owner exit / owner reuse safety contract
do not use unsafe results for correctness claims
```

Data:

```text
bench_results/20260624T094726Z_medium_owner_lease_ceiling/README.md
```

Result:

```text
normal release medium r50:
  median 6.775M ops/s
  steady median 7.071M ops/s

unsafe evidence build, env off:
  median 6.531M ops/s
  steady median 6.812M ops/s

unsafe evidence build, lease elided:
  median 8.195M ops/s
  steady median 8.598M ops/s

debug unsafe lease-elided:
  regular_lease_elided=89870
  medium remote_lease_ms=0.000
  medium remote_qpush_ms=4.470
  medium remote_collect_ms=6.799

small interleaved remote90 quick:
  median 49.256M ops/s
```

Interpretation:

```text
owner lifecycle lease is a real medium r50 ceiling bucket
unsafe elision is about +21% over normal release in this row
direct promotion is NO-GO because owner-exit/reuse protection is removed
next design must reduce lease fixed cost without weakening owner lifetime safety
```

### MediumRunOwnerLeaseSplitAudit-L1

Status:

```text
RECORDED
```

Data:

```text
bench_results/20260624T095302Z_medium_owner_lease_split_audit/README.md
```

Result:

```text
debug medium r50:
  remote_lease_ms=11.120
  remote_lease_enter_ms=4.571
  remote_lease_exit_ms=6.549

interpretation:
  exit-side publish ref decrement is at least as important as enter admission
```

### MediumRunCollectCadenceTuning-L1

Status:

```text
RECORDED
```

Goal:

```text
tune owner medium pending collect cadence after MPSC queue push
```

Scope:

```text
collect period/budget made compile-time overrideable
default changed from period=8,budget=4 to period=32,budget=8
```

Data:

```text
bench_results/20260624T093113Z_medium_collect_cadence_tuning/README.md
```

Result:

```text
release medium r50:
  period=8,budget=4:
    median 5.947M ops/s
    steady median 6.190M ops/s

  period=32,budget=8:
    median 6.714M ops/s
    steady median 7.002M ops/s

  default p32/b8 confirmation:
    median 6.869M ops/s
    steady median 7.191M ops/s

debug medium r50 p32/b8:
  remote_collect_call=9302
  remote_collect_run=71583
  remote_collect_slot=89696
  remote_collect_ms=6.670
  remote_qpush=71583
  remote_qpush_ms=4.609
  remote_lease_ms=7.408
  slots_per_run=[1.572,2.005,1.707,1.000]

small interleaved remote90 quick:
  median 49.283M ops/s
```

Interpretation:

```text
collect cadence was too eager
period=32,budget=8 improves medium r50 by allowing slot coalescing
64K class remains structurally one slot/run
remaining visible residual bucket is owner lifecycle lease and collect slot mutation
```

## Archived Medium Boxes

Detailed records live in each `bench_results/.../README.md`.

```text
MediumRunRemotePublishLocklessClaim-L1:
  bench_results/20260624T025523Z_medium_remote_lockless_claim/README.md
  removed producer-side run mutex for attached remote publish

MediumRunPostClaimRollbackClosure-L1:
  bench_results/20260624T085004Z_medium_postclaim_rollback_closure/README.md
  closed post-claim stranded-pending window

MediumRunOwnerLocalSingleWriterShadow-L2:
  bench_results/20260624T091112Z_medium_single_writer_shadow/README.md
  proved active owner alloc/free single-writer opportunity

MediumRunActiveOwnerLockElision-L1:
  bench_results/20260624T091612Z_medium_active_owner_lock_elision_release_clean/README.md
  elided active owner medium alloc/free run lock

MediumRunPendingQueueMPSC-L1:
  bench_results/20260624T092050Z_medium_pending_queue_mpsc/README.md
  replaced producer pending_lock queue push with MPSC stack

MediumRunMpscRetryAudit-L1:
  bench_results/20260624T092503Z_medium_mpsc_retry_audit/README.md
  found MPSC retry ratio about 0.22%; retry pressure is not dominant
```

## Next Boxes

```text
MediumRunPostMPSCResidualReaudit-L1
  -> recorded as MediumRunMpscRetryAudit-L1

MediumRunCollectWorkCadence-L1
  -> recorded as MediumRunCollectCadenceTuning-L1

MediumRunOwnerLeaseCost-L1
  -> recorded as MediumRunOwnerLeaseCeiling-L1

MediumRunLeaseSafeReduction-L1
  -> next design candidate; lease enter+exit must preserve owner exit / owner reuse safety

MediumRunChunkArena-L1
  -> HOLD until remote/local protocol stabilizes
```

## Safety Gates

Medium runtime hard gates:

```text
medium_invalid_owned_platform_fallback = 0
medium_remote_publish_lost = 0
medium_remote_claim_accepted_twice = 0
medium_collect_duplicate = 0
medium_postclaim_unaccounted = 0
medium_empty_with_pending = 0
medium_decommit_while_pending = 0
medium_owner_dead_queue_push = 0
medium_owner_detach_nonquiescent = 0
medium_owner_exit_pending_nonzero = 0
medium_active_alloc_owner_mismatch = 0
medium_owner_list_owner_mismatch = 0
```

Quiescent run gates:

```text
pending_bits == 0
qstate == IDLE
run is not pending-queued
free_mask & allocated_mask == 0
free_mask | allocated_mask == valid_slot_mask
popcount(allocated_mask) == ALLOCATED slot_state count
popcount(free_mask) == FREE slot_state count
resident charge accounting matches payload_state
```

Small frozen gates stay active:

```text
INVALID platform fallback = 0
duplicate claim = 0
quiescent pending bitmap = 0
qstate/mask quiescent = clean
timeout / abort = 0
```

## Documentation

```text
architecture:
  docs/HZ8_ARCHITECTURE.md
  docs/HZ8_MEDIUM_RUN_V1.md
  docs/HZ8_V1_LANES.md

contracts:
  docs/HZ8_OWNERSHIP_CONTRACT.md
  docs/HZ8_OWNER_LIFECYCLE.md
  docs/HZ8_REMOTE_COLLECT.md
  docs/HZ8_NO_GO_LEDGER.md

bench gates:
  docs/HZ8_BENCH_GATE.md
  docs/HZ8_SMALL_V0_RC1.md
```
