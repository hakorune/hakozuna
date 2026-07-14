# HZ8 Small Transition Inventory native Ubuntu x86_64 gate, 2026-07-14

## Decision

`SmallTransitionInventory-L1` passes native Ubuntu correctness, throughput,
peak-RSS, remote90, and Redis application controls. It reproduces the large
LCG recovery without the xorshift throughput regression of the earlier depot.

Shared-default promotion remains `HOLD`. The strict post-RSS guard is exceeded
on the synthetic wide row, and the Windows MT remote result is still outside
its throughput and RSS guards.

Follow-up attribution found that the native post delta comes from free pages
retained by glibc per-thread arenas after worker join. It is not live HZ8 arena
payload: default/inventory `H8Span` size is `192/192`, and the candidate's
extra `H8ThreadCtx` storage is only 112 bytes per worker. At the same wide
checkpoint, default/inventory Anonymous Private Dirty was `1276/1940 KiB`;
the eight glibc arenas explained the difference. A control-plane
`malloc_trim(0)` changed candidate RSS `3704 -> 2396 KiB`.

`PostRssQuiescentTrimControl-L0` will preserve raw RSS and add settled,
trim-control, and `smaps_rollup` values in separate benchmark binaries. It
does not add trimming to the allocator or normal benchmark lane.

The native Ubuntu ABBA BLOCKS=10 trim-control gate completed. Raw RSS retains
the earlier wide delta, while the control-plane trim removes the anonymous
private dirty pages:

| Row | Throughput delta | Peak RSS delta | Raw post delta | Trim post delta | Trim gate |
|---|---:|---:|---:|---:|---|
| xorshift wide | -0.10% | -1.01% | +7.49% | -34.17% | PASS |
| LCG wide | +201.90% | -78.12% | +42.53% | -3.84% | PASS |

The remaining raw RSS is therefore a libc arena-retention measurement signal,
not live HZ8 span residency. Raw post RSS remains reported for transparency;
the normalized trim-control result is the admissible residency comparison.

```text
native Ubuntu behavior: GO
native Ubuntu xorshift throughput: GO
native Ubuntu LCG recovery: GO
native Ubuntu remote90: GO
native Ubuntu Redis control: GO
native Ubuntu strict post-RSS gate: HOLD
shared default: HOLD
public default: unchanged
```

This is a cross-platform research candidate, not a Linux-only regression or a
Windows-only lane. The remaining work is gate closure, not another inventory
or capacity variant.

## Environment

```text
behavior commit: 509f9cb4
host: native Ubuntu 22.04, Linux 6.8.0-90-generic, x86_64
CPU: AMD Ryzen 7 5825U, 8 cores / 16 logical CPUs
GCC: 11.4.0
Clang: 14.0.0
local rows: fresh-process AB/BA R10, WorkScale=10
remote row: fresh-process AB/BA R10, 16 threads, 100K iterations/thread,
            16..4096 bytes, interleaved target remote=90%
Redis: fresh-process AB/BA R10, Redis + memtier, 5 seconds/run,
       SET:GET=1:10, 4 threads, 50 clients, pipeline=8, 128-byte values
diagnostic counters: absent from every speed binary
```

Baseline is the shared HZ8 default. Candidate adds only
`H8_SMALL_TRANSITION_INVENTORY_L1` and replaces the Mag16 behavior inside that
research sibling.

## Xorshift R10

| Row | Default | Inventory | Delta | Default post | Inventory post | Default peak | Inventory peak |
|---|---:|---:|---:|---:|---:|---:|---:|
| fixed8K | 126.890M | 124.659M | -1.76% | 2.33 MiB | 2.36 MiB | 4.00 MiB | 4.00 MiB |
| fixed16K | 124.798M | 122.523M | -1.82% | 2.12 MiB | 2.00 MiB | 4.00 MiB | 3.88 MiB |
| fixed32K | 95.538M | 97.633M | +2.19% | 2.24 MiB | 2.18 MiB | 4.12 MiB | 4.06 MiB |
| balanced | 153.631M | 153.671M | +0.03% | 2.42 MiB | 2.38 MiB | 26.75 MiB | 26.25 MiB |
| wide_ws | 91.794M | 93.772M | +2.15% | 3.12 MiB | 3.42 MiB | 49.62 MiB | 49.00 MiB |
| larger_sizes | 17.961M | 17.956M | -0.03% | 2.53 MiB | 2.62 MiB | 33.12 MiB | 32.94 MiB |
| remote_small | 52.816M | 52.134M | -1.29% | 13.49 MiB | 13.48 MiB | 22.44 MiB | 20.94 MiB |

