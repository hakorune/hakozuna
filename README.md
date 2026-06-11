# hakozuna (hz3) / hakozuna-mt (hz4) / hakozuna-hz5

[![hz3/hz4 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20411402.svg)](https://doi.org/10.5281/zenodo.20411402)
[![HZ5 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20411598.svg)](https://doi.org/10.5281/zenodo.20411598)

**High-performance memory allocators competitive with mimalloc and tcmalloc**

Public source release with Ubuntu/Linux, macOS, and Windows-native build and benchmark entrypoints.

Part of the [hakorune](https://github.com/hakorune) project.

---

## Variants

- **hz3 (hakozuna)**: Local-heavy performance + minimal RSS footprint. Default for most workloads.
- **hz4 (hakozuna-mt)**: Message-passing, remote-heavy scaling (best at high thread counts).
- **HZ5 (hakozuna-hz5)**: Linux research sidecar for low-RSS, fail-closed,
  descriptor-owned profile families. It is not the default general allocator.
- **HZ6 (future work)**: possible transfer-first successor line; not an
  evaluated allocator in this repository yet. The documentation-first design
  seed lives under `hakozuna-hz6/`.
- **HZ7 TinyRoute (design seed)**: tiny-binary, single-shape allocator line
  distilled from HZ6. Its phased family now lives under `hz7/` as `v1`,
  `v2`, and `v3`.
- Profile selection guide: [PROFILE_GUIDE.md](PROFILE_GUIDE.md)

## Allocator Profile Map

Hakozuna currently contains three allocator lines with deliberately different
metadata and ownership models:

| Line | Focus | Metadata / routing model | Best read as |
|---|---|---|---|
| HZ3 / ACE-Alloc | local-heavy allocation, compact fast path | lookup-first: PTAG32 / table-oriented pointer-to-bin routing | the main ACE-Alloc line |
| HZ4 | remote-heavy / message-passing workloads | remote-free-first: page-local metadata, remote queues, pending collect | the remote-free experiment line |
| HZ5 | page/run-first sidecar allocator prototype | ownership/policy-first: page/run descriptors route owner, profile, and dispatch policy | the low-RSS fail-closed research line |
| HZ7 TinyRoute | tiny-binary direct API allocator design | span-mask first, optional tiny route table later | the HZ6-minimal design seed, organized under `hz7/` |

In short:

- **HZ3 is lookup-first.**
- **HZ4 is remote-free-first.**
- **HZ5 is ownership/policy-first.**

The API is still `malloc` / `free`, but allocator behavior changes sharply
depending on how `free(ptr)` recovers pointer identity and where ownership is
sent next. HZ5 is not a drop-in replacement for HZ3/HZ4 yet; it is an
experimental sidecar line for page/run descriptors, fail-closed ownership, and
profile-specific dispatch.

## Platform Support

- **Ubuntu/Linux**: public build and preload entrypoints under `linux/` (x86_64 and arm64 lanes)
- **Windows native**: public build and benchmark entrypoints under `win/`
- **macOS**: public build and preload entrypoints under `mac/` (separate Apple Silicon development lane)
- Windows guide: `docs/WINDOWS_BUILD.md`
- Windows public summaries: `docs/benchmarks/windows/`

## Quick Start

### Ubuntu/Linux

```bash
./linux/build_linux_release_lane.sh
./linux/run_linux_preload_smoke.sh hz3 /bin/true
./linux/run_linux_preload_smoke.sh hz4 /bin/true
./linux/run_linux_bench_compare.sh
```

Notes:

- Ubuntu/Linux uses a single entrypoint layer for both `x86_64` and `arm64`.
- On Ubuntu arm64, use `./linux/build_linux_arm64_release_lane.sh` for the explicit lane wrapper.
- For benchmark compare runs, `./linux/run_linux_bench_compare.sh` prepares local `mimalloc` / `tcmalloc` caches and uses the CRT smoke binary.
- Record the CPU architecture in Linux benchmark summaries to keep lanes separate.
- HZ5 Linux experiments live under `hakozuna-hz5/`. Exact Local2P rows use
  `linux/run_linux_hz5_local2p_focus.sh`; general full-preload profile sweeps
  use `linux/run_hz5_hakmem_compare.sh`.

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
- Latest hz3/hz4 archived Zenodo record (v3.4):
  https://zenodo.org/records/20411402
- DOI for hz3/hz4 v3.4:
  https://doi.org/10.5281/zenodo.20411402
- All-version DOI for the hz3/hz4 ACE-Alloc artifact series:
  https://doi.org/10.5281/zenodo.18305952
- HZ5 archived Zenodo record (3.5-hz5):
  https://zenodo.org/records/20411598
- DOI for HZ5 3.5-hz5:
  https://doi.org/10.5281/zenodo.20411598
- All-version DOI for the HZ5 sidecar allocator series:
  https://doi.org/10.5281/zenodo.20411597
- GitHub Releases: https://github.com/hakorune/hakozuna/releases
- Citation metadata: `CITATION.cff`
- Changelog: `CHANGELOG.md` (BREAKING changes are explicitly listed per release)
- Distribution policy: `docs/DISTRIBUTION.md`
- Latest published GitHub Release body template: `docs/releases/GITHUB_RELEASE_v3.3.md`
- Next source/artifact release draft: `docs/releases/GITHUB_RELEASE_v3.4.md`

## Benchmark Snapshot (Ubuntu native)

The MT table below is a `RUNS=10`, `T=16` unified rerun on 2026-05-26 using the
same machine and runner for `hz3`, `hz4`, `mimalloc`, `tcmalloc`, and HZ5 rows.
The redis-like row remains from the 2026-02-18 paper snapshot.

- `hz3`: strongest in local-heavy and redis-like workloads.
- `hz4`: strongest in remote-heavy and high-thread cross workloads.
- `HZ5`: strong low-RSS profile-family rows for selected remote-pressure
  workloads; not one default profile.
- Full benchmark log: `docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md`

### MT lane x remote% (median ops/s, RUNS=10, T=16)

| Lane | hz3 | hz4 | mimalloc | tcmalloc | Best HZ5 | HZ5 row |
|------|-----|-----|----------|----------|----------|---------|
| `main_r0` | 292.15M | 85.63M | 146.73M | **318.82M** | 157.44M | `hz5-pagerun64-main` |
| `main_r50` | 31.46M | 62.32M | 14.26M | 64.87M | **79.43M** | `hz5-large128-transfer128` |
| `main_r90` | 22.31M | **67.14M** | 7.72M | 45.42M | 62.31M | `hz5-pagerun64-cross128` |
| `guard_r0` | 318.98M | 156.68M | 258.19M | **375.71M** | 149.00M | `hz5-pagerun64-main` |
| `cross128_r90` | 2.78M | **27.66M** | 3.52M | 7.21M | 22.39M | `hz5-large128-transfer128` |

HZ5 is shown as "Best HZ5" because it is a profile family. The selected HZ5 row
is listed explicitly so the table does not hide profile dependence.

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

## HZ5 Linux General-Profile Snapshot (2026-05-26, Ubuntu x86_64)

HZ5 has moved beyond the earlier exact `64K/a8192` appendix into a Linux
full-preload profile family. The current claim is deliberately profile-scoped:
HZ5 is a low-RSS, fail-closed, descriptor-owned allocator family with strong
mid/main/cross remote-pressure rows, not a universal replacement for `hz3`,
`hz4`, or tcmalloc.

| HZ5 row | Claim scope |
|---------|-------------|
| `hz5-pagerun64-main` / `hz5-pagerun64-cross128` | MidPage PageRun64 general and cross-size profiles |
| `hz5-large128-rss` | low-RSS LargeFront profile |
| `hz5-large128-source16` | broad LargeFront 128K throughput comparison lane |
| `hz5-large128-transfer128` | diagnostic transfer-cache lane, not a default |

Representative paper-facing rows from the RUNS=5 sweep:

| Case | Best HZ5 row | HZ5 ops/s | HZ5 RSS | tcmalloc ops/s | tcmalloc RSS |
|------|--------------|----------:|--------:|---------------:|-------------:|
| `t=8 main r50` | `hz5-large128-source16` | 63.26M | 24MB | 22.36M | 474MB |
| `t=8 main r90` | `hz5-pagerun64-cross128` | 56.58M | 33MB | 27.80M | 367MB |
| `t=8 mid_only r50` | `hz5-pagerun64-main` | 75.94M | 8MB | 19.30M | 497MB |
| `t=8 cross128 r90` | `hz5-large128-transfer128` | 17.16M | 57MB | 11.72M | 183MB |
| `t=8 large128 r90` | `hz5-large128-source16` | 13.16M | 145MB | 12.12M | 182MB |

Current interpretation:

- Keep `hz3` as the default public profile and `hz4` as the remote-heavy profile.
- Present HZ5 as a profile family: strong low-RSS evidence on selected
  mid/main/cross rows, with guard small-object and some LargeFront 128K rows
  still profile-sensitive.
- Keep the older exact `64K/a8192` Local2P rows as appendix evidence for
  explicit route specialization.
- Treat HZ6 only as future work: a possible transfer-first line with
  consumer-visible class transfer caches and an RSS governor, without hot-path
  learning counters. Current design notes are in `hakozuna-hz6/`.

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
- [Distribution Policy](docs/DISTRIBUTION.md)
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

Version: 2026.05.26 (latest archived release anchor: v3.3; next source/artifact draft: v3.4)
