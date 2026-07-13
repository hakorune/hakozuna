#ifndef H8_MEDIUM_PAGE8K_REMOTE_H
#define H8_MEDIUM_PAGE8K_REMOTE_H

#include "../include/h8.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct H8Page8KRemoteOwner H8Page8KRemoteOwner;
typedef struct H8Page8KRemotePage H8Page8KRemotePage;

typedef struct H8Page8KRemoteStats {
  uint64_t dispatch_alloc_attempt;
  uint64_t dispatch_alloc_served;
  uint64_t dispatch_free_attempt;
  uint64_t dispatch_free_owner_present;
  uint64_t dispatch_free_owned;
  uint64_t dispatch_free_success;
  uint64_t dispatch_free_miss;
  uint64_t owner_create;
  uint64_t range_eligible_alloc;
  uint64_t range_served_alloc;
  uint64_t remote_claim_attempt;
  uint64_t remote_claim_success;
  uint64_t remote_claim_reject;
  uint64_t pending_publish;
  uint64_t queue_notify;
  uint64_t queue_dirty;
  uint64_t drain_pages;
  uint64_t drain_slots;
  uint64_t duplicate_reject;
  uint64_t interior_reject;
  uint64_t lost_notification;
  uint64_t inbox_depth;
  uint64_t inbox_depth_max;
  uint64_t page_cap_reject;
  uint64_t drain_all_owner_visits;
  uint64_t drain_all_limit;
  uint64_t drain_all_skipped_live;
  uint64_t owner_close;
  uint64_t orphan_adopt;
  uint64_t publish_retry;
  uint64_t page_decommit;
  uint64_t page_recommit;
  uint64_t page_decommit_fail;
  uint64_t orphan_resident_empty;
  uint64_t orphan_resident_empty_max;
} H8Page8KRemoteStats;

H8Page8KRemoteOwner* h8_page8k_remote_owner_create(uint32_t token);
void h8_page8k_remote_owner_destroy(H8Page8KRemoteOwner* owner);
H8Page8KRemotePage* h8_page8k_remote_page_create(
    H8Page8KRemoteOwner* owner);
#if defined(H8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1)
/* Diagnostic/research API: size must be exactly 8K, 16K, or 32K. */
H8Page8KRemotePage* h8_page8k_remote_page_create_size(
    H8Page8KRemoteOwner* owner, size_t size);
#endif
void* h8_page8k_remote_alloc(H8Page8KRemoteOwner* owner,
                             H8Page8KRemotePage* page);
bool h8_page8k_remote_free(H8Page8KRemoteOwner* current_owner, void* ptr,
                           bool* owned_out);
bool h8_page8k_remote_free_record_current(const void* record, void* ptr,
                                           bool* owned_out);
size_t h8_page8k_remote_drain(H8Page8KRemoteOwner* owner);
bool h8_page8k_remote_quiescent(const H8Page8KRemotePage* page);
H8Page8KRemoteStats h8_page8k_remote_stats(void);
bool h8_page8k_remote_accepts_size(size_t size);
void* h8_page8k_remote_malloc_current(size_t size);
bool h8_page8k_remote_free_current(void* ptr, bool* owned_out);
H8RouteKind h8_page8k_remote_route_current(const void* ptr);
bool h8_page8k_remote_usable_size_current(const void* ptr,
                                          size_t* usable_out,
                                          bool* owned_out);
size_t h8_page8k_remote_drain_all_control(void);
void h8_page8k_remote_thread_shutdown(void);
bool h8_page8k_remote_owner_close(H8Page8KRemoteOwner* owner);
bool h8_page8k_remote_adopt_one(H8Page8KRemoteOwner* owner);
#if defined(H8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1)
/* Adopts only an orphan page whose immutable geometry matches size. */
bool h8_page8k_remote_adopt_one_size(H8Page8KRemoteOwner* owner,
                                      size_t size);
#endif

#endif
