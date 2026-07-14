# HZ8 MediumTransitionInventory Native Ubuntu Gate

Date: 2026-07-14
Platform: native Ubuntu x86_64
Commit under test: `38e4b51e` plus the Linux lane/runner integration

## Contract

`MediumTransitionInventory-L1` is an opt-in owner-local medium-run box. It
publishes only a non-active, same-owner `FULL -> AVAILABLE` transition and
consumes one validated owner/class candidate only after an active-run miss.
The unchanged owner scan remains the reject/miss fallback. Slot state, owner
token, generation, directory ownership, and remote-pending authority remain
the validators; the inventory never establishes validity itself.

The GCC speed binary is built with:

```text
HZ8_DEFAULT_CFLAGS + H8_MEDIUM_TRANSITION_INVENTORY_L1=1
```

It has no diagnostic counters or atomics. GCC and Clang use the same flag for
preload, smoke, and safety checks; a separate GCC diagnostic binary supplies
inventory evidence only.

## Method

Fresh-process AB/BA R10. Each result is the median of the ten in-pair
candidate/baseline throughput ratios. All rows use the existing deterministic
working-set-ring worker and `remote_pct=0`.

## Results

| row | default ops/s | candidate ops/s | paired delta | default post RSS | candidate post RSS | default peak RSS | candidate peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4097..8192 | 6.537M | 16.854M | +157.49% | 2.69 MiB | 2.60 MiB | 35.50 MiB | 35.62 MiB |
| fixed 8 KiB | 101.145M | 99.935M | -1.57% | 2.47 MiB | 2.48 MiB | 4.12 MiB | 4.00 MiB |
| fixed 16 KiB | 73.959M | 74.059M | +1.05% | 2.24 MiB | 2.36 MiB | 4.00 MiB | 4.00 MiB |
| fixed 32 KiB | 45.589M | 46.469M | +2.10% | 2.24 MiB | 2.30 MiB | 4.00 MiB | 4.00 MiB |
| fixed 64 KiB | 24.660M | 34.480M | +38.35% | 2.24 MiB | 2.37 MiB | 3.06 MiB | 3.12 MiB |
| balanced | 40.812M | 42.342M | +0.64% | 2.39 MiB | 2.41 MiB | 25.50 MiB | 25.50 MiB |
| wide_ws | 21.332M | 21.399M | -2.19% | 3.31 MiB | 3.26 MiB | 43.00 MiB | 43.00 MiB |
| larger_sizes | 4.604M | 4.982M | +7.49% | 2.82 MiB | 2.86 MiB | 31.38 MiB | 31.38 MiB |

All per-process benchmark exits were zero, so allocator allocation failures
were zero. GCC and Clang preload, smoke, and safety stress passed.

The diagnostic 4097..8192 run observed `head_attempt=183389` and
`head_hit=183389`; `head_unusable`, duplicate, owner/class mismatch,
indexed-invalid states, and owner-exit residue were all zero.

## Gate And Decision

All Linux gates pass:

```text
4097..8192 >= +15%: PASS (+157.49%)
balanced / wide_ws >= -3%: PASS (+0.64% / -2.19%)
fixed 8/16/32/64 KiB >= -3%: PASS
post and peak RSS <= max(+5%, +1 MiB): PASS
allocation failure = 0; GCC/Clang safety: PASS
```

The contract is now cross-platform research GO. The shared public default
remains HOLD: the Windows R10 fixed-8KiB control was `-4.50%`, outside the
shared `-3%` bound. Re-open promotion only after that Windows control and an
application-like Windows rerun reconcile the remaining layout/noise question.

## Reproduction

```sh
RUNS=10 WORK_SCALE=1 \
  make -C hakozuna-hz8 medium-transition-inventory-gate
```
