# Current Task

HZ6 is now in active Windows/Linux implementation and benchmarking. HZ5 Linux
remains profile-stabilized; new HZ5 work should not blur the HZ6 contract.

## Latest Windows Matrix Read

Latest full matrix:

```text
private/allocators/hakozuna/docs/benchmarks/windows/20260601_045004_allocator_matrix.md
```

Read:

```text
balanced / wide_ws:
  HZ6 strict/speed/rss are far below HZ3 / HZ4 / mimalloc / tcmalloc.
  broad-cap rows improve throughput but raise RSS substantially.

larger_sizes:
  HZ6 strict/speed/rss stay very low-RSS and are still throughput weak.
  broad-cap rows recover throughput well and stay well below the mixed-row RSS
  blow-up seen on balanced / wide_ws.

large_slice_64k / 128k:
  HZ6 strict/speed/rss are strong and low-RSS on the exact large-slice rows.
  128k is particularly strong in the broad lanes and beats tcmalloc in the
  current run.

RSS:
  the matrix now carries peak RSS KB for every allocator row, including
  HZ3 / HZ4 / mimalloc / tcmalloc.
```

Current interpretation:

```text
strong:
  large_slice_64k
  large_slice_128k
  exact large-slice low-RSS lanes

weak:
  balanced
  wide_ws
  small/mixed rows under strict/speed/rss controls

tradeoff:
  broad-cap HZ6 lifts throughput on many rows, but it also raises RSS.
```

Paper runner coverage:

```text
RSS-aware summaries now cover:
  allocator matrix
  Larson paper runner
  Redis workload paper runner
  MT remote paper runner

Next validation order:
  1. rerun the RSS-aware paper runners with the new peak_kb columns
  2. keep the balanced / wide_ws weakness as the main HZ6 warning
  3. keep large_slice_64k / 128k as the strong proof points
```

Latest weakness read:

```text
Larson:
  worker-warmup is good.
  stress and appcap are still the weak lanes, with HZ6 failing or
  underperforming on the heavier T=8/T=16 rows.

Redis:
  HZ6 appcap is weak across SET / GET / LPUSH / LPOP / RANDOM.
  mimalloc and tcmalloc stay ahead on every Redis pattern in the latest run.

Random mixed:
  strict / speed / rss controls still fail on the small and medium/mixed rows.
  broad and appcap survive, but they remain far behind mimalloc / tcmalloc on
  throughput, so mixed throughput is still a weak lane.

HZ6 owner-aware MT remote:
  throughput is usable and RSS is low.
  alloc_failures are still huge, so this remains a pressure lane rather than a
  clean win lane.
```

Failure-row diagnosis:

```text
random_mixed 20260601_070706:
  small strict/speed/rss:
    fails at iter 79, size 1428
    source_alloc = 64
    descriptor_exhausted = 1
    route_register_fail = 0
    source_block_exhausted = 0

  medium strict/speed/rss:
    fails at iter 57, size 21472
    source_alloc = 26
    descriptor_exhausted = 0
    route_register_fail = 1
    source_block_exhausted = 11

  mixed strict/speed/rss:
    fails at iter 55, size 17001
    source_alloc = 26
    descriptor_exhausted = 0
    route_register_fail = 1
    source_block_exhausted = 8
```

Read:

```text
The strict/speed/rss failures are capacity-control failures, not a pure speed
verdict:
  default descriptor capacity is 64
  default route capacity is 64
  default source-block capacity is 16
  random_mixed uses WS=400

small fails first on descriptor capacity.
medium/mixed fail first on source-block plus route registration capacity.

The actual performance problem is the passing broad/appcap rows:
  they survive with low-to-moderate RSS
  but remain far behind HZ3 / HZ4 / mimalloc / tcmalloc
```

Control lane read:

```text
random_mixed 20260601_072659:
  hz6-strict-control / speed-control / rss-control
    pass on small, medium, and mixed
    peak RSS ~4.8 MB
    route_register_probe_total is much smaller than broad/appcap
    throughput is still below broad, but the capacity failures are gone

  broad:
    still the faster HZ6 lane on mixed
    peak RSS ~7.3 MB

  appcap:
    still similar on throughput to broad in some rows
    peak RSS ~196 MB
```

Route lookup diagnostic read:

```text
Implementation:
  route_lookup_probe_total / route_lookup_probe_max added under
  HZ6_DIAGNOSTIC_PROBES only.
  random_mixed diagnostic builds now write to out_win_random_mixed_diag so
  real benchmark executables in out_win_random_mixed are not overwritten.

random_mixed mixed RUNS=1 diagnostic:
  docs/benchmarks/windows/paper/20260601_074225_paper_random_mixed_windows.md

Passing HZ6 rows:
  broad:
    lookup probe total ~14.9M to 15.0M
    lookup max 5..6
    register probe total ~0.44M to 0.50M
    throughput ~28.4M to 33.1M ops/s
    RSS ~7.2 MB

  control:
    lookup probe total ~38.7M to 55.0M
    lookup max 55..144
    register probe total ~57K to 64K
    throughput ~24.0M to 30.9M ops/s
    RSS ~4.7 MB

  appcap:
    lookup probe total ~14.0M to 14.2M
    lookup max 2..4
    register probe total ~28.3M to 32.0M
    throughput ~27.5M to 32.2M ops/s
    RSS ~196 MB

Read:
  control is low-RSS and capacity-clean, but pays a repeated free-route lookup
  cost.

  appcap makes lookup cheap but burns huge registration probe work and RSS.

  broad is currently the best throughput/RSS compromise among HZ6 mixed rows,
  but still far behind HZ3 / mimalloc / tcmalloc.
```

Route4k lane read:

```text
Implementation:
  route4k keeps the control capacities for descriptor / transfer /
  source-block / front-cache, but raises only HZ6_ROUTE_TABLE_CAPACITY to
  4096.

random_mixed mixed RUNS=1 diagnostic:
  docs/benchmarks/windows/paper/20260601_075023_paper_random_mixed_windows.md

Throughput / RSS:
  hz6-strict-route4k:
    32.752M ops/s, 5.008 MB peak
  hz6-speed-route4k:
    28.102M ops/s, 5.016 MB peak
  hz6-rss-route4k:
    30.577M ops/s, 5.008 MB peak

Compared with control:
  strict route4k is about +18% throughput for about +0.8 MB RSS.
  speed route4k is about +21% throughput for about +0.8 MB RSS.
  rss route4k is about +20% throughput for about +0.8 MB RSS.

Probe read:
  route4k reduces lookup probe totals back to broad-like levels:
    route4k ~14.8M..15.4M
    control ~41.6M..71.1M

  route4k keeps appcap's lookup benefit without appcap RSS:
    appcap RSS ~195.7 MB
    route4k RSS ~5.0 MB

  route4k still has broad-like register probe totals:
    ~0.44M..0.50M
  but this is far below appcap's ~28M..32M register probe totals.

Read:
  route4k is the current best HZ6 random_mixed capacity shape:
  broad-class throughput, near-control RSS, and no appcap blow-up.
```

Route4k repeat-3 read:

```text
Source:
  small:
    docs/benchmarks/windows/paper/20260601_081612_paper_random_mixed_windows.md
  medium:
    docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md
  mixed:
    docs/benchmarks/windows/paper/20260601_082153_paper_random_mixed_windows.md

small:
  strict:
    33.013M ops/s, 4,556 KB
  speed:
    26.879M ops/s, 4,556 KB
  rss:
    28.921M ops/s, 4,560 KB

medium:
  strict:
    35.254M ops/s, 4,556 KB
  speed:
    30.282M ops/s, 4,556 KB
  rss:
    32.581M ops/s, 4,560 KB

mixed:
  strict:
    32.851M ops/s, 4,556 KB
  speed:
    28.399M ops/s, 4,552 KB
  rss:
    30.424M ops/s, 4,556 KB

Read:
  route4k stays stable across small / medium / mixed.
  It keeps RSS near the control lane while preserving broad-like throughput.
  This is strong enough to treat route4k as the current candidate-control lane.

Cross-family wiring:
  route4k is now also built into the mixed_ws allocator matrix plus the
  Larson and Redis app-like HZ6 matrices, so the same route-table-only
  capacity shape can be checked outside random_mixed.
```

Next step:

```text
1. Keep the new random_mixed control lane as the diagnostic bridge:
   it removes the capacity failures and keeps RSS low.

2. Promote route4k to the primary random_mixed HZ6 candidate-control lane.
   It isolates route-table capacity as the main cause of the control/broad
   throughput split.

3. Next measurement:
   run small / medium / mixed repeat-3 for route4k vs broad vs control.
   Then carry the same route4k shape into Larson and Redis app-like rows.
   If route4k stays stable, use it as the HZ6 Windows candidate-control
   capacity shape for random_mixed-style and app-like rows.

4. Keep large_slice_64k / 128k as the current strong proof points.
5. Leave Redis and Larson untouched until the random_mixed throughput gap is
   clearer.
```

Route4k cross-benchmark check:

