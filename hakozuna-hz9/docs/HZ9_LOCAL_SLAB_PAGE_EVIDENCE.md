# HZ9 Local Slab Page Evidence

This file holds run logs and interpretation that would otherwise bloat the L1
design contract.

## Cleanup R10

```text
output:
  bench_results/20260701T212915Z_hz9_slabpage_r10_cleanup/

fixed64 local:
  baseline 166.77M
  slab     216.86M
  ratio    1.30
  p25      0.94

fixed64 live4:
  baseline 185.09M
  slab     234.07M
  ratio    1.27
  p25      1.24

medium r50:
  baseline 24.33M
  slab     26.45M
  ratio    1.09
  p25      1.13

main r90:
  baseline 16.93M
  slab     17.39M
  ratio    1.03
  p25      1.02
```

## Fixed64 Alternating R10

```text
output:
  bench_results/20260701T212930Z_hz9_slabpage_fixed64_alternating/

baseline:
  median 206.21M
  p25    184.09M

slab:
  median 246.38M
  p25    230.80M

ratio:
  1.19
```

## Queue/Qstate Cleanup R10

```text
output:
  bench_results/20260701T215242Z_hz9_slabpage_r10_queue/

fixed64:
  baseline 206.61M
  slab     236.07M
  ratio    1.14

medium r50:
  baseline 24.37M
  slab     24.89M
  ratio    1.02

main r90:
  baseline 16.99M
  slab     16.96M
  ratio    0.998
```

## Remote Pending-Bit Short R3

```text
fixed64_local0:
  baseline 209.59M
  slab 217.76M
  ratio 1.04

medium_r50:
  baseline 24.29M
  slab 25.33M
  ratio 1.04

main_r90:
  baseline 16.93M
  slab 16.58M
  ratio 0.98
```

Read:

```text
remote authority:
  materially cleaner than direct remote FREE

performance:
  still plausible, but main_r90 is at the guard edge
  require paired R10 before any promotion discussion
```

## RSS/Cap Debug Probe

```text
output:
  bench_results/20260701T221129Z_hz9_slabpage_rss_probe/

fixed64:
  ratio 38.19x
  registered 1MiB payload / 2MiB raw
  cap fallback 0

medium_r50:
  ratio 1.41x
  registered 4MiB payload / 8MiB raw
  cap fallback 3

main_r90:
  ratio 0.982
  slab pages 0
```

This is debug/counter evidence, not a release promotion gate.

## Cap 4/6/8 Debug Probe

```text
output:
  bench_results/20260701T221618Z_hz9_slabpage_rss_probe/

medium_r50:
  baseline 1.60M
  slab4    2.32M, ratio 1.448, cap fallback 5, raw 8MiB
  slab6    2.31M, ratio 1.442, cap fallback 0, raw 8.5MiB
  slab8    2.31M, ratio 1.438, cap fallback 0, raw 8.5MiB
```

Read:

```text
cap fallback is visible but not the current throughput limiter
cap6/cap8 remove fallback at a raw-byte cost and do not improve throughput
keep L1 cap4 until a release workload proves otherwise
```

## Cap 4/6/8 Release R10

```text
output:
  bench_results/20260701T222000Z_hz9_slabpage_cap_release_r10/

medium_r50:
  baseline 22.09M
  slab4    22.65M, p25 21.77M, peak 8.8MiB
  slab6    22.52M, p25 21.95M, peak 9.4MiB
  slab8    22.13M, p25 20.68M, peak 9.7MiB
```

Read:

```text
cap4 remains the best median point
cap6 is close but does not justify higher raw reservation yet
cap8 is worse on median and p25
```

## Pending Collection Attribution

```text
output:
  bench_results/20260701T222138Z_hz9_slabpage_rss_probe/

medium_r50 debug R3:
  remote claims 31902
  direct active/partial collect slots 31849
  queue collect slots 1
```

Read:

```text
owner queue is not the current hot limiter
direct active/partial pending collection carries the remote workload
next behavior should not tune queue cadence without new evidence
```

## Direct Collect Source Attribution

```text
output:
  bench_results/20260701T223202Z_hz9_slabpage_rss_probe/

medium_r50 debug R3:
  baseline 1.61M
  slab4    2.27M, ratio 1.408

median slab4 counters:
  remote claims 31905
  direct collect slots 31850
  queue collect slots 1
  pending active slots 25006
  pending partial slots 6816
  pending after-owner slots 0
  registered 4MiB payload / 8MiB raw
```

Read:

```text
active page direct collection is the dominant remote capacity path
partial pages are the secondary path
owner-queue drain after active/partial miss is effectively not creating the
capacity used by medium_r50

next behavior should not expand owner queue cadence or page cap first
candidate changes should target active/partial direct collect cost or reduce
the number of direct collect attempts without losing pending capacity
```

