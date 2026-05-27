#ifndef HZ6_TRANSFER_BACKEND_H
#define HZ6_TRANSFER_BACKEND_H

#include "../include/hz6_config.h"
#include "hz6_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6TransferBackendKind {
  HZ6_TRANSFER_BACKEND_SINGLE_CACHE = 1,
  HZ6_TRANSFER_BACKEND_SHARDED_CACHE = 2
} Hz6TransferBackendKind;

typedef struct Hz6TransferBackend {
  Hz6TransferBackendKind kind;
  Hz6TransferCache single_cache;
  Hz6TransferCache shard[HZ6_TRANSFER_SHARD_COUNT];
  size_t shard_count;
  size_t next_push_shard;
} Hz6TransferBackend;

void hz6_transfer_backend_init_single(Hz6TransferBackend* backend,
                                      Hz6TransferObject* objects,
                                      size_t capacity);

void hz6_transfer_backend_init_sharded(Hz6TransferBackend* backend,
                                       Hz6TransferObject* objects,
                                       size_t capacity,
                                       size_t shard_count);

int hz6_transfer_backend_push(Hz6TransferBackend* backend,
                              Hz6TransferObject object);

int hz6_transfer_backend_pop(Hz6TransferBackend* backend,
                             uint16_t class_id,
                             Hz6TransferObject* out);

size_t hz6_transfer_backend_count(const Hz6TransferBackend* backend);

size_t hz6_transfer_backend_capacity(const Hz6TransferBackend* backend);

size_t hz6_transfer_backend_count_class(const Hz6TransferBackend* backend,
                                        uint16_t class_id);

size_t hz6_transfer_backend_shard_count_at(const Hz6TransferBackend* backend,
                                           size_t shard_index);

size_t hz6_transfer_backend_shard_capacity_at(
    const Hz6TransferBackend* backend,
    size_t shard_index);

#ifdef __cplusplus
}
#endif

#endif
