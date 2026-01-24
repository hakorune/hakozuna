#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <time.h>

#ifndef HZ3_TLS
// Prefer native TLS on Windows; A/B via HZ3_TLS_DECLSPEC=0/1.
#ifndef HZ3_TLS_DECLSPEC
#define HZ3_TLS_DECLSPEC 1
#endif
#if HZ3_TLS_DECLSPEC
#define HZ3_TLS __declspec(thread)
#else
#define HZ3_TLS __thread
#endif
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

static inline int hz3_clock_gettime_monotonic(struct timespec* ts) {
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&counter)) {
        return -1;
    }
    uint64_t ns = (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
    ts->tv_sec = (time_t)(ns / 1000000000ULL);
    ts->tv_nsec = (long)(ns % 1000000000ULL);
    return 0;
}

// Provide clock_gettime on Windows for code that expects it.
static inline int clock_gettime(int clock_id, struct timespec* ts) {
    if (clock_id != CLOCK_MONOTONIC || !ts) {
        return -1;
    }
    return hz3_clock_gettime_monotonic(ts);
}

// ----------------------------------------------------------------------------
// Platform Abstraction: Mutex and Once
// Windows: Use SRWLOCK directly (no pthread wrapper overhead)
// Linux: Use pthread_mutex (futex-based, already optimal)
// ----------------------------------------------------------------------------

// Mutex abstraction
typedef SRWLOCK hz3_lock_t;
#define HZ3_LOCK_INITIALIZER SRWLOCK_INIT

static inline void hz3_lock_init(hz3_lock_t* lock) {
    InitializeSRWLock(lock);
}

static inline void hz3_lock_destroy(hz3_lock_t* lock) {
    // SRWLOCK does not require cleanup
    (void)lock;
}

static inline void hz3_lock_acquire(hz3_lock_t* lock) {
    AcquireSRWLockExclusive(lock);
}

static inline void hz3_lock_release(hz3_lock_t* lock) {
    ReleaseSRWLockExclusive(lock);
}

// Once abstraction
typedef INIT_ONCE hz3_once_t;
#define HZ3_ONCE_INIT INIT_ONCE_STATIC_INIT

static BOOL CALLBACK hz3_once_wrapper(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context) {
    void (*init_fn)(void) = (void (*)(void))Parameter;
    init_fn();
    return TRUE;
}

static inline void hz3_once(hz3_once_t* once, void (*init_fn)(void)) {
    InitOnceExecuteOnce(once, hz3_once_wrapper, (PVOID)init_fn, NULL);
}

// Thread ID abstraction
static inline unsigned long hz3_thread_id(void) {
    return (unsigned long)GetCurrentThreadId();
}

#else  // POSIX (Linux)

#include <pthread.h>

#ifndef HZ3_TLS
#ifndef HZ3_TLS_DECLSPEC
#define HZ3_TLS_DECLSPEC 0
#endif
#if HZ3_TLS_DECLSPEC
#define HZ3_TLS __declspec(thread)
#else
#define HZ3_TLS __thread
#endif
#endif

// Mutex abstraction
typedef pthread_mutex_t hz3_lock_t;
#define HZ3_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline void hz3_lock_init(hz3_lock_t* lock) {
    pthread_mutex_init(lock, NULL);
}

static inline void hz3_lock_destroy(hz3_lock_t* lock) {
    pthread_mutex_destroy(lock);
}

static inline void hz3_lock_acquire(hz3_lock_t* lock) {
    pthread_mutex_lock(lock);
}

static inline void hz3_lock_release(hz3_lock_t* lock) {
    pthread_mutex_unlock(lock);
}

// Once abstraction
typedef pthread_once_t hz3_once_t;
#define HZ3_ONCE_INIT PTHREAD_ONCE_INIT

static inline void hz3_once(hz3_once_t* once, void (*init_fn)(void)) {
    pthread_once(once, init_fn);
}

// Thread ID abstraction
static inline unsigned long hz3_thread_id(void) {
    return (unsigned long)pthread_self();
}

#endif  // _WIN32
