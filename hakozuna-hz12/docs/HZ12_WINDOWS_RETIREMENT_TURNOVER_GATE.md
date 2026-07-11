# HZ12 Windows Repeated Retirement Turnover Gate

## Workload

The application-like diagnostic executes eight owner generations in one
process. Each generation owns 64 spans / 65,536 objects of 64 bytes:

```text
depot (except generation 1)
  -> worker allocation
  -> cross-thread free
  -> owner inbox drain and retirement
  -> P4 advisory decision
  -> P1 authority reclaim
  -> depot
```

The same 64 physical spans circulate through the depot after the first
generation. The owner registry and flush-owner route both reuse slot zero with
generation increments. The epoch layer now has an explicit fail-closed finish
transition so one completed retirement cannot authorize the next generation.

## Acceptance Gate

- generation advances exactly once per cycle;
- depot remains exactly 64 after every reclaim;
- stale owner, route mismatch, and limbo remain zero;
- post-reclaim RSS does not grow across generations;
- retirement p99 is below 10 ms.

## Windows Result

The mechanism passed safety and boundedness in the initial run and repeat-5:

- generation: 1 through 8 in every process;
- depot final: 64 in every process;
- limbo: zero;
- post RSS maximum: 6.23-6.26 MiB;
- peak RSS: 10.25-10.28 MiB.

The initial per-span implementation was borderline:

- p50: approximately 9.33-9.60 ms;
- p99: approximately 9.40-10.31 ms;
- maximum: approximately 9.44-10.55 ms;
- one of five repeat runs exceeded the strict 10 ms p99 gate.

`ReturnedBatchSnapshot-L1` preserves separate P4 advisory and P1 authority
passes, but validates up to 64 same-class spans per returned-sink traversal.
The authority pass detaches only snapshots proven complete while holding the
same class lock; incomplete spans remain unchanged. No malloc/free hot-path
work was added.

After this cold-path change, repeat-5 produced:

- p50: 0.628-0.835 ms;
- p99: 0.668-1.039 ms;
- maximum: 0.701-1.188 ms;
- post RSS maximum: 6.31 MiB;
- peak RSS: 10.33 MiB;
- depot final: 64 and limbo: zero in every process.

## Decision

`GO: bounded retirement behavior`.

The lifecycle, generation reuse, depot boundedness, RSS charter, and strict
retirement latency gate are proven for this Windows diagnostic. The batch
snapshot is a cold retirement mechanism, not a production hot-path diagnostic
and not a claim of universal allocator throughput. P4 advisory and P1 authority
remain deliberately separate so advisory evidence cannot become reclaim
authority.
