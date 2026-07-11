#include "h8_internal.h"
#include "h8_medium.h"
#include "h8_medium_page_shadow.h"

#if defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
#define H8_PS_LEAF_BITS 15u
#define H8_PS_ROOT_BITS 17u
#define H8_PS_LEAF_SIZE (1u << H8_PS_LEAF_BITS)
#define H8_PS_ROOT_SIZE (1u << H8_PS_ROOT_BITS)
#define H8_PS_LEAF_MASK (H8_PS_LEAF_SIZE - 1u)
typedef struct H8MediumPageShadowLeaf {
  _Atomic(H8MediumRun*) entries[H8_PS_LEAF_SIZE];
} H8MediumPageShadowLeaf;
static _Atomic(H8MediumPageShadowLeaf*) h8_ps_root[H8_PS_ROOT_SIZE];
static _Atomic uint64_t h8_ps_lookup, h8_ps_hit, h8_ps_miss;
static _Atomic uint64_t h8_ps_mismatch, h8_ps_valid, h8_ps_invalid;

static uint32_t h8_ps_index(uintptr_t addr) { return (uint32_t)(addr >> 16u); }
static H8MediumPageShadowLeaf* h8_ps_leaf(uint32_t index, bool create) {
  uint32_t root_index = index >> H8_PS_LEAF_BITS;
  H8MediumPageShadowLeaf* leaf =
      atomic_load_explicit(&h8_ps_root[root_index], memory_order_acquire);
  if (!leaf && create) {
    leaf = h8_sys_calloc(1u, sizeof(*leaf));
    if (leaf) atomic_store_explicit(&h8_ps_root[root_index], leaf,
                                    memory_order_release);
  }
  return leaf;
}
void h8_medium_page_shadow_register(H8MediumRun* run) {
  if (!run || !run->base) return;
  uintptr_t end = (uintptr_t)run->base + run->run_size;
  for (uintptr_t addr = (uintptr_t)run->base; addr < end;
       addr += H8_MEDIUM_QUANTUM_BYTES) {
    uint32_t index = h8_ps_index(addr);
    H8MediumPageShadowLeaf* leaf = h8_ps_leaf(index, true);
    if (leaf) atomic_store_explicit(&leaf->entries[index & H8_PS_LEAF_MASK],
                                    run, memory_order_release);
  }
}
void h8_medium_page_shadow_unregister(H8MediumRun* run) {
  if (!run || !run->base) return;
  uintptr_t end = (uintptr_t)run->base + run->run_size;
  for (uintptr_t addr = (uintptr_t)run->base; addr < end;
       addr += H8_MEDIUM_QUANTUM_BYTES) {
    uint32_t index = h8_ps_index(addr);
    H8MediumPageShadowLeaf* leaf = h8_ps_leaf(index, false);
    if (!leaf) continue;
    _Atomic(H8MediumRun*)* entry = &leaf->entries[index & H8_PS_LEAF_MASK];
    if (atomic_load_explicit(entry, memory_order_acquire) == run)
      atomic_store_explicit(entry, NULL, memory_order_release);
  }
}
void h8_medium_page_shadow_compare(const void* ptr, H8MediumRun* authority) {
  atomic_fetch_add_explicit(&h8_ps_lookup, 1u, memory_order_relaxed);
  uint32_t index = h8_ps_index((uintptr_t)ptr);
  H8MediumPageShadowLeaf* leaf = h8_ps_leaf(index, false);
  H8MediumRun* candidate = leaf ? atomic_load_explicit(
                                      &leaf->entries[index & H8_PS_LEAF_MASK],
                                      memory_order_acquire) : NULL;
  atomic_fetch_add_explicit(candidate ? &h8_ps_hit : &h8_ps_miss, 1u,
                            memory_order_relaxed);
  if (candidate != authority)
    atomic_fetch_add_explicit(&h8_ps_mismatch, 1u, memory_order_relaxed);
  if (!candidate) return;
  uintptr_t offset = (uintptr_t)ptr - (uintptr_t)candidate->base;
  size_t payload = (size_t)candidate->slot_size * candidate->slot_count;
  bool exact = offset < payload && candidate->slot_size != 0u &&
               (offset % candidate->slot_size) == 0u;
  atomic_fetch_add_explicit(exact ? &h8_ps_valid : &h8_ps_invalid, 1u,
                            memory_order_relaxed);
}
H8MediumPageShadowStats h8_medium_page_shadow_stats(void) {
  H8MediumPageShadowStats s = {
      atomic_load_explicit(&h8_ps_lookup, memory_order_relaxed),
      atomic_load_explicit(&h8_ps_hit, memory_order_relaxed),
      atomic_load_explicit(&h8_ps_miss, memory_order_relaxed),
      atomic_load_explicit(&h8_ps_mismatch, memory_order_relaxed),
      atomic_load_explicit(&h8_ps_valid, memory_order_relaxed),
      atomic_load_explicit(&h8_ps_invalid, memory_order_relaxed)};
  return s;
}
#else
void h8_medium_page_shadow_register(H8MediumRun* run) { (void)run; }
void h8_medium_page_shadow_unregister(H8MediumRun* run) { (void)run; }
void h8_medium_page_shadow_compare(const void* ptr, H8MediumRun* run) {
  (void)ptr; (void)run;
}
H8MediumPageShadowStats h8_medium_page_shadow_stats(void) {
  H8MediumPageShadowStats s = {0}; return s;
}
#endif
