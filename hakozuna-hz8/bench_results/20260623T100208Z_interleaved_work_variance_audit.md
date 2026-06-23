# InterleavedWorkVarianceAudit-L1

Date: 2026-06-23

## Change

Extended per-run `bench-release` output with:

```text
minor_faults
work_ops/s
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
bench_results/20260623T100208Z_work_variance_audit_r10.log
```

## Results

```text
throughput median = 56.76M ops/s
throughput p25    = 56.00M ops/s
throughput min    = 54.96M ops/s

steady median     = 58.76M ops/s
steady p25        = 58.08M ops/s

work median       = 27.334ms
tail median       = 14.421ms

minor faults:
  median = 5393
  min    = 4842
  max    = 5610
```

Stability ratios:

```text
throughput p25 / median = 0.987
throughput min / median = 0.968
steady p25 / median     = 0.988
```

## Per-Run Notes

Slowest end-to-end run:

```text
run 7:
  ops/s        = 54.96M
  work_ops/s   = 57.28M
  work_ms      = 27.931
  tail_ms      = 14.421
  minor_faults = 5413
```

Highest minor-fault run:

```text
run 1:
  ops/s        = 56.00M
  work_ops/s   = 58.53M
  minor_faults = 5610
```

Best run:

```text
run 8:
  ops/s        = 59.84M
  work_ops/s   = 62.36M
  minor_faults = 5413
```

Minor faults do not explain the work-rate variance in this batch.

## Decision

`InterleavedWorkVarianceAudit-L1` does not justify reopening remote protocol.

The current p2-v0 small lane remains above the v0 bring-up gate. Any next
performance work should be a local-leaf or code-shape box, not owner-lease or
remote-head redesign.
