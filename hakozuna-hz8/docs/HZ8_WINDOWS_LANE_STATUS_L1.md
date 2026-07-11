# HZ8 Windows Lane Status L1

This page is the short Windows lane map. It separates public/default evidence,
promotion candidates, diagnostics, and closed experiments.

## Normal Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v2` | public default | Frozen KeepRefill balanced baseline |
| `hz8-reusable-span-mag16` | candidate | Owner-local reusable small-span inventory; pending Linux/full gate |

The normal allocator matrix and MT remote runner include these two HZ8 rows.

## Research Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v3-adaptive-shadow` | diagnostic evidence | Closed adaptive-transfer attribution |
| `hz8-reclaim-shadow` | diagnostic evidence | Owner-retirement reclaimable upper bound |
| `hz8-speed-attribution` | diagnostic-only | Existing hot counter attribution; never a speed result |

Research rows are excluded from normal runs. Use `-IncludeHz8Research`
explicitly when they are needed.

## Closed Lanes

```text
HZ8ReclaimAdapterBehavior-L1:
  NO-GO and removed
  commit-time bounded scans did not reduce peak RSS materially and regressed
  throughput

HZ8AdaptiveTransfer-L1:
  NO-GO
  HZ8 already bulk-splices pending bitmap words

LargeDirect cache variants:
  opt-in historical evidence
  not part of this default/candidate comparison
```

## Commands

```powershell
# Build HZ8 default, candidate, and diagnostic artifacts.
.\win\build_win_allocator_suite.ps1 -OnlyHz8
.\win\build_win_mt_remote_suite.ps1 -OnlyHz8

# List/run normal HZ8 rows.
.\win\run_win_allocator_matrix.ps1 -ListOnly
.\win\run_win_allocator_matrix.ps1 `
  -Allocators hz8-v2,hz8-reusable-span-mag16

# Add diagnostic research rows intentionally.
.\win\run_win_allocator_matrix.ps1 -ListOnly -IncludeHz8Research
```

## Promotion Gate

Mag16 remains compile-time opt-in until all of the following pass:

- Linux GCC and Clang smoke/safety stress;
- Linux paired local, remote, peak-RSS, and post-RSS rows;
- broader Windows public matrix;
- no route, generation, pending, duplicate-free, or owner-exit regression.

Current cross-platform status after the full-magazine churn fix:

```text
Linux correctness/local:
  GO

Linux focused remote90:
  near-neutral (-1.22%), RSS neutral

Windows local and fixed MT remote follow-up:
  GO candidate signal

public default:
  HOLD until the broader public matrix is rerun
```
