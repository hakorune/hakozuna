# Linux Entrypoints

This directory is the public Ubuntu/Linux entrypoint layer for this repository.

Use it to keep Linux build and smoke commands in one place without mixing:

- allocator core source
- private benchmark assets
- generated outputs

## What Lives Here

- [build_linux_release_lane.sh](build_linux_release_lane.sh): public build wrapper for the current Ubuntu release lane
- [build_linux_arm64_release_lane.sh](build_linux_arm64_release_lane.sh): explicit Ubuntu arm64 build wrapper
- [build_linux_bench_compare.sh](build_linux_bench_compare.sh): build the Linux benchmark compare binary
- [build_linux_arm64_bench_compare.sh](build_linux_arm64_bench_compare.sh): explicit Ubuntu arm64 benchmark build wrapper
- [build_linux_hz6_benchmark.sh](build_linux_hz6_benchmark.sh): build the HZ6-only Linux benchmark binary
- [build_linux_hz5_preload_full.sh](build_linux_hz5_preload_full.sh): build the HZ5 full-preload control lane
- [build_linux_arm64_order_gate_release_lane.sh](build_linux_arm64_order_gate_release_lane.sh): explicit Ubuntu arm64 order-gate build wrapper for experimental tuning
- [run_linux_preload_smoke.sh](run_linux_preload_smoke.sh): minimal `LD_PRELOAD` smoke runner for `hz3` and `hz4`
- [run_linux_bench_compare.sh](run_linux_bench_compare.sh): build, prepare allocators, and run the Linux benchmark compare lane
- [run_linux_bench_compare_matrix.sh](run_linux_bench_compare_matrix.sh): build the Linux allocator matrix for `hz3`, `hz4`, `hz5`, `mimalloc`, and `tcmalloc`
- [run_linux_bench_remeasure_matrix.sh](run_linux_bench_remeasure_matrix.sh): run the compare matrix plus the standalone HZ6 Linux matrix
- [run_linux_arm64_bench_compare.sh](run_linux_arm64_bench_compare.sh): explicit Ubuntu arm64 benchmark compare wrapper
- [run_linux_hz6_benchmark.sh](run_linux_hz6_benchmark.sh): build and run the HZ6-only Linux benchmark matrix
- [run_linux_arm64_order_gate_compare.sh](run_linux_arm64_order_gate_compare.sh): explicit Ubuntu arm64 order-gate compare wrapper for experimental tuning
- [prepare_linux_bench_allocators.sh](prepare_linux_bench_allocators.sh): local `mimalloc` / `tcmalloc` cache prep for benchmark runs
- [run_bench_compare.sh](run_bench_compare.sh): thin Linux frontend for the shared allocator compare runner
- [run_linux_hz5_local2p_focus.sh](run_linux_hz5_local2p_focus.sh): HZ5 exact `64K/a8192` appendix-profile runner

HZ6 also has profile/control LD_PRELOAD DSOs for workload-specific lanes:

- `hakozuna-hz6/linux/build_hz6_preload_toy_target.sh`
- `hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh`
- `hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh`
- `hakozuna-hz6/linux/run_hz6_preload_aligned_wrapper_audit.sh`
- `hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh`
- `hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh`

These are not the selected default HZ6 allocator. In shared allocator matrices,
use allocator name `hz6` for selected default. Use `hz6-toy-target` for
Toy/mid-small workloads, `hz6-aligned-target` for real aligned allocation
fallbacks, `hz6-realloc-boundary-target` for fixed-boundary realloc growth, and
`hz6-small-boundary-target` / `hz6-small-boundary-fast-target` for known
small/fixed-boundary profiles.

## Quick Start

```bash
cd /path/to/hakozuna-win
./linux/build_linux_release_lane.sh
./linux/build_linux_arm64_release_lane.sh
./linux/run_linux_preload_smoke.sh hz3 /bin/true
./linux/run_linux_preload_smoke.sh hz4 /bin/true
./linux/run_linux_bench_compare.sh
./linux/run_linux_bench_compare_matrix.sh
./linux/run_linux_bench_remeasure_matrix.sh
./linux/run_linux_hz6_benchmark.sh --runs 1
./hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh --runs 7
./hakozuna-hz6/linux/run_hz6_preload_aligned_wrapper_audit.sh --runs 3
./hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh
```

