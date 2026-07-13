# HZ8 Linux Working Set Parity L0

Updated: 2026-07-13

## Purpose

Provide a benchmark-only Linux worker with the same retained slot-ring shape as
the Windows GeneralMediumPage gate. This box changes no allocator behavior and
does not replace the existing interleaved or phase workers.

## Boundary

- Add an explicit `--working-set-ring 0|1` benchmark option.
- `--working-set-ring 1` requires `--live-window N` and local frees only.
- Each thread owns an `N`-entry pointer ring.
- Each timed iteration selects one slot. An occupied slot is freed; an empty
  slot receives one allocation and the first `min(size, 64)` bytes are touched.
- Remaining live slots are freed before the worker completes, matching the
  Windows worker lifetime.
- Existing `--interleaved` semantics and all production allocator binaries are
  unchanged.

## Observation

The worker reports its selected mode and working-set size through the existing
summary line. A focused smoke must prove that the ring retains more than one
object; production speed lanes receive no allocator counter or atomic.

## Gate

Run fresh-process three-way rotation, `RUNS=10`, with the Windows row shapes:

| row | threads | iterations/thread | working set/thread | size |
| --- | ---: | ---: | ---: | --- |
| fixed8k | 4 | 2,000,000 | 256 | 8192 |
| fixed16k | 4 | 1,200,000 | 256 | 16384 |
| fixed32k | 4 | 800,000 | 256 | 32768 |
| balanced | 8 | 250,000 | 4096 | 16..2048 |
| wide_ws | 8 | 200,000 | 16384 | 16..1024 |
| larger_sizes | 4 | 150,000 | 4096 | 256..8192 |

Acceptance remains:

- exact rows improve versus HZ8 v2;
- balanced, wide, and larger regress no more than 3%;
- peak and post RSS remain equivalent;
- GCC/Clang `-Werror`, API smoke, and safety stress pass.

Only after this parity gate may the cross-platform performance decision move
away from HOLD. `GeneralMediumSingleDecode-L1B` is a separate later behavior
box and must not be stacked before this baseline is captured.

## Result

Two independent native Ubuntu x86_64 fresh-process three-way R10 campaigns
(combined R20) passed the parity gate:

| row | HZ8 v2 | General | EntryBoundary | Entry vs v2 |
| --- | ---: | ---: | ---: | ---: |
| fixed8k | 69.168M | 129.083M | 127.991M | +85.04% |
| fixed16k | 44.428M | 124.795M | 123.178M | +177.25% |
| fixed32k | 24.747M | 94.382M | 94.957M | +283.71% |
| balanced | 153.440M | 155.715M | 156.675M | +2.11% |
| wide_ws | 94.928M | 96.501M | 94.730M | -0.21% |
| larger_sizes | 17.938M | 17.853M | 17.902M | -0.20% |

All 360 samples reported `working_set_ring=1`, balanced allocation/free totals,
and a retained maximum above one object. Peak RSS was neutral within 0.07 MiB
on control rows and lower on all three exact rows.

Decision: `GO(tooling)` for the parity worker and `GO(Windows/Linux research
candidate)` for EntryBoundary. Public HZ8 v2 remains unchanged until an explicit
default-integration box is accepted.
