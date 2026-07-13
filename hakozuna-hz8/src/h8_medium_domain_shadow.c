#include "h8_medium_domain_shadow.h"

#include "h8_internal.h"

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_PAGE8K_RECORD_L1) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_MEDIUM_RECORD_L1) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_OWNER_WITNESS_L1)

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_MEDIUM_RECORD_L1) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_OWNER_WITNESS_L1)
#define H8_MD_STABLE_STORAGE 1
#endif

#define H8_MD_LEAF_BITS 15u
#define H8_MD_ROOT_BITS 17u
#define H8_MD_LEAF_COUNT (1u << H8_MD_LEAF_BITS)
#define H8_MD_ROOT_COUNT (1u << H8_MD_ROOT_BITS)
#define H8_MD_LEAF_MASK (H8_MD_LEAF_COUNT - 1u)
#define H8_MD_TAG_MASK ((uintptr_t)3u)
#define H8_MD_STABLE_RECORD_CAP 65536u
#define H8_MD_STABLE_SLOT_CAP 16u

_Static_assert(_Alignof(H8MediumRun) >= 4u,
               "H8MediumRun must support two pointer-tag bits");

typedef enum H8MediumDomainRecordState {
  H8_MD_RECORD_EMPTY = 0,
  H8_MD_RECORD_LIVE = 1,
  H8_MD_RECORD_CLOSING = 2,
  H8_MD_RECORD_TOMBSTONE = 3
} H8MediumDomainRecordState;

typedef struct H8MediumDomainStableRecord {
  _Atomic uint8_t state;
  uint8_t reserved[7];
  uint64_t generation;
  uintptr_t base;
  uint32_t run_size;
  uint32_t slot_size;
  uint16_t slot_count;
  uint16_t slot_shift;
  uint16_t class_id;
  uint16_t reserved2;
  _Atomic(H8MediumRun*) implementation;
  _Atomic uint32_t slot_state[H8_MD_STABLE_SLOT_CAP];
  _Atomic uint64_t pending_bits;
  _Atomic uint64_t pending_word_mask;
  _Atomic uint8_t qstate;
  _Atomic uint64_t owner_word;
  h8_platform_mutex_t lock;
} H8MediumDomainStableRecord;

_Static_assert(_Alignof(H8MediumDomainStableRecord) >= 4u,
               "stable record must support two pointer-tag bits");

typedef struct H8MediumDomainLeaf {
  _Atomic uintptr_t slots[H8_MD_LEAF_COUNT];
} H8MediumDomainLeaf;

static _Atomic(H8MediumDomainLeaf*) h8_md_root[H8_MD_ROOT_COUNT];
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_OWNER_WITNESS_DIAG)
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
static _Atomic uint64_t h8_md_high_address_fallback;
static _Atomic uint64_t h8_md_stable_record_alloc;
static _Atomic uint64_t h8_md_stable_record_pool_fallback;
static _Atomic uint64_t h8_md_stable_live_lookup;
static _Atomic uint64_t h8_md_stable_closing_lookup;
static _Atomic uint64_t h8_md_stable_tombstone_lookup;
static _Atomic uint64_t h8_md_stable_backend_mismatch;
static _Atomic uint64_t h8_md_stable_geometry_mismatch;
static _Atomic uint64_t h8_md_stable_unregister_missing;
static _Atomic uint64_t h8_md_stable_turnover_pass;
static _Atomic uint64_t h8_md_stable_turnover_fail;
static _Atomic uint64_t h8_md_stable_record_reuse_detected;
static _Atomic uint64_t h8_md_stable_exact_valid;
static _Atomic uint64_t h8_md_stable_exact_invalid;
static _Atomic uint64_t h8_md_stable_exact_tombstone;
static _Atomic uint64_t h8_md_stable_exact_mismatch;
static _Atomic uint64_t h8_md_stable_slot_init;
static _Atomic uint64_t h8_md_stable_slot_sync;
static _Atomic uint64_t h8_md_stable_slot_mismatch;
static _Atomic uint64_t h8_md_stable_slot_bound_fallback;
static _Atomic uint64_t h8_md_stable_slot_final_sync;
static _Atomic uint64_t h8_md_stable_slot_sync_after_closing;
static _Atomic uint64_t h8_md_stable_slot_mirror_alloc_authority_free;
static _Atomic uint64_t h8_md_stable_slot_mirror_free_authority_alloc;
static _Atomic uint64_t h8_md_stable_slot_other_mismatch;
static _Atomic uint64_t h8_md_stable_slot_note_without_record;
static _Atomic uint64_t h8_md_stable_pending_init;
static _Atomic uint64_t h8_md_stable_pending_sync;
static _Atomic uint64_t h8_md_stable_pending_final_sync;
static _Atomic uint64_t h8_md_stable_pending_mismatch;
static _Atomic uint64_t h8_md_stable_pending_sync_after_closing;
static _Atomic uint64_t h8_md_stable_pending_note_without_record;
static _Atomic uint64_t h8_md_stable_lock_acquire;
static _Atomic uint64_t h8_md_stable_lock_fallback;
static _Atomic uint64_t h8_md_stable_unlock_mismatch;
static _Atomic uint64_t h8_md_stable_unregister_lock;
static _Atomic uint64_t h8_md_stable_owner_init;
static _Atomic uint64_t h8_md_stable_owner_sync;
static _Atomic uint64_t h8_md_stable_owner_final_sync;
static _Atomic uint64_t h8_md_stable_owner_mismatch;
static _Atomic uint64_t h8_md_stable_owner_sync_after_closing;
static _Atomic uint64_t h8_md_stable_owner_note_without_record;
static _Atomic uint64_t h8_md_stable_owner_current_match;
static _Atomic uint64_t h8_md_stable_owner_current_miss;
static _Atomic uint64_t h8_md_owner_witness_attempt;
static _Atomic uint64_t h8_md_owner_witness_valid;
static _Atomic uint64_t h8_md_owner_witness_invalid;
static _Atomic uint64_t h8_md_owner_witness_fallback;
#define H8_MD_COUNT(name) \
  atomic_fetch_add_explicit(&(name), 1u, memory_order_relaxed)