```text
Sources:
  mixed_ws matrix:
    docs/benchmarks/windows/20260601_085515_allocator_matrix.md
  Redis workload:
    docs/benchmarks/windows/paper/20260601_085933_paper_redis_workload_windows.md
  Larson T=1 / compact / worker-warmup:
    docs/benchmarks/windows/paper/20260601_091055_paper_larson_windows.md

mixed_ws balanced:
  route4k improves over tiny control but still has heavy alloc_fail.
  rss-route4k is the best HZ6 route4k row:
    3.51M ops/s, 17.3 MB
  rss-broad:
    1.71M ops/s, 99.8 MB
  Read:
    route4k helps RSS/throughput balance, but balanced is not solved by route
    capacity alone.

mixed_ws wide_ws:
  route4k stays low-RSS and roughly broad-class for HZ6, but alloc_fail remains
  large.
  rss-route4k:
    0.437M ops/s, 12.1 MB
  strict-broad:
    0.452M ops/s, 141.5 MB
  Read:
    route4k is a better capacity shape than broad for RSS, but it does not fix
    wide working-set capacity.

mixed_ws larger_sizes:
  route4k is the strongest route4k proof point.
  strict-route4k:
    0.786M ops/s, 15.2 MB
  speed-route4k:
    0.762M ops/s, 14.2 MB
  rss-route4k:
    0.752M ops/s, 14.6 MB
  broad:
    0.751M..0.800M ops/s, 65..70 MB
  Read:
    route4k matches broad-class HZ6 throughput with far lower RSS for larger
    size matrix rows.

Redis workload:
  strict / broad / route4k all timed out at 60s.
  only appcap rows completed, but with very high RSS:
    ~584..647 MB
  Read:
    Redis is not a route-table-only problem. It needs descriptor/source/front
    capacity or a more adaptive admission shape before route4k matters.

Larson T=1:
  stress and worker-warmup:
    default / broad / route4k fail with rc1
    appcap runs, but at ~293..337 MB RSS
  compact control:
    route4k runs and matches broad/appcap throughput with far lower RSS.
    strict-route4k:
      16.25M ops/s, 7.2 MB
    speed-route4k:
      12.91M ops/s, 5.7 MB
    rss-route4k:
      14.97M ops/s, 5.9 MB
  Read:
    route4k is good for compact HZ6 controls, but Larson stress needs a
    non-route capacity or lifecycle fix.

Conclusion:
  route4k is a valid candidate-control capacity shape for compact/random_mixed
  and larger-size matrix checks.

  It is not enough for Redis or Larson stress. The next HZ6 work should attack
  non-route capacity/admission:
    descriptor capacity
    source block capacity
    front cache capacity
    adaptive source/front budget

  Start with a small fixed experiment:
    desc4k-route4k
  or:
    source512-route4k

  The goal is to find the minimum non-route capacity that removes alloc_fail
  without appcap's huge RSS footprint.
```

HZ6-only runner status:

```text
Implemented:
  win/build_win_hz6_capacity_suite.ps1
  win/run_win_hz6_capacity_matrix.ps1

Purpose:
  Run HZ6 capacity/profile lanes without rebuilding or executing
  HZ3 / HZ4 / HZ5 / mimalloc / tcmalloc every time.

Supported app-like families:
  mixed_ws
  random_mixed
  larson
  redis

Default HZ6 profiles:
  strict
  speed
  rss

Default capacity lanes:
  control
  route4k
  appcap

Additional focused lanes:
  desc4k-route4k:
    raises descriptor capacity to 4096 while keeping route4k and the other
    route4k/control capacities.
    Purpose: test whether balanced / Larson failures are descriptor-driven
    without moving to broad/appcap RSS.

  source512-route4k:
    raises source-block capacity to 512 while keeping route4k and the other
    route4k/control capacities.
    Purpose: test whether Redis / medium-mixed failures are source-block
    driven without moving to appcap RSS.

  desc4k-source512-route4k:
    raises descriptor capacity to 4096 and source-block capacity to 512 while
    keeping route4k plus control transfer/front-cache capacities.
    Purpose: test the smallest combined non-route shape before broad/appcap.

  front1k-desc4k-source512-route4k:
    raises front-cache bin capacity to 1024 on top of the combined descriptor /
    source / route lane, while keeping transfer capacity at 512.
    Purpose: isolate whether broad's win comes from front-cache capacity rather
    than transfer capacity.

Notes:
  Existing full comparison runners remain the source for allocator-vs-allocator
  tables. The HZ6-only runner is the fast iteration tool for capacity/admission
  experiments on the stable Windows machine.

  Diagnostic counters stay separate through -DiagnosticHz6Probes, which builds
  into out_win_hz6_capacity_diag instead of the normal speed artifacts.

Example:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 \
    -Families random_mixed \
    -BenchmarkProfiles small,mixed \
    -Hz6Profiles speed,rss \
    -CapacityLanes control,route4k,appcap \
    -Runs 3
```

Next focused capacity tests:

```text
1. random_mixed:
   route4k vs desc4k-route4k vs source512-route4k
   profiles:
     small, medium, mixed

2. mixed_ws:
   balanced / wide_ws / larger_sizes
   check whether desc4k-route4k removes alloc_fail without broad RSS.

3. Redis:
   source512-route4k first.
   If it still times out, route-only and source-block-only are insufficient.

4. Larson:
   desc4k-route4k first on compact/worker controls.
```

New lane smoke:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093158_hz6_capacity_matrix_windows.md

random_mixed small / speed / RUNS=1:
  route4k:
    27.082M ops/s, 4,548 KB
  desc4k-route4k:
    27.320M ops/s, 5,516 KB
  source512-route4k:
    27.415M ops/s, 5,048 KB

