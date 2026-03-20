# Linux arm64 Compare Results (2026-03-20)

This is the first saved Ubuntu arm64 allocator-comparison pass for the current
Linux compare lane. It captures the validated compare runner after the local
allocator cache prep landed.

## Environment

- Host: Ubuntu arm64 (aarch64)
- Lane: Linux arm64 compare
- Benchmark binary: `bench/out/linux/arm64/bench_mixed_ws_crt`
- Allocators: `system`, `hz3`, `hz4`, `mimalloc`, `tcmalloc`
- Raw logs: `private/raw-results/linux/compare_20260320_094616`

## Repro

```bash
./linux/run_linux_arm64_bench_compare.sh
```

This wrapper prepares the local `mimalloc` / `tcmalloc` caches and builds the
compare binary with the current default bench shape:

- runs: `3`
- bench-args: `4 1000000 8192 16 1024`

## Results

Measured median ops/s from the 3-run pass:

| Allocator | ops/s |
|---|---:|
| system | 101,784,537.402 |
| hz3 | 258,813,740.163 |
| hz4 | 244,498,717.726 |
| mimalloc | 312,086,241.912 |
| tcmalloc | 334,052,102.106 |

## Takeaways

- `tcmalloc` led this pass.
- `mimalloc` was next.
- `hz3` and `hz4` both beat `system` on this workload shape.
- `hz3` was slightly ahead of `hz4` in the median pass.

## Notes

- Keep arm64 results separate from x86_64.
- Use [docs/benchmarks/linux/TEMPLATE_ARM64_COMPARE.md](./linux/TEMPLATE_ARM64_COMPARE.md)
  for future same-lane saves.
- This is a compare-lane snapshot, not the paper-grade Linux suite.
