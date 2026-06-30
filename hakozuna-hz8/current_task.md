# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

## Frozen Baselines

```text
small-v0:
  hz8-small-v0-rc1
  docs/HZ8_SMALL_V0_RC1.md

medium-v1 protocol/geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

medium-v1.1 current default:
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md
```

Small-v0 behavior is frozen unless a hard safety issue appears. MediumRun-v1.1
is the current balanced default. Do not reopen its owner queue, pending/qstate
protocol, residency policy, or class geometry for incremental tuning.

User-facing policy:

```text
public allocator:
  HZ8

internal lineage / reference:
  HZ3 / HZ4 / HZ5 / HZ6 / HZ7

experimental lab:
  HZ9
```

Do not ask users to choose among HZ3..HZ9. HZ8 is the balanced release line;
HZ9 remains an opt-in throughput research lane until it earns a separate,
simple public promise.

Platform policy:

```text
Linux:
  current release/default evidence line
  v1.1 stays frozen except hard safety fixes

Windows:
  bring-up lane only
  no public default claim yet
  no parity claim vs Linux v1.1 yet
```

```text
frozen small:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  startup-loaded Linux x86_64 ELF TLS contract
```

## Current Default Shape

```text
medium range:
  4097..65536

medium classes:
  8K / 16K / 24K / 32K / 48K / 64K

medium geometry:
  q64-v12-48k2
  24K class uses 64KiB run / 2 slots
  48K class uses 128KiB run / 2 slots
  64K class uses 128KiB run / 2 slots

medium identity:
  direct registry
  64KiB quantum directory
  power-of-two slot decode for p2 classes
  24K local-free uses exact two-offset decode
  remote/route/usable_size and 48K still use the generic exact non-p2 decode
  interior pointers INVALID

medium residency:
  budgeted empty-resident retention
  TLS active empty-live retention
  ctx-aware collector active-empty-live retention
  owner exit is the hard drain point

legacy comparison:
  q64-run64k2 remains available through medium64k2 build targets

medium remote:
  owner-attached remote free publishes to owner queue
  pending bit is remote claim authority
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

## Latest Benchmark Snapshot

Same-run matrix:

```text
record:
  bench_results/hz8_v11_same_run_matrix_20260629T004316Z/

row family:
  hz8 / hz8_legacy64k2 / hz3 / hz4 / mimalloc / tcmalloc / system
  RUNS=10, THREADS=16, ITERS=100000
```

Representative medians:

```text
guard_local0:
  hz8 264.15M ops/s, RSS 6.14 MiB

fixed24_local0:
  hz8 323.23M ops/s, RSS 6.05 MiB

fixed48_local0:
  hz8 340.68M ops/s, RSS 6.10 MiB

medium_local0:
  hz8 169.49M ops/s, RSS 6.21 MiB

main_local0:
  hz8 136.48M ops/s, RSS 6.53 MiB

main_interleaved_r90:
  hz8 2.68M ops/s, RSS 144.06 MiB

small_interleaved_remote90:
  hz8 0.91M ops/s, RSS 868.28 MiB

Post-L1 targeted R3:

```text
record:
  bench_results/small_interleaved_remote90_hz8_r3.log
  bench_results/main_interleaved_r90_hz8_r3.log
  bench_results/medium_interleaved_r50_hz8_r3.log

hz8 medians:
  small_interleaved_remote90  1.06M ops/s, peak RSS 455.47 MiB
  main_interleaved_r90        4.54M ops/s, peak RSS 113.75 MiB
  medium_interleaved_r50      8.19M ops/s, peak RSS 122.10 MiB
```
```

Interpretation:

```text
local rows:
  low RSS remains the best HZ8 property
  fixed-class local rows are the clearest strength

interleaved / remote-heavy rows:
  remain the weakest area
  small_interleaved_remote90 is the clearest pain point
  main_interleaved_r90 and medium_interleaved_r50 are the next two

L1 observation:
  owner-side remote-pressure collect helped the weakest rows
  small_interleaved_remote90 improved modestly
  main_interleaved_r90 improved clearly
  medium_interleaved_r50 improved the most
  RSS stayed bounded on HZ8 while mimalloc/tcmalloc still trade RSS for speed
```

## Current Direction

```text
HZ8-v1.1:
  frozen balanced default

HZ8-v2:
  throughput lane only
  next work should attack the weakest rows first
  preserve v1.1 safety and RSS

Current candidate box:
  MediumLocalFastTierActiveRun-Shadow-L1
  HOLD because the release path got larger, not smaller

