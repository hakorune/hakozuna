#ifndef HZ11_PORT_H
#define HZ11_PORT_H

#if defined(_MSC_VER) && !defined(__clang__)
#define HZ11_THREAD_LOCAL __declspec(thread)
#else
#define HZ11_THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef CRITICAL_SECTION HZ11_MUTEX;

static inline void hz11_mutex_init(HZ11_MUTEX* m) {
  InitializeCriticalSection(m);
}

static inline void hz11_mutex_lock(HZ11_MUTEX* m) {
  EnterCriticalSection(m);
}

static inline void hz11_mutex_unlock(HZ11_MUTEX* m) {
  LeaveCriticalSection(m);
}
#else
#include <pthread.h>

typedef pthread_mutex_t HZ11_MUTEX;

static inline void hz11_mutex_init(HZ11_MUTEX* m) {
  (void)pthread_mutex_init(m, NULL);
}

static inline void hz11_mutex_lock(HZ11_MUTEX* m) {
  (void)pthread_mutex_lock(m);
}

static inline void hz11_mutex_unlock(HZ11_MUTEX* m) {
  (void)pthread_mutex_unlock(m);
}
#endif

#endif /* HZ11_PORT_H */
