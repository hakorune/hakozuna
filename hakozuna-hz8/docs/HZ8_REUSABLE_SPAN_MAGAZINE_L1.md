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
  PROMOTED after cross-platform and broader Windows gates

control lane:
  hz8-v2-nomag
```

## Linux Gate

Ubuntu x86_64 GCC and Clang smoke/safety stress pass. Paired repeat-5 local
churn confirms large throughput and peak-RSS improvements, including
16..4096 peak RSS reduction from about 4.93 GiB to 2.02 GiB.

The initial public remote matrix was mixed: main remote90 improved about 10%, but small
remote90 throughput regresses about 9.7% and peak RSS rises from about 20.75
MiB to 31.00 MiB. Linux correctness was GO, but broad default promotion was
initially held. Full results are in
`docs/benchmarks/linux/HZ8_REUSABLE_SPAN_MAG16_20260711.md`. The subsequent
full-magazine churn fix removed that blocker.

### Linux remote follow-up

Root-cause attribution found that a full Mag16 still replaced the active span
on every local free. With no inventory slot available, that discarded a usable
active hint without preserving it. The candidate now keeps the current active
span when the magazine is full; this remains inside the Mag16 compile-time box.

A fresh-process focused remote90 repeat-30 reduced the observed gap to -1.22%
with equal 8.125 MiB peak RSS and near-equal post RSS. Focused local repeat-5
remained positive at +7.66%. GCC/Clang smoke and safety stress still pass.

This candidate is HZ8-native. It imports the lesson that reusable front-end
inventory must remain visible, not an HZ11 transfer-cache implementation.

## Windows full-magazine follow-up

Windows was rebuilt from commit `652fa283` and rerun after the full-magazine
churn fix. The local comparison used alternating baseline/candidate order,
five fresh processes per row, eight threads, five million iterations per
thread, and a working set of 16,384.

| Row | HZ8 v2 median | Mag16 median | Ratio | HZ8 v2 peak | Mag16 peak |
|---|---:|---:|---:|---:|---:|
| 16..256 | 134.49M | 329.35M | 2.45x | 3416.09 MiB | 1397.40 MiB |
| 16..2048 | 18.58M | 24.77M | 1.33x | 26500.74 MiB | 22421.15 MiB |
| 16..4096 | 3.34M | 9.63M | 2.88x | 50588.16 MiB | 48179.32 MiB |

This long-lived probe is deliberately harsh and produced much higher absolute
resident pressure than the earlier Windows R5. Use it as a same-session A/B,
not as a replacement for the earlier absolute-RSS snapshot.

The fixed MT remote runner (`T=16`, two million iterations, `ws=400`,
`size=16..2048`, target remote 90%, R5) measured:

| Row | Median ops/s | Actual remote | Fallback | Median peak RSS |
|---|---:|---:|---:|---:|
| HZ8 v2 | 124.891M | 79.70% | 11.45% | 32.20 MiB |
| Mag16 | 131.737M | 77.43% | 13.97% | 18.71 MiB |

The effective remote ratios differ, so the throughput delta is supporting
evidence rather than a remote-speed claim. The important gate result is that
the Linux full-magazine regression does not reproduce on Windows: throughput
remains positive/neutral and peak RSS is lower. One initial baseline process
ended with a non-reproducing Windows fast-fail; direct reruns and all ten runs
in the recorded R5 completed successfully.

## Promotion

The broader Windows repeat-5 gate remained positive after the churn fix:

| Row | No-Mag median | Mag16 median | Ratio |
|---|---:|---:|---:|
| balanced | 18.60M | 52.85M | 2.84x |
| wide_ws | 32.86M | 55.15M | 1.68x |
| larger_sizes | 14.27M | 22.75M | 1.59x |

Together with Linux GCC/Clang safety, Linux focused local/remote, Windows
smoke, long-lived local A/B, and fixed MT remote R5, this clears the default
gate. `H8_REUSABLE_SPAN_MAGAZINE_L1` now defaults to `1`; define it as `0` only
for the explicit pre-promotion control lane.
