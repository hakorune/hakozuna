#ifndef HZ6_TRANSFER_H
#define HZ6_TRANSFER_H

#include "../include/hz6_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6TransferObject {
  void* ptr;
  void* descriptor;
  uint16_t class_id;
  uint32_t generation;
} Hz6TransferObject;

typedef struct Hz6TransferCache {
  Hz6TransferObject* objects;
  size_t capacity;
  size_t count;
} Hz6TransferCache;

void hz6_transfer_init(Hz6TransferCache* cache,
                       Hz6TransferObject* objects,
                       size_t capacity);

int hz6_transfer_push(Hz6TransferCache* cache, Hz6TransferObject object);

int hz6_transfer_pop(Hz6TransferCache* cache,
                     uint16_t class_id,
                     Hz6TransferObject* out);

size_t hz6_transfer_count(const Hz6TransferCache* cache);

#ifdef __cplusplus
}
#endif

#endif

