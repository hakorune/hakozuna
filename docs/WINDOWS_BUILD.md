# Windows Build

Windows bring-up for `hz3` lives under the repo-level [`win`](C:\git\hakozuna-win\win) entrypoints.
Those scripts forward to the maintained implementation under [`hakozuna\win`](C:\git\hakozuna-win\hakozuna\win),
so you can always start from the repository root.

## Prerequisites

- LLVM `clang-cl` in `PATH`
- `llvm-lib` or Visual Studio Build Tools `lib.exe`
- `vcpkg` at `C:\vcpkg` or `$env:VCPKG_ROOT`
- `mimalloc:x64-windows`
- `gperftools:x64-windows`

Quick check:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\check_windows_env.ps1
```

Expected package install if needed:

```powershell
C:\vcpkg\vcpkg.exe install mimalloc:x64-windows gperftools:x64-windows
```

## Minimal hz3 build

From the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_min.ps1
```

Minimal sanity configuration:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_min.ps1 -Minimal
```

Optional Windows mmap stats:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_min.ps1 -MmanStats
```

Outputs:

- `hakozuna\out_win\hz3_win.lib`
- `hakozuna\out_win\bench_minimal.exe`

Minimal outputs:

- `hakozuna\out_win_min\hz3_win.lib`
- `hakozuna\out_win_min\bench_minimal.exe`

Run:

```powershell
.\hakozuna\out_win\bench_minimal.exe 4 1000000 16 1024
```

Args: `[threads] [iters_per_thread] [min_size] [max_size]`

## Bench compare

Build CRT, hz3, mimalloc, and tcmalloc variants:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_bench_compare.ps1
```

Outputs:

- `hakozuna\out_win_bench\bench_mixed_ws_crt.exe`
- `hakozuna\out_win_bench\bench_mixed_ws_hz3.exe`
- `hakozuna\out_win_bench\bench_mixed_ws_mimalloc.exe`
- `hakozuna\out_win_bench\bench_mixed_ws_tcmalloc.exe`

Runtime DLLs for mimalloc and tcmalloc are copied next to the built executables.

Example run:

```powershell
.\hakozuna\out_win_bench\bench_mixed_ws_hz3.exe 4 1000000 8192 16 1024
```

## Full allocator suite

Build the Windows comparison set for `hz3`, `hz4`, `mimalloc`, and `tcmalloc`:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_allocator_suite.ps1
```

This prepares:

- `out_win_suite\bench_mixed_ws_crt.exe`
- `out_win_suite\bench_mixed_ws_hz3.exe`
- `out_win_suite\bench_mixed_ws_hz4.exe`
- `out_win_suite\bench_mixed_ws_mimalloc.exe`
- `out_win_suite\bench_mixed_ws_tcmalloc.exe`

Current status on March 9, 2026:

- `crt`, `hz3`, `hz4`, `mimalloc`, and `tcmalloc` build and run on this machine.
- `hz4` uses a Windows-specific aligned allocation/release path in [hakozuna-mt/src/hz4_os.c](/C:/git/hakozuna-win/hakozuna-mt/src/hz4_os.c) to avoid Linux-style partial `munmap()` assumptions on top of `VirtualAlloc`.
- The `hz4` Windows bench is still built with a conservative config and without bench-side `realloc` while bring-up is in progress.

Run the whole suite with one command:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_suite.ps1
```

Optional custom arguments:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_suite.ps1 `
  -Threads 4 -ItersPerThread 1000000 -WorkingSet 8192 -MinSize 16 -MaxSize 1024
```

Run a small matrix of reusable Windows benchmark profiles and save logs plus a Markdown summary:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_matrix.ps1
```

Default profiles:

- `smoke`
- `balanced`
- `wide_ws`
- `larger_sizes`
- `heavy_mixed`

Outputs:

- `docs\benchmarks\windows\YYYYMMDD_HHMMSS_allocator_matrix.md`
- `docs\benchmarks\windows\YYYYMMDD_HHMMSS_<profile>.log`

