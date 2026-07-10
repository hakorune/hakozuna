# HZ12 Windows Token Drain Policy L4-C

## Purpose

L4-C measures the policy tradeoff exposed by L4-B without changing its default
control. The token pipeline accepts a power-of-two producer drain interval;
the default remains one drain per allocation.

## Matrix

```text
drain interval:
  1, 8, 16, 32, 64, 128, 256 allocations

record:
  ops/s
  inbox peak
  inbox-cap overflow count
  ownerless fallback objects
  registry/generation rejection
  retirement-gate completion
```

## Interpretation

An overflow is a bounded ownerless fallback, not an object drop. L4-C must not
change normal allocation/free or relabel an overflow-prone interval as the L4-B
safe control. A future candidate requires a reproducible throughput/RSS benefit
with bounded fallback and the same lifecycle completion contract.

## Result

The Windows 1P/1C one-second sweep is a no-go for default policy. Every lane
completed retirement with zero registry/generation rejection, confirming that
overflow uses the intended ownerless fallback. However, interval 8 produced
fallback in 2/10 focused runs and interval 16 in 3/10. Larger intervals also
showed cap pressure in the initial R3 sweep. No interval is therefore promoted;
the current 1,024-object cap and drain cadence remain diagnostic evidence only.