#else
#define H8_MD_COUNT(name) ((void)0)
#endif

static bool h8_md_split_key(uintptr_t addr, size_t* root_out,
                            size_t* leaf_out) {
  uintptr_t quantum = addr >> 16u;
  if ((quantum >> (H8_MD_ROOT_BITS + H8_MD_LEAF_BITS)) != 0u) {
    H8_MD_COUNT(h8_md_high_address_fallback);
    return false;
  }
  *root_out = (size_t)(quantum >> H8_MD_LEAF_BITS);
  *leaf_out = (size_t)(quantum & H8_MD_LEAF_MASK);
  return true;
}

static H8MediumDomainLeaf* h8_md_leaf(size_t root_index, bool create) {
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
  size_t root_index = 0;
  size_t leaf_index = 0;
  if (!h8_md_split_key(addr, &root_index, &leaf_index)) {
    return;
  }
  H8MediumDomainLeaf* leaf = h8_md_leaf(root_index, true);
  uintptr_t tagged = h8_md_tag(record, kind);
  if (!leaf || tagged == 0u) {
    H8_MD_COUNT(h8_md_register_conflict);
    return;
  }
  _Atomic uintptr_t* slot = &leaf->slots[leaf_index];
  uintptr_t expected = 0u;
  if (!atomic_compare_exchange_strong_explicit(
          slot, &expected, tagged, memory_order_release, memory_order_acquire) &&
      expected != tagged) {
    H8_MD_COUNT(h8_md_register_conflict);
  }
}

#if defined(H8_MD_STABLE_STORAGE)
static H8MediumDomainStableRecord h8_md_stable_records[H8_MD_STABLE_RECORD_CAP];
static _Atomic size_t h8_md_stable_record_count;

static H8MediumDomainStableRecord* h8_md_stable_record_create(
    H8MediumRun* run) {
  if (run->slot_count > H8_MD_STABLE_SLOT_CAP || !run->slot_state) {
    H8_MD_COUNT(h8_md_stable_slot_bound_fallback);
    return NULL;
  }
  size_t index = atomic_fetch_add_explicit(&h8_md_stable_record_count, 1u,
                                           memory_order_relaxed);
  if (index >= H8_MD_STABLE_RECORD_CAP) {
    H8_MD_COUNT(h8_md_stable_record_pool_fallback);
    return NULL;
  }
  H8MediumDomainStableRecord* record = &h8_md_stable_records[index];
  record->generation = index + 1u;
  record->base = (uintptr_t)run->base;
  record->run_size = run->run_size;
  record->slot_size = run->slot_size;
  record->slot_count = run->slot_count;
  record->slot_shift = run->slot_shift;
  record->class_id = run->class_id;
  h8_platform_mutex_init(&record->lock);
  for (size_t slot = 0u; slot < run->slot_count; ++slot) {
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    atomic_store_explicit(&record->slot_state[slot], state,
                          memory_order_relaxed);
    H8_MD_COUNT(h8_md_stable_slot_init);
  }
  atomic_store_explicit(
      &record->pending_bits,
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire),
      memory_order_relaxed);
  atomic_store_explicit(
      &record->pending_word_mask,
      atomic_load_explicit(&run->pending_word_mask, memory_order_acquire),
      memory_order_relaxed);
  atomic_store_explicit(
      &record->qstate,
      atomic_load_explicit(&run->qstate, memory_order_acquire),
      memory_order_relaxed);
  atomic_store_explicit(
      &record->owner_word,
      atomic_load_explicit(&run->owner_word, memory_order_acquire),
      memory_order_relaxed);
  H8_MD_COUNT(h8_md_stable_owner_init);
  H8_MD_COUNT(h8_md_stable_pending_init);
  atomic_store_explicit(&record->implementation, run, memory_order_relaxed);
  atomic_store_explicit(&record->state, H8_MD_RECORD_LIVE,
                        memory_order_release);
  run->stable_domain_record = record;
  H8_MD_COUNT(h8_md_stable_record_alloc);
  return record;
}

