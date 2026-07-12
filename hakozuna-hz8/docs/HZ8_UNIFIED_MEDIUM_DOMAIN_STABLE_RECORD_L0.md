# HZ8 Unified Medium Domain StableRecord L0

## Decision

`UnifiedMediumDomain-L0` is GO and the kind-only L1 behavior experiment is a
performance NO-GO. Eliminating both backend classifiers requires record
handoff, but passing the current raw `H8MediumRun*` through the directory would
permit a reader/unregister use-after-free race.

This box proves metadata lifetime before any behavior handoff:

```text
UnifiedMediumDomainStableRecord-L0:
  diagnostic-only
  fixed-cap record pool
  records are never reused
  LIVE -> CLOSING -> TOMBSTONE
  old Page8K/medium authorities remain unchanged
  public HZ8 v2 remains unchanged
```

## Immediate Safety Corrections

The 64KiB directory key now rejects addresses that do not fit its 32-bit
quantum index. High addresses fall back to the complete old chain instead of
truncating and aliasing another quantum. Both generic medium and Page8K
metadata have compile-time alignment checks for the two tag bits.

## Stable Record Contract

Each generic medium run receives one record from a 65,536-entry static pool.
The record copies immutable route geometry and holds an advisory implementation
pointer while LIVE:

```text
base / run_size
slot_size / slot_count / slot_shift / class_id
generation
state
implementation pointer
```

Registration publishes only after the old medium authority is initialized.
Unregister changes LIVE to CLOSING before matching-CAS directory removal, then
clears the implementation pointer and publishes TOMBSTONE. Record memory is
not freed or reused. Pool exhaustion skips unified publication and leaves the
old chain authoritative.

L0 readers may compare immutable geometry with the old authority only while
the record is LIVE. CLOSING and TOMBSTONE records are counted but not
dereferenced through the implementation pointer.

This is not yet the final record-handoff design. `slot_state`, `pending_bits`,
and the run lock remain owned by `H8MediumRun`; behavior must not consume them
until their lifetime is also stable or protected by a proven quiescence rule.

## Diagnostic Counters

All counters are excluded from speed/default builds:

```text
high_address_fallback
stable_record_alloc
stable_record_pool_fallback
stable_live_lookup
stable_closing_lookup
stable_tombstone_lookup
stable_backend_mismatch
stable_geometry_mismatch
stable_unregister_missing
stable_pool_current_bytes / stable_pool_peak_bytes
```

## Initial Validation

```text
WSL GCC -Werror build: PASS
dedicated smoke: PASS
dedicated safety stress: PASS
owner exit: 8
handoff: 68
remote free: 8,192
duplicate/invalid fail-closed checks: PASS
```

The dedicated smoke contains a hard counter gate: at least one stable record
and LIVE lookup must be observed, while pool fallback, backend mismatch,
geometry mismatch, and unregister-missing must remain zero.

Windows focused `16K..64K`, two-thread diagnostic:

```text
lookup=50,960
medium_hit=50,959
stable_record_alloc=323
stable_live_lookup=50,959
stable_pool_current_bytes=15,504
stable_record_pool_fallback=0
stable_backend_mismatch=0
stable_geometry_mismatch=0
stable_unregister_missing=0
high_address_fallback=0
```

This confirms the LIVE record path and bounded accounting. It does not yet
prove concurrent reader/unregister behavior.

The dedicated smoke now performs 10,000 generations on the same synthetic
64KiB address. Every generation receives a distinct record, transitions
through CLOSING to TOMBSTONE, removes its matching directory entry, and leaves
the held old record safely readable as TOMBSTONE. Record reuse and turnover
failure remain zero. A synthetic address above the 48-bit directory range also
confirms fallback to NONE/old-chain handling.

The smoke also runs a second 10,000-generation test with a reader thread
continuously loading and observing records while a writer thread repeatedly
publishes and removes the same address. Both WSL/pthread and Windows threads
pass with the hard-zero mismatch/reuse checks intact. Stable comparison
rechecks record state after loading the implementation pointer, so a concurrent
`LIVE -> CLOSING` transition is classified as a transition rather than a
backend mismatch.

The synthetic run metadata remains alive until the reader joins. This proves
the stable record and directory transition, but deliberately does not claim
that the current `H8MediumRun`, slot-state array, pending bitmap, or lock can be
freed while a behavior reader uses them.

## StableValidationCore L0A

