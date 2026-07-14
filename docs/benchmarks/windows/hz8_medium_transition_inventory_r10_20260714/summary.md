# HZ8 Medium Boundary Windows Gate

- Fresh process AB/BA rotation
- Runs: 10
- Work scale: 1x
- Baseline: `hz8`
- Candidate: `hz8-medium-transition-inventory`
- Diagnostic counters: disabled

| row | hz8 ops/s | hz8-medium-transition-inventory ops/s | paired delta | hz8 peak RSS | hz8-medium-transition-inventory peak RSS | RSS delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary4k | 165.006M | 165.923M | +0.98% | 0.00 MiB | 0.00 MiB | n/a |
| range4097_8192 | 25.586M | 54.061M | +109.82% | 39.42 MiB | 39.23 MiB | -0.48% |
| fixed8k | 174.800M | 167.417M | -4.50% | 9.62 MiB | 9.63 MiB | +0.06% |
| fixed16k | 159.964M | 159.682M | -1.50% | 9.88 MiB | 9.87 MiB | -0.06% |
| fixed32k | 38.681M | 52.585M | +35.09% | 10.82 MiB | 10.71 MiB | -0.99% |
| fixed64k | 9.798M | 12.973M | +24.26% | 9.22 MiB | 8.76 MiB | -4.96% |
| balanced | 243.801M | 252.725M | +4.67% | 52.54 MiB | 52.56 MiB | +0.04% |
| wide_ws | 144.946M | 141.601M | -2.03% | 84.20 MiB | 86.07 MiB | +2.22% |
| larger_sizes | 32.116M | 47.246M | +50.28% | 78.00 MiB | 57.04 MiB | -26.88% |

Acceptance guide: the target boundary should improve materially; balanced/wide controls must remain within -3%; peak RSS must remain within max(+5%, +1 MiB); all runs require alloc_fail=0.
