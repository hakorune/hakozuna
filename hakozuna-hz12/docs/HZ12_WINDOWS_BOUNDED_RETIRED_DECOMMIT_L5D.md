# HZ12 Windows Bounded Retired Decommit L5-D

## Purpose

L5-D decommits only retired-owner spans whose existing L2 reclaim gate is fully
open after L5-C. It uses a fixed 64-span (4 MiB) budget and the proven Windows
`MEM_DECOMMIT` helper.

## Admission

```text
exact slot + generation side-table match
L2 reclaim gate fully open
route already detached
fixed invocation budget not exhausted
```

L5-D neither inserts spans into the depot nor recommits them. It is a dedicated
quiescent behavior lane and is not called by normal allocation/free.

## Initial Result

The Windows wide_ws-like lane passed repeat-3 with a 64-span budget. Every run
decommitted 64/64 spans with zero failures and exactly 4.00 MiB released.
Working-set RSS fell by 3.99 MiB in each run: 72.48 to 68.49, 83.46 to 79.47,
and 82.30 to 78.31 MiB. No depot or recommit behavior ran.

## Acceptance

```text
decommit attempts <= fixed budget
every attempted span changes MEM_COMMIT -> MEM_RESERVE
decommitted bytes equal successful L5-C detached bytes
working-set RSS does not increase
all previous lifecycle/reclaim smokes remain green
```

## No-Go

```text
decommit a route-attached span
touch stale-generation spans
partial success hidden from counters
automatic depot/recommit policy in L5-D
```
