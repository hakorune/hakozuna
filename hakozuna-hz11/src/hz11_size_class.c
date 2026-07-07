#include "hz11_size_class.h"

uint8_t hz11_size_table[HZ11_SIZE_TABLE_ENTRIES];
int hz11_size_table_ready = 0; /* non-volatile: write-once, benign race */

void hz11_size_class_init(void) {
  if (hz11_size_table_ready) {
    return;
  }
  for (size_t i = 0; i < HZ11_SIZE_TABLE_ENTRIES; ++i) {
    size_t size_max = (i + 1u) << HZ11_QUANTUM_SHIFT;
    uint8_t c = 0;
    size_t slot = HZ11_MIN_SIZE;
    while (slot < size_max && (c + 1u) < HZ11_CLASS_COUNT) {
      slot <<= 1u;
      c += 1u;
    }
    hz11_size_table[i] = c;
  }
  hz11_size_table_ready = 1;
}

/* HZ11SizeTableStaticInit-L1: the lazy init guard is load-bearing. ld.so can
 * call malloc while loading this LD_PRELOAD library, before any constructor
 * runs. Removing the guard makes the zero-filled table classify every size as
 * class 0 and corrupts the process heap. The shim constructor still performs
 * normal early init for user code, but it cannot protect loader-time malloc. */
