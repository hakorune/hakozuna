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
