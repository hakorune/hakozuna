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
| `hz8-r3-page8k-integrated` | Windows selected opt-in / global HOLD | Exact-8KiB detached-page substrate; strong Windows local result with neutral application gates |
| `hz8-r3-page8k-target-dispatch` | Windows validation pending / global HOLD | R3 child lane that avoids non-target alloc dispatch and owner creation on free miss; native Ubuntu fixed8K +31.90% |
| `hz8-r3-page8k-range4097` | Windows evidence / NO-GO speed candidate | Same 8KiB geometry for 4097..8192 requests; correctness passes but focused throughput is about 12.7% below HZ8 v2 |
| `hz8-small-available4k` | Windows GO / global HOLD | O(1) class-8 reuse visibility; about 9.7x fixed-4KiB speedup and much lower peak RSS |

Windows local and RSS gates are positive, but Linux small/remote gates block a
cross-platform default promotion. Keep this row behind
`-IncludeHz8Research` in normal Windows runners.

The 2KiB+4KiB expansion is not another candidate. It remains buildable only
to reproduce the class-sweep result; its Windows `wide_ws` row reaches the
-5% boundary and its Linux directional gate fails.

The R3 page8K row improves Windows fixed-8K local throughput by 81.77% and
passes balanced, wide working-set, larger-size, remote-safety, and two
Redis-like no-regression gates. Linux fixed-8K is neutral (-0.21%), so R3 is a
Windows selected opt-in rather than a cross-platform default.

The target-dispatch child is not a Windows selection yet. Native Ubuntu R10
cleared the fixed8K gate (`+31.90%`) and preserved median RSS, but balanced
(`-3.72%`) and larger_sizes (`-10.71%`) keep the cross-platform gate on HOLD.
Windows must compare v2, integrated R3, and target dispatch in the same batch.

The normal allocator matrix and MT remote runner include only the public HZ8
row unless research controls are requested explicitly.

## Research Lanes

| Lane | Status | Purpose |
|---|---|---|
| `hz8-v2-nomag` | control | Pre-promotion HZ8 v2 without Mag16 |
| `hz8-v3-adaptive-shadow` | diagnostic evidence | Closed adaptive-transfer attribution |
| `hz8-reclaim-shadow` | diagnostic evidence | Owner-retirement reclaimable upper bound |
| `hz8-magazine-tail-shadow` | diagnostic evidence / closed | Source-refill checkpoint upper bound; behavior NO-GO at only 1.0-1.2MiB maximum |
| `hz8-speed-attribution` | diagnostic-only | Existing hot counter attribution; never a speed result |
| `hz8-medium-fixed8k-cost-audit` | diagnostic-only | Cross-platform active-block path attribution; no behavior or promotion claim |
| `hz8-small-available2k4k` | Windows evidence / global NO-GO | Large fixed 2K/4K gains, but Windows wide reaches -5% and Linux directional rows regress |

Research rows are excluded from normal runs. Use `-IncludeHz8Research`
explicitly when they are needed.

The fixed-8K cost audit is now complete on native Windows and Linux/WSL:
250,000 same-owner active hits, 250,000 owner-matched frees, and zero active
misses in each diagnostic run. The audit identifies the executed path but does
not justify removing state/generation checks or adding another cache.

Build target existence is not a support claim. Candidate, evidence,
diagnostic, and closed rows may all remain buildable so old measurements can
be reproduced; the status tables on this page are authoritative.

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
.\win\build_win_hz8_redis_r3_gate.ps1

# List/run normal HZ8 rows.
.\win\run_win_allocator_matrix.ps1 -ListOnly

# Add diagnostic research rows intentionally.
.\win\run_win_allocator_matrix.ps1 -ListOnly -IncludeHz8Research
.\win\run_win_redis_workload_paper.ps1 -IncludeHz8Research -Runs 10
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
