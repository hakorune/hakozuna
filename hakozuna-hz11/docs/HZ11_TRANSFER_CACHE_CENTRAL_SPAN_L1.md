# HZ11 Transfer Cache + Central Span L1

Status: implemented as an opt-in sibling lane (`libhz11_span_transfer.so`).
Verdict: GO for the remote/mixed target rows; fixed-local guard is neutral.

## Why this box

Fixed-local tcmalloc追撃は構造床に到達した（8箱連続で小粒の限界を確認：counter-gate
+4, SOA +3, TLS-fastpath, byte-accounting-gate, refill-tailcall 0, static-init NO-GO,
const-table NO-GO）。残る差はすべて「意味的に必要な仕事」。

次の本丸は **remote/mixed 行の tcmalloc 追撃**で、それは TransferCache + CentralSpan
がなければ始まらない。HZ11 の現状のミス/オーバーフロー経路は per-object
returned-list（global mutex + singly-linked stack）で、thread churn では
**オブジェクトごとに mutex ロック**を取る。tcmalloc は batch transfer cache で
**1ロックあたり16〜32オブジェクト**を移動する。この差が remote 行の性能差になる。

## tcmalloc reference (from documentation; no local source on this machine)

```text
Thread Cache (per-thread, lock-free pop/push, CAP ~256-1024 per class)
    |  batch remove/insert (batch = 16-32)
    v
Transfer Cache (per-class, mutex-protected, void* array, CAP ~1024)
    |  batch remove/insert
    v
Central Free List (per-class, mutex-protected, manages spans)
    |  span allocate/free
    v
PageHeap (back-end, hugepage-aware)
```

Key properties:
- Transfer cache is a **batch** intermediary: one mutex lock moves 16-32 objects.
- Central free list manages **spans** (multi-page runs carved into objects).
- Thread cache <-> transfer cache uses **batch** remove/insert, not per-object.
- Transfer cache size adapts to demand (grows/shrinks based on hit rate).
- A span "completes" when all its objects are returned -> span goes back to PageHeap.

## HZ11 current shape vs tcmalloc shape

```text
HZ11 current (span lane):
  thread cache (CAP=32, per-class)
      |  miss: hz11_returned_pop (1 object, global mutex lock)
      |  miss: per-thread span bump (1 object, no lock)
      |  miss: hz11_span_carve_for_class (64KiB span, atomic CAS)
      v
  overflow: flush ALL 32 objects one-by-one via hz11_returned_push
            (32 mutex locks for a full-class flush!)

tcmalloc shape:
  thread cache (CAP ~256-1024, per-class)
      |  miss: transfer_remove_range(batch=32)   (1 mutex lock for 32 objects)
      |  miss: central_pop_batch(span carve + batch extract)  (1 mutex lock)
      v
  overflow: transfer_insert_range(batch=16)   (1 mutex lock for 16 objects)
```

HZ11's per-object returned-list costs **1 mutex lock per object** on both miss and
overflow. Under thread churn (larson, main_r90 with 16 threads x 90% remote frees),
the returned-list is the contention bottleneck: 16 threads each doing per-object
push/pop under a single global per-class mutex.

A batch transfer cache reduces lock acquisitions by ~batch_size (16-32x).

## L1 design: what to build

### New component: `src/hz11_transfer_cache.{c,h}`

```c
#define HZ11_TRANSFER_CAP    1024u  /* per class */
#define HZ11_TRANSFER_BATCH  16u    /* objects per batch move */

typedef struct H11TransferCache {
  pthread_mutex_t lock;
  void* slots[HZ11_TRANSFER_CAP];
  uint32_t count;
} H11TransferCache;

/* Batch remove: pop up to max_n objects from the transfer cache.
 * Returns the number actually popped (0 = empty). */
uint32_t hz11_transfer_remove_range(uint8_t class_id, void** out, uint32_t max_n);

/* Batch insert: push up to n objects into the transfer cache.
 * Returns the number actually inserted. The caller must send any excess to
 * hz11_central_stack_insert_range(); arena pointers are never sys_free'd. */
uint32_t hz11_transfer_insert_range(uint8_t class_id, void** items, uint32_t n);
```

### Central source: per-thread current span

The measured lane keeps the live source of truth in each thread's
`H11SpanCurrent`. Refill fills a temporary batch by bumping the current span
until it is exhausted, then carves the next 64KiB span with
`hz11_span_carve_for_class()`.

Do not add a naive `carve_batch()` API that carves a fresh span and returns only
the requested batch: that loses the unissued tail of the 64KiB span and creates
hidden RSS churn.

### Central spill/reuse: per-class object stack

```c
/* When the transfer cache is full, excess arena objects go here.
 * L1: simple mutex + array (no span reclaim). L2: proper span metadata.
 * IMPORTANT: refill must read this stack before carving a fresh span. */
typedef struct H11CentralStack {
  pthread_mutex_t lock;
  void** slots;    /* dynamically grown or fixed large array */
  uint32_t count;
  uint32_t cap;
} H11CentralStack;
```

