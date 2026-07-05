# HZ10 Speed Attack Plan L0

Design memo for the post-RSS throughput push. This is intentionally a review
document, not an implementation patch. The goal is to give other reviewers a
small set of concrete designs to attack before HZ10 starts changing hot paths
again.

## Current Position

RSS is considered locked enough to return to speed work:

```text
lifecycle flush:
  explicit quiescent boundary exists
  current_rss_kb is reported
  hz10-rss-guard is the cheap regression gate

steady state:
  main_r50/r90 do not reproduce monotonic current-RSS growth
  retired pages do not accumulate monotonically in the 8/20/40s checks
```

Latest useful throughput read, THREADS=4 ITERS=500000 RUNS=10, real tcmalloc:

```text
main_local0:    ~0.56x tcmalloc
main_r50:       ~0.47x tcmalloc
main_r90:       ~0.49x tcmalloc
medium_local0:  ~0.55-0.61x tcmalloc
small_remote50: ~0.69x tcmalloc
small_remote90: ~0.70x tcmalloc
slot_count1_r90:~0.54-0.63x tcmalloc depending on run set
```

HZ10 already clears the HZ8 comparison on the latest same-run read. The next
fight is tcmalloc-class throughput while keeping HZ8-class RSS discipline.

## Load-Bearing Constraints

Do not weaken these without a separate correctness design:

```text
accepted remote free == at least drainable before hz10_free() returns
pending bit remains the remote duplicate-free authority
claim() happens before retired_ready_note_remote_free()
publish() happens after retired_ready_note_remote_free()
owner local_free_head stays owner-thread-only plain load/store
page destruction requires idle verification plus retired-ready guards
```

Known NO-GO results that should not be re-opened without new evidence:

```text
remote publish batching for ordinary hz10_free(ptr)
producer TLS staging without producer-side quiescent flush
active-list move-to-front
second-active/two-page ping-pong cache
pending-bit memory-order tweak on this x86_64 box
```

## The Real Question

The next speed box should answer this before implementing behavior:

```text
Is the tcmalloc gap mostly:
  A. public hz10_free() route/validation overhead,
  B. remote publish cost (pending bit + Treiber CAS + ready note),
  C. alloc-side page selection/drain behavior,
  D. page creation/pagemap/pool cold-start cost, or
  E. benchmark harness/inbox shape masking the allocator cost?
```

The existing evidence already weakens C for the main rows: active scan cost is
real but move-to-front and two-slot caches were NO-GO. Existing RMW microbench
points strongly at B for remote rows, but public-entry rows did not visibly
move from the debug-counter cleanup, so we need a stage-level public-shaped
measurement before changing contracts.

## Recommended Attack Order

### 1. HZ10SpeedBaselineRefresh-L0

Re-run the locked comparison after RSS changes, using the split throughput
fields:

```text
rows:
  main_local0 main_r50 main_r90
  medium_local0
  small_remote50 small_remote90
  slot_count1_r90

report:
  hz10 work_loop_ops_per_s
  hz10 work_loop_plus_flush_ops_per_s when flush is enabled
  tcmalloc ops_per_s
  current_rss_kb and post_rss_kb
```

Decision: if numbers differ materially from the locked table, update the
target list before implementing any speed box.

### 2. HZ10PublicFreeStageCost-L0

Add an opt-in measurement lane that times or counts public free stages without
changing default behavior:

```text
route:
  hz10_pagemap_route(ptr)

local:
  owner_thread_token == HZ10_THREAD_TOKEN
  hz10_freelist_page_free(page, ptr)

remote claim:
  hz10_page_remote_free_claim(page, ptr, generation, &slot_index)

remote ready note:
  hz10_retired_ready_note_remote_free(page)

remote publish:
  hz10_page_remote_free_publish(page, ptr)
```

Preferred shape: a dedicated microbench that pre-allocates stable pages and
calls these pieces in public-entry-like ratios. Avoid timing every production
operation with always-on clock reads. Use compile-time opt-in counters or a
separate bench binary.

Required output:

```text
ns/op for route-only
ns/op for local free after route
ns/op for remote claim only
ns/op for ready note in active-page no-op case
ns/op for publish only
ns/op for full remote free
```

Decision: pick the first implementation box from the largest measured stage,
not from intuition.

### 3. HZ10OwnerTokenFastState-Design-L0

Current `owner_thread_token` is only a same-thread identity token:

```text
&hz10_thread_token_storage
```

That is enough to choose local vs remote, but it cannot route a remote free to
owner state without another lookup. A future owner-visible handoff design would
need the page to carry a stable pointer to an owner state object:

```text
Hz10OwnerState:
  thread identity token
  per-class page lists
  remote inbox / handoff queues
  lifecycle state
```

This is a larger boundary change. It may enable owner-mailbox designs, but it
must also solve owner death/quiescent handoff. Do not fold this into a small
remote-stack patch.

Open design questions for reviewers:

```text
Can owner state be stable without a pthread destructor contract?
Can a foreign thread safely enqueue to it after the owner reaches quiescent?
What is the fail-closed behavior if owner state is shutting down?
Does owner-state lookup cost less than current page-stripe publish?
```

### 4. HZ10RemotePublicationV2-Design-L0

Only after stage-cost data, consider replacing per-page Treiber publish with a
different publication target. Candidate shapes:

```text
page-striped Treiber v1 (current):
  pending fetch_or + page stripe CAS
  owner drains only pages it scans
  no owner-state lifetime problem

owner mailbox:
  pending fetch_or + owner queue publish
  owner can drain many remote frees at known check-in points
  needs stable owner-state lifetime and shutdown protocol

producer epoch handoff:
  producer stages frees and flushes at explicit producer quiescent points
  can batch, but requires a new producer-side contract
```

Current recommendation:

```text
keep page-striped Treiber as default
measure owner-mailbox cost in isolation before implementing it in HZ10
do not implement producer staging as default behavior
```

The key review question: can an owner-mailbox reduce total remote free cost
without just replacing one CAS with another CAS plus a harder lifetime problem?

### 5. HZ10LocalPathTrim-L0

Do not ignore local rows. `main_local0` and `medium_local0` are still around
0.55-0.61x tcmalloc. Possible causes:

```text
random-size class selection
public free route lookup
first-page/cold pagemap setup inside short windows
active page refill frequency for large classes
bench touch pattern
```

Recommended measurement:

```text
fixed-size local0 rows by class
alloc-only from warm active page
free-only local route+free
random-size local0 with pre-warmed pages
```

Decision: if local free route dominates, optimize public route/free. If alloc
refill dominates, optimize class/page selection. If fixed-size warm rows are
already close to tcmalloc, focus on mixed-size routing and cold-start instead.

## Initial GO/NO-GO Gates

A speed implementation box should be GO only if it satisfies all of this:

```text
correctness:
  existing smokes green
  hz10-standalone-check green
  no weakening of pending/ready/lifecycle contracts unless explicitly designed

performance:
  public-entry same-run A/B, RUNS=10 when variance is non-trivial
  compare to real tcmalloc where the claim mentions tcmalloc
  report median, not best run
  include RSS guard or current_rss_kb so speed does not steal RSS

rollback:
  opt-in build flag, env flag, or separate bench lane for first measurement
  default path unchanged until A/B is favorable
```

Suggested first implementation after this design review:

```text
HZ10PublicFreeStageCost-L0
```

It is measurement-only, low-risk, and directly selects whether the next real
box should attack route/local free, remote claim/publish, owner handoff, or
page selection. This is the best next move before changing hot-path contracts.

