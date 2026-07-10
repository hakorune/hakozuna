# HZ12 Windows Stable-Duration MT Gate (2026-07-10)

Status: NO-GO for ColdSpanOwner default/profile promotion.

## Method

The legacy allocator matrix completes its fastest rows too quickly for a
promotion decision. The stable runner performs two calibration passes for each
allocator/profile, selects an iteration count targeting two seconds, and then
runs three measured rounds with rotated allocator order, affinity `0xFF`, and
High process priority.

All rows use the same `bench_allocator_compare` workload semantics. Iteration
counts differ only to make the measured duration comparable.

## Results

| Profile | HZ11 | HZ12 core | HZ12 ColdSpanOwner | tcmalloc | Cold peak RSS | tc peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 28.400M | 26.468M | 59.380M | **523.964M** | 49.09 MiB | **46.98 MiB** |
| wide_ws | 16.668M | 21.268M | 32.086M | **441.749M** | 88.46 MiB | **73.84 MiB** |
| larger_sizes | 48.858M | 39.248M | 74.075M | **245.415M** | 91.94 MiB | **82.18 MiB** |

ColdSpanOwner improves the HZ12 core by 2.24x on balanced, 1.51x on wide_ws,
and 1.89x on larger_sizes. That validates cold-span ownership as an internal
HZ12 improvement. It reaches only 11.3%, 7.3%, and 30.2% of tcmalloc
throughput, respectively.

HZ12 peak RSS is also 4.5%, 19.8%, and 11.9% above tcmalloc on these rows.
Therefore this matrix provides neither a throughput win nor a low-RSS Pareto
win over tcmalloc.

## Decision

- Keep ColdSpanOwner as opt-in HZ12 mechanism evidence.
- Do not promote it to default or present it as a tcmalloc competitor.
- Do not tune owner inbox caps or drain timing: those are not large enough to
  explain the stable broad gap.
- If research continues, perform one balanced-row attribution of global
  returned-sink lock/refill traffic. Otherwise freeze the Windows HZ12 speed
  track and retain the completed reclaim/lifetime evidence.

Evidence: `bench_results/windows_stable_mt_gate_r3.md`.
