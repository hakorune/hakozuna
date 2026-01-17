// hz3_list_invariants.c - Linked list invariants fail-fast (debug)
// Purpose: Detect corrupted lists before they propagate to S42/S44/central.

#include "hz3_list_invariants.h"

#if HZ3_LIST_FAILFAST

#include "hz3_arena.h"
#include "hz3_tcache.h"  // for hz3_obj_get_next
#include <stdio.h>
#include <stdlib.h>

void hz3_list_failfast(const char* where, uint8_t owner, int sc,
                       void* head, void* tail, uint32_t n) {
    // Empty list check
    if (n == 0) {
        if (head != NULL || tail != NULL) {
            fprintf(stderr,
                    "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                    "n=0 but head=%p tail=%p (should be NULL)\n",
                    where, owner, sc, head, tail);
            abort();
        }
        return;  // Empty list is valid
    }

    // Non-empty list must have head and tail
    if (head == NULL) {
        fprintf(stderr,
                "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                "n=%u but head=NULL\n",
                where, owner, sc, n);
        abort();
    }
    if (tail == NULL) {
        fprintf(stderr,
                "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                "n=%u but tail=NULL\n",
                where, owner, sc, n);
        abort();
    }

    // Walk the list, checking invariants
    void* cur = head;
    uint32_t count = 0;
    uint32_t max_walk = (n < HZ3_LIST_FAILFAST_MAX_WALK) ? n : HZ3_LIST_FAILFAST_MAX_WALK;

    while (cur != NULL && count < max_walk) {
        // Check: node must be in arena
        uint32_t page_idx;
        if (!hz3_arena_page_index_fast(cur, &page_idx)) {
            fprintf(stderr,
                    "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                    "node=%p (idx=%u) out of arena head=%p tail=%p n=%u\n",
                    where, owner, sc, cur, count, head, tail, n);
            abort();
        }

        // Get next pointer
        void* next = hz3_obj_get_next(cur);

        // Check: self-loop detection
        if (next == cur) {
            fprintf(stderr,
                    "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                    "self-loop at node=%p (idx=%u) head=%p tail=%p n=%u\n",
                    where, owner, sc, cur, count, head, tail, n);
            abort();
        }

        // Check: if next is non-NULL, it must be in arena
        if (next != NULL) {
            uint32_t next_page_idx;
            if (!hz3_arena_page_index_fast(next, &next_page_idx)) {
                fprintf(stderr,
                        "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                        "node=%p->next=%p out of arena (idx=%u) head=%p tail=%p n=%u\n",
                        where, owner, sc, cur, next, count, head, tail, n);
                abort();
            }
        }

        count++;

        // Check if we reached tail
        if (cur == tail) {
            // Verify count matches n
            if (count != n) {
                fprintf(stderr,
                        "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                        "reached tail at count=%u but n=%u head=%p tail=%p\n",
                        where, owner, sc, count, n, head, tail);
                abort();
            }
            // Verify tail->next is NULL (SSOT: slot-based storage assumes this)
            if (next != NULL) {
                fprintf(stderr,
                        "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                        "tail=%p->next=%p should be NULL head=%p n=%u\n",
                        where, owner, sc, tail, next, head, n);
                abort();
            }
            return;  // All checks passed
        }

        cur = next;
    }

    // If we got here without reaching tail, something is wrong
    if (count >= max_walk) {
        fprintf(stderr,
                "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
                "walk limit reached (count=%u, max=%u) without finding tail=%p head=%p n=%u\n",
                where, owner, sc, count, max_walk, tail, head, n);
        abort();
    }

    // cur became NULL before reaching tail
    fprintf(stderr,
            "[HZ3_LIST_FAILFAST] where=%s owner=%u sc=%d "
            "list ended at count=%u before reaching tail=%p head=%p n=%u\n",
            where, owner, sc, count, tail, head, n);
    abort();
}

#endif // HZ3_LIST_FAILFAST
