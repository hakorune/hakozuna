#include "h8_internal.h"
#include "h8_medium.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 2u, 1u},
    {65536u, H8_MEDIUM_RUN_BYTES, 1u, 1u},
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "medium v1 scaffold expects four coarse classes");

bool h8_medium_size_supported(size_t size) {
  return size >= H8_MEDIUM_MIN_SIZE && size <= H8_MEDIUM_MAX_SIZE;
}

uint32_t h8_medium_class_for_size(size_t size) {
  if (size <= 8192u) {
    return 0u;
  }
  if (size <= 16384u) {
    return 1u;
  }
  if (size <= 32768u) {
    return 2u;
  }
  return 3u;
}

const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &k_h8_medium_classes[class_id];
}

uint32_t h8_medium_rounded_size(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return 0u;
  }
  return k_h8_medium_classes[h8_medium_class_for_size(size)].slot_size;
}

bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out) {
  if (!run || !run->base || !ptr || run->slot_size == 0u ||
      run->slot_count == 0u) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  if (addr < base) {
    return false;
  }
  uintptr_t offset = addr - base;
  size_t payload = (size_t)run->slot_size * (size_t)run->slot_count;
  if (offset >= payload || (offset % run->slot_size) != 0u) {
    return false;
  }
  size_t slot = (size_t)(offset / run->slot_size);
  if (slot >= run->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot) {
  if (!run || !run->base || slot >= run->slot_count) {
    return NULL;
  }
  return run->base + slot * (size_t)run->slot_size;
}

H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec) {
    return NULL;
  }
  H8MediumRun* run = h8_sys_calloc(1, sizeof(*run));
  if (!run) {
    return NULL;
  }
  run->slot_state = h8_sys_calloc(spec->slot_count, sizeof(*run->slot_state));
  run->pending_bits = h8_sys_calloc(spec->bitmap_words, sizeof(*run->pending_bits));
  if (!run->slot_state || !run->pending_bits) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  void* payload = mmap(NULL, spec->run_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (payload == MAP_FAILED) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  run->base = payload;
  run->class_id = (uint16_t)class_id;
  run->slot_size = spec->slot_size;
  run->slot_count = spec->slot_count;
  run->run_size = spec->run_size;
  run->free_mask = (run->slot_count == 64u)
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  run->allocated_mask = 0;
  atomic_store_explicit(&run->state, H8_MEDIUM_RUN_ACTIVE,
                        memory_order_relaxed);
  atomic_store_explicit(&run->pending_word_mask, 0, memory_order_relaxed);
  for (uint16_t i = 0; i < run->slot_count; ++i) {
    atomic_store_explicit(&run->slot_state[i], H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_relaxed);
  }
  return run;
}

void h8_medium_run_destroy_scaffold(H8MediumRun* run) {
  if (!run) {
    return;
  }
  if (run->base) {
    munmap(run->base, run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES);
  }
  h8_sys_free(run->pending_bits);
  h8_sys_free(run->slot_state);
  h8_sys_free(run);
}

void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run) {
  if (!run || atomic_load_explicit(&run->state, memory_order_acquire) !=
                  H8_MEDIUM_RUN_ACTIVE ||
      run->free_mask == 0) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
  return h8_medium_slot_ptr(run, slot);
}

bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr) {
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((run->allocated_mask & bit) == 0 || (run->free_mask & bit) != 0 ||
      atomic_load_explicit(&run->pending_bits[slot >> 6u],
                           memory_order_acquire) &
          (UINT64_C(1) << (slot & 63u))) {
    return false;
  }
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                        memory_order_release);
  run->allocated_mask &= ~bit;
  run->free_mask |= bit;
  return true;
}
