# HZ12 Windows Owner Routing Shadow L0

- Runs: 3, runtime: 5s
- Producers/consumers: 4/4, ring: 4096
- Size: 8..1024
- Median diagnostic throughput: 8.227M ops/s

## Raw Shadow Counters

```text
[HZ12_OWNER_SHADOW] alloc_span_seen=37045458 alloc_owner_first=61 alloc_owner_reuse_foreign=27657474 flush_objects_total=37045458 flush_owner_local=0 flush_owner_foreign=37045458 flush_owner_unknown=0 would_route_batches=578821 would_route_objects=37045458 would_keep_ownerless_overflow=0 projected_inbox_current_max=256 projected_orphan_objects=0 projected_reclaim_blocked_spans=61
```
```text
[HZ12_OWNER_SHADOW] alloc_span_seen=41169327 alloc_owner_first=61 alloc_owner_reuse_foreign=30792178 flush_objects_total=41169327 flush_owner_local=0 flush_owner_foreign=41169327 flush_owner_unknown=0 would_route_batches=643253 would_route_objects=41169327 would_keep_ownerless_overflow=0 projected_inbox_current_max=256 projected_orphan_objects=0 projected_reclaim_blocked_spans=61
```
```text
[HZ12_OWNER_SHADOW] alloc_span_seen=39494110 alloc_owner_first=61 alloc_owner_reuse_foreign=29479521 flush_objects_total=39494110 flush_owner_local=0 flush_owner_foreign=39494110 flush_owner_unknown=0 would_route_batches=617083 would_route_objects=39494110 would_keep_ownerless_overflow=0 projected_inbox_current_max=256 projected_orphan_objects=0 projected_reclaim_blocked_spans=61
```
