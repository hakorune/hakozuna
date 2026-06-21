# HZ8 Owner Lifecycle

`OwnerLifecycle-0` is the highest-risk HZ8 v0 box.  Owner state alone is not
enough to exclude pending remote publishers.  HZ8 must use an owner control word
that combines:

```text
owner generation
owner state
publish gate closed/open
in-flight publisher count
```

The owner control word is the admission boundary for remote publish.  Reading a
span owner token does not grant publish authority.

## Owner Control Word

Conceptual owner record:

```c
typedef struct H8OwnerRecord {
    _Atomic uint64_t control;
    H8PendingSpanQueue pending_spans;
} H8OwnerRecord;
```

Conceptual packed fields:

```text
generation:
  owner registry slot incarnation

state:
  ALIVE / DYING / DEAD

publish_closed:
  prevents new remote publish leases

publish_refcount:
  remote producers that have entered the publish critical section
```

`ALIVE/open -> DYING/closed` must be one atomic CAS.  This removes the window
where state changes but new publishers can still enter.

## Owner State Machine

External states:

```text
ALIVE / gate OPEN
  |
  | CAS state=DYING, gate=CLOSED
  v
DYING / gate CLOSED / publishers draining
  |
  | publish_refcount == 0
  | pending-span queue fully drained
  | owned spans released or moved to ORPHAN_READY/permanent orphan owner
  v
DEAD / gate CLOSED
```

Internal DYING phases:

```text
DYING_WRITERS:
  publish gate is closed, but old publishers still hold leases

DYING_QUIESCENT:
  publish_refcount == 0, so old-owner remote_head can no longer grow
```

## Owner Generation

Owner generation is bumped only when an owner registry slot is reused for a new
thread incarnation:

```text
DEAD(slot S, generation G)
  -> new owner initialization
  -> ALIVE(slot S, generation G + 1)
```

Do not bump owner generation during span adoption.  Doing so would make every
span already owned by that owner appear stale.  Span adoption uses a separate
`span_ownership_epoch`.

## Span State Machine

```text
OWNED_ACTIVE(old owner token, epoch E)
  |
  | old owner quiescent and pending work drained
  v
ORPHAN_READY(old owner token, epoch E)
  |
  | adopter CAS claim
  v
ADOPTING(new owner token, epoch E + 1)
  |
  | adopter list connection complete
  v
OWNED_ACTIVE(new owner token, epoch E + 1)
```

Use `ORPHAN_READY`, not `ORPHAN_CANDIDATE`, for the publishable adoption state.
`ORPHAN_READY` means old-owner publishers are gone and remote pending state is
stable enough to claim.

`ADOPTING` is not publishable.  Foreign producers that see `ORPHAN_READY` or
`ADOPTING` return `OWNER_TRANSITION` and retry or enter the orphan slow path.

## Publish Lease

Remote producer admission:

```c
static bool h8_owner_publish_enter(
    H8OwnerRecord* owner,
    uint32_t expected_generation)
{
    uint64_t cur = atomic_load_explicit(
        &owner->control, memory_order_acquire);

    for (;;) {
        if (h8_ctl_generation(cur) != expected_generation ||
            h8_ctl_state(cur) != H8_OWNER_ALIVE ||
            h8_ctl_publish_closed(cur)) {
            return false;
        }

        if (h8_ctl_publish_refs(cur) == H8_PUBLISH_REFS_MAX) {
            h8_fail_closed();
        }

        uint64_t next = h8_ctl_refs_increment(cur);
        if (atomic_compare_exchange_weak_explicit(
                &owner->control,
                &cur,
                next,
                memory_order_acquire,
                memory_order_relaxed)) {
            return true;
        }
    }
}
```

Remote producer exit:

```c
static void h8_owner_publish_exit(H8OwnerRecord* owner)
{
    atomic_fetch_sub_explicit(
        &owner->control,
        H8_PUBLISH_REF_ONE,
        memory_order_acq_rel);
}
```

A producer that has a lease may finish publishing to the old owner.  Owner exit
waits for the reference count to reach zero before orphaning or releasing spans.
A producer that only read an owner token but has no lease must retry if the gate
has closed.

