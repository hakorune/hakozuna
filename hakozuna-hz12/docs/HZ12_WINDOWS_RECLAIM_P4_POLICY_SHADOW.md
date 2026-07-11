# HZ12 Windows Reclaim P4: Checkpoint Policy Shadow

## Purpose

P4 asks whether the bounded P1-P3 lifecycle has enough reclaimable payload to
justify invocation. It is observation only: no span is detached, decommitted,
or inserted into the depot.

The fixed initial policy is:

```text
owner retirement gate is open
and real flush-owner inbox is empty
and snapshot-complete payload >= 4 MiB
then plan at most 64 spans / 4 MiB
```

The threshold and cap are constants, not runtime-tuned knobs. Candidate
enumeration occurs only in the diagnostic retirement-checkpoint sibling.

## Boundary Controls

Two Windows executables use the same fixture and policy code:

- `bench_hz12_widews_reclaim_policy_shadow.exe`: 64 spans, exactly 4 MiB.
- `bench_hz12_widews_reclaim_policy_below.exe`: 63 spans, 4,128,768 bytes.

Both query once before owner retirement and once after epoch acknowledgement,
token-inbox drain, real flush-owner inbox drain, and owner-cache flush.

## Result

Repeat-10 was stable:

| Control | Before retirement | After retirement | Complete spans | Planned bytes |
|---|---:|---:|---:|---:|
| 64-span threshold | no | yes | 64 | 4,194,304 |
| 63-span below | no | no | 63 | 4,128,768 |

The mutation-free owner-ledger shadow continued to report zero detach,
decommit, recycle, and depot behavior.

QPC timing for the cold full query was approximately 3.3-4.2 ms on this
Windows fixture. A concurrent same-class sink-lock probe measured p99 at
0.1-0.2 microseconds, but an unlucky maximum waiter stalled for approximately
3.45-3.83 ms. That is acceptable evidence for owner retirement, but too
expensive to place at frequent allocation checkpoints without a cheaper owner
span index. P4 therefore does not authorize periodic pressure polling.

## Decision

Keep P4 as a retirement-only policy candidate. The next behavior experiment
may invoke the already-proven bounded P1 reclaim only when P4 returns
`would_reclaim`, in a separate executable. Do not add counters, scans, or
atomics to normal malloc/free/refill paths.
