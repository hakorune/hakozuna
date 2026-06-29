# HZ8 Remote Collect

This document tracks the remote collect optimization lane for HZ8 v0.  It is
split from `HZ8_OWNER_LIFECYCLE.md` so the lifecycle document can stay focused
on ownership state, leases, owner exit, adoption, ordering, and zero gates.

## Scope

The remote collect lane covers:

```text
remote publish admission shape
pending span queue work bounding
pending bitmap drain shape
pending word summary mask
pending word density counters
pending word bulk commit
release benchmark separation
```

It does not change:

```text
owner exit semantics
regular adoption default policy
MediumRun
profile policy
per-object descriptors
```

## Closed Sequence

The current optimization sequence is:

```text
1. OwnerAdmissionReuseSafety-L1
2. HotPathStatsShape-L1
3. PendingCarry-L1
4. PendingBitmapWordDrain-L1
5. OrphanPublishNotifyOnly-L1
6. OwnerAdmissionReadShape-L1
7. RemoteAdmissionSingleLease-L1
8. PendingZeroToOneNotify-L1
9. RemoteDrainDensityAudit-L1
10. PendingWordPresenceMask-L1
11. PendingWordBulkOpportunity-L1
12. PendingWordBulkCommit-L1
13. PressureDecommitDiscard-L1
14. SpanCommitCursor-L1
15. ReleaseBenchTarget-L1
16. PendingQueueContentionAudit-L1
17. PendingWordMaskAuthority-L1
```

## Current Weak-Row Follow-Up

The latest full matrix moved the next small remote question back to owner-side
collect timing:

```text
next box:
  HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md

target:
  small_interleaved_remote90

secondary:
  main_interleaved_r90
```

This follow-up should not reopen remote publish authority, pending bitmap
authority, qstate authority, or owner lifecycle. It only tests whether active
miss / active full paths collect too little pending work under remote-heavy
pressure.

## Admission Shape

Keep `active_spans[]` as a weak hint, not as ownership truth.  Owner admission
reuse safety is closed by packing lifecycle refs into the owner control word.
Generation, state, admission gate, and refs are acquired by one CAS, so owner
slot reuse no longer has a separate ref initialization race.

Remote publish now uses one admission lease per free:

```text
regular owner span:
  owner lifecycle lease

permanent orphan span:
  span publish lease
```

The first box keeps the post-lease owner-word recheck.  Recheck elision is a
separate proof box, not part of this lane yet.

## Pending Queue Work Bounding

`PendingCarry-L1` keeps bounded owner collect truly bounded:

```text
carry empty:
  exchange owner pending_head

budget consumed:
  keep the remainder in owner-local pending_carry

owner exit:
  full drain is allowed because it is a cold path
```

`PendingZeroToOneNotify-L1` was the stepping stone that moved notify authority
to the first pending episode.  After the authority cutover, `pending_word_mask`
owns runtime work-set notification and `pending_count` remains debug shadow.
This avoids trying the qstate enqueue CAS on every remote free in a burst.

## Pending Word Presence

Each span has a 64-bit `pending_word_mask` summary.

```text
bit N == 1:
  pending_bits[N] may contain at least one pending slot
```

The mask is the runtime work-set authority.  The truth remains:

```text
pending bitmap
slot-state authority
debug live shadow
```

`pending_count` is now debug-shadow state only.  It is kept for quiescent
consistency checks, but release remote publish and collect no longer update or
load it.

The collector drains words from `pending_word_mask`.  If the mask is empty and
the quiescent debug shadow disagrees, the debug repair path falls back to a
full word scan and increments `quiescent_pending_repair`.

Quiescent exit/adoption verification must still scan the full pending bitmap.

## Word Summary Counters

Debug builds collect:

```text
pending_word_summary_set
pending_word_summary_shadow_hit
pending_word_summary_false_positive
pending_word_summary_false_negative
pending_word_summary_rearm
```

