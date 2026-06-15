# HZ6 Toy Trusted Default Snapshot - 2026-06-16

This archive records the evidence used to move the Toy trusted preload subset
from profile-only into the Ubuntu selected/default LD_PRELOAD bundle.

## Selected Additions

```text
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=4096
HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
```

The selected bundle does not include realloc-boundary slack or raw frontcache
push. Those remain profile/control lanes.

## Promotion Evidence

Focused HZ6-only A/B before selected flag promotion:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy_trusted_default_probe_20260616_030911
hakozuna-hz6/private/raw-results/linux/hz6_toy_trusted_default_probe_stats_20260616_030928
```

After selected flag promotion, the runner variant `toy_trusted_default_off`
recreates the previous selected-equivalent bundle:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy_trusted_default_on_ab_20260616_031042
hakozuna-hz6/private/raw-results/linux/hz6_toy_trusted_default_on_stats_20260616_031100
```

| row | old selected-equivalent | new selected | read |
| --- | ---: | ---: | --- |
| `16..256` | `50.044M` | `63.128M` | `+26.1%` |
| `16..4096` | `27.302M` | `30.688M` | `+12.4%` |
| `1024..4096` | `24.621M` | `27.789M` | `+12.9%` |
| `4096..16384` | `32.747M` | `32.742M` | flat |
| `fixed_4k` | `23.594M` | `25.570M` | `+8.4%` |
| `fixed_8k` | `30.899M` | `31.224M` | `+1.1%` |
| `fixed_16k` | `32.469M` | `32.580M` | `+0.3%` |

Stats/diagnostics stayed safety-clean with `fail=0` on the tested rows.

## Cross-Allocator Refresh

```text
hakozuna-hz6/private/raw-results/linux/hz6_selected_toy_trusted_cross_20260616_031120
hakozuna-hz6/private/raw-results/linux/hz6_selected_toy_trusted_fixed_20260616_031237
```

Focused rows:

| row | hz6 selected | position |
| --- | ---: | --- |
| `16..256` | `61.821M / 30.50 MiB` | below HZ3/HZ4/tcmalloc/system, above mimalloc |
| `16..4096` | `30.638M / 79.62 MiB` | below HZ3/HZ4/tcmalloc, above mimalloc/system/HZ5 |
| `1024..4096` | `28.197M / 91.00 MiB` | below HZ3/HZ4/tcmalloc, above mimalloc/system/HZ5 |
| `4096..16384` | `32.830M / 94.00 MiB` | below HZ3, above tcmalloc/HZ4/mimalloc/system |

Fixed rows:

| row | hz6 selected | position |
| --- | ---: | --- |
| `fixed_4k` | `27.638M / 91.62 MiB` | below HZ3/tcmalloc, above HZ4/mimalloc/system |
| `fixed_8k` | `33.097M / 93.25 MiB` | below HZ3, above tcmalloc/HZ4/mimalloc/system |
| `fixed_16k` | `34.039M / 93.12 MiB` | above HZ3/tcmalloc/HZ4/mimalloc/system |

## Decision

Promote the Toy trusted subset as selected/default because the same-run A/B is
large positive on tiny/mid-small/fixed_4k, flat on the MidPage target, and
safety-clean under stats/diagnostics. Keep small-boundary trusted and realloc
boundary DSOs as profile lanes for workloads that need fixed_4k/fixed_8k
realloc-growth behavior.
