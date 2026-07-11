# HZ12 Windows Reclaim P4: Retirement-Only Behavior

## Contract

This diagnostic sibling connects the P4 advisory decision to the existing P1
authority path:

```text
P4 advisory scan
  -> require would_reclaim
  -> P1 independently repeats the authoritative complete-slot snapshot
  -> reserve depot capacity
  -> ordered detach/decommit/owner-clear/depot commit
```

The scans are intentionally not fused. P4 decides whether the work is worth
attempting; P1 independently proves whether each span may be mutated.

## Windows Result

Repeat-10 completed with:

- policy authorized: 10/10;
- reservation: 64/64 spans;
- detach and decommit: 64/64 spans;
- depot commit: 64/64 spans;
- retired owner clear: 64/64 spans;
- limbo spans: zero;
- payload decommitted: 4 MiB;
- observed working-set reduction: approximately 3.99 MiB.

The concurrent same-class lock probe retained p99 near 0.1 microseconds. An
unlucky maximum waiter remained on the millisecond scale, matching the policy
scan duration. This is why the behavior remains owner-retirement-only.

## Status

Keep as opt-in behavior evidence. Do not promote to the normal speed/broad
lane yet. The next promotion gate needs an application-like thread-retirement
workload that measures total retirement latency and confirms that repeated
owner turnover does not accumulate depot entries or stale generations.
