#ifndef HZ6_TRANSFER_H
#define HZ6_TRANSFER_H

#include "../include/hz6_contract.h"
#include "hz6_transfer_audit_tag.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6TransferObject {
  void* ptr;
  void* descriptor;
  uint16_t class_id;
  uint32_t generation;
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  Hz6TransferAuditTag audit_tag;
#endif
} Hz6TransferObject;

typedef struct Hz6TransferCache {
  Hz6TransferObject* objects;
  size_t capacity;
  size_t count;
} Hz6TransferCache;

typedef struct Hz6TransferReservation {
  Hz6TransferCache* cache;
  size_t index;
  int reserved;
} Hz6TransferReservation;

void hz6_transfer_init(Hz6TransferCache* cache,
                       Hz6TransferObject* objects,
                       size_t capacity);

int hz6_transfer_push(Hz6TransferCache* cache, Hz6TransferObject object);

int hz6_transfer_reserve(Hz6TransferCache* cache,
                         Hz6TransferReservation* out);

void hz6_transfer_cancel(Hz6TransferReservation* reservation);

void hz6_transfer_commit(Hz6TransferReservation* reservation,
                         Hz6TransferObject object);

int hz6_transfer_pop(Hz6TransferCache* cache,
                     uint16_t class_id,
                     Hz6TransferObject* out);

size_t hz6_transfer_count(const Hz6TransferCache* cache);

size_t hz6_transfer_count_class(const Hz6TransferCache* cache,
                                uint16_t class_id);

#ifdef __cplusplus
}
#endif

#endif
