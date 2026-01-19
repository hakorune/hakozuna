# S128 RemoteStash Defer-MinRun (ARCHIVED / NO-GO)

S128 tried to merge `(dst,bin)` groups across multiple `flush_budget` calls by
deferring small groups until `n >= MIN_RUN` (or `age >= MAX_AGE`).

Result:
- NO-GO: the pending hash-table / probe costs and unpredictable branches dominated.
- Keeping the code in the hot boundary increased maintenance cost, so it is archived.

Where it used to live:
- Boundary: `hakozuna/hz3/src/hz3_tcache_remote.c` → `hz3_remote_stash_flush_budget_impl()`

Archived snapshot:
- `s128_defer_minrun.inc` (reference-only, not built)

References:
- Work order: `hakozuna/hz3/docs/PHASE_HZ3_S128_REMOTE_STASH_DEFER_MINRUN_BOX_WORK_ORDER.md`
- NO-GO summary: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`

