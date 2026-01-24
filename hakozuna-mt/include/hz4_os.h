// hz4_os.h - HZ4 OS Box (segment acquire/release)
#ifndef HZ4_OS_H
#define HZ4_OS_H

#include <stddef.h>

void* hz4_os_seg_acquire(void);
void  hz4_os_seg_release(void* seg_base);
void* hz4_os_page_acquire(void);
void  hz4_os_page_release(void* page_base);
void* hz4_os_large_acquire(size_t size);
void  hz4_os_large_release(void* base, size_t size);

#endif // HZ4_OS_H
