#ifndef H8_MEDIUM_H
#define H8_MEDIUM_H

#include "../include/h8.h"
#include "h8_class_map.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

#define H8_MEDIUM_MIN_SIZE (H8_MAX_SMALL_SIZE + 1u)
#define H8_MEDIUM_MAX_SIZE 65536u
#define H8_MEDIUM_QUANTUM_BYTES 65536u
#define H8_MEDIUM_RUN_BYTES 65536u
#if defined(H8_MEDIUM_64K_TWO_SLOT)
#define H8_MEDIUM_64K_RUN_BYTES (2u * H8_MEDIUM_QUANTUM_BYTES)
#define H8_MEDIUM_64K_SLOT_COUNT 2u
#else
#define H8_MEDIUM_64K_RUN_BYTES H8_MEDIUM_RUN_BYTES
#define H8_MEDIUM_64K_SLOT_COUNT 1u
#endif
#define H8_MEDIUM_PAGE_BYTES 4096u
#if !defined(H8_MEDIUM_RESIDENT_BUDGET_CLASSES)
#define H8_MEDIUM_RESIDENT_BUDGET_CLASSES 4u
#endif
#if defined(H8_MEDIUM_UPPER48_CLASS)
#define H8_MEDIUM_CLASS_COUNT 5u
#else
#define H8_MEDIUM_CLASS_COUNT 4u
#endif
typedef enum H8MediumRunState {
  H8_MEDIUM_RUN_UNUSED = 0,
  H8_MEDIUM_RUN_ACTIVE = 1,
  H8_MEDIUM_RUN_RETIRING = 2,
  H8_MEDIUM_RUN_RETIRED = 3
} H8MediumRunState;

typedef enum H8MediumPayloadState {
  H8_MEDIUM_PAYLOAD_LIVE = 0,
  H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT = 1,
  H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED = 2
} H8MediumPayloadState;

typedef enum H8MediumWriterKind {
  H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC = 1,
  H8_MEDIUM_WRITER_OWNER_LOCAL_FREE = 2,
  H8_MEDIUM_WRITER_OWNER_COLLECT = 3,
  H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE = 4,
  H8_MEDIUM_WRITER_GLOBAL_ATTACH = 5,
  H8_MEDIUM_WRITER_OWNER_DETACH = 6
} H8MediumWriterKind;

typedef struct H8MediumClassSpec {
  uint32_t slot_size;
  uint32_t run_size;
  uint16_t slot_shift;
  uint16_t slot_count;
  uint16_t bitmap_words;
} H8MediumClassSpec;

typedef struct H8ThreadCtx H8ThreadCtx;

typedef struct H8MediumRun {
  uint8_t* base;
  uint16_t class_id;
  uint16_t slot_count;
  uint32_t slot_size;
  uint16_t slot_shift;
  uint32_t run_size;
  _Atomic uint64_t owner_word;
  _Atomic uint8_t state;
  _Atomic uint8_t qstate;
  _Atomic uint64_t pending_word_mask;
  _Atomic uint64_t* pending_bits;
  _Atomic uint32_t* slot_state;
  pthread_mutex_t lock;
  uint64_t free_mask;
  uint64_t allocated_mask;
  uint8_t payload_state;
  bool resident_charge;
  bool owner_attached;
  bool payload_chunk_backed;
  void* meta_alloc_base;
  struct H8MediumRun* next_owner;
  struct H8MediumRun* next_global;
  struct H8MediumRun* next_detached;
  struct H8MediumRun* next_pending;
  bool detached_indexed;
#if defined(H8_ENABLE_DEBUG_STATS)
  atomic_uint debug_writer_active;
  atomic_uint debug_writer_kind;
  atomic_uint_fast64_t debug_writer_token;
#endif
} H8MediumRun;

typedef struct H8OwnerRecord H8OwnerRecord;

bool h8_medium_size_supported(size_t size);
uint32_t h8_medium_class_for_size(size_t size);
const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id);
uint32_t h8_medium_rounded_size(size_t size);
bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out);
void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot);
bool h8_medium_run_owned_by_ctx(const H8MediumRun* run,
                                const H8ThreadCtx* ctx);
#if defined(H8_ENABLE_DEBUG_STATS)
void h8_medium_debug_writer_enter(H8MediumRun* run, H8OwnerRecord* owner,
                                  H8MediumWriterKind kind);
void h8_medium_debug_writer_exit(H8MediumRun* run);
#else
#define h8_medium_debug_writer_enter(run, owner, kind) ((void)0)
#define h8_medium_debug_writer_exit(run) ((void)0)
#endif
H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id);
void h8_medium_run_destroy_scaffold(H8MediumRun* run);
void* h8_medium_payload_alloc(size_t run_size, bool* chunk_backed_out);
void h8_medium_payload_free(void* ptr, size_t run_size, bool chunk_backed);
void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run);
bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr);
void h8_medium_release_empty_payload(H8MediumRun* run);
void h8_medium_decommit_empty_locked(H8MediumRun* run);
void h8_medium_mark_live_on_alloc(H8MediumRun* run);
void h8_medium_mark_empty_locked(H8MediumRun* run);
void h8_medium_owner_lease_shadow_open(H8OwnerRecord* owner,
                                       uint16_t generation);
void h8_medium_owner_lease_shadow_close(H8OwnerRecord* owner);
bool h8_medium_owner_lease_shadow_enter(H8OwnerRecord* owner,
                                        uint16_t generation);
void h8_medium_owner_lease_shadow_exit(H8OwnerRecord* owner);
void h8_medium_owner_lease_shadow_check_exit(H8OwnerRecord* owner);
H8PublishResult h8_medium_remote_publish(H8MediumRun* run, void* ptr);
bool h8_medium_owner_has_pending(H8OwnerRecord* owner);
size_t h8_medium_collect_owner_pending_budget(H8OwnerRecord* owner,
                                              size_t run_budget);
void h8_medium_collect_owner_pending_periodic(H8ThreadCtx* ctx);
void h8_medium_collect_owner_pending(H8OwnerRecord* owner);
void h8_medium_lock_global(void);
void h8_medium_unlock_global(void);
H8MediumRun* h8_medium_global_head(void);
H8MediumRun* h8_medium_detached_head_locked(uint32_t class_id);
void h8_medium_detached_add_locked(H8MediumRun* run);
void h8_medium_detached_remove_locked(H8MediumRun* run);
bool h8_medium_ptr_in_run(const H8MediumRun* run, const void* ptr);
H8MediumRun* h8_medium_directory_find(const void* ptr);
H8MediumRun* h8_medium_find_run_locked(const void* ptr, bool route_lookup);
void h8_medium_register_locked(H8MediumRun* run);
void h8_medium_unregister_locked(H8MediumRun* run);
void* h8_medium_malloc_inner(size_t size);
bool h8_medium_free_inner(void* ptr, bool* owned_out);
H8RouteKind h8_medium_route_inner(void* ptr);
void h8_medium_owner_detach_all(H8OwnerRecord* owner);

#endif
