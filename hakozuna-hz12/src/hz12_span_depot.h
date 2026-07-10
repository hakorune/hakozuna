#ifndef HZ12_SPAN_DEPOT_H
#define HZ12_SPAN_DEPOT_H

#include <stdint.h>
#include <stdio.h>

#ifndef HZ12_SPAN_DEPOT_CAP
#define HZ12_SPAN_DEPOT_CAP 64u
#endif

typedef struct H12SpanDepotTakeResult {
  void* span_base;
  uint32_t before_state;
  uint32_t after_state;
  uint8_t class_id;
  uint8_t recommitted;
  uint8_t accounting_reset;
  uint8_t route_attached;
  uint8_t current_installed;
} H12SpanDepotTakeResult;

typedef struct H12SpanDepotStats {
  uint64_t put_attempt;
  uint64_t put_success;
  uint64_t put_duplicate;
  uint64_t put_full;
  uint64_t put_reject_state;
  uint64_t take_attempt;
  uint64_t take_hit;
  uint64_t take_miss;
  uint64_t recommit_success;
  uint64_t recommit_fail;
  uint64_t route_fail;
  uint64_t current_install_fail;
  uint64_t rollback;
  uint32_t depot_current;
  uint32_t depot_max;
} H12SpanDepotStats;

void h12_span_depot_reset(void);
int h12_span_depot_put(const void* ptr);
int h12_span_depot_take(uint8_t class_id, H12SpanDepotTakeResult* out);
uint32_t h12_span_depot_count(void);
void h12_span_depot_stats(H12SpanDepotStats* out);
void h12_span_depot_dump(FILE* out, const H12SpanDepotTakeResult* result);

#endif /* HZ12_SPAN_DEPOT_H */