Run the paper-aligned Windows-native bench condition:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_paper_windows.ps1
```

This reproduces the Windows section in [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L130) with:

- `threads=4`
- `iters=1000000`
- `ws=8192`
- `size=16..1024`
- `RUNS=5`
- median ops/s

Outputs:

- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_windows.md`
- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_windows.log`

Run the paper-aligned Windows-native `SSOT random_mixed` lane:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1
```

This reproduces the Windows-native counterpart of the paper's `SSOT-LDP random_mixed`
section in [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151) with:

- profiles: `small`, `medium`, `mixed`
- `iters=20000000`
- `ws=400`
- `RUNS=10`
- median ops/s

Build artifacts:

- `out_win_random_mixed\bench_random_mixed_crt.exe`
- `out_win_random_mixed\bench_random_mixed_hz3.exe`
- `out_win_random_mixed\bench_random_mixed_hz4.exe`
- `out_win_random_mixed\bench_random_mixed_mimalloc.exe`
- `out_win_random_mixed\bench_random_mixed_tcmalloc.exe`

Outputs:

- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_random_mixed_windows.md`
- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_random_mixed_windows.log`

Run the paper-aligned Windows-native `Larson` lane:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_larson_paper.ps1
```

This reproduces the Windows-native counterpart of the paper's `Larson sweep`
section in [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259) with:

- `runtime=10`
- `min=8`
- `max=1024`
- `chunks=10000`
- `rounds=1`
- `seed=12345`
- thread sweep `1, 4, 8, 16`
- `RUNS=5`
- median ops/s

Build artifacts:

- `out_win_larson\bench_larson_crt.exe`
- `out_win_larson\bench_larson_hz3.exe`
- `out_win_larson\bench_larson_hz4.exe`
- `out_win_larson\bench_larson_mimalloc.exe`
- `out_win_larson\bench_larson_tcmalloc.exe`

Outputs:

- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_larson_windows.md`
- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_larson_windows.log`

Run the paper-aligned Windows-native `MT remote` lane:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_mt_remote_paper.ps1
```

This reproduces the first Windows-native counterpart of the paper's `MT remote`
section in [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199) with:

- `threads=16`
- `iters=2000000`
- `ws=400`
- `size=16..2048`
- `remote_pct=90`
- `ring_slots=65536`
- `RUNS=10`
- median ops/s

Windows `hz3` note:

- `win/build_win_mt_remote_suite.ps1` builds a dedicated `hz3` profile for this lane
- profile: `scale + S97-2 direct-map bucketize + skip_tail_null`
- artifacts are built from [hakozuna/out_win_mt_remote_hz3](/C:/git/hakozuna-win/hakozuna/out_win_mt_remote_hz3) and linked only into the MT remote bench

Build artifacts:

- `out_win_mt_remote\bench_random_mixed_mt_remote_crt.exe`
- `out_win_mt_remote\bench_random_mixed_mt_remote_hz3.exe`
- `out_win_mt_remote\bench_random_mixed_mt_remote_hz4.exe`
- `out_win_mt_remote\bench_random_mixed_mt_remote_mimalloc.exe`
- `out_win_mt_remote\bench_random_mixed_mt_remote_tcmalloc.exe`

Outputs:

- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_mt_remote_windows.md`
- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_mt_remote_windows.log`

Run the first Windows-native app bench lane for the Redis-style workload:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_redis_workload_paper.ps1
```

This reproduces the Windows-native counterpart of the paper's Redis-style
`workload_bench_system` lane in [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L86) with:

- `threads=4`
- `cycles=500`
- `ops=2000`
- `size=16..256`
- `RUNS=5`
- median M ops/sec for `SET`, `GET`, `LPUSH`, `LPOP`, `RANDOM`

Build artifacts:

- `out_win_redis\bench_redis_workload_crt.exe`
- `out_win_redis\bench_redis_workload_hz3.exe`
- `out_win_redis\bench_redis_workload_hz4.exe`
- `out_win_redis\bench_redis_workload_mimalloc.exe`
- `out_win_redis\bench_redis_workload_tcmalloc.exe`

Outputs:

- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_redis_workload_windows.md`
- `docs\benchmarks\windows\paper\YYYYMMDD_HHMMSS_paper_redis_workload_windows.log`

