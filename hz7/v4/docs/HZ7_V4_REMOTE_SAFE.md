# HZ7 V4 Remote-Safe Contract

HZ7 v4 is the smallest HZ7 fork that is allowed to say "cross-thread free is
safe" without claiming remote throughput leadership.

## Core Rule

```text
If a pointer was allocated by HZ7 v4, any thread may free it.

The free path must remain route-first and fail-closed.
Foreign pointers must not be dereferenced before route lookup.
```

## Allowed

```text
coarse global lock
global route table
bounded retained direct buckets
slow-path work outside the lock when route state is already safe
remote-safe smoke and control benches
```

## Not Allowed

```text
owner inboxes
TLS ownership
lock-free remote queues
remote batching
thread-exit drain choreography
remote-fast claims
```

## Evidence Targets

```text
remote smoke passes
cross-thread free passes
route INVALID remains fail-closed
retained direct pointers are not misrouted as VALID
RSS stays bounded
```
