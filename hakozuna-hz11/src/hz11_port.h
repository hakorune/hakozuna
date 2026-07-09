#ifndef HZ11_PORT_H
#define HZ11_PORT_H

#if defined(_MSC_VER) && !defined(__clang__)
#define HZ11_THREAD_LOCAL __declspec(thread)
#else
#define HZ11_THREAD_LOCAL _Thread_local
#endif

#endif /* HZ11_PORT_H */
