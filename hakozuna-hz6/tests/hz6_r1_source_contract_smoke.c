#include "../include/hz6_contract.h"
#include "../scavenge/hz6_scavenge.h"
#include "../source/hz6_source.h"
#include "../source/hz6_source_registry.h"

#if defined(_WIN32)
#include "../source/win_source_virtualalloc.h"
#else
#include "../source/linux_source_mmap.h"
#endif

#include <stdio.h>

static void* smoke_reserve(size_t bytes, size_t align) {
  (void)bytes;
  (void)align;
  return (void*)1;
}

static int smoke_memory_op(void* p, size_t bytes) {
  (void)p;
  (void)bytes;
  return 1;
}

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-source-contract-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  Hz6OsMemoryOps ops;
  ops.reserve = smoke_reserve;
  ops.commit = smoke_memory_op;
  ops.decommit = smoke_memory_op;
  ops.release = smoke_memory_op;
  ops.page_size = 4096;
  ops.allocation_granularity = 65536;
  if (!expect(hz6_source_ops_valid(&ops), "source ops valid")) {
    return 1;
  }
  ops.allocation_granularity = 3000;
  if (!expect(!hz6_source_ops_valid(&ops), "source granularity invalid")) {
    return 1;
  }

  Hz6OsMemoryOps platform_ops;
#if defined(_WIN32)
  platform_ops = hz6_win_virtualalloc_source_ops();
  if (!expect(hz6_source_ops_valid(&platform_ops), "win virtualalloc ops valid")) {
#else
  platform_ops = hz6_linux_mmap_source_ops();
  if (!expect(hz6_source_ops_valid(&platform_ops), "linux mmap ops valid")) {
#endif
    return 1;
  }
  void* mapped =
      platform_ops.reserve(platform_ops.page_size, platform_ops.page_size);
#if defined(_WIN32)
  if (!expect(mapped != NULL, "win virtualalloc reserve") ||
      !expect(platform_ops.commit(mapped, platform_ops.page_size),
              "win virtualalloc commit") ||
      !expect(platform_ops.decommit(mapped, platform_ops.page_size),
              "win virtualalloc decommit") ||
      !expect(platform_ops.release(mapped, platform_ops.page_size),
              "win virtualalloc release")) {
#else
  if (!expect(mapped != NULL, "linux mmap reserve") ||
      !expect(platform_ops.commit(mapped, platform_ops.page_size),
              "linux mmap commit") ||
      !expect(platform_ops.decommit(mapped, platform_ops.page_size),
              "linux mmap decommit") ||
      !expect(platform_ops.release(mapped, platform_ops.page_size),
              "linux mmap release")) {
#endif
    return 1;
  }

  Hz6SourceRegistry source_registry;
  hz6_source_registry_init(&source_registry);
  const Hz6OsMemoryOps* system_lookup =
      hz6_source_registry_lookup(&source_registry, HZ6_SOURCE_SYSTEM);
  const Hz6OsMemoryOps* mmap_lookup =
      hz6_source_registry_lookup(&source_registry,
#if defined(_WIN32)
                                 HZ6_SOURCE_WIN_VIRTUALALLOC);
#else
                                 HZ6_SOURCE_LINUX_MMAP);
#endif
  const Hz6OsMemoryOps* os_paged_lookup =
      hz6_source_registry_lookup(&source_registry, HZ6_SOURCE_OS_PAGED);
#if defined(_WIN32)
  const Hz6SourceKind expected_source_kind = HZ6_SOURCE_WIN_VIRTUALALLOC;
#else
  const Hz6SourceKind expected_source_kind = HZ6_SOURCE_LINUX_MMAP;
#endif
  if (!expect(system_lookup != NULL, "source registry system") ||
      !expect(mmap_lookup != NULL, "source registry platform source") ||
      !expect(os_paged_lookup != NULL, "source registry os paged") ||
      !expect(os_paged_lookup->page_size == mmap_lookup->page_size,
              "source registry os paged maps to platform source") ||
      !expect(hz6_source_registry_lookup(&source_registry,
                                         expected_source_kind) != NULL,
              "source registry platform kind") ||
      !expect(hz6_source_registry_lookup(&source_registry,
                                         HZ6_SOURCE_NONE) == NULL,
              "source registry none miss")) {
    return 1;
  }

  Hz6ScavengeBudget scavenge_budget;
  hz6_scavenge_budget_init(&scavenge_budget, 96);
  if (!expect(hz6_scavenge_remaining_bytes(&scavenge_budget) == 96,
              "scavenge initial remaining") ||
      !expect(hz6_scavenge_account_release(&scavenge_budget, 64),
              "scavenge account first release") ||
      !expect(scavenge_budget.objects_released == 1,
              "scavenge object count") ||
      !expect(!hz6_scavenge_account_release(&scavenge_budget, 64),
              "scavenge bounded") ||
      !expect(hz6_scavenge_remaining_bytes(&scavenge_budget) == 32,
              "scavenge remaining")) {
    return 1;
  }

  printf("hz6-r1-source-contract-smoke ok\n");
  return 0;
}
