# hakozuna (hz3) / hakozuna-mt (hz4)

[![DOI](https://img.shields.io/badge/DOI-10.5281%2Fzenodo.18357813-blue)](https://doi.org/10.5281/zenodo.18357813)

**High-performance memory allocators competitive with mimalloc and tcmalloc**

Part of the [hakorune](https://github.com/hakorune) project.

---

## Variants

- **hz3 (hakozuna)**: Local-heavy performance + minimal RSS footprint. Default for most workloads.
- **hz4 (hakozuna-mt)**: Message-passing, remote-heavy scaling (best at high thread counts).

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
- DOI: https://doi.org/10.5281/zenodo.18357813

## Benchmark Snapshot (2026-02-18, Ubuntu native)

Latest matrix (`RUNS=7`, MT lane x remote%) and redis-like (`RUNS=5`, memtier 15s) show a clear split:

- `hz3`: strongest in local-heavy and redis-like workloads.
- `hz4`: strongest in remote-heavy and high-thread cross workloads.

### MT lane x remote% (median ops/s)

| Lane | hz3 | hz4 | mimalloc | tcmalloc |
|------|-----|-----|----------|----------|
| `main_r0` | **352.9M** | 136.6M | 224.4M | 230.1M |
| `main_r50` | 55.0M | 75.5M | 18.6M | **83.9M** |
| `main_r90` | 41.3M | **67.8M** | 16.1M | 56.2M |
| `guard_r0` | **387.1M** | 265.9M | 311.6M | 370.7M |
| `cross128_r90` | 4.21M | **50.97M** | 9.20M | 7.50M |

### Redis-like (median ops/s, RUNS=5)

| Allocator | ops/s |
|-----------|-------|
| **hz3** | **568,071** |
| mimalloc | 566,827 |
| tcmalloc | 565,494 |
| hz4 | 559,514 |

### Practical profile guidance

- Default profile: `hz3` (`scale` lane).
- Remote-heavy / high-thread profile: `hz4`.
- `hz4` redis preload crash (`rc=139`) was fixed via `malloc_usable_size` interpose; redis-like rerun is now stable (`n_ok=5`).

## Documentation

- [Build Flags Index](docs/BUILD_FLAGS_INDEX.md)
- [Paper Notes](docs/HAKMEM_HZ3_PAPER_NOTES.md)

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

Version: 2026.01.18
