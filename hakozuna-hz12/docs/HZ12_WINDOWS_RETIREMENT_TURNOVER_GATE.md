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

Retirement latency was borderline:

- p50: approximately 9.33-9.60 ms;
- p99: approximately 9.40-10.31 ms;
- maximum: approximately 9.44-10.55 ms;
- one of five repeat runs exceeded the strict 10 ms p99 gate.

## Decision

`HOLD: retirement latency gate`.

The lifecycle, generation reuse, depot boundedness, and RSS charter are proven,
but the retirement-only profile is not promoted while one repeat exceeds the
latency gate. Keep the behavior and turnover lanes as evidence. The next work,
if reopened, should reduce the repeated returned-sink scan or add a cold
owner-span index; do not weaken the 10 ms gate and do not move work into the
malloc/free hot path.
