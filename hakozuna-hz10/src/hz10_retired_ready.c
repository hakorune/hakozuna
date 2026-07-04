#include "hz10_retired_ready.h"

void hz10_retired_ready_mark(Hz10FreelistPage* page,
                             Hz10RetiredReadyStack* stack,
                             uint32_t outstanding) {
  /* Plain writes, owner-only: a foreign thread never reads these until it
   * has observed retired_ready_flag == 1 via an acquire load, and the
   * release store below guarantees both are visible by then.
   * retired_ready_generation is different: it is never read by a
   * foreign thread at all (only by the owner, after a later pop() --
   * see the header's bug (2)), so it does not even need the release
   * ordering the other two rely on, but setting it here alongside them
   * is the natural place. */
  page->retired_ready_stack = stack;
  page->retired_ready_generation = page->generation;
  atomic_store_explicit(&page->retired_ready_outstanding, outstanding,
                        memory_order_relaxed);
  atomic_store_explicit(&page->retired_ready_flag, 1, memory_order_release);
}

void hz10_retired_ready_note_remote_free(Hz10FreelistPage* page) {
  if (!atomic_load_explicit(&page->retired_ready_flag, memory_order_acquire)) {
    return;
  }

  uint32_t prev = atomic_fetch_sub_explicit(&page->retired_ready_outstanding,
                                           1u, memory_order_acq_rel);
  if (prev != 1u) {
    /* Not yet zero (or this mark() cycle's count was already exhausted by
     * a racing/stray call -- see the module comment on why that is a
     * missed-candidate outcome, not a correctness issue). Either way,
     * nothing more to do here. */
    return;
  }

  /* This call's decrement is the one that reached zero -- fetch_sub is a
   * single atomic RMW, so exactly one caller ever observes prev == 1 for
   * a given hz10_retired_ready_mark() cycle. That does NOT yet mean this
   * call is entitled to push: the owner's budgeted cursor walk
   * (hz10_class_pages.c) may concurrently be trying to claim this same
   * flag transition for itself via hz10_retired_ready_cancel() (module
   * comment's bug (3)), having independently decided via its own
   * free_count/local_free_head check to take this page out of `retired`
   * some other way. A CAS, not a plain store, makes the two mutually
   * exclusive -- exactly one of them wins, and the loser must not act. */
  uint32_t expected = 1u;
  if (!atomic_compare_exchange_strong_explicit(
          &page->retired_ready_flag, &expected, 0u, memory_order_acq_rel,
          memory_order_relaxed)) {
    /* Lost: the owner's walk already claimed this page. Do not push --
     * it is being reclaimed/promoted some other way right now. */
    return;
  }
  /* Must be visible before the push below -- see the struct comment on
   * why the budgeted cursor walk needs this to avoid destroying a page
   * that is still linked into the ready stack. */
  atomic_store_explicit(&page->retired_ready_on_stack, 1,
                        memory_order_release);

  Hz10RetiredReadyStack* stack =
      (Hz10RetiredReadyStack*)page->retired_ready_stack;
  Hz10FreelistPage* old_head =
      atomic_load_explicit(&stack->head, memory_order_relaxed);
  do {
    page->retired_ready_next = old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &stack->head, &old_head, page, memory_order_release,
      memory_order_relaxed));
  atomic_fetch_add_explicit(&stack->push_count, 1u, memory_order_relaxed);
}

Hz10FreelistPage* hz10_retired_ready_pop(Hz10RetiredReadyStack* stack) {
  Hz10FreelistPage* head =
      atomic_load_explicit(&stack->head, memory_order_acquire);
  while (head &&
        !atomic_compare_exchange_weak_explicit(
            &stack->head, &head, head->retired_ready_next,
            memory_order_acquire, memory_order_acquire)) {
  }
  if (head) {
    /* Popped node is now exclusively owned by the caller -- safe to
     * clear now that it is unreachable via `stack`. */
    atomic_store_explicit(&head->retired_ready_on_stack, 0,
                          memory_order_relaxed);
  }
  return head;
}

int hz10_retired_ready_cancel(Hz10FreelistPage* page) {
  /* Same CAS, same transition, as hz10_retired_ready_note_remote_free()'s
   * own attempt -- see the module comment's bug (3) for why this must be
   * a CAS race rather than a check-then-act race. */
  uint32_t expected = 1u;
  return atomic_compare_exchange_strong_explicit(
      &page->retired_ready_flag, &expected, 0u, memory_order_acq_rel,
      memory_order_relaxed);
}
