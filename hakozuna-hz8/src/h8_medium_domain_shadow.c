#include "h8_medium_domain_shadow.h"

#include "h8_internal.h"

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)

#define H8_MD_LEAF_BITS 15u
#define H8_MD_ROOT_BITS 17u
#define H8_MD_LEAF_COUNT (1u << H8_MD_LEAF_BITS)
#define H8_MD_ROOT_COUNT (1u << H8_MD_ROOT_BITS)
#define H8_MD_LEAF_MASK (H8_MD_LEAF_COUNT - 1u)
#define H8_MD_TAG_MASK ((uintptr_t)3u)

typedef struct H8MediumDomainLeaf {
  _Atomic uintptr_t slots[H8_MD_LEAF_COUNT];
} H8MediumDomainLeaf;

static _Atomic(H8MediumDomainLeaf*) h8_md_root[H8_MD_ROOT_COUNT];
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
static _Atomic uint64_t h8_md_lookup;
static _Atomic uint64_t h8_md_medium_hit;
static _Atomic uint64_t h8_md_page8k_hit;
static _Atomic uint64_t h8_md_miss;
static _Atomic uint64_t h8_md_kind_match;
static _Atomic uint64_t h8_md_kind_mismatch;
static _Atomic uint64_t h8_md_register_conflict;
static _Atomic uint64_t h8_md_medium_register;
static _Atomic uint64_t h8_md_medium_unregister;
static _Atomic uint64_t h8_md_page8k_register;
#define H8_MD_COUNT(name) \
  atomic_fetch_add_explicit(&(name), 1u, memory_order_relaxed)
#else
#define H8_MD_COUNT(name) ((void)0)
#endif

static uint32_t h8_md_key(uintptr_t addr) {
  return (uint32_t)(addr / H8_MEDIUM_QUANTUM_BYTES);
}

static H8MediumDomainLeaf* h8_md_leaf(uint32_t key, bool create) {
  size_t root_index = (size_t)(key >> H8_MD_LEAF_BITS);
  H8MediumDomainLeaf* leaf =
      atomic_load_explicit(&h8_md_root[root_index], memory_order_acquire);
  if (leaf || !create) {
    return leaf;
  }
  H8MediumDomainLeaf* fresh = h8_sys_calloc(1u, sizeof(*fresh));
  if (!fresh) {
    return NULL;
  }
  H8MediumDomainLeaf* expected = NULL;
  if (!atomic_compare_exchange_strong_explicit(
          &h8_md_root[root_index], &expected, fresh, memory_order_release,
          memory_order_acquire)) {
    h8_sys_free(fresh);
    return expected;
  }
  return fresh;
}

static uintptr_t h8_md_tag(const void* record, H8MediumDomainKind kind) {
  uintptr_t value = (uintptr_t)record;
  if (!record || (value & H8_MD_TAG_MASK) != 0u) {
    return 0u;
  }
  return value | (uintptr_t)kind;
}

static void h8_md_register_quantum(uintptr_t addr, const void* record,
                                   H8MediumDomainKind kind) {
  uint32_t key = h8_md_key(addr);
  H8MediumDomainLeaf* leaf = h8_md_leaf(key, true);
  uintptr_t tagged = h8_md_tag(record, kind);
  if (!leaf || tagged == 0u) {
    H8_MD_COUNT(h8_md_register_conflict);
    return;
  }
  _Atomic uintptr_t* slot = &leaf->slots[key & H8_MD_LEAF_MASK];
  uintptr_t expected = 0u;
  if (!atomic_compare_exchange_strong_explicit(
          slot, &expected, tagged, memory_order_release, memory_order_acquire) &&
      expected != tagged) {
    H8_MD_COUNT(h8_md_register_conflict);
  }
}

