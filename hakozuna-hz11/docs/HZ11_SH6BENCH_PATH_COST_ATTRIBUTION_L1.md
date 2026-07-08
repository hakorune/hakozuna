# HZ11Sh6benchPathCostAttribution-L1

Status: GO for attribution; no optimization or macro promotion.

This box re-attributes the remaining sh6bench macro blocker after
`HZ11CentralPolicyCorrectness-L1`. It checks the active candidate
`libhz11_span_transfer_thread_exit_cap.so` before choosing a metadata-lock or
transfer/central path-cost optimization.

## Boundary

```text
Candidate focus:
  libhz11_span_transfer_thread_exit_cap.so

Diagnostic sibling:
  libhz11_span_transfer_thread_exit_cap_source_diag.so

Runner:
  scripts/run_hz11_sh6bench_path_cost_attribution.sh

Output:
  bench_results/hz11_sh6bench_path_cost_20260708T021718Z/summary.md
```

The diagnostic sibling adds source counters only. No default lane changes are
made.

## Samples

RUNS=3, sh6bench default macro-gate input.

| Allocator | Status | Median wall sec | Wall/tcmalloc | Median max RSS KiB | Max RSS/tcmalloc |
|---|---:|---:|---:|---:|---:|
| tcmalloc | OK:3 | 0.359 | 1.000 | 265344 | 1.000 |
| hz11-span-transfer | OK:3 | 4.546 | 12.663 | 351360 | 1.324 |
| hz11-thread-exit-cap | OK:3 | 4.552 | 12.680 | 351872 | 1.326 |
| hz11-thread-exit-cap-source-diag | OK:3 | 4.648 | 12.947 | 351232 | 1.324 |
| hz11-span-return-source-diag | OK:3 | 18.913 | 52.682 | 352768 | 1.329 |

## Attribution

### Candidate Wall Cost

`hz11-thread-exit-cap` is effectively the same shape as `hz11-span-transfer` on
sh6bench:

```text
span-transfer wall:     4.546s
thread-exit-cap wall:   4.552s
source-diag wall:       4.648s
span-return-source wall: 18.913s
```

The cap and thread-exit fixes do not touch sh6bench's main cost. Source
diagnostics add only a small overhead. Span-return remains much slower because
it adds per-object metadata accounting.

### Transfer/Central Volume

`hz11-thread-exit-cap` per RUNS=3 total:

```text
xfer_hit:       31,375,684
xfer_miss:       3,322,507
xfer_insert:   502,021,151
xfer_spill:     25,573,012
central_hit:     1,598,325
central_miss:    1,724,182
central_insert: 25,573,012
span_create:       16,680
```

This is the active candidate's path-cost signature: very high transfer insert
volume, millions of transfer misses, and tens of millions of central spill
objects. The central stack drains by exit and does not overflow.

Central class stats for `hz11-thread-exit-cap` show no final retention:

```text
class 0 count=0 high_water=3072..3184
class 1 count=0 high_water=1200..1376
class 2 count=0 high_water=512..656
class 3 count=0 high_water=144..160
```

So sh6bench is not blocked by fixed central cap pressure or final central
retention.

### Span Source / RSS

The source diagnostic shows fresh span carve remains material:

```text
hz11-thread-exit-cap-source-diag:
  span_create total: 16670
  arena_carve total: 16670
  span_reuse total: 51
```

But live current spans are small and bounded:

```text
live_current_span is about 48 per active class over RUNS=3
```

Therefore the sh6bench RSS gap is not the larson-style live-current-span leak
and not central retained objects. It is mostly the footprint of created/committed
spans that are not returned to the OS or reused aggressively enough during the
workload.

### Metadata Lock Hypothesis

The earlier metadata-lock result is confirmed only for span-return's added
regression:

```text
hz11-thread-exit-cap-source-diag meta_lock: 0
hz11-span-return-source-diag meta_lock: very high
  class 0: 762,325,104
  class 1: 137,411,808
  class 2: 109,063,200
  class 3:  58,090,192
```

Since the active candidate does not use span-return metadata locking and still
runs about 12.7x slower than tcmalloc, metadata-lock batching alone would not
fix the current macro blocker. It would only make the span-return attribution
lane less bad.

## Decision

Do not jump directly to `HZ11SpanReturnMetadataBatch-L1` as the next macro
candidate fix.

The active sh6bench blocker is:

```text
primary wall lever:
  transfer/central path volume and batch/overflow behavior

primary RSS lever:
  created span/page footprint; not central final retention and not live current
  spans

span-return metadata lock:
  confirmed as span-return-specific added regression, not the active
  thread-exit-cap blocker
```

Next box should be a narrower sh6bench path-cost box for transfer/central volume
or span footprint policy, while keeping span-return metadata batching as a
separate cleanup if that lane is retried.
