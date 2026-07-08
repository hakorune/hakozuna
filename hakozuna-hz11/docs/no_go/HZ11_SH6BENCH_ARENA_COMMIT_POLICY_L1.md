# HZ11Sh6benchArenaCommitPolicy-L1

Status: measured.
Verdict: NO-GO for sh6bench RSS/page-footprint fix.

This box tests whether the remaining sh6bench RSS gap under batch32 is caused by
arena page commit behavior that can be improved without changing transfer batch
width or reintroducing hot per-object span-return metadata accounting.

## Boundary

```text
Candidate base:
  libhz11_span_transfer_thread_exit_cap_batch32.so

Policy probe:
  HZ11_ARENA_NOHUGEPAGE=1

Candidate:
  libhz11_span_transfer_thread_exit_cap_batch32_nohuge.so

Diagnostic:
  libhz11_span_transfer_thread_exit_cap_batch32_nohuge_source_diag.so
```

The policy calls `madvise(..., MADV_NOHUGEPAGE)` on the 4 GiB arena VMA after
the existing `MAP_NORESERVE` mmap. No default lane changes are made.

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_arena_commit_policy.sh

Output:
  bench_results/hz11_sh6bench_arena_commit_20260708T065611Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | max RSS/tcmalloc | minor faults | span_create | carved MiB |
|---|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.365 | 1.000 | 261248 | 1.000 | 67270 | 0 | 0.0 |
| batch32 | 3.508 | 9.611 | 351104 | 1.344 | 88218 | 16753 | 1047.1 |
| batch32-source | 3.548 | 9.721 | 349440 | 1.338 | 88176 | 16728 | 1045.5 |
| batch32-nohuge | 3.522 | 9.649 | 351616 | 1.346 | 88168 | 16747 | 1046.7 |
| batch32-nohuge-source | 3.605 | 9.877 | 352384 | 1.349 | 88181 | 16765 | 1047.8 |

Source totals:

```text
batch32-source:
  span_create 16728
  arena_carve 16728
  span_reuse 89

batch32-nohuge-source:
  span_create 16765
  arena_carve 16765
  span_reuse 47
```

## Interpretation

`MADV_NOHUGEPAGE` does not move the measured RSS or page-fault profile:

```text
max RSS:
  batch32        351104 KiB
  batch32-nohuge 351616 KiB

minor faults:
  batch32        88218
  batch32-nohuge 88168
```

The lane still carves about 1.0 GiB of 64 KiB spans across RUNS=3 while only
about 350 MiB is resident. That means the RSS gap is not explained by the whole
carved arena being eagerly committed, nor by transparent huge-page behavior on
this run.

## Decision

NO-GO for `HZ11_ARENA_NOHUGEPAGE=1`.

Do not promote `libhz11_span_transfer_thread_exit_cap_batch32_nohuge.so`.

Next work should move away from simple arena VMA advice and toward one of:

```text
HZ11Sh6benchLiveObjectFootprintAttribution-L1
  compare live allocated payload/page demand against HZ11 span/page layout.

HZ11Sh6benchSpanSizePolicy-L1
  test whether the fixed 64 KiB span size creates excessive touched-page
  footprint for sh6bench's class distribution.
```
