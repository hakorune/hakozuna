# S17: PTAG dst/bin TLS snapshot (NO-GO)

目的:
- S17（PTAG dst/bin direct）の hot free をさらに詰めるため、arena base / PTAG32 base を TLS にスナップショットして
  hot path から atomic load を避ける（range check を最短化）。

変更内容:
- `HZ3_PTAG_DSTBIN_TLS=1` を追加し、`hz3_free` の PTAG dst/bin 経路で
  - `t_hz3_cache.arena_base`
  - `t_hz3_cache.page_tag32`
  を使う fast path を導入。

SSOT（RUNS=10）:
- TLS OFF（baseline）ログ: `/tmp/hz3_ssot_6cf087fc6_20260101_170646`
  - small: 97.88M
  - medium: 95.94M
  - mixed: 92.51M
- TLS ON（`HZ3_PTAG_DSTBIN_TLS=1`）ログ: `/tmp/hz3_ssot_6cf087fc6_20260101_170741`
  - small: 88.09M (-10.0%)
  - medium: 94.18M (-1.8%)
  - mixed: 95.31M (+3.0%)

判定: NO-GO
- mixed は伸びるが、small の -10% が致命的で総合では採用不可。

復活条件:
- small の退行が解消できる形（TLS構造肥大/追加ロードが消える、または “mixed専用ビルド” として別プロファイル運用）
  が用意できた場合に再検討。