Interpretation:
  shadow reuse was near-perfect, so miss rate is not the limiter
  v2 throughput gains need a larger architectural move than local micro-tuning

Weakness-first order:
  1. small_interleaved_remote90
  2. main_interleaved_r90
  3. medium_interleaved_r50

Next design box:
  docs/HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md

Implementation status:
  HZ8 SmallRemotePressureCollect L1 is in the working tree
  smoke pass: ./h8_smoke
  next check: benchmark the three weakness-first rows again

Current attribution box:
  docs/HZ8_REMOTE_PRESSURE_COLLECT_ATTRIBUTION_L2.md
  source-tagged collect attribution is now in the working tree
  next decision depends on active_full vs active_miss dominance

Latest L2 diagnostic:
  bench_results/hz8_remote_pressure_l2_20260630T095411/

L2 read:
  small_interleaved_remote90 is ACTIVE_HIT_FULL dominated
  main_interleaved_r90 is mixed but still ACTIVE_HIT_FULL leaning
  medium_interleaved_r50 is not using the small remote-pressure collector

Current behavior box:
  docs/HZ8_ACTIVE_FULL_REMOTE_COLLECT_BUDGET_L1.md
  build target: make bench-remoteactivefullbudget
  default build unchanged
  purpose: test whether active-full remote collect budget is too thin

ActiveFullRemoteCollectBudget-L1 result:
  KEEP as control evidence
  not promotion
  larger active-full budget drains pending_after sharply
  small throughput is effectively unchanged and peak RSS worsens
  main throughput is neutral/slightly down
  medium row movement is unrelated to small collect counters

Next likely target:
  medium remote collect cost / collect source policy
  medium_interleaved_r50 and main_interleaved_r90 show large medium_collect_source
  and medium_residual_budget collect/slot attribution

Current medium behavior box:
  docs/HZ8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1.md
  build target: make bench-mediumcapacitybudget
  default build unchanged
  purpose: test whether unbounded capacity-miss collect is too expensive

MediumCapacityCollectBudget-L1 R3:
  bench_results/hz8_medium_capacity_budget_l1_20260630T100420/
  main_interleaved_r90 improved clearly in debug R3
  medium_interleaved_r50 stayed roughly neutral
  small_interleaved_remote90 movement is likely noise for this knob
  KEEP as promising candidate

MediumCapacityCollectBudget-L1 R5:
  bench_results/hz8_medium_capacity_budget_l1_r5_20260630T100542/
  fresh default compare: bench_results/hz8_default_r5_compare_20260630T100915/

R5 compare:
  main_interleaved_r90:
    default 207432 ops/s, peak RSS 39.28 MiB
    candidate 246696 ops/s, peak RSS 33.79 MiB

  medium_interleaved_r50:
    default 364069 ops/s, peak RSS 22.34 MiB
    candidate 373551 ops/s, peak RSS 24.26 MiB

  guard_local0:
    default 7.12M ops/s, peak RSS 7.22 MiB
    candidate 7.16M ops/s, peak RSS 9.35 MiB

  medium_local0:
    default 0.756M ops/s, peak RSS 9.02 MiB
    candidate 0.753M ops/s, peak RSS 7.12 MiB

MediumCapacityCollectBudget-L1 decision:
  KEEP as v2 candidate-watch
  not default yet
  main remote-heavy signal is real enough to preserve
  medium-only gain is small
  guard/local safety is clean in this pass

Next check:
  pair with the small-side active-full defer fix instead of tuning this budget ladder

Current small behavior box:
  docs/HZ8_ACTIVE_FULL_DEFER8_L1.md
  docs/HZ8_ACTIVE_FULL_DEFER4_L1.md
  build target: make bench-activefulldefer8
  balanced build target: make bench-defer4mediumcapacity
  combined build target: make bench-defer8mediumcapacity
  default build unchanged

Bucket probe:
  bench_results/hz8_small_active_full_bucket_20260630T114050/
  small active-full pending was mostly <= 8
  b33p was only 4 in the R3 probe

ActiveFullDefer8-L1 R5:
  bench_results/hz8_active_full_defer8_r5_20260630T114414/
  small_interleaved_remote90:
    4.63M ops/s, peak RSS 69.02 MiB
  main_interleaved_r90:
    176543 ops/s, peak RSS 44.70 MiB
  read:
    very strong small-row fix
    not sufficient alone for mixed main rows

