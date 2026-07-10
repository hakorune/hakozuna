#include "hz12_size_class.h"

uint8_t hz12_size_table[HZ12_SIZE_TABLE_ENTRIES];
int hz12_size_table_ready = 0; /* non-volatile: write-once, benign race */

void hz12_size_class_init(void) {
  if (hz12_size_table_ready) {
    return;
  }
  for (size_t i = 0; i < HZ12_SIZE_TABLE_ENTRIES; ++i) {
    size_t size_max = (i + 1u) << HZ12_QUANTUM_SHIFT;
    uint8_t c = 0;
    size_t slot = hz12_class_slot_size(c);
    while (slot < size_max && (c + 1u) < HZ12_CLASS_COUNT) {
      c += 1u;
      slot = hz12_class_slot_size(c);
    }
    hz12_size_table[i] = c;
  }
  hz12_size_table_ready = 1;
}

/* HZ12SizeTableStaticInit-L1: the lazy init guard is load-bearing. ld.so can
 * call malloc while loading this LD_PRELOAD library, before any constructor
 * runs. Removing the guard makes the zero-filled table classify every size as
 * class 0 and corrupts the process heap. The shim constructor still performs
 * normal early init for user code, but it cannot protect loader-time malloc. */
