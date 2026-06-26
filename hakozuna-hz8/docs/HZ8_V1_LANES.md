# HZ8 V1 Lanes

This document starts after `hz8-small-v0-rc1`.

Small v0 is frozen as a stable baseline.  V1 work should not mutate small-v0
behavior unless a hard safety issue is found.

## Lane 1: MediumRun-v1

Problem:

```text
v0 does not claim >4KiB throughput
main_* and cross128 rows are not meaningful without medium coverage
```

Current decision:

```text
p2-v0 remains the small-v0 default
small class-map default attempts are closed
upper3072 data is carried as size-boundary evidence only
```

First box:

```text
MediumRunBoundaryDesign-L1
docs/HZ8_MEDIUM_RUN_V1.md
```

Scope:

```text
behavior unchanged
define medium range and boundary with small p2-v0
define medium pointer identity and route lookup
define medium ownership token
define local allocation/free lifecycle
define remote free claim authority
define decommit / retained policy
define benchmark rows and promotion gates
```

Initial implementation sequence:

```text
1. MediumRunBoundaryDesign-L1
2. MediumRunRouteShadow-L1
3. MediumRunMetadataScaffold-L1
4. MediumRunLocalOnly-L1
5. MediumRunRemote-L1
6. main_* / cross128 gate matrix
```

Do not mix this lane with preload API expansion or small hot-leaf tuning.

Current MediumRun status:

```text
freeze:
  hz8-medium-v1-rc1 records protocol / geometry / lifecycle
  lazy128 residency is the MediumRun-v1.1 default
  short paired R10 HOLD was superseded by longer fresh alternating gates

implemented:
  4097..65536 routing
  owner-attached medium remote publish
  owner collect cadence
  MPSC medium pending queue
  active owner lock elision
  64KiB quantum directory
  detached-run class index
  directory capacity expanded to avoid phase fallback
  ctx-aware collector active-empty-live retention
  medium active-hit allocation check collapse
  medium same-owner free direct-identity shape
  medium malloc init fast path
  medium class-resolved allocation entry
  medium active owner-check collapse

default geometry:
  8K / 16K / 32K / 64K
  q64-run64k2
  64K class uses 128KiB run / 2 slots

default residency:
  budgeted empty-resident retention
  active empty live retention for TLS active run
  owner exit drains retained and active-live payload

residency candidate:
  lazy128 persistent owner-attached reservation
  128MiB lazy reservation cap
  conservative retained-empty overhead about 212MiB
  promoted as MediumRun-v1.1 default after longer fresh alternating gates
```

Next local-leaf lane:

```text
MediumLocalFreeRunCache-L1:
  status:
    implemented as build-time evidence target
    HOLD as default
  targets:
    bench-mediumfreecache
    bench-release-mediumfreecache
  contract:
    TLS remembers the last medium allocation run
    candidate free path tries cached run before the quantum directory
    cache success still requires:
      pointer range / slot alignment
      owner token match
      slot_state == ALLOCATED
      pending bit == 0
      debug builds verify cached run == directory run
  quick observation:
    medium local0 same-owner frees:
      would_succeed 80,000 / 80,000
      directory_mismatch 0
    medium r50:
      same-owner local frees would_succeed 39,901 / 39,901
      remote frees fallback 40,099
  focused A/B:
    data=bench_results/20260626T045436_medium_free_cache_local_focus/
    medium_local0 ratio:
      median 0.998
      p25 1.069
    main_local0 ratio:
      median 0.988
      p25 1.024
    small_remote focused R20:
      data=bench_results/20260626T_medium_free_cache_small_remote_r20/
      median 0.990
      p25 1.074
  decision:
    do not promote by default
    keep h8_bench_release as lazy128-v1.1 default
    keep bench-release-mediumfreecache as an evidence target
```

Current evidence candidates:

