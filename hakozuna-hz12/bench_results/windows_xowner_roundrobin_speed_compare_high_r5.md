# HZ12 Windows Cross-Owner Round-Robin SpeedCompare

- Generated: 2026-07-10T19:59:29.6087556+09:00
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
| hz11-ownerless | 14848 | 2026-07-10T19:57:44.2733390+09:00 | `07F6A3D6B5C229D78DEA4704E69C279EAF8E8EA9FA6EDD53FAD10D0FB7670B80` | /O2 /DNDEBUG HZ_BENCH_USE_HZ11=1 HZ11_CLASSIFY_SPAN=1 HZ11_CACHE_CAP=32(default) |
| hz12-owner-inbox-speed | 18944 | 2026-07-10T19:57:49.1371058+09:00 | `43D3AD8216CBA696F6DBA5AC3CC58A23C737756A3DD5E65BFBC125C71965B58D` | /O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=off |
| tcmalloc | 12288 | 2026-07-10T19:57:44.5547783+09:00 | `B95D5DCED617892668DD8335BD9EC63FEE93C5CCA2C11A43AAA9785036B0729B` | /O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll |
| tcmalloc_minimal.dll | 253952 | 2026-01-18T01:58:49.9756036+09:00 | `878E64FF5340B981523EC4F895F3CBA84E5E784D627B592F14D9EE4891497651` | gperftools 2.16 minimal runtime |

## Median

| Allocator | Median ops/s | Min | Max | Runs |
| --- | ---: | ---: | ---: | ---: |
| hz11-ownerless | 13.046M | 12.965M | 13.293M | 5 |
| hz12-owner-inbox-speed | 28.896M | 27.507M | 29.309M | 5 |
| tcmalloc | 36.833M | 36.129M | 38.679M | 5 |

## Raw Runs

| Round | Position | Allocator | ops/s | Raw |
| ---: | ---: | --- | ---: | --- |
| 1 | 1 | hz11-ownerless | 13045895.43 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65270303 cross_owner_frees=65270303 local_cleanup=0 waits=43301903 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13045895.43 elapsed=5.003` |
| 1 | 2 | hz12-owner-inbox-speed | 27507196.49 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=137775108 cross_owner_frees=137775108 owner_drained=137774400 waits=52099871 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=27507196.49 elapsed=5.009 peak_rss_mib=10.02 rss_mib=9.86 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 1 | 3 | tcmalloc | 38449945.14 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=192827786 cross_owner_frees=192827786 local_cleanup=0 waits=29361717 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38449945.14 elapsed=5.015` |
| 2 | 1 | hz12-owner-inbox-speed | 29161781.52 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=146105620 cross_owner_frees=146105620 owner_drained=146104551 waits=47558341 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29161781.52 elapsed=5.010 peak_rss_mib=10.27 rss_mib=10.11 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 2 | 2 | tcmalloc | 36129163.26 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=180957900 cross_owner_frees=180957900 local_cleanup=0 waits=30530699 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36129163.26 elapsed=5.009` |
| 2 | 3 | hz11-ownerless | 12965016.7 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=64950087 cross_owner_frees=64950087 local_cleanup=0 waits=42271925 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=12965016.70 elapsed=5.010` |
| 3 | 1 | tcmalloc | 36833175.11 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=184435778 cross_owner_frees=184435778 local_cleanup=0 waits=32766669 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36833175.11 elapsed=5.007` |
| 3 | 2 | hz11-ownerless | 13292571.52 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=66611438 cross_owner_frees=66611438 local_cleanup=0 waits=47349378 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13292571.52 elapsed=5.011` |
| 3 | 3 | hz12-owner-inbox-speed | 28895500.95 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=144723313 cross_owner_frees=144723313 owner_drained=144721744 waits=48535988 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28895500.95 elapsed=5.009 peak_rss_mib=17.51 rss_mib=17.36 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 1 | hz11-ownerless | 13055772.84 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65408478 cross_owner_frees=65408478 local_cleanup=0 waits=42701702 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13055772.84 elapsed=5.010` |
| 4 | 2 | hz12-owner-inbox-speed | 28646254.25 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=143609525 cross_owner_frees=143609525 owner_drained=143609040 waits=46153184 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=28646254.25 elapsed=5.013 peak_rss_mib=9.78 rss_mib=9.62 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 4 | 3 | tcmalloc | 38679176.72 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=193683150 cross_owner_frees=193683150 local_cleanup=0 waits=29515941 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=38679176.72 elapsed=5.007` |
| 5 | 1 | hz12-owner-inbox-speed | 29309069.95 | `[HZ12_XOWNER_INBOX] producers=4 consumers=4 allocs=146878122 cross_owner_frees=146878122 owner_drained=146874104 waits=45726227 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=29309069.95 elapsed=5.011 peak_rss_mib=13.66 rss_mib=13.51 retired_owners=4 retired_pending_owners=0 retired_pending_objects=0 tracked_spans=0 full_capacity_spans=0 accounting_candidates=0 tracked_live=0 release_untracked=0 release_underflow=0` |
| 5 | 2 | tcmalloc | 36565336.45 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=183127838 cross_owner_frees=183127838 local_cleanup=0 waits=31202986 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=36565336.45 elapsed=5.008` |
| 5 | 3 | hz11-ownerless | 13040929.69 | `[XOWNER_PIPELINE] producers=4 consumers=4 ring=4096 allocs=65351608 cross_owner_frees=65351608 local_cleanup=0 waits=43763021 sharing_factor=2.00 cross_owner_rate=100.0 ops/s=13040929.69 elapsed=5.011` |
