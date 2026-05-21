#ifndef HZ5_LOWPAGE64_H
#define HZ5_LOWPAGE64_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HZ5_LOWPAGE64_LOOKUP_ENUM
#define HZ5_LOWPAGE64_LOOKUP_ENUM
enum {
  HZ5_LOWPAGE64_LOOKUP_MISS = 0,
  HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE = 1,
  HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE = 2
};
#endif

size_t hz5_lowpage64_round_raw_bytes(size_t size,
                                     size_t alignment,
                                     size_t header_size);
void hz5_lowpage64_note_raw_range(void* raw_ptr, size_t raw_bytes);
int hz5_lowpage64_lookup(void* ptr);
int hz5_lowpage64_may_own(void* ptr);
int hz5_lowpage64_active_owns(void* ptr);
void* hz5_lowpage64_acquire(size_t raw_bytes);
void hz5_lowpage64_release(void* raw);
void hz5_lowpage64_register_stats_once(void);

#ifdef __cplusplus
}
#endif

#endif
