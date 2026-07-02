# Hakozuna HZ9

HZ9 is a standalone experimental throughput-first allocator line derived from
HZ8.

HZ8 remains the public balanced low-RSS line. HZ9 exists to test whether HZ8's
fail-closed ownership, remote-free authority, and owner lifecycle contracts can
be preserved while improving local medium/main throughput with a separate local
cache architecture.

## Status

```text
state:
  standalone experimental implementation lane

default allocator:
  not HZ9

current public line:
  HZ8

current implementation base:
  copied from HZ8 KeepRefill balanced baseline

previous box:
  HZ9StandaloneSingleFolderClosure-L1
  closed enough for local HZ9 development

held evidence:
  HZ9MediumTLSObjectCache-L0
  HZ9MediumLocalMagazine-L0
  HZ9MediumLocalSlabPage-L1
  HZ9LocalArenaRemoteSeenActiveOnly-L1

current direction:
  HZ9LocalSlabPageRouteBoundary-L0 is the current design box
  SegmentEntry proved fast fused/token-local bodies, but public split
  malloc/free and route-push remain the blocker
  the next substrate must make free/usable_size/realloc share one O(1)
  address-derived fail-closed route authority
  StaticLocalPage is held as profile/local evidence
  OwnerPage / SlabPage / LocalArena lanes are held as profile/evidence
  HZ8 pending/qstate remains the remote authority

next evidence work:
  stop treating fused-only microbenchmarks as behavior evidence
  build the public split-boundary route scaffold before more local-cache tuning
  avoid TLS-cache pop plus public route-push shapes; they collapse to route-free
  speed
  keep H8OwnerRecord/H8ThreadCtx layout unchanged
  keep source/docs/scripts under the 800-line active-file limit
```

## Source Layout Policy

```text
hakozuna-hz9/:
  standalone Makefile / include / src / bench / tests / scripts

hakozuna-hz8/:
  frozen balanced public line
  no further HZ9 behavior work
```

HZ9 starts as a source copy of the HZ8 KeepRefill baseline so experiments can
reuse proven boundary code while changing the local architecture in isolation.

```text
reuse from HZ8:
  route / directory
  slot_state validity
  pending bitmap / qstate
  owner lifecycle
  owner-exit hard drain

change in HZ9:
  medium local entry
  TLS object cache
  future local slab/page substrate
  explicit higher-RSS cache contract
```

Inherited `h8_*` symbols and local artifact names are acceptable naming debt in
L0/L1 when they are built from `hakozuna-hz9/$(ROOT)`. They are not acceptable
if they become runtime or build dependencies on `../hakozuna-hz8`.

External allocator comparison artifacts are opt-in. By default, HZ9 scripts use
local HZ9 build products, system allocator, and platform-installed allocators
found through the loader cache or package manager. Sibling repository artifacts
must be provided explicitly through allocator-specific `*_SO` environment
variables.

## Standalone Quick Commands

Run from the parent checkout with `make -C hakozuna-hz9 ...`. If the
`hakozuna-hz9` directory is opened as the project root, drop `-C hakozuna-hz9`.

Prerequisites are intentionally small:

```text
make
gcc or clang
python3
ripgrep
coreutils / awk / sed
```

```bash
# standalone/source-shape audit
make -C hakozuna-hz9 hz9-standalone-check

# current pre-substrate closure gate
hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh

# current segment-local-cache scaffold smoke
make -C hakozuna-hz9 smoke-hz9segmentlocalcache
ITERS=10000000 make -C hakozuna-hz9 bench-hz9segmentlocalcache-api
ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_api_sweep.sh

# current routeable segment entry scaffold
make -C hakozuna-hz9 smoke-hz9segmententry
RUNS=3 ITERS=3000000 \
  hakozuna-hz9/scripts/run_hz9_segment_entry_handle_probe.sh
MODE=handlecheckedtouch CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=handlebody CLASS_ID=5 ITERS=10000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tokenbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tokencachebody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tokencachestate CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencachebody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencacheptrbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencachesplitbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencachetrustbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencacheapi CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlstokencache CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tokencacheretire CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlsguardbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlsbodychecked CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlscheckedtouch CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlsepochbody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlsroutebody CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry
MODE=tlsroute64body CLASS_ID=5 ITERS=5000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9segmententry

# current public split-boundary route scaffold
make -C hakozuna-hz9 smoke-hz9localslabrouteboundary
MODE=split CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=knownslot CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary

# current owner-page scaffold/API checks
make -C hakozuna-hz9 smoke-hz9ownerpagepool-route \
  smoke-hz9ownerpagepool-api \
  bench-hz9ownerpagepool-purelocal-api \
  bench-release-hz9ownerpagepool-purelocal-api

# owner-local page-pool shadow distribution
RUNS=3 hakozuna-hz9/scripts/run_hz9_owner_page_shadow_probe.sh

# profile/local-only evidence gate, not a mixed default promotion gate
RUNS=3 hakozuna-hz9/scripts/run_hz9_profile_local_gate.sh

# optional baseline/preload smoke
make -C hakozuna-hz9 smoke preload-smoke smoke-hz9localentry

# historical direction gates / probes
RUNS=5 make -C hakozuna-hz9 hz9-candidate-gate
RUNS=10 hakozuna-hz9/scripts/run_hz9_slab_profile_gate.sh
RUNS=3 hakozuna-hz9/scripts/run_hz9_slab_shadow_probe.sh
RUNS=10 hakozuna-hz9/scripts/run_hz9_baseline_public_matrix.sh
RUNS=5 hakozuna-hz9/scripts/run_hz9_substrate_readiness.sh
RUNS=3 hakozuna-hz9/scripts/run_hz9_entry_route_probe.sh
RUNS=1 hakozuna-hz9/scripts/run_hz9_local_entry_probe.sh
RUNS_READINESS=5 hakozuna-hz9/scripts/run_hz9_next_substrate_probe.sh
```

