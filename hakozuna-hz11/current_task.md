# HZ11 Current Task

## Windows Bring-up

```text
HZ11WindowsBringup-L0:
  start with standalone token/front-cache smoke only
  no LD_PRELOAD parity
  no fine128 Windows performance claim
  no span/transfer/pthread/mmap path in L0

added:
  docs/HZ11_WINDOWS_BRINGUP_L0.md
  src/hz11_port.h
  scripts/build_hz11_win64_smoke.ps1

next:
  L0 smoke passes with clang-cl on Windows
  L0 fixed-local speed observation passes:
    hz11-tlsfast ~291M-307M ops/s across 64..4096 byte slots
    system-crt ~56M-58M ops/s on the same microbench
  connected to existing Windows random_mixed runner:
    win/run_win_random_mixed_paper.ps1
    hz11-token / hz11-tlsfast rows
    RUNS=3 small/medium/mixed connectivity snapshot stored under
    docs/benchmarks/windows/hz11_l0_random_mixed_connectivity/
  Windows span/classify smoke passes:
    HZ11_CLASSIFY_SPAN=1
    VirtualAlloc arena
    CRITICAL_SECTION returned-object locks
    INIT_ONCE span init
  opt-in hz11-span row connected to random_mixed runner:
    RUNS=3 small: 149.284M ops/s, peak 4.18MB
    RUNS=3 medium: 147.110M ops/s, peak 4.99MB
    RUNS=3 mixed: 148.464M ops/s, peak 5.04MB
  hz11-span is the selected Windows HZ11 bring-up row:
    selected for Windows random_mixed / allocator-matrix connectivity
    not a DLL replacement
    not Linux fine128 parity
    not a default allocator claim
  next:
    connect hz11-span to win/run_win_allocator_matrix.ps1
    then evaluate whether Windows transfer/fine128 work is worth starting
  allocator-matrix connectivity passes:
    docs/benchmarks/windows/hz11_l1_allocator_matrix_connectivity/20260709_143810_allocator_matrix.md
    smoke: 22.212M ops/s, alloc_fail=0
    balanced: 12.202M ops/s, peak 43080KB, alloc_fail=0
  interpretation:
    hz11-span is now connected to the legacy Windows matrix.
    The matrix balanced row is much weaker than the random_mixed row and should
    be treated as the next Windows pressure signal, not as a regression in the
    standalone fixed-local/random_mixed connectivity rows.
  HZ11WindowsSpanTransferMatrix-L2:
    NO-GO for Windows selected row promotion.
    docs/no_go/HZ11_WINDOWS_SPAN_TRANSFER_MATRIX_L2.md
    docs/benchmarks/windows/hz11_l2_span_transfer_matrix_probe/20260709_170332_allocator_matrix.md
    hz11-span-transfer builds/runs with HZ11_TRANSFER_CENTRAL_SPAN=1, but:
      balanced 13.670M ops/s vs hz11-span 13.945M
      peak RSS 1282312KB vs hz11-span 38180KB
    keep as opt-in evidence/control only.
    do not port Linux transfer/central policy blindly to Windows.
    next pressure work should be Windows-specific transfer/RSS attribution or a
    smaller policy design, not transfer-cap tuning by default.
  HZ11WindowsSpanCache256-L3:
    GO as the selected Windows HZ11 bring-up row.
    docs/HZ11_WINDOWS_SPAN_CACHE256_L3.md
    diagnosis:
      hz11-span-diag balanced:
        refill 987139
        overflow 30818
        returned_push 986176
        returned_pop_hit 960795
      cache cap 32 is too small for the Windows matrix pressure shape.
    matrix A/B:
      repeat-5 direct balanced median:
        hz11-span 10.986M
        hz11-span-tlsfast 12.287M
        hz11-span-cache256 13.771M
        hz11-span-tlsfast-cache256 13.001M
      cache256-diag:
        overflow 30818 -> 3314
        malloc_hit 28669 -> 161584
    random_mixed RUNS=3:
      small 156.195M, RSS 4252KB
      medium 154.459M, RSS 5016KB
      mixed 154.560M, RSS 5124KB
    decision:
      selected Windows row is now hz11-span-cache256.
      keep hz11-span as L1 control.
      keep transfer as NO-GO/control.
      no DLL replacement or Linux fine128 parity claim.
  HZ11WindowsClassAwareRefillBatch-L1:
    CLOSED as evidence / specialist track.
    docs/HZ11_WINDOWS_CLASS_AWARE_REFILL_BATCH_L1.md
    docs/HZ11_WINDOWS_LANE_STATUS_L1.md
    result:
      cache512 alone helps balanced but not wide_ws.
      classdiag shows refill is class-local.
      classbatch32 is strong on matrix but regresses random_mixed medium/mixed.
      classbatch16 is the best candidate-watch / matrix helper:
        matrix balanced/wide_ws/larger_sizes improve materially.
        random_mixed medium/mixed has a small cost.
      classbatch16-4-7 preserves random_mixed but loses larger_sizes.
      pressure-gated classbatch protects random_mixed but loses matrix wins.
    decision:
      selected Windows row remains hz11-span-cache256.
      classbatch16 is opt-in candidate-watch / matrix helper only.
      pressure variants are no-go as implemented.
    next:
      do not continue simple threshold/range tuning.
      prefer app-like confirmation for hz11-span-cache256 or a profile-scoped
      classbatch16 row if the matrix profile matters.
  HZ11WindowsReturnedColdSkip-L2:
    KEEP as Windows matrix evidence / candidate-watch.
    docs/HZ11_WINDOWS_RETURNED_COLD_SKIP_L2.md
    added:
      HZ11_MATRIX_ATTRIB_DIAG diagnostic-only counters
      hz11-span-cache256-matrixattrib
      hz11-span-cache512-matrixattrib
      hz11-span-cache512-classbatch16-matrixattrib
      hz11-span-cache512-classbatch16-coldskip
      hz11-span-cache512-classbatch16-coldskip-matrixattrib
    diagnosis:
      classbatch16 still pays many returned-sink lock attempts that miss and
      immediately fall through to current span bump.
      matrix attribution shows returned_batch_miss -> current_hit dominates
      especially in wide_ws.
    repeat-3 matrix:
      balanced:
        classbatch16 30.591M / 39760KB
        coldskip     30.135M / 38628KB
      wide_ws:
        classbatch16 22.585M / 88732KB
        coldskip     26.770M / 74512KB
      larger_sizes:
        classbatch16 44.455M / 59472KB
        coldskip     43.682M / 60468KB
    repeat-5 loop matrix:
      balanced:
        classbatch16 27.908M / 40232KB
        coldskip     27.690M / 39960KB
      wide_ws:
        classbatch16 21.490M / 85996KB
        coldskip     22.075M / 77148KB
      larger_sizes:
        classbatch16 40.355M / 60404KB
        coldskip     40.812M / 59444KB
    random_mixed RUNS=3:
      coldskip recovers most classbatch16 medium/mixed loss:
        medium 154.331M vs cache256 155.067M
        mixed  154.246M vs cache256 154.947M
    decision:
      selected Windows row remains hz11-span-cache256.
      coldskip is not default; keep as profile-scoped wide_ws / returned-empty
      lock evidence.
      do not tune skip budget blindly.
```