The span publish lease is tracked separately from the owner lifecycle lease in
the debug lane so slow-path adoption and remote publish behavior can be
counted independently.

`ORPHAN_QUIESCING` and `ORPHAN_READY` are also counted in the debug lane so
quiescence and adoption readiness can be checked at quiescent slow-path points.

## Regular Adoption Dry Run

Before default-on adoption is enabled, the small allocator slow path performs a
read-only orphan adoption probe:

```text
scan orphan spans for the requested class
record candidate / block reason counters
do not mutate span ownership
then continue to the normal adoption path
```

This lets `RegularAdoptionDryRun-L1` measure how often adoption would have been
possible without changing allocator behavior.

## Remote Publish Protocol

Remote publish must distinguish owner transition from pointer invalidity:

```text
MISS:
  outside HZ8 owned ranges

INVALID:
  HZ8 range hit, but malformed/stale/double-free/interior slot

OWNER_TRANSITION:
  valid HZ8 pointer, but owner handoff is in progress
```

Conceptual remote publish:

```c
H8PublishResult h8_remote_free_publish(void* ptr)
{
retry:
    H8Span* span = h8_span_from_ptr_checked(ptr);
    if (!span) {
        return H8_PUBLISH_MISS;
    }

    H8SpanOwnerWord ow = atomic_load_explicit(
        &span->owner_word,
        memory_order_acquire);

    if (ow.state == H8_SPAN_ORPHAN_READY ||
        ow.state == H8_SPAN_ADOPTING) {
        return H8_PUBLISH_OWNER_TRANSITION;
    }
    if (ow.state != H8_SPAN_OWNED_ACTIVE) {
        return H8_PUBLISH_INVALID;
    }

    H8OwnerRecord* owner = h8_owner_lookup(ow.owner);
    if (!h8_owner_publish_enter(owner, ow.owner.generation)) {
        goto retry;
    }

    H8SpanOwnerWord confirm = atomic_load_explicit(
        &span->owner_word,
        memory_order_acquire);
    if (confirm.raw != ow.raw) {
        h8_owner_publish_exit(owner);
        goto retry;
    }

    size_t slot = h8_slot_index(span, ptr);
    if (!h8_slot_is_aligned_and_in_range(span, ptr, slot)) {
        h8_owner_publish_exit(owner);
        return H8_PUBLISH_INVALID;
    }
    if (!h8_live_bit_load_acquire(span, slot)) {
        h8_owner_publish_exit(owner);
        return H8_PUBLISH_INVALID;
    }
    if (h8_pending_bit_test_and_set_acq_rel(span, slot)) {
        h8_owner_publish_exit(owner);
        return H8_PUBLISH_DOUBLE_FREE;
    }
    if (!h8_live_bit_load_acquire(span, slot)) {
        h8_pending_bit_clear_release(span, slot);
        h8_owner_publish_exit(owner);
        return H8_PUBLISH_INVALID;
    }

    h8_remote_head_push_release(span, ptr);
    h8_span_notify(owner, span);
    h8_owner_publish_exit(owner);
    return H8_PUBLISH_OK;
}
```

Remote producers still do not clear live bits, decrement `used_count`, push
local free-lists, mutate source state, or decommit spans.

## Pending-Span Queue State

Use a three-state queue marker:

```text
IDLE -> QUEUED -> DRAINING -> IDLE
```

Notify:

```c
static void h8_span_notify(H8OwnerRecord* owner, H8Span* span)
{
    uint8_t expected = H8_Q_IDLE;
    if (atomic_compare_exchange_strong_explicit(
            &span->qstate,
            &expected,
            H8_Q_QUEUED,
            memory_order_acq_rel,
            memory_order_acquire)) {
        h8_pending_span_queue_push_release(
            &owner->pending_spans,
            span);
    }
}
```

`QUEUED` or `DRAINING` means the publisher does not enqueue again.

Collect:

```c
static void h8_collect_span(H8OwnerRecord* owner, H8Span* span)
{
    uint8_t expected = H8_Q_QUEUED;
    if (!atomic_compare_exchange_strong_explicit(
            &span->qstate,
            &expected,
            H8_Q_DRAINING,
            memory_order_acq_rel,
            memory_order_acquire)) {
        h8_fail_closed();
    }

    H8RemoteNode* list = (H8RemoteNode*)atomic_exchange_explicit(
        &span->remote_head,
        (uintptr_t)NULL,
        memory_order_acq_rel);

    while (list) {
        H8RemoteNode* node = list;
        list = node->next;

        size_t slot = h8_slot_index(span, node);
        if (!h8_pending_bit_load_acquire(span, slot) ||
            !h8_live_bit_load_acquire(span, slot)) {
            h8_fail_closed();
        }

        h8_live_bit_clear_release(span, slot);
        h8_pending_bit_clear_release(span, slot);
        h8_local_freelist_push_owner_only(span, slot);
        --span->used_count;
    }

    atomic_store_explicit(
        &span->qstate,
        H8_Q_IDLE,
        memory_order_release);

    if (atomic_load_explicit(&span->remote_head, memory_order_acquire) != 0 ||
        h8_pending_summary_load_acquire(span) != 0) {
        h8_span_notify(owner, span);
    }
}
```

This closes the lost-wakeup race:

```text
publisher during DRAINING:
  publisher does not enqueue; collector rechecks and requeues

publisher after IDLE:
  publisher wins IDLE -> QUEUED and enqueues

simultaneous notify:
  exactly one CAS winner enqueues
```

## Owner Exit Protocol

Conceptual exit:

```c
void h8_owner_exit(H8OwnerRecord* owner)
{
    h8_owner_close_publish_gate_and_set_dying(owner);
    h8_owner_wait_publishers_zero(owner);

    while (H8Span* span = h8_pending_span_queue_pop_acquire(
               &owner->pending_spans)) {
        h8_collect_span(owner, span);
    }

    for (H8Span* span : owner->owned_spans) {
        while (atomic_load_explicit(
                   &span->remote_head,
                   memory_order_acquire) != 0) {
            h8_force_collect_span(owner, span);
        }

        if (atomic_load_explicit(&span->qstate, memory_order_acquire) !=
                H8_Q_IDLE ||
            h8_pending_summary_load_acquire(span) != 0) {
            h8_fail_closed();
        }

        h8_remove_from_owner_lists(owner, span);

        if (span->used_count == 0) {
            h8_release_span(span);
            continue;
        }

        h8_span_mark_orphan_ready(span);
        h8_orphan_registry_push(span);
    }

    h8_owner_store_dead_release(owner);
}
```

During owner exit, collection is cold path and may use unbounded drain plus a
full owned-span scan.

## Adoption Preconditions

Whole-span adoption requires:

```text
old owner state == DEAD
old owner publish gate == CLOSED
old owner publish_refcount == 0
span state == ORPHAN_READY
old owner token matches
qstate == IDLE
remote_head == NULL
remote-pending bitmap == 0
pending_count == 0
span is on no owner pending queue
span removed from old owner lists
no local free-list mutation in progress
no allocation refill in progress
no collect in progress
backing memory is valid and not decommitting
popcount(live bitmap) == used_count
free-list + live + never-initialized slots match slot_count
adopter state == ALIVE
adopter publish gate == OPEN
```

Adoption:

```c
bool h8_adopt_span(H8OwnerRecord* adopter, H8Span* span)
{
    if (!h8_owner_is_alive_and_open(adopter)) {
        return false;
    }

    H8SpanOwnerWord old = atomic_load_explicit(
        &span->owner_word,
        memory_order_acquire);
    if (old.state != H8_SPAN_ORPHAN_READY) {
        return false;
    }

    H8SpanOwnerWord adopting = h8_owner_word_adopting(
        adopter->token,
        old.span_epoch + 1);

    if (!atomic_compare_exchange_strong_explicit(
            &span->owner_word,
            &old,
            adopting,
            memory_order_acq_rel,
            memory_order_acquire)) {
        return false;
    }

    h8_verify_orphan_span_quiescent(span);
    h8_attach_to_owner_class_lists(adopter, span);
    h8_reset_owner_local_queue_links(span);

    H8SpanOwnerWord active = h8_owner_word_active(
        adopter->token,
        adopting.span_epoch);
    atomic_store_explicit(
        &span->owner_word,
        active,
        memory_order_release);
    return true;
}
```

