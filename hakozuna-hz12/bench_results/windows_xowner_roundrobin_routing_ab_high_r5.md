# HZ12 Windows Cross-Owner Round-Robin RoutingAB

- Generated: 2026-07-10T20:03:04.5155349+09:00
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
| hz12-owner-inbox-speed | 18944 | 2026-07-10T20:01:36.7988752+09:00 | `6959132866234D73689B2347701BCA165FAA8B3EAC4A6C825EFFEF7268861C51` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=off |
| hz12-bare-core | 15872 | 2026-07-10T20:01:37.7202185+09:00 | `A37CF7758949C271CACEF5B572B9EE1C8D69CBDEDBE543F562EE0B0C71D72A31` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 bare_core=on owner_mapping=off inbox=off accounting=off diag_counters=off |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-speed | 29.274M | 28.575M | 29.988M | 5 |
| hz12-bare-core | 13.434M | 13.322M | 13.505M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-speed | 28575154.57 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=142883331 cross_owner_frees=142883331 owner_drained=142874675 waits=50035687 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28575154.57 elapsed=5.000 peak_rss_mib=12.05 rss_mib=11.90 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | hz12-bare-core | 13347113.93 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=66859567 cross_owner_frees=66859567 owner_drained=0 waits=69305553 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13347113.93 elapsed=5.009 peak_rss_mib=8.01 rss_mib=7.85 retired_owners=0 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 1 | hz12-bare-core | 13504602.85 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=67659535 cross_owner_frees=67659535 owner_drained=0 waits=67967521 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13504602.85 elapsed=5.010 peak_rss_mib=7.95 rss_mib=7.79 retired_owners=0 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | hz12-owner-inbox-speed | 29345779.19 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=147040733 cross_owner_frees=147040733 owner_drained=147039516 waits=46816996 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29345779.19 elapsed=5.011 peak_rss_mib=10.32 rss_mib=10.16 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-speed | 29274204.56 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=146666783 cross_owner_frees=146666783 owner_drained=146665878 waits=46263790 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29274204.56 elapsed=5.010 peak_rss_mib=11.05 rss_mib=10.90 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | hz12-bare-core | 13434312.71 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=67331452 cross_owner_frees=67331452 owner_drained=0 waits=67689602 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13434312.71 elapsed=5.012 peak_rss_mib=8.30 rss_mib=8.15 retired_owners=0 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz12-bare-core | 13321911.63 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=66749702 cross_owner_frees=66749702 owner_drained=0 waits=64056991 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13321911.63 elapsed=5.011 peak_rss_mib=8.14 rss_mib=7.99 retired_owners=0 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 2 | hz12-owner-inbox-speed | 28750422.03 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=144047538 cross_owner_frees=144047538 owner_drained=144047282 waits=45385008 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28750422.03 elapsed=5.010 peak_rss_mib=10.04 rss_mib=9.88 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-speed | 29987946.39 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=150223232 cross_owner_frees=150223232 owner_drained=150222235 waits=44019438 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29987946.39 elapsed=5.009 peak_rss_mib=14.34 rss_mib=14.18 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | hz12-bare-core | 13465125.73 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=67467337 cross_owner_frees=67467337 owner_drained=0 waits=68245444 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13465125.73 elapsed=5.011 peak_rss_mib=8.19 rss_mib=8.03 retired_owners=0 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
