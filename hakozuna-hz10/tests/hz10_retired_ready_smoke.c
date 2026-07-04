#include "../src/hz10_freelist_page.h"
#include "../src/hz10_retired_ready.h"

#include <pthread.h>
#include <stdio.h>

/*
 * HZ10RetiredReadyQueue-L0 prototype smoke: exercises the module directly
 * (mark/note/pop), without going through Box 3's remote-free pipeline or
 * Box 6's class-list bookkeeping -- see src/hz10_retired_ready.h for why
 * this module is a candidate-only hint layer, not an authoritative
 * reclaim mechanism, and current_task.md's "polling-vs-event-driven"
 * entry for the design this responds to.
 */

/* Case 1: outstanding reaches zero only on the Nth note, not before, and
 * pop() returns the page exactly once. */
static int check_pop_fires_on_last_note(void) {
  Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
  if (!page) {
    fprintf(stderr, "pop_fires: create failed\n");
    return 1;
  }
  Hz10RetiredReadyStack stack = {0};
  hz10_retired_ready_mark(page, &stack, 3u);

  hz10_retired_ready_note_remote_free(page);
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "pop_fires: popped after 1/3 notes, expected NULL\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }
  hz10_retired_ready_note_remote_free(page);
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "pop_fires: popped after 2/3 notes, expected NULL\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }
  hz10_retired_ready_note_remote_free(page);
  Hz10FreelistPage* popped = hz10_retired_ready_pop(&stack);
  if (popped != page) {
    fprintf(stderr, "pop_fires: popped=%p, expected page=%p after 3/3\n",
            (void*)popped, (void*)page);
    hz10_freelist_page_destroy(page);
    return 1;
  }
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "pop_fires: second pop returned non-NULL, expected "
                  "empty stack\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }

  hz10_freelist_page_destroy(page);
  return 0;
}

/* Case 2: a page that was never marked is inert -- note_remote_free() is
 * a safe no-op, pop() never sees it. */
static int check_unmarked_page_is_inert(void) {
  Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
  if (!page) {
    fprintf(stderr, "unmarked: create failed\n");
    return 1;
  }
  Hz10RetiredReadyStack stack = {0};

  hz10_retired_ready_note_remote_free(page);
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "unmarked: pop returned non-NULL for an unmarked "
                  "page's stack\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }

  hz10_freelist_page_destroy(page);
  return 0;
}

/* Case 3: two pages on the same stack resolve independently -- notes for
 * one never affect the other's countdown, and both are eventually
 * popped, LIFO (most-recently-ready first), no duplicates. */
static int check_two_pages_independent_and_lifo(void) {
  Hz10FreelistPage* a = hz10_freelist_page_create(64u, 16u);
  Hz10FreelistPage* b = hz10_freelist_page_create(64u, 16u);
  if (!a || !b) {
    fprintf(stderr, "two_pages: create failed\n");
    return 1;
  }
  Hz10RetiredReadyStack stack = {0};
  hz10_retired_ready_mark(a, &stack, 2u);
  hz10_retired_ready_mark(b, &stack, 1u);

  hz10_retired_ready_note_remote_free(a); /* a: 1 left */
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "two_pages: popped before either page resolved\n");
    hz10_freelist_page_destroy(a);
    hz10_freelist_page_destroy(b);
    return 1;
  }

  hz10_retired_ready_note_remote_free(b); /* b: resolves first */
  Hz10FreelistPage* first = hz10_retired_ready_pop(&stack);
  if (first != b) {
    fprintf(stderr, "two_pages: first pop=%p, expected b=%p\n",
            (void*)first, (void*)b);
    hz10_freelist_page_destroy(a);
    hz10_freelist_page_destroy(b);
    return 1;
  }

  hz10_retired_ready_note_remote_free(a); /* a: resolves now */
  Hz10FreelistPage* second = hz10_retired_ready_pop(&stack);
  if (second != a) {
    fprintf(stderr, "two_pages: second pop=%p, expected a=%p\n",
            (void*)second, (void*)a);
    hz10_freelist_page_destroy(a);
    hz10_freelist_page_destroy(b);
    return 1;
  }
  if (hz10_retired_ready_pop(&stack) != NULL) {
    fprintf(stderr, "two_pages: stack not empty after both pages popped\n");
    hz10_freelist_page_destroy(a);
    hz10_freelist_page_destroy(b);
    return 1;
  }

  hz10_freelist_page_destroy(a);
  hz10_freelist_page_destroy(b);
  return 0;
}