```text
Active status:
  HZ11LinuxReturnedRefillBatchProbe-L1: NO-GO for Linux lane replacement; GO as
  cross-platform evidence. Added Linux non-transfer span siblings mirroring the Windows
  returned-refill classbatch idea (`span-cache256`, `span-cache512-classbatch16/32`).
  RUNS=3 transfer matrix shows classbatch is live on some rows (main_r90 and medium_r90
  improve vs cache256), but `span-transfer` remains faster and much lower RSS on every
  remote row. Do not replace Linux span-transfer or fine128; keep Windows classbatch as
  platform/profile-specific evidence. See docs/HZ11_LINUX_RETURNED_REFILL_BATCH_PROBE_L1.md.
  HZ11RealAppWorkloadExpansion-L1: GO for fine128 real-app breadth. 3 new workload
  families (git clone+repack, redis-6.2.7 source build, redis-benchmark) across 5 lanes,
  all rc=0 (no crash). Redis build: fine128 7% FASTER than tcmalloc + 20% less RSS (real
  compile win). Redis benchmark: ~3% slower + 34% less process-tree RSS. Git repack:
  near-parity wall, RSS higher (+22%, arena overhead on large buffers). cap lanes ≈
  fine128 on all (NO-GO for real apps, consistent with rocksdb). Total real-app evidence:
  6 workloads (espresso/sqlite3/rocksdb/git-repack/redis-build/redis-benchmark). See
  docs/HZ11_REAL_APP_WORKLOAD_EXPANSION_L1.md.
  HZ11Fine128RealAppPositioning-L1: updated current positioning. fine128 is the
  recommended general opt-in HZ11 lane with real-app evidence (espresso win, sqlite3
  mixed, rocksdb near-parity after malloc_usable_size fix). cap768/cap1024 are
  sh6bench-synthetic specialists only; span-transfer remains remote/mixed-only; default
  unchanged. This supersedes the earlier paper positioning where sh6bench was a single
  open gap and rseq was the leading next hypothesis. See
  docs/HZ11_FINE128_REAL_APP_POSITIONING_L1.md.
  HZ11RocksdbPostFixLanePerf-L1: GO for fine128 on real multi-thread DB; cap-specialist
  NO-GO. fine128 is near-parity with tcmalloc on rocksdb (wall 1.009x, readrandom 1.01x,
  RSS 0.956x). cap768/cap1024 ~= fine128 or slower with higher RSS -- the sh6bench win
  does NOT translate to rocksdb (the cap cuts transfer traffic but rocksdb's bottleneck is
  I/O, not the allocator). cap lanes confirmed sh6bench-synthetic specialists. This closes
  the 'does cap help real multi-thread' question. See
  docs/HZ11_ROCKSDB_POST_FIX_LANE_PERF_L1.md.
  HZ11RocksdbReadrandomCrashRootCause-L1: FIX-GO. Root cause: hz11_malloc_usable_size
  routed arena pointers to libc, which read arena slot data as a chunk header and
  SEGFAULTED (minimal repro: p=malloc(N); malloc_usable_size(p); under LD_PRELOAD).
  Fixed with an arena-aware hz11_malloc_usable_size (NULL->0; arena ptr -> slot size;
  non-arena -> libc). rocksdb readrandom now rc=0 at ~tcmalloc parity (fillrandom
  1.99s/9.93us, readrandom 1.11s/5.39us; tcmalloc 1.95s/1.12s); espresso/sqlite3 no
  regression. The real-app multi-thread correctness gap is CLOSED. Perf eval is the next
  box. See docs/HZ11_ROCKSDB_READRANDOM_CRASH_ROOT_CAUSE_L1.md.
  HZ11RealAppEvidenceRerun-L1: claim-grade real-app rerun (RUNS=10, fixed process-tree
  RSS). espresso: HZ11 near-parity + ~3x RSS win (genuine). sqlite3: HZ11 7-12% slower
  + modest RSS win (~0.83x) -- mixed, NOT the earlier (buggy) near-parity+0.4x. rocksdb
  (multi-thread DB, fillrandom+readrandom 8 threads): ALL HZ11 lanes SEGFAULT on
  readrandom; tcmalloc/jemalloc clean -- a FUNDAMENTAL correctness gap on multi-thread
  real reads. This P0 is now resolved by HZ11RocksdbReadrandomCrashRootCause-L1;
  cap1024-on-real-multi-thread needs a post-fix perf rerun. See
  docs/HZ11_REAL_APP_EVIDENCE_RERUN_L1.md.
  HZ11LaneFullEvidenceGate-L1: GO for evidence, with a runner RSS correction. Real-app
  sqlite3 confirms the synthetic lane story on wall: fine128/cap768/cap1024 are
  near-parity with tcmalloc, cap768/cap1024's sh6bench win does NOT translate to this
  single-threaded app, and span-transfer ABORTS (central-overflow issue fixed in fine128).
  The real-app runner now samples process-tree RSS; corrected sqlite3 RSS is a modest HZ11
  win (~0.84x tcmalloc), not the earlier parent-PID-sampled ~0.4x. Espresso RSS must be
  rerun with the fixed runner before any strong RSS multiplier is claimed. Next:
  fine128-centered paper/positioning, plus multi-threaded real-app (rocksdb/redis) as the
  remaining gap. See docs/HZ11_LANE_FULL_EVIDENCE_GATE_L1.md.
  HZ11ThreadCacheCapacityMiddleLane-L1: NO-GO for a middle general candidate. No
  uniform CAP 512-1024 (+byte cap) keeps remote/mixed ~= fine128 -- even cap512-bytes
  (sh6bench only 3.29x) collapses fine128's medium-row dominance (4.5x/5.8x -> ~1.5x
  tcmalloc). The big-CAP vs remote/mixed-medium tension is fundamental, not tunable.
  Lane split confirmed: fine128 generalist, cap1024/cap768-bytes sh6bench specialist,
  span-transfer remote/mixed. Next lever: class-range CAP (code change, follow-up). See
  docs/HZ11_THREAD_CACHE_CAPACITY_MIDDLE_LANE_L1.md.
  HZ11Cap1024BytesCandidatePositioning-L1: MIXED. cap1024-bytes fixes sh6bench
  (1.21x tcmalloc on the synthetic macro gate) but regresses remote/mixed vs fine128
  (medium rows 4.3x->1.5x, 5.9x->1.6x; RSS up on several rows). So cap1024-bytes is a
  sh6bench/macro-churn SPECIALIST opt-in lane; fine128 REMAINS the recommended general
  opt-in macro candidate; span-transfer stays the remote/mixed lane. Lane split is now
  explicit and 3-way. No default change. See
  docs/HZ11_CAP1024_BYTES_CANDIDATE_POSITIONING_L1.md.
  HZ11ThreadCacheCapacityByteCap-L1: GO. CAP1024 + byte cap (2 MiB) closes the
  sh6bench gap to 1.20x tcmalloc (3.5s -> 0.43s) with xmalloc_test RSS bounded
  (27648 vs plain CAP256's 52864) and no regression on the other 5 macro rows.
  Also found+fixed a cached_bytes consistency bug in the SOA slow paths. cap1024-bytes
  is a strong opt-in candidate; NOT promoted (needs a positioning box). See
  docs/HZ11_THREAD_CACHE_CAPACITY_BYTE_CAP_L1.md.
  HZ11ThreadCacheCapacityTuning-L1: HZ11_CACHE_CAP is a CONFIRMED sh6bench lever.
  Varying CAP 32->256 drops sh6bench wall 3.5s->1.9s (-45%; tcmalloc gap 9.8x->5.3x)
  with xfer_insert -40%, the other 5 macro rows flat. BUT plain CAP256 regresses
  xmalloc_test RSS 2.8x (cached-bytes retention). GO for root cause; MIXED for
  promotion. Clean win needs CAP + byte-cap policy (re-enable
  HZ11_CACHE_BYTE_ACCOUNTING). See docs/HZ11_THREAD_CACHE_CAPACITY_TUNING_L1.md.
  HZ11PaperEvidencePackage-L1 consolidates the HZ11/fine128 evidence chain into
  one paper-ready doc (lane taxonomy, macro + remote/mixed tables, 7-item
  negative ladder, claim boundary, next-work warrant). No new measurements; every
  number cites its source. See docs/HZ11_PAPER_EVIDENCE_PACKAGE_L1.md.
  HZ11PerCpuRseqCachePrototype-L2 (locked per-CPU cache) is NO-GO for the
  CPU-locality hypothesis: the locked lane REGRESSED sh6bench 3.66x vs fine128
  (12.917s vs 3.527s) -- per-op spinlock overhead dominated even though the slab
  cut transfer/central traffic. Keep fine128 as the candidate; do NOT pursue the
  lock-free rseq CS. Correctness held (all macro rows OK:5; layer self-enables).
  See docs/HZ11_PERCPU_RSEQ_CACHE_PROTOTYPE_L2.md.
  HZ11PerCpuRseqCacheReadiness-L1 remains the completed boundary/rollback/gate
  reference for this prototype. See docs/HZ11_PERCPU_RSEQ_CACHE_READINESS_L1.md.
  HZ11PaperPositioning-L1 is the current HZ11 line statement (GO for
  positioning; no default promotion). See
  docs/HZ11_PAPER_POSITIONING_L1.md.
  HZ11Fine128CandidatePositioning-L1 is documented as GO. No default promotion.

Current stance:
  HZ11 remains a speed-first research line.
  libhz11_span_transfer.so remains the remote/mixed microbench speed-lane
  candidate.
  libhz11_span_transfer_thread_exit.so fixes larson thread-churn RSS, but is
  not a macro default.
  libhz11_span_transfer_thread_exit_cap_batch32_fine128.so is the current
  recommended general opt-in macro speed-lane candidate.
  libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap1024_bytes.so
  is a sh6bench/macro-churn specialist opt-in lane: it closes synthetic
  sh6bench to about 1.2x tcmalloc, but regresses remote/mixed, so it does not
  replace fine128 as the general recommendation.
  libhz11_span_transfer_thread_exit_cap_batch32.so remains an intermediate
  attribution/candidate step, not the final lane.
  Global fineclass remains a sh6bench RSS research lane only.
  The default allocator path is unchanged.

Macro candidate blockers / lane split:
  python_alloc and mstress now pass under the thread-exit-cap/fine128 family.
  fine128 is the general opt-in macro recommendation because it keeps the best
  balance across macro rows and remote/mixed. Its sh6bench wall remains much
  slower than tcmalloc, but cap1024-bytes shows that the specific sh6bench
  pathology is cache-capacity driven and can be closed on a specialist lane.
  cap1024-bytes is not the general recommendation because it materially
  regresses remote/mixed versus fine128. span-transfer remains the dedicated
  remote/mixed lane. The next claim-strengthening work should use real
  applications and platform coverage rather than another synthetic-only
  promotion.

Do not do yet:
  claim HZ11 generally beats tcmalloc
  promote thread-exit or span-return lanes to default
  promote no-bytes lanes to default without an RSS-cap replacement
  add checked-mode diagnostics to the default path
  claim HZ11 replaces HZ8 or HZ10
```

