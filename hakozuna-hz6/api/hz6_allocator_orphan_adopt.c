#include "hz6_allocator.h"

int hz6_allocator_orphan_adopt_prepare(Hz6Allocator* adopter,
                                       Hz6Allocator* source,
                                       void* ptr,
                                       Hz6RouteResult* source_route,
                                       Hz6ObjectDescriptor** source_descriptor,
                                       Hz6ObjectDescriptor**
                                           adopted_descriptor);

int hz6_allocator_orphan_adopt_commit(Hz6Allocator* adopter,
                                      Hz6Allocator* source,
                                      void* ptr,
                                      const Hz6RouteResult* source_route,
                                      Hz6ObjectDescriptor* source_descriptor,
                                      Hz6ObjectDescriptor* adopted_descriptor);

int hz6_allocator_adopt_orphan(Hz6Allocator* adopter,
                               Hz6Allocator* source,
                               void* ptr) {
  Hz6RouteResult source_route;
  Hz6ObjectDescriptor* source_descriptor = NULL;
  Hz6ObjectDescriptor* adopted_descriptor = NULL;

  if (!hz6_allocator_orphan_adopt_prepare(adopter, source, ptr, &source_route,
                                          &source_descriptor,
                                          &adopted_descriptor)) {
    return 0;
  }

  if (!hz6_allocator_orphan_adopt_commit(adopter, source, ptr, &source_route,
                                         source_descriptor,
                                         adopted_descriptor)) {
    return 0;
  }

  return 1;
}
