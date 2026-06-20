#ifndef HZ6_TRANSFER_H
#define HZ6_TRANSFER_H

#include "../include/hz6_contract.h"
#include "hz6_transfer_audit_tag.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6StatsSnapshot Hz6StatsSnapshot;

#ifndef HZ6_TRANSFER_CLASS_PRESENCE_ARMED_L1
#define HZ6_TRANSFER_CLASS_PRESENCE_ARMED_L1 0
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
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
  _Atomic uint32_t class_count[HZ6_FRONT_CACHE_CLASS_COUNT];
#if HZ6_TRANSFER_CLASS_PRESENCE_ARMED_L1
  uint8_t presence_armed;
#endif
#if HZ6_TRANSFER_CLASS_PRESENCE_OBSERVE_L1
  _Atomic uint32_t presence_gate_check;
  _Atomic uint32_t presence_gate_hit;
  _Atomic uint32_t presence_gate_miss;
  _Atomic uint32_t presence_below_min_check;
  _Atomic uint32_t presence_below_min_hit;
  _Atomic uint32_t presence_below_min_miss;
  _Atomic uint32_t presence_update_below_min_increment;
  _Atomic uint32_t presence_update_below_min_decrement;
  _Atomic uint32_t presence_update_below_min_commit;
  _Atomic uint32_t presence_arm_rebuild;
  _Atomic uint32_t presence_arm_disarm;
  _Atomic uint32_t presence_unarmed_probe;
  _Atomic uint32_t presence_invalid_class;
  _Atomic uint32_t presence_underflow;
  _Atomic uint32_t presence_over_capacity;
  _Atomic uint32_t presence_placeholder_counted;
  _Atomic uint32_t presence_double_increment;
  _Atomic uint32_t presence_double_decrement;
  _Atomic uint32_t presence_sum_mismatch;
  _Atomic uint32_t presence_scan_mismatch;
  _Atomic uint32_t presence_false_zero_shadow;
#endif
#endif
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

int hz6_transfer_class_maybe_present(Hz6TransferCache* cache,
                                     uint16_t class_id);

void hz6_transfer_note_class_presence_stats(
    const Hz6TransferCache* cache,
    Hz6StatsSnapshot* snapshot);

#ifdef __cplusplus
}
#endif

#endif
