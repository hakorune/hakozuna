# HZ8 Windows General Medium Page Gate L1

Updated: 2026-07-13

## Scope

`GeneralMediumPageSubstrateExpansion-L1` extends the existing 64 KiB Page8K
run substrate to exact 8 KiB, 16 KiB, and 32 KiB classes. The public HZ8 v2
default remains unchanged. Speed rows contain no diagnostic counters or added
atomics.

The dedicated runner is:

```powershell
pwsh win/run_win_hz8_page_general_gate.ps1 -Runs 10 -WorkScale 10
```

It uses fresh processes and alternates AB/BA order. Raw output is written below
`results/hz8-page-general-gate/` and is ignored by Git.

## Correctness

```text
general API smoke: PASS
general remote smoke: claims=14, drained=14, reject=0, lost=0

exact 8K/16K/32K:
  route VALID
  exact usable size
  interior free INVALID
  stale free INVALID
```

The fixed-8K API remains a compatibility wrapper over class 0. General geometry
is immutable per page. Owner lifecycle, generation, slot-state authority, and
remote publication ordering are unchanged.

## Cap64 Long Gate

Windows fresh-process AB/BA, RUNS=10, work scale 10, baseline HZ8 v2:

| row | throughput delta | peak RSS delta |
| --- | ---: | ---: |
| fixed8K | +68.77% | -20 KiB |
| fixed16K | +215.54% | -170 KiB |
| fixed32K | +97.60% | +60 KiB |
| balanced | -9.23% | -20.89 MiB |
| wide_ws | +8.05% | -6.90 MiB |
| larger_sizes | -0.09% | +0.91 MiB |

The exact-size signal is strong and RSS-neutral. The candidate is not a public
default because the small-only `balanced` control regresses even though it never
executes the page backend. This is attributed to common-entry code placement or
layout tax, not page behavior.

## 32K Cap Attribution

With owner page cap 64 and the fixed32K diagnostic row:

```text
alloc_attempt:   400384
alloc_served:    210163
page_cap_reject: 190221
```

`alloc_attempt - alloc_served == page_cap_reject`. A 32 KiB class has two slots
per 64 KiB page, so the 256-object working set requires 128 pages per owner.

The cap128 sibling removes the boundary exactly:

```text
alloc_attempt:   400384
alloc_served:    400384
page_cap_reject: 0
```

## Cap128 Long Gate

Direct cap64 versus cap128, Windows fresh-process AB/BA, RUNS=10, work scale 10:

| row | throughput delta | peak RSS delta |
| --- | ---: | ---: |
| fixed8K | -0.84% | +4 KiB |
| fixed16K | -3.37% | +24 KiB |
| fixed32K | +355.18% | -176 KiB |
| balanced | -5.73% | +29.00 MiB |
| wide_ws | -3.39% | +3.22 MiB |
| larger_sizes | +6.58% | -0.70 MiB |

Cap128 is a 32K specialist evidence lane. It is not promoted because two control
rows exceed the -3% gate. Cap64 remains the general research candidate.

## Closed Probe

`ColdDispatch-L1` only marked the non-small malloc and non-arena free branches
as unlikely. It retained exact-size gains but produced `balanced -10.78%` in the
long gate. The flag and build row were removed. Branch hints do not provide a
stable function-layout boundary.

## Decision

```text
GeneralMediumPageSubstrateExpansion-L1 cap64:
  correctness GO
  exact-size performance GO
  public/default promotion HOLD

cap128:
  32K specialist evidence
  HOLD

ColdDispatch-L1:
  NO-GO / removed

HZ8 v2:
  public default unchanged
```

## EntryBoundary-L1A Result

L1A moved only the non-small malloc dispatch into a `noinline` helper. The
small malloc body, free behavior, route authority, and page lifecycle were not
changed. Windows fresh-process AB/BA, RUNS=10, work scale 10, versus HZ8 v2:

| row | throughput delta | peak RSS delta |
| --- | ---: | ---: |
| fixed8K | +19.95% | +18 KiB |
| fixed16K | +146.22% | -176 KiB |
| fixed32K | +96.36% | +224 KiB |
| balanced | -0.14% | -3.86 MiB |
| wide_ws | +3.64% | +0.38 MiB |
| larger_sizes | +0.55% | -0.27 MiB |

The candidate clears every control gate and retains material exact-size gains.
The same-flag API smoke passes. A free-side split is not justified because the
malloc-only boundary already removes the observed control regression.

```text
EntryBoundary-L1A:
  Windows research GO
  cross-platform/default HOLD

next:
  native Linux gate with the same flag shape
  do not add free-side split unless Linux or a later control row requires it
```

## Linux Validation Surface

The Linux build now exposes separate speed targets for the substrate and the
entry boundary:

```bash
make -C hakozuna-hz8 bench-release-page-general
make -C hakozuna-hz8 bench-release-page-general-entry-boundary
make -C hakozuna-hz8 smoke-page-general-entry-boundary-api
make -C hakozuna-hz8 safety-stress-page-general-entry-boundary
```

The dedicated runner uses fresh processes and a three-way rotation across HZ8
v2, general geometry without the boundary, and EntryBoundary-L1A:

```bash
RUNS=10 WORK_SCALE=10 \
  bash hakozuna-hz8/scripts/run_hz8_page_general_linux_gate.sh
```

WSL2 validation passed the `-Werror` build, API smoke, and safety stress. Its
performance samples were not stable enough for promotion: even control rows
changed by double-digit percentages between short campaigns. WSL is therefore
correctness and directional evidence only. Native Ubuntu remains the required
cross-platform performance gate.

```text
Windows performance:      GO research candidate
WSL correctness/safety:   GO
WSL performance verdict:  not admissible
native Ubuntu verdict:    pending
public/default:            unchanged
```
