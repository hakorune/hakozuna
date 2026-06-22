# Current Task

## Now

Current focus:

- `ClassSizedSpanMetaBundle-L1`

Immediate goal:

- Collapse per-span metadata allocation from several small allocations into a
  class-sized bundle.
- Keep live bitmap, pending bitmap, `next_free`, and optional slot-state in one
  zeroed block with `H8Span`.
- Remove full `next_free[] = UINT32_MAX` initialization; entries are only read
  after owner/collector writes them into the free chain.

Current evidence:

- `PhaseSeparatedLifecycleAudit-L1` is the next observation box selected by
  design review.  `remote_ms` only covers the worker remote-free loop; total
  throughput also includes TLS destructor / `h8_owner_exit()` after the worker
  loop ends.
- New debug counters should attribute owner-exit and retire tail:
  `owner_exit_total_ns`, `owner_exit_collect_ns`,
  `owner_exit_span_walk_ns`, `span_retire_count`, `span_retire_total_ns`,
  `span_retire_lock_wait_ns`, `span_retire_madvise_ns`,
  `span_retire_meta_free_ns`.
- Bench output should also report process minor page faults per run as a rough
  payload first-touch signal.
- First audit result:
  - release phase-separated remote90, `RUNS=5`, `T=16`, `iters=100000`:
    median `2.67M ops/s`, alloc phase median `243.5ms`, remote phase median
    `11.5ms`, minor faults median `483856`.
  - total wall estimate is about `599ms`, so the measured worker phases leave
    about `344ms` of lifecycle / thread-exit tail.
  - debug phase-separated remote90 attributes the tail to owner exit:
    `owner_exit_total_ms ~= 16054`, `owner_exit_collect_ms ~= 127`,
    `owner_exit_span_walk_ms ~= 15927`.
  - span retire is the dominant subcomponent:
    `span_retire_count = 90886`, `span_retire_total_ms ~= 15877`,
    `span_retire_lock_wait_ms ~= 14834`, `span_retire_madvise_ms ~= 919`,
    `span_retire_meta_free_ms ~= 16`.
  - conclusion: phase-separated remote90 is currently dominated by
    span-retire tail lock contention, not remote publish.
- `SpanRetireTail-L1` follows directly from that evidence.  The lock protects
  the global span-table slot, not the kernel `madvise` or metadata free.  Once
  the table entry is NULL and the span state is `RETIRED`, new pointer lookup
  cannot acquire the retiring span.
- `SpanRetireTail-L1` first result:
  - lock scope now covers only `RETIRED` publication and span-table unlink.
  - debug phase-separated remote90 improved from about `2.07M` to `3.25M ops/s`.
  - `span_retire_lock_wait_ms` dropped from about `14834` to about `248`.
  - `span_retire_madvise_ms` is now the dominant retire subcomponent at about
    `3099`.
  - release phase-separated remote90, `RUNS=10`, `T=16`, `iters=100000`:
    median `4.72M ops/s`, alloc phase median `246.2ms`, remote phase median
    `11.4ms`.
  - release interleaved remote90 remains healthy in the quick check:
    median `24.1M ops/s`, RSS median `16.0 MiB`.
- Design review selected `SpanRetireBatchMadvise-L1` next:
  logical retire is the correctness boundary, while physical page return can
  be batched within owner exit.  Metadata is outside the payload arena and can
  stay alive until batch purge completes.
- Added purge shape counters:
  `span_purge_run_count`, `span_purge_run_spans_total`,
  `span_purge_run_max`, `span_purge_singleton_runs`,
  `span_purge_madvise_calls`, `span_purge_madvise_bytes`,
  `span_purge_madvise_ns`.
- Natural batching result:
  - debug phase-separated remote90 had `avg_spans_per_run ~= 1.03`.
  - `madvise_calls` remained about `88089` for `90886` retired spans.
  - release phase-separated remote90 stayed around `4.67M ops/s`, so natural
    owner-list batching is not enough.
- `OwnerSpanChunkPlacement-L1` now reserves `32` span indices per owner chunk.
  This should reduce global cursor RMWs and create longer contiguous ranges for
  owner-exit batch purge without changing the hot malloc/free leaf shape.
- `OwnerSpanChunkPlacement-L1` first result:
  - debug phase-separated remote90: `avg_spans_per_run ~= 31.6`,
    `madvise_calls = 2876` for `90886` retired spans.
  - debug `span_retire_madvise_ms` dropped from about `3429` to about `2491`.
  - release phase-separated remote90, `RUNS=10`, `T=16`, `iters=100000`:
    median `5.69M ops/s`, alloc phase median `200.7ms`, remote phase median
    `11.3ms`.
  - release local0 quick check: median `136.0M ops/s`.
  - release interleaved remote90 quick check: median `28.3M ops/s`.
