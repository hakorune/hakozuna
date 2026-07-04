# HZ10 Remote Publish Batch Contract L0

Design note for the next throughput investigation after lifecycle flush. This
is a contract/design box, not an implementation box.

## Motivation

The current remote-free publication cost is dominated by freeing-thread atomic
publication:

```text
pending bit fetch_or
Treiber publish CAS
```

The pending bit is the duplicate-free authority and is not the first lever to
weaken. The remaining plausible lever is reducing the number of Treiber publish
CAS operations by publishing several remote frees to the same page as one list.

## Existing Load-Bearing Contracts

Current `hz10_page_remote_free()` means:

```text
return 1 == the free was accepted and is at least drainable by the owner
return 0 == the free was rejected and no publication is owed
```

Current claim/publish split means:

```text
claim:
  validates pointer
  claims the pending bit
  does not expose the slot to owner drain

publish:
  links the slot onto remote_free_head
  after publish returns, the freeing thread must not touch page
```

The split was added to close a real race with `hz10_retired_ready_note_remote_free()`.
Any batching design must preserve the same safety property: a claimed but
unpublished slot is unreachable from owner drain, and the page must not be
destroyed while such a claim exists.

## Forbidden Shapes

Do not implement these as HZ10 behavior:

```text
producer TLS staging with no mandatory flush point:
  breaks accepted == drainable if a thread frees once and then goes idle

accepted return before publish with only best-effort later flush:
  loses remote frees under thread death, cancellation, or stopped producers

automatic pthread destructor flush:
  not enough; ordinary thread exit can race with foreign threads still
  holding pointers into this thread's pages, and HZ10 deliberately avoided
  destructor-based lifecycle semantics

owner reclaim that treats claimed-but-unpublished slots as free:
  breaks the claim/publish invariant and can destroy a page while a producer
  still owns slot bookkeeping

single atomic_exchange batch push without a safe tail link:
  publishing the new head before tail->next is connected lets the owner drain
  a temporarily truncated stack
```

## Candidate Shapes

### A. Same-call batch publish helper

Add an internal helper that publishes an already-built linked list of remote
free slots for one page with one Treiber CAS:

```text
hz10_page_remote_free_publish_batch(page, first, last, count)
```

The helper would:

```text
require every slot in the batch to have already won claim()
set last->next to the observed old head before CAS
CAS page->remote_free_head[stripe] from old head to first
return only after publication succeeds
```

This preserves `accepted == drainable` because the caller does not return
success to its caller until the batch has been published.

Problem: ordinary `hz10_free(ptr)` has only one pointer. This helper only
matters if a caller already has a batch, such as a bench inbox drain, a future
bulk free API, or an internal owner/handoff path. It does not directly improve
the normal public `free(ptr)` path.

### B. Producer TLS staging with explicit producer flush API

Stage remote frees in the producer thread, publish when:

```text
batch reaches N slots for the same page/stripe
caller explicitly invokes producer_quiescent_flush()
```

This is the only shape that could improve ordinary repeated `free(ptr)` calls,
but it changes the contract unless HZ10 gets a separate producer-side
quiescent API and all callers are required to use it before a thread can go
idle or exit.

Status: not implementable as default behavior yet. HZ10 has an owner-thread
cache flush contract, not a producer-thread remote-publish flush contract.

### C. Owner-visible handoff queue

Route remote frees to an owner-visible queue or mailbox rather than per-page
Treiber stacks, then let the owner publish/merge in larger chunks.

This avoids a producer TLS flush gap, but it is a larger routing redesign:

```text
needs owner identity / mailbox lookup
still needs duplicate-free authority
must preserve fail-closed routing for stale/interior/misaligned pointers
must handle owner death or quiescent handoff
```

Status: design-only. This is likely a different box, not a small optimization
of `hz10_remote_stack.c`.

## Recommended Next Box

Open `HZ10RemotePublishBatchLocality-L0` before implementing behavior.

Measurement goal:

```text
For public-entry remote rows, when draining worker inboxes or synthetic traces,
how often do consecutive remote frees target the same page and same stripe?
What is the upper bound if an ideal batch helper could publish those groups
with one CAS each?
```

Counters to gather:

```text
remote_free_count
same_page_run_count
same_page_run_len_sum
same_page_run_len_max
same_page_run_hist_1_2_4_8_16plus
ideal_publish_cas_count
cas_reduction_ceiling = 1 - ideal_publish_cas_count / remote_free_count
```

Decision rule:

```text
If same-page run lengths are mostly 1, do not implement batching.
If the ceiling is meaningful, implement only the same-call batch helper first,
behind an opt-in bench lane, and keep public hz10_free(ptr) behavior unchanged.
Do not implement producer TLS staging until a producer-side quiescent flush
contract exists.
```

## Current Design Read

GO:

```text
same-call publish_batch helper as an opt-in/internal primitive
measurement-only locality box first
```

NO-GO for now:

```text
default producer TLS staging
returning success before publish
weakening the pending bit
automatic destructor-based flush
```

## HZ10RemotePublishBatchLocality-L0 Result

Log:

```text
bench_results/20260704T235343Z_hz10_remote_publish_batch_locality_l0/combined.log
```

THREADS=4 ITERS=500000 RUNS=10 median read:

```text
row              remote frees   ideal CAS    CAS ceiling   avg run   run len 1
main_r50             999739       950560        4.92%       1.052      94.95%
main_r90            1800534      1707038        5.19%       1.055      94.66%
small_remote50       999739       870090       12.97%       1.149      87.68%
small_remote90      1800534      1523272       15.40%       1.182      85.43%
slot_count1_r90     1800534      1800534        0.00%       1.000     100.00%
```

Decision:

```text
normal public free path:
  NO-GO for remote publish batching now

same-call publish_batch helper:
  keep as possible future primitive for bulk/inbox paths only

producer TLS staging:
  still NO-GO without a producer-side quiescent flush contract
```

Read: the important main rows have too little same-page locality for a
same-call batch helper to be worth implementing as the next throughput box.
The small rows show a larger ceiling, but those rows are not the current main
tcmalloc gap driver. The slot_count=1 row has exactly no batchability because
each page has one slot.
