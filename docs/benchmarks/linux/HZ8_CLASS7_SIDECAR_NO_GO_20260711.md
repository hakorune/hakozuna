# HZ8 Class-7 Detached Sidecar Linux NO-GO (2026-07-11)

## Hypothesis

Global Mag32 helps larger-size churn but regresses 16..256. A previous
class-7-only Mag32 still stored the extra 16 pointers inside `H8ThreadCtx`.
This final capacity box kept the Mag16 context layout unchanged and placed the
class-7 overflow inventory in a separate TLS sidecar.

The box remained compile-time opt-in. It added no production counter or
atomic operation and reset the sidecar only at thread-context create/exit.

## Safety

GCC and Clang `-Werror` smoke and safety stress passed. Owner exit, handoff,
remote free, duplicate-claim, invalid-route, generation, and pending checks
matched Mag16.

## Local Results

Eight threads, 500,000 operations per thread, working set 1,024, alternating
fresh processes.

| Range | Runs | Mag16 ops/s | Sidecar ops/s | Delta | Mag16 peak | Sidecar peak |
|---|---:|---:|---:|---:|---:|---:|
| 16..256 | 20 | 356.19 M | 361.31 M | +1.44% | 3.62 MiB | 3.62 MiB |
| 16..2048 | 10 | 150.86 M | 273.10 M | +81.0% | 76.50 MiB | 17.88 MiB |
| 16..4096 | 10 | 10.99 M | 11.04 M | +0.38% | 1,962.88 MiB | 1,955.94 MiB |

The 16..256 paired result split 10 wins / 10 losses and is noise, not a speed
claim. The sidecar preserves the 16..2048 capacity benefit but does not retain
global Mag32's large 16..4096 improvement.

## Remote Results

Direct repeat-20 used identical work in all processes: 360,317 remote enqueues
and 39,683 local frees, or 90.07925% effective remote work.

| Metric | Mag16 | Sidecar |
|---|---:|---:|
| median ops/s | 31.85 M | 31.39 M (-1.43%) |
| paired wins | - | 7 / 20 |
| median peak RSS | 8.062 MiB | 8.250 MiB |
| median post RSS | 4.746 MiB | 4.736 MiB |

## Decision

```text
correctness: GO
16..2048 mechanism: GO
global Mag32 replacement: NO-GO
default promotion: NO-GO
experiment code retained: no
```

The physically detached sidecar removes the small-context-layout objection,
but it is too narrow to replace global Mag32 and weakens remote throughput.
Capacity tuning stops here for Linux. Keep public Mag16; retain global Mag32
as a Windows candidate/lane unless a new non-capacity mechanism appears.
