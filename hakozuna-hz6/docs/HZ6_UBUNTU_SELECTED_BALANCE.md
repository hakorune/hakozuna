# HZ6 Ubuntu Selected Balance

This note tracks the Ubuntu `LD_PRELOAD` selected lane after the MidPage
preload-boundary malloc skip and static table trim became default.

Latest full matrix raw run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_213629
```

This latest refresh was run after
class4/class5 frontcache storage trim became selected/default:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh \
  --runs 3 \
  --iters 300000 \
  --skip-builds
```

Previous production-shape HZ6-only check after
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

Latest HZ6-only quiescent RSS check after LD_PRELOAD `malloc_trim` was wired to
HZ6 local-free scavenge and Linux mmap retain-cache flush:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_222345
```

This is not a throughput promotion or peak-RSS improvement. It proves that HZ6
can return final all-local-free payload and retained mmap mappings through the
standard `malloc_trim(0)` API after the timed work is complete:

| row | selected current RSS | after malloc_trim current RSS | peak RSS read |
| --- | ---: | ---: | --- |
| `16..4096` | `79.75 MiB` | `27.14 MiB` | peak remains flat |
| `1024..4096` | `91.00 MiB` | `27.10 MiB` | peak remains flat |
| `4096..16384` | `94.38 MiB` | `28.32 MiB` | peak remains flat |
| `fixed_16k` | `93.12 MiB` | `28.26 MiB` | peak remains flat |

Latest HZ6-only production-shape check after
`HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1` became selected:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212533
```

This is a narrow LD_PRELOAD boundary code-shape promotion, not the older broad
preclassified-malloc no-go lane. It only applies after the preload boundary has
already accepted a 4097..32768-byte MidPage request:

| row | selected ops/s | peak RSS |
| --- | ---: | ---: |
| `16_256` | `58.141M` | `30.62 MiB` |
| `16_4096` | `35.805M` | `79.62 MiB` |
| `1024_4096` | `33.437M` | `91.00 MiB` |
| `4096_16384` | `44.720M` | `94.25 MiB` |
| `fixed_4k` | `32.257M` | `91.75 MiB` |
| `fixed_8k` | `41.978M` | `93.12 MiB` |
| `fixed_16k` | `44.608M` | `93.12 MiB` |

Stats-on safety read:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_212542
```

`fail=0` on all focused/fixed rows; source allocation and payload attribution
stay in the expected selected shape.

Latest HZ6-only production-shape check after class4/class5 frontcache storage
trim became selected:

```text
hakozuna-hz6/private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213453
```

This selects `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` with class4 storage
capacity `8192` and class5 storage capacity `4096`:

| row | selected ops/s | peak RSS |
| --- | ---: | ---: |
| `16_256` | `57.642M` | `30.50 MiB` |
| `16_4096` | `35.393M` | `79.62 MiB` |
| `1024_4096` | `33.692M` | `90.88 MiB` |
| `4096_16384` | `44.773M` | `94.12 MiB` |
| `fixed_4k` | `32.050M` | `91.88 MiB` |
| `fixed_8k` | `43.088M` | `93.25 MiB` |
| `fixed_16k` | `44.971M` | `93.12 MiB` |

Stats-on safety and attribution:

```text
hakozuna-hz6/private/raw-results/linux/hz6_frontcache_shape_ab_20260615_213504
```

`alloc_fail=0`, `route_register_fail=0`, `source_block_exhausted=0`, and
`malloc_real_fallback=0` on all focused/fixed rows. Frontcache/static
attribution is now `3002 KiB / 24369 KiB`.

Command:

```bash
./hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh \
  --runs 3 \
  --skip-builds
```

## Profile DSO Note

The selected DSO remains the balanced default. A separate Toy/mid-small profile
DSO is available for workloads dominated by small/Toy requests:

```bash
./hakozuna-hz6/linux/build_hz6_preload_toy_target.sh
./hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh \
  --runs 7 \
  --iters 300000 \
  --ws 4096 \
  --rows focused,fixed \
  --skip-build
```

Direct DSO raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy_target_preload_ab_20260615_220312
```

| row | toy_target vs selected | read |
| --- | ---: | --- |
| `16_256` | `+8.49%` | profile win |
| `16_4096` | `+5.19%` | profile win |
| `1024_4096` | `+4.10%` | profile win |
| `fixed_4k` | `+4.68%` | profile win |
| `fixed_8k` | `+0.15%` | effectively flat/slightly positive |
| `4096_16384` | `-1.42%` | default guard blocks promotion |
| `fixed_16k` | `-1.80%` | default guard blocks promotion |

Use the Toy target DSO only as a profile/control lane. Do not add
`HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1` to the selected flag bundle unless a
future code shape also preserves `4096..16384` and `fixed_16k`.

