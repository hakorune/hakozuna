#ifndef HZ5_LOWPAGE64_H
#define HZ5_LOWPAGE64_H

#include <stddef.h>
#include <stdint.h>

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

#ifndef HZ5_LOWPAGE64_P43_PREPARED_RELEASE
#ifdef BENCHLAB_HZ5_P43_PREPARED_RELEASE
#define HZ5_LOWPAGE64_P43_PREPARED_RELEASE \
  BENCHLAB_HZ5_P43_PREPARED_RELEASE
#else
#define HZ5_LOWPAGE64_P43_PREPARED_RELEASE 0
#endif
#endif

typedef struct Hz5Lowpage64FreeCtx {
  int lookup_kind;
  void* segment_token;
  void* slot_base;
  size_t slot_size;
  uint32_t slot_index;
  uint32_t flags;
} Hz5Lowpage64FreeCtx;

size_t hz5_lowpage64_round_raw_bytes(size_t size,
                                     size_t alignment,
                                     size_t header_size);
void hz5_lowpage64_note_raw_range(void* raw_ptr, size_t raw_bytes);
int hz5_lowpage64_lookup(void* ptr);
int hz5_lowpage64_prepare_free_user(void* ptr, Hz5Lowpage64FreeCtx* ctx);
int hz5_lowpage64_may_own(void* ptr);
int hz5_lowpage64_active_owns(void* ptr);
void* hz5_lowpage64_acquire(size_t raw_bytes);
void hz5_lowpage64_release(void* raw);
void hz5_lowpage64_p43g_note_wrapper(int is_p25_source, int raw_match);
void hz5_lowpage64_p43g_note_old_path(void);
void hz5_lowpage64_p43g_note_prepared_path(void);
void hz5_lowpage64_register_stats_once(void);

#ifdef __cplusplus
}
#endif

#endif
