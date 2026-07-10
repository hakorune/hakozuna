#if HZ12_CENTRAL_SPAN_RETURN || HZ12_CENTRAL_SPAN_REUSE
static int hz12_span_id_for_ptr(const void* ptr, uint32_t* span_id) {
  if (!hz12_arena_base || !ptr || !span_id) return 0;
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz12_arena_base;
  uintptr_t off = p - base;
  if (p < base || off >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  *span_id = (uint32_t)(off >> HZ12_SPAN_SHIFT);
  return 1;
}

static void hz12_span_return_ensure_meta_locked(uint32_t span_id) {
  H12SpanReturnMeta* meta = &hz12_span_return_meta[span_id];
  if (meta->slot_count != 0u) return;
  uint8_t c1 = hz12_span_class[span_id];
  if (c1 == 0u) abort();
  uint8_t class_id = (uint8_t)(c1 - 1u);
  meta->class_id = class_id;
  meta->slot_count = (uint32_t)(HZ12_SPAN_BYTES / hz12_class_slot_size(class_id));
  meta->central_free_count = 0u;
  meta->transfer_count = 0u;
  meta->state = HZ12_SPAN_RETURN_ACTIVE;
}

#if HZ12_CENTRAL_SPAN_RETURN
static void hz12_span_return_account_transfer(void* ptr, int delta) {
  uint32_t span_id;
  if (!hz12_span_id_for_ptr(ptr, &span_id)) abort();
  uint8_t diag_class = 0u;
  if (hz12_span_class[span_id] != 0u) {
    diag_class = (uint8_t)(hz12_span_class[span_id] - 1u);
  }
  hz12_span_source_diag_meta_lock(diag_class);
  pthread_mutex_lock(&hz12_span_return_meta_lock);
  hz12_span_return_ensure_meta_locked(span_id);
  H12SpanReturnMeta* meta = &hz12_span_return_meta[span_id];
  if (delta > 0) {
    meta->transfer_count += 1u;
  } else {
    if (meta->transfer_count == 0u) {
      pthread_mutex_unlock(&hz12_span_return_meta_lock);
      abort();
    }
    meta->transfer_count -= 1u;
  }
  pthread_mutex_unlock(&hz12_span_return_meta_lock);
}
#endif

static void hz12_reusable_span_push(uint8_t class_id, uint32_t span_id) {
  H12ReusableSpanStack* rs = &hz12_reusable_spans[class_id];
  pthread_mutex_lock(&rs->lock);
  if (rs->count >= HZ12_REUSABLE_SPAN_CAP) {
    pthread_mutex_unlock(&rs->lock);
    abort();
  }
  rs->span_ids[rs->count++] = span_id;
  pthread_mutex_unlock(&rs->lock);
}

static void hz12_central_try_return_full_span_locked(uint8_t class_id,
                                                     H12CentralStack* cs,
                                                     uint32_t span_id) {
  H12SpanReturnMeta* meta = &hz12_span_return_meta[span_id];
#if HZ12_CENTRAL_SPAN_RETURN
  if (meta->central_free_count != meta->slot_count ||
      meta->transfer_count != 0u || meta->slot_count == 0u) {
#else
  if (meta->central_free_count != meta->slot_count || meta->slot_count == 0u) {
#endif
    meta->state = HZ12_SPAN_RETURN_CENTRAL_PARTIAL;
    atomic_fetch_add_explicit(&hz12_central_partial_span_count, 1u,
                              memory_order_relaxed);
    return;
  }

  uint32_t write = 0u;
  uint32_t removed = 0u;
  for (uint32_t read = 0u; read < cs->count; ++read) {
    uint32_t item_span_id = UINT32_MAX;
    if (hz12_span_id_for_ptr(cs->slots[read], &item_span_id) &&
        item_span_id == span_id) {
      ++removed;
      continue;
    }
    cs->slots[write++] = cs->slots[read];
  }
  if (removed != meta->slot_count) abort();
  cs->count = write;
  meta->central_free_count = 0u;
  meta->state = HZ12_SPAN_RETURN_REUSABLE;
#if HZ12_CENTRAL_SPAN_RETURN
  atomic_fetch_sub_explicit(&hz12_central_object_count, removed,
                            memory_order_relaxed);
#endif
  atomic_fetch_add_explicit(&hz12_span_return_count, 1u, memory_order_relaxed);
  atomic_fetch_add_explicit(&hz12_central_full_span_count, 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&hz12_span_return_by_class[class_id], 1u,
                            memory_order_relaxed);
  hz12_reusable_span_push(class_id, span_id);
}

#if HZ12_CENTRAL_SPAN_RETURN
static void hz12_central_sweep_full_spans_locked(uint8_t class_id,
                                                 H12CentralStack* cs) {
  uint32_t i = 0u;
  while (i < cs->count) {
    uint32_t before_scan_count = cs->count;
    uint32_t span_id;
    if (!hz12_span_id_for_ptr(cs->slots[i], &span_id)) {
      ++i;
      continue;
    }
    hz12_span_source_diag_meta_lock(class_id);
    pthread_mutex_lock(&hz12_span_return_meta_lock);
    H12SpanReturnMeta* meta = &hz12_span_return_meta[span_id];
    uint32_t before = cs->count;
    if (meta->state != HZ12_SPAN_RETURN_REUSABLE) {
      hz12_central_try_return_full_span_locked(class_id, cs, span_id);
    }
    pthread_mutex_unlock(&hz12_span_return_meta_lock);
    i = cs->count < before ? 0u : i + 1u;
    hz12_span_source_diag_sweep(class_id, before_scan_count);
  }
}
#endif
#endif
