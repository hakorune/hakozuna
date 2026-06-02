# HZ6 Lane Guide

This document is the compact lane map for HZ6 Windows benchmarking. Keep long
experiment notes in `current_task.md`; use this file to answer "which lane is
which?" before running or comparing benchmarks.

## Current Recommendation

| Profile family | Selected HZ6 profile | Selected capacity lane | Why this lane now |
| --- | --- | --- | --- |
| balanced / wide_ws low-RSS speed | `rss` | `descavail-noboost-route4k` | Best balanced and wide_ws low-RSS speed lane; descriptor exhaustion cost is removed without changing capacity. |
| random_mixed same-owner speed | `strict` | `sameownerfast-descavail-noboost-route4k` | Selected same-owner fast lane: `HZ6_SAME_OWNER_FAST_L1` + descriptor availability, promoted from the A-ladder. |
| larger_sizes RSS/speed | `speed` or `rss` | `largerlowrss-front8k-sourcerun-desc8k-route8k` | Best larger_sizes lane; needs larger front retention, not more descriptor-failure cleanup. |
| Larson cross-owner full 10k | `speed` | `ownerlocalityfast-rsscap-1` | Current full Larson cross-owner candidate-control; appcap-class throughput with much lower peak RSS. |
| perf-recovery upper-bound | `strict` / `speed` / `rss` | `ownerlocalityfast-appcap` | Upper-bound / completion control only; too much RSS for default use. |

For a cross-allocator side-by-side summary using past data only, see
[`HZ6_CROSS_ALLOCATOR_COMPARISON.md`](HZ6_CROSS_ALLOCATOR_COMPARISON.md).

```text
Windows profile family:
  balanced / wide_ws low-RSS speed:
    HZ6 profile:
      rss
    capacity lane:
      descavail-noboost-route4k

  random_mixed same-owner speed:
    HZ6 profile:
      strict
    capacity lane:
      sameownerfast-descavail-noboost-route4k

  larger_sizes-rss-speed:
    HZ6 profiles:
      speed or rss
    capacity lane:
      largerlowrss-front8k-sourcerun-desc8k-route8k
    close candidate-control:
      largerlowrss-front6k-sourcerun-desc8k-route8k

  Larson cross-owner full 10k:
    HZ6 profile:
      speed
    capacity lane:
      ownerlocalityfast-rsscap-1
    lower-RSS sibling:
      ownerlocalityfast-rsscap-2
    compact/moderate live-set evidence:
      ownerlocalityfast-rsscap-4

  perf-recovery upper-bound:
    ownerlocalityfast-appcap

  redis-evidence:
    redislowrss-sourcerun-desc8k-route8k
    redislowrss-sourcerun-desc8k-route8k-tombcompact

Primary controls:
  route4k
  noboost-route4k
  descavail-noboost-route4k

Low-capacity / low-RSS baseline:
  control

Redis-like candidate-control:
  redislowrss-route4k
  redislowrss-slim-route4k

Capacity / completion control:
  appcap

Route-lifecycle diagnostic:
  visiblefirst-appcap
  negativefilter-appcap
  sharedir-appcap
  sharedirfirst-appcap
  ownerlocality-appcap
  ownerlocalityfast-appcap
  ownerlocalityfast-rsscap-1
  ownerlocalityfast-rsscap-2
  ownerlocalityfast-rsscap-3
  ownerlocalityfast-rsscap-4
  ownerlocalityfast-widecap-1
  ownerlocalityfast-widecap-2
  ownerlocalityfast-widecap-3
  ownerlocalityfast-widecap-4

Evidence-only source-run controls:
  sourcerun-route4k
  sourcerun-sameclass-route4k

Descriptor lifecycle prototype:
  descavail-noboost-route4k
  sameownerfast-descavail-noboost-route4k

Same-owner A-ladder evidence:
  directlocalfree-descavail-noboost-route4k
  directlocalalloc-descavail-noboost-route4k
  directlocalreuse-descavail-noboost-route4k
  directlocalfreealloc-descavail-noboost-route4k
  directlocalfreereuse-descavail-noboost-route4k

Descriptor lifecycle evidence:
  descriptorless-route4k
  descriptorreserve-route4k
  descriptorcold-route4k
  descriptorcoldgov-route4k

Frozen no-go controls:
  spill-route4k
  borrow-route4k
  cap-route4k
  sourcerun-reclaim-route4k
```

## Recommended Lane Sets

