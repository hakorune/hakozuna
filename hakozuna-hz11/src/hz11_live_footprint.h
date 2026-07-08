#ifndef HZ11_LIVE_FOOTPRINT_H
#define HZ11_LIVE_FOOTPRINT_H

#include <stdint.h>

#ifndef HZ11_LIVE_FOOTPRINT_DIAG
#define HZ11_LIVE_FOOTPRINT_DIAG 0
#endif

#if HZ11_LIVE_FOOTPRINT_DIAG
void hz11_live_footprint_alloc(uint8_t class_id);
void hz11_live_footprint_free(uint8_t class_id);
void hz11_live_footprint_dump(void);
#else
static inline void hz11_live_footprint_alloc(uint8_t class_id) {
  (void)class_id;
}

static inline void hz11_live_footprint_free(uint8_t class_id) {
  (void)class_id;
}

static inline void hz11_live_footprint_dump(void) {}
#endif

#endif /* HZ11_LIVE_FOOTPRINT_H */
