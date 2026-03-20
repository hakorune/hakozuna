# Linux arm64 benchmark template (compare lane)

Use this template for Linux arm64 allocator comparisons based on the compare
lane validated in this repo.

## Summary

- Date: <YYYY-MM-DD>
- CPU: <CPU model>
- Kernel: <kernel version>
- OS: <Ubuntu version>
- Lane: Linux arm64 compare (bench_mixed_ws_crt)

## Build

```bash
./linux/build_linux_arm64_bench_compare.sh
```

Or run the compare wrapper which builds and prepares allocators:

```bash
./linux/run_linux_arm64_bench_compare.sh --runs <N> --bench-args "<threads> <iters> <ws> <min> <max>"
```

## Inputs

- Benchmark binary: `bench/out/linux/arm64/bench_mixed_ws_crt`
- Allocators: `system`, `hz3`, `hz4`, `mimalloc`, `tcmalloc`
- Raw logs: `private/raw-results/linux/`

## Command

```bash
./linux/run_linux_arm64_bench_compare.sh \
  --runs <N> \
  --bench-args "<threads> <iters> <ws> <min> <max>" \
  --outdir <path>
```

## Results (median)

| Allocator | ops/s | Notes |
|---|---|---|
| system | <value> | <notes> |
| hz3 | <value> | <notes> |
| hz4 | <value> | <notes> |
| mimalloc | <value> | <notes> |
| tcmalloc | <value> | <notes> |

## Notes

- Record the full `bench-args` and the allocator list used.
- Keep arm64 results separate from x86_64.
