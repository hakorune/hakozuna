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

ubuntu-toydirectmaptrusted-max4-default:
  Linux/Ubuntu default composition, not a Windows selected-family lane:
  Toy active-free map, trusted Toy active-map owner, DirectLocalFree-L1,
  DirectLocalAlloc-L1, DirectLocalReuse-L1, trusted local-cache owner, and
  HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4. Direct malloc preserves transfer-first
  profiles by trying transfer reuse before local frontcache reuse and returning
  the activated descriptor to Toy active-map registration. Ubuntu repeat-5
  versus explicit macro-off control is broadly positive: local 256B..4K
  +77%..+122%, local 8K +19%..+40%, local 16K +9%..+11%, remote 8K/64K
  +6%..+17%, and reuse +2%..+21%. The only negative guard row was remote 128K
  speed at -3.27%. Keep macro-off as the control lane.

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

mixedclean-front8k-sourcerun-desc16k-source2k-route16k:
  Mixed_ws clean-lane boundary probe: frontcache 8K, descriptor 16K,
  source-block 2K, route 16K, and SourceRunReuse-L1. Balanced is clean but
  below the selected source2k/source4k rows; wide_ws still reports allocation
  failures. Keep as lower-capacity evidence.

mixedclean-front16k-sourcerun-desc16k-source2k-route16k:
  Mixed_ws clean-lane boundary probe with frontcache 16K but descriptor/route
  still 16K. It improves wide_ws substantially versus front8K but still leaves
  allocation failures. It shows wide_ws needs more than the 16K
  descriptor/route band.

mixedclean-front16k-sourcerun-desc16k-transfer2304-source2k-route16k:
  Transfer-isolation control for the desc16 no-go. Increasing transfer cache
  from 2048 to 2304 does not remove the wide_ws allocation failures, so the
  desc16 failure is not just a too-small transfer cache.

mixedclean-front16k-sourcerun-desc16k-transfer2560-source2k-route16k:
  Wider transfer-isolation control for the desc16 no-go. It still reports the
  same wide_ws allocation-failure count as the base desc16 lane, so keep it as
  no-go evidence.

mixedclean-front16k-sourcerun-desc17k-source2k-route17k:
  Selected mixed_ws balanced clean low-RSS lane: frontcache 16K, descriptor 17K,
  source-block 2K, route 17K, and SourceRunReuse-L1. Repeat-3 balanced and
  wide_ws are safety-clean, with the lowest RSS among the clean boundary
  lanes. Boundary scan: balanced `64.117M / 110976 KB`, wide_ws
  `21.119M / 140256 KB`. Selected-family refresh:
  balanced `55.504M / 110780 KB`, wide_ws `19.978M / 140236 KB`.
  Route-only repeat control: balanced `66.308M / 110940 KB`, wide_ws
  `20.155M / 140172 KB`.

mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry:
  Current selected mixed_ws boundary after LinearWrap/LoopCarry route cleanup.
  WideWsSourcePressureAudit-L1 confirms this is not arbitrary route slack:
  `route_register_fail=0`, `alloc_fail=0`, `source_block_exhausted=0`,
  `source_run_reuse_route_fail=0`, and `source_block_active_max=213` in the
  route-capacity closeout run.

mixedclean-front16k-sourcerun-desc17k-source2k-route16k-linearwrap-loopcarry:
  Route-capacity lower-bound no-go/control. It keeps the selected descriptor,
  source-block, frontcache, LinearWrap, and LoopCarry shape but cuts exact route
  capacity to 16K. Wide_ws does not exhaust SourceBlock metadata
  (`source_block_exhausted=0`), but exact route registration fails:
  `route_register_fail=20833`, `source_run_reuse_route_fail=13890`,
  `alloc_fail=6943`, and the later `source_refill_starvation=691124` is an
  aftershock because `alloc_fail > 0` latches the starvation heuristic. Do not
  promote or use as a production RSS cut.

mixedclean-front16k-sourcerun-desc17k-source2k-route8k-linearwrap-loopcarry:
  Hard route-pressure no-go/control. Wide_ws reports `route_register_fail=1506645`
  and `source_run_reuse_route_fail=534333`; source-run materialization then
  collapses into SourceBlock metadata exhaustion (`source_block_exhausted=503237`,
  `source_block_active_max=2048`). This lane is useful evidence that the cliff
  is route exact registration pressure cascading into source-block retention,
  not a simple source-block capacity issue.

mixedclean-front16k-sourcerun-desc17k-source4k-route8k-linearwrap-loopcarry:
  Source-capacity compensation no-go/control for the route8k collapse.
  Raising SourceBlock capacity from 2K to 4K does not rescue wide_ws
  (`alloc_fail` remains at the route8k failure level), so do not retry static
  source-capacity bumps as the next mixed_ws strategy.

mixedclean-front16k-sourcerun-desc17k-source2k-route18k:
  Selected wide_ws sibling. Same descriptor, transfer, source-block, and
  frontcache capacities as desc17/route17, but route capacity is raised from
  17408 to 18432. Repeat-3:
  balanced `64.923M / 111248 KB`, wide_ws `22.184M / 140456 KB`, safety clean.
  It lifts wide_ws about 10% versus the same-run desc17/route17 control with
  about +284 KB peak RSS. Route20 reduces probes further but does not improve
  throughput enough to justify the extra RSS.

