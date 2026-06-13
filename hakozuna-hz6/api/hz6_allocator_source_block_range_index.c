#include "hz6_allocator.h"

#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
static uintptr_t hz6_source_block_route_range_page(uintptr_t address) {
  return address & ~(uintptr_t)(HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY -
                                1u);
}

static size_t hz6_source_block_route_range_hash(uintptr_t page) {
  return (size_t)((page >> 12) %
                  HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY);
}

static size_t hz6_allocator_source_block_index(const Hz6Allocator* allocator,
                                               const Hz6SourceBlock* block) {
  if (!allocator || !block || block < allocator->source_blocks ||
      block >= allocator->source_blocks + HZ6_SOURCE_BLOCK_CAPACITY) {
    return HZ6_SOURCE_BLOCK_CAPACITY;
  }
  return (size_t)(block - allocator->source_blocks);
}
#endif

#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
static int hz6_source_block_small_run_route_eligible(
    const Hz6SourceBlock* block) {
  return block && block->run_front_id == HZ6_FRONT_TOY &&
         block->run_slot_bytes >= HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES &&
         block->run_slot_bytes <= HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES;
}
#endif

void hz6_allocator_source_block_range_index_register(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !block || !block->ptr || block->bytes == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return;
  }
  size_t block_index = hz6_allocator_source_block_index(allocator, block);
  if (block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
    return;
  }
  uintptr_t begin = hz6_source_block_route_range_page((uintptr_t)block->ptr);
  uintptr_t end = (uintptr_t)block->ptr + block->bytes - 1u;
  uintptr_t last = hz6_source_block_route_range_page(end);
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  size_t inserted_count = 0;
#endif
  for (uintptr_t page = begin;; page +=
           HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY) {
    size_t start = hz6_source_block_route_range_hash(page);
    size_t first_tombstone = HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
    int inserted = 0;
    for (size_t probe = 0;
         probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
         ++probe) {
      size_t index = start + probe;
      if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
      }
      Hz6SourceBlockRangeIndexEntry* entry =
          &allocator->source_block_range_index[index];
      if (entry->active == 2u) {
        if (first_tombstone >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
          first_tombstone = index;
        }
        continue;
      }
      if (entry->active == 1u && entry->page != page) {
        continue;
      }
      if (entry->active == 0u &&
          first_tombstone < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        entry = &allocator->source_block_range_index[first_tombstone];
      }
      entry->page = page;
      entry->block_index = (uint32_t)block_index;
      entry->active = 1;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_register;
#endif
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
      ++inserted_count;
#endif
      inserted = 1;
      break;
    }
    if (!inserted) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_register_fail;
#endif
      return;
    }
    if (page == last) {
      break;
    }
  }
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  if (inserted_count > 0 &&
      hz6_source_block_small_run_route_eligible(block)) {
    ++allocator->small_run_route_range_registered;
  }
#endif
#else
  (void)allocator;
  (void)block;
#endif
}

void hz6_allocator_source_block_range_index_unregister(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !block || !block->ptr || block->bytes == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return;
  }
  size_t block_index = hz6_allocator_source_block_index(allocator, block);
  if (block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
    return;
  }
  uintptr_t begin = hz6_source_block_route_range_page((uintptr_t)block->ptr);
  uintptr_t end = (uintptr_t)block->ptr + block->bytes - 1u;
  uintptr_t last = hz6_source_block_route_range_page(end);
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  size_t removed_count = 0;
#endif
  for (uintptr_t page = begin;; page +=
           HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY) {
    size_t start = hz6_source_block_route_range_hash(page);
    for (size_t probe = 0;
         probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
         ++probe) {
      size_t index = start + probe;
      if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
      }
      Hz6SourceBlockRangeIndexEntry* entry =
          &allocator->source_block_range_index[index];
      if (entry->active == 0u) {
        break;
      }
      if (entry->active != 1u) {
        continue;
      }
      if (entry->page != page) {
        continue;
      }
      if (entry->block_index == block_index) {
        entry->active = 2u;
        entry->page = 0;
        entry->block_index = 0;
#if HZ6_DIAGNOSTIC_PROBES
        ++allocator->stats.source_block_route_range_index_unregister;
#endif
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
        ++removed_count;
#endif
      }
      break;
    }
    if (page == last) {
      break;
    }
  }
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  if (removed_count > 0 &&
      hz6_source_block_small_run_route_eligible(block) &&
      allocator->small_run_route_range_registered > 0) {
    --allocator->small_run_route_range_registered;
  }
#endif
#else
  (void)allocator;
  (void)block;
#endif
}

Hz6SourceBlock* hz6_allocator_source_block_range_index_lookup(
    Hz6Allocator* allocator,
    const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !ptr ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_range_index_lookup;
#endif
  uintptr_t page = hz6_source_block_route_range_page((uintptr_t)ptr);
  size_t start = hz6_source_block_route_range_hash(page);
  for (size_t probe = 0;
       probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
       ++probe) {
    size_t index = start + probe;
    if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
      index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_range_index_probe_total;
    if (probe + 1u >
        allocator->stats.source_block_route_range_index_probe_max) {
      allocator->stats.source_block_route_range_index_probe_max = probe + 1u;
    }
#endif
    Hz6SourceBlockRangeIndexEntry* entry =
        &allocator->source_block_range_index[index];
    if (entry->active == 0u) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_miss;
#endif
      return NULL;
    }
    if (entry->active != 1u) {
      continue;
    }
    if (entry->page != page) {
      continue;
    }
    if (entry->block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_stale;
#endif
      return NULL;
    }
    Hz6SourceBlock* block = &allocator->source_blocks[entry->block_index];
    const unsigned char* user = (const unsigned char*)ptr;
    const unsigned char* base = (const unsigned char*)block->ptr;
    if (!hz6_source_block_active(block) || !block->ptr ||
        user < base || user >= base + block->bytes) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_stale;
#endif
      return NULL;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_range_index_hit;
#endif
    return block;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_range_index_miss;
#endif
  return NULL;
#else
  (void)allocator;
  (void)ptr;
  return NULL;
#endif
}