This is not a full tcmalloc `CentralFreeList`. It is a bounded L1 spill/reuse
stack so transfer overflow does not turn into fresh span churn. Use names like
`hz11_central_stack_*` in code rather than implying full span ownership/reclaim.
The central stack is intentionally fail-fast on capacity overflow: L1 has no span
reclaim, so silently dropping arena objects would hide a policy bug.

### Refill path (transfer lane)

```text
hz11_thread_cache_refill (transfer lane):
  1. transfer_remove_range(class, tmp[16], 16) -> n
  2. if n > 0:
       return tmp[0]
       push tmp[1..n-1] into thread cache
  3. if n == 0:
       hz11_central_stack_remove_range(class, tmp[16], 16) -> n
  4. if n == 0:
       bump tmp[16] from H11SpanCurrent, carving a new 64KiB span only when
       the current span is exhausted
  5. if n > 0:
       return tmp[0]
       push tmp[1..n-1] into thread cache
```

One mutex lock for up to 16 objects (vs 1 lock per object in the current
returned-list). The central stack is deliberately checked before fresh span
carving so transfer overflow is later reused instead of accumulating as hidden
RSS.

### Overflow path (transfer lane)

```text
hz11_thread_cache_push_overflow_slow (transfer lane):
  1. collect ~16 objects from thread cache + incoming ptr
  2. transfer_insert_range(class, batch[16], 16) -> n_inserted
  3. if n_inserted < 16: excess -> hz11_central_stack_insert_range
  4. leave 1 object in thread cache (incoming ptr)
```

One mutex lock for ~16 objects (vs 32 locks for a full-class flush).

### Build flag + sibling

```text
HZ11_TRANSFER_CENTRAL_SPAN=1

libhz11_span_transfer.so:
  -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0
  -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1
```

All existing lanes (token, span, soa, nobytes, tlsfast) are byte-identical.

## L1 DEFER (not in this box)

```text
- Full CentralFreeList with span return/reclaim:
    objects returning to their span's free list, span completeness detection,
    span return to arena bump cursor (or madvise). L1 uses a central object
    stack (mutex + array) as the spill target.

- Adaptive transfer cache sizing:
    start with fixed CAP=1024. tcmalloc grows/shrinks based on demand.

- Per-CPU (rseq) instead of per-thread TLS:
    tcmalloc's modern design is per-CPU; HZ11 stays per-thread for L1.

- Span metadata on the hot free path:
    the classify hot path reads ONLY hz11_span_class[]. A future L2 could add
    per-span free_count/free_head for proper span reclaim, but that is NOT
    touched by the free hot path in L1.
```

## Span metadata design notes (for a future L2)

```text
hz11_span_meta[65536]:
  uint8_t  class_id        (already in hz11_span_class[])
  uint16_t slot_count      (HZ11_SPAN_BYTES / slot_size)
  uint16_t free_count      (incremented on central return)
  uint16_t free_head_index (intrusive index within the span, or INVALID)

The classify hot path reads ONLY hz11_span_class[] (unchanged).
The span_meta is touched ONLY by the central source (cold path).
A span is "complete" when free_count == slot_count -> return to arena bump cursor
(or madvise the range). L1 does NOT implement this.
```

## Counters

```text
transfer_remove_hit       batch had objects
transfer_remove_miss      batch empty -> go to central
transfer_insert           objects pushed to transfer
transfer_insert_spill     transfer full -> spill to central
central_remove_hit        central stack had objects
central_remove_miss       central stack empty -> carve fresh span
central_insert            objects spilled to central stack
span_batch_create         span carved + batch extracted
refill_from_transfer      refill served by transfer batch
refill_from_central       refill served by central stack batch
refill_from_span          refill served by fresh span batch
overflow_to_transfer      overflow sent batch to transfer
```

## Smoke

```text
- transfer lane alloc/free/reuse correctness
- overflow sends a BATCH to transfer (not per-object)
- later refill reuses transfer objects (batch)
- transfer overflow spills to central stack when transfer is full
- later refill reuses central stack objects before carving a fresh span
- arena pointers are never sys_free'd
- large allocation still sys fallback
- token/span legacy lanes unchanged
```

## Bench

```text
fixed64 local:  guard only, no >5% regression (transfer adds cold-path code)
fixed16/256/4096: auxiliary

main_r50 / main_r90:
  via bench_matrix_malloc --remote-pct 50/90 --interleaved 1
  (the rows that should IMPROVE with batch transfer vs per-object returned-list)

larson:
  thread churn (if harness available)
```

The HZ8 bench_matrix_malloc harness (hakozuna-hz8/bench/bench_matrix_malloc.c)
supports `--remote-pct 0-100` and `--interleaved 0/1` (confirmed: interleaved
mode sends objects to a random other thread's inbox; remote-pct controls the
probability). This is the harness for measuring the transfer cache's win.

## GO/NO-GO

