#ifndef HZ5_CONTRACT_H
#define HZ5_CONTRACT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz5AllocatorDescriptorV1 {
  uint32_t magic;
  uint16_t abi_version;
  uint16_t struct_size;
  uint32_t lane_kind;
  uint32_t reserved0;
  uint64_t feature_mask;
  uint64_t forbidden_mask;
  char allocator_name[32];
  char lane_name[64];
  char source_commit[48];
  char contract_hash[32];
  char build_id[64];
} Hz5AllocatorDescriptorV1;

const Hz5AllocatorDescriptorV1* hz5_contract_descriptor_v1(void);

#ifdef __cplusplus
}
#endif

#endif
