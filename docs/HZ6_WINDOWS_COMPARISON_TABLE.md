# HZ6 Windows Comparison Table

This note summarizes the current Windows HZ6-only benchmark run and the closest
existing Windows reference lanes in the repository. The numbers are useful for
orientation, but they are not yet a single apples-to-apples cross-family matrix.
For repeatable report generation, use the shared helper at
[`win/write_benchmark_summary_table.ps1`](../../../win/write_benchmark_summary_table.ps1)
to render the HZ6 `results.tsv` into the same markdown shape used here.

## HZ6 Windows Full Benchmark

Source:

- [HZ6 Windows benchmark results](../private/allocators/hakozuna/hakozuna-hz6/private/raw-results/windows/hz6_benchmark_20260528_091244/results.tsv)
- [HZ6 Windows benchmark summary](../private/allocators/hakozuna/hakozuna-hz6/private/raw-results/windows/hz6_benchmark_20260528_091244/summary.tsv)

```text
mode    profile   size    median ops/s    median RSS
local   strict    8192    57.74M          3.7 MiB
local   rss       8192    51.28M          3.7 MiB
local   speed     8192    45.12M          3.7 MiB
local   remote    8192    44.61M          3.7 MiB

local   strict    65536   67.08M          3.7 MiB
local   rss       65536   55.44M          3.7 MiB
local   remote    65536   47.12M          3.7 MiB
local   speed     65536   46.75M          3.7 MiB

remote  remote    131072  62.73M          NA
remote  rss       131072  63.88M          NA
remote  speed     131072  60.14M          4.2 MiB

reuse   remote    131072  62.23M          NA
reuse   rss       131072  63.17M          NA
reuse   speed     131072  63.00M          NA
```

## Closest Windows Reference Lanes

Source:

- [HZ5 Windows Local2P-W2](hakozuna-hz5-windows-local2p-w2.md)

```text
lane                                       metric(s)
hz5-win-local2p-remotefull-slots4          remote-r90 6.46M, remote-r99 5.06M, RSS 77.43 MiB
hz5-win-local2p-remotefull-slots4-rss256   remote-r90 5.70M, remote-r99 5.76M, RSS 78.07 MiB
hz5-win-local2p-remotefull-slots4-sidecarfast
                                           local 11.66M, remote-r90 6.10M, remote-r99 5.23M, RSS 74.55 MiB
hz5-win-local2p-remotefull-slots4-sidecarfast-rss256
                                           control lane, same family; see source doc for exact trace
tcmalloc                                   remote-r90 1.78M, remote-r99 1.41M, RSS 76.54 MiB
```

## Read

```text
HZ6 Windows:
  good internal separation between strict / rss / speed / remote
  local 64K remains the strongest HZ6 local point in this run
  remote/reuse are stable and above 60M ops/s in this harness

HZ5 Windows references:
  sidecarfast is the practical control lane in the current Local2P family
  slots4 is still a useful control/reference lane
  tcmalloc remains the external baseline in that HZ5 Windows family
```

## Next Step

To make this a real apples-to-apples comparison, the next runner should drive
the same Windows benchmark shape for:

```text
crt
hz3
hz4
mimalloc
tcmalloc
hz5
hz6
```

with the same mode/profile/size matrix and a shared results summary.
