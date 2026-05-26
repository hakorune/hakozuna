# Hakozuna HZ5 Source Map

This map names the current HZ5 implementation layers after the P45dr closeout.
Use it as a navigation guide before adding another lane or moving code.

Start here for the current Linux allocator status:

```text
docs/HZ5_LINUX_DEV_BRIEF.md
```

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
* `policy/hz5_policy_local2p_common.h`: cross-platform Local2P algorithm
  vocabulary shared by Linux and Windows Local2P lanes. It intentionally stops
  at common route/cache objects; OS raw allocation, lock primitives, and memory
  return policy stay in the platform-specific Local2P implementations.

## Cross-Platform Commonization Boundary

HZ5 should commonize algorithms before it commonizes OS mechanics. Keep this
split explicit:

```text
common:
  route intent
  wrapper/source tags
  ownership state vocabulary
  Local2P node/cache vocabulary
  trace counter names
  bridge/source/admission contract

platform-specific:
  mmap / VirtualAlloc / aligned allocation
  pthread mutex / SRWLOCK and thread-local storage details
  decommit / madvise / runtime release policy
  backend-specific fast paths
```

The first concrete step is `policy/hz5_policy_local2p_common.h`, which names
the shared Local2P freelist node, exact-route predicate, user-pointer
alignment/layout-fit checks, generation wrap handling, owner-token
initialization, header activation, and ACTIVE-to-FREED transition while leaving
Linux and Windows allocation sources untouched. Helpers that need Local2P
wrapper tail fields are compiled only when a Local2P lane enables that wrapper
layout, so P43i/P45 builds keep their smaller non-Local2P wrapper contract. The
same header also owns the small remote-batch state transitions (push/reset),
while Linux and Windows still keep their own lock primitive and flush
destination. The inbox CAS list splice itself is shared because both backends
use the same C11 atomic handoff shape.

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

## General Linux Front-End Direction

The Linux full-preload work changes HZ5 from an exact sidecar back toward a
general allocator. The current full-preload adapter is an attribution/control
lane; it proves ordinary `malloc` traffic can enter HZ5, but it is not the
final small-object allocator.

Current Linux full-preload front-end layers:

```text
smallfront/
  HZ5-SmallFront-S1
  Linux ordinary malloc/free <= 2048 bytes
  4KiB single-class pages
  descriptor-owned slots
  owner-local TLS free lists
  owner-aware remote inbox

midfront/
  HZ5-MidFront-M1
  Linux ordinary malloc/free 4096..65536 bytes
  one object per page/run-sized span in M1
  descriptor-owned spans
  owner-local TLS span caches
  owner-aware remote inbox

midpagefront/
  HZ5-MidPageFront-M2
  Linux ordinary malloc/free 2049..32768 bytes
  64KiB class-owned slabs
  descriptor-owned slots
  active bitmap validation
  owner/class remote inbox
  M2.2 region-array candidate uses 64MiB aligned source regions with a
  descriptor array indexed by ptr -> region/slab offset

largefront/
  HZ5-LargeFront-L1
  Linux ordinary malloc/free 65537..1048576 bytes
  128K/256K/512K/1M retained span classes
  descriptor-owned spans with page-map lookup
  owner-local retained lists and global remote recycle

ownerhub/
  HZ5-OwnerHub-R1
  Diagnostic owner-slot pending-mask observer
  Tracks cross-front remote backlog without changing allocation behavior
  HZ5-OwnerHub-R2/R3
  Coordinated cross-front drain candidates for remote-heavy mixed workloads

preload/
  full LD_PRELOAD adapter
  dispatches ordinary malloc/free into SmallFront, MidFront, LargeFront, then
  the fallback pointer table
  owns diagnostic dispatch-order lanes such as preload_free_midfirst

docs/HZ5_LINUX_LANE_CLEANUP.md
  Short lane cleanup source for canonical / diagnostic / archive preset status
  and build-script organization rules
```

Current MidFront lane naming:

```text
hz5-midfront-rb16:
  broad default candidate

hz5-midfront-allgate:
  mid-heavy remote candidate

hz5-midfront-globalrecycle:
  control lane

hz5-midfront-takefirst / hz5-midfront-maskhitstop:
  focused diagnostics only
```

Design source:

