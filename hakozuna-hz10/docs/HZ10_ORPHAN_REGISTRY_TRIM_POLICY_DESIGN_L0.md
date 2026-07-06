# HZ10 Orphan Registry Trim Policy Design L0

Design memo for HZ10OrphanRegistryTrimPolicy-L0: reclaim stale, fully-idle
orphan-registry pages so HZ10's post-workload RSS approaches the HZ8 public
contract WITHOUT regressing the larson/thread-churn throughput that the
registry exists to protect. Review document; do not implement until the
proof/gate plan below survives review.

Status: design + probe implemented; trim implementation deferred.

## Review update: probe before trim

The policy direction is GO, but the first implementation must be a diagnostic
probe, not a trim implementation.

Two timing facts matter:

```text
1. HZ8 bench_matrix post_rss is sampled before process exit.
   Therefore an atexit trim is good hygiene, but it cannot improve the
   measured post_rss target row.

2. Most orphan pages are published at worker thread exit, immediately before
   post_rss is sampled. A seconds-scale AGE_T will not fire before that
   measurement point unless there is an explicit quiescent/purge boundary or
   a delayed measurement.
```

So `publish + age + atexit` may be correct for long-running processes, but it
is not yet proven to move the HZ8-style post_rss gate.

First box:

```text
HZ10OrphanRegistryIdleAgeProbe-L0
```

The probe records, without changing allocator behavior:

```text
registry depth by class
fully-idle pages (free_count == slot_count)
live-pinned pages (free_count < slot_count)
hidden pending remote frees
age buckets from orphaned_at_ns to dump time
tail-side age/idle distribution by reading the current registry order
```

Only after this shows a material fully-idle aged population should the trim
policy below be implemented. If live-pinned pages dominate, this box should
NO-GO into a partial-adoption/object-lifetime line rather than trim.

## Probe result (HZ10OrphanRegistryIdleAgeProbe-L0, 20260707)

Implemented an opt-in, behavior-neutral shim probe:

```text
HZ10_SHIM_ORPHAN_REGISTRY_PROBE=1
```

The probe stamps `orphaned_at_ns` when pages enter the registry and dumps
per-class depth, fully-idle pages, live-pinned pages, hidden pending frees,
and age buckets at process exit. It does not trim, drain, adopt, or otherwise
change allocator behavior.

HZ8 public harness, HZ10 preload, THREADS=16 ITERS=50000 RUNS=3 median:

```text
row                         post RSS   depth  idle  live-pinned  hidden pending  age<100ms
small_interleaved_remote90   44.2MB      646    16      630          42,813        646
main_interleaved_r90         96.1MB    2,763     6    2,756          22,359      2,763
medium_interleaved_r50       72.6MB    3,139    29    3,107           4,822      3,139
main_local0                  35.1MB      570   570        0               0        570
medium_local0                 6.3MB      160   160        0               0        160
```

Verdict:

```text
- remote-heavy rows are overwhelmingly live-pinned, not fully idle.
- local rows are fully idle and are the only obvious idle-trim win.
- all sampled pages are <100ms old in the short harness, so a seconds-scale
  age policy cannot move the current post_rss gate without an explicit
  quiescent/delayed boundary.
- hidden pending frees are large on remote-heavy rows; any remote-heavy fix
  needs a drain/adopt/quiescent ownership protocol, not blind idle destroy.
```

Therefore HZ10OrphanRegistryTrimPolicy-L0 should not land as a broad default
RSS fix yet. The clean next box is either a narrow local/explicit-quiescent
idle trim, or a separate remote-heavy drain/adoption policy that can turn
hidden pending/live-pinned orphan capacity into reusable pages before destroy.

Record:

```text
bench_results/20260707T_hz10_orphan_registry_idle_age_probe_l0/
```

## Measured motivation (HZ10PostRssResidualAttribution-L0, 20260707)

HZ8-style public-matrix rows (THREADS=16 ITERS=50000 RUNS=3,
`hakozuna-hz8/bench/bench_matrix_malloc.c`), median:

```text
row                         post RSS    orphan depth   orphan capacity bytes
small_interleaved_remote90   47.3MB        700           45.9MB
main_interleaved_r90         93.5MB      2,731          179.0MB
medium_interleaved_r50       62.3MB      3,030          198.6MB
main_local0                  35.4MB        570           37.4MB
medium_local0                 6.4MB        160           10.5MB
```

