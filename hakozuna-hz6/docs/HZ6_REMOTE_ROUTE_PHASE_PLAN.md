# HZ6 Remote Route Phase Plan

This is the active design note for the HZ6 MT remote-route cliff seen on both
Ubuntu LD_PRELOAD and the Windows selected-family direction.  Keep this box
separate from the already-selected local/mixed_ws preload work until the remote
rows have clean median evidence.

For the design-review prompt that led to this revision, see
`HZ6_REMOTE_ROUTE_CHATGPT_PRO_QUESTION.md`.

## Design Decision

The previous working name, `RemoteRouteOwnerHint-L1`, is closed as a design
direction.  The owner-locality table is a hint, not an ownership proof, and it
must not become a correctness boundary.

The active box is now:

```text
RemoteFreeRouteResolve-L1
```

One job: create one authoritative free-route resolver shared by Linux preload,
Windows wrappers, and core HZ6 route/free logic.

The resolver must return one of these outcomes:

```text
LOCAL_VALID
FOREIGN_VALID
OWNED_INVALID
PROVEN_EXTERNAL
RETRY
UNRESOLVED_INTEGRITY
```

`PROVEN_EXTERNAL` is the only outcome that may call platform `real_free`.
`OWNED_INVALID` and `UNRESOLVED_INTEGRITY` must not silently return or real-free;
in research lanes they should fail fast.

## Problem Box

Workload:

```text
bench_random_mixed_mt_remote threads=16 ws=100 ring_slots=65536
main_r50:     min=16 max=32768  remote_pct=50
main_r90:     min=16 max=32768  remote_pct=90
cross128_r90: min=16 max=131072 remote_pct=90
```

Observed during the 2026-06-19 debug pass:

- Ubuntu selected preload initially timed out on remote-positive rows.
- `main_r0` and `guard_r0` completed, so the issue is not a general wrapper
  failure.
- Remote-positive rows repeatedly fell into route lookup probe loops:
  `hz6_preload_route -> hz6_allocator_route_lookup* ->
  hz6_route_backend_lookup_page_table_*`.
- `cross128_r90` exposed `double free or corruption (!prev)` from
  `hz6_route_table_compact_tombstones()` while remote rehome was unregistering
  and compacting route tables from multiple threads.

This is a route-ownership boundary problem, not an Ubuntu-only launcher issue.
The same box should be treated as suspect on Windows whenever the selected
family uses shared route directory, owner-locality, tombstone compaction, and
remote rehome.

## Implementation Status

2026-06-19 Phase 1A is implemented for the Ubuntu selected preload build:

- `api/hz6_allocator_route_domain.[ch]` owns the allocator-local route-domain
  synchronization boundary.
- `HZ6_ROUTE_DOMAIN_SYNC_L1=1` guards route lookup, last-hit, register,
  unregister, replace, invalid-range register/unregister, and visible
  allocator raw lookups.
- `HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1=1` defers tombstone compaction from
  rehome unregister and records owner-local compaction debt.
- `HZ6_ROUTE_VISIBLE_EXACT_ONLY_L1=1` keeps the legacy all-visible fallback
  from doing foreign invalid-range full-table scans after a local miss.  This
  is a temporary stopgap; Phase 2 must replace this policy with
  `RemoteFreeRouteResolve-L1`.

Smoke evidence after Phase 1A:

```text
bench_random_mixed_mt_remote 16 10000 100 16 32768 50 65536
  ops/s=120972.11 fail=0 timeout=no

bench_random_mixed_mt_remote 16 120000 100 16 131072 90 65536
  ops/s=90634.09 fail=0 timeout=no
```

These are completion/safety smokes only.  They are not paper numbers and they
confirm that the synchronized lane is still far from Phase 3 performance.

## Ownership Model

Keep these roles explicit:

```text
descriptor_storage_owner:
  allocator that stores the descriptor memory

route_index_owner:
  allocator that owns the local exact route entry

logical_owner:
  allocator that currently handles reuse / remote free ownership
```

`route_allocator` currently carries more than one meaning.  The resolver must
avoid adding a third correctness source through owner-locality.  Owner-locality
may guide search order only after the authoritative directory invariants are in
place.

Target structure:

```text
Authoritative Global Exact Directory
  (ptr, object_generation)
    -> route_index_owner
    -> logical_owner
    -> descriptor
    -> descriptor_storage_owner
    -> front_id / class_id

Allocator-local Route Table
  local fast path and owned-invalid proof
  not the global ownership SSOT

Owner Locality Index
  optional derived cache
  no correctness decision
```

## Required Invariants

### Mandatory Publication

Remote-safe lanes must publish every HZ6 pointer to the authoritative global
exact directory before returning it to user code.

```text
local route register success
global directory publish failure
```

means allocation rollback.  The global directory cannot remain best effort in
remote-safe lanes.

### Unique Authoritative Record

For each `(ptr, object_generation)`, at most one authoritative record may be
stable.

Owner checks must include more than an allocator pointer:

```text
allocator pointer
owner slot / token
owner generation
object generation
descriptor
```

