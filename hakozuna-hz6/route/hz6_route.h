#ifndef HZ6_ROUTE_H
#define HZ6_ROUTE_H

#include "../include/hz6_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6RouteEntry {
  uintptr_t base;
  void* descriptor;
#if HZ6_ROUTE_PACKED_META_L1
  uint32_t generation;
  uint32_t meta;
#else
  uint32_t bytes;
  uint32_t generation;
  uint16_t front_id;
  uint16_t class_id;
  unsigned exact_valid : 1;
  unsigned active : 1;
  unsigned tombstone : 1;
#endif
} Hz6RouteEntry;

#if HZ6_ROUTE_PACKED_META_L1
#if HZ6_ROUTE_BYTES16_MINUS1_L2
typedef uint16_t Hz6RouteBytesStorage;
#else
typedef uint32_t Hz6RouteBytesStorage;
#endif
#endif

typedef struct Hz6RouteTable {
  Hz6RouteEntry* entries;
#if HZ6_ROUTE_PACKED_META_L1
  Hz6RouteBytesStorage* bytes;
#endif
  size_t capacity;
  size_t active_count;
  size_t tombstone_count;
  size_t register_used_tombstone;
  size_t register_full_probe_with_tombstone;
} Hz6RouteTable;

#define HZ6_ROUTE_META_EXACT_VALID 0x1u
#define HZ6_ROUTE_META_ACTIVE 0x2u
#define HZ6_ROUTE_META_TOMBSTONE 0x4u

#if HZ6_ROUTE_PACKED_META_L1
#define HZ6_ROUTE_META_FRONT_SHIFT 8u
#define HZ6_ROUTE_META_CLASS_SHIFT 16u
#define HZ6_ROUTE_META_FRONT_MASK 0xffu
#define HZ6_ROUTE_META_CLASS_MASK 0xffffu

static inline uint16_t hz6_route_entry_front_id(const Hz6RouteEntry* entry) {
  return (uint16_t)((entry->meta >> HZ6_ROUTE_META_FRONT_SHIFT) &
                    HZ6_ROUTE_META_FRONT_MASK);
}

static inline uint16_t hz6_route_entry_class_id(const Hz6RouteEntry* entry) {
  return (uint16_t)((entry->meta >> HZ6_ROUTE_META_CLASS_SHIFT) &
                    HZ6_ROUTE_META_CLASS_MASK);
}

static inline int hz6_route_entry_exact_valid(const Hz6RouteEntry* entry) {
  return (entry->meta & HZ6_ROUTE_META_EXACT_VALID) != 0;
}

static inline int hz6_route_entry_active(const Hz6RouteEntry* entry) {
  return (entry->meta & HZ6_ROUTE_META_ACTIVE) != 0;
}

static inline int hz6_route_entry_tombstone(const Hz6RouteEntry* entry) {
  return (entry->meta & HZ6_ROUTE_META_TOMBSTONE) != 0;
}

static inline void hz6_route_entry_set_meta(Hz6RouteEntry* entry,
                                            uint16_t front_id,
                                            uint16_t class_id,
                                            unsigned flags) {
  entry->meta = (flags & 0xffu) |
                (((uint32_t)front_id & HZ6_ROUTE_META_FRONT_MASK)
                 << HZ6_ROUTE_META_FRONT_SHIFT) |
                (((uint32_t)class_id & HZ6_ROUTE_META_CLASS_MASK)
                 << HZ6_ROUTE_META_CLASS_SHIFT);
}

static inline void hz6_route_entry_clear(Hz6RouteEntry* entry) {
  entry->base = 0;
  entry->descriptor = NULL;
  entry->generation = 0;
  entry->meta = 0;
}

static inline void hz6_route_entry_mark_tombstone(Hz6RouteEntry* entry) {
  entry->meta &= ~HZ6_ROUTE_META_ACTIVE;
  entry->meta |= HZ6_ROUTE_META_TOMBSTONE;
}

static inline uint32_t hz6_route_entry_generation(const Hz6RouteTable* table,
                                                  size_t index) {
  return table->entries[index].generation;
}

static inline void hz6_route_entry_set_generation(Hz6RouteTable* table,
                                                  size_t index,
                                                  uint32_t generation) {
  table->entries[index].generation = generation;
}

static inline uint32_t hz6_route_entry_bytes(const Hz6RouteTable* table,
                                             size_t index) {
#if HZ6_ROUTE_BYTES16_MINUS1_L2
  return (table && table->bytes) ? ((uint32_t)table->bytes[index] + 1u) : 0;
#else
  return (table && table->bytes) ? table->bytes[index] : 0;
#endif
}

static inline void hz6_route_entry_set_bytes(Hz6RouteTable* table,
                                             size_t index,
                                             uint32_t bytes) {
  if (table && table->bytes) {
#if HZ6_ROUTE_BYTES16_MINUS1_L2
    table->bytes[index] =
        (Hz6RouteBytesStorage)(bytes == 0 ? 0u : (bytes - 1u));
#else
    table->bytes[index] = bytes;
#endif
  }
}
#else
static inline uint16_t hz6_route_entry_front_id(const Hz6RouteEntry* entry) {
  return entry->front_id;
}

