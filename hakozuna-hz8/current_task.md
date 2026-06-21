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
12. `PendingWordBulkCommit-L1`
13. `PendingQueueContentionAudit-L1`
14. `ThreadLocalContextFastPath-L1`
15. `SmallLocalRelaxedAtomics-L1`
16. `LocalPendingTouchElision-L1`
17. `OwnerSingleWriterLiveWord-L1`
18. `OwnerLocalScalarState-L1`

Current focus:

- `OwnerLocalScalarState-L1`

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
- `pending_bulk_live_missing = 0`
- `pending_bulk_pending_missing = 0`
- `used_count_underflow = 0`
- `pending_count_bitmap_mismatch_quiescent = 0`
- `used_count_live_bitmap_mismatch_quiescent = 0`
- `local_alloc_pending_nonzero = 0`
- `local_free_pending_nonzero = 0`
- `owner_live_set_already_live = 0`
- `owner_live_clear_already_free = 0`

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
- diagnostics are wired; bulk commit stays for the next design question.

PendingWordBulkCommit-L1:

- GO condition was met by density diagnostics.
- implement one span / one word only.
- keep `popcount == 1` on the existing single-slot path.
- use bulk commit only for `popcount >= 2`.
- clear live bits before pending bits.
- clear pending bits with `fetch_and(~claimed)`, not store-zero.
- splice the local free chain once per word.
- subtract `pending_count` and `used_count` once per word.
- do not change owner exit, adoption default, MediumRun, or profile policy.

PressureDecommitDiscard-L1:

- `mprotect(PROT_NONE)` alone does not discard resident pages on Linux.
- call `madvise(MADV_DONTNEED)` before decommitting an empty retired span.
- this is RSS accounting/pressure cleanup, not a new allocation policy.

SpanCommitCursor-L1:

- avoid starting every span-table allocation scan at index zero.
- keep the existing reserved arena and span table shape.
- use a monotonic cursor as a placement hint only.

ReleaseBenchTarget-L1:

- keep debug `bench` for counter attribution.
- add `bench-release` for throughput numbers without debug hot-path counters.
- do not speed-rank debug builds.

PendingQueueContentionAudit-L1:

- add debug-only counters for qstate notify and pending-head push contention.
- add debug-only counters for owner lifecycle CAS retries.
- do not change remote queue behavior in this box.
- use `bench` for attribution and `bench-release` for throughput.

ThreadLocalContextFastPath-L1:

- avoid `pthread_getspecific()` on every malloc/free when the thread context is
  already known.
- keep the pthread key destructor as the owner-exit authority.
- clear the thread-local fast pointer during thread shutdown.

SmallLocalRelaxedAtomics-L1:

- keep the same local allocation/free behavior.
- use relaxed atomics for owner-only local free-list, bump, and bitmap updates.
- do not change remote publish, collect, owner exit, or adoption ordering.

LocalPendingTouchElision-L1:

- remove local-path writes to the pending bitmap.
- keep pending bitmap writes limited to remote producer claim and owner collect
  clear.
- local alloc/free must treat pending nonzero as a fail-closed integrity error.

OwnerSingleWriterLiveWord-L1:

- keep one authoritative live bitmap.
- keep live bit updates immediate before malloc return / free return.
- use owner single-writer load + release store for local live set/clear instead
  of locked atomic RMW.
- keep remote validation as acquire loads.

OwnerLocalScalarState-L1:

- keep scalar accounting fields atomic for cross-thread observation.
- remove owner-side locked RMW from `bump_index` and `used_count`.
- update `bump_index` with owner-only load/store.
- update `used_count` with checked owner-only load/store in local path and owner
  collect.
- do not change remote producer behavior or lifecycle/adoption verification.
