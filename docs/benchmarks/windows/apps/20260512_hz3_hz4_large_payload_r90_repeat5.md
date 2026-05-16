# Windows HZ3 vs HZ4 Large-Payload R90 Repeat-5 Compare

Date: 2026-05-12

This note records the repeat-5 matrix run on the "large payload + heavy handoff"
condition that we wanted to re-check for HZ4.

Shape:

- client threads: `4`
- client connections: `16`
- ratio: `1:9`
- data size: `256`
- test time: `3` seconds
- key maximum: `8000`

Reproduction:

- Build variants: `win/build_win_memcached_allocator_variants.ps1`
- Run profile: `win/run_win_memcached_external_client_allocator_matrix.ps1 -ProfileName larger_payload_r90_4x16 -AllocatorName hz3,hz4 -Runs 5`
- Executables are generated under `out_win_memcached_allocators/`.

Result:

| allocator | ops/sec median | peak rss kb median | private kb median | peak va kb median |
| --- | ---: | ---: | ---: | ---: |
| hz3 | 304,579.35 | 15,272 | 130,372 | 71,519,928 |
| hz4 | 293,095.18 | 13,204 | 21,288 | 4,322,228 |

Interpretation:

- the throughput gap is small enough that this lane is effectively a wash on speed
- HZ4 is still much leaner in RSS, private bytes, and virtual address footprint
- this is a stronger version of the "big payload + heavy handoff" behavior we wanted to isolate
- HZ3 remains competitive on throughput, so the choice between HZ3 and HZ4 depends on whether the workload cares more about speed or footprint

Run detail:

- `hz3` ops/sec: `[465,106.65, 304,579.35, 445,073.17, 209,607.84, 304,515.49]`
- `hz4` ops/sec: `[301,139.09, 293,095.18, 324,112.43, 291,258.90, 225,071.09]`

Notes:

- this repeat-5 run updates the earlier direct smoke baseline from `20260422_hz4_large_payload_r90_compare.md`
- the allocator matrix script now accepts profile and allocator filters, so this lane can be re-run without executing the full matrix