static inline uint16_t hz6_route_entry_class_id(const Hz6RouteEntry* entry) {
  return entry->class_id;
}

static inline int hz6_route_entry_exact_valid(const Hz6RouteEntry* entry) {
  return entry->exact_valid;
}

static inline int hz6_route_entry_active(const Hz6RouteEntry* entry) {
  return entry->active;
}

static inline int hz6_route_entry_tombstone(const Hz6RouteEntry* entry) {
  return entry->tombstone;
}

static inline void hz6_route_entry_set_meta(Hz6RouteEntry* entry,
                                            uint16_t front_id,
                                            uint16_t class_id,
                                            unsigned flags) {
  entry->front_id = front_id;
  entry->class_id = class_id;
  entry->exact_valid = (flags & 0x1u) != 0;
  entry->active = (flags & 0x2u) != 0;
  entry->tombstone = (flags & 0x4u) != 0;
}

static inline void hz6_route_entry_clear(Hz6RouteEntry* entry) {
  entry->active = 0;
  entry->exact_valid = 0;
  entry->tombstone = 0;
}

static inline void hz6_route_entry_mark_tombstone(Hz6RouteEntry* entry) {
  entry->active = 0;
  entry->tombstone = 1;
}

static inline uint32_t hz6_route_entry_generation(const Hz6RouteTable* table,
                                                  size_t index) {
  return table->entries[index].generation;
}

static inline void hz6_route_entry_set_generation(Hz6RouteTable* table,
                                                  size_t index,
                                                  uint32_t generation) {
  table->entries[index].generation = generation;
}

static inline uint32_t hz6_route_entry_bytes(const Hz6RouteTable* table,
                                             size_t index) {
  return table->entries[index].bytes;
}

static inline void hz6_route_entry_set_bytes(Hz6RouteTable* table,
                                             size_t index,
                                             uint32_t bytes) {
  table->entries[index].bytes = bytes;
}
#endif

size_t hz6_route_hash_index(uintptr_t base, size_t capacity);
size_t hz6_route_probe_step(uintptr_t base, size_t capacity);

#if HZ6_ROUTE_LINEAR_WRAP_L1
static inline size_t hz6_route_linear_next_index(size_t index,
                                                 size_t capacity) {
  ++index;
  if (index == capacity) {
    return 0;
  }
  return index;
}
#endif

#if HZ6_ROUTE_DOUBLE_HASH_L1
#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  (((start) + (((probe_index) % (capacity)) * (step))) % (capacity))
#elif HZ6_ROUTE_LINEAR_WRAP_L1
static inline size_t hz6_route_linear_probe_index(size_t start,
                                                  size_t capacity,
                                                  size_t probe_index) {
  size_t index = start + probe_index;
  if (index >= capacity) {
    index -= capacity;
  }
  return index;
}

#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  ((void)(step), hz6_route_linear_probe_index((start), (capacity), \
                                              (probe_index)))
#else
#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  ((void)(step), (((start) + (probe_index)) % (capacity)))
#endif

void hz6_route_table_init(Hz6RouteTable* table,
                          Hz6RouteEntry* entries,
                          size_t capacity);

#if HZ6_ROUTE_PACKED_META_L1
void hz6_route_table_attach_bytes(Hz6RouteTable* table,
                                  Hz6RouteBytesStorage* bytes);
#endif

int hz6_route_table_compact_tombstones(Hz6RouteTable* table,
                                       size_t* moved_count);

int hz6_route_register_exact(Hz6RouteTable* table,
                             void* base,
                             size_t bytes,
                             uint16_t front_id,
                             uint16_t class_id,
                             uint32_t generation,
                             void* descriptor,
                             size_t* probe_count);

int hz6_route_replace_exact_descriptor(Hz6RouteTable* table,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t old_generation,
                                       void* old_descriptor,
                                       uint32_t new_generation,
                                       void* new_descriptor,
                                       size_t* probe_count);

int hz6_route_register_invalid_range(Hz6RouteTable* table,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t* probe_count);

void hz6_route_unregister_exact(Hz6RouteTable* table,
                               void* base,
                               size_t* probe_count);

void hz6_route_unregister_invalid_range(Hz6RouteTable* table,
                                       void* base,
                                       size_t* probe_count);

Hz6RouteResult hz6_route_lookup(const Hz6RouteTable* table, const void* ptr);

Hz6RouteResult hz6_route_lookup_exact_probe(const Hz6RouteTable* table,
                                            const void* ptr,
                                            size_t* probe_count);

Hz6RouteResult hz6_route_lookup_exact(const Hz6RouteTable* table,
                                      const void* ptr);

Hz6RouteResult hz6_route_lookup_probe(const Hz6RouteTable* table,
                                      const void* ptr,
                                      size_t* probe_count);

#ifdef __cplusplus
}
#endif

#endif
