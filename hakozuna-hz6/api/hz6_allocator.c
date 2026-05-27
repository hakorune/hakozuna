#include "hz6_allocator.h"

static Hz6OwnerToken hz6_owner_token_none(void) {
  Hz6OwnerToken token;
  token.slot = 0;
  token.generation = 0;
  return token;
}

Hz6OwnerToken hz6_allocator_owner_token(const Hz6Allocator* allocator) {
  return allocator ? allocator->owner.token : hz6_owner_token_none();
}

int hz6_allocator_debug_set_owner_slot(Hz6Allocator* allocator,
                                       uint32_t slot) {
  if (!allocator) {
    return 0;
  }
  allocator->owner.token.slot = slot;
  return 1;
}
