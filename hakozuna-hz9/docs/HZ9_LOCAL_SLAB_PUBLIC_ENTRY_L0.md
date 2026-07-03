# HZ9 Local Slab Public Entry L0

## Purpose

```text
move HZ9 from micro substrate proof toward public allocator shape
without opening preload, remote protocol, or RSS policy yet
```

## Scope

```text
entry:
  h9_lsp_debug_public_malloc(size)
  h9_lsp_debug_public_free(ptr, owned_out)

fast local path:
  size -> medium class
  TLS active routeleafcompact entry
  same-thread exact last-token free skips route

fallback:
  direct-owned route remains canonical authority for miss / invalid / non-LIFO

non-goals:
  no preload integration
  no multithread remote protocol
  no RSS/cache cap policy
  no HZ8 default behavior change
```

## Measurement Rules

```text
honesty:
  asm audit must keep slot_select = 1 for gate candidates

stability:
  local throughput gate should run through ASLR OFF harness when available
  record explicit ASLR status

baseline:
  routeleafcompact remains current local honest baseline on this host
```

## Gates

```text
publicentry local:
  target is not 800M phantom ceiling
  compare against routeleafcompact honest baseline

correctness:
  exact free accepted
  double free rejected as owned invalid
  interior pointer rejected as owned invalid
  miss pointer not owned
  non-LIFO fallback route_valid / ptr_fallback clean

decision:
  if publicentry is far below routeleafcompact, fix entry shape before preload
```

## L0 Result

```text
ASLR OFF, CLASS_ID=5, ITERS=30000000, TOUCH=1:
  routeleafcompact:
    median 331.698M

  publicentry:
    median 103.979M

  publicentrynonlifo:
    median 80.545M
    route_valid = 30000000
    ptr_fallback = 30000000
    state_mismatch = 0

smoke:
  public exact route VALID
  public interior INVALID
  public after-free INVALID
  public double free rejected as owned
  public miss free not owned

asm audit:
  public_malloc:
    honest-candidate
    slot_select = 1

  public_free:
    route-authority-candidate
    route_call = 1

read:
  public-shaped authority is correct but expensive.
  Synchronizing entry-local bits back to routeable segment state drops LIFO
  throughput by about 3x versus routeleafcompact.

decision:
  PublicEntry-L0 is correct evidence, not performance-ready.
  Do not open preload until public entry shape is improved.
```

## Static Segment Entry Probe

```text
problem:
  publicentry synchronizes entry-local free/alloc bits to the routeable
  segment on every malloc/free fast hit

cost:
  per-op segment stores
  entry state becomes observable through sync calls
  local body loses the routeleafcompact-like entry-local shape

design rule:
  segment is static route metadata on the same-thread fast path
  per-slot alloc/free state is owner-local entry state
  fallback may synchronize before canonical route classification

new probe:
  publicentrynosync
  publicentrynosyncnonlifo

expected read:
  LIFO should recover toward routeleafcompact
  non-LIFO should fall through sync-all + direct-owned route
  route fallback remains the negative/unknown authority
```

## Static Segment Entry Result

```text
ASLR OFF, CLASS_ID=5, ITERS=30000000, TOUCH=1:
  routeleafcompact:
    median 338.121M

  publicentry:
    median 97.058M

  publicentrynosync:
    median 117.482M

  publicentrynosyncnonlifo:
    median 87.625M
    route_valid = 30000000
    ptr_fallback = 30000000
    state_mismatch = 0

asm audit:
  public_nosync_malloc:
    honest-candidate
    slot_select = 1

  public_nosync_free:
    route-authority-candidate
    route_call = 1

read:
  Removing per-op segment sync helps but does not restore routeleafcompact
  throughput.  The remaining gap is public API split / TLS lookup and
  call-footing overhead, not only segment synchronization.

decision:
  keep publicentrynosync as evidence.
  Do not open preload or MT remote work from this shape.
```

## Public API Baseline Rule

```text
routeleafcompact:
  fused worker ceiling
  no public malloc/free call boundary
  no size-to-class per allocation
  not the promotion baseline for public entry

publicentrynosync:
  public-call-shaped debug entry
  has malloc/free call boundary
  has size-to-class on malloc
  should be compared against HZ8 h8_malloc/h8_free in the same micro shape

new baseline bench:
  bench-hz9publicapi-baseline
  binary: h8_bench_hz9publicapi_baseline
  measures HZ8 h8_malloc/h8_free LIFO and non-LIFO with the same size/touch

gate read:
  if HZ9 publicentrynosync beats HZ8 public API, continue per-call cleanup
  if not, fix public entry footing before further HZ9 local design
```

