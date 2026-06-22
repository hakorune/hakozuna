# Current Task

## Now

Current focus:

- `PendingCountRuntimeDecisionElision-L1`

Immediate goal:

- Remove `pending_count` from runtime notify / scan / requeue decisions while
  keeping the count updates as a debug/quiescent shadow.
- Prove runtime progress with `pending_bits + pending_word_mask + qstate DIRTY`
  before removing the release count RMW.
- Treat live `count_mask0_bitmap1` as diagnostic only; quiescent bitmap gates
  are the hard blocker.

Current evidence:

- `RemoteSpanScanAudit-L1` found remote90 was wasting work on full span scans.
- `FullHintNoPendingScanSkip-L1` removed that scan in the bench shape:
  `scan_span` went from millions to zero.
- `RemoteFreeAdmissionCostAudit-L1` found the remote path is clean success-path:
  no span miss, no owner miss, no validation fail, no queue retry.
- `RemoteOwnerLeaseElisionAudit-L1` opt-in only improved remote90 about 7%, so
  owner lease is not the sole bottleneck.
- `RemotePtrLookupCostAudit-L1` found pointer classification is also clean
  success-path: no arena miss, span miss, retired span, or slot OOB in remote90.
- `RemotePendingRmwElisionAudit-L1` shows pending publication is a major cost
  center, but skipping it leaks remote frees and explodes RSS.  It is only a
  single-run speed ceiling, not a valid allocator mode.
- Design review selected `InterleavedRemoteBench-L1` before changing behavior:
  phase-separated remote90 is useful, but it does not show owner collect and
  remote publish interacting in steady state.
- The next behavior candidate is `PendingWordMaskAuthority-L1`, not
  `IntrusiveRemoteHead-L1`.  Pending bitmap remains the object-claim truth;
  pending word mask becomes the runtime work-set / notification authority.
- `InterleavedRemoteBench-L1` exposed a real ordering problem:
  phase-separated remote90 passes, but long interleaved release runs can abort
  in remote collect because a pending slot is observed after its live/state
  authority is no longer allocated.
- A first fix moved `pending_count` reservation before pending bit publication
  and made rollback conditional.  This closed the immediate count-underflow
  abort but did not fully close the pending/live race.
- The collector summary clear was removed as required by the design review:
  collectors may rearm remaining words, but must not clear a summary bit that a
  producer may have set concurrently.
- `PendingPublishCommitPoint-L1` now uses the pending bit as the publish commit
  point.  Post-claim rollback validation was removed.
- Long release interleaved remote90 now runs without abort:
  median about `9.5M ops/s` with `RUNS=3`, `T=2`, `iters=500k`.
- Debug interleaved still observes transient mask repair / false-negative
  counters.  This blocks `PendingWordMaskAuthority-L1` until shadow comparison
  proves the mask can replace `pending_count`.
- `PendingCountElisionShadow-L1` counters show the mismatch shape in debug
  interleaved remote90:
  - `mask_notify_without_count` is nonzero, so word-mask notify would enqueue
    more often than current count authority.
  - `count_requeue_without_mask` is also nonzero, so count still catches some
    work not visible through mask at finish time.
  - Therefore direct count removal is still HOLD.
- `PendingWordMaskExactness-L1` moved summary arm before pending bit publish for
  empty pending words.  This reduced but did not eliminate debug false-negative
  / repair observations.
- Additional finish counters show the remaining blocker clearly:
  - `count_mask0_bitmap1` is nonzero, so there are real cases where pending
    bitmap still has work while the word mask is zero.
  - `count_mask0_bitmap0` is also nonzero, so some count-only observations are
    transient/stale count windows.
  - `publish_arm_raced_nonempty` is zero in the representative run, so the
    current mismatch is not caused by another producer filling the same word
    between the pre-load and the pending bit publish.
- `PendingWordMaskFinishProtocol-L1` adds `DRAINING_DIRTY` so producers can
  hand off requeue responsibility to the collector while it is draining.
- First observation: DIRTY is active in interleaved remote90, but
  `count_mask0_bitmap1` remains nonzero.  The current code still increments
  `pending_count` before pending bit publication, so count-only windows remain.
  This keeps `PendingWordMaskAuthority-L1` on HOLD.
- `PendingCountRuntimeDecisionElision-L1` removes runtime count-driven notify,
  runtime count-driven full scan, and runtime count-driven finish requeue.
