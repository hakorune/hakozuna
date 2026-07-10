# HZ12 Windows Reclaim P3: Core-Only Recycle

## Decision

P3 closes one bounded retirement lifecycle without continuous atomic
accounting:

```text
retire gate
  -> complete returned-sink snapshot
  -> detach route and decommit
  -> bounded depot
  -> recommit on a later take
  -> attach route
  -> rehome advisory ownership
  -> install as the new thread's current span
  -> allocate and free at the same address
```

The allocation/free hot path is unchanged. P3 remains a diagnostic behavior
sibling and does not enable automatic pressure reclaim.

## Ownership Correction

The earlier `assign_span` path used first-owner CAS semantics. A depot span
still carrying a retired token therefore could not be proven to belong to its
new consumer. P3 adds explicit cold rehome operations for both route-shadow
and span-owner side tables. Rehome is advisory locality metadata, never the
route-safety authority.

Rollback clears only the token installed by the current attempt. It then
detaches the route, decommits the payload, and returns the span to the same
bounded depot.

## Windows Result

Success lane, repeat-10:

- reclaim/depot: 64/64 spans
- one span recommitted from the depot
- route attached: yes
- new owner recorded in both side tables: yes
- installed as current: yes
- first allocation reused the exact span base: yes
- rollback: zero
- depot remaining: 63

Forced rollback lane, repeat-10:

- current span deliberately occupied before take
- recycle correctly rejected
- rollback: one
- memory state restored to `MEM_RESERVE`
- route detached
- route-shadow and span-owner tokens cleared
- depot restored to 64 entries

## Close Point

P0-D through P3 now prove a complete bounded retire/reclaim/recycle lifecycle.
The next phase is not more lifecycle mechanics. It is a separately measured
policy experiment that decides when a production checkpoint may invoke this
path. Until that gate exists, automatic reclaim remains disabled.
