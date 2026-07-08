# HZ11Sh6benchSpanReusePolicy-L1

Status: measured.
Verdict: NO-GO for sh6bench RSS/page-footprint fix.

This box tests a narrow span reuse policy under the current best candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32.so
```

The lane adds `HZ11_CENTRAL_SPAN_REUSE=1`, which tracks only objects that spill
to the central stack. When all objects for one span are present in central, the
span is removed from central and made reusable before the next fresh arena
carve.

This intentionally avoids the prior span-return lane's transfer-cache
per-object metadata accounting and metadata lock traffic.

## Boundary

```text
Candidate:
  libhz11_span_transfer_thread_exit_cap_batch32_span_reuse.so

Diagnostic:
  libhz11_span_transfer_thread_exit_cap_batch32_span_reuse_source_diag.so

Touched:
  span/page reuse policy before arena carve

Not touched:
  transfer batch width
  transfer capacity
  central drain policy as the primary lever
  default lane
```

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_span_page_footprint_batch32.sh

Output:
  bench_results/hz11_sh6bench_span_page_batch32_20260708T063931Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | max RSS/tcmalloc | central insert | central high-water max | span_create |
|---|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.353 | 1.000 | 263424 | 1.000 | 0 | 0 | 0 |
| batch32 | 3.497 | 9.907 | 351488 | 1.334 | 26640144 | 3616 | 16769 |
| batch32-source | 3.597 | 10.190 | 350464 | 1.330 | 27374843 | 3328 | 16755 |
| batch32-span-reuse | 3.568 | 10.108 | 350848 | 1.332 | 23035817 | 3104 | 16753 |
| batch32-span-reuse-source | 3.525 | 9.986 | 351872 | 1.336 | 23200343 | 3072 | 16753 |

## Source Counters

RUNS=3 totals:

| Lane | span_create | arena_carve | span_reuse | reuse rate |
|---|---:|---:|---:|---:|
| batch32-source | 16755 | 16755 | 44 | 0.263% |
| batch32-span-reuse-source | 16753 | 16753 | 66 | 0.394% |

The policy slightly increases counted reuse and reduces central insert traffic,
but it does not materially reduce `span_create` or `arena_carve`.

## Interpretation

The tested policy only captures spans that become fully represented in the
central stack. On sh6bench, that condition is too rare to move RSS:

```text
span_create:
  batch32-source             16755
  batch32-span-reuse-source  16753

arena_carve:
  batch32-source             16755
  batch32-span-reuse-source  16753
```

The result confirms that central-only full-span reuse is not enough. The
remaining page footprint likely requires a policy that sees more than central
spill state, or a page/arena commit policy change.

## Decision

NO-GO for this span reuse policy.

Do not promote `libhz11_span_transfer_thread_exit_cap_batch32_span_reuse.so`.

Next work should move to one of:

```text
HZ11Sh6benchArenaCommitPolicy-L1
  reduce committed page footprint without relying on full central-span reuse.

HZ11Sh6benchSpanLifecycleAccounting-L1
  design a lower-overhead span lifecycle signal that avoids the prior
  span-return metadata-lock regression but can observe transfer-held objects.
```