```text
GO:
  transfer counters prove batch reuse (remove_hit > 0, batch sizes > 1)
  central spill is later reused (central_insert > 0 and central_remove_hit > 0
    in a focused stress smoke/bench)
  main_r50 or main_r90 improves clearly over current returned-list path
  fixed-local does not regress > 5%
  span_create_count and post RSS do not materially blow up versus the current
    span lane; transfer cache itself is capped, but carved spans and central
    stack are explicit L1 liabilities until span reclaim exists

NO-GO:
  transfer cache unused (batch never triggers in the bench workload)
  central spill is not reused, causing fresh span churn
  mutex/batch overhead regresses mixed rows
  span_create_count or RSS grows materially without a documented reason
  public hit path instruction count changes materially
```

## Implementation verdict

Build lane:

```text
libhz11_span_transfer.so:
  -DHZ11_CLASSIFY_SPAN=1
  -DHZ11_TLS_FASTPATH=1
  -DHZ11_CACHE_BYTE_ACCOUNTING=0
  -DHZ11_CACHE_SOA=1
  -DHZ11_TRANSFER_CENTRAL_SPAN=1
```

Correctness and hygiene:

```text
make clean && make smoke preload-span-transfer hz11-standalone-check
  green

transfer smoke:
  xfer_hit=64
  xfer_insert=2048
  xfer_spill=128
  central_hit=4
  central_insert=128
```

This proves the transfer batch path and central spill/reuse path are both
exercised. The lane also initializes mutexes explicitly with `pthread_once` and
aborts on central-stack overflow instead of leaking arena objects.

Fixed-local guard (`fixed64`, RUNS=5, ITERS=10M):

| Allocator | ops/s | instr/op | cycles/op |
|---|---:|---:|---:|
| tcmalloc | 189.88M | 87.3 | 22.6 |
| hz11-span-soa | 163.86M | 100.5 | 24.6 |
| hz11-span-transfer | 163.95M | 100.5 | 27.4 |

The public hit path is effectively unchanged: objdump counts for
`hz11_malloc`/`hz11_free` are identical to `span-soa` (69/48), and the fixed64
instruction count is unchanged.

Main mixed rows (`bench/out/bench_matrix_malloc`, THREADS=16, ITERS=100000,
RUNS=10 median, size=16..32768, interleaved=1, live_window=1024):

| Row | Allocator | Median ops/s | p25 | p75 | post RSS | peak RSS |
|---|---:|---:|---:|---:|---:|---:|
| main_local0 | tcmalloc | 451.51M | 329.65M | 463.55M | 11.13MiB | 11.13MiB |
| main_local0 | hz11-span-soa | 400.17M | 389.44M | 405.39M | 8.12MiB | 8.12MiB |
| main_local0 | hz11-span-transfer | 455.58M | 433.35M | 466.82M | 8.12MiB | 8.12MiB |
| main_r50 | tcmalloc | 42.02M | 33.75M | 42.60M | 105.70MiB | 105.70MiB |
| main_r50 | hz11-span-soa | 20.60M | 19.08M | 20.94M | 100.00MiB | 100.00MiB |
| main_r50 | hz11-span-transfer | 87.66M | 84.04M | 90.10M | 74.63MiB | 74.63MiB |
| main_r90 | tcmalloc | 23.47M | 22.75M | 23.57M | 105.03MiB | 105.66MiB |
| main_r90 | hz11-span-soa | 9.99M | 9.71M | 10.09M | 96.50MiB | 96.50MiB |
| main_r90 | hz11-span-transfer | 62.41M | 61.45M | 63.08M | 80.75MiB | 80.75MiB |

Transfer counters from the RUNS=10 process:

```text
main_r50:
  span_create=3271
  xfer_hit=144190
  xfer_miss=1686
  xfer_insert=2308832
  xfer_spill=352
  central_hit=22
  central_miss=1664
  central_insert=352

main_r90:
  span_create=3127
  xfer_hit=362950
  xfer_miss=1601
  xfer_insert=5809328
  xfer_spill=112
  central_hit=7
  central_miss=1594
  central_insert=112
```

Conclusion: GO. The A/B confirms the design hypothesis: the old per-object
returned-list path was the mixed/remote bottleneck. Batch transfer moves the
cost from one mutex operation per object to one mutex operation per batch and
turns the HZ11 mixed rows from below tcmalloc to above tcmalloc in this harness.
This is still an opt-in research lane; it is not a product claim that HZ11
beats tcmalloc broadly.

## Implementation order (completed)

```text
1. src/hz11_transfer_cache.{c,h} -- struct + remove_range/insert_range
2. src/hz11_span.c -- central object stack remove/insert range
3. src/hz11_thread_cache.c -- refill order:
     transfer -> central stack -> span batch
4. src/hz11_thread_cache.c -- overflow order:
     collect batch -> transfer -> central stack spill
5. Makefile + smoke + standalone
6. Measure fixed64 guard + main_r50/r90
7. Record verdict
```
