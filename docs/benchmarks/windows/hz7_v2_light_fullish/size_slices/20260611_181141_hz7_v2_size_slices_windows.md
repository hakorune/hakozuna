# HZ7 v2 Windows Size Slices

Generated: 2026-06-11 18:11:41 +09:00

- benchmark: `bench_random_mixed_compare`
- allocator: `hz7-v2`
- runs: 1
- iters: 20000000
- working_set: 400
- direct_retain_cap: default
- empty_span_cap: default
- purpose: split medium into span-covered and direct-retained slices

| profile | range | note | median ops/s | median peak_kb | runs |
| --- | --- | --- | ---: | ---: | --- |
| small_16_2k | 16..2048 | small span-heavy control | 76.283M | 4600 | 76.283M / 4,600 KB |
| span_4k_16k | 4096..16384 | medium slice still covered by spans | 41.514M | 5264 | 41.514M / 5,264 KB |
| direct_16k_32k | 16385..32768 | medium slice crossing into direct retained regions | 92.901M | 4860 | 92.901M / 4,860 KB |
| mixed_16_32k | 16..32768 | paper mixed range | 48.747M | 5724 | 48.747M / 5,724 KB |

Artifacts: .\docs\benchmarks\windows\hz7_v2_light_fullish\size_slices
