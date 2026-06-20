#ifndef HZ6_TRANSFER_AUDIT_TAG_H
#define HZ6_TRANSFER_AUDIT_TAG_H

#include <stdint.h>

#include "../include/hz6_config.h"
#include "../owner/hz6_owner.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6TransferPublishKind {
  HZ6_TRANSFER_PUBLISH_UNKNOWN = 0,
  HZ6_TRANSFER_PUBLISH_DESTINATION = 1,
  HZ6_TRANSFER_PUBLISH_ORIGIN_FALLBACK = 2
} Hz6TransferPublishKind;

#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
typedef struct Hz6TransferAuditTag {
  uint32_t publish_epoch;
  uint32_t class_demand_epoch_at_publish;
  Hz6OwnerToken producer_token;
  uint8_t publish_kind;
  uint8_t stamped;
  uint8_t accounted;
} Hz6TransferAuditTag;
#endif

#ifdef __cplusplus
}
#endif

#endif