Run the real Windows Redis sanity lane with prebuilt Redis:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_redis_private_layout.ps1 -CopyLegacyPrebuilt
powershell -ExecutionPolicy Bypass -File .\win\run_win_redis_real_sanity.ps1
```

This starts the private Windows Redis binaries from the canonical private path
`private\bench-assets\windows\redis\prebuilt` when available, and falls back to the
legacy `private\hakmem\hakozuna\bench\windows\redis` tree if not yet migrated.
It records an app-level baseline using `redis-benchmark` with:

- `tests=set,get,lpush,lpop`
- `n=100000`
- `clients=50`
- `pipeline=16`
- `RUNS=3`
- median requests/sec

Outputs:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_redis_real_windows_sanity.md`
- `private\raw-results\windows\redis\real_sanity\YYYYMMDD_HHMMSS_redis_real_windows_sanity.log`

Note:

- this is a baseline sanity lane for real Redis on Windows
- allocator matrix work is still split between:
  - real Redis: `system/jemalloc/mimalloc/tcmalloc` via source builds in `private`
  - synthetic Redis-style workload: `crt/hz3/hz4/mimalloc/tcmalloc`

For the source-build matrix, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_redis_private_layout.ps1
powershell -ExecutionPolicy Bypass -File .\win\build_win_redis_source_variant.ps1 -Allocator Jemalloc
powershell -ExecutionPolicy Bypass -File .\win\run_win_redis_real_matrix.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_redis_real_profile_matrix.ps1 -Profiles balanced,lowpipe,kv_only -Runs 3
```

Current status on March 9, 2026:

- the canonical private layout has been validated with `Jemalloc`, `Tcmalloc`, `Mimalloc`, `Hz3`, and `Hz4`
- the current quick end-to-end source-matrix summary is [docs/benchmarks/windows/apps/20260309_195356_redis_real_windows_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_195356_redis_real_windows_matrix.md)
- the richer profile-sweep summaries are:
  - [docs/benchmarks/windows/apps/20260310_145158_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145158_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_145342_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145342_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_145539_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145539_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_154623_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_154623_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_154807_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_154807_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_160740_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_160740_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_161033_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_161033_redis_real_windows_profile_matrix.md)
- raw logs stay under `private\raw-results\windows\redis\real_matrix`
- `hz3/hz4` are wired through Redis `zmalloc` on Windows source builds, with `hz4` using a small `clang-cl`-built helper object to keep the MSVC solution stable

References:

- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)
- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)
- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [win/run_win_redis_real_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_matrix.ps1)

## Memcached Recovery

The next Windows app-bench lane after real Redis is `memcached + memtier_benchmark`.

Current status on March 9, 2026:

- the Linux paper workload and results are documented in the private archive
- `libevent` seed material exists in the private paper snapshot
- upstream `memcached 1.6.40` and `memtier_benchmark 2.2.1` source snapshots are now stored in the canonical private layout
- no Windows-native build files were found in those recovered trees, so the next box is choosing a build path
- the canonical private recovery layout is documented in [docs/WINDOWS_MEMCACHED_RECOVERY.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_RECOVERY.md)
- the first dependency box is now documented in [docs/WINDOWS_MEMCACHED_LIBEVENT.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_LIBEVENT.md)
- the preferred long-term path is now documented as a proper native MSVC lane in [docs/WINDOWS_MEMCACHED_MSVC_PLAN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MSVC_PLAN.md)
- the current platform-shim boundary and probe results are documented in [docs/WINDOWS_MEMCACHED_SHIM.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_SHIM.md)
- the current minimal Windows-native boot lane is documented in [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)
- the current minimal lane now boots, answers `version`, and passes a `set/get` ASCII smoke

Prepare the canonical private layout with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_private_layout.ps1
```

