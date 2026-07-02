# HZ9 Next Substrate

This is the active design gate for the next HZ9 local substrate. Historical
measurements are archived in `HZ9_LANE_HISTORY.md` and the per-lane evidence
docs.

## Current Decision

```text
HZ9LocalArenaRemoteSafePage-L1:
  implemented after LocalArena page-mix evidence
  remote-safe pages were created after class remote history
  mechanism worked, but mixed local/remote frees collapsed throughput
  medium_r50/main_r90 regressed sharply in the release probe
  decision: early NO-GO

SlabPage:
  keep as remote/profile evidence
  do not continue route/scan/freebits/cap tuning for default
  final min4 check kept 48K/64K only:
    medium_r50 improved, but main_local0/main_r90 still regressed
    debug showed main_r90 paid entry/fallback overhead without page use
    decision: close SlabPage default lane
  integrated min4 proof removed public entry split:
    main_local0 recovered, but main_r90 still regressed
    medium_r50 retained a material win
    decision: keep as remote/profile evidence, not default
  no-free-route proof:
    disabling SlabPage free/realloc route checks on no-use rows recovered
    main_local0 from 0.947 to 0.996
    main_r90 improved from 0.911 to 0.940 but stayed below default tolerance
    follow-up no-route proof disabled malloc/free route checks
    focused main_r90 R5 did not improve
    code-shape audit showed no-route restores non-small/free-non-arena shape
    layout audit showed owner/thread layout remains changed even under
    no-route
    layout-neutral proof restored baseline H8OwnerRecord/H8ThreadCtx size and
    no-use code shape, but remained proof-only with mixed short-row movement
    decision: route tax is entry-shape evidence, not a default path

LocalArena:
  keep as HZ9-owned substrate evidence
  do not continue page-mode retuning without a new substrate idea

current gate:
  hakozuna-hz9 standalone shape is clean
  near-limit HZ9 stats/report/build files are split
  existing substrate mechanisms have been compared
  TLS remote-class admission is implemented and held as NO-GO evidence
  owner-local page-pool scaffold / shadow / API / page lifetime are
  implemented without H8OwnerRecord/H8ThreadCtx layout changes
  owner-local page-pool PureLocal L1 is implemented as profile/evidence:
    directory-first free routing removes most broad remote-row route tax
    latest short gate still blocks promotion on medium_local0
    decision: HOLD, not default
  HZ9DirectSlabUseProof-L0 is implemented as profile/evidence:
    direct SlabPage use avoids owner-page/TLS/HZ8 fallback admission
    focused R3 improves medium_r50 and main_r90 materially
    focused R3 regresses medium_local0 and main_local0
    decision: SlabPage body is remote/profile evidence, not the next broad
    local substrate
  HZ9LocalPhaseAdmission-L0 is implemented as localarena_dense_ownerfast_phase8:
    R1 blocks promotion on medium/main local and remote rows
    decision: NO-GO evidence, do not tune threshold without a new mechanism
```

## Current Implementation Posture

