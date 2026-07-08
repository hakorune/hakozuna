#ifndef HZ11_LIVE_FOOTPRINT_H
#define HZ11_LIVE_FOOTPRINT_H

#include <stddef.h>
#include <stdint.h>

#ifndef HZ11_LIVE_FOOTPRINT_DIAG
#define HZ11_LIVE_FOOTPRINT_DIAG 0
#endif

#if HZ11_LIVE_FOOTPRINT_DIAG
void hz11_live_footprint_alloc(uint8_t class_id, void* ptr, size_t size);
void hz11_live_footprint_free(uint8_t class_id, void* ptr);
void hz11_live_footprint_dump(void);
#else
static inline void hz11_live_footprint_alloc(uint8_t class_id, void* ptr,
                                             size_t size) {
  (void)class_id;
  (void)ptr;
  (void)size;
}

static inline void hz11_live_footprint_free(uint8_t class_id, void* ptr) {
  (void)class_id;
  (void)ptr;
}

static inline void hz11_live_footprint_dump(void) {}
#endif

#endif /* HZ11_LIVE_FOOTPRINT_H */
