# HZ5 Linux Local2P Candidate Design

## Status

Design note for the next Linux-only HZ5 candidate lane.

This is not a Windows P43i/P45 change. It is a Linux local-throughput control
lane for exact `64K align=8192`.

## Problem

Native Ubuntu measurements show that the current HZ5 Linux speed lane is not
losing because of fallback or accidental P43 routing.

Trace for `64K/a8192`:

```text
alloc_p25_bridge=100000
free_p25_bridge=100000
wrapper_decode_ok=100000
```

Perf stat for local-only `64K/a8192`, 1M iterations:

```text
allocator  ops/s     cycles      instructions  branches    cache-misses
tcmalloc   249.5M     46.4M      147.5M        32.8M       153K
hz4        131.5M     64.2M      270.2M        54.8M        44K
hz5         68.7M    121.3M      337.6M        87.9M       133K
```

HZ5 is spending too much work in the P25 bridge hot path.

## HZ4 Reference Behavior

For `posix_memalign(&p, 8192, 65536)`, HZ4 does not allocate exactly 64K.
It over-allocates for alignment metadata:

```text
65536 + (8192 - 1) + sizeof(hz4_aligned_hdr_t) = 73759 bytes
```

HZ4 then routes that request through `hz4_large_malloc()`. After the large
header and page rounding, the object becomes a 2-page large span:

```text
align_up(73759 + large_header, 65536) = 131072 bytes
```

The steady-state local-only path is effectively:

```text
alloc: TLS 2-page span pop
free:  TLS 2-page span push
```

HZ4 stats for 100K iterations:

```text
large_acq=131072
large_rel=0
pagebin_acq_miss=1
b15_acq_call=1
remote_free_seen=0
```

This row is not syscall-bound. HZ4 reaches its result by keeping the local
hot path inside a small per-thread large-span cache.

## Design Goal

Add a Linux-only Local2P candidate lane:

```text
BENCHLAB_HZ5_LINUX_LOCAL2P=1
```

The lane targets only:

```text
size == 65536
align == 8192
raw_bytes == 131072
platform == Linux
```

The hot path should avoid:

- P43 source/direct-token topology
- P43 ownership lookup
- P25 relbuf/global work on every local allocation/free pair
- runtime decommit or release in the first speed candidate

## Non-Goals

- Do not weaken, rename, or retune Windows P43i/P45.
- Do not promote this as a paper lane before measurement.
- Do not mix Local2P with P43 token experiments.
- Do not add remote-free owner inbox in the first implementation.
- Do not enable decommit or runtime release in the first implementation.
- Do not make unsupported routes fallback and count as HZ5 wins.

## Proposed Hot Path

### Allocation

```text
hz5_policy_alloc_aligned(65536, 8192)
  if Linux Local2P enabled:
    raw = local2p_tls_pop()
    if raw:
      init wrapper
      return aligned user pointer

    raw = local2p_os_alloc_128k()
    init wrapper
    return aligned user pointer

  otherwise:
    existing P25/P43 policy
```

### Free

```text
hz5_policy_free(ptr)
  decode wrapper

  if source == HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P:
    validate local2p metadata
    mark ACTIVE -> FREED
    if owner is current thread:
      local2p_tls_push(raw)
    else:
      local2p_global_push(raw)
    return

  otherwise:
    existing policy
```

The first candidate should use direct raw 128K spans in a TLS stack, not P25
lowpage nodes. The purpose is to test the topology HZ4 is winning with.

## Source Strategy

Phase 1 source:

```text
posix_memalign(&raw, 8192, 131072)
```

This keeps the first candidate close to the HZ4 comparison shape.

Later source options:

- Linux `mmap` 128K source
- P43 segment source as a cold source only
- RSS release lane with explicit policy

Do not route steady-state local alloc/free through P25 bridge just to obtain
raw spans. That would reintroduce the measured bottleneck.

## Remote Recycle Control

The first Local2P implementation released cross-thread frees directly to the
OS. That made producer/consumer remote-free syscall-bound.

Current control path:

```text
remote free:
  validate wrapper/cookie/state
  ACTIVE -> FREED
  push raw 128K span to bounded global recycle stack, cap 1024 by default

alloc TLS miss:
  pop from global recycle stack
  otherwise posix_memalign(8192, 131072)
```

This keeps the `local2p` route intact and avoids P25/P43 topology changes. It
is still not the final remote-free design: it is a simple global recycle
control, not an owner inbox.

## Metadata

Add a distinct wrapper source tag:

```c
HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P = 6
```

