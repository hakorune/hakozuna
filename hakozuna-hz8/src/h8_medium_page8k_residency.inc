static bool h8_page8k_page_empty(const H8Page8KRemotePage* page) {
  if (atomic_load_explicit(&page->pending_mask, memory_order_acquire) != 0u ||
      atomic_load_explicit(&page->qstate, memory_order_acquire) !=
          H8_PAGE8K_Q_IDLE)
    return false;
  for (size_t i = 0; i < H8_PAGE8K_SLOT_COUNT; ++i) {
    if (atomic_load_explicit(&page->slot_state[i], memory_order_acquire) !=
        H8_PAGE8K_SLOT_FREE)
      return false;
  }
  return true;
}

static void h8_page8k_orphan_resident_remove(H8Page8KRemotePage* page) {
  if (!page->orphan_resident_counted) return;
  H8Page8KRemoteOwner* orphan = h8_page8k_page_owner(page);
  if (orphan && orphan->resident_empty_pages != 0u)
    --orphan->resident_empty_pages;
  page->orphan_resident_counted = false;
  H8_PAGE8K_STAT_STORE(orphan_resident_empty,
                       orphan ? orphan->resident_empty_pages : 0u);
}

static void h8_page8k_orphan_trim(H8Page8KRemotePage* page) {
  H8Page8KRemoteOwner* owner = h8_page8k_page_owner(page);
  if (!owner || owner != g_orphan_owner || !page->resident ||
      page->orphan_resident_counted || !h8_page8k_page_empty(page))
    return;
  if (owner->resident_empty_pages < H8_PAGE8K_ORPHAN_RESIDENT_EMPTY_CAP) {
    ++owner->resident_empty_pages;
    page->orphan_resident_counted = true;
    H8_PAGE8K_STAT_STORE(orphan_resident_empty, owner->resident_empty_pages);
    H8_PAGE8K_STAT_MAX(orphan_resident_empty_max, owner->resident_empty_pages);
    return;
  }
  if (h8_platform_decommit(page->base, H8_PAGE8K_RUN_BYTES) == 0) {
    page->resident = false;
    H8_PAGE8K_STAT_INC(page_decommit);
  } else {
    H8_PAGE8K_STAT_INC(page_decommit_fail);
  }
}

static bool h8_page8k_recommit(H8Page8KRemotePage* page) {
  if (page->resident) return true;
  if (h8_platform_commit(page->base, H8_PAGE8K_RUN_BYTES) != 0) return false;
  page->resident = true;
  H8_PAGE8K_STAT_INC(page_recommit);
  return true;
}
