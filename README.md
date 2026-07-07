# hakozuna (hz3) / hakozuna-mt (hz4) / hakozuna-hz5 / hakozuna-hz6 / hakozuna-hz8 / hakozuna-hz9 / hakozuna-hz10

[![hz3/hz4 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20753903.svg)](https://doi.org/10.5281/zenodo.20753903)
[![HZ5 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20753950.svg)](https://doi.org/10.5281/zenodo.20753950)
[![HZ6 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20753968.svg)](https://doi.org/10.5281/zenodo.20753968)
[![HZ8 DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21084279.svg)](https://doi.org/10.5281/zenodo.21084279)

**High-performance memory allocators competitive with mimalloc and tcmalloc**

Public source release with Ubuntu/Linux, macOS, and Windows-native build and benchmark entrypoints.

Part of the [hakorune](https://github.com/hakorune) project.

---

## Variants

- **hz3 (hakozuna)**: Local-heavy performance + minimal RSS footprint. Historical local-heavy public profile.
- **hz4 (hakozuna-mt)**: Message-passing, remote-heavy scaling (best at high thread counts).
- **HZ5 (hakozuna-hz5)**: Linux research sidecar for low-RSS, fail-closed,
  descriptor-owned profile families. It is not the default general allocator.
- **HZ6 (hakozuna-hz6)**: Windows/Linux allocator family with explicit
  route/source/frontcache contracts, Ubuntu `LD_PRELOAD` selected/profile
  lanes, and selected-family speed/RSS evidence.
- **HZ7 TinyRoute (design seed)**: tiny-binary, single-shape allocator line
  distilled from HZ6. Its phased family now lives under `hz7/` as `v1`,
  `v2`, `v3`, and `v4`.
- **HZ8 (hakozuna-hz8)**: recommended balanced allocator line. Current default
  is HZ8-v2 / KeepRefill plus preload-surface and remote span-lease publish
  hardening.
- **HZ9 (hakozuna-hz9)**: standalone experimental throughput line. It keeps
  HZ8 as the frozen balanced line and develops new HZ9 behavior only under
  `hakozuna-hz9/`.
- **HZ10 (hakozuna-hz10)**: macro/shim research candidate. It is now included
  in the integrated benchmark matrix, but has not replaced HZ8 as the public
  recommendation.
- Profile selection guide: [PROFILE_GUIDE.md](PROFILE_GUIDE.md)

## Allocator Profile Map

Hakozuna currently contains allocator lines with deliberately different
metadata and ownership models:

| Line | Focus | Metadata / routing model | Best read as |
|---|---|---|---|
| HZ3 / ACE-Alloc | local-heavy allocation, compact fast path | lookup-first: PTAG32 / table-oriented pointer-to-bin routing | the main ACE-Alloc line |
| HZ4 | remote-heavy / message-passing workloads | remote-free-first: page-local metadata, remote queues, pending collect | the remote-free experiment line |
| HZ5 | page/run-first sidecar allocator prototype | ownership/policy-first: page/run descriptors route owner, profile, and dispatch policy | the low-RSS fail-closed research line |
| HZ6 | balanced speed/RSS with explicit safety contracts | RouteLayer + descriptor + SourceLayer + FrontCache | the active Windows/Linux successor line |
| HZ7 TinyRoute | tiny-binary direct API allocator design | span-mask first, optional tiny route table later | the HZ6-minimal design seed, organized under `hz7/` |
| HZ8 | recommended balanced line | fail-closed ownership + owner-stable remote free + KeepRefill pressure control | the current public allocator line |
| HZ9 | standalone throughput research | HZ8-derived safety boundary + HZ9-owned local substrates | the experimental successor lane under `hakozuna-hz9/` |
| HZ10 | macro/shim speed research | page-based fail-closed substrate + preload shim + orphan adoption work | the speed-oriented research candidate under `hakozuna-hz10/` |

In short:

- **HZ3 is lookup-first.**
- **HZ4 is remote-free-first.**
- **HZ5 is ownership/policy-first.**
- **HZ6 is contract-first with selected/default and profile-only lanes.**
- **HZ8 is the recommended balanced allocator line.**
- **HZ9 is the separate throughput research line; it is not the HZ8 default.**
- **HZ10 is a newer macro/shim research candidate; it has not replaced HZ8 as
  the public recommendation.**

## Architecture Difference / アーキテクチャの違い

English summary:

```text
HZ3:
  TLS/cache-heavy local speed line.
  Fast local reuse is the priority; safety/remote lifecycle boundaries are
  intentionally lighter than HZ8/HZ9.

HZ8:
  Balanced low-RSS public line.
  Shared medium-run metadata is the authority: slot_state, free/allocated masks,
  pending bitmap, qstate, owner queue, and owner-exit hard drain.

HZ9:
  Throughput-first experimental line.
  Medium/main local reuse is owned by TLS per-class ProductEntry state, while
  remote frees still route through segment metadata and pending-bit owner drain.
  RSS may be heavier than HZ8, but retention must stay explicit and bounded.
```

Japanese summary:

```text
HZ3:
  TLS/cache中心のローカル速度重視ライン。
  ローカル再利用の速さを優先し、HZ8/HZ9ほど重いremote/lifecycle境界は
  背負わない。

HZ8:
  低RSSと安全性のバランスを取った公開推奨ライン。
  slot_state、free/allocated mask、pending bitmap、qstate、owner queue、
  owner-exit hard drainをmedium run側の共有authorityとして持つ。

HZ9:
  throughput-firstの実験ライン。
  medium/mainのローカル再利用はTLS per-class ProductEntryが持ち、
  remote freeはsegment routeとpending-bit owner drainで回収する。
  RSSはHZ8より重くてもよいが、retentionは明示cap付きで扱う。
```

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
- HZ8 paper Zenodo record:
  https://zenodo.org/records/21084279
- DOI for HZ8 paper:
  https://doi.org/10.5281/zenodo.21084279
- All-version DOI for the HZ8 paper series:
  https://doi.org/10.5281/zenodo.21084278
- GitHub Releases: https://github.com/hakorune/hakozuna/releases
- Citation metadata: `CITATION.cff`
- Changelog: `CHANGELOG.md` (BREAKING changes are explicitly listed per release)
- Distribution policy: `docs/DISTRIBUTION.md`
- Latest published GitHub Release body template: `docs/releases/GITHUB_RELEASE_v3.3.md`
- Next source/artifact release draft: `docs/releases/GITHUB_RELEASE_v3.4.md`
- HZ6 source/artifact release draft: `docs/releases/GITHUB_RELEASE_hz6.md`
- HZ8 source/artifact release draft: `docs/releases/GITHUB_RELEASE_hz8.md`
- HZ8 Zenodo description draft: `docs/releases/ZENODO_hz8_DESCRIPTION.md`
- HZ8 release/paper preparation: `hakozuna-hz8/docs/HZ8_PUBLIC_RELEASE_PREP.md`
- HZ8 paper-ready Ubuntu matrix:
  `hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md`

## Benchmark Snapshot (Ubuntu native)

For new cross-line measurements, use the integrated same-run matrix:

```bash
ALLOCATORS=hz8,hz10,hz3,hz4,mimalloc,tcmalloc,system \
ROWS=guard_local0,main_local0,main_interleaved_r50,main_interleaved_r90 \
RUNS=10 THREADS=16 ITERS=50000 \
  hakozuna-hz8/scripts/run_hz8_v11_same_run_matrix.sh
```

Guide: `docs/benchmarks/ALLOCATOR_LINE_INTEGRATED_MATRIX.md`

Latest integrated snapshot:
`docs/benchmarks/20260707_allocator_line_integrated_hz3_hz4_hz8_hz10_r10/README.md`

Current integrated snapshot, including HZ10 and external allocators:

Median ops/s:

| Row | HZ3 | HZ4 | HZ8 | HZ10 | mimalloc | tcmalloc |
|-----|----:|----:|----:|-----:|---------:|---------:|
| `guard_local0` | 156.85M | 49.01M | 207.05M | 137.53M | 88.23M | **354.64M** |
| `main_local0` | 149.31M | 28.82M | 117.94M | 118.18M | 21.32M | **367.54M** |
| `main_interleaved_r50` | 16.86M | 12.28M | 10.84M | 20.18M | 5.63M | **22.22M** |
| `main_interleaved_r90` | 10.20M | 9.75M | 7.04M | 12.60M | 4.28M | **13.90M** |
| `small_interleaved_remote90` | 12.95M | 11.13M | 14.70M | 14.96M | 13.19M | **26.54M** |
| `medium_interleaved_r50` | 15.43M | 8.68M | 9.84M | **19.87M** | 4.20M | 16.76M |

Post-workload RSS:

| Row | HZ3 | HZ4 | HZ8 | HZ10 | mimalloc | tcmalloc |
|-----|----:|----:|----:|-----:|---------:|---------:|
| `guard_local0` | 12.35MiB | 131.11MiB | 2.00MiB | 26.38MiB | 13.33MiB | 7.00MiB |
| `main_local0` | 15.11MiB | 169.99MiB | 3.50MiB | 33.75MiB | 67.89MiB | 9.00MiB |
| `main_interleaved_r50` | 163.77MiB | 258.10MiB | 4.33MiB | 79.50MiB | 190.12MiB | 68.50MiB |
| `main_interleaved_r90` | 205.82MiB | 275.23MiB | 4.62MiB | 89.62MiB | 234.37MiB | 82.88MiB |
| `small_interleaved_remote90` | 130.92MiB | 144.87MiB | 2.95MiB | 43.75MiB | 56.82MiB | 31.62MiB |
| `medium_interleaved_r50` | 148.37MiB | 237.98MiB | 3.83MiB | 69.62MiB | 191.08MiB | 85.12MiB |

Interpretation:

- HZ8 remains the public recommended balanced line: lowest HZ post RSS in this
  integrated matrix.
- HZ10 is the speed-oriented research candidate: stronger remote/interleaved
  throughput, with tens-of-MiB post RSS in these rows.
- Historical HZ3/HZ4/HZ5/HZ6/HZ8 snapshots are archived in
  `docs/benchmarks/ALLOCATOR_HISTORY_SNAPSHOTS.md`.

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
