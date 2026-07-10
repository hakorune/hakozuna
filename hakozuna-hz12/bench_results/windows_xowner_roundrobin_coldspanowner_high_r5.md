# HZ12 Windows Cross-Owner Round-Robin ColdSpanOwnerCompare

- Generated: 2026-07-10T21:51:56.2227350+09:00
- Git HEAD: `01725fd5b1013998cc44438ce478def41e34ae65`
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
| hz12-flushroute-integrated | 18432 | 2026-07-10T21:49:27.1626240+09:00 | `C7C2E41658A09DE35EBE706137A2ADBA1954ADE2CF08E54E403E9BDCFBFCC788` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on owner_fast_load=on accounting=off diag_counters=off drain_pace=256 |
| hz12-coldspanowner | 18944 | 2026-07-10T21:49:32.7361787+09:00 | `66DC76B152458695498AFA1CFEB121E88AB056DD35001060F6AED5EC81F0214F` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 flush_owner_route=on cold_span_owner=on accounting=off diag_counters=off |
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T21:30:56.3458178+09:00 | `EB833ECAE04A01BA0B0666322C13A878F81DFEF4E71116F40F27A27292206512` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12800 | 2026-07-10T20:18:42.2407219+09:00 | `14263CDC7536BA0D3F895D9E08CF3B0FD819253E237D02D327F1740CCEC7EAD4` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Peak RSS median | Final RSS median | Runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-flushroute-integrated | 26.169M | 26.136M | 27.108M | 14.20 MiB | 14.04 MiB | 5 |
| hz12-coldspanowner | 29.064M | 28.427M | 31.050M | 11.32 MiB | 11.16 MiB | 5 |
| hz12-owner-inbox-ownerfastload | 35.096M | 34.357M | 36.203M | 11.88 MiB | 11.72 MiB | 5 |
| tcmalloc | 36.911M | 36.303M | 38.465M | 15.13 MiB | 14.54 MiB | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-flushroute-integrated | 26196870.3 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=131367782 cross_owner_frees=131367782 local_cleanup=0 waits=58347946 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26196870.30 elapsed=5.015 peak_rss_mib=15.18 rss_mib=15.03` |
| 1 | 2 | hz12-coldspanowner | 29063793.2 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=145748991 cross_owner_frees=145748991 local_cleanup=0 waits=49859205 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29063793.20 elapsed=5.015 peak_rss_mib=10.27 rss_mib=10.12` |
| 1 | 3 | hz12-owner-inbox-ownerfastload | 35096426.24 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=175590516 cross_owner_frees=175590516 owner_drained=175589612 waits=42254350 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35096426.24 elapsed=5.003 peak_rss_mib=10.76 rss_mib=10.61 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 4 | tcmalloc | 36573449.36 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=183432409 cross_owner_frees=183432409 local_cleanup=0 waits=29833383 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36573449.36 elapsed=5.015 peak_rss_mib=14.00 rss_mib=13.46` |
| 2 | 1 | hz12-coldspanowner | 29197919.92 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=146216447 cross_owner_frees=146216447 local_cleanup=0 waits=50329083 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29197919.92 elapsed=5.008 peak_rss_mib=11.42 rss_mib=11.27` |
| 2 | 2 | hz12-owner-inbox-ownerfastload | 34914168.79 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=174707491 cross_owner_frees=174707491 owner_drained=174701503 waits=42771026 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34914168.79 elapsed=5.004 peak_rss_mib=11.03 rss_mib=10.88 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 3 | tcmalloc | 36302822.73 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=181654729 cross_owner_frees=181654729 local_cleanup=0 waits=28728659 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36302822.73 elapsed=5.004 peak_rss_mib=15.13 rss_mib=14.54` |
| 2 | 4 | hz12-flushroute-integrated | 27107780.88 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=135706360 cross_owner_frees=135706360 local_cleanup=0 waits=44048680 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27107780.88 elapsed=5.006 peak_rss_mib=16.47 rss_mib=16.32` |
| 3 | 1 | hz12-owner-inbox-ownerfastload | 36203449.78 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=181143827 cross_owner_frees=181143827 owner_drained=181142698 waits=42927093 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36203449.78 elapsed=5.003 peak_rss_mib=13.75 rss_mib=13.59 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | tcmalloc | 36910999.24 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=184701854 cross_owner_frees=184701854 local_cleanup=0 waits=28485616 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36910999.24 elapsed=5.004 peak_rss_mib=13.93 rss_mib=13.34` |
| 3 | 3 | hz12-flushroute-integrated | 26135840.36 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130752126 cross_owner_frees=130752126 local_cleanup=0 waits=44612094 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26135840.36 elapsed=5.003 peak_rss_mib=14.20 rss_mib=14.04` |
| 3 | 4 | hz12-coldspanowner | 31049813.08 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=155314447 cross_owner_frees=155314447 local_cleanup=0 waits=44454952 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=31049813.08 elapsed=5.002 peak_rss_mib=11.32 rss_mib=11.16` |
| 4 | 1 | tcmalloc | 38465088.38 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=192480160 cross_owner_frees=192480160 local_cleanup=0 waits=29508326 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38465088.38 elapsed=5.004 peak_rss_mib=15.82 rss_mib=15.23` |
| 4 | 2 | hz12-flushroute-integrated | 26143524.38 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130761462 cross_owner_frees=130761462 local_cleanup=0 waits=57674789 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26143524.38 elapsed=5.002 peak_rss_mib=13.95 rss_mib=13.79` |
| 4 | 3 | hz12-coldspanowner | 28427489.65 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=142505715 cross_owner_frees=142505715 local_cleanup=0 waits=53342340 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28427489.65 elapsed=5.013 peak_rss_mib=10.77 rss_mib=10.62` |
| 4 | 4 | hz12-owner-inbox-ownerfastload | 34357122.67 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=172030267 cross_owner_frees=172030267 owner_drained=172026880 waits=44436413 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34357122.67 elapsed=5.007 peak_rss_mib=15.43 rss_mib=15.27 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-flushroute-integrated | 26168761.36 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=130994251 cross_owner_frees=130994251 local_cleanup=0 waits=58271827 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26168761.36 elapsed=5.006 peak_rss_mib=12.62 rss_mib=12.47` |
| 5 | 2 | hz12-coldspanowner | 28760049.45 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=144002068 cross_owner_frees=144002068 local_cleanup=0 waits=50299871 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28760049.45 elapsed=5.007 peak_rss_mib=13.68 rss_mib=13.52` |
| 5 | 3 | hz12-owner-inbox-ownerfastload | 35877882.54 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=179579587 cross_owner_frees=179579587 owner_drained=179578660 waits=42128092 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35877882.54 elapsed=5.005 peak_rss_mib=11.88 rss_mib=11.72 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 4 | tcmalloc | 37344088.48 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=186980361 cross_owner_frees=186980361 local_cleanup=0 waits=28143146 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37344088.48 elapsed=5.007 peak_rss_mib=15.32 rss_mib=14.72` |
