#pragma once

#if !defined(_WIN32)
#error "pthread shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

typedef SRWLOCK pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef int pthread_condattr_t;
typedef INIT_ONCE pthread_once_t;
typedef DWORD pthread_key_t;
typedef struct pthread_handle_s {
    HANDLE handle;
    DWORD id;
} pthread_t;
typedef int pthread_attr_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

static inline int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr) {
    (void)attr;
    InitializeSRWLock(m);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t* m) {
    (void)m;
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t* m) {
    AcquireSRWLockExclusive(m);
    return 0;
}

static inline int pthread_mutex_trylock(pthread_mutex_t* m) {
    return TryAcquireSRWLockExclusive(m) ? 0 : EBUSY;
}

static inline int pthread_mutex_unlock(pthread_mutex_t* m) {
    ReleaseSRWLockExclusive(m);
    return 0;
}

static inline int pthread_cond_init(pthread_cond_t* c, const pthread_condattr_t* attr) {
    (void)attr;
    InitializeConditionVariable(c);
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t* c) {
    (void)c;
    return 0;
}

static inline int pthread_cond_signal(pthread_cond_t* c) {
    WakeConditionVariable(c);
    return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t* c) {
    WakeAllConditionVariable(c);
    return 0;
}

static inline DWORD hz_memcached_rel_timeout_ms(const struct timespec* abstime) {
    if (!abstime) {
        return INFINITE;
    }

    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    const uint64_t windows_epoch_100ns = uli.QuadPart;
    const uint64_t unix_epoch_100ns = 116444736000000000ULL;
    uint64_t now_ns = 0;
    if (windows_epoch_100ns > unix_epoch_100ns) {
        now_ns = (windows_epoch_100ns - unix_epoch_100ns) * 100ULL;
    }

    uint64_t target_ns = ((uint64_t)abstime->tv_sec * 1000000000ULL) + (uint64_t)abstime->tv_nsec;
    if (target_ns <= now_ns) {
        return 0;
    }

    uint64_t delta_ms = (target_ns - now_ns + 999999ULL) / 1000000ULL;
    if (delta_ms > (uint64_t)INFINITE - 1ULL) {
        return INFINITE - 1U;
    }
    return (DWORD)delta_ms;
}

static inline int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) {
    return SleepConditionVariableSRW(c, m, INFINITE, 0) ? 0 : EINVAL;
}

static inline int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m, const struct timespec* abstime) {
    DWORD timeout_ms = hz_memcached_rel_timeout_ms(abstime);
    if (SleepConditionVariableSRW(c, m, timeout_ms, 0)) {
        return 0;
    }
    DWORD err = GetLastError();
    return (err == ERROR_TIMEOUT) ? ETIMEDOUT : EINVAL;
}

static inline BOOL CALLBACK hz_memcached_pthread_once_cb(PINIT_ONCE once, PVOID param, PVOID* context) {
    (void)once;
    (void)context;
    void (*fn)(void) = (void (*)(void))param;
    fn();
    return TRUE;
}

static inline int pthread_once(pthread_once_t* once, void (*fn)(void)) {
    return InitOnceExecuteOnce(once, hz_memcached_pthread_once_cb, (PVOID)fn, NULL) ? 0 : -1;
}

static inline int pthread_key_create(pthread_key_t* key, void (*destructor)(void*)) {
    if (!key) {
        return EINVAL;
    }
    DWORD idx = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
    if (idx == FLS_OUT_OF_INDEXES) {
        return EAGAIN;
    }
    *key = idx;
    return 0;
}

static inline int pthread_key_delete(pthread_key_t key) {
    return FlsFree(key) ? 0 : EINVAL;
}

static inline int pthread_setspecific(pthread_key_t key, const void* value) {
    return FlsSetValue(key, (PVOID)value) ? 0 : EINVAL;
}

static inline void* pthread_getspecific(pthread_key_t key) {
    return FlsGetValue(key);
}

static inline pthread_t pthread_self(void) {
    pthread_t self;
    self.handle = NULL;
    self.id = GetCurrentThreadId();
    return self;
}

static inline int pthread_equal(pthread_t a, pthread_t b) {
    return a.id == b.id;
}

struct hz_memcached_pthread_start_ctx {
    void* (*start_routine)(void*);
    void* arg;
};

static inline unsigned __stdcall hz_memcached_pthread_start(void* raw) {
    struct hz_memcached_pthread_start_ctx* ctx = (struct hz_memcached_pthread_start_ctx*)raw;
    void* (*start_routine)(void*) = ctx->start_routine;
    void* arg = ctx->arg;
    free(ctx);
    (void)start_routine(arg);
    return 0;
}

static inline int pthread_attr_init(pthread_attr_t* attr) {
    if (attr) {
        *attr = 0;
    }
    return 0;
}

static inline int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    (void)attr;
    if (!thread || !start_routine) {
        return EINVAL;
    }

    struct hz_memcached_pthread_start_ctx* ctx =
        (struct hz_memcached_pthread_start_ctx*)malloc(sizeof(struct hz_memcached_pthread_start_ctx));
    if (!ctx) {
        return ENOMEM;
    }

    ctx->start_routine = start_routine;
    ctx->arg = arg;

    unsigned thread_id = 0;
    uintptr_t handle = _beginthreadex(NULL, 0, hz_memcached_pthread_start, ctx, 0, &thread_id);
    if (handle == 0) {
        free(ctx);
        return errno ? errno : EAGAIN;
    }

    thread->handle = (HANDLE)handle;
    thread->id = (DWORD)thread_id;
    return 0;
}

static inline int pthread_join(pthread_t thread, void** retval) {
    (void)retval;
    if (!thread.handle) {
        return EINVAL;
    }
    DWORD rc = WaitForSingleObject(thread.handle, INFINITE);
    if (rc != WAIT_OBJECT_0) {
        return EINVAL;
    }
    CloseHandle(thread.handle);
    return 0;
}
