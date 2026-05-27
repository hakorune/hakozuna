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

void* hz6_source_system_alloc(size_t bytes) {
  return bytes == 0 ? NULL : malloc(bytes);
}

void hz6_source_system_free(void* ptr) {
  free(ptr);
}
