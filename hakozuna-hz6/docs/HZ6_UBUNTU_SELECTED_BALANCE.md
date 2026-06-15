# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip and static table trim became default.

Latest full matrix raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_201844
```

This latest refresh was run after
`HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` became selected/default:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh \
  --runs 3 \
  --iters 300000 \
  --skip-builds
```

Latest production-shape HZ6-only check after
`HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1` became selected:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_194710
```

This is not a full cross-allocator matrix refresh, but it confirms the selected
HZ6 preload DSO no longer pays phase-counter calls in stats-off production
runs:

| row | selected ops/s | old phase-count-on ops/s | read |
| --- | ---: | ---: | --- |
| `16_256` | `51.875M` | `50.587M` | selected improves |
| `16_4096` | `33.809M` | `32.999M` | selected improves |
| `1024_4096` | `31.335M` | `31.338M` | selected flat |
| `4096_16384` | `43.570M` | `40.482M` | selected improves |

Latest HZ6-only check after `HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1` became
selected:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_195530
```

| row | previous selected ops/s | raw-pop selected ops/s | read |
| --- | ---: | ---: | --- |
| `16_256` | `50.873M` | `56.845M` | selected improves |
| `16_4096` | `33.516M` | `35.772M` | selected improves |
| `1024_4096` | `31.583M` | `33.437M` | selected improves |
| `4096_16384` | `43.136M` | `43.528M` | selected improves |

Command:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh \
  --runs 3 \
  --skip-builds
```

## Selected Read

The current HZ6 Ubuntu selected lane has its clearest speed/RSS balance on
`4096..16384`:

```text
HZ6 beats HZ4 on throughput:
  hz6 41.718M vs hz4 25.852M

HZ6 uses less peak RSS than HZ4:
  hz6 94.25 MiB vs hz4 112.88 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 41.718M / 94.25 MiB
  mimalloc 1.294M / 255.62 MiB
```

HZ6 is still not the absolute speed ceiling because HZ3 remains far ahead, but
the latest selected lane beats tcmalloc on the MidPage target row:

```text
4096..16384:
  hz3      51.570M /  73.62 MiB
  hz6      41.718M /  94.25 MiB
  tcmalloc 35.058M /  97.88 MiB
