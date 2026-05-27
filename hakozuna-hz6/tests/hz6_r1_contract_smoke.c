#include "../frontcache/hz6_frontcache.h"
#include "../frontcache/hz6_size_class.h"
#include "../include/hz6_contract.h"
#include "../owner/hz6_owner.h"
#include "../policy/hz6_profiles.h"
#include "../route/hz6_route.h"
#include "../route/hz6_route_backend.h"
#include "../scavenge/hz6_scavenge.h"
#include "../source/linux_source_mmap.h"
#include "../source/hz6_source.h"
#include "../source/hz6_source_registry.h"
#include "../transfer/hz6_transfer.h"
#include "../transfer/hz6_transfer_backend.h"

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
  Hz6RouteEntry route_entries[4];
  Hz6RouteTable route_table;
  hz6_route_table_init(&route_table, route_entries, 4);

  SmokeDescriptor descriptor;
  descriptor.marker = 42;

  unsigned char object[256];
  void* base = (void*)object;

  if (!expect(hz6_route_register_exact(&route_table, base, sizeof(object),
                                       HZ6_FRONT_LOCAL2P, 7, 11,
                                       &descriptor),
              "route register")) {
    return 1;
  }

  Hz6RouteResult exact = hz6_route_lookup(&route_table, base);
  if (!expect(exact.kind == HZ6_ROUTE_VALID, "exact pointer valid") ||
      !expect(exact.front_id == HZ6_FRONT_LOCAL2P, "front id") ||
      !expect(exact.class_id == 7, "class id") ||
      !expect(exact.generation == 11, "generation") ||
      !expect(exact.descriptor == &descriptor, "descriptor")) {
    return 1;
  }

  Hz6RouteResult interior = hz6_route_lookup(&route_table, object + 16);
  if (!expect(interior.kind == HZ6_ROUTE_INVALID, "interior pointer invalid")) {
    return 1;
  }

  int foreign = 0;
  Hz6RouteResult miss = hz6_route_lookup(&route_table, &foreign);
  if (!expect(miss.kind == HZ6_ROUTE_MISS, "foreign pointer miss")) {
    return 1;
  }

  Hz6RouteEntry backend_entries[2];
  Hz6RouteBackend route_backend;
  hz6_route_backend_init_exact(&route_backend, backend_entries, 2);
  if (!expect(hz6_route_backend_register_exact(
                  &route_backend, base, sizeof(object), HZ6_FRONT_LOCAL2P, 7,
                  12, &descriptor),
              "route backend register")) {
    return 1;
  }
  Hz6RouteResult backend_exact = hz6_route_backend_lookup(&route_backend, base);
  Hz6RouteResult backend_interior =
      hz6_route_backend_lookup(&route_backend, object + 8);
  if (!expect(backend_exact.kind == HZ6_ROUTE_VALID,
              "route backend valid") ||
      !expect(backend_exact.generation == 12, "route backend generation") ||
      !expect(backend_interior.kind == HZ6_ROUTE_INVALID,
              "route backend invalid")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&route_backend, base);
  if (!expect(hz6_route_backend_lookup(&route_backend, base).kind ==
                  HZ6_ROUTE_MISS,
              "route backend unregister")) {
    return 1;
  }

  Hz6TransferObject transfer_objects[2];
  Hz6TransferCache transfer;
  hz6_transfer_init(&transfer, transfer_objects, 2);

  Hz6TransferObject object_a;
  object_a.ptr = base;
  object_a.descriptor = &descriptor;
  object_a.class_id = 7;
  object_a.generation = 11;

  Hz6TransferObject object_b = object_a;
  object_b.ptr = object + 128;
  object_b.class_id = 8;

  if (!expect(hz6_transfer_push(&transfer, object_a), "transfer push a") ||
      !expect(hz6_transfer_push(&transfer, object_b), "transfer push b") ||
      !expect(hz6_transfer_count_class(&transfer, 7) == 1,
              "transfer class count a") ||
      !expect(hz6_transfer_count_class(&transfer, 8) == 1,
              "transfer class count b") ||
      !expect(!hz6_transfer_push(&transfer, object_a), "transfer bounded")) {
    return 1;
  }

  Hz6TransferObject popped;
  if (!expect(hz6_transfer_pop(&transfer, 7, &popped), "transfer pop class") ||
      !expect(popped.ptr == base, "transfer pop pointer") ||
      !expect(hz6_transfer_count(&transfer) == 1, "transfer count")) {
    return 1;
  }

  Hz6TransferObject backend_objects[2];
  Hz6TransferBackend transfer_backend;
  hz6_transfer_backend_init_single(&transfer_backend, backend_objects, 2);
  if (!expect(hz6_transfer_backend_push(&transfer_backend, object_a),
              "transfer backend push a") ||
      !expect(hz6_transfer_backend_push(&transfer_backend, object_b),
              "transfer backend push b") ||
      !expect(hz6_transfer_backend_capacity(&transfer_backend) == 2,
              "transfer backend capacity") ||
      !expect(hz6_transfer_backend_count_class(&transfer_backend, 7) == 1,
              "transfer backend class count a") ||
      !expect(!hz6_transfer_backend_push(&transfer_backend, object_a),
              "transfer backend bounded")) {
    return 1;
  }
  Hz6TransferObject backend_popped;
  if (!expect(hz6_transfer_backend_pop(&transfer_backend, 8,
                                       &backend_popped),
              "transfer backend pop class") ||
      !expect(backend_popped.ptr == object_b.ptr,
              "transfer backend pop pointer") ||
      !expect(hz6_transfer_backend_count(&transfer_backend) == 1,
              "transfer backend count")) {
    return 1;
  }

  Hz6TransferObject sharded_objects[4];
  Hz6TransferBackend sharded_backend;
  hz6_transfer_backend_init_sharded(&sharded_backend, sharded_objects, 4, 2);
  if (!expect(sharded_backend.kind == HZ6_TRANSFER_BACKEND_SHARDED_CACHE,
              "transfer sharded kind") ||
      !expect(hz6_transfer_backend_push(&sharded_backend, object_a),
              "transfer sharded push a") ||
      !expect(hz6_transfer_backend_push(&sharded_backend, object_b),
              "transfer sharded push b") ||
      !expect(hz6_transfer_backend_capacity(&sharded_backend) == 4,
              "transfer sharded capacity") ||
      !expect(hz6_transfer_backend_shard_count_at(&sharded_backend, 0) == 1,
              "transfer sharded shard zero count") ||
      !expect(hz6_transfer_backend_shard_count_at(&sharded_backend, 1) == 1,
              "transfer sharded shard one count") ||
      !expect(hz6_transfer_backend_count_class(&sharded_backend, 7) == 1,
              "transfer sharded class count") ||
      !expect(hz6_transfer_backend_count(&sharded_backend) == 2,
              "transfer sharded count")) {
    return 1;
  }
  Hz6TransferObject sharded_popped;
  if (!expect(hz6_transfer_backend_pop(&sharded_backend, 7,
                                       &sharded_popped),
              "transfer sharded pop class") ||
      !expect(sharded_popped.ptr == object_a.ptr,
              "transfer sharded pop pointer") ||
      !expect(hz6_transfer_backend_count(&sharded_backend) == 1,
              "transfer sharded count after pop")) {
    return 1;
  }

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
  if (!expect(speed.transfer_first == 1, "speed transfer-first") ||
      !expect(speed.transfer_shards == 4, "speed transfer shards") ||
      !expect(rss.scavenge_local_free_bytes > speed.scavenge_local_free_bytes,
              "rss stronger local scavenge") ||
      !expect(rss.scavenge_orphan_bytes > speed.scavenge_orphan_bytes,
              "rss stronger orphan scavenge") ||
      !expect(strict.strict_owner_remote == 1, "strict owner remote") ||
      !expect(strict.scavenge_local_free_bytes == 0,
              "strict no profile scavenge")) {
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
