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

General remote-tail candidate:
  hz5-general-region-outbox
```

Use this short preset for the current combined general candidate:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-hz5-general-region-outbox \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-general-region-outbox
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

SmallFront remote diagnostic:

hz5-smallfront-remote-outbox:
  sender-side associative owner/class outbox
  mirrors HZ4 lcache's shard x class outbox idea
  candidate only
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

## OwnerHub Finding

External review agreed with the OwnerHub direction in principle:

```text
shared owner-slot pending mask
per-front specialized inbox payloads
no generic RemoteEntry yet
```

This branch already implements that idea as `OwnerHub-R1/R2/R3`.
Initial R1 diagnostic runs used `HZ5_OWNERHUB_STATS=1` and are not raw
performance runs.

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
  not a broad default in the short raw comparison
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

Focused comparison, `threads=16`, `ws=400`, `remote=90`, repeat-3:

```text
main_r90:
  HZ4 corrected   93.28M
  tcmalloc        73.63M
  region-map      27.71M
  inbox           27.00M

large_only_r90:
  base-only       16.97M
  inbox           15.09M, but only 1/3 successful under 15s timeout
  region-map      10.07M
  tcmalloc         7.13M
  HZ4 corrected    6.87M

cross128_r90:
  HZ4 corrected   46.51M
  inbox           20.18M
  region-map      17.23M
  base-only       10.76M
  tcmalloc         7.37M
```

Interpretation:

```text
Region-map fixes the clean-safety timeout issue, but does not close the HZ4
cross128 r90 gap. The next performance work should inspect HZ4's cross128
remote path against HZ5's Small/Mid/Large remote handoff cost.
```

HZ4 path audit found one concrete difference: HZ4 lcache keeps multiple
sender-side outbox slots keyed by shard/class, while HZ5 SmallFront had one
remote batch slot total. `--linux-smallfront-remote-outbox` adds a 64-slot
associative owner/class outbox without changing SmallFront state validation.

Short check with `--linux-smallfront-remote-batch-cap 8`:

```text
main_r90:
  smalloutbox8 + region-map  28.08M
  region-map                 27.71M

cross128_r90:
  smalloutbox8 + region-map  18.59M
  region-map                 17.23M

large_only_r90:
  smalloutbox8 + region-map  17.09M
  region-map                 10.07M
```

Read:

```text
SmallFront outbox helps modestly and removes no safety checks, but it is not
the missing HZ4-level cross128 mechanism by itself.
```

MidFront sender outbox was added as the next HZ4-inspired A/B:

```text
--linux-midfront-remote-outbox
--linux-hz5-general-midoutbox
```

It keeps MidFront descriptor validation and state transitions unchanged, but
uses a small sender-side owner/class outbox instead of one remote batch slot.
The current candidate default is 4 slots; larger slot counts can retain spans
too long before owner reuse sees them.

Focused repeat-3, threads=16/ws=400/r90, stats unset:

```text
main:
  region baseline       30.87M
  midoutbox slots8      23.95M

mid_only:
  region baseline       37.42M
  midoutbox slots8      31.10M

cross128:
  region baseline       16.13M
  midoutbox slots8      26.17M
```

Read:

```text
MidFront outbox is useful evidence for the HZ4-style sender-outbox hypothesis,
especially on cross128, but it is not a default because main/mid_only regress.
The likely issue is delayed publication/reuse when MidFront traffic is the
dominant front-end.
```

Follow-up A/B added a timely-publish flag:

```text
--linux-midfront-outbox-flush-on-miss
```

That publishes matching-class sender outbox slots on MidFront local allocation
miss. It is useful evidence but not a broad win.

Focused repeat-3, threads=16/ws=400/r90, stats unset:

```text
slots4:
  main      35.30M vs region 32.78M
  cross128  24.51M vs region 20.06M
  mid_only  25.80M vs region 29.83M

class-flush:
  main      26.06M
  cross128  19.50M
  mid_only  28.73M
```

Read:

```text
slots4 is the better MidFront-outbox candidate.
flush-on-miss does not stabilize cross128 enough to justify default use.
MidFront outbox remains cross-size candidate only because mid_only regresses.
```

Broad repeat-5 confirmation:

```text
private/raw-results/linux/general_midoutbox_broad_r5_20260525_003432

