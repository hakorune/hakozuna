# HZ8 MediumRun V1 RC1

Status: **RC1 / protocol and geometry freeze candidate**.

This record covers the MediumRun-v1 default after small-v0 rc1.  It does not
change the frozen small-v0 behavior.

## Tested Default

```text
small:
  16..4096
  p2-v0 class map
  frozen small-v0 rc1 behavior

medium:
  4097..65536
  classes: 8K / 16K / 32K / 64K
  geometry: q64-run64k2
  64K class: 128KiB run / 2 slots
  directory: 64KiB quantum directory, 65536 entries
  arena: per-run mmap
```

## Post-RC1 Default Additions

The following behavior-preserving local code-shape boxes were promoted after
the RC1 protocol/geometry record:

```text
MediumActiveHitValidationCollapse-L1:
  default
  active-run allocation uses one checked helper instead of a usable-check plus
  alloc-helper state/free recheck

MediumFreeDirectIdentityShape-L1:
  default
  same-owner medium free reuses one owner-word load / owner-match decision on
  the direct path
```

Evidence:

```text
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
bench_results/hz8_active_hit_ab_20260624T174838Z/
bench_results/hz8_free_identity_ab_20260624T175453Z/
```

These boxes do not change the MediumRun ownership, remote, residency, geometry,
or small-v0 contracts.

## Medium Contracts

```text
ownership:
  owner-attached runs publish owner tokens
  TLS active run must match the current owner
  detached direct-lock fallback remains

remote:
  owner-attached foreign free publishes to owner medium queue
  pending bit is remote claim authority
  owner collector owns slot mutation
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY

residency:
  empty resident retention is budgeted
  TLS active empty run may remain LIVE without budget charge
  owner-thread collector may keep the current active empty run LIVE
  owner exit drains retained and active-live payload
```

## Gate Evidence

```text
gate:
  make medium-v1-gate

record:
  bench_results/20260624T152001Z_medium_v1_gate/README.md

confirmation:
  bench_results/20260624T153937Z_medium_v1_gate/README.md
```

Measured medians:

```text
medium_local0:
  102.54M ops/s
  steady 107.89M
  peak RSS 4.55MiB

medium_interleaved_remote50:
  33.13M ops/s
  steady 35.82M
  peak RSS 45.62MiB

medium_phase_remote90:
  262.5K ops/s
  steady 367.3K
  peak RSS 61.93MiB

main_interleaved_remote90:
  25.84M ops/s
  steady 27.03M
  peak RSS 42.08MiB
```

Confirmation medians:

```text
medium_local0:
  89.52M ops/s
  steady 95.36M

medium_interleaved_remote50:
  33.65M ops/s
  steady 36.02M

medium_phase_remote90:
  257.9K ops/s
  steady 364.3K

main_interleaved_remote90:
  25.80M ops/s
  steady 26.93M
```

## Strengths

```text
medium local:
  active run path is strong after active-empty-live retention

medium steady remote:
  owner queue protocol is safety-clean and materially faster than direct lock

RSS recovery:
  owner exit drains retained and active-live medium payload

main interleaved:
  main remote90 is now meaningful because >4KiB no longer falls to platform
```

## Known Holds

```text
chunk arena:
  improves main fault variance in evidence builds
  still regresses medium r50 in paired gates
  HOLD as default

upper48 size policy:
  can reduce rounded bytes
  failed frozen small interleaved quick gate
  HOLD as default

owner lease redesign:
  unsafe elision ceiling exists
  safe medium lease word A/B was flat
  HOLD

phase remote90:
  remains lifecycle / first-touch stress
  not the primary MediumRun throughput gate
```

## Next Lane

```text
current default:
  MediumRun-v1 RC1 protocol / geometry
  plus post-RC1 local code-shape additions above

same-run allocator matrix:
  recorded in docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

if improving main stability:
  reopen chunk arena only with a medium-r50 no-regression plan

if improving peak RSS:
  reopen SizePolicy-v1 as a separate lane
```
