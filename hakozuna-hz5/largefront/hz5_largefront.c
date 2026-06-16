#include "hz5_largefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"
#include "hz5_midpagefront.h"
#include "hz5_ownerhub.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "hz5_largefront_config.inc"

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LARGEFRONT_L1

#define HZ5_LARGEFRONT_MAGIC UINT64_C(0x485A354C41524731)
#define HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED 0x0001u
#define HZ5_LARGEFRONT_PAGE_SIZE ((size_t)4096)
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
#define HZ5_LARGEFRONT_CLASS_COUNT 8u
#else
#define HZ5_LARGEFRONT_CLASS_COUNT 4u
#endif

#ifndef HZ5_LARGEFRONT_MAP_BITS
#define HZ5_LARGEFRONT_MAP_BITS 22u
#endif

#define HZ5_LARGEFRONT_MAP_CAP ((size_t)1u << HZ5_LARGEFRONT_MAP_BITS)

#ifndef HZ5_LARGEFRONT_REGION_BUCKET_BITS
#define HZ5_LARGEFRONT_REGION_BUCKET_BITS 16u
#endif

#define HZ5_LARGEFRONT_REGION_BUCKET_CAP \
  ((size_t)1u << HZ5_LARGEFRONT_REGION_BUCKET_BITS)

#ifndef HZ5_LARGEFRONT_REGION_GRAN_BITS
#define HZ5_LARGEFRONT_REGION_GRAN_BITS 21u
#endif

#ifndef HZ5_LARGEFRONT_REGION_CAP
#define HZ5_LARGEFRONT_REGION_CAP 65536u
#endif

#ifndef HZ5_LARGEFRONT_REGION_LINK_CAP
#define HZ5_LARGEFRONT_REGION_LINK_CAP 262144u
#endif

#ifndef HZ5_LARGEFRONT_SOURCE_BATCH_COUNT
#define HZ5_LARGEFRONT_SOURCE_BATCH_COUNT 16u
#endif

#include "hz5_largefront_state.inc"

static Hz5LargeTls* hz5_largefront_tls(void) {
  Hz5LargeTls* tls = &g_hz5_largefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  if (tls->remote_batch_cap == 0u) {
    tls->remote_batch_cap = HZ5_LARGEFRONT_REMOTE_BATCH_CAP;
  }
  return tls;
}

static int hz5_largefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_LARGEFRONT_CLASS_COUNT;
}

static uint32_t hz5_largefront_class_bytes(uint32_t class_index) {
  return g_hz5_largefront_classes[class_index];
}

static int hz5_largefront_class_index(size_t size) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
  if (size <= 4096u || size > 1048576u) {
    return -1;
  }
#else
  if (size <= 65536u || size > 1048576u) {
    return -1;
  }
#endif
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (size <= g_hz5_largefront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_largefront_span_stride(uint32_t class_index) {
  return HZ5_LARGEFRONT_PAGE_SIZE +
         (size_t)hz5_largefront_class_bytes(class_index);
}

#include "hz5_largefront_policy.inc"

#include "hz5_largefront_source.inc"

#include "hz5_largefront_lists.inc"

#include "hz5_largefront_transfer128.inc"

static int hz5_largefront_state_cas(Hz5LargeSpan* span,
                                    unsigned char from,
                                    unsigned char to) {
  unsigned char expected = from;
  return atomic_compare_exchange_strong_explicit(&span->state,
                                                 &expected,
                                                 to,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire);
}

static int hz5_largefront_remote_pending_to_local(Hz5LargeSpan* span) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TRUST_REMOTE_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) !=
      (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING) {
    return 0;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
                        memory_order_release);
  return 1;
#else
  return hz5_largefront_state_cas(
      span,
      (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
      (unsigned char)HZ5_LARGESPAN_LOCAL_FREE);
#endif
}

static int hz5_largefront_owner_local_state_transition(Hz5LargeSpan* span,
                                                       unsigned char from,
                                                       unsigned char to) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) != from) {
    return 0;
  }
  atomic_store_explicit(&span->state, to, memory_order_release);
  return 1;
#else
  return hz5_largefront_state_cas(span, from, to);
#endif
}

static int hz5_largefront_activate_local_for_owner(Hz5LargeTls* tls,
                                                   Hz5LargeSpan* span) {
  if (!hz5_largefront_owner_local_state_transition(
          span,
          (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
          (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return 1;
}

static int hz5_largefront_activate_global_for_owner(Hz5LargeTls* tls,
                                                    Hz5LargeSpan* span) {
  if (!hz5_largefront_state_cas(span,
                                (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
                                (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return 1;
}

static void hz5_largefront_mark_orphan(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ORPHAN,
                        memory_order_release);
}

#include "hz5_largefront_remote_drain.inc"
#include "hz5_largefront_alloc_free.inc"
#endif
