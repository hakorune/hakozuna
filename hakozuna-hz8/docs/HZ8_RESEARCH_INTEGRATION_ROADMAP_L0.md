# HZ8 Research Integration Roadmap L0

## Purpose

HZ8 remains the public balanced allocator and the only normal user entry in
the current family. HZ10 through HZ12 are mechanism research lines. They are
not candidates for a whole-core merge into HZ8.

```text
HZ10: route/page substrate evidence
HZ11: front-cache, batch-transfer, and span-speed evidence
HZ12: advisory ownership and bounded retirement/reclaim evidence
HZ8:  public integration and product gate
```

An experimental mechanism enters HZ8 only when it fixes a named HZ8 weakness
and passes paired throughput, peak/post-RSS, safety, and portability gates.

## Current Target

HZ8 already reproduces low post-workload RSS on its public Linux matrix. A
generic long-lived-owner 16-4096-byte probe reached a 4.9 GiB peak before
owner-exit cleanup. This range belongs to HZ8's small span classes, not its
4097-byte-and-up MediumRun layer. Earlier HZ8 evidence also identifies a
similar phase-separated shape as peak-live/first-touch stress, so reclaimable
retention is not yet proven as the cause.

HZ12 proves on Windows and Linux that a bounded retirement contract can:

- keep advisory ownership out of malloc/free;
- distinguish advisory candidate discovery from reclaim authority;
- prove complete reclaim units before detach;
- discard 64 units / 4 MiB per retirement;
- recycle the same virtual addresses through a bounded depot;
- complete eight owner generations with zero limbo spans.

The next question is not whether HZ8 should become HZ12. It is whether the
small-span peak contains complete reclaimable spans at all. L0 must separate
reclaimable, active, live, pending, and non-owned-state blockers before any
behavior design begins.

## Task Order

### Task 0: Freeze Controls

- keep HZ8 v2 / KeepRefill as the public default;
- freeze the public RSS harness and long-lived medium churn attribution row;
- record feature-off build flags and allocator identity;
- keep adaptive transfer, LargeDirect caches, and unrelated knobs out of this
  experiment.

### Task 1: ReclaimAdapterShadow-L0

At existing owner retirement and medium maintenance checkpoints only:

- inspect HZ8-native small spans only at the existing refill slow path;
- attribute active, pending, live, state-blocked, and complete units;
- report reclaimable bytes and owner-generation state;
- model a bounded reclaim budget;
- perform no detach, discard, or allocation-policy change.

The shadow must use HZ8 slot-state, used-count derivation, and pending bitmap
authority. HZ12 owner metadata is a design reference, not safety authority to
copy. MediumRun reclaim is a separate future question and must not be inferred
from a 16-4096-byte row.

Open L1 only if the long-lived churn row exposes a stable material reclaimable
peak and ordinary public rows do not pay a diagnostic cost in feature-off
builds. If blocked-live bytes dominate, close this reclaim hypothesis and
classify the row as peak-live/size-policy evidence.

Windows L0 result: `ACCEPT` as an owner-retirement upper-bound witness.

```text
generic 16..4096, T=8, ws=1024:
  owner-exit spans: 80,538
  complete reclaimable: 80,538 / 5.278 GiB
  blocked live/pending/state: 0 / 0 / 0

remote90 default shape, T=16:
  owner-exit spans: 51,927
  complete reclaimable: 26,574 / 1.742 GiB
  blocked live: 25,353
  blocked pending/state: 0 / 0
```

The first implementation incorrectly scanned an owner list at every refill
and was discarded before commit. The accepted implementation piggybacks on the
existing owner-exit walk, visits each physical span once, and adds no hot-path
hook. Paired generic R3 remained within the normal Windows noise band.

This proves that a material parking lot exists by retirement. It does not yet
prove when each span became complete during owner lifetime. L1 therefore needs
a bounded maintenance admission rule; it must not continuously rescan every
owner span.

### Task 2: ReclaimAdapterBehavior-L1

Implement an opt-in sibling with this ordering:

```text
slow-path candidate discovery
  -> complete-unit authority check
  -> bounded depot reservation
  -> detach route/state
  -> OS-specific discard
  -> clear advisory lifecycle state
  -> depot insertion
```

Windows and Linux share the ordering and failure semantics, not the backing
API. Windows may use decommit/recommit; Linux may use `madvise`-based discard.
Every partial failure must restore a usable unit or report a hard failure;
limbo is never an accepted fallback.

### Task 3: Paired Gate

Measure default HZ8 and the opt-in sibling on both operating systems:

- long-lived mixed-size small-span churn;
- HZ8 public small/main/medium local and remote rows;
- owner-generation turnover;
- peak RSS and post RSS;
- reclaimed/discarded bytes and depot boundedness;
- stale generation, invalid route, duplicate free, rollback, and limbo smoke.

Experimental acceptance:

- the targeted long-lived peak RSS improves by at least 25%;
- public local throughput remains within -3%;
- public remote rows remain within -5%;
- post RSS does not regress materially;
- safety failures, rollback failures, and limbo spans remain zero;
- no hot-path owner lookup, production counter, or new atomic is introduced.

Default promotion requires repeatable Windows and Linux Pareto improvement. A
Windows-only or Linux-only win remains an opt-in profile.

### Task 4: Speed Integration Only After Reclaim

Do not import HZ11 transfer machinery preemptively. HZ8 already has safe batch
collection, and its adaptive-transfer L1 was NO-GO. After reclaim is decided,
use the remaining measured throughput cells to select at most one HZ10/HZ11
mechanism for a separate shadow-first experiment.

## Non-Goals

- no HZ10/HZ11/HZ12 whole-core merge;
- no runtime benchmark detection or profile matrix in the hot path;
- no weakening of MISS/VALID/INVALID or slot-state authority;
- no simultaneous reclaim, transfer, and LargeDirect behavior experiment;
- no claim that HZ8 universally replaces tcmalloc;
- no HZ13-style generation split for a backend integration experiment.

## Decision Labels

```text
GO:
  measured cross-platform Pareto improvement suitable for HZ8 integration

HOLD:
  safe mechanism with incomplete portability or workload coverage

NO-GO:
  no material peak reduction, hot-path tax, safety ambiguity, or public-row
  regression beyond the gate
```
