# HZ12 Windows Retired-Owner Reclaim Shadow L5-A

## Purpose

L5-A reconnects HZ12 to its primary low-RSS charter. It observes spans associated
with a generation-tagged retired owner and composes owner quiescence with the
existing L2 span reclaim gate. It never detaches, decommits, or reuses a span.

## Authority Separation

```text
generation-aware span-owner side table:
  narrows discovery only

L3-D owner retirement gate:
  proves enrolled producers acknowledged retirement and token inbox is empty

L2 reclaim gate:
  remains the span safety authority
```

Owner metadata is stored in a separate side table. It is not read from span
payload, survives payload decommit, and is never consulted before pointer
classification on normal malloc/free.

## Byte Accounting

```text
detach_ready_bytes:
  all reclaim preconditions are true except the route is still attached

reclaimable_bytes:
  the existing L2 gate is fully open, including route detached
```

This distinction prevents a live route from being reported as already
reclaimable. Both values are shadow evidence; L5-A performs no behavior.

## Initial Result

The dedicated Windows smoke passed repeat-20. It reported zero bytes when
foreign-cache scope was incomplete, then identified current/front/returned/
route blockers before detach. After the existing L2 ordered-detach helper ran,
the same span was observed as exactly 65,536 reclaimable bytes. Reusing the
owner slot with a new generation classified the old side-table entry as stale
and attributed zero bytes to the replacement owner.

## Wide Working-Set Attribution

A diagnostic-only Windows runner uses the matrix shape (`T=8`, 200,000
iterations/thread, working set 16,384, sizes 16..1024) without the matrix's
periodic realloc. Physical carved-slot high-water replaces cumulative logical
allocation count as the full-span witness.

Repeat-5 reported 72.19..79.25 MiB of accounting-candidate spans, with a
72.88 MiB median, against 82.00 MiB median peak RSS. Thus the candidate upper
bound explains about 89% of observed peak RSS and is consistent in scale with
the earlier approximately 59 MiB wide_ws parking-lot signal. Fully reclaimable
bytes remain zero because foreign-thread cache scope is intentionally unknown.
This row is attribution evidence, not a throughput comparison.

## Foreign Cache Scope

The current L2 gate scans only the calling thread's cache. L5-A therefore takes
an explicit `thread_scope_complete` witness. If all enrolled threads have not
completed a class-flush checkpoint, foreign cache exposure is unknown and no
bytes may be counted as detach-ready or reclaimable.

## Acceptance

```text
stale generation spans are never attributed to the replacement owner
owner retirement gate must be open
thread scope must be complete
accounting must be clean and full-span complete
blocked reasons identify current/front/returned/route witnesses
reclaimable_bytes equals exact 64 KiB span multiples
normal malloc/free remains unchanged
```

## No-Go

```text
owner token becomes reclaim safety authority
in-page owner metadata is read after decommit
foreign cache scope is assumed complete
route-attached bytes are counted as fully reclaimable
L5-A performs detach, decommit, or depot insertion
```
