# InterleavedTailVarianceAudit-L1

Date: 2026-06-23
Commit under test: 3326d39 plus bench-only per-run attribution

## Change

Added per-run interleaved attribution to `bench-release` output:

```text
run_interleaved=N
work_ms
tail_ms
remote_enqueue
local_free
drain_calls
drain_objects
drain_empty
push_yields
finish_yields
```

This is measurement-only. Allocator behavior is unchanged.

## Command

```sh
make bench-release
./h8_bench_release --runs 10 --threads 16 --iters 100000 \
  --min-size 16 --max-size 2048 --remote-pct 90 \
  --interleaved 1 --live-window 4096
```

Raw log:

```text
bench_results/20260623T095547Z_tail_variance_audit_r10.log
```

## Results

```text
throughput median = 56.78M ops/s
throughput p25    = 48.71M ops/s
throughput min    = 44.89M ops/s

steady median     = 59.51M ops/s
steady p25        = 51.22M ops/s

work median       = 27.345ms
tail median       = 13.870ms

remote_enqueue    = 14,400,461
local_free        = 1,599,539
drain_calls       = 16,098,607
drain_objects     = 14,400,461
drain_empty       = 15,994,858
push_yields       = 0
finish_yields     = 92,508
```

Stability ratios:

```text
throughput p25 / median = 0.858
throughput min / median = 0.790
steady p25 / median     = 0.861
```

The current run clears the bring-up stability shape:

```text
p25 >= median * 0.85
min >= median * 0.60
```

## Interpretation

The weakest run was run 5:

```text
ops/s   = 44.89M
work_ms = 33.199
tail_ms = 15.366
```

The low run correlates primarily with work phase expansion, not a tail-drain
explosion. Tail variation exists, but does not explain the p25 dip alone.

The remote handoff counters are clean:

```text
push_yields = 0
quiescent_pending bitmap_nonzero = 0
quiescent_pending repair = 0
queue_contention duplicate_claim = 0
slot_shadow used_mismatch = 0
```

## Decision

Do not reopen owner leases or `IntrusiveRemoteHead-L1` from this evidence.

`InterleavedTailVarianceAudit-L1` shows that the current remote protocol is not
blocked by tail-drain instability in this run. If further variance work is
needed, the next target is work-phase attribution, not remote queue redesign.