Defer8 + MediumCapacity R3:
  bench_results/hz8_defer8_mediumcapacity_20260630T114531/
  small_interleaved_remote90:
    4.35M ops/s, peak RSS 60.29 MiB
  main_interleaved_r90:
    294364 ops/s, peak RSS 37.69 MiB
  medium_interleaved_r50:
    368147 ops/s, peak RSS 22.54 MiB

Current v2 candidate:
  ActiveFullDefer8 + MediumCapacityCollectBudget
  not default yet
  combined R5 record: bench_results/hz8_defer8_mediumcapacity_r5_20260630T114702/

Combined R5:
  small_interleaved_remote90:
    4.39M ops/s, peak RSS 62.32 MiB
  main_interleaved_r90:
    251703 ops/s, peak RSS 41.00 MiB
  medium_interleaved_r50:
    362853 ops/s, peak RSS 22.38 MiB
  guard_local0:
    7.40M ops/s, peak RSS 9.30 MiB

Current read:
  current best HZ8 v2 candidate
  small remote-heavy cliff is substantially fixed
  main remote-heavy improves versus fresh default R5
  medium row is neutral
  local gate record: bench_results/hz8_defer8_local_gate_20260630T115020/

Local gate:
  main_local0:
    default 951376 ops/s, peak RSS 9.98 MiB
    candidate 941214 ops/s, peak RSS 8.03 MiB
  medium_local0:
    default 804919 ops/s, peak RSS 7.19 MiB
    candidate 810129 ops/s, peak RSS 7.15 MiB

Current decision:
  Defer8 + MediumCapacityCollectBudget was the first v2 RC candidate
  broader weak-row gate found main_remote50 regression risk
  do not replace frozen v1.1 default yet
  next check should prefer the Defer4 balanced lane

Broader gate:
  candidate record: bench_results/hz8_defer8_rc_broad_gate_20260630T115242/
  default record: bench_results/hz8_default_rc_compare_20260630T115859/

Defer8 broader read:
  guard_remote90:
    default 4.051M ops/s, peak RSS 121.13 MiB
    defer8  4.693M ops/s, peak RSS 55.01 MiB
  main_remote50:
    default 401673 ops/s, peak RSS 22.16 MiB
    defer8  367322 ops/s, peak RSS 27.46 MiB
  main_remote90:
    default 200548 ops/s, peak RSS 38.88 MiB
    defer8  287243 ops/s, peak RSS 38.98 MiB
  medium_remote90:
    default 174881 ops/s, peak RSS 50.29 MiB
    defer8  240177 ops/s, peak RSS 41.14 MiB
  decision:
    keep as high remote-pressure evidence/control
    not default because main_remote50 regresses too much

Current balanced v2 RC:
  ActiveFullDefer4 + MediumCapacityCollectBudget
  docs/HZ8_ACTIVE_FULL_DEFER4_L1.md
  build target: make bench-defer4mediumcapacity
  record: bench_results/hz8_defer4_mediumcapacity_gate_20260630T120049/

Defer4 balanced gate:
  guard_remote90:
    default 4.051M ops/s, peak RSS 121.13 MiB
    defer4  4.715M ops/s, peak RSS 61.52 MiB
  main_remote50:
    default 401673 ops/s, peak RSS 22.16 MiB
    defer4  393920 ops/s, peak RSS 22.85 MiB
  main_remote90:
    default 200548 ops/s, peak RSS 38.88 MiB
    defer4  270743 ops/s, peak RSS 33.51 MiB
  medium_remote90:
    default 174881 ops/s, peak RSS 50.29 MiB
    defer4  237225 ops/s, peak RSS 34.96 MiB
  decision:
    keep as balanced HZ8 v2 RC candidate
    Defer4 preserves most remote90 gains while avoiding the Defer8 r50 cliff
    run broader release matrix before any default promotion

Defer4 local gate:
  record: bench_results/hz8_defer4_local_gate_20260630T120825/
  main_local0:
    candidate 959777 ops/s, peak RSS 7.76 MiB
    pending_enqueue = 0, pending_dequeue = 0, remote = 0
  medium_local0:
    candidate 827891 ops/s, peak RSS 8.75 MiB
    pending_enqueue = 0, pending_dequeue = 0, remote = 0
  decision:
    local rows do not exercise the active-full defer path
    local throughput is neutral/positive in this short gate

