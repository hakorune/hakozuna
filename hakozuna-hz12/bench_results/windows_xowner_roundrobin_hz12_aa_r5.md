# HZ12 Windows Cross-Owner Round-Robin Hz12AA

- Generated: 2026-07-10T19:45:19.8131630+09:00
- Git HEAD: `7e8323dadecbd1bfc6897600068bb18d35d1ac2d`
- Compiler: `clang version 18.1.8 (https://github.com/llvm/llvm-project.git 3b5b5c1ec4a3095ab096dd780e84d7ab81f3d7ff)`
- Rounds: 5; runtime: 5s
- Producers/consumers: 4/4; ring: 4096
- Size: 8..1024; cross-owner free target: 100%
- Logical CPUs: 32; affinity mask: `0xFF`
- Order: rotated once per round; cooldown: 250ms

## Build Manifest

| Row | Bytes | Last write | SHA-256 | Build flags / identity |
| --- | ---: | --- | --- | --- |
| hz12-owner-inbox-diag-a | 20480 | 2026-07-10T19:44:07.5072135+09:00 | `3B26762F84D46D3B0C18032C694A3AE0ACE1F9F01F04BD5A252D83E2F0030D16` | A/A alias of hz12-owner-inbox-diag |
| hz12-owner-inbox-diag-b | 20480 | 2026-07-10T19:44:07.5072135+09:00 | `3B26762F84D46D3B0C18032C694A3AE0ACE1F9F01F04BD5A252D83E2F0030D16` | A/A alias of hz12-owner-inbox-diag |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-diag-a | 18.114M | 17.438M | 23.277M | 5 |
| hz12-owner-inbox-diag-b | 18.598M | 17.040M | 22.286M | 5 |

## A/A Band

