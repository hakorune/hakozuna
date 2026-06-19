# Linux x86_64 HZ6 Remote Free Owner Inbox, 2026-06-19

## Box

`RemoteFreeBackpressureOwnerInbox-L1`

This opt-in box handles destination transfer backpressure by publishing the
object into the origin owner's pending inbox.  It keeps route owner, logical
owner, and descriptor owner stable at the origin and avoids remote-thread
origin frontcache mutation.

## Build Flags

Selected/default remains off:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=0
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=0
```

Opt-in A/B:

```text
HZ6_EXTRA_CFLAGS='-UHZ6_REMOTE_PENDING_INBOX_CORE_L1 -DHZ6_REMOTE_PENDING_INBOX_CORE_L1=1 -UHZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 -DHZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1 -UHZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 -DHZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1'
```

## Verification

Selected/off integrity smoke passed and kept all `remote_pending_*` and
`remote_free_pending_*` counters at zero.

Opt-in integrity smoke:

```text
remote_pending_enqueue_attempt=33196
remote_pending_enqueue_success=33196
remote_pending_enqueue_full=0
remote_pending_duplicate_claim=0
remote_pending_publish_fail=0
remote_pending_current=30014
remote_pending_maintenance_check=5003
remote_pending_maintenance_armed=2451
remote_pending_batch_call=2451
remote_pending_batch_items=3182
remote_pending_frontcache_push=3182
remote_pending_frontcache_full=0
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_origin_pending_commit=33196
remote_free_pending_no_rehome=33196
remote_free_pending_publish_fail=0
remote_free_returned_backpressure=110
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
```

The first opt-in smoke showed `remote_free_pending_publish_fail=2801`.  That
was descriptor-storage ineligibility, not a route ownership failure, so the
box now only enqueues descriptors stored by the origin allocator and falls
through to existing origin-transfer fallback otherwise.

## RUNS=3

```text
row       median_ops_s   runs
remote50  14988073.96   3
remote90  10816334.11   3
```

Per-run:

```text
remote50: 14989127.90, 14988073.96, 14639437.91
remote90: 10573173.04, 11082231.36, 10816334.11
```

## Decision

`GO(experimental)/HOLD(default)`

The performance direction is strong, especially remote90, and the publish
failure gate is now zero.  Do not select yet: the owner-local consumer is too
weak (`remote_pending_current` remains high at exit) and backpressure returns
are not fully eliminated.

## Demand Audit Follow-Up

`RemotePendingReuseDemandAudit-L1` added behavior-neutral immutable proof
storage and a per-key nonempty mask.  The first implementation used a scan to
refresh the mask and regressed badly, so the mask was changed to O(1) per-key
counts.

Opt-in smoke with owner inbox + demand audit:

```text
remote_pending_key_nonempty_load=6478
remote_pending_key_nonempty_hit=1
pending_same_key_on_frontcache_miss=0
pending_same_key_on_front_dispatch=1
pending_same_key_on_source_alloc=0
source_alloc_with_matching_pending=0
remote_free_pending_publish_fail=0
remote_free_returned_uncommitted=0
```

RUNS=3 after the O(1) fix:

```text
remote50=14637714.57
remote90=10727161.65
```

This says the large pending balance in this smoke is mostly exit inventory, not
obvious same-key demand falling through to source allocation on the measured
path.

## Exact-Key Claim Core Follow-Up

`RemotePendingExactKeyClaimCore-L1` keeps behavior disconnected, but prepares
the direct-reuse boundary:

- per-class lock remains unchanged
- each class inbox now has per-front heads and counts
- key mask/count updates happen under the class lock
- slot state is now `NONE/QUEUED/CLAIMED`
- immutable proof includes `bytes`
- `hz6_allocator_remote_pending_try_reuse()` exists but is not called by any
  allocation path yet

Opt-in owner inbox + demand audit smoke:

```text
remote_pending_enqueue_success=32881
remote_pending_current=29115
remote_pending_queued_current=29115
remote_pending_claimed_current=0
remote_pending_total_current=29115
remote_pending_frontcache_push=3766
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_pending_publish_fail=0
remote_free_returned_uncommitted=0
```

Quick RUNS=3 on the same opt-in lane:

```text
remote50=12536762.73
remote90=9837030.86
```

Decision: `GO(core)/HOLD(perf)`.  The exact-key structure is the right
prerequisite for direct reuse, but this is not a selected performance box.

## Demand Audit V2 Follow-Up

`RemotePendingReuseDemandAuditV2-L1` is behavior-neutral.  It moves same-key
observation to the point before existing pending maintenance and adds source
prefill/source-block coverage.

Opt-in smoke:

```text
pending_same_key_before_maintenance=920
pending_same_key_after_maintenance=744
pending_maintenance_immediate_reuse_success=975
pending_maintenance_batch_surplus=2569
pending_same_key_on_prefill_attempt=38
prefill_commit_with_matching_pending=102
source_block_commit_with_matching_pending=102
direct_source_attempt_with_matching_pending=0
direct_source_commit_with_matching_pending=0
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
```

Quick RUNS=3:

```text
remote50=13570506.65
remote90=10002012.48
```

Decision: `GO(tooling)`.  The V1 `source_alloc_with_matching_pending=0` did
not cover the main opportunity: existing maintenance is already consuming
same-key pending work before the old audit point.  DirectReuse should now be
evaluated as a replacement for the pending-pop/frontcache-push/frontcache-pop
sequence.

## Direct Reuse Follow-Up

`RemotePendingDirectReuse-L1` wires one exact-key pending claim after
frontcache miss.  It is limited to Toy/MidPage preload direct paths and keeps
route validation in production.

DirectReuse-only, with owner-local maintenance disabled:

```text
remote_pending_direct_gate_load=5164
remote_pending_direct_gate_hit=1453
remote_pending_direct_claim_attempt=1453
remote_pending_direct_claim_success=1453
remote_pending_direct_activate_success=1453
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_pending_frontcache_push=0
```

RUNS=3:

```text
remote50=14219657.91
remote90=1423660.43
```

DirectReuse plus the old owner-local maintenance fallback:

```text
remote_pending_direct_gate_load=5999
remote_pending_direct_gate_hit=3080
remote_pending_direct_claim_success=3065
remote_pending_direct_claim_busy=15
remote_pending_direct_activate_success=3065
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=480
remote_pending_frontcache_push=480
```

RUNS=3:

```text
remote50=14162158.67
remote90=11188549.20
```

Decision: direct-only is `NO-GO(default)` because it starves remote90.  Direct
reuse as a short-circuit before maintenance is `GO(experimental)/HOLD(default)`.

## Exact-Key Maintenance Follow-Up

The previous owner-local maintenance fallback still drained by `class_id`.
After exact-key pending heads, that could move a different front with the same
class id into the current frontcache path.  `RemotePendingExactKeyMaintenance`
changes maintenance to drain only the caller's `(front_id, class_id)` key.

Opt-in DirectReuse + exact-key maintenance smoke:

```text
remote_pending_direct_claim_success=3392
remote_pending_direct_claim_busy=18
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=71
remote_pending_frontcache_push=71
pending_same_key_before_maintenance=18
pending_maintenance_immediate_reuse_success=18
pending_maintenance_batch_surplus=53
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
```

RUNS=3:

```text
remote50=14252255.99
remote90=10071818.95
```

Decision: `GO(correctness)/HOLD(perf)`.  It removes class-only fallback
ambiguity; performance selection still needs a broader run.
