#ifndef HZ5_LEGACY_P17_H
#define HZ5_LEGACY_P17_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* hz5_legacy_p17_wrapper_alloc(size_t size, size_t align);
void hz5_legacy_p17_raw_release(void* raw);

#ifdef __cplusplus
}
#endif

#endif
