# Benchmark Results Index

This page is the public pointer list for dated benchmark summaries.

## Current Summaries

- [Linux arm64 preload ownership fix results, 2026-03-21](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)
- [Linux arm64 compare results, 2026-03-20](./2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- [Linux arm64 free-route order gate follow-up, 2026-03-20](./2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md)
- [Mac benchmark results, 2026-03-19](./2026-03-19_MAC_BENCH_RESULTS.md)
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