static H8MediumDomainStableRecord* h8_md_stable_record_find(H8MediumRun* run) {
  if (run && run->stable_domain_record) {
    H8MediumDomainStableRecord* record = run->stable_domain_record;
    if (atomic_load_explicit(&record->implementation, memory_order_acquire) ==
        run) {
      return record;
    }
  }
  size_t count = atomic_load_explicit(&h8_md_stable_record_count,
                                      memory_order_acquire);
  if (count > H8_MD_STABLE_RECORD_CAP) {
    count = H8_MD_STABLE_RECORD_CAP;
  }
  for (size_t i = count; i > 0u; --i) {
    H8MediumDomainStableRecord* record = &h8_md_stable_records[i - 1u];
    if (atomic_load_explicit(&record->implementation, memory_order_acquire) ==
        run) {
      return record;
    }
  }
  return NULL;
}

void h8_medium_domain_stable_slot_note(H8MediumRun* run, size_t slot,
                                       uint32_t committed_state) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record || slot >= record->slot_count ||
      slot >= H8_MD_STABLE_SLOT_CAP) {
    H8_MD_COUNT(h8_md_stable_slot_note_without_record);
    return;
  }
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
      H8_MD_RECORD_LIVE) {
    H8_MD_COUNT(h8_md_stable_slot_sync_after_closing);
    return;
  }
  atomic_store_explicit(&record->slot_state[slot], committed_state,
                        memory_order_release);
  H8_MD_COUNT(h8_md_stable_slot_sync);
}

void h8_medium_domain_stable_pending_note(H8MediumRun* run) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record) {
    H8_MD_COUNT(h8_md_stable_pending_note_without_record);
    return;
  }
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
      H8_MD_RECORD_LIVE) {
    H8_MD_COUNT(h8_md_stable_pending_sync_after_closing);
    return;
  }
  atomic_store_explicit(
      &record->pending_bits,
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire),
      memory_order_release);
  atomic_store_explicit(
      &record->pending_word_mask,
      atomic_load_explicit(&run->pending_word_mask, memory_order_acquire),
      memory_order_release);
  atomic_store_explicit(
      &record->qstate,
      atomic_load_explicit(&run->qstate, memory_order_acquire),
      memory_order_release);
  H8_MD_COUNT(h8_md_stable_pending_sync);
}

void h8_medium_domain_stable_owner_note(H8MediumRun* run,
                                        uint64_t committed_owner_word) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record) {
    H8_MD_COUNT(h8_md_stable_owner_note_without_record);
    return;
  }
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
      H8_MD_RECORD_LIVE) {
    H8_MD_COUNT(h8_md_stable_owner_sync_after_closing);
    return;
  }
  atomic_store_explicit(&record->owner_word, committed_owner_word,
                        memory_order_release);
  H8_MD_COUNT(h8_md_stable_owner_sync);
}

void h8_medium_domain_stable_owner_compare(H8MediumDomainProbe probe,
                                           uint64_t authority_owner_word,
                                           uint64_t current_owner_word) {
  if (probe.kind != H8_MEDIUM_DOMAIN_RUN || !probe.record) return;
  const H8MediumDomainStableRecord* record = probe.record;
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
      H8_MD_RECORD_LIVE)
    return;
  uint64_t mirrored =
      atomic_load_explicit(&record->owner_word, memory_order_acquire);
  if (mirrored != authority_owner_word) {
    H8_MD_COUNT(h8_md_stable_owner_mismatch);
  } else if (current_owner_word != 0u && mirrored == current_owner_word) {
    H8_MD_COUNT(h8_md_stable_owner_current_match);
  } else {
    H8_MD_COUNT(h8_md_stable_owner_current_miss);
  }
}

bool h8_medium_domain_stable_lock(H8MediumRun* run) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record) {
    H8_MD_COUNT(h8_md_stable_lock_fallback);
    return false;
  }
  h8_platform_mutex_lock(&record->lock);
  H8_MD_COUNT(h8_md_stable_lock_acquire);
  return true;
}

bool h8_medium_domain_stable_unlock(H8MediumRun* run) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record) {
    H8_MD_COUNT(h8_md_stable_unlock_mismatch);
    return false;
  }
  h8_platform_mutex_unlock(&record->lock);
  return true;
}

bool h8_medium_domain_stable_trylock(H8MediumRun* run, int* result_out) {
  H8MediumDomainStableRecord* record = h8_md_stable_record_find(run);
  if (!record) {
    return false;
  }
  int result = h8_platform_mutex_trylock(&record->lock);
  if (result == 0) {
    H8_MD_COUNT(h8_md_stable_lock_acquire);
  }
  if (result_out) {
    *result_out = result;
  }
  return true;
}