Defer4 focus extra:
  record: bench_results/hz8_defer4_rc_focus_extra_20260630T121137/
  guard_remote50:
    5.587M ops/s, peak RSS 12.65 MiB
    at/above default with lower RSS
  small_interleaved_remote90:
    4.39M ops/s, peak RSS 49.05 MiB
    default probe was 527124 ops/s, peak RSS 864.04 MiB
  medium_remote50:
    unpaired R3 was noisy, so use same-run pair below

Defer4 medium r50 same-run pair:
  record: bench_results/hz8_defer4_medium_r50_pair_20260630T121206/
  medium_remote50:
    default   313948 ops/s, peak RSS 27.32 MiB
    candidate 311970 ops/s, peak RSS 24.23 MiB
  decision:
    medium_remote50 is neutral in same-run comparison
    candidate RSS is lower
    pending counters remain zero, so this row is not exercising small defer

Largeish boundary:
  docs/HZ8_LARGEISH_ROUTE_MISS_BOUNDARY.md
  current evidence:
    bench_results/hz8_defer8_rc_broad_gate_20260630T115242/largeish_remote50.log
  read:
    miss = 1600287
    pending_enqueue = 0
    pending_dequeue = 0
    remote_pressure_collect small/active sources = 0
  decision:
    not part of the Defer4 RC pass/fail gate
    treat as separate large/sys route boundary work if it becomes the next priority

Current next box:
  LargeDirectOwned-L1
  docs:
    docs/HZ8_LARGE_DIRECT_OWNED_L1.md
  build target:
    make smoke-largedirect
    make bench-defer4mediumcapacity-largedirect
  purpose:
    turn 64K..128K direct-large allocations from platform fallback into HZ8-owned
    direct objects
  why:
    largeish_remote50 currently reports miss = 1600287
    largeish does not exercise small active-full defer or medium capacity collect
  scope:
    size > H8_MEDIUM_MAX_SIZE and size <= H8_DIRECT_FALLBACK_LIMIT
    HZ8-owned route VALID / INVALID / MISS boundary
    fail-closed free for owned-looking invalid direct objects
  non-goals:
    retained large cache
    remote-fast large handoff
    direct-large throughput leadership
    changing small/medium Defer4 RC policy
  first gate:
    h8_smoke
    h8_smoke_largedirect
    largeish_remote50 focused R3
    route miss should fall materially on the largeish row
    source files remain below 800 lines
  result:
    record: bench_results/hz8_large_direct_l1_20260630T175949/
    throughput median = 227983 ops/s
    peak RSS median = 60.13 MiB
    miss = 0
    invalid = 0
  lookup-first follow-up:
    record: bench_results/hz8_large_direct_lookupfirst_l1_20260630T202900/
    throughput median = 974032 ops/s
    peak RSS median = 22.16 MiB
    miss = 0
    invalid = 0
    medium_free_lookup = 799713
    free_steps = 0
  broader gate:
    record: bench_results/hz8_large_direct_rangeguard_gate_20260630T203238/
    medium_remote50:
      defer4 = 359831 ops/s, peak RSS 23.46 MiB
      largedirect = 314325 ops/s, peak RSS 21.56 MiB
    largeish_remote50:
      defer4 = 228539 ops/s, peak RSS 56.09 MiB, miss = 1600287
      largedirect = 901406 ops/s, peak RSS 22.36 MiB, miss = 0
  read:
    route ownership boundary is fixed
    the first L1 still paid medium lookup before direct free
    lookup-first removes the huge medium free fallback scan
    keep as strong focused win / evidence lane
    do not promote broadly because medium_remote50 regresses
    next ROI is medium remote collect/free path without direct lookup-first tax
  refill-hint probe:
    build target: make bench-defer4mediumcapacity-largedirect-refillhint
    record: bench_results/hz8_large_direct_refillhint_probe_20260630T203855/
    medium_remote50:
      largedirect = 339849 ops/s, peak RSS 21.35 MiB
      refillhint = 337556 ops/s, peak RSS 21.06 MiB
    largeish_remote50:
      largedirect = 939878 ops/s, peak RSS 25.49 MiB
      refillhint = 956964 ops/s, peak RSS 24.67 MiB
    read:
      small positive on largeish, neutral/slightly weak on medium
      keep as probe target only, not promotion

