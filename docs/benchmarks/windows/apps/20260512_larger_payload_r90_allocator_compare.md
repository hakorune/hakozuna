# Windows Large-Payload R90 Allocator Compare

Date: 2026-05-12

This note compares the `larger_payload_r90_4x16` lane across the four allocators
we care about most on Windows: `hz3`, `hz4`, `mimalloc`, and `tcmalloc`.

Throughput is taken from the observed progress line average (`avg: ... ops/sec`)
because that matches the live progress output and is the most consistent
cross-run signal in these logs.

Shape:

- client threads: `4`
- client connections: `16`
- ratio: `1:9`
- data size: `256`
- test time: `3` seconds
- key maximum: `8000`

Result:

| allocator | runs | ops/sec median | peak rss kb median | final rss kb median | steady rss kb median | private kb median | peak va kb median |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hz3 | 15 | 304,475.00 | 15,268 | 15,268 | 15,268 | 130,368 | 71,519,928 |
| hz4 | 15 | 301,099.00 | 13,220 | 13,220 | 13,220 | 21,300 | 4,322,228 |
| mimalloc | 5 | 290,778.00 | 12,792 | 12,792 | 12,792 | 40,996 | 5,353,252 |
| tcmalloc | 5 | 290,816.00 | 16,556 | 16,556 | 16,556 | 16,252 | 4,318,628 |

Interpretation:

- HZ3 is the fastest of the four on this lane, but it carries a very large virtual-memory footprint.
- HZ4 is slightly below HZ3 on throughput, but it is dramatically leaner on RSS / private bytes / VA.
- Mimalloc and tcmalloc are both a bit below the HZ pair on throughput here.
- Mimalloc is the lightest on peak RSS in this comparison, while tcmalloc has the smallest private / VA footprint among the non-HZ allocators.
- If the paper wants to emphasize footprint, HZ4 is the most interesting result on this lane.
- If the paper wants to emphasize raw speed, HZ3 still has the edge.

Related run notes:

- HZ3 / HZ4 repeat-5 lane: `docs/benchmarks/windows/apps/20260512_hz3_hz4_large_payload_r90_repeat5.md`
- Direct per-run summaries are local raw material and should live under `private/raw-results/windows/memcached/.../summaries`.
- The aggregate table above is the public source of truth for this comparison.

Notes:

- The memtier-driven external-client matrix wrapper still needs a bit more care for summary capture on some allocator lanes, so this note uses the direct runner summaries for mimalloc and tcmalloc.
- The direct runner summaries are the source of truth for the throughput medians here.
