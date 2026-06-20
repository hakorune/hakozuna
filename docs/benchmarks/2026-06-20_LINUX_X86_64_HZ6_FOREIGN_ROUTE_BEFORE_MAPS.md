# Linux x86_64 HZ6 Foreign Route Before Maps, 2026-06-20

## Box

`PreloadForeignRouteBeforeMaps-L1`

## Boundary

Default-off flag:

```text
HZ6_PRELOAD_FOREIGN_ROUTE_BEFORE_MAPS_L1=1
```

The preload `free()` hook resolves the route before active-map probes, but
consumes only authoritative foreign-valid routes:

```text
FOREIGN_VALID + visible route:
  dispatch with hz6_free_with_resolved_route_after_maps()

anything else:
  continue through the selected active-map-first free path
```

This does not change route ownership, descriptor state, pending queues, transfer
caches, or real-free policy.  It is a free-dispatch shape box.

## Results

Production-shaped R10:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_r10_20260620_190239`

| row | toy2_split base | route-before-maps | Decision |
|---|---:|---:|---|
| `remote50` | 13.59M | 14.36M | improved |
| `remote90` | 11.42M | 11.34M | roughly flat / slight regression |
| `cross128_r90` | 5.93M | 15.67M | improved |

TLS-armed R10:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_armed_tls_r10_20260620_191029`

| row | toy2_split base | TLS-armed route-before-maps | Decision |
|---|---:|---:|---|
| `local0` | 16.09M | 16.71M | improved / no local tax |
| `remote50` | 14.11M | 14.24M | roughly flat / slight improvement |
| `remote90` | 11.27M | 11.09M | slight regression |
| `cross128_r90` | 11.77M | 18.52M | improved |

Local guard R3:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_local0_r3_20260620_190425`

| row | toy2_split base | route-before-maps | Decision |
|---|---:|---:|---|
| `local0` | 16.21M | 15.50M | regressed |

Stats R1:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_stats_r1_20260620_190208`
- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_armed_tls_stats_r1_20260620_190937`

| row | before-map attempts | early foreign dispatch | after-map route lookup |
|---|---:|---:|---:|
| `remote50` | 80,462 | 39,716 | 1,329 |
| `remote90` | 960,452 | 863,622 | 74,064 |
| `cross128_r90` | 960,452 | 857,621 | 59,814 |

Follow-up observation:

`PreloadRouteBeforeMapsLocalDispatch-L1` keeps the same TLS-armed boundary, but
also consumes local-valid resolver proof before active-map probes when the
resolver did not find a visible active-map hit:

```text
LOCAL_VALID + valid route + visible_hit == 0:
  dispatch with hz6_free_with_resolved_route_after_maps()
```

Stats R1:

- `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_local_dispatch_stats_r1_20260620_191635`

| row | before-map attempts | early foreign dispatch | early local dispatch | fallback | fallback proven external | after-map route lookup |
|---|---:|---:|---:|---:|---:|---:|
| `local0` | 0 | 0 | 0 | 0 | 0 | 81 |
| `remote50` | 80,302 | 39,697 | 40,605 | 0 | 0 | 19 |
| `remote90` | 960,421 | 859,777 | 96,191 | 4,453 | 4,453 | 4,473 |
| `cross128_r90` | 960,327 | 850,617 | 95,276 | 14,434 | 14,434 | 14,453 |

This removes most remaining after-map route lookups in mixed remote rows, but
the stats build is intentionally not used for performance judgment.

Production-shaped R10:

- `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_local_dispatch_r10_20260620_191859`

| row | toy2_split base | local-dispatch route-before-maps | Decision |
|---|---:|---:|---|
| `local0` | 16.29M | 16.51M | improved / no local tax |
| `remote50` | 13.87M | 14.10M | improved |
| `remote90` | 11.23M | 11.46M | improved |
| `cross128_r90` | 10.78M | 28.59M | improved |

The local-valid branch removes the remaining mixed-row route lookup tail without
losing the TLS-armed cross128 gain in this paired run.

## Decision

```text
PreloadForeignRouteBeforeMaps-L1:
  GO(profile candidate)
  HOLD(default)

PreloadForeignRouteBeforeMapsArmed-L1:
  GO(explicit cross128 profile candidate)
  HOLD(default)

PreloadRouteBeforeMapsLocalDispatch-L1:
  GO(explicit profile candidate)
  HOLD(default)
```

The box directly attacks the tiny cross-owner free-path work count by consuming
foreign resolver proof before active-map probes.  It is strong on
`cross128_r90` and does not hurt `remote50` in the paired R10.  TLS arming
removes the local0 tax, and local-valid dispatch recovers the remaining
`remote90` tax in the paired production run.  This is now the shape used by the
explicit profile candidate, but it remains separate from selected/default until
broader allocator comparisons and cross-platform checks are done.

Explicit alias:

```text
hz6-cross128-toy2-route-before-maps-target
```

## Checks

- `./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh`
- default `./hakozuna-hz6/linux/build_hz6_preload.sh`
- default `./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh`
- `./hakozuna-hz6/linux/build_hz6_preload_cross128_toy2_route_before_maps_target.sh`

## Next

The next box should preserve this explicit profile shape and avoid stacking
additional broad free-path changes without fresh A/B evidence.
Candidates:

```text
Cross128ProfileRouteBeforeMaps-L1
  keep as explicit profile and compare against allocator frontier

SelectedDefaultRouteBeforeMaps-L1
  HOLD until Linux x86_64, Windows x64, and local/guard rows all pass
```

Do not enable route-before-maps unconditionally in the selected/default preload
lane without a separate default-promotion box.
