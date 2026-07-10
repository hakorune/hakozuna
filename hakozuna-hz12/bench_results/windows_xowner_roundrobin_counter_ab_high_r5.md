# HZ12 Windows Cross-Owner Round-Robin CounterAB

- Generated: 2026-07-10T19:57:32.9799940+09:00
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
| hz12-owner-inbox-noaccounting | 19968 | 2026-07-10T19:56:19.0290226+09:00 | `F5B62EDB5A5A7D0982A291A2F7B040C593DA0170FED83A377D2F61DA3B1D5322` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=on |
| hz12-owner-inbox-speed | 18944 | 2026-07-10T19:56:20.2246368+09:00 | `48700A0CD03DE49293240C7020A5D18A07FBD9CE89CC75247626FCFAAC350C2F` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=off |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz12-owner-inbox-noaccounting | 25.620M | 24.834M | 26.027M | 5 |
| hz12-owner-inbox-speed | 28.477M | 26.376M | 29.119M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz12-owner-inbox-noaccounting | 25863778.33 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=129467986 cross_owner_frees=129467986 owner_drained=129462571 waits=42218747 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=25863778.33 elapsed=5.006 peak_rss_mib=15.86 rss_mib=15.70 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 2 | hz12-owner-inbox-speed | 29118653.16 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=145880268 cross_owner_frees=145880268 owner_drained=145877522 waits=45410355 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29118653.16 elapsed=5.010 peak_rss_mib=11.12 rss_mib=10.97 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 1 | hz12-owner-inbox-speed | 26398722.49 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=132263711 cross_owner_frees=132263711 owner_drained=132261812 waits=49938497 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26398722.49 elapsed=5.010 peak_rss_mib=12.63 rss_mib=12.48 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | hz12-owner-inbox-noaccounting | 24834487.53 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=124414713 cross_owner_frees=124414713 owner_drained=124414713 waits=45289990 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=24834487.53 elapsed=5.010 peak_rss_mib=9.39 rss_mib=9.23 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 1 | hz12-owner-inbox-noaccounting | 26027074.18 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=130415771 cross_owner_frees=130415771 owner_drained=130415771 waits=38545459 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26027074.18 elapsed=5.011 peak_rss_mib=9.71 rss_mib=9.55 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 3 | 2 | hz12-owner-inbox-speed | 28477193.79 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=142604446 cross_owner_frees=142604446 owner_drained=142603347 waits=44159803 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28477193.79 elapsed=5.008 peak_rss_mib=12.38 rss_mib=12.22 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz12-owner-inbox-speed | 26375920.81 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=132100658 cross_owner_frees=132100658 owner_drained=132096607 waits=47942548 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=26375920.81 elapsed=5.008 peak_rss_mib=12.98 rss_mib=12.82 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 2 | hz12-owner-inbox-noaccounting | 25336121.58 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=126903396 cross_owner_frees=126903396 owner_drained=126903053 waits=43431257 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=25336121.58 elapsed=5.009 peak_rss_mib=8.88 rss_mib=8.73 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 1 | hz12-owner-inbox-noaccounting | 25619791.51 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=128335833 cross_owner_frees=128335833 owner_drained=128335833 waits=52890851 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=25619791.51 elapsed=5.009 peak_rss_mib=10.39 rss_mib=10.23 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | hz12-owner-inbox-speed | 29023181.48 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=145385852 cross_owner_frees=145385852 owner_drained=145385729 waits=46664128 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29023181.48 elapsed=5.009 peak_rss_mib=9.65 rss_mib=9.50 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
