# HZ8 Research Integration Roadmap L0

## Post-Mag16 Order

Mag16 is promoted to the HZ8 v2 default. The next work is intentionally kept
inside HZ8:

1. freeze a cross-allocator public matrix for the new default;
2. attribute the remaining medium/mixed peak-retention weakness;
3. reuse the HZ12 reclaim contract only through an HZ8-native shadow box;
4. reopen HZ11 or HZ12 implementation work only when the shadow identifies a
   missing contract that cannot be expressed in HZ8.

HZ10-HZ12 remain evidence suppliers, not allocator cores to merge. No new
production hot-path counters, ownership reads, or atomics are permitted during
the attribution step.

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

Windows result: `NO-GO`, implementation removed.

Two bounded commit-slow-path policies were tested:

```text
cursor scan:
  scan up to 64 spans, retire up to 8
  peak reduction: less than 1%
  local throughput: about -15% in the initial run

recent-head scan:
  rescan the newest 64 spans, retire up to 8
  baseline peak R3: 5020-5033 MiB
  candidate peak R3: 4805-5006 MiB
  repeatable reduction: less than 1% in two of three runs
  throughput regression: approximately 10-33%
```

The L0 upper bound is real, but complete spans become visible after the useful
commit-time maintenance window. Fixing that timing would require a candidate
publication on local free or another continuously active policy, violating the
no-hot-path-tax constraint. Keep L0 as evidence and do not promote or retain
the L1 behavior code.

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

### Task 4: Speed Integration After Reclaim Closeout

Do not import HZ11 transfer machinery preemptively. HZ8 already has safe batch
collection, and its adaptive-transfer L1 was NO-GO. After reclaim is decided,
use the remaining measured throughput cells to select at most one HZ10/HZ11
mechanism for a separate shadow-first experiment.

This is now the active next task under the name
`HZ8SpeedAdapterAttribution-L0`.

The first post-Mag16 medium/local behavior check is not a new mechanism.
`MediumLocalFastTierActiveRun-L1` already exists behind
`H8_MEDIUM_ENABLE_LOCAL_FAST_TIER` and was previously placed on HOLD. Windows
now exposes it as `hz8-v2-mediumlocalfast`, excluded from normal matrices unless
`-IncludeHz8Research` is supplied. This recheck answers whether the current
Mag16/default code shape changes the old result without mixing in new counters,
atomics, or another cache design.

Recheck gate:

- medium/local throughput must improve materially;
- main/local and fixed medium slices must not regress more than 3%;
- small and remote rows must not regress more than 5%;
- peak/post RSS must remain within 5%;
- safety gates remain zero.

If the candidate fails again, freeze the active-run fast-tier family. The next
medium/local design must remove fixed-cost work from the common entry rather
than add another branch or per-run state field.

Windows closeout (2026-07-11):

| Row | Baseline | Candidate | Ratio |
|---|---:|---:|---:|
| balanced | 68.84M | 67.18M | 0.976 |
| larger_sizes | 32.57M | 31.61M | 0.971 |
| fixed 8K | 66.05M | 64.08M | 0.970 |
| fixed 16K | 38.12M | 37.60M | 0.986 |

These are long AB repeat-5 medians with iteration counts raised by 10x over
the quick matrix rows. The candidate is NO-GO and the active-run fast-tier
family is frozen. The Windows research row remains available only to reproduce
the decision; it is excluded from normal matrices.

A one-run MT remote smoke agreed with the closeout direction: 120.93M ops/s
for the candidate versus 125.90M for default HZ8, with peak RSS increasing
from 18,672 KiB to 20,020 KiB. This row is supporting evidence only because
the effective remote ratios differed; it is not used as the primary gate.

Windows diagnostic result:

| Row | Active miss | Slow collect | Span commit | Owner scan steps |
|---|---:|---:|---:|---:|
| 16..256 | 2,569 | 2,569 | 2,569 | 0 |
| 16..2048 | 21,171 | 21,171 | 21,171 | 0 |
| 16..4096 | 80,538 | 80,538 | 80,538 | 0 |

When the active span is exhausted and owner pending count is zero, HZ8 skips
the owner list entirely. Locally freed older spans therefore remain reusable
but invisible, while the allocator commits a new span. This explains both the
small-row speed gap and the parking-lot RSS evidence without invoking route or
medium machinery.

Next narrow candidate: `HZ8ReusableSpanHint-L1`. Publish one owner-local
same-class span hint when local free makes a span reusable, validate it on
active miss, and fall back to the frozen path on stale/full hints. Keep remote
collector publication as a later, separately measured extension.

The single-hint reading was refined during implementation: HZ8 already uses
`active_spans[class]` as one reusable hint. Local frees overwrite that pointer,
losing previously reusable spans. `HZ8ReusableSpanMagazine-L1` therefore keeps
the displaced hints in a bounded 16-entry owner-local stack per class.

Windows status: `GO(candidate) / HOLD(default)`; see
`HZ8_REUSABLE_SPAN_MAGAZINE_L1.md`.

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
