# Hakozuna HZ5 Source Map

This map names the current HZ5 implementation layers after the P45dr closeout.
Use it as a navigation guide before adding another lane or moving code.

## Current Lowpage64 Path

These files are the active 64K/a8192 path used by the P25/P33/P43i/P45 family.

* `policy/hz5_policy.c`: public allocation/free dispatch, wrapper decode,
  lazy HZ3 fallback decision, and the HZ5 owned/nonactive gate. This layer must
  never send `OWNED_NONACTIVE` HZ5 pointers to HZ3 fallback free.
* `wrapper/hz5_wrapper.c`: interior-pointer wrapper header/cookie
  encode/decode. The historical `P25_HZ4LOWPAGE` wrapper source tag means the
  HZ5 lowpage64 bridge route.
* `lowpage/hz5_lowpage64.c`: P25/P33 bridge speed layer. It owns TLS relbuf,
  global batch, acquired stash, P40 checkpoint/release policy, prepared bridge,
  P43o/P43p/P45 diagnostics, and BRIDGE_COLD/stage1 evidence queues.
* `lowpage/hz5_lowpage64_p43_segment.c`: P43 segment-slot source layer. P43i and
  P45 keep this below the P25 bridge. Direct descriptor release, slot decommit,
  `PAGE_NOACCESS`, and runtime segment release are evidence or future
  RSS-return contracts, not the current speed-clean contract.
* `fallback/hz5_hz3_fallback.c`: lazy HZ3 fallback DLL loader and fallback free
  path. Exact HZ5 routes should keep fallback unloaded.
* `route/hz5_route.c`: size/alignment routing into HZ5 lowpage64 versus
  fallback/unsupported paths.
* `contract/hz5_contract.c`: SpeedLane and diagnostic-lane contract descriptor.

## Experimental Core

These files contain the earlier page/run-first HZ5 core. They are useful for the
longer HZ5 direction, but the current P43i/P45 lowpage64 work is centered on
the `lowpage/` bridge/source split above.

* `core/hz5_tcache.c`: thread-local cache prototype.
* `core/hz5_run.c`: run-level allocation/free prototype.
* `core/hz5_segment.c`: segment metadata and bitmap prototype.
* `core/hz5_owner.c`: owner token and shutdown/destructor logic.
* `core/hz5_remote.c`: remote-free queue/drain prototype.
* `core/hz5_stats.c`: core diagnostic statistics.

## Design Notes

* `docs/HZ5_CONTROL_PLANE_DESIGN.md`: post-P45 control-plane design. P25 bridge
  is the speed layer, P43 segment slots are the source layer, P40 release is
  source-demotion intent, and OPEN/DRAIN/CLOSED admission is the control plane.
* `docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md`: Linux-specific route/lane/benchmark
  classification. Use it before naming a new HZ5 Linux result or paper claim.
* `docs/HZ5_P43I_P43O_ALGO_CONSULT.md`: historical consultation ledger for
  P43i/P43o/P43p/P45. Use it as evidence, not as the current implementation map.
* `docs/source-map.md`: this file.

## Current Rule Of Thumb

```text
P43i:
  selected balanced candidate-watch

P45r1 / P45dr:
  mechanism and retention evidence
  not promotion
  not SpeedLane

Next cleanup:
  make bridge/source/control-plane boundaries easier to read
  avoid new behavior knobs unless the control-plane design changes
```
