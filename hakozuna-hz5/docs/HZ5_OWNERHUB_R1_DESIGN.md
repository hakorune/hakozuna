# HZ5 OwnerHub-R1 Design

OwnerHub-R1 is the first cross-front remote handoff coordination experiment for
Linux full-preload HZ5.

## Decision

Do not replace SmallFront/MidFront/LargeFront inbox payloads with a generic
per-object remote entry yet.

Use this shape instead:

```text
shared:
  owner-slot pending mask
  cross-front pending observation
  later coordinated drain policy

not shared:
  SmallFront node lists
  MidFront span lists
  LargeFront span lists
  front-specific descriptor validation
  front-specific state transitions
```

This preserves HZ5 fail-closed descriptor ownership while allowing the owner to
see remote pressure across Small/Mid/Large.

## R1 Scope

R1 is a dry-run/observation lane:

```text
--linux-ownerhub-r1
```

It records:

```text
remote publish:
  set owner pending bit for front/class

front drain:
  clear owner pending bit for front/class

alloc miss:
  read owner pending mask
  count whether target front/class is pending
  count whether other front/classes are pending
```

R1 does not change allocation behavior. It does not coordinate drain yet.

## Bit Layout

```text
small classes:
  bits 0..13

mid classes:
  bits 16..20

large classes:
  bits 32..35
```

## Observation Hygiene

R1 uses atomic diagnostic counters. Do not compare R1 ops/s with raw speed
lanes.

Use:

```text
HZ5_OWNERHUB_STATS=1
HZ5_PRELOAD_STATS=1
```

for attribution/diagnostic runs only.

Raw performance runs must keep OwnerHub-R1 disabled.

## Keep / No-Go

Keep going to R2 if `cross128 r90` shows frequent alloc misses with other-front
pending bits. That means coordinated drain has a real target.

No-go if other-front pending is rare. In that case the bottleneck is not
cross-front drain scheduling; it is per-front remote handoff cost or benchmark
handoff overhead.

## R2 Candidate Result

R2 was implemented as a bounded coordinated-drain candidate:

```text
on alloc miss:
  drain target front/class first
  then drain bounded work from other pending fronts
```

Budgets:

```text
small: 16 objects
mid:    8 spans
large:  4 spans
```

Small/Mid/Large inbox payloads remained specialized.

Initial per-class budget R2 was not a broad win. It was then fixed to use a
total front budget, not per-class budget:

```text
old:
  small 16 per class = up to 224 objects
  mid   8 per class  = up to 40 spans
  large 4 per class  = up to 16 spans

fixed:
  small 16 total
  mid   8 total
  large 4 total
```

Focused repeat-3 raw comparison after total-budget fix,
`HZ5_PRELOAD_STATS` unset:

```text
main r50:
  inbox   9.89M median
  R2     10.89M median

main r90:
  inbox   6.99M median
  R2      8.69M median

mid_only r50:
  inbox  10.83M median
  R2     10.35M median

mid_only r90:
  inbox  10.30M median
  R2      6.52M median

large_only r50:
  inbox  10.44M median
  R2     10.02M median

large_only r90:
  inbox   8.42M median
  R2      6.14M median

cross128 r50:
  inbox   8.98M median
  R2      9.60M median

cross128 r90:
  inbox   7.01M median
  R2      6.32M median
```

Decision:

```text
OwnerHub-R1:
  useful observation

OwnerHub-R2 bounded coordinated drain:
  mixed-workload candidate only
  not a default remote-heavy replacement for per-front inbox

OwnerHub-R3 front-dirty drain:
  next candidate
  keep the same coordinated drain shape
  replace class-granular pending bits with coarse per-front dirty bits
```

Interpretation:

```text
cross-front pending exists, and total-budget R2 can help main/cross r50 and
main r90. However, the ownerhub pending-mask atomics add fixed remote publish
and drain overhead that hurts single-front remote-heavy rows. The next design
must reduce ownerhub bookkeeping cost or enable it only for mixed/cross-size
profiles.

R3 tests whether the class-granular pending mask is too expensive or too
fragile. It intentionally makes pending state approximate: remote publish marks
only the front as dirty, ordinary class drains do not clear it, and cross-front
drain clears the front dirty bit after a bounded opportunistic drain. This keeps
Small/Mid/Large ownership validation specialized and does not introduce a
generic RemoteEntry payload.

Short raw repeat-3 result:

```text
main r90:
  inbox 7.54M
  R2    9.85M
  R3    6.97M

mid_only r90:
  inbox 7.73M
  R2    6.42M
  R3    7.71M

cross128 r90:
  inbox 6.23M
  R2    5.99M
  R3    6.18M
```

Decision: R3 is useful as a diagnostic, but it is not a broad default. It
recovers some single-front damage from R2 but fails to beat the owner-inbox
baseline on the important mixed r90 row.
```
