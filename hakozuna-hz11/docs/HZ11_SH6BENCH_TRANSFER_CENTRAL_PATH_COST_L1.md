# HZ11Sh6benchTransferCentralPathCost-L1

## Status

```text
Verdict:
  GO as a transfer/central path-cost attribution probe.
  NO-GO as a macro promotion fix.

Candidate:
  libhz11_span_transfer_thread_exit_cap_xferwide.so

Boundary:
  Sibling lane only.
  No default path change.
  No span-return metadata-lock change.
  No central cap overflow or final retention tuning.
```

## Goal

Reduce or prove the sh6bench cost source under the active
`hz11-thread-exit-cap` candidate by changing only transfer-cache spill policy
and batching.

This box tests whether transfer/central trip volume is a wall-time lever before
moving to broader allocator policy changes.

## Candidate Lane

```text
Base:
  -DHZ11_CLASSIFY_SPAN=1
  -DHZ11_TLS_FASTPATH=1
  -DHZ11_CACHE_BYTE_ACCOUNTING=0
  -DHZ11_CACHE_SOA=1
  -DHZ11_TRANSFER_CENTRAL_SPAN=1
  -DHZ11_CURRENT_SPAN_THREAD_EXIT=1
  -DHZ11_CENTRAL_CLASS_DIAG=1
  -DHZ11_CENTRAL_CAP=65536

Probe:
  -DHZ11_TRANSFER_CAP=8192
  -DHZ11_TRANSFER_BATCH=32
```

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_path_cost_attribution.sh

Output:
  bench_results/hz11_sh6bench_path_cost_20260708T043634Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | RSS/tcmalloc | xfer_hit | xfer_miss | xfer_insert | xfer_spill | central_hit | central_miss | central_insert | span_create |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.362 | 1.000 | 267136 | 1.000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| hz11-span-transfer | 4.532 | 12.519 | 351872 | 1.317 | 31362786 | 3335169 | 501811424 | 25773856 | 1610863 | 1724306 | 25773856 | 16722 |
| hz11-thread-exit-cap | 4.542 | 12.547 | 352256 | 1.319 | 31382475 | 3315573 | 502129508 | 25462402 | 1591378 | 1724195 | 25462402 | 16683 |
| hz11-thread-exit-cap-xferwide | 3.627 | 10.019 | 352128 | 1.318 | 17121178 | 862087 | 547886732 | 0 | 0 | 862087 | 0 | 16767 |
| hz11-span-return-source-diag | 18.980 | 52.431 | 352640 | 1.320 | 31194200 | 3503755 | 499113904 | 28471376 | 1779447 | 1724308 | 28471376 | 16722 |

## Interpretation

The xferwide sibling eliminates central spill and central insert traffic for
this sh6bench run:

```text
hz11-thread-exit-cap:
  xfer_spill      25462402
  central_insert  25462402

hz11-thread-exit-cap-xferwide:
  xfer_spill      0
  central_insert  0
```

It also reduces transfer trip counters materially:

```text
xfer_hit:
  31382475 -> 17121178

xfer_miss:
  3315573 -> 862087
```

Wall time improves from `4.542s` to `3.627s`, roughly a 20% candidate-local
improvement.

This is not enough for macro promotion. The probe remains `10.019x` tcmalloc
wall, and RSS is unchanged at about `1.318x` tcmalloc. Span creation also stays
flat at about 16.7K spans, so this box does not explain or fix the RSS/page
footprint blocker.

## Decision

Option A is confirmed as a real wall-time lever: reducing transfer/central trips
and eliminating spill improves sh6bench materially.

It is not sufficient by itself. The remaining blocker is still large enough that
the next box should either isolate the batch-vs-cap contribution or move to the
span/page footprint side after the transfer trip contribution is bounded.

Do not promote `libhz11_span_transfer_thread_exit_cap_xferwide.so` to default or
macro recommendation from this result.
