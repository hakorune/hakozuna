#include "hz6_transfer_backend.h"

int hz6_transfer_backend_push_to_shard(Hz6TransferBackend* backend,
                                       Hz6TransferObject object,
                                       size_t producer_shard) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_push(&backend->single_cache, object);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      backend->shard_count == 0) {
    return 0;
  }

  size_t start = producer_shard % backend->shard_count;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    size_t shard_index = (start + i) % backend->shard_count;
    if (hz6_transfer_push(&backend->shard[shard_index], object)) {
      backend->next_push_shard = (shard_index + 1u) % backend->shard_count;
      return 1;
    }
  }
  return 0;
}

int hz6_transfer_backend_reserve_to_shard(
    Hz6TransferBackend* backend,
    size_t producer_shard,
    Hz6TransferReservation* out) {
  if (!backend || !out) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_reserve(&backend->single_cache, out);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      backend->shard_count == 0) {
    return 0;
  }

  size_t start = producer_shard % backend->shard_count;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    size_t shard_index = (start + i) % backend->shard_count;
    if (hz6_transfer_reserve(&backend->shard[shard_index], out)) {
      backend->next_push_shard = (shard_index + 1u) % backend->shard_count;
      return 1;
    }
  }
  return 0;
}

void hz6_transfer_backend_cancel(Hz6TransferReservation* reservation) {
  hz6_transfer_cancel(reservation);
}

void hz6_transfer_backend_commit(Hz6TransferReservation* reservation,
                                 Hz6TransferObject object) {
  hz6_transfer_commit(reservation, object);
}
