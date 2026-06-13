## Recommended Lane Sets

Use these sets before opening another capacity sweep. They keep HZ6 as a
profile family and avoid accidentally promoting an evidence lane outside the
row where it was measured.

```text
balanced / wide_ws clean low-RSS:
  HZ6 profile:
    rss
  capacity lane:
    mixedclean-front16k-sourcerun-desc17k-source2k-route17k
  pressure evidence, not selected:
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
    ownerlocalityfast-rsscap-2-desc160k
  stable near-capacity sibling:
    ownerlocalityfast-rsscap-2-desc192k
  selected lower-RSS sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k
    repeat-3 full 10k clean; use when about -1.3% throughput is acceptable for
    about 90MB lower peak RSS versus desc160k
  current selected low-RSS sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512
    repeat-3 full 10k clean at about 40.754M ops/s and 439912 KB peak RSS.
    It combines dir192k, descriptor no-backptr, SourceBlock no-route-backptr,
    RoutePackedMeta-L1, RoutePackedMeta-L2 routebytes16, StorageOwner16, and
    OwnerSourceSideMeta-L2.
  lower-RSS candidate/sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512
    direct closeout is about 44.831M ops/s and 430708 KB peak RSS; use only
    when about 9 MiB lower RSS is worth the measured throughput tradeoff.
  routebytes16 comparison control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512
    repeat-3 full 10k clean at about 48.367M ops/s and 449144 KB peak RSS.
  directory-capacity control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512
    repeat-3 full 10k clean at about 44.580M ops/s and 472176 KB peak RSS.
    It trims shared-route-directory / owner-locality capacity from 262K to
    192K while keeping route capacity at 192K.
  descriptor no-backptr comparison control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512
    repeat-3 full 10k clean; original no-backptr median is about 40.710M ops/s
    and 476784 KB peak RSS. Same-run control versus dir192k is about 45.310M
    ops/s and 476788 KB peak RSS.
  directory-capacity no-go controls:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512
    one-run safety counters stay clean, but owner-locality misses and huge
    full-table probes appear; speed regresses too much for selection.
  previous run512 control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
    repeat-3 full 10k clean; current median is about 48.512M ops/s and
    499820 KB peak RSS. Keep it as the descriptor no-backptr comparison
    baseline/control.
  descriptor-capacity boundary control:
    ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512
    repeat-3 full 10k clean at about 40.400M ops/s and 498080 KB peak RSS.
    Keep it as a tiny-RSS sibling/control, not a default replacement, because
    the RSS win versus desc160k is small and the next lower descriptor caps
    fail warmup.
  descriptor side-owner no-go:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512
    one-run diagnostic reaches `46.490M / 475672 KB` and a 32-byte hot
    descriptor entry, but it is not safety-clean. Keep it as evidence that
    side owner metadata needs the descriptor's owning allocator, not the
    current allocator.
  route192k control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
    repeat-3 clean at about 44.610M ops/s and 628844 KB peak RSS.
  source16k baseline control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
    repeat-3 full 10k clean; keep as the route-capacity control for the
    route192k metadata-slim result.
  lower-RSS / lower-throughput source-cap control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k
    repeat-3 full 10k clean; current median is about 44.308M ops/s and
    623084 KB peak RSS. Keep as a lowest-RSS control, not the selected
    throughput/RSS sibling.
  source-cap interpolation control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k
    run-1 full 10k clean at about 44.471M ops/s and 644836 KB peak RSS.
  route-capacity boundary controls:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k
    route224k is clean control; route160k/128k fail warmup and define the
    current no-go lower bound.
  descriptor-capacity boundary controls:
    ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512
    desc156k and below fail warmup with `descriptor_exhausted=3` and
    `alloc_fail=1`; static descriptor capacity cuts are closed under the
    current descriptor representation.
  source-run metadata ladder:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
    run512 is the previous selected lowest-RSS sibling/control. Run2048/run1024
    remain source-run metadata controls; no-backptr run512 is the descriptor
    layout control, dir192k/no-backptr is a directory-capacity control,
    routepacked/no-routebackptr/dir192k is the L1 control, and
    routebytes16 is the clean comparison-control sibling; OwnerSourceSideMeta-L2
    is the current selected low-RSS balance sibling, with FrontCachePackedMeta-L1
    as the lower-RSS candidate/sibling.
  source-block over-retention control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k
    passes, but raises peak RSS and is not selected
  compact/moderate thindesc evidence:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
    post-pack 4k repeat-3 keeps safety clean, but full 10k fails during warmup
    with source_block_exhausted=257; do not use as a paper-facing full-10k
    claim yet
  no-go static-table controls:
    ownerlocalityfast-rsscap-2-desc160k-route128k
    ownerlocalityfast-rsscap-2-desc160k-source2k
  stable controls:
    ownerlocalityfast-rsscap-1
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
  -CapacityLanes noboost-route4k,mixedclean-front16k-sourcerun-desc17k-source2k-route17k,descavail-noboost-route4k,sameownerfast-descavail-noboost-route4k,largerlowrss-front8k-sourcerun-desc8k-route8k,largerlowrss-front6k-sourcerun-desc8k-route8k,ownerlocalityfast-rsscap-4,ownerlocalityfast-appcap
```

