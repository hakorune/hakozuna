# Paper Bench Results (2026-02-18)

This snapshot supersedes the low-run quick sweep in `docs/benchmarks/2026-01-24_PAPER_BENCH_RESULTS.md`.

## Scope
- MT lane x remote% matrix: `RUNS=10` (median)
- redis-like (memtier 15s): `RUNS=10` (median)
- Allocators: `hz3`, `hz4`, `mimalloc`, `tcmalloc`

## Environment
- Host: Ubuntu Linux (native), 16 cores
- CPU pin: `taskset -c 0-15`
- Benchmark binary: `hakozuna/out/bench_random_mixed_mt_remote_malloc`

## Repro Commands

### 1) MT lane x remote% matrix (`RUNS=10`)
```bash
cd /mnt/workdisk/public_share/hakmem
scripts/run_mt_lane_remote_matrix.sh \
  --runs 10 \
  --cpu-list 0-15 \
  --outdir /tmp/mt_lane_remote_matrix_paper_20260218_025726
```

Artifacts:
- `/tmp/mt_lane_remote_matrix_paper_20260218_025726/raw.tsv`
- `/tmp/mt_lane_remote_matrix_paper_20260218_025726/summary.tsv`
- `/tmp/mt_lane_remote_matrix_paper_20260218_025726/matrix.md`

### 2) redis-like (`RUNS=10`, memtier 15s)
```bash
cd /mnt/workdisk/public_share/hakmem
RUNS=10 \
TARGETS=redis \
ALLOCATORS=hz3,hz4,mimalloc,tcmalloc \
MEMTIER_SECONDS=15 \
OUTDIR=/tmp/realworld_redis_paper_20260218_031128 \
hakozuna/scripts/run_realworld_4pack.sh
```

Artifacts:
- `/tmp/realworld_redis_paper_20260218_031128/results.csv`
- `/tmp/realworld_redis_paper_20260218_031128/summary.tsv`

## Results

### MT lane x remote% (median ops/s)

| Lane | hz3 | hz4 | mimalloc | tcmalloc |
|---|---:|---:|---:|---:|
| `guard_r0` | **376.4M** | 266.7M | 310.0M | 372.0M |
| `guard_r50` | 125.3M | 116.8M | **131.0M** | 109.2M |
| `guard_r90` | 101.8M | **108.6M** | 74.2M | 67.1M |
| `main_r0` | **375.4M** | 137.4M | 224.2M | 232.7M |
| `main_r50` | 66.5M | 78.1M | 17.9M | **84.3M** |
| `main_r90` | 62.6M | **67.6M** | 13.0M | 54.9M |
| `cross128_r0` | 4.05M | **80.15M** | 52.13M | 10.51M |
| `cross128_r50` | 4.67M | **52.56M** | 14.09M | 10.20M |
| `cross128_r90` | 1.80M | **50.65M** | 10.94M | 7.50M |

### redis-like (median ops/s, RUNS=10)

| Allocator | ops/s |
|---|---:|
| **hz3** | **571,199** |
| mimalloc | 568,740 |
| tcmalloc | 568,052 |
| hz4 | 560,576 |

## Profile Guidance (as of 2026-02-18)
- Default profile: `hz3` (local-heavy + redis-like)
- Remote-heavy / high-thread profile: `hz4`
- `hz4` redis preload segfault issue has been fixed and is stable in this run (`n_ok=10`)
