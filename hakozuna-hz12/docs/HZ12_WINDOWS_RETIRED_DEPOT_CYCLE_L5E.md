# HZ12 Windows Retired Depot Cycle L5-E

## Goal

L5-E verifies that the fixed L5-D decommit set can enter the existing bounded
64-span depot and return at the same addresses through recommit-before-route.
It is a diagnostic lifecycle box, not an automatic allocator policy.

## Fixed Order

```text
L5-C ordered detach, route last
-> L5-D Windows decommit
-> depot put, maximum 64 spans
-> depot take by original class
-> recommit at the original address
-> accounting reset
-> route attach
-> diagnostic current-span install
-> explicit checkpoint clears that current reference
```

The cycle records every inserted address and requires every returned address
to belong to that exact set. The generation-aware owner side table must still
attribute each span to the retiring token.

## Boundaries

```text
diagnostic-only
fixed budget <= 64
no normal malloc/free connection
no automatic depot admission
no production counter or hot-path owner lookup
no claim that recommit restores resident RSS before payload touch
```

## Acceptance

For `detach=64`, `decommit=64`, and `depot=64`:

- all 64 depot puts succeed;
- all 64 takes recommit, reset accounting, attach the route, and install the
  current span;
- returned addresses exactly match the inserted set;
- owner generation remains exact;
- the depot ends empty;
- failures and mismatches remain zero.

The explicit checkpoint after each take exists only because the existing depot
API installs one current span per class. It permits repeated same-class takes
without changing depot or allocator policy.

## No-Go

- a stale or foreign-generation span enters the cycle;
- an address outside the inserted set is returned;
- route/accounting/current restoration is partial;
- the depot exceeds 64 entries or remains non-empty;
- L5-E requires an automatic production policy or a malloc/free hot-path tax.

## Windows Result

The fixed wide_ws-like lane passed repeat-3 with:

```text
threads=8
iters=200000/thread
working_set=16384/thread
detach/decommit/depot budget=64/64/64

detach:       64/64, 4.00 MiB
decommit:     64/64, 4.00 MiB
depot put:    64/64
depot take:   64/64
recommit:     4.00 MiB
address miss: 0
owner miss:   0
remaining:    0
```

The L5-D observation point remained before depot recommit. Working-set RSS
fell by 3.99-4.00 MiB in each run. L5-E does not claim that recommit alone
restores resident RSS because the recommitted payload was not faulted in.

This is GO as bounded lifecycle evidence. It is not a default reclaim or depot
policy. The next independent gate is bounded post-recommit allocation followed
by a second detach/decommit cycle.
