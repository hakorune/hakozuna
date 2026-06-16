# HZ6 Ubuntu Profile Registry

Short registry for Ubuntu LD_PRELOAD profile DSOs. This file separates
standard profile-frontier entries from explicit controls so runner defaults
stay intentional.

## Selected

| Alias | Builder | Use |
| --- | --- | --- |
| `hz6` | `linux/build_hz6_preload.sh` | Selected speed/RSS balance default. |

## Standard Profile Frontier

These aliases are included by default in
`linux/run_hz6_preload_profile_frontier.sh`.

| Alias | Builder | Use |
| --- | --- | --- |
| `hz6-toy-trusted-target` | `build_hz6_preload_toy_trusted_target.sh` | Light small/fixed16 comparison profile. |
| `hz6-small-boundary-trusted-target` | `build_hz6_preload_small_boundary_trusted_target.sh` | Broad small/fixed-boundary profile. |
| `hz6-realloc-boundary-4k-target` | `build_hz6_preload_realloc_boundary_4k_target.sh` | Exact 4K realloc-growth profile. |
| `hz6-realloc-boundary-8k-target` | `build_hz6_preload_realloc_boundary_8k_target.sh` | Exact 8K realloc-growth profile. |
| `hz6-realloc-boundary-adaptive-4k-target` | `build_hz6_preload_realloc_boundary_adaptive_4k_target.sh` | Adaptive 4K boundary profile. |
| `hz6-realloc-boundary-adaptive-8k-target` | `build_hz6_preload_realloc_boundary_adaptive_8k_target.sh` | Adaptive 8K boundary profile. |
| `hz6-realloc-boundary-adaptive-target` | `build_hz6_preload_realloc_boundary_adaptive_target.sh` | Combined adaptive boundary profile. |
| `hz6-aligned-target` | `build_hz6_preload_aligned_target.sh` | Aligned-allocation profile/control. |
| `hz6-calloc-direct-target` | `build_hz6_preload_calloc_direct_target.sh` | Calloc code-shape control. |
| `hz6-calloc-real-target` | `build_hz6_preload_calloc_real_target.sh` | Real-calloc fallback profile/control. |
| `hz6-calloc-large-real-target` | `build_hz6_preload_calloc_large_real_target.sh` | Large-calloc RSS/speed profile. |

## Explicit Controls

These aliases are intentionally excluded from the default profile frontier.
Use them by passing `--allocators`.

| Alias | Builder | Reason |
| --- | --- | --- |
| `hz6-small-boundary-trusted-toy-map8192-target` | `build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh` | Speed-leaning fixed-boundary RSS profile; strongest on fixed_16k in the latest gap matrix, but mixed-small rows regress. |
| `hz6-small-boundary-trusted-toy-map8192-external-target` | `build_hz6_preload_small_boundary_trusted_toy_map8192_external_target.sh` | Lower-RSS fixed-boundary profile; latest fixed-gap refresh beats tcmalloc balance on fixed rows, but mixed-small rows remain weak. |
| `hz6-small-boundary-trusted-toy-map8192-external-meta-off-target` | `build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_target.sh` | Narrow fixed-boundary RSS control; combines external Toy-map8192 with SourceBlockMetaSlim-L1 for clean A/B, not a default frontier entry. |
| `hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target` | `build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target.sh` | Narrow fixed-boundary RSS control; adds route16K static-floor trim to external meta-off after stats/prod fixed-row guards stayed clean. |
| `hz6-toy-map-external-target` | `build_hz6_preload_toy_map_external_target.sh` | RSS profile/control; cuts Toy active-map fixed storage but speed is mixed. |
| `hz6-workload-capacity-lite-target` | `build_hz6_preload_workload_capacity_lite_target.sh` | Workload-proxy capacity ladder; route64K/descriptors16K/source2K. |
| `hz6-workload-capacity-narrow-target` | `build_hz6_preload_workload_capacity_narrow_target.sh` | Workload-capacity narrow control; route40K/descriptors10K/source1280 without elastic descriptor overflow. |
| `hz6-workload-capacity-hybrid-target` | `build_hz6_preload_workload_capacity_hybrid_target.sh` | Preferred workload-capacity hybrid recommendation; route40K/descriptors10K/source1280 plus 1024-slot elastic descriptor depot. |
| `hz6-workload-capacity-lite-map8192-target` | `build_hz6_preload_workload_capacity_lite_map8192_target.sh` | Workload-capacity RSS/balance control; lite capacity plus Toy active-map 8192. |
| `hz6-workload-capacity-lean-target` | `build_hz6_preload_workload_capacity_lean_target.sh` | Workload-capacity bridge profile between lite and plus; route73728/descriptors18432/source2304. |
| `hz6-workload-capacity-plus-target` | `build_hz6_preload_workload_capacity_plus_target.sh` | Workload-capacity high-live-set probe between lite and mid; route80K/descriptors20K/source2560. |
| `hz6-workload-descriptor-overflow-target` | `build_hz6_preload_workload_descriptor_overflow_target.sh` | Workload descriptor-pressure control; selected static tables plus 2048-slot elastic descriptor depot. |
| `hz6-workload-descriptor-hybrid-target` | `build_hz6_preload_workload_descriptor_hybrid_target.sh` | Legacy spelling of the workload-capacity hybrid shape; kept for archived matrices and direct A/B compatibility. |
| `hz6-source-run-meta-off-target` | `build_hz6_preload_source_run_meta_off_target.sh` | SourceBlockMetaSlim control; selected flags with inline source-run metadata compiled out. |
| `hz6-workload-capacity-mid-target` | `build_hz6_preload_workload_capacity_mid_target.sh` | Workload-proxy capacity ladder; route96K/descriptors24K/source3K. |
| `hz6-workload-capacity-target` | `build_hz6_preload_workload_capacity_target.sh` | Workload-proxy capacity profile; widens route/descriptors/source blocks for larger working sets. |
| `hz6-midpage-skip-transfer-target` | `build_hz6_preload_midpage_skip_transfer_target.sh` | Watch/control only; latest frontier regressed `16..4096` and `4096..16384`. |
| `hz6-small-boundary-target` | `build_hz6_preload_small_boundary_target.sh` | Superseded by trusted profile for broad use. |
| `hz6-small-boundary-fast-target` | `build_hz6_preload_small_boundary_fast_target.sh` | Raw-push component remains default-off. |
| `hz6-midpage-trusted-class` | `build_hz6_preload_midpage_trusted_class_target.sh` | Historical comparison alias for selected trusted-class shape. |

## Read Rules

```text
selected/default:
  only linux/hz6_preload_flags.sh and build_hz6_preload.sh

standard profile frontier:
  broad periodic comparison set
  must stay reasonably small

explicit controls:
  useful evidence or workload-specific probes
  do not promote without focused/fixed/stats/RSS/cross-allocator guards
```

## Broad Guard

```sh
./hakozuna-hz6/linux/run_hz6_broad_guard.sh
```

This orchestration runner composes the standard profile frontier, fixed-gap
matrix, and workload-proxy matrix into one raw-result root. The workload leg
uses the current explicit workload controls (`capacity-narrow` and
`capacity-hybrid`) rather than older capacity-lite by default. Use it before
promoting a profile/default change; it does not add new benchmark semantics.

## Consistency Check

```sh
./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh
```

This verifies that the standard registry aliases match
`run_hz6_preload_profile_frontier.sh` and the broad guard profile leg, explicit
controls are excluded from the default frontier, broad guard fixed/workload HZ6
aliases are registered, builders exist, and aliases resolve through both the
HZ6 autobuild helper and `bench/lib/bench_common.sh`.
