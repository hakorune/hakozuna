# Linux x86_64 HZ6 Remote Pending Inbox Core, 2026-06-19

## Box

`RemotePendingInboxCore-L1`

This is a behavior-off core box for the owner-stable remote pending design.
It adds a compile-time-gated per-class owner inbox and maintenance boundary,
but does not route any remote free path into the inbox yet.

## Build Flags

Selected/default:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=0
```

Opt-in smoke:

```text
HZ6_EXTRA_CFLAGS='-UHZ6_REMOTE_PENDING_INBOX_CORE_L1 -DHZ6_REMOTE_PENDING_INBOX_CORE_L1=1'
```

## Verification

```bash
git diff --check
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
HZ6_EXTRA_CFLAGS='-UHZ6_REMOTE_PENDING_INBOX_CORE_L1 -DHZ6_REMOTE_PENDING_INBOX_CORE_L1=1' \
  ./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Selected/off smoke:

```text
remote_pending_enqueue_attempt=0
remote_pending_enqueue_success=0
remote_pending_enqueue_full=0
remote_pending_duplicate_claim=0
remote_pending_owner_dead=0
remote_pending_publish_fail=0
remote_pending_current=0
remote_pending_high_water=0
remote_pending_maintenance_check=0
remote_pending_maintenance_armed=0
remote_pending_batch_call=0
remote_pending_batch_items=0
remote_pending_frontcache_push=0
remote_pending_frontcache_full=0
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
```

Opt-in/on smoke:

```text
remote_pending_enqueue_attempt=0
remote_pending_enqueue_success=0
remote_pending_enqueue_full=0
remote_pending_duplicate_claim=0
remote_pending_owner_dead=0
remote_pending_publish_fail=0
remote_pending_current=0
remote_pending_high_water=0
remote_pending_maintenance_check=0
remote_pending_maintenance_armed=0
remote_pending_batch_call=0
remote_pending_batch_items=0
remote_pending_frontcache_push=0
remote_pending_frontcache_full=0
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
```

`remote_free_returned_backpressure` remains nonzero in both smokes. That is
the existing behavior target for the next box, not a new pending-inbox
regression.

## Decision

`GO(core)/HOLD(behavior)`

Keep the core box available behind `HZ6_REMOTE_PENDING_INBOX_CORE_L1`, but
leave it off in selected preload until `RemoteFreeBackpressureOwnerInbox-L1`
connects destination-transfer-full fallback to owner-stable pending publish.
