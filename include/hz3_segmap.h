#pragma once

#include "hz3_types.h"

// SegMap: 2-level radix tree for ptr -> SegMeta* lookup
// 48-bit addr: [upper 11 bits (L1)] [16 bits (L2)] [21 bits (seg offset)]
// L1: 2^11 = 2048 entries (covers 4TB virtual address space)
// L2: 2^16 = 65536 entries per L1 slot

#define HZ3_SEGMAP_L1_BITS 11
#define HZ3_SEGMAP_L2_BITS 16
#define HZ3_SEGMAP_L1_SIZE (1u << HZ3_SEGMAP_L1_BITS)
#define HZ3_SEGMAP_L2_SIZE (1u << HZ3_SEGMAP_L2_BITS)

// L1 entry: holds atomic pointer to L2 table
typedef struct {
    _Atomic(Hz3SegMeta*) * _Atomic l2;  // L2 table (lazily allocated)
} Hz3SegMapL1;

// Global segmap (static zero-init)
extern Hz3SegMapL1 g_hz3_segmap[HZ3_SEGMAP_L1_SIZE];

// O(1) lock-free lookup (acquire semantics)
// Returns NULL if segment not registered
static inline Hz3SegMeta* hz3_segmap_get(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    uint32_t l1_idx = (addr >> (HZ3_SEG_SHIFT + HZ3_SEGMAP_L2_BITS)) & (HZ3_SEGMAP_L1_SIZE - 1);
    uint32_t l2_idx = (addr >> HZ3_SEG_SHIFT) & (HZ3_SEGMAP_L2_SIZE - 1);

    Hz3SegMapL1* l1 = &g_hz3_segmap[l1_idx];
    _Atomic(Hz3SegMeta*)* l2 = atomic_load_explicit(&l1->l2, memory_order_acquire);
    if (!l2) return NULL;
    return atomic_load_explicit(&l2[l2_idx], memory_order_acquire);
}

// Fast reject: returns 0 if L2 table is absent for this address range.
// Safe to use as a hint before hz3_segmap_get() in hot paths.
static inline int hz3_segmap_has_l2(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    uint32_t l1_idx = (addr >> (HZ3_SEG_SHIFT + HZ3_SEGMAP_L2_BITS)) & (HZ3_SEGMAP_L1_SIZE - 1);
    Hz3SegMapL1* l1 = &g_hz3_segmap[l1_idx];
    _Atomic(Hz3SegMeta*)* l2 = atomic_load_explicit(&l1->l2, memory_order_acquire);
    return l2 != NULL;
}

// Register segment (CAS for L2 lazy alloc)
// base must be 2MB aligned
void hz3_segmap_set(void* base, Hz3SegMeta* meta);

// Unregister segment (for segment free/reuse)
void hz3_segmap_clear(void* base);

// Unregister segment and return previous meta (linearizable).
// Returns NULL if already unregistered.
Hz3SegMeta* hz3_segmap_take(void* base);
