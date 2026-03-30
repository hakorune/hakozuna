# Linux Arm64 Profiling

This page records the narrow profiling path for Ubuntu/Linux arm64 allocator
tuning. Keep it separate from Linux x86_64, macOS, and Windows.

Use it when the arm64 compare lane already exists and you want to explain why a
lane is winning or losing.

## Scope

- Platform: Ubuntu/Linux arm64
- Compare binary: `bench/out/linux/arm64/bench_mixed_ws_crt`
- Current compare shape: `4 1000000 8192 16 1024`
- Allocator list: `system,hz3,hz4,mimalloc,tcmalloc`
- Raw logs: `private/raw-results/linux/`

Current status:

- The Linux preload ownership crash is already fixed in the default Linux arm64
  path by the registry-backed ownership check documented in
  [2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](../2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md).
- `HZ4_FREE_ROUTE_ORDER_GATEBOX=1` is stable again on this lane, but it is not
  the current promotion target for the mixed compare workload.

## Workflow

1. Freeze the workload shape in `docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md`.
2. Run the arm64 compare lane once to confirm the ranking.
3. Profile one allocator lane at a time.
4. Start with `perf stat`.
5. If the path is still unclear, use `perf record -g`, then `perf report` and
   `perf annotate`.
6. Move to `strace -c` if syscall time looks dominant.
7. Use `bpftrace` when futex, wakeup, or lock wait behavior needs more detail.
8. Use FlameGraph, `hotspot`, or `heaptrack` only after the first hot path is
   known.

## Build And Prep

```bash
./linux/build_linux_arm64_bench_compare.sh
ENV_FILE="$(mktemp)"
./linux/prepare_linux_bench_allocators.sh --arch arm64 > "$ENV_FILE"
# shellcheck disable=SC1090
source "$ENV_FILE"
rm -f "$ENV_FILE"
```

The prepare step writes `MIMALLOC_SO` and `TCMALLOC_SO` to a small env file
from the local private cache, then the runner sources it.

Resolve the live `hz3` / `hz4` libraries the same way the compare runner does:

```bash
ROOT_DIR="$PWD"
source ./bench/lib/bench_common.sh
HZ3_SO="$(bench_find_allocator_library hz3)"
HZ4_SO="$(bench_find_allocator_library hz4)"
```

## Single-Lane Profiling

Use the shared benchmark binary and preload only one allocator.

```bash
perf stat -o /tmp/hz4_arm64_perf_stat.txt \
  -e task-clock,context-switches,cpu-migrations,page-faults \
  -- env LD_PRELOAD="$HZ4_SO" \
  ./bench/out/linux/arm64/bench_mixed_ws_crt 4 1000000 8192 16 1024
```

If PMU events are available on the machine, you can extend the counter list
with `cycles,instructions,branches,branch-misses`.

```bash
perf record -g -o /tmp/hz4_arm64_perf.data -- \
  env LD_PRELOAD="$HZ4_SO" \
  ./bench/out/linux/arm64/bench_mixed_ws_crt 4 1000000 8192 16 1024

perf report --stdio -i /tmp/hz4_arm64_perf.data
```

For allocator swaps:

- `system`: omit `LD_PRELOAD`
- `hz3`: preload the live `HZ3_SO`
- `hz4`: preload the live `HZ4_SO`
- `mimalloc`: preload `MIMALLOC_SO`
- `tcmalloc`: preload `TCMALLOC_SO`

## What To Inspect First

Current arm64 `hz4` reads point at:

- `hz4_free`
- `hz4_large_header_valid`
- `hz4_os_seg_acquire`
- page-fault and anonymous-page handling

Treat `hz4_free` as the first box. If that does not explain the result, move to
`hz4_large_header_valid`, then `hz4_os_seg_acquire`.

## Guardrails

- Change one box at a time.
- Keep stats-enabled screening and no-stats promotion separate.
- Record CPU architecture and kernel version with every summary.
- Do not promote an arm64 win into Windows x64 or macOS without rerunning
  those lanes.
- Keep the result lane-specific until the other target lanes confirm it.

## Related Docs

- [Profile Guide](../../../PROFILE_GUIDE.md)
- [Linux Entrypoints](../../../linux/README.md)
- [Linux Benchmark Summaries](./README.md)
- [Cross-Platform Benchmark Conditions](../CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [Mac Profiling](../../MAC_PROFILING.md)
- [Windows Profile Boundaries](../../WINDOWS_PROFILE_BOUNDARIES.md)