New-owner remote publish is allowed only after the final `ADOPTING -> ACTIVE`
release store.

## Adoption Audit Lane

Adoption opportunity accounting lives in the debug lane and runs only on the
slow path.  It records:

```text
candidate scans
candidate-ready hits
blocked-by-state
blocked-by-quiesce
empty-after-drain
target-closed
successful adoption
```

These counters must not change allocator behavior.  They exist to make later
`RegularAdoptionDryRun-L1` decisions measurable before default-on adoption is
enabled.

## v0 Simplification

Evidence-only v0 may use:

```text
H8_V0_STRICT_OWNER_LIFETIME:
  owner thread exit after outstanding frees is not supported
  valid_free_after_owner_dead must remain zero
```

General allocator v0 needs at least a permanent orphan owner:

```text
H8_ORPHAN_OWNER:
  process-lifetime owner
  owns orphaned nonempty spans
  has a global orphan pending-span queue
  collects only on slow paths
  releases spans when they become empty
```

Regular owner re-adoption can wait for v1.  Permanent orphan owner prevents
valid remote frees from becoming invalid merely because the allocating thread
exited.

In v0, the only formal handoff path is:

```text
old owner ALIVE
  -> DYING / publish gate closed
  -> publish refs == 0
  -> pending queue fully drained
  -> span handoff to permanent orphan owner
  -> old owner DEAD
```

That handoff must be a whole-span transition, not a per-object rehome.  The
implementation surface should stay generic (`h8_span_handoff(...)`) so v1 can
add regular-owner adoption later.  The destination owner placement selects the
list (`orphan_head` vs `owned_head`), but v0 must still pin the target to the
permanent orphan owner only.

## Atomic Ordering

| Operation | Ordering |
| --- | --- |
| span owner word read | acquire |
| publisher lease CAS | acquire |
| owner ALIVE/open -> DYING/closed | acq_rel |
| publisher ref decrement | acq_rel |
| owner refs==0 wait | acquire |
| live bit read | acquire |
| pending bit claim | acq_rel |
| remote node head publish | release |
| qstate IDLE -> QUEUED | acq_rel |
| pending-span queue push | release |
| pending-span queue pop | acquire |
| qstate QUEUED -> DRAINING | acq_rel |
| remote_head exchange | acq_rel |
| live bit clear | release |
| pending bit clear | release |
| qstate -> IDLE | release |
| finish head recheck | acquire |
| ACTIVE -> ORPHAN_READY | acq_rel |
| ORPHAN_READY -> ADOPTING | acq_rel |
| ADOPTING -> ACTIVE | release |
| owner -> DEAD | release |

## Zero Gate Defense

```text
dead_owner_publish_lost:
  owner control word combines DEAD/CLOSED and publish refs
  publish requires lease
  owner waits refs==0 before DEAD

orphan_adoption_with_pending_writer:
  ORPHAN_READY only after refs==0
  adopter claims ORPHAN_READY only

remote_publish_lost:
  debug accounting checks claim == collected + outstanding

remote_collect_duplicate:
  pending bit must be 1 at collect
  collect clears pending exactly once

remote_pending_count_mismatch:
  checked only at quiescent points such as owner exit/adoption

span_decommit_while_pending:
  decommit CASes to RETIRING, then revalidates used_count, remote_head,
  qstate, pending bitmap, and live bitmap
```

## Non-Negotiable Parts

Do not simplify:

```text
publish lease / in-flight writer barrier
span owner token revalidation after lease
OWNER_TRANSITION separate from MISS/INVALID
three-state qstate
IDLE store followed by remote_head/pending recheck
publisher zero before ORPHAN_READY
ADOPTING publish ban
ACTIVE publication after owner list connection
pending span decommit ban
owner_generation only for owner slot reuse
```
