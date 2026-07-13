# HZ8 Small Partial Depot trace parity and recovery, 2026-07-13

## Decision

The original native Ubuntu NO-GO was trace-specific, not an OS backing
failure. With the exact Windows LCG ring trace, native Ubuntu reproduces the
same linear span-commit/RSS problem and the original depot removes it.

```text
trace-parity behavior: GO
Windows/Ubuntu ownership and reclaim contract: common
P1 TransitionOnlyMetadataTouch: GO(best research)/HOLD(default)
P2 ColdActivate: NO-GO
P3 Hint1: NO-GO
P4 ColdInactiveFree: NO-GO
shared default promotion: HOLD
```

P1 is the best recovery, but its xorshift R20 balanced result remains 0.45
percentage points outside the `-3%` guard. No behavior flag is added to the
public default.

## Environment

```text
base commit: 45cd4cd8
host: native Ubuntu 22.04, Linux 6.8.0-90-generic, x86_64
CPU: AMD Ryzen 7 5825U, 16 logical CPUs
GCC: 11.4.0
local rows: fresh-process rotation R10, WorkScale=10
remote row: 16 threads, 100K iterations/thread, effective remote=90.04%
speed diagnostics: disabled
```

## P0 Trace Parity

`--working-set-lcg 1` uses the Windows formula and seeds:

```text
state = state * 1664525 + 1013904223
seed = 1234 + thread index
same random value selects ring slot and allocation size
```

The bench emits a deterministic hash for the first 65,536 events per thread.
An independent Python oracle in the gate runner matched all six row hashes and
all default/candidate runs.

| Row | Default | Original depot | Delta | Default peak | Depot peak |
|---|---:|---:|---:|---:|---:|
| fixed8K | 166.783M | 163.455M | -2.00% | 5.50 MiB | 5.56 MiB |
| fixed16K | 142.677M | 143.268M | +0.41% | 5.50 MiB | 5.50 MiB |
| fixed32K | 24.742M | 24.597M | -0.59% | 5.88 MiB | 6.00 MiB |
| balanced | 18.376M | 155.341M | +745.35% | 770.62 MiB | 48.38 MiB |
| wide_ws | 25.642M | 76.250M | +197.36% | 422.25 MiB | 92.25 MiB |
| larger_sizes | 7.922M | 16.580M | +109.30% | 277.00 MiB | 59.88 MiB |
| remote-small | 52.404M | 52.646M | +0.46% | 21.81 MiB | 21.31 MiB |

This reproduces the Windows failure shape on Linux. The earlier xorshift trace
did not create enough Mag16 overflow phase amplitude, so it exposed only the
candidate's common-path tax.

## Recovery Boxes

### P1 TransitionOnlyMetadataTouch-L1B

P1 leaves active-span free before depot metadata and performs bump/transition
work only below the inactive Mag16-full boundary. It retains the intrusive
depot, full capacity, and existing Mag/depot uniqueness protection.

Xorshift two-campaign R20:

| Row | Original/default | P1/default | P1/original |
|---|---:|---:|---:|
| fixed8K | -0.14% | +0.58% | +0.72% |
| fixed16K | -0.44% | +0.41% | +0.85% |
| fixed32K | +0.32% | -0.71% | -1.03% |
| balanced | -6.55% | -3.45% | +3.31% |
| wide_ws | -3.59% | -2.55% | +1.08% |
| larger_sizes | -0.75% | +0.45% | +1.22% |
| remote-small | +1.83% | +2.92% | +1.07% |

Windows-LCG parity R10 retained bounded behavior:

| Row | P1/default | P1 peak |
|---|---:|---:|
| balanced | +737.79% | 48.31 MiB |
| wide_ws | +200.37% | 92.12 MiB |
| larger_sizes | +112.82% | 59.88 MiB |

Fixed and remote rows stayed inside `-3%`. P1 is the selected recovery
research lane, but balanced does not clear the default gate.

### Rejected Boxes

| Box | Xorshift balanced | Xorshift wide | LCG result | Decision |
|---|---:|---:|---|---|
| P2 ColdActivate-L1B | -6.26% | -3.04% | bounded, gains retained | NO-GO; empty-pop isolation is insufficient |
| P3 Hint1-L1C | -5.26% | -2.96% | lost all mixed-row gains and RSS bounds | NO-GO; capacity one cannot absorb phase amplitude |
| P4 ColdInactiveFree-L1D | -8.79% | -3.05% | bounded, gains retained | NO-GO; inactive-free call tax exceeds code-size benefit |

P4 reduced `h8_free_inner` from P1's `0x329` bytes to `0x291`, below the
default `0x2a1`, but throughput became worse. Dynamic call frequency, not text
size alone, controls this row.

Removing the speed Mag-pop indexed check was also measured after P1. It did
not improve xorshift (`balanced -3.86%`, `wide -3.22%`) and was reverted.

## Correctness

The selected P1 diagnostic LCG sample reported active push/pop use with:

```text
duplicate_push=0
pop_reject=0
index_mismatch=0
commit_nonempty=0
final depth=0
```

All behavior remains owner-local advisory inventory. Owner, generation,
class, state, exact slot, pending, and remote publication remain authority.
GCC and Clang `-Werror` preload, smoke, safety, release, and diagnostic builds
pass for original, P1, P2, P3, and P4. Every speed binary is free of depot
diagnostic symbols.

## Reproduction

```sh
# Original default/depot trace-parity gate.
RUNS=10 WORK_SCALE=10 TRACE_PARITY=1 \
  make -C hakozuna-hz8 small-partial-depot-gate

# Three-way recovery gate; override recovery target/bin/label for P2-P4.
RUNS=10 WORK_SCALE=10 TRACE_PARITY=0 \
  make -C hakozuna-hz8 small-partial-recovery-gate
```

Raw results live under `private/raw-results/linux/hz8_small_partial_*_20260713/`.
