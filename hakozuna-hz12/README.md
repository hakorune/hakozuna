# Hakozuna HZ12

HZ12 is a Windows-first standalone allocator research line for advisory
ownership and reclaimable spans. It is not a public allocator or a replacement
for HZ8. Its initial span core is a deliberately renamed HZ11-derived baseline
inside this folder; HZ12 does not compile against or link HZ11 sources.

## Position

```text
HZ8:
  public balanced allocator
  low RSS, fail-closed ownership, practical cross-thread correctness

HZ11:
  speed-first ownerless front-cache / transfer-cache / span experiment
  selected Windows row: hz11-span-cache256

HZ12:
  standalone HZ12 span substrate and hz12_* API
  advisory ownership at flush granularity
  bounded owner inboxes and reclaimable spans
  low-RSS research with cross-owner throughput as secondary evidence
```

HZ12 exists because the HZ11 Windows cross-owner pipeline measured a large,
deterministic ownerless gap: 100% cross-owner frees, sharing factor 2.0, and
HZ11 7.45M ops/s versus tcmalloc 27.35M in the initial R3. HZ12 does not put an
owner check or atomic on every free. Ownership is advisory locality metadata,
used only at cold flush/drain/reclaim boundaries.

The first bounded owner-inbox L1 behavior prototype reached 24.516M ops/s in a
same-session 100% cross-owner R3, versus 9.138M for HZ11 ownerless and 28.553M
for tcmalloc. This is mechanism evidence only; HZ12 is not a public allocator
or a default lane.

The later public-API integration probe kept normal free owner-blind and routed
only class-cache flush batches. Its safe owner-by-class inbox reached 26.128M
ops/s versus HZ11 ownerless at 12.939M and tcmalloc at 36.318M in the fixed R5,
with 11.79 MiB median peak RSS. It remains opt-in/HOLD because same-owner local
random_mixed regressed by about 7%.

ColdSpanOwner-L1 then moved owner assignment to 64 KiB span acquisition and
inbox drain to current-span replacement. It restored local random_mixed to
within 1.4..2.5% of core and reached 29.064M ops/s with 11.32 MiB peak RSS in
the fixed xowner R5, versus tcmalloc at 36.911M and 15.13 MiB. It is the current
opt-in integration candidate; owner lifetime/thread churn remains a blocker.

ColdSpanOwner-L2 adds generation-tagged owner slots and Windows FLS teardown.
Its 128-thread churn smoke completed with 127 slot reuses, zero table-full
events, and matching 128 attach/detach operations. Local R5 stayed within
1.1..1.7% of core. Concurrent publish-versus-retire remains the next gate.

## First Rule Set

```text
do:
  start Windows-first with shadow-only owner-routing diagnostics
  keep owner tags advisory, never the safety authority
  route only at flush granularity, not on every free
  bound inboxes and fall back to ownerless recycling on overflow
  use ownership to make whole-span reclaim/decommit observable

do not:
  alter HZ11 selected/default behavior
  add per-free owner lookup or atomic metadata to a speed lane
  add a lock-free remote queue in L0/L1
  use owner routing as a reason to weaken route or free safety
  claim HZ12 improves throughput before a behavior lane proves it
```

## Read First

```text
current_task.md
docs/HZ12_CHARTER_L0.md
docs/HZ12_WINDOWS_OWNER_ROUTING_SHADOW_L0.md
docs/HZ12_WINDOWS_COLD_SPAN_OWNER_L1_20260710.md
docs/HZ12_WINDOWS_COLD_SPAN_OWNER_L2_CHURN_20260710.md
docs/HZ12_WINDOWS_BOUNDED_OWNER_INBOX_L1.md
docs/HZ12_WINDOWS_DEAD_OWNER_ADOPTION_SHADOW_L2A.md
docs/HZ12_WINDOWS_RETIRED_INBOX_ADOPTION_L2B.md
docs/HZ12_WINDOWS_WHOLE_SPAN_ACCOUNTING_SHADOW_L2C.md
docs/HZ12_WINDOWS_RECLAIM_DETACH_GATE_L2D.md
docs/HZ12_WINDOWS_ORDERED_SPAN_DETACH_L2E.md
docs/HZ12_WINDOWS_DETACHED_SPAN_DECOMMIT_L2F.md
docs/HZ12_WINDOWS_DECOMMITTED_SPAN_DEPOT_L2G.md
docs/HZ12_WINDOWS_SPAN_DEPOT_CYCLE_L2H.md
docs/HZ12_WINDOWS_OWNER_REGISTRY_L3A.md
docs/HZ12_WINDOWS_TOKEN_INBOX_L3B.md
docs/HZ12_WINDOWS_OWNER_EPOCH_L3C.md
docs/HZ12_WINDOWS_OWNER_RETIRE_GATE_L3D.md
docs/HZ12_WINDOWS_TOKEN_RETIRE_LIVE_L4A.md
docs/HZ12_WINDOWS_TOKEN_XOWNER_PIPELINE_L4B.md
docs/HZ12_WINDOWS_TOKEN_DRAIN_POLICY_L4C.md
docs/HZ12_WINDOWS_RETIRED_OWNER_RECLAIM_SHADOW_L5A.md
docs/HZ12_WINDOWS_CLASS_FLUSH_CHECKPOINT_L5B.md
docs/HZ12_WINDOWS_BOUNDED_RETIRED_DETACH_L5C.md
docs/HZ12_WINDOWS_BOUNDED_RETIRED_DECOMMIT_L5D.md
docs/HZ12_WINDOWS_RETIRED_DEPOT_CYCLE_L5E.md
docs/HZ12_WINDOWS_RETIRED_SPAN_RECYCLE_L5F.md
docs/HZ12_WINDOWS_MEASUREMENT_TABLE_20260710.md
docs/HZ12_WINDOWS_TCMALLOC_HOTPATH_AUDIT_20260710.md
docs/HZ12_SOURCE_LAYOUT.md
docs/HZ12_PORTABILITY_CONTRACT_L0.md
docs/HZ12_NO_GO_LEDGER.md
```

