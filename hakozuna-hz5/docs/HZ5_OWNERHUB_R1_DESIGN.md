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

## R2 Candidate

If R1 confirms cross-front backlog, R2 should add:

```text
on alloc miss:
  drain target front/class first
  then drain bounded work from other pending fronts
```

Initial budgets:

```text
small: 16 objects
mid:    8 spans
large:  4 spans
```

The first R2 implementation should still keep Small/Mid/Large inbox payloads
specialized.