## Ubuntu Lane Split

Ubuntu/Linux is one entrypoint layer with two CPU lanes:

- `x86_64` (published reference lane)
- `arm64` (Ubuntu on ARM64, including Apple Silicon Linux VMs)

The same scripts are used for both lanes. Always record the CPU architecture in
benchmark summaries and keep Linux `arm64` results separate from Linux `x86_64`.

## Build Boxes

Default behavior:

- `hz3`: `make -C hakozuna clean all_ldpreload_scale`
- `hz4`: `make -C hakozuna-mt clean all_stable`

For the explicit arm64 lane, use:

```bash
./linux/build_linux_arm64_release_lane.sh
```

For benchmark compare runs on arm64, use:

```bash
./linux/run_linux_arm64_bench_compare.sh
```

For the experimental arm64 free-route order-gate tuning preset, use:

```bash
./linux/run_linux_arm64_order_gate_compare.sh
```

Keep this preset separate from the shared arm64 default.
The Linux arm64 follow-up on 2026-03-21 made it stable again, but the rebuilt
default lane still won on median throughput for the current compare workload.

You can select a different public `hz3` lane by passing a make target:

```bash
./linux/build_linux_release_lane.sh --hz3-target all_ldpreload_scale_r90_pf2_s97_2
```

Examples:

- `all_ldpreload_scale`
- `all_ldpreload_fast`
- `all_ldpreload_scale_r50`
- `all_ldpreload_scale_r90`
- `all_ldpreload_scale_r90_pf2_s97_2`

## HZ5 Exact-Profile Runner

HZ5 Linux results are currently scoped to exact `64K/a8192` profile rows:

- `hz5-local2p-linkflags`: low-final-RSS local/mixed exact speed profile
- `hz5-local2p-rssretain2048tls`: retained-cache RSS-throughput profile
- `hz5-local2p-remotebatch`: producer/consumer remote-free profile

Run the focus matrix with:

```bash
./linux/run_linux_hz5_local2p_focus.sh
```

Keep HZ5 results separate from the default hz3/hz4 public allocator comparison.
HZ5 is a research sidecar, and unsupported exact-only routes must not be counted
as HZ5 wins.

## HZ6 Profile/Control DSO

The default HZ6 Linux `LD_PRELOAD` lane is allocator name `hz6` and is built by:

```bash
./hakozuna-hz6/linux/build_hz6_preload.sh
```

The Toy/mid-small profile DSO is built by:

```bash
./hakozuna-hz6/linux/build_hz6_preload_toy_target.sh
```

It can be compared directly against selected default:

```bash
./hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh --runs 7
```

It can also be named in shared compare matrices as `hz6-toy-target` after it is
built, or through `run_linux_bench_compare_matrix.sh`, which builds it when the
allocator list includes that name. Keep `hz6-toy-target` separate from `hz6`;
it is a profile/control DSO, not the selected default.

The aligned-allocation profile DSO is built by:

```bash
./hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh
```

It enables `HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1=1`, a control that tracks
real `posix_memalign` / `aligned_alloc` fallback pointers so `free()` can skip
the HZ6 route lookup and call real free directly.  It is useful for aligned-heavy
workloads but is not selected default because mixed/fixed guards reject the
extra free-side gate.  Use `hz6-aligned-target` in shared compare matrices only
for that profile.

The realloc-boundary profile DSO is built by:

```bash
./hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh
```

It enables `HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1=1`, a control that maps
4096-byte allocations to the 8K MidPage slot and 8192-byte allocations to the
32K MidPage slot.  This can remove copy-heavy fixed-boundary realloc patterns,
but it is not selected default because broad mixed-small guards do not cleanly
promote it.  Use `hz6-realloc-boundary-target` in shared compare matrices only
for that profile.

The small-boundary profile DSOs are built by:

```bash
./hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh
./hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh
```