Promotion blockers:

```text
pending_word_summary_false_negative_quiescent = 0
quiescent_pending_repair = 0
qstate_idle_with_pending = 0
remote_publish_lost = 0
remote_collect_duplicate = 0
```

## Authority Cutover

`PendingWordMaskAuthority-L1` removes `pending_count` from the release
publish/collect paths.  Runtime decisions are made from the pending bitmap,
`pending_word_mask`, and qstate dirty handoff.  Debug builds keep `pending_count`
as a shadow and quiescent consistency check.

## Bulk Opportunity

`PendingWordBulkOpportunity-L1` measured whether non-empty words were dense
enough to justify a bulk commit path.

Debug counters:

```text
pending_word_drain_count
pending_word_popcount_1
pending_word_popcount_2
pending_word_popcount_3_4
pending_word_popcount_5_8
pending_word_popcount_9_16
pending_word_popcount_17_plus
pending_slots_drained
pending_words_rearmed
pending_word_new_publish_during_drain
```

Derived values:

```text
slots_per_nonzero_word =
  pending_slots_drained / pending_word_drain_count

singleton_ratio =
  pending_word_popcount_1 / pending_word_drain_count

multi_ratio =
  1 - singleton_ratio
```

Bulk gate:

```text
GO:
  slots_per_nonzero_word >= 2.0
  and multi_ratio >= 40%

STRONG GO:
  slots_per_nonzero_word >= 4.0

HOLD:
  slots_per_nonzero_word in 1.5..2.0

NO-GO:
  singleton_ratio >= 70%
```

The measured density met the strong GO threshold, so
`PendingWordBulkCommit-L1` is active.

## Bulk Commit

Bulk commit is intentionally narrow:

```text
scope:
  one span / one pending bitmap word

single slot:
  popcount == 1 keeps the single-slot path

bulk slot set:
  popcount >= 2 uses the bulk path
```

Required order:

```text
1. snapshot committed pending bits
2. publish each claimed slot_state as FREE(next)
3. clear pending bits with one word RMW
4. splice the local free chain once
5. subtract pending_count once in debug shadow
```

`pending_bits` must be cleared with `fetch_and(~claimed)`, not store-zero, so
new remote publishes arriving during collect remain visible and can rearm the
summary mask.

## RSS Cleanup

`PressureDecommitDiscard-L1` adds `madvise(MADV_DONTNEED)` before
`mprotect(PROT_NONE)` when retiring an empty span.  Linux `mprotect(PROT_NONE)`
alone does not discard resident pages, so RSS can otherwise grow across repeated
benchmark runs even when spans are logically retired.

## Release Benchmark

`bench-release` is the performance lane.  It builds the harness without
allocator debug counters and without per-op benchmark attribution.

`bench-release-audit` keeps the release allocator shape but enables benchmark
attribution such as class-map and fragmentation accounting.

`bench` remains the debug-counter build.  It enables allocator debug counters
and benchmark attribution.

Do not speed-rank audit or debug builds.

## Queue Contention Audit

`PendingQueueContentionAudit-L1` is a debug-only attribution box.  It measures
remote queue contention without changing allocator behavior.

Counters:

```text
qstate_notify_attempt_count
qstate_notify_success_count
qstate_notify_skip_nonidle_count
pending_head_push_attempt_count
pending_head_push_retry_count
pending_head_push_success_count
owner_lifecycle_enter_cas_retry_count
owner_lifecycle_exit_cas_retry_count
remote_publish_pending_claim_duplicate_count
```

Interpretation:

```text
qstate skip high:
  zero-to-one notify is doing its job, but span bursts are dense

pending_head_push_retry high:
  owner pending queue head is contended

owner lifecycle retry high:
  remote free is bottlenecked on owner admission CAS

duplicate claim nonzero:
  double-free or stale publish path must be investigated
```

Use `bench` to collect these counters and `bench-release` for throughput.
