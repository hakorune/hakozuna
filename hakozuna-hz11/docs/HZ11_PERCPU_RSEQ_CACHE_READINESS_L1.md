# HZ11PerCpuRseqCacheReadiness-L1

Status: GO to design the L2 prototype boundary. NOT GO to implement allocator code
in this box. NOT GO to adopt rseq. NOT a claim that rseq fixes sh6bench.

This is a readiness/design box. It fixes the boundary, rollback, and adoption gate
for a per-CPU/rseq front cache *before* any allocator code is written, then issues
a GO/NO-GO on starting the prototype. Per HZ11 discipline, ambiguity is killed here
so `HZ11PerCpuRseqCachePrototype-L2` can be a narrow, well-scoped implementation box.

## Boundary

```text
readiness/design only
no allocator policy implementation
no default promotion
no public claim that per-CPU/rseq will fix sh6bench
```

This box is docs-only. It does not touch the Makefile, `.gitignore`, `src/`,
`include/`, or `bench/lib/bench_common.sh`. Every implementation action named below
is *specified for the L2 prototype box*, not executed here.

## Context

`HZ11PaperPositioning-L1` is the current HZ11 line statement. fine128 is the
recommended opt-in macro speed-lane candidate (not default). span-transfer remains
the remote/mixed microbench lane. sh6bench is the one open wall-time pathology:
fine128 sh6bench wall is 10.120x tcmalloc.

First-hand sh6bench attribution (`HZ11_SH6BENCH_PATH_COST_ATTRIBUTION_L1`,
`HZ11_SH6BENCH_TRANSFER_CENTRAL_PATH_COST_L1`) localizes the wall blocker:

```text
primary wall lever:  transfer/central path volume and batch/overflow behavior
                      (xfer_insert ~502M, xfer_spill/central_insert ~25.5M)
primary RSS lever:   created span/page footprint (span_create ~16.7K, span_reuse ~51)
                      not central final retention, not live current spans
```

The `xferwide` probe eliminated central spill entirely (-> 0) and still left
10.019x tcmalloc wall. So transfer-cache *policy* knobs are exhausted; the next
lever is *structure* (CPU locality). That is why per-CPU/rseq is the leading
technical hypothesis for the sh6bench wall gap. It is a hypothesis, not a proven fix.

## System Readiness (verified on this machine)

```text
kernel:        Linux 6.8.0-90-generic (rseq syscall needs >= 4.18)   -> OK
glibc:         2.35 (auto per-thread rseq registration via __rseq_offset /
               __rseq_size / __rseq_flags)                            -> OK
headers:       sys/rseq.h + linux/rseq.h present; struct rseq and
               struct rseq_cs fully defined; __NR_rseq == 334         -> OK
librseq:       not installed                                          -> not needed
CPUs:          16 logical (AMD Ryzen 7 5825U, 8c/16t), 1 NUMA node
per-CPU array: size to 16 logical CPUs (or sysconf(_SC_NPROCESSORS_CONF))
```

The machine is fully ready. glibc rseq symbols are the preferred registration
mechanism, but the L2 implementation must **verify** them at runtime and fall back
if they are absent or unregistered (see Fallback Strategy).

## Decision

```text
GO   to design the L2 prototype boundary (what this box fixes).
NOT GO   to implement allocator code in this box (docs-only).
NOT GO   to adopt rseq as a lane (adoption is gated; see Q5).
NOT a claim   that rseq fixes sh6bench (leading hypothesis, must earn the gate).
```

Rationale for GO-to-design:

```text
- System is fully ready (kernel/glibc/headers/CPUs above).
- The sh6bench wall lever is documented (transfer/central volume); xferwide proved
  widening the transfer cache is insufficient, pointing to per-CPU locality.
- Rollback is clean: the .so filename is the A/B selector via LD_PRELOAD; no other
  lane or the default path is touched.
```

## Q1 - Where the per-CPU/rseq cache sits

Candidate base: `libhz11_span_transfer_thread_exit_cap_batch32_fine128.so`.

