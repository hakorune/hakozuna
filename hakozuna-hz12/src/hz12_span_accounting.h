#ifndef HZ12_SPAN_ACCOUNTING_H
#define HZ12_SPAN_ACCOUNTING_H

#include <stdint.h>
#include <stdio.h>

typedef struct H12WholeSpanShadow {
  uint32_t tracked_spans;
  uint32_t full_capacity_spans;
  uint32_t accounting_candidates;
  uint64_t tracked_allocations;
  uint64_t tracked_releases;
  uint64_t tracked_live_objects;
  uint64_t release_untracked;
  uint64_t release_underflow;
} H12WholeSpanShadow;

typedef struct H12SpanAccountingRecord {
  uint32_t span_id;
  uint32_t slot_capacity;
  uint32_t allocated;
  uint32_t released;
  uint32_t live;
  uint32_t carved_slots;
  uint8_t class_id;
  uint8_t accounting_candidate;
} H12SpanAccountingRecord;

void h12_span_accounting_reset(void);
void h12_span_accounting_on_alloc(void* ptr);
void h12_span_accounting_on_release(void* ptr);
H12WholeSpanShadow h12_span_accounting_scan(void);
int h12_span_accounting_query(const void* ptr,
                              H12SpanAccountingRecord* out);
int h12_span_accounting_recycle(const void* ptr, uint8_t class_id);
void h12_span_accounting_dump(FILE* out);

#endif /* HZ12_SPAN_ACCOUNTING_H */
