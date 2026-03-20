# Benchmarks

This directory collects benchmark results and the notes needed to reproduce
them.

## Start Here

- [Linux arm64 preload ownership fix results, 2026-03-21](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- [Linux arm64 compare results, 2026-03-20](./2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- [Linux arm64 free-route order gate follow-up, 2026-03-20](./2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md)
- [Linux arm64 lane map](./linux/ARM64_LANE_MAP.md)
- [Linux arm64 profiling guide](./linux/ARM64_PROFILING.md)
- [GO / NO-GO ledger](./GO_NO_GO_LEDGER.md)
- [Cross-platform benchmark conditions](./CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [Benchmark results index](./INDEX.md)
- [Mac design boxes](../MAC_DESIGN_BOXES.md)
- [Mac build notes](../MAC_BUILD.md)
- [Mac benchmark prep order](../MAC_BENCH_PREP.md)
- [Mac benchmark results, 2026-03-19](./2026-03-19_MAC_BENCH_RESULTS.md)

## Platform Entrypoints

Use the platform entry docs when you need the build/run front door for a
specific lane:

- [Linux entrypoints](../../linux/README.md)
- [Linux arm64 lane map](./linux/ARM64_LANE_MAP.md)
- [Linux arm64 profiling guide](./linux/ARM64_PROFILING.md)
- [macOS entrypoints](../../mac/README.md)
- [Windows entrypoints](../../win/README.md)

## Before You Implement

- Write the conditions first, then implement.
- Freeze the workload shape before changing allocator code.
- Keep OS-specific launch details in the platform matrix, not inside the hot path.

## Optimization Queue

Next optimization steps, in order:

1. B70 chunk-pages on the segment-registry lane set.
   - Current live default: `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=16`
   - `main_r50` keeps a tiny throughput edge at `chunk4`, but `cross64_r90` and the lock/refill counters prefer `chunk16`
2. Research the Mac paper-suite outliers:
   - `hz4` `malloc-large` large-path box (large extent cache band/cap A/B)
   - segment-registry high-remote fallback box (slot-count A/B at `32768` vs `65536`)
   - Keep `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` parked until those two boxes are understood
3. Use `docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md` as the shared condition ledger before adding new OS-specific runs
4. Use `docs/MAC_DESIGN_BOXES.md` for Mac-only tuning boxes before promoting anything to a shared default

## Current Mac Benchmark Takeaways

- `bench_mixed_ws` is smoke-only.
- `redis-like` is a good first allocator comparison once smoke is green, and its median rerun is now captured in the Mac results doc.
- The first Mac `larson` pass is still a smoke shape; use the canonical SSOT shape before concluding anything.
- The canonical larson rerun is the one to trust for cross-platform comparison.
- `bench_random_mixed_mt_remote_malloc` uses `ring_slots=262144` as the current M1 Mac tuning value to avoid fallback noise.
- MT remote logs are especially useful because they expose `ring_full_fallback`, `overflow_sent`, `overflow_received`, and `[EFFECTIVE_REMOTE]`.
- `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1` is the current promising MT A/B for the segment-registry lane set; on `guard_r0`, `main_r50`, and `cross64_r90` it collapses `large_validate_calls`, with the biggest win on `cross64_r90`.
- `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=16` is the current B70 live default for the segment-registry lane set; `main_r50` keeps a tiny throughput edge at `chunk4`, but `cross64_r90` and the lock/refill counters both prefer `chunk16`.
- Fresh reruns after the B70 promotion moved `hz4` ahead of `mimalloc` and `tcmalloc` on canonical Larson and MT remote; the direct B37 rerun on the live `chunk16` default was far slower, so B37 is historical on the new baseline.
- `HZ4_MID_PREFETCHED_BIN_HEAD_BOX=1` was the promising pre-B70 Mac Larson mid candidate and the canonical MT small-path candidate; keep its small-path story separate from the segment-registry free-route box, and do not treat it as the next Larson mid default on top of `chunk16`.
- `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` is still a live mid candidate, but it is now parked behind the two paper-suite outlier boxes: `hz4 malloc-large` and the segment-registry high-remote fallback lane.
- The current-tree B37 sweep shows `PREV_SCAN_MAX=2` is a cross/high-remote specialist on the live `chunk16` baseline, while `PREV_SCAN_MAX=1` is no-go. Keep B37 separate from both the live `chunk16` default and the `FREE_BATCH_CONSUME_MIN=2` lane-specific candidate.
- `mimalloc-bench` subset should start with `cache-thrash`, `cache-scratch`, and `malloc-large`; the `malloc-large` follow-up is now an extent-cache band/cap A/B, not a new runner.

## Mac-Specific Knobs

- MT remote: use `ring_slots=262144` for the current M1 Mac tuning pass.
- Allocator switching: use `DYLD_INSERT_LIBRARIES`.
- Timing on macOS: prefer `python3` + `time.time_ns()` in shell wrappers.

## Where New Results Go

- Add new platform-wide summaries here when they matter for allocator comparisons.
- Add per-run artifacts under a dated file in this directory.
- Update [Benchmark results index](./INDEX.md) when a new dated summary lands.
- Put lane status and keep/freeze/reject calls in [GO / NO-GO ledger](./GO_NO_GO_LEDGER.md).
- Keep platform tuning notes in `docs/MAC_BUILD.md` and `docs/MAC_BENCH_PREP.md`.

## Current Linux arm64 Note

- The Linux arm64 default path now carries the registry-backed preload
  ownership fix documented in
  [2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md).
- The arm64 `order-gate` lane is stable again, but it is not the current
  promotion target for the mixed compare workload.
