#ifndef HZ12_LIVE_FOOTPRINT_H
#define HZ12_LIVE_FOOTPRINT_H

#include <stddef.h>
#include <stdint.h>

#ifndef HZ12_LIVE_FOOTPRINT_DIAG
#define HZ12_LIVE_FOOTPRINT_DIAG 0
#endif

#if HZ12_LIVE_FOOTPRINT_DIAG
void hz12_live_footprint_alloc(uint8_t class_id, void* ptr, size_t size);
void hz12_live_footprint_free(uint8_t class_id, void* ptr);
void hz12_live_footprint_dump(void);
#else
static inline void hz12_live_footprint_alloc(uint8_t class_id, void* ptr,
                                             size_t size) {
  (void)class_id;
  (void)ptr;
  (void)size;
}

static inline void hz12_live_footprint_free(uint8_t class_id, void* ptr) {
  (void)class_id;
  (void)ptr;
}

static inline void hz12_live_footprint_dump(void) {}
#endif

#endif /* HZ12_LIVE_FOOTPRINT_H */
