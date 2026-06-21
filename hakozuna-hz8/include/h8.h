#ifndef H8_H
#define H8_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum H8RouteKind {
  H8_ROUTE_MISS = 0,
  H8_ROUTE_VALID = 1,
  H8_ROUTE_INVALID = 2
} H8RouteKind;

typedef enum H8PublishResult {
  H8_PUBLISH_OK = 0,
  H8_PUBLISH_MISS = 1,
  H8_PUBLISH_INVALID = 2,
  H8_PUBLISH_DOUBLE_FREE = 3,
  H8_PUBLISH_OWNER_TRANSITION = 4,
  H8_PUBLISH_RETRY = 5
} H8PublishResult;

typedef struct H8Stats {
  size_t arena_reserved_bytes;
  size_t arena_committed_bytes;
  size_t small_span_count;
  size_t owner_count;
  size_t orphan_span_count;
  size_t local_alloc_count;
  size_t local_free_count;
  size_t remote_publish_count;
  size_t remote_collect_count;
  size_t owner_exit_count;
  size_t pending_enqueue_count;
  size_t pending_dequeue_count;
  size_t orphan_handoff_count;
  size_t handoff_success_count;
} H8Stats;

typedef struct H8DebugStats {
  size_t owner_publish_enter_count;
  size_t owner_publish_exit_count;
  size_t handoff_fail_count;
  size_t invalid_count;
  size_t miss_count;
  size_t owner_transition_count;
  size_t adoption_scan_count;
  size_t adoption_candidate_count;
  size_t adoption_block_state_count;
  size_t adoption_block_quiesce_count;
  size_t adoption_empty_count;
  size_t adoption_target_closed_count;
  size_t adoption_success_count;
} H8DebugStats;

void h8_init(void);
void h8_shutdown(void);
void* h8_malloc(size_t size);
void* h8_calloc(size_t count, size_t size);
void h8_free(void* ptr);
H8RouteKind h8_route(void* ptr);
H8Stats h8_stats(void);
H8DebugStats h8_debug_stats(void);

#ifdef __cplusplus
}
#endif

#endif
