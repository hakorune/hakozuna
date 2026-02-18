# hakozuna (hz3) / hakozuna-mt (hz4)

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.18674502.svg)](https://doi.org/10.5281/zenodo.18674502)

**High-performance memory allocators competitive with mimalloc and tcmalloc**

Part of the [hakorune](https://github.com/hakorune) project.

---

## Variants

- **hz3 (hakozuna)**: Local-heavy performance + minimal RSS footprint. Default for most workloads.
- **hz4 (hakozuna-mt)**: Message-passing, remote-heavy scaling (best at high thread counts).
- Profile selection guide: `PROFILE_GUIDE.md`

## Quick Start

```bash
# hz3 (scale lane, default)
cd hakozuna/hz3
make clean all_ldpreload_scale
LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app

# hz4 (standalone allocator)
cd ../hz4
make clean all
LD_PRELOAD=./libhakozuna_hz4.so ./your_app
```

## Paper / Artifacts

- [ACE-Alloc Paper (English)](docs/paper/main_en.pdf)
- [ACE-Alloc Paper (日本語)](docs/paper/main_ja.pdf)
- Zenodo Record (v3.0): https://zenodo.org/records/18674502
- DOI: https://doi.org/10.5281/zenodo.18674502
- GitHub Release (v3.0.0): https://github.com/hakorune/hakozuna/releases/tag/v3.0.0
- Citation metadata: `CITATION.cff`
- Changelog: `CHANGELOG.md` (BREAKING changes are explicitly listed per release)
- GitHub Release body template: `docs/releases/GITHUB_RELEASE_v3.0.md`

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

- [Build Flags Index](docs/BUILD_FLAGS_INDEX.md)
- [Paper Notes](docs/HAKMEM_HZ3_PAPER_NOTES.md)
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

Version: 2026.02.18 (release anchor: v3.0.0)
