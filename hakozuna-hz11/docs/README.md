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
  transfer-cache cap/batch sibling for sh6bench; eliminates central spill and
  improves wall about 20%, but remains far slower than tcmalloc and does not
  improve RSS

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
  documents, including macro gates, span-source attribution, and thread-exit
  failed promotion records
```
