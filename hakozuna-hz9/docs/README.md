# HZ9 Docs

HZ9 is a standalone experimental throughput line forked from the HZ8
KeepRefill baseline. HZ8 remains the public balanced line; HZ9 is for isolated
local-throughput experiments and must not depend on `../hakozuna-hz8` at build
or runtime.

```text
HZ9_MEDIUM_TLS_OBJECT_CACHE_L0.md:
  implemented object-cache evidence and HOLD decision

HZ9_CURRENT_STATUS.md:
  active lane, next implementation order, and verification commands

HZ9_SEGMENT_LOCAL_CACHE_L0.md:
  current design-prep lane, standalone segment metadata scaffold, remote
  drain/release boundaries, and local body API sweep

HZ9_LOCAL_SLAB_PAGE_L1.md:
  held 64K slab/page prototype, route boundary, counters, and blockers

HZ9_LOCAL_ARENA_L0.md:
  HZ9-owned LocalArena substrate, adaptive class cuts, and HOLD decision

HZ9_LOCAL_MAGAZINE_L0.md:
  first prototype design, measurements, and HOLD decision

HZ9_DIFFERENTIATION.md:
  why HZ9 is not HZ3/HZ8/tcmalloc/mimalloc

HZ9_STANDALONE_CLOSURE.md:
  standalone contract, worker audit result, source-shape splits, and allowed
  HZ8 naming debt

HZ9_NEXT_SUBSTRATE.md:
  active freeze boundary and requirements for the next local substrate

HZ9_POST_OWNER_PAGE_SUBSTRATE_CLOSURE_L1.md:
  post owner-page closure, closed lanes, and next-substrate requirements

HZ9_STATIC_LOCAL_PAGE_SCAFFOLD_L0.md:
  held source-shape box for static TLS state and owner-local plain bits

HZ9_DIRECT_SLAB_USE_PROOF_L0.md:
  completed proof for isolating SlabPage body cost from entry/route overhead;
  remote/profile evidence, not the selected next behavior

HZ9_FRESH_LOCAL_PAGE_SUBSTRATE_L0.md:
  post-SlabPage design-prep box for entry/integrated proof results and the
  next page substrate

HZ9_OWNER_LOCAL_PAGE_POOL_L0.md:
  owner-page substrate evidence: scaffold/shadow/API/page-lifetime proof,
  PureLocal L1 implementation contract, and HOLD decision

HZ9_LANE_HISTORY.md:
  compact archive of closed HZ9 boxes and detailed evidence reads

../README.md and ../src/README.md:
  standalone source-layout policy
```

Current lane:

```text
TLS object cache:
  HOLD evidence

SlabPage:
  profile/evidence only
  not default while non-target main/medium gates remain weak

LocalArena:
  HZ9-owned substrate evidence
  adaptive_min4 and remote-safe page modes are evidence only
  remote-safe page mode is early NO-GO after mixed local/remote atomic-page
  collapse on medium_r50/main_r90

next:
  HZ9_SEGMENT_LOCAL_CACHE_L0 is the current no-behavior source-shape box
  use HZ9_NEXT_SUBSTRATE.md as the next-behavior boundary before routing it
  entry-bypass, integrated SlabPage, route-off, and layout-neutral proofs are
  closed as evidence
  owner-page PureLocal L1 is implemented and held as profile/evidence
  owner-page ownerfast/disabled-fast variants are attribution only
  DirectSlabUse is complete and remains remote/profile evidence
  no active behavior box is selected until the next substrate avoids both
  remote admission cost and local owner-page overhead
  require local/main/small no-regression before any behavior promotion
```

Standalone commands:

```bash
make -C hakozuna-hz9 hz9-standalone-check
make -C hakozuna-hz9 smoke-hz9segmentlocalcache
ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_api_sweep.sh
ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_local_payload_sweep.sh
hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh
RUN_PROBE=1 hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh
hakozuna-hz9/scripts/run_hz9_local_entry_cost_probe.sh

RUNS=5 make -C hakozuna-hz9 hz9-candidate-gate
RUNS=10 hakozuna-hz9/scripts/run_hz9_slab_profile_gate.sh
RUNS=3 hakozuna-hz9/scripts/run_hz9_slab_shadow_probe.sh
RUNS=10 hakozuna-hz9/scripts/run_hz9_baseline_public_matrix.sh
RUNS=5 hakozuna-hz9/scripts/run_hz9_substrate_readiness.sh
RUNS_SIDECAR=3 hakozuna-hz9/scripts/run_hz9_post_owner_page_closure.sh
RUNS=3 hakozuna-hz9/scripts/run_hz9_entry_route_probe.sh
THREADS=4 ITERS=10000 hakozuna-hz9/scripts/run_hz9_substrate_mechanism_probe.sh
RUNS=1 hakozuna-hz9/scripts/run_hz9_local_entry_probe.sh
RUNS=3 VARIANTS=baseline,slabclasses_min0 \
  ROWS=main_local0,main_r90,medium_r50,medium_local0,small_interleaved_remote90 \
  hakozuna-hz9/scripts/run_hz9_candidate_gate.sh
MODE=slab hakozuna-hz9/scripts/run_hz9_code_shape_audit.sh
```