The profile-local gate builds only its required release binaries, then reuses
the common candidate-gate sampler with build skipping. Use the common candidate
gate directly when comparing broad/mixed default candidates.

`hz9-standalone-check` is a source-shape audit. It checks for symlinks,
accidental `hakozuna-hz8` path references, executable bits on shell scripts,
and ignored local build products. It does not replace the smoke/build commands
above. See `docs/HZ9_STANDALONE_CLOSURE.md` for the standalone contract.

Older held boxes still have reproducibility hooks:

```bash
hakozuna-hz9/scripts/run_hz9_local_mag_class_gate.sh
hakozuna-hz9/scripts/run_hz9_code_shape_audit.sh
```

## Position

```text
HZ3:
  local throughput reference
  speed-first TLS/cache-heavy allocator

HZ8:
  balanced low-RSS allocator
  fail-closed pointer ownership
  pending bitmap / qstate remote authority
  owner-exit hard drain

HZ9:
  throughput-first experimental line
  keeps HZ8 external safety contracts where useful
  may accept a heavier RSS/cache contract than HZ8
```

HZ9 is not intended to be a clone of HZ3, tcmalloc, or mimalloc. The target is
an HZ8-derived allocator with a faster local tier and explicit safety/RSS gates.

## Current Standalone Design

```text
local:
  TLS per-class medium segment cache scaffold
  segment-backed local slots, not HZ8 medium-run objects

state:
  LOCAL segments support put/take/free_allocated
  REMOTE_SEEN segments reject local allocation
  RETIRED segments are not reused in L0

remote:
  remote mark records pending bits and moves segment out of LOCAL
  owner-drain retires remote-contaminated segments in L0

lifecycle:
  release_all clears touched TLS segment state before behavior integration

non-goal:
  no allocator routing yet
  no H8OwnerRecord / H8ThreadCtx field additions in L0
```

## Active Design Lane

```text
HZ9SegmentLocalCache-L0:
  current design-prep lane
  segment-backed local slots, not HZ8 medium-run objects
  scaffold/drain/release-all/API sweep are implemented
  no allocator routing yet
  no public entry branch
  no H8OwnerRecord / H8ThreadCtx field additions in L0
  remote pending/qstate remains HZ8-derived authority
```

## Held Evidence

```text
HZ9OwnerLocalPagePoolPureLocal-L1:
  owner-page substrate evidence lane
  scaffold/shadow/API/page lifetime are implemented
  local alloc/free behavior is implemented
  route/state/API safety smoke is clean
  broad default is HOLD after local/remote perf gate

HZ9MediumLocalSlabPage-Shadow-L1:
  completed coverage evidence and class-distribution counters
  medium-only local substrate design
  does not replace remote pending/qstate in the first box
  must preserve owned INVALID fail-closed and owner-exit hard drain

HZ9MediumLocalSlabPage-L1:
  profile/evidence behavior prototype
  64K class only
  128KiB pages, two 64KiB slots
  4 pages per class per thread in L1
  remote frees claim pending bits and owner allocation collects them
  thread exit purges empty slab pages but retains route metadata
  fixed64 local signal is strong
  medium_r50/main_r90 no-regression gates fail, so not default
  smoke:
    make -C hakozuna-hz9 smoke-hz9slabroute smoke-hz9slabpage

next default candidate:
  no current default candidate
  next substrate must avoid LocalArena mixed local/remote atomic-page collapse
```

## Non-Goals

```text
do not mutate HZ8 default
do not replace remote pending bitmap in L0
do not add remote concurrent freelists in L0
do not weaken owned-looking INVALID fail-closed behavior
do not ship HZ9 as a user-facing replacement before matrix evidence exists
```

## Read First

```text
docs/HZ9_CURRENT_STATUS.md
docs/HZ9_SEGMENT_LOCAL_CACHE_L0.md
docs/HZ9_SEGMENT_ROUTE_PROOFS_L0.md
docs/HZ9_SEGMENT_ENTRY_L1.md
docs/HZ9_MEDIUM_TLS_OBJECT_CACHE_L0.md
docs/HZ9_LOCAL_SLAB_PAGE_L1.md
docs/HZ9_STANDALONE_CLOSURE.md
docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md
docs/HZ9_LOCAL_MAGAZINE_L0.md
docs/HZ9_DIFFERENTIATION.md
tests/README.md
bench/README.md
```
