# HZ6 Remote Pending Inbox Plan

## Box Direction

Current next box:

```text
RemoteFreeBackpressureOwnerInbox-L1
```

Goal:

```text
remote producer publishes only
owner-local consumer frontcache-fills in small batches
route_index_owner = origin
logical_owner = origin
descriptor owner = origin
```

## Boundaries

Remote producer may:

```text
validate route proof
validate descriptor owner/generation/state
publish one descriptor into the origin owner inbox
return COMMITTED only after publish succeeds
```

Remote producer must not:

```text
pop or drain origin transfer cache
push origin frontcache
activate pending objects
rehome the route
change logical owner away from origin
```

Owner-local consumer may:

```text
consume per-class pending inbox entries
validate descriptor / generation / route owner
move REMOTE_PENDING -> LOCAL_FREE
push local frontcache
```

## Initial Hook

The first behavior hook is opt-in only:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_INBOX_CORE_L1=1
```

It is used only after destination transfer reserve returns `BACKPRESSURE`.
The existing selected lane remains unchanged unless the new flag is enabled.

## Counters

Core inbox counters:

```text
remote_pending_enqueue_attempt
remote_pending_enqueue_success
remote_pending_enqueue_full
remote_pending_duplicate_claim
remote_pending_owner_dead
remote_pending_publish_fail
remote_pending_current
remote_pending_high_water
remote_pending_maintenance_check
remote_pending_maintenance_armed
remote_pending_batch_call
remote_pending_batch_items
remote_pending_frontcache_push
remote_pending_frontcache_full
remote_pending_generation_mismatch
remote_pending_owner_mismatch
remote_pending_route_mismatch
remote_pending_state_mismatch
```

Behavior attribution counters:

```text
remote_free_origin_pending_commit
remote_free_pending_no_rehome
remote_free_pending_publish_fail
```

## Zero Gates

These must stay zero before selection:

```text
remote_pending_enqueue_full
remote_pending_duplicate_claim
remote_pending_publish_fail
remote_pending_generation_mismatch
remote_pending_owner_mismatch
remote_pending_route_mismatch
remote_pending_state_mismatch
remote_free_pending_publish_fail
remote_free_returned_uncommitted
remote_free_returned_stale
remote_free_returned_integrity_failure
```

`remote_free_returned_backpressure` is the behavior target. It may remain
nonzero in opt-in experiments until the pending path fully covers the selected
remote rows.

## 2026-06-19 First Behavior Read

`RemoteFreeBackpressureOwnerInbox-L1` is a strong opt-in candidate but remains
off by default.

```text
remote_free_origin_pending_commit=33196
remote_free_pending_publish_fail=0
remote_pending_enqueue_success=33196
remote_pending_batch_items=3182
remote_pending_current=30014
remote_free_returned_backpressure=110
```

RUNS=3:

```text
remote50=14988073.96
remote90=10816334.11
```

Next work should keep the publish boundary but improve owner-local consumption
before promotion.

## 2026-06-19 Demand Audit

`RemotePendingReuseDemandAudit-L1` added immutable publication proof and an
O(1) per-key nonempty mask:

```text
published_ptr
published_generation
published_front_id
published_class_id
published_owner_token
```

The first mask refresh used descriptor-table scanning on pop and regressed, so
it was replaced with per-key counts.  With owner inbox + audit enabled:

```text
remote_pending_key_nonempty_load=6478
remote_pending_key_nonempty_hit=1
pending_same_key_on_frontcache_miss=0
pending_same_key_on_front_dispatch=1
pending_same_key_on_source_alloc=0
source_alloc_with_matching_pending=0
```

RUNS=3:

```text
remote50=14637714.57
remote90=10727161.65
```

Interpretation: the measured lane has a large pending inventory at exit, but
the current demand points did not observe same-key demand falling through to
source allocation.  Direct reuse should therefore use the nonempty mask as a
cheap gate and should be judged by `source_alloc_with_matching_pending`, not by
forcing `remote_pending_current` to zero.

## 2026-06-20 Exact-Key Claim Core

`RemotePendingExactKeyClaimCore-L1` is a prerequisite-only box.  It does not
wire direct reuse into malloc yet.

Changes:

```text
class lock count: unchanged
class inbox heads: split by front
slot state: NONE / QUEUED / CLAIMED
key count/mask updates: class-lock protected
published proof: ptr/generation/bytes/front/class/owner token
raw mask helper: no stats writes
public boundary: hz6_allocator_remote_pending_try_reuse()
```

The existing batch maintenance still drains by class, but now pops from the
per-front sub-heads.  The new direct-reuse API validates immutable proof,
owner token, exact route, requested size, and activates from
`REMOTE_PENDING`; no caller is connected yet, so behavior remains unchanged.

Opt-in smoke:

```text
remote_pending_current=29115
remote_pending_queued_current=29115
remote_pending_claimed_current=0
remote_pending_total_current=29115
remote_pending_frontcache_push=3766
remote_pending_*_mismatch=0
remote_free_pending_publish_fail=0
```

RUNS=3:

```text
remote50=12536762.73
remote90=9837030.86
```

Decision: keep as `GO(core)/HOLD(perf)`.  The next box is
`RemotePendingReuseDemandAuditV2-L1`, not direct-reuse selection.

## 2026-06-20 Demand Audit V2

`RemotePendingReuseDemandAuditV2-L1` confirms that V1 observed too late.

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
```

RUNS=3:

```text
remote50=13570506.65
remote90=10002012.48
```

Interpretation: direct-source fallback still has no same-key overlap, but the
old owner-local maintenance path is already consuming same-key pending entries
and creating surplus frontcache work.  The next box can wire
`RemotePendingDirectReuse-L1` as a default-off replacement for that path.
