#ifndef H8_HZ9_SLAB_ROUTE_INTERNAL_H
#define H8_HZ9_SLAB_ROUTE_INTERNAL_H

#define H9_SLAB_ROUTE_MAX_PAGES 1024u
#define H9_SLAB_ROUTE_MAX_SLOTS 64u
#define H9_SLAB_ROUTE_HASH_CAP 2048u
#define H9_SLAB_PAGE_BYTES (128u * 1024u)
#ifndef H9_SLAB_LOCAL_PAGE_CAP
#define H9_SLAB_LOCAL_PAGE_CAP 4u
#endif
#define H9_SLAB_DISABLED_PAGE ((H9MediumSlabRoutePage*)(uintptr_t)1u)

struct H9MediumSlabRoutePage {
  void* raw_base;
  size_t raw_bytes;
  uint8_t* base;
  size_t bytes;
  uint32_t class_id;
  uint32_t slot_size;
  uint16_t slot_count;
#if defined(H9_SLAB_ALLOC_CURSOR_L1)
  uint16_t alloc_cursor;
#endif
  uint64_t owner_word;
#if defined(H9_SLAB_LOCAL_FREE_BITS_L1)
  uint64_t local_free_bits;
#endif
  _Atomic uint64_t pending_bits;
  _Atomic uint8_t qstate;
  _Atomic uint32_t slot_state[H9_SLAB_ROUTE_MAX_SLOTS];
  _Atomic bool registered;
  struct H9MediumSlabRoutePage* next_pending;
};

H9MediumSlabRoutePage* h9_slab_route_register_page(
    void* raw_base, size_t raw_bytes, void* base, size_t bytes,
    uint32_t class_id, uint32_t slot_size, uint16_t slot_count,
    H8OwnerRecord* owner, uint32_t initial_state);
void h9_slab_route_purge_empty_page(H9MediumSlabRoutePage* page);

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
size_t h9_slab_collect_pending_page(H9MediumSlabRoutePage* page);
void h9_slab_signal_work(H8OwnerRecord* owner, H9MediumSlabRoutePage* page);
void h9_slab_pending_queue_push(H8OwnerRecord* owner,
                                H9MediumSlabRoutePage* page);
#endif

#endif
