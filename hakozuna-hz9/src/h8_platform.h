#ifndef H8_PLATFORM_H
#define H8_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef INIT_ONCE h8_platform_once_t;
typedef DWORD h8_platform_thread_key_t;
typedef SRWLOCK h8_platform_mutex_t;
#define H8_PLATFORM_ONCE_INIT INIT_ONCE_STATIC_INIT
#define H8_PLATFORM_MUTEX_INIT SRWLOCK_INIT
#else
#include <pthread.h>
typedef pthread_once_t h8_platform_once_t;
typedef pthread_key_t h8_platform_thread_key_t;
typedef pthread_mutex_t h8_platform_mutex_t;
#define H8_PLATFORM_ONCE_INIT PTHREAD_ONCE_INIT
#define H8_PLATFORM_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

int h8_platform_once(h8_platform_once_t* once, void (*fn)(void));

int h8_platform_thread_key_create(h8_platform_thread_key_t* key,
                                  void (*destructor)(void*));
void* h8_platform_thread_getspecific(h8_platform_thread_key_t key);
int h8_platform_thread_setspecific(h8_platform_thread_key_t key, void* value);

int h8_platform_mutex_init(h8_platform_mutex_t* mutex);
int h8_platform_mutex_destroy(h8_platform_mutex_t* mutex);
int h8_platform_mutex_lock(h8_platform_mutex_t* mutex);
int h8_platform_mutex_unlock(h8_platform_mutex_t* mutex);
int h8_platform_mutex_trylock(h8_platform_mutex_t* mutex);

void h8_platform_yield(void);
uint64_t h8_platform_now_ns(void);

void* h8_platform_reserve(size_t bytes);
int h8_platform_commit(void* ptr, size_t bytes);
void* h8_platform_reserve_rw(size_t bytes);
void h8_platform_release(void* ptr, size_t bytes);
int h8_platform_purge(void* ptr, size_t bytes);

#endif
