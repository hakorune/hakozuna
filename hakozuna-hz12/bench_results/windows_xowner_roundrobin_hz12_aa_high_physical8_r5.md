# HZ12 Windows Cross-Owner Round-Robin Hz12AA

- Generated: 2026-07-10T19:47:14.7759392+09:00
- Git HEAD: `7e8323dadecbd1bfc6897600068bb18d35d1ac2d`
- Compiler: `clang version 18.1.8 (https://github.com/llvm/llvm-project.git 3b5b5c1ec4a3095ab096dd780e84d7ab81f3d7ff)`
- Rounds: 5; runtime: 5s
- Producers/consumers: 4/4; ring: 4096
- Size: 8..1024; cross-owner free target: 100%
- Logical CPUs: 32; affinity mask: `0x5555`
- Priority class: High
- Order: rotated once per round; cooldown: 250ms

## Build Manifest

| Row | Bytes | Last write | SHA-256 | Build flags / identity |
| --- | ---: | --- | --- | --- |
| hz12-owner-inbox-diag-a | 20480 | 2026-07-10T19:44:07.5072135+09:00 | `3B26762F84D46D3B0C18032C694A3AE0ACE1F9F01F04BD5A252D83E2F0030D16` | A/A alias of hz12-owner-inbox-diag |
| hz12-owner-inbox-diag-b | 20480 | 2026-07-10T19:44:07.5072135+09:00 | `3B26762F84D46D3B0C18032C694A3AE0ACE1F9F01F04BD5A252D83E2F0030D16` | A/A alias of hz12-owner-inbox-diag |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-diag-a | 16.649M | 15.156M | 19.894M | 5 |
| hz12-owner-inbox-diag-b | 16.005M | 15.316M | 20.970M | 5 |

## A/A Band

- B/A median delta: -3.87%
- B/A min/max delta: -23.01% / 14.34%

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-diag-a | 16648663.91 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=83499699 cross_owner_frees=83499699 owner_drained=83483291 waits=51033735 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=16648663.91 elapsed=5.015 peak_rss_mib=18.01 rss_mib=17.86 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=202 full_capacity_spans=175 accounting_candidates=175 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | hz12-owner-inbox-diag-b | 16004670.41 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=80200082 cross_owner_frees=80200082 owner_drained=80189389 waits=54183505 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=16004670.41 elapsed=5.011 peak_rss_mib=13.61 rss_mib=13.46 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=131 full_capacity_spans=103 accounting_candidates=103 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 1 | hz12-owner-inbox-diag-b | 15316281.16 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=76865918 cross_owner_frees=76865918 owner_drained=76865918 waits=59176953 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=15316281.16 elapsed=5.019 peak_rss_mib=9.50 rss_mib=9.34 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=70 full_capacity_spans=42 accounting_candidates=42 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | hz12-owner-inbox-diag-a | 19894202.35 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=99854377 cross_owner_frees=99854377 owner_drained=99853444 waits=63685521 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=19894202.35 elapsed=5.019 peak_rss_mib=12.29 rss_mib=12.13 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=111 full_capacity_spans=83 accounting_candidates=83 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-diag-a | 15155866.33 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=76090571 cross_owner_frees=76090571 owner_drained=76090571 waits=60538719 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=15155866.33 elapsed=5.021 peak_rss_mib=9.16 rss_mib=9.01 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=65 full_capacity_spans=38 accounting_candidates=38 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | hz12-owner-inbox-diag-b | 16087061 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=80788305 cross_owner_frees=80788305 owner_drained=80787254 waits=53919029 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=16087061.00 elapsed=5.022 peak_rss_mib=12.40 rss_mib=12.25 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=113 full_capacity_spans=85 accounting_candidates=85 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz12-owner-inbox-diag-b | 20969858.16 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=105250224 cross_owner_frees=105250224 owner_drained=105233446 waits=61575068 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=20969858.16 elapsed=5.019 peak_rss_mib=18.54 rss_mib=18.39 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=210 full_capacity_spans=182 accounting_candidates=182 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 2 | hz12-owner-inbox-diag-a | 18339579.26 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=91732316 cross_owner_frees=91732316 owner_drained=91732316 waits=76486756 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=18339579.26 elapsed=5.002 peak_rss_mib=8.49 rss_mib=8.33 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=54 full_capacity_spans=27 accounting_candidates=27 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-diag-a | 16395341.73 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=82211508 cross_owner_frees=82211508 owner_drained=82208105 waits=52682247 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=16395341.73 elapsed=5.014 peak_rss_mib=16.51 rss_mib=16.35 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=179 full_capacity_spans=151 accounting_candidates=151 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | hz12-owner-inbox-diag-b | 15447196.09 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=77412891 cross_owner_frees=77412891 owner_drained=77412891 waits=57255447 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=15447196.09 elapsed=5.011 peak_rss_mib=10.62 rss_mib=10.46 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=87 full_capacity_spans=60 accounting_candidates=60 tracked_live=0 release_untracked=0 release_underflow=0` |
