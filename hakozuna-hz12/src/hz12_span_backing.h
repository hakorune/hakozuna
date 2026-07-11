#ifndef HZ12_SPAN_BACKING_H
#define HZ12_SPAN_BACKING_H

#include <stdint.h>

enum {
  HZ12_BACKING_STATE_UNKNOWN = 0,
  HZ12_BACKING_STATE_RESIDENT = 1,
  HZ12_BACKING_STATE_DISCARDED = 2
};

int h12_span_backing_discard(void* span_base,
                             uint32_t* before_state,
                             uint32_t* after_state);
int h12_span_backing_recommit(void* span_base,
                              uint32_t* before_state,
                              uint32_t* after_state);
int h12_span_backing_validate_discarded(void* span_base);

#endif
