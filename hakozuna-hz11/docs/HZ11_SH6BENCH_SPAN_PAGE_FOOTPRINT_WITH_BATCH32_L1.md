# HZ11Sh6benchSpanPageFootprintWithBatch32-L1

## Status

```text
Verdict:
  GO as sh6bench RSS/page-footprint attribution.
  NO-GO as a macro promotion fix.

Decision:
  Remaining sh6bench RSS is dominated by created/committed span footprint.
  Central transient retention and current-span reserve are not the primary RSS
  explanations.
```

## Goal

Attribute the remaining sh6bench RSS/page footprint and residual wall under the
current best candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32.so
```

This box keeps `batch32` in place and adds a source/page diagnostic sibling.

## Compared Lanes

```text
tcmalloc
hz11-thread-exit-cap
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-source-diag
```

The diagnostic sibling uses:

```text
HZ11_TRANSFER_BATCH=32
HZ11_SPAN_SOURCE_DIAG=1
HZ11_CENTRAL_CLASS_DIAG=1
HZ11_CURRENT_SPAN_THREAD_EXIT=1
HZ11_CENTRAL_CAP=65536
```

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_span_page_footprint_batch32.sh

Output:
  bench_results/hz11_sh6bench_span_page_batch32_20260708T063053Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | current RSS KiB | central final | central high-water max | span_create |
|---|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.357 | 1.000 | 265728 | 261760 | 0 | 0 | 0 |
| hz11-thread-exit-cap | 4.494 | 12.588 | 351616 | 351232 | 464 | 3152 | 16681 |
| hz11-thread-exit-cap-batch32 | 3.500 | 9.804 | 352128 | 351872 | 64 | 3584 | 16765 |
| hz11-thread-exit-cap-batch32-source-diag | 3.542 | 9.922 | 351744 | 351488 | 752 | 3232 | 16745 |

## Source Attribution

For `hz11-thread-exit-cap-batch32-source-diag`, RUNS=3 totals:

| class | span_create | arena_carve | span_reuse | live_current_span | created_high_water |
|---:|---:|---:|---:|---:|---:|
| 0 | 4631 | 4631 | 28 | 48 | 1545 |
| 1 | 1763 | 1763 | 23 | 48 | 589 |
| 2 | 2779 | 2779 | 10 | 48 | 928 |
| 3 | 3100 | 3100 | 1 | 51 | 1035 |
| 4 | 1432 | 1432 | 1 | 48 | 478 |
| 5 | 1708 | 1708 | 4 | 48 | 571 |
| 6 | 1140 | 1140 | 3 | 48 | 381 |
| 9 | 192 | 192 | 0 | 48 | 64 |

Totals:

```text
span_create:       16745
arena_carve:       16745
span_reuse:           70
live_current_span:   387
```

`span_reuse / span_create` is about `0.4%`. The source counters therefore point
to fresh arena carve and missing span reuse as the page-footprint source.

## Non-Causes

Central transient retention is not the RSS driver:

```text
batch32 central final total:      64
batch32 central high-water max: 3584
```

The final count is tiny, and high-water is transient object count, not retained
span/page count.

Current-span reserve is also too small to explain the gap:

```text
live_current_span total across RUNS=3: 387
approximately per run:                129
```

Even treating each live current span as a full span, this is far smaller than
the total created span/page footprint and does not explain the persistent
`~1.32x` RSS ratio by itself.

## Decision

The next box should change or test span/page allocation and reuse policy, not
central drain/retention policy and not transfer capacity.

Candidate directions:

```text
HZ11Sh6benchSpanReusePolicy-L1
  add or test a reusable span/page source for sh6bench classes under batch32.

HZ11Sh6benchArenaCommitPolicy-L1
  test page commit/decommit or smaller span footprint policy if reuse alone
  cannot reduce RSS.
```

Do not promote batch32 to macro default from this result. The best active lane
still has sh6bench around `9.8x` tcmalloc wall and `1.32x` max RSS.
