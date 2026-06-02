#include "hz6_route.h"

#include <stdlib.h>
#include <string.h>

static int hz6_route_table_insert_compacted(Hz6RouteTable* table,
                                            const Hz6RouteEntry* saved) {
  if (!table || !table->entries || !saved || !saved->active) {
    return 0;
  }
  size_t start = hz6_route_hash_index(saved->base, table->capacity);
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = (start + i) % table->capacity;
    Hz6RouteEntry* entry = &table->entries[index];
    if (entry->active || entry->tombstone) {
      continue;
    }
    *entry = *saved;
    entry->active = 1;
    entry->tombstone = 0;
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
    if (table->entries[i].active) {
      ++active_count;
    }
  }

  if (active_count == 0) {
    memset(table->entries, 0, table->capacity * sizeof(table->entries[0]));
    table->active_count = 0;
    table->tombstone_count = 0;
    return 1;
  }

  Hz6RouteEntry* saved =
      (Hz6RouteEntry*)malloc(active_count * sizeof(Hz6RouteEntry));
  if (!saved) {
    return 0;
  }

  size_t saved_count = 0;
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (!entry->active) {
      continue;
    }
    saved[saved_count] = *entry;
    saved[saved_count].tombstone = 0;
    ++saved_count;
  }

  memset(table->entries, 0, table->capacity * sizeof(table->entries[0]));
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
