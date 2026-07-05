# HZ10ThreadExitOwnershipHandoff-Design-L0

Status: design record; persistent owner-record prep implemented,
orphan adoption not started.

Purpose: turn the larson thread-churn RSS finding into a safe implementation
plan. `HZ10LarsonThreadChurnAttribution-L0` showed that 94.2-94.5% of HZ10
current RSS in the larson sweep is active pages owned by already-exited
threads. `retired_pages=0` in that sweep, so the first fix should target
abandoned active pages, not retired-list reclaim or page-pool tuning.

## Verdict

Do not implement automatic destructor reclaim.

Implement ownership handoff in two separate boxes instead:

```text
1. HZ10PersistentOwnerRecord-L1
   Replace the TLS-address owner token with a persistent owner record pointer.
   Thread-local state becomes a pointer to an mmap-backed owner record that
   stays valid after pthread exit while any page may still reference it.

2. HZ10OrphanActiveAdoption-L1
   On pthread exit, mark the owner record exited and enqueue its active page
   lists into a global orphan registry. Later allocating threads may adopt
   orphan active pages for the same class and reuse their capacity.
   No page destruction in this box.
```

This is intentionally narrower than "free everything at thread exit". It
attacks the measured larson RSS driver by letting future threads reuse pages
that would otherwise be permanently abandoned, while preserving the existing
remote-free claim/publish safety rules.

## Why Destructor Reclaim Is Still Forbidden

`hz10_public_entry_flush_thread_cache_quiescent()` requires a true quiescent
boundary:

```text
No thread may allocate from, locally free into, or remote-free into one of the
calling thread's pages, including a remote free mid claim()/publish().
```

Ordinary pthread exit does not imply that. Another thread can still hold and
free an object whose page was created by the exiting thread. Existing
`tests/README.md` also records a known page-destroy gap: a caller must drain
before destroy, and a new foreign push racing with destroy is not defended
against. Therefore a destructor that destroys active pages would be a
correctness bug, even if it improves RSS in a benchmark.

## Current Hazard

Today each page stores:

```text
page->owner_thread_token = &hz10_thread_token_storage
```

That address is a TLS address. Once a pthread exits, a future pthread may get
the same TLS address. A stale page whose owner token still equals that address
can then be misclassified as local to the new thread. That is exactly the
wrong direction: a new thread could push a free into a page list it does not
own.

The next implementation must remove this token-ABA hazard before any orphan
adoption or handoff logic is trusted.

## Design Shape

### 1. Persistent Owner Record

Introduce a process-private owner record:

```text
typedef struct Hz10ThreadOwner {
  _Atomic(uint32_t) state;       // LIVE, EXITED, ADOPTING
  Hz10ClassState classes[HZ10_CLASS_COUNT];
  struct Hz10ThreadOwner* next_orphan;
  uint64_t owner_id;             // diagnostic only
} Hz10ThreadOwner;
```

Thread-local public entry state becomes:

```text
static _Thread_local Hz10ThreadOwner* hz10_current_owner;
```

`hz10_current_owner` is lazily allocated from allocator-private mmap metadata,
not from libc malloc. Page creation writes:

```text
page->owner_thread_token = hz10_current_owner;
```

The owner record is never recycled while any page can still reference it.
This makes owner identity stable across pthread exit and prevents TLS address
reuse from creating a false local-free match.

### 2. Atomic Owner Token

`Hz10FreelistPage.owner_thread_token` must become an atomic pointer before it
is updated by handoff/adoption:

```text
_Atomic(void*) owner_thread_token;
```

Rules:

```text
create:
  store current owner record with release semantics after page initialization.

free:
  load owner token with acquire semantics before local-vs-remote dispatch.

thread exit:
  store an exited owner/orphan-visible state only after the owner no longer
  runs allocator code.

adoption:
  compare_exchange page owner from exited owner record to the adopting owner
  record while holding the orphan registry lock for list ownership.
```

The pagemap `owner` field stays the page pointer. Do not try to update
pagemap ownership for this box.

### 3. Thread Exit Handoff

The pthread TLS destructor may do only these things:

```text
1. flush front-cache objects back into their pages, if front cache is enabled;
2. mark the persistent owner record EXITED;
3. enqueue non-empty per-class active lists into the global orphan registry;
4. emit optional diagnostic stats.
```

It must not:

```text
- call hz10_public_entry_flush_thread_cache_quiescent();
- destroy pages;
- drain remote frees for reclaim;
- unregister pagemap records;
- return page bases to the page pool.
```

### 4. Orphan Active Adoption

On the allocation slow path, after the thread's own active list misses and
before creating a fresh page, the owner may try:

```text
hz10_orphan_registry_try_adopt(class_id, current_owner)
```

The registry returns at most one active page for the requested class. The
adopting thread:

```text
1. atomically changes page owner from exited_owner to current_owner;
2. drains remote frees from that page;
3. prepends the page to current_owner->classes[class_id].list.active;
4. allocates from it if capacity exists.
```