```text
HZ9OwnerLocalPagePoolPureLocal-L1:
  status:
    implemented
    profile/evidence only
    do not promote as broad HZ9 default

  mechanism:
    local allocation pop returns owner-page pointers
    same-owner free pushes back into atomic local_free_bits
    remote exact free marks REMOTE_SEEN and claims pending bits
    directory-first free routing tries HZ8 medium directory before owner-page
    global route
    classes are disabled after REMOTE_SEEN in TLS state

  detail:
    docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md

  proof:
    HZ9OwnerLocalPagePoolScaffold-L0 is clean:
      no H8OwnerRecord/H8ThreadCtx size change
      no h8_malloc_inner / h8_free_inner code-shape growth
    HZ9OwnerLocalPagePoolShadow-L0 is wired:
      local rows show pure-local bucket
      remote rows classify remote pressure
    HZ9OwnerLocalPagePoolPureLocal API/page-lifetime proof is wired:
      TLS owner-page state creates and releases one mapped page per class
      route smoke validates real mapped page exact pointer handling
      REMOTE_SEEN / DRAINING / DETACHED state smoke is clean
    HZ9EntryBypassProof-L0 is NO-GO for public entry split default path
    public all-medium dispatch remains too expensive even with size bypass
    HZ9SlabIntegratedMin4Proof-L0 is HOLD evidence:
      integrated path improves medium_r50 but still regresses main_r90
    HZ9SlabLayoutNeutralProof-L0 restored no-use code shape and hot struct
      layout, but remains proof-only

  latest release gate:
    bench_results/20260702T113432Z_hz9_owner_page_perf_gate
    guard_local0 1.130
    small_interleaved_remote90 1.001
    medium_local0 0.954
    main_local0 1.016
    medium_r50 0.965
    main_r90 0.977

  read:
    remote regression is mostly removed
    medium local is still below baseline
    further owner-page work must remove local owner-page overhead, not only
    remote admission cost

  source-shape rule:
    do not grow h8_hz9_local_entry.c into a substrate body
    do not add build target blocks back into the top-level Makefile
    new target families go into mk/*.mk
```

## Next Implementation Box

```text
Latest behavior box:
  HZ9LocalPhaseAdmission-L0

SSOT / result:
  docs/HZ9_LOCAL_PHASE_ADMISSION_L0.md
  bench_results/20260702T121109Z_localarena_phase8_r1

read:
  LocalArena phase admission blocks some bad page creation, but still regresses
  medium_local0/main_local0 and medium_r50/main_r90. The problem is not only
  remote contamination timing; the substrate entry/body cost remains too high.

latest read:
  DirectSlabUse proof confirms the same split:
    remote-heavy rows win
    medium/main local rows lose
  Do not continue SlabPage sidecar/entry/direct-use tuning as the next default
  path. Treat SlabPage as a remote/profile component only.

debug read:
  HZ9DirectSlabDebugProbe-L0 confirms local rows have alloc_fallback=0 and
  active-thread free routing, yet still regress.
  The local loss is in the SlabPage local body/free path, not in capacity
  fallback. Remote wins come from pending retry/direct pending collection.

next posture:
  do not continue LocalArena threshold tuning
  choose the next HZ9 substrate only after a source-shape/cost read

source-shape read:
  RUN_SMOKE=0 RUN_PROBE=0 scripts/run_hz9_pre_substrate_recheck.sh
  PASS
  bench_results/20260702T121610Z_hz9_layout_audit
  bench_results/20260702T121610Z_hz9_code_shape_audit
  H8OwnerRecord/H8ThreadCtx layout and public entry code shape are controlled.
  The next blocker is substrate body/local cost, not hidden layout growth.

next evidence box:
  HZ9SubstrateCostMatrix-L0
  docs/HZ9_SUBSTRATE_COST_MATRIX_L0.md
  Compare baseline, SlabDirectUse, LocalArena phase admission, and OwnerPage
  purelocal under one gate before choosing another substrate.

matrix read:
  bench_results/20260702T121943Z_hz9_substrate_cost_matrix
  SlabDirectUse is strong remote/profile evidence but fails local/small gates.
  LocalArena phase8 is broad NO-GO.
  OwnerPage is the closest local substrate shape, but still loses medium_local0
  and moves main_r90/small_remote90.
  OwnerPage purelocal does not grow h8_malloc_inner / h8_free_inner /
  h8_free_non_arena_inner.

ownerpage body read:
  bench_results/20260702T122959Z_hz9_candidate_gate
  ownerfast-bits replaces purelocal local_free_bits CAS/fetch_or with
  load/store on pure local pages.
  It recovers fixed64_local0 from 0.548 to 0.963 and medium_local0 from 0.948
  to 0.996, proving local_free_bits RMW is a major local body bucket.
  It regresses medium_r50 to 0.860 and main_r90 to 0.932, so it is not a broad
  default path.

ownerpage class-cut read:
  bench_results/20260702T123734Z_hz9_candidate_gate
  <=32K ownerfast_bits cut does not stabilize remote/main rows.
  Full ownerfast_bits is the better proof but still misses medium_r50.
  Stop OwnerPage local mutation class-cut tuning for default.

ownerpage disabled-fast read:
  bench_results/20260702T124529Z_hz9_candidate_gate
  bench_results/20260702T124822Z_hz9_candidate_gate
  disabled_fast_reject reduces medium_r50 state_ensure from per-allocation
  scale to hundreds, but local/small/main movement remains unstable.
  Combining it with ownerfast_bits does not produce a broad candidate.
  Stop OwnerPage fixed-cost retuning for default unless a new substrate shape
  changes the branch/body model.

current scaffold:
  HZ9StaticLocalPageScaffold-L0
  docs/HZ9_STATIC_LOCAL_PAGE_SCAFFOLD_L0.md
  behavior change: none
  purpose:
    test the next substrate source shape before behavior
    static TLS state, no dynamic state ensure
    owner-local plain bits, no atomic local_free_bits RMW in the scaffold
    no H8OwnerRecord / H8ThreadCtx field additions

static local page shadow read:
  local-only:
    medium_local0 hit_ratio 0.999
    local-streak phase hit_ratio 0.995
  mixed r50:
    class-disable admission hit_ratio 0.001
    local-streak phase hit_ratio 0.043
  decision:
    bit-only static TLS scaffold is useful profile/local evidence
    do not wire it as a mixed-row default mechanism
    next behavior must either be profile-only or use a different page backing /
    ownership shape that does not collapse under frequent remote frees

profile-local evidence gate:
  scripts/run_hz9_profile_local_gate.sh
  scope:
    local-only rows and profile/evidence variants
  rule:
    results can justify profile documentation or a profile build, not broad
    mixed default promotion
  first smoke:
    RUNS=1 THREADS=2 ITERS=10000
    ownerpage_ownerfast_bits wins medium/main local but regresses guard_local0
    staticlocal_shadow wins fixed48/main/guard but regresses medium_local0
    next read needs RUNS>=3 before making any profile build decision
```

