# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip became default.

Raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_162527
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
  hz6 40.387M vs hz4 30.943M

HZ6 uses less peak RSS than HZ4:
  hz6 115.25 MiB vs hz4 130.00 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 40.387M / 115.25 MiB
  mimalloc 1.319M / 255.50 MiB
```

HZ6 is not the speed ceiling:

```text
4096..16384:
  hz3      76.584M /  73.62 MiB
  tcmalloc 44.321M / 105.50 MiB
  hz6      40.387M / 115.25 MiB
```

So the honest positioning is:

```text
HZ6 Ubuntu selected is now a strong balanced MidPage lane.
It is faster and lower-RSS than HZ4 on the MidPage row.
It is much stronger than mimalloc/system on the larger mixed row.
It still trails HZ3 and tcmalloc on the speed/RSS frontier.
```

## Matrix

| row | allocator | median ops/s | median peak MiB | ops/s per MiB |
| --- | --- | ---: | ---: | ---: |
| `16_256` | `hz3` | 263873547.574 | 6.75 | 39092377.418 |
| `16_256` | `hz4` | 226302634.740 | 7.00 | 32328947.820 |
| `16_256` | `hz5` | 193785.872 | 105.00 | 1845.580 |
| `16_256` | `hz6` | 55655949.613 | 51.25 | 1085969.749 |
| `16_256` | `mimalloc` | 53668202.883 | 5.62 | 9541013.846 |
| `16_256` | `system` | 107037753.714 | 4.50 | 23786167.492 |
| `16_256` | `tcmalloc` | 261853759.650 | 9.25 | 28308514.557 |
| `16_4096` | `hz3` | 93236585.458 | 53.88 | 1730609.475 |
| `16_4096` | `hz4` | 55053028.453 | 59.62 | 923321.232 |
| `16_4096` | `hz5` | 193732.672 | 105.00 | 1845.073 |
| `16_4096` | `hz6` | 40400379.069 | 100.62 | 401494.450 |
| `16_4096` | `mimalloc` | 7066513.084 | 42.75 | 165298.552 |
| `16_4096` | `system` | 16771952.458 | 33.57 | 499548.555 |
| `16_4096` | `tcmalloc` | 99044836.359 | 41.38 | 2393832.903 |
| `1024_4096` | `hz3` | 88341567.724 | 63.50 | 1391205.791 |
| `1024_4096` | `hz4` | 49301614.285 | 53.62 | 919377.423 |
| `1024_4096` | `hz5` | 194313.668 | 104.75 | 1855.023 |
| `1024_4096` | `hz6` | 38671310.240 | 111.88 | 345665.343 |
| `1024_4096` | `mimalloc` | 5585232.019 | 47.88 | 116662.810 |
| `1024_4096` | `system` | 9716498.091 | 41.50 | 234132.484 |
| `1024_4096` | `tcmalloc` | 94645972.009 | 50.12 | 1888198.943 |
| `4096_16384` | `hz3` | 76583581.904 | 73.62 | 1040184.474 |
| `4096_16384` | `hz4` | 30942971.268 | 130.00 | 238022.856 |
| `4096_16384` | `hz5` | 201227.163 | 124.43 | 1617.196 |
| `4096_16384` | `hz6` | 40386709.202 | 115.25 | 350426.978 |
| `4096_16384` | `mimalloc` | 1318757.833 | 255.50 | 5161.479 |
| `4096_16384` | `system` | 2958820.566 | 66.38 | 44574.711 |
| `4096_16384` | `tcmalloc` | 44321353.212 | 105.50 | 420107.613 |

## Next Optimization Read

The next HZ6 work should not chase `16..256` first. HZ3/HZ4/tcmalloc are
structurally far ahead there, and HZ6 pays preload/metadata costs that are not
likely to disappear with one local tweak.

The best next targets are:

```text
1. 4096..16384 MidPage RSS/speed frontier
   close the remaining tcmalloc gap while preserving the HZ4/mimalloc RSS win

2. 16..4096 / 1024..4096 RSS pressure
   HZ6 throughput is respectable, but peak RSS is too high versus HZ4/tcmalloc

3. HZ5 preload-full sanity
   HZ5 is effectively unusable in this matrix shape; keep it as a compatibility
   data point, not an optimization target
```

Recommended next diagnostic:

```text
MidPageFrontcacheRSSAudit-L1
  split HZ6 selected peak RSS by frontcache bins, active-map storage, route
  table, source retain, and MidPage source blocks on 16..4096 / 1024..4096 /
  4096..16384.
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
   test whether all rows need the selected global frontcache 8192 table

3. MidPagePayloadTrimAudit-L1
   revisit 32K source payload after fixed table costs are understood
```
