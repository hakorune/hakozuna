#include "hz6_allocator_init_internal.h"
#include "hz6_allocator_route_domain.h"

void hz6_allocator_init_backends(Hz6Allocator* allocator) {
  hz6_allocator_route_domain_init(allocator);
  switch (allocator->profile.route_backend_policy) {
    case HZ6_ROUTE_POLICY_PAGE_TABLE:
      hz6_route_backend_init_page_table_with_granularity(
          &allocator->route_backend, allocator->route_entries,
          HZ6_ROUTE_TABLE_CAPACITY, allocator->profile.route_page_granularity);
      break;
    case HZ6_ROUTE_POLICY_EXACT_TABLE:
    default:
      hz6_route_backend_init_exact(&allocator->route_backend,
                                   allocator->route_entries,
                                   HZ6_ROUTE_TABLE_CAPACITY);
      break;
  }
#if HZ6_ROUTE_PACKED_META_L1
  hz6_route_table_attach_bytes(&allocator->route_backend.exact_table,
                               allocator->route_bytes);
#endif

  size_t transfer_capacity = allocator->profile.transfer_capacity;
  if (transfer_capacity == 0 ||
      transfer_capacity > HZ6_TRANSFER_CACHE_CAPACITY) {
    transfer_capacity = HZ6_TRANSFER_CACHE_CAPACITY;
  }
  if (allocator->profile.transfer_shards > 1) {
    hz6_transfer_backend_init_sharded(&allocator->transfer_backend,
                                      allocator->transfer_objects,
                                      transfer_capacity,
                                      allocator->profile.transfer_shards);
  } else {
    hz6_transfer_backend_init_single(&allocator->transfer_backend,
                                     allocator->transfer_objects,
                                     transfer_capacity);
  }
}
