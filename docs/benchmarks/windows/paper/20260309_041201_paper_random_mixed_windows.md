# Paper-Aligned Windows Random Mixed

Generated: 2026-03-09 04:12:01 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=10`, `ITERS=20,000,000`, `WS=400`

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `10`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 45.331M | 5,484 | `44.998M / 5,484 KB, 45.114M / 5,484 KB, 45.074M / 5,488 KB, 45.765M / 5,484 KB, 45.524M / 5,488 KB, 45.714M / 5,480 KB, 45.137M / 5,480 KB, 45.749M / 5,484 KB, 45.759M / 5,480 KB, 45.111M / 5,484 KB` |
| hz3 | 151.101M | 6,200 | `151.208M / 6,200 KB, 151.517M / 6,200 KB, 151.529M / 6,208 KB, 151.915M / 6,200 KB, 151.571M / 6,200 KB, 149.078M / 6,208 KB, 150.994M / 6,196 KB, 150.379M / 6,208 KB, 150.995M / 6,200 KB, 146.719M / 6,208 KB` |
| hz4 | 134.253M | 12,608 | `134.931M / 12,608 KB, 133.645M / 12,608 KB, 134.171M / 12,612 KB, 134.669M / 12,608 KB, 134.198M / 12,604 KB, 134.308M / 12,612 KB, 134.813M / 12,612 KB, 134.169M / 12,608 KB, 134.829M / 12,608 KB, 130.294M / 12,608 KB` |
| mimalloc | 98.553M | 6,668 | `98.932M / 6,676 KB, 98.628M / 6,664 KB, 98.841M / 6,660 KB, 98.752M / 6,680 KB, 98.478M / 6,668 KB, 98.102M / 6,676 KB, 98.423M / 6,668 KB, 98.260M / 6,668 KB, 98.749M / 6,668 KB, 98.255M / 6,668 KB` |
| tcmalloc | 120.308M | 9,524 | `120.561M / 9,524 KB, 119.729M / 9,528 KB, 119.385M / 9,532 KB, 120.356M / 9,520 KB, 120.261M / 9,528 KB, 118.969M / 9,520 KB, 120.188M / 9,524 KB, 120.487M / 9,524 KB, 120.579M / 9,524 KB, 120.674M / 9,524 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `10`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 8.191M | 11,000 | `8.310M / 11,004 KB, 8.178M / 10,996 KB, 8.061M / 11,000 KB, 7.339M / 10,796 KB, 8.204M / 11,000 KB, 7.542M / 10,792 KB, 8.261M / 11,004 KB, 8.503M / 11,000 KB, 8.502M / 11,000 KB, 7.783M / 10,792 KB` |
| hz3 | 151.737M | 6,140 | `151.576M / 6,140 KB, 151.034M / 6,140 KB, 152.126M / 6,140 KB, 152.101M / 6,140 KB, 150.657M / 6,140 KB, 151.815M / 6,140 KB, 151.673M / 6,144 KB, 152.306M / 6,148 KB, 151.801M / 6,140 KB, 150.346M / 6,144 KB` |
| hz4 | 91.705M | 10,228 | `91.820M / 10,228 KB, 91.913M / 10,228 KB, 91.733M / 10,232 KB, 91.371M / 10,228 KB, 91.677M / 10,228 KB, 92.412M / 10,228 KB, 91.959M / 10,228 KB, 91.051M / 10,228 KB, 91.284M / 10,232 KB, 87.420M / 10,232 KB` |
| mimalloc | 87.207M | 12,166 | `86.801M / 12,228 KB, 87.389M / 12,200 KB, 87.219M / 11,076 KB, 86.948M / 12,224 KB, 87.077M / 12,132 KB, 87.697M / 12,340 KB, 87.196M / 11,976 KB, 87.526M / 12,272 KB, 86.914M / 12,028 KB, 87.553M / 12,012 KB` |
| tcmalloc | 149.698M | 10,804 | `149.608M / 10,792 KB, 150.057M / 10,812 KB, 150.325M / 10,812 KB, 149.691M / 10,804 KB, 149.401M / 10,796 KB, 149.497M / 10,804 KB, 149.298M / 10,804 KB, 150.212M / 10,800 KB, 149.704M / 10,808 KB, 150.715M / 10,792 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `10`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.477M | 11,704 | `7.463M / 11,940 KB, 7.514M / 11,700 KB, 7.642M / 11,700 KB, 7.442M / 11,708 KB, 7.443M / 11,712 KB, 7.401M / 11,700 KB, 7.498M / 11,704 KB, 7.416M / 11,708 KB, 7.490M / 11,704 KB, 7.576M / 11,620 KB` |
| hz3 | 146.284M | 6,908 | `146.322M / 6,904 KB, 146.246M / 6,908 KB, 144.792M / 6,916 KB, 146.550M / 6,908 KB, 146.966M / 6,908 KB, 145.588M / 6,908 KB, 146.054M / 6,908 KB, 145.370M / 6,908 KB, 146.965M / 6,900 KB, 146.682M / 6,912 KB` |
| hz4 | 85.948M | 18,668 | `85.933M / 18,668 KB, 85.920M / 18,668 KB, 86.584M / 18,668 KB, 85.962M / 18,668 KB, 86.711M / 18,668 KB, 86.408M / 18,668 KB, 86.261M / 18,668 KB, 84.953M / 18,668 KB, 85.647M / 18,672 KB, 85.305M / 18,668 KB` |
| mimalloc | 87.313M | 12,846 | `87.893M / 12,860 KB, 87.390M / 12,784 KB, 86.406M / 12,768 KB, 87.047M / 12,776 KB, 86.906M / 12,996 KB, 87.839M / 12,832 KB, 86.854M / 12,980 KB, 87.697M / 12,872 KB, 87.468M / 13,128 KB, 87.236M / 12,692 KB` |
| tcmalloc | 147.307M | 11,404 | `147.245M / 11,400 KB, 147.756M / 11,400 KB, 146.778M / 11,404 KB, 147.109M / 11,416 KB, 147.867M / 11,408 KB, 147.095M / 11,404 KB, 147.887M / 11,404 KB, 147.369M / 11,408 KB, 148.202M / 11,400 KB, 146.118M / 11,412 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)