Active + retired + pool + metadata together account for ~0.6-3.7MB on every
row. The remainder -- 90%+ of post RSS on the remote-heavy rows -- is the
orphan adoption registry depth printed by `hz10_shim_orphan_adoption_stats`
(`src/hz10_public_entry_owner.c`). Those pages are no longer in any live
owner's class lists (so they do not appear in `class_totals`), but they
remain mapped as future adoption candidates. (Capacity bytes can exceed RSS
because not every retained quantum is resident at sampling time; the
direction is what is decisive.)

## The two-edged registry

The orphan registry is simultaneously:

1. The larson/thread-churn throughput fix. Without it, every new thread in a
   churn workload allocates fresh pages and RSS climbs without bound (the
   "orphan RSS cliff" the orphan/partial adoption series closed -- see
   `HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md`, where larson went from
   ~9.2GB base to ~0.6GB with adoption). Pages from exited owners are
   recycled to new owners instead of being re-minted.
2. The post-workload RSS residual. When adoption demand falls behind inflow
   (or the workload ends), the registry retains pages that were published
   but never re-adopted. They sit mapped until process exit.

`HZ10PartialOrphanAdoption-L1` (already shipped, shim default via
`make preload`) attacks the THROUGHPUT side: it lets a live thread adopt a
registry page that still has free capacity, so new allocations fill trapped
slots. That box REDUCES the registry by consuming it. This box is the
complement on the RSS side: it REMOVES registry pages that have gone stale
and idle, so the registry does not retain capacity that adoption is no
longer asking for. The two are independent and both needed -- partial
adoption does not bound the residual on rows where adoption demand has
stopped (no thread is allocating that class at that moment).

## What "trim" may and may not touch

Inventory of the constraints, carried over from the partial-adoption proof
(`HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md` P1/P6) and the attribution
guardrails:

```text
C1  only fully orphaned registry pages. A registry page is by construction
    in no live owner's class list and no retired/ready list (thread exit
    publishes ACTIVE pages; the dead owner's RETIRED list is a separate
    box). Trim must never touch a page still reachable from a live owner.
C2  only FULLY IDLE pages: free_count == slot_count. Registry pages may
    carry live objects (the partial-adoption motivation measured 32k
    pages pinned by one live 384B object each). A page with live objects
    cannot be reclaimed -- those objects' memory is still handed out.
    Such a page stays until its live objects are remote-freed (landing in
    its stripes, owner-agnostic A1) and a later sweep finds it idle, or
    until a live thread adopts it via the partial path.
C3  pop exclusivity. The registry pop is the exclusive transfer operation
    (P1). Trim must take a page under the registry lock before destroying
    it, exactly as adoption does -- never destroy a page that is still
    registry-linked. The existing guarded idle-destroy (the adopter's
    rejection branch) is the destroy path to reuse.
C4  pagemap release before unmap. Destroy = unregister from the pagemap
    (route can no longer resolve the base), then return the quantum to the
    pool / madvise / munmap. Order is load-bearing: a foreign thread's
    in-flight free must route-fail (fail-closed) rather than touch freed
    memory.
C5  no hot-path cost. Trim runs on the publish path (owner exit) and at
    process exit -- never on malloc/free. The speed campaign just spent
    five NO-GO boxes establishing that the hot path is at a floor; this
    box must not add per-op work back.
C6  census is unreliable for short rows. The attribution note: the census
    sampler may not wake before a short benchmark exits. So the trim's
    reliable triggers are publish-path and atexit, not the census thread.
    (A census-ridden sweep can be an opt-in add-on for long-running
    servers, but it is not the primary trigger.)
```

## Mechanism

### Reclaim unit

One registry page, taken from the cold (oldest) end of its class list. The
registry is LIFO for adoption (`HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md`
policy: recently orphaned pages are cache-warmer and emptier). Trim therefore
reclaims from the OPPOSITE end -- the tail -- so it removes the pages least
likely to be adopted next and never starves the warm adoption head.

### Per-page timestamp

Stamp `orphaned_at_ns = hz10_platform_now_ns()` when a page is published to
the registry (`hz10_orphan_append_active`, `src/hz10_public_entry_owner.c`).
The clock already exists (`hz10_platform_now_ns`). This is the only new
per-page state. HZ10OrphanRegistryIdleAgeProbe-L0 may add this field before
trim exists; it is observational until a trim policy reads it for reclaim.

### Reclaim predicate

A registry page is reclaimable when, under the registry lock:

```text
R1  popped from its class list (exclusivity, C3);
R2  free_count == slot_count (fully idle, C2);
R3  (now_ns - orphaned_at_ns) >= AGE_T   -- aged out, OR
    class depth > HIGH_WATER              -- cap safety (any age).
```