Allocator-pointer-only unregister is not enough for remote rehome safety.

### Coherent Snapshot

Directory lookup may return only:

```text
complete old record
complete new record
RETRY
MISS
```

It must not observe `base` without a coherent payload and translate that to
MISS.  The preferred implementation is a per-entry sequence protocol:

```text
writer:
  sequence = odd
  update fields
  sequence = next even

reader:
  read sequence
  read fields
  read sequence again
  accept only if both sequence reads match and are even
```

Use a sharded writer lock or equivalent when the directory update spans route
state that must be committed together.

### Route-Domain Synchronization

Writer-only route table locking is insufficient.  Lookup, last-hit cache,
register, unregister, replace, and compact all share the route-domain.

Phase 1 correctness may use a simple route-domain mutex first:

```text
route lookup:
  route-domain read/sync boundary

register/unregister/replace/compact:
  route-domain exclusive boundary
```

After correctness is proven, read-side cost can be reduced.  Do not leave
all-visible scan or foreign allocator backend lookup as raw unsynchronized
access once route-domain locking exists.

### Rehome Is Transfer, Not Delete/Insert

For a live object whose pointer, descriptor, and object generation remain the
same, rehome must not be:

```text
unregister old
register new
```

Use a checked transfer primitive:

```text
compare_and_transfer_owner(
  ptr,
  object_generation,
  expected_old_owner,
  expected_descriptor,
  new_owner)
```

This avoids shared-directory MISS windows and lets failed preconditions abort
before publication.

### Remote Free Transaction

Remote free and route transfer must be one transaction.  The safe order is:

```text
1. reserve destination transfer slot
2. acquire origin/destination route-domain locks in stable order
3. mark directory record UPDATING
4. revalidate ptr/generation/descriptor/old owner
5. prepare destination local route
6. remove origin local route without compaction
7. commit descriptor state / transfer object
8. publish directory owner transfer
9. mark directory record stable
10. release locks
11. record origin compaction debt
```

Rule:

```text
before publish:
  all operations are rollback-capable

after publish:
  no fallible operation remains
```

### External vs Unresolved

Current preload `free()` treats route MISS as permission to call real `free()`.
The new resolver must split this:

```text
PROVEN_EXTERNAL:
  real_free allowed

UNRESOLVED_INTEGRITY:
  real_free prohibited

OWNED_INVALID:
  real_free prohibited
```

All-visible scan may remain only as a diagnostic shadow oracle:

```text
directory miss but visible scan hit
  -> record counter
  -> fail fast
```

It must not be a normal fallback that repairs the result and continues.

## Phase Split

### Phase 1A: Route-Domain Correctness

Goal: remove route table data races before changing policy.

Implement:

- `api/hz6_allocator_route_domain.[ch]`
- route-domain synchronization for lookup/register/unregister/replace/compact
- `route_last_hit` inside the same synchronization boundary or disabled for
  MT remote diagnostic lanes
- remote unregister records compaction debt instead of running compaction

Compaction policy:

```text
unregister:
  add tombstone debt
  compact_requested = 1
  return

owner-local maintenance point:
  exclusive route-domain lock
  compact once
```

Selected candidates must keep:

```text
route_compact_remote_path_attempt = 0
```

### Phase 1B: Authoritative Directory And Rehome Transaction

Goal: make the global exact directory authoritative enough for remote free.

Implement:

- `api/hz6_allocator_global_route_directory.[ch]`
- coherent snapshot lookup
- mandatory publish for remote-safe lanes
- checked in-place owner transfer
- directory tombstone counters
- rollback-safe remote rehome transaction

Owner-locality becomes a derived cache only.  It can be disabled or stale
without changing correctness.

### Phase 2: Shared Core Free Resolver

Goal: move free-route policy out of platform wrappers.

Implement:

- `api/hz6_allocator_route_resolve_free.[ch]`
- result enum:
  `LOCAL_VALID`, `FOREIGN_VALID`, `OWNED_INVALID`, `PROVEN_EXTERNAL`,
  `RETRY`, `UNRESOLVED_INTEGRITY`
- common resolver used by Linux preload, Windows wrappers, `realloc()`, and
  `malloc_usable_size()` where ownership proof is needed

Resolver order:

```text
active-map fast paths
  -> global exact directory coherent lookup
      -> local valid / foreign valid / updating retry / miss
  -> local route lookup
      -> valid / invalid / miss
  -> HZ6 address-domain check
      -> owned invalid or unresolved integrity
      -> proven external
```

All-visible lookup is debug shadow only.

### Phase 3: Performance After Correctness

Only after Phase 1 and Phase 2 prove correctness:

- reduce route-domain read contention,
- tune directory probe/tombstone maintenance,
- add owner-locality cache as an optional acceleration,
- tune rehome frequency and transfer-cache behavior.

Do not tune route capacity or tombstone thresholds in the same patch as the
ownership resolver.

### Phase 4: Cross-Platform Promotion Gate

Run Ubuntu first because `gdb`/`LD_PRELOAD` inspection is cheaper.  Then port
the same boxed primitives and rerun Windows x64.

