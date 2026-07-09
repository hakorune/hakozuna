# HZ11 Docs

HZ11 is a speed-first allocator research line. It starts from design documents
and small boxes rather than a copy of HZ8, HZ9, or HZ10.

```text
HZ11_POSITIONING_L0.md:
  identity, non-goals, relation to HZ3/HZ8/HZ10, tcmalloc-facing architecture,
  and public claims that are allowed or forbidden

HZ11_THREAD_CACHE_FAST_PATH_L0.md:
  first implementation box design: per-thread cache, size-class table,
  pointer-token free fast path, route fallback, counters, gates, and risks

HZ11_WINDOWS_BRINGUP_L0.md:
  Windows bring-up scaffold for the standalone token/front-cache smoke. This is
  not fine128 parity and not a Windows malloc replacement; it establishes the
  minimal portability seam before span/transfer/DLL work.

HZ11_WINDOWS_FIXED_LOCAL_BENCH_L0.md:
  Windows L0 fixed-local speed observation. Compares system CRT, HZ11 token, and
  HZ11 tlsfast public-entry lanes in a standalone microbench.

HZ11_WINDOWS_SPAN_BRINGUP_L1.md:
  Windows span/classify smoke bring-up. Adds VirtualAlloc-backed arena support
  and verifies HZ11_CLASSIFY_SPAN=1 can pass the standalone smoke.

HZ11_WINDOWS_SPAN_CACHE512_PRESSURE_L4.md:
  Windows cap-pressure follow-up for `hz11-span-cache256`. Adds matrix-only
  `cache512` and `cache512-diag` rows. Keep as pressure evidence / HOLD:
  balanced improves, larger_sizes slightly improves, but wide_ws remains
  refill-dominated.

HZ11_WINDOWS_CLASS_AWARE_REFILL_BATCH_L1.md:
  Windows class-aware returned-refill behavior probe. Adds opt-in
  `hz11-span-cache512-classbatch` rows that batch-pop returned objects for
  dominant classes and seed the front cache. Repeat confirmation keeps it as
  Windows profile-specific evidence: matrix rows improve, but random_mixed
  general rows still block selected-row promotion.

HZ11_WINDOWS_LANE_STATUS_L1.md:
  cleanup/orientation record for the Windows HZ11 lanes after cache512,
  classdiag, classbatch, narrow classbatch, and pressure-gate probes. Fixes the
  active status: `hz11-span-cache256` remains selected, `classbatch16` is
  candidate-watch/matrix helper, pressure-gated classbatch is no-go as
  implemented, and diagnostic rows must not be used for speed ranking.

HZ11_WINDOWS_RETURNED_COLD_SKIP_L2.md:
  Windows refill subpath attribution and returned-empty-lock probe. Adds
  diagnostic-only `HZ11_MATRIX_ATTRIB_DIAG` rows and an opt-in
  `classbatch16-coldskip` behavior row. Keep as wide_ws / returned-empty-lock
  evidence; the selected Windows row remains `hz11-span-cache256`.

HZ11_LINUX_RETURNED_REFILL_BATCH_PROBE_L1.md:
  Ubuntu cross-check for the Windows returned-refill classbatch idea. Adds
  Linux non-transfer span siblings (`span-cache256`, `span-cache512-classbatch`)
  and measures them on the transfer matrix. Classbatch is live but loses to
  Linux `span-transfer` on throughput and RSS, so it is cross-platform evidence
  only, not a Linux lane replacement.

HZ11_FRONTEND_ATTRIBUTION_L0.md:
  perf/objdump attribution that identified HZ11's remaining fixed64 gap as
  instruction count rather than efficiency

HZ11_STATS_COMPILE_GATE_L1.md:
  compile-time hot-counter opt-out; kept as a small speed cleanup, but measured
  as too small to be the main tcmalloc-gap lever

HZ11_TLS_FAST_PATH_L1.md:
  public-entry TLS-present fast path A/B; tests whether moving resolver/init
  checks to a slow helper reduces the fixed malloc/free instruction budget

HZ11_CACHE_BYTE_ACCOUNTING_GATE_L1.md:
  opt-in no-bytes sibling lane that removes global cached-byte accounting from
  cache pop/push to price the remaining hot body cost

HZ11_REMAINING_BODY_ATTRIBUTION_L0.md:
  objdump attribution after TLSFastPath and no-bytes lanes; identifies
  class-cache address calculation as the next largest actionable cost

HZ11_TRANSFER_CACHE_CENTRAL_SPAN_L1.md:
  implemented opt-in tcmalloc-shaped middle-end (batch transfer cache + central
  object stack); replaces the per-object returned-list sink; measured as GO for
  remote/mixed rows while keeping fixed-local hit-path instruction count neutral

HZ11_TRANSFER_PROMOTION_MATRIX_L1.md:
  re-runnable promotion gate for `libhz11_span_transfer.so` across main/local,
  main remote, small remote, and medium remote rows; emits p25/p75, RSS, and
  transfer counters, then classifies GO/NO-GO for speed-lane recommendation

HZ11_CURRENT_SPAN_POOL_THREAD_EXIT_L1.md:
  opt-in thread-exit current-span suffix pool for the transfer lane; fixes
  larson thread-churn RSS without changing the default lane

HZ11_CENTRAL_POLICY_CORRECTNESS_L1.md:
  opt-in thread-exit plus central-cap correctness sibling; makes python_alloc
  and mstress pass the focused gate without changing default lanes

HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1.md:
  sh6bench attribution for the thread-exit-cap candidate; identifies
  transfer/central path volume and span footprint as the active blockers

HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1.md:
  transfer-cache cap/batch xferwide probe for sh6bench; eliminates central
  spill and improves wall about 20%, but remains far slower than tcmalloc and
  must be split into batch-vs-cap before promotion decisions

HZ11_SH6BENCH_TRANSFER_BATCH_VS_CAP_L1.md:
  splits the xferwide sh6bench wall improvement into transfer batch vs transfer
  capacity; shows batch width is the wall lever, not cap/spill elimination

HZ11_SH6BENCH_TRANSFER_BATCH_GRANULARITY_L1.md:
  compares transfer batch widths 16/32/64/128 for sh6bench; selects batch32 as
  the smallest useful wall candidate and rejects wider batches as regressions

HZ11_SH6BENCH_SPAN_PAGE_FOOTPRINT_WITH_BATCH32_L1.md:
  source/page attribution with batch32 in place; shows sh6bench RSS is dominated
  by fresh arena-carved spans and missing span reuse, not central retention

HZ11_SH6BENCH_LIVE_FOOTPRINT_OBSERVATION_L1.md:
  diagnostic-only live class footprint observation under batch32; shows active
  spans are almost full after HZ11 class rounding, shifting the remaining RSS
  question toward requested-size distribution, class packing, and residency

HZ11_SH6BENCH_REQUEST_SIZE_CLASS_PACKING_OBSERVATION_L1.md:
  diagnostic-only requested-size versus HZ11 slot-size observation under
  batch32; attributes the sh6bench RSS gap primarily to power-of-two
  class-packing waste

HZ11_SH6BENCH_SIZE_CLASS_POLICY_CANDIDATE_L1.md:
  opt-in fine size-class policy candidate for sh6bench after class-packing
  attribution; keeps batch32 transfer policy fixed and requires focused
  correctness before any macro gate

HZ11_FINECLASS_PYTHON_ALLOC_CURRENT_RSS_L1.md:
  diagnostic-only attribution for the fineclass python_alloc current-RSS macro
  gate miss; treats the small miss as a sampling artifact on a tiny denominator
  rather than a steady fineclass resident-footprint regression

HZ11_SELECTIVE_FINECLASS_RANGE_L1.md:
  opt-in selective fineclass range box; adds fine128/fine256 siblings, selects
  fine128 as the next candidate because it brings focused sh6bench RSS inside
  the guard while avoiding the worst global-fineclass remote/mixed RSS expansion

HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1.md:
  diagnostic-only macro RSS gate semantics box; defines max RSS as the primary
  hard guard and requires focused RUNS>=10 stability evidence before current
  RSS can hard-fail tiny or timing-sensitive rows

HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md:
  reclassifies the prior fine128 macro gate under the documented current-RSS
  semantics rule; marks fine128 GO as the next opt-in macro candidate, with
  final remote/mixed confirmation still required before stronger promotion

HZ11_FINE128_REMOTE_MIXED_FINAL_CONFIRM_L1.md:
  RUNS=10 remote/mixed confirmation for fine128; confirms fine128 as the
  recommended opt-in macro speed-lane candidate while keeping span-transfer as
  the cleaner remote/mixed-only lane

HZ11_FINE128_CANDIDATE_POSITIONING_L1.md:
  final lane-positioning record after fine128 confirmation; documents fine128
  as the recommended opt-in macro candidate, span-transfer as the remote/mixed
  microbench lane, global fineclass as research-only, and default unchanged

HZ11_PAPER_POSITIONING_L1.md:
  current HZ11 line statement; converts the fine128 evidence chain into a
  publishable narrative (near-parity-or-better wall on 5/6 macro rows, major RSS
  wins, remote/mixed microbench dominance, one open sh6bench pathology), states
  the claim boundary, and defines the exact gate per-CPU/rseq must earn before
  it is adopted; no policy change, no default promotion

HZ11_PERCPU_RSEQ_CACHE_READINESS_L1.md:
  readiness/design box for a per-CPU/rseq front cache on the fine128 base; fixes
  the L2 prototype boundary (where it sits, minimal sibling under
  -DHZ11_PERCPU_RSEQ=1, glibc-symbols-preferred-but-verify fallback, LD_PRELOAD-only
  rollback) and the 6-check adoption gate (remote/mixed regression baseline = fine128;
  span-transfer stays the dedicated remote/mixed lane); GO to design only, not to
  implement or adopt, not a claim that rseq fixes sh6bench

HZ11_PERCPU_RSEQ_CACHE_PROTOTYPE_L2.md:
  locked per-CPU cache prototype (rseq selects the CPU only; per-CPU spinlock;
  no hand-rolled rseq CS). NO-GO for the CPU-locality hypothesis: the locked lane
  regressed sh6bench 3.66x vs fine128 (lock overhead dominated, though the slab
  did cut transfer/central traffic). Correctness held. Keep fine128; do NOT pursue
  the lock-free rseq CS. Proves a locked per-CPU layer is a net loss; does NOT
  prove lock-free locality would lose (measurement confounded by lock cost)

HZ11_PAPER_EVIDENCE_PACKAGE_L1.md:
  paper-ready consolidation of the HZ11/fine128 evidence: final lane taxonomy,
  the macro and remote/mixed evidence tables, the 7-item negative-result ladder
  plus the sh6bench open-gap spine, the claim boundary, and the next-work warrant.
  No new measurements; every number cites its source doc. Companion to
  HZ11PaperPositioning-L1 (which states the line; this carries the evidence)

HZ11_THREAD_CACHE_CAPACITY_TUNING_L1.md:
  tests whether sh6bench wall is dominated by HZ11_CACHE_CAP=32 thread-cache overflow
  into the transfer cache. GO for the cache-cap root cause: CAP 32->256 drops sh6bench
  wall 3.5s->1.9s (-45%, tcmalloc gap 9.8x->5.3x) with xfer_insert -40%, monotonic,
  other 5 macro rows flat. MIXED for promotion: plain CAP256 regresses xmalloc_test RSS
  2.8x (cached-bytes under HZ11_CACHE_BYTE_ACCOUNTING=0). Clean win needs CAP + byte-cap
  policy. Reuses the sh6bench path-cost runner via a new ALLOC_LIST env (default preserved)

HZ11_THREAD_CACHE_CAPACITY_BYTE_CAP_L1.md:
  big CAP + HZ11_CACHE_BYTE_ACCOUNTING=1 on the fine128/SOA base. GO: cap1024-bytes
  (CAP=1024, 2 MiB byte cap) closes sh6bench to 1.20x tcmalloc (3.5s->0.43s,
  xfer_insert 868M->173K) with xmalloc_test RSS bounded (27648 vs plain CAP256 52864)
  and no regression on the other 5 macro rows. Made byte accounting functional on the
  SOA push/pop (was a #error/no-op) and fixed a cached_bytes consistency bug in the SOA
  slow paths (flush_class/overflow_slow) that timed out cap256/512-bytes before the fix.
  Not promoted (needs a positioning box)

HZ11_CAP1024_BYTES_CANDIDATE_POSITIONING_L1.md:
  positioning box: decide whether cap1024-bytes replaces fine128 as the recommended
  opt-in macro candidate. MIXED: cap1024-bytes fixes sh6bench (1.21x tcmalloc, synthetic
  macro; other macro rows flat) but materially regresses the remote/mixed microbench vs
  fine128 (medium rows 4.3x->1.5x, 5.9x->1.6x tcmalloc). So cap1024-bytes is a
  sh6bench/macro-churn SPECIALIST opt-in lane; fine128 remains the recommended general
  opt-in macro candidate; span-transfer stays the remote/mixed lane (3-way split).
  Rollback = LD_PRELOAD fine128 instead. No default change, no claim broadening

HZ11_THREAD_CACHE_CAPACITY_MIDDLE_LANE_L1.md:
  searches for a middle CAP (512/768) or tighter byte cap (1 MiB) that improves sh6bench
  without breaking remote/mixed -- a true general candidate. NO-GO: no uniform CAP
  512-1024 (+byte) keeps remote/mixed ~= fine128. sh6bench win saturates at CAP768
  (1.24x), but even cap512-bytes collapses fine128's medium-row dominance (4.5x/5.8x ->
  ~1.5x tcmalloc); the big-CAP vs remote/mixed-medium tension is fundamental. Lane split
  confirmed: fine128 generalist, cap1024/cap768-bytes sh6bench specialist. Next lever:
  class-range CAP (code change, follow-up). Adds cap768-bytes + cap1024-bytes1m siblings

HZ11_LANE_FULL_EVIDENCE_GATE_L1.md:
  full-evidence gate: 5 lanes across synthetic (macro RUNS=5 + remote/mixed RUNS=10) AND
  real-app (espresso SPEC app + sqlite3 in-memory DB, RUNS=3). GO for evidence with a
  runner RSS correction: the initial runner sampled the bash wrapper PID, so it now samples
  process-tree RSS. Corrected sqlite3 keeps the wall story (fine128/cap768/cap1024
  near-parity, cap lanes indistinguishable from fine128, span-transfer aborts) but the RSS
  win is modest (~0.84x tcmalloc), not the earlier ~0.4x. Espresso RSS needs rerun before a
  strong multiplier claim. Next: fine128-centered paper; multi-threaded real-app
  (rocksdb/redis) is the remaining gap. New runner run_hz11_real_app_gate.sh +
  bench/hz11_real_app_sqlite.sql

HZ11_REAL_APP_EVIDENCE_RERUN_L1.md:
  claim-grade real-app rerun with the fixed process-tree RSS runner (RUNS=10) + a
  multi-threaded real DB (rocksdb). BOUNDS the claim: espresso is a genuine HZ11 win
  (near-parity wall + ~3x less RSS); sqlite3 is mixed (7-12% slower + ~0.83x RSS, not the
  earlier buggy near-parity+0.4x); and EVERY HZ11 lane SEGFAULTS on rocksdb's
  multi-threaded readrandom (tcmalloc/jemalloc clean) -- a fundamental correctness gap.
  P0 next: fix the rocksdb crash before any real-app/multi-thread claim. cap1024-on-real-
  multi-thread is blocked by the crash. compile workload deferred (no suitable target)

HZ11_ROCKSDB_READRANDOM_CRASH_ROOT_CAUSE_L1.md:
  FIX-GO. Root cause of the rocksdb readrandom segfault: hz11_malloc_usable_size routed
  arena pointers to libc, which read arena slot data as a chunk header and faulted
  (minimal repro: p=malloc(N); malloc_usable_size(p); under LD_PRELOAD -> SIGSEGV; gdb bt
  in libc __malloc_usable_size). Refuted: API-coverage, foreign-pointer classify, realloc.
  Fixed with an arena-aware hz11_malloc_usable_size (NULL->0; arena ptr -> slot size;
  non-arena -> libc). rocksdb readrandom now rc=0 at ~tcmalloc parity; espresso/sqlite3 no
  regression. Real-app multi-thread correctness gap CLOSED. Perf eval is the next box

HZ11_ROCKSDB_POST_FIX_LANE_PERF_L1.md:
  post-fix rocksdb lane perf (fillrandom+readrandom, 8 threads, RUNS=3). GO for fine128 on
  real multi-thread DB: near-parity with tcmalloc (wall 1.009x, readrandom 1.01x, RSS
  0.956x). cap-specialist NO-GO: cap768/cap1024 ~= fine128 or slower with higher RSS; the
  sh6bench win does NOT translate to rocksdb (cap cuts transfer traffic 4.6M->25K but
  rocksdb's bottleneck is I/O, not the allocator). cap lanes confirmed sh6bench-synthetic
  specialists. Closes the 'does cap help real multi-thread' question

HZ11_REAL_APP_WORKLOAD_EXPANSION_L1.md:
  expands real-app evidence to 3 new workload families: git clone+repack (near-parity wall,
  RSS higher due to arena overhead on large buffers), redis-6.2.7 source build (fine128
  7% FASTER than tcmalloc + 20% less RSS -- a real compile win), redis-benchmark (real
  server, ~3% slower + 34% less process-tree RSS). All rc=0 (malloc_usable_size fix covers
  redis server-style). cap lanes ≈ fine128 on all (NO-GO for real apps). Total real-app
  evidence: 6 workloads across 4 families. Redis RSS is process-tree total (server+client)

HZ11_FINE128_REAL_APP_POSITIONING_L1.md:
  current HZ11 positioning after the capacity, real-app, malloc_usable_size, and rocksdb
  post-fix boxes. GO for updated positioning: fine128 is the recommended general opt-in
  HZ11 lane with real-app evidence (espresso win, sqlite3 mixed, rocksdb near-parity);
  cap768/cap1024 are sh6bench-synthetic specialists only; span-transfer remains
  remote/mixed-only. Supersedes the older paper positioning where sh6bench was one open
  gap and per-CPU/rseq was the leading next hypothesis

docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.md:
  full macro gate for the batch32 fineclass candidate; keeps the useful
  sh6bench RSS reduction but does not promote because python_alloc current RSS
  misses the macro threshold

docs/no_go/HZ11_FINECLASS_REMOTE_MIXED_TRADEOFF_L1.md:
  remote/mixed transfer-matrix tradeoff for the global fineclass candidate;
  keeps fineclass as a sh6bench RSS research lane only because remote/mixed
  throughput/RSS/span-create tradeoffs are material

docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1.md:
  full macro gate for the selective fine128 candidate; correctness, larson,
  xmalloc/cache_scratch, and sh6bench max RSS pass, but current RSS fails on
  python_alloc and sh6bench under the old single-sample hard-fail rule; this
  is superseded by HZ11_MACRO_SPEED_LANE_FINE128_RECLASSIFY_L1.md

docs/no_go/HZ11_SH6BENCH_ARENA_COMMIT_POLICY_L1.md:
  `MADV_NOHUGEPAGE` arena policy probe under batch32; rules out simple THP or
  whole-arena eager commit as the sh6bench RSS/page-footprint lever

HZ11_SYS_RESOLVER_SPLIT_L0.md:
  cleanup box that moves dlsym/bootstrap/system allocator wrappers out of
  hz11_thread_cache.c

HZ11_TOKEN_HELPERS_SPLIT_L0.md:
  cleanup box that moves token-table types and inline helpers out of
  hz11_thread_cache.h

HZ11_PUBLIC_ENTRY_HELPERS_L0.md:
  cleanup box that consolidates duplicated malloc/free with-thread-cache bodies
  in hz11_public_entry.c

HZ11_NO_GO_LEDGER.md:
  short index of decisions that should not be retried without new evidence

no_go/README.md:
  archive index for detailed HZ11 NO-GO, failed-promotion, and attribution-only
  documents, including macro gates, span-source attribution, thread-exit failed
  promotion records, and sh6bench span-reuse failures
```
