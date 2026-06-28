# HZ8 MediumRun V1

Status:

```text
protocol / geometry RC summary
4097..65536 routes through medium runs
v1.1 balanced default is frozen
v2 throughput work is design/evidence only
```

## Purpose

MediumRun-v1 keeps the medium range explicit so these rows stay meaningful:

```text
main_r0
main_r50
main_r90
cross128_r90
```

Small-v0 remains:

```text
size:
  16..4096

class map:
  p2-v0

state:
  frozen small-v0 / rc1 default
```

MediumRun does not replace the small path. It handles requests above the small
boundary without falling directly to the platform allocator.

## Range

```text
small:
  16..4096

medium-v1:
  4097..65536

large / direct:
  >65536
```

The upper bound keeps MediumRun focused on ownership, routing, remote free,
and RSS pressure rather than huge-allocation policy.

## Geometry

MediumRun uses run-sized payload mappings rather than fixed 64KiB small spans.

Current default:

```text
class map:
  8K / 16K / 24K / 32K / 48K / 64K

run geometry:
  q64-v12-48k2
  8K  class:  64KiB run / 8 slots
  16K class:  64KiB run / 4 slots
  24K class:  64KiB run / 2 slots
  32K class:  64KiB run / 2 slots
  48K class: 128KiB run / 2 slots
  64K class: 128KiB run / 2 slots

identity:
  64KiB quantum directory
  power-of-two slot decode for p2 classes
  24K local-free exact two-offset decode
  generic exact non-p2 decode for 48K and for remote/route/usable_size paths

legacy comparison:
  q64-run64k2 remains available through medium64k2 build targets
  directory capacity: 65536 quanta
  per-run mmap remains the default
```

## Pointer Identity

The medium contract keeps pointer identity fail-closed:

```text
VALID:
  pointer resolves to an owned run slot

INVALID:
  pointer looks medium-like but does not match the run slot state

MISS:
  pointer is not owned by HZ8 medium routing
```

Medium pointer identity is still driven by:

```text
exact slot decode
owner-attached run state
directory-based route lookup
```

## Ownership

The ownership contract remains:

```text
owner queue
pending bitmap
qstate
quantum-directory
```

Keep the current medium owner queue semantics frozen until a narrower
mechanics lane shows a clear win.

## Remote Free Authority

Remote free stays attached-owner and fail-closed.

Current medium remote findings:

```text
owner-attached remote correctness remains clean
owner collect is the largest debug bucket
notify / queue-push is close to one per remote free
current owner collect cadence is eager but still not free
```

The next protocol change, if any, should come from a dedicated audit doc, not
from re-opening the medium geometry contract.

## RSS Policy

The medium policy still prefers bounded resident memory over aggressive
retention.

Current stance:

```text
empty-run resident retention uses a fixed budget
upper48 / chunk-carving ideas are evidence only
do not promote a new medium geometry from the current evidence alone
```

## Implementation Order

Keep the medium lane in this order:

```text
1. Freeze the v1.1 balanced default
2. Use the current matrix / RC docs as evidence
3. Route new work through narrow mechanics lanes only
4. Revisit larger size or run-pool changes only when the evidence is explicit
```

## Current Next-Lane Decision

Do not promote 48K or chunk carving from the current evidence.

```text
reason:
  the changes can reduce rounded bytes
  they do not automatically reduce queue episodes or owner cadence cost
  they add more geometry without a single clearly dominant win
```

The current stable default stays frozen.

## Gates

Safety:

```text
invalid platform fallback = 0
duplicate claim = 0
remote publish lost = 0
timeout / abort = 0
```

Performance:

```text
small-v0 guard_local0:
  no material regression

small_interleaved_remote90:
  no material regression

main_*:
  measured only after the medium lane is default-routable
```

Memory:

```text
post RSS recovers
peak RSS is explained by live rounded payload
decommit does not run while pending
```

## Holds

```text
unbounded resident medium cache
medium regular adoption
remote_head without pending claim authority
runtime size-policy profiles
full arbitrary class redesign
```

## Read Next

```text
docs/HZ8_MEDIUM_RUN_V1_RC1.md
docs/HZ8_MEDIUM_RUN_V1_MATRIX.md
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
docs/HZ8_BENCH_GATE.md
docs/HZ8_V2_HZ9_DESIGN.md
```