Every throughput row clears the `-3%` guard. Peak RSS is neutral or lower.
The wide post-RSS median rises by about 0.30 MiB (`+9.6%`), which is small in
absolute terms but exceeds the stated `+5%` promotion guard. Larger post RSS
rises about 0.09 MiB (`+4.2%`).

The remote generator performed exactly the same work in both lanes:

```text
remote_enqueue=1,440,700
local_free=159,300
effective remote=90.04%
```

## Windows-LCG Parity R10

| Row | Default | Inventory | Delta | Default post | Inventory post | Default peak | Inventory peak |
|---|---:|---:|---:|---:|---:|---:|---:|
| fixed8K | 162.584M | 168.111M | +3.40% | 2.37 MiB | 2.36 MiB | 5.50 MiB | 5.56 MiB |
| fixed16K | 138.846M | 143.777M | +3.55% | 2.17 MiB | 2.12 MiB | 5.50 MiB | 5.50 MiB |
| fixed32K | 24.644M | 25.978M | +5.41% | 2.62 MiB | 2.55 MiB | 6.00 MiB | 6.00 MiB |
| balanced | 17.979M | 154.904M | +761.60% | 3.49 MiB | 2.71 MiB | 770.50 MiB | 48.38 MiB |
| wide_ws | 25.282M | 74.349M | +194.07% | 2.78 MiB | 3.84 MiB | 422.38 MiB | 92.25 MiB |
| larger_sizes | 7.748M | 16.680M | +115.29% | 3.82 MiB | 2.91 MiB | 277.00 MiB | 60.00 MiB |
| remote_small | 52.280M | 52.520M | +0.46% | 13.49 MiB | 13.50 MiB | 22.06 MiB | 21.44 MiB |

The six local trace hashes matched between allocators and matched the
standalone Windows-LCG oracle. This confirms identical allocation/free/index
events rather than merely similar distributions.

LCG wide also has a small absolute but large relative post-RSS rise. Its peak
RSS nevertheless falls by about 330 MiB, and the balanced/larger post medians
fall.

## Redis R10

| Allocator | Median ops/s | Post RSS | Peak RSS |
|---|---:|---:|---:|
| Default | 0.576M | 13.12 MiB | 13.12 MiB |
| Inventory | 0.571M | 13.06 MiB | 13.06 MiB |

```text
throughput: -0.91%
post RSS:   -0.48%
peak RSS:   -0.48%
```

The application control clears both the `-3%` throughput guard and `+5%` RSS
guard.

## Correctness And Diagnostics

GCC and Clang `-Werror` builds passed for default and inventory preload,
smoke, safety, speed, and diagnostic targets. Safety retained owner exit,
handoff, 8,192 remote frees, duplicate claim, and invalid-free fail-closed
coverage with zero allocation failure.

`nm` found no inventory diagnostic globals in the speed binary; they appear
only in the explicit diagnostic sibling. Focused diagnostics reported:

| Row | Candidate | Push | Duplicate | Pop hit | Pop reject | Remote publish | Direct activate | Depth max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| LCG balanced | 20,769 | 20,743 | 0 | 20,040 | 0 | 0 | 26 | 67 |
| xorshift balanced | 451,763 | 277,967 | 0 | 277,607 | 0 | 0 | 173,796 | 35 |
| remote90 | 42,041 | 9,857 | 0 | 9,845 | 0 | 9,857 | 32,184 | 7 |

Remote publication occurs only after normal owner collection. The remote
producer does not mutate owner-local inventory. Owner exit resets the
inventory before handoff.

## Reproduction

```sh
RUNS=10 WORK_SCALE=10 REMOTE_WORK_SCALE=1 TRACE_PARITY=0 \
  make -C hakozuna-hz8 small-transition-inventory-gate

RUNS=10 WORK_SCALE=10 REMOTE_WORK_SCALE=1 TRACE_PARITY=1 \
  make -C hakozuna-hz8 small-transition-inventory-gate

RUNS=10 DURATION=5 \
  make -C hakozuna-hz8 small-transition-inventory-redis-gate
```

The runner records raw per-run CSV, manifests, logs, RSS, summaries, trace
hashes, and exact remote work counts under `hakozuna-hz8/bench_results/`.