## Restart Pointers

```text
README.md
docs/README.md
docs/HZ11_TRANSFER_CACHE_CENTRAL_SPAN_L1.md
docs/HZ11_TRANSFER_PROMOTION_MATRIX_L1.md
docs/HZ11_CURRENT_SPAN_POOL_THREAD_EXIT_L1.md
docs/HZ11_CENTRAL_POLICY_CORRECTNESS_L1.md
docs/HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1.md
docs/HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1.md
docs/HZ11_SH6BENCH_TRANSFER_BATCH_VS_CAP_L1.md
docs/HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1.md
docs/HZ11_SH6BENCH_SPAN_PAGE_FOOTPRINT_WITH_BATCH32_L1.md
docs/HZ11_SH6BENCH_LIVE_FOOTPRINT_OBSERVATION_L1.md
docs/HZ11_SH6BENCH_REQUEST_SIZE_CLASS_PACKING_OBSERVATION_L1.md
docs/HZ11_SH6BENCH_SIZE_CLASS_POLICY_CANDIDATE_L1.md
docs/HZ11_FINECLASS_PYTHON_ALLOC_CURRENT_RSS_L1.md
docs/HZ11_SELECTIVE_FINECLASS_RANGE_L1.md
docs/HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1.md
docs/HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md
docs/HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md
docs/HZ11_FINE128_CANDIDATE_POSITIONING_L1.md
docs/HZ11_NO_GO_LEDGER.md

NO-GO / attribution records:
  docs/no_go/README.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_GATE_L1.md
  docs/no_go/HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md
  docs/no_go/HZ11_SPAN_SOURCE_ATTRIBUTION_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_THREAD_EXIT_CAP_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_BATCH32_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.md
  docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1.md
  docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1.md
  docs/no_go/HZ11_SH6BENCH_SPAN_REUSE_POLICY_L1.md
  docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1.md

Cleanup / baseline docs:
  docs/HZ11_TLS_FAST_PATH_L1.md
  docs/HZ11_CACHE_BYTE_ACCOUNTING_GATE_L1.md
  docs/HZ11_REMAINING_BODY_ATTRIBUTION_L0.md
  docs/HZ11_SYS_RESOLVER_SPLIT_L0.md
  docs/HZ11_TOKEN_HELPERS_SPLIT_L0.md
  docs/HZ11_PUBLIC_ENTRY_HELPERS_L0.md
```

