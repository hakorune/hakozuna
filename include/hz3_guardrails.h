#pragma once

#include "hz3_config.h"
#include "hz3_types.h"

// Guardrails: compile-time checks and debug-only range asserts.
#if HZ3_GUARDRAILS
#if defined(__GNUC__) || defined(__clang__)
#define HZ3_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define HZ3_COLD __attribute__((cold))
#else
#define HZ3_WARN_UNUSED_RESULT
#define HZ3_COLD
#endif

#define HZ3_GUARDRAIL_TRAP() __builtin_trap()

#define HZ3_ASSERT_BIN_RANGE(bin) \
    do { \
        if (__builtin_expect((uint32_t)(bin) >= (uint32_t)HZ3_BIN_TOTAL, 0)) { \
            HZ3_GUARDRAIL_TRAP(); \
        } \
    } while (0)

#define HZ3_ASSERT_DST_RANGE(dst) \
    do { \
        if (__builtin_expect((uint32_t)(dst) >= (uint32_t)HZ3_NUM_SHARDS, 0)) { \
            HZ3_GUARDRAIL_TRAP(); \
        } \
    } while (0)

#define HZ3_ASSERT_FLAT_RANGE(flat) \
    do { \
        if (__builtin_expect((uint32_t)(flat) >= (uint32_t)(HZ3_BIN_TOTAL * HZ3_NUM_SHARDS), 0)) { \
            HZ3_GUARDRAIL_TRAP(); \
        } \
    } while (0)

_Static_assert(HZ3_BIN_TOTAL <= 255, "guardrails: HZ3_BIN_TOTAL must fit in 8 bits");
_Static_assert(HZ3_NUM_SHARDS <= 255, "guardrails: HZ3_NUM_SHARDS must fit in 8 bits");
#if HZ3_BIN_PAD_LOG2
_Static_assert((HZ3_BIN_PAD & (HZ3_BIN_PAD - 1)) == 0, "guardrails: HZ3_BIN_PAD must be power of 2");
#endif
#else
#define HZ3_WARN_UNUSED_RESULT
#define HZ3_COLD
#define HZ3_ASSERT_BIN_RANGE(bin) do { (void)(bin); } while (0)
#define HZ3_ASSERT_DST_RANGE(dst) do { (void)(dst); } while (0)
#define HZ3_ASSERT_FLAT_RANGE(flat) do { (void)(flat); } while (0)
#endif

// Segment size validation (always enabled)
_Static_assert(HZ3_SEG_SIZE % HZ3_PAGE_SIZE == 0,
               "HZ3_SEG_SIZE must be multiple of HZ3_PAGE_SIZE");
_Static_assert(HZ3_PAGES_PER_SEG == HZ3_SEG_SIZE / HZ3_PAGE_SIZE,
               "HZ3_PAGES_PER_SEG must equal HZ3_SEG_SIZE / HZ3_PAGE_SIZE");