- `ClassSizedSpanMetaBundle-L1` is the next allocation-phase box:
  span creation still pays multiple `calloc` calls plus a full `next_free`
  initialization loop.  This box is behavior-preserving and should primarily
  affect phase-separated peak-live rows and span commit metadata timing.
- `ClassSizedSpanMetaBundle-L1` first result:
  - `H8Span`, live bitmap, pending bitmap, `next_free`, and optional slot-state
    now live in one class-sized zeroed allocation.
  - full `next_free[] = UINT32_MAX` initialization was removed.
  - debug phase-separated remote90: `span_commit_meta_ms ~= 110 -> 85`.
  - release phase-separated remote90 remains healthy: median `5.79M ops/s`.
  - release local0 check: median `143.1M ops/s`.
  - release interleaved remote90 check: median `28.7M ops/s`.
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
- `QuiescentPendingBitmapGate-L1` is the next correctness box:
  owner exit, handoff, and adoption must not rely on count-zero as the pending
  truth.
- `QuiescentPendingBitmapGate-L1` now uses full pending bitmap checks for
  owner exit, handoff, and adoption quiescence.  Representative checks pass
  with `quiescent_pending_repair = 0`.
- `PendingWordMaskAuthority-L1` is now being applied:
  release builds no longer update `pending_count` on remote publish/collect.
- Representative release checks after count RMW removal:
  - interleaved remote90 passes with median about `9.9M ops/s`, RSS about
    `9 MiB`.
  - phase-separated remote90 passes with median about `1.16M ops/s`.
  - no RSS leak is visible in these rows.
- `RemoteClaimCloseOrdering-L1` remains the next v0 hard gate before treating
  small remote as closed.
- `RemoteClaimCloseOrdering-L1` now publishes slot-state FREE before pending
  clear when slot-state authority is enabled.  Both default and slot-authority
  debug interleaved rows keep the existing zero gates clean.
- Phase timing shows phase-separated remote90 is allocation/span-commit bound:
  alloc phase median about `644ms`, remote free phase about `12ms`.
- Debug span commit timing attributes the cost to global table lock wait:
  `span_commit_total_ms ~= 23721`, `lock_wait_ms ~= 22235`,
  `mprotect_ms ~= 1239`.
- After lock elision, debug span commit timing shifts to mprotect:
  `lock_wait_ms = 0`, `mprotect_ms ~= 9434` cumulative in debug remote90.
- `SpanCommitMprotectElision-L1` removes per-span mprotect by reserving the
  arena RW with `MAP_NORESERVE`.
- Representative release results after RW arena:
  - local0: median about `150.24M ops/s`
  - remote50 phase-separated: median about `4.65M ops/s`
  - remote90 phase-separated: median about `2.70M ops/s`
  - debug span commit timing: `total_ms ~= 148`, `mprotect_ms ~= 7`
- Interleaved remote90 became noisy and lower in the representative run; track
  this separately before claiming an interleaved win.

Current measured baseline:

- Release baseline after `RemoteClaimCloseOrdering-L1`, `RUNS=10`, `T=16`,
  `iters=100000`, `size=16..2048`:
  - local0: median `114.59M ops/s`, RSS median `15.0 MiB`
  - remote50 phase-separated: median `2.45M ops/s`, RSS median `22.5 MiB`
  - remote90 phase-separated: median `1.37M ops/s`, RSS median `28.7 MiB`
  - remote90 interleaved: median `26.98M ops/s`, RSS median `15.4 MiB`
- Debug safety rows:
  - smoke: pass
  - regular-adoption smoke: pass
  - debug remote90: `invalid=0`, `validate_fail=0`,
    `duplicate_claim=0`, `pending_word_false_neg=0`,
    `pending_word_repair=0`, `quiescent_pending_repair=0`
  - debug interleaved remote90: same pending/remote zero gates are clean
  - slot-state authority debug interleaved remote90: same pending/remote zero
    gates are clean
- Known debug note:
  - slot shadow live-snapshot counters can be nonzero during interleaved
    stress; they are not currently the pending authority promotion blocker.

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
   - completed.
   - keep pending_count updates.
   - remove count from runtime notify / scan / requeue decisions.
   - use count only for shadow counters and cold/quiescent paths.

12. `QuiescentPendingBitmapGate-L1`
   - completed.
   - replace owner exit / handoff / adoption count-zero checks with stable
     publish-gate-closed, refs-zero, qstate-IDLE, mask-zero, full-bitmap-zero
     checks.

13. `PendingWordMaskAuthority-L1`
   - completed.
   - remove release per-object pending_count RMW.
   - keep debug shadow count.

14. `RemoteClaimCloseOrdering-L1`
   - completed.
   - ensure collector publishes non-allocated state before clearing pending
     claim bits.

15. `SpanCommitLockElision-L1`
   - implemented; under measurement.
   - allocate span table indices monotonically with `span_alloc_cursor`.
   - no table scan / global table lock on commit.
   - keep retire-side lock as-is.

