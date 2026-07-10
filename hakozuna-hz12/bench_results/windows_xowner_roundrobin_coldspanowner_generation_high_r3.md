# HZ12 Windows Cross-Owner Round-Robin ColdSpanOwnerCompare

- Generated: 2026-07-10T22:02:06.2261843+09:00
- Git HEAD: `2abb6a983985c84fd4002c6f22e144d39865a1dd`
- Compiler: `clang version 18.1.8 (https://github.com/llvm/llvm-project.git 3b5b5c1ec4a3095ab096dd780e84d7ab81f3d7ff)`
- Rounds: 3; runtime: 5s
- Producers/consumers: 4/4; ring: 4096
- Size: 8..1024; cross-owner free target: 100%
- Logical CPUs: 32; affinity mask: `0xFF`
- Priority class: High
- Order: rotated once per round; cooldown: 250ms

## Build Manifest

| Row | Bytes | Last write | SHA-256 | Build flags / identity |
| --- | ---: | --- | --- | --- |
| hz12-flushroute-integrated | 18944 | 2026-07-10T22:00:22.8059415+09:00 | `EFDE3765F57D976CF24750B43E69F5F9B9CF93402082B8AAC70E26D0747BC699` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on owner_fast_load=on accounting=off diag_counters=off drain_pace=256 |
| hz12-coldspanowner | 20480 | 2026-07-10T22:00:28.4919367+09:00 | `157A03F1FF93B3B67E397BD61982D893F4BA2BF0960C43F6906F38583B24C4B7` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on cold_span_owner=on accounting=off diag_counters=off |
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T21:54:29.1606070+09:00 | `D0D9F468E08DB92041F80E2015D94CC26E47C33EBD5CD5B27E4E8BCED83CB953` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12800 | 2026-07-10T20:18:42.2407219+09:00 | `14263CDC7536BA0D3F895D9E08CF3B0FD819253E237D02D327F1740CCEC7EAD4` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Peak RSS median | Final RSS median | Runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-flushroute-integrated | 21.778M | 21.389M | 21.778M | 10.01 MiB | 9.86 MiB | 3 |
| hz12-coldspanowner | 29.195M | 27.318M | 29.195M | 14.63 MiB | 14.27 MiB | 3 |
| hz12-owner-inbox-ownerfastload | 37.573M | 35.756M | 37.573M | 15.46 MiB | 15.30 MiB | 3 |
| tcmalloc | 36.510M | 34.971M | 36.510M | 13.87 MiB | 13.28 MiB | 3 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-flushroute-integrated | 21778086.24 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=108947015 cross_owner_frees=108947015 local_cleanup=0 waits=68964155 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21778086.24 elapsed=5.003 peak_rss_mib=10.01 rss_mib=9.86` |
| 1 | 2 | hz12-coldspanowner | 27317757.79 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=136864029 cross_owner_frees=136864029 local_cleanup=0 waits=51524521 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27317757.79 elapsed=5.010 peak_rss_mib=14.63 rss_mib=14.27` |
| 1 | 3 | hz12-owner-inbox-ownerfastload | 37572859.47 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=188241078 cross_owner_frees=188241078 owner_drained=188240062 waits=42878844 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37572859.47 elapsed=5.010 peak_rss_mib=15.46 rss_mib=15.30 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 4 | tcmalloc | 36509731.62 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=182826917 cross_owner_frees=182826917 local_cleanup=0 waits=30370932 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36509731.62 elapsed=5.008 peak_rss_mib=13.87 rss_mib=13.28` |
| 2 | 1 | hz12-coldspanowner | 29195454.36 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=146353753 cross_owner_frees=146353753 local_cleanup=0 waits=50041861 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29195454.36 elapsed=5.013 peak_rss_mib=12.23 rss_mib=11.86` |
| 2 | 2 | hz12-owner-inbox-ownerfastload | 37261006.02 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=186699844 cross_owner_frees=186699844 owner_drained=186699588 waits=45221037 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37261006.02 elapsed=5.011 peak_rss_mib=10.62 rss_mib=10.46 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 3 | tcmalloc | 34970726.79 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=175376831 cross_owner_frees=175376831 local_cleanup=0 waits=31572898 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34970726.79 elapsed=5.015 peak_rss_mib=13.57 rss_mib=12.97` |
| 2 | 4 | hz12-flushroute-integrated | 21653673.02 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=108587393 cross_owner_frees=108587393 local_cleanup=0 waits=66725431 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21653673.02 elapsed=5.015 peak_rss_mib=9.43 rss_mib=9.27` |
| 3 | 1 | hz12-owner-inbox-ownerfastload | 35756383.32 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=179258374 cross_owner_frees=179258374 owner_drained=179255661 waits=41783788 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35756383.32 elapsed=5.013 peak_rss_mib=10.77 rss_mib=10.61 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | tcmalloc | 35950329.09 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=180199245 cross_owner_frees=180199245 local_cleanup=0 waits=31285348 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35950329.09 elapsed=5.012 peak_rss_mib=13.75 rss_mib=13.15` |
| 3 | 3 | hz12-flushroute-integrated | 21389336.95 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=107197094 cross_owner_frees=107197094 local_cleanup=0 waits=57053115 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21389336.95 elapsed=5.012 peak_rss_mib=9.99 rss_mib=9.84` |
| 3 | 4 | hz12-coldspanowner | 28368766 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=141881308 cross_owner_frees=141881308 local_cleanup=0 waits=49531393 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28368766.00 elapsed=5.001 peak_rss_mib=13.17 rss_mib=12.80` |
