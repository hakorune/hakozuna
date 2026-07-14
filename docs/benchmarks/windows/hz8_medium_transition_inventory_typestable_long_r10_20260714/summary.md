# HZ8 Medium Boundary Windows Gate

- Fresh process AB/BA rotation
- Runs: 10
- Work scale: 10x
- Baseline: `hz8`
- Candidate: `hz8-medium-transition-inventory`
- Diagnostic counters: disabled

| row | hz8 ops/s | hz8-medium-transition-inventory ops/s | paired delta | hz8 peak RSS | hz8-medium-transition-inventory peak RSS | RSS delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary4k | 332.126M | 332.179M | +0.18% | 37.57 MiB | 37.62 MiB | +0.12% |
| range4097_8192 | 35.729M | 136.667M | +281.70% | 40.00 MiB | 39.84 MiB | -0.41% |
| fixed8k | 182.553M | 173.336M | -5.07% | 9.65 MiB | 9.63 MiB | -0.14% |
| fixed16k | 167.274M | 165.767M | +0.74% | 9.87 MiB | 9.87 MiB | +0.08% |
| fixed32k | 39.538M | 53.689M | +36.05% | 10.97 MiB | 11.05 MiB | +0.75% |
| fixed64k | 15.208M | 19.863M | +29.36% | 9.22 MiB | 8.76 MiB | -4.98% |
| balanced | 554.516M | 595.301M | +9.48% | 53.01 MiB | 52.97 MiB | -0.08% |
| wide_ws | 398.647M | 392.316M | -1.35% | 97.36 MiB | 97.36 MiB | 0.00% |
| larger_sizes | 51.679M | 131.066M | +154.34% | 97.98 MiB | 98.78 MiB | +0.81% |

Acceptance guide: the target boundary should improve materially; balanced/wide controls must remain within -3%; peak RSS must remain within max(+5%, +1 MiB); all runs require alloc_fail=0.
