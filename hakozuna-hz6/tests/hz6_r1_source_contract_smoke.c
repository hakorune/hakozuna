#include "../include/hz6_contract.h"
#include "../scavenge/hz6_scavenge.h"
#include "../source/linux_source_mmap.h"
#include "../source/hz6_source.h"
#include "../source/hz6_source_registry.h"

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

  Hz6OsMemoryOps linux_ops = hz6_linux_mmap_source_ops();
  if (!expect(hz6_source_ops_valid(&linux_ops), "linux mmap ops valid")) {
    return 1;
  }
  void* mapped = linux_ops.reserve(linux_ops.page_size, linux_ops.page_size);
  if (!expect(mapped != NULL, "linux mmap reserve") ||
      !expect(linux_ops.commit(mapped, linux_ops.page_size),
              "linux mmap commit") ||
      !expect(linux_ops.decommit(mapped, linux_ops.page_size),
              "linux mmap decommit") ||
      !expect(linux_ops.release(mapped, linux_ops.page_size),
              "linux mmap release")) {
    return 1;
  }

  Hz6SourceRegistry source_registry;
  hz6_source_registry_init(&source_registry);
  const Hz6OsMemoryOps* system_lookup =
      hz6_source_registry_lookup(&source_registry, HZ6_SOURCE_SYSTEM);
  const Hz6OsMemoryOps* mmap_lookup =
      hz6_source_registry_lookup(&source_registry, HZ6_SOURCE_LINUX_MMAP);
  const Hz6OsMemoryOps* os_paged_lookup =
      hz6_source_registry_lookup(&source_registry, HZ6_SOURCE_OS_PAGED);
  if (!expect(system_lookup != NULL, "source registry system") ||
      !expect(mmap_lookup != NULL, "source registry linux mmap") ||
      !expect(os_paged_lookup != NULL, "source registry os paged") ||
      !expect(os_paged_lookup->page_size == mmap_lookup->page_size,
              "source registry os paged maps to linux mmap") ||
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