```text
MediumSlotPtrKnown-L1:
  status:
    implemented
  scope:
    allocation helpers return from already-validated slot with unchecked
    pointer arithmetic
    removes post-mutation slot_count/base validation from the hot alloc path
  quick sanity:
    data=bench_results/20260626T051304_medium_slotptr_known_quick/
    medium_local0 median 177.19M
    main_local0 median 169.80M
  decision:
    keep as code-shape cleanup

MediumCollectActiveRefillHint-L1:
  status:
    implemented as build-time evidence target
    HOLD as default
  targets:
    bench-mediumrefillhint
    bench-release-mediumrefillhint
  contract:
    owner collector may set TLS active_medium_runs[class] to a collected run
    only when:
      accepted remote frees are present
      ctx belongs to the owner being collected
      run owner token matches ctx owner
      current active hint is absent or not usable
    remote protocol, qstate, and pending authority are unchanged
  quick debug:
    active_refill_hint about 9.5k in medium r50 T=16 30k-iters
  paired R10:
    data=bench_results/20260626T052107_active_refill_paired_r10/
    medium_r50 ratio:
      median 1.013
      p25 0.978
    main_r90 ratio:
      median 0.960
      p25 1.014
    medium_local0 ratio:
      median 0.990
      p25 1.024
  decision:
    keep as evidence target
    do not promote by default

MediumRemoteRunEpisodeAttribution-L1:
  status:
    implemented
  data:
    bench_results/20260626T060900_remote_episode_attr/
  medium_r50 debug R3:
    collect_slots_per_run:
      [2.240, 2.668, 1.841, 1.928]
    owner_list_reuse / active_miss_unusable:
      about 0.98
  main_r90 debug R3:
    collect_slots_per_run:
      [5.536, 3.604, 1.976, 0.000]
    owner_list_reuse / active_miss_unusable:
      about 0.98
  read:
    active miss -> owner-list hit dominates the visible medium interleaved
    reuse symptom
    direct active refill is not acceptable because it regressed main_r90
    the next candidate should avoid mutating active directly

MediumOwnerClassRefillCandidate-L1:
  status:
    implemented as build-time evidence target
    HOLD as default
  targets:
    bench-mediumrefillcandidate
    bench-release-mediumrefillcandidate
    H8_MEDIUM_ENABLE_REFILL_CANDIDATE
  contract:
    active_medium_runs remains authoritative hot hint
    owner collector records one owner/class candidate after accepted frees
    malloc checks the candidate only after active miss and before owner-list
    scan
    remote protocol, qstate, and pending authority are unchanged
  quick debug:
    medium r50 T=16 30k-iters
    refill_reuse 25,681
    candidate install 123,161
    candidate attempt 94,229
    candidate hit 25,681
    owner_mismatch 0
    unusable 68,548
  paired R10 batch 1:
    data=bench_results/20260626T061701_refill_candidate_ab/
    medium_r50 ratio:
      median 1.034
      p25 1.074
    main_r90 ratio:
      median 1.078
      p25 1.034
    medium_local0 ratio:
      median 1.078
      p25 1.043
  paired R10 batch 2:
    data=bench_results/20260626T061746_refill_candidate_ab2/
    medium_r50 ratio:
      median 0.981
      p25 0.850
    main_r90 ratio:
      median 0.976
      p25 0.959
    medium_local0 ratio:
      median 1.014
      p25 1.018
  decision:
    do not promote by default
    keep as evidence target
    active miss -> owner-list hit is real, but a single owner/class candidate
    hint does not produce stable medium/main gains

MediumOwnerAvailableRunIndex-L1:
  status:
    implemented as build-time evidence target
    HOLD as default
  targets:
    bench-mediumavailable
    bench-release-mediumavailable
    H8_MEDIUM_ENABLE_AVAILABLE_INDEX
  contract:
    medium_by_class remains lifecycle inventory
    medium_available_shadow tracks non-active attached runs with free_mask != 0
    malloc checks available head after active miss and before owner-list scan
    remote protocol, qstate, pending authority, and lazy128 residency are
    unchanged
  shadow quick:
    default debug medium r50 T=16 30k-iters:
      head_attempt 90,443
      head_hit 90,443
      owner_hit_without_available 0
    behavior candidate debug medium r50 T=16 30k-iters:
      owner_scan 2,993
      owner_steps 16,408
      head_attempt 90,342
      head_hit 90,342
      owner_hit_without_available 0
      mismatch gates 0
  paired R10:
    data=bench_results/20260626T075921_available_index_active_excluded2_ab/
    medium_r50 ratio:
      median 1.010
      p25 0.991
    main_r90 ratio:
      median 1.051
      p25 1.125
    medium_local0 ratio:
      median 0.983
      p25 0.977
    small_remote90 ratio:
      median 1.005
      p25 1.181
  decision:
    do not promote by default
    keep as evidence target
    available index removes most owner-list scans, but medium_r50 does not
    clear the +5% behavior promotion gate

MediumAvailableIndexActiveInvariant-L1:
  status:
    implemented
  scope:
    available index is explicitly non-active
    debug/candidate path rejects active-indexed runs
    active_indexed is a hard zero gate before any promotion
  paired R10 after invariant closure:
    data=bench_results/20260626T082044_available_index_invariant_ab/
    medium_r50 ratio:
      median 0.989
      p25 0.982
    main_r90 ratio:
      median 1.144
      p25 1.167
    medium_local0 ratio:
      median 1.018
      p25 1.053
  decision:
    invariant is retained
    available-index remains HOLD because it does not improve medium_r50

MediumAvailableHitCostAttribution-L1:
  status:
    implemented as debug-only counters on the available-index target
  output:
    medium_available_hit_cost
      lock_ms
      alloc_ms
      active_ms
      collect_ms
      ns_per_reuse
  read:
    next behavior candidate requires one residual component with plausible
    >=5% medium_r50 budget
    debug timing points to post-discovery allocation / live-on-alloc cost
    rather than owner-list scan itself

MediumInlineOwnerAlloc-L1:
  status:
    implemented as opt-in release evidence target
  build:
    bench-release-mediumavailableinline
  shape:
    removes h8_medium_run_alloc_local_scaffold calls from
    h8_medium_malloc_class_inner under H8_MEDIUM_ENABLE_INLINE_OWNER_ALLOC
  data:
    bench_results/20260625T233602Z_medium_inline_owner_alloc_focus/
  focused medium_r50 R10:
    median ratio 0.998
    p25 ratio 0.844
  decision:
    HOLD
    release call-shape cleanup is not enough and worsens p25 stability

MediumPostCollectCapacityUtility-L1:
  status:
    implemented as debug/audit attribution
    behavior unchanged
  purpose:
    decide whether remaining medium r50 work is:
      demand-driven collect on active miss
      periodic collect cadence
      class geometry / size policy
      or remote-lane freeze
  output:
    medium_post_collect_active_miss
    medium_collect_source
    medium_collect_credit
  saved data:
    bench_results/20260626T014615Z_post_collect_capacity_utility_l2/
    debug_medium_r50_R3.txt
    debug_main_r90_R3.txt
    release_medium_r50_R10.txt
    release_main_r90_R10.txt
  quick debug read:
    default debug medium r50 T=16 30k-iters:
      active_miss_total 276,787
      active_miss_owner_pending 248,423
      active_miss_active_pending 106,115
      active_miss_active_pending_slots 166,720
      owner_hit_pos [65,883, 59,102, 45,319, 34,797, 62,793]
      collect_source:
        periodic_active [32,850 calls, 235,845 runs, 449,665 slots]
        periodic_owner [8,157 calls, 59,421 runs, 116,472 slots]
        capacity [8,082 calls, 79,514 runs, 152,797 slots]
        owner_exit [48 calls, 525 runs, 1,033 slots]
      collect_credit:
        created 719,967
        reused 717,777
        discarded 2,190
        outstanding_at_next_collect 28,997
        reuse_distance [34,506, 45,060, 82,146, 366,514, 189,551]
        reused_class [47,924, 95,832, 191,548, 382,473]
        quick_class [11,200, 26,602, 41,902, 82,008]
  read:
    active_pending / active_miss_total is about 38%
    quick reuse within <=7 owner-medium allocs is about 22%
    collect capacity is mostly reused overall, but not predominantly quick
  decision:
    attribution-only
    do not change qstate, owner lease, pending authority, or residency policy

MediumActiveMissDemandCollect64K-L1:
  status:
    implemented as opt-in evidence target
    HOLD as default
  builds:
    bench-mediumdemand64
    bench-release-mediumdemand64
  scope:
    64K class only
    use only the existing owner pending queue collector
    max 2 one-run collect attempts
    stop once the active run opens
    retry active allocation once
    replace this allocation's periodic collect tick on retry hit
  data:
    bench_results/20260626T025816Z_medium_demand64_quick/
  debug medium_r50 R3:
    trigger 55,529
    retry_hit 34,035
    target_not_reached 21,494
    nonactive_runs_processed 36,399
    retry_hit / trigger about 61%
  release medium_r50 R10:
    baseline 33.17M
    demand64 33.63M
    ratio about 1.014
  release main_r90 R10:
    baseline 24.47M
    demand64 24.16M
    ratio about 0.987
    demand trigger is zero for main_r90 because it does not use 64K class
  decision:
    mechanism is valid evidence, but it does not clear the +5% medium_r50 gate
    do not promote
    do not expand to budget 4 or 32K without a new attribution signal

MediumRun-v1.1 remote-lane closeout:
  status:
    freeze medium remote owner-side micro-tuning after demand64 evidence
  rationale:
    available index, inline owner allocation, refill hints, local free cache,
    post-collect utility, and 64K demand collect did not expose a safe +5%
    default behavior bucket
  HOLD:
    owner lease redesign
    qstate / pending protocol changes
    broad collect cadence changes
    available-index promotion
    demand collect budget expansion
    32K demand collect expansion
  next:
    move to SizePolicy / geometry shadow before changing remote protocol again

MediumSizePolicy-v1.2-Shadow:
  status:
    implemented as attribution / shadow
  scope:
    behavior unchanged
    keep small-v0 class map frozen
    add medium class-policy shadow for request / rounded / run-pressure data
  output:
    medium_v12_sizepolicy_shadow:
      policy=8/16/24/32/48/64
      alloc / remote_live distribution
      rounded_bytes / remote_rounded_bytes
      remote_runs / remote_run_ratio
    medium_sizepolicy_runtime:
      active_miss_class
      active_pending_class
      owner_list_hit_class
      active_switch_class
    medium_sizepolicy_collect:
      full_to_nonfull_class
      full_to_nonfull_source
  quick sanity:
    medium r50 debug smoke showed v12 remote run ratio about 0.818 vs old
    one-slot-64K model and remote rounded ratio about 1.175
    v12 increases run count vs current default q64-run64k2 for medium_r50-like
    shapes because the 48K shadow class is one-slot
  required observations:
    class-wise active miss and active pending
    class-wise active episode length and run switches
    class-wise full-to-nonfull collect by source
    requested bytes / rounded bytes by candidate medium maps
    expected run count and slots-per-run pressure
  promotion:
    no behavior promotion from shadow alone

MediumSizePolicy-v1.2-Shadow-48K2:
  status:
    NEXT
  scope:
    behavior unchanged
    extend the existing v12 shadow with a 48K two-slot run-pressure estimate
  rationale:
    the 8/16/24/32/48/64 map improves rounded bytes, but one-slot 48K worsens
    run pressure relative to current q64-run64k2 in medium_r50-like shapes
    model 48K as a two-slot 128K run before considering any behavior change
  quick sanity:
    medium_r50-shaped smoke:
      one-slot v12 run ratio vs default64k2 about 1.31
      v12_48k2 run ratio vs default64k2 about 1.00
    main_r90-shaped smoke:
      v12_48k2 run ratio vs default64k2 about 1.00
  promotion:
    no behavior promotion from shadow alone

64K two-slot:
  medium r50 positive
  promoted after budget16/order-rotated frozen small evidence
  current default geometry

chunk carve:
  per-run mmap removal candidate
  release medium r50 ratio about 0.984
  HOLD as default

upper48 medium shadow:
  rounded bytes about 9.5% lower
  run estimate unchanged
  RSS / first-touch evidence only

upper48 medium A/B target:
  build macro H8_MEDIUM_UPPER48_CLASS
  smoke-clean
  medium r50 paired signal is positive
  small interleaved quick gate failed
  HOLD as default
```

