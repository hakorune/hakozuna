# Current Task

## Now

Current focus:

- `RemotePendingRmwElisionAudit-L1`

Immediate goal:

- Keep the current remote admission and lookup counters.
- Add an evidence-only opt-in that skips remote pending publication metadata.
- Measure the upper bound of removing pending bit claim, pending_count,
  pending word summary, and notify.

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

Current measured baseline:

- release local r0: about `31M ops/s`
- release remote90, 2-thread small bench: about `1.3M ops/s`
- release remote90 with `H8_ENABLE_REMOTE_LEASE_ELISION=1`: about `1.38M ops/s`
- release remote90 with `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION=1`, RUNS=1:
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
   - evidence-only opt-in via `H8_ENABLE_REMOTE_LEASE_ELISION=1`.
   - default behavior keeps owner lifecycle lease.
   - measured benefit is modest, so do not promote as production behavior yet.

3. `RemotePtrLookupCostAudit-L1`
   - debug-only lookup counters are active.
   - add attribution around arena check, span table lookup, span state check,
     slot index/range check, and owner-word load.
   - behavior unchanged.

4. `RemotePendingRmwElisionAudit-L1`
   - current task.
   - evidence-only opt-in via `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION=1`.
   - skips remote pending metadata publication after validation.
   - not a production correctness mode.
   - RUNS must be 1 because remote frees are intentionally not reclaimed.

5. Candidate next boxes:
   - `RemotePendingRmwElisionAudit-L1`
   - `RemoteBoundaryFastPath-L1`
   - `InterleavedRemoteBench-L1`

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

### RemotePendingRmwElisionAudit-L1

- Add evidence-only `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION=1`.
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

Key archived notes:

- Pending word summary is a hint, not ownership truth.
- Bulk pending collect is one span / one word only.
- Tagged slot state authority remains opt-in.
- Release default does not call slot shadow helpers from hot paths.
- `H8_ENABLE_REMOTE_LEASE_ELISION=1` is evidence-only and unsafe as a general
  production mode without a replacement lifecycle proof.
