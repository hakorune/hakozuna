#include "hz6_allocator.h"

Hz6ProfileId hz6_allocator_profile_id(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.id : HZ6_PROFILE_STRICT;
}

int hz6_allocator_profile_transfer_first(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_first != 0 : 0;
}

int hz6_allocator_profile_strict_owner_remote(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.strict_owner_remote != 0 : 0;
}

Hz6SourceKind hz6_allocator_profile_source_kind(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.source_kind : HZ6_SOURCE_NONE;
}

size_t hz6_allocator_profile_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  return allocator ? hz6_profile_source_refill_batch(&allocator->profile,
                                                     front_id, class_id)
                   : 0;
}

size_t hz6_allocator_profile_transfer_capacity(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_capacity : 0;
}
