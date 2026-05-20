#ifndef HZ5_HZ3_FALLBACK_H
#define HZ5_HZ3_FALLBACK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  HZ5_HZ3_FALLBACK_UNLOADED = 0,
  HZ5_HZ3_FALLBACK_LOADING = 1,
  HZ5_HZ3_FALLBACK_READY = 2,
  HZ5_HZ3_FALLBACK_FAILED = 3
};

void hz5_hz3_fallback_register_once(void);
uint32_t hz5_hz3_fallback_state(void);
void* hz5_hz3_fallback_alloc(size_t size, size_t align);
void hz5_hz3_fallback_free(void* ptr);
void hz5_hz3_fallback_note_crt_bypass_alloc(void);
void hz5_hz3_fallback_note_crt_bypass_free(void);

#ifdef __cplusplus
}
#endif

#endif