The record now performs exact-pointer geometry validation without
dereferencing `H8MediumRun`. It supports both power-of-two shift geometry and
the v12 non-power-of-two modulo geometry. The 10,000-generation turnover test
alternates 16KiB and 24KiB slots and verifies, per generation:

```text
base: exact
base + slot_size: exact
base + 1: invalid
held record after TOMBSTONE: invalid
old authority comparison mismatch: 0
```

WSL and Windows smoke pass. This establishes a stable immutable route core,
not a complete allocation/free backend. Mutable slot state, pending protocol,
and synchronization lifetime remain outside the record.

## Next Gate

Before behavior work:

1. run Windows focused medium/larger diagnostics;
2. run native Ubuntu broad shadow and turnover stress;
3. design stable mutable slot-state/pending authority or a cold-path
   quiescence proof for their lifetime and lock;
4. verify pool bytes plateau or cleanly reach the fixed cap fallback;
5. run ASan/UBSan where supported.

Record handoff remains HOLD. Kind-only L1 remains a NO-GO reproduction lane.

## StableSlotState L0 Design

The next diagnostic box embeds a bounded slot-state mirror in each stable
record. The current `H8MediumRun::slot_state` remains authoritative. All
release stores are routed through one helper, and successful CAS transitions
notify the same mirror hook only after authority commit.

```text
authority mutation
  -> stable mirror sync
  -> immediate equality check

register
  -> initialize mirror from authoritative slot_state[]

unregister
  -> final mirror sync while run metadata is alive
  -> CLOSING / directory remove / TOMBSTONE
```

The mirror is diagnostic-only and adds no operations to public/speed builds.
It does not become allocation/free authority in L0. Acceptance requires all
supported classes to fit the embedded bound, nonzero sync coverage, zero
state mismatch, and a readable final TOMBSTONE snapshot after run teardown.

Pending bitmap, qstate, and lock remain separate later boxes. They must not be
mixed into the slot-state proof.

## StableSlotState L0 Result

The generic medium release-store sites now use a common helper. The helper is
identical to the old atomic store when the diagnostic macro is disabled.
StableRecord builds mirror the committed state after each registered-run
mutation. Registration snapshots stores that occurred before publication;
unregister enters CLOSING first and performs the authoritative final compare.

WSL and Windows smoke pass. The WSL gate observed:

```text
registered slot sync: 8
final slot sync: 70,000
final state mismatch: 0
slot sync after CLOSING: 0
slot bound fallback: 0
pre-publication note without record: 12
```

The 12 pre-publication notifications are initial stores from runs that have no
published stable record yet. They are intentionally not mismatches; the
registration snapshot captures their resulting authority state. No mutation
was observed after CLOSING, so the existing unregister boundary is quiescent
for slot-state in this smoke.

StableSlotState-L0 is GO as diagnostic evidence. It is not behavior authority.
Remote pending bitmap and qstate remain authoritative in `H8MediumRun` and are
the next independent lifetime box.

## StablePendingState L0 Design

The stable record mirrors the generic medium pending bitmap, pending-word mask,
and qstate. Existing atomics remain authoritative. Successful pending and
qstate mutations call one diagnostic snapshot helper after commit; public and
speed builds compile the helper to no operations.

Because bitmap and qstate are separate atomics, transient snapshots are not
used for acceptance. Unregister publishes CLOSING first, then compares a final
authority snapshot with the mirror before TOMBSTONE. A final mismatch means
the mirror needs a sequence/version protocol; it must not be hidden by copying
the final values first.

Acceptance requires nonzero remote mutation coverage, zero final mismatch,
zero update after CLOSING, and unchanged remote/owner-exit safety. Lock
lifetime and behavior handoff remain out of scope.

## StablePendingState L0 Result

A dedicated live-owner test keeps the allocating thread alive while another
thread frees its 16KiB object. This forces generic medium remote publication,
qstate notification, owner-side collection, and final owner-exit handling.

WSL diagnostic summary:

```text
stable records: 20,004
stable live lookups: 13,057
stable tombstone observations: 10,658
slot sync: 12
slot final sync: 70,000
pending/qstate sync: 5
pending/qstate final sync: 20,000
pending/qstate final mismatch: 0
pending update after CLOSING: 0
stable pool bytes: 2,720,544
```

The same hard gate passes on Windows. StablePendingState-L0 is GO as
diagnostic evidence. Public/default behavior remains unchanged. The stable
record pool cost is diagnostic-only and must not be used as an RSS result.

