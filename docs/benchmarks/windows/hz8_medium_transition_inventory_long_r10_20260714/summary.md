# HZ8 Medium Boundary Windows Gate

- Fresh process AB/BA rotation
- Runs: 10
- Work scale: 10x
- Baseline: `hz8`
- Candidate: `hz8-medium-transition-inventory`
- Diagnostic counters: disabled

| row | hz8 ops/s | hz8-medium-transition-inventory ops/s | paired delta | hz8 peak RSS | hz8-medium-transition-inventory peak RSS | RSS delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary4k | 329.787M | 325.558M | -1.69% | 37.58 MiB | 37.62 MiB | +0.11% |
| range4097_8192 | 36.293M | 133.339M | +270.56% | 39.96 MiB | 39.79 MiB | -0.43% |
| fixed8k | 176.229M | 173.324M | -1.98% | 9.63 MiB | 9.65 MiB | +0.16% |
| fixed16k | 161.414M | 164.192M | +1.95% | 9.87 MiB | 9.89 MiB | +0.18% |
| fixed32k | 38.455M | 52.167M | +37.80% | 10.87 MiB | 11.02 MiB | +1.40% |
| fixed64k | 14.999M | 21.401M | +34.06% | 9.22 MiB | 8.77 MiB | -4.93% |
| balanced | 549.104M | 555.674M | +6.83% | 53.10 MiB | 53.04 MiB | -0.10% |
| wide_ws | 376.839M | 387.033M | +3.29% | 97.38 MiB | 97.31 MiB | -0.07% |
| larger_sizes | 52.993M | 128.371M | +142.42% | 97.96 MiB | 98.76 MiB | +0.82% |

Acceptance guide: the target boundary should improve materially; balanced/wide controls must remain within -3%; peak RSS must remain within max(+5%, +1 MiB); all runs require alloc_fail=0.
