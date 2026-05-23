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
  if Local2P enabled:
    try local2p_direct_decode(ptr)
    if source == HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P:
      validate local2p metadata
      mark ACTIVE -> FREED
      if owner is current thread:
        local2p_tls_push(raw)
      else:
        local2p_global_push(raw)
      return

  decode wrapper for non-Local2P routes

  otherwise existing policy
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

## Local / Remote Split

Local and remote Local2P rows share only the front-end attribution work:

```text
direct decode
source/cookie validation
state validation
owner token comparison
```

After that point the hot paths should stay separate.

Local-only target:

```text
alloc: TLS object-node pop
free:  owner-local state mark + TLS object-node push
```

Remote-free target:

```text
producer alloc: owner TLS / owner inbox / cold source
consumer free:  remote state mark + owner handoff
```

Do not force one shared recycle fast path for both cases. Local throughput is a
single-thread freelist problem; remote-free is a cross-thread handoff problem.
Common helpers are useful for decode and validation, but recycle policy should
remain lane-specific.

Current rule:

```text
local-speed lanes may optimize dispatch/header/cookie/state
remote lanes may optimize inbox/batching/handoff
RSS lanes may optimize cap/release/decommit
```

Never promote a local-speed improvement as a remote-free improvement unless the
producer/consumer row confirms it.

Implementation organization follows the same boundary:

```text
common helpers:
  local2p_validate_free_header
  local2p_recycle_local
  local2p_recycle_remote

local helper:
  owner-local TLS object-node push

remote helper:
  remote trace, optional batch, optional owner inbox, global fallback
```

The helpers are intentionally small and inline-friendly. They exist to make the
route boundary explicit, not to force one shared local/remote fast path.

### Remote-Batch Candidate

`hz5-linux-local2p-remote-batch` is a remote-only A/B on top of owner inbox.
The consumer/freeing thread keeps a small per-thread batch for one owner and
splices that list into the owner's inbox with one CAS when the batch fills:

```text
remote free:
  validate Local2P metadata
  ACTIVE -> FREED via locked exchange
  append node to freeing-thread remote batch
  when batch is full, CAS whole batch to owner inbox

owner alloc:
  TLS pop
  owner inbox pop/drain
  global/cold source
```

This intentionally trades some return latency for fewer remote inbox CAS
operations. It is not a local-speed candidate and should be judged by
producer/consumer remote-free first.

Initial RUNS=10 result:

```text
private/raw-results/linux/local2p_remotebatch_runs10

remote pairs/s:
  remote-batch 13.78M
  P25 control  12.08M
  HZ4          11.88M
  owner inbox   9.99M
  fast-cookie   8.23M

local:
  remote-batch 207.7M ops/s
  fast-cookie  206.8M ops/s

mixed:
  fast-cookie  213.8M ops/s
  remote-batch 206.9M ops/s
```

This is a real remote-free win, but not a replacement for the local/mixed speed
reference. Keep it as a remote lane and tune batch cap / flush policy there.

Batch cap is intentionally a build-time A/B:

```text
--linux-local2p-remote-batch-cap 8
--linux-local2p-remote-batch-cap 16  # default
--linux-local2p-remote-batch-cap 32
```

Smaller caps return spans to the owner sooner but pay more inbox CAS work.
Larger caps reduce CAS pressure but delay reuse and can increase cold-source
pressure. Judge by producer/consumer remote-free first, then verify local,
mixed, and RSS do not regress unexpectedly.

Initial cap sweep:

```text
private/raw-results/linux/local2p_remotebatch_cap_runs10

remote pairs/s:
  cap32 15.28M
  cap16 15.22M
  cap8  14.87M

mixed:
  cap8  204.9M ops/s
  cap16 204.7M ops/s
  cap32 199.5M ops/s
```

Keep cap16 as the balanced default. Use cap32 only for remote-only rows because
the remote win is small and mixed throughput is worse.

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

## Current Attack Order

