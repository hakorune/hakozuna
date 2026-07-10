# HZ12 Windows Cross-Owner Round-Robin ColdSpanOwnerCompare

- Generated: 2026-07-10T22:04:45.2054851+09:00
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
| hz12-flushroute-integrated | 18944 | 2026-07-10T22:03:09.7286495+09:00 | `593040785F1FCCC0E900349AA8F89E717926AAA8E170B1620B840C10C85FBBF1` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on owner_fast_load=on accounting=off diag_counters=off drain_pace=256 |
| hz12-coldspanowner | 19968 | 2026-07-10T22:03:15.3111943+09:00 | `0F15CA85C4CC3493778FEA1AF8D89AD0C2F8445A4B3CFEA6CBCD564F694DFA81` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on cold_span_owner=on accounting=off diag_counters=off |
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T21:54:29.1606070+09:00 | `D0D9F468E08DB92041F80E2015D94CC26E47C33EBD5CD5B27E4E8BCED83CB953` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12800 | 2026-07-10T20:18:42.2407219+09:00 | `14263CDC7536BA0D3F895D9E08CF3B0FD819253E237D02D327F1740CCEC7EAD4` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Peak RSS median | Final RSS median | Runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-flushroute-integrated | 22.051M | 21.575M | 22.051M | 9.87 MiB | 9.71 MiB | 3 |
| hz12-coldspanowner | 31.107M | 28.465M | 31.107M | 16.05 MiB | 15.68 MiB | 3 |
| hz12-owner-inbox-ownerfastload | 37.826M | 35.326M | 37.826M | 14.11 MiB | 13.96 MiB | 3 |
| tcmalloc | 38.578M | 36.702M | 38.578M | 14.32 MiB | 13.84 MiB | 3 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-flushroute-integrated | 21880986.15 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=109654700 cross_owner_frees=109654700 local_cleanup=0 waits=68077100 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21880986.15 elapsed=5.011 peak_rss_mib=8.99 rss_mib=8.84` |
| 1 | 2 | hz12-coldspanowner | 28464506.53 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=142635850 cross_owner_frees=142635850 local_cleanup=0 waits=41554845 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28464506.53 elapsed=5.011 peak_rss_mib=12.01 rss_mib=11.64` |
| 1 | 3 | hz12-owner-inbox-ownerfastload | 35967299.1 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=180219353 cross_owner_frees=180219353 owner_drained=180218876 waits=43430897 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35967299.10 elapsed=5.011 peak_rss_mib=14.11 rss_mib=13.96 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 4 | tcmalloc | 36702228.55 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=183645216 cross_owner_frees=183645216 local_cleanup=0 waits=30648354 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36702228.55 elapsed=5.004 peak_rss_mib=14.32 rss_mib=13.84` |
| 2 | 1 | hz12-coldspanowner | 29194453.12 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=146247724 cross_owner_frees=146247724 local_cleanup=0 waits=51669087 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29194453.12 elapsed=5.009 peak_rss_mib=16.05 rss_mib=15.68` |
| 2 | 2 | hz12-owner-inbox-ownerfastload | 37825745.37 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=189478964 cross_owner_frees=189478963 owner_drained=189476363 waits=41821218 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37825745.37 elapsed=5.009 peak_rss_mib=11.91 rss_mib=11.75 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 3 | tcmalloc | 38578132.06 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=193246586 cross_owner_frees=193246586 local_cleanup=0 waits=29294218 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38578132.06 elapsed=5.009 peak_rss_mib=14.19 rss_mib=13.60` |
| 2 | 4 | hz12-flushroute-integrated | 22051243.26 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=110498006 cross_owner_frees=110498006 local_cleanup=0 waits=68193901 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=22051243.26 elapsed=5.011 peak_rss_mib=9.87 rss_mib=9.71` |
| 3 | 1 | hz12-owner-inbox-ownerfastload | 35326450.15 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=176997498 cross_owner_frees=176997498 owner_drained=176997498 waits=44349190 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35326450.15 elapsed=5.010 peak_rss_mib=10.20 rss_mib=10.05 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | tcmalloc | 38089470.55 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=190785492 cross_owner_frees=190785492 local_cleanup=0 waits=29743882 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38089470.55 elapsed=5.009 peak_rss_mib=12.68 rss_mib=12.09` |
| 3 | 3 | hz12-flushroute-integrated | 21574722.35 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=108113486 cross_owner_frees=108113486 local_cleanup=0 waits=69138120 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21574722.35 elapsed=5.011 peak_rss_mib=9.69 rss_mib=9.54` |
| 3 | 4 | hz12-coldspanowner | 31107199.87 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=155835851 cross_owner_frees=155835851 local_cleanup=0 waits=52055888 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=31107199.87 elapsed=5.010 peak_rss_mib=11.76 rss_mib=11.39` |
