# HZ12 Windows Cross-Owner Round-Robin OwnerFastCompare

- Generated: 2026-07-10T20:06:41.2134119+09:00
- Git HEAD: `7e8323dadecbd1bfc6897600068bb18d35d1ac2d`
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
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T20:03:54.9364602+09:00 | `FCDE8B6FEC1057E485FD5320640761B2A686A7FBD168F862B07E41E6DDA846D4` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12288 | 2026-07-10T19:57:44.5547783+09:00 | `B95D5DCED617892668DD8335BD9EC63FEE93C5CCA2C11A43AAA9785036B0729B` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-ownerfastload | 35.542M | 26.547M | 36.062M | 5 |
| tcmalloc | 37.064M | 36.606M | 37.880M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-ownerfastload | 36004258.3 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=180450041 cross_owner_frees=180450041 owner_drained=180446358 waits=42833181 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36004258.30 elapsed=5.012 peak_rss_mib=11.48 rss_mib=11.32 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | tcmalloc | 37063822.63 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=185492898 cross_owner_frees=185492898 local_cleanup=0 waits=29725630 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37063822.63 elapsed=5.005` |
| 2 | 1 | tcmalloc | 36606469.67 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=183042686 cross_owner_frees=183042686 local_cleanup=0 waits=29810472 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36606469.67 elapsed=5.000` |
| 2 | 2 | hz12-owner-inbox-ownerfastload | 34391377.04 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=172248782 cross_owner_frees=172248782 owner_drained=172231459 waits=42383987 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34391377.04 elapsed=5.008 peak_rss_mib=16.73 rss_mib=16.57 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-ownerfastload | 35541615.87 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=177758520 cross_owner_frees=177758520 owner_drained=177756746 waits=42667586 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35541615.87 elapsed=5.001 peak_rss_mib=11.46 rss_mib=11.30 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | tcmalloc | 36928904.54 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=184928602 cross_owner_frees=184928602 local_cleanup=0 waits=31179165 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36928904.54 elapsed=5.008` |
| 4 | 1 | tcmalloc | 37596758.55 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=188008084 cross_owner_frees=188008084 local_cleanup=0 waits=29521383 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37596758.55 elapsed=5.001` |
| 4 | 2 | hz12-owner-inbox-ownerfastload | 26546521.94 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=132980310 cross_owner_frees=132980310 owner_drained=132975874 waits=48205407 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26546521.94 elapsed=5.009 peak_rss_mib=13.38 rss_mib=13.23 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-ownerfastload | 36061559.17 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=180483971 cross_owner_frees=180483971 owner_drained=180475973 waits=50207336 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36061559.17 elapsed=5.005 peak_rss_mib=11.44 rss_mib=11.28 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | tcmalloc | 37879527.18 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=189701657 cross_owner_frees=189701657 local_cleanup=0 waits=29131927 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37879527.18 elapsed=5.008` |
