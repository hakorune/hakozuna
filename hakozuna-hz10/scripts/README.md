# HZ10 Scripts

Keep scripts narrow and reproducible.

```text
check_hz10_standalone.sh:
  standalone/source hygiene gate

run_hz10_rss_guard.sh:
  short lifecycle-flush RSS guard; checks current_rss_kb plus busy reclaim
  counters for main_r50, main_r90, and slot_count1_r90

run_hz10_larson_thread_churn_attribution.sh:
  macro attribution runner for HZ10LarsonThreadChurnAttribution-L0; sweeps
  small larson shapes and records sampled current RSS plus per-thread
  `hz10_shim_exit_stats` lines under HZ10_SHIM_THREAD_EXIT_STATS=1
```
