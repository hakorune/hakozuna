# HZ6 SourceBlockRoute Dynmap Selected-Small Snapshot

This directory is a paper-facing curated snapshot for the 2026-06-06 HZ6
selected-small lane update. It preserves the evidence used to promote
`sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k`
as the selected-small candidate row.

Raw exploratory `results/` directories are intentionally not committed here.
This folder keeps only the repeat matrix and the selected-family wiring smoke
needed to reproduce the lane decision.

## Files

| File | Role |
| --- | --- |
| `20260606_185942_hz6_capacity_matrix_windows.md` | repeat-3 candidate comparison against the former DirectLocalFreeReuse selected-small lane |
| `20260606_185942_hz6_capacity_matrix_windows.log` | command log for the repeat-3 comparison |
| `20260606_190517_hz6_capacity_matrix_windows.md` | selected-family `selected-small-fixed` smoke after rewiring |
| `20260606_190517_hz6_capacity_matrix_windows.log` | command log for the selected-family smoke |

## Decision Summary

Repeat-3 versus
`directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k`:

| Profile | DirectLocalFreeReuse | SourceBlockRoute dynmap | Speed delta | RSS delta |
| --- | ---: | ---: | ---: | ---: |
| balanced | 76.471M / 96,776 KB | 82.939M / 98,288 KB | +8.46% | +1,512 KB |
| wide_ws | 0.450M / 69,260 KB | 0.467M / 72,836 KB | +3.78% | +3,576 KB |
| larger_sizes | 35.407M / 70,952 KB | 37.857M / 71,684 KB | +6.92% | +732 KB |
| large_slice_4k | 51.878M / 41,916 KB | 54.969M / 42,568 KB | +5.96% | +652 KB |
| large_slice_8k | 60.728M / 25,376 KB | 59.271M / 25,944 KB | -2.40% | +568 KB |
| large_slice_16k | 46.308M / 17,096 KB | 53.940M / 17,664 KB | +16.48% | +568 KB |

Selected-family smoke:

```text
PASS:
  selected-small-fixed builds and runs
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
```

Decision:

```text
Promote SourceBlockRoute dynmap as the selected-small candidate row.
Keep DirectLocalFreeReuse as the simple control/baseline.
Do not change broad/global HZ6 defaults based on this snapshot alone.
```
