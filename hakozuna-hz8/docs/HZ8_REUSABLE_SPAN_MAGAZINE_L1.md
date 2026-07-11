# HZ8 Reusable Span Magazine L1

## Problem

Speed attribution showed that an active miss becomes a new span commit whenever
owner pending count is zero. HZ8 intentionally skips its owner list in that
case. The existing `active_spans[class]` pointer is a single reusable hint, but
each local free can replace it and lose the previous reusable span.

In the Windows diagnostic 16-4096 row, active miss, slow collect, and span
commit were all exactly 80,538 while owner-list scan steps were zero.

## Candidate

`H8_REUSABLE_SPAN_MAGAZINE_L1` adds a bounded owner-local stack:

```text
classes: H8 small classes only
capacity: 16 span pointers per class
producer: same-owner local free
consumer: active-span exhaustion
validation: owner, generation, class, state, and local availability
fallback: unchanged HZ8 v2 path
```

When a local free replaces the class active pointer, the displaced pointer is
stored if capacity remains. Allocation pops and validates stale/full entries
only after the active span is exhausted. It does not change remote publication,
route authority, pending bits, MediumRun, or LargeDirect behavior.

The feature is compile-time opt-in. Default HZ8 has no extra fields, branch, or
hot-path store.

## Windows R5

Same machine and runner, baseline and candidate alternating:

| Row | HZ8 v2 median | Mag16 median | Ratio |
|---|---:|---:|---:|
| 16..256 | 133.49M | 420.65M | 3.15x |
| 16..2048 | 18.02M | 46.10M | 2.56x |
| 16..4096 | 9.65M | 39.61M | 4.11x |

The long-lived 16-4096 peak-RSS probe measured:

```text
HZ8 v2:  5020-5033 MiB
Mag16:   2107-2109 MiB
change:  approximately -58%
```

The default remote90 shape measured median throughput of 122.05M for HZ8 v2
and 123.20M for Mag16. Effective remote ratios varied between processes, so
this is a neutrality signal rather than a throughput claim. Median committed
small-span bytes fell from 1,294,532,608 to 733,216,768, approximately 43%.

All allocations/frees completed, and the Windows default, adoption, reclaim
shadow, adaptive shadow, and dedicated Mag16 smokes pass. The pthread-based
safety-stress source is not directly buildable with the Windows clang-cl lane;
Linux safety-stress remains required before promotion.

## Decision

```text
Windows candidate:
  GO

HZ8 default promotion:
  HOLD

remaining gates:
  Linux GCC/Clang smoke and safety stress
  Linux paired local/remote/RSS matrix
  Windows broader public rows and repeatable effective-remote control
```

This candidate is HZ8-native. It imports the lesson that reusable front-end
inventory must remain visible, not an HZ11 transfer-cache implementation.
