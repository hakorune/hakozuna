# HZ8 Small Transition Inventory Matched Remote Gate

Generated: 2026-07-14 15:32:51 +09:00

- mode: fresh-process `A-B-B-A`, matched remote publication, no ring-full local fallback
- blocks: `20`; samples/lane: `40`; admissible pairs: `40`
- allocator behavior flags: unchanged

| allocator | median ops/s | actual remote | fallback | post working set | peak working set | private usage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| default | 62.379M | 90.00% | 0.00% | 13,460 KiB | 13,720 KiB | 52,226 KiB |
| inventory | 62.200M | 90.00% | 0.00% | 13,476 KiB | 13,740 KiB | 52,980 KiB |

- independent-median throughput delta: `-0.29%`
- paired median throughput delta: `+1.63%`
- paired median post/peak/private deltas: `+0.16% / +0.16% / -0.86%`
- promotion gate: `PASS`

Artifacts: `20260714_153251_hz8_transition_inventory_matched_remote.csv`, `20260714_153251_hz8_transition_inventory_matched_remote.log`
