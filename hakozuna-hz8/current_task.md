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
19. `LocalPathAttribution-L1`
20. `LocalHotLineAudit-L1`
21. `InitOnceLeafElision-L1`
22. `LocalFreelistProofElision-L1`
23. `TaggedSlotStateShadow-L1`
24. `TaggedSlotStateAuthority-L1`
25. `TaggedSlotStateNoNextFree-L1`
26. `TaggedSlotStateReleaseShadowElision-L1`
27. `RemoteSpanScanAudit-L1`
28. `BumpActiveSpanHint-L1`
29. `FullHintNoPendingScanSkip-L1`

Current focus:

- `FullHintNoPendingScanSkip-L1`

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

LocalPathAttribution-L1:

- add debug-only counters that explain local allocation/free shape.
- keep production/release builds counter-free.
- measure active hint hits/misses, freelist vs bump allocation, slow collect,
  span commit, class-list scans, and local free reject reasons before choosing
  the next optimization.

LocalHotLineAudit-L1:

- add debug-only counters for local hot metadata touches.
- measure live bitmap word distribution for local alloc/free.
- measure local free-head, pending-check, and used-count touch counts.
- do not change layout or behavior before the audit data is available.

InitOnceLeafElision-L1:

- keep `h8_init()` at public/API boundaries.
- remove duplicate `pthread_once()` from internal thread-context leaf lookup.
- keep slow owner creation and pthread-key destructor behavior unchanged.

LocalFreelistProofElision-L1:

- keep live bitmap as the remote-visible authority.
- keep live set before malloc return and live clear before free return.
- make freelist/bump allocation pending checks debug-only.
- remove the separate local-free live test and use live-clear old value as the
  same-incarnation double-free authority.

TaggedSlotStateShadow-L1:

- add debug-only tagged slot state as a shadow map.
- keep live bitmap as the only authority in this box.
- update shadow state beside existing local and collect transitions.
- verify shadow/live/pending/used/free-chain equivalence at quiescent points.
- do not use slot state for boundary classification or remote admission yet.

TaggedSlotStateAuthority-L1:

- add an opt-in `H8_ENABLE_SLOT_STATE_AUTHORITY=1` path.
- keep default behavior on live bitmap authority until stress and A/B pass.
- when enabled, classify and remote-validate with `slot_state + pending`.
- keep live bitmap as debug shadow during the transition.

TaggedSlotStateNoNextFree-L1:

- keep authority path opt-in.
- when authority is enabled, stop reading/writing `next_free[]`.
- use `FREE(next)` payload as the free-list link.
- keep default live-bitmap path on existing `next_free[]`.

TaggedSlotStateReleaseShadowElision-L1:

- keep tagged slot state as authority only when `H8_ENABLE_SLOT_STATE_AUTHORITY=1`.
- keep debug shadow updates and verification under `H8_ENABLE_DEBUG_STATS`.
- in release default mode, do not call slot shadow helpers from local or remote
  hot paths.
- preserve live bitmap authority and existing `next_free[]` behavior in default
  release mode.

RemoteSpanScanAudit-L1:

- behavior unchanged.
- add debug-only attribution for active-span hint misses.
- split owned-by-class scan into usable / full / state-blocked spans.
- use this to decide whether remote90 is blocked by pending collection,
  active-span placement, or list topology.

BumpActiveSpanHint-L1:

- keep `active_spans[]` as a weak hint.
- publish the current span to `active_spans[class]` after successful bump
  allocation, matching the existing freelist-pop path.
- target remote-heavy rows where local free is rare and bump allocation is the
  dominant local allocation path.

FullHintNoPendingScanSkip-L1:

- behavior policy is limited to allocation slow path.
- if the active hint is full and the owner has no pending remote spans, skip the
  owned-by-class full-span scan and commit/refill instead.
- rely on local free updating the active hint when same-owner reusable slots
  exist.
- count skipped scans with `local_find_skip_scan_no_pending`.
