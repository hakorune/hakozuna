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
10. `PendingWordPresenceMask-L1`
11. `PendingWordBulkOpportunity-L1`

Current focus:

- `PendingWordBulkOpportunity-L1`

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
- `pending_word_summary_false_negative_quiescent = 0`
- `pending_word_summary_repair = 0`
- `pending_word_summary_invalid_bit = 0`
- `pending_word_summary_set = 0`
- `pending_word_summary_shadow_hit = 0`
- `pending_word_summary_false_positive = 0`
- `pending_word_summary_rearm = 0`
- `pending_word_drain_count = 0`
- `pending_word_popcount_1 = 0`
- `pending_word_popcount_2 = 0`
- `pending_word_popcount_3_4 = 0`
- `pending_word_popcount_5_8 = 0`
- `pending_word_popcount_9_16 = 0`
- `pending_word_popcount_17_plus = 0`
- `pending_slots_drained = 0`
- `pending_words_rearmed = 0`
- `pending_word_new_publish_during_drain = 0`

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

PendingWordPresenceMask-L1:

- next box is collect-side work reduction, not a new lifecycle path.
- add a per-span pending-word summary mask as a hint only.
- do not use the summary as ownership truth.
- keep quiescent full-bitmap verification for exit/adoption.
- summary-driven drain is live; next design question is whether density
  justifies `PendingWordBulkCommit-L1`.

PendingWordBulkOpportunity-L1:

- measure whether non-empty words are dense enough for bulk commit.
- keep the current summary-driven drain behavior unchanged.
- collect popcount buckets and slots-per-nonzero-word in debug lane.
- only move to bulk commit if the density gates actually justify it.