## Alloc-First Direct Collect Probe

```text
box:
  HZ9SlabAllocFirstDirectCollect-L1

change:
  scan FREE slots first
  collect pending only after allocation miss
  retry once after pending collect

output:
  bench_results/20260701T223814Z_hz9_slab_alloc_first_release_r10/
```

Release R10:

```text
fixed64_local0:
  baseline 198.48M
  alloc-first slab4 221.74M
  ratio 1.117

medium_r50:
  baseline 35.32M
  alloc-first slab4 33.40M
  ratio 0.945

main_r90:
  baseline 26.39M
  alloc-first slab4 26.28M
  ratio 0.996
  peak RSS worsened in this run
```

Decision:

```text
NO-GO for default SlabPage behavior

reason:
  local fixed64 improves, but medium_r50 loses more than the remote gate allows
  debug/counter positive signal did not survive release paired measurement

status:
  behavior moved behind H9_SLAB_ALLOC_FREE_FIRST_L1 opt-in macro
  default HZ9 slab page remains collect-first
```

## Alloc Cursor Probe

```text
box:
  HZ9SlabAllocCursor-L1

change:
  start page-local slot scan at the slot after the last successful allocation
  instead of always starting from slot 0

output:
  bench_results/20260701T224305Z_hz9_slab_alloc_cursor_release_r10/
  bench_results/20260701T224321Z_hz9_slab_alloc_cursor_repeat_r10/
```

Release R10 first run:

```text
fixed64_local0:
  baseline 193.57M
  cursor slab4 202.08M
  ratio 1.044

medium_r50:
  baseline 34.98M
  cursor slab4 35.20M
  ratio 1.006

main_r90:
  baseline 26.52M
  cursor slab4 25.73M
  ratio 0.970
```

Release R10 repeat:

```text
medium_r50:
  cursor slab4 34.58M
  baseline 34.92M
  ratio 0.990

main_r90:
  cursor slab4 24.53M
  baseline 27.41M
  ratio 0.895
```

Decision:

```text
NO-GO for default SlabPage behavior

reason:
  cursor gives only small fixed64/local signal
  main_r90 regression repeated even though 64K is not in the row
  likely code-shape / front-end cost is not worth the small local benefit

status:
  behavior moved behind H9_SLAB_ALLOC_CURSOR_L1 opt-in macro
  default HZ9 slab page keeps fixed slot scan order
```

## No-Page Route Fast Reject Probe

```text
box:
  HZ9SlabNoPageRouteFastReject-L1

change:
  return MISS/false from slab route/free/usable before route init when no
  slab pages have ever been registered

output:
  bench_results/20260701T224718Z_hz9_slab_no_page_fast_reject_r10/
```

Release R10:

```text
fixed64_local0:
  baseline 193.05M
  slab4 231.61M
  ratio 1.200

medium_r50:
  baseline 34.86M
  slab4 34.06M
  ratio 0.977

main_r90:
  baseline 25.46M
  slab4 15.29M
  ratio 0.601
```

Debug main_r90 check:

```text
registered_pages = 0
route_valid = 0
free_valid = 0
direct_slot = 0
```

Decision:

```text
HOLD / opt-in only

reason:
  main_r90 does not create slab pages, so the cliff is non-slab hook/code-shape
  cost rather than page behavior
  h9_slab_find already has a no-page fast NULL path
  pre-init rejection is too small and adds an extra atomic load once pages exist

status:
  behavior is behind H9_SLAB_NO_PAGE_FAST_REJECT_L1
  default HZ9 slab page keeps the existing h9_slab_find no-page fast path
```

## Slab Hook Range Gate

```text
box:
  HZ9SlabHookRangeGate-L1

change:
  publish a monotonic min/max address range for registered slab pages
  skip slab route/free/usable hooks when the pointer is outside that range

output:
  bench_results/20260701T225856Z_hz9_slab_range_gate_r10/
```

Release R10:

```text
fixed64_local0:
  baseline 205.27M
  slab4 245.37M
  ratio 1.195

medium_r50:
  baseline 37.73M
  slab4 36.85M
  ratio 0.977

main_r90:
  baseline 24.85M
  slab4 22.91M
  ratio 0.922
```

Decision:

```text
cleanup retained, default promotion still NO-GO

reason:
  the range gate is conservative and reduces avoidable slab hook calls
  fixed64 still has strong profile signal
  main_r90 and medium_r50 still fail no-regression gates

status:
  SlabPage remains profile/evidence only
  do not broaden page caps or owner lifecycle before non-target hook cost is
  removed by a different architecture
```