The Toy target DSO now includes the Toy direct-class fast-reuse profile shape:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy_target_fast_reuse_preload_ab_20260616_004700
```

| row | selected | toy-target fast-reuse | delta | read |
| --- | ---: | ---: | ---: | --- |
| `16..256` | `57.689M` | `75.220M` | `+30.39%` | strong profile win |
| `16..4096` | `36.163M` | `41.684M` | `+15.27%` | strong profile win |
| `1024..4096` | `33.491M` | `38.482M` | `+14.90%` | strong profile win |
| `4096..16384` | `44.667M` | `44.216M` | `-1.01%` | selected guard still blocks promotion |
| `fixed_4k` | `31.958M` | `36.041M` | `+12.77%` | strong profile win |
| `fixed_8k` | `42.905M` | `42.559M` | `-0.80%` | selected guard still blocks promotion |
| `fixed_16k` | `44.398M` | `44.821M` | `+0.95%` | acceptable but not enough to override fixed_8k/target |

Keep this as `hz6-toy-target`, not selected default.

Lower-bound Toy direct-class gates were also tested as controls:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy_direct_class_min_gate_ab_20260616_001000
```

| row | selected | direct | min1025 | min2049 | min3073 | read |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `16..4096` | `36.234M` | `37.394M` | `36.362M` | `35.464M` | `35.696M` | lower bounds mostly give up the profile win |
| `1024..4096` | `33.577M` | `34.535M` | `34.208M` | `33.563M` | `33.427M` | `min1025` is useful only as a weak profile/control |
| `4096..16384` | `44.811M` | `44.289M` | `43.204M` | `45.079M` | `44.698M` | guard recovery comes from excluding too much of the mid-small win |
| `fixed_4k` | `31.949M` | `33.156M` | `33.117M` | `33.530M` | `33.582M` | fixed-boundary profile remains useful |
| `fixed_8k` | `42.417M` | `42.101M` | `42.659M` | `42.553M` | `42.782M` | small positive control signal |
| `fixed_16k` | `44.475M` | `44.385M` | `44.560M` | `44.624M` | `44.683M` | guard-safe but not enough for selected promotion |

The lower-bound gate is not a selected-default escape hatch. Keep it available
for runner A/B and profile shaping, but keep the selected DSO balanced.

The stronger fixed-boundary profile combines Toy direct-class with realloc
boundary slack:

```bash
./hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh
```

Cross-allocator raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_small_boundary_cross_20260616_000000
```

| row | hz6 | small-boundary | hz3 | hz4 | tcmalloc | read |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `16..4096` | `36.250M` | `37.602M` | `67.134M` | `48.035M` | `82.472M` | profile improves HZ6, still behind HZ3/HZ4/tcmalloc |
| `1024..4096` | `33.775M` | `34.671M` | `62.465M` | `43.574M` | `78.786M` | profile improves HZ6, still behind HZ3/HZ4/tcmalloc |
| `fixed_4k` | `31.918M` | `45.696M` | `59.773M` | `14.184M` | `42.226M` | profile beats tcmalloc/HZ4, trails HZ3 |
| `fixed_8k` | `42.747M` | `44.947M` | `55.236M` | `13.734M` | `28.272M` | profile beats tcmalloc/HZ4, trails HZ3 |
| `4096..16384` | `43.767M` | `43.467M` | `51.808M` | `26.535M` | `34.878M` | selected remains the better balanced HZ6 default |

Use `hz6-small-boundary-target` only when the workload is known to be
small/fixed-boundary heavy. It is not a selected replacement because it gives
up the MidPage target balance while still trailing HZ3/HZ4/tcmalloc on broad
mid-small rows.

## Selected Read

The current HZ6 Ubuntu selected lane has its clearest speed/RSS balance on
`4096..16384`:

```text
HZ6 beats HZ4 on throughput:
  hz6 42.006M vs hz4 26.034M

HZ6 uses less peak RSS than HZ4:
  hz6 94.12 MiB vs hz4 111.88 MiB

HZ6 beats mimalloc strongly on both speed and RSS on this row:
  hz6 42.006M / 94.12 MiB
  mimalloc 1.286M / 255.38 MiB