Read:
  Both focused non-route lanes build and run cleanly.
  On small, neither lane is needed for capacity correctness:
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
  The next useful reads are medium/mixed, mixed_ws balanced/wide_ws, Redis,
  and Larson where route4k alone still failed or timed out.
```

Focused capacity read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093251_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_093323_hz6_capacity_matrix_windows.md

random_mixed medium/mixed:
  route4k remains the cleanest lane.
  desc4k-route4k and source512-route4k do not improve speed and add RSS.
  alloc_fail and descriptor/source/route exhaustion are already 0 on route4k.

mixed_ws balanced/wide_ws/larger_sizes:
  source512-route4k alone does not help while descriptor capacity remains 512.

  desc4k-route4k reduces alloc_fail dramatically, but moves pressure into
  source allocation and RSS:
    balanced:
      route4k 3.195M ops/s, 17.7 MB, alloc_fail 1.51M
      desc4k-route4k 0.469M ops/s, 130.2 MB, alloc_fail 11K
    wide_ws:
      route4k 0.388M ops/s, 12.2 MB, alloc_fail 1.50M
      desc4k-route4k 0.181M ops/s, 129.1 MB, alloc_fail 982K
    larger_sizes:
      route4k 0.747M ops/s, 14.6 MB, alloc_fail 459K
      desc4k-route4k 0.558M ops/s, 70.3 MB, alloc_fail 3K

Read:
  Descriptor capacity is a real blocker on mixed_ws, but lifting it alone
  exposes source-placement/RSS pressure. The next lane is the combined
  desc4k-source512-route4k check, not a broad/appcap jump.

Follow-up:
  desc4k-source512-route4k still trails broad and has broad-like RSS.
  Since mixed_ws rows report transfer_push/pop = 0, the broad delta is likely
  front-cache capacity rather than transfer capacity.
  Next check:
    front1k-desc4k-source512-route4k vs broad

Front-cache isolation read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_093548_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_093735_hz6_capacity_matrix_windows.md

mixed_ws / rss / RUNS=1:
  balanced:
    desc4k-source512-route4k:
      0.564M ops/s, 122.1 MB, source_alloc 703K
    front1k-desc4k-source512-route4k:
      1.736M ops/s, 100.3 MB, source_alloc 191K
    broad:
      1.701M ops/s, 99.8 MB, source_alloc 191K

  wide_ws:
    front1k-desc4k-source512-route4k:
      0.191M ops/s, 93.4 MB
    broad:
      0.145M ops/s, 93.4 MB

  larger_sizes:
    front1k-desc4k-source512-route4k:
      0.728M ops/s, 65.3 MB
    broad:
      0.747M ops/s, 65.2 MB

Read:
  front1k-desc4k-source512-route4k reproduces broad behavior while keeping
  transfer capacity at 512. In these mixed_ws rows, transfer_push/pop remain 0,
  so broad's improvement is explained by front-cache capacity rather than
  transfer capacity.

  This is a good mechanistic result, but it is not a promotion lane:
    route4k is low-RSS but capacity-failing.
    broad/front1k shape is capacity-cleaner but RSS-heavy.

Next:
  attack front-cache/source placement rather than raising fixed capacities:
    reuse source-block slots more efficiently
    reduce source_alloc per successful object
    avoid broad-like RSS for balanced/wide_ws
```

HZ6 no-go checks after cleanup:

```text
Sources:
  docs/benchmarks/windows/paper/20260601_100622_hz6_capacity_matrix_windows.md
  docs/benchmarks/windows/paper/20260601_100945_hz6_capacity_matrix_windows.md

Toy block prefill minimum:
  attempt:
    force non-strict toy block prefill to keep a minimum refill count instead
    of dropping to per-object source when source admission clamps to 1.

  result:
    smoke required strict opt-out.
    mixed_ws route4k lost throughput and did not solve descriptor exhaustion.

Descriptor-exhaustion scavenge-on-refill:
  attempt:
    call profile scavenge once when source/refill cannot find a free
    descriptor, then retry descriptor lookup.

  result:
    route4k still descriptor-exhausts on balanced/wide_ws/larger_sizes.
    balanced route4k regressed sharply:
      old route4k: ~3.2M ops/s, ~17 MB
      scavenge-on-refill route4k: 0.461M ops/s, 25.2 MB

Decision:
  Both behavior changes are no-go and were reverted.
  The remaining problem is not a one-shot refill tweak. It needs a clearer
  front-cache/source-lifetime design, likely class/run reuse or a bounded
  descriptor recycling policy that does not scan/release on the allocation
  slow path.
```

HZ6 diagnostic follow-up:

```text
Source:
  docs/benchmarks/windows/paper/20260601_101314_hz6_capacity_matrix_windows.md

mixed_ws / rss / diagnostic probes / RUNS=1:
  route4k / balanced:
    3.223M ops/s, 17.7 MB
    alloc_fail 1.51M
    descriptor_exhausted 3.39M
    descriptor_probe_total 1.736B
    source_block_probe_total 193.2M

  front1k-desc4k-source512-route4k / balanced:
    1.698M ops/s, 100.5 MB
    alloc_fail 45.8K
    route_register_fail 91.9K
    source_block_exhausted 31.9K
    route_register_probe_total 1.109B
    route_unregister_probe_total 25.3M

  broad / balanced:
    same shape as front1k-desc4k-source512-route4k.

Read:
  route4k is not bottlenecked by route capacity anymore. It is dominated by
  descriptor exhaustion and descriptor full-scan pressure.

  front1k/broad removes descriptor exhaustion, but shifts the cost into
  source allocation, route register/unregister probe pressure, and RSS.

  The next useful work is not another fixed capacity lane. It needs
  descriptor/source lifetime diagnostics and then a design that reuses
  descriptors/source slots without putting scan/release work on the allocation
  slow path.
```

HZ6 Redis focused read:

```text
Source:
  docs/benchmarks/windows/paper/20260601_101345_hz6_capacity_matrix_windows.md

redis / rss / RUNS=1 / TimeoutSeconds=20:
  source512-route4k:
    all patterns timeout with rc124.

  front1k-desc4k-source512-route4k:
    all patterns timeout with rc124.

  appcap:
    completes all patterns, but peak RSS is about 597 MB.
    SET 12.06M
    GET 14.44M
    LPUSH 6.40M
    LPOP 15.98M
    RANDOM 7.63M

Read:
  Redis needs appcap-scale descriptor/source/front capacity or a smarter
  lifetime/source strategy. The focused non-route lanes are still too small,
  while appcap proves completion at unacceptable RSS.
```

Current HZ6 Windows direction:

```text
Stop:
  fixed capacity lane proliferation.

Keep:
  route4k as the low-RSS candidate-control for random_mixed and compact rows.
  broad/front1k as mechanism evidence for front-cache capacity.
  appcap as completion/control evidence only.

Next:
  add diagnostic-only state/lifetime attribution before behavior:
    descriptor state counts at failure/checkpoint
    source-block state counts at failure/checkpoint
    route register/unregister pressure by source/front if practical

  Then attack:
    descriptor recycling without full descriptor scans
    source-block/slot reuse by class or run
    route registration lifetime so broad-like completion does not require
    appcap-like RSS.

Do not:
  add hot-path atomics to real benchmark lanes.
  hide route4k failures with appcap defaulting.
  add another large fixed capacity lane without a lifetime hypothesis.
```

HZ6 lifetime-state diagnostic:

```text
Source:
  docs/benchmarks/windows/paper/20260601_102002_hz6_capacity_matrix_windows.md

Implementation:
  added diagnostic-only failure-state snapshots under HZ6_DIAGNOSTIC_PROBES.
  normal speed artifacts are not rebuilt with these counters unless
  -DiagnosticHz6Probes is requested.

mixed_ws / balanced / route4k:
  descriptor failure state:
    active_max = 512
    local_free_max = 457
    transfer/free/remote/orphan/released = 0

  source-block failure state:
    active_max = 128
    registered_max = 128
    ref_nonzero_max = 128
    ref_zero_max = 0

mixed_ws / balanced / front1k-desc4k-source512-route4k:
  descriptor failure state:
    all zero because descriptor capacity no longer fails.

  source-block failure state:
    active_max = 512
    registered_max = 512
    ref_nonzero_max = 512
    ref_zero_max = 0

larger_sizes:
  route4k still descriptor-fails with active_max = 512.
  front1k-desc4k-source512-route4k still source-block-fails with
  active/registered/ref_nonzero = 512.

Read:
  The failure is real lifetime retention, not just a poor free-slot search:
    descriptors are full of live or locally cached objects.
    source blocks are all active, route-registered, and still referenced.

  Next behavior should avoid fixed capacity growth and instead reduce lifetime
  retention:
    class/run reuse before new source blocks
    source-block slot reuse or partial-run reuse
    descriptor recycling only when route/fail-closed ownership remains valid

  A blind descriptor free-list alone is not enough because the route4k failure
  snapshot shows the table is full of ptr-backed descriptors, not missed null
  descriptors.
```

## HZ6 Windows Current Read

```text
Latest app-like matrix:
  results/windows-benchmark-suite/20260528_115721/app-like

Default HZ6 rows:
  hz6-strict / hz6-speed / hz6-rss
  small R1 capacities
  keep as smoke/control rows

Broad HZ6 rows:
  hz6-strict-broad / hz6-speed-broad / hz6-rss-broad
  same profiles with larger fixed capacities for broad working-set matrix rows
  use for large_slices and same-runner broad comparison

Appcap HZ6 rows:
  hz6-strict-appcap / hz6-speed-appcap / hz6-rss-appcap
  app-like fixed capacity rows for Larson/live-set separation

Capacity finding:
  the earlier 8K..64K Windows weakness was dominated by alloc_fail from
  descriptor/route/source capacity exhaustion.

Route finding:
  exact-route hash probing before fail-closed scan fallback restores much of
  the mid/large working-set performance while preserving invalid/interior
  pointer scan checks.
```