Use these sets before opening another capacity sweep. They keep HZ6 as a
profile family and avoid accidentally promoting an evidence lane outside the
row where it was measured.

```text
balanced / wide_ws low-RSS speed:
  HZ6 profile:
    rss
  capacity lane:
    descavail-noboost-route4k

random_mixed same-owner speed:
  HZ6 profile:
    strict
  capacity lane:
    sameownerfast-descavail-noboost-route4k

larger_sizes RSS/speed:
  HZ6 profile:
    speed for throughput, rss for lower RSS
  capacity lane:
    largerlowrss-front8k-sourcerun-desc8k-route8k
  tighter-retention candidate-control:
    largerlowrss-front6k-sourcerun-desc8k-route8k

Larson cross-owner full 10k:
  HZ6 profile:
    speed
  capacity lane:
    ownerlocalityfast-rsscap-1
  lower-RSS sibling:
    ownerlocalityfast-rsscap-2
  compact/moderate live-set only:
    ownerlocalityfast-rsscap-4

performance upper-bound / completion control:
  ownerlocalityfast-appcap

generic safety/control baseline:
  route4k

capacity completion control:
  appcap

Redis evidence only:
  redislowrss-sourcerun-desc8k-route8k
  redislowrss-sourcerun-desc8k-route8k-tombcompact
  redislowrss-sourcerun-desc8k-route8k-slotlookup
```

Recommended comparison matrix for Windows HZ6 mixed profiles:

```powershell
.\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles balanced,wide_ws,larger_sizes `
  -Hz6Profiles strict,speed,rss `
  -CapacityLanes noboost-route4k,descavail-noboost-route4k,sameownerfast-descavail-noboost-route4k,largerlowrss-front8k-sourcerun-desc8k-route8k,largerlowrss-front6k-sourcerun-desc8k-route8k,ownerlocalityfast-rsscap-4,ownerlocalityfast-appcap
```

Next Windows focus:

```text
Profile-family read:
  rss + descavail-noboost-route4k is the current balanced / wide_ws low-RSS
  speed candidate-control.
  strict + sameownerfast-descavail-noboost-route4k is the current
  random_mixed same-owner speed candidate-control.
  largerlowrss-front8k-sourcerun-desc8k-route8k is the current larger_sizes
  RSS/speed candidate-control.
  ownerlocalityfast-rsscap-1 is the current full Larson cross-owner
  candidate-control; ownerlocalityfast-rsscap-2 is the lower-RSS sibling.
  ownerlocalityfast-rsscap-3/4 are too tight for full 10k Larson and should be
  used only for compact/moderate live-set evidence.
  ownerlocalityfast-rsscap-4 is now a historical larger_sizes high-RSS
  comparison control.
  ownerlocalityfast-appcap is the perf-recovery upper-bound / completion
  control, not the default.
  Redis lanes are frozen as evidence-only.

Wide / mixed profiles:
  noboost-route4k
  descavail-noboost-route4k
  sameownerfast-descavail-noboost-route4k
  largerlowrss-front8k-sourcerun-desc8k-route8k
  ownerlocalityfast-rsscap-4

Next experiment:
  do not start another static capacity sweep until the profile-family matrix
  above is enough for the paper/dev comparison table.

Do not:
  reopen Redis capacity tuning as the next broad HZ6 track
  treat ownerlocalityfast-appcap as promotion while peak working set remains
  appcap-sized
```

`ownerlocalityfast-rsscap-*` is the perf-recovery RSS reduction family, not an
existing promotion lane. The first larger_sizes D-lite read shows that the row
is `256..8192` mid/source-block pressure, not the LargeSpan front:
`large_span_central_push/pop/source_alloc` stay at zero.