Next MediumRun choice:

```text
short term:
  MediumRun-v1 RC1 is recorded in docs/HZ8_MEDIUM_RUN_V1_RC1.md
  post-RC1 local code-shape additions are promoted as default
  same-run allocator matrix is recorded in docs/HZ8_MEDIUM_RUN_V1_MATRIX.md
  main/medium local attribution is recorded in
  docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md

if objective is local speed:
  active-hit validation and direct-identity free are complete
  medium malloc init fast path is complete
  medium class-resolved allocation entry is complete
  medium active owner-check collapse is complete
  medium collect cadence attribution is recorded
  medium active slot mutation attribution is recorded
  MediumActiveEmptyAllocFastPath-L1 simple pre-branch shape was NO-GO
  post-RC1 code-shape boxes through MediumDirectoryPtrInRunInline-L1 are
  promoted and recorded
  latest HZ8-only snapshot:
    guard_local0 372.68M
    main_local0_i0 201.95M
    medium_local0 166.22M
    main_interleaved_remote50 40.14M
    medium_interleaved_remote50 median about 36M..37M
  refreshed same-run allocator matrix is recorded:
    docs/HZ8_MEDIUM_RUN_V1_MATRIX.md
    bench_results/hz8_post_asm_same_run_matrix_20260624T222026Z/
  next speed decision should use the fresh-process matrix result, not only
  same-process HZ8-only R10 runs

if objective is medium r50 stability:
  HZ8-only same-process median is strong
  HZ8-only fresh-process direct/preload median is around 28M..29M
  full mixed same-run matrix median is much lower at 18.40M
  p25/min can still show high-minor-fault outliers
  MediumR50MatrixPressureAttribution-L1 did not reproduce a simple
  preceding-allocator-pressure cause
  MediumR50FaultModeCapture-L1 shows outliers in medium r50 only:
    direct 5/30
    preload 4/30
    medium_local0 0/30
  MediumR50FaultSourceAttribution-L1 identifies the immediate cost:
    outlier run:
      budget_reject about 102k
      madvise about 103k
      madvise_ms about 1877ms
    normal run:
      budget_reject 0
      madvise 574
      madvise_ms about 5ms
  budget32 sweep does not materially solve the outliers
  retention shadows are recorded in docs/HZ8_MEDIUM_RUN_V1.md:
    causal shadow
    exact-cap 2Q shadow
    serialized exact-cap victim shadow
  direct runtime 2Q/probation behavior was attempted and reverted:
    release quick did not complete in practical time
    O(1) probation unlink was still not enough
  next step is not another raw retention behavior:
    either accept the outlier as a v1.1 known weakness
    or design a lower-cost retention event path with stronger predicted benefit
  do not reopen the medium remote queue / lease protocol before new evidence

if objective is RSS / rounded bytes:
  upper48 remains evidence-only unless frozen small gates are reworked

if objective is main stability / first-touch:
  chunk arena remains evidence-only until medium r50 no-regression and
  fault-outlier attribution are solved
```

