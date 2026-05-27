#include "hz6_front_util.h"

int hz6_front_free_local_to_cache(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route,
                                  uint16_t expected_class_id) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != expected_class_id) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  return hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr);
}

int hz6_front_free_remote_to_transfer(Hz6Allocator* allocator,
                                      void* ptr,
                                      Hz6RouteResult route,
                                      uint16_t expected_class_id) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != expected_class_id) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  return hz6_allocator_remote_free_active_descriptor(allocator, descriptor,
                                                    ptr);
}