main_r50:
  region     31.73M
  midoutbox  33.56M
  HZ4        82.14M
  tcmalloc   79.18M

main_r90:
  region     30.09M, ok 5/5
  midoutbox  22.88M, ok 3/5
  HZ4        69.96M
  tcmalloc   51.25M

cross128_r50:
  region     30.30M
  midoutbox  35.53M
  HZ4        42.58M
  tcmalloc   10.17M

cross128_r90:
  region     19.10M
  midoutbox  18.16M
  HZ4        43.36M
  tcmalloc    7.49M

mid_only_r90:
  region     33.64M, ok 5/5
  midoutbox  22.97M, ok 3/5
  HZ4        67.89M
  tcmalloc   50.18M
```

Decision:

```text
Do not promote MidFront outbox to the combined default.
It is useful evidence for r50/cross-size sender batching, but r90 failures and
mid_only regressions make it no-go for the next broad profile.
Keep `hz5-general-region-outbox` as the current combined candidate.
```

Perf stat spot-check, stats unset:

```text
private/raw-results/linux/remote_cost_perf_20260525_003835

mid_only_r90:
  HZ4:
    50.58M ops/s
    364.9 cycles/op
    216.7 instructions/op
    44.4 branches/op

  HZ5 region:
    21.89M ops/s
    456.2 cycles/op
    393.1 instructions/op
    87.6 branches/op

cross128_r90:
  HZ4:
    29.96M ops/s
    588.5 cycles/op
    339.1 instructions/op
    69.7 branches/op

  HZ5 region:
    21.98M ops/s
    510.8 cycles/op
    400.6 instructions/op
    90.0 branches/op
```

Interpretation:

```text
The clearest stable gap is MidFront remote-heavy instruction count:
HZ5 spends about 1.8x HZ4 instructions/op and about 2x branches/op on
mid_only_r90. The next design target should reduce MidFront remote free/drain
state-machine cost, not add more cross-front drain policy.
```

MidFront direct-free-state diagnostic:

```text
--linux-midfront-remote-direct-free-state
```

Design:

```text
default:
  remote free: ACTIVE -> REMOTE_PENDING
  owner drain: REMOTE_PENDING -> LOCAL_FREE

direct-free-state:
  remote free: ACTIVE -> LOCAL_FREE
  owner drain: verify LOCAL_FREE and push to owner cache
```

This keeps the first remote-free CAS and still rejects double-free before reuse,
but removes the owner-drain state CAS from the diagnostic lane.

Focused repeat-5, threads=16/ws=400/r90, stats unset:

```text
private/raw-results/linux/midfront_directfree_focus_20260525_004106

main:
  region      29.21M
  directfree  31.11M

mid_only:
  region      26.57M
  directfree  32.11M

cross128:
  region      21.46M
  directfree  20.10M
```

Perf spot-check:

```text
private/raw-results/linux/directfree_perf_20260525_004141

mid_only_r90:
  region      25.99M ops/s, 361.3 instructions/op
  directfree  25.98M ops/s, 362.3 instructions/op
```

Read:

```text
direct-free-state is promising enough to keep as candidate-watch for MidFront
r90 throughput, but the first perf spot-check did not prove an instruction
reduction. Treat it as an implementation hypothesis, not a confirmed cause.
```

Direct-drain follow-up:

```text
--linux-midfront-remote-trust-drain-state
```

This diagnostic requires `--linux-midfront-remote-direct-free-state`. The
freeing thread still performs the first `ACTIVE -> LOCAL_FREE` CAS, but owner
drain trusts inbox provenance and skips the `LOCAL_FREE` state load/check before
pushing the span to the owner cache.

Focused general repeat-3, stats unset:

```text
private/raw-results/linux/general_direct_trust_r3_20260525_010715

main_r90:
  region      25.91M
  directfree  31.09M
  trustdrain  23.43M

mid_only_r90:
  region      18.46M
  directfree  37.54M
  trustdrain  20.85M