/* Case 4: concurrent-note stress -- HZ10_STRESS_PAGES pages, each marked
 * with HZ10_STRESS_NOTES_PER_PAGE outstanding, HZ10_STRESS_THREADS
 * threads each calling note_remote_free() on every page exactly
 * HZ10_STRESS_NOTES_PER_PAGE / HZ10_STRESS_THREADS times (evenly
 * divided) -- simulates many foreign threads racing to free the last
 * few slots of many retired pages concurrently. After joining, the
 * owner must be able to pop every single page exactly once, no
 * duplicates, no losses. */
#define HZ10_STRESS_PAGES 64u
#define HZ10_STRESS_THREADS 4u
#define HZ10_STRESS_NOTES_PER_PAGE HZ10_STRESS_THREADS

typedef struct StressArg {
  Hz10FreelistPage** pages;
  uint32_t page_count;
} StressArg;

static void* stress_worker(void* raw) {
  StressArg* arg = (StressArg*)raw;
  for (uint32_t p = 0; p < arg->page_count; ++p) {
    hz10_retired_ready_note_remote_free(arg->pages[p]);
  }
  return NULL;
}

static int check_concurrent_notes_no_dup_no_loss(void) {
  Hz10FreelistPage* pages[HZ10_STRESS_PAGES];
  for (uint32_t i = 0; i < HZ10_STRESS_PAGES; ++i) {
    pages[i] = hz10_freelist_page_create(64u, 16u);
    if (!pages[i]) {
      fprintf(stderr, "stress: create %u failed\n", i);
      return 1;
    }
  }
  Hz10RetiredReadyStack stack = {0};
  for (uint32_t i = 0; i < HZ10_STRESS_PAGES; ++i) {
    hz10_retired_ready_mark(pages[i], &stack, HZ10_STRESS_NOTES_PER_PAGE);
  }

  pthread_t threads[HZ10_STRESS_THREADS];
  StressArg arg = {pages, HZ10_STRESS_PAGES};
  for (uint32_t t = 0; t < HZ10_STRESS_THREADS; ++t) {
    if (pthread_create(&threads[t], NULL, stress_worker, &arg) != 0) {
      fprintf(stderr, "stress: pthread_create failed\n");
      return 1;
    }
  }
  for (uint32_t t = 0; t < HZ10_STRESS_THREADS; ++t) {
    pthread_join(threads[t], NULL);
  }

  uint32_t popped_count = 0;
  int seen[HZ10_STRESS_PAGES] = {0};
  Hz10FreelistPage* popped;
  while ((popped = hz10_retired_ready_pop(&stack)) != NULL) {
    int found = 0;
    for (uint32_t i = 0; i < HZ10_STRESS_PAGES; ++i) {
      if (pages[i] == popped) {
        if (seen[i]) {
          fprintf(stderr, "stress: page %u popped more than once\n", i);
          return 1;
        }
        seen[i] = 1;
        found = 1;
        break;
      }
    }
    if (!found) {
      fprintf(stderr, "stress: popped a pointer not in pages[]\n");
      return 1;
    }
    ++popped_count;
  }

  int failed = 0;
  if (popped_count != HZ10_STRESS_PAGES) {
    fprintf(stderr, "stress: popped_count=%u, expected %u\n", popped_count,
            HZ10_STRESS_PAGES);
    failed = 1;
  }

  for (uint32_t i = 0; i < HZ10_STRESS_PAGES; ++i) {
    hz10_freelist_page_destroy(pages[i]);
  }
  return failed;
}

int main(void) {
  if (check_pop_fires_on_last_note()) {
    return 1;
  }
  if (check_unmarked_page_is_inert()) {
    return 2;
  }
  if (check_two_pages_independent_and_lifo()) {
    return 3;
  }
  if (check_concurrent_notes_no_dup_no_loss()) {
    return 4;
  }
  puts("hz10_retired_ready_smoke ok");
  return 0;
}
