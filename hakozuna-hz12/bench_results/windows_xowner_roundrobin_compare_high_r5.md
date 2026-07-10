# HZ12 Windows Cross-Owner Round-Robin Compare

- Generated: 2026-07-10T19:49:30.6622390+09:00
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
| hz11-ownerless | 14848 | 2026-07-10T19:47:49.5528143+09:00 | `2377FABA20FC7A3E5C43AAF32251D24082760D84A025A73DAC57F8DFC6B35CD6` | /O2 /DNDEBUG HZ_BENCH_USE_HZ11=1 HZ11_CLASSIFY_SPAN=1 HZ11_CACHE_CAP=32(default) |
| hz12-owner-inbox-diag | 20480 | 2026-07-10T19:47:52.1110138+09:00 | `5491664444C3A0F230ABDF38FB9AA2F4E834F48CA6948D01CA627FCCF5D69B4C` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=on diag_counters=on |
| tcmalloc | 12288 | 2026-07-10T19:47:49.8206647+09:00 | `EF15FBB7C6BAA814CCB3CD7366DBB078856E4A1812EB9AD929E430238F948854` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz11-ownerless | 13.036M | 12.928M | 13.084M | 5 |
| hz12-owner-inbox-diag | 17.604M | 17.540M | 21.126M | 5 |
| tcmalloc | 38.029M | 37.344M | 38.664M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz11-ownerless | 13056382.8 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65401453 cross_owner_frees=65401453 local_cleanup=0 waits=43844717 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13056382.80 elapsed=5.009` |
| 1 | 2 | hz12-owner-inbox-diag | 17549768.19 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=87894934 cross_owner_frees=87894934 owner_drained=87893493 waits=62620149 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17549768.19 elapsed=5.008 peak_rss_mib=10.85 rss_mib=10.70 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=89 full_capacity_spans=62 accounting_candidates=62 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 3 | tcmalloc | 37930187.94 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=190150412 cross_owner_frees=190150412 local_cleanup=0 waits=29066570 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37930187.94 elapsed=5.013` |
| 2 | 1 | hz12-owner-inbox-diag | 20161951.8 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=101029615 cross_owner_frees=101029615 owner_drained=101029615 waits=70320671 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=20161951.80 elapsed=5.011 peak_rss_mib=9.91 rss_mib=9.76 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=76 full_capacity_spans=48 accounting_candidates=48 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | tcmalloc | 38664307.59 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=193647799 cross_owner_frees=193647799 local_cleanup=0 waits=29362219 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38664307.59 elapsed=5.008` |
| 2 | 3 | hz11-ownerless | 12928140.1 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=64777519 cross_owner_frees=64777519 local_cleanup=0 waits=43259821 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12928140.10 elapsed=5.011` |
| 3 | 1 | tcmalloc | 38028623.22 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=190418778 cross_owner_frees=190418778 local_cleanup=0 waits=30659701 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38028623.22 elapsed=5.007` |
| 3 | 2 | hz11-ownerless | 13036086.81 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65356708 cross_owner_frees=65356708 local_cleanup=0 waits=44090452 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13036086.81 elapsed=5.014` |
| 3 | 3 | hz12-owner-inbox-diag | 21125818.11 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=105856191 cross_owner_frees=105856191 owner_drained=105852817 waits=65160697 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21125818.11 elapsed=5.011 peak_rss_mib=10.99 rss_mib=10.84 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=91 full_capacity_spans=65 accounting_candidates=65 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz11-ownerless | 13035751.93 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65323536 cross_owner_frees=65323536 local_cleanup=0 waits=44430017 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13035751.93 elapsed=5.011` |
| 4 | 2 | hz12-owner-inbox-diag | 17603551.48 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=88181613 cross_owner_frees=88181613 owner_drained=88181613 waits=60898069 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17603551.48 elapsed=5.009 peak_rss_mib=10.65 rss_mib=10.49 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=85 full_capacity_spans=57 accounting_candidates=57 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 3 | tcmalloc | 37343968.39 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=187019124 cross_owner_frees=187019124 local_cleanup=0 waits=29702096 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37343968.39 elapsed=5.008` |
| 5 | 1 | hz12-owner-inbox-diag | 17539751.16 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=87913509 cross_owner_frees=87913509 owner_drained=87913026 waits=63266978 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17539751.16 elapsed=5.012 peak_rss_mib=11.36 rss_mib=11.21 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=95 full_capacity_spans=67 accounting_candidates=67 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | tcmalloc | 38636558.54 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=193601273 cross_owner_frees=193601273 local_cleanup=0 waits=29938012 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38636558.54 elapsed=5.011` |
| 5 | 3 | hz11-ownerless | 13084454.3 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65526603 cross_owner_frees=65526603 local_cleanup=0 waits=44625435 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13084454.30 elapsed=5.008` |
