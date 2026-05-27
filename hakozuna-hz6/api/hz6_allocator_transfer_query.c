#include "hz6_allocator.h"

Hz6TransferBackendKind hz6_allocator_transfer_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_TRANSFER_BACKEND_SINGLE_CACHE;
  }
  return allocator->transfer_backend.kind;
}

size_t hz6_allocator_transfer_capacity(const Hz6Allocator* allocator) {
  return allocator
             ? hz6_transfer_backend_capacity(&allocator->transfer_backend)
             : 0;
}

size_t hz6_allocator_transfer_count(const Hz6Allocator* allocator) {
  return allocator ? hz6_transfer_backend_count(&allocator->transfer_backend)
                   : 0;
}

size_t hz6_allocator_transfer_count_class(const Hz6Allocator* allocator,
                                          uint16_t class_id) {
  return allocator
             ? hz6_transfer_backend_count_class(&allocator->transfer_backend,
                                                class_id)
             : 0;
}

size_t hz6_allocator_transfer_shard_count_at(const Hz6Allocator* allocator,
                                             size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_count_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}

size_t hz6_allocator_transfer_shard_capacity_at(
    const Hz6Allocator* allocator,
    size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_capacity_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}