## Public API Baseline Result

```text
ASLR OFF, CLASS_ID=5, ITERS=30000000, TOUCH=1:
  HZ8 h8_malloc/h8_free LIFO:
    median 50.457M

  HZ8 h8_malloc/h8_free non-LIFO:
    median 50.432M

comparison:
  publicentrynosync / HZ8 public LIFO:
    117.482M / 50.457M = 2.33x

  publicentrynosyncnonlifo / HZ8 public non-LIFO:
    87.625M / 50.432M = 1.74x

read:
  routeleafcompact remains a fused ceiling, not a public gate.
  Against the correct public-call baseline, publicentrynosync is a strong
  positive signal.  Continue with per-call cleanup before preload/MT.
```

## Current Entry Pure-Free Probe

```text
change:
  malloc updates a TLS current-entry pointer
  pure free loads that pointer directly instead of last_class + entries[index]
  pure free omits owned_out because product free does not return ownership
  correctness/classification remains covered by the owned_out nosync path

ASLR OFF, CLASS_ID=5, ITERS=30000000, TOUCH=1:
  HZ8 h8_malloc/h8_free LIFO:
    median 48.733M

  HZ8 h8_malloc/h8_free non-LIFO:
    median 50.447M

  HZ9 public nosync LIFO:
    median 116.963M

  HZ9 current-entry pure LIFO:
    median 121.061M

  HZ9 current-entry pure non-LIFO:
    median 93.928M

comparison:
  current-entry pure LIFO / HZ8 public LIFO:
    121.061M / 48.733M = 2.48x

  current-entry pure non-LIFO / HZ8 public non-LIFO:
    93.928M / 50.447M = 1.86x

read:
  current-entry TLS 1-load helps, but only modestly.
  HZ9 public-call-shaped path already beats HZ8 strongly; the next meaningful
  step is product entry integration and matrix gates, not chasing fused ceiling.
```

## Product Entry L0

```text
build:
  bench-release-hz9localslabpublicentry
  flag: H9_LOCAL_SLAB_PUBLIC_ENTRY_L0

malloc:
  medium size only
  HZ9 public nosync local slab path first
  fallback to HZ8 if no HZ9 ctx/page/segment is available

free:
  HZ8 small arena pointers skip HZ9 and go directly to HZ8
  current-entry exact hit frees locally without route
  fallback first checks the HZ9 segment hash; non-HZ9 pointers return MISS
  only HZ9 segment hits synchronize that segment's entry before route

cap:
  H9_LSP_MAX_SEGMENTS = 1024
  H9_LSP_HASH_CAP = 2048
  create_segment preflights cap before reserve/commit
```

### Quick Matrix R3

```text
command shape:
  RUNS=3 THREADS=8 ITERS=30000
  baseline: h8_bench_release
  candidate: h8_bench_release_hz9localslabpublicentry

fixed64_local0:
  base 130.158M
  cand 297.355M

medium_local0:
  base 96.384M
  cand 135.600M

main_local0:
  base 125.472M
  cand 163.399M

guard_local0:
  base 237.237M
  cand 203.300M

read:
  product entry is now matrix-shaped enough to continue.
  fixed64/medium/main local are positive in this quick run.
  guard still needs R10 because small-path variance remains visible, but the
  prior severe small-free miss tax is gone.
```

## Product Entry Hazards Closed In L0

```text
small free miss tax:
  prior product free routed every small/H8 arena free through HZ9 hash route
  fix: h8_arena_contains(ptr) skips HZ9 product route

run-to-run segment cliff:
  HZ9 segments persist across runs in the same process
  old cap 64 exhausted after repeated threaded runs
  create_segment then reserve/committed before failing cap, producing slow runs
  fix: widen cap and preflight free segment slot before reserve/commit

remaining non-goal:
  no segment release/lifecycle policy yet
  no MT remote protocol yet
  no promotion decision from R3 quick matrix
```

## Product Entry R5 ASLR-Off

