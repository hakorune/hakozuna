# HZ12 Windows Class-Flush Checkpoint L5-B

## Purpose

L5-B turns the foreign-cache scope witness from unknown to explicit in the
wide_ws-like diagnostic. Every enrolled worker flushes all local class caches,
clears advisory current-span references, acknowledges the retirement epoch,
and only then exits.

## Boundary

The checkpoint is an explicit diagnostic call. It is not attached to normal
malloc/free, a Windows thread destructor, or a production teardown path. A
partial current span may be abandoned as a non-candidate; L5-B performs no
span detach, route change, decommit, or depot insertion.

## Result

Repeat-5 completed with zero foreign-scope, current-span, and front-cache
blockers. Physical full-span candidates were 80.19..81.25 MiB, median
80.75 MiB, against 89.70 MiB median peak RSS. Every owner span remained blocked
by the returned sink and route, so fully reclaimable bytes correctly stayed
zero. The result moves the next pressure point to bounded returned-sink detach.

## No-Go

```text
assume thread scope complete without every enrolled checkpoint
run the checkpoint from normal allocation/free
detach or decommit from L5-B
count partial current spans as full-span candidates
```
