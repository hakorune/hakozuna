#ifndef HZ6_ALLOCATOR_API_ROUTE_TRANSFER_H
#define HZ6_ALLOCATOR_API_ROUTE_TRANSFER_H

#include "hz6_allocator_types.h"
#include "hz6_allocator_route_resolve_free.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr);

Hz6RouteResult hz6_allocator_route_lookup_exact(
    const Hz6Allocator* allocator,
    const void* ptr);

void hz6_allocator_route_visibility_register(Hz6Allocator* allocator);

void hz6_allocator_route_visibility_unregister(Hz6Allocator* allocator);

Hz6RouteResult hz6_allocator_route_lookup_visible(
    Hz6Allocator* allocator,
    const void* ptr);

Hz6RouteResult hz6_allocator_route_lookup_visible_only(
    Hz6Allocator* allocator,
    const void* ptr);

Hz6RouteResult hz6_allocator_route_lookup_visible_after_local_miss(
    Hz6Allocator* allocator,
    const void* ptr);

#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
Hz6Allocator* hz6_allocator_descriptor_storage_owner(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count);
#endif

#if HZ6_DIAGNOSTIC_PROBES
Hz6Allocator* hz6_allocator_descriptor_storage_owner_diagnostic(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count);
#endif

int hz6_allocator_route_negative_filter_skip_local(
    Hz6Allocator* allocator,
    const void* ptr);

Hz6OwnerLocalityKind hz6_allocator_route_owner_locality_hint(
    const Hz6Allocator* allocator,
    const void* ptr);

void hz6_allocator_route_shared_directory_dryrun(Hz6Allocator* allocator,
                                                 const void* ptr);

Hz6RouteResult hz6_allocator_route_shared_directory_lookup_exact(
    Hz6Allocator* allocator,
    const void* ptr);

int hz6_allocator_route_rehome_exact(Hz6Allocator* allocator,
                                     const Hz6RouteResult* route);

typedef enum Hz6RouteRegisterReason {
  HZ6_ROUTE_REGISTER_REASON_UNKNOWN = 0,
  HZ6_ROUTE_REGISTER_REASON_SOURCE_RUN_SLOT = 1,
  HZ6_ROUTE_REGISTER_REASON_DIRECT_SOURCE = 2,
  HZ6_ROUTE_REGISTER_REASON_MATERIALIZE = 3,
  HZ6_ROUTE_REGISTER_REASON_REHOME = 4
} Hz6RouteRegisterReason;

typedef enum Hz6RouteUnregisterReason {
  HZ6_ROUTE_UNREGISTER_REASON_UNKNOWN = 0,
  HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW = 1,
  HZ6_ROUTE_UNREGISTER_REASON_CAP_RELEASE = 2,
  HZ6_ROUTE_UNREGISTER_REASON_DESCRIPTORLESS_DETACH = 3,
  HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE = 4,
  HZ6_ROUTE_UNREGISTER_REASON_REHOME = 5
} Hz6RouteUnregisterReason;

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr);

void hz6_free_with_route_prechecked(Hz6Allocator* allocator,
                                    void* ptr,
                                    Hz6RouteResult route,
                                    int visible_hit);

void hz6_free_with_resolved_route_after_maps(Hz6Allocator* allocator,
                                             void* ptr,
                                             Hz6RouteResult route,
                                             int visible_hit);

void hz6_allocator_route_unregister_exact_reason(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteUnregisterReason reason);

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor);

int hz6_allocator_route_register_exact_reason(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t generation,
    void* descriptor,
    Hz6RouteRegisterReason reason);

void hz6_allocator_route_maintain_tombstones(Hz6Allocator* allocator);

int hz6_allocator_route_replace_exact_descriptor(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t old_generation,
    void* old_descriptor,
    uint32_t new_generation,
    void* new_descriptor);