Ubuntu/Linux should be attacked as separate workload lanes, not as a direct
Windows P43i/P45 port.

1. `local2p-speed`
   - goal: reduce local `64K/a8192` hot-path instructions toward tcmalloc
   - current step: Local2P direct free decode before generic wrapper decode
2. `local2p-remote`
   - goal: beat P25/HZ4 producer/consumer remote-free
   - current step: owner inbox / MPSC queue candidate
3. `rss-control`
   - goal: keep low final RSS without confusing the speed lane
   - next step: separate release/decommit policy lane

Do not add remote inbox or RSS release policy to the speed lane until the
direct free decode cost is measured.

Direct free decode result:

```text
local 64K/a8192, 10M iterations:
  before direct decode: about 140M ops/s, 2.42B instructions
  after direct decode:  about 150.5M ops/s, 2.30B instructions

repeat5 focus:
  local:       143.5M ops/s
  mixed final: 148.1M ops/s
```

Next distinct lane is `local2p-remote`, not more global-lock work in the speed
lane.

## Owner Inbox Candidate

The global recycle stack removes per-remote-free OS release, but it still
serializes producer/consumer traffic through one lock. The next remote-free
candidate keeps the Local2P source and wrapper semantics but routes cross-thread
frees to the allocating thread's inbox.

Build knob:

```text
--linux-local2p-owner-inbox
```

Candidate path:

```text
alloc owner thread:
  TLS pop
  owner inbox pop
  global recycle pop
  OS source

remote free thread:
  validate wrapper/cookie/state
  ACTIVE -> FREED
  MPSC push raw span to owner inbox
```

Constraints:

- This is a `local2p-remote` candidate, not the speed default.
- The first candidate assumes the allocation owner thread is still alive while
  remote frees arrive.
- If the owner inbox is unavailable, fall back to the bounded global recycle
  stack.
- Do not add RSS release/decommit policy here.

First result:

```text
repeat5 producer/consumer pairs/s:
  local2p-fast:  7.17M
  local2p-inbox: 10.47M
  p25-control:  12.57M
```

The inbox candidate improves remote-free materially but does not yet beat P25.
Next remote work should focus on reducing remote push cost or adding larger
handoff batches.

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

### Object-Node Candidate

`hz5-linux-local2p-object-node` changes only the recycle topology:

```text
current fast lane:
  TLS/global/inbox node = raw span pointer

object-node lane:
  TLS/global/inbox node = aligned user pointer
  freed user memory stores freelist next
```

The wrapper header still stores `raw`, cookie, owner, generation, and state.
The first object-node A/B keeps the existing Local2P safety checks:

```text
free:
  direct Local2P decode
  wrapper cookie check
  Local2P cookie check
  ACTIVE -> FREED atomic state transition
  push aligned user pointer into recycle cache

alloc cached reuse:
  pop aligned user pointer
  recover raw from wrapper header
  skip invariant wrapper-prefix rewrite
  refresh Local2P attribution/state
```

This is the tcmalloc-like topology test. It is still not the final slim-header
or same-owner-fast-state design.

### Same-Owner Fast-State Candidate

`hz5-linux-local2p-same-owner-fast-state` keeps the object-node topology and
changes only the state transition:

```text
owner == current:
  atomic_load ACTIVE check
  atomic_store FREED

owner != current:
  atomic_exchange ACTIVE -> FREED
  remote path
```

This removes the locked RMW from the owner-local hot path while keeping remote
free on the safer transition. It still detects sequential double-free before
reuse, but it is a candidate lane because it is intentionally weaker than
locked exchange for concurrent invalid frees.

### Route-Cookie Candidate

`hz5-linux-local2p-route-cookie` keeps object-node and same-owner fast-state,
but uses the Local2P cookie as the direct route attribution guard:

```text
direct decode:
  magic/layout/source/raw checks
  no generic wrapper-cookie recompute

local2p free:
  Local2P cookie check
  state transition
  owner-local or remote recycle
```

