# HZ8 LargeDirect Default Probe L1

## Status

```text
box:
  HZ8LargeDirectDefaultProbe-L1

state:
  build-target probe
  not default behavior yet
```

## Why

`cross128_r90` mixes small, medium, and 65,537..131,072 byte allocations.
The current HZ8 default owns small/medium but sends the large side to system
allocation.

Focused observation shows the weakness is at this large/direct boundary.

```text
cross128_r90 short R3:
  HZ8 default:
    ~70k ops/s

  HZ8 default + H8_LARGE_DIRECT_OWNED_L1:
    ~2.811M ops/s

cross128_r0 short R3:
  HZ8 default:
    ~263k ops/s

  HZ8 default + H8_LARGE_DIRECT_OWNED_L1:
    ~6.797M ops/s
```

Read this as evidence for a default candidate, not as a promotion decision.

## Scope

```text
added targets:
  preload-largedirectdefault
  bench-release-largedirectdefault

flags:
  HZ8_DEFAULT_CFLAGS
  + H8_LARGE_DIRECT_OWNED_L1

unchanged:
  small-v0
  medium q64-v12-48k2
  KeepRefill
  lazy128
  owner queue / pending / qstate authority
```

No allocator source behavior changes are made by this box. It only provides
official comparison artifacts for the existing LargeDirectOwned lane.

## Hot-Path Closure

The first probe exposed a medium hot-path tax: enabling LargeDirect made
`h8_free_inner()` call the direct-large exact lookup before the medium lookup,
even for medium-only rows.

The closure keeps the default build unchanged and limits direct lookup to
LargeDirect builds:

```text
default HZ8:
  no direct-large range guard in free/route/realloc hot paths

LargeDirect build:
  inline min/max range guard
  call direct-large lookup only when the pointer is inside the known
  direct-large address envelope

direct-large implementation:
  publishes min/max range in H8Global
  locked hash remains final authority
```

This preserves the cross128 fast route while avoiding the earlier medium-local
regression.

## Observed Gate

Primary R10 after hot-path closure:

```text
record:
  bench_results/20260630T230450Z_hz8_largedirect_probe_gate/

cross128_r90:
  baseline median:  62.940k ops/s
  candidate median: 2.835M ops/s
  ratio:            45.048x
  peak RSS:         150.17 MiB -> 260.07 MiB
  post RSS:         107.04 MiB -> 190.61 MiB

cross128_r0:
  baseline median:  259.749k ops/s
  candidate median: 5.848M ops/s
  ratio:            22.512x
  peak RSS:         5.93 MiB -> 6.86 MiB
  post RSS:         5.40 MiB -> 5.99 MiB
```

Regression checks:

```text
full R5:
  bench_results/20260630T230137Z_hz8_largedirect_probe_gate/

small_interleaved_remote90:
  median ratio: 1.013

main_interleaved_r90:
  median ratio: 1.031

medium_interleaved_r50:
  median ratio: 0.977
  noisy in the full R5 batch

medium_interleaved_r50 focused R20 fresh-process:
  bench_results/20260630T230432Z_hz8_largedirect_probe_gate/
  median ratio: 0.997
  p25 ratio:    1.015

medium_local0 focused R10:
  bench_results/20260630T230122Z_hz8_largedirect_probe_gate/
  median ratio: 1.019
  p25 ratio:    1.024
```

Smoke:

```text
h8_smoke:
  pass

h8_smoke_largedirect:
  pass
```

## Gates

Primary:

```text
cross128_r90:
  material improvement

cross128_r0:
  material improvement
```

Regression:

```text
small_interleaved_remote90:
  regression <= 2%

main_interleaved_r90:
  regression <= 2%

medium_interleaved_r50:
  regression <= 2%

guard_local0 / main_local0 / medium_local0:
  regression <= 2%
```

Safety / RSS:

```text
owned INVALID remains fail-closed
owner exit drains direct-owned allocations
peak RSS remains acceptable for large/direct rows
post RSS returns near baseline after worker exit
```

## Decision

```text
GO:
  cross128 fixes hold under RUNS=10
  regression rows clean
  RSS and safety gates clean

HOLD:
  cross128 improves but small/main/medium regress
  or cross128 RSS tradeoff is not acceptable for the public default

NO-GO:
  cross128 improvement is unstable
  direct-owned lifecycle or RSS gates fail
```

Current read:

```text
throughput:
  GO

small/main/medium regressions:
  GO after focused medium R20

RSS:
  needs explicit product decision
  cross128_r90 gains about 45x throughput but raises peak/post RSS

default promotion:
  reasonable if cross128 is part of the public MT matrix contract
  otherwise keep as opt-in LargeDirect profile
```