If no capacity exists after drain, keep the page owned by the adopter and let
the existing active-list eviction/retired machinery handle it later. Do not
destroy it in the adoption box.

### 5. Retired Pages

Do not adopt or reclaim retired pages in `HZ10OrphanActiveAdoption-L1`.

Reason: retired pages can have `retired_ready_stack` pointing at the owner's
class state. Making the owner record persistent keeps that pointer valid after
pthread exit, but transferring retired ready/cancel state safely is a separate
box. The larson attribution that opened this work had `retired_pages=0`, so
retired handoff is not needed for the first measured win.

Retired pages in an exited owner record remain safe but retained. A later
`HZ10OrphanRetiredReclaim-L1` would need its own ready-stack transfer or
hazard/epoch protocol.

## Required Gates

Correctness:

```text
smoke-shim-api
smoke-shim-foreign
public-entry smoke, including cross-thread free cases
ASan/UBSan public-entry and shim smokes
smoke-tsan-aslr-off
hz10-standalone-check
```

New smokes:

```text
owner token ABA:
  Create a page in thread A, let A exit through the handoff destructor, then
  create many thread B instances. A stale page must never local-free through a
  reused TLS address. With persistent owner records, this should be impossible.

orphan active adoption:
  Thread A allocates/frees enough to leave active pages, exits, thread B
  allocates the same class, and B must reuse/adopt at least one orphan page
  before creating fresh pages.

remote free during exit:
  Thread A exits after handing off active pages while thread B frees objects
  from A's pages. Accepted frees must remain drainable by the eventual adopter.
```

Bench gates:

```text
make bench-larson-thread-churn-attribution
make bench-macro-matrix
make hz10-rss-guard
```

Primary expected win:

```text
larson HZ10 current RSS should move toward glibc/tcmalloc/mimalloc instead of
the current multi-GB orphan-page footprint.
```

Guardrails:

```text
- No default public-entry micro row may regress by more than measurement noise.
- hz10+fine remains diagnostic only unless a separate macro matrix says
  otherwise.
- No page destruction may be added to the pthread destructor in this box.
```

## Open Questions For Review

```text
1. Should orphan adoption be default-on once the owner-record refactor lands,
   or env/compile-flag gated for the first macro A/B?

2. Should the orphan registry be one global mutex with per-class lists first,
   or per-class locks immediately? Recommendation: one global lock for L1,
   because adoption is slow-path and the macro signal is RSS, not hot-path
   throughput.

3. Should exited owner records be reused after all lists are empty?
   Recommendation: no in L1. Owner-record reuse reopens the identity ABA class
   this box exists to close. Add reuse only with a generation/hazard proof.

4. How should large allocations behave? Recommendation: unchanged. Large
   allocations are registered with large flags and freed directly; this box is
   for small-class public-entry pages only.
```

## Next Implementation Pointer

`HZ10PersistentOwnerRecord-L1` is the prep box. It has a clear rollback
boundary: the token type and TLS-state access change, but no adoption or
reclaim policy changes yet. Once that is green, implement
`HZ10OrphanActiveAdoption-L1` as an opt-in lane and run the larson
attribution/macro matrix A/B.

## Persistent Owner Record Implementation Record

Implemented:

```text
- `Hz10FreelistPage.owner_thread_token` is now an atomic pointer with
  helper load/store accessors.
- `src/hz10_public_entry.c` no longer uses a TLS char address as the owner
  token. Each thread lazily allocates an mmap-backed persistent owner record,
  and pages store that record pointer as their owner identity.
- Per-class state moved from a `_Thread_local Hz10ClassState[]` array into
  the persistent owner record.
- No orphan adoption, destructor handoff, page destruction, or reclaim policy
  was added in this prep box.
```

New regression smoke:

```text
tests/hz10_public_entry_owner_smoke.c:
  a thread allocates a one-slot page and exits; a later thread frees the
  pointer. The free must be accepted through the remote path and must not
  increment the page's free_count locally. This locks out the old TLS-token
  false-local class.
```

Validation:

```text
make -C hakozuna-hz10 smoke-public-entry smoke-retired-ready \
  smoke-shim-api smoke-shim-foreign
make -B -C hakozuna-hz10 preload preload-fine smoke-shim-api \
  smoke-shim-foreign
make -C hakozuna-hz10 smoke-tsan-aslr-off
make -B -C hakozuna-hz10 DEBUG_CFLAGS="... -fsanitize=address,undefined ..." \
  LDFLAGS="-fsanitize=address,undefined" smoke-public-entry smoke-retired-ready
make -C hakozuna-hz10 hz10-standalone-check
git diff --check
```

Next implementation pointer:

```text
HZ10OrphanActiveAdoption-L1:
  add an opt-in pthread-exit handoff that enqueues active pages from exited
  owner records, then let later allocating threads adopt those active pages
  before creating fresh pages. Still no destructor page destruction.
```