static void h8_md_stable_pending_do_final_sync(
    H8MediumDomainStableRecord* record, H8MediumRun* run) {
  uint64_t bits =
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire);
  uint64_t word_mask =
      atomic_load_explicit(&run->pending_word_mask, memory_order_acquire);
  uint8_t qstate = atomic_load_explicit(&run->qstate, memory_order_acquire);
  if (atomic_load_explicit(&record->pending_bits, memory_order_acquire) != bits ||
      atomic_load_explicit(&record->pending_word_mask, memory_order_acquire) !=
          word_mask ||
      atomic_load_explicit(&record->qstate, memory_order_acquire) != qstate) {
    H8_MD_COUNT(h8_md_stable_pending_mismatch);
  }
  atomic_store_explicit(&record->pending_bits, bits, memory_order_release);
  atomic_store_explicit(&record->pending_word_mask, word_mask,
                        memory_order_release);
  atomic_store_explicit(&record->qstate, qstate, memory_order_release);
  H8_MD_COUNT(h8_md_stable_pending_final_sync);
}

static void h8_md_stable_owner_do_final_sync(
    H8MediumDomainStableRecord* record, H8MediumRun* run) {
  uint64_t owner_word =
      atomic_load_explicit(&run->owner_word, memory_order_acquire);
  if (atomic_load_explicit(&record->owner_word, memory_order_acquire) !=
      owner_word) {
    H8_MD_COUNT(h8_md_stable_owner_mismatch);
  }
  atomic_store_explicit(&record->owner_word, owner_word,
                        memory_order_release);
  H8_MD_COUNT(h8_md_stable_owner_final_sync);
}

