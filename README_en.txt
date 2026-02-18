Hakozuna Public Repository Guide (English)
==========================================

This public repository contains two allocator implementations:

- hakozuna/     : hz3 (optimized for local-heavy workloads)
- hakozuna-mt/  : hz4 (optimized for remote-heavy, high-thread workloads)

Recommended profile selection
-----------------------------

- If unsure, start with hz3
- Use hz3 for Redis-like and local-heavy patterns
- Use hz4 for cross-thread free heavy (remote-heavy) patterns

Latest benchmark and paper
--------------------------

- Benchmark snapshot: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- Paper (Japanese): docs/paper/main_ja.pdf
- Paper (English): docs/paper/main_en.pdf
- Zenodo v3.0: https://zenodo.org/records/18674502
- DOI: https://doi.org/10.5281/zenodo.18674502

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
