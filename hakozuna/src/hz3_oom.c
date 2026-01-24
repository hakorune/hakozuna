#include "hz3_oom.h"
#include "hz3_config.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#if HZ3_OOM_SHOT
static _Atomic int g_hz3_oom_shot_fired = 0;
#endif

void hz3_oom_note(const char* where, uint64_t info0, uint64_t info1) {
#if !HZ3_OOM_SHOT && !HZ3_OOM_FAILFAST
    (void)where;
    (void)info0;
    (void)info1;
    return;
#endif

#if HZ3_OOM_FAILFAST
    fprintf(stderr, "[HZ3_OOM_FAILFAST] where=%s info0=%llu info1=%llu\n",
            where,
            (unsigned long long)info0,
            (unsigned long long)info1);
    abort();
#elif HZ3_OOM_SHOT
    if (atomic_exchange_explicit(&g_hz3_oom_shot_fired, 1, memory_order_relaxed) == 0) {
        fprintf(stderr, "[HZ3_OOM] where=%s info0=%llu info1=%llu\n",
                where,
                (unsigned long long)info0,
                (unsigned long long)info1);
    }
#endif
}
