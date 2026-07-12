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

## L1 Kind-Only Behavior Contract

Native Ubuntu larger_sizes reproduced the complete split:

```text
lookup=309536
medium_hit=309487
page8k_hit=49
kind_match=309536
kind_mismatch=0
register_conflict=0
```

This clears L0 for an opt-in `UnifiedMediumDomainKind-L1` behavior box. L1
uses the kind only and never dereferences the tagged record.

```text
PAGE8K:
  run Page8K authority
  success -> return
  owned-invalid -> fail fast
  miss -> continue through the remaining old chain

MEDIUM_RUN:
  run medium authority
  success -> return
  owned-invalid -> fail fast
  miss -> check Page8K authority, then continue through the old chain

NONE / unavailable leaf / publish failure:
  run the complete old Page8K-first chain
```

A kind hit is a dispatch hint, not ownership proof. Backend MISS is fail-open
to the old chain; backend owned-invalid is fail-closed. This preserves stale,
interior, duplicate-free, and Page8K-first INVALID semantics even if a shadow
entry is absent or stale.

The speed lane must not compile shadow counters or add owner, generation, or
diagnostic atomics. L0 remains the diagnostic oracle and TargetDispatch remains
the immediate behavior rollback.

### L1 Gate

```text
behavior acceptance against TargetDispatch:
  larger_sizes improvement >= 5%
  fixed8K regression <= 3%
  balanced / wide_ws regression <= 3%
  allocation failure = 0
  median RSS regression <= max(3%, 1MiB)

safety:
  L0 kind mismatch / conflict = 0
  owned-invalid never reaches another backend or system free
  NONE and backend MISS reproduce the old chain
  owner exit / remote / generation / duplicate / invalid gates pass

cross-platform default:
  separate HZ8-v2 gates remain unchanged
```

Record handoff is explicitly out of scope. `H8MediumRun` is freed after
unregister, so passing the tagged payload into medium validation remains HOLD
until a type-stable descriptor and reader-lifetime proof exist.

## L1 Kind-Only Result

The behavior box passed GCC and Clang `-Werror` preload, smoke, and safety
builds. Owner-exit, handoff, remote, duplicate, and invalid summaries matched
TargetDispatch. No diagnostic counters are compiled into the speed lane.

Native Ubuntu fresh-process AB/BA R10 measured TargetDispatch against
UnifiedMediumDomainKind-L1:

| Row | TargetDispatch | Unified kind | Delta |
|---|---:|---:|---:|
| fixed8K | 241.212M | 200.110M | -17.04% |
| balanced | 254.110M | 250.915M | -1.26% |
| wide_ws | 269.006M | 258.344M | -3.96% |
| larger_sizes | 113.683M | 116.948M | +2.87% |

Median RSS was unchanged on all four rows. The kind lookup helps the intended
larger row, but does not reach the required `+5%`. PAGE8K still performs its
registry authority lookup after the unified kind lookup, causing a large
fixed8K regression. Wide also exceeds its `-3%` guard.

```text
UnifiedMediumDomain-L0 shadow: GO
UnifiedMediumDomainKind-L1 behavior: NO-GO performance
record handoff: HOLD
public default: HZ8 v2 unchanged
```

Keep the L1 target only as an explicit reproduction lane. Do not stack it into
TargetDispatch or normal HZ8 builds. Any follow-up that consumes the tagged
record must be a separate box: Page8K metadata is type-stable, but generic
medium metadata is not and still requires a lifetime proof.
