#ifndef HZ10_PLATFORM_H
#define HZ10_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef INIT_ONCE hz10_platform_once_t;
typedef DWORD hz10_platform_thread_key_t;
typedef SRWLOCK hz10_platform_mutex_t;
#define HZ10_PLATFORM_ONCE_INIT INIT_ONCE_STATIC_INIT
#define HZ10_PLATFORM_MUTEX_INIT SRWLOCK_INIT
#else
#include <pthread.h>
typedef pthread_once_t hz10_platform_once_t;
typedef pthread_key_t hz10_platform_thread_key_t;
typedef pthread_mutex_t hz10_platform_mutex_t;
#define HZ10_PLATFORM_ONCE_INIT PTHREAD_ONCE_INIT
#define HZ10_PLATFORM_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

int hz10_platform_once(hz10_platform_once_t* once, void (*fn)(void));

int hz10_platform_thread_key_create(hz10_platform_thread_key_t* key,
                                    void (*destructor)(void*));
void* hz10_platform_thread_getspecific(hz10_platform_thread_key_t key);
int hz10_platform_thread_setspecific(hz10_platform_thread_key_t key,
                                    void* value);

int hz10_platform_mutex_init(hz10_platform_mutex_t* mutex);
int hz10_platform_mutex_destroy(hz10_platform_mutex_t* mutex);
int hz10_platform_mutex_lock(hz10_platform_mutex_t* mutex);
int hz10_platform_mutex_unlock(hz10_platform_mutex_t* mutex);
int hz10_platform_mutex_trylock(hz10_platform_mutex_t* mutex);

void hz10_platform_yield(void);
uint64_t hz10_platform_now_ns(void);

void* hz10_platform_reserve(size_t bytes);
int hz10_platform_commit(void* ptr, size_t bytes);
void* hz10_platform_reserve_rw(size_t bytes);
void hz10_platform_release(void* ptr, size_t bytes);
int hz10_platform_purge(void* ptr, size_t bytes);

#endif
