# HZ11 Windows Fixed Local Bench L0

Status: Windows L0 speed observation.

This is a narrow standalone measurement for the Windows token/front-cache
bring-up. It does not claim fine128 parity, LD_PRELOAD parity, DLL
interposition, or a Windows malloc replacement.

## Build Shape

```text
compiler:
  clang-cl /O2

benchmark:
  bench/hz11_fixed_local_bench.c

lanes:
  system-crt:
    plain malloc/free

  hz11-token:
    benchmark malloc/free redirected to hz11_malloc/hz11_free
    HZ11_CLASSIFY_SPAN=0
    HZ11_TLS_FASTPATH=0

  hz11-tlsfast:
    benchmark malloc/free redirected to hz11_malloc/hz11_free
    HZ11_CLASSIFY_SPAN=0
    HZ11_TLS_FASTPATH=1

runner:
  scripts/run_hz11_win64_fixed_local_bench.ps1
```

## Result

Local machine, RUNS=5, ITERS=5,000,000 per run.

```text
source result directory:
  results/windows/hz11_l0_fixed_local/20260709_140107
```

| slot | system-crt ops/s | hz11-token ops/s | hz11-tlsfast ops/s |
|---:|---:|---:|---:|
| 64 | 57.81M | 280.58M | 307.50M |
| 256 | 56.97M | 272.14M | 301.48M |
| 1024 | 56.84M | 263.35M | 291.43M |
| 4096 | 56.58M | 265.81M | 306.45M |

## Reading

```text
GO:
  Windows L0 standalone token/front-cache speed path is alive.
  HZ11_TLS_FASTPATH improves the token lane in this fixed-local microbench.

NOT CLAIMED:
  fine128 Windows performance
  remote/mixed performance
  RSS behavior
  DLL/provider replacement behavior
  real-app behavior
```

The benchmark is deliberately local and LIFO-shaped, so it mostly measures the
steady-state public-entry front-cache path. It should be treated as an upper
bound / bring-up signal before adding Windows span, transfer, RSS, or app lanes.
