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
  ProductEntry-L0 is active
  Phase 2 substrate and Phase 3 pointer-token work are held as evidence
  medium/main local reuse uses TLS per-class ProductEntry state
  remote frees claim segment pending bits and owner drain merges them back
  segment lifecycle/cache caps are part of the performance/RSS contract
  HZ8 remains the public balanced line; HZ9 is not the HZ8 default

next evidence work:
  read docs/HZ9_PHASES.md before opening another lane
  use honest ProductEntry and matrix measurements, not fused-only ceilings
  keep same-thread local fast path entry-local
  keep foreign/invalid/miss classification on route authority
  verify owner-drain, lifecycle release, and RSS caps in R10 gates
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
  medium/main ProductEntry local path
  TLS per-class entry-local bits
  owner-drain merge from pending bits to entry-local free bits
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
MODE=inlinebody CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=ptrtoken CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=ptrentry CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastpublic CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastledger CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=hotcold CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastonly CLASS_ID=5 ITERS=3000000 TOUCH=1 \
  hakozuna-hz9/h8_bench_hz9localslabrouteboundary
MODE=lastentry CLASS_ID=5 ITERS=3000000 TOUCH=1 \
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
  does not try to be HZ8/HZ9-style fail-closed remote-lifecycle line

HZ8:
  balanced low-RSS allocator
  fail-closed pointer ownership
  pending bitmap / qstate remote authority
  owner-exit hard drain

HZ9:
  throughput-first experimental line
  keeps HZ8 external safety contracts where useful
  moves local medium/main hot state into TLS per-class ProductEntry
  keeps remote frees on route + pending-bit owner drain
  may accept a heavier RSS/cache contract than HZ8
```

HZ9 is not intended to be a clone of HZ3, tcmalloc, or mimalloc. The target is
an HZ8-derived allocator with a faster local tier and explicit safety/RSS gates.

## Architecture Snapshot

English:

```text
HZ8 medium/main:
  malloc/free
    -> shared medium-run authority
    -> slot_state + free/allocated masks
    -> pending bitmap + qstate + owner queue
    -> owner-exit hard drain

HZ9 medium/main ProductEntry:
  same-thread malloc/free
    -> TLS ProductEntry[class]
    -> entry-local alloc/free bits

  foreign free
    -> segment route
    -> atomic pending_bits claim
    -> owner drain: pending_bits -> entry-local free_bits

  lifecycle/RSS
    -> bounded segment cache
    -> drain before retire/release/owner exit
```

Japanese:

```text
HZ8 medium/main:
  malloc/free
    -> 共有medium-run authority
    -> slot_state + free/allocated mask
    -> pending bitmap + qstate + owner queue
    -> owner-exit hard drain

HZ9 medium/main ProductEntry:
  same-thread malloc/free
    -> TLS ProductEntry[class]
    -> entry-local alloc/free bits

  foreign free
    -> segment route
    -> atomic pending_bits claim
    -> owner drainでpending_bitsをentry-local free_bitsへ戻す

  lifecycle/RSS
    -> bounded segment cache
    -> retire/release/owner exit前に必ずdrain
```

## Current Standalone Design

```text
local:
  TLS per-class ProductEntry for medium/main local reuse
  entry-local alloc/free bits are the same-thread fast authority
  calloc bypasses ProductEntry in L0 so control/realloc allocations stay on the
  HZ8-derived inner path

state:
  segment metadata provides route and lifecycle identity
  local slot bits are not synchronized to segment metadata on every operation
  retired/cache segment counts are bounded

remote:
  foreign frees route to segment metadata and atomically claim pending bits
  owner drain exchanges pending bits and merges reclaimed slots into entry-local
  free bits

lifecycle:
  retire/release drains pending first
  owner/thread exit releases cached segments
  release-pressure smoke must exercise real segment_release

non-goal:
  no remote concurrent freelist in L0
  no HZ8 default behavior mutation
```

## Active Design Lane

```text
HZ9ProductEntry-L0:
  active implementation lane
  public preload ProductEntry path exists for medium/main sizes
  small/guard/control allocations bypass ProductEntry when required
  owner drain prevents dirty segment switch/drop storms
  lifecycle cache cap and segment_release pressure are part of the gate
  R10 matrix/RSS/lifecycle evidence is required before promotion claims
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
