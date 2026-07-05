# HZ10 Scripts

Keep scripts narrow and reproducible.

```text
check_hz10_standalone.sh:
  standalone/source hygiene gate

run_hz10_rss_guard.sh:
  short lifecycle-flush RSS guard; checks current_rss_kb plus busy reclaim
  counters for main_r50, main_r90, and slot_count1_r90
```