mixedclean-front16k-sourcerun-desc18k-source2k-route18k:
  Clean boundary control above the selected desc17 lane. Repeat-3 is
  safety-clean, but it uses slightly more RSS and does not improve wide_ws
  enough to replace desc17: balanced `64.979M / 111524 KB`, wide_ws
  `20.398M / 140860 KB`.

mixedclean-front16k-sourcerun-desc20k-source2k-route20k:
  Clean wide-speed sibling/control. Repeat-3 gives a little more wide_ws speed
  than desc17 but with more RSS: balanced `59.888M / 113076 KB`, wide_ws
  `21.498M / 142676 KB`. Keep desc17 as the low-RSS selected lane.

mixedclean-front16k-sourcerun-desc22k-source2k-route22k:
  Boundary control between desc20 and desc24. Run-1 was clean but repeat did
  not beat desc17/20 enough to justify promotion.

mixedclean-front16k-sourcerun-desc24k-source2k-route24k:
  Previous mixed_ws clean low-RSS lane / extra-capacity control: frontcache
  16K, descriptor 24K, source-block 2K, route 24K, and SourceRunReuse-L1.
  Superseded by desc17 after the desc17 repeat-3 lowered RSS further while
  staying safety-clean.

mixedclean-front16k-sourcerun-desc32k-source2k-route32k:
  Previous mixed_ws clean low-RSS lane / extra-capacity control: frontcache
  16K, descriptor 32K, source-block 2K, route 32K, and SourceRunReuse-L1.
  Desc24 and then desc17 superseded it with lower RSS and comparable or better
  repeat-3 speed, so keep this as a control.

mixedclean-front16k-sourcerun-desc32k-source3k-route32k:
  Source-capacity midpoint control between source2K and source4K. Run-1 stayed
  safety-clean, but it did not improve wide_ws enough to justify the extra RSS:
  balanced `60.420M / 127648 KB`, wide_ws `20.113M / 156504 KB`. Keep as
  no-promotion evidence.

mixedclean-front16k-sourcerun-desc32k-source4k-route32k:
  Speed/control sibling for mixed_ws clean rows: same front/descriptor/route
  shape as the old desc32/source2K lane but source-block capacity 4K.
  Repeat-3 is safety-clean and can be faster in balanced rows, but uses much
  more peak RSS than desc17, so keep it as speed/control only.

mixedclean-front8k-sourcerun-desc32k-source4k-route32k:
  Tightening control for the clean mixed lane. Descriptor/route/source are
  wide enough, but frontcache 8K makes wide_ws much slower. Keep as evidence
  that clean wide_ws needs frontcache 16K, not just descriptor/route capacity.

mixedclean-front12k-sourcerun-desc32k-source4k-route32k:
  Frontcache tightening control between 8K and 16K. It can keep wide_ws clean,
  but source2K/front16K is the cleaner RSS default after repeat-3.

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

redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr1024:
redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr2048:
  Redis-only RouteTombstoneCompact threshold boundary controls. The base
  tombcompact lane keeps the conservative threshold
  `max(HZ6_ROUTE_TOMBSTONE_COMPACT_MIN, route_capacity / 2)`, which means an
  8192-route row compacts only after about 4096 tombstones in a single
  allocator. The aggressive lanes intentionally remove that half-cap floor and
  use absolute 1024/2048 thresholds. Use these only to test whether Redis
  route-churn wants earlier tombstone compaction; do not treat them as broad
  HZ6 defaults unless SET/GET/RANDOM and RSS all improve with clean safety
  counters. First refreshed Redis long diagnostic proved the lanes fire
  (aggr1024: LPUSH/RANDOM, aggr2048: RANDOM), but the non-diagnostic run did
  not produce a clean winner: aggr1024 lowered RANDOM probe work yet raised
  peak, aggr2048 helped SET/LPUSH shape but lost RANDOM, and base/regular
  tombcompact each still own different patterns. Keep both as threshold
  boundary controls.

redislowrss-sourcerun-desc8k-route8k-condtombdry:
  Redis-only ConditionalTombCompact dry-run. It does not compact. It projects
  whether a conditional policy would compact after frontcache-overflow
  unregisters when an absolute tombstone minimum is reached and either the
  tombstone/active ratio or route occupancy is high, with a cooldown gate.
  Use this as the final Redis route-churn design witness before adding a
  behavior lane; if it does not separate RANDOM/LPUSH pressure from GET/LPOP
  low-pressure rows, close the fixed-threshold tombcompact track. First
  diagnostic run separated the rows cleanly: LPUSH projected 4 compactions,
  RANDOM projected 8, and SET/GET/LPOP projected zero.

redislowrss-sourcerun-desc8k-route8k-condtombcompact:
  Redis-only ConditionalTombCompact behavior. It uses the same table-local
  condition as `condtombdry` and compacts only when the absolute tombstone
  minimum, ratio/occupancy pressure, and cooldown gates pass. Diagnostic run
  proved it compacts LPUSH/RANDOM only with clean safety counters. Repeat-3 is
  useful but not selected: it improves SET/RANDOM versus base and keeps peak
  close to the Redis envelope, but row geomean trails aggr1024 and GET/LPOP
  regress versus the best controls.

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
  For full 10k T16, use `speed + ownerlocalityfast-rsscap-2-desc160k` as the
  current candidate-control and `speed + ownerlocalityfast-rsscap-2-desc192k`
  as the stable near-capacity sibling. Use rsscap-3/4 only for compact/moderate
  live-set checks.

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