## Folder Policy

HZ12 is self-contained under `hakozuna-hz12/`. The initial HZ12 core was
forked from the proven HZ11 Windows span baseline and renamed to `hz12_*` so
ownership and reclaim can evolve without modifying HZ11. Future changes belong
in HZ12; synchronization back to HZ11 is intentionally out of scope.

Allocator behavior, advisory ownership, inbox handoff, span accounting, and
the read-only reclaim gate are separate source modules. See
`docs/HZ12_SOURCE_LAYOUT.md` before adding another lifecycle box.

L3-B adds a diagnostic token inbox bound to `slot + generation`. Its two-
producer repeat-20 smoke accepts ACTIVE/RETIRING batches, ownerless-frees DEAD
and stale batches, and rebinds only an empty inbox to a replacement generation.
Normal `hz12_malloc`/`hz12_free` performs neither token lookup nor diagnostic
counter updates.

L3-C adds an enrollment-bounded owner epoch. Its repeat-20 smoke requires each
producer recorded at retirement start to acknowledge the epoch before
`RETIRING -> DEAD`; unregistering cannot shrink that captured set. This remains
a cold diagnostic lifecycle coordinator, not an allocator barrier.

L3-D composes the captured producer epoch with token-inbox emptiness for a
controlled teardown decision. Its repeat-20 smoke blocks before checkpoints,
blocks again while final-flush objects remain queued, and opens only after the
exact inbox drain. This gate is not connected to normal allocation/free.

L4-A is a paced live diagnostic that joins token publication, owner draining,
and quiescent retirement. Its repeat-5 run completed without overflow,
registry rejection, or generation rejection. It is lifecycle evidence only,
not a performance lane or public allocator behavior.

L4-B extends that evidence to a one-producer/one-consumer 100% cross-owner
pipeline. It passed repeat-5 with no token rejection or overflow, but its eager
owner drain was about 13% slower than the owner-id L1 control. It is therefore
a safe token-routing control, not a promoted performance profile.

L4-C found that widening the token drain interval improves apparent throughput
but does not reproducibly avoid bounded fallback on Windows. It is recorded as
no-go policy evidence; HZ12 does not promote a token-drain default.

L5-A returns to the low-RSS charter with a generation-aware span-owner side
table and a read-only retired-owner reclaim scan. Its repeat-20 smoke reports
zero bytes for incomplete thread scope, exactly 64 KiB only after the existing
L2 gate opens, and excludes stale-generation spans.

The L5-A wide working-set diagnostic found a 72.88 MiB median physical
full-span candidate upper bound against 82.00 MiB median peak RSS. Foreign
cache scope is still unknown, so it reports zero fully reclaimable bytes.

L5-B adds an explicit diagnostic class-flush checkpoint. Its wide_ws-like
repeat-5 exposes a median 80.75 MiB physical candidate set with no foreign-cache
blocker; returned-sink contents and live routes remain the final blockers.

L5-C uses a fixed 64-span quiescent budget. Its repeat-3 detached 64/64 spans
with zero failures and exposed exactly 4.00 MiB as reclaimable, without
decommit or depot insertion.

L5-D decommits the same fixed 64-span success set. Repeat-3 released exactly
4.00 MiB and reduced working-set RSS by 3.99 MiB in every run, with no depot or
automatic policy enabled.

L5-E connects only that bounded diagnostic set to the existing 64-span depot.
It requires same-address recommit, accounting reset, route restoration, exact
owner generation, and an empty depot after the cycle. It remains disconnected
from normal allocation/free policy.

The Windows repeat-3 passed 64/64 puts and takes in every run, recommitted
4.00 MiB, and reported zero address or owner-generation mismatches. This is
bounded lifecycle evidence, not an automatic depot policy.

L5-F completed the second lifecycle cycle in repeat-3: all 64 spans were fully
carved, touched, normally freed, detached again, decommitted again, and returned
to the depot. It touched 9,984..10,048 class-dependent slots per run, released
4.00 MiB again, and reported zero address, generation, or lifecycle failures.
The bounded Windows mechanism is complete; automatic reclaim policy remains
out of scope.

The provenance-safe Windows cross-owner runner now separates allocator behavior
from diagnostics. In R5, the counter-free/accounting-free owner-inbox mechanism
reached 28.896M ops/s versus HZ11 ownerless at 13.046M and tcmalloc at 36.833M.
It preserves bounded inbox/fallback behavior but does not yet provide automatic
reclaim authority; atomic accounting remains a diagnostic judge only.

The follow-up OwnerFastLoad candidate keeps the first-writer CAS but handles an
already matching span owner with a relaxed load. It reached 35.542M ops/s
versus tcmalloc at 37.597M in the same R5 100% cross-owner pipeline (94.5%),
and passed repeat-10 retirement/pending safety checks. It remains opt-in until
local/random and broad workload controls are complete.

Fair RSS sampling later measured HZ12 OwnerFastLoad at 36.427M ops/s and
15.54 MiB peak RSS versus tcmalloc at 36.439M and 14.81 MiB. The row is speed
parity, not a low-RSS win. Local random_mixed confirms the HZ12 core remains
strong and compact, while a decomposition rejects per-free owner lookup and
keeps only allocation attribution for the next flush-time routing experiment.
See `docs/HZ12_WINDOWS_OWNERFAST_RSS_AND_BROAD_20260710.md`.
