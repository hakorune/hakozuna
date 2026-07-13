# HZ8 Windows Default vs tcmalloc R10

Updated: 2026-07-13

## Scope

This snapshot compares the promoted Windows `hz8` default with tcmalloc on
the same machine and binaries. It deliberately separates two workload shapes:

- local mixed/fixed-size rows from `bench_mixed_ws`;
- a 16-thread, target-90% cross-thread-free row from the paper-aligned MT
  remote runner.

All results are medians of 10 fresh processes. The local matrix uses AB/BA
allocator rotation and `WorkScale=10`. Diagnostic counters are disabled.

## Local And Mixed Rows

| row | tcmalloc ops/s | HZ8 ops/s | HZ8 delta | tcmalloc peak RSS | HZ8 peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| fixed8K | 534.157M | 175.534M | -67.14% | 14.15 MiB | 9.64 MiB |
| fixed16K | 500.418M | 163.381M | -67.35% | 17.00 MiB | 9.86 MiB |
| fixed32K | 300.840M | 41.633M | -86.16% | 40.85 MiB | 11.14 MiB |
| balanced | 451.412M | 58.789M | -86.98% | 45.44 MiB | 7,533.55 MiB |
| wide_ws | 312.887M | 63.812M | -79.61% | 71.47 MiB | 4,025.39 MiB |
| larger_sizes | 189.556M | 29.041M | -84.68% | 74.34 MiB | 2,404.91 MiB |

The promoted page substrate improves HZ8 substantially relative to its own
rollback lane, but this local/broad matrix does not establish tcmalloc parity.
The long mixed rows also expose a large HZ8 peak-residency weakness. The HZ8
low-RSS claim must not be generalized to these rows.

## MT Remote Row

Parameters: `threads=16`, `iters=2000000`, `ws=400`, `size=16..2048`, target
`remote_pct=90`, `ring_slots=65536`.

| allocator | median ops/s | actual remote | fallback | median peak RSS |
| --- | ---: | ---: | ---: | ---: |
| HZ8 | 129.292M | 76.47% | 15.04% | 18.93 MiB |
| tcmalloc | 124.778M | 72.59% | 19.34% | 728.57 MiB |

On this remote-small row, HZ8 is approximately `3.62%` faster while using
approximately `38.5x` less peak RSS. Actual remote and fallback percentages
are reported because the concurrent ring workload does not produce identical
effective ratios for allocators with different timing.

## Decision

```text
Do claim:
  HZ8 is competitive on this Windows remote-small row and has a large RSS
  advantage there.

Do not claim:
  HZ8 has reached tcmalloc across local or broad mixed workloads.
  HZ8 is universally low-peak-RSS.

Next measured weakness:
  local/broad fixed cost and mixed-size peak retention.
```

Commands:

```powershell
.\win\run_win_hz8_page_general_gate.ps1 `
  -Runs 10 -WorkScale 10 -Baseline tcmalloc -Candidate default

.\win\run_win_mt_remote_paper.ps1 `
  -Runs 10 -Allocators hz8,tcmalloc -TimeoutSeconds 300
```
