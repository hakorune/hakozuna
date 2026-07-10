#ifndef HZ12_SNAPSHOT_RECYCLE_H
#define HZ12_SNAPSHOT_RECYCLE_H

#include <stdint.h>

typedef struct H12SnapshotRecycleResult {
  void* span_base;
  uint32_t before_state;
  uint32_t after_state;
  uint8_t class_id;
  uint8_t recommitted;
  uint8_t route_attached;
  uint8_t owner_assigned;
  uint8_t current_installed;
  uint8_t rollback;
} H12SnapshotRecycleResult;

int h12_snapshot_recycle_take(uint8_t class_id,
                              H12SnapshotRecycleResult* out);

#endif /* HZ12_SNAPSHOT_RECYCLE_H */