```

HZ6 is still not the absolute speed ceiling because HZ3 remains far ahead, but
the latest selected lane beats tcmalloc on the MidPage target row:

```text
4096..16384:
  hz3      52.193M /  73.38 MiB
  hz6      42.006M /  94.12 MiB
  tcmalloc 34.873M / 100.25 MiB
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
| `16_256` | `hz3` | 238560905.195 | 6.62 | 36009193.237 |
| `16_256` | `hz4` | 212302242.672 | 7.00 | 30328891.810 |
| `16_256` | `hz5` | 197234.002 | 105.38 | 1871.734 |
| `16_256` | `hz6` | 55909629.166 | 30.38 | 1840646.228 |
| `16_256` | `mimalloc` | 52002339.239 | 5.50 | 9454970.771 |
| `16_256` | `system` | 98652270.231 | 4.25 | 23212298.878 |
| `16_256` | `tcmalloc` | 230272550.591 | 9.25 | 24894329.794 |
| `16_4096` | `hz3` | 66443052.573 | 53.38 | 1244834.709 |
| `16_4096` | `hz4` | 47063932.273 | 59.50 | 790990.458 |
| `16_4096` | `hz5` | 191913.892 | 105.50 | 1819.089 |
| `16_4096` | `hz6` | 34827844.513 | 79.62 | 437398.361 |
| `16_4096` | `mimalloc` | 6879767.385 | 42.62 | 161402.167 |
| `16_4096` | `system` | 16470818.116 | 33.75 | 488024.240 |
| `16_4096` | `tcmalloc` | 77055835.236 | 41.50 | 1856767.114 |
| `1024_4096` | `hz3` | 60821420.656 | 62.88 | 967338.698 |
| `1024_4096` | `hz4` | 42075723.048 | 52.62 | 799538.680 |
| `1024_4096` | `hz5` | 195778.480 | 105.62 | 1853.524 |
| `1024_4096` | `hz6` | 31870137.264 | 90.88 | 350703.024 |
| `1024_4096` | `mimalloc` | 5490971.185 | 48.00 | 114395.233 |
| `1024_4096` | `system` | 8491524.539 | 41.38 | 205233.221 |
| `1024_4096` | `tcmalloc` | 77413201.738 | 48.75 | 1587963.113 |
| `4096_16384` | `hz3` | 52193101.951 | 73.38 | 711319.958 |
| `4096_16384` | `hz4` | 26033989.239 | 111.88 | 232706.049 |
| `4096_16384` | `hz5` | 195763.786 | 117.95 | 1659.729 |
| `4096_16384` | `hz6` | 42005896.368 | 94.12 | 446277.783 |
| `4096_16384` | `mimalloc` | 1285911.494 | 255.38 | 5035.385 |
| `4096_16384` | `system` | 2808213.035 | 66.12 | 42468.250 |
| `4096_16384` | `tcmalloc` | 34872599.207 | 100.25 | 347856.351 |

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

Fixed-size residency follow-up:

```text
raw: hakozuna-hz6/private/raw-results/linux/hz6_midpage_rss_audit_20260615_204203
runner: run_hz6_midpage_rss_audit.sh --rows fixed_mid

fixed_4k:
  72.25 MiB logical source payload
  64.00 MiB 8K all-local-free payload
   8.00 MiB 32K all-local-free payload

fixed_8k:
  142.25 MiB logical source payload
  126.00 MiB 8K all-local-free payload
   16.00 MiB 32K all-local-free payload

fixed_16k:
  520.25 MiB logical source payload
  520.00 MiB 32K all-local-free payload
  16384 matching 32K frontcache entries
  ref mismatch = 0

read:
  The fixed_mid RSS premium is mostly frontcache-retained source-run payload,
  not ACTIVE descriptors. However the existing free-time 32K cold-retire gate
  does not fire on this final fixed_16k shape, so defaulting that behavior is
  still wrong. A new design would need an explicit quiescent/snapshot/scavenge
  trigger or a class-specific supply cap, not the previous per-free gate.
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

PageKindFreeSelectorDryRun-L1 predicted Toy/MidPage pages accurately, but the
behavior A/B did not pass.  Raw `hz6_page_kind_selector_first_prod_20260616_000902`
regressed every focused/fixed row, including `4096..16384 44.447M -> 39.926M`.
Keep all-free page-kind lookup behavior off; the saved wrong first active-map
probe is not worth the extra lookup/table cost.

The narrow preload code-shape follow-ups are also closed as controls/no-go for
selected default:
  HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1:
    useful target/fixed signal, but tiny guard regressed.
  HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1=1:
    fixed-row signal, but repeat-15 target guard regressed.
  Toy active-map full mask-wrap:
    cleaned up the control implementation, but still mixed/regressed guards.

As a profile lane, however, combining small-boundary target with trusted-owner
and raw-push is strong.  Repeat-15 no-stats raw
`hz6_midpage_payload_trim_ab_20260616_002519` improved every tiny/focused/fixed
guard row, while stats raw `hz6_midpage_payload_trim_ab_20260616_002549` kept
`fail=0`.  Keep it as `hz6-small-boundary-fast-target`, not selected default.

Cross-allocator profile position:
  raw:
    hz6_ubuntu_selected_balance_20260616_003133
    hz6_ubuntu_size_slices_20260616_003055

  The fast profile beats selected HZ6 on most focused/fixed rows and beats
  tcmalloc/HZ4/mimalloc on 4096..16384, fixed_8k, and fixed_16k speed.
  fixed_4k also beats tcmalloc speed.  It still trails HZ3 on the
  tiny/small speed/RSS frontier and keeps HZ6-class RSS rather than HZ3/HZ4 RSS.
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