## Archived Detailed Results

```text
Detailed per-box Linux results are archived in the cited box docs and
docs/HZ11_NO_GO_LEDGER.md. Keep this file as the restart surface only.

Most relevant current Linux docs:
  docs/HZ11_FINE128_REAL_APP_POSITIONING_L1.md
  docs/HZ11_REAL_APP_WORKLOAD_EXPANSION_L1.md
  docs/HZ11_ROCKSDB_POST_FIX_LANE_PERF_L1.md
  docs/HZ11_ROCKSDB_READRANDOM_CRASH_ROOT_CAUSE_L1.md
  docs/HZ11_THREAD_CACHE_CAPACITY_MIDDLE_LANE_L1.md

Most relevant Windows docs:
  docs/HZ11_WINDOWS_LANE_STATUS_L1.md
  docs/HZ11_WINDOWS_CLASS_AWARE_REFILL_BATCH_L1.md
  docs/HZ11_WINDOWS_SPAN_CACHE256_L3.md
  docs/no_go/HZ11_WINDOWS_SPAN_TRANSFER_MATRIX_L2.md
```

## Next Step

```text
HZ11 Linux:
  Freeze new tuning by default; use the line for paper/positioning and
  correctness fixes unless a new evidence gap is found.
  fine128 remains the recommended general opt-in lane.
  cap768/cap1024 remain sh6bench-synthetic specialists.
  span-transfer remains remote/mixed-only.

HZ11 Windows:
  selected row remains hz11-span-cache256.
  classbatch16 is candidate-watch / matrix helper only.
  Continue only with app-like confirmation or profile-scoped evidence if needed.

Writing:
  HZ8 paper exists. Natural order is HZ10 ownership/reclaim, then HZ11
  speed/lane-specialization, unless Windows evidence changes priority.
```