static void h8_md_stable_slot_do_final_sync(
    H8MediumDomainStableRecord* record, H8MediumRun* run) {
  for (size_t slot = 0u; slot < record->slot_count; ++slot) {
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    uint32_t mirror =
        atomic_load_explicit(&record->slot_state[slot], memory_order_acquire);
    if (mirror != state) {
      H8_MD_COUNT(h8_md_stable_slot_mismatch);
      uint32_t mirror_tag = h8_slot_state_tag(mirror);
      uint32_t authority_tag = h8_slot_state_tag(state);
      if (mirror_tag == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
          authority_tag == (H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT)) {
        H8_MD_COUNT(h8_md_stable_slot_mirror_alloc_authority_free);
      } else if (mirror_tag == (H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT) &&
                 authority_tag ==
                     (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
        H8_MD_COUNT(h8_md_stable_slot_mirror_free_authority_alloc);
      } else {
        H8_MD_COUNT(h8_md_stable_slot_other_mismatch);
      }
    }
    atomic_store_explicit(&record->slot_state[slot], state,
                          memory_order_release);
    H8_MD_COUNT(h8_md_stable_slot_final_sync);
  }
}
#endif

void h8_medium_domain_shadow_register_medium(H8MediumRun* run) {
  if (!run || !run->base) {
    return;
  }
#if defined(H8_MD_STABLE_STORAGE)
  H8MediumDomainStableRecord* stable = h8_md_stable_record_create(run);
  if (!stable) {
    return;
  }
  const void* published = stable;
#else
  const void* published = run;
#endif
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  for (uintptr_t quantum = base; quantum < end;
       quantum += H8_MEDIUM_QUANTUM_BYTES) {
    h8_md_register_quantum(quantum, published, H8_MEDIUM_DOMAIN_RUN);
  }
  H8_MD_COUNT(h8_md_medium_register);
}

void h8_medium_domain_shadow_unregister_medium(H8MediumRun* run) {
  if (!run || !run->base) {
    return;
  }
#if defined(H8_MD_STABLE_STORAGE)
  H8MediumDomainStableRecord* stable = h8_md_stable_record_find(run);
  if (!stable) {
    H8_MD_COUNT(h8_md_stable_unregister_missing);
    return;
  }
  h8_platform_mutex_lock(&stable->lock);
  H8_MD_COUNT(h8_md_stable_unregister_lock);
  atomic_store_explicit(&stable->state, H8_MD_RECORD_CLOSING,
                        memory_order_release);
  h8_md_stable_slot_do_final_sync(stable, run);
  h8_md_stable_pending_do_final_sync(stable, run);
  h8_md_stable_owner_do_final_sync(stable, run);
  const void* published = stable;
#else
  const void* published = run;
#endif
  uintptr_t tagged = h8_md_tag(published, H8_MEDIUM_DOMAIN_RUN);
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  for (uintptr_t quantum = base; quantum < end;
       quantum += H8_MEDIUM_QUANTUM_BYTES) {
    size_t root_index = 0;
    size_t leaf_index = 0;
    if (!h8_md_split_key(quantum, &root_index, &leaf_index)) {
      continue;
    }
    H8MediumDomainLeaf* leaf = h8_md_leaf(root_index, false);
    if (!leaf) {
      continue;
    }
    _Atomic uintptr_t* slot = &leaf->slots[leaf_index];
    uintptr_t expected = tagged;
    if (!atomic_compare_exchange_strong_explicit(
            slot, &expected, 0u, memory_order_release, memory_order_acquire) &&
        expected != 0u) {
      H8_MD_COUNT(h8_md_register_conflict);
    }
  }
#if defined(H8_MD_STABLE_STORAGE)
  atomic_store_explicit(&stable->implementation, NULL, memory_order_release);
  atomic_store_explicit(&stable->state, H8_MD_RECORD_TOMBSTONE,
                        memory_order_release);
  h8_platform_mutex_unlock(&stable->lock);
#endif
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
  size_t root_index = 0;
  size_t leaf_index = 0;
  if (!h8_md_split_key((uintptr_t)ptr, &root_index, &leaf_index)) {
    H8_MD_COUNT(h8_md_miss);
    return probe;
  }
  H8MediumDomainLeaf* leaf = h8_md_leaf(root_index, false);
  uintptr_t tagged =
      leaf ? atomic_load_explicit(&leaf->slots[leaf_index], memory_order_acquire)
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

void h8_medium_domain_stable_compare(H8MediumDomainProbe probe,
                                     H8MediumRun* authority) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  if (probe.kind != H8_MEDIUM_DOMAIN_RUN || !probe.record) {
    if (authority) {
      H8_MD_COUNT(h8_md_stable_backend_mismatch);
    }
    return;
  }
  const H8MediumDomainStableRecord* record = probe.record;
  uint8_t state = atomic_load_explicit(&record->state, memory_order_acquire);
  if (state == H8_MD_RECORD_LIVE) {
    H8MediumRun* implementation = atomic_load_explicit(
        &record->implementation, memory_order_acquire);
    uint8_t confirmed =
        atomic_load_explicit(&record->state, memory_order_acquire);
    if (confirmed != H8_MD_RECORD_LIVE) {
      if (confirmed == H8_MD_RECORD_CLOSING) {
        H8_MD_COUNT(h8_md_stable_closing_lookup);
      } else if (confirmed == H8_MD_RECORD_TOMBSTONE) {
        H8_MD_COUNT(h8_md_stable_tombstone_lookup);
      } else {
        H8_MD_COUNT(h8_md_stable_backend_mismatch);
      }
      return;
    }
    H8_MD_COUNT(h8_md_stable_live_lookup);
    if (implementation != authority) {
      H8_MD_COUNT(h8_md_stable_backend_mismatch);
      return;
    }
    if (!authority || record->base != (uintptr_t)authority->base ||
        record->run_size != authority->run_size ||
        record->slot_size != authority->slot_size ||
        record->slot_count != authority->slot_count ||
        record->slot_shift != authority->slot_shift ||
        record->class_id != authority->class_id) {
      H8_MD_COUNT(h8_md_stable_geometry_mismatch);
    }
  } else if (state == H8_MD_RECORD_CLOSING) {
    H8_MD_COUNT(h8_md_stable_closing_lookup);
  } else if (state == H8_MD_RECORD_TOMBSTONE) {
    H8_MD_COUNT(h8_md_stable_tombstone_lookup);
  } else {
    H8_MD_COUNT(h8_md_stable_backend_mismatch);
  }
#else
  (void)probe;
  (void)authority;
#endif
}

bool h8_medium_domain_stable_exact_compare(H8MediumDomainProbe probe,
                                           const void* ptr,
                                           bool authority_exact) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  bool stable_exact = false;
  if (probe.kind == H8_MEDIUM_DOMAIN_RUN && probe.record && ptr) {
    const H8MediumDomainStableRecord* record = probe.record;
    uint8_t state = atomic_load_explicit(&record->state, memory_order_acquire);
    if (state == H8_MD_RECORD_LIVE) {
      uintptr_t addr = (uintptr_t)ptr;
      if (addr >= record->base && record->slot_size != 0u &&
          record->slot_count != 0u) {
        uintptr_t offset = addr - record->base;
        size_t payload =
            (size_t)record->slot_size * (size_t)record->slot_count;
        if (offset < payload) {
          if ((record->slot_size & (record->slot_size - 1u)) == 0u) {
            uintptr_t mask = ((uintptr_t)1u << record->slot_shift) - 1u;
            stable_exact = (offset & mask) == 0u;
          } else {
            stable_exact = (offset % (uintptr_t)record->slot_size) == 0u;
          }
        }
      }
      uint8_t confirmed =
          atomic_load_explicit(&record->state, memory_order_acquire);
      if (confirmed != H8_MD_RECORD_LIVE) {
        stable_exact = false;
        H8_MD_COUNT(h8_md_stable_exact_tombstone);
      } else if (stable_exact) {
        H8_MD_COUNT(h8_md_stable_exact_valid);
      } else {
        H8_MD_COUNT(h8_md_stable_exact_invalid);
      }
    } else {
      H8_MD_COUNT(h8_md_stable_exact_tombstone);
    }
  } else {
    H8_MD_COUNT(h8_md_stable_exact_invalid);
  }
  if (stable_exact != authority_exact) {
    H8_MD_COUNT(h8_md_stable_exact_mismatch);
  }
  return stable_exact;
#else
  (void)probe;
  (void)ptr;
  (void)authority_exact;
  return false;
#endif
}