- B/A median delta: 3.53%
- B/A min/max delta: -24.19% / 23.03%

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-diag-a | 23276694.5 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=116400248 cross_owner_frees=116400248 owner_drained=116382414 waits=59823002 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=23276694.50 elapsed=5.001 peak_rss_mib=17.87 rss_mib=17.71 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=201 full_capacity_spans=173 accounting_candidates=173 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=116400248 route_batches=592203 route_objects=116382414 fallback_unknown=0 fallback_overflow=17834 fallback_adopted=0 owner_drain_batches=451001 owner_drain_objects=116382414 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=2 retired_owner_pending_objects=466 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 1 | 2 | hz12-owner-inbox-diag-b | 17646090.83 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=88418685 cross_owner_frees=88418685 owner_drained=88392173 waits=61516319 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17646090.83 elapsed=5.011 peak_rss_mib=11.16 rss_mib=11.01 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=93 full_capacity_spans=65 accounting_candidates=65 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=88418685 route_batches=482761 route_objects=88392173 fallback_unknown=0 fallback_overflow=26512 fallback_adopted=0 owner_drain_batches=344778 owner_drain_objects=88392173 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=4 retired_owner_pending_objects=1024 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 2 | 1 | hz12-owner-inbox-diag-b | 18804176.04 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=94230103 cross_owner_frees=94230103 owner_drained=94229983 waits=57775669 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18804176.04 elapsed=5.011 peak_rss_mib=10.52 rss_mib=10.37 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=84 full_capacity_spans=56 accounting_candidates=56 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=94230103 route_batches=440605 route_objects=94229983 fallback_unknown=0 fallback_overflow=120 fallback_adopted=0 owner_drain_batches=365420 owner_drain_objects=94229983 inbox_current_max=971 retired_owner_marked=4 retired_owner_with_pending=2 retired_owner_pending_objects=512 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 2 | 2 | hz12-owner-inbox-diag-a | 18162462.51 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=91049211 cross_owner_frees=91049211 owner_drained=91043634 waits=58661104 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18162462.51 elapsed=5.013 peak_rss_mib=11.38 rss_mib=11.22 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=97 full_capacity_spans=69 accounting_candidates=69 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=91049211 route_batches=546738 route_objects=91043634 fallback_unknown=0 fallback_overflow=5577 fallback_adopted=0 owner_drain_batches=353306 owner_drain_objects=91043634 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=4 retired_owner_pending_objects=1015 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 3 | 1 | hz12-owner-inbox-diag-a | 17951476.04 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=89931181 cross_owner_frees=89931181 owner_drained=89931181 waits=61079351 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17951476.04 elapsed=5.010 peak_rss_mib=10.71 rss_mib=10.56 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=87 full_capacity_spans=59 accounting_candidates=59 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=89931181 route_batches=431248 route_objects=89931181 fallback_unknown=0 fallback_overflow=0 fallback_adopted=0 owner_drain_batches=348999 owner_drain_objects=89931181 inbox_current_max=1011 retired_owner_marked=4 retired_owner_with_pending=3 retired_owner_pending_objects=768 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 3 | 2 | hz12-owner-inbox-diag-b | 17040438.93 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=85388053 cross_owner_frees=85388053 owner_drained=85387541 waits=64715738 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17040438.93 elapsed=5.011 peak_rss_mib=10.16 rss_mib=10.01 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=79 full_capacity_spans=51 accounting_candidates=51 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=85388053 route_batches=416758 route_objects=85387541 fallback_unknown=0 fallback_overflow=512 fallback_adopted=0 owner_drain_batches=331800 owner_drain_objects=85387541 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=3 retired_owner_pending_objects=702 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 4 | 1 | hz12-owner-inbox-diag-b | 22286488.3 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=111690889 cross_owner_frees=111690889 owner_drained=111688405 waits=60549015 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=22286488.30 elapsed=5.012 peak_rss_mib=14.43 rss_mib=14.27 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=144 full_capacity_spans=116 accounting_candidates=116 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=111690889 route_batches=547772 route_objects=111688405 fallback_unknown=0 fallback_overflow=2484 fallback_adopted=0 owner_drain_batches=433840 owner_drain_objects=111688405 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=3 retired_owner_pending_objects=537 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 4 | 2 | hz12-owner-inbox-diag-a | 18114485.33 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=90751597 cross_owner_frees=90751597 owner_drained=90750721 waits=59005039 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18114485.33 elapsed=5.010 peak_rss_mib=11.76 rss_mib=11.61 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=103 full_capacity_spans=75 accounting_candidates=75 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=90751597 route_batches=470440 route_objects=90750721 fallback_unknown=0 fallback_overflow=876 fallback_adopted=0 owner_drain_batches=352025 owner_drain_objects=90750721 inbox_current_max=1017 retired_owner_marked=4 retired_owner_with_pending=3 retired_owner_pending_objects=707 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 5 | 1 | hz12-owner-inbox-diag-a | 17438251.62 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=87354093 cross_owner_frees=87354093 owner_drained=87353656 waits=61779810 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17438251.62 elapsed=5.009 peak_rss_mib=11.10 rss_mib=10.95 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=92 full_capacity_spans=64 accounting_candidates=64 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=87354093 route_batches=421897 route_objects=87353656 fallback_unknown=0 fallback_overflow=437 fallback_adopted=0 owner_drain_batches=339325 owner_drain_objects=87353656 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=4 retired_owner_pending_objects=999 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
| 5 | 2 | hz12-owner-inbox-diag-b | 18597597.35 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=93178945 cross_owner_frees=93178945 owner_drained=93122569 waits=58442353 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18597597.35 elapsed=5.010 peak_rss_mib=11.59 rss_mib=11.44 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=102 full_capacity_spans=74 accounting_candidates=74 tracked_live=0 release_untracked=0 release_underflow=0
[HZ12_OWNER_INBOX] deferred_objects=93178945 route_batches=467999 route_objects=93122569 fallback_unknown=0 fallback_overflow=56376 fallback_adopted=0 owner_drain_batches=361187 owner_drain_objects=93122569 inbox_current_max=1024 retired_owner_marked=4 retired_owner_with_pending=3 retired_owner_pending_objects=764 adoption_shadow_scans=1 adoption_shadow_pending_owners=0 adoption_shadow_pending_objects=0 adoption_reject_active=0 adoption_reject_duplicate=0 adoption_batches=0 adoption_objects=0

` |