void h8_medium_domain_shadow_register_medium(H8MediumRun* run) {
  if (!run || !run->base) {
    return;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  for (uintptr_t quantum = base; quantum < end;
       quantum += H8_MEDIUM_QUANTUM_BYTES) {
    h8_md_register_quantum(quantum, run, H8_MEDIUM_DOMAIN_RUN);
  }
  H8_MD_COUNT(h8_md_medium_register);
}

void h8_medium_domain_shadow_unregister_medium(H8MediumRun* run) {
  if (!run || !run->base) {
    return;
  }
  uintptr_t tagged = h8_md_tag(run, H8_MEDIUM_DOMAIN_RUN);
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  for (uintptr_t quantum = base; quantum < end;
       quantum += H8_MEDIUM_QUANTUM_BYTES) {
    uint32_t key = h8_md_key(quantum);
    H8MediumDomainLeaf* leaf = h8_md_leaf(key, false);
    if (!leaf) {
      continue;
    }
    _Atomic uintptr_t* slot = &leaf->slots[key & H8_MD_LEAF_MASK];
    uintptr_t expected = tagged;
    if (!atomic_compare_exchange_strong_explicit(
            slot, &expected, 0u, memory_order_release, memory_order_acquire) &&
        expected != 0u) {
      H8_MD_COUNT(h8_md_register_conflict);
    }
  }
  H8_MD_COUNT(h8_md_medium_unregister);
}

void h8_medium_domain_shadow_register_page8k(void* base, const void* record) {
  h8_md_register_quantum((uintptr_t)base, record, H8_MEDIUM_DOMAIN_PAGE8K);
  H8_MD_COUNT(h8_md_page8k_register);
}

H8MediumDomainProbe h8_medium_domain_shadow_lookup(const void* ptr) {
  H8MediumDomainProbe probe = {H8_MEDIUM_DOMAIN_NONE, NULL};
  H8_MD_COUNT(h8_md_lookup);
  if (!ptr) {
    H8_MD_COUNT(h8_md_miss);
    return probe;
  }
  uint32_t key = h8_md_key((uintptr_t)ptr);
  H8MediumDomainLeaf* leaf = h8_md_leaf(key, false);
  uintptr_t tagged =
      leaf ? atomic_load_explicit(&leaf->slots[key & H8_MD_LEAF_MASK],
                                  memory_order_acquire)
           : 0u;
  probe.kind = (H8MediumDomainKind)(tagged & H8_MD_TAG_MASK);
  probe.record = (const void*)(tagged & ~H8_MD_TAG_MASK);
  if (probe.kind == H8_MEDIUM_DOMAIN_RUN) {
    H8_MD_COUNT(h8_md_medium_hit);
  } else if (probe.kind == H8_MEDIUM_DOMAIN_PAGE8K) {
    H8_MD_COUNT(h8_md_page8k_hit);
  } else {
    probe.kind = H8_MEDIUM_DOMAIN_NONE;
    probe.record = NULL;
    H8_MD_COUNT(h8_md_miss);
  }
  return probe;
}

void h8_medium_domain_shadow_compare(H8MediumDomainProbe probe,
                                     H8MediumDomainKind expected) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
  _Atomic uint64_t* counter = probe.kind == expected ? &h8_md_kind_match
                                                      : &h8_md_kind_mismatch;
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
#else
  (void)probe;
  (void)expected;
#endif
}

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
#define H8_MD_LOAD(name) atomic_load_explicit(&(name), memory_order_relaxed)
#endif
H8MediumDomainShadowStats h8_medium_domain_shadow_stats(void) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
  H8MediumDomainShadowStats stats = {
      H8_MD_LOAD(h8_md_lookup),
      H8_MD_LOAD(h8_md_medium_hit),
      H8_MD_LOAD(h8_md_page8k_hit),
      H8_MD_LOAD(h8_md_miss),
      H8_MD_LOAD(h8_md_kind_match),
      H8_MD_LOAD(h8_md_kind_mismatch),
      H8_MD_LOAD(h8_md_register_conflict),
      H8_MD_LOAD(h8_md_medium_register),
      H8_MD_LOAD(h8_md_medium_unregister),
      H8_MD_LOAD(h8_md_page8k_register)};
  return stats;
#else
  H8MediumDomainShadowStats stats = {0};
  return stats;
#endif
}
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
#undef H8_MD_LOAD
#endif
#undef H8_MD_COUNT

#else

void h8_medium_domain_shadow_register_medium(H8MediumRun* run) { (void)run; }
void h8_medium_domain_shadow_unregister_medium(H8MediumRun* run) { (void)run; }
void h8_medium_domain_shadow_register_page8k(void* base, const void* record) {
  (void)base;
  (void)record;
}
H8MediumDomainProbe h8_medium_domain_shadow_lookup(const void* ptr) {
  H8MediumDomainProbe probe = {H8_MEDIUM_DOMAIN_NONE, NULL};
  (void)ptr;
  return probe;
}
void h8_medium_domain_shadow_compare(H8MediumDomainProbe probe,
                                     H8MediumDomainKind expected) {
  (void)probe;
  (void)expected;
}
H8MediumDomainShadowStats h8_medium_domain_shadow_stats(void) {
  H8MediumDomainShadowStats stats = {0};
  return stats;
}

#endif