Next Windows focus:

```text
Profile-family read:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k is the
  current balanced / wide_ws clean low-RSS selected lane.
  rss + descavail-noboost-route4k is still useful as a very fast low-RSS
  pressure row, but it is not paper/default clean because it relies on large
  allocation-failure/source-block-exhaustion counts.
  strict + sameownerfast-descavail-noboost-route4k is the current
  random_mixed same-owner speed candidate-control.
  largerlowrss-front8k-sourcerun-desc8k-route8k is the current larger_sizes
  RSS/speed candidate-control.
  ownerlocalityfast-rsscap-2-desc160k is the current full Larson cross-owner
  candidate-control; ownerlocalityfast-rsscap-2-desc192k is the stable
  near-capacity sibling.
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k is the
  selected low-RSS Larson sibling after the source-block recovery run.
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

ownerlocalityfast-rsscap-2-desc192k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 196608 instead of 262144. Use it to
  test whether rsscap-2 peak can be reduced without crossing the rsscap-3
  warmup-failure boundary. Repeat-3 makes it the stable near-capacity sibling:
  about 43.679M ops/s and 974,296 KB peak, with safety counters clean.

ownerlocalityfast-rsscap-2-desc160k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 163840. It is intentionally closer to
  the failing rsscap-3 descriptor budget; treat failures as boundary evidence,
  not as a route/source behavior verdict. Repeat-3 promotes it to the current
  full 10k Larson candidate-control: corrected median is about 43.721M ops/s
  and 928,228 KB peak, with safety counters clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k:
  ThinDescriptor/source16k Larson low-RSS baseline. It completes full-10k and
  remains a useful control, but MetadataSlim-L1 showed route capacity can be
  reduced further without losing completion.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k:
  Clean route-capacity control. Same as thindesc-source16k, but route capacity
  is trimmed from 262144 to 196608. Latest repeat-3:
  `44.610M / 628844 KB`, safety clean. SourceBlockMetaSlim-L1 superseded it
  as the lowest-RSS sibling with route192k-run512. Route160k and route128k are
  no-go boundary controls: both fail warmup from route-table saturation.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512:
  Previous Larson lowest-RSS sibling/control. Same route capacity as route192k,
  but `HZ6_SOURCE_RUN_MAX_SLOTS` is reduced to 512. Repeat-3:
  `48.512M / 499820 KB`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512:
  Larson descriptor-layout comparison control. Same route/run512 shape, but
  `HZ6_DESCRIPTOR_NO_BACKPTR_L1=1` removes the allocator back-pointer from
  `Hz6ObjectDescriptor`. Repeat-3:
  `40.710M / 476784 KB`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512:
  Larson directory-capacity comparison control. Same no-backptr route/run512
  shape, but `HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY` is trimmed to 196608 so the
  owner-locality/shared-directory index is smaller. It is superseded by the
  routebytes16 comparison-control sibling and the later OwnerSourceSideMeta-L2
  selected sibling. Repeat-3:
  `44.580M / 472176 KB`, safety clean. Same-run no-backptr control is
  `45.310M / 476788 KB`, so dir192k trades about -1.6% throughput for about
  4.6 MB lower peak RSS. Dir128k/dir96k are no-go controls because
  owner-locality misses and full-table probes appear.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512:
  SourceBlockNoRouteBackptr-L1 minimum-RSS Larson control. Same no-backptr /
  dir192k / route192k / run512 shape, but `HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1`
  removes the SourceBlock route-backend pointer and relies on allocator-explicit
  SourceBlock release/unregister. Repeat-3:
  `41.107M / 469868 KB`, safety clean, `source_block_entry_bytes=136`.
  Keep it as a clean isolation control; routebytes16/routepacked supersedes it
  for selected lowest-RSS comparisons.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512:
  RoutePackedMeta-L1 comparison control. Same descriptor no-backptr /
  SourceBlock no-route-backptr / dir192k / route192k / run512 shape, plus
  `HZ6_ROUTE_PACKED_META_L1=1`. It packs route entry front/class/state into a
  32-bit meta word and moves bytes to a side array. Repeat-3 full 10k:
  `47.616M / 456048 KB`, safety clean. The 1k diagnostic smoke reports
  `route_entry_bytes=24`, `route_invalid=0`, `route_miss=0`, and
  `route_register_fail=0`. Superseded by RoutePackedMeta-L2 routebytes16 for
  the selected Larson lowest-RSS sibling.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512:
  Superseded Larson lowest-RSS control. It keeps the RoutePackedMeta-L1
  shape and stores route bytes as `uint16_t(bytes - 1)` behind the route
  accessor boundary. The L2 dry-run showed raw uint16 overflow only at 64KiB
  source-block envelopes (`route_bytes16_overflow=147`,
  `route_bytes16_max=65536`), while biased16 was clean
  (`route_bytes16_minus1_overflow=0`, `route_bytes16_minus1_zero=0`,
  `route_bytes16_minus1_max_stored=65535`). Same-run full-10k A/B repeat-3:
  routepacked `45.079M / 456040 KB`, routebytes16 `48.367M / 449144 KB`,
  safety clean. Superseded by OwnerSourceSideMeta-L2 after the 2026-06-05
  repeat-3.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512:
  OwnerSourceSideMeta-L1 dry-run. Same routebytes16 comparison-control lane, plus
  `HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1` in diagnostic builds only. It projects
  whether descriptor owner side metadata can be made owner-source-aware without
  changing behavior. Full 10k run=1:
  `46.202M / 449164 KB`, safety clean,
  `owner_source_side_meta_foreign=871979714`,
  `owner_source_side_meta_miss=0`, `owner_source_side_meta_probe_max=1`.
  Read: the problem is lookup frequency, not depth. Do not promote
  StorageOwner16 directly; design an O(1) owner-source side metadata map.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512:
  Current Larson lowest-RSS selected sibling. It combines routebytes16 with
  StorageOwner16 ownerless descriptors and stores an O(1) descriptor-storage
  hint on each SourceBlock. Full 10k non-diagnostic repeat-3:
  routebytes16 comparison control `40.750M / 449128 KB`, L2
  `40.754M / 439912 KB`. Worker-warmup 10k run=1: routebytes16
  `40.126M / 448948 KB`, L2 `40.787M / 439740 KB`.
  Full 10k diagnostic dry-run comparison:
  `owner_source_side_meta_l2_hit=1253200849`,
  `owner_source_side_meta_l2_miss_no_block=0`,
  `owner_source_side_meta_l2_miss_inactive=0`,
  `owner_source_side_meta_l2_storage_mismatch=0`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512:
  Lowest-RSS candidate/sibling over OwnerSourceSideMeta-L2. It adds
  `HZ6_FRONTCACHE_PACKED_META_L1=1`, shrinking `Hz6FrontCacheEntry` from
  32 to 24 bytes by packing bytes-minus-one, class id, and the descriptor-cold
  governor detached flag into a 32-bit meta word. Direct full-10k closeout:
  OwnerSourceSideMeta-L2 baseline `46.467M / 439912 KB`, FrontCachePacked
  `44.831M / 430708 KB`, safety clean. Keep as the lower-RSS sibling/candidate,
  not as the selected throughput/RSS balance lane.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512:
  SourceBlockPackedFlags-L1 lower-RSS candidate over OwnerSourceSideMeta-L2.
  It adds `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1`, packing SourceBlock
  `source_kind`, `active`, `route_registered`, and `run_active` into one
  state flag word. It keeps `source_release`, OwnerSourceSideMeta-L2, and
  source-run bitmap/slot metadata inline. Build/smoke is clean; diagnostic
  sizeof check for the selected routepacked/routebytes16/L2 shape shows
  `source_block_entry_bytes=144 -> 128`. Repeat-3 closeout:
  `41.070M / 435304 KB`, safety clean. Keep as a component lower-RSS
  candidate/control; the combined packed lane is the current minimum-RSS
  candidate.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512:
  Combined packed minimum-RSS candidate over OwnerSourceSideMeta-L2. It enables
  both `HZ6_FRONTCACHE_PACKED_META_L1=1` and
  `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1`. The two optimizations target separate
  metadata tables, so the RSS savings compose cleanly in the smoke/repeat.
  Repeat-3 full 10k: `40.837M / 426084 KB`, safety clean. This is lower RSS
  than FrontCachePacked alone (`41.131M / 430692 KB`) and SourceBlockPacked
  alone (`41.070M / 435304 KB`) at similar throughput. Keep as the current
  combined-packed source16k control. It is superseded by the source10k trim as
  the current minimum-RSS sibling.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512:
  Current combined packed minimum-RSS sibling. Same selected packed flags as
  source16k, but `HZ6_SOURCE_BLOCK_CAPACITY=10240`. Boundary run-1:
  `47.375M / 412736 KB`, safety clean, `source_block_exhausted=0`. This cuts
  about 13 MB peak RSS versus the same probe family's source16k control
  (`43.514M / 426088 KB`). Repeat-3 confirmation:
  `44.864M / 412280 KB`, safety clean, static table `234502 KiB`.
  Guard rows are safety-clean: worker10k `45.707M / 412128 KB`, main4k
  `48.305M / 331152 KB`, worker4k `51.418M / 331100 KB`, main1k
  `55.927M / 290380 KB`, worker1k `55.836M / 290372 KB`.
  Keep as current minimum-RSS candidate/sibling, not a broad throughput
  promotion.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source12k-route192k-run512:
  Source-capacity backup/control for the combined packed lane.
  `HZ6_SOURCE_BLOCK_CAPACITY=12288`, run-1 `43.910M / 417332 KB`, safety clean.
  Keep as a conservative fallback if source10k shows repeat or broad-row
  instability.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source2k-route192k-run512:
  Source-capacity no-go/control. `HZ6_SOURCE_BLOCK_CAPACITY=2048` fails
  cross-owner main-warmup with `source_block_exhausted=257` and
  `source_block_fail_active_max=2048`.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source8k-route192k-run512:
  Source-capacity no-go/control. `HZ6_SOURCE_BLOCK_CAPACITY=8192` still fails
  cross-owner main-warmup with `source_block_exhausted=257` and
  `source_block_fail_active_max=8192`. The first tested passing local-only
  SourceBlock capacity is source10k.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2dryrun-source16k-route192k-run512:
  Validation lane for OwnerSourceSideMeta-L2. Same L2 behavior plus
  `HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1`, so it compares the O(1) source-block
  storage hint against the old scan-based storage-owner result. Use only for
  mismatch/safety validation, not speed ranking.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512:
  StorageOwner16-L1 RSS-first descriptor side metadata evidence/control. It
  removes owner from the hot descriptor and stores packed owner slot/generation
  in the descriptor storage allocator's side table. This fixes the old
  allocator-local side-owner16 safety failure. Repeat-3 full 10k:
  `42.024M / 444520 KB`, safety clean. Keep as evidence/control; routepacked
  remains selected because storageowner16 saves about 11.5 MB RSS but loses
  about 12% throughput.

ownerlocalityfast-rsscap-2-desc144k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 147456. It is the narrow probe between
  selected desc160k and failing rsscap-3/131k. Use only to test whether the
  descriptor budget can be tightened one more step; failures are boundary
  evidence, not a route/source behavior verdict. First full 10k Larson run
  fails warmup allocation with `alloc_fail=1` and source-block exhaustion, so
  keep it as no-go/boundary evidence. Diagnostic read confirms the descriptor
  table is effectively full of active objects; because T=16 and chunks=10000
  imply roughly 160k live objects, desc160k is close to the
  one-descriptor-per-live-object lower bound.

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

