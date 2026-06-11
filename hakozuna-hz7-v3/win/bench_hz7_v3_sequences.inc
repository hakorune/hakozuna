static void h7_run_v3_span_sequence(uint32_t iters) {
  h7_run_malloc_free("small64", 64u, iters);
  h7_run_size_rows_malloc_free(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_malloc_free(kDirectRetainRows, kDirectRetainRowCount, iters);

  h7_run_size_rows_route_invariant(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_route_valid(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_route_invalid(kSpanAuditRows, kSpanAuditRowCount, iters);

  h7_run_route_valid("small64", 64u, iters);
  h7_run_size_rows_route_valid(kDirectRetainRows, kDirectRetainRowCount, iters);

  h7_run_route_invalid("small64", 64u, iters);
  h7_run_size_rows_route_invalid(kDirectRetainRows, kDirectRetainRowCount, iters);
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
  h7_run_size_rows_malloc_batch_free(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_malloc_batch_free(kDirectRetainRows, kDirectRetainRowCount, iters);

  h7_run_free_batch("small64", 64u, iters);
  h7_run_size_rows_free_batch(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_free_batch(kDirectRetainRows, kDirectRetainRowCount, iters);
}

static void h7_run_v3_retained_sequence(uint32_t iters) {
  h7_run_free_retained_loop("small64", 64u, iters);
  h7_run_size_rows_free_retained_loop(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_free_retained_loop(kDirectRetainRows, kDirectRetainRowCount, iters);
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

typedef void (*H7V3ScenarioFn)(uint32_t iters);

typedef struct H7V3Scenario {
  const char* name;
  H7V3ScenarioFn run;
} H7V3Scenario;

static const H7V3Scenario kV3Scenarios[] = {
    {"span", h7_run_v3_span_sequence},
    {"fresh", h7_run_v3_fresh_sequence},
    {"batch", h7_run_v3_batch_sequence},
    {"retained", h7_run_v3_retained_sequence},
    {"mixed", h7_run_v3_mixed_sequence},
};

static void h7_run_v3_scenarios(uint32_t iters) {
  size_t i;
  for (i = 0; i < sizeof(kV3Scenarios) / sizeof(kV3Scenarios[0]); ++i) {
    kV3Scenarios[i].run(iters);
  }
}
