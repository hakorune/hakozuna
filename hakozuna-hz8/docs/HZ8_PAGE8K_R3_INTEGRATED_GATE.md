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

## Native Target Dispatch Box

The native Ubuntu gate showed that the R3 candidate probes the page8K backend
for non-target medium allocations and creates a page8K owner on non-page free
misses. `TargetDispatch-L1` isolates those corrections behind
`H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1`.

```text
lane:
  hz8-r3-page8k-target-dispatch

control:
  hz8-r3-page8k-integrated

malloc boundary:
  call the page8K backend only when size == 8192

free boundary:
  preserve page8K-first authority ordering
  classify with the existing TLS owner, including NULL
  do not create a page8K owner on classifier miss

unchanged:
  page8K ownership, generation, slot-state CAS, remote publication
  malloc/free diagnostics and public HZ8 v2 default
```

An authority that reports owned-invalid must fail fast; it must never fall
through to the next allocator. Passing a NULL current owner remains valid: a
real page8K pointer follows the existing remote publication path, while a miss
returns without allocating owner state. The old R3 target remains available as
the immediate A/B rollback. Measure this box against old R3 first so
route-shape attribution is not mixed with a default-promotion claim.

### Windows TargetDispatch Revalidation

The old Windows result in the next section belongs to the original integrated
R3 lane. It must not be reused as a TargetDispatch result. The first Windows
TargetDispatch matrix showed a positive balanced signal but did not reproduce
the native-Ubuntu fixed-8K gain consistently. Revalidate the exact row before
any behavior or default decision.

```text
control:
  hz8-v2

rollback sibling:
  hz8-r3-page8k-integrated

candidate:
  hz8-r3-page8k-target-dispatch

authoritative Windows row:
  T=8, iters=500000, ws=400, size=8192, remote=0
  fresh-process alternating AB/BA repeat-10

diagnostic sibling only:
  H8_PAGE8K_REMOTE_DIAGNOSTIC=1
  dispatch alloc attempt / served
  dispatch free attempt / owner-present / owned / success / miss
  TLS owner creation
```

The diagnostic sibling may contain relaxed atomic counters. The speed sibling
must not. A fixed-8K result with a missing run, grouped execution, a different
thread/working-set shape, or diagnostic counters is not promotion evidence.
Remote90 remains lifecycle evidence because effective remote percentage can
fall when the benchmark ring uses local fallback.

Native Linux exposes matching diagnostic-only targets:

```text
make -C hakozuna-hz8 smoke-page8k-api-r3-target-dispatch-diag
make -C hakozuna-hz8 bench-release-page8k-r3-target-dispatch-diag
```

These targets are attribution-only and must not be used for throughput.

#### Windows Revalidation Result

The authoritative Windows fixed-8K row and the three control rows ran as
fresh processes in alternating AB/BA order. Values are repeat-10 medians.

| Row | HZ8 v2 | TargetDispatch | Delta |
|---|---:|---:|---:|
| fixed8K | 106.186M | 180.292M | +69.8% |
| balanced | 48.912M | 48.741M | -0.35% |
| wide_ws | 47.763M | 48.900M | +2.38% |
| larger_sizes | 21.851M | 21.396M | -2.08% |

All runs completed with zero allocation failure. The diagnostic sibling
reported `2,000,789` exact alloc attempts and services, `2,000,789` owned and
successful frees, and eight owner creations. The additional `31,464` free
misses are non-page cleanup/classification traffic and did not create owners.
This closes dispatch wiring as a cause of the earlier weak result.

Repeat-3 median peak working set remained near the HZ8 v2 control:

| Row | HZ8 v2 | TargetDispatch |
|---|---:|---:|
| fixed8K | 21.16MiB | 20.59MiB |
| balanced | 789.78MiB | 790.78MiB |
| wide_ws | 428.11MiB | 428.30MiB |
| larger_sizes | 297.85MiB | 297.35MiB |

The maximum increase is `1.00MiB` (`0.13%`) and is treated as measurement
parity, not an RSS improvement claim. TargetDispatch is a Windows opt-in GO;
the public HZ8 v2 default remains unchanged because the native-Ubuntu control
rows do not yet clear the cross-platform gate.

Native Ubuntu revalidation after this Windows commit did not reproduce the old
`larger_sizes -10.71%` magnitude exactly, but the regression remained outside
gate: `-6.37%` at the original iteration count and `-7.85%` with 10x longer
runs. Diagnostic attribution reported 49 page-owned frees and 309,487 page
classifier misses in the focused larger row. The corresponding long balanced
and wide rows were `-0.22%` and `-2.13%`, so general small-path code layout is
not the primary larger-size explanation. Native fixed8K measured `+27.08%` and
`+24.04%` in these two batches; cross-platform default promotion remains HOLD.

### Address Filter Follow-up

A monotonic min/max address filter was tested after target dispatch and removed.
It did not improve the generic larger-size row and added two atomic loads to
every page8K free. In paired R5, fixed8K fell from `276.11M` to `238.80M` at
the median. This shape is `NO-GO`; do not put an always-on address filter in
front of page authority.

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

The focused builder and the normal research-only Windows suites expose
`hz8-r3-page8k-target-dispatch` alongside v2 and integrated R3. It remains
excluded unless `-IncludeHz8Research` is specified.

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
