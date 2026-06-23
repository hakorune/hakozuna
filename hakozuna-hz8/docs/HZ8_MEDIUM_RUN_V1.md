# HZ8 MediumRun V1

Status:

```text
metadata scaffold present
no allocator behavior change yet
```

## Purpose

MediumRun-v1 exists to make these rows meaningful:

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

MediumRun does not replace the small path.  It handles requests above the small
boundary without falling directly to the platform allocator.

## Initial Range

```text
small:
  16..4096

medium-v1:
  4097..65536

large / direct:
  >65536
```

The upper bound is intentionally conservative.  It keeps the first medium
implementation focused on run ownership, routing, remote free, and RSS pressure
without mixing in huge allocation policy.

## Geometry

MediumRun uses run-sized payload mappings rather than fixed 64KiB small spans.

Initial shape:

```text
run payload:
  power-of-two page multiple

run slot size:
  rounded request size

slot count:
  run_payload_size / slot_size

metadata:
  outside payload
```

The final size table is not frozen in this document.  The current scaffold uses
coarse power-of-two medium classes to prove ownership and remote free before
SizePolicy-v1 refines the table.

Current scaffold:

```text
class sizes:
  8192
  16384
  32768
  65536

run payload:
  65536 bytes

slots/run:
  8
  4
  2
  1
```

Smoke coverage:

```text
range classification:
  4096 excluded
  4097 included
  65536 included
  65537 excluded

class mapping:
  4097..8192 -> 8192
  8193..16384 -> 16384
  16385..32768 -> 32768
  32769..65536 -> 65536

pointer identity helper:
  aligned slot pointer accepted
  interior pointer rejected
  out-of-run pointer rejected
```

## Pointer Identity

Boundary classification:

```text
arena miss:
  MISS

inside known medium run and aligned to slot:
  candidate VALID / INVALID based on slot state

inside known medium run but misaligned:
  INVALID

inside retired/decommitted medium run:
  INVALID
```

No platform free fallback for arena-owned invalid pointers.

Medium identity must include:

```text
run index
slot index
slot size
owner token
run epoch
state
```

## Ownership

MediumRun follows the HZ8 owner lifecycle rules:

```text
owner lifecycle lease:
  protects regular owner-owned run publication

run publish lease:
  protects orphan/adoption-style per-run handoff if needed

owner exit:
  close admission
  wait refs zero
  drain pending
  retire or handoff run
```

V1 should avoid regular adoption at first.

Initial ownership policy:

```text
source:
  current owner only

remote free:
  publish to owning run

owner exit:
  drain and retire empty medium runs
  handoff nonempty runs only if required by correctness
```

## Remote Free Authority

Medium remote free needs a per-slot claim authority.

Initial rule:

```text
slot_state:
  allocation validity authority

pending bitmap or equivalent:
  remote-free duplicate claim authority

qstate / pending queue:
  run collection scheduling authority
```

Do not remove the duplicate-claim authority in the first version.

## RSS Policy

Default RSS behavior should match small-v0 discipline:

```text
logical retirement:
  immediate

table unlink:
  immediate

physical decommit:
  batched when safe

post-run RSS:
  must recover
```

Resident retention is HOLD for v1 unless data proves it is needed.  A retained
pool can improve first-touch but directly conflicts with low-RSS claims.

## Implementation Order

```text
1. MediumRunBoundaryDesign-L1
   docs only

2. MediumRunRouteShadow-L1
   classify >4096 requests and record would-medium counts
   still direct-fallback

3. MediumRunMetadataScaffold-L1
   compile structs, tables, and verification helpers
   no runtime routing
   status: implemented
   smoke: range and coarse class table covered

4. MediumRunLocalOnly-L1
   same-owner malloc/free only
   final remote publication still pending
   status: initial routing scaffold implemented
   smoke: create / alloc / free / double-free reject / interior reject
   routing: h8_malloc / h8_free / h8_route connected for 4097..65536
   implementation: global mutex scaffold, not final owner-local fast path
   RSS: empty run payload uses MADV_DONTNEED and remains reusable

5. MediumRunRemote-L1
   remote free publish/collect
   duplicate claim gates

6. MediumRunLifecycle-L1
   owner exit, purge, post-RSS recovery
```

## Gates

Safety:

```text
invalid platform fallback = 0
duplicate claim = 0
remote publish lost = 0
quiescent pending bitmap = 0
timeout / abort = 0
```

Performance:

```text
small-v0 guard_local0:
  no material regression

small_interleaved_remote90:
  no material regression

main_*:
  measured only after medium is default-routable
```

Memory:

```text
post RSS recovers
peak RSS is explained by live rounded payload
decommit does not run while pending
```

## Holds

```text
resident medium cache
medium regular adoption
remote_head without pending claim authority
runtime size-policy profiles
full arbitrary class redesign
```
