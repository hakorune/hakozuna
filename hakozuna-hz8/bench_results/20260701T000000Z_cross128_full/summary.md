# HZ8 cross128_r90 Full Row Snapshot

This run captures the full `cross128_r90` MT-lane row after the earlier probe
only established a diagnostic floor.

```text
allocator_behavior_sha:
  2f43f7e1

runs:
  10

threads:
  16

iters:
  50000

row shape:
  cross128_r90
  size=16..131072
  remote_pct=90
  interleaved=1
```

Command:

```bash
timeout 600s env LD_PRELOAD=/mnt/workdisk/public_share/hakozuna_repo/hakozuna-hz8/libhakozuna_hz8_preload.so \
  /mnt/workdisk/public_share/hakozuna_repo/hakozuna-hz8/bench/out/bench_matrix_malloc \
  --runs 10 --threads 16 --iters 50000 \
  --min-size 16 --max-size 131072 --remote-pct 90 --interleaved 1 --live-window 0
```

## Result

| Row | median ops/s | p25 ops/s | p75 ops/s | median post RSS | median peak RSS | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `cross128_r90` | 37.342k | 37.127k | 37.405k | 151.40 MiB | 196.85 MiB | 10 | 0 |

## Note

The earlier `ITERS=5000` probe remains useful as a diagnostic floor, but this
full row is the value that should be used when comparing against the other MT
lane rows.
