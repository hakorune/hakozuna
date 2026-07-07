# HZ11 Central FreeList Span Return L1

Status: implemented as an opt-in sibling lane (`libhz11_span_return.so`) and
measured.
Verdict: NO-GO for macro promotion.

## Purpose

Move the HZ11 transfer lane's central spill target from a retained-object array
to a span-aware central free list, then measure whether fully free spans can be
reused to control macro RSS.

This box exists because `HZ11MacroFailureAttribution-L1` showed:

```text
python_alloc:
  central class 2 overflow; cap 65536 passes 3/3

mstress:
  central class 0 overflow; cap 65536 passes 3/3

larson:
  cap 65536 still uses 2.345x tcmalloc max RSS

sh6bench:
  cap 65536 still uses 1.322x tcmalloc max RSS and remains much slower
```

Increasing a fixed central object stack is useful for attribution, but it is not
a macro memory policy.

## Boundary

```text
Scope:
  hakozuna-hz11/ only

New opt-in flag:
  HZ11_CENTRAL_SPAN_RETURN=1

New sibling:
  libhz11_span_return.so
  preload-span-return

Do not change:
  libhz11_span_transfer.so
  default allocator
  fixed-local malloc/free hit path
```

The new lane is allowed to touch cold refill/overflow/central code. It must not
add span-return bookkeeping to the hot malloc/free cache-hit path.

## Current Problem

The transfer lane currently has this middle-end:

```text
thread cache
  -> transfer cache
  -> central object stack
  -> per-thread current span / fresh span carve
```

The central object stack is bounded and fail-fast. It can reuse spilled objects,
but it has no span ownership model. Once the allocator has created many spans,
it has no way to identify a fully free span and put it back into a reusable span
pool. Macro workloads that churn through span-sized waves therefore retain too
much central memory.

## Target Shape

```text
thread cache
  -> transfer cache
  -> central free list
       tracks central object membership by span
       detects central-only full spans
       moves full spans to reusable span stack
  -> reusable span stack
  -> arena bump carve
```

L1 does not return pages to the OS. It returns full spans to an internal reusable
span stack. `madvise` or pageheap-style release is deferred.

## Span Metadata

Add span metadata only for the `HZ11_CENTRAL_SPAN_RETURN=1` sibling:

```c
typedef enum H11SpanReturnState {
  HZ11_SPAN_ACTIVE = 0,
  HZ11_SPAN_CENTRAL_PARTIAL = 1,
  HZ11_SPAN_CENTRAL_FULL_REUSABLE = 2
} H11SpanReturnState;

typedef struct H11SpanReturnMeta {
  uint8_t class_id;
  uint32_t slot_count;
  uint32_t central_free_count;
  uint32_t transfer_count;
  uint32_t state;
} H11SpanReturnMeta;
```

The existing `hz11_span_class[]` remains the direct classify table used by the
free path. It is not replaced. The new metadata is cold-path policy state.

### Span Id

```text
span_id = ((uintptr_t)ptr - (uintptr_t)hz11_arena_base) >> HZ11_SPAN_SHIFT
slot_index = ((uintptr_t)ptr - span_base) / hz11_class_slot_size(class_id)
```

L1 can avoid a full per-span bitmap if the central free list never has duplicate
valid frees. HZ11 speed mode already assumes valid C programs, so
`central_free_count` is sufficient for this box. A checked-mode bitmap can be a
later correctness diagnostic if needed.

## Transfer Occupancy

The return condition must exclude objects still held in the transfer cache.
Therefore the span-return sibling must account transfer cache membership:

```text
transfer_insert_range:
  for every inserted object:
    span_meta[span_id].transfer_count++

transfer_remove_range:
  for every removed object:
    span_meta[span_id].transfer_count--
```

This is cold-path transfer-cache work. It must be compiled only into
`HZ11_CENTRAL_SPAN_RETURN=1`.

## Central Insert

On central insert:

```text
1. classify ptr to span_id and class_id
2. append ptr to the per-class central free list
3. span_meta[span_id].central_free_count++
4. state becomes central_partial unless the full-span condition is met
5. if full-span condition is met, remove that span's objects from the central
   free list and push the span to the reusable span stack
```

Full-span condition for L1:

```text
central_free_count == slot_count
transfer_count == 0
span is not the current span of the inserting thread
```

The current-span exclusion avoids returning a span that the same thread could
still bump from. It is conservative and local. If this is not sufficient, L1
should narrow the return policy further to spans whose bump cursor is known
complete.

## Central Remove

On central remove:

```text
1. pop object from the per-class central free list
2. span_meta[span_id].central_free_count--
3. if the span had central_full_reusable state, this is a bug: reusable spans
   must not also remain on object lists
4. state becomes active or central_partial depending on count
```

Fail fast on impossible state. Do not silently route around metadata mistakes.

## Reusable Span Stack

Per class:

```c
typedef struct H11ReusableSpanStack {
  pthread_mutex_t lock;
  uint32_t span_ids[HZ11_REUSABLE_SPAN_CAP];
  uint32_t count;
} H11ReusableSpanStack;
```

When refill needs a fresh span:

```text
1. pop reusable span for class
2. if found:
     reset span_meta central/transfer counts to 0
     mark active
     use that span as the refill source
3. otherwise:
     hz11_span_carve_for_class(class_id)
```

Counters:

```text
span_return_count
span_reuse_count
central_full_span_count
central_partial_span_count
central_objects
span_return_by_class[class]
high_water_by_class[class]
```

## Removing A Full Span From Central

L1 has two acceptable implementations:

```text
Option A:
  central free list is per-class intrusive list, and full-span return filters
  the class list to remove every object from that span.

Option B:
  central free list stores objects per span, and per-class pop chooses from
  partial spans.
```

Option A is simpler to land but can be O(n) on central insert when a span becomes
full. That is acceptable for L1 measurement because it is a cold path and keeps
the boundary small. Option B is the better long-term shape if O(n) filtering is
material in macro rows.

## Measurement Script

Add a dedicated gate after implementation:

```text
scripts/run_hz11_span_return_gate.sh
```

Default conditions:

```text
RUNS=3 or RUNS=5

Rows:
  python_alloc
  mstress
  larson
  sh6bench
  xmalloc_test

Allocators:
  tcmalloc
  hz11-span-transfer
  hz11-span-return
```

Outputs:

```text
bench_results/hz11_span_return_<timestamp>/
  README.log
  samples.csv
  summary.md
  per-run logs
```

The macro script must capture:

```text
wall_sec
max_rss_kb
current_rss_kb from /proc/<pid>/status before exit where possible
exit code
HZ11_DUMP_STATS
span-return counters
```

Do not use `ru_maxrss` as post/current RSS.

## GO

```text
python_alloc and mstress pass without requiring a huge fixed central cap
larson RSS improves materially vs hz11-span-transfer
sh6bench RSS improves or is no worse than hz11-span-transfer
xmalloc_test wall time does not regress more than 5%
fixed-local hit path is unchanged
span_return_count or span_reuse_count proves the policy is active on rows where
  span return is expected
```

## NO-GO

```text
correctness crash
RSS unchanged on larson and sh6bench
span-return bookkeeping materially hurts remote/mixed transfer rows
policy requires touching span metadata on hot malloc/free cache hits
false span return: object still present in transfer/thread cache after span reuse
```

## Implementation Order

```text
1. add HZ11_CENTRAL_SPAN_RETURN flag and libhz11_span_return.so sibling
2. add span-return metadata and counters behind the new flag
3. account transfer_count in transfer insert/remove behind the new flag
4. account central_free_count in central insert/remove behind the new flag
5. implement conservative central-only full-span return to reusable stack
6. make refill check reusable span stack before arena bump carve
7. add focused smoke for full-span return/reuse correctness
8. add macro gate script and measure
9. update GO/NO-GO ledger and current_task
```

