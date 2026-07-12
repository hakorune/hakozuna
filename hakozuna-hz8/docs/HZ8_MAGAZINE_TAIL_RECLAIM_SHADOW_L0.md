# HZ8 MagazineTailReclaim Shadow L0

## Question

Does the owner-local Mag16 contain a material set of completely empty spans at
the moment HZ8 is about to commit another small span?

The existing reclaim shadow already proves a large owner-exit upper bound. Its
removed behavior sibling also proved that bounded owner-list scans do not
materially improve peak RSS. L0 must not repeat either experiment.

## Observation Point

The shadow runs only immediately before `h8_span_commit_for_class()`. It scans
the current thread's bounded magazine, never an owner list and never another
thread's cache. Feature-off builds compile the call to a no-op.

```text
inventory spans / bytes
completely empty spans / bytes
oldest-half (tail) spans
oldest-half completely empty spans
hypothetical decommit bytes
blocked by live / pending / state
class inventory and empty bytes
maximum current inventory / empty / hypothetical bytes
maximum current empty bytes by class
```

Array order is stack order. The lower half is called the tail/cold proxy because
the stack pops from its upper end. This is not a last-use timestamp; adding a
per-operation epoch write is explicitly forbidden.

## Gate

```text
open behavior design only if:
  hypothetical decommit bytes >= 8MiB at refill checkpoints
  tail_empty / tail_spans is material and repeatable
  one or more classes dominate clearly

close if:
  blocked_live dominates
  hypothetical bytes are small
  signal only appears at owner exit
```

Any future behavior must act at an existing slow-path checkpoint. Do not add a
hot-path atomic, continuous owner scan, cap ladder, or HZ12 core dependency.

## Windows Result

One diagnostic run per broad row produced:

| Row | Checkpoints | Max inventory | Max empty | Max hypothetical tail decommit |
|---|---:|---:|---:|---:|
| balanced | 12,552 | 2.81MiB | 2.31MiB | 1.19MiB |
| wide_ws | 6,735 | 2.25MiB | 1.88MiB | 1.00MiB |
| larger_sizes | 3,887 | 2.75MiB | 2.13MiB | 1.13MiB |

Pending and state blockers were zero. Live objects explain the remaining
inventory. Although cumulative hypothetical traffic is large, the maximum
simultaneously reclaimable tail is only about 1.0-1.2MiB.

## Decision

```text
MagazineTailReclaim behavior: NO-GO / CLOSED
shadow lane: retain as diagnostic evidence
public default: unchanged
```

The signal misses the 8MiB admission threshold by a wide margin. A decommit
behavior would add lifecycle and refault cost to recover too little peak RSS.
Do not try a tail fraction, age epoch, Mag cap, or class mask ladder.
