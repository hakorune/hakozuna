#ifndef HZ12_FLUSH_OWNER_ROUTE_H
#define HZ12_FLUSH_OWNER_ROUTE_H

#include <stdint.h>

struct H12ThreadCache;

void hz12_flush_owner_route_attach(struct H12ThreadCache* tc);
void hz12_flush_owner_route_batch(struct H12ThreadCache* tc,
                                  uint8_t class_id,
                                  void** items,
                                  uint32_t count);
void hz12_flush_owner_route_drain(struct H12ThreadCache* tc);

#endif /* HZ12_FLUSH_OWNER_ROUTE_H */