Minimum metadata:

```text
source
raw
raw_bytes
requested
wrapper cookie
local2p state
local2p generation or cookie
owner id / owner token
```

Minimum state machine:

```text
ACTIVE -> FREED
```

The optimized implementation uses `atomic_exchange(ACTIVE -> FREED)` rather
than compare-exchange. The safety property is the same for this state machine:
only one free can observe `ACTIVE`.

The first safe candidate should reject invalid frees by:

- wrapper decode failure
- source mismatch
- raw_bytes mismatch
- cookie mismatch
- state not ACTIVE
- unsupported route

Invalid owned-looking pointers must not fall back to HZ3/CRT or P43 release.

Diagnostic-only cost switches exist for attribution experiments:

```text
--linux-local2p-no-cookie
--linux-local2p-no-cas
```

Do not use those as paper or safety lanes.

## TLS Cache

Start with the smallest useful cache:

```text
cap1: one 128K raw span per thread
```

Then measure:

```text
cap8: up to eight 128K raw spans per thread
```

Expected local-only behavior for alloc/free pairs:

```text
first allocation: OS source
steady state: TLS pop / TLS push
```

Overflow policy for the first candidate should be explicit and visible in the
lane name or build metadata. Prefer keeping it simple:

```text
cap full -> OS release or clearly separated fallback path
```

Do not silently push overflow into P25 global structures in a speed result
without separate attribution.

## Remote-Free Policy

Local2P is initially a local-only throughput lane.

Remote-free should not push into the freeing thread's TLS cache.

Phase 1 options:

- fail closed in standalone exact-only safety tests
- escape to an explicit existing safe path and count it separately

Later phases may add:

- owner id in wrapper metadata
- owner inbox
- remote batch drain
- comparison against P43 token remote/RSS lanes

## Trace Lane

Speed builds must keep counters off.

Trace builds should expose:

```text
alloc_local2p_tls_hit
alloc_local2p_os
alloc_local2p_escape
free_local2p_tls
free_local2p_remote
free_local2p_invalid_cookie
free_local2p_invalid_state
free_local2p_overflow
```

Trace and speed lanes must remain mutually exclusive.

## Measurement Gates

### A. Local-Only Throughput

Workload:

```text
threads=1
iters=1M and 10M
size=65536
align=8192
```

Compare:

```text
tcmalloc
HZ4
HZ5 P25
HZ5 Local2P cap1
HZ5 Local2P cap8
HZ5 p25attr
HZ5 p43-token
```

Go criteria:

```text
minimum: Local2P >= HZ5 P25 + 50%
strong:  Local2P >= HZ4 - 10%
stretch: Local2P approaches tcmalloc instruction count trend
```

Record:

```text
ops/s median
ru_maxrss
perf stat: cycles, instructions, branches, branch-misses, cache refs/misses
trace counts from trace-only lane
```

### B. Safety Smoke

Cases:

```text
valid alloc/free
double free before reuse
foreign pointer
mutated wrapper magic
mutated wrapper cookie
wrong source
cross-thread free
unsupported routes
```

Expected:

```text
valid: succeeds
invalid: fail closed, no fallback, no crash
unsupported: NULL or explicit non-HZ5 result, not a HZ5 win
```

### C. Remote-Free

Workload:

```text
producer alloc
consumer free
64K/a8192
```

Compare:

```text
Local2P with explicit remote policy
P25 bridge
P43 token remote candidate
HZ4
tcmalloc
```

This decides whether Local2P needs an owner inbox.

### D. RSS Plateau

Workloads:

```text
64K/a8192 plateau
64K + 256K mixed prelude
cap1/cap8 comparison
```

Expected:

```text
no unbounded growth
cap choice is visible in RSS
RSS lane remains separate from speed lane if release policy is added
```

## Implementation Order

1. Add build flag and lane descriptor for `hz5-linux-local2p`.
2. Add wrapper source tag and Local2P metadata behind the build flag.
3. Implement trace-only Local2P counters.
4. Implement cap1 TLS 128K raw span stack.
5. Wire exact `64K/a8192` alloc/free into Local2P only under the flag.
6. Add safety smoke tests.
7. Run local-only A/B against HZ4 and tcmalloc.
8. Add cap8 only after cap1 behavior is understood.
9. Add remote-free policy only after local-only A/B is complete.

## Current Decision

Proceed with Linux Local2P as the next development lane.

Do not continue tuning P43 direct token for local-only throughput. Keep P43
token/source lanes for remote-free, RSS, and source-control experiments.
