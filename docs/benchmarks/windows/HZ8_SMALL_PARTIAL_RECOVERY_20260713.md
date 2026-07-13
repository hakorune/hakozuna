# HZ8 Small Partial Recovery Windows Gate, 2026-07-13

## Decision

```text
P1 TransitionOnly Windows correctness: GO
P1 TransitionOnly Windows research lane: GO
LCG partial-span recovery and bounded RSS: GO
xorshift common-path default gate: NO-GO
remote-small no-regression: GO
cross-platform/default promotion: HOLD
public HZ8 default: unchanged
```

P1 improves the original depot on the Windows xorshift mixed controls, but it
does not remove enough of the Windows common-path cost to approach the public
default. It remains research evidence rather than a default candidate.

## Configuration

```text
host: Windows x86_64
compiler: clang-cl / MSVC ABI
rotation: fresh-process ABC/BCA/CAB
runs: 5
work scale: 1x Windows gate (long row counts)
diagnostic counters: disabled in all throughput binaries

A: hz8 public default
B: original SmallPartialTransitionDepot-L1
C: P1 TransitionOnlyMetadataTouch-L1B
```

The Windows comparison harness accepts an optional final trace argument:

```text
0: Windows LCG (default; backward compatible)
1: xorshift parity control
```

## LCG R5

| Row | Default | Original depot | P1 | Original/default | P1/default | P1/original | Default peak | Original peak | P1 peak |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| fixed8K | 161.529M | 149.845M | 175.543M | -7.23% | +8.68% | +17.15% | 9.67 MiB | 9.64 MiB | 9.64 MiB |
| fixed16K | 155.540M | 153.356M | 148.078M | -1.40% | -4.80% | -3.44% | 9.86 MiB | 9.88 MiB | 9.89 MiB |
| fixed32K | 38.165M | 35.528M | 35.455M | -6.91% | -7.10% | -0.20% | 10.81 MiB | 10.79 MiB | 10.82 MiB |
| balanced | 54.817M | 203.221M | 218.713M | +270.72% | +298.99% | +7.62% | 790.49 MiB | 52.45 MiB | 52.61 MiB |
| wide_ws | 52.771M | 134.554M | 138.350M | +154.98% | +162.17% | +2.82% | 427.57 MiB | 96.47 MiB | 96.23 MiB |
| larger_sizes | 21.979M | 30.434M | 30.259M | +38.47% | +37.67% | -0.58% | 297.94 MiB | 66.23 MiB | 68.53 MiB |

P1 retains the original depot's large LCG recovery and bounded peak RSS.

## Xorshift R5

| Row | Default | Original depot | P1 | Original/default | P1/default | P1/original | Default peak | Original peak | P1 peak |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| fixed8K | 135.092M | 134.751M | 135.887M | -0.25% | +0.59% | +0.84% | 10.71 MiB | 10.69 MiB | 10.73 MiB |
| fixed16K | 129.715M | 130.327M | 138.598M | +0.47% | +6.85% | +6.35% | 15.97 MiB | 15.98 MiB | 15.85 MiB |
| fixed32K | 107.135M | 103.455M | 107.880M | -3.44% | +0.70% | +4.28% | 25.45 MiB | 25.27 MiB | 25.32 MiB |
| balanced | 224.454M | 171.163M | 183.724M | -23.74% | -18.15% | +7.34% | 31.12 MiB | 30.54 MiB | 30.50 MiB |
| wide_ws | 159.840M | 143.959M | 146.654M | -9.94% | -8.25% | +1.87% | 53.99 MiB | 53.47 MiB | 53.56 MiB |
| larger_sizes | 27.921M | 28.942M | 28.824M | +3.66% | +3.23% | -0.41% | 42.12 MiB | 41.30 MiB | 40.57 MiB |

P1 consistently improves the original depot's xorshift fixed cost, but the
remaining mixed-row regressions are far outside the `-3%` default gate.

## Remote-Small R5

| Allocator | Median ops/s | Actual remote | Fallback | Peak RSS median |
|---|---:|---:|---:|---:|
| HZ8 default | 136.176M | 76.01% | 15.54% | 19.18 MiB |
| Original depot | 134.761M | 77.71% | 13.66% | 18.10 MiB |
| P1 TransitionOnly | 141.382M | 76.04% | 15.52% | 20.33 MiB |

Effective remote ratios vary because the bounded ring can fall back locally.
P1 is neutral-to-positive and stays within the remote RSS allowance.

## Correctness

```text
Windows all-HZ8 behavior/diagnostic build: PASS
Windows P1 smoke: PASS
Windows HZ8-only MT build: PASS
P1 diagnostic balanced:
  duplicate_push=0
  pop_reject=0
  index_mismatch=0
  commit_nonempty=0
  shutdown depth=0
```

The first unrestricted MT suite build exceeded the five-minute orchestration
timeout while compiling unrelated allocators. The HZ8-only build completed in
90.8 seconds and generated the P1 remote executable successfully.

## Reproduction

```powershell
pwsh win/build_win_allocator_suite.ps1 -OnlyHz8
pwsh win/run_win_hz8_small_partial_recovery_gate.ps1 `
  -Runs 5 -WorkScale 1 -Trace both

pwsh win/build_win_mt_remote_suite.ps1 -OnlyHz8
pwsh win/run_win_mt_remote_paper.ps1 `
  -Runs 5 -IncludeHz8Research `
  -Allocators hz8,hz8-small-partial-depot,hz8-small-partial-transition-only
```

Raw local results are under ignored `results/hz8-small-partial-recovery-win-r5/`.
Raw remote results are under ignored
`results/hz8-small-partial-recovery-remote-r5/`.

## SmallTierMembership-L1 Follow-up

Diagnostic-only attribution measured Mag16 duplicate-check work at 195,806
pointer comparisons on LCG and 608,400 on xorshift. An owner-local per-span
refcount reduced these to 12,257 and 59,889 O(1) checks while preserving every
P1 transition/push/pop/hit/reject count. A bit was explicitly rejected because
duplicate magazine hints require multiplicity.

Fresh-process four-way R5 then produced:

| Trace | Row | Membership/P1 | Membership/default | Membership peak |
|---|---|---:|---:|---:|
| LCG | balanced | +19.68% | +385.21% | 52.52 MiB |
| LCG | wide_ws | +10.52% | +165.62% | 95.34 MiB |
| LCG | larger_sizes | +13.71% | +41.47% | 65.88 MiB |
| xorshift | balanced | -3.90% | -11.76% | 30.47 MiB |
| xorshift | wide_ws | -0.76% | -9.32% | 53.58 MiB |
| xorshift | larger_sizes | -9.77% | -11.21% | 38.82 MiB |

Decision: `NO-GO(default)`. The linear scan is material on the LCG recovery
trace but is not the common-path blocker. Keep the lane only in the focused
reproduction gate; do not register it in normal or MT matrices. The dedicated
membership smoke passed with owner shutdown cleanup (`owners=4`, zero local and
remote failures).
