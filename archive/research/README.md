# hz3 archive/research（研究箱の置き場）

ここは **NO-GO / 実験 / 一時的な枝**を “本線を汚さずに” 保存するためのアーカイブです。

## ルール（最重要）

- 本線（`hakozuna/hz3/src`）に **研究用の分岐や重い層を残さない**。
  - 例: transfer cache / slow-start / 動的チューニング等で NO-GO になったものは、必ず revert してからここへ。
- 研究箱は **「コード + ドキュメント + SSOTログ」**をセットで残す。
- “導線” は SSOT を基準に固定する（取り違え禁止）。

## フォルダ構造

`hakozuna/hz3/archive/research/<topic>/`
- `README.md`（必須）
  - 目的 / 変更点 / GO条件 / 結果 / SSOTログ / 失敗理由 / 復活条件
  - 可能なら、比較対象ログ（baseline と experiment）を両方書く

## 索引

- hz3 NO-GO summary: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`

## 入口（迷ったら）

- 研究箱一覧（READMEだけ）:
  - `find hakozuna/hz3/archive/research -maxdepth 2 -name README.md | sort`
- 最近触ったもの（mtime順）:
  - `ls -lt hakozuna/hz3/archive/research | head`

## 最近追加（例）

- `s89_remote_stash_microagg` / `s90_owner_stash_stripes` / `s91_owner_stash_spill_prefetch`
- `s98_1_push_remote_microopt`
