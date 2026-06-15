# HZ6 Ubuntu LD_PRELOAD Lanes

This document is the compact lane ledger for the Linux/Ubuntu `hz6`
LD_PRELOAD allocator. Keep long experimental notes in the root
`current_task.md`; keep Windows selected-family rows in
`HZ6_SELECTED_FAMILY_SUMMARY.md`.

## Selected Bundle

Build:

```bash
./hakozuna-hz6/linux/build_hz6_preload.sh
```

Authoritative selected flags:

```text
hakozuna-hz6/linux/hz6_preload_flags.sh
```

This document mirrors the selected bundle for review. Build and A/B scripts
should source the shared flag file and use key-based define replacement for
controls.

Output:

```text
hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so
```

Default flags selected by `build_hz6_preload.sh`:

```text
HZ6_ROUTE_TABLE_CAPACITY=65536
HZ6_OBJECT_DESCRIPTOR_CAPACITY=16384
HZ6_SOURCE_BLOCK_CAPACITY=2048
HZ6_FRONT_CACHE_BIN_CAPACITY=4096
HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1
HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY=8192
HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY=4096
HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=32768
HZ6_TOY_SOURCE_BLOCK_BYTES=65536
HZ6_MIDPAGE_RUN_BYTES=786432
HZ6_MIDPAGE_32K_RUN_BYTES=2097152
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1
HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1
HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1
HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1
HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1
HZ6_LINUX_MMAP_RETAIN_L1=1
HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128
HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1
HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1
HZ6_ROUTE_HASH_XOR_FOLD_L1=1
HZ6_ROUTE_LINEAR_WRAP_L1=1
HZ6_ROUTE_LOOP_CARRY_L1=1
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1
HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
```

The preload build script also explicitly keeps these no-go/control lanes off
unless `HZ6_EXTRA_CFLAGS` overrides them for an A/B run:

```text
HZ6_LINUX_MMAP_RETAIN_TLS_L1=0
HZ6_SOURCE_RUN_REUSE_L1=0
HZ6_ROUTE_PACKED_META_L1=0
HZ6_PRELOAD_FAST_FREE_L1=0
HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=0
HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=0
```

## Current Read

The preload lane is a real Ubuntu performance lane, but it remains separate
from the direct HZ6 API and Windows selected-family rows.

Latest selected-default focused guards, after current-bias, 8K run768, 32K
run2048, MidPage active-map mask-index, MidPage active-map register fast-slot,
raw direct-local pop, Toy free fast-slot, preload MidPage direct class, and
class4/class5 frontcache storage trim:

| Row | Selected read |
| --- | ---: |
| `16..256` HZ6-only repeat-5 | `57.642M / 30.50 MiB` |
| `16..4096` HZ6-only repeat-5 | `35.393M / 79.62 MiB` |
| `1024..4096` HZ6-only repeat-5 | `33.692M / 90.88 MiB` |
| `4096..16384` HZ6-only repeat-5 | `44.773M / 94.12 MiB` |
| `fixed_4k` HZ6-only repeat-5 | `32.050M / 91.88 MiB` |
| `fixed_8k` HZ6-only repeat-5 | `43.088M / 93.25 MiB` |
| `fixed_16k` HZ6-only repeat-5 | `44.971M / 93.12 MiB` |

Latest cross-allocator refresh after class4/class5 frontcache storage trim,
repeat-3, `bench_mixed_ws_crt`, raw
`private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_213629`:

| Row | hz6 | mimalloc | tcmalloc | system | hz6 peak KB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `16..256` | `55.910M` | `52.002M` | `230.273M` | `98.652M` | `31,104` |
| `16..4096` | `34.828M` | `6.880M` | `77.056M` | `16.471M` | `81,536` |
| `1024..4096` | `31.870M` | `5.491M` | `77.413M` | `8.492M` | `93,056` |
| `4096..16384` | `42.006M` | `1.286M` | `34.873M` | `2.808M` | `96,384` |

Important caveat:

```text
tcmalloc remains much faster on tiny and mixed small rows. On 4096..16384, HZ6
now edges tcmalloc on speed while keeping lower RSS and better ops-per-MiB.
HZ6 is clearly ahead of mimalloc on the selected mixed_ws preload rows, but HZ3
remains the higher speed/RSS frontier.
```

Follow-up:

```text
MidPage 8K run768, 32K run2048, active-map mask-index, active-map register
fast-slot, raw direct-local pop, Toy free fast-slot, phase-counter compile-out,
and preload MidPage direct class are now selected. The current selected-only
post-promotion read reaches 44.720M / 94.25 MiB on the HZ6-only
4096..16384 repeat-5. The latest full cross refresh shows the selected family
ahead of tcmalloc on speed and RSS on the MidPage target row. Keep 32K run1536,
run768, and run512 as direct controls.
```

Current lane state:

| Area | Selected | Closed controls |
| --- | --- | --- |
| MidPage source runs | `8K run768`, `32K run2048` | smaller/larger 32K fine ladder, 8K 512K guard-negative |
| MidPage active map | external cap16K/probe4, unaligned, mask-index, register fast-slot | cap32K/64K, probe8, class index, no-overwrite, same-class victim |
| MidPage free path | current-bias free order | free fast-slot, current-bias free fast-slot, page-hint behavior, unconditional/aligned MidPage-first |
| MidPage malloc path | preload-boundary noinline transfer skip, direct class, descriptor-out | core transfer-skip, broad preclassified malloc, trusted activation skip |
| RSS/storage | static table trim, class4/class5 frontcache storage trim | broad class storage trim and cold-class trims remain controls |
| Next lane | diagnostic/design | Cold source-block retire, current-bias predicate variants, and active-map collision/layout are closed as controls/no-go. Next work should look outside active-map capacity/probe and free-path source release. |

Current follow-up read:

| Lane | Status | Read |
| --- | --- | --- |
| MidPage active-map deeper probe | no-go direction | Miss attribution showed `found_elsewhere=0`; misses are not probe-limit misses. |
| MidPage active-map register fast-slot | selected/default | Probe audit showed 4096..16384 register/free hits average about `1.15` probes with `88.3%` base-slot hits. Register fast-slot improves target with better guard balance than enabling free fast-slot too. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | watch/control | Re-tested after descriptor-out: target improves, but `16..256` repeat-7 is still slightly weaker. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | watch/control | Helps Toy/tiny by skipping impossible MidPage probes, but target has `addr_envelope_skip=0`. Keep off. |
| Next likely lane | diagnostic/design | Toy active-map free fast-slot is now selected after raw-pop. Keep current-bias variants, cold-retire behavior, and active-map layout as controls/no-go. Next prefer a broader hot-path attribution refresh or a non-active-map preload boundary/code-shape lane. |
| Hot-path attribution refresh recipe | diagnostic/design | Use `run_hz6_preload_free_order_ab.sh --variants selected` for phase/hook counters and `run_hz6_midpage_payload_trim_ab.sh --stats --diagnostics --variants selected` for source/payload counters. Required fields include Toy/MidPage free attempts/hits, route-after-map split, real fallback, `mh_*` hint counters, source_alloc, MidPage class split, and cold-retire attempt/scan/block counters. |
| `HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1` | selected/default | Production DSO code-shape control. Compiles preload hook phase counters to no-op macros so stats-off runs do not pay counter function calls or size-bucket branches. Stats/diagnostic runners preserve phase counters unless explicitly testing `phase_count_off`. |
| `HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES=8192/16384` | control/no-go | Raising the preload-boundary shortcut lower bound gave tiny/guard micro-wins but lost the MidPage target badly. Keep selected `4096`. |
| `HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=0` / boundary off after Toy free fast-slot | control/no-go | Re-tested the boundary code shape after the latest selected free-path changes. Production repeat-9 raw `hz6_midpage_payload_trim_ab_20260615_202612` showed inline improved tiny/16..4096 but regressed target (`4096..16384 43.812M -> 42.896M`); boundary-off still broke target (`43.812M -> 35.205M`). Keep selected noinline boundary. |
| `HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1` | selected/default | Narrow LD_PRELOAD boundary code-shape control. It classifies requests already inside the 4097..32768-byte MidPage helper directly into class4/class5, avoiding the policy struct path. This is not the older broad `toy_preclassified_malloc` no-go. A/B repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_212317` was non-negative on all focused/fixed rows and target-positive (`4096..16384 43.285M -> 44.309M`). Post-promotion selected-only raw `hz6_midpage_payload_trim_ab_20260615_212533` reads `44.720M` on `4096..16384`; stats-on safety raw `hz6_midpage_payload_trim_ab_20260615_212542` kept `fail=0`. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` with c4=8192/c5=4096 | selected/default | Reopened after direct-class because the earlier frontcache trim no-go was pre-current-shape. Production repeat-15 raw `hz6_frontcache_shape_ab_20260615_213336` improved every focused/fixed row: `16..4096 33.998M -> 35.676M`, `1024..4096 33.232M -> 33.731M`, `4096..16384 44.144M -> 44.174M`, `fixed_16k 43.952M -> 44.358M`. Stats-on raw `hz6_frontcache_shape_ab_20260615_213400` kept fail counters zero and cut frontcache/static attribution from `10242/31609 KiB` to `3002/24369 KiB`. Post-promotion selected-only raw `hz6_frontcache_shape_ab_20260615_213453` confirmed the selected DSO shape. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1` | control/no-go for default | Narrow LD_PRELOAD Toy-boundary helper. It classifies `size <= 4096` directly into Toy classes without changing broad `hz6_malloc()` classification. The lane is a useful mid-small diagnostic but not selected: raw `hz6_midpage_payload_trim_ab_20260615_214226` improved `16..4096 35.690M -> 37.665M` and `1024..4096 32.640M -> 34.986M`, but regressed target/fixed guards (`4096..16384 44.877M -> 44.658M`, `fixed_16k 44.699M -> 44.226M`). Noinline+unlikely retest raw `hz6_midpage_payload_trim_ab_20260615_214404` kept the mid-small win and recovered target in that run, but still regressed `fixed_8k` and `fixed_16k`. Keep as a named control, not selected. |
| Toy target DSO | profile/control | `./hakozuna-hz6/linux/build_hz6_preload_toy_target.sh` builds `out/linux/hz6_preload_toy_target/libhakozuna_hz6_preload.so` with the selected bundle plus `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1` and max4096. Use this when the workload is known to be Toy/mid-small heavy. It is intentionally separate from selected default because target/fixed guards rejected default promotion. Direct DSO runner raw `hz6_toy_target_preload_ab_20260615_220312` confirms the split: `16..256 +8.49%`, `16..4096 +5.19%`, `1024..4096 +4.10%`, `fixed_4k +4.68%`, and `fixed_8k +0.15%`, while `4096..16384 -1.42%` and `fixed_16k -1.80%`. Shared allocator aliases `hz6-toy-target` and `hz6_toy_target` resolve this DSO for matrix runners. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=1024/2048/3072` | control/no-go for default | Capped Toy-boundary follow-up from the worker review. Raw `hz6_midpage_payload_trim_ab_20260615_214901` kept mid-small gains, but every cap regressed the target row versus selected (`4096..16384 40.170M`; max1024 `39.422M`, max2048 `39.879M`, max3072 `39.389M`). Class4 storage ladders `c4=12288/16384,c5=4096` also failed target/fixed guards. Keep as controls only; do not split selected default for Toy preclassification without a dedicated profile lane. |
| `-O3` / `-fno-semantic-interposition` preload build flags | control/no-go | Build-flag A/B raw `hz6_midpage_payload_trim_ab_20260615_202754` did not beat selected `-O2`: `-O3` regressed all focused rows and `-fno-semantic-interposition` was mixed but target-negative (`4096..16384 44.118M -> 43.909M`). Keep selected build flags unchanged. |
| `HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1` | selected/default | Production-only direct-local reuse code-shape control. Bypasses the generic `hz6_allocator_frontcache_pop()` wrapper in stats-off builds; disabled under diagnostics. Repeat-15 improved all focused rows and stats safety stayed clean. |
| `HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1=1` | control/no-go | Source-block prefill descriptor-out reduced route lookup probes but regressed production stats-off target speed. Keep default off. |
| current-bias retest after raw-pop | control/no-go | Production repeat-7 still rejects current-bias variants: `current_bias_2x` and `4x` gave small guard gains but regressed 4096..16384 (`44.164M -> 43.598M/43.268M`). Keep selected 1x generic predicate. |
| `HZ6_SAME_OWNER_FAST_L1=1` after raw-pop | control/no-go | Production repeat-7 gave small guard wins but did not beat the target (`4096..16384 44.492M -> 44.339M`). Keep off. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` after raw-pop | control/no-go | Production repeat-7 improved some guard rows but regressed the target (`4096..16384 44.492M -> 43.482M`). Keep off. |
| `HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` after raw-pop | selected/default | Production repeat-15 selected it as the only balanced Toy/small follow-up: `16..256 55.832M -> 56.483M`, `16..4096 35.258M -> 35.726M`, `4096..16384 43.657M -> 44.074M`; `1024..4096` was a small no-stats wobble (`33.027M -> 32.865M`) but stats-on safety was clean and the route-after-maps sample dropped (`1396 -> 1171`). |
| Toy/small raw-pop follow-up controls | control/no-go | `toy_addr_envelope`, `toy_preclassified_malloc`, `current_bias_off`, and `direct_max5` remain runner controls. They produced guard wins in some rows, but repeat-15 did not match the selected target balance. Raw roots: `hz6_midpage_payload_trim_ab_20260615_201443`, `hz6_midpage_payload_trim_ab_20260615_201517`, and diagnostic `hz6_midpage_payload_trim_ab_20260615_201548`. |
| `HZ6_PRELOAD_FAST_FREE_L1=1` after Toy free fast-slot | control/no-go | Re-tested broad route-prechecked free after route-after-maps fell to about `200..1100` per focused row. Production repeat-9 raw `hz6_midpage_payload_trim_ab_20260615_202343` still regressed tiny and target (`4096..16384 43.990M -> 43.603M`), so keep off. |
| Fixed-row audit controls | diagnostic/control | `run_hz6_midpage_payload_trim_ab.sh --rows fixed` now covers fixed 4K/8K/16K guards. Raw `hz6_midpage_payload_trim_ab_20260615_203102` showed isolated wins (`toy_preclassified_malloc` on 4K, `same_owner_fast` on 16K), but follow-up raw `hz6_midpage_payload_trim_ab_20260615_203155` kept `same_owner_fast` target-negative (`4096..16384 43.781M -> 43.188M`). Post-residency repeat-7 raw `hz6_midpage_payload_trim_ab_20260615_204454` kept the read: `direct_max5` nudged fixed_4k (`28.506M -> 28.965M`) but lost 16..4096, and `same_owner_fast` nudged fixed_16k (`39.192M -> 39.437M`) but lost the target. Keep as controls only. |
| Fixed_4k free-order/frontcache controls | control/no-go | `run_hz6_preload_free_order_ab.sh --rows fixed` and `run_hz6_frontcache_shape_ab.sh --rows fixed` now cover fixed rows. Attribution raw `hz6_preload_free_order_ab_20260615_204722` shows fixed_4k still mostly Toy class4 (`toy_hit ~= 406K`) with small MidPage-map interaction (`mid_hit ~= 6.4K`) and only about `1.1K` route fallbacks. `current_bias_2x` stats-on looked interesting, but production raw `hz6_midpage_payload_trim_ab_20260615_205053` regressed 16..4096, 1024..4096, fixed_8k, and fixed_16k. Frontcache controls are also no-go: `frontcache8192` raw `hz6_midpage_payload_trim_ab_20260615_205229` regressed tiny/16..4096, `storage_trim_c4_8192` raw `hz6_midpage_payload_trim_ab_20260615_205330` destroyed fixed_16k by trimming class5 to 3072, and `storage_trim_c4_8192_c5_4096` raw `hz6_midpage_payload_trim_ab_20260615_205408` still regressed target and fixed_8k/16k. |
| Toy class4 malloc-path audit | diagnostic/control | Diagnostic-only `toy_class4_*` counters split class4 malloc fast attempts/hits, front dispatch, frontcache pop/activate, free-cache, and active-map register/collision. Raw `hz6_midpage_payload_trim_ab_20260615_205845` shows fixed_4k is already fast-reuse dominated (`toy4_fast 1220355`, `toy4_hit 1217280`, `toy4_front 3075`) with visible active-map collision (`toy4_collision 65160`). Toy active-map controls did not promote: diagnostic raw `hz6_midpage_payload_trim_ab_20260615_205922` showed `toy_map64k` halves collision but raises RSS/regresses rows, and production repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_210005` kept `toy_probe8` no-go because target/fixed_4k/fixed_16k were flat or negative despite 16..4096/1024..4096 wins. |
| Toy active-map index controls | control/no-go | Added runner controls `toy_mask_index`, `toy_shift12_index`, and `toy_shift12_mask`. Diagnostic raw `hz6_midpage_payload_trim_ab_20260615_210443` rejected `shift12` immediately (`16..256` collapsed from about `24.5M` to `0.78M`). Production repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_210625` rejected branchless `toy_mask_index`: it improved tiny/mixed/fixed_4k but regressed target badly (`4096..16384 43.379M -> 40.302M`). Keep Toy active-map index shape selected as-is. |
| Toy active-map free probe audit | diagnostic/closeout | Added diagnostic-only Toy/class4 free hit base-slot and probe counters to avoid guessing from register collision alone. Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260615_215547` shows class4 free lookup is already cheap: `16..4096` base `94.3%` / avg probe `1.06`, `1024..4096` base `94.6%` / avg `1.06`, `fixed_4k` base `94.1%` / avg `1.07`, max probe `4`. This closes active-map capacity/probe as the next default lever; register collision is visible, but free lookup is not a large probe wall. |
| `HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_*` | control/no-go | Diagnostic-first attempt to route preload frees from source-run metadata after Toy/MidPage active-map misses. Raw `hz6_midpage_payload_trim_ab_20260615_221341` rejected it: run-route attempts had zero hits (`16..4096 938/0`, `1024..4096 1138/0`, `4096..16384 602/0`), while range-index tables raised RSS sharply (`4096..16384 94.00 -> 139.38 MiB`). Keep variants `runroute_dryrun_after_maps` and `runroute_after_maps` as evidence only; do not default. |
| Preload wrapper attribution | diagnostic-only | Added `[HZ6_PRELOAD_WRAPPER_DETAIL]` and `[HZ6_PRELOAD_WRAPPER_SIZE_DETAIL]` to split `posix_memalign` / `aligned_alloc` HZ6-compatible calls, real fallbacks, alignment buckets, and size buckets. `[HZ6_PRELOAD_PHASE_STATS]` also reports `calloc_zero_bytes`, and `[HZ6_PRELOAD_SIZE_STATS]` reports calloc size buckets. Temporary aligned-wrapper smoke proved counters fire for `posix_memalign(64, 2048)`, `aligned_alloc(4096, 4096)`, and `calloc(128, 32)`. Mixed_ws phase-count raw `hz6_midpage_payload_trim_ab_20260615_223042` shows aligned wrapper calls are zero on current selected rows, so do not behaviorize >16-byte aligned allocation without real-app or aligned bench evidence. |
| HZ6 bench quiescent scavenge probe | diagnostic/control | Added exported `hz6_preload_scavenge_local_free()` plus opt-in bench env `HZ_BENCH_SCAVENGE_BEFORE_RSS=all` and runner variant `selected_scavenge_before_rss`. Stats-on raw `hz6_midpage_payload_trim_ab_20260615_211346` proved final all-local-free payload is recoverable: 4096..16384 current RSS `94.25 -> 70.67 MiB`, fixed_16k `93.25 -> 59.94 MiB`, with payload attribution dropping to about `0.25 MiB`. No-stats raw `hz6_midpage_payload_trim_ab_20260615_211402` confirmed current RSS recovery while peak RSS stayed flat, as expected from `ru_maxrss`. Keep diagnostic/control, not default runtime behavior. |
| LD_PRELOAD `malloc_trim` hook | selected API/control behavior | `malloc_trim(size_t pad)` interposition now scavenges HZ6 local-free descriptors, flushes Linux mmap retain-cache mappings, then forwards to real libc `malloc_trim` if present. This exposes quiescent RSS recovery through a standard opt-in API without adding malloc/free hot-path work. Stats-on raw `hz6_midpage_payload_trim_ab_20260615_222328` shows current RSS dropping to the `27..28 MiB` floor while payload attribution drops to about `0.25 MiB`: `16..4096 79.62 -> 27.32 MiB`, `4096..16384 94.12 -> 28.49 MiB`, `fixed_16k 93.38 -> 28.29 MiB`. No-stats raw `hz6_midpage_payload_trim_ab_20260615_222345` confirms the production-shape trim result: `4096..16384 94.38 -> 28.32 MiB`, `fixed_16k 93.12 -> 28.26 MiB`; peak RSS remains flat as expected. |
| Toy prefill batch ladder | control/no-go | Raw `hz6_midpage_payload_trim_ab_20260615_203243` tested `HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=64/96/192/256`. Some fixed rows improved, but tiny and `1024..4096` regressed and larger batches weakened the target. Keep selected max128. |
| Source-run reuse/reclaim on fixed rows | control/no-go | Raw `hz6_midpage_payload_trim_ab_20260615_203339` re-tested `sourcerun`, `sourcerun_sameclass`, and `sourcerun_reclaim` on focused+fixed rows. All were guard-negative or target-flat/negative; keep `HZ6_SOURCE_RUN_REUSE_L1=0` for preload. |
| Ubuntu fixed-size slice matrix | diagnostic/comparison | `linux/run_hz6_ubuntu_size_slices_matrix.sh` records cross-allocator fixed-size rows. Raw `hz6_ubuntu_size_slices_20260615_203643` shows HZ6 at `31.376M/91.75MiB` on 4K, `41.815M/93.12MiB` on 8K, and `44.770M/93.12MiB` on 16K. Raw `hz6_ubuntu_size_slices_20260615_203813` shows HZ6 strongly ahead of HZ4/tcmalloc/mimalloc on 32K/64K and ahead of all non-system rows on 128K/256K speed, with system keeping the lowest RSS. Use this runner to decide fixed-size RSS/source-residency work; it is not a selected-flag promotion by itself. |