Current medium collect candidate:
  MediumKeepRefillEmpty-L1
  docs:
    docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  build target:
    make bench-mediumkeeprefillempty
  purpose:
    keep owner-local refill-candidate runs active-live when a remote collect
    drains them to zero, instead of immediately marking them empty
  scope:
    opt-in HZ8 v2 candidate only
    paired with Defer4 + MediumCapacityCollectBudget
    default build unchanged
  record:
    bench_results/hz8_medium_keeprefill_empty_l1_20260630T204156/
  same-run R3:
    medium_remote50:
      defer4 = 326521 ops/s, peak RSS 22.21 MiB
      keeprefill = 1397369 ops/s, peak RSS 21.75 MiB
    main_remote90:
      defer4 = 285865 ops/s, peak RSS 31.21 MiB
      keeprefill = 1787769 ops/s, peak RSS 30.77 MiB
  attribution:
    medium_remote50 empty_ms:
      defer4 = 26313.306 ms
      keeprefill = 78.695 ms
    main_remote90 empty_ms:
      defer4 = 46821.476 ms
      keeprefill = 143.879 ms
  decision:
    KEEP as strong HZ8 v2 candidate evidence
    not default promotion yet
    next gate should compare Defer4 vs KeepRefill on the broader RC rows
    watch active_live_peak / RSS because this deliberately keeps refill
    candidates live instead of cycling them through empty/reactivate
  broad gate:
    record: bench_results/hz8_medium_keeprefill_empty_broad_gate_20260630T204641/
    guard_remote90:
      defer4 = 4.61M ops/s, peak RSS 47.38 MiB
      keeprefill = 4.66M ops/s, peak RSS 49.91 MiB
    main_remote50:
      defer4 = 427029 ops/s, peak RSS 21.75 MiB
      keeprefill = 1862192 ops/s, peak RSS 19.75 MiB
    main_remote90:
      defer4 = 281138 ops/s, peak RSS 33.02 MiB
      keeprefill = 1958436 ops/s, peak RSS 28.05 MiB
    medium_remote50:
      defer4 = 339018 ops/s, peak RSS 21.13 MiB
      keeprefill = 1442871 ops/s, peak RSS 21.01 MiB
    medium_remote90:
      defer4 = 228223 ops/s, peak RSS 37.14 MiB
      keeprefill = 1441980 ops/s, peak RSS 29.13 MiB
    main_local0:
      defer4 = 962453 ops/s, peak RSS 7.44 MiB
      keeprefill = 930269 ops/s, peak RSS 7.57 MiB
    medium_local0:
      defer4 = 826416 ops/s, peak RSS 6.67 MiB
      keeprefill = 825518 ops/s, peak RSS 8.71 MiB
  broad read:
    main/medium remote-heavy rows improve strongly
    the prior Defer8-style r50 cliff is not present in this short gate
    guard_remote90 is neutral/slightly positive
    local rows are neutral to slightly weak
    safety counters stayed clean: invalid_owned = 0, route_authority_mismatch = 0
    promote to HZ8 v2 RC candidate-watch, but keep frozen v1.1 default unchanged
    next step: longer repeat / release-sized matrix before default switch
  release gate:
    build target: make bench-release-mediumkeeprefillempty
    record: bench_results/hz8_keeprefill_release_gate_20260630T210134/
    baseline: h8_bench_release
    candidate: h8_bench_release_mediumkeeprefillempty
    RUNS=5, THREADS=16, ITERS=100000
    main_interleaved_remote90:
      release = 3.62M ops/s, peak RSS 183.86 MiB
      keeprefill = 5.92M ops/s, peak RSS 97.65 MiB
    medium_interleaved_remote50:
      release = 4.29M ops/s, peak RSS 96.62 MiB
      keeprefill = 5.81M ops/s, peak RSS 105.05 MiB
    small_guard_local0:
      release = 312.94M ops/s, peak RSS 5.39 MiB
      keeprefill = 328.33M ops/s, peak RSS 3.71 MiB
    small_interleaved_remote90:
      release = 0.56M ops/s, peak RSS 1795.07 MiB
      keeprefill = 12.76M ops/s, peak RSS 74.05 MiB
  release read:
    release build confirms the win is not debug-counter-only
    small_interleaved_remote90 cliff is largely removed
    main remote-heavy wins on both speed and RSS
    medium remote50 wins speed with modest RSS increase
    keep as the current HZ8 v2 RC nucleus
    remaining work is public/cross-allocator matrix and any final naming cleanup
  bench plumbing:
    make bench-release-mediumkeeprefillempty
    make preload-mediumkeeprefillempty
    scripts/run_medium_chunk_paired_gate.sh now accepts:
      BASELINE_MAKE_TARGET
      CANDIDATE_MAKE_TARGET
    .gitattributes pins *.sh to LF so bash scripts do not break under
    core.autocrlf=true
  preload smoke:
    record: bench_results/hz8_keeprefill_preload_matrix_smoke_20260630T211800/
    allocators: hz8, hz8_keeprefill, system
    small_interleaved_remote90:
      hz8 = 0.44M ops/s, peak RSS 1136.37 MiB
      hz8_keeprefill = 6.54M ops/s, peak RSS 43.04 MiB
      system = 3.60M ops/s, peak RSS 35.37 MiB
    main_interleaved_r90:
      hz8 = 2.04M ops/s, peak RSS 138.06 MiB
      hz8_keeprefill = 2.44M ops/s, peak RSS 107.60 MiB
      system = 2.20M ops/s, peak RSS 112.28 MiB
    medium_interleaved_r50:
      hz8 = 2.11M ops/s, peak RSS 151.89 MiB
      hz8_keeprefill = 2.46M ops/s, peak RSS 126.07 MiB
      system = 1.96M ops/s, peak RSS 74.42 MiB
    note:
      mimalloc/tcmalloc were not found by the default resolver in this
      Windows/WSL workspace; use MIMALLOC_SO / TCMALLOC_SO or the Ubuntu bench
      side for the public cross-allocator matrix
  local mimalloc matrix:
    local mimalloc build:
      private/bench-assets/linux/allocators/local/mimalloc-install/lib/libmimalloc.so.2.2
    record: bench_results/hz8_keeprefill_mimalloc_matrix_r5_20260630T212900/
    small_interleaved_remote90:
      hz8 = 0.41M ops/s, peak RSS 1126.86 MiB
      hz8_keeprefill = 6.15M ops/s, peak RSS 52.03 MiB
      mimalloc = 8.48M ops/s, peak RSS 332.82 MiB
      system = 3.24M ops/s, peak RSS 34.18 MiB
    main_interleaved_r90:
      hz8 = 2.05M ops/s, peak RSS 116.45 MiB
      hz8_keeprefill = 2.40M ops/s, peak RSS 103.01 MiB
      mimalloc = 4.41M ops/s, peak RSS 1043.07 MiB
      system = 1.99M ops/s, peak RSS 115.73 MiB
    medium_interleaved_r50:
      hz8 = 2.78M ops/s, peak RSS 111.25 MiB
      hz8_keeprefill = 2.36M ops/s, peak RSS 133.27 MiB
      mimalloc = 4.77M ops/s, peak RSS 1202.80 MiB
      system = 1.80M ops/s, peak RSS 71.03 MiB
    HZ8-only medium r50 R10:
      record: bench_results/hz8_keeprefill_medium_r50_preload_r10_20260630T213200/
      hz8 = 2.32M ops/s, peak RSS 131.45 MiB
      hz8_keeprefill = 2.52M ops/s, peak RSS 134.18 MiB
    read:
      KeepRefill strongly fixes small remote-heavy and improves main remote-heavy
      mimalloc is still faster in these rows but pays much higher RSS
      medium r50 is positive but noisy, so avoid overclaiming that row