- Representative debug interleaved remote90 after this change:
  - `pending_word_false_neg = 0`
  - `pending_word_repair = 0`
  - `invalid = 0`
  - `validate_fail = 0`
  - `duplicate_claim = 0`
- Raw `count_mask0_bitmap1` can still be nonzero because the diagnostic is not
  a coherent snapshot and can see the bit-before-mask publish window.

Current measured baseline:

- release local r0: about `31M ops/s`
- release remote90, 2-thread small bench: about `1.3M ops/s`
- release remote90 with unsafe remote lease elision: about `1.38M ops/s`
- release remote90 with unsafe dropped pending publish, RUNS=1:
  about `1.58M ops/s`, with RSS around `1.25 GiB`
- release remote90 with both lease and pending publish elision, RUNS=1:
  about `1.73M ops/s`, with RSS around `1.25 GiB`

## Rules

- Keep `counter / invariant lane` first.
- Keep v0 behavior stable until dry-run and zero gates are proven.
- Do not add a new global route on the hot path.
- Keep the implementation under 800 lines per task slice.
- Keep `active_spans[]` as a weak hint, not ownership truth.
- Do not touch `MediumRun` until the small remote path is closed.
- Evidence-only opt-ins must be explicitly documented and must not become
  production correctness modes without design review.

## Hard Gates

- `remote_publish_lost = 0`
- `remote_collect_duplicate = 0`
- `remote_pending_count_mismatch = 0`
- `span_decommit_while_pending = 0`
- `invalid_owned_fallback = 0`
- `dead_owner_publish_lost = 0`
- `local_alloc_pending_nonzero = 0`
- `local_free_pending_nonzero = 0`
- `owner_live_set_already_live = 0`
- `owner_live_clear_already_free = 0`

## Active Sequence

1. `RemoteFreeAdmissionCostAudit-L1`
   - debug-only remote stage counters are active.
   - confirms regular-owner success path dominates remote90.

2. `RemoteOwnerLeaseElisionAudit-L1`
   - evidence-only opt-in via `H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION=1`.
   - default behavior keeps owner lifecycle lease.
   - measured benefit is modest, so do not promote as production behavior yet.

3. `RemotePtrLookupCostAudit-L1`
   - debug-only lookup counters are active.
   - add attribution around arena check, span table lookup, span state check,
     slot index/range check, and owner-word load.
   - behavior unchanged.

4. `RemotePendingRmwElisionAudit-L1`
   - evidence-only opt-in via
     `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH=1`.
   - skips remote pending metadata publication after validation.
   - not a production correctness mode.
   - RUNS must be 1 because remote frees are intentionally not reclaimed.

5. `EnvFlagSemantics-L1`
   - completed.
   - unsafe evidence flags accept exact `1` only.
   - unsafe flags are renamed under `H8_UNSAFE_EVIDENCE_*`.
   - deprecated aliases remain exact-`1` only for compatibility.
   - stable flag documentation lives in `docs/HZ8_ENV_FLAGS.md`.

6. `InterleavedRemoteBench-L1`
   - implemented.
   - add a benchmark mode where owner threads allocate while foreign threads
     consume and remote-free through bounded SPSC rings.
   - no allocator behavior change.

7. `PendingPublishCommitPoint-L1`
   - completed for the release interleaved abort.
   - make pending bit visibility mean the remote free is committed enough for
     owner collect to process.
   - long interleaved remote90 must not abort.
   - hard requirement before count elision or mask authority.

8. `PendingCountElisionShadow-L1`
   - implemented.
   - compare pending_count authority with pending_word_mask authority.
   - keep pending_count in production until mask exactness is proven.
   - shadow counters are implemented.

9. `PendingWordMaskExactness-L1`
   - completed as a shadow repair attempt.
   - arm the pending word mask before making the first pending bit visible.
   - reduce mask false-negative and repair events.
   - direct authority removal remains blocked while false-negative / repair are
     nonzero.

10. `PendingWordMaskFinishProtocol-L1`
   - completed as a runtime handoff protocol.
   - add `H8_Q_DRAINING_DIRTY`.
   - publish word-mask responsibility from the actual pending word `0 -> 1`
     transition.
   - let producers mark draining spans dirty instead of pushing duplicates.
   - let collectors convert DIRTY to QUEUED at finish.
   - measure `qstate_dirty_set`, `qstate_dirty_self_set`, and
     `qstate_dirty_requeue`.

