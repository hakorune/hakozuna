# hakozuna (hz3) / hakozuna-mt (hz4)

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.19120414.svg)](https://doi.org/10.5281/zenodo.19120414)

**High-performance memory allocators competitive with mimalloc and tcmalloc**

Public source release with both Ubuntu/Linux and Windows-native build and benchmark entrypoints.

Part of the [hakorune](https://github.com/hakorune) project.

---

## Variants

- **hz3 (hakozuna)**: Local-heavy performance + minimal RSS footprint. Default for most workloads.
- **hz4 (hakozuna-mt)**: Message-passing, remote-heavy scaling (best at high thread counts).
- Profile selection guide: `PROFILE_GUIDE.md`

## Platform Support

- **Ubuntu/Linux**: public build and preload entrypoints under `linux/`
- **Windows native**: public build and benchmark entrypoints under `win/`
- **macOS**: public build and preload entrypoints under `mac/` (bring-up in progress)
- Windows guide: `docs/WINDOWS_BUILD.md`
- Windows public summaries: `docs/benchmarks/windows/`

## Quick Start

### Ubuntu/Linux

```bash
./linux/build_linux_release_lane.sh
./linux/run_linux_preload_smoke.sh hz3 /bin/true
./linux/run_linux_preload_smoke.sh hz4 /bin/true
```

### macOS

```bash
./mac/check_mac_env.sh
./mac/build_mac_release_lane.sh
./mac/run_mac_preload_smoke.sh hz3 /usr/bin/env true
```

### Windows native

```powershell
powershell -ExecutionPolicy Bypass -File .\win\check_windows_env.ps1
powershell -ExecutionPolicy Bypass -File .\win\build_win_allocator_suite.ps1
powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_matrix.ps1
```

This repository already includes public Windows-native allocator comparisons and paper-aligned benchmark lanes.

## Feedback / Repro Reports

- GitHub Issues are welcome for bugs, performance regressions, and compatibility trouble.
- Issue templates are available from the `New issue` page.
- For benchmark or integration reports, start from `docs/REPRO_REPORT_TEMPLATE.md`.
- Please include allocator, platform, commit or release, workload or lane, exact command, and median result when possible.

## Paper / Artifacts

- ACE-Alloc Paper (English): `docs/paper/main_en.pdf`
- ACE-Alloc Paper (Japanese): `docs/paper/main_ja.pdf`
- Local paper workspace: `private/paper/`
- Zenodo Record (v3.2): https://zenodo.org/records/19120414
- DOI: https://doi.org/10.5281/zenodo.19120414
- GitHub Releases: https://github.com/hakorune/hakozuna/releases
- Citation metadata: `CITATION.cff`
- Changelog: `CHANGELOG.md` (BREAKING changes are explicitly listed per release)
- GitHub Release body template: `docs/releases/GITHUB_RELEASE_v3.2.md`

## Benchmark Snapshot (2026-02-18, Ubuntu native)

Latest matrix (`RUNS=10`, MT lane x remote%) and redis-like (`RUNS=10`, memtier 15s) show a clear split:

- `hz3`: strongest in local-heavy and redis-like workloads.
- `hz4`: strongest in remote-heavy and high-thread cross workloads.
- Full benchmark log: `docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md`

### MT lane x remote% (median ops/s)

| Lane | hz3 | hz4 | mimalloc | tcmalloc |
|------|-----|-----|----------|----------|
| `main_r0` | **375.4M** | 137.4M | 224.2M | 232.7M |
| `main_r50` | 66.5M | 78.1M | 17.9M | **84.3M** |
| `main_r90` | 62.6M | **67.6M** | 13.0M | 54.9M |
| `guard_r0` | **376.4M** | 266.7M | 310.0M | 372.0M |
| `cross128_r90` | 1.80M | **50.65M** | 10.94M | 7.50M |

Lane legend:

- `r0` / `r50` / `r90`: target remote-free ratio of `0%`, `50%`, and `90%`
- `main_*`: standard MT `random_mixed` lane at `T=16`, size range `16..32768`
- `guard_*`: small-only guard lane at `T=16`, size range `16..2048`, used to isolate small-object fixed cost
- `cross128_*`: harsher cross-thread lane at `T=16`, size range `16..131072`, used to stress mixed large-path and cross-thread behavior

### Redis-like (median ops/s, RUNS=10)

| Allocator | ops/s |
|-----------|-------|
| **hz3** | **571,199** |
| mimalloc | 568,740 |
| tcmalloc | 568,052 |
| hz4 | 560,576 |

### Practical profile guidance

- Default profile: `hz3` (`scale` lane).
- Remote-heavy / high-thread profile: `hz4`.
- `hz4` redis preload crash (`rc=139`) was fixed via `malloc_usable_size` interpose; redis-like rerun is now stable (`n_ok=10`).

## Documentation

- [Linux Entrypoints](linux/README.md)
- [Windows Build](docs/WINDOWS_BUILD.md)
- [Mac Build](docs/MAC_BUILD.md)
- [Mac Entrypoints](mac/README.md)
- [Repo Structure](docs/REPO_STRUCTURE.md)
- [Windows Redis Matrix](docs/WINDOWS_REDIS_MATRIX.md)
- [Windows Memcached Recovery](docs/WINDOWS_MEMCACHED_RECOVERY.md)
- [Windows Memcached libevent](docs/WINDOWS_MEMCACHED_LIBEVENT.md)
- [Windows Memcached Native MSVC Plan](docs/WINDOWS_MEMCACHED_MSVC_PLAN.md)
- [Windows Memcached Shim](docs/WINDOWS_MEMCACHED_SHIM.md)
- [Windows Memcached Minimal Main](docs/WINDOWS_MEMCACHED_MIN_MAIN.md)
- [Build Flags Index](docs/BUILD_FLAGS_INDEX.md)
- [Paper Notes](docs/HAKMEM_HZ3_PAPER_NOTES.md)
- [Repro Report Template](docs/REPRO_REPORT_TEMPLATE.md)
- [Profile Guide](PROFILE_GUIDE.md)
- [Safe Defaults](docs/SAFE_DEFAULTS.md)
- [Compatibility Notes](docs/COMPATIBILITY.md)

## Design Principles (Box Theory)

1. **Boundary concentration**: Minimize hot path / control layer crossings
2. **Reversibility**: All optimizations toggleable via compile-time flags
3. **Observability**: SSOT (atexit one-shot stats) for reproducible profiling
4. **Fail-fast**: Detect invalid states at boundaries, abort early

## Safety Notes

Some performance flags disable debug invariants:
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` is incompatible with `HZ3_LIST_FAILFAST`, `HZ3_CENTRAL_DEBUG`, `HZ3_XFER_DEBUG`

## License

Apache License 2.0

---

Version: 2026.02.18 (release anchor: 3.0)
