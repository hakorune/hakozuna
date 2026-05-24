# HZ5 Linux Development Brief

This is the short entry point for the current HZ5 Linux allocator work. Keep
large measurement logs in `current_task.md` and route-specific design docs.

## Current Direction

HZ5 Linux is no longer only an exact `64K/a8192` sidecar. It is being rebuilt
as a general LD_PRELOAD allocator candidate:

```text
SmallFront-S1:
  ordinary malloc <= 2048

MidFront-M1:
  ordinary malloc 2049..65536

LargeFront-L1:
  ordinary malloc 65537..1048576

Local2P:
  exact 64K/a8192 appendix/special profile
```

Windows P43i/P45 remains separate and must not be weakened by Linux front-end
experiments.

## Current Best HZ5 Linux Rows

```text
Local exact 64K/a8192:
  hz5-local2p-linkflags

RSS-retained exact 64K/a8192:
  hz5-local2p-rssretain2048tls

Producer/consumer exact remote-free:
  hz5-local2p-remotebatch

General malloc local/mixed:
  hz5-smallfront-s1 + hz5-midfront-rb16 + hz5-largefront-l1

Remote-heavy large:
  hz5-largefront-inbox
```

Diagnostic-only LargeFront rows:

```text
hz5-largefront-rb16:
  remote batch cap16
  not a clear win

hz5-largefront-takefirst:
  direct remote drain return
  broad regression risk

hz5-largefront-map-base-only:
  maps only the returned base page
  timeout/root-cause diagnostic only
  weakens interior-pointer invalid-free attribution

hz5-largefront-region-map:
  source-region lookup instead of per-page insertion
  LargeFront-L2 candidate
  keeps interior-pointer invalid-free attribution
```

## Latest LargeFront Finding

LargeFront-L1 fixed the `cross128` fallback hole for local traffic.

Focused repeat-3 medians with `HZ5_PRELOAD_STATS` disabled:

```text
cross128 r0:
  tcmalloc  80.22M
  HZ5 L1    63.45M
  HZ4       30.79M

large_only r0:
  tcmalloc 104.12M
  HZ4       94.63M
  HZ5 L1    81.93M
```

Owner-inbox improved remote-heavy large traffic:

```text
large_only r90:
  L1 global recycle  3.66M
  owner inbox        8.81M

cross128 r90:
  L1 global recycle  3.03M
  owner inbox        6.51M
```

Remote batch and take-first did not become broad winners.

## Current Weakness

`cross128 r90` remains behind HZ4/tcmalloc. LargeFront-only tuning is no longer
the whole bottleneck:

```text
likely cause:
  cross-size remote handoff cost across SmallFront, MidFront, and LargeFront

not enough:
  LargeFront class-drain tuning only
  MidFront allgate only
  LargeFront remote batch only
```

## OwnerHub-R1 Finding

After the Pro review, the next direction is OwnerHub-R1:

```text
shared owner-slot pending mask
per-front specialized inbox payloads
no generic RemoteEntry yet
dry-run observation only
```

Initial diagnostic runs used `HZ5_OWNERHUB_STATS=1` and are not raw performance
runs.

Short r90 observations:

```text
cross128:
  publish_small=84 publish_mid=1977 publish_large=3090
  miss_any_pending=3302
  miss_target_pending=537
  miss_other_pending=3258

main:
  publish_small=373 publish_mid=4270 publish_large=0
  miss_any_pending=2143
  miss_target_pending=480
  miss_other_pending=2090

large_only:
  publish_large=8233
  miss_any_pending=414
  miss_target_pending=414
  miss_other_pending=0
```

Interpretation:

```text
large_only is a same-front problem.
main/cross128 frequently have other-front pending work at alloc miss.
OwnerHub-R2 coordinated drain has a real target.
```

OwnerHub-R2 bounded coordinated drain was implemented and measured. After
fixing the budgets to be total-front budgets instead of per-class budgets, it
helps some mixed rows but is still not a broad default:

```text
main r50:
  inbox  9.89M
  R2    10.89M

main r90:
  inbox  6.99M
  R2     8.69M

cross128 r50:
  inbox  8.98M
  R2     9.60M

cross128 r90:
  inbox  7.01M
  R2     6.32M

mid_only/large_only r90:
  R2 regressed versus inbox
```

Current decision:

