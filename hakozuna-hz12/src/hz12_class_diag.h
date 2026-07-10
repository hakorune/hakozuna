#ifndef HZ12_CLASS_DIAG_H
#define HZ12_CLASS_DIAG_H

#include <stdint.h>

/* Diagnostic-only class attribution. Keep this out of H12Stats so speed rows
 * and the public ABI do not carry per-class arrays. */
#ifndef HZ12_CLASS_DIAG
#define HZ12_CLASS_DIAG 0
#endif

/* Matrix attribution diagnostics are a second, narrower layer used when a
 * Windows matrix row is refill-dominated. Keep it separate from CLASS_DIAG so
 * classdiag rows can stay lightweight and speed rows keep zero production cost. */
#ifndef HZ12_MATRIX_ATTRIB_DIAG
#define HZ12_MATRIX_ATTRIB_DIAG 0
#endif

#if HZ12_CLASS_DIAG
void hz12_class_diag_malloc(uint8_t class_id);
void hz12_class_diag_hit(uint8_t class_id);
void hz12_class_diag_refill(uint8_t class_id);
void hz12_class_diag_overflow(uint8_t class_id);
void hz12_class_diag_returned_pop_hit(uint8_t class_id);
void hz12_class_diag_returned_pop_miss(uint8_t class_id);
void hz12_class_diag_returned_push_range(uint8_t class_id, uint32_t count);
void hz12_class_diag_returned_sink_depth(uint8_t class_id, int32_t delta);
void hz12_class_diag_dump_stats(void);
#define HZ12_CLASS_DIAG_MALLOC(c) hz12_class_diag_malloc((c))
#define HZ12_CLASS_DIAG_HIT(c) hz12_class_diag_hit((c))
#define HZ12_CLASS_DIAG_REFILL(c) hz12_class_diag_refill((c))
#define HZ12_CLASS_DIAG_OVERFLOW(c) hz12_class_diag_overflow((c))
#define HZ12_CLASS_DIAG_RETURNED_POP_HIT(c) hz12_class_diag_returned_pop_hit((c))
#define HZ12_CLASS_DIAG_RETURNED_POP_MISS(c) hz12_class_diag_returned_pop_miss((c))
#define HZ12_CLASS_DIAG_RETURNED_PUSH_RANGE(c, n) \
  hz12_class_diag_returned_push_range((c), (n))
#define HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(c, d) \
  hz12_class_diag_returned_sink_depth((c), (d))
#else
static inline void hz12_class_diag_dump_stats(void) {}
#define HZ12_CLASS_DIAG_MALLOC(c) ((void)0)
#define HZ12_CLASS_DIAG_HIT(c) ((void)0)
#define HZ12_CLASS_DIAG_REFILL(c) ((void)0)
#define HZ12_CLASS_DIAG_OVERFLOW(c) ((void)0)
#define HZ12_CLASS_DIAG_RETURNED_POP_HIT(c) ((void)0)
#define HZ12_CLASS_DIAG_RETURNED_POP_MISS(c) ((void)0)
#define HZ12_CLASS_DIAG_RETURNED_PUSH_RANGE(c, n) ((void)0)
#define HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(c, d) ((void)0)
#endif

#if HZ12_MATRIX_ATTRIB_DIAG
void hz12_matrix_diag_cache_at_refill(uint8_t class_id, uint32_t count);
void hz12_matrix_diag_cache_after_batch(uint8_t class_id, uint32_t count);
void hz12_matrix_diag_returned_one(uint8_t class_id, int hit);
void hz12_matrix_diag_returned_batch(uint8_t class_id, uint32_t count);
void hz12_matrix_diag_current_hit(uint8_t class_id);
void hz12_matrix_diag_bump_batch(uint8_t class_id, uint32_t count);
void hz12_matrix_diag_span_new(uint8_t class_id);
void hz12_matrix_diag_sys_fallback(uint8_t class_id);
void hz12_matrix_diag_dump_stats(void);
#define HZ12_MATRIX_DIAG_CACHE_AT_REFILL(c, n) \
  hz12_matrix_diag_cache_at_refill((c), (n))
#define HZ12_MATRIX_DIAG_CACHE_AFTER_BATCH(c, n) \
  hz12_matrix_diag_cache_after_batch((c), (n))
#define HZ12_MATRIX_DIAG_RETURNED_ONE(c, hit) \
  hz12_matrix_diag_returned_one((c), (hit))
#define HZ12_MATRIX_DIAG_RETURNED_BATCH(c, n) \
  hz12_matrix_diag_returned_batch((c), (n))
#define HZ12_MATRIX_DIAG_CURRENT_HIT(c) hz12_matrix_diag_current_hit((c))
#define HZ12_MATRIX_DIAG_BUMP_BATCH(c, n) hz12_matrix_diag_bump_batch((c), (n))
#define HZ12_MATRIX_DIAG_SPAN_NEW(c) hz12_matrix_diag_span_new((c))
#define HZ12_MATRIX_DIAG_SYS_FALLBACK(c) hz12_matrix_diag_sys_fallback((c))
#else
static inline void hz12_matrix_diag_dump_stats(void) {}
#define HZ12_MATRIX_DIAG_CACHE_AT_REFILL(c, n) ((void)0)
#define HZ12_MATRIX_DIAG_CACHE_AFTER_BATCH(c, n) ((void)0)
#define HZ12_MATRIX_DIAG_RETURNED_ONE(c, hit) ((void)0)
#define HZ12_MATRIX_DIAG_RETURNED_BATCH(c, n) ((void)0)
#define HZ12_MATRIX_DIAG_CURRENT_HIT(c) ((void)0)
#define HZ12_MATRIX_DIAG_BUMP_BATCH(c, n) ((void)0)
#define HZ12_MATRIX_DIAG_SPAN_NEW(c) ((void)0)
#define HZ12_MATRIX_DIAG_SYS_FALLBACK(c) ((void)0)
#endif

#endif /* HZ12_CLASS_DIAG_H */
