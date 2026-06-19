# Benchmark Results Index

This page is the public pointer list for dated benchmark summaries.

## Current Summaries

- [Linux x86_64 HZ6 remote backpressure capacity, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_BACKPRESSURE_CAPACITY.md)
- [Linux x86_64 HZ6 remote backpressure drain, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_BACKPRESSURE_DRAIN.md)
- [Linux x86_64 HZ6 remote backpressure origin transfer, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_BACKPRESSURE_ORIGIN_TRANSFER.md)
- [Linux x86_64 HZ6 remote backpressure origin transfer reasons, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_BACKPRESSURE_ORIGIN_TRANSFER_REASONS.md)
- [Linux x86_64 HZ6 remote free backpressure policy, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_FREE_BACKPRESSURE_POLICY.md)
- [Linux x86_64 HZ6 remote free commit status, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_FREE_COMMIT_STATUS.md)
- [Linux x86_64 HZ6 remote free status dispatch, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_FREE_STATUS_DISPATCH.md)
- [Linux x86_64 HZ6 route rehome probes, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_ROUTE_REHOME_PROBES.md)
- [Linux x86_64 HZ6 remote free overflow, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_FREE_OVERFLOW.md)
- [Linux x86_64 HZ6 remote free consumer rehome, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_FREE_CONSUMER_REHOME.md)
- [Linux x86_64 HZ6 preload foreign resolved dispatch, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_PRELOAD_FOREIGN_RESOLVED_DISPATCH.md)
- [Linux x86_64 HZ6 remote route baseline, 2026-06-19](./2026-06-19_LINUX_X86_64_HZ6_REMOTE_ROUTE_BASELINE.md)
- [Linux arm64 preload ownership fix results, 2026-03-21](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- [Linux arm64 compare results, 2026-03-20](./2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- [Linux arm64 free-route order gate follow-up, 2026-03-20](./2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md)
- [Mac benchmark results, 2026-03-19](./2026-03-19_MAC_BENCH_RESULTS.md)
- [Windows HZ4 foreign-pointer smoke, 2026-03-21](./windows/apps/20260321_hz4_foreign_probe.md)
- [Windows HZ4 one-shot ownership box, 2026-03-21](./windows/apps/20260321_hz4_win_one_shot_ownership.md)
- [Windows paper-aligned bench, 2026-03-21](./windows/paper/20260321_044818_paper_windows.md)
- [Windows Redis profile matrix, 2026-03-21](./windows/apps/20260321_044828_redis_real_windows_profile_matrix.md)
- [Windows large-payload R90 allocator compare, 2026-05-12](./windows/apps/20260512_larger_payload_r90_allocator_compare.md)
- [Windows HZ3 vs HZ4 large-payload R90 repeat-5, 2026-05-12](./windows/apps/20260512_hz3_hz4_large_payload_r90_repeat5.md)
- [Paper benchmark results, 2026-02-18](./2026-02-18_PAPER_BENCH_RESULTS.md)

## Usage

- Write new public summary docs in `docs/benchmarks/` with a dated name.
- Keep raw logs and scratch output in `private/raw-results/`.
- Update this index when a new summary lands.
- Update the GO / NO-GO ledger when a result changes a decision.
- For Linux results, include the CPU architecture (`x86_64` or `arm64`) in the summary title.

## Related Docs

- [GO / NO-GO ledger](./GO_NO_GO_LEDGER.md)
- [Cross-platform benchmark conditions](./CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [Mac benchmark prep order](../MAC_BENCH_PREP.md)