11. `PendingCountRuntimeDecisionElision-L1`
   - current task.
   - keep pending_count updates.
   - remove count from runtime notify / scan / requeue decisions.
   - use count only for shadow counters and cold/quiescent paths.

12. `QuiescentPendingBitmapGate-L1`
   - next correctness box.
   - replace owner exit / handoff / adoption count-zero checks with stable
     publish-gate-closed, refs-zero, qstate-IDLE, mask-zero, full-bitmap-zero
     checks.

13. `PendingWordMaskAuthority-L1`
   - behavior candidate after shadow evidence.
   - remove release per-object pending_count RMW only after zero gates hold.

14. `IntrusiveRemoteHead-L1`
   - HOLD.
   - only revisit if mask-authority data shows collect CPU still dominates.

## Current Task Details

### RemotePtrLookupCostAudit-L1

- Add debug-only counters for:
  - remote ptr lookup enter
  - arena miss
  - span table miss
  - retired span
  - slot range invalid
  - span lookup success
  - owner-word load success
- Keep `h8_span_from_ptr_checked()` behavior unchanged.
- Do not change MISS / VALID / INVALID semantics.
- Use the new counters with remote90 and local0 to decide if a specialized
  remote pointer classification helper is justified.

### EnvFlagSemantics-L1

- Add `docs/HZ8_ENV_FLAGS.md`.
- Keep development flags permissive but exact enough to avoid prefix surprises.
- Unsafe evidence flags:
  - `H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION=1`
  - `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH=1`
- Deprecated compatibility aliases:
  - `H8_ENABLE_REMOTE_LEASE_ELISION=1`
  - `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION=1`
- Unsafe flags are not allowed for smoke, preload, safety, or promotion claims.

### RemotePendingRmwElisionAudit-L1

- Add evidence-only `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH=1`.
- In opt-in mode, validate the object and return `OK` without:
  - pending bit claim
  - pending word summary update
  - pending_count increment
  - qstate notify / pending queue push
- This intentionally drops the remote free publication and is only for speed
  ceiling measurement in the benchmark shape.
- Do not use this mode for smoke, safety, preload, or production claims.
- Do not use this mode with `RUNS > 1`; RSS grows because remote frees are not
  reclaimed.

### InterleavedRemoteBench-L1

- Add `--interleaved 1` to `bench/h8_bench`.
- Each thread allocates and sends remote objects to the next thread through an
  SPSC ring while also draining its own inbound ring.
- Keep existing phase-separated mode unchanged.
- Record the same counters so phase-separated and interleaved runs are directly
  comparable.
- Required comparison rows:
  - phase-separated remote90
  - interleaved remote90
  - local r0 sanity
- Current result:
  - debug interleaved remote90 runs and exposes mask repair / false-negative
    observations.
  - long release interleaved remote90 still aborts in remote collect, so this
    lane is currently a stress reproducer, not a promotion benchmark.

### PendingPublishCommitPoint-L1

- Problem:
  - phase-separated publish/collect hides a race where collector can observe a
    pending slot while the producer has not reached a fully committed publish
    state, or while local/collect state has already made the slot non-live.
- Already applied:
  - reserve `pending_count` before making the pending bit visible.
  - remove post-claim rollback validation.  Once the pending bit is visible,
    the remote free is committed and the owner collector may process it.
  - stop clearing `pending_word_mask` in the collector when a word has no
    remaining bits.
- Commit contract:
  - claim-before validation checks allocation validity.
  - `pending_count++` reserves accounting.
  - pending bit `0 -> 1` is the publish commit point.
  - after the pending bit is visible, producer rollback is not allowed.
- Still required:
  - keep long interleaved remote90 in the required check set.
  - resolve transient debug mask repair / false-negative observations before
    making the mask authoritative.

### PendingCountElisionShadow-L1

- Add shadow counters for:
  - `old_word == 0` but current `pending_count` notify authority did not notify.
  - `pending_count == 0` but word was already nonzero.
  - collector finish where `pending_word_mask != 0` would requeue.
  - collector finish where `pending_count != 0` but mask is zero.
- Do not remove or change `pending_count` behavior in this phase.
- Promotion prerequisite:
  - phase-separated remote90: no regression.
  - interleaved remote90: no abort.
  - debug shadow mismatches understood and documented.
