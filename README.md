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

## Benchmark Snapshot (2026-01-24, Ubuntu native, 16 cores)

**SSOT T=16 / R=90 (median of 10):**

| Allocator | ops/s |
|-----------|-------|
| hz4 | 81.7M |
| tcmalloc | 77.8M |
| mimalloc | 72.2M |
| hz3 | 71.6M |

**R sweep (T=16):**

| R | hz3 | hz4 | mimalloc | tcmalloc |
|---|-----|-----|---------|----------|
| 0% | **359.6M** | 249.6M | 300.5M | 357.0M |
| 50% | 94.4M | 97.4M | 84.6M | 103.3M |
| 90% | 75.3M | **97.3M** | 75.1M | 74.9M |

**ST dist_app (16–65536):**

| Allocator | ops/s |
|-----------|-------|
| tcmalloc | 80.2M |
| hz3 | 75.2M |
| mimalloc | 73.4M |
| hz4 | 51.6M |

**RSS MT remote (T=16/R=90):**

| Allocator | Max RSS (GB) |
|-----------|--------------|
| **hz3** | **1.36 GB** |
| mimalloc | 1.52 GB |
| hz4 | 2.04 GB |
| tcmalloc | 2.34 GB |

Full results and commands: `docs/benchmarks/2026-01-24_PAPER_BENCH_RESULTS.md`

## Notes

- hz3 stability for paper runs: `S62_RETIRE=0, S62_PURGE=0`.
- hz3 has two lanes: `fast` (low latency) and `scale` (high parallelism).

Summary: hakozuna wins most multi-threaded workloads, especially remote-free heavy cases (e.g. T=8 R=90 at +46%). At T=32 R=90, hz3 and mimalloc are very close; in WSL2 the median slightly favors hz3, but variance is large.

### RSS snapshot (WSL2, ru_maxrss, lower is better)

| Benchmark | hz3 | mimalloc | tcmalloc | system |
|-----------|-----|----------|----------|--------|
| random_mixed | 3.0 MB | 2.1 MB | 7.8 MB | 2.0 MB |
| alloc-test | 28.9 MB | 72.4 MB | 20.8 MB | 19.6 MB |
| espresso | 6.2 MB | 7.6 MB | 10.7 MB | 2.6 MB |

Note: RSS varies by workload; see `docs/paper/RESULTS_20260118.md` for full context.

### memcached (ops/sec, higher is better)

| Threads | hakozuna | mimalloc | tcmalloc |
|---------|-----|----------|----------|
| T=1 | 278,109 | 283,697 | 280,453 |
| T=4 | **816,008** | 741,478 | 809,366 |
| T=8 | 1,298,515 | 1,301,294 | 1,304,450 |
| T=16 | **1,487,819** | 1,471,710 | 1,374,252 |

### MT Remote-Free (ops/sec, higher is better)

| Condition | hakozuna | mimalloc | tcmalloc |
|-----------|-----|----------|----------|
| T=8 R=90% | **193.1M** | 132.3M | 153.0M |
| T=16 R=50% | **270.6M** | 208.6M | 226.5M |
| T=32 R=90% | **198.8M** | 196.5M | 147.6M |

### random_mixed (ops/sec, higher is better)

| Allocator | Throughput |
|-----------|------------|
| tcmalloc | 134.6M |
| hakozuna | 132.6M |
| mimalloc | 130.2M |
| system | 108.5M |

**Summary**: hakozuna wins in most multi-threaded workloads, especially remote-free heavy cases (e.g. T=8 R=90 at +46%). At T=32 R=90, hz3 and mimalloc are very close; in WSL2 the median slightly favors hz3, but variance is large.

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