Next design consultation:

```text
Current default:
  keep hz8-medium-v1-rc1 protocol / geometry / lifecycle

Closed candidates:
  raw owner/class 2Q retention behavior
  owner lease redesign
  medium remote queue / qstate micro-tuning

Open design question:
  choose the next v1.1 lane without disturbing the RC1 default

Candidate lanes:
  A. accept medium-r50 fresh-process retention outliers as known weakness
  B. design a lower-cost retention event path with stronger prediction first
  C. reopen ChunkArena only with medium-r50 no-regression plan
  D. reopen SizePolicy-v1 for rounded-byte / peak-RSS improvement

Required answer:
  decide whether B/C/D has enough evidence to implement now, or whether the
  correct next step is RC/matrix documentation only.
```

Implemented closeout support:

```text
MediumRunV1RC1RetentionCloseout-L1:
  behavior unchanged
  make medium-retention-closeout
  fixed fresh-process direct/preload R30 diagnostic for medium r50
  summarizes p25/min, p95/max minor faults, post/peak RSS, and outlier count
  RC1 diagnostic only; stable-default blocker remains outlier-free retention

  result:
    bench_results/medium_retention_closeout_20260625T075433Z/
    direct outliers 1/30
    preload outliers 2/30
    stable-default retention promotion remains HOLD

MediumBudgetRejectMadvFreeEvidence-L1:
  build-time evidence only
  make medium-retention-closeout-madvfree
  data:
    bench_results/medium_madvfree_evidence_20260625T085006Z/
  result:
    direct outliers 0/30
    preload outliers 0/30
    max minor faults fell materially
    peak RSS increased materially
  interpretation:
    confirms budget-reject DONTNEED/refault is the outlier source
    not promoted as default
    ChunkArena remains a separate placement/VMA lane

MediumChunkArenaShardedCarve-L1:
  next implementation target
  build-time candidate only
  replace per-run mmap with owner-sharded monotonic chunk carve
  keep RC1 directory / retention / remote protocol unchanged
  keep quantum reuse disabled
  keep per-run MADV_DONTNEED semantics

  reason:
    old chunk candidate used a global arena lock and chunk-list scan
    previous regression may be lock-shape, not chunking itself

  promotion:
    candidate must not regress medium r50 median/p25
    candidate must not worsen fresh-process outlier count
    main/fault stability must improve materially before default consideration

  initial quick R3:
    data=bench_results/20260625T090638Z_medium_chunk_paired_gate/
    medium r50:
      30.70M baseline -> 28.86M candidate
      min worsened 23.85M -> 9.22M
    main r90:
      20.04M baseline -> 24.15M candidate
    small remote90:
      58.12M baseline -> 53.32M candidate
    decision:
      candidate remains evidence-only
      do not promote without a no-regression redesign

  three-way R5 audit:
    data=bench_results/20260625T091737Z_medium_chunk_creation_audit/
    medium r50 median:
      per-run-mmap 32.92M
      chunk16m 33.13M
      chunk16m-sharded 32.82M
    main r90 median:
      per-run-mmap 19.87M
      chunk16m 24.72M
      chunk16m-sharded 25.51M
    interpretation:
      chunk placement materially helps main r90 in this batch
      old global-lock chunk did not reproduce the medium-r50 regression here
      sharded carve increased peak RSS/outlier risk versus global chunk
      next ChunkArena work should not assume sharding alone is the fix

  global chunk R10 paired:
    data=bench_results/20260625T091840Z_medium_chunk_paired_gate/
    medium r50:
      32.64M baseline -> 34.62M chunk
      high-fault outliers remain in both builds
    main r90:
      24.73M baseline -> 24.28M chunk
    small remote90:
      54.21M baseline -> 53.39M chunk
    decision:
      chunk placement is not a retention-outlier fix
      global chunk can remain a v1.1 evidence candidate
      default promotion still requires fresh-process outlier gate

  global chunk fresh R30:
    data=bench_results/medium_chunk_closeout_20260625T100242Z/
    direct:
      median 34.76M
      p25 32.47M
      outliers 5/30
      max faults 1,221,670
    preload:
      median 36.65M
      p25 8.37M
      outliers 7/30
      max faults 783,356
    decision:
      global chunk fails fresh-process retention gate
      ChunkArena remains HOLD as default
      retention/purge outlier must be solved before stable-default promotion

MediumLazyPurgeShadow-L1:
  implemented
  behavior unchanged
  tracks budget-reject runs that a bounded lazy purge queue could have kept
  counts later reuse that would have avoided MADV_DONTNEED refault
  tracks conservative outstanding bytes, peak, and 16MiB/32MiB cap pressure

  forced low-budget smoke:
    build flag:
      H8_MEDIUM_RESIDENT_BUDGET_CLASSES=1
    medium r50 debug:
      budget_reject 4,255
      lazy candidates 4,255
      lazy reuse 4,228
      lazy peak 2,555,904 bytes
      over16m 0
      over32m 0
    interpretation:
      immediately-reused budget rejects dominate in the forced case
      bounded lazy purge is the next plausible behavior box
      formal default-budget R30 evidence is still required
  default-budget debug R3:
    medium r50 debug/audit
    budget_reject 0
    lazy candidates 0
    zero gates clean

MediumBudgetRejectLazyPurge-L1:
  implemented as build-time candidate only
  build macro:
    H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE
  candidate default cap:
    128MiB
  policy:
    budget-reject owner-attached empty runs may stay resident in a bounded
    lazy-purge pool
    owner exit / detach / hard drain still uses MADV_DONTNEED
    no runtime knob

  cap comparison:
    16MiB:
      data=bench_results/medium_lazy_closeout_20260625T102457Z/
      direct outliers 3/30
      preload outliers 2/30
      decision: NO-GO
    64MiB:
      data=bench_results/medium_lazy64_closeout_20260625T102543Z/
      data=bench_results/medium_lazy64_repeat_20260625T102617Z/
      direct outliers 0/30 in both batches
      preload outliers 2/30 then 1/30
      decision: HOLD, preload still not clean enough
    128MiB:
      data=bench_results/medium_lazy128_closeout_20260625T102649Z/
      direct outliers 0/30
      preload outliers 0/30
      direct p95 faults 14,430
      preload p95 faults 12,767
      post RSS unchanged around 3.5MiB..3.6MiB
      median peak RSS remains around the RC1 range for this workload
      decision: promotion candidate

  interpretation:
    confirms the stable-default blocker is a DONTNEED/refault retention
    policy issue, not medium remote protocol or chunk placement
    128MiB cap is the first bounded lazy candidate that closes both direct
    and preload R30 in the observed workload
    default promotion still requires paired main/medium/small gates and a
    final fresh-process repeat

  paired gate:
    make medium-lazy-paired-gate
    data=bench_results/20260625T102900Z_lazy128_medium_chunk_paired_gate/
    medium_interleaved_remote50:
      baseline 35.56M
      lazy128 35.29M
      ratio 0.992
    main_interleaved_remote90:
      baseline 24.87M
      lazy128 25.37M
      ratio 1.020
    small_interleaved_remote90:
      baseline 47.08M
      lazy128 54.27M
      ratio 1.153
    small_guard_local0:
      first paired wall median regressed, but steady_work did not indicate a
      small leaf regression
      repeat i1:
        data=bench_results/20260625T103100Z_lazy128_small_repeat/
        baseline 163.47M
        lazy128 166.06M
      repeat i0:
        data=bench_results/20260625T103100Z_lazy128_small_i0_repeat/
        baseline 164.48M
        lazy128 260.66M
    decision:
      medium r50, main r90, and small remote90 pass the first paired gate
      small local regression was not reproduced in targeted repeats
      final default promotion should still run a fresh repeat batch

  fresh repeat:
    paired repeat:
      data=bench_results/20260625T121546Z_medium_chunk_paired_gate/
      medium_interleaved_remote50 ratio 1.037
      main_interleaved_remote90 ratio 1.018
      small_guard_local0 ratio 1.048
      small_interleaved_remote90 ratio 1.016
    R30 repeat:
      data=bench_results/medium_lazy128_repeat_20260625T121557Z/
      direct outliers 0/30
      preload outliers 0/30
      direct max faults 20,426
      preload max faults 15,884
    decision:
      lazy128 passes the repeat gate
      was ready for promotion review at this point
      superseded by later paired R10 failures; default remains HOLD

  semantic closure:
    box:
      MediumLazyPurgeSemanticClosure-L1
    contract:
      lazy charge is a persistent owner-attached run reservation
      allocation does not release lazy charge
      lazy-charged run does not acquire normal resident budget
      owner detach / owner exit / run destroy release lazy charge
      detached run cannot retain lazy charge
      conservative retained-empty overhead is about 212MiB:
        normal empty budget 64MiB
        lazy reservation 128MiB
        active-empty-LIVE structural bound about 20MiB
    closeout:
      data=bench_results/medium_lazy128_semantic_closeout_20260625T160444Z/
      direct outliers 0/30
      preload outliers 0/30
      direct max faults 21,189
      preload max faults 19,702
    paired:
      data=bench_results/20260625T160436Z_medium_chunk_paired_gate/
      medium r50 ratio 0.969
      main r90 ratio 0.998
      small local ratio 1.060
      small remote90 ratio 0.967
      data=bench_results/20260625T160516Z_medium_chunk_paired_gate/
      medium r50 ratio 0.978
      main r90 ratio 1.038
      small local ratio 0.948
      small remote90 ratio 0.991
    decision:
      semantic closure fixes the lifecycle mismatch and keeps R30 outliers at 0
      but paired performance is slightly below the 0.98 medium-r50 gate
      keep lazy128 semantic closure as a candidate
      HOLD default promotion unless the v1.1 gate explicitly accepts this
      tradeoff or a lower-overhead persistent reservation is implemented

  cost audit:
    box:
      MediumLazyReservationCostAudit-L1
    status:
      implemented debug/audit counters only
    data:
      bench_results/medium_lazy_cost_audit_20260625T173626Z/
    medium r50 debug R3:
      acquire=15
      reuse=0
      keep_live=0
      empty_fast=0
      drop=15
      drop_detach=0
      drop_destroy=0
      normal_skip=0
      cap_reject=0
      cas_retry=0
    interpretation:
      no lazy cap pressure or CAS retry was observed in this short run
      budget rejects mapped one-for-one to lazy reservations
      this supports keeping promotion decisions on paired release gates and
      fresh-process stability gates rather than on atomic contention theory

  paired after cost audit:
    data:
      bench_results/20260625T181810Z_medium_chunk_paired_gate/
    ratios:
      medium r50 median 0.993
      medium r50 p25 1.140
      main r90 median 0.982
      main r90 p25 1.122
      small local median 1.162
      small remote90 median 1.017
    decision:
      lazy128 cleared the paired median gate in this batch
      p25 stability improved on medium and main rows
      keep lazy128 as the leading v1.1 residency candidate
      later paired R10 batches still gate default promotion

  final stability:
    data:
      bench_results/medium_retention_closeout_20260625T183955Z/
    direct R30:
      outliers 0/30
      fault median 11,096
      fault p95 18,660
      fault max 20,647
      median peak RSS 45,875,200
      median post RSS 3,665,920
    preload R30:
      outliers 0/30
      fault median 10,596
      fault p95 17,826
      fault max 22,998
      median peak RSS 44,171,264
      median post RSS 3,844,096

  saturation:
    test:
      medium-lazy-saturation
    data:
      bench_results/medium_lazy_saturation_20260625T184159Z/
    result:
      live_lazy=134217728
      peak_lazy_shadow=134348800
      resident=67108864
      active=2097152
      acquire=1024
      cap_reject=2544
      drop=1024
    interpretation:
      runtime lazy reservation reached the 128MiB cap and did not exceed it
      normal resident budget stayed at 64MiB
      active-live stayed far below the 20MiB structural bound
      owner/thread exit drained lazy, resident, and active charges to zero

  final decision:
    lazy128 semantic candidate has cleared:
      fresh direct/preload R30 stability
      saturation accounting
    but post-default paired R10 did not clear relative throughput:
      bench_results/20260625T190352Z_medium_chunk_paired_gate/
        medium r50 median ratio 0.972
        main r90 median ratio 0.938
      bench_results/20260625T190422Z_medium_chunk_paired_gate/
        medium r50 median ratio 0.915
        main r90 median ratio 0.990
    follow-up:
      short R10 was treated as suspect because both variants showed large
      cold/outlier swings
      longer fresh-process alternating gates were run
    longer fresh alternating:
      medium r50:
        data=bench_results/20260626T_lazy128_medium_r50_fresh_alt_r10_i300k/
        median ratio 7.128
        nolazy fault median 1,984,487
        lazy128 fault median 21,581
      main r90:
        data=bench_results/20260626T_lazy128_main_r90_fresh_alt_r10_i300k/
        median ratio 0.998
        p25 ratio 1.077
      small local:
        data=bench_results/20260626T_lazy128_small_fresh_alt_r10_i300k/
        median ratio 1.035
        p25 ratio 1.057
      small remote90:
        data=bench_results/20260626T_lazy128_small_remote_fresh_alt_r20_i300k/
        median ratio 1.047
        p25 ratio 1.102
    promote lazy128 as MediumRun-v1.1 default
```