```text
ownerlocalityfast-rsscap-1:
  transfer/frontcache trim only.
  Performance is preserved, but peak is essentially unchanged.
  Keep as no-op evidence.

ownerlocalityfast-rsscap-2:
  rsscap-1 plus source-block capacity 8192.
  larger_sizes run1 drops peak by roughly 57 MiB versus appcap and preserves
  or improves throughput with safety counters clean.
  Promising evidence; superseded by rsscap-3 if guard rows stay clean.

ownerlocalityfast-rsscap-3:
  rsscap-2 plus descriptor capacity 131072.
  larger_sizes run1 drops peak again to about 191..196 MiB with clean safety
  counters. Strong evidence; superseded by rsscap-4 on larger_sizes if guard
  rows are acceptable.

ownerlocalityfast-rsscap-4:
  rsscap-3 plus route capacity 131072.
  larger_sizes run1 and repeat-3 drop peak again to about 166..172 MiB and
  keep safety counters clean. Current strongest larger_sizes RSSCap
  candidate-control, but not broad promotion: wide_ws guard regresses versus
  ownerlocalityfast-appcap.

ownerlocalityfast-widecap-1:
  appcap descriptor / route / source-block capacity, with transfer/frontcache
  kept wide at 32768. Wide_ws restores appcap-class speed, but peak remains
  appcap-sized. Keep as wide hot-cache evidence.

ownerlocalityfast-widecap-2:
  appcap descriptor / route / source-block capacity, with transfer/frontcache
  at 16384. Wide_ws still restores most appcap-class speed with appcap-sized
  peak. Use as the narrow hot-cache baseline for further wide_ws RSS trims.

ownerlocalityfast-widecap-3:
  widecap-2 plus source-block capacity 8192. Wide_ws keeps speed near
  widecap-2 while dropping peak by roughly 110 MiB in speed/rss profiles.
  Current source-block trim evidence for wide_ws.

ownerlocalityfast-widecap-4:
  widecap-3 plus descriptor capacity 131072. Wide_ws drops peak further
  while keeping clean safety counters. Wide_ws repeat-3 keeps or improves
  throughput versus appcap / widecap-2 and lowers peak to about 351..773 MiB
  depending on strict/speed/rss profile. Balanced guard is acceptable.
  Larger_sizes guard is weaker than rsscap-4, so this is the current wide_ws
  RSS/speed candidate-control, not a broad promotion.
```

`route4k` is the current HZ6 Windows lane to use first when checking whether
HZ6 remains low-RSS while avoiding tiny-route-table artifacts. It is not a
promotion claim. It is the cleanest candidate-control shape so far.

`appcap` proves whether a workload can complete when HZ6 gets very large
descriptor / route / transfer / source-block / front-cache capacities. It is a
diagnostic capacity control, not the default production shape.

`visiblefirst-appcap` is a diagnostic-only route-lifecycle upper-bound test.
It uses appcap capacity plus `HZ6_VISIBLE_FIRST_FREE_L1=1` and must be built
with `HZ6_DIAGNOSTIC_PROBES=1`. Use it to measure whether skipping expensive
worker-local route MISS scans can recover Larson main-warmup; do not use it as
a production or paper-facing lane.

## Capacity Lanes