## Active Constraints

```text
must preserve:
  standalone hakozuna-hz9 source tree
  no runtime/build dependency on ../hakozuna-hz8
  small arena fast path
  owned INVALID fail-closed
  pending/qstate remote duplicate-free authority
  owner-exit hard drain

must not:
  grow h8_malloc_inner / h8_free_inner on baseline builds
  add public-entry checks for small-owned pointers
  add HZ9 behavior flags back to hakozuna-hz8
  depend on benchmark row identity
  add a remote concurrent freelist in L1
  embed large experimental state directly in hot H8ThreadCtx/H8OwnerRecord
```

## Evidence Read

```text
SlabPage min0:
  proves 8K/16K coverage can help main rows
  remains blocked by small_remote/main-local stability and hot-struct footprint

SlabPage sidecar/freebits/storemut:
  isolates route/scan/CAS costs
  remote/profile wins remain real
  local default gates remain unclean

DirectSlabUse:
  removes owner-page/TLS/HZ8 fallback admission from malloc
  still loses medium/main local with all-local slab hits
  confirms SlabPage is not the next broad local substrate

LocalArena:
  HZ9-owned pages can improve medium_local0
  remote-heavy rows fail when owner-fast pages become remote-contaminated
  remote-safe page mode still collapses under mixed local/remote frees

local entry seam:
  useful only if repaid by a high-hit substrate
  do not put another substrate body into h8_hz9_local_entry.c

entry bypass proof:
  min4 size bypass keeps a remote/profile win but still regresses main rows
  next default candidate should avoid public all-medium entry split

integrated SlabPage proof:
  avoiding public entry split is necessary but not sufficient
  current SlabPage remains profile evidence due to main_r90 regression

route-off proofs:
  show that global slab maybe-route checks can be too expensive for no-use rows
  explain some main_local0 movement in short gates
  do not explain main_r90 in focused repeat gates
  restore internal non-small/free-non-arena code shape in no-route form
  leave H8OwnerRecord/H8ThreadCtx layout changed
  layout-neutral proof restores both baseline layout and code shape
  remains proof-only, not behavior evidence
  next substrate must avoid no-use route tax and no-use layout contamination
```