int hz6_allocator_source_block_register_invalid_range(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_source_block_route_dryrun(Hz6Allocator* allocator,
                                             const void* ptr);

void hz6_allocator_small_run_route_dryrun(Hz6Allocator* allocator,
                                          const void* ptr);

Hz6RouteResult hz6_allocator_small_run_route_lookup(Hz6Allocator* allocator,
                                                    const void* ptr);

int hz6_allocator_route_register_shared_invalid_range(Hz6Allocator* allocator,
                                                      void* base,
                                                      size_t bytes,
                                                      uint16_t front_id,
                                                      uint16_t class_id);

void hz6_allocator_route_unregister_shared_invalid_range(
    Hz6Allocator* allocator,
    void* base);

Hz6RouteBackendKind hz6_allocator_route_backend_kind(
    const Hz6Allocator* allocator);

size_t hz6_allocator_route_page_granularity(const Hz6Allocator* allocator);

Hz6TransferBackendKind hz6_allocator_transfer_backend_kind(
    const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_capacity(const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_count(const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_count_class(const Hz6Allocator* allocator,
                                          uint16_t class_id);

size_t hz6_allocator_transfer_shard_count_at(const Hz6Allocator* allocator,
                                             size_t shard_index);

size_t hz6_allocator_transfer_shard_capacity_at(
    const Hz6Allocator* allocator,
    size_t shard_index);

size_t hz6_allocator_shared_route_directory_bytes(void);

size_t hz6_allocator_owner_locality_index_bytes(void);

int hz6_allocator_transfer_push(Hz6Allocator* allocator,
                                Hz6TransferObject object);

int hz6_allocator_transfer_reserve(Hz6Allocator* allocator,
                                   uint16_t class_id,
                                   Hz6TransferReservation* out);

void hz6_allocator_transfer_cancel(Hz6TransferReservation* reservation);

void hz6_allocator_transfer_commit(Hz6TransferReservation* reservation,
                                   Hz6TransferObject object);

int hz6_allocator_transfer_pop(Hz6Allocator* allocator,
                               uint16_t class_id,
                               Hz6TransferObject* out);

int hz6_allocator_remote_free_overflow_reserve(
    Hz6Allocator* allocator,
    Hz6TransferReservation* out);

void hz6_allocator_remote_free_overflow_commit(
    Hz6TransferReservation* reservation,
    Hz6TransferObject object);

int hz6_allocator_remote_free_overflow_pop(Hz6Allocator* allocator,
                                           uint16_t class_id,
                                           Hz6TransferObject* out);

int hz6_allocator_remote_free_drain_transfer_one(Hz6Allocator* allocator,
                                                 uint16_t class_id);

void hz6_allocator_remote_pending_inbox_init(Hz6Allocator* allocator);

typedef enum Hz6RemotePendingReuseStatus {
  HZ6_REMOTE_PENDING_REUSE_EMPTY = 0,
  HZ6_REMOTE_PENDING_REUSE_BUSY = 1,
  HZ6_REMOTE_PENDING_REUSE_OK = 2,
  HZ6_REMOTE_PENDING_REUSE_INTEGRITY_FAILURE = 3
} Hz6RemotePendingReuseStatus;

int hz6_allocator_remote_pending_enqueue(Hz6Allocator* allocator,
                                         Hz6ObjectDescriptor* descriptor,
                                         void* ptr,
                                         uint32_t generation,
                                         uint16_t front_id,
                                         uint16_t class_id);

int hz6_allocator_remote_pending_external_ticket_publish(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    uint32_t generation,
    uint16_t front_id,
    uint16_t class_id);

int hz6_allocator_remote_pending_external_ticket_consume_one(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

size_t hz6_allocator_remote_pending_maintenance_class(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t budget);

size_t hz6_allocator_remote_pending_maintenance_class_external_only(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t budget);

Hz6RemotePendingReuseStatus hz6_allocator_remote_pending_try_reuse(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    void** out_ptr,
    Hz6ObjectDescriptor** out_descriptor);

Hz6RemotePendingReuseStatus
hz6_allocator_remote_pending_try_reuse_known_nonempty(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    void** out_ptr,
    Hz6ObjectDescriptor** out_descriptor);

int hz6_allocator_remote_pending_key_nonempty(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id);

void hz6_allocator_remote_pending_note_accounting_snapshot(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot);

void hz6_allocator_remote_pending_closeout_for_destroy(
    Hz6Allocator* allocator);

void hz6_allocator_remote_pending_note_after_destroy(
    Hz6Allocator* allocator);

void hz6_allocator_remote_pending_storage_release(Hz6Allocator* allocator);

int hz6_allocator_remote_pending_key_maybe_nonempty_raw(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_frontcache_miss(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_front_dispatch(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_source_alloc(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_before_maintenance(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_after_maintenance(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_maintenance_reuse_success(
    Hz6Allocator* allocator,
    size_t drained);

void hz6_allocator_remote_pending_note_prefill_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_prefill_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_source_block_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_direct_source_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_remote_pending_note_direct_source_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

void hz6_allocator_note_transfer_push(Hz6Allocator* allocator);

void hz6_allocator_note_transfer_pop(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