- Current observation:
  - debug interleaved remote90:
    `mask_notify_without_count > 0` and `count_requeue_without_mask > 0`.
  - release interleaved remote90:
    passes, median about `10M ops/s` on the current 2-thread small row.
  - next step is mask exactness repair, not authority removal.

### PendingWordMaskExactness-L1

- Applied:
  - when a pending word is currently empty, arm `pending_word_mask` before
    publishing the first pending bit.
- Effect:
  - debug interleaved false-negative / repair counters dropped but are still
    nonzero.
  - mask authority remains HOLD.
- Current representative debug interleaved observation:
  - `pending_word_false_neg` around tens, down from around hundreds.
  - `count_requeue_without_mask` still nonzero.
  - `count_mask0_bitmap1` remains nonzero, proving direct mask authority is not
    safe yet.
  - first-word extra notify and post-publish double-arm were tested locally but
    did not eliminate the mismatch, so they were not kept.

### PendingWordMaskFinishProtocol-L1

- Applied:
  - add `H8_Q_DRAINING_DIRTY`.
  - producers that publish a new pending word while a span is draining mark the
    span DIRTY instead of pushing a duplicate pending queue entry.
  - collectors that rearm work while draining also mark the span DIRTY.
  - collector finish uses CAS:
    - `DRAINING -> IDLE` if no dirty handoff happened.
    - `DRAINING_DIRTY -> QUEUED` and one queue push if work arrived during the
      drain.
- New counters:
  - `qstate_dirty_set`
  - `qstate_dirty_self_set`
  - `qstate_dirty_requeue`
- Current representative debug interleaved observation:
  - DIRTY handoff is active.
  - `count_mask0_bitmap1` remains nonzero.
  - `pending_count` still acts as the rescue authority.
- Interpretation:
  - The qstate finish handshake is useful attribution, but it does not by
    itself prove pending word mask authority.
  - The remaining mismatch is consistent with the current publish order where
    `pending_count++` happens before the pending bit commit.
  - Next design decision should address whether to introduce an explicit
    publish in-flight marker, demote count earlier, or keep count authority and
    move to another bottleneck.

### PendingCountRuntimeDecisionElision-L1

- Applied:
  - producer notify is now driven by actual pending word `0 -> nonzero` only.
  - `prev == 0` / count-first notify is diagnostic only.
  - collector no longer performs runtime full scan when `pending_count != 0`
    and `pending_word_mask == 0`.
  - collector finish no longer re-notifies only because `pending_count != 0`.
- Still present:
  - `pending_count++/--` remains in release for now.
  - count shadow diagnostics remain.
  - owner exit / handoff / adoption still use count-zero quiescent checks until
    `QuiescentPendingBitmapGate-L1`.
- Current representative checks:
  - debug interleaved remote90 passes with no false negative / repair.
  - phase-separated debug remote90 remains clean.
  - release interleaved remote90 passes.
- Interpretation:
  - runtime progress can be made without count decisions.
  - next correctness box should replace cold quiescent count gates with stable
    full pending bitmap verification before removing release count RMW.

### PendingWordMaskAuthority-L1 Design Notes

- Before count elision, fix/verify the collector summary rule:
  - collector may OR/rearm a word if pending remains.
  - collector must not clear a summary bit after processing a word; a producer
    may have set it concurrently.
- Release authority target:
  - pending bitmap: remote-free claim truth
  - pending word mask: runtime work-set and notify/requeue authority
  - pending_count: debug exact object-count shadow only
- Do not change pending_count into word count.
- `RemoteHeadNoPendingBitmap` remains hard NO-GO because remote/remote
  duplicate-free claim still needs a shared object claim.

## Archive

Completed / parked tasks:

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
30. `RemoteFreeAdmissionCostAudit-L1`
31. `RemoteOwnerLeaseElisionAudit-L1`
32. `RemotePtrLookupCostAudit-L1`
33. `RemotePendingRmwElisionAudit-L1`

Key archived notes:

- Pending word summary is a hint, not ownership truth.
- Bulk pending collect is one span / one word only.
- Tagged slot state authority remains opt-in.
- Release default does not call slot shadow helpers from hot paths.
- Unsafe evidence flags are exact-`1` only and documented in
  `docs/HZ8_ENV_FLAGS.md`.
