#ifndef HZ11_SIZE_CLASS_H
#define HZ11_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

/* HZ11 size classes: power-of-two slot sizes 16..65536 (13 classes), looked up
 * by a 16B-quantum table (NOT the HZ10 fine-class branch ladder). Objects larger
 * than 64KiB are LARGE and bypass the cache (system allocator). */

#define HZ11_MIN_SIZE 16u
#define HZ11_MAX_CACHED_SIZE 65536u           /* 64 KiB */
#define HZ11_CLASS_COUNT 13u                  /* slot_size = HZ11_MIN_SIZE << class_id */
#define HZ11_LARGE_CLASS 255u                 /* sentinel: bypass cache */
#define HZ11_QUANTUM_SHIFT 4u                 /* 16-byte quanta */
#define HZ11_SIZE_TABLE_ENTRIES \
  (HZ11_MAX_CACHED_SIZE >> HZ11_QUANTUM_SHIFT) /* 4096 */

extern uint8_t hz11_size_table[HZ11_SIZE_TABLE_ENTRIES];
extern volatile int hz11_size_table_ready;

void hz11_size_class_init(void);

/* Hot path: inlined so malloc does not pay a call per classification. */
static inline uint8_t hz11_size_class(size_t size) {
  if (size == 0u) {
    size = HZ11_MIN_SIZE;
  }
  if (size > HZ11_MAX_CACHED_SIZE) {
    return HZ11_LARGE_CLASS;
  }
  if (!hz11_size_table_ready) {
    hz11_size_class_init();
  }
  return hz11_size_table[(size - 1u) >> HZ11_QUANTUM_SHIFT];
}

static inline size_t hz11_class_slot_size(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return 0u;
  }
  return (size_t)HZ11_MIN_SIZE << class_id;
}

#endif /* HZ11_SIZE_CLASS_H */