Current route shadow:

```text
data:
  bench_results/20260623T204614Z_medium_route_shadow.md

shape:
  16..32768 interleaved remote90 audit R3

medium candidates:
  4,203,559 / 4,800,000 allocations

medium remote-live:
  3,783,343 objects

interpretation:
  main-like workload is dominated by >4KiB requests
  MediumRun-v1 needs local and remote support before main_* claims
```

## Lane 2: SizePolicy-v1 Evidence

Problem:

```text
phase_remote90 peak RSS is about 3.8GiB for 16..4096 p2-v0
```

Interpretation:

```text
cause:
  p2-v0 rounded live payload under a barrier workload

not cause:
  remote publish failure
  reclamation failure
  owner-exit leak
```

First box:

```text
SizePolicyV1Shadow-L1
```

Scope:

```text
behavior unchanged
measure requested live bytes
measure p2-v0 rounded live bytes
shadow candidate class maps
compute lower-bound spans per owner/class
estimate candidate phase peak RSS
report per-class pressure
```

Candidate maps:

```text
p2-v0:
  16,32,64,128,256,512,1024,2048,4096

upper1p5:
  16,32,64,128,256,512,1024,1536,2048,3072,4096

future candidates:
  only after shadow evidence
```

Current shadow result:

```text
data:
  bench_results/20260623T184720Z_size_policy_v1_shadow.md

remote-live rounded bytes, 16..4096 phase remote90 R3:
  p2-v0      3,947,889,632
  upper1536  3,855,197,152
  upper1p5   3,485,462,496

lower-bound spans:
  p2-v0      60,314
  upper1536  58,974
  upper1p5   53,610
```