```

So the honest positioning is:

```text
HZ6 Ubuntu selected is now a strong balanced MidPage lane.
It is faster and lower-RSS than HZ4 on the MidPage row.
It is much stronger than mimalloc/system on the larger mixed row.
It still trails HZ3 on speed and RSS.
It beats tcmalloc on speed, RSS, and ops-per-MiB on 4096..16384.
It beats mimalloc on throughput across the focused rows, but not on small-row
RSS.
It still trails HZ3/HZ4/tcmalloc on tiny and mixed small-object speed.
```

## Matrix

| row | allocator | median ops/s | median peak MiB | ops/s per MiB |
| --- | --- | ---: | ---: | ---: |
| `16_256` | `hz3` | 266361558.380 | 6.75 | 39460971.612 |
| `16_256` | `hz4` | 227579727.152 | 7.12 | 31941014.337 |
| `16_256` | `hz5` | 196904.504 | 105.75 | 1861.981 |
| `16_256` | `hz6` | 61519570.298 | 30.50 | 2017035.092 |
| `16_256` | `mimalloc` | 53048128.206 | 5.62 | 9430778.348 |
| `16_256` | `system` | 105418028.075 | 4.38 | 24095549.274 |
| `16_256` | `tcmalloc` | 230663294.216 | 9.25 | 24936572.348 |
| `16_4096` | `hz3` | 93861086.062 | 53.88 | 1742201.133 |
| `16_4096` | `hz4` | 55391265.063 | 59.00 | 938835.001 |
| `16_4096` | `hz5` | 195153.965 | 105.25 | 1854.194 |
| `16_4096` | `hz6` | 42773731.026 | 79.75 | 536347.724 |
| `16_4096` | `mimalloc` | 7091341.510 | 42.88 | 165395.720 |
| `16_4096` | `system` | 17037459.572 | 33.62 | 506690.247 |
| `16_4096` | `tcmalloc` | 99883241.485 | 41.75 | 2392412.970 |
| `1024_4096` | `hz3` | 87488198.936 | 63.50 | 1377766.912 |
| `1024_4096` | `hz4` | 50984265.262 | 53.25 | 957450.991 |
| `1024_4096` | `hz5` | 198065.661 | 104.88 | 1888.588 |
| `1024_4096` | `hz6` | 39940337.922 | 91.12 | 438302.748 |
| `1024_4096` | `mimalloc` | 5539515.872 | 48.12 | 115106.823 |
| `1024_4096` | `system` | 9970855.091 | 41.38 | 240987.434 |
| `1024_4096` | `tcmalloc` | 99868842.249 | 49.62 | 2012470.373 |
| `4096_16384` | `hz3` | 76032620.884 | 73.75 | 1030950.792 |
| `4096_16384` | `hz4` | 31186247.027 | 130.25 | 239433.758 |
| `4096_16384` | `hz5` | 200032.715 | 122.71 | 1630.165 |
| `4096_16384` | `hz6` | 54836373.471 | 94.50 | 580279.084 |
| `4096_16384` | `mimalloc` | 1303440.932 | 255.38 | 5104.027 |
| `4096_16384` | `system` | 2982212.131 | 66.25 | 45014.523 |
| `4096_16384` | `tcmalloc` | 46507324.357 | 99.00 | 469770.953 |

HZ6-only repeat-5 after `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1`
promotion:

```text
raw: hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_145322
16..256      56.227M / 30.38 MiB
16..4096     40.939M / 79.88 MiB
1024..4096   40.079M / 91.00 MiB
4096..16384  50.574M / 94.38 MiB
```

## Next Optimization Read

The next HZ6 work should not chase `16..256` first. HZ3/HZ4/tcmalloc are
structurally far ahead there, and HZ6 pays preload/metadata costs that are not
likely to disappear with one local tweak.

The best next targets are:

```text
1. 4096..16384 MidPage speed frontier
   hold the new tcmalloc speed/RSS lead while trying to close the HZ3 gap

2. 16..4096 / 1024..4096 speed/RSS balance
   HZ6 throughput is respectable and RSS is much better after static trim, but
   HZ4/tcmalloc still have stronger frontiers

3. HZ5 preload-full sanity
   HZ5 is effectively unusable in this matrix shape; keep it as a compatibility
   data point, not an optimization target
```

## Fixed-Size Slice Read

The fixed-size comparison runner is:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_size_slices_matrix.sh \
  --runs 3 \
  --iters 300000 \
  --ws 4096 \
  --rows fixed_mid \
  --skip-builds
```

It is a diagnostic/comparison lane, not a selected-flag promotion lane. The
first fixed_mid refresh is:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_size_slices_20260615_203643
```

| row | hz6 | hz3 | hz4 | tcmalloc | read |
| --- | ---: | ---: | ---: | ---: | --- |
| `fixed_4k` | `31.376M / 91.75 MiB` | `60.642M / 68.50 MiB` | `14.186M / 59.50 MiB` | `44.508M / 70.38 MiB` | HZ6 beats HZ4/mimalloc/system, but trails HZ3/tcmalloc and has higher RSS. |
| `fixed_8k` | `41.815M / 93.12 MiB` | `53.735M / 69.75 MiB` | `13.638M / 64.75 MiB` | `28.138M / 73.62 MiB` | HZ6 beats HZ4/tcmalloc/mimalloc/system on speed; HZ3 remains faster and lower-RSS. |
| `fixed_16k` | `44.770M / 93.12 MiB` | `44.663M / 73.12 MiB` | `11.121M / 65.75 MiB` | `13.210M / 94.25 MiB` | HZ6 reaches the HZ3 speed frontier and beats the other measured allocators on speed; RSS remains above HZ3/HZ4/system. |

Large fixed slices use a smaller working set to avoid turning the comparison
into a memory-capacity test:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_size_slices_matrix.sh \
  --runs 3 \
  --iters 120000 \
  --ws 512 \
  --rows large_span \
  --skip-builds
```

Raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_size_slices_20260615_203813
```

| row | hz6 | hz3 | hz4 | tcmalloc | read |
| --- | ---: | ---: | ---: | ---: | --- |
| `fixed_32k` | `47.088M / 36.50 MiB` | `87.501M / 13.50 MiB` | `11.548M / 10.88 MiB` | `15.509M / 33.25 MiB` | HZ6 is below HZ3 but much faster than HZ4/tcmalloc/mimalloc/system. |
| `fixed_64k` | `18.137M / 35.88 MiB` | `38.326M / 15.25 MiB` | `8.320M / 142.50 MiB` | `7.746M / 90.25 MiB` | HZ6 trails HZ3, but has a strong speed/RSS balance versus HZ4/tcmalloc/mimalloc. |
| `fixed_128k` | `17.276M / 38.00 MiB` | `0.796M / 32.45 MiB` | `0.318M / 14.96 MiB` | `4.685M / 183.25 MiB` | HZ6 is near system speed and far ahead of HZ3/HZ4/tcmalloc/mimalloc; system keeps much lower RSS. |
| `fixed_256k` | `13.871M / 41.75 MiB` | `0.577M / 42.02 MiB` | `0.246M / 20.15 MiB` | `2.772M / 360.93 MiB` | HZ6 is the strongest non-system speed row and avoids the huge tcmalloc/mimalloc RSS cost. |

Updated optimization read:

```text
HZ6 has a real fixed-size strength band from 8K through 256K.
The next fixed-size work should not be generic active-map widening. It should
either target fixed_4k speed/RSS specifically or explain the fixed_mid RSS
premium with source-block/frontcache residency attribution.
```

Completed diagnostic:

```text
MidPageFrontcacheRSSAudit-L1
  split HZ6 selected peak RSS by frontcache bins, active-map storage, route
  table, source retain, and MidPage source blocks on 16..4096 / 1024..4096 /
  4096..16384.

FrontcacheCapacityShapeAudit-L1
  added class-level frontcache push/pop-empty/bin-max attribution and tested
  MidPage class-specific cap controls. Keep selected cap4096: class4 reaches
  cap4096 on 1024..4096, class5 cap3072 did not win, and class5 cap2560/2048
  slowed 4096..16384 by increasing empty pops.
```

## MidPage RSS Audit

Raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_rss_audit_20260614_164214
```

Command:

```bash
./hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh \
  --iters 200000 \
  --skip-build
```

Diagnostic build:

```bash
./hakozuna-hz6/linux/build_hz6_preload_diag.sh
```

The audit is diagnostic-only; throughput from this lane is not a selected
ranking number because probe counters are enabled.

| row | peak MiB | attributed MiB | static MiB | payload MiB | frontcache MiB | toy map MiB | midpage map MiB | active source blocks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `16_4096` | 100.25 | 122.10 | 61.73 | 54.75 | 20.00 | 3.75 | 1.88 | 864 |
| `1024_4096` | 111.62 | 132.85 | 61.73 | 65.50 | 20.00 | 3.75 | 1.88 | 1036 |
| `4096_16384` | 114.88 | 462.10 | 61.73 | 394.75 | 20.00 | 3.75 | 1.88 | 1582 |

Read:

```text
The fixed allocator-local table cost is large:
  static_table_bytes ~= 61.73 MiB with allocator_count=5
  frontcache_table_bytes ~= 20.00 MiB
  Toy active map ~= 3.75 MiB
  MidPage active map ~= 1.88 MiB

The 4096..16384 row additionally has real MidPage source payload pressure.
The payload attribution is logical backing capacity and can exceed resident RSS,
so it should guide source/run design but not be treated as exact resident pages.
```

Current optimization read:

```text
Static table trim is selected.
Frontcache storage/shape is closed as control/no-go for default.
MidPage payload/cold-retire behavior is closed as control/no-go for default.
Current-bias predicate variants and active-map capacity/layout follow-ups are
closed as controls/no-go.

Next work should use HZ6_UBUNTU_PRELOAD_LANES.md as the authoritative ledger
and start with a broader hot-path attribution refresh or a non-active-map
preload boundary/code-shape lane.
```

## Static Table Trim Promotion

Raw runs:

```text
hakozuna-hz6/private/raw-results/linux/hz6_static_table_trim_ab_20260614_164920
hakozuna-hz6/private/raw-results/linux/hz6_static_table_trim_confirm_20260614_165003
```

Promotion:

```text
HZ6_ROUTE_TABLE_CAPACITY=65536
HZ6_OBJECT_DESCRIPTOR_CAPACITY=16384
HZ6_SOURCE_BLOCK_CAPACITY=2048
HZ6_FRONT_CACHE_BIN_CAPACITY=4096
```

Safety repeat-3 with stats showed zero route-register failures, descriptor
exhaustion, source-block exhaustion, frontcache overflow, and real malloc
fallbacks on all selected rows.

Stats-off repeat-5 confirmation:

| row | old selected ops/s | old peak MiB | trim ops/s | trim peak MiB | read |
| --- | ---: | ---: | ---: | ---: | --- |
| `16_4096` | 41519124.134 | 100.62 | 43580843.543 | 79.75 | faster, about `-20.87 MiB` |
| `1024_4096` | 39965910.677 | 111.75 | 41848751.456 | 91.00 | faster, about `-20.75 MiB` |
| `4096_16384` | 40863038.822 | 115.25 | 42903601.635 | 94.38 | faster, about `-20.87 MiB` |

Read:

```text
This is a strong speed/RSS balance win and should be kept as the Ubuntu
selected default. The previous wide capacities remain the wide_l0 control.
```

## MidPage 32K Run512 Promotion

Raw runs:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260614_194352
hakozuna-hz6/private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
```

Promotion:

```text
HZ6_MIDPAGE_32K_RUN_BYTES=524288
```

Read:

```text
The original payload-trim hypothesis did not hold. Smaller 32K runs
128K/192K/224K did not reduce peak RSS materially, increased source_alloc on
4096..16384, and slowed the target. Larger 32K runs reduced source churn and
512K was the best observed control after static table trim.
```

Stats-off repeat-7 confirmation:

| row | selected 256K ops/s | selected 256K peak MiB | run512 ops/s | run512 peak MiB |
| --- | ---: | ---: | ---: | ---: |
| `16_256` | 59530701.620 | 30.38 | 60730135.954 | 30.50 |
| `16_4096` | 41810730.766 | 79.88 | 43235284.212 | 79.88 |
| `1024_4096` | 40855743.986 | 91.12 | 41567595.499 | 91.00 |
| `4096_16384` | 42175742.820 | 94.38 | 45298101.255 | 94.50 |

Read:

```text
At this historical checkpoint, run512 was selected for speed. It did not
materially improve RSS, but it kept the static table RSS win intact and moved
4096..16384 past the earlier tcmalloc median from the cross snapshot. Later
run768 and run1536 promotions refreshed the broad matrix and superseded this
intermediate tcmalloc read.
```

## MidPage 32K Run768 Promotion

Promotion:

```text
HZ6_MIDPAGE_32K_RUN_BYTES=786432
```

Read:

```text
At this historical checkpoint after the free-hint/free-fastslot no-go closeout,
the 4096..16384 row still trailed tcmalloc speed by about 7% while keeping
lower RSS. Increasing the
32K MidPage run above 512K is a better lever than further free-path code-shape
work because it reduces source churn without adding per-free classification
overhead.
```

Stats-off repeat-7 confirmation versus run512:

| row | run512 ops/s | run512 peak MiB | run768 ops/s | run768 peak MiB |
| --- | ---: | ---: | ---: | ---: |
| `16_256` | 57622875.452 | 30.50 | 58278310.110 | 30.38 |
| `16_4096` | 41850938.091 | 79.75 | 42389468.930 | 79.88 |
| `1024_4096` | 40600013.800 | 91.00 | 40386578.716 | 91.00 |
| `4096_16384` | 43110203.756 | 94.50 | 44324019.040 | 94.50 |

Safety spot-check:

```text
route_invalid=0
route_miss=0
alloc_fail=0
descriptor_exhausted=0
route_register_fail=0
source_block_exhausted=0
```

Decision:

```text
Run768 is selected for the Ubuntu preload speed/RSS balance.
Run512 remains the direct control.
Run1M and run1.5M are target-positive controls but less guard-clean.
```
