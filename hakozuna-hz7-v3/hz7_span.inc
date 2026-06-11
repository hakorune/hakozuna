static uint64_t* h7_span_bitmap(H7Span* span) {
  return (uint64_t*)((unsigned char*)span + sizeof(H7Span));
}

static int h7_span_is_full(const H7Span* span) {
  return span->used_count == span->slot_count;
}

static int h7_span_is_empty(const H7Span* span) {
  return span->used_count == 0u;
}

static unsigned char* h7_span_slots(H7Span* span) {
  return (unsigned char*)span + span->slot_offset;
}

static unsigned char* h7_span_slot_ptr(H7Span* span, uint32_t index) {
  return h7_span_slots(span) + (size_t)index * span->slot_size;
}

static int h7_bitmap_test(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  return (bitmap[index / 64u] & (UINT64_C(1) << (index % 64u))) != 0;
}

static int h7_small_slot_index(H7Span* span, void* ptr, uint32_t* out_index) {
  uintptr_t slots;
  uintptr_t addr;
  uint32_t index;
  if (!span || !ptr || span->slot_size == 0) {
    return 0;
  }
  slots = (uintptr_t)h7_span_slots(span);
  addr = (uintptr_t)ptr;
  if (addr < slots || addr >= (uintptr_t)span + H7_SPAN_BYTES) {
    return 0;
  }
  if (((addr - slots) % span->slot_size) != 0) {
    return 0;
  }
  index = (uint32_t)((addr - slots) / span->slot_size);
  if (index >= span->slot_count || !h7_bitmap_test(span, index)) {
    return 0;
  }
  if (out_index) {
    *out_index = index;
  }
  return 1;
}

static void h7_bitmap_set(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] |= (UINT64_C(1) << (index % 64u));
}

static void h7_bitmap_clear(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] &= ~(UINT64_C(1) << (index % 64u));
}

static uint32_t* h7_span_free_slot_next_cell(H7Span* span, uint32_t index) {
  return (uint32_t*)h7_span_slot_ptr(span, index);
}

static uint32_t h7_span_free_slot_next(H7Span* span, uint32_t index) {
  return *h7_span_free_slot_next_cell(span, index);
}

static void h7_span_set_free_slot_next(H7Span* span,
                                       uint32_t index,
                                       uint32_t next) {
  *h7_span_free_slot_next_cell(span, index) = next;
}

static uint32_t h7_span_pop_free_slot(H7Span* span) {
  uint32_t index = span->free_head;
  if (index != H7_FREE_NONE) {
    span->free_head = h7_span_free_slot_next(span, index);
  }
  return index;
}

static void h7_span_push_free_slot(H7Span* span, uint32_t index) {
  h7_span_set_free_slot_next(span, index, span->free_head);
  span->free_head = index;
}

static void h7_list_remove(H7Span** head, H7Span* span) {
  if (!span) {
    return;
  }
  if (span->prev) {
    span->prev->next = span->next;
  } else if (*head == span) {
    *head = span->next;
  }
  if (span->next) {
    span->next->prev = span->prev;
  }
  span->next = 0;
  span->prev = 0;
}

static void h7_list_push(H7Span** head, H7Span* span) {
  span->prev = 0;
  span->next = *head;
  if (*head) {
    (*head)->prev = span;
  }
  *head = span;
}

static H7Span* h7_empty_span_pop(H7Class* klass) {
  H7Span* span = klass->empty;
  if (!span) {
    return 0;
  }
  klass->empty = span->next;
  if (klass->empty) {
    klass->empty->prev = 0;
  }
  span->next = 0;
  span->prev = 0;
  --klass->empty_count;
  return span;
}

static int h7_empty_span_try_push(H7Class* klass, H7Span* span) {
  if (klass->empty_count >= H7_EMPTY_SPAN_CAP) {
    return 0;
  }
  h7_list_push(&klass->empty, span);
  ++klass->empty_count;
  return 1;
}

static int h7_class_for_size(size_t size) {
  size_t count = sizeof(g_h7_classes) / sizeof(g_h7_classes[0]);
  size_t i;
  for (i = 0; i < count; ++i) {
    if (size <= g_h7_classes[i].slot_size) {
      return (int)i;
    }
  }
  return -1;
}

static uint32_t h7_span_slot_count(uint32_t slot_size,
                                   uint32_t* out_bitmap_words,
                                   uint32_t* out_slot_offset) {
  size_t header = sizeof(H7Span);
  size_t max_slots = (H7_SPAN_BYTES - h7_align_up(header, slot_size)) /
                     slot_size;
  for (;;) {
    size_t bitmap_words = (max_slots + 63u) / 64u;
    size_t slot_offset =
        h7_align_up(header + bitmap_words * sizeof(uint64_t), slot_size);
    size_t slot_count = (H7_SPAN_BYTES - slot_offset) / slot_size;
    if (slot_count == max_slots) {
      *out_bitmap_words = (uint32_t)bitmap_words;
      *out_slot_offset = (uint32_t)slot_offset;
      return (uint32_t)slot_count;
    }
    max_slots = slot_count;
  }
}

