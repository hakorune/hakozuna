#include "hz6_source.h"

#include <stdlib.h>

static int hz6_power_of_two(size_t value) {
  return value != 0 && (value & (value - (size_t)1)) == 0;
}

int hz6_source_ops_valid(const Hz6OsMemoryOps* ops) {
  if (!ops || !ops->reserve || !ops->commit || !ops->decommit ||
      !ops->release) {
    return 0;
  }
  if (!hz6_power_of_two(ops->page_size) ||
      !hz6_power_of_two(ops->allocation_granularity)) {
    return 0;
  }
  if (ops->allocation_granularity < ops->page_size) {
    return 0;
  }
  return 1;
}

static int hz6_source_system_commit(void* p, size_t bytes) {
  return p != NULL && bytes != 0;
}

static int hz6_source_system_decommit(void* p, size_t bytes) {
  (void)p;
  return bytes != 0;
}

static void* hz6_source_system_reserve(size_t bytes, size_t align) {
  (void)align;
  return hz6_source_system_alloc(bytes);
}

Hz6OsMemoryOps hz6_system_source_ops(void) {
  Hz6OsMemoryOps ops;
  ops.reserve = hz6_source_system_reserve;
  ops.commit = hz6_source_system_commit;
  ops.decommit = hz6_source_system_decommit;
  ops.release = hz6_source_system_release;
  ops.page_size = 16;
  ops.allocation_granularity = 16;
  return ops;
}

void* hz6_source_system_alloc(size_t bytes) {
  return bytes == 0 ? NULL : malloc(bytes);
}

void hz6_source_system_free(void* ptr) {
  free(ptr);
}

int hz6_source_system_release(void* ptr, size_t bytes) {
  (void)bytes;
  free(ptr);
  return 1;
}