```text
default:
  Tiny R1 smoke/control shape. Useful for build sanity, not performance claims.

broad:
  Historical broad-capacity control. Often faster than tiny lanes, but can
  raise RSS substantially.

control:
  Low-capacity baseline: small descriptor / route / transfer / source-block /
  front-cache limits. Useful for seeing how low-RSS HZ6 behaves under pressure.

route4k:
  control plus a 4096-entry route table. This isolates route capacity while
  keeping other capacities near control values.

noboost-route4k:
  route4k plus `HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1`. This keeps the
  low-capacity shape but prevents alloc-fail pressure from increasing source
  refill batch size. Latest repeat-3 strongly improves balanced and wide_ws
  while preserving larger_sizes, and random_mixed repeat-3 is neutral to
  positive. Treat it as the current Windows low-capacity candidate-control
  lane.

redislowrss-route4k:
  Redis-like low-RSS candidate-control. It keeps the route table at 4096,
  combines descriptor capacity 4096 with source-block capacity 512, keeps
  frontcache/transfer modest, and disables starvation source-refill boost.
  L0 redis_short showed descriptor capacity is the main collapse point and
  descriptor+source-block capacity gives a better Redis SET/LPUSH/RANDOM shape
  without moving all the way to appcap. Mixed_ws guard strongly regresses, so
  keep this lane Redis-like only; do not use it as a general HZ6 primary lane.

redislowrss-slim-route4k:
  Redis-like slim candidate-control. It keeps the same 4096 route table and
  starvation-boost disable, but trims descriptor capacity to 2048 and
  source-block capacity to 256. Use this as the next narrow Redis experiment
  after `redislowrss-route4k` if we want a smaller RSS footprint or less
  retention pressure while keeping Redis-like completion behavior. Latest L0
  checks made it the better Redis-like balance than `redislowrss-route4k`,
  while mixed_ws/random_mixed still remain guards rather than adoption targets.
  The staged `redis_long` row completed on this lane at 29.0 MiB peak working
  set while `noboost-route4k` timed out, so treat this as BURST_SUPPLY
  upper-shape evidence for the future control plane rather than as a new
  general default. Diagnostic builds now also surface ControlPlane-R1 dry-run
  counters on Redis rows, which is the bridge to the next behavior-gated
  experiment.

appcap:
  High-capacity application-like control. Use it to separate capacity failure
  from policy failure. Do not treat it as the HZ6 default lane.

visiblefirst-appcap:
  Diagnostic-only appcap variant. In non-strict `free()`, visible/shared route
  lookup is tried before local route lookup; visible MISS falls back to local
  lookup so local INVALID is not converted to MISS. This is an upper-bound
  probe for cross-owner warmup, not a promotion lane. It is now treated as
  no-go evidence rather than a production candidate.

negativefilter-appcap:
  Diagnostic-only appcap variant. It uses a conservative local owned-range hint
  to skip local route lookup only when the pointer is definitely not local.
  Diagnostic builds shadow-verify the skip and record false-skip counters.
  The source-block-only hint is now no-go as an optimization lane because
  route rehome can create local exact routes without local source-block
  ownership. Keep this lane as evidence that the next design needs rehome-aware
  owner ranges or a shared route directory.

sharedir-appcap:
  Diagnostic-only appcap variant. It publishes exact routes into a process-wide
  shared route directory and probes it only after local MISS. Behavior is
  unchanged. Use it to measure whether a shared directory could avoid the
  worker-local route MISS scan in cross-owner warmup.

sharedirfirst-appcap:
  Experimental behavior variant. After the allocator has observed a foreign
  visibility hit, it tries the shared directory first and reconstructs exact
  route results from the directory. Compact/moderate main-warmup can recover
  strongly, but 10k-chunk stress main-warmup currently times out. Keep it
  evidence-only until large live-set scaling is fixed.

ownerlocality-appcap:
  Diagnostic-only hint/backend lane. It uses a cheap owner-locality index to
  decide whether a pointer is definitely foreign, then consults the shared
  route directory backend for foreign exact lookup before falling back to the
  ordinary local route path. Use it to measure whether a low-cost locality hint
  can prune worker-local MISS scans without turning source-block ownership into
  the only truth source. The Larson runner now releases each worker live set
  before worker allocator teardown, so main-warmup no longer adds an accidental
  post-measurement owner-death cleanup stress on top of the timed cross-owner
  handoff lane.

ownerlocalityfast-appcap:
  Non-diagnostic owner-locality behavior lane. It uses the same owner-locality
  index and shared-directory exact backend as `ownerlocality-appcap`, but does
  not force `HZ6_DIAGNOSTIC_PROBES=1`. Use it after the diagnostic lane has
  shown clean counters to check whether the route-lifecycle fix is still fast
  without diagnostic probe overhead. Repeat-3 now makes this the preferred
  candidate-control lane for the fast Larson owner-locality comparison rows.

ownerlocalityfast-rsscap-1:
  Non-diagnostic owner-locality behavior lane with appcap descriptor / route /
  source-block capacity, but reduced transfer and frontcache capacity. Use it
  as the current full 10k Larson cross-owner candidate-control. Latest run1
  matches `ownerlocalityfast-appcap` throughput while cutting peak from
  roughly 2.82GB to roughly 1.23GB. It is also evidence that larger_sizes peak
  is not primarily transfer/frontcache capacity.

ownerlocalityfast-rsscap-2:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, and source-block capacity. Current larger_sizes run1 keeps or
  improves throughput while cutting peak working set by about 57 MiB versus
  ownerlocalityfast-appcap. In full 10k Larson it is the lower-RSS sibling of
  rsscap-1: lower peak, slightly lower throughput, safety clean. Treat as
  source-block trim evidence; rsscap-3 is the stronger follow-up only when
  descriptor trim is safe for the target row.

ownerlocalityfast-rsscap-3:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, source-block, and descriptor capacity. Current larger_sizes run1
  keeps safety counters clean and lowers peak to about 191..196 MiB while
  preserving appcap-class throughput. It fails full 10k Larson warmup, so treat
  it as RSSCap evidence only, not as the cross-owner full-stress lane.

ownerlocalityfast-rsscap-4:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, source-block, descriptor, and route capacity. Current
  larger_sizes run1 lowers peak to about 166..172 MiB and improves throughput
  versus rsscap-3, with safety counters clean. Treat as the current
  larger_sizes RSSCap candidate-control only; wide_ws guard regresses versus
  ownerlocalityfast-appcap. It is excellent for compact/moderate Larson
  live-set checks, but too tight for full 10k Larson warmup.

directlocalfree-ownerlocalityfast-rsscap-4:
  ownerlocalityfast-rsscap-4 plus DirectLocalFree-L1. Use this to test whether
  the same-owner free-path improvement also helps the larger_sizes selected
  lane without changing its RSS-capacity shape. This is a selected-family
  control lane, not a default replacement. Repeat-3 is mixed: larger_sizes and
  some rss rows improve, but balanced strict and wide_ws speed regress.

ownerlocalityfast-widecap-1:
  Non-diagnostic owner-locality behavior lane for wide_ws. It keeps appcap
  descriptor / route / source-block capacity, but sets transfer/frontcache to
  32768. It shows that wide_ws needs a larger hot cache than rsscap-1.

ownerlocalityfast-widecap-2:
  Same as widecap-1, but transfer/frontcache are 16384. It preserves
  appcap-like source allocation behavior for wide_ws and is the baseline for
  trimming source-block / descriptor capacity without reverting to rsscap-1.

ownerlocalityfast-widecap-3:
  widecap-2 plus source-block capacity 8192. Current wide_ws run1 cuts peak
  versus widecap-2 while keeping clean safety counters and largely preserving
  throughput.

ownerlocalityfast-widecap-4:
  widecap-3 plus descriptor capacity 131072. Current wide_ws run1 cuts peak
  further and keeps safety counters clean. Repeat-3 confirms the wide_ws shape:
  speed/rss improve versus appcap while peak drops substantially. Balanced is
  acceptable, but larger_sizes remains worse than rsscap-4. Treat as wide_ws
  candidate-control evidence, not a universal ownerlocalityfast replacement.

directlocalfree-ownerlocalityfast-widecap-4:
  ownerlocalityfast-widecap-4 plus DirectLocalFree-L1. Use this to test whether
  the same-owner free-path improvement lifts the wide_ws selected lane while
  preserving its existing RSS profile. First selected-family read regressed
  wide_ws, so keep it as no-go/control evidence unless a later class/front gate
  reopens the question.
```