App-like repeat-3 read:

```text
Larson:
  HZ6 default and broad rows fail during warmup.
  HZ6 stats show descriptor exhaustion at default 64 and broad 4096.
  appcap rows run at T=1 with no alloc_fail, but throughput remains far below
  HZ3/tcmalloc/mimalloc, so the remaining issue is lifecycle/front-source
  behavior rather than raw live-set capacity.

Random mixed:
  small and medium/mixed rows still fail on the strict / speed / rss controls.
  broad and appcap rows survive, but they remain far behind HZ3/HZ4/mimalloc/
  tcmalloc on throughput.

MT remote:
  HZ6 rows are extremely slow under the legacy remote runner.
  Treat as remote-contract mismatch until an HZ6 owner/remote-aware runner is
  available.

Redis workload:
  HZ6 default rows are strong on SET / GET / LPUSH.
  HZ6 broad rows are weak, so broad-cap is not a Redis default.
```

Latest HZ6 standalone Windows benchmark:

```text
results:
  private/allocators/hakozuna/hakozuna-hz6/private/raw-results/windows/
    hz6_benchmark_20260528_124050

local:
  strict 8192   ~60.16M ops/s
  strict 65536  ~68.05M ops/s
  speed  8192   ~47.57M ops/s
  rss    8192   ~52.44M ops/s

remote 131072:
  strict/speed/rss ~57.49M..60.44M ops/s

reuse 131072:
  strict/speed/rss ~58.30M..63.88M ops/s
```

Read:

```text
The standalone runner is a stable contract baseline for local/remote/reuse.
Remote/reuse are usable, but still profile-sensitive.

Treat the legacy mt_remote runner as contract-mismatched until the HZ6 owner-
aware remote runner exists.
The mt_remote paper runner is now timeout-bounded and skips HZ6 rows by
default. Use `-IncludeHz6Legacy` only to debug the mismatch; HZ6 remote verdicts
should come from the HZ6 standalone local/remote/reuse runner or the owner-
aware HZ6 mt-remote runner.
```

Post-route-hash large-slice signal on Windows:

```text
8K:
  hz6-strict-broad ~54.7M ops/s, near tcmalloc ~57.1M

16K / 32K / 64K:
  hz6-strict-broad beats or roughly matches tcmalloc in the latest local run

128K:
  hz6-speed-broad ~69.6M ops/s, above tcmalloc ~46.7M
```

Next HZ6 rule:

```text
Do:
  keep fixed broad-cap rows for reproducible broad comparison
  keep small-cap rows as controls
  inspect alloc_fail before interpreting a row
  expose HZ6 stats in app-like failure rows before adding new knobs

Do not:
  call hz6-*-broad the production/default lane yet
  hide small-cap controls
  add adaptive capacity to comparison rows before a dry-run/growth policy exists

Next order:
  1. Keep the HZ6 standalone local/remote/reuse runner as the first contract
     baseline.
  2. Compare HZ6 against HZ3 / HZ4 / HZ5 on the same machine and same runner.
  3. Keep HZ6 appcap rows as Larson capacity-separation evidence.
  4. Build an HZ6 owner/remote-aware runner for MT remote.
  5. Larson lifecycle/front-source diagnostic.
  6. Redis default-row mechanism readout.
  7. hz6-adaptive-cap-dryrun only after the above evidence is clean.

Research candidate:
  hz6-adaptive-cap-dryrun
  then hz6-adaptive-cap only if growth counters justify it
```

Latest capacity-breakdown read:

```text
results/windows-hz6-capacity-breakdown/20260528_105215_allocator_matrix.md

8K default HZ6:
  route_register_fail dominates alloc_fail.

64K default HZ6:
  descriptor_exhausted dominates alloc_fail.

Broad-cap rows:
  clear all three tracked exhaustion counters in the checked 8K/64K slices.
```

Long historical task notes were archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```

## Current Claim

```text
MidPage:
  PageRun64 remains the strong keep for general malloc main/mid/cross64 rows.

LargeFront:
  128K remote-heavy rows are the active tcmalloc chase area.
  No single LargeFront diagnostic is broad enough to replace saved profiles.

Documentation:
  keep current_task short.
  put long result logs in docs/archive or dedicated result docs.

HZ6:
  keep separate from HZ5 implementation.
  design route-safe / transfer-first / RSS-aware contracts before writing code.
```

## HZ6 Design Seed

HZ6 docs now live under:

```text
hakozuna-hz6/
```

Read in this order:

```text
hakozuna-hz6/README.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
hakozuna-hz6/docs/current_task.md
```

Current HZ6 position:

```text
HZ3 lesson:
  thin local/TLS cache

HZ4 lesson:
  remote grouping and page/span reuse

HZ5 lesson:
  fail-closed descriptor ownership and low-RSS profile discipline

