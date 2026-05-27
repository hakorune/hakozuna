#include "../api/hz6_allocator.h"
#include "../frontcache/hz6_frontcache.h"
#include "../include/hz6_contract.h"
#include "../owner/hz6_owner.h"
#include "../policy/hz6_profiles.h"
#include "../route/hz6_route.h"
#include "../source/hz6_source.h"
#include "../transfer/hz6_transfer.h"

#include <stdio.h>
#include <stdlib.h>

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
      !expect(!hz6_transfer_push(&transfer, object_a), "transfer bounded")) {
    return 1;
  }

  Hz6TransferObject popped;
  if (!expect(hz6_transfer_pop(&transfer, 7, &popped), "transfer pop class") ||
      !expect(popped.ptr == base, "transfer pop pointer") ||
      !expect(hz6_transfer_count(&transfer) == 1, "transfer count")) {
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
  Hz6ProfileConfig strict = hz6_profile_config(HZ6_PROFILE_STRICT);
  if (!expect(speed.transfer_first == 1, "speed transfer-first") ||
      !expect(strict.strict_owner_remote == 1, "strict owner remote")) {
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
      !expect(hz6_frontcache_pop(&front_bin, &front_popped),
              "frontcache pop") ||
      !expect(front_popped.ptr == base, "frontcache pointer")) {
    return 1;
  }

  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_SPEED);
  if (!expect(allocator.profile.transfer_first == 1, "allocator profile")) {
    return 1;
  }
  if (!expect(hz6_route_register_exact(&allocator.route_table, base,
                                       sizeof(object), HZ6_FRONT_LOCAL2P, 7,
                                       12, &descriptor),
              "allocator route register")) {
    return 1;
  }
  if (!expect(hz6_owns(&allocator, base), "allocator owns exact") ||
      !expect(hz6_owns(&allocator, object + 1),
              "allocator owns invalid interior")) {
    return 1;
  }
  hz6_free(&allocator, base);
  hz6_free(&allocator, object + 1);
  hz6_free(&allocator, &foreign);
  Hz6StatsSnapshot stats = hz6_stats_snapshot(&allocator);
  if (!expect(stats.route_valid == 1, "stats valid") ||
      !expect(stats.route_invalid == 1, "stats invalid") ||
      !expect(stats.route_miss == 1, "stats miss")) {
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

  printf("hz6-r1-contract-smoke ok\n");
  return 0;
}
