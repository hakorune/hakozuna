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

### MediumRunCollectWordCommitRearm-L1

Status:

```text
RECORDED
```

Scope:

```text
medium collector commits one pending-word snapshot as a unit
slot_state FREE publication happens before pending clear
pending_bits is cleared once per snapshot
collector rechecks remaining pending and self-arms DRAINING_DIRTY
collector finish rechecks pending and requeues if needed
```

Data:

```text
bench_results/20260624T100942Z_medium_collect_word_commit_rearm/README.md
```

Result:

```text
debug medium r50:
  qstate_dirty set=17 self_set=114 requeue=131
  collect_finish_pending_rearm=189
  empty_with_pending=0

release medium r50:
  median 7.583M ops/s
  steady median 7.867M ops/s

small interleaved remote90 quick rerun:
  median 48.794M ops/s
```

Interpretation:

```text
collector finish responsibility is now explicit
medium pending clear is run-snapshot shaped instead of slot-shaped
next lane is medium owner lease word shadow
```

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

## MediumRunCollectWordCommitRearm-L1

Status: recorded.

Data:

```text
bench_results/20260624T100942Z_medium_collect_word_commit_rearm/README.md
```

Result:

```text
collector now commits one pending snapshot as a word:
  slot_state FREE publication
  allocated_mask batch clear
  pending word batch clear
  free_mask batch set
  remaining pending rearm/self-DIRTY

debug medium r50:
  collect_finish_pending_rearm=189
  qstate dirty set/self/requeue = 17/114/131
  empty_with_pending=0

release medium r50:
  median 7.583M ops/s
  steady median 7.867M ops/s

small interleaved remote90 quick:
  median 48.794M ops/s
```

Interpretation:

```text
collector finish race is closed by remaining-word rearm
collect mutation is now closer to run-word granularity
next safe lease step is shadowing a dedicated medium owner admission word
```

## MediumRunOwnerLeaseWordShadow-L1

Status: recorded.

Scope:

```text
debug-only owner-scoped medium_publish_ctl
existing owner control word remains authority
shadow enter decision compared against existing h8_owner_publish_enter
shadow refs closed and checked at owner exit
release hot path has no shadow call
```

Hard zero gates:

```text
medium_lease_enter_decision_mismatch = 0
medium_lease_ref_underflow = 0
medium_lease_ref_nonzero_at_owner_exit = 0
medium_lease_enter_after_close = 0
medium_owner_reuse_with_medium_refs = 0
```

Data:

```text
bench_results/20260624T110022Z_medium_owner_lease_word_shadow/README.md
```

Result:

```text
debug medium r50:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

release medium r50:
  median 7.479M ops/s
  steady median 7.751M ops/s

small interleaved remote90 quick:
  median 50.412M ops/s
  steady median 55.011M ops/s
```

Interpretation:

```text
medium-specific owner admission word matches existing owner control decisions
owner exit observes zero shadow refs
next step can A/B this word as medium remote publish lease authority
```

## MediumRunOwnerLeaseWord-L1

Status: recorded, HOLD as default.

Data:

```text
bench_results/20260624T110834Z_medium_owner_lease_word/README.md
```

Candidate:

```text
release medium remote publish used owner->medium_publish_ctl
owner exit closed medium_publish_ctl and waited medium refs zero
release exit path used fetch_sub
debug still compared medium_publish_ctl decision with packed owner control
```

Result:

```text
debug medium r50:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

release medium r50:
  candidate median 7.429M ops/s
  candidate steady median 7.751M ops/s

prior shadow baseline:
  median 7.479M ops/s
  steady median 7.751M ops/s

small interleaved remote90 quick:
  median 50.124M ops/s
```

Decision:

```text
do not default
safety proof passes, but performance is flat/slightly below baseline
keep debug shadow only; packed owner control remains medium remote authority
```

## MediumRunResidualCostReaudit-L1

Status: recorded.

Data:

```text
bench_results/20260624T112027Z_medium_residual_reaudit/README.md
```

Change:

```text
bench output only
added derived medium_residual line from existing counters
allocator behavior unchanged
```

Result:

```text
debug medium r50:
  lease_ns_per_pub=155.4
  claim_ns_per_pub=31.0
  qpush_ns_per_push=64.1
  collect_ns_per_run=91.6
  collect_runs_per_call=6.846
  collect_slots_per_run=1.219

class density:
  8K  slots/run=1.416
  16K slots/run=1.784
  32K slots/run=1.613
  64K slots/run=1.000

release medium r50:
  median 7.621M ops/s
  steady median 7.906M ops/s
```

Interpretation:

```text
lease is still the largest measured debug bucket, but lease-word A/B was flat
queue push fixed cost is visible, but retry pressure is low
collect cadence is already batching calls; remaining density is limited by 64K one-slot runs
```

## MediumRunSizePolicy/ChunkArenaEvidence-L1

Status: recorded.

Data:

```text
bench_results/20260624T113104Z_medium_size_policy_evidence/README.md
```

Change:

```text
bench output only
added medium class distribution counters
added simple current-run vs hypothetical two-slot-64K run-mix estimate
allocator behavior unchanged
```

Result:

```text
debug medium r50:
  throughput median 3.188M ops/s
  steady median 3.230M ops/s
  one_slot_alloc_ratio=0.531900
  one_slot_remote_ratio=0.531512
  current_runs=63529
  two_slot_64k_runs=39646
  two_slot_64k_ratio=0.624061

release medium r50:
  throughput median 7.345M ops/s
  steady median 7.589M ops/s

short debug phase r90:
  one_slot_alloc_ratio=0.537600
  one_slot_remote_ratio=0.540055
  current_runs=12818
  two_slot_64k_runs=7968
  two_slot_64k_ratio=0.621626
```

Interpretation:

```text
about 53-54% of medium objects are 64K one-slot class
one queue episode per remote free is structurally common for that class
lease-word A/B and queue retry audit do not point to another narrow queue/lease win
medium size policy / run geometry is now the stronger evidence lane
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

MediumRunCollectWordCommitRearm-L1:
  bench_results/20260624T100942Z_medium_collect_word_commit_rearm/README.md
  closed remaining-pending finish race and batched collect word commit

MediumRunOwnerLeaseWordShadow-L1:
  bench_results/20260624T110022Z_medium_owner_lease_word_shadow/README.md
  proved dedicated medium owner lease word as shadow authority

MediumRunOwnerLeaseWord-L1:
  bench_results/20260624T110834Z_medium_owner_lease_word/README.md
  A/B was safety-clean but did not improve release medium r50; HOLD

MediumRunResidualCostReaudit-L1:
  bench_results/20260624T112027Z_medium_residual_reaudit/README.md
  added derived residual line; found no small safe queue/lease follow-up

MediumRunSizePolicy/ChunkArenaEvidence-L1:
  bench_results/20260624T113104Z_medium_size_policy_evidence/README.md
  found 64K one-slot class is about 53-54% of medium objects
```

## Next Boxes

```text
MediumRunPostMPSCResidualReaudit-L1
  -> recorded as MediumRunMpscRetryAudit-L1

MediumRunCollectWorkCadence-L1
  -> recorded as MediumRunCollectCadenceTuning-L1

MediumRunOwnerLeaseCost-L1
  -> recorded as MediumRunOwnerLeaseCeiling-L1

MediumRunOwnerLeaseWordShadow-L1
  -> recorded

MediumRunOwnerLeaseWord-L1
  -> recorded HOLD

MediumRunSizePolicy/ChunkArenaEvidence-L1
  -> recorded

MediumRun64KGeometry/SizePolicy-v1
  -> likely next evidence lane if medium v1 continues

MediumRunChunkArena-L1
  -> possible follow-up if phase rows remain dominated by global registry scans
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
