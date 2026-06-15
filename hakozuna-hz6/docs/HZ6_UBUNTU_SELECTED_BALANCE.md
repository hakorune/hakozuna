# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip and static table trim became default.

Raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_103414
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
  hz6 45.283M vs hz4 30.938M

HZ6 uses less peak RSS than HZ4:
  hz6 94.38 MiB vs hz4 130.62 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 45.283M / 94.38 MiB
  mimalloc 1.323M / 255.50 MiB
```

HZ6 is still not the absolute speed ceiling because HZ3 remains far ahead, but
the latest selected lane now beats tcmalloc on the MidPage target row:

```text
4096..16384:
  hz3      75.143M /  73.75 MiB
  hz6      45.283M /  94.38 MiB
  tcmalloc 44.034M / 103.75 MiB
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
| `16_256` | `hz3` | 262327421.348 | 6.75 | 38863321.681 |
| `16_256` | `hz4` | 224768625.986 | 7.12 | 31546473.823 |
| `16_256` | `hz5` | 197333.161 | 105.62 | 1868.243 |
| `16_256` | `hz6` | 56806302.523 | 30.38 | 1870166.338 |
| `16_256` | `mimalloc` | 53272429.451 | 5.62 | 9470654.125 |
| `16_256` | `system` | 105392585.537 | 4.38 | 24089733.837 |
| `16_256` | `tcmalloc` | 255571947.656 | 9.12 | 28007884.675 |
| `16_4096` | `hz3` | 93106449.675 | 53.75 | 1732213.017 |
| `16_4096` | `hz4` | 54867560.545 | 59.75 | 918285.532 |
| `16_4096` | `hz5` | 191994.509 | 106.00 | 1811.269 |
| `16_4096` | `hz6` | 40577808.516 | 79.75 | 508812.646 |
| `16_4096` | `mimalloc` | 7083629.701 | 42.75 | 165698.940 |
| `16_4096` | `system` | 17043403.430 | 33.75 | 504989.731 |
| `16_4096` | `tcmalloc` | 98824536.492 | 41.25 | 2395746.339 |
| `1024_4096` | `hz3` | 87508682.502 | 63.62 | 1375382.043 |
| `1024_4096` | `hz4` | 49813963.526 | 54.12 | 920350.365 |
| `1024_4096` | `hz5` | 190443.125 | 106.00 | 1796.633 |
| `1024_4096` | `hz6` | 38725314.451 | 91.00 | 425552.906 |
| `1024_4096` | `mimalloc` | 5567539.328 | 48.00 | 115990.403 |
| `1024_4096` | `system` | 9745318.811 | 41.25 | 236250.153 |
| `1024_4096` | `tcmalloc` | 92348582.839 | 49.50 | 1865627.936 |
| `4096_16384` | `hz3` | 75143217.337 | 73.75 | 1018891.083 |
| `4096_16384` | `hz4` | 30938333.034 | 130.62 | 236848.483 |
| `4096_16384` | `hz5` | 200733.461 | 124.29 | 1615.003 |
| `4096_16384` | `hz6` | 45282626.874 | 94.38 | 479815.914 |
| `4096_16384` | `mimalloc` | 1322625.952 | 255.50 | 5176.618 |
| `4096_16384` | `system` | 2942973.086 | 66.00 | 44590.501 |
| `4096_16384` | `tcmalloc` | 44033565.290 | 103.75 | 424419.906 |

HZ6-only repeat-5 after `HZ6_MIDPAGE_32K_RUN_BYTES=1572864` promotion:

```text
raw: hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_103405
16..256      57.985M / 30.38 MiB
16..4096     41.868M / 79.75 MiB
1024..4096   40.253M / 91.12 MiB
4096..16384  48.563M / 94.50 MiB
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
Run512 is selected for speed. It does not materially improve RSS, but it keeps
the static table RSS win intact and moves 4096..16384 past the earlier tcmalloc
median from the cross snapshot. Refresh the broad matrix after this promotion
before making final tcmalloc claims.
```

## MidPage 32K Run768 Promotion

Promotion:

```text
HZ6_MIDPAGE_32K_RUN_BYTES=786432
```

Read:

```text
After the free-hint/free-fastslot no-go closeout, the 4096..16384 row still
trailed tcmalloc speed by about 7% while keeping lower RSS. Increasing the
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
