# Linux x86_64 HZ6 Route-Before-Maps Cross128 Tail Stats, 2026-06-20

## Box

`RouteBeforeMapsCross128TailStats-L1`

Observation-only pass for the explicit
`hz6-cross128-toy2-route-before-maps-target` profile.  The goal is to explain
the remaining low tail in `cross128_r90` without changing behavior.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_cross128_tail_stats_r10_20260620_192907
```

Command shape:

```text
RUNS=10
ROW=cross128_r90
HZ6_PRELOAD_STATS=1
HZ6_DIAGNOSTIC_PROBES=1
HZ6_TOY_SMALL_HOTPATH_DIAG_L1=1
profile=owner_inbox_toy2_route_before_maps
```

## Counters

The diagnostic build is not used for production ops/s judgment.  It is used to
compare counter shape between faster and slower diagnostic runs.

| run | ops/s | before fallback | fallback proven external | after-map route lookup | pending inline drain | pending external drain | route lock max wait |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 456.83K | 253,210 | 253,210 | 253,232 | 401,619 | 93,660 | 22,608 |
| 2 | 324.46K | 400,851 | 400,851 | 400,871 | 325,411 | 41,466 | 18,861 |
| 3 | 329.69K | 396,978 | 396,978 | 396,999 | 330,012 | 39,455 | 22,394 |
| 4 | 338.52K | 389,336 | 389,336 | 389,356 | 327,044 | 52,216 | 23,827 |
| 5 | 328.92K | 399,231 | 399,231 | 399,251 | 307,905 | 58,965 | 68,983 |
| 6 | 333.96K | 386,722 | 386,722 | 386,740 | 309,210 | 68,191 | 140,160 |
| 7 | 358.66K | 400,077 | 400,077 | 400,095 | 314,243 | 53,462 | 22,703 |
| 8 | 339.88K | 394,377 | 394,377 | 394,395 | 309,853 | 60,693 | 15,495 |
| 9 | 328.80K | 400,472 | 400,470 | 400,492 | 316,813 | 50,678 | 22,230 |
| 10 | 333.59K | 390,613 | 390,613 | 390,636 | 304,584 | 69,590 | 25,347 |

Other notable counters:

```text
remote_free_returned_backpressure = 0 in every run
remote_free_backpressure_origin_transfer_full = 0 in every run
free_route_before_maps_attempt ~= 960K in every run
```

## Read

The remaining `cross128_r90` tail is not explained by returned backpressure in
this profile: the route-before-maps + owner-inbox path commits the free without
returning backpressure in all ten diagnostic runs.

The clearest tail signal is the amount of early resolver fallback classified as
`PROVEN_EXTERNAL`.  Those fallbacks almost exactly match the later
`free_route_lookup_after_maps` count.  The fastest diagnostic run had about
`253K` fallback/after-map lookups; most lower runs had about `390K-400K`.

This points at the next small box:

```text
PreloadRouteBeforeMapsExternalDispatch-L1
  consume PROVEN_EXTERNAL from the authoritative free resolver before active maps
  dispatch directly to real_free
  keep default-off and profile-only
  hard-gate free_route_real_free_unproven = 0
```

The correctness argument must be explicit: only `PROVEN_EXTERNAL` may dispatch
to platform `real_free`; `UNRESOLVED_INTEGRITY`, `OWNED_INVALID`, and `RETRY`
must continue to prohibit real-free.

## Decision

```text
RouteBeforeMapsCross128TailStats-L1:
  GO(observation)
  DESIGN checkpoint

PreloadRouteBeforeMapsExternalDispatch-L1:
  NO-GO(profile)
  HOLD(research flag)
```

Follow-up default-off implementation results:

- stats R3:
  `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_external_dispatch_stats_r3_20260620_194008`
- production R10:
  `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_external_dispatch_r10_20260620_194051`

Stats confirmed the mechanism: early external dispatch removed the
`PROVEN_EXTERNAL` fallback and kept `free_route_real_free_unproven=0`.

Production R10 rejected the profile shape for promotion:

| row | toy2_split base | external-dispatch route-before-maps | Decision |
|---|---:|---:|---|
| `local0` | 16.20M | 16.00M | roughly flat / slight regression |
| `remote50` | 13.82M | 13.94M | roughly flat |
| `remote90` | 11.34M | 4.15M | regressed hard |
| `cross128_r90` | 20.65M | 36.37M | improved |

Do not include external dispatch in `hz6-cross128-toy2-route-before-maps-target`.
The flag remains default-off for research only.

Follow-up attribution:

- diagnostic off/on matrix:
  `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_external_dispatch_diag_matrix_r3_20260620_194533`
- production off/on verify:
  `hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_external_dispatch_prod_verify_r5_20260620_194706`

The follow-up weakens the claim that external dispatch alone caused the
`remote90` collapse:

```text
diagnostic remote90:
  external_dispatch = 0 in both off/on builds
  ops/s roughly flat between off/on

production verify remote90:
  external off median = 4.20M
  external on  median = 3.97M
```

In the same session, the rebuilt `toy2_split` profile also measured only
`3.39M-4.05M` on `remote90`, so this was a weak remote90 batch/profile-state
signal rather than proof that `PROVEN_EXTERNAL` real-free directly destroys
remote90.

The conservative decision remains unchanged: do not include external dispatch in
the explicit route-before-maps profile.  The evidence supports it as a
cross128-only research switch, not a profile/default candidate.  The next
optimization should separate cross128 tail improvement from remote90 stability
with paired same-batch profile checks.
