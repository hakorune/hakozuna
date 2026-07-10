#ifndef HZ12_RECLAIM_CARVE_DIAG_H
#define HZ12_RECLAIM_CARVE_DIAG_H

#include <stdint.h>

/* Diagnostic-only: bypasses returned reuse to exercise an installed span. */
void* h12_reclaim_carve_current(uint8_t class_id);

#endif /* HZ12_RECLAIM_CARVE_DIAG_H */