H8MediumDomainAcquireResult h8_medium_domain_stable_acquire_probe(
    H8MediumDomainProbe probe, const void* ptr, H8MediumRun** run_out) {
  if (run_out) *run_out = NULL;
#if defined(H8_MD_STABLE_STORAGE)
  if (probe.kind != H8_MEDIUM_DOMAIN_RUN || !probe.record || !ptr) {
    return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
  }
  H8MediumDomainStableRecord* record =
      (H8MediumDomainStableRecord*)probe.record;
  h8_platform_mutex_lock(&record->lock);
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
      H8_MD_RECORD_LIVE) {
    h8_platform_mutex_unlock(&record->lock);
    return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
  }
  H8MediumRun* run =
      atomic_load_explicit(&record->implementation, memory_order_acquire);
  uintptr_t address = (uintptr_t)ptr;
  size_t payload = (size_t)record->slot_size * (size_t)record->slot_count;
  if (!run || record->slot_size == 0u || address < record->base ||
      address - record->base >= payload) {
    h8_platform_mutex_unlock(&record->lock);
    return H8_MEDIUM_DOMAIN_ACQUIRE_INVALID;
  }
  uintptr_t offset = address - record->base;
  bool exact = (record->slot_size & (record->slot_size - 1u)) == 0u
                   ? (offset & ((uintptr_t)record->slot_size - 1u)) == 0u
                   : (offset % (uintptr_t)record->slot_size) == 0u;
  if (!exact) {
    h8_platform_mutex_unlock(&record->lock);
    return H8_MEDIUM_DOMAIN_ACQUIRE_INVALID;
  }
  if (run_out) *run_out = run;
  return H8_MEDIUM_DOMAIN_ACQUIRE_VALID;
#else
  (void)probe;
  (void)ptr;
  return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
#endif
}

void h8_medium_domain_stable_release_probe(H8MediumDomainProbe probe) {
#if defined(H8_MD_STABLE_STORAGE)
  if (probe.kind == H8_MEDIUM_DOMAIN_RUN && probe.record) {
    H8MediumDomainStableRecord* record =
        (H8MediumDomainStableRecord*)probe.record;
    h8_platform_mutex_unlock(&record->lock);
  }
#else
  (void)probe;
#endif
}

H8MediumDomainAcquireResult h8_medium_domain_owner_witness_acquire(
    H8MediumDomainProbe probe, const void* ptr, uint64_t current_owner_word,
    H8MediumRun** run_out) {
  if (run_out) *run_out = NULL;
#if defined(H8_MD_STABLE_STORAGE)
  H8_MD_COUNT(h8_md_owner_witness_attempt);
  if (probe.kind != H8_MEDIUM_DOMAIN_RUN || !probe.record || !ptr ||
      current_owner_word == 0u) {
    H8_MD_COUNT(h8_md_owner_witness_fallback);
    return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
  }
  H8MediumDomainStableRecord* record =
      (H8MediumDomainStableRecord*)probe.record;
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
          H8_MD_RECORD_LIVE ||
      atomic_load_explicit(&record->owner_word, memory_order_acquire) !=
          current_owner_word) {
    H8_MD_COUNT(h8_md_owner_witness_fallback);
    return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
  }
  H8MediumRun* run =
      atomic_load_explicit(&record->implementation, memory_order_acquire);
  uintptr_t address = (uintptr_t)ptr;
  size_t payload = (size_t)record->slot_size * (size_t)record->slot_count;
  if (!run || record->slot_size == 0u || address < record->base ||
      address - record->base >= payload) {
    H8_MD_COUNT(h8_md_owner_witness_invalid);
    return H8_MEDIUM_DOMAIN_ACQUIRE_INVALID;
  }
  uintptr_t offset = address - record->base;
  bool exact = (record->slot_size & (record->slot_size - 1u)) == 0u
                   ? (offset & ((uintptr_t)record->slot_size - 1u)) == 0u
                   : (offset % (uintptr_t)record->slot_size) == 0u;
  if (!exact) {
    H8_MD_COUNT(h8_md_owner_witness_invalid);
    return H8_MEDIUM_DOMAIN_ACQUIRE_INVALID;
  }
  if (atomic_load_explicit(&record->state, memory_order_acquire) !=
          H8_MD_RECORD_LIVE ||
      atomic_load_explicit(&record->owner_word, memory_order_acquire) !=
          current_owner_word ||
      atomic_load_explicit(&record->implementation, memory_order_acquire) !=
          run) {
    H8_MD_COUNT(h8_md_owner_witness_fallback);
    return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
  }
  if (run_out) *run_out = run;
  H8_MD_COUNT(h8_md_owner_witness_valid);
  return H8_MEDIUM_DOMAIN_ACQUIRE_VALID;
#else
  (void)probe;
  (void)ptr;
  (void)current_owner_word;
  return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
#endif
}

