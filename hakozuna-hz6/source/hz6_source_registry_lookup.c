#include "hz6_source_registry.h"

const Hz6OsMemoryOps* hz6_source_registry_lookup(
    const Hz6SourceRegistry* registry,
    Hz6SourceKind kind) {
  if (!registry || kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  for (size_t i = 0; i < HZ6_SOURCE_REGISTRY_CAPACITY; ++i) {
    const Hz6SourceBinding* binding = &registry->binding[i];
    if (binding->available && binding->kind == kind) {
      return &binding->ops;
    }
  }
  return NULL;
}