```text
OwnerHub-R1:
  keep as observation evidence

OwnerHub-R2:
  mixed-workload candidate only
  needs lower-cost bookkeeping before it can be a default remote-heavy lane

OwnerHub-R3:
  implemented as the next candidate
  uses coarse per-front dirty bits instead of class-granular pending bits
  keeps per-front inbox payloads and fail-closed descriptor validation
  should be measured against R2 and the owner-inbox baseline without stats
```

Short raw repeat-3 results show R3 is not a broad default:

```text
main r90:
  inbox 7.54M
  R2    9.85M
  R3    6.97M

mid_only r90:
  inbox 7.73M
  R2    6.42M
  R3    7.71M

cross128 r90:
  inbox 6.23M
  R2    5.99M
  R3    6.18M
```

R3 reduces some R2 damage but does not create a general win. OwnerHub should
remain a candidate for selected mixed rows, while the default remote-heavy path
should stay per-front until a lower-cost handoff design is found.

## Latest Timeout Finding

The broad paper-shape matrix produced HZ5 r90 timeout tails. A focused
large_only r90 probe showed the tail was frequent enough to reproduce under a
10s timeout.

```text
threads=16, ws=400, large_only r90, iters=250000, timeout=10s, repeat20:
  hz5_inbox       ok=14 timeout=6
  hz5_ownerhub_r2 ok=3  timeout=17
  hz5_ownerhub_r3 ok=7  timeout=13
```

`perf` on a slow OwnerHub-R2 process attributed the hot samples to
`hz5_largefront_alloc`, around LargeFront page-map insertion. The current L1
map registers every 4KiB page in each retained large span, so a 128K span
performs 32 map insertions before reuse.

The base-only diagnostic validates the cause:

```text
--linux-largefront-map-base-only

large_only r90, repeat20:
  ok=20 timeout=0

cross128 r90, repeat10:
  ok=10 timeout=0

main r90, repeat10:
  ok=7 timeout=3
```

Decision:

```text
Do not make base-only default.
Implement a LargeFront-L2 range/region ownership map instead.
```

Region-map was implemented as the first L2 candidate:

```text
--linux-largefront-region-map

source refill:
  register one coarse source region

lookup:
  ptr -> 2MiB-granule bucket -> region -> span index -> descriptor
```

Focused check:

```text
/bin/true under full preload: OK
large malloc/free smoke: OK
interior free smoke: OK

large_only r90, repeat20:
  ok=20 timeout=0
  median successful ops/s about 15.15M

cross128 r90, repeat10:
  ok=10 timeout=0
  median successful ops/s about 16.33M

main r90, repeat10:
  ok=10 timeout=0
  median successful ops/s about 22.34M
```

Current read:

```text
Region-map is a cleaner replacement for base-only.
It should be compared in the next broad matrix before becoming the lead
LargeFront remote-heavy candidate.
```

## Next Technical Question

Should LargeFront region-map become the lead LargeFront row after a broad
comparison, or does it need a lower-overhead per-span/radix variant?

Immediate design target:

```text
LargeFront-L2 validation:
  compare region-map against hz5_inbox, base-only, HZ4, and tcmalloc
  focus on large_only/cross128/main r50/r90 at paper shape
  keep exact base-pointer free fast
  keep interior-pointer invalid frees fail-closed
```

The earlier cross-front handoff question remains relevant for Small/Mid remote
tails, but it is no longer the first timeout fix.

Candidate designs:

```text
1. LargeFront per-span range map:
   register one range per retained span
   simplest production replacement for per-page map insertion

2. LargeFront source-region map:
   register one larger source region and resolve span by descriptor metadata
   more complex, potentially lower insertion cost under batch source allocation

3. OwnerHub-R2 coordinated drain:
   shared owner pending mask
   per-front specialized inbox payloads
   drain bounded Small/Mid/Large work on alloc miss

4. Full shared remote handoff layer:
   one owner-slot inbox shared by SmallFront/MidFront/LargeFront
   entries carry front-end kind + class + pointer
   not recommended yet

5. Benchmark-shape SPSC transfer lane:
   optimize producer/consumer handoff directly
   likely diagnostic/paper appendix, not default allocator

6. Stop remote tuning for now:
   treat HZ5 as local/mixed candidate
   document remote-heavy as future work
```

## Where To Read Next

```text
Route and lane names:
  docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md

LargeFront design and measurements:
  docs/HZ5_LARGEFRONT_L1_DESIGN.md

Paper benchmark policy:
  docs/HZ5_PAPER_BENCH_SUITE.md

Implementation map:
  docs/source-map.md

Full chronological log:
  current_task.md
```
