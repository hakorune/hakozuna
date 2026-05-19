#include "hz5_internal.h"

#include <stdatomic.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static _Atomic(uint32_t) g_hz5_owner_next_slot = 1;
static _Atomic(unsigned char) g_hz5_owner_alive[UINT16_MAX + 1u];
static _Thread_local Hz5OwnerToken t_hz5_owner;

#if defined(_WIN32)
static INIT_ONCE g_hz5_owner_fls_once = INIT_ONCE_STATIC_INIT;
static DWORD g_hz5_owner_fls_index = FLS_OUT_OF_INDEXES;

static VOID CALLBACK hz5_owner_fls_destructor(void* value) {
    (void)value;
    if (t_hz5_owner.slot != 0) {
        atomic_store_explicit(&g_hz5_owner_alive[t_hz5_owner.slot],
                              0,
                              memory_order_release);
    }
    Hz5RemoteBuffer* buffer = hz5_remote_tls_buffer();
    uint32_t pending = buffer ? buffer->count : 0;
    if (pending != 0) {
        hz5_stats_inc(HZ5_STAT_TLS_DESTRUCTOR_FLUSH);
        hz5_stats_add(HZ5_STAT_TLS_DESTRUCTOR_FLUSH_ENTRY, pending);
        hz5_remote_flush_buffer(buffer);
    }
    if (t_hz5_owner.slot != 0) {
        hz5_remote_release_owner(t_hz5_owner);
    }
    hz5_tcache_release_all();
}

static BOOL CALLBACK hz5_owner_init_fls(PINIT_ONCE once,
                                        PVOID param,
                                        PVOID* context) {
    (void)once;
    (void)param;
    (void)context;
    g_hz5_owner_fls_index = FlsAlloc(hz5_owner_fls_destructor);
    return g_hz5_owner_fls_index != FLS_OUT_OF_INDEXES;
}

static void hz5_owner_register_thread_destructor(void) {
    if (!InitOnceExecuteOnce(&g_hz5_owner_fls_once,
                             hz5_owner_init_fls,
                             NULL,
                             NULL)) {
        return;
    }
    if (g_hz5_owner_fls_index == FLS_OUT_OF_INDEXES) {
        return;
    }
    if (!FlsGetValue(g_hz5_owner_fls_index)) {
        FlsSetValue(g_hz5_owner_fls_index, (void*)1);
    }
}
#else
static void hz5_owner_register_thread_destructor(void) {
}
#endif

Hz5OwnerToken hz5_owner_current(void) {
    if (t_hz5_owner.slot == 0) {
        uint32_t slot = atomic_fetch_add_explicit(&g_hz5_owner_next_slot,
                                                  1,
                                                  memory_order_relaxed);
        t_hz5_owner.slot = (uint16_t)(slot ? slot : 1);
        t_hz5_owner.generation = 1;
        atomic_store_explicit(&g_hz5_owner_alive[t_hz5_owner.slot],
                              1,
                              memory_order_release);
        hz5_owner_register_thread_destructor();
    }
    return t_hz5_owner;
}

Hz5OwnerToken hz5_owner_current_if_initialized(void) {
    return t_hz5_owner;
}

int hz5_owner_equal(Hz5OwnerToken a, Hz5OwnerToken b) {
    return a.slot == b.slot && a.generation == b.generation;
}

int hz5_owner_is_alive(Hz5OwnerToken owner) {
    if (owner.slot == 0) {
        return 0;
    }
    return atomic_load_explicit(&g_hz5_owner_alive[owner.slot],
                                memory_order_acquire) != 0;
}
