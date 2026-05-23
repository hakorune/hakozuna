# HZ5 OwnerLifetime-O1 Design

OwnerLifetime-O1 is the minimum Linux owner-lifetime hardening step before
adding more owner-aware front-ends such as MidFront-M1.

## Decision

SmallFront-S1 already moved remote ownership away from raw TLS pointers:

```text
page.owner = Hz5OwnerToken
remote free -> sender batch keyed by owner/class
batch flush -> global owner-slot x class inbox
owner alloc miss -> drain current owner slot/class inbox
```

This removes direct references to another thread's TLS object from page
metadata. The remaining Linux gap is owner lifetime: `hz5_owner` currently does
not mark owners dead on thread exit, and owner slots are not generation-reused
safely.

O1 should solve the minimum safety problem:

```text
remote free must not publish to a dead/stale owner inbox
dead-owner pages must not crash or touch freed TLS
owner generation must be checked before enqueue/drain decisions
```

O1 does not need to implement perfect reclamation.

## Scope

Covered:

```text
Linux pthread key destructor or equivalent TLS destructor
owner state: EMPTY / ALIVE / DYING / DEAD
generation-aware owner tokens
remote publish liveness/generation check
dead owner path to orphan/global queue
no raw TLS pointer ownership
```

Not covered in O1:

```text
full orphan page reclamation
owner slot aggressive reuse
epoch-based reclamation
RSS trimming
moving active allocations on owner death
```

## Owner State

```c
typedef enum Hz5OwnerState {
    HZ5_OWNER_EMPTY = 0,
    HZ5_OWNER_ALIVE = 1,
    HZ5_OWNER_DYING = 2,
    HZ5_OWNER_DEAD = 3,
} Hz5OwnerState;
```

The public token shape remains:

```c
typedef struct Hz5OwnerToken {
    uint16_t slot;
    uint16_t generation;
} Hz5OwnerToken;
```

Required checks:

```text
owner.slot != 0
owner.slot < owner_table_cap
owner generation matches owner table generation
owner state is ALIVE for normal remote publish
```

## Remote Publish Rule

```text
if owner is alive and generation matches:
    publish to owner-slot/class inbox
else:
    publish to orphan/global queue
```

The orphan/global queue can be simple in O1. Its purpose is safety, not optimal
reuse.

## Thread Exit

On owner thread exit:

```text
mark owner state DYING
flush sender-side remote batch for the exiting thread
move or detach owner inbox entries to orphan/global queue
mark owner state DEAD
do not free active user allocations
```

If complete inbox transfer is too large for O1, the minimum acceptable behavior
is:

```text
state becomes DEAD
new remote publishes reject the dead owner
existing inbox lists are not touched through a raw TLS pointer
```

## SmallFront Interaction

SmallFront after commit `32306cc` already has the right shape for O1:

```text
page owner:
  Hz5OwnerToken

remote inbox:
  global owner-slot x small-class table

remote batch key:
  owner token + small class
```

O1 should add liveness/generation checks to remote publish and drain. It should
not move SmallFront back to TLS-pointer ownership.

## MidFront Interaction

MidFront-M1 should reuse the same owner lifetime model:

```text
span owner:
  Hz5OwnerToken

remote inbox:
  global owner-slot x mid-class table

remote state:
  ACTIVE -> REMOTE_PENDING
```

Do not build MidFront on assumptions that contradict O1.

## Acceptance

Smoke:

```text
/bin/true under full preload
thread creates owner, allocates, exits
remote free after owner exit does not crash
remote publish to dead owner goes orphan/global
double-free remains fail-closed
foreign pointer remains not-owned/invalid
```

Benchmark guard:

```text
SmallFront <=2KiB r0/r90 should not materially regress
malloc_real remains 0 in benchmark body
track_insert_fail remains 0
```

## Stop Rules

Stop and redesign if:

```text
owner slot can be reused with the same generation while old pages exist
remote free can publish to a dead owner's inbox as if alive
thread exit can dereference a dead thread's TLS object
SmallFront invalid/double-free falls through to real libc
```

