# HZ6 Remote Route Design Consultation Notes

This file records the external design-review prompt and the accepted revision
direction for the HZ6 remote-route repair.  The active work order is
`HZ6_REMOTE_ROUTE_PHASE_PLAN.md`.

## Context

We are developing HZ6, a research allocator with Linux LD_PRELOAD and Windows
selected-family lanes.  The current issue is a multi-threaded remote-free
benchmark cliff.  The same route-ownership structure is expected to matter on
both Ubuntu and Windows, so an Ubuntu-only workaround is not acceptable.

Canonical workload shape:

```text
bench_random_mixed_mt_remote threads=16 ws=100 ring_slots=65536
main_r0      min=16 max=32768  remote_pct=0
main_r50     min=16 max=32768  remote_pct=50
main_r90     min=16 max=32768  remote_pct=90
guard_r0     guard lane remote_pct=0
cross128_r90 min=16 max=131072 remote_pct=90
```

Promotion target:

```text
RUNS=10, T=16, median ops/s
Must pass on Ubuntu x86_64 and Windows x64 before selection.
```

## Observed Failure

During an Ubuntu LD_PRELOAD debug pass:

- local-only rows completed,
- remote-positive rows timed out or became extremely slow,
- `cross128_r90` exposed `double free or corruption (!prev)`,
- gdb showed remote-free stacks spending time in:

```text
free
hz6_preload_route
hz6_allocator_route_lookup*
hz6_route_backend_lookup_page_table_*
```

The crash stack pointed at:

```text
hz6_route_table_compact_tombstones
hz6_allocator_route_unregister_exact_reason
hz6_allocator_route_rehome_exact
hz6_free_route_dispatch
```

Interpretation:

- route-tombstone compaction was mutating a route table while remote rehome was
  also unregistering/registering routes from multiple threads,
- remote-positive free sometimes fell through into local or visible route-table
  scans,
- the issue is at the route ownership boundary, not at the Linux launcher.

## Original Design Plan Under Review

The original proposal used `RemoteRouteOwnerHint-L1`, a preload-visible
local-vs-foreign hint, as the next box.

Proposed phase split:

1. Correctness fence:
   serialize route table register/unregister/compact as writer mutations, and
   avoid shared-directory empty windows during rehome.

2. Preload route boundary:
   remove all-visible route lookup from the normal remote-free path.  Keep it
   only as a rollback/debug fallback.

3. `RemoteRouteOwnerHint-L1`:
   before local page-table route lookup, classify the pointer as:
   `definitely local`, `definitely foreign`, or `unknown`.

   For `definitely foreign`, do not run local route-table lookup.  Use shared
   directory or owner-only lookup, then fail fast if ownership cannot be proven.

   For `unknown`, preserve a conservative fallback until counters prove it can
   be narrowed.

4. Directory tombstone policy:
   measure shared-directory and owner-locality tombstone pressure.  Prefer
   generation-preserving in-place owner update for rehome if the same pointer
   remains active.

5. Cross-platform gate:
   only promote after Ubuntu x86_64 and Windows x64 both pass the RUNS=10 MT
   remote matrix without abort, timeout, or real-free fallback for HZ6-owned
   pointers.

## Constraints

- Keep changes boxed and reversible with build flags.
- Do not silently real-free an HZ6-owned pointer.
- Do not hide integrity failures behind fallback.
- Avoid broad route-capacity tuning in the same patch as ownership routing.
- If a C source file grows beyond roughly 800 lines, split the new logic into a
  focused module instead of adding more cross-cutting code.
- Prefer one obvious boundary for local-vs-foreign routing decisions.

## Questions For Review

1. Is the `definitely local / definitely foreign / unknown` split the right
   ownership boundary for remote free, or should the route layer expose a
   different primitive?

2. For `definitely foreign` with shared-directory miss, is fail-fast preferable
   to falling back to all-visible scan?  What invariants must be checked to
   ensure this cannot turn into a lost free?

3. Should remote rehome update shared-directory / owner-locality in place
   instead of unregister/register, assuming pointer and descriptor generation
   remain valid?

4. Should route table compaction be protected by an allocator-local writer lock,
   or should compaction be moved out of the remote-free path entirely?

5. What counters would best distinguish:
   - local route miss cliff,
   - shared-directory tombstone cliff,
   - owner-locality tombstone cliff,
   - rehome churn,
   - real HZ6 ownership loss?

6. Is there a cleaner cross-platform design that keeps Ubuntu LD_PRELOAD and
   Windows selected-family behavior aligned without duplicating route policy in
   platform wrappers?

## Desired Output

Please respond with:

- the strongest objection to `RemoteRouteOwnerHint-L1`,
- the safest route ownership invariant set,
- a minimal implementation shape with module boundaries,
- counters needed before performance tuning,
- and a GO / NO-GO recommendation for implementing Phase 1 and Phase 2 first.

## Accepted Review Result

The review accepted the broad phase ordering but rejected
`RemoteRouteOwnerHint-L1` as the correctness boundary.

Decision:

```text
RemoteRouteOwnerHint-L1: NO-GO
RemoteFreeRouteResolve-L1: GO
```

Reason:

- owner-locality is a search hint, not ownership proof,
- current shared-directory publication is not coherent enough to treat a miss
  as external,
- writer-only route locking does not protect lookup or last-hit cache readers,
- current unregister/register rehome creates a MISS window,
- current preload MISS -> real_free behavior conflates external and unresolved
  HZ6 ownership.

Accepted replacement:

```text
RemoteFreeRouteResolve-L1

LOCAL_VALID
FOREIGN_VALID
OWNED_INVALID
PROVEN_EXTERNAL
RETRY
UNRESOLVED_INTEGRITY
```

Key design changes:

- make the global exact directory authoritative and coherently published,
- synchronize route lookup, last-hit, mutation, and compaction in a
  route-domain boundary,
- defer compaction out of remote unregister,
- replace unregister/register rehome with checked in-place owner transfer,
- move free-route policy into a shared core resolver used by Ubuntu and
  Windows wrappers,
- split `PROVEN_EXTERNAL` from `UNRESOLVED_INTEGRITY`; only the former may call
  platform real free.

Follow the revised phase plan in `HZ6_REMOTE_ROUTE_PHASE_PLAN.md`.
