Hakozuna Public Repository Guide (English)
==========================================

This public repository contains two stable allocator implementations and one
research sidecar:

- hakozuna/     : hz3 (optimized for local-heavy workloads)
- hakozuna-mt/  : hz4 (optimized for remote-heavy, high-thread workloads)
- hakozuna-hz5/: HZ5 research sidecar for exact over-aligned profiles

Platform support
----------------

- Ubuntu/Linux public entrypoints: linux/
- Windows-native public entrypoints: win/
- Windows build and benchmark guide: docs/WINDOWS_BUILD.md
- HZ5 Linux exact-profile runner: linux/run_linux_hz5_local2p_focus.sh

Recommended profile selection
-----------------------------

- If unsure, start with hz3
- Use hz3 for Redis-like and local-heavy patterns
- Use hz4 for cross-thread free heavy (remote-heavy) patterns
- Treat HZ5 as an appendix/research profile, not as the default allocator

Latest benchmark and paper
--------------------------

- Benchmark snapshot: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- Public paper PDF (English): docs/paper/main_en.pdf
- Public paper PDF (Japanese): docs/paper/main_ja.pdf
- Local paper workspace: private/paper/
- The public paper PDFs currently match the v3.2 paper revision.
- v3.3 is a source/artifact release centered on Linux arm64 coverage and
  ownership-routing bug fixes.
- Latest archived Zenodo record (v3.3): https://zenodo.org/records/19139939
- DOI (v3.3): https://doi.org/10.5281/zenodo.19139939

HZ5 Linux appendix profiles
---------------------------

Current HZ5 Linux results are scoped to exact 64K allocations with 8192-byte
alignment. The reporting rows are:

- hz5-local2p-linkflags: low-final-RSS local/mixed speed profile
- hz5-local2p-rssretain2048tls: retained-cache RSS-throughput profile
- hz5-local2p-remotebatch: producer/consumer remote-free profile

These rows should not be merged into a single general-purpose claim.

Minimal usage examples
----------------------

1) hz3

cd hakozuna/hz3
make clean all_ldpreload_scale
LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app

2) hz4

cd ../hz4
make clean all
LD_PRELOAD=./libhakozuna_hz4.so ./your_app

Notes
-----

- See PROFILE_GUIDE.md for profile details
- README.md is the primary entry point for public documentation
