# HZ5 Source Cleanup Plan

This note tracks cleanup targets without changing allocator behavior. Use it
before splitting active hot-path files.

## Rule

```text
Do not refactor active hot paths while a benchmark hypothesis is still open.
Split code only when the boundary is stable enough to preserve measurement
comparability.
```

## Current Large Files

| File | Current role | Cleanup status |
| --- | --- | --- |
| `largefront/hz5_largefront.c` | active LargeFront 128K tcmalloc-chase surface | now a thin wrapper over include-local LargeFront fragments |
| `largefront/hz5_largefront_config.inc` | LargeFront compile-time lane defaults | split out from `hz5_largefront.c`; include-local to keep benchmark knobs centralized |
| `largefront/hz5_largefront_state.inc` | LargeFront types, global storage, TLS, and forward declarations | split out from `hz5_largefront.c`; include-local while active lanes remain open |
| `largefront/hz5_largefront_lists.inc` | LargeFront local/global free-list and remote-hold helpers | split out from `hz5_largefront.c`; include-local while list policy settles |
| `largefront/hz5_largefront_remote_publish.inc` | LargeFront owner inbox publish and remote batch helpers | split out from `hz5_largefront.c`; include-local while remote inbox policy settles |
| `largefront/hz5_largefront_source.inc` | LargeFront source refill / region / lookup helpers | split out from `hz5_largefront.c`; keep include-local while the source path settles |
| `largefront/hz5_largefront_remote_drain.inc` | LargeFront owner inbox drain and ownerhub drain helpers | split out from `hz5_largefront.c`; include-local while the remote drain policy settles |
| `largefront/hz5_largefront_alloc_free.inc` | LargeFront public alloc/free and tail helpers | split out from `hz5_largefront.c`; keep include-local while the public surface remains stable |
| `largefront/hz5_largefront_transfer128.inc` | isolated LargeFront L9 transfer128 diagnostic helpers | split out from `hz5_largefront.c`; keep include-local until the diagnostic stabilizes |
| `largefront/hz5_largefront_policy.inc` | LargeFront L0/L1 observe counters and policy selectors | split out from `hz5_largefront.c`; include-local to avoid link/build churn |
| `midpagefront/hz5_midpagefront.c` | active PageRun64/MidPage history and saved profile implementation | defer behavior split; `hz5_midpagefront_tail.inc` now holds the public API tail while the saved profile remains active |
| `midpagefront/hz5_midpagefront_config.inc` | MidPageFront compile-time lane defaults and compatibility checks | split out from `hz5_midpagefront.c`; include-local to avoid preprocessor/link churn |
| `midpagefront/hz5_midpagefront_state.inc` | MidPageFront internal types, global maps, and TLS state | split out from `hz5_midpagefront.c`; include-local while active lanes remain open |
| `midpagefront/hz5_midpagefront_stats.inc` | MidPage nodeless/M4 cold stats helpers | split out from `hz5_midpagefront.c`; keep include-local until the diagnostics stabilize |
| `midpagefront/hz5_midpagefront_m4_state.inc` | MidPage M4 slot-state/counter helper cluster | split out from `hz5_midpagefront.c`; keep include-local while the M4 magazine lane settles |
| `midpagefront/hz5_midpagefront_m4_magazine.inc` | MidPage M4 local magazine/overflow helper cluster | split out from `hz5_midpagefront.c`; keep include-local while the M4 magazine lane settles |
| `midpagefront/hz5_midpagefront_empty_release.inc` | MidPage empty-slab release and source-free helper cluster | split out from `hz5_midpagefront.c`; keep include-local while empty release policy settles |
| `midpagefront/hz5_midpagefront_source_pages.inc` | MidPage region source refill and new-page initialization helpers | split out from `hz5_midpagefront.c`; keep include-local while source policy settles |
| `midpagefront/hz5_midpagefront_m4_pagerun.inc` | MidPage M4 remote packet and PageRun helper cluster | split out from `hz5_midpagefront.c`; keep include-local while the pagerun profile settles |
| `midpagefront/hz5_midpagefront_pagerun_alloc.inc` | MidPage PageRun refill/current allocation helpers | split out from `hz5_midpagefront.c`; keep include-local while the pagerun hot profile settles |
| `midpagefront/hz5_midpagefront_nodeless.inc` | MidPage nodeless partial / ptrcache / hot-slot helpers | split out from `hz5_midpagefront.c`; keep include-local while the nodeless diagnostic lane settles |
| `midpagefront/hz5_midpagefront_nodeless_alloc.inc` | MidPage nodeless refill/current allocation helpers | split out from `hz5_midpagefront.c`; keep include-local while the nodeless diagnostic lane settles |
| `midpagefront/hz5_midpagefront_region.inc` | MidPage class / region / slot lookup helpers | split out from `hz5_midpagefront.c`; keep include-local while the lookup path settles |
| `midpagefront/hz5_midpagefront_m4_refill.inc` | MidPage M4 magazine refill helper | split out from `hz5_midpagefront.c`; keep include-local while the M4 refill path settles |
| `midpagefront/hz5_midpagefront_remote_queue.inc` | MidPage local pop, owner publish, and remote batch helpers | split out from `hz5_midpagefront.c`; keep include-local while remote queue policy settles |
| `midpagefront/hz5_midpagefront_remote_drain.inc` | MidPage owner inbox drain helper | split out from `hz5_midpagefront.c`; keep include-local while drain policy settles |
| `midpagefront/hz5_midpagefront_remote_experiments.inc` | MidPage M6/M7/M8/M10/M11 remote experiment helpers | split out from `hz5_midpagefront.c`; keep include-local while the remote experiment lanes settle |
| `midfront/hz5_midfront_remote_batch.inc` | MidFront remote batch helper cluster | split out from `hz5_midfront.c`; keep include-local while the remote-batch profile settles |
| `midfront/hz5_midfront_tail.inc` | MidFront public API tail helpers | split out from `hz5_midfront.c`; keep include-local while the public surface remains stable |
| `preload/hz5_preload_full_support.inc` | Preload full bootstrap / stats / pointer-track support helpers | split out from `hz5_preload_full.c`; keep include-local while the full preload control path settles |
| `lowpage/hz5_lowpage64_p43o.inc` | LowPage P43O admission/projection diagnostics | split out from `hz5_lowpage64.c`; keep include-local while the admission probes settle |
| `lowpage/hz5_lowpage64_p43_segment_state.inc` | LowPage P43 segment types, globals, TLS cache, and counters | split out from `hz5_lowpage64_p43_segment.c`; keep include-local while the segment-slot source stabilizes |
| `lowpage/hz5_lowpage64_p43_segment_helpers.inc` | LowPage P43 segment lookup/slot-mask helper cluster | split out from `hz5_lowpage64_p43_segment.c`; keep include-local while the segment-slot source stabilizes |
| `lowpage/hz5_lowpage64_p43_segment_tail.inc` | LowPage P43 segment stats snapshot tail | split out from `hz5_lowpage64_p43_segment.c`; keep include-local while the snapshot schema remains active |
| `lowpage/hz5_lowpage64_p43p_bridge.inc` | LowPage P43P/P44/P45 bridge diagnostics | split out from `hz5_lowpage64.c`; keep include-local while the bridge probes settle |
| `lowpage/hz5_lowpage64_control.inc` | LowPage control-plane list / checkpoint / relbuf helpers | split out from `hz5_lowpage64.c`; `hz5_lowpage64_control_tail.inc` now holds the p37/p39/stash helper tail while the control-plane lanes settle |
| `lowpage/hz5_lowpage64_state_storage.inc` | LowPage global/TLS control-plane storage | split out from `hz5_lowpage64_control.inc`; keep include-local so state declarations stay near control helpers |
| `lowpage/hz5_lowpage64_stats_storage.inc` | LowPage diagnostic counter storage | split out from `hz5_lowpage64_control.inc`; keep include-local so counter users do not gain link churn |
| `lowpage/hz5_lowpage64_stats_print.inc` | LowPage diagnostic print include router | split out from `hz5_lowpage64.c`; keep include-local so the hot file stays below 1000 lines |
| `lowpage/hz5_lowpage64_stats_snapshot.inc` | LowPage snapshot diagnostic print helpers | split out from `hz5_lowpage64_stats_print.inc`; keep include-local while the diagnostic schema remains active |
| `lowpage/hz5_lowpage64_stats_once.inc` | LowPage atexit diagnostic print helpers | split out from `hz5_lowpage64_stats_print.inc`; keep include-local while the diagnostic schema remains active |
| `lowpage/hz5_lowpage64.c` | exact-route P25/P43/P45 historical hot path | do not touch during Linux general malloc work |
| `lowpage/hz5_lowpage64_p43g.inc` | LowPage P43g prepare/wrapper note helpers | split out from `hz5_lowpage64.c`; include-local to keep the hot file slimmer |
| `lowpage/hz5_lowpage64_p45dr.inc` | LowPage P45 stage1 drain diagnostics | split out from `hz5_lowpage64.c`; keep the cold diagnostic block include-local |
| `policy/hz5_policy.c` | exact-route wrapper/policy control | Local2P helpers moved to `hz5_policy_local2p.inc`; keep the main policy file slimmer |
| `policy/hz5_policy_tail.inc` | HZ5 policy public API tail helpers | split out from `hz5_policy.c`; keep include-local while the public surface remains stable |
| `policy/hz5_policy_local2p.inc` | Local2P helper cluster | split out from `hz5_policy.c`; include-local to keep policy control readable |
| `linux/build_linux_hz5_standalone.sh` | HZ5 Linux build/profile router | helperized repeated flag groups; keep alias names human-readable |

