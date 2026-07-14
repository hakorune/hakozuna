# HZ8 Shared-Default Windows Full Check

Generated: 2026-07-14

This is a broad `RUNS=1` development check after promoting
`SmallTransitionInventory-L1` to the shared HZ8 default. It is not a
paper-quality repeat campaign. The environment and baseline binaries are the
same Windows setup used by the existing repository runners.

## Scope

- allocator matrix: all standard profiles from smoke through fixed 8 MiB and
  heavy mixed
- allocators: HZ8, HZ3, HZ4, mimalloc, and tcmalloc
- MT remote: T=16, requested remote 90%, size 16..2048
- Redis-like: SET, GET, LPUSH, LPOP, and RANDOM
- separate matched-remote promotion gate: Blocks=20, 40/40 admissible pairs
- default and rollback smoke/safety on Windows and WSL/Linux

## Representative Matrix Results

| row | HZ8 | tcmalloc | HZ8 vs tcmalloc | HZ8 peak RSS | tcmalloc peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| balanced | 266.60M | 186.62M | +42.9% | 52.52 MiB | 42.34 MiB |
| wide_ws | 139.69M | 85.92M | +62.6% | 93.41 MiB | 67.75 MiB |
| larger_sizes | 30.01M | 60.77M | -50.6% | 75.70 MiB | 57.03 MiB |
| fixed 1 KiB | 225.44M | 124.38M | +81.3% | not sampled | 25.39 MiB |
| fixed 2 KiB | 179.64M | 72.09M | +149.2% | 37.68 MiB | 37.74 MiB |
| fixed 4 KiB | 147.55M | 54.80M | +169.3% | not sampled | 40.31 MiB |
| fixed 8 KiB | 34.37M | 39.09M | -12.1% | 22.28 MiB | 31.36 MiB |
| fixed 64 KiB | 9.60M | 53.84M | -82.2% | 9.21 MiB | 27.41 MiB |
| fixed 1 MiB | 0.357M | 4.88M | -92.7% | 4.84 MiB | 41.46 MiB |
| fixed 8 MiB | 0.254M | 0.324M | -21.7% | 4.73 MiB | 105.21 MiB |
| heavy_mixed | 278.23M | 172.92M | +60.9% | 361.96 MiB | 234.92 MiB |

HZ8 completed every allocator-matrix row with zero allocation failures. HZ4
crashed on the 128 KiB and larger rows; that is a baseline issue, not an HZ8
failure.

Very short fast rows can exit before the external RSS sampler observes HZ8, so
`not sampled` must not be interpreted as zero RSS. Use the longer rows and the
dedicated MT/RSS runners for memory claims.

## MT Remote

| allocator | throughput | actual remote | fallback | peak RSS |
| --- | ---: | ---: | ---: | ---: |
| HZ8 | 66.28M | 77.79% | 13.57% | 19.44 MiB |
| mimalloc | 107.93M | 79.21% | 12.00% | 523.47 MiB |
| tcmalloc | 123.34M | 69.36% | 22.93% | 796.43 MiB |

HZ8 reaches 53.7% of tcmalloc throughput while using about 1/41 of its peak
RSS in this run. Actual remote and fallback ratios differ, so this row is a
development comparison rather than a strict equal-work remote claim. The
separate matched-remote gate is the promotion authority and passed with
fallback zero.

## Redis-Like

| pattern | HZ8 | tcmalloc | delta |
| --- | ---: | ---: | ---: |
| SET | 56.45M | 80.45M | -29.8% |
| GET | 262.52M | 322.07M | -18.5% |
| LPUSH | 55.90M | 76.37M | -26.8% |
| LPOP | 814.32M | 999.23M | -18.5% |
| RANDOM | 165.93M | 172.27M | -3.7% |

HZ8 peak RSS was 16.43 MiB versus 13.20 MiB for tcmalloc and 10.45 MiB for
mimalloc. This synthetic Redis-like lane is therefore not an HZ8 RSS win.

## Decision

```text
shared default:
  keep HZ8 SmallTransitionInventory-L1

strong surfaces:
  balanced and wide small-object matrix
  fixed 512B..4KiB
  heavy mixed throughput
  remote-heavy peak RSS

remaining weaknesses:
  variable 4KiB..8KiB medium
  fixed 16KiB..64KiB
  direct 128KiB..4MiB throughput
  Redis-like mutation throughput
  heavy_mixed peak RSS

next optimization target:
  medium/direct transition and backing reuse
  do not reopen the small transition-inventory policy
```

## Artifacts

- `20260714_162551_allocator_matrix.md`
- `20260714_162655_paper_mt_remote_windows.md`
- `20260714_162704_paper_redis_workload_windows.md`
- `../paper/20260714_153251_hz8_transition_inventory_matched_remote.md`
