# HZ8 Small Partial Transition Depot native Ubuntu gate, 2026-07-13

## Decision

`SmallPartialTransitionDepot-L1` passes native Linux correctness and remote
controls, but it does not pass the local throughput gate. Keep it as an
opt-in Windows research lane and do not add it to the shared HZ8 default.

```text
Windows behavior: GO
native Ubuntu correctness: GO
native Ubuntu performance: NO-GO
cross-platform/default promotion: HOLD
```

## Environment

```text
commit under test: dd4565555f0efd6d302a69865106f4dad7c03154
host: native Ubuntu 22.04, Linux 6.8.0-90-generic, x86_64
CPU: AMD Ryzen 7 5825U, 16 logical CPUs
GCC: 11.4.0
Clang: 14.0.0
local rows: fresh-process AB/BA R10, WorkScale=10, working-set ring
remote row: fresh-process AB/BA R10, 16 threads, 100K iterations/thread,
            16..4096 bytes, interleaved target remote=90%
diagnostic counters: disabled in every throughput binary
```

Baseline is the current Linux HZ8 default, GeneralMediumPage plus
EntryBoundary. Candidate adds only `H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1`.

## Result

| Row | Default | Candidate | Delta | Default post | Candidate post | Default peak | Candidate peak |
|---|---:|---:|---:|---:|---:|---:|---:|
| fixed8K | 125.025M | 126.811M | +1.43% | 2.28 MiB | 2.34 MiB | 3.88 MiB | 4.00 MiB |
| fixed16K | 123.477M | 125.855M | +1.93% | 2.11 MiB | 2.11 MiB | 4.00 MiB | 3.94 MiB |
| fixed32K | 94.481M | 94.684M | +0.21% | 2.18 MiB | 2.24 MiB | 4.00 MiB | 4.00 MiB |
| balanced | 152.003M | 142.445M | -6.29% | 2.41 MiB | 2.41 MiB | 26.75 MiB | 26.25 MiB |
| wide_ws | 96.976M | 90.064M | -7.13% | 3.09 MiB | 3.23 MiB | 49.62 MiB | 49.06 MiB |
| larger_sizes | 17.765M | 17.807M | +0.24% | 2.58 MiB | 2.53 MiB | 33.12 MiB | 32.81 MiB |
| remote-small | 52.023M | 51.834M | -0.36% | 13.49 MiB | 13.49 MiB | 22.56 MiB | 22.06 MiB |

The paired-run median deltas confirm that the two failing rows are not an
independent-median artifact: balanced is `-6.81%` and wide_ws is `-5.78%`.
All other throughput medians pass the `-3%` guard. Peak/post RSS is neutral
within about 0.15 MiB and does not reproduce the large Windows reduction.

The remote generator performed exactly the same work in both lanes:

```text
remote_enqueue=1,440,700
local_free=159,300
effective remote=90.04%
```

## Diagnostic Attribution

A separate atomic diagnostic build confirms that the box is wired and used.
On one WorkScale=10-equivalent sample per row:

| Row | Depot push | Pop hit | Pop reject | Index mismatch | Commit nonempty | Final depth |
|---|---:|---:|---:|---:|---:|---:|
| balanced, classes 6+7 | 24,965 | 24,761 | 0 | 0 | 0 | 0 |
| wide_ws, classes 5+6 | 25,977 | 25,482 | 0 | 0 | 0 | 0 |

Moving the transition calculation under the Mag16-full branch was also tested
as an isolated follow-up R10. Balanced remained outside the gate at `-7.06%`;
the source change was rejected and is not included in the commit.

## Correctness

GCC and Clang `-Werror` builds passed for the default and candidate. Candidate
smoke and safety stress retained:

```text
owner_exit=8
handoff=68
remote=8192
duplicate_claim=1
invalid=7
allocation failure=0
```

The release candidate binary contains no depot diagnostic atomics. The new
diagnostic target is explicitly separate from the speed lane.

## Reproduction

```sh
RUNS=10 WORK_SCALE=10 REMOTE_WORK_SCALE=1 \
  make -C hakozuna-hz8 small-partial-depot-gate

make -C hakozuna-hz8 bench-release-small-partial-depot-diag
```

Raw logs are under
`private/raw-results/linux/hz8_small_partial_depot_r10_20260713/`.