Optionally seed the private layout with the paper `libevent` archive and extracted tree:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_private_layout.ps1 -CopyPaperLibevent
```

Prepare the first native libevent dependency box with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_libevent.ps1 -InstallIfMissing
```

Build the current Windows-native minimal `memcached.exe` lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_memcached_min_main.ps1
```

Run the current Windows-native smoke with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_min_smoke.ps1
```

This currently verifies:

- process boot on Windows native `memcached_win_min_main.exe`
- `version`
- `set/get`

Artifacts:

- `out_win_memcached\memcached_win_min_main.exe`
- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_min_smoke.md`
- `private\raw-results\windows\memcached\smoke\YYYYMMDD_HHMMSS_memcached_min_smoke.log`

Run the next small Windows-native throughput lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_min_throughput.ps1
```

This currently verifies:

- process boot on Windows native `memcached_win_min_main.exe`
- small parallel `set`
- small parallel `get`

Artifacts:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_min_throughput.md`
- `private\raw-results\windows\memcached\throughput\YYYYMMDD_HHMMSS_memcached_min_throughput.log`

Run the next reusable Windows-native matrix lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_min_matrix.ps1
```

This currently sweeps:

- `smoke`
- `balanced`
- `higher_clients`
- `larger_payload`

Artifacts:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_min_matrix.md`
- `private\raw-results\windows\memcached\matrix\YYYYMMDD_HHMMSS_memcached_min_matrix.log`

Run the next slightly more realistic Windows-native mixed lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_min_mixed.ps1
```

This currently verifies:

- process boot on Windows native `memcached_win_min_main.exe`
- mixed ASCII traffic with alternating `set` and `get`
- small parallel clients on the native minimal-main lane

Artifacts:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_min_mixed.md`
- `private\raw-results\windows\memcached\mixed\YYYYMMDD_HHMMSS_memcached_min_mixed.log`

Run the first repo-level Windows-native benchmark-client lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_benchmark_client.ps1
```

This currently verifies:

- process boot on Windows native `memcached_win_min_main.exe`
- warmup preload across a shared keyspace
- random client traffic with a configurable `get/set` ratio
- the first read-heavy app-style benchmark-client lane without requiring `memtier_benchmark`

Artifacts:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_benchmark_client.md`
- `private\raw-results\windows\memcached\benchmark_client\YYYYMMDD_HHMMSS_memcached_benchmark_client.log`

Run the next benchmark-client profile sweep with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_benchmark_client_matrix.ps1
```

This currently sweeps:

- `read_heavy`
- `balanced`
- `write_heavy`
- `larger_payload`

Artifacts:

- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_benchmark_client_matrix.md`
- `private\raw-results\windows\memcached\benchmark_client_matrix\YYYYMMDD_HHMMSS_memcached_benchmark_client_matrix.log`

Run the first external-client lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_external_client.ps1
```

This lane currently:

- starts the native Windows `memcached_win_min_main.exe`
- builds a native Windows `memtier_benchmark.exe` into `out_win_memtier`
- runs the external memtier client against the native Windows `memcached_win_min_main.exe`
- currently completes end-to-end on this machine

Run the external-client profile sweep with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_memcached_external_client_matrix.ps1
```

Run the allocator comparison on top of the stable external-client lane with:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_memcached_allocator_variants.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_memcached_external_client_allocator_matrix.ps1 -SkipBuild -Runs 5
```

Artifacts:

- `out_win_memtier\memtier_benchmark.exe`
- `out_win_memtier\memtier_benchmark_build.log`
- `out_win_memcached_allocators\memcached_win_min_main_{crt,hz3,hz4,mimalloc,tcmalloc}.exe`
- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_external_client.md`
- `private\raw-results\windows\memcached\external_client\YYYYMMDD_HHMMSS_memcached_external_client.log`
- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_external_client_matrix.md`
- `private\raw-results\windows\memcached\external_client_matrix\YYYYMMDD_HHMMSS_memcached_external_client_matrix.log`
- `docs\benchmarks\windows\apps\YYYYMMDD_HHMMSS_memcached_external_client_allocator_matrix.md`
- `private\raw-results\windows\memcached\external_client_allocator_matrix\YYYYMMDD_HHMMSS_memcached_external_client_allocator_matrix.log`

