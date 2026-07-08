# HZ11CurrentSpanPoolThreadExit-L1

Status: GO for larson RSS attribution/fix; keep opt-in pending broader macro
gate.

This box addresses the larson RSS source found by
`HZ11SpanSourceAttribution-L1`: fresh per-thread current-span carve under thread
churn. It does not promote `libhz11_span_transfer.so` to a macro default.

## Boundary

```text
Opt-in flag:
  HZ11_CURRENT_SPAN_THREAD_EXIT=1

Sibling:
  libhz11_span_transfer_thread_exit.so

Runner:
  scripts/run_hz11_current_span_thread_exit.sh

Output:
  bench_results/hz11_current_span_thread_exit_20260708T000125Z/summary.md
```

The default transfer lane is unchanged. The new sibling adds a pthread-key TLS
destructor only when `HZ11_CURRENT_SPAN_THREAD_EXIT=1`.

## Design

On thread exit:

```text
for each class:
  flush normal thread-cache objects through the existing transfer path
  push only the unallocated suffix of H11SpanCurrent into a per-class span pool
```

On refill:

```text
transfer cache -> central stack -> current-span suffix pool -> reusable span
-> fresh arena carve
```

The pool stores current-span ranges (`base`, `bump_index`, `slot_count`), not
individual objects. This avoids flooding the central stack and keeps the reuse
boundary in one place: immediately before fresh arena carve.

If the pool is full, the suffix is dropped, matching the old behavior for that
unissued part of the span. Allocated objects are never dropped.

## Larson Gate

RUNS=3, `LARSON_SECONDS=2`, `LARSON_THREADS=4`.

| Allocator | Status | Median wall sec | Median max RSS KiB | RSS vs transfer |
|---|---:|---:|---:|---:|
| hz11-span-transfer | OK:3 | 4.170 | 654080 | 1.000 |
| hz11-thread-exit | OK:3 | 4.138 | 273280 | 0.418 |
| tcmalloc | OK:3 | 4.144 | 278912 | 0.426 |

Pool observation from the thread-exit lane:

```text
run1: span_create=24, current_span_pool push=24 pop=5 drop=0
run2: span_create=23, current_span_pool push=22 pop=4 drop=0
run3: span_create=17, current_span_pool push=23 pop=11 drop=0
```

The attribution target moved as intended: larson no longer creates tens of
thousands of fresh current spans, and RSS is slightly below tcmalloc on this
gate.

## Decision

GO for the larson current-span/thread-churn box. Keep it as an opt-in sibling
until a broader macro gate checks the previous failure rows:

```text
python_alloc
mstress
sh6bench
xmalloc_test
cache_scratch
larson
```

This box does not address the sh6bench span-return wall regression; that remains
owned by a separate metadata-lock batching/removal box if span-return is retried.