cross128_r90:
  region      13.98M
  directfree  14.10M
  trustdrain  13.73M

large_only_r90:
  region      16.29M
  directfree  19.12M
  trustdrain  14.20M
```

Decision:

```text
trustdrain:
  no-go for promotion; it removes owner-drain state validation and does not win.

directfree:
  candidate-watch. It improved this focused general matrix and keeps the first
  remote-free CAS, but still needs broad repeat and safety smoke before
  promotion.
```

Broad repeat-5 result:

```text
private/raw-results/linux/directfree_broad_r5_20260525_011108

main_r90:
  region      20.68M
  directfree  25.85M
  HZ4         75.04M
  tcmalloc    51.74M

mid_only_r90:
  region      24.68M
  directfree  25.37M
  HZ4         83.65M
  tcmalloc    49.91M

cross128_r90:
  region      18.27M
  directfree  15.25M
  HZ4         29.31M
  tcmalloc     7.60M

large_only_r90:
  region      21.76M
  directfree  19.18M
  HZ4          6.63M
  tcmalloc     7.15M
```

Updated decision:

```text
directfree:
  useful main/mid remote-heavy candidate
  not a broad default because cross128_r90 and large_only_r90 regress versus
  region

default:
  --linux-hz5-general-region-outbox

candidate preset:
  --linux-hz5-general-directfree

HZ4-inspired dispatch diagnostic:
  --linux-hz5-general-midfirst

HZ4-inspired lookup diagnostic:
  --linux-hz5-general-midcache
```

## HZ4 Path Audit Finding

HZ4 is not just faster at the same MidFront design. Its mid remote-heavy path is
structurally different:

```text
HZ4 mid:
  ptr -> page by direct 4KiB mask
  owner page per class in TLS
  remote free pushes to page->remote_head
  owner allocation drains that page-local remote list

HZ5 MidFront-M1:
  ptr -> SmallFront miss lookup first in preload free()
  ptr -> MidFront hash page-map lookup
  one object per span for 4K/8K/16K/32K/64K
  remote free publishes through owner/class inbox
```

Worker audit also flagged preload free dispatch as a concrete fixed cost:
MidFront frees currently pay a SmallFront ownership miss before the MidFront
hit. Added diagnostic lane:

```text
--linux-hz5-general-midfirst
```

It keeps `general-region-outbox` routes but changes preload free dispatch from
`Small -> Mid -> Large` to `Mid -> Small -> Large`. This is safe as a
diagnostic because each front-end still validates descriptor ownership and
invalid owned-looking pointers fail closed.

Expected interpretation:

```text
mid_only improves:
  front dispatch miss cost matters

main/cross128 regresses:
  global reordering is not a default; use the result to justify a page-kind or
  front-hint ownership map instead
```

Full audit:

```text
docs/HZ5_HZ4_REMOTE_PATH_AUDIT.md
```

Short diagnostic results:

```text
midfirst:
  no-go. It regressed main, mid_only, and cross128 in the first smoke.

midcache:
  diagnostic-only. It slightly improved mid_only in one smoke but regressed
  main/cross128 badly.
```

Updated read:

```text
Do not chase global preload dispatch reordering.
Do not expect a two-entry MidFront lookup cache to close the HZ4 gap.
Next HZ4-inspired work should target sender rbuf grouping or the Mid/Large
route split.
```

## Next Technical Question

Why is HZ4 still much stronger on `cross128 r90`, and which part of HZ5's
Small/Mid/Large remote handoff is paying the extra cost?

Immediate design target:

```text
HZ4 vs HZ5 remote-path audit:
  compare cross128 r90 allocation/free paths
  identify per-free atomic, owner lookup, inbox publish, and drain costs
  keep region-map as the clean LargeFront ownership lookup candidate
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

Cleanup/modularization audit:
  docs/HZ5_LINUX_CLEANUP_AUDIT.md

LargeFront design and measurements:
  docs/HZ5_LARGEFRONT_L1_DESIGN.md

Paper benchmark policy:
  docs/HZ5_PAPER_BENCH_SUITE.md

Implementation map:
  docs/source-map.md

Full chronological log:
  current_task.md
```
