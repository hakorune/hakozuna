# Windows Redis Matrix

This document is the SSOT for the Windows real-Redis app bench layout.

Use it to keep the public scripts and the private Redis assets separated cleanly.

Related policy:

- shared profile names vs Windows-only knob boxes:
  - [docs/WINDOWS_PROFILE_BOUNDARIES.md](/C:/git/hakozuna-win/docs/WINDOWS_PROFILE_BOUNDARIES.md)

## Public Scripts

These live in git and may be committed:

- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)
- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [win/run_win_redis_real_sanity.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_sanity.ps1)
- [win/run_win_redis_real_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_matrix.ps1)
- [win/run_win_redis_real_profile_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_profile_matrix.ps1)

## Canonical Private Layout

Use these private paths as the current SSOT:

- `private/bench-assets/windows/redis/prebuilt`
- `private/bench-assets/windows/redis/source`
- `private/raw-results/windows/redis`

Purpose:

- `prebuilt`: quick sanity baseline with `redis-server.exe` and `redis-benchmark.exe`
- `source`: allocator-aware source build lane
- `raw-results`: local logs that should not be published

Older trees under `private/hakmem/hakozuna/bench/windows` are now legacy fallback paths.

## Recommended Order

1. Prepare private folders
2. Run the prebuilt sanity lane
3. Build a source variant
4. Run the source matrix

## Commands

Prepare the private layout:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_redis_private_layout.ps1
```

Copy legacy prebuilt binaries into the canonical private path:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_redis_private_layout.ps1 -CopyLegacyPrebuilt
```

Build one allocator-aware Redis source variant:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_redis_source_variant.ps1 -Allocator Jemalloc
```

Supported allocator names:

- `Jemalloc`
- `Tcmalloc`
- `Mimalloc`
- `Hz3`
- `Hz4`

Run the real Redis source matrix:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_redis_real_matrix.ps1
```

Run the richer real Redis profile matrix:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_redis_real_profile_matrix.ps1 -Profiles balanced,lowpipe,kv_only -Runs 3
```

Investigate Windows-only `hz4` define boxes without changing the default lane:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\build_win_redis_source_variant.ps1 -Allocator Hz4 -Hz4ExtraDefines @('HZ4_TLS_DIRECT=1')
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_redis_real_profile_matrix.ps1 -Allocators Hz4 -Profiles balanced -Runs 3 -SkipBuild
```

## Output Policy

Keep these public:

- summary markdown under [docs/benchmarks/windows/apps](/C:/git/hakozuna-win/docs/benchmarks/windows/apps)

Keep these private:

- raw Redis benchmark logs under `private/raw-results/windows/redis`
- source trees and prebuilt binaries under `private/bench-assets/windows/redis`

## Current Status

- The prebuilt sanity lane works on this machine.
- The source-build matrix now has public repo-level scripts.
- `Jemalloc`, `Tcmalloc`, `Mimalloc`, `Hz3`, and `Hz4` source builds have been validated from `private/bench-assets/windows/redis/source`.
- Quick matrix summaries have been generated under [docs/benchmarks/windows/apps](/C:/git/hakozuna-win/docs/benchmarks/windows/apps).
- The current end-to-end quick source-matrix reference is [docs/benchmarks/windows/apps/20260309_195356_redis_real_windows_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260309_195356_redis_real_windows_matrix.md).
- The new profile-sweep references are:
  - [docs/benchmarks/windows/apps/20260310_145158_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145158_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_145342_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145342_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_145539_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_145539_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_154623_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_154623_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_154807_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_154807_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_160740_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_160740_redis_real_windows_profile_matrix.md)
  - [docs/benchmarks/windows/apps/20260310_161033_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_161033_redis_real_windows_profile_matrix.md)
- The private asset tree is still in migration from `private/hakmem/...` to `private/bench-assets/...`.
- Shared workload profile names and Windows-only knob probes are now separated in:
  - [docs/WINDOWS_PROFILE_BOUNDARIES.md](/C:/git/hakozuna-win/docs/WINDOWS_PROFILE_BOUNDARIES.md)