## Focused Mechanism Lanes

```text
localexactfree-noboost-route4k:
  noboost-route4k plus LocalExactFirstFree-L1. Same low-RSS capacity shape,
  but free/free_remote try exact route lookup before the full route lookup.
  Exact MISS still falls back to the normal full lookup, so invalid-range and
  foreign/fallback safety semantics are preserved. Use as a random_mixed
  same-owner hot-path probe, not as a default lane. First random_mixed run
  confirms full lookup probes are removed on exact-valid frees, but normal
  throughput is not materially better, so keep it as mechanism evidence.

directlocalfree-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1. Local-owner TOY/MIDPAGE/LOCAL2P
  frees bypass front lookup / function-pointer dispatch / wrapper validation
  and call descriptor-to-frontcache directly. LARGE and all remote/foreign
  paths remain on the normal front contract. Use as a narrow same-owner
  free-path overhead probe. Repeat-3 strongly improves random_mixed and
  modestly improves mixed_ws balanced / wide_ws / larger_sizes with flat RSS.
  Treat as candidate-control evidence, but keep it named explicitly because it
  bypasses the generic front contract for selected local-owner fronts.

descavail-noboost-route4k:
  noboost-route4k plus DescriptorAvailCount-L1. It keeps the same low-RSS
  capacity shape but avoids full descriptor-table scans once the allocator
  knows no descriptor is available. Repeat-3 strongly improves mixed_ws
  balanced and wide_ws and keeps larger_sizes slightly positive, with route
  safety counters clean. Random_mixed is mostly neutral, so this is a
  descriptor-failure cost candidate-control rather than the primary
  same-owner hot-path lane.

directlocalfree-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1 and DescriptorAvailCount-L1. This
  explicit composition is strongest in random_mixed and edges out descavail in
  mixed_ws wide_ws/larger_sizes, but it loses to descavail alone in mixed_ws
  balanced. Keep it as a named candidate-control lane, not a silent replacement
  for either parent mechanism.

sameownerfast-descavail-noboost-route4k:
  Selected random_mixed same-owner lane. This is the cleaned-up contract flag:
  HZ6_SAME_OWNER_FAST_L1 plus DescriptorAvailCount-L1 on the noboost-route4k
  capacity shape. It encodes the strong A-ladder composition
  (DirectLocalFree-L1 + DirectLocalAlloc-L1 + DirectLocalReuse-L1) without
  forcing benchmark scripts to carry a pile of mechanism flags. Use this name
  for current comparisons; keep the longer directlocal* names as
  evidence/control lanes.

directlocalalloc-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalAlloc-L1 and DescriptorAvailCount-L1. This is
  a random_mixed ablation lane: TOY/MIDPAGE/LOCAL2P malloc tries the existing
  cached/transfer reuse helper before the generic front function-pointer path.
  LARGE and cross-owner paths stay on the normal front contract.

directlocalreuse-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalAlloc-L1, DirectLocalReuse-L1, and
  DescriptorAvailCount-L1. This is the narrower local-cache reuse ablation:
  malloc only activates materialized LOCAL_FREE frontcache descriptors before
  falling back to the normal front path. Descriptorless materialization and
  transfer reuse are intentionally excluded from the direct probe.

directlocalfreealloc-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1, DirectLocalAlloc-L1, and
  DescriptorAvailCount-L1. This composition tests whether the existing
  free-path win and the new alloc dispatch-bypass compose on random_mixed.
  Keep it as A-ladder evidence until repeat data justifies a shared
  same-owner front fast path.

directlocalfreereuse-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1, DirectLocalAlloc-L1,
  DirectLocalReuse-L1, and DescriptorAvailCount-L1. This closes the A-ladder:
  free goes through the direct local cache helper and malloc first tries only
  materialized LOCAL_FREE frontcache descriptors for TOY/MIDPAGE/LOCAL2P.
  Repeat-3 random_mixed gives the cleanest broad same-owner signal, so use it
  as B-design input. It is still not the selected balanced/wide_ws low-RSS
  default because rss + descavail remains stronger there.

largerlowrss-front8k-sourcerun-desc8k-route8k:
  Larger_sizes-targeted low-RSS lane: descriptor 8K, route 8K, source-block
  512, frontcache 8K, and SourceRunReuse-L1. It keeps allocation failures at
  zero and reduces source/route churn by retaining larger object slots long
  enough to avoid constant source recreation. Repeat-3 larger_sizes beats
  ownerlocalityfast-rsscap-4 while using less than half the peak RSS. Do not
  use it as a universal mixed_ws lane: wide_ws guard regresses badly and
  balanced uses much more RSS than descavail.

largerlowrss-front6k-sourcerun-desc8k-route8k:
  Tighter larger_sizes front-retention candidate-control: descriptor 8K, route
  8K, source-block 512, frontcache 6144, and SourceRunReuse-L1. Repeat-3
  larger_sizes is effectively tied with front8k while keeping the same 72MB
  peak band. Promotion check across large_slice rows did not justify replacing
  front8k: front6k is good for larger_sizes itself, but front8k covers 4K/8K
  and 32K slice rows more consistently. Keep front6k as close candidate-control.

largerlowrss-front4k-sourcerun-desc8k-route8k:
  Front-retention lower-bound control: descriptor 8K, route 8K, source-block
  512, frontcache 4096, and SourceRunReuse-L1. Run1 larger_sizes speed drops
  sharply, so keep it as no-go evidence that front retention below 6K is too
  small for this profile.

desc4k-route4k:
  route4k plus descriptor capacity 4096. Descriptor-pressure probe only.

source512-route4k:
  route4k plus source-block capacity 512. Source-block-pressure probe only.

desc4k-source512-route4k:
  route4k plus descriptor 4096 and source-block 512. Combined pressure probe.

redislowrss-route4k:
  noboost plus descriptor 4096 and source-block 512. This is the named
  Redis-like low-RSS candidate-control lane after redis_short L0 showed
  `desc4k-source512-route4k` avoids Redis allocation-failure spam at a much
  lower peak working set than appcap. Evidence-only outside Redis-like rows:
  mixed_ws guard shows the capacity shape over-retains and slows broad mixed
  profiles. Redis long diagnostics now show this lane is the stronger
  completion/LPUSH shape than the slim lane.

redislowrss-sourcerun-route4k:
  redislowrss-route4k plus SourceRunReuse-L1. This is a narrow follow-up for
  Redis-like rows where redislowrss-route4k completes but still reports
  source_block_exhausted/source_prefill_fallback in RANDOM. It tests whether
  reusing physical slots inside existing source blocks can reduce Redis source
  pressure without widening capacity again. Latest redis_long run strongly
  improved all Redis patterns and lowered peak working set, but mixed_ws guard
  still keeps this outside general promotion.

redislowrss-sourcerun-route8k:
  redislowrss-sourcerun-route4k with route capacity 8192. Evidence lane for
  paper-aligned Redis LPUSH, where route4k showed route_register_fail. Route8k
  removes route register failure but exposes descriptor pressure, so it is not
  the final paper-row candidate.

redislowrss-sourcerun-desc8k-route8k:
  redislowrss-sourcerun with descriptor 8192 and route 8192. Current
  paper-aligned Redis candidate-control: removes route/descriptor/source-block
  exhaustion in the checked diagnostic row and keeps peak working set far below
  appcap while restoring LPUSH into a usable range. Keep this Redis-specific;
  do not promote it as the general mixed_ws lane.

redislowrss-sourcerun-desc8k-route8k-tombcompact:
  redislowrss-sourcerun-desc8k-route8k plus RouteTombstoneCompact-L1. Narrow
  Redis RANDOM route-churn behavior lane. It compacts exact-route tombstones
  after overflow unregisters cross a threshold, without changing descriptor or
  source retention policy. Use it to test whether RANDOM's full-table
  tombstone probes are the remaining bottleneck before attempting retained
  overflow or source-run slot lookup. First paper-row repeat-3 roughly doubles
  RANDOM and modestly improves LPUSH with a small peak increase, but SET/GET
  are slightly lower, so keep it Redis-specific.

redislowrss-sourcerun-desc8k-route8k-retainedoverflow:
  redislowrss-sourcerun-desc8k-route8k plus RouteRetainedOverflow-L1. When a
  LOCAL_FREE object cannot enter frontcache, it is retained in the bounded
  transfer cache as TRANSFER_FREE instead of immediately unregistering its
  exact route and releasing its descriptor/source. This tests whether Redis
  SET/LPUSH/RANDOM are losing time to overflow unregister churn before we add a
  larger retained-overflow data structure. First diagnostic row is no-go as a
  Redis fix: transfer retention succeeds mechanically but final
  frontcache-overflow unregisters and RANDOM tombstone full-probes remain.

redislowrss-sourcerun-desc8k-route8k-slotlookup:
  redislowrss-sourcerun-desc8k-route8k plus SourceRunSlotLookup-L1. It keeps
  the same source-run reuse contract but prefers the reusable block with the
  most free slots instead of the first reusable block found. Narrow lookup
  policy probe for Redis paper rows after tombstones and retained overflow have
  already been measured. Repeat-3 showed it is a useful source-run witness but
  not a promotion lane: SET is slightly better, RANDOM is roughly neutral, GET
  and peak are a bit worse.

redislowrss-slim-route4k:
  noboost plus descriptor 2048 and source-block 256. This is the slimmer Redis
  follow-up lane. Use it only if we need to reduce peak / retention further
  than `redislowrss-route4k`. Redis long diagnostics showed this can collapse
  LPUSH through descriptor/source-block exhaustion, so keep it as a control
  rather than the current Redis candidate.

front1k-desc4k-source512-route4k:
  route4k plus descriptor/source/front-cache widening. Reproduces broad-like
  behavior more than it solves the low-RSS lane. Evidence/control only.

sourcerun-route4k:
  Source-run slot reuse evidence lane. It proves reusable-run mechanics can
  reduce some source pressure, but it did not solve balanced mixed rows.

sourcerun-sameclass-route4k:
  Narrower same-class source-run reclaim evidence lane. Safer than broad donor
  reclaim and mildly positive on the latest rows, but still not a promotion
  lane.

descriptorless-route4k:
  Descriptor lifecycle prototype. Source-block cached slots can drop their
  descriptor while staying in frontcache; reuse materializes a fresh descriptor
  and exact route. This lane includes source-run metadata because descriptorless
  cached slots need a physical slot owner after the descriptor is detached. Use
  it to test whether cached slots should stop pinning descriptor capacity. The
  current L1 is evidence/control only: it preserves route safety in the latest
  checks, but descriptor materialization can still fail under a full descriptor
  table, so it is not a promotion lane.

descriptorreserve-route4k:
  Descriptor materialization-reserve prototype. Extends descriptorless-route4k
  by reserving the detached descriptor for the same cached physical slot, so
  reuse can materialize without a fresh descriptor-table search. Evidence only:
  it removes descriptorless materialization failures in the latest checks, but
  it does not solve balanced / wide_ws and can reduce the descriptor pool
  available to normal allocation.

descriptorcold-route4k:
  Selective descriptorless prototype. Only over-soft-cap frontcache bins detach
  descriptors, reducing the broad descriptorless failure loop. Evidence only:
  it has a small balanced / wide_ws signal, but larger_sizes regresses, so the
  simple soft-cap gate is not a promotion policy.

descriptorcoldgov-route4k:
  Class/pressure-aware descriptor governor prototype. Descriptorless is treated
  as cold-cache descriptor compression, not as a hot reuse path. The first L1
  gates detach to small/mixed classes, limits detached entries, and admits
  materialization only when a descriptor is available. Latest repeat-3 is
  promising on balanced and preserves larger_sizes, but wide_ws is still weak,
  so keep it as candidate-control evidence rather than default promotion.

descriptorcoldgov-widews-route4k:
  Budget-expanded descriptor governor variant for wide_ws probing. It keeps the
  same class gate as descriptorcoldgov-route4k but raises the detach budget to
  test whether wide_ws is limited by detached-slot budget saturation rather
  than by the class gate itself. Latest repeat-3 did not improve wide_ws and
  badly regressed balanced, so keep it as no-go/control evidence.
```

