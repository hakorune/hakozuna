#include "hz10_freelist_page.h"
#include "hz10_platform.h"
#include "hz10_public_entry_owner.h"
#include "hz10_size_class.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static _Thread_local int hz10_probe_in_dump;

static void hz10_probe_write_all(const char* text, size_t len) {
  while (len > 0u) {
    ssize_t n = write(STDERR_FILENO, text, len);
    if (n <= 0) {
      return;
    }
    text += (size_t)n;
    len -= (size_t)n;
  }
}

static void hz10_probe_writef(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) {
    return;
  }
  size_t len = (size_t)n;
  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1u;
  }
  hz10_probe_write_all(buf, len);
}

void hz10_shim_dump_orphan_registry_probe(void) {
  if (hz10_probe_in_dump) return;
  hz10_probe_in_dump = 1;
  uint64_t now_ns = hz10_platform_now_ns();
  uint64_t total_depth = 0u, total_idle = 0u, total_live = 0u;
  uint64_t total_hidden = 0u, total_age_lt_100ms = 0u, total_age_lt_1s = 0u;
  uint64_t total_age_lt_4s = 0u, total_age_lt_16s = 0u, total_age_ge_16s = 0u;

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10OrphanRegistryProbeClassStats stats = {0};
    hz10_public_entry_orphan_registry_probe_class_stats(c, now_ns, &stats);
    if (stats.depth == 0u) continue;
    total_depth += stats.depth;
    total_idle += stats.idle_pages;
    total_live += stats.live_pinned_pages;
    total_hidden += stats.hidden_pending_slots;
    total_age_lt_100ms += stats.age_lt_100ms;
    total_age_lt_1s += stats.age_lt_1s;
    total_age_lt_4s += stats.age_lt_4s;
    total_age_lt_16s += stats.age_lt_16s;
    total_age_ge_16s += stats.age_ge_16s;
    hz10_probe_writef(
        "hz10_shim_orphan_registry_probe class=%u slot_size=%u slot_count=%u "
        "depth=%llu idle_pages=%llu live_pinned_pages=%llu "
        "hidden_pending_slots=%llu age_lt_100ms=%llu age_lt_1s=%llu "
        "age_lt_4s=%llu age_lt_16s=%llu age_ge_16s=%llu\n",
        (unsigned)c, (unsigned)hz10_size_class_slot_size(c),
        (unsigned)hz10_size_class_slot_count(c),
        (unsigned long long)stats.depth,
        (unsigned long long)stats.idle_pages,
        (unsigned long long)stats.live_pinned_pages,
        (unsigned long long)stats.hidden_pending_slots,
        (unsigned long long)stats.age_lt_100ms,
        (unsigned long long)stats.age_lt_1s,
        (unsigned long long)stats.age_lt_4s,
        (unsigned long long)stats.age_lt_16s,
        (unsigned long long)stats.age_ge_16s);
  }

  hz10_probe_writef(
      "hz10_shim_orphan_registry_probe totals depth=%llu idle_pages=%llu "
      "live_pinned_pages=%llu hidden_pending_slots=%llu age_lt_100ms=%llu "
      "age_lt_1s=%llu age_lt_4s=%llu age_lt_16s=%llu age_ge_16s=%llu\n",
      (unsigned long long)total_depth, (unsigned long long)total_idle,
      (unsigned long long)total_live, (unsigned long long)total_hidden,
      (unsigned long long)total_age_lt_100ms,
      (unsigned long long)total_age_lt_1s,
      (unsigned long long)total_age_lt_4s,
      (unsigned long long)total_age_lt_16s,
      (unsigned long long)total_age_ge_16s);
  hz10_probe_in_dump = 0;
}

void hz10_shim_dump_orphan_registry_drain_probe(void) {
  if (hz10_probe_in_dump) return;
  hz10_probe_in_dump = 1;
  uint64_t total_depth = 0u, total_already_idle = 0u, total_drain_idle = 0u;
  uint64_t total_drain_capacity = 0u, total_truly_live = 0u;
  uint64_t total_skipped_live_owner = 0u, total_pending_before = 0u;
  uint64_t total_merged = 0u;

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10OrphanRegistryDrainProbeClassStats stats = {0};
    hz10_public_entry_orphan_registry_drain_probe_class_stats(c, &stats);
    if (stats.depth == 0u) continue;
    total_depth += stats.depth;
    total_already_idle += stats.already_idle_pages;
    total_drain_idle += stats.drain_idle_pages;
    total_drain_capacity += stats.drain_capacity_pages;
    total_truly_live += stats.truly_live_pinned_pages;
    total_skipped_live_owner += stats.skipped_live_owner_pages;
    total_pending_before += stats.pending_before_slots;
    total_merged += stats.merged_slots;
    hz10_probe_writef(
        "hz10_shim_orphan_registry_drain_probe class=%u slot_size=%u "
        "slot_count=%u depth=%llu already_idle_pages=%llu "
        "drain_idle_pages=%llu drain_capacity_pages=%llu "
        "truly_live_pinned_pages=%llu skipped_live_owner_pages=%llu "
        "pending_before_slots=%llu merged_slots=%llu drain_idle_bytes=%llu\n",
        (unsigned)c, (unsigned)hz10_size_class_slot_size(c),
        (unsigned)hz10_size_class_slot_count(c),
        (unsigned long long)stats.depth,
        (unsigned long long)stats.already_idle_pages,
        (unsigned long long)stats.drain_idle_pages,
        (unsigned long long)stats.drain_capacity_pages,
        (unsigned long long)stats.truly_live_pinned_pages,
        (unsigned long long)stats.skipped_live_owner_pages,
        (unsigned long long)stats.pending_before_slots,
        (unsigned long long)stats.merged_slots,
        (unsigned long long)(stats.drain_idle_pages * HZ10_PAGE_QUANTUM));
  }

  hz10_probe_writef(
      "hz10_shim_orphan_registry_drain_probe totals depth=%llu "
      "already_idle_pages=%llu drain_idle_pages=%llu "
      "drain_capacity_pages=%llu truly_live_pinned_pages=%llu "
      "skipped_live_owner_pages=%llu pending_before_slots=%llu "
      "merged_slots=%llu drain_idle_bytes=%llu\n",
      (unsigned long long)total_depth,
      (unsigned long long)total_already_idle,
      (unsigned long long)total_drain_idle,
      (unsigned long long)total_drain_capacity,
      (unsigned long long)total_truly_live,
      (unsigned long long)total_skipped_live_owner,
      (unsigned long long)total_pending_before,
      (unsigned long long)total_merged,
      (unsigned long long)(total_drain_idle * HZ10_PAGE_QUANTUM));
  hz10_probe_in_dump = 0;
}