Interpretation:

```text
upper1p5:
  strongest current candidate for phase peak reduction
  still not the RC1 default
  needs paired A/B only after the small/medium boundary decision
```

Follow-up A/B:

```text
data:
  bench_results/20260623T190034Z_upper1p5_cache_shape.md

result:
  phase peak RSS -11.1%
  phase throughput +8.1%
  interleaved remote90 -2.2%
  guard_local0 -27.2%

decision:
  upper1p5 remains HOLD as a default map
```

Second follow-up:

```text
data:
  bench_results/20260623T190416Z_upper3072_cache_shape.md

candidate:
  upper3072-v0
  16,32,64,128,256,512,1024,2048,3072,4096
  p2-v0 behavior preserved for <=2048

R5 result:
  guard_local0 -0.46%
  interleaved remote90 -1.62%
  phase remote90 +9.04%
  phase peak RSS -8.87%
  phase minor faults -8.89%

decision:
  CONDITIONAL GO before R10 x 2
```

`upper3072-v0` is the current best candidate because it reduces the 16..4096
phase peak without opening the frequent 1536-byte class in the 16..2048 guard
local row.

Paired R10 x 2 follow-up:

```text
data:
  bench_results/20260623T192704Z_upper3072_paired_r10x2.md

combined 20-run median:
  local -3.74%
  interleaved remote90 -3.71%
  phase remote90 +7.89%
  phase peak RSS -8.87%
  phase minor faults -8.90%

decision:
  upper3072-v0 HOLD as default
  keep as evidence for SizePolicy-v1
```

