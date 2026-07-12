# HZ8 Page8K Range4097 L1

## Scope

This lane tests whether requests from `4097..8192` should use the existing
8KiB page geometry. It does not change the eight-slot, 64KiB page run and it
does not change `hz8-v2`.

```text
eligible:
  4097..8192 bytes

geometry:
  8192-byte slots
  8 slots per 64KiB page run

other sizes:
  existing HZ8 routes
```

The lane is enabled only by `H8_MEDIUM_PAGE8K_RANGE4097_L1`. Diagnostic builds
record eligible and served allocations; the public speed lane has no added
counter or atomic.

## Correctness

Linux and Windows range API smokes pass. The Windows diagnostic smoke reports:

```text
range eligible=3 served=3
```

Existing page8K remote, lifecycle, residency, thread-turnover, and safety
checks remain passing. The range probe therefore did not expose a fallback or
ownership safety defect.

## Focused Windows Result

Five direct runs were collected with `T=4`, `iters=120000`, `working_set=2048`,
and `size=4097..8192`. This is a directional focused result, not a paper
matrix or a promotion gate.

| Lane | Median ops/s | Relative to HZ8 v2 |
|---|---:|---:|
| `hz8-v2` | 25.09M | 1.00x |
| `hz8-r3-page8k-integrated` | 25.01M | 1.00x |
| `hz8-r3-page8k-range4097` | 21.90M | 0.87x |

All three lanes completed `242432/242432` allocation attempts with zero
allocation failures. The range lane is therefore slower even when it serves
the tested page allocations successfully. The result is consistent with the
page8K route/remote lifecycle fixed cost outweighing the benefit of replacing
the existing medium classes for this size interval.

## Decision

```text
correctness:
  GO

speed candidate:
  NO-GO

public default:
  unchanged hz8-v2

future:
  retain the lane as reproducible evidence only; do not open a cap or geometry
  ladder from this result
```

This closes the first class-wide page8K expansion attempt. The exact-8192 R3
lane remains a separate Windows opt-in candidate; this result does not change
its status.
