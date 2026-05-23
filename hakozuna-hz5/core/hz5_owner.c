#include "hz5_internal.h"

#include <stdatomic.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

static _Atomic(uint32_t) g_hz5_owner_next_slot = 1;
static _Atomic(unsigned char) g_hz5_owner_state[UINT16_MAX + 1u];
static _Atomic(uint32_t) g_hz5_owner_generation[UINT16_MAX + 1u];
static _Thread_local Hz5OwnerToken t_hz5_owner;

static uint16_t hz5_owner_next_generation(uint16_t slot) {
    uint32_t generation = atomic_fetch_add_explicit(
                              &g_hz5_owner_generation[slot],
                              1,
                              memory_order_acq_rel) +
                          1u;
    if ((uint16_t)generation == 0) {
        generation = atomic_fetch_add_explicit(&g_hz5_owner_generation[slot],
                                               1,
                                               memory_order_acq_rel) +
                     1u;
    }
    return (uint16_t)generation;
}

static void hz5_owner_mark_state(Hz5OwnerToken owner, Hz5OwnerState state) {
    if (owner.slot == 0) {
        return;
    }
    atomic_store_explicit(&g_hz5_owner_state[owner.slot],
                          (unsigned char)state,
                          memory_order_release);
}

static void hz5_owner_destruct_current(void) {
    Hz5OwnerToken owner = t_hz5_owner;
    if (owner.slot == 0) {
        return;
    }

    hz5_owner_mark_state(owner, HZ5_OWNER_DYING);
    Hz5RemoteBuffer* buffer = hz5_remote_tls_buffer();
    uint32_t pending = buffer ? buffer->count : 0;
    if (pending != 0) {
        hz5_stats_inc(HZ5_STAT_TLS_DESTRUCTOR_FLUSH);
        hz5_stats_add(HZ5_STAT_TLS_DESTRUCTOR_FLUSH_ENTRY, pending);
        hz5_remote_flush_buffer(buffer);
    }
    hz5_remote_release_owner(owner);
    hz5_tcache_release_all();
    hz5_owner_mark_state(owner, HZ5_OWNER_DEAD);
    t_hz5_owner = (Hz5OwnerToken){0, 0};
}

#if defined(_WIN32)
static INIT_ONCE g_hz5_owner_fls_once = INIT_ONCE_STATIC_INIT;
static DWORD g_hz5_owner_fls_index = FLS_OUT_OF_INDEXES;

static VOID CALLBACK hz5_owner_fls_destructor(void* value) {
    (void)value;
    hz5_owner_destruct_current();
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
static pthread_once_t g_hz5_owner_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t g_hz5_owner_key;
static _Atomic(unsigned char) g_hz5_owner_key_ready;

static void hz5_owner_pthread_destructor(void* value) {
    (void)value;
    hz5_owner_destruct_current();
}

static void hz5_owner_init_pthread_key(void) {
    if (pthread_key_create(&g_hz5_owner_key,
                           hz5_owner_pthread_destructor) == 0) {
        atomic_store_explicit(&g_hz5_owner_key_ready,
                              1,
                              memory_order_release);
    }
}

static void hz5_owner_register_thread_destructor(void) {
    pthread_once(&g_hz5_owner_key_once, hz5_owner_init_pthread_key);
    if (atomic_load_explicit(&g_hz5_owner_key_ready,
                             memory_order_acquire) == 0) {
        return;
    }
    if (!pthread_getspecific(g_hz5_owner_key)) {
        pthread_setspecific(g_hz5_owner_key, (void*)1);
    }
}
#endif

Hz5OwnerToken hz5_owner_current(void) {
    if (t_hz5_owner.slot == 0) {
        uint32_t slot = atomic_fetch_add_explicit(&g_hz5_owner_next_slot,
                                                  1,
                                                  memory_order_relaxed);
        if (slot == 0) {
            slot = 1;
        }
        if (slot > UINT16_MAX) {
            slot = ((slot - 1u) % UINT16_MAX) + 1u;
        }
        t_hz5_owner.slot = (uint16_t)slot;
        t_hz5_owner.generation = hz5_owner_next_generation(t_hz5_owner.slot);
        atomic_store_explicit(&g_hz5_owner_state[t_hz5_owner.slot],
                              (unsigned char)HZ5_OWNER_ALIVE,
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
    uint32_t generation = atomic_load_explicit(
        &g_hz5_owner_generation[owner.slot],
        memory_order_acquire);
    if ((uint16_t)generation != owner.generation) {
        return 0;
    }
    return atomic_load_explicit(&g_hz5_owner_state[owner.slot],
                                memory_order_acquire) ==
           (unsigned char)HZ5_OWNER_ALIVE;
}
