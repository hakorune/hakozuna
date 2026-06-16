#ifndef HZ6_ALLOCATOR_ROUTE_PROBE_HIST_H
#define HZ6_ALLOCATOR_ROUTE_PROBE_HIST_H

#include "hz6_allocator.h"

static inline size_t hz6_allocator_route_probe_bucket(size_t probes) {
  if (probes <= 1) {
    return 0;
  }
  if (probes <= 4) {
    return 1;
  }
  if (probes <= 8) {
    return 2;
  }
  if (probes <= 16) {
    return 3;
  }
  if (probes <= 64) {
    return 4;
  }
  return 5;
}

static inline void hz6_allocator_note_route_probe_hist(size_t* hist,
                                                       size_t probes) {
  if (!hist) {
    return;
  }
  ++hist[hz6_allocator_route_probe_bucket(probes)];
}

#endif