If R2 fails (live objects remain), repush and move on -- the page stays a
valid adoption candidate. If R1/R2 hold and R3 holds, destroy via the
existing guarded idle-destroy path (C3/C4): pagemap unregister, then pool
return / madvise. Releasing the lock only after the page is unregistered
keeps a concurrent adopter from ever observing a half-destroyed page.

### Triggers

```text
T-publish: on owner exit, in hz10_orphan_append_active AFTER appending the
           dying owner's active pages, sweep a bounded budget (K pages) of
           the classes just touched: pop-from-tail, apply R1/R2/R3, destroy
           or repush. Bounded budget keeps exit latency flat regardless of
           registry size. Fires on every thread exit -- the inflow point --
           so it runs precisely when the registry is growing.
T-atexit:  drain the whole registry at process exit (shim atexit hook).
           Pop every class list tail-to-head; destroy fully-idle pages;
           repush+leave any with live objects (they will be cleaned up by
           process teardown anyway, but the idle bulk is returned first).
           This zeroes the exit residual and is good hygiene regardless of
           the in-workload measurement. It does NOT improve the current
           HZ8 bench_matrix post_rss metric, which samples before atexit.
T-census:  OPTIONAL opt-in: if HZ10_SHIM_CENSUS_SEC is set, ride a trim
           sweep on the existing census thread for long-running servers
           where publish/atexit do not fire often enough. Not the primary
           trigger (C6).
```

### Caps (safety bound)

```text
HZ10_ORPHAN_REGISTRY_CAP_PAGES (per class)   -- default tuned per lane
HZ10_ORPHAN_REGISTRY_CAP_BYTES  (global)     -- optional product profile
```

The cap is a SAFETY bound, not the primary drainer. Age (R3) is what drains
the residual below the steady churn depth; the cap only bounds the worst
case where age alone is too slow (a burst of exits on one class). The cap
must be set ABOVE the measured steady-state per-class depth under churn so
it does not trigger during normal larson operation (see Gates).

Why age, not just cap: at end of workload the registry depth is typically
BELOW any reasonable cap (churn has equilibrated), so a cap-only policy
would leave the residual intact. Age is what distinguishes "churning fast,
keep" (young pages, adoption imminent) from "stale, no demand, reclaim"
(old pages). The cap cannot make that distinction.

## Policy knobs (v1, fixed; tune box later)

```text
AGE_T:        seconds an orphan page must sit unadopted before it is
              reclaimable. Must be >> adoption latency under churn (ms) so
              live-cycle pages never age out, and << the post-workload
              measurement window so the residual drains in time. v1
              default conservative (high); tighten in the tune box.
HIGH_WATER:   per-class depth above which R3's age requirement is waived
              (reclaim oldest idle regardless of age) down to LOW_WATER.
              Set above measured steady-state per-class churn depth.
LOW_WATER:    depth to reclaim down to when HIGH_WATER triggers.
K:            publish-path sweep budget (pages examined per exit). Small
              constant; exit latency must stay flat.
```

## Why this preserves larson (the make-or-break argument)

