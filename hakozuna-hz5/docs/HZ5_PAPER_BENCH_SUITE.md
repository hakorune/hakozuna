# HZ5 Paper Benchmark Suite Plan

This document defines which benchmarks are paper-main, appendix, or diagnostic
for HZ5 work. The goal is to stop benchmark sprawl and keep claims tied to a
stable evaluation tier.

## Rule

Do not mix benchmark tiers in one claim.

- Paper-main answers: "Can this allocator be a general/default profile?"
- Appendix answers: "Does this narrow profile win in its claimed domain?"
- Diagnostic answers: "Why did a lane win or lose?"

## Current HZ5 Constraint

The existing HZ5 Linux lanes are standalone/direct-API lanes:

```text
hz5_aligned_alloc(size, align)
hz5_free(ptr)
```

The existing paper main suite is LD_PRELOAD based:

```text
same benchmark binary + malloc/free/posix_memalign interposition
```

Therefore HZ5 Local2P results are comparable as a controlled appendix profile,
but they must not be inserted into the paper main tables until HZ5 has an
LD_PRELOAD-compatible general allocator lane or an equivalent same-binary
adapter.

## Tiers

### Paper Main

Use this tier for general allocator claims.

Required workloads:

1. MT random-mixed remote matrix
   - `guard`, `main`, `cross128`
   - remote `%`: `0,50,90`
   - `RUNS=10`
   - `taskset -c 0-15`
   - same benchmark binary, allocator selected with LD_PRELOAD

2. App-like Redis
   - memtier Redis lane
   - `RUNS=10`
   - same profile as the paper snapshot

3. RSS / ru_maxrss summary
   - recorded from the paper-main runner when available
   - used to reject speed-only wins that retain too much memory

Current paper-main allocators:

```text
hz3
hz4
mimalloc
tcmalloc
system optional
```

HZ5 enters this tier only after a general LD_PRELOAD lane exists.

### Paper Appendix

Use this tier for narrow HZ5 profile claims.

Required HZ5 appendix workloads:

1. exact local throughput
   - `size=65536`
   - `align=8192`
   - `RUNS=10`

2. exact mixed prelude
   - 64K/a8192 plateau prelude
   - unsupported/source-control probe
   - final 64K/a8192 throughput

3. exact RSS plateau
   - 64K/a8192 live-set plateau
   - peak RSS and final RSS

4. exact producer/consumer remote-free
   - producer allocates
   - consumer frees
   - documents whether the lane is remote-capable

5. guard rows
   - `2048:8192`
   - `4096:8192`
   - `8192:8192`
   - `65536:4096`
   - `65537:16`
   - `262144:4096`
   - `65536:8192` as positive control

Current HZ5 appendix candidates:

```text
hz5-local2p-fast
hz5-local2p
hz5-p25
hz4
tcmalloc
mimalloc
system
```

Interpretation rules:

- `hz5-local2p-fast` may claim local exact `64K/a8192` throughput only.
- It is not a remote-free profile unless a later owner-inbox/remote queue lane
  fixes producer/consumer performance.
- It is not a general allocator profile unless it passes paper-main workloads.

### Diagnostic

Use this tier for implementation decisions only.

Examples:

- trace counters
- perf stat
- unsafe A/B variants
- no-cookie / no-CAS / no-owner-check / no-wrapper-decode
- cap sweeps
- single-run smoke tests

Diagnostic results are not paper claims.

## Canonical Entry Points

### Existing paper main

The paper snapshot lives in the paper/hakmem tree:

```bash
cd /mnt/workdisk/public_share/hakozuna_paper/hakmem
scripts/run_mt_lane_remote_matrix.sh \
  --runs 10 \
  --cpu-list 0-15 \
  --outdir /tmp/mt_lane_remote_matrix_paper_<timestamp>

RUNS=10 \
TARGETS=redis \
ALLOCATORS=hz3,hz4,mimalloc,tcmalloc \
MEMTIER_SECONDS=15 \
OUTDIR=/tmp/realworld_redis_paper_<timestamp> \
hakozuna/scripts/run_realworld_4pack.sh
```

### HZ5 appendix/focus

Run from this repository:

```bash
./linux/run_linux_hz5_local2p_focus.sh \
  --runs 10 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256 \
  --allocators hz5-local2p-fast,hz5-p25,hz4,tcmalloc,mimalloc,system \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/local2p_focus_runs10
```

### One front door

Use:

```bash
./linux/run_paper_allocator_suite.sh --tier appendix-hz5
./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5
```

`paper-main` is documented by the front door but requires the paper/hakmem tree
and currently excludes HZ5 unless an HZ5 LD_PRELOAD lane is provided.

## Current HZ5 Result Classification

As of `4b3ecb7`:

- `hz5-local2p-fast` is HZ4-class on local exact `64K/a8192`.
- It is also HZ4-class on the exact mixed-prelude final throughput.
- It is much weaker on producer/consumer remote-free because remote frees are
  released to the OS in the first Local2P lane.
- It has low final RSS in the plateau test, but low RSS throughput.

Paper wording should be:

```text
HZ5 Local2P is a Linux exact-overaligned local-throughput candidate, not a
general allocator profile.
```

Do not write:

```text
HZ5 is faster than hz3/hz4/tcmalloc generally.
```

