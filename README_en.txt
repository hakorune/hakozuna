Hakozuna Public Repository Guide (English)
==========================================

This public repository contains two stable allocator implementations and one
research sidecar:

- hakozuna/     : hz3 (optimized for local-heavy workloads)
- hakozuna-mt/  : hz4 (optimized for remote-heavy, high-thread workloads)
- hakozuna-hz5/: HZ5 Linux research sidecar for low-RSS, fail-closed,
                 descriptor-owned profile families

HZ6 is a future-work name only. It would be a transfer-first successor line if
we decide to pursue broader tcmalloc-like class-transfer throughput.

Platform support
----------------

- Ubuntu/Linux public entrypoints: linux/
- Windows-native public entrypoints: win/
- Windows build and benchmark guide: docs/WINDOWS_BUILD.md
- HZ5 Linux exact-profile runner: linux/run_linux_hz5_local2p_focus.sh
- HZ5 Linux general profile runner: linux/run_hz5_hakmem_compare.sh

Recommended profile selection
-----------------------------

- If unsure, start with hz3
- Use hz3 for Redis-like and local-heavy patterns
- Use hz4 for cross-thread free heavy (remote-heavy) patterns
- Treat HZ5 as a research profile family, not as the default allocator

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

HZ5 Linux profile family
------------------------

HZ5 now has two paper-facing scopes.

1. Exact Local2P appendix rows for explicit 64K/a8192 route specialization:

- hz5-local2p-linkflags: low-final-RSS local/mixed speed profile
- hz5-local2p-rssretain2048tls: retained-cache RSS-throughput profile
- hz5-local2p-remotebatch: producer/consumer remote-free profile

2. Linux full-preload general-profile rows for low-RSS, fail-closed,
descriptor-owned allocation:

- hz5-pagerun64-main / hz5-pagerun64-cross128: MidPage PageRun64 profiles
- hz5-large128-rss: low-RSS LargeFront profile
- hz5-large128-source16: broad LargeFront 128K throughput lane
- hz5-large128-transfer128: transfer-cache diagnostic lane

HZ5 should be described as a profile family. It is strong on several
mid/main/cross remote-pressure rows with much lower RSS than tcmalloc, but it is
not a single universal replacement for hz3/hz4 or tcmalloc.

HZ6 future branch
-----------------

HZ6 is a possible future transfer-first design:

- class transfer caches are first-class boxes, not diagnostics layered on owner
  inboxes
- RSS governance is part of the control plane
- any learning/policy layer must stay off the malloc/free hot path
- strict-safety and speed-profile contracts may need to be separated explicitly

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
