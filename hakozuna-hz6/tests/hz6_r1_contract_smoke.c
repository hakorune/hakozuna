#include "../api/hz6_allocator.h"
#include "../frontcache/hz6_frontcache.h"
#include "../frontcache/hz6_size_class.h"
#include "../fronts/large/hz6_large128_front.h"
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

  void* allocated = hz6_malloc(&allocator, 48);
  if (!expect(allocated != NULL, "allocator malloc") ||
      !expect(hz6_owns(&allocator, allocated), "allocator owns exact") ||
      !expect(hz6_owns(&allocator, (unsigned char*)allocated + 1),
              "allocator owns invalid interior")) {
    return 1;
  }
  hz6_free(&allocator, (unsigned char*)allocated + 1);
  hz6_free(&allocator, allocated);
  hz6_free(&allocator, allocated);
  hz6_free(&allocator, &foreign);
  void* reused = hz6_malloc(&allocator, 48);
  if (!expect(reused == allocated, "allocator frontcache reuse")) {
    return 1;
  }
  hz6_free(&allocator, reused);
  Hz6StatsSnapshot stats = hz6_stats_snapshot(&allocator);
  if (!expect(stats.route_valid == 3, "stats valid") ||
      !expect(stats.route_invalid == 2, "stats invalid") ||
      !expect(stats.route_miss == 1, "stats miss") ||
      !expect(stats.source_alloc == 1, "stats source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&allocator);

  Hz6Allocator large_allocator;
  hz6_allocator_init_with_profile(&large_allocator, HZ6_PROFILE_REMOTE);
  void* large_object = hz6_malloc(&large_allocator, 65536);
  if (!expect(large_object != NULL, "large128 malloc")) {
    return 1;
  }
  Hz6RouteResult large_route =
      hz6_route_lookup(&large_allocator.route_table, large_object);
  if (!expect(large_route.kind == HZ6_ROUTE_VALID, "large128 route valid") ||
      !expect(large_route.front_id == HZ6_FRONT_LARGE,
              "large128 route front") ||
      !expect(large_route.class_id == HZ6_LARGE128_CLASS_ID,
              "large128 route class") ||
      !expect(hz6_free_remote(&large_allocator, large_object),
              "large128 remote free")) {
    return 1;
  }
  void* large_reused = hz6_malloc(&large_allocator, 65536);
  if (!expect(large_reused == large_object, "large128 transfer reuse")) {
    return 1;
  }
  hz6_free(&large_allocator, large_reused);
  Hz6StatsSnapshot large_stats = hz6_stats_snapshot(&large_allocator);
  if (!expect(large_stats.transfer_push == 1, "large128 transfer push") ||
      !expect(large_stats.transfer_pop == 1, "large128 transfer pop") ||
      !expect(large_stats.source_alloc == 1, "large128 source alloc")) {
    return 1;
  }
  if (!expect(hz6_malloc(&large_allocator, HZ6_LARGE128_BYTES + 1) == NULL,
              "unsupported over-large allocation")) {
    return 1;
  }
  hz6_allocator_destroy(&large_allocator);

  Hz6Allocator remote_allocator;
  hz6_allocator_init_with_profile(&remote_allocator, HZ6_PROFILE_REMOTE);
  void* remote_object = hz6_malloc(&remote_allocator, 128);
  if (!expect(remote_object != NULL, "remote allocator malloc") ||
      !expect(hz6_free_remote(&remote_allocator, remote_object),
              "remote transfer free") ||
      !expect(!hz6_free_remote(&remote_allocator, remote_object),
              "remote double-free rejected")) {
    return 1;
  }
  void* transfer_reused = hz6_malloc(&remote_allocator, 128);
  if (!expect(transfer_reused == remote_object, "transfer reuse")) {
    return 1;
  }
  hz6_free(&remote_allocator, transfer_reused);
  Hz6StatsSnapshot remote_stats = hz6_stats_snapshot(&remote_allocator);
  if (!expect(remote_stats.route_valid == 3, "remote stats valid") ||
      !expect(remote_stats.route_invalid == 1, "remote stats invalid") ||
      !expect(remote_stats.transfer_push == 1, "remote stats push") ||
      !expect(remote_stats.transfer_pop == 1, "remote stats pop") ||
      !expect(remote_stats.source_alloc == 1, "remote stats source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&remote_allocator);

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
