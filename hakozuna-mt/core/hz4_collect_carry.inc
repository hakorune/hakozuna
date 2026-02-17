// hz4_collect_carry.inc - CarryBox (Carry Slot Management)
// Box Theory: carry slot の消費・管理を担当
//
// Carry slots are per-(tid, sc) remainder buffers to reduce requeue/pushback.
// This box provides consume operations for both array and list modes.

#ifndef HZ4_COLLECT_CARRY_INC
#define HZ4_COLLECT_CARRY_INC

#include "hz4_tls.h"
#include "hz4_page.h"

// ============================================================================
// Carry Consume: carry slots から objects を配列に回収
// ============================================================================
static inline uint32_t hz4_carry_consume(hz4_tls_t* tls, uint8_t sc,
                                         void** out, uint32_t budget) {
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("carry_consume: sc out of range");
    }
#endif
    if (sc >= HZ4_SC_MAX) {
        return 0;
    }

    hz4_carry_t* c = &tls->carry[sc];

#if HZ4_FAILFAST
    if (c->n > HZ4_CARRY_SLOTS) {
        HZ4_FAIL("carry_consume: n overflow");
    }
#endif

    if (c->n == 0) {
        return 0;
    }

    uint32_t got = 0;

    // Consume from slots (LIFO: start from top slot)
    while (c->n > 0 && got < budget) {
        uint32_t idx = c->n - 1;
        hz4_carry_slot_t* slot = &c->slot[idx];

        if (!slot->head) {
            // Empty slot, pop it
            slot->page = NULL;
            c->n--;
            continue;
        }

        // Drain from this slot
        void* cur = slot->head;
        while (cur && got < budget) {
            void* next = hz4_obj_get_next(cur);
            out[got++] = cur;
            cur = next;
        }

        if (cur) {
            // Still has items, update head
            slot->head = cur;
        } else {
            // Slot exhausted, pop it
            slot->head = NULL;
            slot->page = NULL;
            c->n--;
        }
    }

    return got;
}

// ============================================================================
// Carry Consume List: carry から list を直接取得
// ============================================================================
static inline uint32_t hz4_carry_consume_list(hz4_tls_t* tls, uint8_t sc,
                                              void** head_out, void** tail_out,
                                              uint32_t budget) {
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("carry_consume_list: sc out of range");
    }
#endif
    if (sc >= HZ4_SC_MAX) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    hz4_carry_t* c = &tls->carry[sc];
    if (c->n == 0) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

    while (c->n > 0 && got < budget) {
        uint32_t idx = c->n - 1;
        hz4_carry_slot_t* slot = &c->slot[idx];

        if (!slot->head) {
            slot->page = NULL;
            c->n--;
            continue;
        }

        void* cur = slot->head;
        while (cur && got < budget) {
            void* next = hz4_obj_get_next(cur);
            // Prepend to result list (順序逆転するが freelist なので OK)
            hz4_obj_set_next(cur, head);
            if (!tail) tail = cur;
            head = cur;
            cur = next;
            got++;
        }

        if (cur) {
            slot->head = cur;
        } else {
            slot->head = NULL;
            slot->page = NULL;
            c->n--;
        }
    }

    *head_out = head;
    *tail_out = tail;
    return got;
}

#endif // HZ4_COLLECT_CARRY_INC
