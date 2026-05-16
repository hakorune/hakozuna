# Windows HZ3 vs HZ4 Large-Payload R90 Compare

Date: 2026-04-22

This note records a direct comparison on the "large payload + heavy handoff"
condition we wanted to re-check for HZ4.

Shape:

- client threads: `4`
- client connections: `16`
- ratio: `1:9`
- data size: `256`
- test time: `3` seconds
- key maximum: `8000`

Reproduction:

- Build variants: `win/build_win_memcached_allocator_variants.ps1`
- Run profile: `win/run_win_memcached_external_client_allocator_matrix.ps1 -ProfileName larger_payload_r90_4x16 -AllocatorName hz3,hz4`
- Executables are generated under `out_win_memcached_allocators/`.

Result:

| allocator | ops/sec | peak rss kb | private kb | peak va kb |
| --- | ---: | ---: | ---: | ---: |
| hz3 | 170,792 | 15,224 | 130,380 | 71,519,844 |
| hz4 | 171,761 | 13,148 | 21,296 | 4,322,144 |

Interpretation:

- throughput is essentially tied, with HZ4 slightly ahead on this run
- HZ4 is much leaner in both RSS and private/virtual memory on this condition
- this matches the "big payload + heavy handoff" memory behavior we wanted to re-check

Notes:

- this is a direct smoke comparison, not a full repeated matrix
- the large-payload + R90 lane was added to the allocator matrix scripts for follow-up runs
