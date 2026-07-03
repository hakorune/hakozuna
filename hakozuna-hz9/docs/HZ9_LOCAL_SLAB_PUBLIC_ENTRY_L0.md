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
