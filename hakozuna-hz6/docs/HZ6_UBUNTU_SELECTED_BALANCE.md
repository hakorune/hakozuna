# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip and static table trim became default.

Raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_112139
```

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
  hz6 48.327M vs hz4 30.738M

HZ6 uses less peak RSS than HZ4:
  hz6 94.25 MiB vs hz4 129.50 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 48.327M / 94.25 MiB
  mimalloc 1.321M / 255.50 MiB
```

HZ6 is still not the absolute speed ceiling because HZ3 remains far ahead, but
the latest selected lane now beats tcmalloc on the MidPage target row:

```text
4096..16384:
  hz3      75.258M /  73.75 MiB
  hz6      48.327M /  94.25 MiB
  tcmalloc 44.795M / 102.38 MiB
```

So the honest positioning is:

```text
HZ6 Ubuntu selected is now a strong balanced MidPage lane.
It is faster and lower-RSS than HZ4 on the MidPage row.
It is much stronger than mimalloc/system on the larger mixed row.
It still trails HZ3 on speed and RSS.
It now beats tcmalloc on speed, RSS, and ops-per-MiB on 4096..16384.
```

## Matrix

| row | allocator | median ops/s | median peak MiB | ops/s per MiB |
| --- | --- | ---: | ---: | ---: |
| `16_256` | `hz3` | 262454654.397 | 6.75 | 38882171.022 |
| `16_256` | `hz4` | 215809561.011 | 7.12 | 30289061.195 |
| `16_256` | `hz5` | 193837.258 | 106.12 | 1826.499 |
| `16_256` | `hz6` | 56533447.576 | 30.38 | 1861183.459 |
| `16_256` | `mimalloc` | 53413349.379 | 5.62 | 9495706.556 |
| `16_256` | `system` | 104712272.142 | 4.38 | 23934233.632 |
| `16_256` | `tcmalloc` | 260451088.262 | 9.25 | 28156874.407 |
| `16_4096` | `hz3` | 91402402.137 | 53.75 | 1700509.807 |
| `16_4096` | `hz4` | 55611734.221 | 59.75 | 930740.322 |
| `16_4096` | `hz5` | 193704.579 | 105.50 | 1836.062 |
| `16_4096` | `hz6` | 40911849.855 | 79.75 | 513001.252 |
| `16_4096` | `mimalloc` | 7107190.368 | 42.75 | 166250.067 |
| `16_4096` | `system` | 16658099.406 | 33.62 | 495408.161 |
| `16_4096` | `tcmalloc` | 98446562.468 | 41.25 | 2386583.333 |
| `1024_4096` | `hz3` | 87227134.443 | 63.50 | 1373655.661 |
| `1024_4096` | `hz4` | 50045009.230 | 54.00 | 926759.430 |
| `1024_4096` | `hz5` | 190767.778 | 106.12 | 1797.576 |
| `1024_4096` | `hz6` | 38617503.418 | 91.00 | 424368.169 |
| `1024_4096` | `mimalloc` | 5537842.689 | 48.00 | 115371.723 |
| `1024_4096` | `system` | 9547199.663 | 41.25 | 231447.265 |
| `1024_4096` | `tcmalloc` | 98138128.336 | 48.88 | 2007941.245 |
| `4096_16384` | `hz3` | 75258058.943 | 73.75 | 1020448.257 |
| `4096_16384` | `hz4` | 30738462.042 | 129.50 | 237362.641 |
| `4096_16384` | `hz5` | 199420.171 | 123.11 | 1619.913 |
| `4096_16384` | `hz6` | 48326902.110 | 94.25 | 512752.277 |
| `4096_16384` | `mimalloc` | 1320779.353 | 255.50 | 5169.391 |
| `4096_16384` | `system` | 2932553.878 | 65.50 | 44771.815 |
| `4096_16384` | `tcmalloc` | 44794507.548 | 102.38 | 437553.187 |

HZ6-only repeat-5 after `HZ6_MIDPAGE_32K_RUN_BYTES=2097152` promotion:

```text
raw: hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_112129
16..256      57.306M / 30.50 MiB
16..4096     41.608M / 79.75 MiB
1024..4096   39.868M / 91.00 MiB
4096..16384  49.480M / 94.38 MiB
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

Next optimization order from this evidence:

```text
1. AllocatorStaticTableTrimAudit-L1
   avoid or shrink full table multiplication for helper/cold allocators if safe

2. FrontcacheCapacityShapeAudit-L1
   closed for behavior. Keep class-max attribution for future lazy storage;
   do not shrink selected frontcache4096 by class yet.

3. MidPagePayloadTrimAudit-L1
   revisit 32K source payload after fixed table costs are understood
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
