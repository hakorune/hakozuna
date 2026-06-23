# UsedCount Cold Derivation Reverify 20260623T143713Z

HEAD: 2b07bcb

Scope:

- confirmed UsedCountColdDerivationShadow-L1 / UsedCountReleaseElision-L1 / UsedCountFieldRemoval-L1 are already present in current HEAD
- ran debug smoke, safety stress, and debug interleaved remote90 R3

Commands:

- make smoke safety-stress bench
- ./h8_smoke
- ./h8_safety_stress
- ./h8_bench --runs 3 --threads 16 --iters 50000 --min-size 16 --max-size 2048 --remote-pct 90 --interleaved 1 --live-window 4096

Key results:

- smoke: pass
- safety stress: pass
- debug interleaved remote90 median: 6.66M ops/s
- used_count_cold owner_exit=799 verify_quiescent=109425 derived_mismatch=0 derived_scan=110224
- slot_shadow mismatch counters: all 0
- quiescent_pending bitmap_nonzero=0 repair=0
- remote duplicate_claim=0 validate_fail=0 invalid=0

Raw logs:

- bench_results/20260623T143713Z_used_count_smoke.log
- bench_results/20260623T143713Z_used_count_safety.log
- bench_results/20260623T143713Z_used_count_debug_interleaved_r3.log

Interpretation:

- used_count remains closed in the current code shape.
- release cold decisions derive from slot_state; debug mirror/counters are proof lineage only.
- no new implementation was needed for this box because the current HEAD already contains the completed lane.
