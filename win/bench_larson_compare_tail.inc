#if defined(HZ_BENCH_USE_HZ6)
    bench_print_larson_hz6_summary(&hz6_stats);
#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
    print_hz6_front_alloc_paths(&hz6_stats);
    print_hz6_front_prefill_paths(&hz6_stats);
    print_hz6_frontcache_class_diag(&hz6_stats);
#endif
#else
    hz_bench_dump_stats(stdout, "larson_main_final");
#if defined(HZ_BENCH_USE_HZ11) && HZ_BENCH_HZ11_SUMMARY
    hz11_class_diag_dump_stats();
    hz11_matrix_diag_dump_stats();
#endif
#endif
    printf("Done sleeping...\n");

cleanup:
    if (handles) {
        for (size_t i = 0; i < threads; ++i) {
            if (handles[i]) {
                CloseHandle(handles[i]);
                handles[i] = NULL;
            }
        }
    }
    if (all_blocks) {
        for (size_t i = 0; i < total_slots; ++i) {
            if (all_blocks[i]) {
                bench_free(all_blocks[i]);
                all_blocks[i] = NULL;
            }
        }
    }
    free(all_sizes);
    free(all_blocks);
    free(handles);
    free(tds);
    hz_bench_allocator_thread_teardown();
    return exit_code;
}
