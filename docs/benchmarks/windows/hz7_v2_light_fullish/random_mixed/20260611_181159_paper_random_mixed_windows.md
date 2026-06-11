# Paper-Aligned Windows Random Mixed

Generated: 2026-06-11 18:11:59 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=1`, `ITERS=20,000,000`, `WS=400`
- selected allocators: `crt, hz3, hz4, hz5-policy, hz7-tinyroute, hz7-v2, hz6-strict, hz6-speed, hz6-rss, hz6-strict-broad, hz6-speed-broad, hz6-rss-broad, hz6-strict-control, hz6-speed-control, hz6-rss-control, hz6-strict-route4k, hz6-speed-route4k, hz6-rss-route4k, hz6-strict-appcap, hz6-speed-appcap, hz6-rss-appcap, mimalloc, tcmalloc`
- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`; `>16KiB` uses direct OS regions with bounded 32K/64K direct retain buckets, and it is not an interposer/general allocator row yet.
- `hz7-v2` is the TinyRoute v2 row: it keeps the direct-API/global-lock safety model while testing the v2 task-track changes such as remote-safe smoke and SlowPathOutsideLock.
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 35.687M | 5,420 | `35.687M / 5,420 KB` |
| hz3 | 148.763M | 6,736 | `148.763M / 6,736 KB` |
| hz4 | 115.705M | 12,120 | `115.705M / 12,120 KB` |
| hz5-policy | 29.831M | 5,772 | `29.831M / 5,772 KB` |
| hz7-tinyroute | 73.211M | 5,016 | `73.211M / 5,016 KB` |
| hz7-v2 | 66.550M | 4,576 | `66.550M / 4,576 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 27.431M | 7,840 | `27.431M / 7,840 KB` |
| hz6-speed-broad | 20.275M | 7,840 | `20.275M / 7,840 KB` |
| hz6-rss-broad | 22.092M | 7,840 | `22.092M / 7,840 KB` |
| hz6-strict-control | 26.129M | 4,880 | `26.129M / 4,880 KB` |
| hz6-speed-control | 21.534M | 4,888 | `21.534M / 4,888 KB` |
| hz6-rss-control | 22.997M | 4,876 | `22.997M / 4,876 KB` |
| hz6-strict-route4k | 30.869M | 5,104 | `30.869M / 5,104 KB` |
| hz6-speed-route4k | 24.594M | 5,096 | `24.594M / 5,096 KB` |
| hz6-rss-route4k | 27.245M | 5,104 | `27.245M / 5,104 KB` |
| hz6-strict-appcap | 30.954M | 235,140 | `30.954M / 235,140 KB` |
| hz6-speed-appcap | 24.441M | 235,140 | `24.441M / 235,140 KB` |
| hz6-rss-appcap | 26.991M | 235,144 | `26.991M / 235,144 KB` |
| mimalloc | 93.371M | 6,064 | `93.371M / 6,064 KB` |
| tcmalloc | 118.444M | 9,012 | `118.444M / 9,012 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 6.317M | 9,968 | `6.317M / 9,968 KB` |
| hz3 | 150.228M | 6,440 | `150.228M / 6,440 KB` |
| hz4 | 80.038M | 9,796 | `80.038M / 9,796 KB` |
| hz5-policy | 5.891M | 10,140 | `5.891M / 10,140 KB` |
| hz7-tinyroute | 9.556M | 6,680 | `9.556M / 6,680 KB` |
| hz7-v2 | 51.330M | 5,136 | `51.330M / 5,136 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 33.468M | 7,380 | `33.468M / 7,380 KB` |
| hz6-speed-broad | 28.654M | 7,384 | `28.654M / 7,384 KB` |
| hz6-rss-broad | 31.704M | 7,376 | `31.704M / 7,376 KB` |
| hz6-strict-control | 28.916M | 4,420 | `28.916M / 4,420 KB` |
| hz6-speed-control | 25.845M | 4,428 | `25.845M / 4,428 KB` |
| hz6-rss-control | 27.293M | 4,424 | `27.293M / 4,424 KB` |
| hz6-strict-route4k | 33.628M | 4,652 | `33.628M / 4,652 KB` |
| hz6-speed-route4k | 28.841M | 4,652 | `28.841M / 4,652 KB` |
| hz6-rss-route4k | 31.008M | 4,644 | `31.008M / 4,644 KB` |
| hz6-strict-appcap | 34.056M | 234,688 | `34.056M / 234,688 KB` |
| hz6-speed-appcap | 28.569M | 234,688 | `28.569M / 234,688 KB` |
| hz6-rss-appcap | 31.029M | 234,680 | `31.029M / 234,680 KB` |
| mimalloc | 85.974M | 11,520 | `85.974M / 11,520 KB` |
| tcmalloc | 149.021M | 10,288 | `149.021M / 10,288 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.795M | 11,304 | `7.795M / 11,304 KB` |
| hz3 | 138.275M | 7,448 | `138.275M / 7,448 KB` |
| hz4 | 75.667M | 18,264 | `75.667M / 18,264 KB` |
| hz5-policy | 6.151M | 11,304 | `6.151M / 11,304 KB` |
| hz7-tinyroute | 10.490M | 7,040 | `10.490M / 7,040 KB` |
| hz7-v2 | 49.748M | 5,656 | `49.748M / 5,656 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 31.478M | 7,380 | `31.478M / 7,380 KB` |
| hz6-speed-broad | 27.457M | 7,368 | `27.457M / 7,368 KB` |
| hz6-rss-broad | 30.175M | 7,380 | `30.175M / 7,380 KB` |
| hz6-strict-control | 27.152M | 4,424 | `27.152M / 4,424 KB` |
| hz6-speed-control | 23.165M | 4,424 | `23.165M / 4,424 KB` |
| hz6-rss-control | 25.288M | 4,424 | `25.288M / 4,424 KB` |
| hz6-strict-route4k | 31.420M | 4,648 | `31.420M / 4,648 KB` |
| hz6-speed-route4k | 26.937M | 4,648 | `26.937M / 4,648 KB` |
| hz6-rss-route4k | 29.818M | 4,656 | `29.818M / 4,656 KB` |
| hz6-strict-appcap | 31.528M | 234,688 | `31.528M / 234,688 KB` |
| hz6-speed-appcap | 26.611M | 234,684 | `26.611M / 234,684 KB` |
| hz6-rss-appcap | 29.586M | 234,680 | `29.586M / 234,680 KB` |
| mimalloc | 83.449M | 11,776 | `83.449M / 11,776 KB` |
| tcmalloc | 144.315M | 10,896 | `144.315M / 10,896 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)