## Release Profile Gate Repeats

```text
script:
  scripts/run_hz9_slab_profile_gate.sh

outputs:
  bench_results/20260701T230337Z_hz9_slab_profile_gate/
  bench_results/20260701T230349Z_hz9_slab_profile_gate/
```

First repeat:

```text
fixed64_local0:
  ratio 1.149

medium_r50:
  ratio 1.097

main_r90:
  ratio 1.035
```

Second repeat:

```text
fixed64_local0:
  ratio 1.155

medium_r50:
  ratio 1.137

main_r90:
  ratio 0.980
```

Read:

```text
profile signal:
  strong for fixed64 and medium_r50

default promotion:
  still HOLD

reason:
  main_r90 median is only borderline in the second repeat
  p25 stability remains weaker than required for a default line
  SlabPage should stay profile/evidence until non-target hot-path isolation is
  proven across repeated gates
```

## Route-Find / Route-Last Attribution

```text
debug output:
  bench_results/20260702T075000Z_hz9_route_last_attribution_l1_hz9_slabpage_rss_probe/

sidecar2:
  medium_local0 active_hit 240000, hash_hit 0
  main_local0   active_hit 210365, hash_hit 0
  medium_r50    active_hit 120273, hash_hit 119727
  main_r90      active_hit 20246,  hash_hit 183804

route-last:
  medium_local0 last_hit 239952 but slower than sidecar2
  main_local0   last_hit 210333 but slower than sidecar2
  medium_r50    last_hit 135499, hash_hit 104453, slower than sidecar2
```

```text
release output:
  bench_results/20260702T080000Z_hz9_route_last_full_r5_hz9_candidate_gate/

sidecar2 ratios:
  guard_local0 1.060, small_remote90 0.979
  medium_local0 1.004, main_local0 0.958
  medium_r50 1.464, main_r90 1.648

route-last ratios:
  guard_local0 0.920, small_remote90 0.927
  medium_local0 0.861, main_local0 0.988
  medium_r50 1.163, main_r90 1.583
```

Read:

```text
local rows are not losing to slab route hash lookup; they already hit the
active thread page. A last-page cache adds fixed cost and weakens the remote
win. SlabPage route lookup is frozen as a tuning target.
```

## Mutation Attribution

```text
debug output:
  bench_results/20260702T084000Z_hz9_slab_mutation_attr_l1_hz9_slabpage_rss_probe/

sidecar2:
  medium_local0:
    free_local 240000
    scan/hit 1.00
    scan_miss 0

  main_local0:
    free_local 210365
    scan/hit 1.00
    scan_miss 0

  medium_r50:
    free_local 120273
    scan/hit 1.16
    scan_miss 3549

  main_r90:
    free_local 20552
    scan/hit 6.34
    scan_miss 128020
```

Read:

```text
local rows do not lose because allocation scans too many slots; they find a
free slot immediately. The local weakness is more likely the atomic
slot_state/free-boundary path and SlabPage substrate shape. Remote-heavy main
can become scan-heavy, so future remote behavior may still need a bitmap or
cursor, but that is not the local-row blocker.
```

## Sidecar Closure Evidence

Thread sidecar moved per-thread SlabPage active/pages arrays out of
`H8ThreadCtx` under `H9_SLAB_THREAD_SIDECAR_L1`.

```text
layout:
  H8ThreadCtx baseline             144 bytes
  H8ThreadCtx slabclasses_min0     384 bytes
  H8ThreadCtx thread sidecar       152 bytes

small/guard:
  guard_local0:
    slabclasses_min0               0.885 / 0.946
    thread sidecar                 1.015 / 1.117
  small_interleaved_remote90:
    slabclasses_min0               0.984 / 0.980
    thread sidecar                 0.978 / 1.000

full direction:
  medium_r50                       1.399
  main_r90                         1.587
  medium_local0                    0.960
  main_local0                      0.921
  small_interleaved_remote90       1.008
```

Owner sidecar moved owner SlabPage pending queue and adaptive-hot state into
`H9SlabOwnerState` under `H9_SLAB_OWNER_SIDECAR_L1`.

```text
layout:
  H8OwnerRecord baseline           440 bytes
  H8OwnerRecord slabclasses_min0   464 bytes
  H8OwnerRecord thread sidecar     464 bytes
  H8OwnerRecord thread+owner       448 bytes
  H9SlabOwnerState                  24 bytes

full direction:
  medium_r50                       1.404
  main_r90                         1.599
  medium_local0                    0.966
  main_local0                      0.911
  small_interleaved_remote90       0.960
```

Read: sidecars are valid source cleanup and reduce hot-struct footprint, but
they do not close the local throughput blocker. SlabPage stays profile/evidence,
not default.