Latest verified result on this machine:

- [docs/benchmarks/windows/apps/20260310_111947_558_memcached_external_client.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_111947_558_memcached_external_client.md)
- `10s / 4x16`
- final `Totals`: `358055.27 123701.67 55324.62 0.17643 0.01500 1.00700 1.02300 32271.08`
- observed progress: `avg: 379941 ops/sec`
- `connection dropped.` count: `0`
- server slot count: `64`
- the decisive Windows fix is in [memcached.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c):
  - Windows `maxconns_fast` can no longer use raw `SOCKET` values as a proxy for connection count
  - the fast reject path now uses `stats_state.curr_conns + stats_state.reserved_fds`
- companion verification reruns:
  - [docs/benchmarks/windows/apps/20260310_111845_985_memcached_external_client.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_111845_985_memcached_external_client.md)
    - `2s / 4x16`
    - `connection dropped.` count: `0`
    - server slot count: `64`
  - [docs/benchmarks/windows/apps/20260310_111921_025_memcached_external_client.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_111921_025_memcached_external_client.md)
    - `2s / 1x64`
    - `connection dropped.` count: `0`
    - server slot count: `64`
- latest profile sweep:
  - [docs/benchmarks/windows/apps/20260310_113133_memcached_external_client_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_113133_memcached_external_client_matrix.md)
  - `balanced_4x16`: `373581.74 ops/s`, drop `0`, slots `64`
  - `read_heavy_4x16`: `322095.91 ops/s`, drop `0`, slots `64`
  - `write_heavy_4x16`: `349574.60 ops/s`, drop `0`, slots `64`
  - `larger_payload_4x16`: `282035.35 ops/s`, drop `0`, slots `64`
- latest allocator comparison on the stable external-client lane:
  - [docs/benchmarks/windows/apps/20260310_123228_memcached_external_client_allocator_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_123228_memcached_external_client_allocator_matrix.md)
  - first stable `RUNS=5` median compare:
    - `balanced_4x16`: `hz3 342971.65`, `tcmalloc 336591.12`, `mimalloc 330056.11`, `hz4 312939.85`, `crt 302875.09`
    - `larger_payload_4x16`: `crt 357877.47`, `tcmalloc 346926.69`, `mimalloc 344732.05`, `hz3 340825.66`, `hz4 336877.55`
    - `scale_8x16`: `hz4 353966.75`, `mimalloc 331862.21`, `hz3 289561.64`, `tcmalloc 254263.66`, `crt 241951.42`
  - expanded `RUNS=3` profile compare:
    - [docs/benchmarks/windows/apps/20260310_132051_memcached_external_client_allocator_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_132051_memcached_external_client_allocator_matrix.md)
    - `balanced_1x64`: `mimalloc 136039.57`, `crt 125940.97`, `tcmalloc 125669.44`, `hz4 120264.29`, `hz3 112373.60`
    - `balanced_2x32`: `crt 255461.16`, `tcmalloc 250997.54`, `hz3 227584.17`, `hz4 217075.04`, `mimalloc 195537.99`
    - `balanced_4x16`: `tcmalloc 518404.07`, `hz3 412289.11`, `crt 367422.84`, `mimalloc 339212.91`, `hz4 329115.28`
    - `read_heavy_4x16`: `mimalloc 605146.89`, `tcmalloc 536538.35`, `hz4 420253.77`, `hz3 412444.43`, `crt 366806.11`
    - `write_heavy_4x16`: `mimalloc 558239.97`, `hz4 526121.36`, `tcmalloc 410896.23`, `hz3 341288.80`, `crt 339538.87`
    - `larger_payload_4x16`: `mimalloc 596192.63`, `hz3 572178.24`, `tcmalloc 562957.37`, `crt 381344.38`, `hz4 372231.26`
    - `long_balanced_4x16`: `hz3 386808.08`, `crt 377573.52`, `hz4 318636.24`, `tcmalloc 285559.86`, `mimalloc 284732.32`
    - `scale_8x16`: `hz3 358347.81`, `crt 294699.57`, `hz4 270541.80`, `mimalloc 266543.85`, `tcmalloc 246079.79`
  - all compared variants in both summaries finished with `connection dropped.=0`
  - `scale_8x16`: `287084.53 ops/s`, drop `0`, slots `128`
