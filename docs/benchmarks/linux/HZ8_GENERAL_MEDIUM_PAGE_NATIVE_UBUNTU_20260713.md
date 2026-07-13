# HZ8 General Medium Page native Ubuntu gate (2026-07-13)

## Decision

- `GeneralMediumPage`: `GO(correctness/performance research)`
- `EntryBoundary-L1A`: `GO(Windows/Linux x64 research candidate)`
- Public/default remains HZ8 v2.
- The recorded Linux R10 is an immediate-local-free control, not a workload-
  equivalent reproduction of the Windows slot-ring gate. Do not use it alone
  to accept or reject cross-platform performance.

The corrected slot-ring parity R20 passes every throughput and RSS guard. This
clears the Windows/Linux research gate, but does not itself change the public
default.

## Immediate-Free Control Conditions

- Commit under test: `e06de12d065119ab1c87def31d80649c39124087`
- Native Ubuntu 22.04, Linux 6.8, x86_64
- AMD Ryzen 7 5825U, 8 cores / 16 threads
- GCC release speed binaries; diagnostic counters disabled
- `RUNS=10 WORK_SCALE=10`
- Fresh-process `baseline/general/entry_boundary` three-way rotation
- All rows used `--interleaved 1 --remote-pct 0`, which allocates and frees one
  object per iteration. In this mode `--live-window` sizes only the unused
  remote inbox; it does not retain a local working set.
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

The parity worker additionally checked all 360 fresh-process samples:

- `working_set_ring=1`
- allocation count equals final free count
- retained `max_live_per_thread > 1`

## Working-Set Parity R20

The corrected worker retains the Windows-equivalent per-thread slot ring. The
table combines two independent fresh-process R10 campaigns.

| row | HZ8 v2 | General | vs v2 | EntryBoundary | vs v2 | Entry vs General |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed8k | 69.168M | 129.083M | +86.62% | 127.991M | +85.04% | -0.85% |
| fixed16k | 44.428M | 124.795M | +180.89% | 123.178M | +177.25% | -1.30% |
| fixed32k | 24.747M | 94.382M | +281.38% | 94.957M | +283.71% | +0.61% |
| balanced | 153.440M | 155.715M | +1.48% | 156.675M | +2.11% | +0.62% |
| wide_ws | 94.928M | 96.501M | +1.66% | 94.730M | -0.21% | -1.84% |
| larger_sizes | 17.938M | 17.853M | -0.47% | 17.902M | -0.20% | +0.28% |

Paired per-run EntryBoundary/v2 medians across the 20 pairs are fixed8K
`+86.09%`, fixed16K `+176.65%`, fixed32K `+282.03%`, balanced `+1.11%`, wide
`+0.19%`, and larger `-0.01%`.

| row | v2 post | Entry post | v2 peak | Entry peak |
| --- | ---: | ---: | ---: | ---: |
| fixed8k | 1.96 MiB | 2.22 MiB | 4.25 MiB | 4.00 MiB |
| fixed16k | 2.23 MiB | 2.23 MiB | 4.31 MiB | 4.00 MiB |
| fixed32k | 2.30 MiB | 2.24 MiB | 4.25 MiB | 4.12 MiB |
| balanced | 2.39 MiB | 2.43 MiB | 26.75 MiB | 26.75 MiB |
| wide_ws | 3.09 MiB | 3.09 MiB | 49.62 MiB | 49.62 MiB |
| larger_sizes | 2.54 MiB | 2.54 MiB | 33.12 MiB | 33.12 MiB |

Raw parity results:
`private/raw-results/linux/hz8_general_medium_page_ws_parity_r10_20260713/`.
Second campaign:
`private/raw-results/linux/hz8_general_medium_page_ws_parity_r10_repeat2_20260713/`.

## Immediate-Free Control

| row | HZ8 v2 | General | vs v2 | EntryBoundary | vs v2 | Entry vs General |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed8k | 127.144M | 129.449M | +1.81% | 128.818M | +1.32% | -0.49% |
| fixed16k | 125.720M | 128.014M | +1.83% | 126.805M | +0.86% | -0.94% |
| fixed32k | 123.048M | 126.910M | +3.14% | 126.860M | +3.10% | -0.04% |
| balanced | 261.736M | 271.898M | +3.88% | 261.317M | -0.16% | -3.89% |
| wide_ws | 259.649M | 277.431M | +6.85% | 275.553M | +6.13% | -0.68% |
| larger_sizes | 128.471M | 111.655M | -13.09% | 112.301M | -12.59% | +0.58% |

Under immediate local reuse, the exact-size Windows gains are not reproduced.
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

- This is a valid immediate-local-churn control, but not the requested Windows-
  equivalent working-set gate. Windows retains 256 objects per exact row and
  4096/16384 objects in the mixed rows through a slot ring.
- Exact-row improvement remains positive but only `+0.86%` to `+3.10%` for
  EntryBoundary, far below the Windows signal.
- `balanced` and `wide_ws` remain inside the `-3%` guard versus HZ8 v2.
- `larger_sizes` fails the guard at `-12.59%`.
- EntryBoundary does not recover General's native layout/routing tax.
- Cross-platform/default promotion remains HOLD until a separate Linux slot-
  ring worker reproduces the Windows working-set shape. Retain both boxes as
  opt-in research lanes with immediate rollback to HZ8 v2.

The parity rerun now supersedes that HOLD condition:

- all three exact rows improve substantially;
- balanced, wide, and larger remain within the `-3%` guard;
- peak and post RSS are equivalent at the workload scale;
- Windows/Linux x64 research-candidate gate: GO;
- public/default integration: unchanged, requiring a separate promotion box.
