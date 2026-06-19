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
