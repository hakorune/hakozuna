# HZ5 Control Plane Design

This is the implementation-facing design note after the P45dr closeout. P43i
remains the selected balanced candidate-watch; P45r1 and P45dr are mechanism
evidence, not promotion lanes.

## Core Principle

HZ5 should keep the fast bridge and source-control responsibilities separate.

```text
P25 bridge:
  speed layer
  release_prepared -> release_common -> TLS relbuf -> global batch -> stash

P43 source:
  segment-slot source layer
  committed/free slots and new segment admission

P40 release:
  source-demotion intent under P43i/P45
  not an RSS-return operation

Admission governor:
  OPEN / DRAIN / CLOSED
  controls bridge -> source movement
```

The P43i/P45 contract forbids:

```text
slot decommit
PAGE_NOACCESS
runtime segment release
direct prepared release
descriptor relbuf
```

Those operations belong to a future RSS-return contract, not the current
lockless prepared-bridge contract.

## Closeout From P45

P45r1 showed that refined stage1 can avoid the old all-stage1 failure mode.
P45dr then showed that remaining stage1 retention is localized and bounded
enough to stop adding local knobs.

```text
P45r1:
  refined-gate mechanism evidence

P45dr:
  stage1-retention diagnostic evidence

Promotion:
  no

Next:
  HZ5 core control-plane design
```

The key interpretation is:

```text
rss-bounded plateau retention:
  DRAIN/CLOSED dominated
  not an OPEN demote miss

mixed-prelude final exact 64K:
  no old stage1 exposure in the repeat-3 diagnostic

guard/fallback rows:
  no refined stage1 leakage
```

## State Names To Make Explicit

The next source cleanup should make these state roles visible in names and
comments before adding new behavior.

```text
ACTIVE:
  user-visible allocation

BRIDGE_RETAINED:
  committed object retained by the P25 bridge speed layer

BRIDGE_COLD:
  P40 release intent staged bridge-side
  committed and reusable

SOURCE_HOT_FREE:
  recently reusable P43 source slot

SOURCE_WARM_FREE:
  normal P43 committed/free source slot

SOURCE_COLD_FREE:
  older P43 committed/free slot
  still committed under P43i/P45

COLD_DECOMMITTED:
  future RSS-return contract only

RETIRED:
  future tombstone/stale range guard
```

## Implementation Guidance

Do next:

```text
name bridge/source/control-plane boundaries
keep P45/P43p lanes as evidence
keep P43i as balanced candidate-watch
separate route-class pressure in diagnostics
```

Do not do next:

```text
more P43p pop/age sweeps
P45 acquire-limit sweep
actual drain
trim/decommit/runtime release
direct descriptor release
promotion of P45r1/P45dr
```

If a future behavior lane is added, it should be based on the explicit HZ5 core
state model rather than another local P43p/P45 knob.