The first combines Toy direct fast reuse with realloc-boundary slack.  The
`fast` sibling also enables trusted-owner preload boundary and raw frontcache
push code-shape controls.  Use `hz6-small-boundary-target` or
`hz6-small-boundary-fast-target` only when the workload is known to favor
small/fixed-boundary behavior; neither is the selected default `hz6` lane.

## HZ5 Full-Preload Research Lanes

HZ5 also has Linux full-preload research front-ends for ordinary `malloc`
traffic. These are separate from the exact `64K/a8192` Local2P appendix lanes.

Current front-end split:

- `hz5-smallfront-s1`: ordinary `malloc` up to 2KiB.
- `hz5-midfront-rb16`: ordinary `malloc` 4KiB..64KiB, broad baseline/default MidFront candidate.
- `hz5-midfront-allgate`: MidFront remote-heavy co-lead candidate.
- `hz5-midfront-globalrecycle`: MidFront control lane, not the lead policy.

Build examples:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-midfront-owner-fast-state \
  --linux-midfront-remote-batch-cap 16 \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-midfront-rb16

./linux/build_linux_hz5_standalone.sh \
  --linux-midfront-owner-fast-state \
  --linux-midfront-remote-batch-cap 16 \
  --linux-midfront-drain-all-on-miss \
  --linux-midfront-drain-empty-gated \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-midfront-allgate
```

Run the MidFront observation matrix with:

```bash
./linux/run_linux_hz5_midfront_observe.sh
```

The runner keeps raw performance runs separate from attribution counters:

- `raw.tsv`: `HZ5_PRELOAD_STATS` unset.
- `attrib.tsv`: separate short attribution smoke with `HZ5_PRELOAD_STATS=1`.

Do not mix attribution-counter runs into performance medians.

For cross-allocator paper-main style comparison against the existing `hakmem`
benchmark binary, use:

```bash
./linux/run_hz5_hakmem_compare.sh
```

This keeps the layout separated:

- benchmark binary: `/mnt/workdisk/public_share/hakmem/hakozuna/out/...`
- runner: this repository under `linux/`
- results: this repository under `private/raw-results/linux/`

The `hakmem` tree is treated as an external benchmark asset. Do not copy the
whole `hakmem` tree into this repository.

## GO / NO-GO And Conditions

- Quick GO / NO-GO status: [docs/benchmarks/GO_NO_GO_LEDGER.md](../docs/benchmarks/GO_NO_GO_LEDGER.md)
- Shared workload conditions: [docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md](../docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- Linux arm64 preload ownership fix results: [docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](../docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- Linux arm64 lane map: [docs/benchmarks/linux/ARM64_LANE_MAP.md](../docs/benchmarks/linux/ARM64_LANE_MAP.md)
- Linux arm64 profiling guide: [docs/benchmarks/linux/ARM64_PROFILING.md](../docs/benchmarks/linux/ARM64_PROFILING.md)
- macOS is tracked as a separate Apple Silicon M1 development lane; see [mac/README.md](../mac/README.md) and [docs/MAC_BENCH_PREP.md](../docs/MAC_BENCH_PREP.md)

## Published Ubuntu Lane

Ubuntu/Linux is the already-published public release lane for this repository.

Current public references:

- [README.md](../README.md)
- [docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md](../docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md)
- [docs/benchmarks/linux/README.md](../docs/benchmarks/linux/README.md)
- [bench/README.md](../bench/README.md)

## Historical Notes

Some older docs still mention paths such as `hakozuna/hz3` or runner scripts under `scripts/`.

In this repo snapshot:

- `hz3` builds are driven from [hakozuna/Makefile](../hakozuna/Makefile)
- `hz4` builds are driven from [hakozuna-mt/Makefile](../hakozuna-mt/Makefile)
- public Linux summaries should collect under [docs/benchmarks/linux](../docs/benchmarks/linux)
- private raw logs and vendor trees should stay under [private](../private)

## See Also

- [mac/README.md](../mac/README.md)
- [win/README.md](../win/README.md)
- [docs/REPO_STRUCTURE.md](../docs/REPO_STRUCTURE.md)
