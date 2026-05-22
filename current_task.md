# Current Task: HZ5 Ubuntu Standalone Exact Lane

## Goal

Build an Ubuntu/Linux development lane for HZ5 that can be moved to native
Ubuntu later for paper-facing measurement.

The target lane is standalone and fallback-free:

- do not merge to `main`
- do not depend on HZ3 fallback
- do not use process-wide `LD_PRELOAD` as the primary HZ5 measurement path
- call the HZ5 API directly from a dedicated benchmark binary
- route only supported exact HZ5 cases, starting with `64K align=8192`

## Branch

Use:

```bash
codex/hz3-rss-largepath-research
```

Current reference commit:

```bash
b4051ed Wrap no-HZ3 CRT fallback allocations
```

## Phase 1: WSL2 Development

Status: initial implementation complete.

Purpose:

- make Linux compilation work
- add a standalone HZ5 build entrypoint
- add a direct HZ5 benchmark entrypoint
- keep commands reproducible for later native Ubuntu rerun

The WSL2 numbers are not paper results.

## Phase 2: Native Ubuntu Measurement

Status: pending.

Purpose:

- rerun the same commands on native Ubuntu
- use `RUNS=10` median
- compare against glibc, mimalloc, tcmalloc, hz3, and hz4 where the workload is
  semantically comparable

## Initial Workloads

Start with:

- exact `64K align=8192` allocation/free
- exact `4K/8K align=8192` smoke
- mixed small/mid/large only after unsupported routes are explicitly defined
- RSS plateau after the standalone exact lane is stable

## Implementation Notes

- HZ5 lowpage64 has Windows-specific pieces; port only the minimum needed for
  the Linux exact lane first.
- P43 segment-slot paths still contain Windows APIs and should remain out of
  the first standalone lane unless explicitly ported.
- Unsupported routes should fail closed in standalone mode instead of falling
  back to HZ3 or CRT.

## Implemented In This Pass

- `linux/build_linux_hz5_standalone.sh`
  - builds `hakozuna-hz5/out/linux/<arch>/libhakozuna_hz5_standalone.so`
  - builds `hakozuna-hz5/out/linux/<arch>/bench_hz5_standalone_aligned64k`
  - builds the generic comparison binary `bench/out/linux/<arch>/bench_aligned64k`
- `linux/run_linux_hz5_standalone_compare.sh`
  - compares HZ5 standalone against glibc, mimalloc, and tcmalloc
  - defaults to exact `4096:8192,8192:8192,65536:8192`
  - accepts `--cases size:align,...` for explicit exact-route sweeps
  - accepts `--size N --align N` for a single-case compatibility path
  - accepts `--allocators` for optional `hz3` / `hz4` LD_PRELOAD comparison
  - records per-run logs under `private/raw-results/linux/`
  - writes `results.tsv` with case, allocator, run, and log path
- `bench/bench_hz5_standalone_aligned64k.c`
  - calls `hz5_aligned_alloc()` / `hz5_free()` directly
- `BENCHLAB_HZ5_STANDALONE_EXACT_ONLY`
  - makes unsupported HZ5 routes return `NULL`
  - prevents fallback to HZ3 or CRT allocation paths in standalone mode

## Verified On WSL2 Only

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1000 65536 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 4096 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 8192 8192
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 1 \
  --threads 1 --iters 1000 \
  --skip-build --skip-prepare-allocators
```

Native Ubuntu first pass:

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192
```

Optional HZ3/HZ4 inclusion after building those lanes:

```bash
./linux/build_linux_release_lane.sh
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192 \
  --allocators hz5,system,hz3,hz4,mimalloc,tcmalloc
```

Fail-closed checks:

```bash
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 65537 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 1024 16
```

Both reject unsupported routes instead of falling back.
