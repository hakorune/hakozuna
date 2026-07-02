H8DebugStats h8_debug_stats(void) {
  H8DebugStats stats;
  memset(&stats, 0, sizeof(stats));
  h8_debug_stats_snapshot(&stats);
  return stats;
}
