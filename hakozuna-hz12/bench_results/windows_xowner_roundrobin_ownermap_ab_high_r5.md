# HZ12 Windows Cross-Owner Round-Robin OwnerMapAB

- Generated: 2026-07-10T20:05:21.7839083+09:00
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
| hz12-owner-inbox-speed | 18944 | 2026-07-10T20:03:53.7875366+09:00 | `4D1C98B3447CC619F59C44F61EC7F8196761C86F6B0EC1B469AC08B40CF8C624` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=off |
| hz12-owner-inbox-ownerfastload | 18944 | 2026-07-10T20:03:54.9364602+09:00 | `FCDE8B6FEC1057E485FD5320640761B2A686A7FBD168F862B07E41E6DDA846D4` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-speed | 29.292M | 27.918M | 29.832M | 5 |
| hz12-owner-inbox-ownerfastload | 36.052M | 34.178M | 36.701M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-speed | 27918152.77 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=139843599 cross_owner_frees=139843599 owner_drained=139843599 waits=43171852 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27918152.77 elapsed=5.009 peak_rss_mib=9.16 rss_mib=9.01 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | hz12-owner-inbox-ownerfastload | 36404591.54 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=182371994 cross_owner_frees=182371994 owner_drained=182371057 waits=42251047 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36404591.54 elapsed=5.010 peak_rss_mib=12.24 rss_mib=12.09 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 1 | hz12-owner-inbox-ownerfastload | 36052098.55 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=180559736 cross_owner_frees=180559736 owner_drained=180558716 waits=42326702 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36052098.55 elapsed=5.008 peak_rss_mib=10.11 rss_mib=9.95 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | hz12-owner-inbox-speed | 29832378.73 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=149486992 cross_owner_frees=149486992 owner_drained=149486992 waits=45622752 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29832378.73 elapsed=5.011 peak_rss_mib=9.42 rss_mib=9.27 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-speed | 29291666.7 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=146734551 cross_owner_frees=146734551 owner_drained=146732056 waits=45083211 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29291666.70 elapsed=5.009 peak_rss_mib=13.43 rss_mib=13.28 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | hz12-owner-inbox-ownerfastload | 36701251.09 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=183881467 cross_owner_frees=183881467 owner_drained=183879352 waits=42408707 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36701251.09 elapsed=5.010 peak_rss_mib=15.31 rss_mib=15.16 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz12-owner-inbox-ownerfastload | 34535680.72 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=172942011 cross_owner_frees=172942011 owner_drained=172936445 waits=41663624 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34535680.72 elapsed=5.008 peak_rss_mib=17.73 rss_mib=17.58 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 2 | hz12-owner-inbox-speed | 29822408.83 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=149412156 cross_owner_frees=149412156 owner_drained=149410148 waits=46248296 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29822408.83 elapsed=5.010 peak_rss_mib=10.92 rss_mib=10.77 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-speed | 29268500.89 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=146613001 cross_owner_frees=146613001 owner_drained=146610460 waits=45067022 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29268500.89 elapsed=5.009 peak_rss_mib=11.97 rss_mib=11.82 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | hz12-owner-inbox-ownerfastload | 34177675.34 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=171171901 cross_owner_frees=171171901 owner_drained=171170768 waits=43303782 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=34177675.34 elapsed=5.008 peak_rss_mib=12.21 rss_mib=12.06 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
