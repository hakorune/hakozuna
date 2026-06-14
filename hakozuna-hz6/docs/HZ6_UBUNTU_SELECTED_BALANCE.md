# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip and static table trim became default.

Raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
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
  hz6 41.264M vs hz4 30.932M

HZ6 uses less peak RSS than HZ4:
  hz6 94.38 MiB vs hz4 130.38 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 41.264M / 94.38 MiB
  mimalloc 1.319M / 255.50 MiB
```

HZ6 is not the speed ceiling:

```text
4096..16384:
  hz3      75.740M /  73.75 MiB
  tcmalloc 44.812M / 103.38 MiB
  hz6      41.264M /  94.38 MiB
```

So the honest positioning is:

```text
HZ6 Ubuntu selected is now a strong balanced MidPage lane.
It is faster and lower-RSS than HZ4 on the MidPage row.
It is much stronger than mimalloc/system on the larger mixed row.
It still trails HZ3 on speed and RSS.
It trails tcmalloc on speed, but now beats tcmalloc on RSS and ops-per-MiB on
4096..16384.
```

## Matrix

| row | allocator | median ops/s | median peak MiB | ops/s per MiB |
| --- | --- | ---: | ---: | ---: |
| `16_256` | `hz3` | 261137719.594 | 6.62 | 39417014.278 |
| `16_256` | `hz4` | 228704966.134 | 7.00 | 32672138.019 |
| `16_256` | `hz5` | 198891.000 | 106.25 | 1871.915 |
| `16_256` | `hz6` | 60381038.772 | 30.38 | 1987853.128 |
| `16_256` | `mimalloc` | 53002399.074 | 5.62 | 9422648.724 |
| `16_256` | `system` | 106096945.872 | 4.38 | 24250730.485 |
| `16_256` | `tcmalloc` | 256744583.941 | 9.38 | 27386088.954 |
| `16_4096` | `hz3` | 92920593.685 | 53.75 | 1728755.231 |
| `16_4096` | `hz4` | 55847545.137 | 58.88 | 948578.261 |
| `16_4096` | `hz5` | 196665.439 | 106.12 | 1853.149 |
| `16_4096` | `hz6` | 42216406.068 | 79.75 | 529359.324 |
| `16_4096` | `mimalloc` | 7072408.028 | 42.75 | 165436.445 |
| `16_4096` | `system` | 16868372.622 | 33.62 | 501661.639 |
| `16_4096` | `tcmalloc` | 97961302.836 | 41.75 | 2346378.511 |
| `1024_4096` | `hz3` | 87602299.227 | 63.50 | 1379563.767 |
| `1024_4096` | `hz4` | 50673307.640 | 54.25 | 934070.187 |
| `1024_4096` | `hz5` | 194373.336 | 105.00 | 1851.175 |
| `1024_4096` | `hz6` | 39671501.717 | 91.00 | 435950.568 |
| `1024_4096` | `mimalloc` | 5577984.601 | 48.00 | 116208.013 |
| `1024_4096` | `system` | 9747748.258 | 41.25 | 236309.049 |
| `1024_4096` | `tcmalloc` | 98389491.924 | 49.38 | 1992698.571 |
| `4096_16384` | `hz3` | 75740310.410 | 73.75 | 1026987.260 |
| `4096_16384` | `hz4` | 30931888.723 | 130.38 | 237253.221 |
| `4096_16384` | `hz5` | 201542.521 | 124.70 | 1616.179 |
| `4096_16384` | `hz6` | 41263985.190 | 94.38 | 437234.280 |
| `4096_16384` | `mimalloc` | 1318430.246 | 255.50 | 5160.197 |
| `4096_16384` | `system` | 2999584.040 | 66.25 | 45276.740 |
| `4096_16384` | `tcmalloc` | 44812285.593 | 103.38 | 433492.485 |

## Next Optimization Read

The next HZ6 work should not chase `16..256` first. HZ3/HZ4/tcmalloc are
structurally far ahead there, and HZ6 pays preload/metadata costs that are not
likely to disappear with one local tweak.

The best next targets are:

```text
1. 4096..16384 MidPage speed frontier
   close the remaining tcmalloc speed gap while preserving the new RSS lead

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
