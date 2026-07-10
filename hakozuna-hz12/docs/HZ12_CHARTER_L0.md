# HZ12 Charter L0

## Thesis

HZ12 explores whether flush-granularity advisory ownership can make spans
reclaimable without putting ownership work on the malloc/free hot path.

The primary claim under investigation is low-RSS span recycling. Cross-owner
throughput is a secondary outcome, not the public headline.

## Standalone Baseline

HZ12 starts from a renamed copy of HZ11's proven Windows span core, kept under
`hakozuna-hz12/include` and `hakozuna-hz12/src` with a `hz12_*` public API.
This is a code-lineage decision, not a runtime dependency: HZ12 builds without
including, linking, or modifying HZ11 sources. The fork creates a stable place
to change ownership, adoption, and reclaim contracts without risking HZ11's
selected lane.

## The Fourth Position

```text
mimalloc:
  per-free owner check and remote handoff

tcmalloc:
  ownerless central batching

HZ11:
  ownerless current-holder recycling

HZ12:
  advisory owner routing at flush granularity
  bounded owner inbox with ownerless fallback
```

## Core Invariants

```text
1. Route and pointer validity remain independent of ownership metadata.
2. Ownership is advisory locality metadata, not a free-safety authority.
3. Normal free does not read owner metadata or perform an owner atomic.
4. Flush groups objects by owner only on a cold cache-overflow path.
5. Inbox overflow falls back to ownerless recycling; it never drops objects.
6. Dead owners are adopted or downgraded before their spans can be reclaimed.
7. Span reclaim requires a separately verified whole-span state, never an
   inference from an inbox count alone.
```

## Opening Evidence

The HZ11 Windows xowner pipeline is the opening evidence, not a performance
claim for HZ12. It guarantees 100% cross-owner frees and measured HZ11 7.45M
ops/s versus tcmalloc 27.35M ops/s in initial R3. This justifies testing an
ownership model; it does not justify an ownership mechanism by itself.

## L0-L2 Ladder

```text
L0 OwnerRoutingShadow:
  no behavior change; project routing, pressure, orphan, and reclaim signals

L1 BoundedOwnerInbox:
  flush-time mutex inbox and batch splice; ownerless overflow fallback

L2 ReclaimableSpan:
  verified span state, dead-owner adoption, bounded whole-span reclaim/decommit
```
