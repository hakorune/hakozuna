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

Focused repeat-3 raw comparison, `HZ5_PRELOAD_STATS` unset:

```text
main r50:
  inbox  11.38M median
  R2     11.11M median

main r90:
  inbox   9.01M median
  R2      7.87M median

mid_only r50:
  inbox  11.70M median
  R2     11.14M median

mid_only r90:
  inbox   8.84M median
  R2      6.91M median

large_only r50:
  inbox   9.09M median
  R2      9.56M median

large_only r90:
  inbox   7.58M median
  R2      7.13M median

cross128 r50:
  inbox   8.25M median
  R2     10.38M median

cross128 r90:
  inbox   7.83M median
  R2      6.96M median
```

Decision:

```text
OwnerHub-R1:
  useful observation

OwnerHub-R2 bounded coordinated drain:
  diagnostic only for now
  not a broad remote-heavy win
```

Interpretation:

```text
cross-front pending exists, but naive opportunistic drain adds enough work on
miss paths to hurt r90. The next design should reduce remote handoff cost or
make drain scheduling more selective, rather than draining every other front
on each miss.
```
