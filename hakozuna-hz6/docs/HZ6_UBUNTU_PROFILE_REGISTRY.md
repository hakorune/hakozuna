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
| `hz6-small-boundary-trusted-toy-map8192-target` | `build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh` | Best fixed-boundary RSS profile, but mixed-small rows regress. |
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

## Consistency Check

```sh
./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh
```

This verifies that the standard registry aliases match
`run_hz6_preload_profile_frontier.sh`, explicit controls are excluded from the
default frontier, builders exist, and aliases resolve through both the HZ6
autobuild helper and `bench/lib/bench_common.sh`.
