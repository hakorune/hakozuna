# HZ6 Remote Route Phase Plan

This is the active design note for the HZ6 MT remote-route cliff seen on both
Ubuntu LD_PRELOAD and the Windows selected-family direction.  Keep this box
separate from the already-selected local/mixed_ws preload work until the remote
rows have clean median evidence.

For an external design-review prompt, see
`HZ6_REMOTE_ROUTE_CHATGPT_PRO_QUESTION.md`.

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
- `main_r0` and `guard_r0` were healthy enough to complete, so the issue is
  not a general malloc/free wrapper failure.
- Remote-positive rows repeatedly fell into route lookup probe loops:
  `hz6_preload_route -> hz6_allocator_route_lookup* ->
  hz6_route_backend_lookup_page_table_*`.
- `cross128_r90` also exposed a correctness break:
  `double free or corruption (!prev)` from
  `hz6_route_table_compact_tombstones()` while remote rehome was unregistering
  and compacting route tables from multiple threads.

This is a route-ownership boundary problem, not an Ubuntu-only launcher issue.
The same box should be treated as suspect on Windows whenever the selected
family uses shared route directory, owner-locality, tombstone compaction, and
remote rehome.

## Current Evidence

Debug-only patches proved these points:

- Adding shared route directory and owner-locality to the Linux preload selected
  flags removes the first all-visible scan wall for short remote rows.
- Serializing route table mutation around register/unregister/compact removes
  the compaction crash in the tested path.
- Disabling the final preload visible fallback turns the immediate hang into
  completion on smaller cross rows, proving that visible fallback is too
  expensive for MT remote.
- The 300K rows are still not paper-ready:
  `main_r50` completed around 61s in one debug pass, and `cross128_r90` still
  exceeded a 90s timeout.  Hot stacks then showed local page-table lookup miss,
  not only visible fallback.

Do not start RUNS=10 paper numbers from this state.  Use these numbers only as
debug evidence.

## Phase Split

### Phase 1: Correctness Fence

Goal: no abort, no silent real-free of HZ6 pointers, no concurrent route-table
compaction mutation.

Boundary:

- Route-table writes are serialized per allocator.
- Tombstone compaction is treated as a writer, not as a background read-side
  cleanup.
- Rehome must not create a shared-directory empty window.  Register the new
  owner before unregistering the old owner, and rely on owner-qualified
  unregister to avoid deleting the new mapping.

Rollback:

- `HZ6_ROUTE_TOMBSTONE_COMPACT_L1=0` remains the emergency safety rollback, but
  it is not a performance solution because tombstones accumulate.

### Phase 2: Preload Route Boundary

Goal: `free()` must not use all-visible route lookup as the normal remote path.

Boundary:

- `hz6_preload_route()` should route in this order:
  1. exact shared route directory hit,
  2. owner-locality owner-only lookup when owner is known,
  3. local allocator lookup only when local ownership is plausible,
  4. explicit miss handling.
- All-visible lookup is a debug fallback only.  It should be behind a rollback
  flag and disabled for the selected MT remote lane.

Rollback:

- `HZ6_PRELOAD_ROUTE_VISIBLE_FALLBACK_L1=1` restores the conservative old
  fallback for compatibility/bisect only.

### Phase 3: Remote Miss Without Page-Table Scan

Goal: a pointer that is not local should not pay a full local route-table miss.

Hypothesis:

- The remaining 300K cliff is caused by plausible-local misses entering the
  local page-table backend after shared/owner hints fail or become tombstoned.

Design target:

- Add a cheap preload-visible "HZ6 pointer owner hint" that is stable enough to
  answer `definitely foreign`, `definitely local`, or `unknown`.
- For `definitely foreign`, do not run local route-table lookup.  Use
  shared-directory / owner-only lookup and fail fast if neither can prove
  ownership.
- For `unknown`, preserve the current conservative fallback until counters show
  it is safe to narrow.

Counters to read first:

- preload route miss real count,
- shared-directory lookup/register/unregister/stale,
- owner-locality hit/miss/probe max,
- route lookup page valid/invalid/miss and probe max,
- route rehome attempt/success/fail,
- route tombstone current/max/compact success.

### Phase 4: Directory Tombstone Policy

Goal: shared route directory and owner-locality must not become long-run
tombstone fields.

Hypothesis:

- Route exact tombstones are no longer the only table that matters.  Directory
  tombstones can make owner/shared lookup expensive or fail into route scans.

Design target:

- Add directory tombstone counters before adding compaction.
- Prefer generation-preserving in-place owner update for rehome over
  unregister/register churn when the same pointer remains active.
- Only compact or rebuild shared/owner directories behind a single writer
  boundary if counters prove tombstones are the cliff.

### Phase 5: Cross-Platform Promotion Gate

Run Ubuntu first because `gdb`/`LD_PRELOAD` inspection is cheaper.  Then port
the same boxed flags and rerun Windows x64.

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
- median-based rows are within range to compare against HZ5/HZ4/tcmalloc,
- Ubuntu x86_64 and Windows x64 agree on the shape before promotion.

## Current Status

Status: active design / not selected-final.

Next implementation box:

```text
RemoteRouteOwnerHint-L1
```

One job: make preload free decide local-vs-foreign before entering the local
page-table route lookup.  Do not tune tombstone thresholds or route capacity in
the same patch.