## Next Substrate Requirements

```text
coverage:
  8K/16K/24K/32K for main_local0 and main_r90
  48K/64K for medium_local0 and medium_r50

local shape:
  selected before HZ8 medium fallback when possible
  no small-owned public dispatch tax
  no broad adaptive blocked checks on local rows that will not use substrate
  no public all-medium HZ9 entry for default candidates
  no broad free-side route check before HZ9-owned pages exist
  no additional SlabPage route-off flags as a default strategy
  no H8OwnerRecord/H8ThreadCtx size or hot-offset change for no-use rows

remote shape:
  keep HZ8-style pending bitmap/qstate authority
  avoid LocalArena-style mixed local/remote atomic-page collapse
  make remote-safe capacity a structural property, not a late page-mode patch

state shape:
  prefer HZ9-owned sidecar/files over growing HZ8-derived hot structs
  keep entry seam small
  put behavior bodies in new HZ9-owned source/includes
  allocate optional owner/thread substrate state out-of-line only after the
  substrate is selected
```

## Performance Gate

```text
primary:
  medium_local0 >= baseline * 1.03
  main_local0 >= baseline * 1.00
  medium_r50 >= baseline * 1.05

non-target:
  guard_local0 >= baseline * 0.98
  small_interleaved_remote90 median and p25 >= baseline * 0.98
  main_r90 >= baseline * 0.98

RSS:
  retained/cache/page overhead must be explicit
  owner exit must drain all HZ9-owned retained state
```

If a candidate only wins a subset such as `medium_local0` or remote-heavy rows
while losing local/small/main stability, it stays profile evidence.

## Completed Observation Box

```text
HZ9SubstrateMechanismCompare-L1:
  complete
  behavior change: none
  script: scripts/run_hz9_substrate_mechanism_probe.sh
  variants: baseline, tlscache_gated, tlscache_remoteclass, remoteactive

reason:
  TLS cache and LocalArena fail in different ways
  compare them under the same short debug rows before choosing a new substrate
  avoid adding another page-mode branch from stale assumptions
```

Reproduce the archived read:

```bash
hakozuna-hz9/scripts/run_hz9_substrate_mechanism_probe.sh
TARGET=remoteactive hakozuna-hz9/scripts/run_hz9_local_entry_cost_probe.sh
```

## Closed TLS Behavior Candidate

```text
HZ9MediumTLSCacheRemoteClassAdmission-L1:
  build target: hz9mediumtlscache_remoteclass
  after a remote free is successfully published for a medium class:
    block TLS cache pop for that class
    block TLS cache push for that class
    flush the current thread's cached entries for that class

reason:
  mechanism probe showed TLS cache local rows can have near-perfect reuse
  remote rows have low pop/push reuse and high flush churn
  class-level remote admission is the smallest test to preserve local reuse
  without continuing LocalArena page-mode retuning

promotion requirement:
  local rows must retain the TLS cache win
  medium_r50/main_r90 must return to HZ8 baseline tolerance
  state_mismatch and owned invalid gates must remain zero

result:
  NO-GO as behavior
  keep as evidence target
```

Mechanism probe:

```text
log:
  bench_results/20260702T081836Z_hz9_substrate_mechanism_probe

medium_local0:
  tlscache_remoteclass preserved TLS local reuse
  pop_hit=99880
  push_hit=100000
  reuse_ratio=0.999

medium_r50:
  remoteclass did block cache use after remote evidence
  pop_hit=3
  push_hit=7
  remote_mark=6
  remote_pop_block=99983
  remote_push_block=49797
```

Release direction gate:

```text
log:
  bench_results/20260702T081928Z_hz9_candidate_gate
  RUNS=5 THREADS=2 ITERS=20000

ratios:
  medium_local0 0.949
  main_local0 0.966
  medium_r50 0.898
  main_r90 0.924
  small_remote90 1.004

decision:
  remote-class admission removes the low-hit remote TLS churn, but the
  remaining admission/fallback shape still loses medium/main rows.
```

Current read:

```text
Do not keep tuning TLS object-cache admission around HZ8 medium runs.
The next HZ9 substrate must avoid the HZ8 medium-run local fixed cost itself.
```

## Archived SlabPage Combination Box

```text
HZ9SlabLocalFastCombination-L1:
  status:
    closed as profile/evidence
    not a current implementation box

  no new substrate body yet
  wire the existing SlabPage local/remote split parts into one target

target:
  slablocalfast

enabled parts:
  H9_SLAB_ENTRY_SPLIT_L1
  H9_SLAB_THREAD_SIDECAR_L1
  H9_SLAB_OWNER_SIDECAR_L1
  H9_SLAB_LOCAL_FREE_BITS_L1
  H9_SLAB_LOCAL_STORE_MUTATION_L1
  H9_SLAB_ALLOC_FREE_FIRST_L1
  H9_SLAB_NO_PAGE_FAST_REJECT_L1

reason:
  SlabPage already has the right authority split:
    owner-local free bits for local reuse
    atomic pending bits / qstate for remote frees
    sidecar page state to avoid growing hot HZ8-derived structs
  Before writing another substrate, verify whether this combination closes
  the local fixed-cost gap.

GO read:
  medium_local0 >= baseline * 1.03
  main_local0 no regression
  medium_r50/main_r90 no regression beyond evidence tolerance

NO-GO read:
  if this still loses local rows, stop SlabPage target tuning and design a
  fresh local page substrate instead of adding more flags.
```

First release read:

```text
log:
  bench_results/20260702T083223Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  medium_local0 0.902
  main_local0 0.854
  medium_r50 1.391
  main_r90 1.385
  small_remote90 0.994

decision:
  slablocalfast is not default as-is
  it is strong remote/profile evidence

read:
  local_free_bits/store-mutation/free-first/sidecar combination does make
  remote-heavy medium/main rows much faster, but local rows still pay too much
  entry/page fixed cost.
```

Closed narrow follow-up:

```text
HZ9SlabLocalFastAdaptiveHot-L1:
  combine slablocalfast with existing remote-hot admission
  only use the slab path when owner/class remote evidence says it is useful
  candidate gate variant:
    slablocalfast_adaptive_hot
    slablocalfast_adaptive_poll32
    slablocalfast_adaptive_poll32_min2
    slablocalfast_adaptive_poll32_min4

goal:
  preserve the remote-heavy wins from slablocalfast
  restore medium_local0/main_local0 to baseline tolerance

poll32 variant:
  cache the remote-hot class mask in TLS
  refresh the owner atomic mask every 32 medium allocation attempts
  purpose is to avoid paying owner sidecar/atomic polling on pure local rows

min2 variant:
  excludes 8K/16K slab attempts
  tests whether main_local0 fixed cost can be reduced while keeping 24K/32K
  remote-heavy coverage

min4 variant:
  excludes 8K/16K/24K/32K slab attempts
  keeps only 48K/64K SlabPage coverage
  tests whether medium_r50 can retain the SlabPage win without contaminating
  main-local-heavy classes

min4 release read:
  log: bench_results/20260702T085219Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000
  medium_local0 1.011
  main_local0 0.964
  medium_r50 1.225
  main_r90 0.965
  small_remote90 0.990

min4 debug read:
  log: bench_results/20260702T085227Z_hz9_substrate_mechanism_probe
  medium_r50 uses 48K/64K SlabPage capacity and improves materially
  main_r90 does not allocate SlabPage pages, but still pays entry/fallback
  fixed cost and regresses

if this fails:
  close SlabPage as default substrate work
  keep it as remote/profile evidence only
  design a fresh local page substrate rather than adding more SlabPage flags
```

