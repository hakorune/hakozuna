#include "hz6_source_registry.h"

#if defined(__linux__)
#include "linux_source_mmap.h"
#endif

#if defined(_WIN32)
#include "win_source_virtualalloc.h"
#endif

static void hz6_source_registry_add(Hz6SourceRegistry* registry,
                                    Hz6SourceKind kind,
                                    Hz6OsMemoryOps ops) {
  if (!registry || kind == HZ6_SOURCE_NONE || !hz6_source_ops_valid(&ops)) {
    return;
  }

  for (size_t i = 0; i < HZ6_SOURCE_REGISTRY_CAPACITY; ++i) {
    Hz6SourceBinding* binding = &registry->binding[i];
    if (!binding->available || binding->kind == kind) {
      binding->kind = kind;
      binding->ops = ops;
      binding->available = 1;
      return;
    }
  }
}

void hz6_source_registry_init(Hz6SourceRegistry* registry) {
  if (!registry) {
    return;
  }

  for (size_t i = 0; i < HZ6_SOURCE_REGISTRY_CAPACITY; ++i) {
    registry->binding[i].kind = HZ6_SOURCE_NONE;
    registry->binding[i].available = 0;
  }

  hz6_source_registry_add(registry, HZ6_SOURCE_SYSTEM,
                          hz6_system_source_ops());

#if defined(__linux__)
  Hz6OsMemoryOps linux_ops = hz6_linux_mmap_source_ops();
  hz6_source_registry_add(registry, HZ6_SOURCE_LINUX_MMAP, linux_ops);
  hz6_source_registry_add(registry, HZ6_SOURCE_OS_PAGED, linux_ops);
#endif

#if defined(_WIN32)
  Hz6OsMemoryOps win_ops = hz6_win_virtualalloc_source_ops();
  hz6_source_registry_add(registry, HZ6_SOURCE_WIN_VIRTUALALLOC, win_ops);
  hz6_source_registry_add(registry, HZ6_SOURCE_OS_PAGED, win_ops);
#endif
}
