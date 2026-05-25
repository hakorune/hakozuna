static void hz5_lowpage64_prepare_ctx_clear(Hz5Lowpage64FreeCtx* ctx) {
  if (!ctx) {
    return;
  }
  ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_MISS;
  ctx->segment_token = NULL;
  ctx->slot_base = NULL;
  ctx->slot_size = 0;
  ctx->slot_index = 0;
  ctx->flags = 0;
}

static void hz5_lowpage64_p43g_count_prepare(int lookup_kind) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_calls, 1);
  if (lookup_kind == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_active, 1);
  } else if (lookup_kind == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_nonactive, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_miss, 1);
  }
#else
  (void)lookup_kind;
#endif
}

int hz5_lowpage64_prepare_free_user(void* ptr, Hz5Lowpage64FreeCtx* ctx) {
  hz5_lowpage64_prepare_ctx_clear(ctx);
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int lookup_kind = hz5_lowpage64_p43_prepare_free_user(ptr, ctx);
#else
  int lookup_kind = hz5_lowpage64_lookup(ptr);
  if (ctx) {
    ctx->lookup_kind = lookup_kind;
  }
#endif
  hz5_lowpage64_p43g_count_prepare(lookup_kind);
  return lookup_kind;
}

int hz5_lowpage64_prepare_free_raw(void* raw, Hz5Lowpage64FreeCtx* ctx) {
  hz5_lowpage64_prepare_ctx_clear(ctx);
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int lookup_kind = hz5_lowpage64_p43_prepare_free_raw(raw, ctx);
#else
  int lookup_kind = hz5_lowpage64_lookup(raw);
  if (ctx) {
    ctx->lookup_kind = lookup_kind;
  }
#endif
  hz5_lowpage64_p43g_count_prepare(lookup_kind);
  return lookup_kind;
}

int hz5_lowpage64_release_p43_token(void* segment_token,
                                    uint32_t slot_index,
                                    void* raw) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  if (!segment_token || !raw) {
    return 0;
  }
  return hz5_lowpage64_p43_release_token(segment_token, slot_index, raw);
#else
  (void)segment_token;
  (void)slot_index;
  (void)raw;
  return 0;
#endif
}

int hz5_lowpage64_may_own(void* ptr) {
  return hz5_lowpage64_lookup(ptr) != HZ5_LOWPAGE64_LOOKUP_MISS;
}

int hz5_lowpage64_active_owns(void* ptr) {
  return hz5_lowpage64_lookup(ptr) == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
}

void hz5_lowpage64_p43g_note_wrapper(int is_p25_source, int raw_match) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  if (is_p25_source) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_source_p25, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_source_other, 1);
  }
  if (!raw_match) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_raw_mismatch, 1);
  }
#else
  (void)is_p25_source;
  (void)raw_match;
#endif
}

void hz5_lowpage64_p43g_note_old_path(void) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_release_old_path_calls, 1);
#endif
}

void hz5_lowpage64_p43g_note_prepared_path(void) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_release_prepared_calls, 1);
#endif
}
