Hakozuna Public Repository Guide (English)
==========================================

This public repository contains two allocator implementations:

- hakozuna/     : hz3 (optimized for local-heavy workloads)
- hakozuna-mt/  : hz4 (optimized for remote-heavy, high-thread workloads)

Platform support
----------------

- Ubuntu/Linux public entrypoints: linux/
- Windows-native public entrypoints: win/
- Windows build and benchmark guide: docs/WINDOWS_BUILD.md

Recommended profile selection
-----------------------------

- If unsure, start with hz3
- Use hz3 for Redis-like and local-heavy patterns
- Use hz4 for cross-thread free heavy (remote-heavy) patterns

Latest benchmark and paper
--------------------------

- Benchmark snapshot: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- Public paper PDF (English): docs/paper/main_en.pdf
- Public paper PDF (Japanese): docs/paper/main_ja.pdf
- Local paper workspace: private/paper/
- The public paper PDFs currently match the v3.2 paper revision.
- v3.3 is a source/artifact release centered on Linux arm64 coverage and
  ownership-routing bug fixes.
- Latest archived Zenodo record (v3.2): https://zenodo.org/records/19120414
- DOI (v3.2): https://doi.org/10.5281/zenodo.19120414

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
