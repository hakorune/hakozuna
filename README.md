# hakozuna (hz3)

**A high-performance memory allocator competitive with mimalloc and tcmalloc**

Part of the [hakorune](https://github.com/hakorune) project.

---

## Highlights

- **Performance**: Matches or exceeds mimalloc/tcmalloc in multi-threaded workloads
- **Remote-free optimization**: +28% over mimalloc at 50%+ remote-free ratio
- **Box Theory design**: Boundary-focused architecture with reversible optimizations
- **Two lanes**: `fast` (low latency) and `scale` (high parallelism)

## Architecture

![hz3 Architecture](docs/images/architecture_overview.png)

## Paper

- [ACE-Alloc Paper (English)](docs/paper/main_en.pdf)
- [ACE-Alloc Paper (日本語)](docs/paper/main_ja.pdf)

## Quick Start

```bash
# Build (scale lane, default)
make clean all_ldpreload_scale

# Smoke test
LD_PRELOAD=./libhakozuna_hz3_scale.so /bin/true

# Run with your application
LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app
```

## Build Options

| Target | Output | Use case |
|--------|--------|----------|
| `all_ldpreload_scale` | `libhakozuna_hz3_scale.so` | Default, 32 shards, high parallelism |
| `all_ldpreload_fast` | `libhakozuna_hz3_fast.so` | 8 shards, low latency |
| `all_ldpreload_scale_tolerant` | - | Collision-tolerant for very high thread counts |

### Preset Variants

```bash
# Remote-heavy optimized (r50 = 50% remote ratio target)
make all_ldpreload_scale_r50
make all_ldpreload_scale_r50_s97_1   # S97-1 remote bucketize
make all_ldpreload_scale_r50_s97_8   # S97-8 sort+group (stack-only)

# High remote ratio (r90 = 90% remote ratio target)
make all_ldpreload_scale_r90
make all_ldpreload_scale_r90_pf2
make all_ldpreload_scale_r90_pf2_s97_2   # Direct-map (best at T>=16)
make all_ldpreload_scale_r90_pf2_s97_8_t8 # Sort+group (best at T=8)
```

## Benchmarks

Run the SSOT benchmark suite:

```bash
RUNS=10 ITERS=20000000 WS=400 ./scripts/run_bench_hz3_ssot.sh
```

### Results Summary (vs mimalloc)

| Benchmark | Condition | Result |
|-----------|-----------|--------|
| Larson | T=8-16 | **+15%** |
| memcached | T=4 | **+10%** |
| MT remote | T=8 R=90% | **+28%** |
| random_mixed | - | comparable |

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
