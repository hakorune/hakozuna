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
