# HZ10 HZ8 Public Purge Matrix L0

Median results. `purge` calls `hz10_public_entry_purge_orphan_registry_quiescent(NULL)` before post_rss sampling.

| row | baseline post RSS | purge post RSS | RSS delta | baseline ops/s | purge ops/s | ops delta |
|---|---:|---:|---:|---:|---:|---:|
| small_interleaved_remote90 | 41811968 | 5849088 | -35962880 | 15025471.367 | 15049676.442 | 0.2% |
| main_interleaved_r90 | 99090432 | 8220672 | -90869760 | 12590964.010 | 12446156.759 | -1.2% |
| medium_interleaved_r50 | 70123520 | 7376896 | -62746624 | 19870969.844 | 21123700.602 | 6.3% |
| main_local0 | 35258368 | 3989504 | -31268864 | 101070602.996 | 116330167.117 | 15.1% |
| medium_local0 | 6422528 | 3182592 | -3239936 | 221266718.775 | 199992500.281 | -9.6% |

Verdict: formal lane for the explicit quiescent purge RSS gate.

## Method

The baseline and purge binaries are both built from the HZ8 public `bench_matrix_malloc.c` source.
The purge binary is generated under OUTDIR with one extra dlsym call at the post_rss sample point.
HZ8 source files are not modified by this lane.