Under steady thread churn (larson 4t/128c), the time between a page being
orphaned and re-adopted is milliseconds (a new thread starts, hits a malloc
slow-path miss, and pops the page). `AGE_T` is in seconds, so
`AGE_T` >> adoption latency, and live-cycle pages are adopted (removed from
the registry by the adopter's pop) long before they satisfy R3. The
publish-path sweep therefore reclaims nothing during healthy churn -- it
only reclaims pages that have genuinely gone stale (no adopter came for
them within AGE_T). `HIGH_WATER` is set above the measured steady per-class
depth, so the cap does not trigger during normal operation either.

The residual fix is the symmetric case: when churn stops or the workload
ends, no adopter comes, pages age past R3, and the next publish-path sweep
(or the atexit drain) reclaims them. The same self-tuning property makes
the policy safe under churn and effective at quiescence.

## Expected effect and gates

Target the attribution rows directly:

```text
row                         post RSS now    target (toward live+metadata)
small_interleaved_remote90   47.3MB          ~1-2MB
main_interleaved_r90         93.5MB          ~3-4MB
medium_interleaved_r50       62.3MB          ~3-4MB
main_local0                  35.4MB          ~1MB
medium_local0                 6.4MB          ~0.5MB
```

(The endpoint is not promised; measure. The live+metadata floor is the
theoretical bottom since trim cannot reclaim pages with live objects.)

Gates:

```text
G-RSS (target):  post RSS on the HZ8-style rows drops toward live+metadata;
                 orphan depth collapses in hz10_shim_orphan_adoption_stats.
G-larson (CRITICAL):  larson 4t/128c current_rss stays bounded (the cliff
                 must NOT return). This is the whole reason the registry
                 exists; regressing it is immediate NO-GO. Also confirm
                 larson throughput stays in the competitor range.
G-macro (wall):  python_alloc / mstress / sh6bench wall-time flat. Trim is
                 publish-path + atexit only; it must not add malloc/free
                 per-op cost (C5).
G-idle-only:     no page with free_count < slot_count is ever destroyed.
                 New smokes: reclaim skips a page with a live slot then
                 reclaims it once that slot is remote-freed; concurrent
                 adopter vs trim never observe the same page (P1/C3).
G-fail-closed:   a foreign free of a pointer into a just-trimmed page
                 route-fails (pagemap unregister happened before unmap, C4).
G-default-lane:  micro matrix + rss-guard + full smokes unchanged. Ship as
                 an opt-in compile/env lane first (matching
                 HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION's rollout); flip to
                 shim default only after G-larson holds across a parameter
                 sweep.
```

Parameter sweep (after the idle/age probe, before any default flip):

```text
AGE_T in {1s, 4s, 16s, 60s} x HIGH_WATER in {steady, 2x, 4x}
measure: HZ8 rows (RSS target) + larson (RSS cliff guard) + macro (wall)
find the widest AGE_T/HIGH_WATER band where G-RSS improves AND G-larson holds.
If the band is narrow or absent, age-based is at its limit and the box
reopens as demand-adaptive (adoption hit-rate driven) instead.
```

## GO / NO-GO

```text
GO (to shim default):
  G-RSS: HZ8-style post RSS drops materially toward live+metadata;
  G-larson: larson RSS cliff stays closed AND throughput in range;
  G-macro: no wall-time regression;
  G-idle-only / G-fail-closed smokes green;
  a non-trivial AGE_T band satisfies the above (not a knife-edge tuning).

NO-GO:
  G-larson regresses (the registry's purpose is broken), OR
  G-macro regresses (trim leaked onto the hot path), OR
  G-RSS does not move because residual pages mostly carry live objects
  (meaning the residual is not idle-stale but live-pinned, and trim is the
  wrong tool -- the fix would be on the partial-adoption / object-lifetime
  side instead).
```

## Implementation shape (preview, not commitment)

```text
- extend Hz10FreelistPage (or a registry-side shadow node) with
  orphaned_at_ns; stamp in hz10_orphan_append_active.
- new hz10_orphan_trim_class(class_id, budget, now_ns) under the registry
  lock: pop-from-tail loop, apply R1/R2/R3, destroy-idle or repush.
- call trim from hz10_orphan_append_active (publish path, bounded budget)
  and from a shim atexit hook (full drain).
- reuse the existing guarded idle-destroy (adopter rejection branch) for
  C3/C4 -- do not write a second destroy path.
- opt-in lane first: HZ10_ENABLE_ORPHAN_REGISTRY_TRIM, sibling .so (e.g.
  libhz10_orphan_trim.so) mirroring the partial-adoption rollout.
```

## Open questions for reviewers

```text
1. AGE_T vs adoption latency: what is the measured p99 time-to-readopt under
   larson 4t/128c? If it is within an order of magnitude of a sub-second
   AGE_T, the safe band may be too narrow and demand-adaptive should be
   preferred from the start. Measure before fixing AGE_T.
2. Per-class vs global cap: the attribution depth is per-class (class 8
   dominates larson). A global CAP_BYTES interacts with class skew; a
   per-class CAP_PAGES is cleaner. v1 per-class, revisit if a skew-heavy
   workload leaves one class over-retained.
3. Publish-path budget K vs exit latency: exiting a thread that owned many
   pages already does work; the trim sweep must not make exit latency
   unbounded. K should be a small constant with continuation across exits
   (if the class is still over cap, the NEXT exit continues the sweep).
4. Interaction with partial adoption: when a live thread adopts a page, it
   pops from the warm (head) end; trim reclaims from the cold (tail) end.
   Confirm the registry structure supports O(1) tail removal (or accept
   O(depth) sweep, bounded by K). If the registry is singly-linked LIFO,
   tail removal needs a second pointer or a different shape -- decide
   before implementing.
5. Live-pinned residual: if G-RSS does not move because residual pages
   carry live objects (NO-GO clause), the real lever is object-lifetime /
   partial-adoption reach, not trim. State the diagnostic (orphan pages
   with free_count < slot_count dominate) so the NO-GO routes correctly.
6. Retired-list orphans: thread exit publishes ACTIVE pages today
   (HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md P6 / open Q4). Trim inherits
   that scope -- it cannot reclaim retired-list orphans until the separate
   retired-orphan box lands. Confirm the measurement rows are active-only.
```