```text
command shape:
  setarch $(uname -m) -R
  RUNS=5 THREADS=8 ITERS=50000
  baseline: h8_bench_release
  candidate: h8_bench_release_hz9localslabpublicentry

fixed64_local0:
  base 169.871M
  cand 419.453M
  ratio 2.47x

medium_local0:
  base 133.684M
  cand 168.910M
  ratio 1.26x

main_local0:
  base 122.580M
  cand 172.312M
  ratio 1.41x

guard_local0:
  base 288.976M
  cand 285.909M
  ratio 0.99x

read:
  local product entry stem is viable against HZ8 public path.
  ProductEntry-L0 remains local-only evidence: MT remote free and lifecycle
  release are not implemented yet.
```

### RSS Read

```text
R5 ASLR OFF post RSS median:
  fixed64_local0:
    base 2.83 MiB
    cand 2.75 MiB

  medium_local0:
    base 3.29 MiB
    cand 4.65 MiB

  main_local0:
    base 3.41 MiB
    cand 4.80 MiB

  guard_local0:
    base 2.77 MiB
    cand 2.91 MiB

read:
  absolute RSS is still small, but medium/main already show about +40%.
  ProductEntry-L0 must track segment retention from here onward.
```

## RSS / Segment Gate

```text
required in every ProductEntry matrix:
  throughput median / p25
  post RSS median
  peak RSS median
  minor faults median
  HZ9 segment_create
  HZ9 segment_live
  HZ9 segment_committed_bytes / committed_peak
  HZ9 segment_reserved_bytes / reserved_peak
  HZ9 segment_cap_reject

decision rule:
  local throughput win is not sufficient by itself
  if medium/main RSS grows without an explicit segment cap/release story,
  ProductEntry remains evidence-only

next implementation:
  expose HZ9 local slab segment counters in bench summary
  use them to explain R10 RSS before MT remote work

note:
  RSS remains the canonical resident-memory metric.
  segment committed/reserved counters explain the retention substrate and may
  exceed resident RSS when pages are reserved/committed but sparsely touched.
```

## Product Entry R10 ASLR-Off With Segment Counters

```text
command shape:
  setarch $(uname -m) -R
  RUNS=10 THREADS=8 ITERS=50000
  baseline: h8_bench_release
  candidate: h8_bench_release_hz9localslabpublicentry

fixed64_local0:
  throughput:
    base 183.269M
    cand 434.556M
    ratio 2.37x
  post RSS:
    base 2.80 MiB
    cand 2.81 MiB
  segments:
    create=80 live=80 committed=20.0 MiB reserved=40.0 MiB cap_reject=0

medium_local0:
  throughput:
    base 145.891M
    cand 182.962M
    ratio 1.25x
  post RSS:
    base 3.25 MiB
    cand 7.55 MiB
  segments:
    create=480 live=480 committed=120.0 MiB reserved=240.0 MiB cap_reject=0

main_local0:
  throughput:
    base 143.083M
    cand 153.690M
    ratio 1.07x
  post RSS:
    base 3.52 MiB
    cand 5.79 MiB
  segments:
    create=320 live=320 committed=80.0 MiB reserved=160.0 MiB cap_reject=0

guard_local0:
  throughput:
    base 322.201M
    cand 281.900M
    ratio 0.87x
  post RSS:
    base 2.90 MiB
    cand 3.02 MiB
  segments:
    create=0 live=0 committed=0 reserved=0 cap_reject=0

read:
  fixed64 and medium local throughput remain strong.
  main local only barely clears positive territory in R10.
  guard still regresses, although no HZ9 segment is created; this is dispatch
  footing / arena skip cost, not segment retention.
  medium/main RSS growth is explained by segment retention across runs.
```

## Next Decision

```text
do not open MT remote yet:
  local ProductEntry is not stable enough on guard/main and has no release
  story for retained HZ9 segments

next box:
  HZ9ProductEntrySegmentRelease-L1

goal:
  release or recycle ProductEntry test segments at thread/run boundaries
  keep local throughput benefit
  lower medium/main post RSS and segment_live after matrix runs

secondary:
  reduce guard tax by making the ProductEntry free dispatch bypass cheaper for
  HZ8 small arena pointers
```

## Segment Release L1 Probe