## Implementation Notes

The implemented L1 stayed behind:

```text
HZ11_CENTRAL_SPAN_RETURN=1
libhz11_span_return.so
preload-span-return
```

`libhz11_span_transfer.so` remains unchanged.

The span-return lane uses the same transfer cache shape, adds cold-path span
metadata, and uses a reusable span stack. Full-span return is conservative:

```text
transfer_count excludes objects currently in the transfer cache
central_free_count counts objects seen in central
full-span candidates are moved to reusable stack
stale reusable candidates are skipped if later central activity invalidates them
overflow performs a central sweep before fail-fast
```

The lane currently compiles with `HZ11_CENTRAL_CAP=65536` to keep the attribution
rows running while measuring whether span return fixes macro RSS. That means it
does not satisfy the desired "no huge fixed cap" policy goal.

## Result

Run:

```text
RUNS=3 ./hakozuna-hz11/scripts/run_hz11_span_return_gate.sh
```

Output:

```text
bench_results/hz11_span_return_20260707T225952Z/summary.md
```

Summary:

| Workload | tcmalloc wall | transfer wall | span-return wall | span-return/tcmalloc max RSS | span-return/transfer max RSS | span return counters |
|---|---:|---:|---:|---:|---:|---:|
| python_alloc | 0.952s | FAIL | 0.915s | 1.140 | NA | return=1578 reuse=0 |
| mstress | 0.189s | FAIL | 0.240s | 1.226 | NA | return=873 reuse=639 |
| larson | 4.139s | 4.162s | 4.170s | 2.348 | 1.002 | return=0 reuse=0 |
| sh6bench | 0.359s | 4.562s | 17.805s | 1.314 | 1.001 | return=0 reuse=0 |
| xmalloc_test | 2.059s | 2.015s | 2.035s | 0.083 | 0.984 | return=0 reuse=0 |

Gate:

```text
python_alloc/mstress correctness:
  PASS. span-return runs 3/3 on both rows where span-transfer aborts.

larson RSS:
  FAIL. span-return max RSS is 2.348x tcmalloc and materially unchanged vs
  span-transfer.

sh6bench RSS:
  FAIL. max RSS is slightly lower than the prior macro gate but still 1.314x
  tcmalloc and essentially unchanged vs span-transfer in this A/B.

sh6bench wall:
  FAIL. span-return is 17.805s vs span-transfer 4.562s.

xmalloc_test regression:
  PASS. span-return is 1.010x span-transfer wall and slightly lower RSS.

fixed-local hit path:
  PASS in a light fixed64 guard. ITERS=5M/RUNS=3 gave span-transfer median
  154.39M ops/s and span-return median 167.67M ops/s. This is not a promotion
  result; it only confirms the new metadata is not on the measured hit path.
```

Interpretation:

```text
NO-GO for macro promotion.

Span-return bookkeeping can avoid the python_alloc/mstress central overflow
abort, and counters prove the policy is active on those rows. It does not solve
the macro RSS problem that blocked promotion. Larson does not use the central
return path, creates the full 65536-span arena, and keeps the same RSS. Sh6bench
does not return/reuse spans and pays a large bookkeeping cost.
```

## Expected Outcomes

```text
GO:
  span-return lane becomes the macro candidate; transfer remains remote/mixed
  microbench lane; next box can tune transfer batch/cap or page release.

MIXED:
  abort rows pass and some RSS improves, but larson/sh6bench remain weak.
  Next box should attribute retained spans vs transfer cache occupancy.

NO-GO:
  span-return bookkeeping adds complexity without RSS improvement.
  Keep transfer as remote/mixed microbench lane and do not promote macro claims.
```

Measured outcome: NO-GO.

## Next Box

```text
HZ11SpanSourceAttribution-L1:
  larson reaches span_create=65536 with no central return activity, so the next
  question is span source/current-span/pageheap policy rather than central
  stack capacity. Sh6bench needs attribution for span-return bookkeeping cost
  before any retry.
```