Latest short debug read:

```text
medium_local0:
  arena_attempt == arena_alloc_hit
  malloc_fallback == 0
  local row is not paying HZ8 fallback in the LocalArena target

medium_r50:
  arena_attempt is high but arena_alloc_hit is about one tenth of attempts
  most allocations fall back to HZ8 after attempting the local arena path
  admission shadow shows remote_seen_no_capacity is the dominant bucket
  remote_seen_active_nonfull roughly matches the remaining useful hit bucket
  next substrate/admission must keep active-nonfull reuse while avoiding new
  page creation after remote-heavy evidence

small_interleaved_remote90:
  free_arena_skip covers the row
  free_check/local_outer remain zero
  current local-entry seam is not the small-row maybe-check contaminant
```

RemoteActive behavior read:

```text
target:
  localarena_dense_ownerfast_remoteactive

mechanism:
  remote-heavy page_create dropped from about 1024 to about 12 in the
  medium_r50 short debug probe
  peak RSS dropped sharply versus remote-safe pages
  useful active-nonfull reuse remained but was not enough to beat baseline

R1 release:
  medium_r50 0.913 vs baseline, versus remotesafe 0.481
  main_r90 0.908 vs baseline, versus remotesafe 0.501
  medium_local0 0.757 vs baseline
  small_remote90 1.047 vs baseline

decision:
  HOLD as evidence
  do not promote

read:
  remote admission shape is directionally correct
  LocalArena still does not provide a clean local default substrate
  the next candidate should keep the admission lesson but replace the local
  page substrate or remove its local-row fixed cost
```

## Source-Shape Gate

```text
before behavior:
  make -C hakozuna-hz9 hz9-standalone-check
  hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh

when hakozuna-hz9 is opened as project root:
  make -C . hz9-standalone-check
  scripts/run_hz9_pre_substrate_recheck.sh

line limit:
  no active source/script/doc/Makefile/mk file over 800 lines
  split docs before they become chronological ledgers
```

## Readiness Commands

After the source-shape gate passes, use these before opening another substrate
behavior box:

```bash
RUNS_READINESS=5 hakozuna-hz9/scripts/run_hz9_next_substrate_probe.sh
RUNS=5 hakozuna-hz9/scripts/run_hz9_substrate_readiness.sh
RUNS=3 hakozuna-hz9/scripts/run_hz9_entry_route_probe.sh
```

Local equivalents from inside `hakozuna-hz9/`:

```bash
RUNS_READINESS=5 scripts/run_hz9_next_substrate_probe.sh
RUNS=5 scripts/run_hz9_substrate_readiness.sh
RUNS=3 scripts/run_hz9_entry_route_probe.sh
```

## Evidence Map

```text
HZ9_LANE_HISTORY.md:
  compact chronology and closed-box decisions

HZ9_LOCAL_SLAB_PAGE_EVIDENCE.md:
  SlabPage sidecar, route, freebits, and profile evidence

HZ9_LOCAL_ARENA_L0.md:
  LocalArena class/admission/remote-safe evidence

HZ9_STANDALONE_CLOSURE.md:
  standalone single-folder contract and source-shape splits

HZ9_FRESH_LOCAL_PAGE_SUBSTRATE_L0.md:
  next substrate design-prep and entry-bypass proof gate

HZ9_SUBSTRATE_COST_MATRIX_L0.md:
  active direction-finding matrix for current substrate families
```
