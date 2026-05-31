#include "hz6_allocator.h"

int hz6_allocator_release_orphan(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind != HZ6_ROUTE_VALID || !route.descriptor) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->ptr != ptr || descriptor->state != HZ6_STATE_ORPHAN) {
    return 0;
  }

  hz6_allocator_route_unregister_exact(allocator, ptr);
#if HZ6_DIAGNOSTIC_PROBES
  if (descriptor->source_release) {
    ++allocator->stats.source_owned_release;
  }
#endif
  return hz6_allocator_release_descriptor_source(descriptor);
}
