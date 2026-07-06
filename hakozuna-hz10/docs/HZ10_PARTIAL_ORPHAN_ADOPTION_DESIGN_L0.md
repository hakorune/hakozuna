# HZ10 Partial Orphan Adoption Design L0

Design + ownership proof for HZ10PartialOrphanAdoption-L1: letting a live
thread adopt an orphan page THAT STILL HAS LIVE SLOTS, so new allocations
fill its free capacity instead of paying for fresh pages. Review document;
do not implement until the proof obligations below survive review.

## Measured motivation (HZ10OrphanResidualCensus-L0, 20260706)

```text
larson 4t/128c, hz10+orphan lane, steady state census (RUNS=3 median):
  max RSS ~2.687GB
  orphan_unadopted: 32,332 pages = 2.119GB  <- 79% of RSS
  live_slots in those pages:      32,333
  free_slots in those pages:   5,466,xxx
  hidden_free (undrained):     0
  dominant class: 8 (384B), 32,327 pages
```

One live 384B object pins one 64KiB page. Idle-only adoption can never
touch these (they are not idle and may stay non-idle for the object's
lifetime). Free capacity trapped: ~5.47M slots x 384B ~= 2.0GB. Filling
that capacity with new allocations is the only shape that recovers it
without moving live objects (slots are smaller than the OS page granule,
so madvise cannot; object migration is out of the question for a malloc).

## Invariant inventory: what "ownership" actually protects

Owner-only, plain (non-atomic) state of a page:

```text
O1  local_free_head / free_count       (alloc, local free, drain merge)
O2  in-slot marker writes during drain/alloc
O3  next/prev_in_owner_list links      (class list membership)
O4  drain right: exchanging stripe heads + clearing pending bits
    (multi-producer push side is owner-agnostic; the DRAIN side is
    owner-only because it merges into O1)
```

Owner-agnostic, already multi-thread safe regardless of owner liveness:

```text
A1  remote free claim/publish (pending fetch_or + stripe CAS)
A2  pagemap route/validation, generation
A3  retired_ready machinery (guarded by its own flags/CAS)
```

Adoption = transferring O1-O4 from an EXITED owner record to a live one.

## Transfer protocol

Registry: per-class list(s) of orphan pages, filled at thread exit by the
existing orphan-publish step, protected by a registry lock (adoption is a
slow-path event; a plain mutex is acceptable v1 -- measure before getting
clever).

Adopting thread, inside the malloc slow path (same hook as idle adoption,
after find/harvest miss, before fresh-page creation):

```text
T1  lock registry; pop ONE page of the wanted class; unlock.
    (Pop grants exclusivity: the page is now reachable by no other
    adopter and by no registry scan.)
T2  set page->owner = my owner record   (plain store suffices, proof P3)
    set page->class_id unchanged (sanity-check it)
T3  drain stripes (owner right O4 now mine), merging into local_free_head
T4  link into my class active list via the normal
    hz10_class_pages_add() (eviction rules apply unchanged)
T5  allocate from it like any active page.
Rejection path (e.g. class mismatch, or the page drained to fully idle
and we prefer destroy): idle -> destroy via the normal guarded path;
otherwise re-push to registry under the lock.
```

## Proof obligations

P1 (exclusivity). Only the registry pop hands out a page, under the lock;
    thread exit publishes each page exactly once (it removes the page from
    the dying thread's own lists first, so no page is simultaneously in a
    TLS class list and the registry). Two adopters cannot hold the same
    page. The existing idle-adoption scan must ALSO follow pop-probe-
    re-push discipline -- if today's implementation probes (drains) pages
    while they sit in the registry, that is a latent violation of O4 to
    fix in this box, not to inherit.

P2 (dead owner never returns). The owner record is persistent (mmap-
    backed, never freed) and its EXITED state is set by the exiting
    thread BEFORE it publishes pages to the registry, with the registry
    publish being a release operation. An adopter's pop is the matching
    acquire, so the adopter observes every plain write the dead owner
    ever made to O1-O3 (its last drains/frees happened-before exit
    publish). Nothing else can write O1-O4 between publish and pop (P1).

P3 (the free-path owner compare needs NO added ordering). hz10_free's
    local/remote decision compares page->owner against the CALLING
    thread's own record. A record pointer value equal to "mine" is only
    ever stored into page->owner by me (T2 runs on the adopter). By
    per-location coherence a thread cannot read a value older than its
    own last write, so:
      - the adopter, after its own T2, correctly takes the local path;
      - every other thread reads either the EXITED record or the
        adopter's record -- both unequal to its own -- and takes the
        remote path, which is owner-agnostic (A1). A stale read is
        therefore never unsafe, only conservatively remote.
    This is the same self-write/coherence argument review #1 substituted
    for the unsound store-ordering claim in the entry-trim E4 section.

P4 (in-flight remote frees). A foreign thread mid claim()/publish()
    against the page while it is adopted: pending bits and stripe CAS
    are owner-agnostic (A1); the adopter's subsequent drains observe the
    slot exactly as the dead owner would have. The claim->note->publish
    ordering contract is untouched. The adopter may drain (T3)
    concurrently with such publishes -- that is the normal owner/producer
    relationship, not a new race.

P5 (adopter already holds live objects from this page). Possible: the
    dead owner allocated the object, handed it to the (future) adopter.
    Before T2 the adopter's free of it took the remote path (safe);
    after T2 it takes the local path (it IS the owner now; O1 is its
    own). During T1-T4 the adopter is single-threadedly inside the
    malloc slow path and cannot concurrently call free, so there is no
    self-race window. State this as a code-structure requirement:
    adoption must not call out to anything that can reenter the public
    entry (no logging through malloc, etc.).

