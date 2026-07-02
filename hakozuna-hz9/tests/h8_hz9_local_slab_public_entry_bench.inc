static int h9_lsp_run_public_entry_mode(const char* mode, uint32_t class_id,
                                        uint32_t slot_size, uint64_t iters,
                                        bool touch, bool non_lifo) {
  H9LspRouteLeafBenchResult result = {0};
  double start = now_seconds();
  bool ok = h9_lsp_debug_publicentry_bench(slot_size, iters, touch, non_lifo,
                                           &result);
  double elapsed = now_seconds() - start;
  if (!ok) {
    fprintf(stderr, "lsp public entry bench failed\n");
    return 3;
  }
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  H9LspStats stats = h9_lsp_debug_stats();
  printf("mode=%s class=%u iters=%llu touch=%u ops_per_s=%.2f "
         "elapsed=%.6f route_valid=%zu route_invalid=%zu route_miss=%zu "
         "malloc_hit=0 ptr_fast=%llu ptr_fallback=%zu "
         "state_mismatch=%llu segment_create=%zu sink=%" PRIuPTR "\n",
         mode, class_id, (unsigned long long)iters, touch ? 1u : 0u,
         (double)result.ok / elapsed, elapsed, stats.route_valid,
         stats.route_invalid, stats.route_miss,
         (unsigned long long)result.ok - stats.route_valid, stats.route_valid,
         (unsigned long long)result.state_mismatch, stats.segment_create,
         result.sink);
  h9_lsp_debug_reset();
  return 0;
}
