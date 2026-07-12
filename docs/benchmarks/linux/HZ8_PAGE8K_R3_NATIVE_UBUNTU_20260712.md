# HZ8 Page8K R3 Native Ubuntu x86_64 Gate

## Environment

```text
date: 2026-07-12
host: Ubuntu 22.04.5 LTS, Linux 6.8.0-90-generic, x86_64
control: hz8-v2 (Mag16)
candidate: hz8-r3-page8k-integrated
ordering: alternating AB/BA, fresh process
runs: 5 per lane and row
```

GCC 11.4 and Clang 14 builds used `-Wall -Wextra -Werror`. R3 remains an
opt-in exact-8KiB behavior box; no allocator default or hot-path diagnostic
counter was changed.

## Native Results

Values are repeat-5 medians. RSS is process `VmRSS`/`VmHWM`; synthetic rows
report bytes directly from `h8_bench`, while Redis-like reports MiB.

| Row | RUNS | HZ8 v2 ops/s | R3 ops/s | Delta | v2 post / peak RSS | R3 post / peak RSS | Safety |
|---|---:|---:|---:|---:|---:|---:|---|
| fixed8K | 5 | 207.840M | 233.532M | +12.36% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB | PASS |
| balanced | 5 | 268.107M | 250.363M | -6.62% | 1.875 / 1.875 MiB | 1.875 / 1.875 MiB | PASS |
| wide_ws | 5 | 217.539M | 217.040M | -0.23% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB | PASS |
| larger_sizes | 5 | 113.052M | 98.131M | -13.20% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB | PASS |
| Redis-like aggregate | 5 | 663.320M | 660.890M | -0.37% | 1.715 / 4.852 MiB | 1.816 / 5.117 MiB | PASS |

The Redis-like aggregate is the sum of SET, GET, LPUSH, LPOP, and RANDOM
throughputs for `threads=4 cycles=500 ops=2000 size=16..256`. That size range
does not target the exact-8KiB box, so it is a no-regression guard only. R3
changed median post RSS by +5.92% and median peak RSS by +5.48% on this row.

## Safety

Both compilers passed the HZ8 v2 and R3 smoke and safety-stress binaries. R3
also passed the exact-8KiB API smoke. The observed lifecycle summaries matched
between control and candidate:

```text
smoke:  owners=68 local=72 remote=32
safety: owners=9 owner_exit=8 handoff=68 remote=8192 collect=0
        duplicate_claim=1 invalid=7
page8K API smoke: PASS
```

No safety failure or allocation failure was observed.

## Decision

```text
native Ubuntu R3 performance promotion: NO-GO
Linux R3 opt-in research/correctness lane: GO
HZ8 v2 public default: unchanged
cross-platform default promotion: HOLD
```

The fixed-8KiB box has a positive native signal, but it misses the required
`+30%` gate. Balanced exceeds its `-3%` regression bound, larger_sizes exceeds
its `-5%` bound, and Redis-like peak RSS narrowly exceeds its `+5%` guard.
This is mixed Linux evidence, not a basis for default promotion.

Raw logs are retained under
`private/raw-results/linux/hz8_page8k_r3_native_20260712/`.