This is the first cookie consolidation step. It does not remove the Local2P
cookie check; it removes the extra generic wrapper-cookie check from the
Local2P fast path.

### Reuse-State-Only Candidate

`hz5-linux-local2p-reuse-state-only` keeps route-cookie and optimizes only TLS
cache hits:

```text
TLS hit:
  wrapper prefix already initialized
  Local2P owner/generation/cookie already initialized
  store state = ACTIVE
  return aligned user pointer

non-TLS source:
  full Local2P attribution init
```

Generation is not a temporal-safety boundary because the user pointer does not
carry the generation. The candidate keeps sequential double-free-before-reuse
detection through the state field while avoiding redundant owner/cookie writes
on owner-local reuse.

### Slim-Check Candidate

`hz5-linux-local2p-slim-check` keeps reuse-state-only and reduces duplicate
free-time checks after direct Local2P decode:

```text
direct decode:
  magic/layout/source/raw checks

local2p free:
  Local2P cookie check
  state transition
  owner-local or remote recycle
```

The generic `source/requested/raw_bytes` checks are skipped only in this
candidate because the exact route has already been narrowed by direct decode and
the Local2P cookie still includes the raw bytes value.

### Fast-Cookie Candidate

`hz5-linux-local2p-fast-cookie` keeps slim-check and changes only the Local2P
cookie mix:

```text
full cookie:
  raw, aligned, raw_bytes, generation, owner, process secret

fast cookie:
  raw, aligned, process secret
```

This keeps a route-local corrupted-cookie guard while removing fields that are
already invariant on owner-local TLS reuse. Treat it as a speed candidate until
the final safety contract is written.

Initial RUNS=10 result:

```text
private/raw-results/linux/local2p_fastcookie_runs10

local:
  fast-cookie 206.2M ops/s
  slim-check  197.1M ops/s
  tcmalloc    250.0M ops/s

mixed:
  fast-cookie 215.3M ops/s
  slim-check  204.3M ops/s
  tcmalloc    265.5M ops/s
```

Remote-free remains a separate lane: fast-cookie measured 8.13M pairs/s versus
12.52M for the P25 control in the same run.

### Free-First Candidate

`hz5-linux-local2p-free-first` keeps the fast-cookie Local2P route and changes
only `hz5_free()` dispatch order:

```text
before:
  P1/P2 ownership check
  Local2P direct decode

candidate:
  Local2P direct decode
  P1/P2 ownership check
```

This isolates the cost of the generic ownership branch in the standalone
Local2P speed lane. It does not change cookie, state, owner, remote-free, or
RSS behavior.

Initial RUNS=10 result:

```text
private/raw-results/linux/local2p_freefirst_runs10

local:
  fast-cookie 210.3M ops/s
  free-first  210.0M ops/s

mixed:
  free-first  217.6M ops/s
  fast-cookie 210.0M ops/s
```

Treat free-first as a mixed-speed candidate. It did not improve pure local
median and slightly regressed remote-free in the same run.

The runner also exposes this same build as
`hz5-local2p-freefirst-fastcookie`. The longer name is preferred for new A/B
tables because it makes the compound nature explicit: fast-cookie is still the
route/cookie policy, and free-first changes only dispatch order.

Follow-up RUNS=10 result:

```text
private/raw-results/linux/local2p_freefirst_fastcookie_runs10

local:
  freefirst-fastcookie 205.1M ops/s
  fast-cookie          200.4M ops/s
  tcmalloc             254.6M ops/s

mixed:
  fast-cookie          204.6M ops/s
  freefirst-fastcookie 202.3M ops/s
  tcmalloc             270.4M ops/s

remote pairs/s:
  remotebatch          15.26M
  p25                  12.28M
  fast-cookie           8.24M
  freefirst-fastcookie  8.03M
```

Decision: keep `fast-cookie` as the local/mixed reference and `remotebatch` as
the remote-free reference. `freefirst-fastcookie` is a named A/B row, not a
promotion.

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
