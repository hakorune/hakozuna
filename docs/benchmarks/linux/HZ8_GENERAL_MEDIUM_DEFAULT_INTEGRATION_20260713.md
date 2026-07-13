# HZ8 General Medium default integration on native Ubuntu (2026-07-13)

## Decision

- `GeneralMediumDefaultIntegration-L1`: `GO(Linux default)`
- `HZ8_V2_ROLLBACK_CFLAGS`: `GO(rollback)`
- Shared Windows/Linux public label: `HOLD(Windows integration)`

The Linux `preload`, `smoke`, `safety-stress`, and `bench-release` targets now
select GeneralMediumPage + EntryBoundary-L1A. The prior HZ8 v2 behavior remains
available through explicit `*-v2-rollback` targets.

## Validation

- Native Ubuntu 22.04, Linux x86_64, AMD Ryzen 7 5825U
- GCC and Clang `-Wall -Wextra -Werror`: PASS
- default and rollback normal smoke: PASS
- default and rollback safety stress: PASS
- exact 8K/16K/32K API smoke: PASS
- default LD_PRELOAD smoke: PASS
- diagnostic Page counter symbols in default speed binary: none
- UnifiedDomain and Mag32 dry-run flags remain rooted in rollback flags
- 120/120 benchmark samples: working-set integrity PASS

## Default Versus Rollback R10

Fresh-process AB/BA, Windows-equivalent slot-ring worker, `WORK_SCALE=10`:

| row | rollback | default | delta | rollback post | default post | rollback peak | default peak |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed8k | 69.158M | 128.702M | +86.10% | 2.04 MiB | 2.29 MiB | 4.25 MiB | 4.06 MiB |
| fixed16k | 44.524M | 123.189M | +176.68% | 2.23 MiB | 2.12 MiB | 4.38 MiB | 4.00 MiB |
| fixed32k | 24.824M | 94.637M | +281.23% | 2.37 MiB | 2.36 MiB | 4.38 MiB | 4.12 MiB |
| balanced | 151.899M | 157.101M | +3.42% | 2.42 MiB | 2.42 MiB | 26.75 MiB | 26.75 MiB |
| wide_ws | 92.337M | 96.266M | +4.25% | 3.08 MiB | 3.08 MiB | 49.62 MiB | 49.62 MiB |
| larger_sizes | 17.884M | 17.946M | +0.35% | 2.56 MiB | 2.56 MiB | 33.12 MiB | 33.12 MiB |

Paired medians are `+85.82%`, `+175.21%`, `+281.24%`, `+3.39%`,
`+3.64%`, and `+0.41%` in table order. No control row regresses.

Raw results:
`private/raw-results/linux/hz8_general_default_integration_r10_20260713/`.

## Boundary

This is a build-selection promotion only. Ownership, generation, slot-state,
remote publication, and residency contracts are unchanged. Existing research
lanes do not inherit the promoted flags. Windows must switch its normal
`hz8-v2` lane and pass a native MSVC build before the shared public label moves.