## Frozen No-Go Lanes

```text
spill-route4k:
  No-go. Frontcache spill increased source allocation and RSS.

borrow-route4k:
  No-go. Borrowing moved some source-block pressure but did not produce a clean
  throughput/RSS improvement.

cap-route4k:
  No-go. Local cap behavior slowed the lane and worsened RSS.

sourcerun-reclaim-route4k:
  No-go. Broad source-run donor reclaim caused route churn / probe explosions.
```

Keep these lanes buildable as controls, but do not use them as the next
optimization target unless a new diagnostic specifically reopens the question.

## Benchmark Family Read

```text
mixed_ws / random_mixed:
  Best for capacity-shape and low-RSS tradeoff checks.
  Latest selected-lane random_mixed run shows capacity is not the blocker:
  `noboost-route4k` is roughly as fast as widecap/rsscap/appcap while using
  far less RSS. Treat `noboost-route4k` as the random_mixed low-RSS control
  and attack same-owner hot-path overhead before adding more capacity lanes.

Larson worker-warmup:
  Same-owner small-object control. This is the lane for hot-path / toy-small
  behavior without cross-owner handoff stress.

Larson main-warmup:
  Cross-owner handoff stress. Treat failures here as route visibility /
  remote-free / transfer ownership evidence, not as a pure hot-path verdict.
  For full 10k T16, use `speed + ownerlocalityfast-rsscap-1` as the current
  candidate-control and `speed + ownerlocalityfast-rsscap-2` as the lower-RSS
  sibling. Use rsscap-3/4 only for compact/moderate live-set checks.

Redis workload:
  App-like pattern control. Useful for detecting whether HZ6 capacity changes
  actually survive more realistic access mixes. Current route4k/noboost rows
  need a focused shorter profile before they are useful for candidate ranking,
  because the default Redis-like row can timeout while emitting many allocation
  failure stats lines. Use `redis_tiny`, `redis_short`, `redis_medium`, and
  `redis_long` before returning to `redis_workload`, so collapse phases are
  visible before the paper-sized row times out.
```

