# HZ12 Windows Cross-Owner Round-Robin OwnerFastFullCompare

- Generated: 2026-07-10T20:22:05.4081792+09:00
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
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T20:18:47.8750404+09:00 | `22A4E35B7A03256B34E831A5DD2176DD2A449E2DA550F477EE036B88CBBE7364` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |
| tcmalloc | 12800 | 2026-07-10T20:18:42.2407219+09:00 | `14263CDC7536BA0D3F895D9E08CF3B0FD819253E237D02D327F1740CCEC7EAD4` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Peak RSS median | Final RSS median | Runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-ownerless | 12.992M | 12.527M | 13.032M | 7.79 MiB | 7.63 MiB | 5 |
| hz12-owner-inbox-ownerfastload | 36.427M | 35.257M | 36.837M | 15.54 MiB | 15.39 MiB | 5 |
| tcmalloc | 36.439M | 36.075M | 38.602M | 14.81 MiB | 14.22 MiB | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz11-ownerless | 13023309.82 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65207905 cross_owner_frees=65207905 local_cleanup=0 waits=43064378 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13023309.82 elapsed=5.007 peak_rss_mib=7.79 rss_mib=7.63` |
| 1 | 2 | hz12-owner-inbox-ownerfastload | 36836833.25 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=184312480 cross_owner_frees=184312480 owner_drained=184309984 waits=42267274 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36836833.25 elapsed=5.003 peak_rss_mib=10.53 rss_mib=10.38 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 3 | tcmalloc | 36438979.35 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=182505437 cross_owner_frees=182505437 local_cleanup=0 waits=31333097 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36438979.35 elapsed=5.009 peak_rss_mib=13.95 rss_mib=13.41` |
| 2 | 1 | hz12-owner-inbox-ownerfastload | 36426770.14 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=182467152 cross_owner_frees=182467152 owner_drained=182464203 waits=44645361 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36426770.14 elapsed=5.009 peak_rss_mib=17.58 rss_mib=17.43 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | tcmalloc | 38602132.06 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=193307889 cross_owner_frees=193307889 local_cleanup=0 waits=29860011 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38602132.06 elapsed=5.008 peak_rss_mib=14.88 rss_mib=14.29` |
| 2 | 3 | hz11-ownerless | 12991821.27 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65038489 cross_owner_frees=65038489 local_cleanup=0 waits=43897599 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12991821.27 elapsed=5.006 peak_rss_mib=6.95 rss_mib=6.79` |
| 3 | 1 | tcmalloc | 37365485.11 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=187158764 cross_owner_frees=187158764 local_cleanup=0 waits=30239932 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=37365485.11 elapsed=5.009 peak_rss_mib=13.69 rss_mib=13.09` |
| 3 | 2 | hz11-ownerless | 13032015.16 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65318304 cross_owner_frees=65318304 local_cleanup=0 waits=43217282 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13032015.16 elapsed=5.012 peak_rss_mib=7.26 rss_mib=7.11` |
| 3 | 3 | hz12-owner-inbox-ownerfastload | 35256917.55 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=176706056 cross_owner_frees=176706056 owner_drained=176703251 waits=42760999 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35256917.55 elapsed=5.012 peak_rss_mib=15.54 rss_mib=15.39 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz11-ownerless | 12767937.36 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=63982869 cross_owner_frees=63982869 local_cleanup=0 waits=42372199 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12767937.36 elapsed=5.011 peak_rss_mib=10.23 rss_mib=10.08` |
| 4 | 2 | hz12-owner-inbox-ownerfastload | 36602918.28 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=183306701 cross_owner_frees=183306701 owner_drained=183305209 waits=40935977 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36602918.28 elapsed=5.008 peak_rss_mib=17.79 rss_mib=17.64 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 3 | tcmalloc | 36074525.44 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=180508253 cross_owner_frees=180508253 local_cleanup=0 waits=27734557 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36074525.44 elapsed=5.004 peak_rss_mib=15.62 rss_mib=15.03` |
| 5 | 1 | hz12-owner-inbox-ownerfastload | 35645947.47 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=178399822 cross_owner_frees=178399822 owner_drained=178391750 waits=40623034 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=35645947.47 elapsed=5.005 peak_rss_mib=12.05 rss_mib=11.89 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | tcmalloc | 36088715.7 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=180613910 cross_owner_frees=180613910 local_cleanup=0 waits=27909357 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36088715.70 elapsed=5.005 peak_rss_mib=14.81 rss_mib=14.22` |
| 5 | 3 | hz11-ownerless | 12526668.97 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=62699175 cross_owner_frees=62699175 local_cleanup=0 waits=41009849 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12526668.97 elapsed=5.005 peak_rss_mib=8.38 rss_mib=8.22` |
