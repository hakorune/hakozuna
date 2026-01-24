#define _GNU_SOURCE

#include "hz3_os_purge.h"

#if HZ3_OS_PURGE_STATS
#include "hz3_dtor_stats.h"
#include <errno.h>
#endif

// ============================================================================
// OS Purge Box (shared helper implementation)
// ============================================================================

#if HZ3_OS_PURGE_STATS
HZ3_DTOR_STAT(g_hz3_os_purge_calls);
HZ3_DTOR_STAT(g_hz3_os_purge_ok);
HZ3_DTOR_STAT(g_hz3_os_purge_fail);
HZ3_DTOR_STAT(g_hz3_os_purge_skip_range);
HZ3_DTOR_STAT(g_hz3_os_purge_enomem);
HZ3_DTOR_STAT(g_hz3_os_purge_other);

HZ3_DTOR_ATEXIT_FLAG(g_hz3_os_purge);

static void hz3_os_purge_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_calls);
    uint32_t ok = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_ok);
    uint32_t fail = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_fail);
    uint32_t skip_range = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_skip_range);
    uint32_t enomem = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_enomem);
    uint32_t other = HZ3_DTOR_STAT_LOAD(g_hz3_os_purge_other);
    if (calls > 0 || ok > 0 || fail > 0 || skip_range > 0) {
        fprintf(stderr,
                "[HZ3_OS_PURGE] calls=%u ok=%u fail=%u skip_range=%u enomem=%u other=%u\n",
                calls, ok, fail, skip_range, enomem, other);
    }
}
#endif

int hz3_os_madvise_dontneed_checked(void* addr, size_t len) {
#if HZ3_OS_PURGE_STATS
    HZ3_DTOR_STAT_INC(g_hz3_os_purge_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_hz3_os_purge, hz3_os_purge_atexit_dump);
#endif

    if (!hz3_os_in_arena_range(addr, len)) {
#if HZ3_OS_PURGE_STATS
        HZ3_DTOR_STAT_INC(g_hz3_os_purge_skip_range);
#endif
        return -1;
    }

    int ret = madvise(addr, len, MADV_DONTNEED);
#if HZ3_OS_PURGE_STATS
    if (ret == 0) {
        HZ3_DTOR_STAT_INC(g_hz3_os_purge_ok);
    } else {
        HZ3_DTOR_STAT_INC(g_hz3_os_purge_fail);
        if (errno == ENOMEM) {
            HZ3_DTOR_STAT_INC(g_hz3_os_purge_enomem);
        } else {
            HZ3_DTOR_STAT_INC(g_hz3_os_purge_other);
        }
    }
#endif
    return ret;
}
