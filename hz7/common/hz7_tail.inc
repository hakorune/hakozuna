static H7RouteKind h7_route_unlocked(void* ptr) {
  H7RouteResult route = h7_route_lookup_raw(ptr);
  return h7_region_user_route_kind(route, ptr);
}

H7RouteKind h7_route(void* ptr) {
  H7RouteKind kind;
  h7_lock();
  kind = h7_route_unlocked(ptr);
  h7_unlock();
  return kind;
}

static H7Stats h7_stats_locked(void) {
  H7Stats stats = g_h7_stats;
  stats.route_capacity = H7_ROUTE_CAPACITY;
  stats.empty_span_cap = H7_EMPTY_SPAN_CAP;
  stats.direct_retain_cap = H7_DIRECT_RETAIN_CAP;
#ifdef H7_REMOTE_NATURAL_PRESET
  stats.remote_natural_preset = 1u;
#else
  stats.remote_natural_preset = 0u;
#endif
  return stats;
}

H7Stats h7_stats(void) {
  H7Stats stats;
  h7_lock();
  stats = h7_stats_locked();
  h7_unlock();
  return stats;
}
