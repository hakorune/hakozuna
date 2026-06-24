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

### MediumRunMpscRetryAudit-L1

Status:

```text
RECORDED
```

Goal:

```text
determine whether medium MPSC queue push cost is CAS contention or per-push fixed work
```

Data:

```text
bench_results/20260624T092503Z_medium_mpsc_retry_audit/README.md
```

Result:

```text
debug medium r50:
  push_attempt=77859
  push_retry=174
  push_success=77859
  retry_ratio about 0.22%
  remote_qpush_ms=4.740
  remote_collect_ms=8.579
  remote_lease_ms=7.141
  remote_claim_ms=2.982

release medium r50:
  median 6.015M ops/s
  steady median 6.282M ops/s
```

Interpretation:

```text
medium MPSC CAS contention is not dominant
queue cost is mostly push count / fixed work rather than retry pressure
do not pursue another queue CAS tweak next
```

### MediumRunPendingQueueMPSC-L1

Status:

```text
RECORDED
```

Goal:

```text
remove owner->pending_lock from medium remote queue producer push
```

Scope:

```text
medium pending head is an atomic MPSC stack
producer queue push is lockless
owner collector keeps owner-only carry
medium_pending_count is heuristic; head/carry are checked too
small pending queue unchanged
```

Data:

```text
bench_results/20260624T092050Z_medium_pending_queue_mpsc/README.md
```

Result:

```text
debug medium r50:
  remote_qpush_ms 5.235 -> 4.336
  remote_collect_ms 9.833 -> 8.356
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0

release medium r50:
  median 5.308M -> 6.031M ops/s
  steady median 5.479M -> 6.244M ops/s

release small interleaved remote90 quick rerun:
  median 49.906M ops/s
  steady median 55.078M ops/s
```

Interpretation:

```text
medium MPSC queue push is a valid improvement
remaining medium r50 cost is now owner lifecycle lease, collect work/cadence, and slot mutation
```

### MediumRunResidualCostReaudit-L1

Status:

```text
RECORDED
```

Goal:

```text
measure residual medium r50 buckets after active owner lock elision
```

Data:

```text
bench_results/20260624T091842Z_medium_residual_reaudit/README.md
```

Result:

```text
debug medium r50:
  remote_qpush_ms=5.235
  remote_collect_ms=9.833
  remote_lease_ms=7.192
  remote_claim_ms=3.012
  slots_per_run=[1.181,1.377,1.468,1.000]

release medium r50:
  median 5.308M ops/s
  steady median 5.479M ops/s
```

Interpretation:

```text
queue push remained material after active owner lock elision
this justified MediumRunPendingQueueMPSC-L1
```

### MediumRunActiveOwnerLockElision-L1

Status:

```text
RECORDED
```

Goal:

```text
remove run->lock from proven active owner medium alloc/free paths
```

Scope:

```text
TLS active owner-match medium allocation is lockless
direct same-owner medium free is lockless
owner-list scan remains locked
global detached reuse / attach / detach remain locked
detached direct free remains locked
owner collect remains locked
debug writer guard is compiled out of release
```

Data:

```text
bench_results/20260624T091612Z_medium_active_owner_lock_elision_release_clean/README.md
```

Result:

```text
debug medium r50:
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
  remote_run_lock_ms=0.000
  remote_lockless_accept=0
  remote_lockless_rb_invalid=0

release medium local:
  median 21.895M ops/s
  steady median 22.993M ops/s

release medium r50:
  median 5.397M ops/s
  steady median 5.563M ops/s

release small interleaved remote90 quick:
  median 50.690M ops/s
  steady median 55.438M ops/s
```

Interpretation:

```text
active owner medium local lock elision is safe in current probes
medium local improved materially
medium r50 still needs remote queue / collect residual attribution
small v0 quick gate remains clean
```

### MediumRunOwnerLocalSingleWriterShadow-L2

Status:

```text
RECORDED
```

Goal:

```text
prove whether active owner medium alloc/free can drop run->lock
```

Scope:

```text
debug-only writer guard
guard enters before the current run mutex
actual allocator behavior unchanged
release build has no debug writer fields/counters
```

Data:

```text
bench_results/20260624T091112Z_medium_single_writer_shadow/README.md
```

Result:

```text
debug medium local:
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
  lock_elide_alloc=179976
  lock_elide_free=180000

debug medium r50:
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
  lock_elide_alloc=230632
  lock_elide_free=90130
  remote_pub=89870
  remote_run_lock_ms=0.000
```