A CPU-local front cache inserts between `hz11_thread_cache_pop/push`
(`src/hz11_thread_cache.c`) and `hz11_transfer_remove_range/insert_range`, on the
fine128 base.

```text
malloc:
  rseq critical section reads cpu_id, serves from that CPU's slab.
  on miss, refills via the UNCHANGED transfer/central/span path.

free:
  pushes to the CURRENT CPU-local cache (rseq) - whichever CPU the freeing thread
  is on at that moment.
  on overflow, the existing thread/transfer/central paths handle it.
  NO owner-tracking. NO cross-CPU routing in the prototype.
```

The whole layer is gated by `HZ11_PERCPU_RSEQ`.

Important scoping (do not overclaim): the prototype does NOT route a free back to
the CPU that owns the object. A free lands on the current CPU's cache and overflows
to the existing paths. Cross-CPU/owner routing is explicitly out of scope.

## Q2 - First minimal box

Recommended implementation box: `HZ11PerCpuRseqCachePrototype-L2`.

Narrow opt-in sibling:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128_rseq.so
```

Built by cloning the fine128 Makefile rule and appending `-DHZ11_PERCPU_RSEQ=1`
(boolean-flag convention; default 0). New flag declared in house style:

```c
#ifndef HZ11_PERCPU_RSEQ
#define HZ11_PERCPU_RSEQ 0
#endif
```

Scope of the first box:

```text
- CPU-local front cache for small classes only.
- refill/flush to the existing thread/transfer/central path (no policy rewrite).
- Linux-only compile guard.
- explicit fallback when rseq is unavailable.
```

## Q3 - Boundary

Allowed:

```text
- CPU-local front cache for small classes
- refill/flush to existing thread/transfer/central path
- Linux-only compile guard
- explicit fallback when rseq unavailable
```

Not allowed:

```text
- rewriting transfer/central policy in the same box
- changing default lane
- changing fine128 class policy
- changing public claims
```

## Q4 - Rollback

The build keeps all of these unchanged:

```text
- fine128 without rseq (libhz11_span_transfer_thread_exit_cap_batch32_fine128.so)
- span-transfer lane (libhz11_span_transfer.so)
- the default allocator path
```

The `_rseq` lane is a separate Makefile target and a separate `.so`. A/B selection
is `LD_PRELOAD` only - the `.so` filename is the selector. Rollback = stop
preloading `_rseq`, preload fine128 instead.

L2 prototype box must (listed, not done here):

```text
- add the libhz11_..._fine128_rseq.so target (clone fine128 rule + -DHZ11_PERCPU_RSEQ=1)
- add a preload-*-fine128-rseq convenience target
- add the new .so to .gitignore
- add the new .so to scripts/check_hz11_standalone.sh build-product list
  (that script checks a FIXED product list; .gitignore alone is not enough)
- register the named allocator hz11-thread-exit-cap-batch32-fine128-rseq in
  bench/lib/bench_common.sh (or use the absolute .so path in gate commands)
```

## Q5 - rseq adoption gate

Nothing is adopted or recommended until this gate passes. rseq must earn itself.

```text
1. sh6bench wall moves materially toward tcmalloc.
   Target: <= ~1.5x for adoption.
   A partial improvement can be attribution-GO only, NOT adoption.

2. No regression on the 5/6 macro near-parity rows.

3. max RSS remains inside the existing guard.

4. No additional material remote/mixed regression VERSUS fine128 (the candidate base).
   span-transfer remains the dedicated remote/mixed lane and is NOT the baseline here.

5. Fallback path works when rseq is unavailable.

6. Rollback is a separate .so / LD_PRELOAD selector only.
```

## Touched Modules (L2 prototype box; listed not done)

```text
new:      src/hz11_percpu_cache.c
          include/hz11_percpu_cache.h
