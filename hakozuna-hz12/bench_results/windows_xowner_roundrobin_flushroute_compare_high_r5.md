# HZ12 Windows Cross-Owner Round-Robin FlushRouteCompare

- Generated: 2026-07-10T21:28:42.3803443+09:00
- Git HEAD: `6924a51b994b1688aa79da4ca2a101767dc5c31c`
- Compiler: `clang version 18.1.8 (https://github.com/llvm/llvm-project.git 3b5b5c1ec4a3095ab096dd780e84d7ab81f3d7ff)`
- Rounds: 5; runtime: 5s
- Producers/consumers: 4/4; ring: 4096
- Size: 8..1024; cross-owner free target: 100%
- Logical CPUs: 32; affinity mask: `0xFF`
- Priority class: High
- Order: rotated once per round; cooldown: 250ms

## Build Manifest

| Row | Bytes | Last write | SHA-256 | Build flags / identity |
| --- | ---: | --- | --- | --- |
| hz11-ownerless | 14848 | 2026-07-10T20:18:41.9663699+09:00 | `4408412C3B79BAC116721B00E10B1BFD15F5AA1BF7BBC50DA2EA03EDEC01A145` | /O2 /DNDEBUG HZ_BENCH_USE_HZ11=1 HZ11_CLASSIFY_SPAN=1 HZ11_CACHE_CAP=32(default) |
| hz12-flushroute-integrated | 18432 | 2026-07-10T21:26:45.6858169+09:00 | `596C9CCE5BD0150450332BBD1E02A4DBC2EF4BC2866EAEDEEDE27B86B491374B` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on owner_fast_load=on accounting=off diag_counters=off drain_pace=256 |
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T20:18:47.8750404+09:00 | `22A4E35B7A03256B34E831A5DD2176DD2A449E2DA550F477EE036B88CBBE7364` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12800 | 2026-07-10T20:18:42.2407219+09:00 | `14263CDC7536BA0D3F895D9E08CF3B0FD819253E237D02D327F1740CCEC7EAD4` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Peak RSS median | Final RSS median | Runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-ownerless | 12.939M | 12.817M | 13.250M | 6.61 MiB | 6.46 MiB | 5 |
| hz12-flushroute-integrated | 26.128M | 26.000M | 27.909M | 11.79 MiB | 11.63 MiB | 5 |
| hz12-owner-inbox-ownerfastload | 34.324M | 33.600M | 38.770M | 15.80 MiB | 15.64 MiB | 5 |
| tcmalloc | 36.318M | 35.570M | 37.493M | 13.81 MiB | 13.21 MiB | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz11-ownerless | 13197184.71 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65988431 cross_owner_frees=65988431 local_cleanup=0 waits=43583444 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13197184.71 elapsed=5.000 peak_rss_mib=6.36 rss_mib=6.21` |
| 1 | 2 | hz12-flushroute-integrated | 26624561.2 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=133322397 cross_owner_frees=133322397 local_cleanup=0 waits=58689940 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26624561.20 elapsed=5.007 peak_rss_mib=12.88 rss_mib=12.72` |
| 1 | 3 | hz12-owner-inbox-ownerfastload | 33599501.97 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=168070955 cross_owner_frees=168070955 owner_drained=168068440 waits=52532437 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=33599501.97 elapsed=5.002 peak_rss_mib=11.89 rss_mib=11.73 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 4 | tcmalloc | 36318332.06 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=182051839 cross_owner_frees=182051839 local_cleanup=0 waits=31356223 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36318332.06 elapsed=5.013 peak_rss_mib=13.68 rss_mib=13.09` |
| 2 | 1 | hz12-flushroute-integrated | 26000073.77 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130340676 cross_owner_frees=130340676 local_cleanup=0 waits=58612004 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26000073.77 elapsed=5.013 peak_rss_mib=11.79 rss_mib=11.63` |
| 2 | 2 | hz12-owner-inbox-ownerfastload | 37637823.39 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=188695229 cross_owner_frees=188695229 owner_drained=188689406 waits=40892955 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37637823.39 elapsed=5.013 peak_rss_mib=15.80 rss_mib=15.64 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 3 | tcmalloc | 36700788.68 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=183998270 cross_owner_frees=183998270 local_cleanup=0 waits=28586187 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36700788.68 elapsed=5.013 peak_rss_mib=13.81 rss_mib=13.21` |
| 2 | 4 | hz11-ownerless | 12894471.6 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=64640146 cross_owner_frees=64640146 local_cleanup=0 waits=42848211 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12894471.60 elapsed=5.013 peak_rss_mib=6.61 rss_mib=6.46` |
| 3 | 1 | hz12-owner-inbox-ownerfastload | 38769586.54 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=194331215 cross_owner_frees=194331215 owner_drained=194329438 waits=41106107 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38769586.54 elapsed=5.012 peak_rss_mib=16.57 rss_mib=16.42 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | tcmalloc | 36047933.35 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=180756075 cross_owner_frees=180756075 local_cleanup=0 waits=29690891 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36047933.35 elapsed=5.014 peak_rss_mib=14.56 rss_mib=13.97` |
| 3 | 3 | hz11-ownerless | 13250344.78 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=66401262 cross_owner_frees=66401262 local_cleanup=0 waits=44619531 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13250344.78 elapsed=5.011 peak_rss_mib=6.70 rss_mib=6.54` |
| 3 | 4 | hz12-flushroute-integrated | 27909131.19 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=139835456 cross_owner_frees=139835456 local_cleanup=0 waits=55004453 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27909131.19 elapsed=5.010 peak_rss_mib=16.11 rss_mib=15.96` |
| 4 | 1 | tcmalloc | 35570193.66 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=178213675 cross_owner_frees=178213674 local_cleanup=0 waits=33506746 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35570193.66 elapsed=5.010 peak_rss_mib=14.56 rss_mib=13.96` |
| 4 | 2 | hz11-ownerless | 12939330.96 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=64732062 cross_owner_frees=64732062 local_cleanup=0 waits=44545557 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12939330.96 elapsed=5.003 peak_rss_mib=6.43 rss_mib=6.27` |
| 4 | 3 | hz12-flushroute-integrated | 26057565.04 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130626954 cross_owner_frees=130626954 local_cleanup=0 waits=58074404 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26057565.04 elapsed=5.013 peak_rss_mib=11.61 rss_mib=11.45` |
| 4 | 4 | hz12-owner-inbox-ownerfastload | 34091785.02 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=170993382 cross_owner_frees=170993382 owner_drained=170987232 waits=50867139 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34091785.02 elapsed=5.016 peak_rss_mib=16.57 rss_mib=16.42 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz11-ownerless | 12816784.47 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=64259511 cross_owner_frees=64259511 local_cleanup=0 waits=43084425 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12816784.47 elapsed=5.014 peak_rss_mib=7.29 rss_mib=7.13` |
| 5 | 2 | hz12-flushroute-integrated | 26128137.15 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130660031 cross_owner_frees=130660031 local_cleanup=0 waits=57862433 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26128137.15 elapsed=5.001 peak_rss_mib=9.96 rss_mib=9.81` |
| 5 | 3 | hz12-owner-inbox-ownerfastload | 34323780.64 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=172110025 cross_owner_frees=172110025 owner_drained=172109673 waits=43212287 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34323780.64 elapsed=5.014 peak_rss_mib=11.21 rss_mib=11.06 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 4 | tcmalloc | 37493245.85 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=187695193 cross_owner_frees=187695193 local_cleanup=0 waits=30765690 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37493245.85 elapsed=5.006 peak_rss_mib=13.44 rss_mib=12.96` |
