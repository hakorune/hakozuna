# HZ11 Linux Returned Refill Batch Probe L1

Status: CLOSED as Linux cross-check evidence. NO-GO for replacing the existing
Linux lanes.

## Question

Windows `HZ11WindowsClassAwareRefillBatch-L1` showed that batching returned
objects out of the per-class returned sink can improve specific Windows matrix
pressure rows.

Does the same non-transfer span shape matter on Ubuntu, enough to challenge the
existing Linux lane split?

## Boundary

This probe intentionally does not modify Linux `fine128`, `span-transfer`, or
the default path. It adds opt-in non-transfer span siblings only:

```text
libhz11_span_cache256.so
libhz11_span_cache512_classbatch16.so
libhz11_span_cache512_classbatch32.so
```

The classbatch siblings enable:

```text
HZ11_CLASSIFY_SPAN=1
HZ11_TLS_FASTPATH=1
HZ11_CACHE_BYTE_ACCOUNTING=0
HZ11_CACHE_SOA=1
HZ11_CACHE_CAP=512
HZ11_RETURNED_REFILL_BATCH=1
HZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4
HZ11_RETURNED_REFILL_BATCH_COUNT=16 or 32
```

They deliberately do not enable `HZ11_TRANSFER_CENTRAL_SPAN`, because the
Windows mechanism operates on the returned-object sink, not the Linux
transfer/central lane.

## Measurement

Source:

```text
bench_results/hz11_transfer_promotion_20260709T121034Z/summary.md
```

Command:

```text
RUNS=3 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
  --allocators tcmalloc,hz11-span-transfer,hz11-span-cache256,\
hz11-span-cache512-classbatch16,hz11-span-cache512-classbatch32
```

## Key Results

| Row | span-transfer ops/s | cache256 ops/s | classbatch16 ops/s | classbatch32 ops/s | Read |
| --- | ---: | ---: | ---: | ---: | --- |
| main_local0 | 432.642M | 405.299M | 401.477M | 403.191M | classbatch does not help local |
| main_r50 | 87.810M | 53.931M | 61.259M | 50.448M | classbatch16 helps cache256, still below transfer |
| main_r90 | 58.485M | 20.863M | 39.643M | 40.055M | classbatch helps cache256, still below transfer |
| small_remote90 | 66.131M | 18.609M | 16.743M | 20.693M | no useful Linux win |
| medium_r50 | 75.893M | 54.050M | 50.589M | 42.806M | worse than cache256 and transfer |
| medium_r90 | 55.838M | 20.451M | 29.356M | 32.785M | partial win vs cache256, still below transfer |

RSS is also unfavorable versus `span-transfer`:

| Row | cache256 RSS / transfer | classbatch16 RSS / transfer | classbatch32 RSS / transfer |
| --- | ---: | ---: | ---: |
| main_r50 | 2.981x | 4.077x | 4.556x |
| main_r90 | 2.955x | 4.321x | 4.638x |
| small_remote90 | 3.250x | 7.019x | 7.192x |
| medium_r50 | 2.727x | 3.595x | 4.448x |
| medium_r90 | 3.256x | 5.275x | 5.078x |

## Interpretation

The mechanism is live on Linux: classbatch improves some non-transfer span rows,
especially `main_r90` and `medium_r90` versus `cache256`.

But that is not the relevant Linux comparison. The existing Linux
`libhz11_span_transfer.so` lane already solves remote/mixed better: it is faster
on every tested remote row and uses far less RSS. The Windows returned-sink
pressure shape does not map to Linux's current best remote/mixed structure.

## Decision

```text
GO:
  Keep as cross-platform evidence.
  The Windows classbatch result is real, but platform/lane specific.

NO-GO:
  Do not replace Linux span-transfer.
  Do not promote Linux span-cache512-classbatch as a new lane.
  Do not port this idea into fine128 without a separate design; fine128 uses
  transfer/central, not the returned sink path.
```

## Current Lane Impact

```text
Linux fine128:
  unchanged, still the recommended general opt-in lane.

Linux span-transfer:
  unchanged, still the remote/mixed microbench lane.

Linux cap768/cap1024:
  unchanged, still sh6bench-synthetic specialists.

Windows classbatch:
  remains Windows profile-specific evidence; this Ubuntu probe does not weaken
  the Windows result, but it blocks a simple Linux promotion story.
```