modified: src/hz11_thread_cache.c        (pop/push/refill integration points)
          include/hz11_thread_cache.h    (struct / flag)
          Makefile                        (new target + preload convenience target)
          .gitignore                      (new .so)
          scripts/check_hz11_standalone.sh (add new .so to build-product list)
          bench/lib/bench_common.sh       (register named allocator, or use abs path)
```

## Fallback Strategy

```text
- HZ11_PERCPU_RSEQ defaults to 0 -> the whole feature no-ops; non-rseq builds are
  byte-identical to today.
- rseq code is under #ifdef __linux__.
- glibc rseq symbols (__rseq_offset / __rseq_size / __rseq_flags, glibc 2.35+) are
  PREFERRED, but the implementation must VERIFY them at runtime and fall back:
    if __rseq_size == 0 (absent or not registered), the per-CPU layer self-disables
    at first touch and the existing thread/transfer path is used.
- rseq abort (CPU migration / preemption inside the critical section) retries or
  falls back per-op. This is correctness-preserving because the per-CPU slab is
  only a cache; missing it always falls through to the existing path.
```

## Rollback Strategy

```text
- separate .so target + LD_PRELOAD selector; no other lane or default touched.
- rollback = stop preloading _rseq, preload fine128.
- the default path and span-transfer are never modified by this work.
```

## Benchmark Gate Commands

Exact invocations. The named allocator `hz11-thread-exit-cap-batch32-fine128-rseq`
requires the L2 registry edit in bench/lib/bench_common.sh; until then, pass the
absolute `.so` path instead.

```bash
# Macro gate (sh6bench is the row of interest; also python_alloc, larson,
# xmalloc_test, cache_scratch, mstress). Candidate = the rseq sibling.
RUNS=5 hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh \
  --allocators tcmalloc,hz11-thread-exit-cap-batch32-fine128,hz11-thread-exit-cap-batch32-fine128-rseq \
  --candidate hz11-thread-exit-cap-batch32-fine128-rseq

# Remote/mixed matrix (RUNS=10). Baseline for the regression check is fine128;
# span-transfer is included as the dedicated remote/mixed lane reference.
RUNS=10 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit-cap-batch32-fine128,hz11-thread-exit-cap-batch32-fine128-rseq

# Focused current-RSS sampling (python_alloc, sh6bench).
RUNS=10 SAMPLE_SLEEP=0.005 \
  hakozuna-hz11/scripts/run_hz11_macro_current_rss_semantics.sh
```

tcmalloc reference: `/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4`
(override via `TCMALLOC_SO`).

## Honest Caveats

```text
- The xferwide residual (~3.6s wall after central spill = 0, still 10.019x) means
  rseq may recover only PART of the sh6bench gap. Gate check 1 is the arbiter.
- The sh6bench RSS blocker is created span/page footprint, a SEPARATE lever that
  per-CPU locality does NOT address. Gate check 3 (max RSS inside guard) must still
  pass; if RSS regresses, that is a future pageheap-release / span-reuse problem,
  not something rseq fixes.
- per-CPU/rseq is the LEADING hypothesis, not a guarantee. It must earn adoption.
```

## Claim Boundary

Allowed by this box:

```text
- per-CPU/rseq is the leading next technical hypothesis for the sh6bench wall gap.
- it is gated: nothing is adopted until the Q5 gate passes.
- rollback is LD_PRELOAD-only; no default or lane is changed.
```

Not allowed:

```text
- per-CPU/rseq will fix sh6bench.
- rseq is adopted or recommended as a lane.
- the prototype does owner-tracking or cross-CPU routing.
- any change to the default path, span-transfer lane, or fine128 class policy.
- HZ11 generally beats tcmalloc (carried from HZ11PaperPositioning-L1).
```

## Next Step

```text
If this readiness box is accepted: proceed to HZ11PerCpuRseqCachePrototype-L2 as a
narrow opt-in sibling under -DHZ11_PERCPU_RSEQ=1, with the boundary, fallback,
rollback, and gate fixed here. Do NOT adopt or promote on prototype completion
unless the Q5 gate passes.
```
