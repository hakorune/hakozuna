#ifndef HZ11_SIZE_CLASS_H
#define HZ11_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

/* HZ11 default size classes: power-of-two slot sizes 16..65536 (13 classes),
 * looked up by a 16B-quantum table (NOT the HZ10 fine-class branch ladder).
 * Objects larger than 64KiB are LARGE and bypass the cache (system allocator).
 *
 * HZ11_FINE_SIZE_CLASSES is an opt-in sh6bench attribution/candidate lane.
 * HZ11_FINE_LINEAR_MAX selects the 16B-stepped prefix, then the table returns
 * to powers of two through 64KiB. The global fineclass lane uses 1024. */

#define HZ11_MIN_SIZE 16u
#define HZ11_MAX_CACHED_SIZE 65536u           /* 64 KiB */
#ifndef HZ11_FINE_SIZE_CLASSES
#define HZ11_FINE_SIZE_CLASSES 0
#endif
#ifndef HZ11_FINE_LINEAR_MAX
#define HZ11_FINE_LINEAR_MAX 1024u
#endif

#if HZ11_FINE_SIZE_CLASSES
#if HZ11_FINE_LINEAR_MAX == 128u
#define HZ11_FINE_LINEAR_CLASSES 8u           /* 16, 32, ... 128 */
#define HZ11_FINE_POWER_BASE 256u
#define HZ11_CLASS_COUNT 17u
#elif HZ11_FINE_LINEAR_MAX == 256u
#define HZ11_FINE_LINEAR_CLASSES 16u          /* 16, 32, ... 256 */
#define HZ11_FINE_POWER_BASE 512u
#define HZ11_CLASS_COUNT 24u
#elif HZ11_FINE_LINEAR_MAX == 512u
#define HZ11_FINE_LINEAR_CLASSES 32u          /* 16, 32, ... 512 */
#define HZ11_FINE_POWER_BASE 1024u
#define HZ11_CLASS_COUNT 39u
#elif HZ11_FINE_LINEAR_MAX == 1024u
#define HZ11_FINE_LINEAR_CLASSES 64u          /* 16, 32, ... 1024 */
#define HZ11_FINE_POWER_BASE 2048u
#define HZ11_CLASS_COUNT 70u
#else
#error "HZ11_FINE_LINEAR_MAX must be one of 128, 256, 512, or 1024"
#endif
#else
#define HZ11_CLASS_COUNT 13u                  /* slot_size = HZ11_MIN_SIZE << class_id */
#endif
#define HZ11_LARGE_CLASS 255u                 /* sentinel: bypass cache */
#define HZ11_QUANTUM_SHIFT 4u                 /* 16-byte quanta */
#define HZ11_SIZE_TABLE_ENTRIES \
  (HZ11_MAX_CACHED_SIZE >> HZ11_QUANTUM_SHIFT) /* 4096 */

extern uint8_t hz11_size_table[HZ11_SIZE_TABLE_ENTRIES];
extern int hz11_size_table_ready;

void hz11_size_class_init(void);

/* Hot path: inlined so malloc does not pay a call per classification.
 * HZ11SizeTableStaticInit-L1: the lazy-init check CANNOT be removed because
 * ld.so itself calls malloc during library loading, before any constructor
 * runs. Removing the check corrupts the heap (all sizes get class 0).
 * The flag is non-volatile to let the compiler cache it; the write-once race
 * is benign (idempotent init). */
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
#if HZ11_FINE_SIZE_CLASSES
  if (class_id < HZ11_FINE_LINEAR_CLASSES) {
    return (size_t)HZ11_MIN_SIZE * ((size_t)class_id + 1u);
  }
  return (size_t)HZ11_FINE_POWER_BASE << (class_id - HZ11_FINE_LINEAR_CLASSES);
#else
  return (size_t)HZ11_MIN_SIZE << class_id;
#endif
}

#endif /* HZ11_SIZE_CLASS_H */
