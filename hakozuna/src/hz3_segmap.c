#define _GNU_SOURCE

#include "hz3_segmap.h"
#include "hz3_oom.h"

#include <sys/mman.h>
#include <stddef.h>

// Global segmap (zero-initialized by static storage)
Hz3SegMapL1 g_hz3_segmap[HZ3_SEGMAP_L1_SIZE];

// Allocate L2 table via mmap
static _Atomic(Hz3SegMeta*)* hz3_segmap_alloc_l2(void) {
    size_t size = sizeof(_Atomic(Hz3SegMeta*)) * HZ3_SEGMAP_L2_SIZE;
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        hz3_oom_note("segmap_l2", size, 0);
        return NULL;
    }
    return (_Atomic(Hz3SegMeta*)*)mem;
}

void hz3_segmap_set(void* base, Hz3SegMeta* meta) {
    uintptr_t addr = (uintptr_t)base;
    uint32_t l1_idx = (addr >> (HZ3_SEG_SHIFT + HZ3_SEGMAP_L2_BITS)) & (HZ3_SEGMAP_L1_SIZE - 1);
    uint32_t l2_idx = (addr >> HZ3_SEG_SHIFT) & (HZ3_SEGMAP_L2_SIZE - 1);

    Hz3SegMapL1* l1 = &g_hz3_segmap[l1_idx];

    // Ensure L2 table exists (CAS to avoid double allocation)
    _Atomic(Hz3SegMeta*)* l2 = atomic_load_explicit(&l1->l2, memory_order_acquire);
    if (!l2) {
        _Atomic(Hz3SegMeta*)* new_l2 = hz3_segmap_alloc_l2();
        if (!new_l2) return;  // allocation failed

        // CAS: only one thread wins
        _Atomic(Hz3SegMeta*)* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &l1->l2, &expected, new_l2,
                memory_order_acq_rel, memory_order_acquire)) {
            l2 = new_l2;  // we won
        } else {
            // Another thread won, use their L2 and free ours
            munmap(new_l2, sizeof(_Atomic(Hz3SegMeta*)) * HZ3_SEGMAP_L2_SIZE);
            l2 = expected;  // expected now contains the winning pointer
        }
    }

    // Store meta with release semantics
    atomic_store_explicit(&l2[l2_idx], meta, memory_order_release);
}

void hz3_segmap_clear(void* base) {
    uintptr_t addr = (uintptr_t)base;
    uint32_t l1_idx = (addr >> (HZ3_SEG_SHIFT + HZ3_SEGMAP_L2_BITS)) & (HZ3_SEGMAP_L1_SIZE - 1);
    uint32_t l2_idx = (addr >> HZ3_SEG_SHIFT) & (HZ3_SEGMAP_L2_SIZE - 1);

    Hz3SegMapL1* l1 = &g_hz3_segmap[l1_idx];
    _Atomic(Hz3SegMeta*)* l2 = atomic_load_explicit(&l1->l2, memory_order_acquire);
    if (!l2) return;

    // Clear entry with release semantics
    atomic_store_explicit(&l2[l2_idx], NULL, memory_order_release);
}

Hz3SegMeta* hz3_segmap_take(void* base) {
    uintptr_t addr = (uintptr_t)base;
    uint32_t l1_idx = (addr >> (HZ3_SEG_SHIFT + HZ3_SEGMAP_L2_BITS)) & (HZ3_SEGMAP_L1_SIZE - 1);
    uint32_t l2_idx = (addr >> HZ3_SEG_SHIFT) & (HZ3_SEGMAP_L2_SIZE - 1);

    Hz3SegMapL1* l1 = &g_hz3_segmap[l1_idx];
    _Atomic(Hz3SegMeta*)* l2 = atomic_load_explicit(&l1->l2, memory_order_acquire);
    if (!l2) return NULL;

    // Clear entry and return old value (single-writer teardown boundary).
    return atomic_exchange_explicit(&l2[l2_idx], NULL, memory_order_acq_rel);
}