Local-only tuning is not the next ROI.

Current policy:
  keep the v1.1 default stable
  keep Windows as bring-up / evidence only
  do not promote profile-only lanes without focused guards
```

## Gates To Preserve

```text
small frozen:
  INVALID platform fallback = 0
  duplicate claim accepted twice = 0
  quiescent pending bitmap = 0
  timeout / abort = 0

medium:
  invalid owned platform fallback = 0
  remote publish lost = 0
  claim accepted twice = 0
  collect duplicate = 0
  idle with pending = 0
  empty with pending = 0
  decommit while pending = 0
  owner/list mismatch = 0
  route authority mismatch = 0
```

## Read First

```text
lane guide:
  docs/HZ8_V1_LANES.md

medium protocol/geometry RC:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

same-run matrix:
  docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

main/medium attribution:
  docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md

benchmark gates:
  docs/HZ8_BENCH_GATE.md

small remote pressure collect:
  docs/HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md

active-full defer balanced RC:
  docs/HZ8_ACTIVE_FULL_DEFER4_L1.md

largeish route boundary:
  docs/HZ8_LARGEISH_ROUTE_MISS_BOUNDARY.md

large direct owned evidence:
  docs/HZ8_LARGE_DIRECT_OWNED_L1.md

medium keep-refill-empty candidate:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md

v2 design:
  docs/HZ8_V2_HZ9_DESIGN.md
```

## Archive Map

```text
Archived latest-direction ledger:
  docs/archive/current_task_20260629_hz8_latest_direction.md

General archive index:
  docs/archive/README.md
```