HZ6 plan:
  make RouteLayer, FrontCache, TransferLayer, SourceLayer, ScavengeLayer,
  OwnerLayer, and PolicyLayer explicit from the first implementation.
```

## Current LargeFront Read

Saved / comparison lanes:

```text
large128-rss:
  source batch4, low-RSS fixed profile.

large128-source16:
  source batch16, current best r90/t8 comparison lane.
```

Diagnostics:

```text
large128-r50-hold8:
  wins t8/r50 in one focused run.
  loses source16 badly on r90.
  keep as r50 diagnostic only.

large128-direct-header:
  unsafe ptr-4096 header lookup upper bound.
  improves t4/r50 and wins t8/r90 in one run.
  not promotable because it is not fail-closed for foreign pointers.

large128-base-directmap:
  safe exact-base one-slot route cache.
  improves t4/r90 versus source16.
  loses source16 on t8 rows.
  no-promote diagnostic.

large128-r50-drain-directmap:
  combines drain1 and base-directmap.
  current RUNS=3 no-go: it does not beat either parent and r90 RSS/ops regress.

large128-ownerfast:
  enables LargeFront same-owner load/store state transition.
  current RUNS=3 no-go: t4/t8 r50 and t8/r90 regress versus source16.

large128-drainbulk:
  source16 plus bulk local-list commit during owner remote drain.
  diagnostic only: t4 rows show signal, but t8 rows regress versus source16.
  useful evidence that local_push overhead is only a slice of the remaining
  drain/refill gap.

large128-draintrust:
  source16 plus trusted load/store for owner-drained
  REMOTE_PENDING->LOCAL_FREE transitions.
  diagnostic only: t8/r50 beats tcmalloc in RUNS=3, but t8/r90 loses source16.
  useful evidence that drain state CAS is a real slice, but not the broad fix.
```

Recent result roots:

```text
private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802
private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345
private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731
private/raw-results/linux/hz5_large128_current_focus_r3_20260526_052851
private/raw-results/linux/hz5_large128_drain_directmap_r3_20260526_053047
private/raw-results/linux/hz5_large128_t4r50_perf_current_20260526_053300
private/raw-results/linux/hz5_large128_rb_current_r3_20260526_053400
private/raw-results/linux/hz5_large128_batch32_smoke_20260526_053458
private/raw-results/linux/hz5_large128_base_directmap4_r3_20260526_053547
private/raw-results/linux/hz5_large128_ownerfast_r3_20260526_053858
private/raw-results/linux/hz5_large128_direct_header_recheck_r3_20260526_055156
private/raw-results/linux/hz5_large128_source_populate_r3_20260526_055548
private/raw-results/linux/hz5_large128_l0_compare_20260526_060606
private/raw-results/linux/hz5_large128_drainbulk_r3_20260526_061000
private/raw-results/linux/hz5_large128_drainbulk_tail_r3_20260526_061107
private/raw-results/linux/hz5_large128_draintrust_r3_20260526_061614
private/raw-results/linux/hz5_large128_draintrustbulk_manual_r3_20260526_061931
private/raw-results/linux/hz5_large128_draintrust_budget1_manual_r3_20260526_062121
private/raw-results/linux/hz5_large128_source16_draintrust_l0_compare_20260526_062558
private/raw-results/linux/hz5_large128_source16_draintrust_perf_20260526_062616
private/raw-results/linux/hz5_large128_source16_draintrust_perf_t4_20260526_062634
private/raw-results/linux/hz5_large128_source16_draintrust_median_r3_20260526_062644
private/raw-results/linux/hz5_large128_transfer128_smoke_r3_20260526_063953
private/raw-results/linux/hz5_large128_transfer128_flushmiss_r3_20260526_064056
private/raw-results/linux/hz5_large128_transfer128_tlsfirst_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_ownershard_r3
private/raw-results/linux/hz5_large128_transfer128_shard16_smoke_r1
private/raw-results/linux/hz5_large128_transfer128_shard16_mask_smoke_r1
```

## Next Engineering Direction

```text
1. Keep source16 and large128-rss as the current comparison pair.
2. Treat hold8/base-directmap/direct-header/drain-directmap as diagnostics.
3. Current t4/r50 perf gap is instruction/branch count plus page-fault/refill
   pressure, not cache-miss rate alone.
4. rb32/rb64, batch32, and ownerfast do not fix t4/r50; keep them
   diagnostic/no-go.
5. Direct-header recheck does not improve t4/r50; free lookup alone is not
   the next primary fix.
6. MAP_POPULATE source refill is a hard no-go; page-fault prepopulation
   explodes RSS and throughput collapses.
7. L0 confirms r50-drain republish churn is huge; it is not a clean t4/r50
   fix. Source16 has no republish but has many REMOTE_PENDING->LOCAL_FREE
   conversions.
8. Drainbulk reduces part of that conversion overhead and can improve t4, but
   t8 regressions mean local-push batching is not the broad fix.