The remaining lifetime boundary is synchronization itself. A behavior reader
cannot use the current embedded `H8MediumRun::lock` after run teardown. The
next design must either place the lock in stable control storage or prove a
cold-path quiescence protocol without adding per-free reader refcounts.

## StableLock L1 Diagnostic Design

The fixed-cap stable record owns a mutex initialized once and never destroyed
or reused. In the stable research lane only, generic medium run-lock helpers
use this mutex after record publication; pre-publication or pool-fallback runs
retain the original embedded lock. Public/default builds remain unchanged.

Unregister acquires the stable mutex before publishing CLOSING, taking final
slot/pending snapshots, removing the directory entry, and publishing
TOMBSTONE. This makes lock lifetime independent of `H8MediumRun` metadata and
provides cold-path quiescence without per-free reader refcounts.

Acceptance requires nonzero stable-lock coverage, zero unlock mismatch, zero
registered-run fallback, zero writer after CLOSING, and all existing remote,
owner-exit, duplicate, and invalid gates. This remains diagnostic behavior;
record handoff is not enabled by the lock experiment alone.

## StableLock L1 Diagnostic Result

The stable research lane now routes both ordinary medium operations and the
remote collector through one backend lock helper. Published runs use the
record-owned mutex; pre-publication/pool-fallback runs use the legacy embedded
mutex. Unregister holds the stable mutex across CLOSING, final slot/pending
snapshots, directory removal, and TOMBSTONE publication.

WSL and Windows smoke pass without deadlock. Stable-lock acquisition and
unregister-lock coverage are nonzero, unlock mismatch is zero, and the
slot/pending CLOSING guards remain zero. The live-owner remote handoff still
observes five pending transitions and completes safely.

Diagnostic pool usage after the 20,000-generation stress is approximately
3.52MiB on WSL and 2.88MiB on Windows; the difference reflects platform mutex
size. This is research metadata, not a public RSS result.

StableLock-L1 is GO as diagnostic lifetime evidence. Record handoff remains a
separate performance/safety experiment because the directory reader still
needs to enter through the stable record rather than a raw run pointer.

## Page8KRecord L1 Experiment

The first behavior handoff is deliberately narrower than generic medium.
Page8K metadata is already type-stable for the current page lifetime, so a
unified-domain Page8K hit can pass that record directly to the existing free
state machine. The free implementation still validates the run boundary,
exact 8KiB slot geometry, slot state, owner publication, and remote pending
contract. Owned-invalid pointers remain fail-closed.

This lane removes only the second Page8K registry lookup. Generic medium keeps
its old directory, lock, slot-state, and pending authority. A unified miss
also falls through the complete old chain, preserving high-address and
publication-failure safety. The speed lane has no diagnostic counters or new
atomics.

Lane:

```text
hz8-r3-unified-page8k-record
```

Acceptance is attribution rather than promotion: fixed8K should recover the
kind-only L1 regression and preferably remain within 3% of TargetDispatch;
smoke, safety stress, owner exit, remote publication, duplicate rejection,
and invalid-pointer rejection must remain unchanged. A regression above 5%
or any safety failure closes direct Page8K record handoff. Public HZ8 v2 and
generic-medium behavior remain unchanged either way.

## Page8KRecord L1 Result

The dedicated lane builds without diagnostic counters. WSL smoke and safety
stress pass with owner exit 8, remote free 8,192, duplicate claim rejection 1,
and invalid rejection 7. A preliminary WSL R5 fixed8K comparison measured
`506.65M -> 535.79M` (`+5.75%`) against TargetDispatch.

Native Windows ABAB R10 measured:

| Row | TargetDispatch | Page8K record | Delta |
|---|---:|---:|---:|
| fixed8K | 10.110M | 9.941M | -1.68% |
| balanced | 52.374M | 54.396M | +3.86% |
| wide_ws | 52.666M | 52.910M | +0.46% |
| larger_sizes | 21.589M | 21.574M | -0.07% |

The platform response differs, but both results remove the kind-only L1
fixed8K cliff and all Windows control rows stay inside the acceptance guard.
This confirms that the second Page8K classifier was the important kind-only
cost. Page8KRecord-L1 is GO as an opt-in research candidate. It does not
promote UnifiedMediumDomain or change the HZ8 v2 public default. Generic
medium record handoff remains a separate lifetime/authority experiment.
