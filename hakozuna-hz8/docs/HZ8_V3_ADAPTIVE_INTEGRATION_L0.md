# HZ8 v3 Adaptive Integration L0

## Charter

HZ8 remains the public allocator family. HZ8 v2 / KeepRefill stays frozen as
the recommended default while v3 evaluates whether proven HZ11 and HZ12
mechanisms can be integrated without weakening the HZ8 safety/RSS contract.

```text
HZ8 v2:
  current public/recommended default
  balanced low-RSS contract

HZ8 v3 research:
  same HZ8 family
  not public default
  no new HZ generation
```

The goal is not to embed multiple complete allocators and select one on every
allocation. The goal is one HZ8 core whose bounded slow paths can adapt class
policy at refill, flush, epoch, and scavenge boundaries.

## Evidence Sources

```text
HZ11 Windows fine128 transfer:
  balanced      422.016M, 81.7% of tcmalloc
  wide_ws       313.499M, 73.4% of tcmalloc
  larger_sizes  365.921M, 149.0% of tcmalloc
  local rows within -3% of selected HZ11
  remaining gap: wide_ws RSS +15.0% versus tcmalloc

HZ12:
  advisory ownership and cold-path reclaim authority research
  source for whole-span reclaim rules, not a speed backend to copy wholesale
```

HZ11 proves that bounded batch transfer is a strong throughput mechanism.
HZ12 defines the research path for recovering span residency without adding
per-free ownership accounting. HZ8 v3 should combine their contracts only
after each mechanism passes an HZ8-specific shadow gate.

## Non-Negotiable Invariants

```text
- HZ8 v2 binaries and defaults remain unchanged during L0/L1 research.
- MISS / VALID / INVALID fail-closed routing remains authoritative.
- owned-looking INVALID never falls through to the system allocator.
- remote pending/qstate correctness is not replaced by ownerless transfer.
- no per-op runtime profile selector.
- no production hot-path diagnostics or accounting atomics.
- mode changes occur only at bounded slow-path checkpoints.
- no HZ13 or new generation for a backend-policy experiment.
```

## Proposed Class Policy States

The first behavior-capable design may use three class-local states:

```text
BALANCED:
  current HZ8 v2 policy

TRANSFER_PRESSURE:
  bounded batch reuse is preferred before new source admission

RSS_PRESSURE:
  retained-empty budget is reduced and reclaim is preferred
```

L0 does not change behavior. It computes a recommendation in diagnostic builds
from existing slow-path events and records whether the recommendation would
have changed. The allocator continues to execute BALANCED behavior.

## L0 Signals

Use only events already occurring off the hot path:

```text
refill pressure:
  source/refill request after local reuse miss

transfer opportunity:
  bounded reusable objects observed at refill/flush boundaries

remote pressure:
  inbox/collector drain work at existing checkpoints

RSS pressure:
  retained-empty or resident-byte budget crossing
```

Counters are compiled only into a dedicated diagnostic lane. The speed/default
build receives no new fields, atomics, branches, or callbacks.

## Implementation Ladder

```text
L0:
  shadow-only class governor
  no behavior change
  diagnostic lane only

L1:
  small-class bounded transfer adapter
  only if L0 predicts useful transfer pressure consistently

L2:
  cold-path span reclaim adapter
  only if retained-byte evidence identifies reclaimable residency

L3:
  combined hysteresis and cross-platform gate
  only after L1 and L2 pass independently
```

## Acceptance / No-Go

L0 acceptance:

```text
- default/speed HZ8 object code is unchanged by the feature-off build
- diagnostic lane reports class-attributed pressure at slow-path checkpoints
- repeated workloads produce stable recommendations
- no safety, route, remote, teardown, or RSS behavior changes
```

L1 continuation gate:

```text
- balanced broad MT reaches at least 70% of tcmalloc
- local rows remain within -3% of HZ8 v2
- remote-heavy public rows do not regress by more than 5%
- RSS remains within 10% of HZ8 v2 and does not exceed tcmalloc on wide_ws
```

Immediate no-go:

```text
- per-allocation mode checks or new production atomics
- owner/pending safety weakened to obtain transfer throughput
- wide_ws RSS traded away without a reclaim path
- one workload-specific threshold promoted as a general runtime policy
- HZ11 or HZ12 core copied wholesale into HZ8
```

## First Box

```text
HZ8AdaptiveTransferShadow-L0

scope:
  diagnostic build only
  class-local slow-path observations
  BALANCED / TRANSFER_PRESSURE / RSS_PRESSURE recommendation
  no allocator behavior change
```