- earlier Windows-side hardening still remains in place:
  - Windows memcached socket-slot map updates are mutex-protected in [memcached.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c)
  - Windows memtier socket setup initializes its keepalive / nodelay flag in [shard_connection.cpp](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/client/memtier_benchmark-2.2.1/shard_connection.cpp)
  - Windows memcached socket shims propagate `WSAENOBUFS -> EAGAIN` and related errno mappings in:
    - [win/memcached/include/sys/socket.h](/C:/git/hakozuna-win/win/memcached/include/sys/socket.h)
    - [win/memcached/include/sys/uio.h](/C:/git/hakozuna-win/win/memcached/include/sys/uio.h)
    - [win/memcached/include/unistd.h](/C:/git/hakozuna-win/win/memcached/include/unistd.h)
- current interpretation:
  - the external-client lane is now stable enough for wider profile sweeps
  - the next box is performance/profile expansion, not reconnect-storm triage

References:

- [docs/WINDOWS_MEMCACHED_RECOVERY.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_RECOVERY.md)
- [docs/WINDOWS_MEMCACHED_LIBEVENT.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_LIBEVENT.md)
- [docs/WINDOWS_MEMCACHED_MSVC_PLAN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MSVC_PLAN.md)
- [docs/WINDOWS_MEMTIER_PROBE.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMTIER_PROBE.md)
- [docs/WINDOWS_MEMCACHED_SHIM.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_SHIM.md)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)
- [win/prepare_win_memcached_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_memcached_private_layout.ps1)
- [win/prepare_win_memcached_libevent.ps1](/C:/git/hakozuna-win/win/prepare_win_memcached_libevent.ps1)
- [win/probe_win_memcached_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memcached_msvc.ps1)
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_smoke.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_smoke.ps1)
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [win/run_win_memcached_min_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_matrix.ps1)
- [win/run_win_memcached_min_mixed.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_mixed.ps1)
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [win/run_win_memcached_benchmark_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client_matrix.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_matrix.ps1)
- [win/probe_win_memtier_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memtier_msvc.ps1)
- [win/run_win_memtier_probe_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memtier_probe_matrix.ps1)

## Hook paths

Link-mode hook:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_link.ps1
```

Inject-mode hook:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_inject.ps1
```

Outputs:

- `hakozuna\out_win_link\hz3_win.lib`
- `hakozuna\out_win_link\bench_minimal_link.exe`
- `hakozuna\out_win_inject\hz3_hook_iat.dll`
- `hakozuna\out_win_inject\hz3_injector.exe`
- `hakozuna\out_win_inject\bench_minimal_inject.exe`

Inject-mode run:

```powershell
.\hakozuna\out_win_inject\hz3_injector.exe `
  .\hakozuna\out_win_inject\hz3_hook_iat.dll `
  .\hakozuna\out_win_inject\bench_minimal_inject.exe 4 1000000 16 1024
```

Hook environment variables:

- `HZ3_HOOK_ENABLE=1` or `0`
- `HZ3_HOOK_FAILFAST=1`
- `HZ3_HOOK_LOG_SHOT=1`
- `HZ3_HOOK_IAT_ALL_MODULES=1`
- `HZ3_HOOK_IAT_READY`

## Notes

- Windows compatibility shims live in `hakozuna\win\include`.
- This path is static-link first; Linux `LD_PRELOAD` workflow does not apply directly on Windows.
- If you already have an older local Windows setup, prefer the repo-level `win` scripts above because they track the current source layout.

