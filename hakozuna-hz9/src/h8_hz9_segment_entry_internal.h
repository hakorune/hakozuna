#ifndef H8_HZ9_SEGMENT_ENTRY_INTERNAL_H
#define H8_HZ9_SEGMENT_ENTRY_INTERNAL_H

#include "h8_internal.h"
#include "h8_hz9_segment_entry.h"

#if defined(H9_SEGMENT_ENTRY_L1)

#ifndef H9_SEGMENT_ENTRY_PAGE_CAP
#define H9_SEGMENT_ENTRY_PAGE_CAP 64u
#endif

typedef struct H9SegmentEntryPage {
  void* base;
  uint32_t slot_size;
  uint32_t run_size;
  uint16_t slot_count;
  uint16_t class_id;
  uint64_t free_bits;
  uint64_t alloc_bits;
  uint64_t cache_bits;
} H9SegmentEntryPage;

extern H9SegmentEntryPage h9_segment_entry_pages[H9_SEGMENT_ENTRY_PAGE_CAP];
extern uint32_t h9_segment_entry_page_count;
extern _Thread_local uint32_t
    h9_segment_entry_active[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uintptr_t
    h9_segment_entry_handle[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local void*
    h9_segment_entry_cache_ptr[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uintptr_t
    h9_segment_entry_cache_page[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uint32_t
    h9_segment_entry_cache_slot[H8_MEDIUM_CLASS_COUNT];

H9SegmentEntryPage* h9_segment_entry_page_for_ptr(void* ptr,
                                                  uint32_t* page_out,
                                                  uint32_t* slot_out);
bool h9_segment_entry_alloc_page_slot(H9SegmentEntryPage* page,
                                      void** ptr_out,
                                      uint32_t* slot_out);

#endif

#endif