bool h8_medium_domain_stable_turnover_test(size_t iterations) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  if (iterations == 0u || iterations > H8_MD_STABLE_RECORD_CAP / 2u) {
    H8_MD_COUNT(h8_md_stable_turnover_fail);
    return false;
  }
  H8MediumRun synthetic = {0};
  _Atomic uint32_t synthetic_slot_state[4] = {0};
  _Atomic uint64_t synthetic_pending_bits[1] = {0};
  synthetic.base = (uint8_t*)(uintptr_t)UINT64_C(0x100000000);
  synthetic.class_id = 1u;
  synthetic.run_size = H8_MEDIUM_QUANTUM_BYTES;
  synthetic.slot_state = synthetic_slot_state;
  synthetic.pending_bits = synthetic_pending_bits;
  const void* previous_record = NULL;
  for (size_t i = 0u; i < iterations; ++i) {
    bool non_power_of_two = (i & 1u) != 0u;
    synthetic.class_id = non_power_of_two ? 2u : 1u;
    synthetic.slot_count = non_power_of_two ? 2u : 4u;
    synthetic.slot_size = non_power_of_two ? 24576u : 16384u;
    synthetic.slot_shift = non_power_of_two ? 0u : 14u;
    h8_medium_domain_shadow_register_medium(&synthetic);
    H8MediumDomainProbe probe =
        h8_medium_domain_shadow_lookup(synthetic.base);
    if (probe.kind != H8_MEDIUM_DOMAIN_RUN || !probe.record ||
        probe.record == previous_record) {
      if (probe.record == previous_record && previous_record) {
        H8_MD_COUNT(h8_md_stable_record_reuse_detected);
      }
      H8_MD_COUNT(h8_md_stable_turnover_fail);
      return false;
    }
    h8_medium_domain_stable_compare(probe, &synthetic);
    if (!h8_medium_domain_stable_exact_compare(probe, synthetic.base, true) ||
        !h8_medium_domain_stable_exact_compare(
            probe, synthetic.base + synthetic.slot_size, true) ||
        h8_medium_domain_stable_exact_compare(
            probe, synthetic.base + 1u, false)) {
      H8_MD_COUNT(h8_md_stable_turnover_fail);
      return false;
    }
    h8_medium_domain_shadow_unregister_medium(&synthetic);
    h8_medium_domain_stable_compare(probe, NULL);
    if (h8_medium_domain_stable_exact_compare(probe, synthetic.base, false)) {
      H8_MD_COUNT(h8_md_stable_turnover_fail);
      return false;
    }
    H8MediumDomainProbe after =
        h8_medium_domain_shadow_lookup(synthetic.base);
    if (after.kind != H8_MEDIUM_DOMAIN_NONE) {
      H8_MD_COUNT(h8_md_stable_turnover_fail);
      return false;
    }
    previous_record = probe.record;
  }
#if UINTPTR_MAX > UINT64_C(0x0000ffffffffffff)
  H8MediumDomainProbe high = h8_medium_domain_shadow_lookup(
      (const void*)(uintptr_t)UINT64_C(0x0001000000000000));
  if (high.kind != H8_MEDIUM_DOMAIN_NONE) {
    H8_MD_COUNT(h8_md_stable_turnover_fail);
    return false;
  }
#endif
  H8_MD_COUNT(h8_md_stable_turnover_pass);
  return true;
#else
  (void)iterations;
  return false;
#endif
}

void h8_medium_domain_shadow_compare(H8MediumDomainProbe probe,
                                     H8MediumDomainKind expected) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  _Atomic uint64_t* counter = probe.kind == expected ? &h8_md_kind_match
                                                      : &h8_md_kind_mismatch;
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
#else
  (void)probe;
  (void)expected;
#endif
}

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_OWNER_WITNESS_DIAG)
#define H8_MD_LOAD(name) atomic_load_explicit(&(name), memory_order_relaxed)
#endif
H8MediumDomainShadowStats h8_medium_domain_shadow_stats(void) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_OWNER_WITNESS_DIAG)
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  size_t stable_count = H8_MD_LOAD(h8_md_stable_record_count);
  if (stable_count > H8_MD_STABLE_RECORD_CAP) {
    stable_count = H8_MD_STABLE_RECORD_CAP;
  }
