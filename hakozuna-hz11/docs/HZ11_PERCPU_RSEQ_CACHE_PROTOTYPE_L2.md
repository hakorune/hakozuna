# HZ11PerCpuRseqCachePrototype-L2

Box name: HZ11PerCpuLockedCachePrototype-L2 (the `.so` lane keeps `_rseq` because
it is rseq-selected). Status: **NO-GO for the CPU-locality hypothesis (locked
test).** Keep fine128 as the candidate. Do NOT attempt the lock-free rseq CS.

This prototype uses rseq **only to select the current CPU**. The per-CPU cache
mutation is **locked** (per-CPU spinlock) for correctness. There is **no
hand-rolled rseq critical section** in this box. The lock-free single-commit rseq
CS is explicitly deferred — and, per this box's result, **not justified**.

## Boundary

```text
correctness-first locked diagnostic lane
rseq cpu_id for CPU selection only; no hand-rolled rseq CS
per-CPU spinlock serializes each CPU's slab mutation
self-disable on any rseq probe uncertainty -> identity fine128
opt-in sibling only; fine128 / span-transfer / default unchanged
```

## What was built

```text
new:    src/hz11_percpu_cache.c, src/hz11_percpu_cache.h
mod:    src/hz11_public_entry.c  (pop/push hooks, gated #if HZ11_PERCPU_RSEQ)
        Makefile (.so rule = fine128 + -DHZ11_PERCPU_RSEQ=1; preload; clean; .PHONY;
                  CORE_SRC += hz11_percpu_cache.c)
        .gitignore, scripts/check_hz11_standalone.sh (build-product list)
        scripts/run_hz11_macro_speed_lane_gate.sh (allocator case + dump case)
        scripts/run_hz11_transfer_promotion_matrix.sh (allocator case)
lane:   libhz11_span_transfer_thread_exit_cap_batch32_fine128_rseq.so
```

