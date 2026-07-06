# HZ10 Explicit Quiescent Orphan Purge L0

Median RUNS=3, HZ8 bench_matrix harness, HZ10 preload. `purge` calls `hz10_public_entry_purge_orphan_registry_quiescent(NULL)` before post_rss sampling.

| row | baseline post RSS | purge post RSS | RSS delta | baseline ops/s | purge ops/s | ops delta |
|---|---:|---:|---:|---:|---:|---:|
| small_interleaved_remote90 | 44957696 | 5722112 | -39235584 | 15172340.338 | 14829932.283 | -2.3% |
| main_interleaved_r90 | 95289344 | 6987776 | -88301568 | 12756277.400 | 12925799.882 | 1.3% |
| medium_interleaved_r50 | 73269248 | 14467072 | -58802176 | 19191525.329 | 19445337.782 | 1.3% |
| main_local0 | 35258368 | 3768320 | -31490048 | 116489241.418 | 120522077.535 | 3.5% |
| medium_local0 | 6553600 | 3092480 | -3461120 | 211469301.927 | 222438543.706 | 5.2% |

Verdict: see notes.md.