P6 (no interaction with destruction). An orphan page in the registry is
    in nobody's retired list and nobody's ready stack (thread exit
    publishes ACTIVE pages; the dead owner's RETIRED list is explicitly
    out of scope, a separate box, same as idle adoption today). The only
    destroy path for a registry page is the adopter's own rejection
    branch after pop (exclusive by P1), which reuses the existing
    guarded idle-destroy. Generation/pagemap state never changes during
    adoption, so route stays valid throughout.

P7 (front-cache lane compatibility). Front cache (opt-in) holds slots of
    pages owned by the CALLING thread only. An adopted page's slots can
    enter the adopter's front cache after T2 like any owned page. Dead
    owners' front caches died with them (TLS) -- their cached slots were
    returned at flush or leaked with the thread; adoption does not
    change that story either way. (If the front lane's thread-exit
    behavior ever changes, revisit.)

## Policy knobs (v1 fixed, tune box later)

```text
adopt trigger:   malloc slow path, after find/harvest miss, before fresh
adopt count:     1 page per miss (the larson shape hands 100+ free slots
                 per adopted page; batching is not needed for v1)
class match:     exact class only (no cross-class adoption)
registry order:  LIFO (recently orphaned pages are cache-warmer and
                 statistically emptier)
```

## Expected effect and gates

Expected on the larson census shape: new allocations consume the ~5.47M
trapped free slots before any fresh page is created; orphan_unadopted
bytes converge toward live-set density. Order-of-magnitude: 2.69GB ->
sub-1GB, i.e. 9.6x tcmalloc -> ~2-3x. Do not promise the endpoint;
measure.

```text
gates:
  census: orphan_unadopted pages/bytes collapse; adopted grows then
    recycles; hidden_free stays 0
  larson: throughput stays competitor-range (2.0M+ ops/s); RSS target
    directional (>=50% cut vs the 2.69GB baseline)
  python/redis rows: unchanged (adoption should never trigger there --
    assert adopt_count ~= 0 in their exit stats)
  micro matrix + rss-guard + full smokes: unchanged (default lane is
    untouched; this extends the opt-in orphan lane)
  new smokes: third-thread free of adopted-page slot (P4); adopter
    frees its own pre-adoption object (P5); double-adoption exclusivity
    + registry re-push (P1); TSan hammer: threads exiting while others
    free into their pages while others adopt.
rollback: extends HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION only; flag off is
  byte-identical default.
```

## Implementation probe (20260706)

Implemented as a sibling opt-in lane, leaving the idle-only orphan lane as
the reference:

```text
libhz10_orphan.so:
  HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1

libhz10_orphan_partial.so:
  HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1
  HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1
```

Open Q1 audit result: the idle-adoption path pops a page from the registry
before probing/draining; it does not drain in-place while the page remains
registry-linked. The partial lane uses the stricter transfer shape: pop,
owner transfer, drain, and class-list insertion only if free capacity is
available. If no capacity remains, it restores the old exited owner and
re-pushes the page.

RUNS=3 larson 4t/128c/2s probe:

```text
allocator              throughput   current_rss_kb
hz10+orphan            2.069M       2,817,024
hz10+orphan-partial    2.085M         733,568
```

Census median:

```text
bucket                       hz10+orphan             hz10+orphan-partial
orphan_unadopted pages       32,329                  3
orphan_unadopted bytes       2,118,713,344           196,608
class 8 orphan pages         32,324                  1
class 8 adopted pages        0                       273
class 8 adopted live slots   0                       32,583
```

Default-candidate matrix RUNS=3:

```text
python_alloc idle-only partial: 0.920s / 116,848 KiB
python_alloc partial:           0.940s / 116,876 KiB
redis_setget idle-only partial: 0.540s / 8,064 KiB client
redis_setget partial:           0.540s / 8,064 KiB client
larson idle-only partial:       4.305s / 2,687,104 KiB
larson partial:                 4.215s /   601,216 KiB
```

Verdict: GO for LD_PRELOAD shim default. Source compile-time defaults stay off
so public-entry and front-cache research lanes remain isolated; `make preload`
now builds `libhz10.so` with orphan + partial adoption, and `make preload-base`
builds the former no-orphan shim as `libhz10_base.so`.

Shim default confirmation RUNS=2:

```text
larson hz10      4.181s /   602,752 KiB
larson hz10-base 4.205s / 9,216,704 KiB
python/redis     unchanged within this short confirmation
```

## Open questions for reviewers

```text
1. P1 retrofit: does today's idle-adoption scan probe pages in-place in
   the registry (drain-before-pop)? If yes it already exercises O4
   without exclusivity and this box must fix that first -- audit before
   building on it.
2. Registry lock: global mutex vs per-class. Adoption frequency in
   larson is bounded by page-fill rate (1 adoption per ~100 allocs of
   that class); v1 global mutex looks safe -- confirm with the adoption
   counter before optimizing.
3. Should the adopter prefer registry pages over its OWN retired pages
   (harvest currently runs first)? v1: keep harvest first (own pages are
   warmer); revisit if census shows registry starvation.
4. Adopted-page eviction: an adopted page evicted to the adopter's
   retired list, then the adopter exits -- the page re-orphans through
   the same publish path. Confirm the exit publish walks retired as well
   as active for ADOPTED pages, or accept retired-orphan loss as the
   already-named separate box (recommended: the separate box, keep this
   one narrow).
```
