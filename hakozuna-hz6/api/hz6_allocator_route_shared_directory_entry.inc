#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
static void hz6_shared_route_directory_entry_begin_update(
    Hz6SharedRouteDirectoryEntry* entry) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  unsigned int sequence =
      atomic_load_explicit(&entry->sequence, memory_order_relaxed);
  if ((sequence & 1u) == 0u) {
    ++sequence;
  } else {
    sequence += 2u;
  }
  atomic_store_explicit(&entry->sequence, sequence, memory_order_release);
#else
  (void)entry;
#endif
}

static void hz6_shared_route_directory_entry_end_update(
    Hz6SharedRouteDirectoryEntry* entry) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  unsigned int sequence =
      atomic_load_explicit(&entry->sequence, memory_order_relaxed);
  if ((sequence & 1u) != 0u) {
    ++sequence;
  } else {
    sequence += 2u;
  }
  atomic_store_explicit(&entry->sequence, sequence, memory_order_release);
#else
  (void)entry;
#endif
}

static void hz6_shared_route_directory_entry_publish(
    Hz6SharedRouteDirectoryEntry* entry,
    Hz6Allocator* allocator,
    void* base,
  uint16_t front_id,
  uint16_t class_id,
    uint32_t generation,
    void* descriptor) {
  hz6_shared_route_directory_entry_begin_update(entry);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
  atomic_store_explicit(&entry->descriptor, (uintptr_t)descriptor,
                        memory_order_release);
  atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
  atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
  atomic_store_explicit(&entry->generation, generation, memory_order_release);
  atomic_store_explicit(&entry->owner_slot, allocator->owner.token.slot,
                        memory_order_release);
  atomic_store_explicit(&entry->owner_generation,
                        allocator->owner.token.generation,
                        memory_order_release);
  atomic_store_explicit(&entry->base, (uintptr_t)base, memory_order_release);
#else
  atomic_store_explicit(&entry->base, (uintptr_t)base, memory_order_release);
  atomic_store_explicit(&entry->descriptor, (uintptr_t)descriptor,
                        memory_order_release);
  atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
  atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
  atomic_store_explicit(&entry->generation, generation, memory_order_release);
  atomic_store_explicit(&entry->owner_slot, allocator->owner.token.slot,
                        memory_order_release);
  atomic_store_explicit(&entry->owner_generation,
                        allocator->owner.token.generation,
                        memory_order_release);
  atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
#endif
  hz6_shared_route_directory_entry_end_update(entry);
}

#endif

static Hz6SharedRouteLookup hz6_shared_route_directory_lookup_result(
    Hz6SharedRouteLookupStatus status,
    Hz6RouteResult route) {
  Hz6SharedRouteLookup result;
  result.status = status;
  result.route = route;
  return result;
}

static Hz6SharedRouteLookup hz6_shared_route_directory_lookup_miss(void) {
  return hz6_shared_route_directory_lookup_result(
      HZ6_SHARED_ROUTE_LOOKUP_MISS, hz6_route_miss());
}
