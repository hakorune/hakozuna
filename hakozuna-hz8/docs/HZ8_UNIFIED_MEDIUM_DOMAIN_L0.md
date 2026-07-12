# HZ8 Unified Medium Domain L0

## Purpose

Native Ubuntu TargetDispatch evidence isolates a repeated free-classification
tax. The `larger_sizes` diagnostic observed only 49 Page8K-owned frees but
309,487 Page8K classifier misses before the generic medium directory lookup.
The current path therefore pays two independent classifiers for almost every
non-Page8K medium free.

```text
current:
  Page8K registry lookup
  -> MISS
  -> generic medium directory lookup

target shape:
  unified 64KiB-domain lookup
  -> PAGE8K or MEDIUM or NONE
  -> selected backend remains the route authority
```

L0 is diagnostic-only. It does not change free dispatch, allocation policy,
ownership, generation, slot-state CAS, remote publication, or the public HZ8
v2 default.

## Contract

The unified directory selects a backend; it never declares a pointer VALID.

```text
PAGE8K:
  Page8K validates exact slot, generation, state, owner, and pending protocol.

MEDIUM:
  generic medium validates run membership, exact slot, state, and ownership.

NONE:
  the current large/system path remains authoritative.
```

An owned-looking INVALID pointer must never become NONE or fall through to a
different backend. Directory kind and backend route state are separate facts.

## L0 Structure

Use a sparse two-level directory keyed by `addr >> 16`, matching the 64KiB
quantum shared by generic medium runs and Page8K runs. Each leaf entry is one
atomic tagged pointer:

```text
tag 0: NONE
tag 1: MEDIUM_RUN
tag 2: PAGE8K
payload: type-stable backend metadata pointer
```

Generic medium registration and Page8K page creation dual-publish into the
shadow after their current authority is initialized. Generic medium removal
clears only its matching shadow entry. Existing registries remain unchanged.

Free lookup records the shadow kind once, then compares it with the backend
selected by the existing Page8K-first path. L0 does not dereference the shadow
payload on the free path; this keeps metadata lifetime out of the first proof.

## Diagnostics

All counters require `H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0` and are excluded from
speed lanes.

```text
lookup
medium_hit
page8k_hit
miss
kind_match
kind_mismatch
register_conflict
medium_register / medium_unregister
page8k_register
```

## Acceptance

```text
kind_mismatch = 0
register_conflict = 0
Page8K exact/invalid safety remains unchanged
medium valid/invalid safety remains unchanged
GCC and Clang -Werror builds pass
Windows and Linux API/safety smoke pass
larger_sizes shows that medium_hit replaces the prior Page8K MISS population
```

L0 throughput is not a promotion result because the shadow adds atomic lookup
and counters. If L0 passes, L1 may route free dispatch through the unified
directory in an opt-in speed sibling while retaining old classifiers as a
diagnostic oracle.

## No-Go

- replacing both production registries in L0;
- treating a kind hit as VALID;
- dereferencing metadata after its current authority lifetime ends;
- adding per-free owner or generation checks to the directory;
- weakening Page8K-first INVALID handling;
- promoting from shadow throughput.

## Initial Status

```text
design contract: fixed
L0 shadow: implemented; initial Windows/WSL evidence is zero-mismatch
L1 behavior: blocked on zero mismatch/conflict evidence
public default: HZ8 v2 unchanged
```

## Initial Evidence (2026-07-12)

Windows focused diagnostic runs:

```text
fixed8K:
  lookup=10,173
  page8k_hit=10,016
  medium_hit=157
  kind_mismatch=0
  register_conflict=0

16K..64K:
  lookup=10,173
  medium_hit=10,173
  page8k_hit=0
  kind_mismatch=0
  register_conflict=0
```

WSL GCC `-Werror`, fixed8K:

```text
lookup=20,000
page8k_hit=20,000
kind_match=20,000
kind_mismatch=0
register_conflict=0
```

The Windows HZ8 suite produced the new shadow executable and existing HZ8
variants before the two-minute orchestration timeout. The focused executable
ran successfully. GCC builds cleanly. Clang currently stops on two pre-existing
`unused-but-set-variable` diagnostics in medium diagnostic code; this is a
toolchain cleanup item, not L0 mismatch evidence.

The dedicated WSL smoke and safety-stress targets pass with owner exit 8,
handoff 68, remote free 8,192, and the existing duplicate/invalid fail-closed
checks intact.

L0 remains diagnostic-only. The remaining gate is a broader medium/larger
shadow run, including native Ubuntu reproduction of the 309,487 classifier
MISS population. L1 remains blocked until that evidence is collected.