## Safe Near-Term Cleanup

```text
1. Keep docs short and archive long history files.
2. Keep build aliases human-readable.
3. Do not add runner aliases for one-off diagnostics unless measured more than
   once.
4. Prefer deleting or archiving no-go documentation before splitting active C.
5. When source split begins, start with cold diagnostics or policy observers,
   not allocation/free hot paths.
```

## Future Source Split Candidates

### LargeFront

Potential stable split after the current t4/r50 investigation:

```text
hz5_largefront.c:
  public alloc/free and hot path

hz5_largefront_source.c:
  source refill, region registration, source free list

hz5_largefront_remote.c:
  owner inbox, drain budget, remote batch diagnostics

hz5_largefront_policy.c:
  L0/L1 policy observation and selectors

hz5_largefront_map.c:
  page/base/region map helpers
```

### MidPageFront

Potential stable split after PageRun64 is frozen:

```text
hz5_midpagefront.c:
  public alloc/free and PageRun64 hot path

hz5_midpagefront_region.c:
  64MiB region array and page lookup

hz5_midpagefront_remote.c:
  remote packet/ticket historical diagnostics

hz5_midpagefront_magazine.c:
  M4/M5/M6 archived diagnostics if still kept
```

## Current Decision

For this cleanup phase, active C hot paths remain intact. Small diagnostic-only
helpers may be split into include-local files when that reduces churn in the
main implementation without changing build/link behavior. The current
LargeFront splits are `hz5_largefront_config.inc`, `hz5_largefront_state.inc`,
`hz5_largefront_lists.inc`, `hz5_largefront_remote_publish.inc`,
`hz5_largefront_source.inc`, `hz5_largefront_remote_drain.inc`,
`hz5_largefront_alloc_free.inc`,
`hz5_largefront_transfer128.inc`, and
`hz5_largefront_policy.inc`. MidPage now also keeps cold stats in
`hz5_midpagefront_config.inc`, `hz5_midpagefront_state.inc`,
`hz5_midpagefront_stats.inc`, plus the M4 slot-state helper cluster in
`hz5_midpagefront_m4_state.inc`, plus the M4 local magazine/overflow helper
cluster in `hz5_midpagefront_m4_magazine.inc`, plus the empty-slab release
cluster in `hz5_midpagefront_empty_release.inc`, plus the region source/new-page
helpers in `hz5_midpagefront_source_pages.inc`, plus the M4 remote/PageRun
cluster in `hz5_midpagefront_m4_pagerun.inc`, plus PageRun allocation helpers in
`hz5_midpagefront_pagerun_alloc.inc`, plus the nodeless diagnostic lane in
`hz5_midpagefront_nodeless.inc`, plus nodeless allocation helpers in
`hz5_midpagefront_nodeless_alloc.inc`, plus the class/region/slot lookup helpers in
`hz5_midpagefront_region.inc`, plus the M4 refill helper in
`hz5_midpagefront_m4_refill.inc`, plus remote queue/drain helpers in
`hz5_midpagefront_remote_queue.inc` and `hz5_midpagefront_remote_drain.inc`,
plus the M6/M7/M8/M10/M11 remote experiment lane in
`hz5_midpagefront_remote_experiments.inc`, and LowPage now keeps P43g helpers in
`hz5_lowpage64_p43g.inc`, P43O diagnostics in `hz5_lowpage64_p43o.inc`, plus
P43 segment lookup/slot-mask helpers in `hz5_lowpage64_p43_segment_helpers.inc`,
P43 segment state/counter storage in `hz5_lowpage64_p43_segment_state.inc`,
P43P/P44/P45 bridge diagnostics in `hz5_lowpage64_p43p_bridge.inc`, plus P45
drain diagnostics in `hz5_lowpage64_p45dr.inc`, plus lowpage control helpers in
`hz5_lowpage64_control.inc`, with global/TLS control-plane storage isolated in
`hz5_lowpage64_state_storage.inc`, diagnostic counter storage isolated in
`hz5_lowpage64_stats_storage.inc`, and diagnostic print helpers routed through
`hz5_lowpage64_stats_print.inc`, `hz5_lowpage64_stats_snapshot.inc`, and
`hz5_lowpage64_stats_once.inc`. Policy now keeps Local2P
helpers in `hz5_policy_local2p.inc`. MidFront now also keeps its remote batch
helpers in `hz5_midfront_remote_batch.inc`. Preload full now also keeps its
bootstrap/stat/pointer-track support helpers in
`hz5_preload_full_support.inc`. The Linux build script now centralizes the
repeated profile flag groups so the alias table stays readable.
