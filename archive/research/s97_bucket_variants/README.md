# S97 bucketize variants (ARCHIVED / NO-GO)

This folder archives “flush-side bucketize” variants that are no longer kept in the mainline `hz3_remote_stash_flush_budget_impl()` path.

Why archive:
- The S97 boundary is hot and already complex; keeping many NO-GO variants in-tree increases maintenance cost and review noise.
- These variants were useful for learning, but are not part of the current recommended lanes.

Scope:
- Archived implementations (copied from `hakozuna/hz3/src/hz3_tcache_remote.c`):
  - `s97_bucket_3_sparseset.inc` (S97-3)
  - `s97_bucket_4_touched.inc` (S97-4)
  - `s97_bucket_5_flatslot.inc` (S97-5)

Status summary (r90, pinned, warmup, 20 runs median; details in work orders):
- S97-3: workload/thread-band dependent, not a general replacement → archived.
- S97-4: wins around T~=16 but is clear NO-GO at T=8 on this host → archived (lane split preferred to keep mainline clean).
- S97-5: reset O(1) does not fix instruction/branch overhead → NO-GO.

Pointers:
- Flag index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md` (S97 section)
- Work orders:
  - `hakozuna/hz3/docs/PHASE_HZ3_S97_3_REMOTE_STASH_SPARSESET_BUCKET_BOX_WORK_ORDER.md`
  - `hakozuna/hz3/docs/PHASE_HZ3_S97_4_REMOTE_STASH_TOUCHED_BUCKET_BOX_WORK_ORDER.md`
  - `hakozuna/hz3/docs/PHASE_HZ3_S97_5_REMOTE_STASH_FLATSLOT_BUCKET_BOX_WORK_ORDER.md`

