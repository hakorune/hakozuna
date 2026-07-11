# HZ8 Windows Lane Status L1

This page is the short Windows lane map. It separates public/default evidence,
promotion candidates, diagnostics, and closed experiments.

## Normal Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v2` | public default | KeepRefill balanced default with Mag16 reusable small-span inventory |

## Candidate Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v2-mag32` | Windows GO / global HOLD | Larger/local capacity candidate; explicit research selection only |

Windows local and RSS gates are positive, but Linux small/remote gates block a
cross-platform default promotion. Keep this row behind
`-IncludeHz8Research` in normal Windows runners.

The normal allocator matrix and MT remote runner include only the public HZ8
row unless research controls are requested explicitly.

## Research Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v2-nomag` | control | Pre-promotion HZ8 v2 without Mag16 |
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
# Build HZ8 default, control, and diagnostic artifacts.
.\win\build_win_allocator_suite.ps1 -OnlyHz8
.\win\build_win_mt_remote_suite.ps1 -OnlyHz8

# List/run normal HZ8 rows.
.\win\run_win_allocator_matrix.ps1 -ListOnly

# Add diagnostic research rows intentionally.
.\win\run_win_allocator_matrix.ps1 -ListOnly -IncludeHz8Research
```

## Promotion Gate

Mag16 passed all of the following promotion gates:

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
  GO

public default:
  PROMOTED
```
