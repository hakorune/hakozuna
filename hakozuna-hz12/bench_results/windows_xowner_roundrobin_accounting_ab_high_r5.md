# HZ12 Windows Cross-Owner Round-Robin AccountingAB

- Generated: 2026-07-10T19:51:50.6833749+09:00
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
| hz12-owner-inbox-diag | 20480 | 2026-07-10T19:50:15.3717723+09:00 | `25ABAF88E32A91475600232FCE1CE64171630E09AE2AD05151D437AC5532CD97` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=on diag_counters=on |
| hz12-owner-inbox-noaccounting | 19968 | 2026-07-10T19:50:16.5094303+09:00 | `0CE4D6EA39C21032F556E78E4D30F6079AF346DBF3BDC4C591354677AC4E56AE` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=on |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-diag | 18.420M | 17.152M | 21.414M | 5 |
| hz12-owner-inbox-noaccounting | 25.351M | 24.258M | 27.481M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-diag | 18041521.49 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=90330896 cross_owner_frees=90330896 owner_drained=90330252 waits=59383203 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18041521.49 elapsed=5.007 peak_rss_mib=10.70 rss_mib=10.55 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=88 full_capacity_spans=60 accounting_candidates=60 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | hz12-owner-inbox-noaccounting | 24258324.53 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=121502231 cross_owner_frees=121502231 owner_drained=121502131 waits=55778261 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=24258324.53 elapsed=5.009 peak_rss_mib=9.67 rss_mib=9.52 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 1 | hz12-owner-inbox-noaccounting | 26064685.97 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=130568477 cross_owner_frees=130568477 owner_drained=130568376 waits=42358230 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26064685.97 elapsed=5.009 peak_rss_mib=11.91 rss_mib=11.75 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | hz12-owner-inbox-diag | 20169781.98 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=101056935 cross_owner_frees=101056935 owner_drained=101054604 waits=69200266 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=20169781.98 elapsed=5.010 peak_rss_mib=11.48 rss_mib=11.32 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=98 full_capacity_spans=70 accounting_candidates=70 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-diag | 21414458.35 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=107264191 cross_owner_frees=107264191 owner_drained=107264191 waits=63904024 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=21414458.35 elapsed=5.009 peak_rss_mib=10.88 rss_mib=10.72 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=91 full_capacity_spans=63 accounting_candidates=63 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | hz12-owner-inbox-noaccounting | 25351411.04 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=127063787 cross_owner_frees=127063787 owner_drained=127063787 waits=47603673 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=25351411.04 elapsed=5.012 peak_rss_mib=8.77 rss_mib=8.62 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz12-owner-inbox-noaccounting | 24984301.67 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=125144863 cross_owner_frees=125144863 owner_drained=125144863 waits=61125576 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=24984301.67 elapsed=5.009 peak_rss_mib=8.68 rss_mib=8.53 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 2 | hz12-owner-inbox-diag | 18419706.09 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=92310556 cross_owner_frees=92310556 owner_drained=92310279 waits=62288107 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18419706.09 elapsed=5.012 peak_rss_mib=10.90 rss_mib=10.75 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=90 full_capacity_spans=63 accounting_candidates=63 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-diag | 17152470.83 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=85968002 cross_owner_frees=85968002 owner_drained=85958304 waits=60839145 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=17152470.83 elapsed=5.012 peak_rss_mib=11.27 rss_mib=11.12 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=96 full_capacity_spans=68 accounting_candidates=68 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | hz12-owner-inbox-noaccounting | 27480735.44 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=137647926 cross_owner_frees=137647926 owner_drained=137646981 waits=37689533 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27480735.44 elapsed=5.009 peak_rss_mib=12.28 rss_mib=12.13 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
