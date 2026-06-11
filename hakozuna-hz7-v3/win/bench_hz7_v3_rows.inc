typedef struct H7SizeRow {
  const char* label;
  size_t size;
} H7SizeRow;

static const H7SizeRow kSpanRows[] = {
    {"span4k", 4096u},
    {"span8k", 8192u},
    {"span16k", 16384u},
};

static void h7_run_size_rows_malloc_free(const H7SizeRow* rows,
                                         size_t count,
                                         uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_malloc_free(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_route_valid(const H7SizeRow* rows,
                                         size_t count,
                                         uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_route_valid(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_route_invalid(const H7SizeRow* rows,
                                           size_t count,
                                           uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_route_invalid(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_route_invariant(const H7SizeRow* rows,
                                             size_t count,
                                             uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_route_invariant(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_malloc_batch_free(const H7SizeRow* rows,
                                               size_t count,
                                               uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_malloc_batch_free(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_free_batch(const H7SizeRow* rows,
                                        size_t count,
                                        uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_free_batch(rows[i].label, rows[i].size, iters);
  }
}

static void h7_run_size_rows_free_retained_loop(const H7SizeRow* rows,
                                                size_t count,
                                                uint32_t iters) {
  size_t i;
  for (i = 0; i < count; ++i) {
    h7_run_free_retained_loop(rows[i].label, rows[i].size, iters);
  }
}
