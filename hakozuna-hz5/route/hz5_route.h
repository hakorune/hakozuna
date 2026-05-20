#ifndef HZ5_ROUTE_H
#define HZ5_ROUTE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int hz5_route_exact_a8192(size_t size, size_t align);
int hz5_route_hz3_medium_can_satisfy(size_t size, size_t align);
void hz5_route_p12_on_alloc(size_t size, size_t align);

#ifdef __cplusplus
}
#endif

#endif
