# HZ4 Status Short

最終更新: 2026-02-16

このファイルは `hz4` の現況を短く確認するための入口です。  
詳細は SSOT と各 phase ドキュメントを参照してください。

## Current Snapshot

- 採用状態（SSOT）: `CURRENT_TASK.md`
- GO 一覧: `hakozuna/hz4/docs/HZ4_GO_BOXES.md`
- Archived 一覧: `hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md`
- knob 早見表: `hakozuna/hz4/docs/HZ4_KNOB_QUICK_REFERENCE.md`

## Key Defaults (high impact)

- `HZ4_MID_PAGE_SUPPLY_RESV_BOX=1`（B70）
- `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=8`（B70）
- `HZ4_ST_FREE_USEDDEC_RELAXED=1`（B33）
- `HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX=auto`（B40）

## Typical Experiment Profile (non-default)

- `HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1`
- `HZ4_MID_OWNER_LOCAL_STACK_BOX=1`
- `HZ4_MID_OWNER_LOCAL_STACK_SLOTS=8`
- `HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1`

## Recent NO-GO Focus

- `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_SUMMARY.md`
- `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_B71_B72.md`
- `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_B73_B75.md`

## Latest Bench Artifacts

直近の観測結果はここを更新して共有する（古い行は1つだけ残して入れ替え）。

| Item | Path |
|---|---|
| `last_outdir` | `/tmp/<replace_with_latest_outdir>/` |
| `last_report` | `/tmp/<replace_with_latest_outdir>/REPORT.md` |
| `last_summary` | `/tmp/<replace_with_latest_outdir>/summary.tsv` |
| `last_raw` | `/tmp/<replace_with_latest_outdir>/raw.tsv` |
| `last_perf` | `/tmp/<replace_with_latest_outdir>/perf/` |

## Notes

- archived knob を有効化する場合は `HZ4_ALLOW_ARCHIVED_BOXES=1` が必要。
- bench/AB 実行前に `GO` と `Archived` の両方を確認して lane 混線を避ける。
