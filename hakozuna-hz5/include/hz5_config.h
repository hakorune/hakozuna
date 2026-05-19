#ifndef HZ5_CONFIG_H
#define HZ5_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#define HZ5_PAGE_SIZE ((size_t)4096)
#define HZ5_SEG_SIZE ((size_t)2 * 1024 * 1024)
#define HZ5_SEG_PAGES ((uint32_t)(HZ5_SEG_SIZE / HZ5_PAGE_SIZE))
#define HZ5_SEG_HEADER_PAGES 16u
#define HZ5_FIRST_DATA_PAGE HZ5_SEG_HEADER_PAGES

#define HZ5_SEG_MAGIC 0x485A35474152454EULL
#define HZ5_SEG_VERSION 1u
#define HZ5_SEG_MAX 64u
#define HZ5_REMOTE_BUF_CAP 8u
#define HZ5_RUN_CACHE_CLASS_COUNT 16u
#define HZ5_RUN_CACHE_CAP 128u

#ifndef HZ5_DIAGNOSTIC_STATS
#define HZ5_DIAGNOSTIC_STATS 1
#endif

#endif
