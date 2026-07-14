#ifndef H8_BENCH_POST_RSS_CONTROL_H
#define H8_BENCH_POST_RSS_CONTROL_H

#include <stddef.h>

typedef struct H8PostRssSnapshot {
  size_t vm_rss_bytes;
  size_t rss_anon_bytes;
  size_t rss_file_bytes;
  size_t rss_shmem_bytes;
  size_t rollup_rss_bytes;
  size_t pss_bytes;
  size_t private_clean_bytes;
  size_t private_dirty_bytes;
  size_t anonymous_bytes;
} H8PostRssSnapshot;

typedef struct H8PostRssControl {
  H8PostRssSnapshot raw;
  H8PostRssSnapshot settled;
  H8PostRssSnapshot trimmed;
  int trim_result;
} H8PostRssControl;

void h8_bench_post_rss_control_run(H8PostRssControl* control);
void h8_bench_post_rss_control_print(int run,
                                     const H8PostRssControl* control);

#endif