* `docs/HZ5_SMALLFRONT_S1_DESIGN.md`: HZ5-native small allocator front-end
  plan. It combines hz3-style size classes/TLS speed, hz4-style owner-aware
  remote handling, and HZ5 fail-closed descriptor ownership.
* `docs/HZ5_OWNER_LIFETIME_O1.md`: minimum Linux owner-lifetime hardening before
  adding more owner-aware front-ends.
* `docs/HZ5_MIDFRONT_M1_DESIGN.md`: HZ5-native mid allocator front-end plan for
  ordinary 4K..64K malloc traffic.
* `docs/HZ5_MIDPAGEFRONT_M2_DESIGN.md`: HZ5-native 64KiB class-owned slab
  prototype for ordinary 2049..32768 malloc traffic.
* `docs/HZ5_LARGEFRONT_L1_DESIGN.md`: HZ5-native large allocator front-end plan
  for ordinary >64K malloc traffic. L1 is the cross128 coverage fix, not the
  final 2MiB split/merge pool.
* `docs/HZ5_OWNERHUB_R1_DESIGN.md`: shared owner pending-mask observer and
  planned coordinated drain layer for cross-front remote-heavy workloads.
* `docs/HZ5_HZ4_REMOTE_PATH_AUDIT.md`: code-level HZ4/HZ5 remote-heavy path
  comparison and current MidFront dispatch-cost diagnostic.

Do not implement SmallFront by adding per-object wrappers to small objects or
by depending on the full-preload pointer table for HZ5-owned small frees.
Do not implement MidFront by routing ordinary malloc directly through the P2
hot path; P2 may become a slow source/provider after MidFront ownership is
stable.
Do not implement LargeFront-L1 by porting the full hz3/hz4 large split/merge
pool yet. Use MidFront-style descriptor ownership first, then add a page-run
pool only if broad measurements show RSS or size-mix pressure.

## Design Notes

* `docs/HZ5_LINUX_STATUS.md`: short active status dashboard for HZ5 Linux.
  Start here before continuing development.
* `docs/HZ5_BENCH_RESULTS_INDEX.md`: raw benchmark directory index and main
  decisions supported by recent active measurements. Full history lives under
  `docs/archive/HZ5_BENCH_RESULTS_INDEX_HISTORY_2026-05.md`.
* `docs/archive/current_task_2026-05-hz5-linux.md`: short current-status index.
  Full chronological history lives in
  `docs/archive/current_task_2026-05-hz5-linux_history.md`.
* `docs/HZ5_CONTROL_PLANE_DESIGN.md`: post-P45 control-plane design. P25 bridge
  is the speed layer, P43 segment slots are the source layer, P40 release is
  source-demotion intent, and OPEN/DRAIN/CLOSED admission is the control plane.
* `docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md`: Linux-specific route/lane/benchmark
  classification. Use it before naming a new HZ5 Linux result or paper claim.
  Full history lives under
  `docs/archive/HZ5_LINUX_ROUTE_LANE_MATRIX_HISTORY_2026-05.md`.
* `docs/HZ5_SMALLFRONT_S1_DESIGN.md`: Linux general allocator front-end plan for
  ordinary small malloc traffic.
* `docs/HZ5_OWNER_LIFETIME_O1.md`: Linux owner lifetime and orphan safety plan.
* `docs/HZ5_MIDFRONT_M1_DESIGN.md`: Linux general allocator front-end plan for
  ordinary 4K..64K malloc traffic.
* `docs/HZ5_LARGEFRONT_L1_DESIGN.md`: Linux general allocator front-end plan for
  ordinary >64K malloc traffic.
* `docs/HZ5_P43I_P43O_ALGO_CONSULT.md`: historical consultation ledger for
  P43i/P43o/P43p/P45. The current file is a short summary; full history lives
  under `docs/archive/HZ5_P43I_P43O_ALGO_CONSULT_HISTORY_2026-05.md`.
* `docs/HZ5_SOURCE_CLEANUP_PLAN.md`: cleanup boundary map for large active
  source files. Use it before splitting LargeFront or MidPageFront.
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
  keep OwnerLifetime-O1 hardening
  keep MidFront-M1 behind explicit selector lanes
  keep Local2P/P25/P43 exact lanes independent
  avoid new behavior knobs unless the route boundary changes
```