```text
change:
  H9_LOCAL_SLAB_PUBLIC_ENTRY_L0 releases each thread's TLS public-entry
  segments during h8_thread_shutdown()

scope:
  local-only
  remote protocol still closed
  no reusable segment cache yet

R5 ASLR OFF, THREADS=8, ITERS=50000:
  medium_local0:
    base throughput 124.818M
    cand throughput 128.191M
    base post RSS 3.13 MiB
    cand post RSS 3.26 MiB
    segments create=240 release=240 live=0

  main_local0:
    base throughput 134.400M
    cand throughput 141.323M
    base post RSS 3.39 MiB
    cand post RSS 3.71 MiB
    segments create=160 release=160 live=0

  guard_local0:
    base throughput 269.092M
    cand throughput 271.296M
    base post RSS 2.90 MiB
    cand post RSS 2.85 MiB
    segments create=0 release=0 live=0

read:
  release fixes retained segment live bytes and returns RSS near HZ8.
  throughput benefit shrinks because synchronous thread-exit release is counted
  in the public matrix run.

next:
  keep release for correctness/RSS evidence
  investigate reusable bounded segment cache or cheaper run-boundary release
  before MT remote protocol
```

## Bounded Segment Cache L1

```text
goal:
  recover release-cost throughput while keeping segment retention bounded

design:
  clean thread-exit ProductEntry segments enter a small global cache
  cache cap: 48 segments
  dirty/non-empty segments release immediately
  same-class create path reuses cached segments before reserve/commit

expected:
  segment_create drops after warmup
  segment_cache_hit/store explain reuse
  segment_cache_live/bytes bound retained substrate
  RSS may sit above full release but below unbounded retention

gate:
  medium/main throughput should recover toward no-release ProductEntry
  post/peak RSS must remain explained by cache_live and cache_bytes
  cap_reject must stay 0
```

R5 ASLR-off result:

```text
medium_local0:
  base throughput 123.747M
  cand throughput 234.203M
  base post RSS 3.27 MiB
  cand post RSS 3.22 MiB
  cache_hit=192 cache_store=240 cache_live=48 cache_bytes=12 MiB
  segment_create=48 release=0 live=48

main_local0:
  base throughput 131.981M
  cand throughput 219.130M
  base post RSS 3.59 MiB
  cand post RSS 3.65 MiB
  cache_hit=128 cache_store=160 cache_live=32 cache_bytes=8 MiB
  segment_create=32 release=0 live=32

guard_local0:
  base throughput 315.442M
  cand throughput 228.076M
  segment_create=0 cache_live=0
```

read:
  bounded cache recovers medium/main throughput and keeps observed post RSS near
  HZ8 for these local rows, but guard still pays a ProductEntry free-dispatch
  tax despite never creating a segment.

next:
  add a cheap ProductEntry presence guard so pure small/guard processes skip the
  ProductEntry free path until a HZ9 segment has actually been created.

Presence/inner-free R5 ASLR-off result:

```text
medium_local0:
  base throughput 121.400M
  cand throughput 207.444M
  cand post RSS 2.95 MiB
  cache_hit=192 cache_store=240 cache_live=48 cache_bytes=12 MiB

main_local0:
  base throughput 127.223M
  cand throughput 187.648M
  cand post RSS 3.38 MiB
  cache_hit=128 cache_store=160 cache_live=32 cache_bytes=8 MiB

guard_local0:
  base throughput 224.023M
  cand throughput 229.450M
  cand post RSS 2.69 MiB
  segment_create=0 cache_live=0
```

read:
  moving ProductEntry free classification into h8_free_inner after the arena
  fast return removes the small/guard tax. Pure small rows now bypass HZ9
  ProductEntry entirely while medium non-arena frees still reach ProductEntry
  when a segment has been created.

## ProductEntry Remote Pending L0

```text
goal:
  open the first MT-safe ProductEntry remote boundary without optimizing MT
  throughput yet

design:
  same-thread exact free:
    current pointer-token/local entry fast path remains owner-local

  foreign/exact ProductEntry free:
    route to segment/slot
    atomic pending_bits fetch_or claims the slot
    first claim is accepted as remote pending
    duplicate claim is rejected as owned invalid
    remote producer does not mutate local free_bits or alloc_bits

  owner drain:
    pending_bits is not a dirty-segment marker
    owner uses atomic_exchange(pending_bits, 0)
    claimed slots are merged into entry-local alloc_bits/free_bits
    remote producer never mutates entry-local bits
    same-thread allocation keeps using the same segment after drain

non-goal:
  no owner queue cadence
  no remote freelist
  no remote performance claim

hard gates:
  duplicate remote claim accepted = 0
  remote publish lost = 0
  remote mutated local bits = 0
  load/store clear of pending_bits = 0
  owned invalid forwarded to platform = 0
```

Prior dirty-drop read:

```text
dirty-drop guard:
  safe but weak
  medium_r50 17.916M vs HZ8 23.056M
  main_r90 14.674M vs HZ8 15.000M
  reason: pending was treated as segment contamination instead of capacity
```

Rejected dirty-switch probe:

```text
bench_results/20260703T_hz9_product_remote_dirty_switch_l0_r3_aslroff

attempt:
  when pending_bits != 0, sync the current entry and switch the class to a
  fresh segment while keeping the dirty segment route-visible.

result:
  medium_interleaved_remote50:
    HZ8 23.886M
    HZ9 1.016M
    create/release/live=1108/116/992
    cap_reject=716339

  main_interleaved_remote90:
    HZ8 16.009M
    HZ9 1.161M
    create/release/live=1081/78/1003
    cap_reject=628218

decision:
  NO-GO. Switching to fresh segments without owner drain creates a segment
  storm and exhausts the segment cap. The L0 default remains dirty-drop /
  no-reuse until there is a real owner drain or bounded retired-segment policy.
```

OwnerDrain-L0 result:

```text
bench_results/20260703T_hz9_product_owner_drain_single_segment_l0_retry2

shape:
  one active ProductEntry segment per class/thread
  remote free only fetch_or claims pending_bits
  owner drains pending_bits with atomic_exchange into entry-local bits
  if the active segment is full after drain, ProductEntry returns NULL and
  the public path falls back to HZ8 instead of creating a replacement segment

main_interleaved_remote90, R3 ASLR-off:
  HZ8 prior same-lane reference: about 15.2M..16.0M
  HZ9 19.014M
  post RSS 5.02 MiB
  peak RSS 13.50 MiB
  create/release/live = 90 / 87 / 3
  remote_claim / drain / drain_slots / drain_invalid = 442665 / 156058 / 490209 / 0
  cap_reject = 0

medium_interleaved_remote50, R3 ASLR-off:
  HZ8 24.499M reference in retry run
  HZ9 24.499M
  post RSS 5.08 MiB
  peak RSS 9.73 MiB
  create/release/live = 111 / 98 / 13
  remote_claim / drain / drain_slots / drain_invalid = 325025 / 203365 / 346034 / 0
  cap_reject = 0

decision:
  GO as L0 evidence. Owner drain fixes the dirty-drop/switch failure mode:
  remote rows no longer crash, no segment storm, cap_reject remains zero, and
  remote throughput is near or above HZ8 while local rows still retain the
  ProductEntry advantage.
```

## ProductEntry Lifecycle L0

```text
policy:
  thread shutdown drains pending into entry-local bits, syncs segment bits,
  then releases each active ProductEntry segment.

  cached segments stay route-visible and generation-bump on cache pop.
  raw-released segments are removed from the route hash before platform release.
  hash removal uses a tombstone, not zero, so open-addressing probe chains are
  preserved.

hard gates:
  raw release hash-remove smoke pass; stale pointer route visibility = 0
  pending lost during shutdown = 0
  cache while pending = 0
  cap_reject on owner-drain rows = 0
```
Lifecycle check:
```text
bench_results/20260703T_hz9_product_lifecycle_l0_r10
medium_local0, R10 ASLR-off:
  throughput 192.982M, post/peak RSS 3.40 / 3.44 MiB
  create/release/live=48/0/48, cap_reject=0
main_local0:
  throughput 162.495M, post/peak RSS 3.69 / 3.71 MiB
  create/release/live=32/0/32, cap_reject=0
medium_interleaved_remote50:
  throughput 27.486M, post/peak RSS 9.03 / 10.49 MiB
  remote_claim/drain/drain_slots/drain_invalid=1070193/674462/1141338/0
main_interleaved_remote90:
  throughput 22.045M, post/peak RSS 10.52 / 14.11 MiB
  remote_claim/drain/drain_slots/drain_invalid=1531470/533162/1677467/0

read:
  R10 keeps local/remote throughput strong, cap_reject zero, and drain_invalid
  zero. Cached live segments intentionally raise retained RSS versus HZ8; this
  remains the explicit HZ9 throughput-vs-retention tradeoff.
```

## Contract Split

```text
immediate fail-closed remains:
  miss vs owned route
  interior / tail / misaligned pointer
  stale generation once route fallback runs
  same-thread double free through last-token miss + fallback sync

delayed in future MT design:
  foreign per-slot double-free classification may belong to owner drain

not allowed:
  ledger/last-token miss may not forward directly to platform
  preload integration may not rely on unsynchronized segment slot bits
```