static void h7_span_prepare_region(H7Span* span, uint16_t class_id) {
  H7Class* klass = &g_h7_classes[class_id];
  uint32_t bitmap_words = 0;
  uint32_t slot_offset = 0;
  uint32_t slot_count =
      h7_span_slot_count(klass->slot_size, &bitmap_words, &slot_offset);
  uint32_t i;
  memset(span, 0, sizeof(H7Span));
  h7_region_header_init(&span->region,
                        H7_REGION_SMALL_SPAN,
                        H7_REGION_ACTIVE,
                        H7_SPAN_BYTES);
  span->class_id = class_id;
  span->slot_size = klass->slot_size;
  span->slot_count = slot_count;
  span->free_head = 0;
  span->bitmap_words = bitmap_words;
  span->slot_offset = slot_offset;
  for (i = 0; i < slot_count; ++i) {
    h7_span_set_free_slot_next(
        span, i, (i + 1u < slot_count) ? i + 1u : H7_FREE_NONE);
  }
}

static void h7_span_mark_committed(H7Span* span) {
  g_h7_stats.reserved_bytes += span->region.region_size;
  ++g_h7_stats.span_count;
}

static void h7_span_mark_released(H7Span* span) {
  g_h7_stats.reserved_bytes -= span->region.region_size;
  --g_h7_stats.span_count;
}

static int h7_span_commit_prepared(H7Span* span) {
  if (!h7_route_register(span, H7_SPAN_BYTES, H7_REGION_SMALL_SPAN)) {
    return 0;
  }
  h7_span_mark_committed(span);
  return 1;
}

static int h7_span_is_prepared_region(H7Span* span) {
  return span && span->region.magic == H7_MAGIC &&
         span->region.kind == H7_REGION_SMALL_SPAN;
}

static void h7_span_detach_for_release(H7Span* span,
                                       H7PendingRelease* release) {
  if (!span) {
    return;
  }
  h7_route_unregister(span);
  h7_span_mark_released(span);
  h7_region_mark_released(&span->region);
  h7_pending_release_set(release, span, span->region.region_size);
}

static void h7_span_mark_slot_active(H7Span* span) {
  ++span->used_count;
  g_h7_stats.active_bytes += span->slot_size;
}

static void h7_span_mark_slot_inactive(H7Span* span) {
  --span->used_count;
  g_h7_stats.active_bytes -= span->slot_size;
}

static void h7_span_update_partial_after_alloc(H7Class* klass, H7Span* span) {
  if (h7_span_is_full(span)) {
    h7_list_remove(&klass->partial, span);
    return;
  }
  if (span->used_count == 1u) {
    h7_list_push(&klass->partial, span);
  }
}

static void h7_span_update_partial_after_free(H7Class* klass,
                                              H7Span* span,
                                              int was_full,
                                              H7PendingRelease* release) {
  if (!h7_span_is_empty(span)) {
    if (was_full) {
      h7_list_push(&klass->partial, span);
    }
    return;
  }
  h7_list_remove(&klass->partial, span);
  if (!h7_empty_span_try_push(klass, span)) {
    h7_span_detach_for_release(span, release);
  }
}

static void* h7_small_alloc_from_span(H7Span* span) {
  H7Class* klass = &g_h7_classes[span->class_id];
  uint32_t index = h7_span_pop_free_slot(span);
  unsigned char* ptr;
  if (index == H7_FREE_NONE) {
    return 0;
  }
  ptr = h7_span_slot_ptr(span, index);
  h7_bitmap_set(span, index);
  h7_span_mark_slot_active(span);
  h7_span_update_partial_after_alloc(klass, span);
  return ptr;
}

static void* h7_small_alloc_existing(size_t size) {
  int class_id = h7_class_for_size(size);
  H7Class* klass;
  H7Span* span;
  if (class_id < 0) {
    return 0;
  }
  klass = &g_h7_classes[class_id];
  span = klass->partial;
  if (!span) {
    span = h7_empty_span_pop(klass);
  }
  if (!span) {
    return 0;
  }
  return h7_small_alloc_from_span(span);
}

static void* h7_small_commit_and_alloc(H7Span* prepared) {
  if (!h7_span_is_prepared_region(prepared)) {
    return 0;
  }
  if (!h7_span_commit_prepared(prepared)) {
    return 0;
  }
  return h7_small_alloc_from_span(prepared);
}

static void h7_small_free(H7Span* span,
                          void* ptr,
                          H7PendingRelease* release) {
  H7Class* klass;
  uint32_t index;
  int was_full;
  if (!span || span->region.magic != H7_MAGIC ||
      span->region.kind != H7_REGION_SMALL_SPAN ||
      span->class_id >= sizeof(g_h7_classes) / sizeof(g_h7_classes[0])) {
    return;
  }
  klass = &g_h7_classes[span->class_id];
  if (!h7_small_slot_index(span, ptr, &index)) {
    return;
  }
  was_full = h7_span_is_full(span);
  h7_bitmap_clear(span, index);
  h7_span_push_free_slot(span, index);
  h7_span_mark_slot_inactive(span);
  h7_span_update_partial_after_free(klass, span, was_full, release);
}
