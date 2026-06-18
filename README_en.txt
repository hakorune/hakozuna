Hakozuna Public Repository Guide (English)
==========================================

This public repository contains two stable allocator implementations and two
research allocator families:

- hakozuna/     : hz3 (optimized for local-heavy workloads)
- hakozuna-mt/  : hz4 (optimized for remote-heavy, high-thread workloads)
- hakozuna-hz5/: HZ5 Linux research sidecar for low-RSS, fail-closed,
                 descriptor-owned profile families
- hakozuna-hz6/: HZ6 selected-family allocator prototype with route safety,
                 explicit ownership states, and speed/RSS profile lanes

Allocator profile map
---------------------

Hakozuna contains four allocator lines with deliberately different metadata
and ownership models.

| Line | Focus | Metadata / routing model | Best read as |
|------|-------|--------------------------|--------------|
| HZ3 / ACE-Alloc | local-heavy allocation, compact fast path | lookup-first: PTAG32 / table-oriented pointer-to-bin routing | main ACE-Alloc line |
| HZ4 | remote-heavy / message-passing workloads | remote-free-first: page-local metadata, remote queues, pending collect | remote-free experiment line |
| HZ5 | page/run-first sidecar allocator prototype | ownership/policy-first: page/run descriptors route owner, profile, and dispatch policy | low-RSS fail-closed research line |
| HZ6 | balanced speed/RSS with explicit safety contracts | RouteLayer + descriptor + SourceLayer + FrontCache | selected-family successor line |

In short:

- HZ3 is lookup-first.
- HZ4 is remote-free-first.
- HZ5 is ownership/policy-first.
- HZ6 is contract-first with selected/default and profile-only lanes.

The API is still malloc/free, but allocator behavior changes sharply depending
on how free(ptr) recovers pointer identity and where ownership is sent next.

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
- HZ5-inclusive MT snapshot: RUNS=10 / T=16 / Ubuntu native / 2026-05-26
- Public paper PDF (English): docs/paper/main_en.pdf
- Public paper PDF (Japanese): docs/paper/main_ja.pdf
- Local paper workspace: private/paper/
- Latest hz3/hz4 archived Zenodo record (v3.4):
  https://zenodo.org/records/20753903
- DOI for hz3/hz4 v3.4:
  https://doi.org/10.5281/zenodo.20753903
- All-version DOI for the hz3/hz4 ACE-Alloc artifact series:
  https://doi.org/10.5281/zenodo.18305952
- HZ5 archived Zenodo record (3.5-hz5):
  https://zenodo.org/records/20753950
- DOI for HZ5 3.5-hz5:
  https://doi.org/10.5281/zenodo.20753950
- All-version DOI for the HZ5 sidecar allocator series:
  https://doi.org/10.5281/zenodo.20411597
- HZ6 archived Zenodo record:
  https://zenodo.org/records/20753968
- DOI for HZ6:
  https://doi.org/10.5281/zenodo.20753968
- All-version DOI for the HZ6 selected-family allocator series:
  https://doi.org/10.5281/zenodo.20753967

Representative MT snapshot
--------------------------

The table uses a same-machine, same-runner RUNS=10 / T=16 rerun. HZ5 is shown
as "Best HZ5" because it is a profile family, not one default allocator.

| Lane | hz3 | hz4 | mimalloc | tcmalloc | Best HZ5 | HZ5 row |
|------|-----|-----|----------|----------|----------|---------|
| main_r0 | 292.15M | 85.63M | 146.73M | 318.82M | 157.44M | hz5-pagerun64-main |
| main_r50 | 31.46M | 62.32M | 14.26M | 64.87M | 79.43M | hz5-large128-transfer128 |
| main_r90 | 22.31M | 67.14M | 7.72M | 45.42M | 62.31M | hz5-pagerun64-cross128 |
| guard_r0 | 318.98M | 156.68M | 258.19M | 375.71M | 149.00M | hz5-pagerun64-main |
| cross128_r90 | 2.78M | 27.66M | 3.52M | 7.21M | 22.39M | hz5-large128-transfer128 |

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

HZ6 Windows selected-family snapshot
------------------------------------

HZ6 is reported separately from the Ubuntu MT table because it uses
Windows-native selected-family runners and profile-specific lanes.

| Lane | Selected HZ6 row | ops/s | Peak RSS |
|------|------------------|------:|---------:|
| random_mixed small | sameownerfast-descavail-noboost-route4k | 45.755M | 4,968 KB |
| random_mixed medium | sameownerfast-descavail-noboost-route4k | 42.408M | 4,964 KB |
| random_mixed mixed | sameownerfast-descavail-noboost-route4k | 41.306M | 4,964 KB |
| mixed_ws balanced | mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry | 66.922M | 111,244 KB |
| mixed_ws wide_ws | mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry | 21.853M | 140,708 KB |

Selected-small decision evidence promotes SourceBlockRoute dynmap on selected
rows such as balanced (+8.46%) and large_slice_16k (+16.48%), while preserving
the explicit selected-family/profile-only separation.

HZ6 selected-family branch
--------------------------

HZ6 is an experimental selected-family allocator prototype:

- route safety, descriptor ownership, SourceLayer, and FrontCache contracts are
  first-class parts of the design
- selected/default and profile-only lanes are intentionally separated
- RSS governance and balanced speed/RSS profiles are part of the evaluation
- benchmark results are workload- and platform-specific evidence, not a
  universal allocator ranking
- current design notes live under hakozuna-hz6/

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
