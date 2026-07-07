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

HZ11_SPAN_BACKED_CLASSIFY_L1.md:
  span-backed direct-index classify A/B; measured as NO-GO for the tcmalloc
  gate and showed classification alone is not the main lever

HZ11_FRONTEND_ATTRIBUTION_L0.md:
  perf/objdump attribution that identified HZ11's remaining fixed64 gap as
  instruction count rather than efficiency

HZ11_STATS_COMPILE_GATE_L1.md:
  compile-time hot-counter opt-out; kept as a small speed cleanup, but measured
  as too small to be the main tcmalloc-gap lever

HZ11_CACHE_SHAPE_L1.md:
  pointer-top cache A/B; measured as NO-GO because instruction count increased
  despite modest throughput noise on one lane

HZ11_TLS_FAST_PATH_L1.md:
  public-entry TLS-present fast path A/B; tests whether moving resolver/init
  checks to a slow helper reduces the fixed malloc/free instruction budget

HZ11_CACHE_BYTE_ACCOUNTING_GATE_L1.md:
  opt-in no-bytes sibling lane that removes global cached-byte accounting from
  cache pop/push to price the remaining hot body cost

HZ11_REMAINING_BODY_ATTRIBUTION_L0.md:
  objdump attribution after TLSFastPath and no-bytes lanes; identifies
  class-cache address calculation as the next largest actionable cost

HZ11_CACHE_LAYOUT_L1.md:
  SOA class-cache layout sibling; measured as a small win but NO-GO for the
  instruction-count gate

HZ11_SIZE_TABLE_STATIC_INIT_L1.md:
  NO-GO record for removing the size-class table lazy-init guard; loader-time
  malloc before constructors makes the guard load-bearing

HZ11_STATIC_CONST_SIZE_TABLE_L1.md:
  NO-GO record for replacing the runtime-filled size-class table with a const
  .rodata table; correct but slower on the speed-ceiling lane

HZ11_TRANSFER_CACHE_CENTRAL_SPAN_L1.md:
  implemented opt-in tcmalloc-shaped middle-end (batch transfer cache + central
  object stack); replaces the per-object returned-list sink; measured as GO for
  remote/mixed rows while keeping fixed-local hit-path instruction count neutral

HZ11_TRANSFER_PROMOTION_MATRIX_L1.md:
  re-runnable promotion gate for `libhz11_span_transfer.so` across main/local,
  main remote, small remote, and medium remote rows; emits p25/p75, RSS, and
  transfer counters, then classifies GO/NO-GO for speed-lane recommendation

HZ11_MACRO_SPEED_LANE_GATE_L1.md:
  macro workload gate for the promoted HZ11 transfer lane; compares glibc,
  tcmalloc, mimalloc when available, span-soa, and span-transfer across Python,
  Larson, xmalloc, sh6bench, and local optional rows without hot-path changes

HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md:
  diagnostic sibling and cap A/B for macro failures; attributes python_alloc
  and mstress aborts to central stack class pressure, then records that
  cap-only does not solve larson/sh6bench RSS

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
  decisions that should not be retried in the HZ11 hot path without new
  evidence
```