HZ3/HZ4 comparison read:

| Row | hz6 | hz3 | hz4 | Read |
| --- | ---: | ---: | ---: | --- |
| `16..256` | `61.520M` | `266.362M` | `227.580M` | HZ6 now beats mimalloc here, but HZ3/HZ4/tcmalloc/system are still architecture-fast. |
| `16..4096` | `42.774M` | `93.861M` | `55.391M` | HZ6 is about `0.77x` HZ4 after raw-pop, and beats system/mimalloc. |
| `1024..4096` | `39.940M` | `87.488M` | `50.984M` | HZ6 is about `0.78x` HZ4 after raw-pop, and beats system/mimalloc. |
| `4096..16384` old default | `29.409M` | `74.802M` | `31.089M` | HZ6 was about `0.95x` HZ4 and had the clearest close target. |
| `4096..16384` MidPage unaligned/probe4 | `31.505M` | n/a | `30.916M` | HZ6 now edges HZ4 while keeping lower RSS. |
| `4096..16384` raw-pop selected | `54.836M` | `76.033M` | `31.186M` | HZ6 strongly beats HZ4 and tcmalloc on the balanced MidPage row, but HZ3 remains the frontier. |

Architecture read:

```text
HZ3/HZ4:
  page/segment metadata first
  thin TLS/tcache local path
  class/owner recovery without HZ6 route descriptors in the common path

HZ6:
  explicit RouteLayer / descriptor / SourceLayer / FrontCache contracts
  stronger VALID / INVALID / MISS separation
  better scoped RSS and safety boundaries, but more hot-path work

Near-term target:
  hold the 4096..16384 tcmalloc/HZ4 lead while closing the HZ3 gap.
  Current raw-pop selected cross repeat-3:
    hz6 54.836M / 94.50 MiB
    hz4 31.186M / 130.25 MiB
    tcmalloc 46.507M / 99.00 MiB
    hz3 76.033M / 73.75 MiB

Longer-term target:
  design a local-page/run metadata fast lane if HZ6 must chase HZ3/HZ4 tiny
  object rows. Do not blur that with the current descriptor-first default lane.
```