Prefix monomorph follow-up:

```text
data:
  bench_results/20260623T203530Z_upper3072_prefix_monomorph_r10x2.md

change:
  p2 prefix geometry specialized for <=2048 in the upper3072 build

combined 20-run median:
  local +2.22%
  interleaved remote90 -3.56%
  phase remote90 +7.69%
  phase peak RSS -8.88%
  phase minor faults -8.90%

decision:
  local regression was code-shape sensitive
  interleaved hot gate still fails
  upper3072 remains evidence-only
```

Final small class-map decision:

```text
p2-v0:
  keep as small-v0 / rc1 default

upper1p5:
  evidence-only

upper3072:
  evidence-only

small class-map default attempts:
  closed for now
```

Carry the upper3072 data into the MediumRun / size-boundary design.  Do not
reopen the small class-map default lane unless new workload evidence changes the
objective function.

Do not add runtime profile knobs.  Development A/B may use build-time
configuration only.

## Lane 3: RC1 Regression Guard

Before merging behavior changes, rerun:

```text
h8_smoke
h8_safety_stress
preload smoke
guard_local0
small_interleaved_remote90
```

For wider changes, also rerun:

```text
small_phase_remote90
same-run matrix smoke
```

Small-v0 hard gates remain:

```text
invalid platform fallback = 0
duplicate claim = 0
quiescent pending bitmap = 0
quiescent pending repair = 0
timeout / abort = 0
```

## Lane 4: App / Preload Compatibility

Current preload API surface:

```text
malloc
calloc
free
```

Next compatibility work should be measured separately from allocator-core v1
work.  Do not mix app compatibility fixes with SizePolicy or MediumRun boxes.

## Explicit Holds

```text
owner lease elision:
  HOLD

intrusive remote_head:
  HOLD

regular adoption default-on:
  HOLD

resident empty span pool:
  HOLD

small-v0 hot leaf tuning:
  HOLD without new assembly evidence
```
