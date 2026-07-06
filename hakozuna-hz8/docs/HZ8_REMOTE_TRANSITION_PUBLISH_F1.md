# HZ8RemoteTransitionPublish-F1

Status: NO-GO. Smoke-clean, but xmalloc repro stayed frozen. Code reverted.

## Purpose

`HZ8MacroFailureAttribution-L0` classified the `xmalloc_test` row as a BUG:
under 4-worker contention it can spin at 100% CPU with no progress, with perf
samples in owner lifecycle, remote publish, remote collect, and `sched_yield`.

This box addresses the narrow remote-free transition spin. It does not change
HZ8's small-arena retention policy or commit-failure behavior.

## Failure Shape

The visible unbounded yield loop is in `h8_free_inner()`:

```text
local free failed
h8_remote_free_publish_known(span, slot) returned OWNER_TRANSITION
loop:
  h8_remote_free_publish(ptr)
  if OWNER_TRANSITION: sched_yield()
```

`OWNER_TRANSITION` is returned when the span owner is closing/reused and the
normal owner lifecycle lease cannot be acquired. That is correct as a
conservative fallback, but under a contention storm it can devolve into a pure
yield-spin loop.

The important observation is that the remote-free operation does not actually
need a live owner lease once the owner is already closing. It needs a stable
span long enough to publish the pending bit. HZ8 already has a span publish
lease for exactly this kind of handoff protection.

## Mechanism

When `h8_owner_publish_enter(owner, generation)` fails, try one bounded
transition-assist path before returning `OWNER_TRANSITION`:

```text
load owner control
if control.generation == span owner generation and owner is not ALIVE/open:
  acquire span publish lease
  re-read span owner word
  require same owner slot + generation and span state OWNED_ACTIVE
  publish the pending bit using existing h8_remote_free_publish_locked()
  release span publish lease
```

If any check fails, return `OWNER_TRANSITION` exactly as before.

This keeps the correctness boundary:

```text
- no ownerless publish
- no publish to a reused owner generation
- no publish after span publish is closed
- no publish after span is no longer OWNED_ACTIVE
- the pending-bit commit and duplicate-free checks remain unchanged
```

The owner pointer is only used as the notification queue target after proving
the span still names the same owner slot/generation. If the exiting owner has
already passed its normal pending-queue collection point, its span-quiesce walk
still checks `pending_word_mask` and drains the span directly.

## Gates

Required:

```text
make -C hakozuna-hz8 smoke preload-smoke
./hakozuna-hz8/h8_smoke
```

Optional repro gate when `xmalloc-test` is available:

```text
timeout 15s env LD_PRELOAD=libhakozuna_hz8_preload.so \
  xmalloc-test -w 4 -t 2 -s -1
```

GO if the smokes stay green and the xmalloc repro, when available, no longer
hangs in a yield-spin loop.

NO-GO if the transition assist must publish without a span lease, without
matching owner generation, or after the span has left `OWNED_ACTIVE`.

## Result

Prototype implemented in `h8_remote_free_publish_known()`:

```text
owner lifecycle lease failure
  -> if same owner generation and owner is closing
  -> acquire span publish lease
  -> revalidate same owner slot/generation and OWNED_ACTIVE
  -> publish with the existing pending-bit path
```

Verification completed:

```text
git diff --check
make -C hakozuna-hz8 smoke preload-smoke
./hakozuna-hz8/h8_smoke
make -C hakozuna-hz10 hz10-standalone-check
```

External rerun of the original xmalloc repro was negative:

```text
glibc baseline:       rc=0,   2.05s, 198MB, completed
HZ8 with prototype:   rc=137, 15.0s, 1.79MB, no progress
```

Perf stayed on the same shape:

```text
h8_owner_lifecycle_enter       8.71%
h8_remote_free_publish_known   5.45%
h8_span_collect_remote         4.56%
h8_owner_lifecycle_exit        3.52%
kernel do_sched_yield          2.44%
```

Verdict: NO-GO. The one-shot transition assist did not break the livelock.
The prototype was reverted from `h8_remote_inbox.c`.

## Follow-Up

The next xmalloc box should not retry this shape. The hot frame is owner
lifecycle acquisition itself, not merely the outer `h8_free_inner()` retry
loop. A better next box is an attribution/repair design for owner lifecycle
lease contention:

```text
- count owner_lifecycle_enter retry/yield depth per free
- identify whether progress is blocked by owner exit, pending collection, or
  repeated owner reuse
- add a real escape path only after the blocked state is named
```

Possible escape paths are queueing the remote free to a durable owner/orphan
queue, futex/blocking wait under sustained transition, or making publish
owner-lease-free by construction. Each needs a separate proof; none should be
mixed into this reverted NO-GO box.