- The current Windows `hz4` read is:
  - synthetic knob A/B can help
  - clean real Redis `balanced / RUNS=3` still does not justify flipping the default Windows lane
  - current clean baseline: [docs/benchmarks/windows/apps/20260310_213130_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_213130_redis_real_windows_profile_matrix.md)
  - current clean `HZ4_TLS_DIRECT=1`: [docs/benchmarks/windows/apps/20260310_213410_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_213410_redis_real_windows_profile_matrix.md)
  - current clean `HZ4_MID_PAGE_SUPPLY_RESV_BOX=1`: [docs/benchmarks/windows/apps/20260310_213453_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_213453_redis_real_windows_profile_matrix.md)
  - current clean `HZ4_TLS_DIRECT=1 + HZ4_MID_PAGE_SUPPLY_RESV_BOX=1`: [docs/benchmarks/windows/apps/20260310_215118_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_215118_redis_real_windows_profile_matrix.md)
  - current `HZ4_PAGE_META_SEPARATE=1` follow-up: [docs/benchmarks/windows/apps/20260310_213042_redis_real_windows_profile_matrix.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260310_213042_redis_real_windows_profile_matrix.md)

Current quick source-matrix snapshot:

- `Jemalloc`: `set 249999.98`, `get 333333.34`, `lpush 249999.98`, `lpop 333333.34 req/s`
- `Tcmalloc`: `set 333333.34`, `get 499999.97`, `lpush 499999.97`, `lpop 499999.97 req/s`
- `Mimalloc`: `set 499999.97`, `get 499999.97`, `lpush 333333.34`, `lpop 333333.34 req/s`

Current profile-sweep highlights (`RUNS=3` medians):

- `balanced` (`clients=50`, `pipeline=16`, `requests=100000`)
  - `set`: `mimalloc/tcmalloc 1298701.25`, `jemalloc 1176470.62`
  - `get`: `mimalloc 1515151.50`, `tcmalloc 1470588.12`, `jemalloc 1408450.62`
- `lowpipe` (`clients=40`, `pipeline=1`, `requests=50000`)
  - all allocators are close to `~128k-134k req/s`
- `kv_only` (`clients=80`, `pipeline=16`, `requests=150000`)
  - `set`: `mimalloc 1363636.38`, `tcmalloc 1293103.50`, `jemalloc 1209677.50`
  - `get`: `mimalloc 1546391.75`, `tcmalloc 1515151.50`, `jemalloc 1415094.38`
- `list_only` (`clients=60`, `pipeline=16`, `requests=150000`)
  - `lpush`: `mimalloc 1271186.38`, `tcmalloc 1250000.00`, `jemalloc 1006711.38`
  - `lpop`: `mimalloc 1327433.62`, `tcmalloc 1315789.50`, `jemalloc 1071428.62`
- `highpipe` (`clients=80`, `pipeline=32`, `requests=200000`)
  - `set`: `mimalloc 2040816.38`, `tcmalloc 1941747.62`, `jemalloc 1639344.25`
  - `get`: `mimalloc 2298850.75`, `tcmalloc 2247191.00`, `jemalloc 2000000.00`
  - `lpush`: `mimalloc 1785714.25`, `tcmalloc 1724138.00`, `jemalloc 1360544.25`
  - `lpop`: `mimalloc 1869158.88`, `tcmalloc 1851851.75`, `jemalloc 1492537.25`
- `balanced` quick lane with newly enabled `hz3/hz4` (`RUNS=1`)
  - `hz3`: `set 1219512.12`, `get 1492537.25`, `lpush 1265822.75`, `lpop 1298701.25`
  - `hz4`: `set 1388889.00`, `get 1492537.25`, `lpush 1176470.62`, `lpop 1219512.12`
- `balanced` quick all-allocator compare (`RUNS=1`)
  - `set`: `hz3 1408450.62`, `hz4 1315789.50`, `mimalloc 1265822.75`, `tcmalloc 1250000.00`, `jemalloc 1190476.25`
  - `get`: `hz3/mimalloc 1492537.25`, `tcmalloc 1515151.50`, `hz4 1408450.62`, `jemalloc 1333333.25`
