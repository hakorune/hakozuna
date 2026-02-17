# HZ4 Knob Quick Reference

最終更新: 2026-02-16

このページは `hz4` の実験/観測でよく使う knob の早見表です。  
厳密な定義は config ヘッダを正とします。

## Source of Truth

- `hakozuna/hz4/core/hz4_config_core.h`
- `hakozuna/hz4/core/hz4_config_collect.h`
- `hakozuna/hz4/core/hz4_config_remote.h`
- `hakozuna/hz4/core/hz4_config_archived.h`
- `hakozuna/hz4/docs/HZ4_GO_BOXES.md`
- `hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md`
- `hakozuna/hz4/docs/HZ4_STATUS_SHORT.md`
- `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_SUMMARY.md`
- `CURRENT_TASK.md`

## Frequently Used Mid Knobs

| Knob | Default | Purpose | Source |
|---|---:|---|---|
| `HZ4_MID_PAGE_SUPPLY_RESV_BOX` | `1` | B70: page create 時の seg lock 取得をページ予約で償却 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES` | `8` | B70: lock 1回あたりの予約ページ数 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_OWNER_REMOTE_QUEUE_BOX` | `0` | owner remote queue（mid owner-path）を有効化 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_OWNER_LOCAL_STACK_BOX` | `0` | owner-local stack（B57 系）を有効化 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_OWNER_LOCAL_STACK_SLOTS` | `16` | owner-local stack の depth | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE` | `0` | B58: remote pressure あり page を stack から除外 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_STATS_B1` | `0` | mid one-shot counters を有効化 | `hakozuna/hz4/core/hz4_config_collect.h` |
| `HZ4_MID_LOCK_TIME_STATS` | `0` | lock wait/hold の時系列カウンタを有効化 | `hakozuna/hz4/core/hz4_config_collect.h` |

## Archived Safety

- `HZ4_ALLOW_ARCHIVED_BOXES=0` が default です。
- archived knob を有効化すると `hakozuna/hz4/core/hz4_config_archived.h` で `#error` 停止します。
- archived box の一覧と復活手順は `hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md` を参照してください。

## Common Build Patterns

- Base build:
  - `make -C hakozuna/hz4 clean all`
- Mid observation build:
  - `make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA='-DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'`
- Phase22 style baseline (non-default experiment profile):
  - `make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'`