## Commands

Build a focused HZ6 capacity suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_hz6_capacity_suite.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap
```

List the rows without running them:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -ListOnly `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,sourcerun-sameclass-route4k
```

Run a quick one-pass capacity check:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap `
  -Runs 1
```

Run a focused Redis-like timeout triage row:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families redis `
  -Hz6Profiles rss `
  -CapacityLanes route4k,noboost-route4k `
  -BenchmarkProfiles redis_short `
  -Runs 1 `
  -SkipBuild
```

Run staged Redis-like pressure rows:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families redis `
  -Hz6Profiles rss `
  -CapacityLanes noboost-route4k,redislowrss-slim-route4k `
  -BenchmarkProfiles redis_short,redis_medium,redis_long `
  -Runs 1 `
  -SkipBuild `
  -ContinueOnFailure
```

## Lane Rules

- Speed / paper lanes must not include diagnostic atomics or trace-only probes.
- Research lanes must have explicit names and must not silently replace
  `route4k`.
- `appcap` is a capacity/completion control, not a production default.
- A lane is not promoted from one good row. It needs RSS, safety, repeat
  stability, and guard behavior.
- Owned-looking invalid pointers must never be converted into foreign fallback.
- Do not put OS queries such as `VirtualQuery` on hot free paths.
