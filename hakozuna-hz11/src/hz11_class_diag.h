#ifndef HZ11_CLASS_DIAG_H
#define HZ11_CLASS_DIAG_H

#include <stdint.h>

/* Diagnostic-only class attribution. Keep this out of H11Stats so speed rows
 * and the public ABI do not carry per-class arrays. */
#ifndef HZ11_CLASS_DIAG
#define HZ11_CLASS_DIAG 0
#endif

#if HZ11_CLASS_DIAG
void hz11_class_diag_malloc(uint8_t class_id);
void hz11_class_diag_hit(uint8_t class_id);
void hz11_class_diag_refill(uint8_t class_id);
void hz11_class_diag_overflow(uint8_t class_id);
void hz11_class_diag_returned_pop_hit(uint8_t class_id);
void hz11_class_diag_returned_pop_miss(uint8_t class_id);
void hz11_class_diag_dump_stats(void);
#define HZ11_CLASS_DIAG_MALLOC(c) hz11_class_diag_malloc((c))
#define HZ11_CLASS_DIAG_HIT(c) hz11_class_diag_hit((c))
#define HZ11_CLASS_DIAG_REFILL(c) hz11_class_diag_refill((c))
#define HZ11_CLASS_DIAG_OVERFLOW(c) hz11_class_diag_overflow((c))
#define HZ11_CLASS_DIAG_RETURNED_POP_HIT(c) hz11_class_diag_returned_pop_hit((c))
#define HZ11_CLASS_DIAG_RETURNED_POP_MISS(c) hz11_class_diag_returned_pop_miss((c))
#else
static inline void hz11_class_diag_dump_stats(void) {}
#define HZ11_CLASS_DIAG_MALLOC(c) ((void)0)
#define HZ11_CLASS_DIAG_HIT(c) ((void)0)
#define HZ11_CLASS_DIAG_REFILL(c) ((void)0)
#define HZ11_CLASS_DIAG_OVERFLOW(c) ((void)0)
#define HZ11_CLASS_DIAG_RETURNED_POP_HIT(c) ((void)0)
#define HZ11_CLASS_DIAG_RETURNED_POP_MISS(c) ((void)0)
#endif

#endif /* HZ11_CLASS_DIAG_H */
