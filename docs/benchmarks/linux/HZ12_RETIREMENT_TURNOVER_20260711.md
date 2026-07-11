# HZ12 Linux Retirement Turnover (2026-07-11)

## Contract

- Ubuntu/Linux backing: `mmap` arena plus `madvise(MADV_DONTNEED)` discard
- 8 repeated owner generations
- 64 same-class spans per generation, 64-byte objects
- P4 advisory query and P1 authority reclaim remain separate calls
- `RUNS=5`; table reports the median of the five per-process summaries

## Result

| Metric | Result |
|---|---:|
| retirement p50 | 1.089 ms |
| retirement p99 | 1.144 ms |
| retirement max | 1.167 ms |
| reclaimed spans | 64/64 per generation; 512 total |
| decommitted/discarded bytes | 4 MiB per generation; 32 MiB total |
| depot final/current/max | 64 / 64 / 64 |
| limbo spans | 0 |
| peak RSS | 7.56 MiB |
| post RSS | 3.56 MiB |
| generation reuse | correct; slot 0, generations 1 through 8 |

Command:

```sh
RUNS=5 hakozuna-hz12/linux/run_hz12_retirement_turnover.sh
```

## Scope

This result validates the HZ12 ownership/reclaim contract and Linux backing.
HZ8/HZ11/mimalloc/tcmalloc do not expose the same owner-retirement authority
API, so they are not represented as retirement-latency rows here. See the
[separate common-workload allocator matrix](HZ8_HZ11_HZ12_ALLOCATOR_COMPARE_20260711.md).