#endif
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
      H8_MD_LOAD(h8_md_page8k_register),
      H8_MD_LOAD(h8_md_high_address_fallback),
      H8_MD_LOAD(h8_md_stable_record_alloc),
      H8_MD_LOAD(h8_md_stable_record_pool_fallback),
      H8_MD_LOAD(h8_md_stable_live_lookup),
      H8_MD_LOAD(h8_md_stable_closing_lookup),
      H8_MD_LOAD(h8_md_stable_tombstone_lookup),
      H8_MD_LOAD(h8_md_stable_backend_mismatch),
      H8_MD_LOAD(h8_md_stable_geometry_mismatch),
      H8_MD_LOAD(h8_md_stable_unregister_missing),
      H8_MD_LOAD(h8_md_stable_turnover_pass),
      H8_MD_LOAD(h8_md_stable_turnover_fail),
      H8_MD_LOAD(h8_md_stable_record_reuse_detected),
      H8_MD_LOAD(h8_md_stable_exact_valid),
      H8_MD_LOAD(h8_md_stable_exact_invalid),
      H8_MD_LOAD(h8_md_stable_exact_tombstone),
      H8_MD_LOAD(h8_md_stable_exact_mismatch),
      H8_MD_LOAD(h8_md_stable_slot_init),
      H8_MD_LOAD(h8_md_stable_slot_sync),
      H8_MD_LOAD(h8_md_stable_slot_mismatch),
      H8_MD_LOAD(h8_md_stable_slot_bound_fallback),
      H8_MD_LOAD(h8_md_stable_slot_final_sync),
      H8_MD_LOAD(h8_md_stable_slot_sync_after_closing),
      H8_MD_LOAD(h8_md_stable_slot_mirror_alloc_authority_free),
      H8_MD_LOAD(h8_md_stable_slot_mirror_free_authority_alloc),
      H8_MD_LOAD(h8_md_stable_slot_other_mismatch),
      H8_MD_LOAD(h8_md_stable_slot_note_without_record),
      H8_MD_LOAD(h8_md_stable_pending_init),
      H8_MD_LOAD(h8_md_stable_pending_sync),
      H8_MD_LOAD(h8_md_stable_pending_final_sync),
      H8_MD_LOAD(h8_md_stable_pending_mismatch),
      H8_MD_LOAD(h8_md_stable_pending_sync_after_closing),
      H8_MD_LOAD(h8_md_stable_pending_note_without_record),
      H8_MD_LOAD(h8_md_stable_lock_acquire),
      H8_MD_LOAD(h8_md_stable_lock_fallback),
      H8_MD_LOAD(h8_md_stable_unlock_mismatch),
      H8_MD_LOAD(h8_md_stable_unregister_lock),
      H8_MD_LOAD(h8_md_stable_owner_init),
      H8_MD_LOAD(h8_md_stable_owner_sync),
      H8_MD_LOAD(h8_md_stable_owner_final_sync),
      H8_MD_LOAD(h8_md_stable_owner_mismatch),
      H8_MD_LOAD(h8_md_stable_owner_sync_after_closing),
      H8_MD_LOAD(h8_md_stable_owner_note_without_record),
      H8_MD_LOAD(h8_md_stable_owner_current_match),
      H8_MD_LOAD(h8_md_stable_owner_current_miss),
      H8_MD_LOAD(h8_md_owner_witness_attempt),
      H8_MD_LOAD(h8_md_owner_witness_valid),
      H8_MD_LOAD(h8_md_owner_witness_invalid),
      H8_MD_LOAD(h8_md_owner_witness_fallback),
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
      sizeof(H8MediumDomainStableRecord) * stable_count,
      sizeof(H8MediumDomainStableRecord) * stable_count};
#else
      0u,
      0u};
#endif
  return stats;
#else
  H8MediumDomainShadowStats stats = {0};
  return stats;
#endif
}
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
#undef H8_MD_LOAD
#endif
#undef H8_MD_COUNT
#if defined(H8_MD_STABLE_STORAGE)
#undef H8_MD_STABLE_STORAGE
#endif

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
void h8_medium_domain_stable_compare(H8MediumDomainProbe probe,
                                     H8MediumRun* authority) {
  (void)probe;
  (void)authority;
}
bool h8_medium_domain_stable_turnover_test(size_t iterations) {
  (void)iterations;
  return false;
}
bool h8_medium_domain_stable_exact_compare(H8MediumDomainProbe probe,
                                           const void* ptr,
                                           bool authority_exact) {
  (void)probe;
  (void)ptr;
  (void)authority_exact;
  return false;
}
void h8_medium_domain_stable_slot_note(H8MediumRun* run, size_t slot,
                                       uint32_t committed_state) {
  (void)run;
  (void)slot;
  (void)committed_state;
}
void h8_medium_domain_stable_pending_note(H8MediumRun* run) { (void)run; }
void h8_medium_domain_stable_owner_note(H8MediumRun* run,
                                        uint64_t committed_owner_word) {
  (void)run;
  (void)committed_owner_word;
}
void h8_medium_domain_stable_owner_compare(H8MediumDomainProbe probe,
                                           uint64_t authority_owner_word,
                                           uint64_t current_owner_word) {
  (void)probe;
  (void)authority_owner_word;
  (void)current_owner_word;
}
bool h8_medium_domain_stable_lock(H8MediumRun* run) {
  (void)run;
  return false;
}
bool h8_medium_domain_stable_unlock(H8MediumRun* run) {
  (void)run;
  return false;
}
bool h8_medium_domain_stable_trylock(H8MediumRun* run, int* result_out) {
  (void)run;
  if (result_out) *result_out = -1;
  return false;
}
H8MediumDomainAcquireResult h8_medium_domain_stable_acquire_probe(
    H8MediumDomainProbe probe, const void* ptr, H8MediumRun** run_out) {
  (void)probe;
  (void)ptr;
  if (run_out) *run_out = NULL;
  return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
}
void h8_medium_domain_stable_release_probe(H8MediumDomainProbe probe) {
  (void)probe;
}
H8MediumDomainAcquireResult h8_medium_domain_owner_witness_acquire(
    H8MediumDomainProbe probe, const void* ptr, uint64_t current_owner_word,
    H8MediumRun** run_out) {
  (void)probe;
  (void)ptr;
  (void)current_owner_word;
  if (run_out) *run_out = NULL;
  return H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK;
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
