#ifndef HZ6_SOURCE_REGISTRY_H
#define HZ6_SOURCE_REGISTRY_H

#include "hz6_source.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_SOURCE_REGISTRY_CAPACITY 4u

typedef struct Hz6SourceBinding {
  Hz6SourceKind kind;
  Hz6OsMemoryOps ops;
  int available;
} Hz6SourceBinding;

typedef struct Hz6SourceRegistry {
  Hz6SourceBinding binding[HZ6_SOURCE_REGISTRY_CAPACITY];
} Hz6SourceRegistry;

void hz6_source_registry_init(Hz6SourceRegistry* registry);

const Hz6OsMemoryOps* hz6_source_registry_lookup(
    const Hz6SourceRegistry* registry,
    Hz6SourceKind kind);

#ifdef __cplusplus
}
#endif

#endif
