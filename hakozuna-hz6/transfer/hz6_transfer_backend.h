#ifndef HZ6_TRANSFER_BACKEND_H
#define HZ6_TRANSFER_BACKEND_H

#include "hz6_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6TransferBackendKind {
  HZ6_TRANSFER_BACKEND_SINGLE_CACHE = 1
} Hz6TransferBackendKind;

typedef struct Hz6TransferBackend {
  Hz6TransferBackendKind kind;
  Hz6TransferCache single_cache;
} Hz6TransferBackend;

void hz6_transfer_backend_init_single(Hz6TransferBackend* backend,
                                      Hz6TransferObject* objects,
                                      size_t capacity);

int hz6_transfer_backend_push(Hz6TransferBackend* backend,
                              Hz6TransferObject object);

int hz6_transfer_backend_pop(Hz6TransferBackend* backend,
                             uint16_t class_id,
                             Hz6TransferObject* out);

size_t hz6_transfer_backend_count(const Hz6TransferBackend* backend);

#ifdef __cplusplus
}
#endif

#endif
