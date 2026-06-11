static void h7_run_v3_span_sequence(uint32_t iters) {
  h7_run_malloc_free("small64", 64u, iters);
  h7_run_size_rows_malloc_free(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_malloc_free("direct32k", 32768u, iters);

  h7_run_size_rows_route_invariant(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_size_rows_route_valid(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_size_rows_route_invalid(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);

  h7_run_route_valid("small64", 64u, iters);
  h7_run_route_valid("direct32k", 32768u, iters);

  h7_run_route_invalid("small64", 64u, iters);
  h7_run_route_invalid("direct32k", 32768u, iters);
}

static void h7_run_v3_fresh_sequence(uint32_t iters) {
  h7_run_random_toggle("fresh_small_ws400", 16u, 2048u, iters, 400u, 1u);
  h7_run_random_toggle("fresh_medium_ws400", 4096u, 32768u, iters, 400u, 1u);
  h7_run_random_toggle("fresh_mixed_ws400", 16u, 32768u, iters, 400u, 1u);
  h7_run_random_toggle("fresh_small_ws400_notouch", 16u, 2048u, iters, 400u, 0u);
  h7_run_random_toggle("fresh_medium_ws400_notouch", 4096u, 32768u, iters, 400u, 0u);
  h7_run_random_toggle("fresh_mixed_ws400_notouch", 16u, 32768u, iters, 400u, 0u);
}

static void h7_run_v3_batch_sequence(uint32_t iters) {
  h7_run_malloc_batch_free("small64", 64u, iters);
  h7_run_size_rows_malloc_batch_free(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_malloc_batch_free("direct32k", 32768u, iters);

  h7_run_free_batch("small64", 64u, iters);
  h7_run_size_rows_free_batch(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_free_batch("direct32k", 32768u, iters);
}

static void h7_run_v3_retained_sequence(uint32_t iters) {
  h7_run_free_retained_loop("small64", 64u, iters);
  h7_run_size_rows_free_retained_loop(kSpanRows, sizeof(kSpanRows) / sizeof(kSpanRows[0]), iters);
  h7_run_free_retained_loop("direct32k", 32768u, iters);
}

static void h7_run_v3_mixed_sequence(uint32_t iters) {
  h7_run_mixed_steady("small_ws400", 16u, 2048u, iters, 400u);
  h7_run_mixed_steady("span_medium_ws400", 4096u, 16384u, iters, 400u);
  h7_run_mixed_steady("direct_medium_ws400", 16385u, 32768u, iters, 400u);
  h7_run_mixed_steady("medium_ws400", 4096u, 32768u, iters, 400u);
  h7_run_mixed_steady("mixed_ws400", 16u, 32768u, iters, 400u);

  h7_run_random_toggle("small_ws400", 16u, 2048u, iters, 400u, 1u);
  h7_run_random_toggle("medium_ws400", 4096u, 32768u, iters, 400u, 1u);
  h7_run_random_toggle("mixed_ws400", 16u, 32768u, iters, 400u, 1u);
}
