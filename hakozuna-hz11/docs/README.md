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

HZ11_NO_GO_LEDGER.md:
  decisions that should not be retried in the HZ11 hot path without new
  evidence
```
