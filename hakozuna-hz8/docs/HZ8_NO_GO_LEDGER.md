# HZ8 No-Go Ledger

This ledger records design directions that are closed for HZ8 unless new
evidence changes the problem.  Keep this file short and decision-oriented.

## Standing No-Go Rules

| Direction | Status | Reason |
| --- | --- | --- |
| Separate HZ3 local allocator and HZ4 remote allocator | NO-GO | Splits object truth and reintroduces route selection, duplicated metadata, and owner transfer. |
| Small per-object descriptor | NO-GO | Moves HZ8 back toward HZ6's descriptor and exact-route cost center. |
| Remote free ownership transfer | NO-GO | Reintroduces route rehome, source owner mutation, storage owner mutation, and split-brain transfer transactions. |
| Bounded remote-free sink | NO-GO | Requires a second sink on full and recreates transfer-cache/owner-inbox backpressure. |
| Platform free for owned-looking INVALID pointers | HARD NO-GO | Violates fail-closed ownership and can hide double frees or stale HZ8 pointers. |
| Hot-path production diagnostic counters | NO-GO | Pollutes the path HZ8 is trying to keep leaf-shaped. Use debug builds. |
| Profile-specific metadata layout | NO-GO | Turns HZ8 into an evidence family instead of one allocator. |
| Public v0 profile/policy selector | NO-GO | v0 must prove one balanced layout before exposing policy knobs. |
| Policy reads on malloc/free hit paths | NO-GO | Profiles may tune slow-path policy only. |
| Owner generation bump during span adoption | NO-GO | Owner generation represents owner slot incarnation. Span adoption uses `span_ownership_epoch`. |
| Boolean remote queued marker | NO-GO | Use `IDLE / QUEUED / DRAINING` and finish recheck to close lost wakeups. |
| Medium/large per-4KiB route registration | NO-GO | Recreates known remote-tail pressure from page-level registration. |
| All sizes in v0 | NO-GO | Hides whether local small, remote small, or boundary safety caused a result. |

## Design Budget

Always-on global stats budget:

```text
<= 16 fields
```

Debug stats may be richer, but debug builds are not speed-rankable.

v0 behavior boxes:

```text
ArenaGate-0
SmallLocal-0
RemoteSpanInbox-0
OwnerLifecycle-0
PressureController-0
PreloadBoundary-0
```

Do not stack behavior boxes without a focused A/B or smoke result for the
previous box.

Highest-risk box:

```text
OwnerLifecycle-0
```

This box must receive focused design and smoke evidence before any default
claim.  Owner death with nonempty remote MPSC state is the easiest place to
lose a publish, double-collect a node, or adopt a span while a foreign producer
still has write authority.

## Promotion Blockers

Any nonzero item blocks promotion:

```text
invalid_owned_fallback
invalid_owned_real_free
segment_directory_false_miss
stale_segment_valid
slot_alignment_escape
remote_duplicate_claim
remote_publish_lost
remote_node_cycle
remote_collect_duplicate
remote_pending_bitmap_mismatch
remote_pending_shadow_mismatch
span_decommit_while_pending
span_release_while_queued
segment_release_with_live_span
owner_generation_mismatch
dead_owner_publish_lost
orphan_adoption_with_pending_writer
```

Any of these states blocks the design direction until explained:

```text
remote_queue_full
remote_free_returned_backpressure
route rehome on small free
small exact route registration
owner change during remote free
platform free after HZ8-owned range hit
```

In HZ8, `remote_queue_full` and `remote_free_returned_backpressure` should not
exist as normal states.  Their appearance means the design has drifted away
from intrusive per-span remote free.
