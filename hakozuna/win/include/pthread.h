#pragma once

#if !defined(_WIN32)
#error "pthread shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <errno.h>

typedef SRWLOCK pthread_mutex_t;
typedef int pthread_mutexattr_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT

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

typedef INIT_ONCE pthread_once_t;
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

static inline BOOL CALLBACK hz3_pthread_once_cb(PINIT_ONCE once, PVOID param, PVOID* context) {
    (void)once;
    (void)context;
    void (*fn)(void) = (void (*)(void))param;
    fn();
    return TRUE;
}

static inline int pthread_once(pthread_once_t* once, void (*fn)(void)) {
    return InitOnceExecuteOnce(once, hz3_pthread_once_cb, (PVOID)fn, NULL) ? 0 : -1;
}

typedef DWORD pthread_key_t;

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

typedef DWORD pthread_t;

static inline pthread_t pthread_self(void) {
    return GetCurrentThreadId();
}
