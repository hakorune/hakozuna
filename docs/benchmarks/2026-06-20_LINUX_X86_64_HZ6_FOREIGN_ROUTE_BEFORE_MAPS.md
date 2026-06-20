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

Local guard R3:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_local0_r3_20260620_190425`

| row | toy2_split base | route-before-maps | Decision |
|---|---:|---:|---|
| `local0` | 16.21M | 15.50M | regressed |

Stats R1:

- `hakozuna-hz6/private/raw-results/linux/hz6_foreign_route_before_maps_stats_r1_20260620_190208`

| row | before-map attempts | early foreign dispatch | after-map route lookup |
|---|---:|---:|---:|
| `remote50` | 80,462 | 39,716 | 1,329 |
| `remote90` | 960,452 | 863,622 | 74,064 |
| `cross128_r90` | 960,452 | 857,621 | 59,814 |

## Decision

```text
PreloadForeignRouteBeforeMaps-L1:
  GO(profile candidate)
  HOLD(default)
```

The box directly attacks the tiny cross-owner free-path work count by consuming
foreign resolver proof before active-map probes.  It is strong on
`cross128_r90` and does not hurt `remote50` in the paired R10, but the local0
tax means it should not be selected/default without an arming gate.

## Checks

- `./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh`
- default `./hakozuna-hz6/linux/build_hz6_preload.sh`
- default `./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh`

## Next

The next box should preserve the early-foreign win while avoiding local0 tax.
Candidates:

```text
RemotePressureArmedRouteBeforeMaps-L1
  arm only after recent foreign-visible resolver hits

Toy2ProfileRouteBeforeMaps-L1
  keep as an explicit high-remote/cross128 profile flag
```

Do not enable route-before-maps unconditionally in the selected/default preload
lane.
