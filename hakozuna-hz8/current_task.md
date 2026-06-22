# Current Task

## Now

Current focus:

- `EnvFlagSemantics-L1`

Immediate goal:

- Make unsafe evidence flags explicit and exact.
- Move environment flag documentation into stable docs.
- Keep `PendingPublishCommitPoint-L1` blocked until env cleanup is committed.

Next allocator goal:

- Keep the new interleaved remote benchmark lane.
- Close the pending publish commit-point race that the interleaved lane exposed.
- Do not promote pending word mask authority until interleaved remote90 runs
  without pending/live invariant failures.

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
   - current task.
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
   - next allocator task.
   - make pending bit visibility mean the remote free is committed enough for
     owner collect to process.
   - long interleaved remote90 must not abort.
   - hard requirement before count elision or mask authority.

8. `PendingCountElisionShadow-L1`
   - compare pending_count authority with pending_word_mask authority.
   - keep pending_count in production until mask exactness is proven.

9. `PendingWordMaskAuthority-L1`
   - behavior candidate after shadow evidence.
   - remove release per-object pending_count RMW only after zero gates hold.

10. `IntrusiveRemoteHead-L1`
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
  - rollback `pending_count` only if the producer successfully clears its own
    pending bit.
  - stop clearing `pending_word_mask` in the collector when a word has no
    remaining bits.
- Still required:
  - define a single commit point for pending bit visibility.
  - decide whether the commit authority is pending bit, pending word mask, or an
    additional in-flight guard.
  - make long interleaved remote90 pass without `invalid`, `validate_fail`,
    pending count underflow, or collect-on-non-live abort.

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
