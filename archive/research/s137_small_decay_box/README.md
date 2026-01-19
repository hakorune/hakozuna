# S136/S137 HotSC Decay Boxes (ARCHIVED / NO-GO)

## Purpose
- Reduce partial pages for hot small size classes (sc=9..13) in dist_app.
- S136: hot sc only bin_target clamp to refill_batch (event-only, S58 adjust).
- S137: hot sc only small bin trim at epoch boundary (event-only).

## Changes (archived)
- S136: `hz3_tcache_decay.c` adjusted `bin_target` for hot SC range.
- S137: `hz3_epoch.c` trimmed small bins for hot SC range.
- Both are now **stubbed** in trunk and archived.

## GO Criteria
- RSS mean/p95 drops (>= 5%) on dist_app (max=32768/2048) with <=2% ops/s loss.

## Result (NO-GO)
- S58 `adjust_calls=0` on all runs (epoch not firing).
- With epoch forcing (`HZ3_S134_EPOCH_ON_SMALL_SLOW=1`) RSS still did not move meaningfully.
- net: no consistent RSS reduction; effect not stable.

## Logs
- S137 OFF:
  - `/tmp/dist_app_s137_off_32768.csv` (mean_kb=14028.8, p95_kb=16128)
  - `/tmp/dist_app_s137_off_2048.csv` (mean_kb=13952, p95_kb=16000)
- S137 ON:
  - `/tmp/dist_app_s137_on_32768.csv` (mean_kb=14259.2, p95_kb=16256)
  - `/tmp/dist_app_s137_on_2048.csv` (mean_kb=13747.2, p95_kb=15872)
- Epoch forced (S134):
  - `/tmp/dist_app_s137_epoch_32768.csv` (mean_kb=13798.4, p95_kb=15872)
  - `/tmp/dist_app_s137_epoch_2048.csv` (mean_kb=13619.2, p95_kb=15872)

## Notes
- Archive includes both S136 and S137 because they are coupled (hot SC decay on epoch).
- Next: move to S135-1C (full pages / tail waste top5) for fragmentation triage.
