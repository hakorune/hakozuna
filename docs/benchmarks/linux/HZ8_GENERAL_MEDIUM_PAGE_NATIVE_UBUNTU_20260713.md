# HZ8 General Medium Page native Ubuntu gate (2026-07-13)

## Decision

- `GeneralMediumPage`: `GO(correctness)/NO-GO(native performance)`
- `EntryBoundary-L1A`: `GO(Windows research)/NO-GO(cross-platform default)`
- Public/default remains HZ8 v2.
- Do not add the free-side split on this branch. The native larger-size tax is
  already outside the gate, while EntryBoundary does not recover it.

## Conditions

- Commit under test: `e06de12d065119ab1c87def31d80649c39124087`
- Native Ubuntu 22.04, Linux 6.8, x86_64
- AMD Ryzen 7 5825U, 8 cores / 16 threads
- GCC release speed binaries; diagnostic counters disabled
- `RUNS=10 WORK_SCALE=10`
- Fresh-process `baseline/general/entry_boundary` three-way rotation
- Values are medians of 10 samples per row and lane
- Raw results:
  `private/raw-results/linux/hz8_general_medium_page_r10_20260713/`

## Correctness

Both GCC and Clang rebuilt with `-Wall -Wextra -Werror` and passed:

- normal smoke: PASS
- exact GeneralMediumPage API smoke: PASS
- safety stress: PASS
- safety totals: `owner_exit=8`, `handoff=68`, `remote=8192`,
  `duplicate_claim=1`, `invalid=7`

## Throughput

| row | HZ8 v2 | General | vs v2 | EntryBoundary | vs v2 | Entry vs General |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed8k | 127.144M | 129.449M | +1.81% | 128.818M | +1.32% | -0.49% |
| fixed16k | 125.720M | 128.014M | +1.83% | 126.805M | +0.86% | -0.94% |
| fixed32k | 123.048M | 126.910M | +3.14% | 126.860M | +3.10% | -0.04% |
| balanced | 261.736M | 271.898M | +3.88% | 261.317M | -0.16% | -3.89% |
| wide_ws | 259.649M | 277.431M | +6.85% | 275.553M | +6.13% | -0.68% |
| larger_sizes | 128.471M | 111.655M | -13.09% | 112.301M | -12.59% | +0.58% |

The exact-size Windows gains do not transfer to this native Ubuntu host.
EntryBoundary is effectively neutral against General on all three exact rows.
The paired per-run EntryBoundary/v2 median for `larger_sizes` is `-11.94%`;
all ten paired samples are negative, so the rejection is not a ratio-of-medians
artifact.

## RSS

| row | v2 post | Entry post | v2 peak | Entry peak |
| --- | ---: | ---: | ---: | ---: |
| fixed8k | 1.625 MiB | 1.625 MiB | 1.625 MiB | 1.625 MiB |
| fixed16k | 1.750 MiB | 1.625 MiB | 1.750 MiB | 1.625 MiB |
| fixed32k | 1.625 MiB | 1.625 MiB | 1.625 MiB | 1.625 MiB |
| balanced | 1.625 MiB | 1.625 MiB | 1.625 MiB | 1.625 MiB |
| wide_ws | 1.625 MiB | 1.625 MiB | 1.625 MiB | 1.625 MiB |
| larger_sizes | 1.625 MiB | 1.625 MiB | 1.625 MiB | 1.625 MiB |

RSS is neutral at this workload's measurement resolution.

## Gate Reading

- Exact-row improvement remains positive but only `+0.86%` to `+3.10%` for
  EntryBoundary, far below the Windows signal.
- `balanced` and `wide_ws` remain inside the `-3%` guard versus HZ8 v2.
- `larger_sizes` fails the guard at `-12.59%`.
- EntryBoundary does not recover General's native layout/routing tax.
- Cross-platform/default promotion is rejected; retain both boxes as opt-in
  research lanes with immediate rollback to HZ8 v2.