9. Draintrust shows the drain CAS cost matters. The latest RUNS=3 recheck is
   split: t8/r90 wins strongly with low RSS, but t8/r50 loses source16 and
   t4 rows remain far behind tcmalloc.
10. Draintrust+drainbulk composition is no-go in manual RUNS=3.
11. Draintrust+budget1 is a t4/r50-only local optimum: 29.56M on manual RUNS=3,
   but t4/r90 and t8 rows collapse. Do not add it as a named lane.
12. L0 source16/draintrust recheck shows draintrust increases source refill
    pressure on some rows. Example: t4/r90 source16 source_refill=392 versus
    draintrust source_refill=1143 in the L0 run. Trusted drain can reduce a
    state-transition slice, but can also perturb reuse/refill timing.
13. Perf recheck says instruction count is not always the only t8 limiter:
    at t8/r50 HZ5 and tcmalloc are close on aggregate instructions/op, while
    elapsed still differs. Treat parallel efficiency, owner-drain timing, and
    source/refill pressure as co-equal suspects.
14. Transfer128 class cache is a useful structural diagnostic: it improves
    t4/r50 with very low RSS, showing class-level transfer can cut owner-inbox
    cost. It is not broad yet; t8 rows regress, likely from global transfer
    contention or poor high-thread consumption order.
15. Transfer128-tlsfirst is no-go: producer-local retention starves consumer
    reuse and worsens RSS.
16. Transfer128-ownershard is no-go: routing by old owner slot preserves
    neither transfer128's t4/r50 signal nor source16's r90 behavior.
17. Transfer128-shard16 is no-go even after adding a nonempty mask to avoid
    empty-shard locks. Consumer-visible sharding loses the global transfer128
    t4/r50 win and does not recover t8.
18. Do not add another policy until a concrete hotspot explains the row split.
19. Keep speed lanes free of HZ5_PRELOAD_STATS and hot-path counters.
```

## Cleanup Status

Completed in this cleanup phase:

```text
HZ5_LINUX_PROFILE_MATRIX.md:
  shortened to current registry.
  history moved to docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md.

current_task.md:
  shortened to current state.
  history moved to docs/archive/current_task_2026-05-hz5-linux-post-largefront.md.

HZ5_LINUX_DEV_BRIEF.md:
  restored to a short entry point.
  history moved to docs/archive/HZ5_LINUX_DEV_BRIEF_HISTORY_2026-05.md.

HZ5_LINUX_ROUTE_LANE_MATRIX.md:
  shortened to current route/lane naming rules.
  history moved to docs/archive/HZ5_LINUX_ROUTE_LANE_MATRIX_HISTORY_2026-05.md.

HZ5_BENCH_RESULTS_INDEX.md:
  shortened to current active evidence index.
  history moved to docs/archive/HZ5_BENCH_RESULTS_INDEX_HISTORY_2026-05.md.

HZ5_P0_ALIGNED_RUN_8192.md:
  shortened to lifecycle index.
  history moved to docs/archive/HZ5_IMPLEMENTATION_LIFECYCLE_HISTORY_2026-05.md.

HZ5_LINUX_LOCAL2P_DESIGN.md:
  shortened to current Local2P profile boundary.
  history moved to docs/archive/HZ5_LINUX_LOCAL2P_DESIGN_HISTORY_2026-05.md.

HZ5_P43I_P43O_ALGO_CONSULT.md:
  shortened to current exact-route consultation summary.
  history moved to docs/archive/HZ5_P43I_P43O_ALGO_CONSULT_HISTORY_2026-05.md.

HZ5_SOURCE_CLEANUP_PLAN.md:
  added source-code split plan.
  active LargeFront/MidPageFront C files are intentionally not split until the
  current LargeFront tcmalloc-chase measurement direction stabilizes.

build_linux_hz5_standalone.sh:
  argument parser moved to linux/hz5_build_arg_parser.sh.

linux/hz5_build_arg_parser.sh:
  human-facing profile aliases moved to linux/hz5_build_profile_aliases.sh.
  parser is now a short dispatcher.
  low-level feature flags split into exact/midpage/front-end groups.

build_linux_hz5_standalone.sh:
  reduced below 1000 lines.
  long usage text, compound profile helpers, validation, and build-config
  output moved to focused linux/hz5_build_*.sh helpers.
```

Remaining cleanup candidates:

```text
hakozuna-hz5/largefront/hz5_largefront.c:
  large but acceptable during active optimization.
  only refactor after measurements stabilize.

hakozuna-hz5/midpagefront/hz5_midpagefront.c:
  large but active/stable enough to defer until allocator behavior settles.
```

## Reading Order

```text
1. current_task.md
2. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
3. hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
4. linux/HZ5_BUILD_SCRIPT_LAYOUT.md
5. hakozuna-hz5/docs/HZ5_SOURCE_CLEANUP_PLAN.md
6. hakozuna-hz5/docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md
7. hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux-post-largefront.md
```
