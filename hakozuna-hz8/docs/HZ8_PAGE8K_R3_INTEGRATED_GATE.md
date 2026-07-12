# HZ8 Page8K R3 Integrated Gate

## Purpose

R3 evaluates the page8K substrate through the real HZ8 medium entry. It is a
research sibling, not a standalone substrate score and not a default change.

```text
lane:
  hz8-r3-page8k-integrated

control:
  hz8-v2

scope:
  exact 8KiB allocation/free only

unchanged:
  all other small, medium, large, ownership, and route paths
```

Diagnostics use a separate `hz8-r3-page8k-integrated-diag` binary. Atomic
counters are forbidden in the speed lane.

## Paired Rows

```text
fixed8k_local:
  T=8, iters=500000, ws=400, size=8192, remote=0

balanced:
  T=8, iters=250000, ws=4096, size=16..2048

wide_ws:
  T=8, iters=200000, ws=16384, size=16..1024

larger_sizes:
  T=4, iters=150000, ws=4096, size=256..8192

fixed8k_remote90_control:
  T=8, iters=500000, ws=400, size=8192, remote=90
```

Run baseline/candidate in alternating AB/BA order. Use at least repeat-5 median.

## Gate

```text
accept:
  fixed8k_local >= +30%
  balanced >= -3%
  wide_ws >= -3%
  larger_sizes >= -5%
  repeat-3 median peak RSS <= baseline on the control rows
  allocation failure = 0
  safety and lifecycle smoke pass

no-go:
  fixed8k_local < +30%
  any control row exceeds its regression bound
  owned-looking invalid reaches fallback
  speed lane contains diagnostic atomics
  remote90 is used as a promotion claim
```

Remote90 remains lifecycle evidence only. R3 cannot be promoted from standalone
page throughput or remote safety; only the integrated paired rows decide.

## Windows Result

The corrected paired run used the explicit benchmark argument list. An earlier
local helper used PowerShell's reserved `$args` name and is invalid evidence.

```text
throughput, alternating AB/BA, repeat-5 median:
  fixed8k_local: baseline 129.191M, candidate 234.830M, +81.77%
  balanced:      baseline  52.477M, candidate  58.669M, +11.80%
  wide_ws:       baseline  49.299M, candidate  51.135M,  +3.72%
  larger_sizes:  baseline  21.573M, candidate  24.149M, +11.94%

peak RSS, repeat-3 median:
  fixed8k_local: baseline 20.98MiB, candidate 12.40MiB
  balanced:      baseline 789.69MiB, candidate 770.78MiB
  wide_ws:       baseline 426.31MiB, candidate 423.45MiB
  larger_sizes:  baseline 297.55MiB, candidate  63.84MiB
```

Windows peak RSS has substantial run-to-run variance in the broad rows. These
values are a regression guard, not a paper claim. The candidate did not exceed
the baseline median on any measured row.

The exact-8K remote90 speed lane completed repeat-5 with zero allocation
failure. A separate diagnostic run reported 257,545 claims and drains, zero
reject, zero lost notification, and eight clean owner closes. Long runs can
fall below the requested effective remote percentage because the benchmark
ring falls back locally; remote90 therefore remains correctness evidence only.

## Decision

```text
R3 integrated candidate: GO
HZ8 public default: unchanged
remote performance claim: HOLD
```

R3 establishes that the detached page substrate survives the real HZ8 medium
entry and improves every paired control row. Keep it as an explicit candidate
until Linux integration and application-like evidence confirm the same
direction. Do not merge its ownership protocol into the public default merely
from this Windows synthetic gate.

## Cross-Platform And Application Gate

Linux exposes only explicit opt-in targets:

```text
make -C hakozuna-hz8 preload-page8k-r3
make -C hakozuna-hz8 smoke-page8k-r3
make -C hakozuna-hz8 safety-stress-page8k-r3
make -C hakozuna-hz8 bench-release-page8k-r3
```

Windows Redis-like uses the focused builder below so the gate does not rebuild
unrelated HZ6 research variants:

```text
pwsh win/build_win_hz8_redis_r3_gate.ps1
pwsh win/run_win_redis_workload_paper.ps1 -IncludeHz8Research
```

The Redis-like row is a no-regression check because its default size range does
not target exact 8KiB. Accept only when R3 remains within -5% throughput and
+5% peak RSS of HZ8 v2. It is not evidence for an 8KiB speed claim.

Windows alternating AB/BA repeat-5 passed this gate. The sum of the five
pattern throughputs had medians of 1590.27M for HZ8 v2 and 1589.62M for R3
(-0.04%). Median peak RSS was 16.45MiB and 16.56MiB (+0.64%). Treat this as
neutral application-like evidence, not as a performance win.

The medium-range Redis-like control (`4096..8192`) also passed repeat-5. Its
aggregate median changed from 796.76M to 806.61M (+1.24%), while median peak
RSS changed from 68.40MiB to 68.50MiB (+0.15%). This closes the Windows
application-like no-regression gate.

WSL Ubuntu GCC `-Werror` builds passed for preload, smoke, safety stress, and
release bench. The smoke and safety binaries also ran successfully. Fixed-8K
local AB/BA repeat-5 medians were 558.78M for HZ8 v2 and 557.60M for R3
(-0.21%). Median peak RSS was 3.45MiB and 2.11MiB respectively. The Windows
speed gain therefore does not reproduce on Linux; Linux classifies R3 as a
correctness-neutral opt-in rather than a performance lane.

Native Ubuntu x86_64 then ran the full alternating AB/BA repeat-5 gate. R3
changed fixed8K by `+12.36%`, balanced by `-6.62%`, wide_ws by `-0.23%`, and
larger_sizes by `-13.20%`. The Redis-like aggregate changed by `-0.37%`, while
its median peak RSS increased by `+5.48%`. GCC and Clang smoke/safety passed,
but the performance and RSS bounds did not. See
`docs/benchmarks/linux/HZ8_PAGE8K_R3_NATIVE_UBUNTU_20260712.md`.

Final lane posture:

```text
Windows:
  R3 selected opt-in performance candidate: GO

Linux:
  R3 correctness / opt-in research control: GO
  R3 performance promotion: NO-GO

Cross-platform public default:
  HOLD; keep HZ8 v2 unchanged
```

This is an OS-backend difference under a shared route/ownership/lifecycle
contract, not evidence that both platforms should use the same behavior lane.
