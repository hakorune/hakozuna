#include "../frontcache/hz6_frontcache.h"
#include "../frontcache/hz6_size_class.h"
#include "../include/hz6_contract.h"
#include "../owner/hz6_owner.h"
#include "../policy/hz6_profiles.h"
#include "../scavenge/hz6_scavenge.h"
#include "../source/linux_source_mmap.h"
#include "../source/hz6_source.h"
#include "../source/hz6_source_registry.h"

#include <stdio.h>

typedef struct SmokeDescriptor {
  int marker;
} SmokeDescriptor;

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
    fprintf(stderr, "hz6-r1-contract-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  SmokeDescriptor descriptor;
  descriptor.marker = 42;

  unsigned char object[256];
  void* base = (void*)object;

  Hz6OwnerRecord owner;
  owner.token.slot = 3;
  owner.token.generation = 5;
  owner.state = HZ6_OWNER_ALIVE;

  Hz6OwnerToken same;
  same.slot = 3;
  same.generation = 5;
  Hz6OwnerToken stale;
  stale.slot = 3;
  stale.generation = 4;

  if (!expect(hz6_owner_is_alive(&owner, same), "owner alive") ||
      !expect(!hz6_owner_is_alive(&owner, stale), "owner stale rejected")) {
    return 1;
  }

  Hz6ProfileConfig speed = hz6_profile_config(HZ6_PROFILE_SPEED);
  Hz6ProfileConfig rss = hz6_profile_config(HZ6_PROFILE_RSS);
  Hz6ProfileConfig strict = hz6_profile_config(HZ6_PROFILE_STRICT);
  Hz6ProfileConfig remote = hz6_profile_config(HZ6_PROFILE_REMOTE);
  if (!expect(speed.transfer_first == 1, "speed transfer-first") ||
      !expect(speed.transfer_shards == 4, "speed transfer shards") ||
      !expect(speed.transfer_shard_policy == HZ6_TRANSFER_SHARD_OWNER_SLOT,
              "speed transfer owner shard policy") ||
      !expect(speed.route_backend_policy == HZ6_ROUTE_POLICY_PAGE_TABLE,
              "speed page route policy") ||
      !expect(speed.route_page_granularity == HZ6_ROUTE_PAGE_GRANULARITY,
              "speed page route") ||
      !expect(speed.source_kind == HZ6_SOURCE_OS_PAGED,
              "speed source kind") ||
      !expect(rss.scavenge_local_free_bytes > speed.scavenge_local_free_bytes,
              "rss stronger local scavenge") ||
      !expect(rss.scavenge_orphan_bytes > speed.scavenge_orphan_bytes,
              "rss stronger orphan scavenge") ||
      !expect(rss.route_backend_policy == HZ6_ROUTE_POLICY_EXACT_TABLE,
              "rss exact route policy") ||
      !expect(rss.route_page_granularity == 0, "rss exact route") ||
      !expect(strict.strict_owner_remote == 1, "strict owner remote") ||
      !expect(strict.scavenge_local_free_bytes == 0,
              "strict no profile scavenge") ||
      !expect(strict.route_backend_policy == HZ6_ROUTE_POLICY_EXACT_TABLE,
              "strict exact route policy") ||
      !expect(strict.route_page_granularity == 0, "strict exact route") ||
      !expect(remote.route_backend_policy == HZ6_ROUTE_POLICY_PAGE_TABLE,
              "remote page route policy") ||
      !expect(remote.route_page_granularity == HZ6_ROUTE_PAGE_GRANULARITY,
              "remote page route") ||
      !expect(remote.source_kind == HZ6_SOURCE_OS_PAGED,
              "remote source kind")) {
    return 1;
  }
  Hz6ProfileConfig class_shard = remote;
  class_shard.transfer_shard_policy = HZ6_TRANSFER_SHARD_CLASS_ID;
  if (!expect(hz6_profile_transfer_producer_shard(&remote, 5, 7) == 1,
              "remote producer shard policy") ||
      !expect(hz6_profile_transfer_consumer_shard(&remote, 6, 7) == 2,
              "remote consumer shard policy") ||
      !expect(hz6_profile_transfer_producer_shard(&class_shard, 5, 7) == 3,
              "class producer shard policy") ||
      !expect(hz6_profile_transfer_consumer_shard(&class_shard, 6, 10) == 2,
              "class consumer shard policy") ||
      !expect(hz6_profile_transfer_producer_shard(&rss, 5, 7) == 0,
              "rss producer shard policy single") ||
      !expect(hz6_profile_transfer_consumer_shard(&strict, 6, 7) == 0,
              "strict consumer shard policy single") ||
      !expect(hz6_profile_source_refill_batch(&speed, HZ6_FRONT_LARGE, 8) ==
                  speed.source_batch,
              "speed source refill batch policy") ||
      !expect(hz6_profile_source_refill_batch(&rss, HZ6_FRONT_LOCAL2P, 6) ==
                  rss.source_batch,
              "rss source refill batch policy") ||
      !expect(hz6_profile_scavenge_local_free_budget(&rss) ==
                  rss.scavenge_local_free_bytes,
              "rss local free scavenge budget policy") ||
      !expect(hz6_profile_scavenge_orphan_budget(&rss) ==
                  rss.scavenge_orphan_bytes,
              "rss orphan scavenge budget policy") ||
      !expect(hz6_profile_scavenge_local_free_budget(&strict) == 0,
              "strict local free scavenge budget policy") ||
      !expect(hz6_profile_scavenge_orphan_budget(&strict) == 0,
              "strict orphan scavenge budget policy")) {
    return 1;
  }

  Hz6SizeClass class_zero = hz6_size_class_for_request(0);
  Hz6SizeClass class_mid = hz6_size_class_for_request(48);
  Hz6SizeClass class_large = hz6_size_class_for_request(2000);
  if (!expect(class_zero.id == 0 && class_zero.bytes == 16,
              "size class zero") ||
      !expect(class_mid.id == 1 && class_mid.bytes == 64,
              "size class mid") ||
      !expect(class_large.id == 4 && class_large.bytes == 4096,
              "size class large") ||
      !expect(hz6_size_class_valid(class_large), "size class valid")) {
    return 1;
  }

  Hz6FrontCacheEntry front_entries[1];
  Hz6FrontCacheBin front_bin;
  hz6_frontcache_bin_init(&front_bin, front_entries, 1);
  Hz6FrontCacheEntry front_entry;
  front_entry.ptr = base;
  front_entry.descriptor = &descriptor;
  front_entry.class_id = 7;
  front_entry.generation = 11;
  Hz6FrontCacheEntry front_popped;
  if (!expect(hz6_frontcache_push(&front_bin, front_entry),
              "frontcache push") ||
      !expect(!hz6_frontcache_push(&front_bin, front_entry),
              "frontcache bounded") ||
      !expect(hz6_frontcache_remove(&front_bin, base, &descriptor, 11, NULL),
              "frontcache remove") ||
      !expect(!hz6_frontcache_pop(&front_bin, &front_popped),
              "frontcache empty after remove") ||
      !expect(hz6_frontcache_push(&front_bin, front_entry),
              "frontcache push after remove") ||
      !expect(hz6_frontcache_pop(&front_bin, &front_popped),
              "frontcache pop") ||
      !expect(front_popped.ptr == base, "frontcache pointer")) {
    return 1;
  }

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

  printf("hz6-r1-contract-smoke ok\n");
  return 0;
}
