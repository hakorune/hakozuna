#include "hz5.h"

#include "hz5_contract.h"
#include "hz5_policy.h"
#include "hz5_wrapper.h"

#include <stddef.h>

void* hz5_malloc(size_t size) {
  return hz5_aligned_alloc(size, 16u);
}

void* hz5_aligned_alloc(size_t size, size_t align) {
  static const Hz5PolicyHooks hooks = {0};
  return hz5_policy_alloc_aligned(size, align, &hooks);
}

Hz5FreeResult hz5_free(void* ptr) {
  static const Hz5PolicyHooks hooks = {0};
  if (!ptr) {
    return HZ5_FREE_OK_HZ5;
  }
  hz5_policy_free(ptr, &hooks);
  return HZ5_FREE_OK_HZ5;
}

int hz5_owns(void* ptr) {
  Hz5WrapperHdr* wrapped = NULL;
  if (hz5_wrapper_decode(ptr, &wrapped)) {
    return 1;
  }
  return hz5_p1_owns(ptr);
}

const void* hz5_allocator_descriptor_v1(void) {
  return hz5_contract_descriptor_v1();
}