16. `SpanCommitMprotectElision-L1`
   - implemented; under measurement.
   - reserve arena as RW.
   - remove commit/decommit mprotect calls.

17. `IntrusiveRemoteHead-L1`
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

### QuiescentPendingBitmapGate-L1

- Applied:
  - add shared full pending bitmap quiescent helper.
  - owner exit uses qstate/mask/full-bitmap instead of count-zero to decide a
    span is pending-clean.
  - handoff precondition uses the same pending-clean helper.
  - orphan adoption quiescent checks use the same pending-clean helper.
  - cold repair can rearm `pending_word_mask` from full bitmap and queue the
    span.
- Scope:
  - owner exit span quiesce
  - span handoff precondition
  - orphan adoption quiescent checks
- Required gate:
  - publish admission closed by caller where applicable
  - publisher refs zero
  - `qstate == IDLE`
  - `pending_word_mask == 0`
  - full pending bitmap scan is zero
- Count handling:
  - `pending_count` may still be updated in release in this box.
  - it is not pending truth for quiescent ownership transitions.
  - it remains a shadow until `PendingWordMaskAuthority-L1`.
- Cold repair:
  - if the bitmap has pending bits but mask is zero at a quiescent attempt,
    rearm mask from the full bitmap and queue the span.
  - repair must be rare and becomes a promotion diagnostic.
- Current checks:
  - smoke passes.
  - regular adoption opt-in smoke passes.
  - debug interleaved remote90 passes with existing zero gates intact.
  - release interleaved remote90 passes.
  - representative `quiescent_pending_repair = 0`.

### PendingWordMaskAuthority-L1

- Applied:
  - release build does not increment `pending_count` in remote publish.
  - release build does not decrement `pending_count` in remote collect.
  - debug build keeps `pending_count` as a reserved-or-committed pending
    obligation shadow.
- Required checks:
  - debug interleaved remote90 remains clean.
  - release interleaved remote90 does not leak RSS.
  - phase-separated remote90 remains clean.
  - quiescent pending repair remains zero in representative rows.
- Current result:
  - smoke and regular-adoption smoke pass.
  - debug interleaved remote90 remains clean:
    `pending_word_false_neg = 0`, `pending_word_repair = 0`,
    `quiescent_pending_repair = 0`.
  - release interleaved remote90 passes without visible RSS leak.
  - release phase-separated remote90 passes.
- Note:
  - this removes one remote publish/collect RMW from release builds.
  - the measured 2-thread interleaved row is noisy and not yet a clear
    throughput win; keep the change because it removes an invalid production
    dependency on count authority.

### RemoteClaimCloseOrdering-L1

- Problem:
  - pending bitmap is the remote/remote duplicate claim truth.
  - if collector clears pending before publishing a non-allocated slot state,
    a stale producer can observe ALLOCATED and re-claim the same object.
- Required ordering for slot-state authority:
  - validate `ALLOCATED`
  - publish `FREE(next)`
  - clear pending bit
  - publish free-list head
- Non-slot-state release mode:
  - live bit remains allocation authority.
  - live clear already happens before pending clear.
- Scope:
  - single-slot remote collect
  - bulk word remote collect
  - no intrusive remote head changes in this box.
- Current checks:
  - default smoke passes.
  - slot-state authority smoke passes.
  - regular adoption smoke passes.
  - default debug interleaved remote90 passes.
  - slot-state authority debug interleaved remote90 passes.
  - release interleaved remote90 passes, representative median about
    `11.4M ops/s`.

### SpanCommitLockElision-L1

- Applied:
  - new span commit uses monotonic `span_alloc_cursor`.
  - commit no longer takes `h8_span_table_lock`.
  - retired span table slots are not reused in v0.
- Evidence:
  - phase-separated remote90 alloc phase drops from about `644ms` to about
    `406ms`.
  - release remote90 median improves from about `1.37M` to about `1.71M`.
  - debug timing shows lock wait is eliminated and mprotect becomes the next
    dominant commit cost.

### SpanCommitMprotectElision-L1

- Rationale:
  - the reserved small arena prevents system allocator collisions.
  - allocator safety is MISS/VALID/INVALID classification, not SIGSEGV on stale
    user payload access.
  - RW anonymous mappings still allocate physical pages lazily on first touch.
- Scope:
  - reserve small arena as RW.
  - span commit does not call `mprotect`.
  - span retire uses `madvise(MADV_DONTNEED)` but does not restore PROT_NONE.
- Current checks:
  - smoke passes.
  - regular adoption smoke passes.
  - release remote90 phase-separated improves to about `2.70M ops/s`.
  - release remote50 phase-separated improves to about `4.65M ops/s`.
  - release local0 improves to about `150.24M ops/s`.
  - debug remote90 keeps pending/remote zero gates clean.
- Follow-up:
  - interleaved remote90 variance worsened in the representative run.  This is
    likely a page-fault/RSS timing artifact of lazy RW arena commit and needs a
    separate interleaved stability audit.

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