Design: a global `H11PerCpu g_percpu[MAX_CPUS]` of bounded pointer stacks
(`HZ11_PERCPU_CAP=32`) for the first `HZ11_PERCPU_NCLASSES=12` small classes
(<= ~2 KiB; covers sh6bench's 1..1000 B range). `hz11_percpu_pop/push` read the
current CPU from the glibc-registered rseq area (`cpu_id`) and serialize the slab
op with a per-CPU spinlock. Lazy probe on first use; disables on `__rseq_size==0`,
`cpu_id` UNINITIALIZED/REG_FAILED, or non-x86-64/non-Linux. Counters
(`rseq_enabled/fallback`, `percpu_hit/miss/flush/abort`) dump via
`HZ11_PERCPU_STATS=1` atexit.

## System readiness (verified first-hand)

```text
kernel 6.8.0 (>= 4.18), glibc 2.35, nproc 16, 1 NUMA
#include <sys/rseq.h> resolves; __rseq_offset=2464, __rseq_size=32, RSEQ_SIG=0x53053053
runtime probe: cpu_id valid (e.g. 4) -> layer self-enables (enabled=1)
```

## Phase 1 - correctness: PASS

```text
Macro gate RUNS=5: python_alloc, mstress, sh6bench, larson, xmalloc_test,
cache_scratch all OK:5 under the rseq lane. No crashes, no aborts, no timeouts.
Layer active: HZ11_PERCPU_STATS dump shows enabled=1, hit/miss/flush reconcile
with malloc/free counts, abort=0. Counters fire on the churn/multi-thread rows
(larson, xmalloc_test, sh6bench, mstress). Correctness and fallback hold.
```

## Phase 2 - measurement: sh6bench REGRESSED (locked lane is a net loss)

Macro gate medians (RUNS=5), fine128-rseq vs fine128 vs tcmalloc:

| workload | tcmalloc wall | fine128 wall | rseq wall | rseq/fine128 | rseq xfer_insert | fine128 xfer_insert |
|---|---:|---:|---:|---:|---:|---:|
| sh6bench | 0.360 | 3.527 | **12.917** | **3.66x slower** | 757,752,618 | 868,988,121 |
| xmalloc_test | 2.042 | 2.023 | 2.026 | ~flat | 123,257,900 | 1,011,706,885 |
| larson | 4.152 | 4.146 | 4.143 | ~flat | 3,640,424 | 60,294,583 |
| mstress | 0.188 | 0.215 | 0.237 | 1.10x slower | 1,992,382 | 2,324,161 |
| python_alloc | 0.031 | 0.031 | 0.031 | flat | 28,832 | 38,720 |
| cache_scratch | 1.173 | 1.178 | 1.181 | flat | 0 | 20 |

```text
sh6bench wall: 3.527s -> 12.917s (3.66x WORSE than fine128; 35.9x tcmalloc).
This is the exact row the box hoped to fix. It moved the wrong way.
```

### The confound (important, recorded honestly)

The per-CPU slab DID absorb transfer traffic - the locality mechanism is real:

```text
sh6bench   xfer_insert: 868,988,121 -> 757,752,618  (12.8% fewer transfer inserts)
xmalloc_test xfer_insert: 1,011,706,885 -> 123,257,900 (8.2x fewer)
larson      xfer_insert: 60,294,583 -> 3,640,424      (16.6x fewer)
```

But the per-op spinlock cost (CAS acquire + atomic release on every eligible
malloc/free; hundreds of millions of ops on sh6bench) dominates and cancels or
exceeds the locality benefit on wall time. On xmalloc_test the slab cut transfer
inserts 8.2x yet wall stayed flat - the lock overhead exactly offset the win. On
sh6bench the lock overhead exceeded it, producing a 3.66x regression.

So this measurement is **confounded by lock overhead**: it proves a *locked*
per-CPU layer is a net loss, and that the slab absorbs traffic, but it does NOT
cleanly isolate whether lock-free per-CPU locality would win - only a lock-free
rseq CS could test that.

## Decision

Per the agreed gate ("locked lane must improve sh6bench materially to justify the
lock-free rseq CS"):

```text
NO-GO for the CPU-locality hypothesis (as cheaply testable).
The locked lane did NOT improve sh6bench; it regressed 3.66x.
-> Do NOT attempt the hand-rolled lock-free single-commit rseq CS.
-> Keep fine128 (libhz11_span_transfer_thread_exit_cap_batch32_fine128.so)
   as the recommended opt-in macro candidate.
```

This box can only judge the locality hypothesis; it **cannot adopt a real rseq
lane** (that requires a lock-free single-commit CS box, which is now NOT
warranted).

### What this proves and does not prove

```text
PROVES:
  - A locked per-CPU front cache is a net loss on sh6bench (lock overhead dominates).
  - The per-CPU slab does absorb transfer/central traffic (locality mechanism works).
  - rseq cpu_id selection + the lazy self-disable probe are correct and stable.
DOES NOT PROVE:
  - That lock-free per-CPU locality would lose too (untested; lock overhead
    confounds this measurement).
  - That locality is definitively the wrong lever for sh6bench.
```

The honest read: the cheap test did not pay off, so per the gate the expensive
lock-free rseq CS is not pursued. If sh6bench is revisited later, the remaining
documented levers are (a) a lock-free rseq CS only if a new justification arises,
and (b) the separate RSS lever (created span/page footprint, span_reuse ~51),
which per-CPU locality does not address regardless.

## Claim Boundary

```text
Allowed:
  - the locked per-CPU prototype is correct and self-disables safely.
  - it absorbs transfer/central traffic (xfer_insert drops), proving the slab works.
  - it is a net wall-time LOSS on sh6bench (3.66x vs fine128) due to lock overhead.

Not allowed:
  - per-CPU/rseq improves sh6bench. (It does not, in this locked form.)
  - locality is the wrong lever for sh6bench (confounded; not cleanly tested).
  - the rseq lane is adopted or recommended. (It is not.)
  - HZ11 generally beats tcmalloc (carried from HZ11PaperPositioning-L1).
```

## Status of the lane

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128_rseq.so is built and
registered but is NOT a candidate. It remains in-tree as a reproducible
negative-result diagnostic (HZ11_PERCPU_STATS=1 shows the slab is active).
fine128 remains the recommended opt-in macro candidate. Default unchanged.
```