Promotion shape:

```text
RUNS=10 T=16 ws=100 ring_slots=65536
main_r0      16..32768  remote=0
main_r50     16..32768  remote=50
main_r90     16..32768  remote=90
guard_r0     guard lane remote=0
cross128_r90 16..131072 remote=90
```

Success criterion:

- no abort or timeout on all rows,
- no real-free fallback for HZ6-owned pointers,
- `free_resolve_real_free_unproven == 0`,
- `route_compact_remote_path_attempt == 0`,
- `rehome_post_publish_fail == 0`,
- median-based rows are within range to compare against HZ5/HZ4/tcmalloc,
- Ubuntu x86_64 and Windows x64 agree on the shape before promotion.

## Module Boundaries

### `api/hz6_allocator_route_domain.[ch]`

Responsibilities:

- allocator-local route synchronization,
- locked lookup/register/unregister/replace,
- compaction debt,
- deferred maintenance,
- route-last-hit synchronization.

Raw backend access should be closed behind this module.

### `api/hz6_allocator_global_route_directory.[ch]`

Responsibilities:

- `publish_exact`,
- `lookup_exact_snapshot`,
- `transfer_owner_exact`,
- `retire_exact`,
- directory tombstone counters,
- directory maintenance.

This is the upgraded form of the current shared-directory.

### `api/hz6_allocator_route_resolve_free.[ch]`

Responsibilities:

- global directory lookup,
- local route lookup,
- HZ6 address-domain classification,
- resolver result selection.

Linux preload and Windows wrappers call this module instead of each carrying
their own route policy.

### `api/hz6_allocator_remote_route_transfer.[ch]`

Responsibilities:

- transfer slot reservation,
- ordered dual-allocator locking,
- route transfer transaction,
- descriptor / transfer commit,
- rollback.

Do not implement remote rehome as generic unregister/register composition.

## Counters

Keep these under `HZ6_DIAGNOSTIC_PROBES` or a dedicated diagnostic build.  MT
remote lanes must not update foreign allocator non-atomic stats directly; use
thread-local aggregation or relaxed atomics.

Resolver:

```text
free_resolve_attempt
free_resolve_global_hit_local
free_resolve_global_hit_foreign
free_resolve_global_miss
free_resolve_global_retry
free_resolve_local_lookup_attempt
free_resolve_local_lookup_skipped
free_resolve_local_valid
free_resolve_local_invalid
free_resolve_local_miss
free_resolve_proven_external
free_resolve_unresolved_integrity
free_resolve_real_free_unproven
```

Directory:

```text
shared_dir_publish_attempt/success/fail
shared_dir_lookup_probe_total/max/hist
shared_dir_snapshot_retry
shared_dir_updating_retry_exhausted
shared_dir_duplicate_same_generation
shared_dir_duplicate_generation_mismatch
shared_dir_owner_token_stale
shared_dir_descriptor_generation_mismatch
shared_dir_tombstone_current/max
shared_dir_tombstone_probe_total
```

Divergence:

```text
owner_hint_foreign_shared_hit
owner_hint_foreign_shared_miss
owner_hint_local_shared_foreign
shared_miss_visible_shadow_hit
shared_miss_local_exact_hit
shared_hit_local_local_route_miss
shared_hit_foreign_descriptor_owner_mismatch
```

Route-domain:

```text
route_lock_read_contended
route_lock_write_contended
route_lock_max_wait
route_compact_deferred
route_compact_owner_maintenance
route_compact_remote_path_attempt
route_compact_moved
```

Rehome transaction:

```text
rehome_prepare_attempt
rehome_transfer_reserve_fail
rehome_expected_owner_mismatch
rehome_generation_mismatch
rehome_descriptor_mismatch
rehome_destination_route_prepare_fail
rehome_directory_transfer_success
rehome_rollback_success
rehome_rollback_fail
rehome_post_publish_fail
```

## GO / NO-GO Ledger

| Item | Decision |
| --- | --- |
| Phase 1 before performance work | GO |
| Writer-only route lock | NO-GO |
| Route lookup / last-hit / mutation / compaction in one route-domain sync boundary | GO |
| Compaction inside remote unregister | NO-GO |
| Deferred owner-local compaction maintenance | GO |
| Current unregister-register rehome with a lock around it | NO-GO |
| Checked in-place owner transfer | GO |
| Phase 2 implemented only in preload wrapper | NO-GO |
| Ubuntu/Windows shared core resolver | GO |
| All-visible lookup as normal fallback | NO-GO |
| All-visible lookup as diagnostic shadow oracle | GO |
| Hint miss or unresolved state calling real-free | NO-GO |
| `RemoteRouteOwnerHint-L1` as selected design | NO-GO |
| `RemoteFreeRouteResolve-L1` as active design | GO |

## Current Status

Status: active design / not selected-final.

Next implementation box:

```text
RemoteFreeRouteResolve-L1 Phase 1A
```

One job: introduce the route-domain boundary and deferred compaction without
changing ownership policy.  If any touched C file approaches 800 lines, move
the new logic into the module files listed above before continuing.
