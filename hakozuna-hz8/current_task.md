# Current Task

HZ8 optimization is being implemented in this order:

1. `OwnerAdmissionReuseSafety-L1`
2. `HotPathStatsShape-L1`
3. `PendingCarry-L1`
4. `PendingBitmapWordDrain-L1`
5. `OrphanPublishNotifyOnly-L1`
6. `OwnerAdmissionReadShape-L1`
7. `RemoteAdmissionSingleLease-L1`
8. `PendingZeroToOneNotify-L1`
9. `RemoteDrainDensityAudit-L1`

Current focus:

- `RemoteDrainDensityAudit-L1`

Rules:

- Keep `counter / invariant lane` first.
- Keep v0 behavior stable until dry-run and zero gates are proven.
- Do not add a new global route on the hot path.
- Keep the implementation under 800 lines per task slice.
- Keep `active_spans[]` as a weak hint, not ownership truth.
- Do not touch `MediumRun` until the small remote path is closed.

Validation gates already in use:

- `remote_publish_lost = 0`
- `remote_collect_duplicate = 0`
- `remote_pending_count_mismatch = 0`
- `span_decommit_while_pending = 0`
- `invalid_owned_fallback = 0`
- `dead_owner_publish_lost = 0`

New gates for the current sequence:

- `lifecycle_ref_underflow = 0`
- `stale_owner_lease_success = 0`
- `owner_reuse_with_live_ref = 0`
- `owner_dead_with_lifecycle_refs = 0`
- `owner_generation_attach_mismatch = 0`
- `pending_span_count_mismatch = 0`
- `pending_carry_duplicate = 0`
- `pending_span_dual_membership = 0`
- `qstate_idle_while_queued = 0`
- `pending_word_lost = 0`
- `pending_count_underflow = 0`
- `pending_bit_without_live = 0`
- `collect_same_slot_twice = 0`
- `regular_publish_without_owner_lease = 0`
- `orphan_publish_without_span_lease = 0`
- `remote_publish_dual_lease = 0`
- `post_lease_owner_word_mismatch = 0`
- `pending_zero_to_one_without_notify = 0`

OwnerAdmissionReadShape-L1:

- closed by removing the redundant plain owner generation read from remote
  publish admission.
- remaining owner generation uses are ownership state, not admission shape.

RemoteAdmissionSingleLease-L1:

- split remote admission so regular-owner spans use one lifecycle lease path and
  permanent-orphan spans use one span-lease path.
- keep the post-lease owner-word recheck in the first box.
- do not elide the recheck until the debug counter proves it is stable.

PendingZeroToOneNotify-L1:

- make `pending_count == 0 -> 1` the authority for notify.
- keep collector recheck and requeue logic unchanged for now.

RemoteDrainDensityAudit-L1:

- measure collect work density before changing drain shape again.
- prefer audit counters over another queue redesign.
- current implementation adds call / carry / requeue / word density counters.
- stop here and ask the next design question before changing queue shape.
