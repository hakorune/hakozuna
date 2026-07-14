#include "h8_bench_post_rss_control.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void h8_post_rss_read_status(H8PostRssSnapshot* snapshot) {
  FILE* file = fopen("/proc/self/status", "r");
  if (!file) return;
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    size_t kib = 0;
    if (sscanf(line, "VmRSS: %zu kB", &kib) == 1) {
      snapshot->vm_rss_bytes = kib * 1024u;
    } else if (sscanf(line, "RssAnon: %zu kB", &kib) == 1) {
      snapshot->rss_anon_bytes = kib * 1024u;
    } else if (sscanf(line, "RssFile: %zu kB", &kib) == 1) {
      snapshot->rss_file_bytes = kib * 1024u;
    } else if (sscanf(line, "RssShmem: %zu kB", &kib) == 1) {
      snapshot->rss_shmem_bytes = kib * 1024u;
    }
  }
  fclose(file);
}

static void h8_post_rss_read_rollup(H8PostRssSnapshot* snapshot) {
  FILE* file = fopen("/proc/self/smaps_rollup", "r");
  if (!file) return;
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    size_t kib = 0;
    if (sscanf(line, "Rss: %zu kB", &kib) == 1) {
      snapshot->rollup_rss_bytes = kib * 1024u;
    } else if (sscanf(line, "Pss: %zu kB", &kib) == 1) {
      snapshot->pss_bytes = kib * 1024u;
    } else if (sscanf(line, "Private_Clean: %zu kB", &kib) == 1) {
      snapshot->private_clean_bytes = kib * 1024u;
    } else if (sscanf(line, "Private_Dirty: %zu kB", &kib) == 1) {
      snapshot->private_dirty_bytes = kib * 1024u;
    } else if (sscanf(line, "Anonymous: %zu kB", &kib) == 1) {
      snapshot->anonymous_bytes = kib * 1024u;
    }
  }
  fclose(file);
}

static void h8_post_rss_snapshot(H8PostRssSnapshot* snapshot) {
  memset(snapshot, 0, sizeof(*snapshot));
  h8_post_rss_read_status(snapshot);
  h8_post_rss_read_rollup(snapshot);
}

static void h8_post_rss_settle(void) {
  struct timespec delay = {.tv_sec = 0, .tv_nsec = 100000000L};
  while (nanosleep(&delay, &delay) != 0) {
  }
}

void h8_bench_post_rss_control_run(H8PostRssControl* control) {
  memset(control, 0, sizeof(*control));
  h8_post_rss_snapshot(&control->raw);
  h8_post_rss_settle();
  h8_post_rss_snapshot(&control->settled);
  control->trim_result = malloc_trim(0);
  h8_post_rss_snapshot(&control->trimmed);
}

static void h8_post_rss_print_snapshot(const char* name,
                                       const H8PostRssSnapshot* snapshot) {
  printf(" %s_vm_rss=%zu %s_rss_anon=%zu %s_rss_file=%zu"
         " %s_rss_shmem=%zu %s_rollup_rss=%zu %s_pss=%zu"
         " %s_private_clean=%zu %s_private_dirty=%zu %s_anonymous=%zu",
         name, snapshot->vm_rss_bytes, name, snapshot->rss_anon_bytes, name,
         snapshot->rss_file_bytes, name, snapshot->rss_shmem_bytes, name,
         snapshot->rollup_rss_bytes, name, snapshot->pss_bytes, name,
         snapshot->private_clean_bytes, name, snapshot->private_dirty_bytes,
         name, snapshot->anonymous_bytes);
}

void h8_bench_post_rss_control_print(int run,
                                     const H8PostRssControl* control) {
  printf("post_rss_control run=%d trim_result=%d", run,
         control->trim_result);
  h8_post_rss_print_snapshot("raw", &control->raw);
  h8_post_rss_print_snapshot("settled", &control->settled);
  h8_post_rss_print_snapshot("trim", &control->trimmed);
  putchar('\n');
}