## Promotion History

| Stage | Decision | Evidence |
| --- | --- | --- |
| Route capacity | `HZ6_ROUTE_TABLE_CAPACITY=131072` | Initial preload lane moved from about `0.236M` to about `3.05M`. |
| Source retain | `HZ6_LINUX_MMAP_RETAIN_L1=1` | Repeat-3 moved to about `7.0M..7.3M`; strace showed mmap/munmap churn reduction. |
| 64K retain stack | `HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1` | Repeat median moved to about `9.7M`; targets Toy 64KiB source blocks. |
| Toy full prefill | `HZ6_TOY_FULL_BLOCK_PREFILL_L1=1`, max128 | First guard moved to about `12.3M..12.6M`; Toy source churn dropped. |
| Tombstone compact | `HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1` | Removed the 1M long-run cliff; non-aggressive compact only. |
| Frontcache 8192 | `HZ6_FRONT_CACHE_BIN_CAPACITY=8192` | Focused 1M median moved to about `34.2M`; overflow unregister/tombstone churn removed. |
| Toy active fast free | `PreloadToyActiveFastFree-L1` | 16..256 route probes dropped from about `2.12M` to about `37.7K`; focused cross edged past mimalloc. |
| MidPage run 768K | `HZ6_MIDPAGE_RUN_BYTES=786432` | Current selected 8K MidPage run size after current-bias. Earlier run256K moved 4096..16384 from `16.549M` to `19.394M`; run768K later passed repeat-15 and selected repeat-5. |
| External MidPage active map | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1`, external, cap8192/probe2 | Repeat-7 versus no-map moved 1024..4096 from `30.962M` to `32.011M` and 4096..16384 from `18.983M` to `19.903M`. |
| Realloc in-place | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` | Repeat-5 versus control-off moved 16..256 `50.810M -> 55.313M`, 16..4096 `33.867M -> 36.556M`, 1024..4096 `31.473M -> 34.678M`, and 4096..16384 `19.971M -> 30.118M`. |
| MidPage unaligned/probe4 | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1`, probe4 | 4096..16384 active-map hits moved from `3,321` to `915,393`; repeat-5 HZ4 guard reached `hz6 31.505M` vs `hz4 30.916M` with lower HZ6 RSS. |
| MidPage active-map cap16K | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384`, probe4 | Resume diagnostic showed MidPage active-map misses on 4096..16384. Focused repeat-5 held `27.067M` versus pre-promotion default `25.908M`; cross repeat-3 held `27.752M` with about `+0.6MB` RSS. Cap32K and cap16K probe2/probe8 were weaker. |
| MidPage alloc descriptor-out | `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1` | MidPage alloc returns the activated descriptor to active-map registration, avoiding the post-alloc exact route lookup without changing prefill policy. Focused repeat-5 moved 4096..16384 median `30.004M -> 34.600M`, 16..4096 `41.126M -> 41.897M`, and 16..256 `55.985M -> 56.595M`; 1024..4096 was essentially flat. |
| MidPage miss audit after cap16K | `HZ6_PRELOAD_STATS=per_allocator` | Cap16K diagnostic still showed MidPage `free_hit=397136`, `free_miss=19254`, and `free_local_route_valid=9633` on the 200K 4096..16384 guard. The remaining miss path is real, but preload-boundary shortcuts must pass tiny guards. |
| MidPageSourcePressureAudit-L1 | diagnostic-only/closed baseline | Split MidPage 8K/32K alloc, run prefill, active-map register/free hits, and preload local route fallback before changing source-run or route-register behavior. Follow-up lanes selected run2048 and closed borrow/low-water/cold-retire controls. Active-map internals are closed for now. |
| MidPage source pressure read | diagnostic-only | 4096..16384 is 32K dominated: `midpage_32k_alloc_call=271026`, `midpage_8k_alloc_call=352`, `midpage_32k_prefill_run_call=2769`, `midpage_8k_prefill_run_call=352` on the 200K guard. |
| MidPage route-register class split | diagnostic-only | Source-run-slot route registration is visible now: on the 500K 4096..16384 guard, `route_register_reason_source_run_slot=17320`, split as `8K=5760` and `32K=11224`. This is much smaller than the 1M-class active-map register/free path, so do not chase source-run-slot route registration first. |
| MidPage active-map miss attribution | diagnostic-only | On selected 500K 4096..16384, MidPage active-map misses were not probe-limit misses: `free_miss=3102`, `probe_empty=2658`, `probe_occupied=444`, `found_elsewhere=0`. Deeper free probing is therefore not the next lever. |
| MidPage active-map lifetime audit | diagnostic-only | On selected 500K 4096..16384, `register_overwrite=1287` while route fallback was `8K=472`, `32K=815`. Overwrite explains much of the remaining fallback shape, but preserving old entries is not automatically better. |
| MidPage same-class victim dry-run | diagnostic/control | Same-class alternate victims exist (`same_class_alt=924` on a diagnostic 4096..16384 run), but behavior `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` regressed target median `34.597M -> 34.169M`; do not promote. |
| MidPage register callsite audit | diagnostic-only | With descriptor-out selected, 500K 4096..16384 showed `midpage_active_map_register_direct=333540`, `front_alloc=666979`, and `route_fallback=0`. The remaining target pressure is normal active-map registration, not post-alloc exact route fallback. |
| MidPage trusted activation skip | control/no-go | `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` skips the source-block bounds check on trusted local-free MidPage reuse. Focused repeat-5 was not better: 4096..16384 `35.114M -> 34.891M`, 16..256 `57.131M -> 56.538M`; keep off. |
| MidPage preclassified malloc | no-go direction | Perf showed malloc classification cost on 4096..16384, but preclassifying MidPage in the hot malloc shape regressed tiny guard badly (`16..256 55.463M -> 40.188M`) despite target gain. Do not keep the code-shape change. |
| MidPage free-cache audit | diagnostic/control | Selected 500K 4096..16384 had `midpage_active_map_free_cache_attempt=998685`, `success=998685`, `fail=0`; 8K/32K frontcache pushes were `339583/678023`. Failure is not the issue; only a thinner success path could help. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | control/no-go | Direct trusted cache push after active-map validation was flat/negative on the correct repeat-5: 4096..16384 `34.657M -> 34.477M`, 1024..4096 `40.039M -> 40.290M`, 16..256 `56.214M -> 55.622M`. Keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | control/no-go | Skipping empty transfer-first probes for MidPage direct-local reuse is target-positive but guard-sensitive. Helper-shape repeat-7 moved 4096..16384 `34.804M -> 38.944M`, but regressed 16..4096 `41.947M -> 40.394M` and 1024..4096 `40.552M -> 39.062M`; keep off. |
| TransferProbeAudit-L1 | diagnostic-only | Selected diagnostic 500K showed 4096..16384 has `333720` MidPage direct transfer probes and every probe was empty; 16..4096 had only `63` empty probes. The target witness is real, but selected default still needs guard isolation. |
| MidPage target DSO | control alias | `./hakozuna-hz6/linux/build_hz6_preload_midpage_target.sh` builds `out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so` with the selected outer-guard noinline MidPage malloc boundary. Use when a named MidPage-target DSO is useful. |
| MidPage guard-isolated transfer skip | control/no-go | noinline and noinline+unlikely helper shapes kept the 4096..16384 win (`~39.4M`), but still regressed 16..256, 16..4096, and 1024..4096 beyond the promotion gate. Keep as target DSO/control only. |
| MidPage preload-boundary malloc skip | selected/default | Wrapper-level MidPage malloc boundary keeps selected `hz6_malloc()` shape clean. The first direct boundary shape improved target but regressed guards; the promoted outer-guard noinline shape passed repeat-15. Confirmation against explicit boundary-off control: 4096..16384 `33.735M -> 40.056M`, 16..256 `55.957M -> 56.539M`, 16..4096 `40.969M -> 41.339M`, 1024..4096 `39.059M -> 39.953M`. |
| Static table trim | selected/default | `HZ6_ROUTE_TABLE_CAPACITY=65536`, `HZ6_OBJECT_DESCRIPTOR_CAPACITY=16384`, `HZ6_SOURCE_BLOCK_CAPACITY=2048`, and `HZ6_FRONT_CACHE_BIN_CAPACITY=4096` reduce allocator-local fixed table RSS. Confirm repeat-5 without stats moved 16..4096 `41.519M / 100.62 MiB -> 43.581M / 79.75 MiB`, 1024..4096 `39.966M / 111.75 MiB -> 41.849M / 91.00 MiB`, and 4096..16384 `40.863M / 115.25 MiB -> 42.904M / 94.38 MiB`; no route/descriptor/source failures in the repeat-3 safety lane. |
| MidPage 32K run2048 | selected/default | `HZ6_MIDPAGE_32K_RUN_BYTES=2097152` further reduces 32K source churn after run1536. Focused repeat-15 moved 4096..16384 `48.278M -> 49.789M`; stats repeat-3 kept fail counters 0 and cut 4096..16384 source_alloc `900 -> 723`. Keep run1536, run768, and run512 as direct controls. |
| MidPage active-map mask-index | selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1` replaces modulo wrapping with mask wrapping for the selected power-of-two active-map capacity. Production repeat-15 moved 4096..16384 `49.443M -> 50.231M`, kept 16..4096 flat, and 1024..4096 slightly positive. Stats repeat-3 kept fail counters 0; R1 smokes pass. |
| MidPage active-map register fast-slot | selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1` handles empty/same-pointer base-slot register without entering the bounded probe loop. Probe audit showed 4096..16384 register/free hits average `1.15` probes with `88.3%` base-slot hits. Production repeat-15 moved 4096..16384 `48.584M -> 50.160M`; stats repeat-3 kept fail counters 0; R1 smokes pass. |
| MidPage 32K fine ladder | control/no-go | Post-run2048 repeat-7 kept 2M as the local peak: 4096..16384 selected `49.494M`, run2048 rebuild `49.675M`, run2304 `48.864M`, run2560 `48.411M`, run3072 `44.866M`, and run4096 `46.384M`. Keep 1792K/2304K/2560K/3072K/4096K as controls, not selected. |
| MidPage 32K run1536 | control | Previous selected `HZ6_MIDPAGE_32K_RUN_BYTES=1572864`; keep as direct control for run2048. |
| MidPage 32K run768 | control | Earlier selected `HZ6_MIDPAGE_32K_RUN_BYTES=786432`; keep as direct control for run1536/run2048. |
| MidPage supply/map resume | diagnostic/control | After run768, selected diagnostic shows 4096..16384 has only about `2.2K` free route fallbacks for `1M` frees; free path is no longer the main wall. Remaining pressure is MidPage supply/frontcache shape: `midpage_source_alloc=649`, split as `8K alloc_call=180` and `32K alloc_call=469`. |
| MidPage 8K run768 | selected/default | `HZ6_MIDPAGE_RUN_BYTES=786432` reduces 8K source-run churn after current-bias. Repeat-15 versus run256K kept guards flat/positive and moved 4096..16384 +0.71%; post-promotion selected repeat-5 reached 46.496M / 94.25 MiB on 4096..16384 with clean safety stats. Keep 256K and 512K as direct controls. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | control/no-go | MidPage front retries local frontcache only after successful prefill_run, skipping a second transfer-first probe. Repeat-15 was not selected-safe: 16..256 regressed -7.11% and 4096..16384 regressed -2.08%. Keep off. |
| MidPage active-map cap/probe widening | control/no-go | `cap32K/64K` and `probe8` remove most remaining route-after-map fallbacks, but larger/hotter maps regress target speed and raise RSS. Example focused repeat-5: cap32K/probe4 cut 4096..16384 `route_after_maps ~2199 -> 124`, but target speed fell and RSS rose. Keep cap16K/probe4 selected. |
| MidPage low-water refill | control/no-go | Default-off behavior control. After a MidPage cache-miss refill successfully returns an object, optionally prefill one more run if that class remains below a small low-water mark. Stats-off repeat-9 did not pass; strong 8K=256/32K=128 was target-flat and guard-negative. Keep off. |
| `HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1` | control/no-go | Narrowed version of the guard-negative broad `borrow_larger` experiment. It only lets MidPage 8K misses try a 32K local-free entry. Production repeat-7 was guard-safe but target-flat/weak (`4096..16384 50.077M -> 49.896M`), and stats repeat-3 showed `borrow_success=0` on the selected target row. Keep off. |
| `MidPageActiveMapCollisionLayoutAudit-L1` | diagnostic/control closeout | `run_hz6_midpage_supply_map_ab.sh` now reports active-map register collisions, overwrites, and free miss found-elsewhere alongside avg probe/base-slot ratios. Diagnostic raw `private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_183000` showed selected 4096..16384 has real register collision (`68890`) but `miss_found_elsewhere=0`; widening to 32K/64K cuts route fallbacks (`1465 -> 114 -> 2`) but raises RSS and does not produce a clean speed win. Do not pursue active-map capacity/probe/layout behavior now. |
| `MidPage32KRunFineLadder-L1` | control/no-go for default | `run_hz6_midpage_payload_trim_ab.sh` now supports 1920/1984/2112/2176/2240 KiB variants plus diagnostic payload attribution. Production repeat-7 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163111` kept selected/run2048 best on 4096..16384 (`50.895M / 94.50 MiB`; `run2112k 48.394M`, `run2176k 48.825M`). Diagnostic repeat-3 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163443` showed larger runs reduce source_alloc (`723 -> 708/696`) but increase payload attribution (`399.25 MiB -> 400.00/402.25 MiB`) and lose speed. Keep `HZ6_MIDPAGE_32K_RUN_BYTES=2097152`. |
| `MidPagePayloadResidencyAudit-L1` | diagnostic-selected | `Hz6StatsSnapshot` and `HZ6_PRELOAD_MEMORY_ATTR` split MidPage source payload by 8K/32K source blocks and descriptor residency. RSS audit raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_171658` shows 4096..16384 has `354.00 MiB` of MidPage 32K payload across `177` blocks, `0` active descriptors, `11328` local-free descriptors, and `354.00 MiB` all-local-free payload with `ref mismatch=0`. The runner now supports `--rows focused,fixed_mid,large_span`; fixed-size raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_204203` shows fixed_16k has `520.00 MiB` 32K all-local-free payload with `16384` matching frontcache entries and `ref mismatch=0`. Proceed to retire/soft-cap design only through diagnostics; do not jump directly to default behavior. |
| `MidPageColdSourceBlockRetireDryRun-L1` | diagnostic-selected | Residency snapshot now reports retire-candidate blocks/payload/descriptors/frontcache entries. RSS audit raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_172905` shows 4096..16384 has `354.00 MiB` of 32K retire-candidate payload, `11328` candidate descriptors, and `11328` matching frontcache entries with `ref mismatch=0`. This supports a default-off out-of-line 32K cold-retire behavior control with high-water and max-blocks guards. |
| `MidPage32KColdRetireBehavior-L1` | control/no-go for default | Default-off RSS behavior control. High-water-triggered helper drains all-local-free class5 source blocks only after verifying descriptor refs and matching frontcache entries. Eager raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_173814` caused source churn (`source_alloc 723 -> 3831`) and large speed loss. Quiescent max16 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174116` retired payload without churn, but production repeat-5 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174140` still regressed 4096..16384 (`32.246M -> 27.628M`) with no maxRSS win. Fixed-row retest raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204233` did not lower peak RSS, and diagnostic raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204253` showed `retire attempt=0`; the free-time gate misses the final all-local-free fixed_16k state. Keep as control/no-go; do not default. |
| Frontcache class-max attribution | diagnostic-only | `HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL` prints class-level push/pop-empty/max occupancy. FrontcacheShapeAudit repeat-3 showed class4 reaches cap4096 on 1024..4096, while class5 reaches about 2832 on 4096..16384. This supports future lazy/cold storage design, but not a broad cap shrink. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | control/watch, not selected | Default-off class-specific frontcache backing storage trim. It shrinks allocator-local frontcache entry arrays by class while preserving class2/class4 4096 storage and class5 3072 storage. It is buildable and mostly speed-neutral/target-positive in production-shape repeat-7, but peak RSS barely moved and diagnostic target was weak. Keep as a storage-layout control, not selected. |
| `FrontcacheStorageLayoutAuditV2-L1` | control/no-go for default | `run_hz6_frontcache_shape_ab.sh --variants` can run focused storage variants without rebuilding all older cap controls. Production repeat-5 raw `private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161719` kept selected best on 4096..16384 (`51.332M / 94.38 MiB` vs `storage_trim_cold32 50.271M / 94.38 MiB`) and peak RSS stayed flat. Diagnostic repeat-3 raw `private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161900` showed clean counters and a real attribution cut (`frontcache table 10242 KiB -> 2152 KiB`), but source payload dominates resident pressure. Keep as evidence/control, not default. |
| Preload hook path attribution | diagnostic-only | `HZ6_PRELOAD_HOOK_DETAIL` splits free() hook flow across Toy active-map attempt/hit/miss, MidPage active-map attempt/hit/miss, route-after-map lookup, local/visible route valid, invalid, and real fallback. First short read showed 4096..16384 pays mostly Toy active-map misses before MidPage hits. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Unconditionally trying MidPage active-map before Toy active-map recovers part of the 4096..16384 Toy-miss wall, but the guard cost is too high: tiny/Toy rows regress materially. Keep off in selected default; use as evidence for a future class-aware/free-hint gate. |
| `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | control/no-go | Cheap selective MidPage-first free gate using MidPage active-map alignment. It behaved like unconditional MidPage-first because Toy-heavy rows also passed the alignment gate; guard regressions were large. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | selected/default | Cheap selective MidPage-first free gate using allocator-local active-map current counts. If MidPage active entries exceed Toy active entries, free tries MidPage first. Promotion repeat-15 kept 16..4096 and 1024..4096 flat/slightly positive, limited 16..256 to -0.45%, and improved 4096..16384 by +3.24% with flat RSS. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1=1` | control/no-go for default | Code-shape control that simplifies the selected current-bias predicate to `midpage_current > toy_current`. Diagnostic raw `private/raw-results/linux/hz6_preload_free_order_ab_20260615_174543` was mixed; production raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174700` gave only a small target lift (`31.516M -> 31.781M`) while 16..256 and 1024..4096 regressed. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=4` | control/no-go for default | More conservative MidPage-first threshold. Diagnostic shape looked interesting, but production raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174700` did not hold: target was flat/negative and guard rows regressed. Keep selected 1:1/delta0. |
| `HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1=1` | diagnostic-only/no-go direction | Dry-run for selective MidPage-first free ordering. The recent min/max envelope covered 4096..16384 well, but false positives were huge on 16..4096 and 1024..4096. Behavior remains Toy-first; future work needs a tighter range/page hint table. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | diagnostic-only | Tighter selective MidPage-first free dry-run. Preload MidPage malloc boundary hits register base/last 4K pages in a small TLS hint table; free() probes the table and reports through the existing `mh_*` hook-detail counters. Capacity32768 was much cleaner than the broad envelope, but the behavior A/B showed the per-free probe cost is too high. Keep as evidence only. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | control/no-go | Behavior A/B for the page-hint gate. Hinted frees use MidPage-first ordering; unhinted frees preserve selected Toy-first ordering. It reduced 4096..16384 Toy active-map attempts, but short repeat-5 regressed every focused row, so the per-free hint probe/table overhead is not selected-safe. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | control/no-go | Re-tested after mask-index and register fast-slot. Repeat-15 was target-flat (`50.375M -> 50.408M`) and guard-weak on 16..256 and 1024..4096, so keep off. |

Closeout details for the selective MidPage-first free lanes are in:

```text
HZ6_UBUNTU_PRELOAD_FREE_HINT_CLOSEOUT.md
HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md
```

## Selected Controls

Keep these controls available when changing the preload lane:

| Control | Why |
| --- | --- |
| no MidPage active map | Direct control for `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2`. |
| internal MidPage active map | MidPage-only upper-bound; repeat-7 4096..16384 reached `20.504M`, but balanced locality was worse than external storage. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=1572864` | Previous selected 32K run size and direct control for run2048. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=786432` | Earlier selected 32K run size and direct control below run1536. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=524288` | Earlier selected 32K run size and direct control below run768. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=262144` | Earlier selected 32K run size and direct control for run512. |
| `HZ6_MIDPAGE_RUN_BYTES=262144` | Previous selected 8K run size and direct control for run768. |
| `HZ6_MIDPAGE_RUN_BYTES=524288` | 8K source-run widening control. It lowers source_alloc on 4096..16384 and remains target-positive after current-bias, but did not pass guard gates. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | MidPage low-water refill behavior control. Default thresholds 8K=128/32K=64 and strong thresholds 8K=256/32K=128 did not pass selected gates. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=0` | Direct control for MidPage active-map 8K-alignment gating. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=8192` | Previous balanced default and direct control for the selected cap16K promotion. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=2` | Balanced control for the selected probe4 MidPage active map. Probe2 is slightly better on 1024..4096 but weaker on the HZ4-close 4096..16384 target. |
| `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=0` | Direct control for the selected MidPage active-map register fast-slot. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | Free-side active-map fast-slot control. Target-flat and guard-weak after register fast-slot. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1` | Gated free-side fast-slot control. It proved the extra current-bias branch is not free; keep off. |
| `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=0` | Direct control for the selected MidPage descriptor-out malloc path. |
| frontcache 8192/16384 | Wide frontcache controls after static table trim. 8192 is the previous selected capacity; 16384 was flat enough not to promote. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072` | Class-specific cap control. Safe in the first FrontcacheShapeAudit repeat-3 but did not improve speed/RSS enough to promote. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific frontcache backing-storage trim control. It tests RSS from storage shape rather than logical bin caps. |
| route table 262144 | Capacity upper-bound; not selected by current evidence. |
| `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=0` | Direct control for preload realloc in-place. |
| `wide_l0` static tables | Previous preload table capacities: route 131072, descriptor 32768, source blocks 4096, frontcache bin 8192. Keep as direct RSS/safety control for the selected trim. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=0` | Direct control for the selected balanced current-bias free ordering. |

MidPage 32K run-size closeout details are in
`HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md`.

## No-Go / Evidence-Only

| Lane | Read |
| --- | --- |
| Toy-map widening to MidPage | Reduced route probes but created too many map misses/collisions and slightly regressed focused repeat; do not widen the Toy active map. |
| Toy active-map capacity 65536/16384 | Both capacity directions regressed Toy high rows in focused repeat; keep the selected 32768 default. |
| Toy active-map probe8 | Roughly flat on 1024..4096 and weaker on 16..4096; keep probe4. |
| MidPage active-map cap32K | Regressed both 1024..4096 and 4096..16384 in the resume repeat-3; too large for current locality. |
| MidPage active-map cap16K probe2/probe8 | Both were weaker than cap16K/probe4 in focused repeat-3. |
| `HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1=1` | Re-arming the MidPage active map after preload local route-valid shifted hits slightly but produced only a small/flat 4096..16384 change; keep as evidence-only. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | MidPage-only prechecked free improved 4096..16384 median in a short focused repeat (`27.855M control -> 28.094M`) but regressed the tiny 16..256 guard (`57.9M control -> 55.0M`); keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1=1` | 4K-granularity index helped some Toy/high guards but regressed the target 4096..16384 row to about `24.8M`; keep the selected 8K-shift index. |
| `HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1=1` | Preserving existing entries on full probe looked plausible, but target 4096..16384 regressed before and after descriptor-out. Latest retest moved `35.281M -> 34.100M`; keep current base-slot overwrite policy. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1` | Current-bias-gated free fast-slot tried to avoid the guard cost of plain free fast-slot, but repeat-7 weakened tiny and target rows. Keep off. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=128K/192K/224K` | Smaller 32K runs looked like payload-trim candidates, but RSS stayed flat while source allocation rose and 4096..16384 slowed. Keep as no-go evidence. |
| `HZ6_MIDPAGE_RUN_BYTES=384K/512K/640K` | 8K run widening controls below selected run768. 512K remained target-positive but guard-negative; 384K/640K are evidence-only. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | Post-prefill cache-only reuse skipped a second transfer-first probe, but repeat-15 regressed both tiny and target rows. Keep off. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | Eagerly prefill one more MidPage run after miss-boundary reuse. It adds source/refill work and did not improve stats-off selected balance. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=32768/65536` and probe8 | Larger maps/probes remove remaining route fallback but lose speed/RSS to map hotness. Keep selected cap16K/probe4. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=2` | Current-bias target upper-bound. 4096..16384 can improve more than 1x, but 16..4096 regresses too much. Keep off. |
| `HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Toy active-map negative envelope. It is conservative and buildable, but the first repeat-3 did not pass promotion: tiny was slightly positive while 1024..4096 and 4096..16384 were weak. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | Unconditional preload free order swap. Target-positive but guard-negative; keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | Page-table dry-run for selective MidPage-first ordering. Use before any behavior gate; it must avoid the false-positive wall observed in the broad recent-envelope dry-run. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | Page-hinted MidPage-first behavior control. It is narrower than unconditional MidPage-first but still slower in short repeat-5; keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | MidPage active-map free lookup fast-slot control. After register fast-slot selection, repeat-15 was target-flat and guard-weak; keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=2560/2048` | Class5 cap shrink increased class5 empty pops and slowed 4096..16384 in FrontcacheShapeAudit-L1. Keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY=3072` with 32K cap3072 | Broad MidPage cap shrink regressed 1024..4096 badly because class4 can genuinely reach cap4096. Keep off. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific storage trim did not produce a meaningful peak RSS reduction in the focused preload rows. Production-shape repeat-7 was acceptable but not enough for default; diagnostic repeat-1 was target-weak. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1=1` | Class-salted MidPage active-map index did not reduce collision pressure in diagnostic (`register_collision 369139 -> 373449`, `free_miss 3632 -> 4624`) and regressed 1024..4096 in the first A/B. Keep off. |
| `HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1=1` | Avoiding post-prefill exact route lookup by prefill/direct-pop retry is too disruptive: first repeat-3 dropped 4096..16384 from about `29.5M` to about `0.52M` and raised RSS. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Conservative min/max negative filter skips Toy/tiny MidPage-map probes (`addr_envelope_skip=366` on 16..256 diagnostic) and improved non-target guards in first repeat-3, but did not help the 4096..16384 target (`addr_envelope_skip=0`, slight target regression). Keep as control, not selected. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` | Prefer same-class entry as overwrite victim when the probe window is full. Dry-run found candidates, but behavior regressed 1024..4096 and 4096..16384 in the first repeat-3. Keep off. |
| `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` | Trusted local-free MidPage activation can skip source-block bounds validation safely in principle, but first focused repeat-5 did not improve target or tiny guard. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | Direct MidPage active-map free success cache path. It removes the generic cache helper call but did not improve the target and regressed tiny guard, so keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | MidPage direct-local reuse skips the transfer-first probe. It is a useful target witness, but the tested helper shape regressed non-MidPage guards enough to block selection. |
| `build_hz6_preload_midpage_target.sh` | Named MidPage-target DSO alias for the selected preload-boundary transfer-skip shape. |
| `build_hz6_preload_toy_target.sh` | Named Toy/mid-small target DSO. It keeps selected default intact and adds only the Toy direct-class boundary helper for workloads where the mid-small win is more important than MidPage/fixed guard balance. |
| `build_hz6_preload_midpage_boundary_control.sh` | Explicit boundary-off control DSO for confirming the selected preload-boundary transfer-skip shape. |
| `run_hz6_preload_midpage_boundary_ab.sh` | Repeat runner for selected default versus boundary-off control on `4096..16384`, `16..256`, `16..4096`, and `1024..4096`. |
| `run_hz6_preload_toy_target_ab.sh` | Repeat runner for direct selected DSO versus Toy/mid-small target DSO on focused and fixed rows. Use this to validate the profile DSO without changing selected default flags. Shared compare aliases `hz6-toy-target` / `hz6_toy_target` are smoke-tested through `run_linux_bench_compare_matrix.sh` in raw `hz6_toy_target_matrix_alias_smoke_20260615_220832`. |
| `run_hz6_ubuntu_selected_balance_matrix.sh` | Cross-allocator speed/RSS balance matrix for selected HZ6 versus system/HZ3/HZ4/HZ5/mimalloc/tcmalloc. |
| `build_hz6_preload_diag.sh` | Diagnostic preload build wrapper with `HZ6_DIAGNOSTIC_PROBES=1`; also preserves preload phase counters despite production selected compile-out. Use for attribution, not selected speed ranking. |
| `run_hz6_midpage_rss_audit.sh` | Diagnostic RSS attribution runner for `16..4096`, `1024..4096`, and `4096..16384`. |
| `run_hz6_midpage_supply_map_ab.sh` | Focused A/B runner for 8K run widening, supply/frontcache, MidPage 8K->32K borrow, SourceRunReuse controls, and MidPage active-map capacity/probe controls. Supports `--variants`, `--include-tiny`, `--diagnostics`, `--stats`, and `--no-stats`; use diagnostics for class-detail/borrow attribution and `--no-stats --no-diagnostics` for production-shape speed/RSS ranking. |
| `run_hz6_midpage_payload_trim_ab.sh` | Focused A/B runner for 32K run-size payload/supply controls, including the run2048 fine ladder and phase-count compile-out controls. Supports `--variants`, `--include-tiny`, `--stats`, and `--no-stats`; use `--no-stats` for speed/RSS promotion gates and `--stats` for fail/source attribution. |
| `run_hz6_preload_free_order_ab.sh` | Focused A/B runner for preload free-order controls: selected, unconditional MidPage-first, aligned-first, current-bias 1x/2x/4x, delta64, and phase-count controls. |
| `run_hz6_static_table_trim_ab.sh` | Builds selected trim and wide-table controls, then compares speed/RSS plus failure counters. |
| `run_hz6_midpage_payload_trim_ab.sh` | Builds MidPage 32K run-size controls and compares selected speed/RSS plus source/failure counters. |
| `run_hz6_frontcache_shape_ab.sh` | Builds selected, class-specific MidPage frontcache cap controls, and storage-trim control. Use default `--stats --diagnostics` for attribution; use `--no-stats --no-diagnostics` for production-shape speed/RSS ranking. |
| MidPage noinline/branch-isolated transfer skip | Still guard-sensitive. Branch/layout isolation did not make it selected-safe, so do not add it to `build_hz6_preload.sh`. |
| MidPage preclassified malloc shape | Direct 4097..32768 MidPage classification improved target in short repeat, but disturbed `16..256` too much. Avoid broad malloc code-shape changes unless small guards are isolated first. |
| active-map slot-index/code-shape helper | No selected-row win; changing this header shape can disturb MidPage/Toy preload layout, so keep the existing body. |
| `HZ6_LINUX_MMAP_RETAIN_TLS_L1=1` | Did not reduce mmap count and regressed repeat-3. |
| `HZ6_SOURCE_RUN_REUSE_L1=1` | Reduced source allocation count but reusable-run scan/activation cost dominated. |
| `HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1` | Guard-safe but did not produce target candidates after run2048; keep as a narrow control, not default. |
| Toy source blocks 128K/256K | Raised RSS and regressed throughput. |
| `HZ6_ROUTE_PACKED_META_L1=1` | Lowered RSS slightly but was slower/less stable than unpacked route metadata. |
| aggressive route tombstone compact | Too costly; keep normal compact only. |
| `HZ6_PRELOAD_FAST_FREE_L1=1` | Prechecked-route reuse did not improve mixed_ws; route registration remained dominant. |

## Diagnostic Use

Use stats only for attribution, not speed ranking:

```bash
HZ6_PRELOAD_STATS=1 \
LD_PRELOAD=$PWD/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so \
./bench/out/linux/x86_64/bench_mixed_ws_crt 4 500000 4096 4096 16384
```

Useful lines:

```text
[HZ6_PRELOAD_STATS]
[HZ6_PRELOAD_ROUTE_DETAIL]
[HZ6_PRELOAD_FRONT_DETAIL]
[HZ6_PRELOAD_PHASE_STATS]
[HZ6_PRELOAD_SIZE_STATS]
```

`[HZ6_PRELOAD_SIZE_STATS]` is the 1024..4096 boundary audit line. It is
diagnostic-only and splits malloc/realloc requests into `<=1024`,
`1025..4096`, `4097..16384`, and `>16384` buckets, plus owned old-size
realloc buckets and realloc copy calls. Use it with the existing route/source
and phase lines before changing Toy/MidPage boundary behavior.

Use diagnostic builds when detailed probe counters are needed:

```bash
./hakozuna-hz6/linux/build_hz6_preload_diag.sh
```

Use the MidPage RSS audit runner to split selected peak RSS pressure:

```bash
./hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh --iters 200000
```

The diagnostic preload emits one additional attribution line:

```text
[HZ6_PRELOAD_MEMORY_ATTR]
```

Important caveat:

```text
preload_attributed_bytes is an attribution estimate, not exact RSS.
source_block_payload_bytes is logical source backing capacity and can exceed
resident pages. Use the line to choose the next lane, not as a replacement for
peak_kb from the benchmark.
```

## Raw Evidence

Recent raw evidence directories:

```text
private/raw-results/linux/hz6_midpage_rss_audit_20260614_164214
private/raw-results/linux/hz6_static_table_trim_ab_20260614_164920
private/raw-results/linux/hz6_static_table_trim_confirm_20260614_165003
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
private/raw-results/linux/hz6_midpage_payload_trim_ab_20260614_194352
private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_162527
private/raw-results/linux/hz6_preload_activefast_cross_20260613
private/raw-results/linux/hz6_preload_midrun_ladder_r3_20260613
private/raw-results/linux/hz6_preload_midrun_default256_guard_20260613
private/raw-results/linux/hz6_preload_midmap_route_cap_small_r3_20260613
private/raw-results/linux/hz6_preload_midmap_selected_guard_20260613
private/raw-results/linux/hz6_preload_midmap_external_control_r7_20260613
private/raw-results/linux/hz6_preload_midmap_external_diag_20260613
private/raw-results/linux/hz6_preload_realloc_audit_20260613
private/raw-results/linux/hz6_preload_realloc_inplace_ab_r5_20260613
private/raw-results/linux/hz6_preload_realloc_cross_r5_20260613
private/raw-results/linux/hz6_preload_vs_hz3_hz4_r5_20260613
private/raw-results/linux/hz6_midpage_hz4close_diag_20260613
private/raw-results/linux/hz6_midpage_unaligned_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_hz4_guard_r5_20260613
private/raw-results/linux/hz6_toy_high_alloc_path_diag_20260613
private/raw-results/linux/hz6_toy_active_map_tune_r5_20260613
private/raw-results/linux/hz6_toy_active_map_cap16_r5_20260613
private/raw-results/linux/hz6_clean_rebuild_default_matrix_guard_r5_20260613
private/raw-results/linux/hz6_preload_1024_4096_boundary_audit_20260613
private/raw-results/linux/hz6_toy_preclassified_malloc_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_matrix_r5_20260613
private/raw-results/linux/hz6_direct_local_reuse_rawpop_r5_20260613
private/raw-results/linux/hz6_toy_active_map_free_fastslot_r5_20260613
private/raw-results/linux/hz6_preload_selected_cross_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_focus_r9_20260613
private/raw-results/linux/hz6_preload_selected_cross_actskip_r5_20260613
private/raw-results/linux/hz6_toy_run_local_meta_diag_20260613
private/raw-results/linux/hz6_smallrun_route_behavior_toy_r5_20260613
```

Storage rule:

```text
private/raw-results/linux:
  raw local evidence, not paper-facing curated output

HZ6_UBUNTU_PRELOAD_LANES.md:
  selected/default/control/no-go summary only

current_task.md:
  detailed chronological experiment ledger
```
