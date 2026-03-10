# Windows Memcached Minimal Main

This document is the SSOT for the Windows-native minimal entrypoint box for
`memcached`.

## Goal

Avoid porting the full Unix `main()` all at once.

Instead:

1. keep the upstream Unix `main()` intact for non-Windows lanes
2. exclude it from the Windows-native minimal-main translation unit
3. reuse the server initialization path from the same source body

## Public Files

- [win/memcached/memcached_win_min_main.c](/C:/git/hakozuna-win/win/memcached/memcached_win_min_main.c)
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_smoke.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_smoke.ps1)
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [win/run_win_memcached_min_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_matrix.ps1)
- [win/run_win_memcached_min_mixed.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_mixed.ps1)
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [win/run_win_memcached_benchmark_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client_matrix.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/probe_win_memtier_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memtier_msvc.ps1)
- [win/probe_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/probe_win_memcached_min_main.ps1)
- [win/probe_win_memcached_min_link.ps1](/C:/git/hakozuna-win/win/probe_win_memcached_min_link.ps1)

## Current Shape

The Windows minimal main currently supports only:

- `-p`
- `-U`
- `-t`
- `-m`
- `-l`
- `-v`
- `-h`

The first target is still:

- TCP first
- ASCII first
- no daemon mode
- no Unix signal control beyond `SIGINT` and `SIGTERM`
- no Unix-style `getopt()` path

## Current Result

The wrapper translation unit compile-probes successfully:

- [private/raw-results/windows/memcached/compile_probe/20260309_230711_memcached_win_min_main_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_230711_memcached_win_min_main_clang_cl.log)

Current output is warnings only:

- deprecated `_getpid` / `unlink` aliases
- one Winsock `getsockopt()` pointer-type warning inherited from upstream code
- unused helper warnings from the excluded Unix `main()` path

The manual first-link check also succeeds and produces:

- [manual_memcached_win_min_main.exe](/C:/git/hakozuna-win/private/raw-results/windows/memcached/link_probe/manual_memcached_win_min_main.exe)
- [out_win_memcached/memcached_win_min_main.exe](/C:/git/hakozuna-win/out_win_memcached/memcached_win_min_main.exe)

With `C:\vcpkg\installed\x64-windows\bin` added to `PATH`:

- `manual_memcached_win_min_main.exe -h` exits `0`
- the minimal usage text prints correctly
- a short boot stays alive without the old `max_fds` assertion
- TCP ASCII smoke now answers `VERSION 1.6.40`
- a basic `set/get` smoke now returns `STORED` and `VALUE ... END`
- the repo-level smoke runner now writes:
  - [docs/benchmarks/windows/apps/20260309_231519_memcached_min_smoke.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_231519_memcached_min_smoke.md)
  - [private/raw-results/windows/memcached/smoke/20260309_231519_memcached_min_smoke.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/smoke/20260309_231519_memcached_min_smoke.log)
- the repo-level throughput runner now writes:
  - [docs/benchmarks/windows/apps/20260309_232133_memcached_min_throughput.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_232133_memcached_min_throughput.md)
  - [private/raw-results/windows/memcached/throughput/20260309_232133_memcached_min_throughput.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/throughput/20260309_232133_memcached_min_throughput.log)
- the repo-level matrix runner now writes:
  - [docs/benchmarks/windows/apps/20260309_232923_memcached_min_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_232923_memcached_min_matrix.md)
  - [private/raw-results/windows/memcached/matrix/20260309_232923_memcached_min_matrix.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/matrix/20260309_232923_memcached_min_matrix.log)
- the repo-level mixed runner now writes:
  - [docs/benchmarks/windows/apps/20260309_233226_memcached_min_mixed.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_233226_memcached_min_mixed.md)
  - [private/raw-results/windows/memcached/mixed/20260309_233226_memcached_min_mixed.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/mixed/20260309_233226_memcached_min_mixed.log)
- the repo-level benchmark-client runner now writes:
  - [docs/benchmarks/windows/apps/20260309_233609_memcached_benchmark_client.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_233609_memcached_benchmark_client.md)
  - [private/raw-results/windows/memcached/benchmark_client/20260309_233609_memcached_benchmark_client.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/benchmark_client/20260309_233609_memcached_benchmark_client.log)
- the repo-level benchmark-client matrix runner now writes:
  - [docs/benchmarks/windows/apps/20260310_001355_memcached_benchmark_client_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_001355_memcached_benchmark_client_matrix.md)
  - [private/raw-results/windows/memcached/benchmark_client_matrix/20260310_001355_memcached_benchmark_client_matrix.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/benchmark_client_matrix/20260310_001355_memcached_benchmark_client_matrix.log)
- the native Windows memtier build lane now produces:
  - [out_win_memtier/memtier_benchmark.exe](/C:/git/hakozuna-win/out_win_memtier/memtier_benchmark.exe)
  - [out_win_memtier/memtier_benchmark_build.log](/C:/git/hakozuna-win/out_win_memtier/memtier_benchmark_build.log)
- the first external-client lane now completes end-to-end:
  - [docs/benchmarks/windows/apps/20260310_010443_memcached_external_client.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_010443_memcached_external_client.md)
  - [private/raw-results/windows/memcached/external_client/20260310_010443_memcached_external_client.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/external_client/20260310_010443_memcached_external_client.log)

The runtime blockers closed in this box were:

- Windows `getrlimit()` shim returning `0`, which made `max_fds` invalid
- Windows socket handles being treated as dense POSIX `conns[sfd]` indexes
- Windows socket I/O going through CRT `_read/_write/_close`
- worker notification using `pipe()` instead of a libevent-friendly socketpair

## Next Box

The next box is post-boot stabilization:

1. keep the manual link lane reproducible until the public link script is cleaned up
2. add a repo-level smoke runner for `version` and `set/get`
3. keep the small throughput runner reproducible
4. keep the reusable matrix and mixed runners reproducible
5. keep the first repo-level benchmark-client lane reproducible
6. keep the benchmark-client profile sweep reproducible
7. keep the external-client lane reproducible now that native memtier build is public
8. keep the memtier compile/link probe reproducible
9. then stabilize connection teardown under external client pressure
