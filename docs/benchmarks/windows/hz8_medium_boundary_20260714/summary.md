# HZ8 Medium Boundary Windows Gate

- Fresh process AB/BA rotation
- Runs: 5
- Work scale: 1x
- Baseline: `hz8-pre-transition-rollback`
- Candidate: `hz8`
- Diagnostic counters: disabled

| row | hz8-pre-transition-rollback ops/s | hz8 ops/s | delta | hz8-pre-transition-rollback peak RSS | hz8 peak RSS | RSS delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary4k | 15.323M | 159.209M | +939.01% | 818.89 MiB | 0.00 MiB | -100.00% |
| range4097_8192 | 23.474M | 23.643M | +0.72% | 38.63 MiB | 39.09 MiB | +1.19% |
| fixed8k | 163.664M | 164.171M | +0.31% | 9.64 MiB | 9.66 MiB | +0.24% |
| fixed16k | 154.790M | 158.142M | +2.17% | 9.85 MiB | 9.86 MiB | +0.08% |
| fixed32k | 38.216M | 36.880M | -3.50% | 11.00 MiB | 10.78 MiB | -2.02% |
| fixed64k | 10.678M | 10.487M | -1.79% | 9.21 MiB | 9.19 MiB | -0.25% |
| balanced | 51.059M | 261.787M | +412.72% | 789.99 MiB | 52.58 MiB | -93.34% |
| wide_ws | 50.102M | 133.430M | +166.32% | 427.18 MiB | 95.96 MiB | -77.54% |
| larger_sizes | 21.114M | 30.602M | +44.94% | 297.39 MiB | 61.32 MiB | -79.38% |

Acceptance guide: the target boundary should improve materially; balanced/wide controls must remain within -3%; peak RSS must remain within max(+5%, +1 MiB); all runs require alloc_fail=0.
