#include "h8_internal.h"

#if defined(H9_OWNER_LOCAL_PAGE_POOL_SHADOW_L0) && \
    defined(H8_ENABLE_DEBUG_STATS)

void h9_owner_page_shadow_note_alloc(H8ThreadCtx* ctx, uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8_DEBUG_INC(h9_owner_page_shadow_alloc_seen);
  H8_DEBUG_INC(h9_owner_page_shadow_alloc_class[class_id]);

  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  if (!active || !h8_medium_run_owned_by_ctx(active, ctx)) {
    return;
  }
  H8_DEBUG_INC(h9_owner_page_shadow_alloc_active_owner);
  if (active->free_mask != 0) {
    H8_DEBUG_INC(h9_owner_page_shadow_alloc_active_nonfull);
  }
}

void h9_owner_page_shadow_note_local_free(H8ThreadCtx* ctx,
                                          H8MediumRun* run) {
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8_DEBUG_INC(h9_owner_page_shadow_local_free_seen);
  H8_DEBUG_INC(h9_owner_page_shadow_local_free_class[run->class_id]);
  if (ctx && ctx->active_medium_runs[run->class_id] == run) {
    H8_DEBUG_INC(h9_owner_page_shadow_pure_local_candidate);
  }
}

void h9_owner_page_shadow_note_remote_free(H8ThreadCtx* ctx,
                                           H8MediumRun* run) {
  (void)ctx;
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8_DEBUG_INC(h9_owner_page_shadow_remote_free_seen);
  H8_DEBUG_INC(h9_owner_page_shadow_remote_free_class[run->class_id]);
}

#endif