Interpretation:

```text
active owner medium alloc/free lock-elision opportunity is material
no attached writer overlap was observed in local or r50 probes
next behavior box can be MediumRunActiveOwnerLockElision-L1
```

### MediumRunPostClaimRollbackClosure-L1

Status:

```text
RECORDED
```

Goal:

```text
close the lockless medium post-claim stranded-pending window
```

Scope:

```text
postcheck failure always attempts pending rollback
DRAINING / DRAINING_DIRTY no longer leaves producer bit published
collector snapshot acceptance is still allowed after rollback closes the bit
owner mark-alive explicitly resets medium owner lists and pending queue fields
no performance-path redesign
```

Acceptance:

```text
smoke pass
safety stress pass
medium r50 debug:
  remote_lockless_rb_invalid may be nonzero
  remote_lockless_rb_accept may be nonzero
  invalid_owned remains 0
  owner/list mismatch remains 0
small quick gates no material regression
all source files remain below 800 lines
```

Data:

```text
bench_results/20260624T085004Z_medium_postclaim_rollback_closure/README.md
```

Result:

```text
debug r50:
  remote_pub=89870
  remote_run_lock_ms=0.000
  remote_lockless_claim=89870
  remote_lockless_accept=0
  remote_lockless_rb_invalid=0
  remote_lockless_rb_accept=73
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0

release r50:
  initial R5 median about 4.737M ops/s
  rerun R7 median about 5.237M ops/s

small interleaved remote90 quick:
  median about 49.613M ops/s
```

Interpretation:

```text
post-claim failure now closes the pending bit before returning
collector snapshot acceptance remains possible without stranded pending bits
owner generation start explicitly clears medium queue/list state
next performance box is MediumRunOwnerLocalSingleWriterShadow-L2
```

### MediumRunRemotePublishLocklessClaim-L1

Status:

```text
RECORDED
```

Goal:

```text
remove producer-side run mutex from owner-attached medium remote publish
```

Scope:

```text
keep owner lifecycle lease
keep owner token / run state recheck
keep slot_state ALLOCATED precheck and post-claim recheck
keep pending bit as duplicate-claim authority
keep collector and local mutation under run mutex
do not elide local run lock yet
do not change detached direct-lock fallback
```

Publish protocol:

```text
1. load owner token
2. enter owner publish lease
3. recheck owner token and run state
4. validate slot pointer
5. slot_state acquire check == ALLOCATED
6. pending bit fetch_or claim
7. duplicate bit => DOUBLE_FREE
8. slot_state acquire recheck
9. if recheck failed:
   - DRAINING / DRAINING_DIRTY means collector acceptance
   - otherwise try rollback
   - rollback bit already gone means collector acceptance
10. old pending word == 0 signals qstate work
```

Debug counters:

```text
medium_remote_lockless_claim_count
medium_remote_lockless_claim_collector_accept
medium_remote_lockless_claim_rollback_invalid
medium_remote_lockless_claim_rollback_accepted
```

Acceptance:

```text
smoke pass
safety stress pass
medium r50 debug records run_lock_ms near zero for remote publish
invalid_owned fallback = 0
duplicate claim accepted twice = 0
medium remote publish lost = 0
small quick gates no material regression
all source files remain below 800 lines
```

Data:

```text
bench_results/20260624T025523Z_medium_remote_lockless_claim/README.md
```

Result:

```text
debug r50:
  remote_pub=89870
  remote_run_lock_ms=0.000
  remote_lockless_claim=89870
  remote_lockless_accept=41
  remote_lockless_rb_invalid=0
  remote_lockless_rb_accept=28
  invalid_owned=0

release r50:
  median about 5.177M ops/s
  steady work median about 5.349M ops/s

small quick gates:
  guard local0 median about 309.991M ops/s
  small interleaved remote90 median about 44.857M ops/s
```

Interpretation:

```text
producer-side run mutex is removed from attached medium remote publish
correctness paths for post-claim collector acceptance are exercised
release improvement is not material
remaining medium cost is now queue push / owner collect / local-run locking
```

## Next Boxes

```text
MediumRunPostMPSCResidualReaudit-L1
  -> recorded as MediumRunMpscRetryAudit-L1

MediumRunCollectWorkCadence-L1
  -> if collect work/cadence remains dominant

MediumRunOwnerLeaseCost-L1
  -> HOLD until collect work/cadence is understood, but now a visible residual bucket

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
