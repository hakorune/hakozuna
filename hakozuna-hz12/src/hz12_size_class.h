#ifndef HZ12_SIZE_CLASS_H
#define HZ12_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

/* HZ12 default size classes: power-of-two slot sizes 16..65536 (13 classes),
 * looked up by a 16B-quantum table (NOT the HZ10 fine-class branch ladder).
 * Objects larger than 64KiB are LARGE and bypass the cache (system allocator).
 *
 * HZ12_FINE_SIZE_CLASSES is an opt-in sh6bench attribution/candidate lane.
 * HZ12_FINE_LINEAR_MAX selects the 16B-stepped prefix, then the table returns
 * to powers of two through 64KiB. The global fineclass lane uses 1024. */

#define HZ12_MIN_SIZE 16u
#define HZ12_MAX_CACHED_SIZE 65536u           /* 64 KiB */
#ifndef HZ12_FINE_SIZE_CLASSES
#define HZ12_FINE_SIZE_CLASSES 0
#endif
#ifndef HZ12_FINE_LINEAR_MAX
#define HZ12_FINE_LINEAR_MAX 1024u
#endif

#if HZ12_FINE_SIZE_CLASSES
#if HZ12_FINE_LINEAR_MAX == 128u
#define HZ12_FINE_LINEAR_CLASSES 8u           /* 16, 32, ... 128 */
#define HZ12_FINE_POWER_BASE 256u
#define HZ12_CLASS_COUNT 17u
#elif HZ12_FINE_LINEAR_MAX == 256u
#define HZ12_FINE_LINEAR_CLASSES 16u          /* 16, 32, ... 256 */
#define HZ12_FINE_POWER_BASE 512u
#define HZ12_CLASS_COUNT 24u
#elif HZ12_FINE_LINEAR_MAX == 512u
#define HZ12_FINE_LINEAR_CLASSES 32u          /* 16, 32, ... 512 */
#define HZ12_FINE_POWER_BASE 1024u
#define HZ12_CLASS_COUNT 39u
#elif HZ12_FINE_LINEAR_MAX == 1024u
#define HZ12_FINE_LINEAR_CLASSES 64u          /* 16, 32, ... 1024 */
#define HZ12_FINE_POWER_BASE 2048u
#define HZ12_CLASS_COUNT 70u
#else
#error "HZ12_FINE_LINEAR_MAX must be one of 128, 256, 512, or 1024"
#endif
#else
#define HZ12_CLASS_COUNT 13u                  /* slot_size = HZ12_MIN_SIZE << class_id */
#endif
#define HZ12_LARGE_CLASS 255u                 /* sentinel: bypass cache */
#define HZ12_QUANTUM_SHIFT 4u                 /* 16-byte quanta */
#define HZ12_SIZE_TABLE_ENTRIES \
  (HZ12_MAX_CACHED_SIZE >> HZ12_QUANTUM_SHIFT) /* 4096 */

extern uint8_t hz12_size_table[HZ12_SIZE_TABLE_ENTRIES];
extern int hz12_size_table_ready;

void hz12_size_class_init(void);

/* Hot path: inlined so malloc does not pay a call per classification.
 * HZ12SizeTableStaticInit-L1: the lazy-init check CANNOT be removed because
 * ld.so itself calls malloc during library loading, before any constructor
 * runs. Removing the check corrupts the heap (all sizes get class 0).
 * The flag is non-volatile to let the compiler cache it; the write-once race
 * is benign (idempotent init). */
static inline uint8_t hz12_size_class(size_t size) {
  if (size == 0u) {
    size = HZ12_MIN_SIZE;
  }
  if (size > HZ12_MAX_CACHED_SIZE) {
    return HZ12_LARGE_CLASS;
  }
  if (!hz12_size_table_ready) {
    hz12_size_class_init();
  }
  return hz12_size_table[(size - 1u) >> HZ12_QUANTUM_SHIFT];
}

static inline size_t hz12_class_slot_size(uint8_t class_id) {
  if (class_id >= HZ12_CLASS_COUNT) {
    return 0u;
  }
#if HZ12_FINE_SIZE_CLASSES
  if (class_id < HZ12_FINE_LINEAR_CLASSES) {
    return (size_t)HZ12_MIN_SIZE * ((size_t)class_id + 1u);
  }
  return (size_t)HZ12_FINE_POWER_BASE << (class_id - HZ12_FINE_LINEAR_CLASSES);
#else
  return (size_t)HZ12_MIN_SIZE << class_id;
#endif
}

#endif /* HZ12_SIZE_CLASS_H */
