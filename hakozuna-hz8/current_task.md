# Current Task

HZ8 adoption path is being implemented in this order:

1. `AdoptionOpportunityAudit-L1`
2. `SpanAtomicOwnership-L1`
3. `SpanPublishLease-L1`
4. `OwnerLifecycleLease-L1`
5. `OrphanQuiescing-L1`
6. `RegularAdoptionDryRun-L1`
7. `RegularAdoption-L1`

Current focus:

- `RegularAdoptionDryRun-L1`

Rules:

- Keep `counter / invariant lane` first.
- Keep v0 behavior stable until dry-run and zero gates are proven.
- Do not add a new global route on the hot path.
- Keep the implementation under 800 lines per task slice.

Validation gates already in use:

- `remote_publish_lost = 0`
- `remote_collect_duplicate = 0`
- `remote_pending_count_mismatch = 0`
- `span_decommit_while_pending = 0`
- `invalid_owned_fallback = 0`
- `dead_owner_publish_lost = 0`
