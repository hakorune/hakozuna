#include "hz6_route.h"

#include <stdlib.h>
#include <string.h>

typedef struct Hz6RouteCompactionEntry {
  Hz6RouteEntry entry;
  uint32_t bytes;
} Hz6RouteCompactionEntry;

static int hz6_route_table_insert_compacted(Hz6RouteTable* table,
                                            const Hz6RouteCompactionEntry* saved) {
  if (!table || !table->entries || !saved ||
      !hz6_route_entry_active(&saved->entry)) {
    return 0;
  }
  size_t start = hz6_route_hash_index(saved->entry.base, table->capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(saved->entry.base, table->capacity);
#else
  size_t step = 1;
#endif
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step, table->capacity, i);
    Hz6RouteEntry* entry = &table->entries[index];
    if (hz6_route_entry_active(entry) || hz6_route_entry_tombstone(entry)) {
      continue;
    }
    *entry = saved->entry;
    hz6_route_entry_set_bytes(table, index, saved->bytes);
    hz6_route_entry_set_meta(
        entry,
        hz6_route_entry_front_id(&saved->entry),
        hz6_route_entry_class_id(&saved->entry),
        (hz6_route_entry_exact_valid(&saved->entry)
             ? HZ6_ROUTE_META_EXACT_VALID
             : 0u) |
            HZ6_ROUTE_META_ACTIVE);
    ++table->active_count;
    return 1;
  }
  return 0;
}

int hz6_route_table_compact_tombstones(Hz6RouteTable* table,
                                       size_t* moved_count) {
  if (moved_count) {
    *moved_count = 0;
  }
  if (!table || !table->entries || table->capacity == 0 ||
      table->tombstone_count == 0) {
    return 1;
  }

  size_t active_count = 0;
  for (size_t i = 0; i < table->capacity; ++i) {
    if (hz6_route_entry_active(&table->entries[i])) {
      ++active_count;
    }
  }

  if (active_count == 0) {
    memset(table->entries, 0, table->capacity * sizeof(table->entries[0]));
#if HZ6_ROUTE_PACKED_META_L1
    if (table->bytes) {
      memset(table->bytes, 0, table->capacity * sizeof(table->bytes[0]));
    }
#endif
    table->active_count = 0;
    table->tombstone_count = 0;
    return 1;
  }

  Hz6RouteCompactionEntry* saved =
      (Hz6RouteCompactionEntry*)malloc(active_count *
                                       sizeof(Hz6RouteCompactionEntry));
  if (!saved) {
    return 0;
  }

  size_t saved_count = 0;
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (!hz6_route_entry_active(entry)) {
      continue;
    }
    saved[saved_count].entry = *entry;
    saved[saved_count].bytes = hz6_route_entry_bytes(table, i);
    hz6_route_entry_set_meta(
        &saved[saved_count].entry,
        hz6_route_entry_front_id(entry),
        hz6_route_entry_class_id(entry),
        (hz6_route_entry_exact_valid(entry) ? HZ6_ROUTE_META_EXACT_VALID : 0u) |
            HZ6_ROUTE_META_ACTIVE);
    ++saved_count;
  }

  memset(table->entries, 0, table->capacity * sizeof(table->entries[0]));
#if HZ6_ROUTE_PACKED_META_L1
  if (table->bytes) {
    memset(table->bytes, 0, table->capacity * sizeof(table->bytes[0]));
  }
#endif
  table->active_count = 0;
  table->tombstone_count = 0;

  for (size_t i = 0; i < saved_count; ++i) {
    if (!hz6_route_table_insert_compacted(table, &saved[i])) {
      free(saved);
      return 0;
    }
  }

  if (moved_count) {
    *moved_count = saved_count;
  }
  free(saved);
  return 1;
}
